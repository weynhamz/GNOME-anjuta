/*
    anjuta_cbs.c
    Copyright (C) 2000  Kh. Naba Kumar Singh

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

#include "text_editor.h"
#include "anjuta.h"
#include "controls.h"
#include "fileselection.h"
#include "utilities.h"
#include "resources.h"
#include "messagebox.h"
#include "launcher.h"
#include "debugger.h"

/* Blurb ?? */
extern gboolean closing_state;

gint
on_anjuta_session_die(GnomeClient * client, gpointer data)
{
	gtk_main_quit();
	return FALSE;
}

/* Saves the current anjuta session */
gint on_anjuta_session_save_yourself (GnomeClient * client, gint phase,
		       GnomeSaveStyle s_style, gint shutdown,
		       GnomeInteractStyle i_style, gint fast, gpointer data)
{
	gchar *argv[] = { "rm",	"-rf", NULL};
	gchar *prefix, **res_argv;
	gint res_argc, counter;
	GList* node;

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

gint on_anjuta_delete (GtkWidget * w, GdkEvent * event, gpointer data)
{
	TextEditor *te;
	GList *list;
	gboolean file_not_saved;
	gint max_recent_files, max_recent_prjs;
	
	if (!app) return TRUE;
	file_not_saved = FALSE;
	max_recent_files = preferences_get_int (app->preferences, MAXIMUM_RECENT_FILES);
	max_recent_prjs = preferences_get_int (app->preferences, MAXIMUM_RECENT_PROJECTS);
	list = app->text_editor_list;
	while (list)
	{
		te = list->data;
		if (te->full_filename)
			app->recent_files = update_string_list (app->recent_files,
					te->full_filename, max_recent_files);
		if (!text_editor_is_saved (te))
			file_not_saved = TRUE;
		list = g_list_next (list);
	}
	if (app->project_dbase->project_is_open)
	{
		app->recent_projects =
			update_string_list (app->recent_projects,
					    app->project_dbase->proj_filename,
					    max_recent_prjs);
	}
	anjuta_save_settings ();

	if (file_not_saved || (!app->project_dbase->is_saved && app->project_dbase->project_is_open))
	{
		messagebox2 (GNOME_MESSAGE_BOX_QUESTION,
			     _("Some files (or the project) are not saved.\n"
				"Do you still want to exit?"),
			     GNOME_STOCK_BUTTON_YES,
			     GNOME_STOCK_BUTTON_NO,
			     GTK_SIGNAL_FUNC
			     (on_anjuta_exit_yes_clicked), NULL,
			     NULL);
		return TRUE;
	}
	else
	{
		on_anjuta_exit_yes_clicked (NULL, NULL);
	}
	return TRUE;
}

void
on_anjuta_exit_yes_clicked (GtkButton * b, gpointer data)
{
	anjuta_clean_exit ();
}

void
on_anjuta_destroy (GtkWidget * w, gpointer data)
{
	/* Nothing to be done here */
}

void
on_anjuta_notebook_switch_page (GtkNotebook * notebook,
				GtkNotebookPage * page,
				gint page_num, gpointer user_data)
{
	anjuta_set_current_text_editor (anjuta_get_notebook_text_editor
					(page_num));
	anjuta_grab_text_focus ();
}

void
on_open_filesel_ok_clicked (GtkButton * button, gpointer user_data)
{
	gchar *full_filename;
	full_filename = fileselection_get_filename (app->fileselection);
	if (!full_filename)
		return;
	if (strlen (extract_filename (full_filename)) == 0)
	{
		g_free (full_filename);
		return;
	}
	if (file_is_regular (full_filename) == FALSE)
	{
		anjuta_error (_("Not a regular file: %s."), full_filename);
		g_free (full_filename);
		return;
	}
	if (file_is_readable (full_filename) == FALSE)
	{
		anjuta_error (_("No read permission for: %s."), full_filename);
		g_free (full_filename);
		return;
	}
	anjuta_goto_file_line (full_filename, -1);
	gtk_widget_hide (app->fileselection);
	g_free (full_filename);
}

void
on_open_filesel_cancel_clicked (GtkButton * button, gpointer user_data)
{
	gtk_widget_hide (app->fileselection);
}

void
on_save_as_filesel_ok_clicked (GtkButton * button, gpointer user_data)
{
	gchar *filename, *buff;

	filename = fileselection_get_filename (app->save_as_fileselection);
	if (file_is_regular (filename))
	{
		buff =
			g_strdup_printf (_
					 ("The file \"%s\" already exist.\nDo you want to overwrite it?."),
					 filename);
		messagebox2 (GNOME_MESSAGE_BOX_QUESTION, buff,
			     GNOME_STOCK_BUTTON_YES, GNOME_STOCK_BUTTON_NO,
			     GTK_SIGNAL_FUNC
			     (on_save_as_overwrite_yes_clicked), NULL,
			     user_data);
		g_free (buff);
	}
	else
		on_save_as_overwrite_yes_clicked (NULL, user_data);
	g_free (filename);
}

void
on_save_as_overwrite_yes_clicked (GtkButton * button, gpointer user_data)
{
	TextEditor *te;
	gchar *full_filename;
	gint page_num;
	GtkWidget *child;

	full_filename = fileselection_get_filename (app->save_as_fileselection);
	if (!full_filename)
		return;

	te = anjuta_get_current_text_editor ();
	g_return_if_fail (te != NULL);
	g_return_if_fail (strlen (extract_filename (full_filename)) != 0);

	if (te->filename)
	{
		g_free (te->filename);
		te->filename = NULL;
	}
	if (te->full_filename)
	{
		if (app->project_dbase->project_is_open == FALSE)
			tags_manager_remove (app->tags_manager,
					     te->full_filename);
		g_free (te->full_filename);
		te->full_filename = NULL;
	}
	te->full_filename = g_strdup (full_filename);
	te->filename = g_strdup (extract_filename (full_filename));
	text_editor_save_file (te);
	gtk_widget_hide (app->save_as_fileselection);
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
			page_num =
				gtk_notebook_get_current_page (GTK_NOTEBOOK
								   (app->widgets.notebook));
			g_return_if_fail (page_num >= 0);
			child =
				gtk_notebook_get_nth_page (GTK_NOTEBOOK
							   (app->widgets.notebook), page_num);
			gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (app->widgets.notebook),
							 child,
							 anjuta_get_notebook_text_editor
							 (page_num)->filename);
		}
	}
	anjuta_update_title ();
	g_free (full_filename);
}

gboolean
on_anjuta_window_focus_in_event (GtkWidget * widget,
				 GdkEventFocus * event, gpointer user_data)
{
	gint page_num;
	TextEditor *te;

	if (g_list_length (GTK_NOTEBOOK (app->widgets.notebook)->children) >
	    0)
	{
		page_num =
			gtk_notebook_get_current_page (GTK_NOTEBOOK
						       (app->widgets.
							notebook));
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

void
on_save_as_filesel_cancel_clicked (GtkButton * button, gpointer user_data)
{
	gtk_widget_hide (app->save_as_fileselection);
	closing_state = FALSE;
}

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
	messages_hide (app->messages);
}


void
on_mesg_win_undock_button_clicked (GtkButton * button, gpointer user_data)
{
	messages_undock (app->messages);
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
