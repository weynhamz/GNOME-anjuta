/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

#include "anjuta-app.h"
#include "anjuta-callbacks.h"

gint
on_anjuta_app_delete_event (GtkWidget *w, GdkEvent *event, gpointer data)
{
#if 0
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
#endif
	return FALSE;
}

void
on_anjuta_app_destroy_event (GtkWidget * w, gpointer data)
{
	/* Nothing to be done here */
	gtk_main_quit ();
}

#if 0

gint
on_anjuta_session_die(GnomeClient * client, gpointer data)
{
	gtk_main_quit();
	return FALSE;
}

/* Saves the current anjuta session */
gint on_anjuta_session_save_yourself (GnomeClient * client, gint phase,
									  GnomeSaveStyle s_style, gint shutdown,
									  GnomeInteractStyle i_style, gint fast,
									  gpointer data)
{
	gchar *argv[] = { "rm",	"-rf", NULL};
	const gchar *prefix;
	gchar **res_argv;
	gint res_argc, counter;
	GList* node;

#ifdef DEBUG
	g_message ("Going to save session ...");
#endif
	prefix = gnome_client_get_config_prefix (client);
	argv[2] = gnome_config_get_real_path (prefix);
	gnome_client_set_discard_command (client, 3, argv);

	res_argc = g_list_length (app->text_editor_list) + 1;
	if (app->project_dbase->project_is_open)
		res_argc++;
	res_argv = g_malloc (res_argc * sizeof (char*));
	res_argv[0] = data;
	counter = 1;
	if (app->project_dbase->project_is_open)
	{
		res_argv[counter] = app->project_dbase->proj_filename;
		counter++;
	}
	node = app->text_editor_list;
	while (node)
	{
		TextEditor* te;
		
		te = node->data;
		if (te->full_filename)
		{
			res_argv[counter] = te->full_filename;
			counter++;
		}
		node = g_list_next (node);
	}
	gnome_client_set_restart_command(client, res_argc, res_argv);
	g_free (res_argv);
	anjuta_save_settings ();	
	return TRUE;
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
#endif
