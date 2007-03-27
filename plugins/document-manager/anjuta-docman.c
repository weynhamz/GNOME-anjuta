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

#include <libanjuta/resources.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-markable.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-factory.h>

#include <gtk/gtkfilechooserdialog.h>
#include <libgnomevfs/gnome-vfs.h>

#include <gdl/gdl-icons.h>

#include "anjuta-docman.h"
#include "file_history.h"
#include "plugin.h"
#include "action-callbacks.h"
#include "editor-tooltips.h"

static gpointer parent_class;

/* Preference keys */
#define EDITOR_TABS_POS            "editor.tabs.pos"
#define EDITOR_TABS_HIDE           "editor.tabs.hide"
#define EDITOR_TABS_ORDERING       "editor.tabs.ordering"
#define EDITOR_TABS_RECENT_FIRST   "editor.tabs.recent.first"


struct _AnjutaDocmanPriv {
	DocmanPlugin *plugin;
	AnjutaPreferences *preferences;
	IAnjutaEditor *current_editor;
	
	GtkWidget *fileselection;
	
	GList *editors;
	GtkWidget *popup_menu;
	gboolean shutingdown;
};

struct _AnjutaDocmanPage {
	GtkWidget *widget;
	GtkWidget *close_image;
	GtkWidget *close_button;
	GtkWidget *mime_icon;
	GtkWidget *label;
	GtkWidget *menu_label;
	GtkWidget *box;
	gboolean is_current;
};

typedef struct _AnjutaDocmanPage AnjutaDocmanPage;

static void on_notebook_switch_page (GtkNotebook * notebook,
									 GtkNotebookPage * page,
									 gint page_num, AnjutaDocman *docman);


static void anjuta_docman_order_tabs(AnjutaDocman *docman);
static void anjuta_docman_update_page_label (AnjutaDocman *docman, GtkWidget *te);
static void anjuta_docman_grab_text_focus (AnjutaDocman *docman);
static void on_notebook_switch_page (GtkNotebook * notebook,
									 GtkNotebookPage * page,
									 gint page_num, AnjutaDocman *docman);

static void
on_text_editor_notebook_close_page (GtkButton* button, 
									AnjutaDocman* docman)
{
	GList* node;
	
	node = docman->priv->editors;
	while (node)
	{
		AnjutaDocmanPage *page;
		IAnjutaEditor *te;
		page = (AnjutaDocmanPage *) node->data;
		if (page->close_button == GTK_WIDGET(button))
		{
			te = IANJUTA_EDITOR (page->widget);
			anjuta_docman_set_current_editor(docman, te);
			break;
		}
		node = g_list_next (node);
	}
	
	on_close_file_activate (NULL, docman->priv->plugin);
}

static void
on_text_editor_notebook_close_button_enter_cb (GtkButton* button, 
											   AnjutaDocmanPage* page)
{
	g_return_if_fail (page != NULL);
	
	gtk_widget_set_sensitive (page->close_image, TRUE);
}

static void
on_text_editor_notebook_close_button_leave_cb (GtkButton* button, 
											   AnjutaDocmanPage* page)
{
	g_return_if_fail (page != NULL);
	
	if (! page->is_current)
		gtk_widget_set_sensitive (page->close_image,FALSE);
}

static void
editor_tab_widget_destroy (AnjutaDocmanPage* page)
{
	g_return_if_fail(page != NULL);
	g_return_if_fail(page->close_button != NULL);

	page->close_image = NULL;
	page->close_button = NULL;
	page->mime_icon = NULL;
	page->label = NULL;
	page->menu_label = NULL;
	page->is_current = FALSE;
}

static GtkWidget*
editor_tab_widget_new(AnjutaDocmanPage* page, AnjutaDocman* docman)
{
	GtkWidget *close_button;
	GtkWidget *close_pixmap;
	GtkRcStyle *rcstyle;
	GtkWidget *label, *menu_label;
	GtkWidget *box;
	GtkWidget *event_hbox;
	GtkWidget *event_box;
	GtkTooltips *tooltips;
	int h, w;
	GdkColor color;
	gchar* uri;
	IAnjutaFile* editor;
	
	g_return_val_if_fail(GTK_IS_WIDGET (page->widget), NULL);
	
	if (page->close_image)
		editor_tab_widget_destroy (page);
	
	gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);

	close_pixmap = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	gtk_widget_show(close_pixmap);
	
	/* setup close button, zero out {x,y}thickness to get smallest possible
	 * size */
	close_button = gtk_button_new();
	gtk_button_set_focus_on_click (GTK_BUTTON (close_button), FALSE);
	gtk_container_add(GTK_CONTAINER(close_button), close_pixmap);
	gtk_button_set_relief(GTK_BUTTON(close_button), GTK_RELIEF_NONE);
	rcstyle = gtk_rc_style_new ();
	rcstyle->xthickness = rcstyle->ythickness = 0;
	gtk_widget_modify_style (close_button, rcstyle);
	gtk_rc_style_unref (rcstyle);
	
	gtk_widget_set_size_request (close_button, w, h);
	tooltips = gtk_tooltips_new ();
	gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips), close_button,
						  _("Close file"),
						  NULL);

	label = gtk_label_new (ianjuta_editor_get_filename (IANJUTA_EDITOR(page->widget),
														NULL));
	gtk_misc_set_alignment (GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show (label);

	menu_label = gtk_label_new (ianjuta_editor_get_filename (IANJUTA_EDITOR(page->widget),
															NULL));
	gtk_widget_show (menu_label);
  	 
	color.red = 0;
	color.green = 0;
	color.blue = 0;
	
	gtk_widget_modify_fg (close_button, GTK_STATE_NORMAL, &color);
	gtk_widget_modify_fg (close_button, GTK_STATE_INSENSITIVE, &color);
	gtk_widget_modify_fg (close_button, GTK_STATE_ACTIVE, &color);
	gtk_widget_modify_fg (close_button, GTK_STATE_PRELIGHT, &color);
	gtk_widget_modify_fg (close_button, GTK_STATE_SELECTED, &color);
	gtk_widget_show(close_button);
	
	/* create our layout/event boxes */
	event_box = gtk_event_box_new();
	gtk_event_box_set_visible_window (GTK_EVENT_BOX (event_box), FALSE);

	event_hbox = gtk_hbox_new (FALSE, 2);	
	box = gtk_hbox_new(FALSE, 2);
	
	/* Add a nice mime-type icon */
	editor = IANJUTA_FILE(page->widget);
	uri = ianjuta_file_get_uri(editor, NULL);
	if (uri != NULL)
	{
		const int ICON_SIZE = 16;
		GdlIcons* icons = gdl_icons_new(ICON_SIZE);
		GdkPixbuf *pixbuf = gdl_icons_get_uri_icon(icons, uri);
		GtkWidget* image = gtk_image_new_from_pixbuf (pixbuf);
		gtk_box_pack_start (GTK_BOX(event_hbox), image, FALSE, FALSE, 0);
		page->mime_icon = image;
		g_object_unref(pixbuf);
		g_object_unref(icons);
	}
	g_free(uri);
	
	gtk_box_pack_start (GTK_BOX(event_hbox), label, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (event_hbox), close_button, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (event_box), event_hbox);
	
	/* setup the data hierarchy */
	g_object_set_data (G_OBJECT (box), "event_box", event_box);
	
	/* pack our top-level layout box */
	gtk_box_pack_start (GTK_BOX (box), event_box, TRUE, TRUE, 0);
	
	/* show the widgets of the tab */
	gtk_widget_show_all(box);

	gtk_signal_connect (GTK_OBJECT (close_button), "clicked",
				GTK_SIGNAL_FUNC(on_text_editor_notebook_close_page),
				docman);
	gtk_signal_connect (GTK_OBJECT (close_button), "enter",
				GTK_SIGNAL_FUNC(on_text_editor_notebook_close_button_enter_cb),
				page);
	gtk_signal_connect (GTK_OBJECT (close_button), "leave",
				GTK_SIGNAL_FUNC(on_text_editor_notebook_close_button_leave_cb),
				page);

	page->close_image = close_pixmap;
	page->close_button = close_button;
	page->label = label;
	page->menu_label = menu_label;

	return box;
}

static AnjutaDocmanPage *
anjuta_docman_page_new (GtkWidget *editor, AnjutaDocman* docman)
{
	AnjutaDocmanPage *page;
	
	page = g_new0 (AnjutaDocmanPage, 1);
	page->widget = GTK_WIDGET (editor);
	page->box = editor_tab_widget_new (page, docman);
	return page;
}

static void
anjuta_docman_page_destroy (AnjutaDocmanPage *page)
{
	editor_tab_widget_destroy (page);
	g_free (page);
}

static void
on_open_filesel_response (GtkDialog* dialog, gint id, AnjutaDocman *docman)
{
	gchar *uri;
	gchar *entry_filename = NULL;
	int i;
	GSList * list;
	int elements;

	if (id != GTK_RESPONSE_ACCEPT)
	{
		gtk_widget_hide (docman->priv->fileselection);
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
		list = g_slist_remove(list, entry_filename);
		g_free(entry_filename);
	}
}

static GtkWidget*
create_file_open_dialog_gui (GtkWindow* parent, AnjutaDocman* docman)
{
	GtkWidget* dialog = 
		gtk_file_chooser_dialog_new (_("Open file"), 
									parent,
									GTK_FILE_CHOOSER_ACTION_OPEN,
									GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
									GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
									NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER(dialog), TRUE);
	g_signal_connect (G_OBJECT(dialog), "response", 
					  G_CALLBACK(on_open_filesel_response), docman);
	g_signal_connect_swapped (G_OBJECT(dialog), "delete-event",
							  G_CALLBACK(gtk_widget_hide), dialog);	
	return dialog;
}

static GtkWidget*
create_file_save_dialog_gui (GtkWindow* parent, AnjutaDocman* docman)
{
	GtkWidget* dialog = 
		gtk_file_chooser_dialog_new (_("Save file as"), 
									parent,
									GTK_FILE_CHOOSER_ACTION_SAVE,
									GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
									GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
									NULL); 
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
	return dialog;
}

void
anjuta_docman_open_file (AnjutaDocman *docman)
{
	if (!docman->priv->fileselection)
	{
		GtkWidget *parent;
		parent = gtk_widget_get_toplevel (GTK_WIDGET (docman));
		docman->priv->fileselection =
			create_file_open_dialog_gui(GTK_WINDOW (parent), docman);
	}
	gtk_widget_show (docman->priv->fileselection);
}

gboolean
anjuta_docman_save_editor_as (AnjutaDocman *docman, IAnjutaEditor *te,
							  GtkWidget *parent_window)
{
	gchar* uri;
	GnomeVFSURI* vfs_uri;
	gchar* file_uri;
	const gchar* filename;
	GtkWidget *parent;
	GtkWidget *dialog;
	gint response;
	gboolean file_saved = TRUE;
	
	g_return_val_if_fail (ANJUTA_IS_DOCMAN (docman), FALSE);
	g_return_val_if_fail (IANJUTA_IS_EDITOR (te), FALSE);
	
	if (parent_window)
		parent = parent_window;
	else
		parent = gtk_widget_get_toplevel (GTK_WIDGET (docman));
	
	dialog = create_file_save_dialog_gui (GTK_WINDOW (parent), docman);
	
	if ((file_uri = ianjuta_file_get_uri(IANJUTA_FILE(te), NULL)) != NULL)
	{
		gtk_file_chooser_set_uri (GTK_FILE_CHOOSER(dialog), file_uri);
		g_free (file_uri);
	}
	else if ((filename = ianjuta_editor_get_filename(te, NULL)) != NULL)
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER(dialog), filename);
	else
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER(dialog), "");
	
	response = gtk_dialog_run (GTK_DIALOG (dialog));
	if (response != GTK_RESPONSE_ACCEPT)
	{
		gtk_widget_destroy (dialog);
		return FALSE;
	}
	
	uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
	vfs_uri = gnome_vfs_uri_new(uri);
	if (gnome_vfs_uri_exists(vfs_uri))
	{
		GtkWidget *msg_dialog;
		msg_dialog = gtk_message_dialog_new (GTK_WINDOW (dialog),
											 GTK_DIALOG_DESTROY_WITH_PARENT,
											 GTK_MESSAGE_QUESTION,
											 GTK_BUTTONS_NONE,
											 _("The file '%s' already exists.\n"
											 "Do you want to replace it with the"
											 " one you are saving?"),
											 uri);
		gtk_dialog_add_button (GTK_DIALOG (msg_dialog),
							   GTK_STOCK_CANCEL,
							   GTK_RESPONSE_CANCEL);
		anjuta_util_dialog_add_button (GTK_DIALOG (msg_dialog),
								  _("_Replace"),
								  GTK_STOCK_REFRESH,
								  GTK_RESPONSE_YES);
		if (gtk_dialog_run (GTK_DIALOG (msg_dialog)) == GTK_RESPONSE_YES)
			ianjuta_file_savable_save_as (IANJUTA_FILE_SAVABLE (te), uri,
										  NULL);
		else
			file_saved = FALSE;
		gtk_widget_destroy (msg_dialog);
	}
	else
	{
		ianjuta_file_savable_save_as (IANJUTA_FILE_SAVABLE (te), uri, NULL);
	}
	
	if (anjuta_preferences_get_int (ANJUTA_PREFERENCES (docman->priv->preferences),
									EDITOR_TABS_ORDERING))
		anjuta_docman_order_tabs (docman);

	gtk_widget_destroy (dialog);
	g_free (uri);
	gnome_vfs_uri_unref (vfs_uri);
	return file_saved;
}

gboolean
anjuta_docman_save_editor (AnjutaDocman *docman, IAnjutaEditor *te,
						   GtkWidget *parent_window)
{
	gchar *uri;
	gboolean ret = TRUE;
	
	uri = ianjuta_file_get_uri (IANJUTA_FILE(te), NULL);
	
	if (uri == NULL)
	{
		anjuta_docman_set_current_editor (docman, te);
		ret = anjuta_docman_save_editor_as (docman, te, parent_window);
	}
	else
	{
		/* TODO: Error checking */
		ianjuta_file_savable_save (IANJUTA_FILE_SAVABLE(te), NULL);
	}
	g_free (uri);
	return ret;
}

static void
anjuta_docman_dispose (GObject *obj)
{
	AnjutaDocman *docman;
	GList *node;
	
	docman = ANJUTA_DOCMAN (obj);
	docman->priv->shutingdown = TRUE;
	
	DEBUG_PRINT ("Disposing AnjutaDocman object");
	if (docman->priv->popup_menu)
	{
		g_object_unref (docman->priv->popup_menu);
		docman->priv->popup_menu = NULL;
	}
	if (docman->priv->editors)
	{
		/* Destroy all text editors. Note that by call gtk_widget_destroy,
		   we are ensuring "destroy" signal is emitted in case other plugins
		   hold refs on the editors
		*/
		GList *editors;
		
		editors = anjuta_docman_get_all_editors (docman);
		node = editors;
		while (node)
		{
			gtk_widget_destroy (node->data);
			node = g_list_next (node);
		}
		g_list_free (docman->priv->editors);
		g_list_free (editors);
		docman->priv->editors = NULL;
	}
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (G_OBJECT(obj)));
}

static void
anjuta_docman_finalize (GObject *obj)
{
	AnjutaDocman *docman;
	
	DEBUG_PRINT ("Finalising AnjutaDocman object");
	docman = ANJUTA_DOCMAN (obj);
	if (docman->priv)
	{
		if (docman->priv->fileselection)
			gtk_widget_destroy (docman->priv->fileselection);
		g_free (docman->priv);
		docman->priv = NULL;
	}
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (G_OBJECT(obj)));
}

static void
anjuta_docman_instance_init (AnjutaDocman *docman)
{
	docman->priv = g_new0 (AnjutaDocmanPriv, 1);
	docman->priv->popup_menu = NULL;
	docman->priv->fileselection = NULL;
	
	gtk_notebook_popup_enable (GTK_NOTEBOOK (docman));
	gtk_notebook_set_scrollable (GTK_NOTEBOOK (docman), TRUE);
	g_signal_connect (G_OBJECT(docman), "switch-page",
					  G_CALLBACK (on_notebook_switch_page), docman);
}

static void
anjuta_docman_class_init (AnjutaDocmanClass *klass)
{
	static gboolean initialized;
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = anjuta_docman_finalize;
	object_class->dispose = anjuta_docman_dispose;
	
	if (!initialized)
	{
		initialized = TRUE;
		
		/* Signal */
		g_signal_new ("editor-added",
			ANJUTA_TYPE_DOCMAN,
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (AnjutaDocmanClass, editor_added),
			NULL, NULL,
			g_cclosure_marshal_VOID__OBJECT,
			G_TYPE_NONE,
			1,
			G_TYPE_OBJECT);
		g_signal_new ("editor-changed",
			ANJUTA_TYPE_DOCMAN,
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (AnjutaDocmanClass, editor_changed),
			NULL, NULL,
			g_cclosure_marshal_VOID__OBJECT,
			G_TYPE_NONE,
			1,
			G_TYPE_OBJECT);
			
	}
}

GtkWidget*
anjuta_docman_new (DocmanPlugin* plugin, AnjutaPreferences *pref)
{

	GtkWidget *docman = NULL;
	docman = gtk_widget_new (ANJUTA_TYPE_DOCMAN, NULL);
	if (docman)
	{
		ANJUTA_DOCMAN (docman)->priv->plugin = plugin;
		ANJUTA_DOCMAN (docman)->priv->preferences = pref;
	}

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
	DEBUG_PRINT ("Switching notebook page");
	if (docman->priv->shutingdown == FALSE)
	{
		/* TTimo: reorder so that the most recently used files are always
		 * at the beginning of the tab list
		 */
		widget = gtk_notebook_get_nth_page (notebook, page_num);
		anjuta_docman_set_current_editor (docman, IANJUTA_EDITOR (widget));
		
		if (!g_tabbing &&
			!anjuta_preferences_get_int (docman->priv->preferences, EDITOR_TABS_ORDERING) && 
			anjuta_preferences_get_int (docman->priv->preferences, EDITOR_TABS_RECENT_FIRST))
		{
			gtk_notebook_reorder_child (notebook, widget, 0);
		}
	}
}

static AnjutaDocmanPage *
anjuta_docman_page_from_widget (AnjutaDocman *docman, IAnjutaEditor *te)
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

static void
on_editor_save_point (IAnjutaEditor *editor, gboolean entering,
					  AnjutaDocman *docman)
{
	anjuta_docman_update_page_label (docman, GTK_WIDGET (editor));
}

static void
on_editor_destroy (IAnjutaEditor *te, AnjutaDocman *docman)
{
	// GtkWidget *submenu;
	// GtkWidget *wid;
	AnjutaDocmanPage *page;
	gint page_num;
	
	DEBUG_PRINT ("text editor destroy");

	page = anjuta_docman_page_from_widget (docman, te);
	gtk_signal_handler_block_by_func (GTK_OBJECT (docman),
				       GTK_SIGNAL_FUNC (on_notebook_switch_page),
				       docman);
#if 0 // FIXME
	g_signal_handlers_disconnect_by_func (G_OBJECT (te),
										  G_CALLBACK (on_editor_save_point),
										  docman);
	g_signal_handlers_disconnect_by_func (G_OBJECT (te),
										  G_CALLBACK (on_editor_destroy),
										  docman);

	breakpoints_dbase_clear_all_in_editor (debugger.breakpoints_dbase, te);
#endif
	
	docman->priv->editors = g_list_remove (docman->priv->editors, page);
	anjuta_docman_page_destroy (page);

	/* This is called to set the next active document */
	if (docman->priv->shutingdown == FALSE)
	{
		if (docman->priv->current_editor == te)
			anjuta_docman_set_current_editor (docman, NULL);
	
		if (GTK_NOTEBOOK (docman)->children == NULL)
			anjuta_docman_set_current_editor (docman, NULL);
		else
		{
			GtkWidget *current_editor;
			page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (docman));
			current_editor = gtk_notebook_get_nth_page (GTK_NOTEBOOK (docman),
														page_num);
			anjuta_docman_set_current_editor (docman, IANJUTA_EDITOR (current_editor));
		}
	}
	gtk_signal_handler_unblock_by_func (GTK_OBJECT (docman),
			    GTK_SIGNAL_FUNC (on_notebook_switch_page),
			    docman);
}


IAnjutaEditor *
anjuta_docman_add_editor (AnjutaDocman *docman, const gchar *uri,
						  const gchar *name)
{
	IAnjutaEditor *te;
	IAnjutaEditorFactory* factory;
	GtkWidget *event_box;
	gchar *tip;
	gchar *ruri;

	EditorTooltips *tooltips = NULL;
	AnjutaDocmanPage *page;
	AnjutaStatus *status;

	status = anjuta_shell_get_status (docman->shell, NULL);
	factory = anjuta_shell_get_interface(docman->shell, IAnjutaEditorFactory, NULL);
	
	if (name && uri)
		te = ianjuta_editor_factory_new_editor(factory, 
										   uri, name, NULL);
	
	else if (uri)
		te = ianjuta_editor_factory_new_editor(factory, 
										   uri, "", NULL);
	else if (name)
		te = ianjuta_editor_factory_new_editor(factory, 
										   "", name, NULL);
	else
		te = ianjuta_editor_factory_new_editor(factory, 
										   "", "", NULL);

	
	/* File cannot be loaded, texteditor brings up an error dialog */
	if (te == NULL)
	{
		return NULL;
	}
	ianjuta_editor_set_popup_menu (te, docman->priv->popup_menu, NULL);
	
	
	gtk_widget_show (GTK_WIDGET(te));
	page = anjuta_docman_page_new (GTK_WIDGET(te), docman);

	if (tooltips == NULL) {
		tooltips = editor_tooltips_new();
	}

	docman->priv->editors = g_list_append (docman->priv->editors, (gpointer)page);

	ruri =  gnome_vfs_format_uri_for_display (uri ? uri : "");
	
	/* set the tooltips */	
	tip = g_markup_printf_escaped("<b>%s</b> %s\n",
				       _("Path:"), ruri );
	
	event_box = g_object_get_data (G_OBJECT(page->box), "event_box");
	editor_tooltips_set_tip (tooltips, event_box, tip, NULL);
	g_free (tip);
	g_free (ruri);

	gtk_notebook_prepend_page_menu (GTK_NOTEBOOK (docman), GTK_WIDGET(te), page->box, 
									page->menu_label);
	
	/* Tab dnd ordering is only in gtk >= 2.10 */
#if (GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 10))
	gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK (docman), GTK_WIDGET(te),
									  TRUE);
#endif
	
	g_signal_handlers_block_by_func (GTK_OBJECT (docman),
				       GTK_SIGNAL_FUNC (on_notebook_switch_page),
				       docman);
	gtk_notebook_set_current_page (GTK_NOTEBOOK (docman), 0);
	

	if (anjuta_preferences_get_int (ANJUTA_PREFERENCES (docman->priv->preferences),
									EDITOR_TABS_ORDERING))
			anjuta_docman_order_tabs (docman);

	gtk_signal_handler_unblock_by_func (GTK_OBJECT (docman),
			    GTK_SIGNAL_FUNC (on_notebook_switch_page),
			    docman);
	g_signal_connect (G_OBJECT (te), "save-point",
					  G_CALLBACK (on_editor_save_point), docman);
	g_signal_connect (G_OBJECT (te), "destroy",
					  G_CALLBACK (on_editor_destroy), docman);
				   
	g_signal_emit_by_name (docman, "editor-added", te);
	anjuta_docman_set_current_editor(docman, te);
	anjuta_shell_present_widget( ANJUTA_SHELL (docman->shell), GTK_WIDGET (docman), NULL);
	return te;
}

void
anjuta_docman_remove_editor (AnjutaDocman *docman, IAnjutaEditor* te)
{
	if (!te)
		te = anjuta_docman_get_current_editor (docman);

	if (te == NULL)
		return;

	gtk_widget_destroy (GTK_WIDGET (te));
}

void
anjuta_docman_set_popup_menu (AnjutaDocman *docman, GtkWidget *menu)
{
	if (menu)
		g_object_ref (menu);
	if (docman->priv->popup_menu)
		g_object_unref (docman->priv->popup_menu);
	docman->priv->popup_menu = menu;
}

IAnjutaEditor *
anjuta_docman_get_current_editor (AnjutaDocman *docman)
{
	return docman->priv->current_editor;
}

void
anjuta_docman_set_current_editor (AnjutaDocman *docman, IAnjutaEditor * te)
{
	gchar *uri;
	IAnjutaEditor *ote = docman->priv->current_editor;
	
	if (ote == te)
		return;
	
	if (ote != NULL)
	{
		AnjutaDocmanPage *page = anjuta_docman_page_from_widget (docman, ote);
		if (page && page->close_button != NULL)
		{
			gtk_widget_set_sensitive (page->close_image, FALSE);
			if (page->mime_icon)
				gtk_widget_set_sensitive (page->mime_icon, FALSE);
			page->is_current = FALSE;
		}
	}
	docman->priv->current_editor = te;
	if (te != NULL)
	{
		gint page_num;
		AnjutaDocmanPage *page = anjuta_docman_page_from_widget (docman, te);
		if (page && page->close_button != NULL)
		{
			gtk_widget_set_sensitive (page->close_image, TRUE);
			if (page->mime_icon)
				gtk_widget_set_sensitive (page->mime_icon, TRUE);
			page->is_current = TRUE;
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
		
		gtk_widget_grab_focus (GTK_WIDGET (te));
		anjuta_docman_grab_text_focus (docman);
		
		gtk_signal_handler_unblock_by_func (GTK_OBJECT (docman),
					GTK_SIGNAL_FUNC (on_notebook_switch_page),
					docman);
	}
/* 
	if (te == NULL)
	{
		const gchar *dir = g_get_home_dir();
		chdir (dir);
	}
*/
	uri = ianjuta_file_get_uri(IANJUTA_FILE(te), NULL);
	if (te && uri)
	{
		gchar *hostname;
		gchar *filename;
		
		filename = g_filename_from_uri (uri, &hostname, NULL);
		if (hostname == NULL && filename)
		{
			gchar *dir;
			dir = g_path_get_dirname (filename);
			if (dir)
				chdir (dir);
			g_free (dir);
		}
		g_free (hostname);
		g_free (filename);
	}
	g_free (uri);
	g_signal_emit_by_name (G_OBJECT (docman), "editor_changed", te);
	return;
}

IAnjutaEditor *
anjuta_docman_goto_file_line (AnjutaDocman *docman, const gchar *fname, glong lineno)
{
	return anjuta_docman_goto_file_line_mark (docman, fname, lineno, FALSE);
}

IAnjutaEditor *
anjuta_docman_goto_file_line_mark (AnjutaDocman *docman, const gchar *fname,
								   glong line, gboolean mark)
{
	gchar *uri;
	GnomeVFSURI* vfs_uri;
	GList *node;
	const gchar *linenum;
	glong lineno;
	gboolean is_local_uri;
	gchar *normalized_path = NULL;
	
	IAnjutaEditor *te;

	g_return_val_if_fail (fname, NULL);
	
	/* FIXME: */
	/* filename = anjuta_docman_get_full_filename (docman, fname); */
	vfs_uri = gnome_vfs_uri_new (fname);
	
	/* Extract linenum which comes as fragement identifier */
	linenum = gnome_vfs_uri_get_fragment_identifier (vfs_uri);
	if (linenum)
		lineno = atoi (linenum);
	else
		lineno = line;
	
	/* Restore URI without fragement identifier (linenum) */
	uri = gnome_vfs_uri_to_string (vfs_uri,
								   GNOME_VFS_URI_HIDE_FRAGMENT_IDENTIFIER);
	
	/* Get the normalized file path for comparision */
	is_local_uri = gnome_vfs_uri_is_local (vfs_uri);
	if (is_local_uri)
		normalized_path = realpath (gnome_vfs_uri_get_path (vfs_uri), NULL);
	if (normalized_path == NULL)
		normalized_path = g_strdup (uri);
	
	gnome_vfs_uri_unref (vfs_uri);
	/* g_free(filename); */

	g_return_val_if_fail (uri != NULL, NULL);
	
	node = docman->priv->editors;
	
	while (node)
	{
		AnjutaDocmanPage *page;
		gboolean te_is_local_uri;
		gchar *te_uri;
		gchar *te_normalized_path = NULL;
		
		page = (AnjutaDocmanPage *) node->data;
		te = IANJUTA_EDITOR (page->widget);
		te_uri = ianjuta_file_get_uri(IANJUTA_FILE(te), NULL);
		
		if (te_uri == NULL)
		{
			node = g_list_next (node);
			continue;
		}
		
		/* Get the normalized file path for comparision */
		vfs_uri = gnome_vfs_uri_new (te_uri);
		te_is_local_uri = gnome_vfs_uri_is_local (vfs_uri);
		if (te_is_local_uri)
			te_normalized_path = realpath (gnome_vfs_uri_get_path (vfs_uri),
										   NULL);
		if (te_normalized_path == NULL)
			te_normalized_path = g_strdup (te_uri);
		gnome_vfs_uri_unref (vfs_uri);
		
		if (strcmp (normalized_path, te_normalized_path) == 0)
		{
			if (lineno >= 0)
			{
				ianjuta_editor_goto_line(te, lineno, NULL);
				if (mark)
  	            {
  	            	ianjuta_markable_delete_all_markers(IANJUTA_MARKABLE(te),
  	                									IANJUTA_MARKABLE_LINEMARKER,
  	                                                    NULL);
					ianjuta_markable_mark(IANJUTA_MARKABLE(te), lineno,
  	                				IANJUTA_MARKABLE_LINEMARKER, NULL);
				}
  	        }
			anjuta_docman_show_editor (docman, GTK_WIDGET (te));
			an_file_history_push (te_uri, lineno);
			g_free (uri);
			g_free (te_uri);
			g_free (normalized_path);
			g_free (te_normalized_path);
			return te;
		}
		g_free (te_uri);
		g_free (te_normalized_path);
		node = g_list_next (node);
	}
	te = anjuta_docman_add_editor (docman, uri, NULL);
	if (te)
	{
		an_file_history_push (ianjuta_file_get_uri(IANJUTA_FILE(te), NULL), 
							  lineno);
		if (lineno >= 0) 
		{
			ianjuta_editor_goto_line(te, lineno, NULL);
            if (mark)
  	        {
  	        	ianjuta_markable_mark(IANJUTA_MARKABLE(te), lineno,
  	            					IANJUTA_MARKABLE_LINEMARKER, NULL);
			}
		}
	}
	g_free (uri);
	g_free (normalized_path);
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
	IAnjutaEditor *te;
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
		if (strcmp(ianjuta_editor_get_filename(te, NULL), fname) == 0)
		{
			g_free (fname);
			return ianjuta_file_get_uri (IANJUTA_FILE(te), NULL);
		}
	}
	/* Next, see if the name matches any of the opened files */
	for (te_list = docman->priv->editors; te_list; te_list = g_list_next (te_list))
	{
		AnjutaDocmanPage *page;
		page = (AnjutaDocmanPage *) te_list->data;
		te = IANJUTA_EDITOR (page->widget);
		if (strcmp(fname, ianjuta_editor_get_filename(te, NULL)) == 0)
		{
			g_free (fname);
			return ianjuta_file_get_uri(IANJUTA_FILE(te), NULL);
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
			anjuta_docman_set_current_editor (docman, IANJUTA_EDITOR (te));
			anjuta_shell_present_widget (ANJUTA_SHELL (docman->shell), GTK_WIDGET (docman), NULL);
			return;
		}
		i++;
		node = g_list_next (node);
	}
}

static void
anjuta_docman_update_page_label (AnjutaDocman *docman, GtkWidget *te_widget)
{
	GdkColor tmpcolor, *colorp = NULL;
	AnjutaDocmanPage *page;
	gchar *basename;
	gchar *uri;
	IAnjutaEditor *te;
	const gchar* te_filename;
	
	te = IANJUTA_EDITOR (te_widget);
	if (te == NULL)
		return;

	page = anjuta_docman_page_from_widget (docman, te);
	if (!page || page->label == NULL || page->menu_label == NULL)
		return;
	
	if (!ianjuta_file_savable_is_dirty(IANJUTA_FILE_SAVABLE(te), NULL))
	{
		/* setting GdkColor value in gtk_widget_modify_fg to NULL will 
		 * reset it to the default colors */
		colorp = NULL;
	}
	else
	{
		gdk_color_parse("red", &tmpcolor);
		colorp = &tmpcolor;
	}
	gtk_widget_modify_fg (page->label, GTK_STATE_NORMAL, colorp);
	gtk_widget_modify_fg (page->label, GTK_STATE_INSENSITIVE, colorp);
	gtk_widget_modify_fg (page->label, GTK_STATE_ACTIVE, colorp);
	gtk_widget_modify_fg (page->label, GTK_STATE_PRELIGHT, colorp);
	gtk_widget_modify_fg (page->label, GTK_STATE_SELECTED, colorp);

    gtk_widget_modify_fg (page->menu_label, GTK_STATE_NORMAL, colorp);
	gtk_widget_modify_fg (page->menu_label, GTK_STATE_INSENSITIVE, colorp);
  	gtk_widget_modify_fg (page->menu_label, GTK_STATE_ACTIVE, colorp);
  	gtk_widget_modify_fg (page->menu_label, GTK_STATE_PRELIGHT, colorp);
  	gtk_widget_modify_fg (page->menu_label, GTK_STATE_SELECTED, colorp);
	
	uri = ianjuta_file_get_uri(IANJUTA_FILE(te), NULL);
	if (uri)
	{
		basename = g_path_get_basename (uri);
		gtk_label_set_text (GTK_LABEL (page->label), basename);
		gtk_label_set_text (GTK_LABEL (page->menu_label), basename);
		g_free (basename);
		g_free (uri);
	}
	else if ((te_filename = ianjuta_editor_get_filename(te, NULL)) != NULL)
	{
		gtk_label_set_text (GTK_LABEL (page->label), te_filename);
		gtk_label_set_text (GTK_LABEL (page->menu_label), te_filename);	
	}
}

static void
anjuta_docman_grab_text_focus (AnjutaDocman *docman)
{
	anjuta_shell_present_widget(docman->shell, GTK_WIDGET(docman), NULL);
}

void
anjuta_docman_delete_all_markers (AnjutaDocman *docman, gint marker)
{
	GList *node;
	AnjutaDocmanPage *page;
	node = docman->priv->editors;
	
	while (node)
	{
		IAnjutaEditor* te;
		page = node->data;
		te = IANJUTA_EDITOR (page->widget);
		ianjuta_markable_delete_all_markers(IANJUTA_MARKABLE(te), marker, NULL);
		node = g_list_next (node);
	}
}

void
anjuta_docman_delete_all_indicators (AnjutaDocman *docman)
{
	GList *node;
	IAnjutaEditor *te;

	node = docman->priv->editors;
	while (node)
	{
		gchar *uri;
		AnjutaDocmanPage *page;
		
		page = node->data;
		te = IANJUTA_EDITOR (page->widget);
		uri = ianjuta_file_get_uri(IANJUTA_FILE(te), NULL);
		if (uri == NULL)
		{
			node = g_list_next (node);
			continue;
		}
		g_free (uri);
		ianjuta_markable_unmark(IANJUTA_MARKABLE(te), -1, -1, NULL);
		node = g_list_next (node);
	}
}

IAnjutaEditor *
anjuta_docman_get_editor_from_path (AnjutaDocman *docman, const gchar *szFullPath)
{
	GList *node;
	AnjutaDocmanPage *page;
	IAnjutaEditor *te;
	
	g_return_val_if_fail (szFullPath != NULL, NULL);
	node = docman->priv->editors;
	while (node)
	{
		gchar* uri;
		page = node->data;
		te = IANJUTA_EDITOR (page->widget);
		uri = ianjuta_file_get_uri (IANJUTA_FILE(te), NULL);
		if (uri != NULL)
		{
			if (strcmp (szFullPath, uri) == 0)
			{
				g_free (uri);
				return te ;
			}
			g_free (uri);
		}
		node = g_list_next (node);
	}
	return NULL;
}

/* Saves a file to keep it synchronized with external programs */
void 
anjuta_docman_save_file_if_modified (AnjutaDocman *docman, const gchar *szFullPath)
{
	IAnjutaEditor *te;

	g_return_if_fail ( szFullPath != NULL );

	te = anjuta_docman_get_editor_from_path (docman, szFullPath);
	if( NULL != te )
	{
		if(!ianjuta_file_savable_is_dirty (IANJUTA_FILE_SAVABLE(te), NULL))
		{
			ianjuta_file_savable_save (IANJUTA_FILE_SAVABLE(te), NULL);
		}
	}
	return;
}

/* If an external program changed the file, we must reload it */
void 
anjuta_docman_reload_file (AnjutaDocman *docman, const gchar *szFullPath)
{
	IAnjutaEditor *te;

	g_return_if_fail (szFullPath != NULL);

	te = anjuta_docman_get_editor_from_path (docman, szFullPath);
	if( NULL != te )
	{
		glong nNowPos =  ianjuta_editor_get_lineno(te, NULL);
		ianjuta_file_open(IANJUTA_FILE(te), szFullPath, NULL); 
		ianjuta_editor_goto_line(te, nNowPos, NULL);
	}
	return;
}

typedef struct _order_struct order_struct;
struct _order_struct
{
	const gchar *m_label;
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
anjuta_docman_order_tabs (AnjutaDocman *docman)
{
	gint i, num_pages;
	GtkWidget *widget;
	order_struct *tab_labels;
	
	num_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK(docman));
	if (num_pages < 2)
		return;
	tab_labels = g_new0 (order_struct, num_pages);
	for (i = 0; i < num_pages; i++)
	{
		if((widget = gtk_notebook_get_nth_page (GTK_NOTEBOOK(docman),
												i)) == NULL)
		{
			tab_labels[i].m_label = NULL;
			tab_labels[i].m_widget = NULL;
		}
		else
		{
			tab_labels[i].m_widget = widget;
			tab_labels[i].m_label = ianjuta_editor_get_filename (IANJUTA_EDITOR(widget),
																NULL);
		}
	}
	qsort (tab_labels, num_pages, sizeof(order_struct), do_ordertab1);
	for (i = 0; i < num_pages; i++)
		gtk_notebook_reorder_child (GTK_NOTEBOOK (docman),
									tab_labels[i].m_widget, i);
	g_free (tab_labels);
}


gboolean
anjuta_docman_set_editor_properties (AnjutaDocman *docman)
{
	/* FIXME:
	TextEditor *te = docman->priv->current_editor;
	if (te)
	{
		gchar *word;
		// FIXME: anjuta_set_file_properties (app->current_text_editor->uri);
		word = text_editor_get_current_word (docman->priv->current_editor);
		prop_set_with_key(te->props_base, "current.file.selection"
		  , word?word:"");
		if (word)
			g_free(word);
		prop_set_int_with_key(te->props_base, "current.file.lineno"
		  , text_editor_get_current_lineno(docman->priv->current_editor));
		return TRUE;
	}
	else
		return FALSE; */
	return TRUE;
}

IAnjutaEditor*
anjuta_docman_find_editor_with_path (AnjutaDocman *docman,
									 const gchar *file_path)
{
	IAnjutaEditor *te;
	GList *tmp;
	
	te = NULL;
	for (tmp = docman->priv->editors; tmp; tmp = g_list_next(tmp))
	{
		gchar* uri;
		AnjutaDocmanPage *page;
		
		page = (AnjutaDocmanPage *) tmp->data;
		if (!page)
			continue;
		te = IANJUTA_EDITOR(page->widget);
		if (!te)
			continue;
		uri = ianjuta_file_get_uri (IANJUTA_FILE(te), NULL);
		if (uri && 0 == strcmp (file_path, uri))
		{
			g_free (uri);
			return te;
		}
		g_free (uri);
	}
	return NULL;
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
		IAnjutaEditor *te;
		page = (AnjutaDocmanPage *) node->data;
		te = IANJUTA_EDITOR (page->widget);
		editors = g_list_prepend (editors, te);
		node = g_list_next (node);
	}
	editors = g_list_reverse (editors);
	return editors;
}

void
anjuta_docman_set_busy (AnjutaDocman *docman, gboolean state)
{
	GList *node;
	
	node = docman->priv->editors;
	while (node)
	{
		AnjutaDocmanPage *page;
		IAnjutaEditor *te;
		page = (AnjutaDocmanPage *) node->data;
		te = IANJUTA_EDITOR (page->widget);
		/* FIXME:
		text_editor_set_busy (te, state);*/
		node = g_list_next (node);
	}
	gdk_flush ();
}

ANJUTA_TYPE_BEGIN(AnjutaDocman, anjuta_docman, GTK_TYPE_NOTEBOOK);
ANJUTA_TYPE_END;
