/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    anjuta-docman.c
    Copyright (C) 2003  Naba Kumar <naba@gnome.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <libanjuta/pixmaps.h>
#include <libanjuta/resources.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-preferences.h>

#include <gtk/gtkfilechooserdialog.h>
#include <libgnomevfs/gnome-vfs.h>

#include "text_editor.h"
#include "anjuta-docman.h"
#include "file_history.h"
#include "plugin.h"
#include "action-callbacks.h"

gboolean closing_state;

struct _AnjutaDocmanPriv {
	AnjutaPreferences *preferences;
	TextEditor *current_editor;
	GtkWidget *fileselection;
	GtkWidget *save_as_fileselection;
	GList *editors;
};

static void anjuta_docman_order_tabs(AnjutaDocman *docman);
static void anjuta_docman_update_page_label (AnjutaDocman *docman, GtkWidget *te);
static void anjuta_docman_grab_text_focus (AnjutaDocman *docman);
static void on_notebook_switch_page (GtkNotebook * notebook,
									 GtkNotebookPage * page,
									 gint page_num, AnjutaDocman *docman);

struct _AnjutaDocmanPage {
	GtkWidget *widget;
	GtkWidget *close_image;
	GtkWidget *close_button;
	GtkWidget *label;
	GtkWidget *box;
};

typedef struct _AnjutaDocmanPage AnjutaDocmanPage;

static void
on_text_editor_notebook_close_page (GtkButton* button, 
									AnjutaDocman* docman)
{
	/* This is a hack because we cannot get the real
	Editor plugin */
	EditorPlugin* dummy_plugin = g_new0(EditorPlugin, 1);
	dummy_plugin->docman = GTK_WIDGET(docman);

	on_close_file1_activate (NULL, dummy_plugin);
	
	g_free(dummy_plugin);
}

static void
editor_tab_widget_destroy (AnjutaDocmanPage* page)
{
	g_return_if_fail(page != NULL);
	g_return_if_fail(page->close_button != NULL);

	g_object_unref (G_OBJECT (page->close_button));
	g_object_unref (G_OBJECT (page->close_image));
	g_object_unref (G_OBJECT (page->label));
	
	page->close_image = NULL;
	page->close_button = NULL;
	page->label = NULL;
}

static GtkWidget*
editor_tab_widget_new(AnjutaDocmanPage* page, AnjutaDocman* docman)
{
	GtkWidget *button15;
	GtkWidget *close_pixmap;
	GtkWidget *tmp_toolbar_icon;
	GtkWidget *label;
	GtkWidget *box;
	int h, w;
	GdkColor color;
	
	g_return_val_if_fail(GTK_IS_WIDGET (page->widget), NULL);
	
	if (page->close_image)
		editor_tab_widget_destroy (page);
	
	gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);

	tmp_toolbar_icon = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	gtk_widget_show(tmp_toolbar_icon);
	
	button15 = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button15), tmp_toolbar_icon);
	gtk_button_set_relief(GTK_BUTTON(button15), GTK_RELIEF_NONE);
	gtk_widget_set_size_request (button15, w, h);
	
	close_pixmap = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	gtk_widget_set_size_request(close_pixmap, w,h);
	gtk_widget_set_sensitive(close_pixmap, FALSE);
	
	label = gtk_label_new (TEXT_EDITOR (page->widget)->filename);
	gtk_widget_show (label);
	
	color.red = 0;
	color.green = 0;
	color.blue = 0;
	
	gtk_widget_modify_fg (button15, GTK_STATE_NORMAL, &color);
	gtk_widget_modify_fg (button15, GTK_STATE_INSENSITIVE, &color);
	gtk_widget_modify_fg (button15, GTK_STATE_ACTIVE, &color);
	gtk_widget_modify_fg (button15, GTK_STATE_PRELIGHT, &color);
	gtk_widget_modify_fg (button15, GTK_STATE_SELECTED, &color);
	gtk_widget_show(button15);
	
	box = gtk_hbox_new(FALSE, 2);
	gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), button15, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), close_pixmap, FALSE, FALSE, 0);
	
	gtk_widget_show(box);

	gtk_signal_connect (GTK_OBJECT (button15), "clicked",
				GTK_SIGNAL_FUNC(on_text_editor_notebook_close_page),
				docman);

	page->close_image = close_pixmap;
	page->close_button = button15;
	page->label = label;
	
	g_object_ref (page->close_button);
	g_object_ref (page->close_image);
	g_object_ref (page->label);

	return box;
}

static AnjutaDocmanPage *
anjuta_docman_page_new (GtkWidget *editor, AnjutaDocman* docman)
{
	AnjutaDocmanPage *page;
	// gchar *lab;
	
	page = g_new0 (AnjutaDocmanPage, 1);
	page->widget = GTK_WIDGET (editor);
	g_object_ref (G_OBJECT (page->widget));
	page->box = editor_tab_widget_new (page, docman);
	return page;
}

static void
anjuta_docman_page_destroy (AnjutaDocmanPage *page)
{
	editor_tab_widget_destroy (page);
	g_object_unref (G_OBJECT (page->widget));
	g_free (page);
}

void
anjuta_docman_open_file (AnjutaDocman *docman)
{
	gtk_widget_show (docman->priv->fileselection);
}

void
anjuta_docman_save_as_file (AnjutaDocman *docman)
{
	TextEditor *te;

	te = anjuta_docman_get_current_editor (docman);
	if (te == NULL)
		return;
	gtk_file_chooser_set_uri (GTK_FILE_CHOOSER(docman->priv->save_as_fileselection),
								te->uri);
	gtk_widget_show (docman->priv->save_as_fileselection);
}

static void
on_open_filesel_ok_clicked (GtkDialog* dialog, gint id, AnjutaDocman *docman)
{
	gchar *uri;
	gchar *entry_filename = NULL;
	int i;
	GSList * list;
	int elements;

	if (id != GTK_RESPONSE_ACCEPT)
	{
		gtk_widget_hide (docman->priv->save_as_fileselection);
		return;
	}
	
	list = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(dialog));
	elements = g_slist_length(list);
	for(i=0;i<elements;i++)
	{
		uri = g_strdup(g_slist_nth_data(list,i));
		if (!uri)
			return;
		anjuta_docman_goto_file_line (docman, uri, -1);
		g_free (uri);
	}

	if (entry_filename)
	{
		g_slist_remove(list, entry_filename);
		g_free(entry_filename);
	}
}

static void
save_as_real (AnjutaDocman *docman)
{
	TextEditor *te;
	gchar *uri, *saved_filename, *saved_uri, *filename;
	/*gint page_num;
	GtkWidget *child;*/
	gint status;
	uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER(docman->priv->save_as_fileselection));
	if (!uri)
		return;

	te = anjuta_docman_get_current_editor (docman);
	g_return_if_fail (te != NULL);

	saved_filename = te->filename;
	saved_uri = te->uri;
	
	filename = g_filename_from_uri(uri, NULL, NULL);
	te->uri = g_strdup (uri);
	te->filename = g_path_get_basename(filename); 
	status = text_editor_save_file (te, TRUE);
	gtk_widget_hide (docman->priv->save_as_fileselection);
	if (status == FALSE)
	{
		g_free (te->filename);
		te->filename = saved_filename;
		g_free (te->uri);
		te->uri = saved_uri;
		if (closing_state) closing_state = FALSE;
		return;
	} else {
		if (saved_filename)
			g_free (saved_filename);
		if (saved_uri)
		{
			g_free (saved_uri);
		}
		if (closing_state)
		{
			anjuta_docman_remove_editor (docman, NULL);
			closing_state = FALSE;
		}
		else
		{
			text_editor_set_hilite_type(te);
		}
		// anjuta_update_title ();
	}
}

static void
on_save_as_filesel_ok_clicked (GtkDialog* dialog, gint id, AnjutaDocman *docman)
{
	gchar* uri;
	GnomeVFSURI* vfs_uri;
	
	if (id != GTK_RESPONSE_ACCEPT)
	{
		gtk_widget_hide (docman->priv->save_as_fileselection);
		return;
	} 	

	uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
	vfs_uri = gnome_vfs_uri_new(uri);
	if (gnome_vfs_uri_exists(vfs_uri))
	{
		GtkWidget *dialog;
		GtkWidget *parent;
		parent = gtk_widget_get_toplevel (GTK_WIDGET (docman));
		dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
										 GTK_DIALOG_DESTROY_WITH_PARENT,
										 GTK_MESSAGE_QUESTION,
										 GTK_BUTTONS_NONE,
										 _("The file '%s' already exists.\n"
										 "Do you want to replace it with the one you are saving?"),
										 uri);
		gtk_dialog_add_button (GTK_DIALOG(dialog),
							   GTK_STOCK_CANCEL,
							   GTK_RESPONSE_CANCEL);
		anjuta_util_dialog_add_button (GTK_DIALOG (dialog),
								  _("_Replace"),
								  GTK_STOCK_REFRESH,
								  GTK_RESPONSE_YES);
		if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
			save_as_real (docman);
		gtk_widget_destroy (dialog);
	}
	else
		save_as_real (docman);
	g_free (uri);

	if (anjuta_preferences_get_int (ANJUTA_PREFERENCES (docman->priv->preferences),
									EDITOR_TABS_ORDERING))
		anjuta_docman_order_tabs (docman);
}

static GtkWidget*
create_file_open_dialog_gui(GtkWindow* parent, AnjutaDocman* docman)
{
	GtkWidget* dialog = 
		gtk_file_chooser_dialog_new (_("Open file"), 
									parent,
									GTK_FILE_CHOOSER_ACTION_OPEN,
									GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
									GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
									NULL);
	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER(dialog), TRUE);
	g_signal_connect(G_OBJECT(dialog), "response", 
					G_CALLBACK(on_open_filesel_ok_clicked), docman);
	g_signal_connect_swapped (GTK_OBJECT (dialog), 
                             "response", 
                             G_CALLBACK (gtk_widget_hide),
                             GTK_OBJECT (dialog));

	return dialog;
}

static GtkWidget*
create_file_save_dialog_gui(GtkWindow* parent, AnjutaDocman* docman)
{
	GtkWidget* dialog = 
		gtk_file_chooser_dialog_new (_("Save file as"), 
									parent,
									GTK_FILE_CHOOSER_ACTION_SAVE,
									GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
									GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
									NULL); 

	g_signal_connect(G_OBJECT(dialog), "response", 
					G_CALLBACK(on_save_as_filesel_ok_clicked), docman);	
	g_signal_connect_swapped (GTK_OBJECT (dialog), 
                             "response", 
                             G_CALLBACK (gtk_widget_hide),
                             GTK_OBJECT (dialog));
	return dialog;
}

gpointer parent_class;

static void
anjuta_docman_instance_init (AnjutaDocman *docman)
{
	GtkWidget *parent;

	/* FIXME: */
	// parent = gtk_widget_get_toplevel (GTK_WIDGET (docman));
	parent = NULL;
	docman->priv = g_new0 (AnjutaDocmanPriv, 1);
	docman->priv->fileselection =
		create_file_open_dialog_gui(GTK_WINDOW (parent), docman);
	/* Set to the current dir */
	/* Spends too much time */
	/* getcwd(wd, PATH_MAX);
	   fileselection_set_dir (app->fileselection, wd); */
	docman->priv->save_as_fileselection =
		create_file_save_dialog_gui (GTK_WINDOW (parent), docman);
	gtk_window_set_modal ((GtkWindow *) docman->priv->save_as_fileselection, TRUE);
	g_signal_connect (G_OBJECT(docman), "switch_page",
					  G_CALLBACK (on_notebook_switch_page), docman);
}

static void
anjuta_docman_class_init (AnjutaDocmanClass *klass)
{

}

GtkWidget*
anjuta_docman_new (AnjutaPreferences *pref)
{
	GtkWidget *docman = NULL;
	docman = gtk_widget_new (ANJUTA_TYPE_DOCMAN, NULL);
	if (docman)
	    ANJUTA_DOCMAN (docman)->priv->preferences = pref;
	return docman;
}

/*! state flag for Ctrl-TAB */
static gboolean g_tabbing = FALSE;

static void
on_notebook_switch_page (GtkNotebook * notebook,
						 GtkNotebookPage * page,
						 gint page_num, AnjutaDocman *docman)
{
	GtkWidget *widget;
	/* TTimo: reorder so that the most recently used files are always
	 * at the beginning of the tab list
	 */
	widget = gtk_notebook_get_nth_page (notebook, page_num);
	anjuta_docman_set_current_editor (docman, TEXT_EDITOR (widget));
	if (!g_tabbing &&
		!anjuta_preferences_get_int (docman->priv->preferences, EDITOR_TABS_ORDERING) && 
		anjuta_preferences_get_int (docman->priv->preferences, EDITOR_TABS_RECENT_FIRST))
	{
		gtk_notebook_reorder_child (notebook, widget, 0);
	}
	// anjuta_docman_grab_text_focus ();
}

TextEditor *
anjuta_docman_add_editor (AnjutaDocman *docman, gchar *uri, gchar *name)
{
	GtkWidget *te;
	AnjutaDocmanPage *page;

	// cur_page = anjuta_docman_get_current_editor ();
	te = text_editor_new (ANJUTA_PREFERENCES (docman->priv->preferences),
						  uri, name);
	/* File cannot be loaded, texteditor brings up an error dialog */
	if (te == NULL)
	{
		return NULL;
	}
	
	gtk_widget_show (te);
	page = anjuta_docman_page_new (te, docman);
	
	anjuta_docman_set_current_editor (docman, TEXT_EDITOR (te));
	docman->priv->editors = g_list_append (docman->priv->editors, (gpointer)page);
	
	// breakpoints_dbase_set_all_in_editor (debugger.breakpoints_dbase, te);
	
	gtk_notebook_prepend_page (GTK_NOTEBOOK (docman), te, page->box);
	gtk_notebook_set_menu_label_text(GTK_NOTEBOOK (docman), te,
									 TEXT_EDITOR (te)->filename);
	
	g_signal_handlers_block_by_func (GTK_OBJECT (docman),
				       GTK_SIGNAL_FUNC (on_notebook_switch_page),
				       docman);
	if (anjuta_preferences_get_int (ANJUTA_PREFERENCES (docman->priv->preferences),
									EDITOR_TABS_ORDERING))
			anjuta_docman_order_tabs (docman);
	
	gtk_signal_handler_unblock_by_func (GTK_OBJECT (docman),
			    GTK_SIGNAL_FUNC (on_notebook_switch_page),
			    docman);
	anjuta_docman_set_current_editor(docman, TEXT_EDITOR (te));
	return TEXT_EDITOR (te);
}

static AnjutaDocmanPage *
anjuta_docman_page_from_widget (AnjutaDocman *docman, TextEditor *te)
{
	GList *node;
	node = docman->priv->editors;
	while (node)
	{
		AnjutaDocmanPage *page;
		page = node->data;
		g_assert (page);
		if (page->widget == GTK_WIDGET (te))
			return page;
		node = g_list_next (node);
	}
	return NULL;
}

void
anjuta_docman_remove_editor (AnjutaDocman *docman, TextEditor* te)
{
	// GtkWidget *submenu;
	// GtkWidget *wid;
	AnjutaDocmanPage *page;
	gint page_num;
	
	if (!te)
		te = anjuta_docman_get_current_editor (docman);

	if (te == NULL)
		return;

	page = anjuta_docman_page_from_widget (docman, te);

	gtk_signal_handler_block_by_func (GTK_OBJECT (docman),
				       GTK_SIGNAL_FUNC (on_notebook_switch_page),
				       docman);
#if 0 // FIXME
	if (te->uri != NULL)
	{
		gint max_recent_files;
		GtkWidget *recent_submenu;

		max_recent_files =
			anjuta_preferences_get_int (app->preferences,
					     MAXIMUM_RECENT_FILES);
		app->recent_files =
			glist_path_dedup(update_string_list (dockman->priv->recent_files,
					    te->uri, max_recent_files));
		submenu =
			create_submenu (_("Recent Files "), app->recent_files,
					GTK_SIGNAL_FUNC (on_recent_files_menu_item_activate));
		
		recent_submenu = egg_menu_merge_get_widget (anjuta_ui_get_menu_merge (docman->ui),
													"/MenuMain/MenuFile/RecentFiles");
		gtk_menu_item_set_submenu (GTK_MENU_ITEM(recent_submenu), submenu);
	}
	breakpoints_dbase_clear_all_in_editor (debugger.breakpoints_dbase, te);
#endif
	
	page_num = 
		gtk_notebook_page_num (GTK_NOTEBOOK (docman), GTK_WIDGET (te));
	gtk_notebook_remove_page (GTK_NOTEBOOK (docman), page_num);
	docman->priv->editors = g_list_remove (docman->priv->editors, page);
	anjuta_docman_page_destroy (page);
	
	if (docman->priv->current_editor == te)
		docman->priv->current_editor = NULL;
	
	/* This is called to set the next active document */
	if (GTK_NOTEBOOK (docman)->children == NULL)
		anjuta_docman_set_current_editor (docman, NULL);
	else
	{
		GtkWidget *current_editor;
		page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (docman));
		current_editor = gtk_notebook_get_nth_page (GTK_NOTEBOOK (docman),
													page_num);
		anjuta_docman_set_current_editor (docman, TEXT_EDITOR (current_editor));
		gtk_widget_grab_focus (GTK_WIDGET (current_editor));
	}
	gtk_signal_handler_unblock_by_func (GTK_OBJECT (docman),
			    GTK_SIGNAL_FUNC (on_notebook_switch_page),
			    docman);
}

TextEditor *
anjuta_docman_get_current_editor (AnjutaDocman *docman)
{
	return docman->priv->current_editor;
}

void
anjuta_docman_set_current_editor (AnjutaDocman *docman, TextEditor * te)
{
	TextEditor *ote = docman->priv->current_editor;
	
	if (ote != NULL)
	{
		AnjutaDocmanPage *page = anjuta_docman_page_from_widget (docman, ote);
		if (page && page->close_button != NULL)
		{
			gtk_widget_hide (page->close_button);
			gtk_widget_show (page->close_image);
		}
	}
	docman->priv->current_editor = te;
	if (te != NULL)
	{
		gint page_num;
		AnjutaDocmanPage *page = anjuta_docman_page_from_widget (docman, te);
		if (page && page->close_button != NULL)
		{
			gtk_widget_show (page->close_button);
			gtk_widget_hide (page->close_image);
		}
		page_num = gtk_notebook_page_num (GTK_NOTEBOOK (docman),
										  GTK_WIDGET (te));
		g_signal_handlers_block_by_func (GTK_OBJECT (docman),
						   GTK_SIGNAL_FUNC (on_notebook_switch_page),
						   docman);
		gtk_notebook_set_current_page (GTK_NOTEBOOK (docman), page_num);
		if (anjuta_preferences_get_int (ANJUTA_PREFERENCES (docman->priv->preferences),
										EDITOR_TABS_ORDERING))
				anjuta_docman_order_tabs (docman);
		
		gtk_signal_handler_unblock_by_func (GTK_OBJECT (docman),
					GTK_SIGNAL_FUNC (on_notebook_switch_page),
					docman);
	}
	/*
	main_toolbar_update ();
	extended_toolbar_update ();
	format_toolbar_update ();
	browser_toolbar_update ();
	anjuta_update_title ();
	anjuta_update_app_status (FALSE, NULL);
	*/
	if (te == NULL)
	{
		const gchar *dir = g_get_home_dir();
		chdir (dir);
		return;
	}
	if (te->uri)
	{
		gchar* dir;
		dir = g_dirname (te->uri);
		chdir (dir);
		g_free (dir);
		return;
	}
	return;
}

TextEditor *
anjuta_docman_goto_file_line (AnjutaDocman *docman, gchar *fname, glong lineno)
{
	return anjuta_docman_goto_file_line_mark (docman, fname, lineno, TRUE);
}


TextEditor *
anjuta_docman_goto_file_line_mark (AnjutaDocman *docman, gchar *fname,
								   glong lineno, gboolean mark)
{
	gchar *uri;
	GnomeVFSURI* vfs_uri;
	GList *node;

	TextEditor *te;

	g_return_val_if_fail (fname, NULL);
	
	/*filename = anjuta_docman_get_full_filename (docman, fname);*/
	vfs_uri = gnome_vfs_uri_new(fname);
	uri = gnome_vfs_uri_to_string(vfs_uri, GNOME_VFS_URI_HIDE_NONE);
	gnome_vfs_uri_unref(vfs_uri);
	/* g_free(filename); */

	g_return_val_if_fail (uri != NULL, NULL);
	
	node = docman->priv->editors;
	
	while (node)
	{
		AnjutaDocmanPage *page;
		page = (AnjutaDocmanPage *) node->data;
		te = TEXT_EDITOR (page->widget);
		if (te->uri == NULL)
		{
			node = g_list_next (node);
			continue;
		}
		if (strcmp (uri, te->uri) == 0)
		{
			text_editor_check_disk_status (te, TRUE);
			if (lineno >= 0)
				text_editor_goto_line (te, lineno, mark, TRUE);
			anjuta_docman_show_editor (docman, GTK_WIDGET (te));
			anjuta_docman_grab_text_focus (docman);
			gtk_widget_grab_focus (GTK_WIDGET (te));
			g_free (uri);
			// an_file_history_push(te->uri, lineno);
			return te;
		}
		node = g_list_next (node);
	}
	te = anjuta_docman_add_editor (docman, uri, NULL);
	if (te)
	{
		an_file_history_push(te->uri, lineno);
		if (lineno >= 0)
			text_editor_goto_line (te, lineno, mark, FALSE);
	}
	g_free (uri);
	return te ;
}

static gchar*
get_real_path(const gchar *file_name)
{
	if (file_name)
	{
		gchar path[PATH_MAX+1];
		memset(path, '\0', PATH_MAX+1);
		realpath(file_name, path);
		return g_strdup(path);
	}
	else
		return NULL;
}

gchar *
anjuta_docman_get_full_filename (AnjutaDocman *docman, const gchar *fn)
{
	TextEditor *te;
	GList *te_list;
	gchar *real_path;
	gchar *fname;
	
	g_return_val_if_fail(fn, NULL);
	real_path = get_real_path (fn);
	
	/* If it is full and absolute path, there is no need to 
	go further, even if the file is not found*/
	if (fn[0] == '/')
	{
		return real_path;
	}
	
	/* First, check if we can get the file straightaway */
	if (g_file_test (real_path, G_FILE_TEST_IS_REGULAR))
		return real_path;
	g_free(real_path);

	/* Get the name part of the file */
	fname = g_path_get_basename (fn);
	
	/* Next, check if the current text editor buffer matches this name */
	if (NULL != (te = anjuta_docman_get_current_editor(docman)))
	{
		if (strcmp(te->filename, fname) == 0)
		{
			g_free (fname);
			return g_strdup (te->uri);
		}
	}
	/* Next, see if the name matches any of the opened files */
	for (te_list = docman->priv->editors; te_list; te_list = g_list_next (te_list))
	{
		AnjutaDocmanPage *page;
		page = (AnjutaDocmanPage *) te_list->data;
		te = TEXT_EDITOR (page->widget);
		if (strcmp(fname, te->filename) == 0)
		{
			g_free (fname);
			return g_strdup(te->uri);
		}
	}
	g_free (fname);
	return NULL;
}

void
anjuta_docman_show_editor (AnjutaDocman *docman, GtkWidget* te)
{
	GList *node;
	gint i;

	if (te == NULL)
		return;
	node = GTK_NOTEBOOK (docman)->children;
	i = 0;
	while (node)
	{
		GtkWidget *t;

		t = gtk_notebook_get_nth_page (GTK_NOTEBOOK (docman), i);
		if (t && t == te)
		{
			gtk_notebook_set_current_page (GTK_NOTEBOOK (docman), i);
			anjuta_docman_set_current_editor (docman, TEXT_EDITOR (te));
			return;
		}
		i++;
		node = g_list_next (node);
	}
}

// FIXME: Should be called from a te callback
static void
anjuta_docman_update_page_label (AnjutaDocman *docman, GtkWidget *te)
{
	GdkColor tmpcolor;
	AnjutaDocmanPage *page;
	
	if (te == NULL)
		return;

	page = anjuta_docman_page_from_widget (docman, TEXT_EDITOR (te));
	if (!page || page->label == NULL)
		return;
	
	if (text_editor_is_saved(TEXT_EDITOR (te)))
	{
		gdk_color_parse("black",&tmpcolor);
	}
	else
	{
		gdk_color_parse("red",&tmpcolor);
	}
	gtk_widget_modify_fg (page->label, GTK_STATE_NORMAL, &tmpcolor);
	gtk_widget_modify_fg (page->label, GTK_STATE_INSENSITIVE, &tmpcolor);
	gtk_widget_modify_fg (page->label, GTK_STATE_ACTIVE, &tmpcolor);
	gtk_widget_modify_fg (page->label, GTK_STATE_PRELIGHT, &tmpcolor);
	gtk_widget_modify_fg (page->label, GTK_STATE_SELECTED, &tmpcolor);
}

#if 0
void anjuta_set_zoom_factor(gint zoom)
{
	TextEditor *te = anjuta_get_current_text_editor();
	if (te)
		text_editor_set_zoom_factor(te, zoom);
}
#endif

static void
anjuta_docman_grab_text_focus (AnjutaDocman *docman)
{
	GtkWidget *text;
	text = GTK_WIDGET (anjuta_docman_get_current_editor (docman));
	if (!text)
		return; // This is not an error condition.
	text_editor_grab_focus (TEXT_EDITOR (text));
}

void
anjuta_docman_delete_all_markers (AnjutaDocman *docman, gint marker)
{
	GList *node;
	AnjutaDocmanPage *page;
	node = docman->priv->editors;
	
	while (node)
	{
		TextEditor* te;
		page = node->data;
		te = TEXT_EDITOR (page->widget);
		text_editor_delete_marker_all (te, marker);
		node = g_list_next (node);
	}
}

void
anjuta_docman_delete_all_indicators (AnjutaDocman *docman)
{
	GList *node;
	TextEditor *te;

	node = docman->priv->editors;
	while (node)
	{
		AnjutaDocmanPage *page;
		page = node->data;
		te = TEXT_EDITOR (page->widget);
		if (te->uri == NULL)
		{
			node = g_list_next (node);
			continue;
		}
		text_editor_set_indicator (te, -1, -1);
		node = g_list_next (node);
	}
}

TextEditor *
anjuta_docman_get_editor_from_path (AnjutaDocman *docman, const gchar *szFullPath )
{
	GList *node;
	AnjutaDocmanPage *page;
	TextEditor *te;

	g_return_val_if_fail (szFullPath != NULL, NULL);
	node = docman->priv->editors;
	while (node)
	{
		page = node->data;
		te = (TextEditor *) page->widget;
		if (te->uri != NULL)
		{
			if ( !strcmp ( szFullPath, te->uri) )
			{
				return te ;
			}
		}
		node = g_list_next (node);
	}
	return NULL;
}

/* Saves a file to keep it synchronized with external programs */
void 
anjuta_docman_save_file_if_modified (AnjutaDocman *docman, const gchar *szFullPath)
{
	TextEditor *te;

	g_return_if_fail ( szFullPath != NULL );

	te = anjuta_docman_get_editor_from_path (docman, szFullPath);
	if( NULL != te )
	{
		if( !text_editor_is_saved (te) )
		{
			text_editor_save_file (te, TRUE);
		}
	}
	return;
}

/* If an external program changed the file, we must reload it */
void 
anjuta_docman_reload_file (AnjutaDocman *docman, const gchar *szFullPath)
{
	TextEditor *te;

	g_return_if_fail (szFullPath != NULL);

	te = anjuta_docman_get_editor_from_path (docman, szFullPath);
	if( NULL != te )
	{
		glong	nNowPos = te->current_line ;
		/*text_editor_check_disk_status ( te, TRUE );asd sdf*/
		text_editor_load_file (te);
		text_editor_goto_line (te,  nNowPos, FALSE, FALSE);
	}
	return;
}

typedef struct _order_struct order_struct;
struct _order_struct
{
	gchar *m_label;
	GtkWidget *m_widget;
};

static int
do_ordertab1 (const void *a, const void *b)
{
	order_struct aos,bos;
	aos = *(order_struct*)a;
	bos = *(order_struct*)b;
	return (g_strcasecmp (aos.m_label, bos.m_label));
}

static void
anjuta_docman_order_tabs(AnjutaDocman *docman)
{
	gint i,j;
	GList *children;
	GtkWidget *widget,*label;
	order_struct *tab_labels;
	children = gtk_container_children (GTK_CONTAINER(docman));
	
	j = g_list_length(children);
	if (j<2)
		return;
	tab_labels = g_new0(order_struct,j);
	for (i=0;i<j;i++)
	{
		widget = gtk_notebook_get_nth_page (GTK_NOTEBOOK(docman), i);
		if (widget)
		{
			label = gtk_notebook_get_tab_label (GTK_NOTEBOOK (docman), widget);
			children = gtk_container_children (GTK_CONTAINER (label));
			for (; children; children = g_list_next(children))
			{
				if (GTK_IS_LABEL (children->data))
				{
					tab_labels[i].m_label = GTK_LABEL (children->data)->label;
					tab_labels[i].m_widget = widget;
				}
			}
		}
	}
	qsort (tab_labels, j , sizeof(order_struct), do_ordertab1);
	for (i=0;i<j;i++)
		gtk_notebook_reorder_child (GTK_NOTEBOOK (docman),
									tab_labels[i].m_widget, i);
	g_free (tab_labels);
}

gboolean
anjuta_docman_set_editor_properties (AnjutaDocman *docman)
{
	TextEditor *te = docman->priv->current_editor;
	if (te)
	{
		gchar *word;
		// FIXME: anjuta_set_file_properties (app->current_text_editor->uri);
		word = text_editor_get_current_word (docman->priv->current_editor);
		prop_set_with_key(docman->priv->preferences->props, "current.file.selection"
		  , word?word:"");
		if (word)
			g_free(word);
		prop_set_int_with_key(docman->priv->preferences->props, "current.file.lineno"
		  , text_editor_get_current_lineno(docman->priv->current_editor));
		return TRUE;
	}
	else
		return FALSE;
}

TextEditor*
anjuta_docman_find_editor_with_path (AnjutaDocman *docman,
									 const gchar *file_path)
{
	TextEditor *te;
	GList *tmp;
	
	te = NULL;
	for (tmp = docman->priv->editors; tmp; tmp = g_list_next(tmp))
	{
		AnjutaDocmanPage *page;
		page = (AnjutaDocmanPage *) tmp->data;
		if (!page)
			continue;
		te = (TextEditor *) page->widget;
		if (!te)
			continue;
		if (te->uri && 0 == strcmp(file_path, te->uri))
			break;
	}
	return te;
}

GList*
anjuta_docman_get_all_editors (AnjutaDocman *docman)
{
	GList *editors;
	GList *node;
	
	editors = NULL;
	node = docman->priv->editors;
	while (node)
	{
		AnjutaDocmanPage *page;
		TextEditor *te;
		page = (AnjutaDocmanPage *) node->data;
		te = (TextEditor *) page->widget;
		editors = g_list_prepend (editors, te);
		node = g_list_next (node);
	}
	editors = g_list_reverse (editors);
	return editors;
}

ANJUTA_TYPE_BEGIN(AnjutaDocman, anjuta_docman, GTK_TYPE_NOTEBOOK);
ANJUTA_TYPE_END;
