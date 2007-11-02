/*
    anjuta_cbs.c
    Copyright (C) 2000  Naba Kumar <naba@gnome.org>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include <gnome.h>

#include <libanjuta/resources.h>

#include "text_editor.h"
#include "anjuta.h"
#include "controls.h"
#include "fileselection.h"
#include "utilities.h"
#include "launcher.h"
#include "debugger.h"


/* Blurb ?? */
extern gboolean closing_state;

gint on_anjuta_delete (GtkWidget *w, GdkEvent *event, gpointer data)
{
	AnjutaApp *app;
	TextEditor *te;
	GList *list;
	gboolean file_not_saved;
	gint max_recent_files, max_recent_prjs;
	
	app = ANJUTA_APP (w);

	file_not_saved = FALSE;
	max_recent_files = 
		anjuta_preferences_get_int (ANJUTA_PREFERENCES (app->preferences),
									MAXIMUM_RECENT_FILES);
	max_recent_prjs =
		anjuta_preferences_get_int (ANJUTA_PREFERENCES (app->preferences),
									MAXIMUM_RECENT_PROJECTS);
	list = app->text_editor_list;
	while (list)
	{
		te = list->data;
		if (te->full_filename)
			app->recent_files =
				glist_path_dedup(update_string_list (app->recent_files,
													 te->full_filename,
													 max_recent_files));
		if (!text_editor_is_saved (te))
			file_not_saved = TRUE;
		list = g_list_next (list);
	}
	if (app->project_dbase->project_is_open)
	{
		app->recent_projects =
				glist_path_dedup(update_string_list (app->recent_projects,
												app->project_dbase->proj_filename,
													 max_recent_files));
	}
	anjuta_app_save_settings (app);

	/* No need to check for saved project, as it is done in 
	close project call later. */
	if (file_not_saved)
	{
		GtkWidget *dialog;
		gint response;
		dialog = gtk_message_dialog_new (GTK_WINDOW (app),
										 GTK_DIALOG_DESTROY_WITH_PARENT,
										 GTK_MESSAGE_QUESTION,
										 GTK_BUTTONS_NONE,
										 _("One or more files are not saved.\n"
										 "Do you still want to exit?"));
		gtk_dialog_add_buttons (GTK_DIALOG (dialog),
								GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
								GTK_STOCK_QUIT,   GTK_RESPONSE_YES,
								NULL);
		response = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		if (response == GTK_RESPONSE_YES)
			anjuta_clean_exit ();
		return TRUE;
	}
	else
		anjuta_app_clean_exit (app);
	return TRUE;
}

void
on_anjuta_destroy (GtkWidget * w, gpointer data)
{
	/* Nothing to be done here */
}

void
on_anjuta_dnd_drop (gchar* filename, gpointer data)
{
	anjuta_goto_file_line (filename, 0);
}

gboolean
on_anjuta_window_focus_in_event (GtkWidget * widget,
				 GdkEventFocus * event, gpointer user_data)
{
	gint page_num;
	TextEditor *te;

	if (g_list_length (GTK_NOTEBOOK (app->notebook)->children) > 0)
	{
		page_num =
			gtk_notebook_get_current_page (GTK_NOTEBOOK
						       (app->notebook));
		if (page_num >= 0)
			te = anjuta_get_notebook_text_editor (page_num);
		else
			te = NULL;
	}
	else if (app->text_editor_list == NULL)
	{
		te = NULL;
	}
	else
	{
		/* If there is TextEditor in the list, but no page 
		 * * in the notebook, we don't change the current
		 * * TextEditor
		 */
		return FALSE;
	}
	if (te == anjuta_get_current_text_editor ())
		return FALSE;
	anjuta_set_current_text_editor (te);
	return FALSE;
}

enum {
	m___ = 0,
	mS__ = GDK_SHIFT_MASK,
	m_C_ = GDK_CONTROL_MASK,
	m__M = GDK_MOD1_MASK,
	mSC_ = GDK_SHIFT_MASK | GDK_CONTROL_MASK,
};

enum {
	ID_NEXTBUFFER = 1, /* Note: the value mustn't be 0 ! */
	ID_PREVBUFFER,
	ID_FIRSTBUFFER
};

typedef struct {
	int modifiers;
	unsigned int gdk_key;
	int id;
} ShortcutMapping;

static ShortcutMapping global_keymap[] = {
	{ m_C_, GDK_Tab,		 ID_NEXTBUFFER },
	{ mSC_, GDK_ISO_Left_Tab, ID_PREVBUFFER },
	{ m__M, GDK_1, ID_FIRSTBUFFER },
	{ m__M, GDK_2, ID_FIRSTBUFFER + 1},
	{ m__M, GDK_3, ID_FIRSTBUFFER + 2},
	{ m__M, GDK_4, ID_FIRSTBUFFER + 3},
	{ m__M, GDK_5, ID_FIRSTBUFFER + 4},
	{ m__M, GDK_6, ID_FIRSTBUFFER + 5},
	{ m__M, GDK_7, ID_FIRSTBUFFER + 6},
	{ m__M, GDK_8, ID_FIRSTBUFFER + 7},
	{ m__M, GDK_9, ID_FIRSTBUFFER + 8},
	{ m__M, GDK_0, ID_FIRSTBUFFER + 9},
	{ 0,   0,		 0 }
};

gint
on_anjuta_window_key_press_event (GtkWidget   *widget,
				  GdkEventKey *event,
				  gpointer     user_data)
{
	int modifiers;
	int i;

	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_APP (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	modifiers = event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK);
  
	for (i = 0; global_keymap[i].id; i++)
		if (event->keyval == global_keymap[i].gdk_key &&
		    (event->state & global_keymap[i].modifiers) == global_keymap[i].modifiers)
			break;

	if (!global_keymap[i].id)
		return FALSE;

	switch (global_keymap[i].id) {
	case ID_NEXTBUFFER:
	case ID_PREVBUFFER: {
		GtkNotebook *notebook = GTK_NOTEBOOK (app->notebook);
		int pages_nb;
		int cur_page;

		if (!notebook->children)
			return FALSE;

		if (!g_tabbing)
		{
			g_tabbing = TRUE;
		}

		pages_nb = g_list_length (notebook->children);
		cur_page = gtk_notebook_get_current_page (notebook);

		if (global_keymap[i].id == ID_NEXTBUFFER)
			cur_page = (cur_page < pages_nb - 1) ? cur_page + 1 : 0;
		else
			cur_page = cur_page ? cur_page - 1 : pages_nb -1;

		gtk_notebook_set_page (notebook, cur_page);

		break;
	}
	default:
		if (global_keymap[i].id >= ID_FIRSTBUFFER &&
		  global_keymap[i].id <= (ID_FIRSTBUFFER + 9))
		{
			GtkNotebook *notebook = GTK_NOTEBOOK (app->notebook);
			int page_req = global_keymap[i].id - ID_FIRSTBUFFER;

			if (!notebook->children)
				return FALSE;
			gtk_notebook_set_page(notebook, page_req);
		}
		else
			return FALSE;
	}

	/* Note: No reason for a shortcut to do more than one thing a time */
	gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "key_press_event");

	return TRUE;
}

gint
on_anjuta_window_key_release_event (GtkWidget   *widget,
				  GdkEventKey *event,
				  gpointer     user_data)
{
	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_APP (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	if (g_tabbing && ((event->keyval == GDK_Control_L) ||
		(event->keyval == GDK_Control_R)))
	{
		GtkNotebook *notebook = GTK_NOTEBOOK (app->notebook);
		GtkWidget *widget;
		int cur_page;
		g_tabbing = FALSE;
		
		if (anjuta_preferences_get_int (app->preferences,
										EDITOR_TABS_RECENT_FIRST))
		{
			/*
			TTimo: move the current notebook page to first position
			that maintains Ctrl-TABing on a list of most recently edited files
			*/
			cur_page = gtk_notebook_get_current_page (notebook);
			widget = gtk_notebook_get_nth_page (notebook, cur_page);
			gtk_notebook_reorder_child (notebook, widget, 0);
		}
	}
	return FALSE;
}

void
on_open_filesel_ok_clicked (GtkButton * button, gpointer user_data)
{
	gchar *full_filename;
	gchar *entry_filename = NULL;
	int i;
	GList * list;
	int elements;
	
	list = fileselection_get_filelist(app->fileselection);
	elements = g_list_length(list);
	/* If filename is only written in entry but not selected (Bug #506441) */
	if (elements == 0)
	{
		entry_filename = fileselection_get_filename(app->fileselection);
		if (entry_filename)
		{
			list = g_list_append(list, entry_filename);
			elements++;
		}
	}
	for(i=0;i<elements;i++)
	{
		full_filename = g_strdup(g_list_nth_data(list,i));
		if (!full_filename)
			return;
		anjuta_goto_file_line (full_filename, -1);
		g_free (full_filename);
	}

	if (entry_filename)
	{
		g_list_remove(list, entry_filename);
		g_free(entry_filename);
	}
	/* g_free(list); */
}

static void
save_as_real (void)
{
	TextEditor *te;
	gchar *full_filename, *saved_filename, *saved_full_filename;
	gint page_num;
	GtkWidget *child;
	gint status;

	full_filename = fileselection_get_filename (app->save_as_fileselection);
	if (!full_filename)
		return;

	te = anjuta_get_current_text_editor ();
	g_return_if_fail (te != NULL);
	g_return_if_fail (strlen (extract_filename (full_filename)) != 0);

	saved_filename = te->filename;
	saved_full_filename = te->full_filename;
	
	te->full_filename = g_strdup (full_filename);
	te->filename = g_strdup (extract_filename (full_filename));
	status = text_editor_save_file (te, TRUE);
	gtk_widget_hide (app->save_as_fileselection);
	if (status == FALSE)
	{
		g_free (te->filename);
		te->filename = saved_filename;
		g_free (te->full_filename);
		te->full_filename = saved_full_filename;
		if (closing_state) closing_state = FALSE;
		g_free (full_filename);
		return;
	} else {
		if (saved_filename)
			g_free (saved_filename);
		if (saved_full_filename)
		{
			g_free (saved_full_filename);
		}
		if (closing_state)
		{
			anjuta_remove_current_text_editor ();
			closing_state = FALSE;
		}
		else
		{
			text_editor_set_hilite_type(te);
			if (te->mode == TEXT_EDITOR_PAGED)
			{
				GtkLabel* label;
				
				page_num =
					gtk_notebook_get_current_page (GTK_NOTEBOOK
									   (app->notebook));
				g_return_if_fail (page_num >= 0);
				child =
					gtk_notebook_get_nth_page (GTK_NOTEBOOK
								   (app->notebook), page_num);
				/* This crashes */
				/* gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (app->notebook),
								 child,
								 anjuta_get_notebook_text_editor
								 (page_num)->filename);
				*/
				label = GTK_LABEL(te->widgets.tab_label);
				gtk_label_set_text(GTK_LABEL(label), anjuta_get_notebook_text_editor
								 (page_num)->filename);
	
				gtk_notebook_set_menu_label_text(GTK_NOTEBOOK(app->notebook),
								child,
								anjuta_get_notebook_text_editor
								(page_num)->filename);
			}
		}
		anjuta_update_title ();
		g_free (full_filename);
	}
}

void
on_save_as_filesel_ok_clicked (GtkButton * button, gpointer user_data)
{
	gchar *filename;

	filename = fileselection_get_filename (app->save_as_fileselection);
	if (file_is_regular (filename))
	{
		GtkWidget *dialog;
		dialog = gtk_message_dialog_new (GTK_WINDOW (app),
										 GTK_DIALOG_DESTROY_WITH_PARENT,
										 GTK_MESSAGE_QUESTION,
										 GTK_BUTTONS_NONE,
										 _("The file '%s' already exists.\n"
										 "Do you want to replace it with the one you are saving?"),
										 filename);
		gtk_dialog_add_button (GTK_DIALOG(dialog),
							   GTK_STOCK_CANCEL,
							   GTK_RESPONSE_CANCEL);
		anjuta_dialog_add_button (GTK_DIALOG (dialog),
								  _("_Replace"),
								  GTK_STOCK_REFRESH,
								  GTK_RESPONSE_YES);
		if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
			save_as_real ();
		gtk_widget_destroy (dialog);
	}
	else
		save_as_real ();
	g_free (filename);

	if (anjuta_preferences_get_int (ANJUTA_PREFERENCES (app->preferences),
									EDITOR_TABS_ORDERING))
		anjuta_order_tabs ();
}

void
on_save_as_filesel_cancel_clicked (GtkButton * button, gpointer user_data)
{
	gtk_widget_hide (app->save_as_fileselection);
	closing_state = FALSE;
}

static void
build_msg_save_real (void)
{
	gchar *filename;
	FILE *msgfile; 
	
	filename = fileselection_get_filename (app->save_as_build_msg_sel);
	msgfile = fopen(filename, "w");
	if (! msgfile)
	{
		anjuta_error (_("Could not open file for writing"));
		return;
	}
	
	an_message_manager_save_build(app->messages, msgfile);
			
	fclose(msgfile);

	return;
}

void
on_build_msg_save_ok_clicked(GtkButton * button, gpointer user_data)
{
	gchar *filename;

	filename = fileselection_get_filename (app->save_as_build_msg_sel);
	if (file_is_regular (filename))
	{
		GtkWidget *dialog;
		dialog = gtk_message_dialog_new (GTK_WINDOW (app),
										 GTK_DIALOG_DESTROY_WITH_PARENT,
										 GTK_MESSAGE_QUESTION,
										 GTK_BUTTONS_NONE,
										 _("The file '%s' already exists.\n"
										 "Do you want to replace it with the one you are saving?."),
										 filename);
		gtk_dialog_add_button (GTK_DIALOG (dialog),
							   GTK_STOCK_CANCEL,
							   GTK_RESPONSE_CANCEL);
		anjuta_dialog_add_button (GTK_DIALOG (dialog),
								  _("_Replace"),
								  GTK_STOCK_REFRESH,
								  GTK_RESPONSE_YES);
		if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
			build_msg_save_real ();
		gtk_widget_destroy (dialog);
	}
	else
		build_msg_save_real ();
	g_free (filename);
}

void
on_build_msg_save_cancel_clicked(GtkButton * button, gpointer user_data)
{
	gtk_widget_hide (app->save_as_build_msg_sel);
	closing_state = FALSE;
}

#if 0
void
on_prj_list_undock_button_clicked (GtkButton * button, gpointer user_data)
{
	project_dbase_undock (app->project_dbase);
}


void
on_prj_list_hide_button_clicked (GtkButton * button, gpointer user_data)
{
	project_dbase_hide (app->project_dbase);
}


void
on_mesg_win_hide_button_clicked (GtkButton * button, gpointer user_data)
{
	gtk_widget_hide(GTK_WIDGET(app->messages));
}


void
on_mesg_win_undock_button_clicked (GtkButton * button, gpointer user_data)
{
	an_message_manager_undock (app->messages);
}

#endif

void
on_anjuta_progress_cancel (gpointer data)
{
	app->in_progress = FALSE;
	app->progress_cancel_cb (data);
	return;
}

gdouble
on_anjuta_progress_cb (gpointer data)
{
	return (app->progress_value);
}
