/*
    find_in_files.c
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
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#include <gnome.h>
#include "anjuta.h"
#include "resources.h"
#include "launcher.h"
#include "find_in_files.h"
#include "utilities.h"


FindInFiles *
find_in_files_new ()
{
	FindInFiles *ff;
	ff = g_malloc (sizeof (FindInFiles));
	if (ff)
	{
		ff->regexp_history = NULL;
		ff->cur_row = 0;
		ff->is_showing = FALSE;
		ff->length = 0;
		ff->win_pos_x = FR_CENTRE;
		ff->win_pos_y = FR_CENTRE;
		create_find_in_files_gui (ff);
	}
	return ff;
}

void
find_in_files_destroy (FindInFiles * ff)
{
	gint i;
	if (ff)
	{
		gtk_widget_unref (ff->widgets.window);
		gtk_widget_unref (ff->widgets.file_entry);
		gtk_widget_unref (ff->widgets.file_combo);
		gtk_widget_unref (ff->widgets.clist);
		gtk_widget_unref (ff->widgets.case_sensitive_check);
		gtk_widget_unref (ff->widgets.add);
		gtk_widget_unref (ff->widgets.remove);
		gtk_widget_unref (ff->widgets.clear);
		gtk_widget_unref (ff->widgets.regexp_entry);
		gtk_widget_unref (ff->widgets.regexp_combo);
		gtk_widget_unref (ff->widgets.help);
		gtk_widget_unref (ff->widgets.ok);
		gtk_widget_unref (ff->widgets.cancel);
		if (ff->widgets.window)
			gtk_widget_destroy (ff->widgets.window);
		for (i = 0; i < g_list_length (ff->regexp_history); i++)
			g_free (g_list_nth_data (ff->regexp_history, i));
		if (ff->regexp_history)
			g_list_free (ff->regexp_history);
		g_free (ff);
	}
}

void
find_in_files_show (FindInFiles * ff)
{
	if (ff->regexp_history)
		gtk_combo_set_popdown_strings (GTK_COMBO
					       (ff->widgets.regexp_combo),
					       ff->regexp_history);
	if (ff->is_showing)
	{
		gdk_window_raise (ff->widgets.window->window);
		return;
	}
	gtk_widget_set_uposition (ff->widgets.window, ff->win_pos_x,
				  ff->win_pos_y);
	gtk_widget_show (ff->widgets.window);
	gtk_widget_grab_focus (ff->widgets.file_entry);
	ff->is_showing = TRUE;
}

void
find_in_files_hide (FindInFiles * ff)
{
	if (!ff)
		return;
	if (ff->is_showing == FALSE)
		return;
	gdk_window_get_root_origin (ff->widgets.window->window,
				    &ff->win_pos_x, &ff->win_pos_y);
	gtk_widget_hide (ff->widgets.window);
	ff->is_showing = FALSE;
}

void
find_in_files_save_session ( FindInFiles* ff, ProjectDBase *p )
{
	g_return_if_fail( NULL != p );
	g_return_if_fail( NULL != ff );
	session_save_strings( p, SECSTR(SECTION_FIND_IN_FILES), ff->regexp_history );
}

void
find_in_files_load_session( FindInFiles * ff, ProjectDBase *p )
{
	g_return_if_fail( NULL != p );
	g_return_if_fail( NULL != ff );
	ff->regexp_history = session_load_strings( p, SECSTR(SECTION_FIND_IN_FILES), ff->regexp_history );
}


/* Private */

void
find_in_files_process (FindInFiles * ff)
{
	gint i;
	gchar *command, *temp, *file;
	gboolean case_sensitive;

	if (anjuta_is_installed ("grep", TRUE) == FALSE)
		return;
	case_sensitive =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					      (ff->widgets.
					       case_sensitive_check));
	temp = gtk_entry_get_text (GTK_ENTRY (ff->widgets.regexp_entry));
	command = g_strconcat ("grep -n -r -e \"", temp, "\"", NULL);
	ff->regexp_history =
		update_string_list (ff->regexp_history, temp,
				    COMBO_LIST_LENGTH);
	if (!case_sensitive)
	{
		temp = g_strconcat (command, " -i ", NULL);
		g_free (command);
		command = temp;
	}
	for (i = 0; i < ff->length; i++)
	{
		gtk_clist_get_text (GTK_CLIST (ff->widgets.clist), i, 0,
				    &file);
		temp = g_strconcat (command, " ", file, " ", NULL);
		g_free (command);
		command = temp;
	}
	anjuta_clear_execution_dir();
	if (launcher_execute (command,
			      find_in_files_mesg_arrived,
			      find_in_files_mesg_arrived,
			      find_in_files_terminated) == FALSE)
	{
		g_free (command);
		return;
	}
	anjuta_update_app_status (TRUE, _("Find in files"));
	g_free (command);
	anjuta_message_manager_clear (app->messages, MESSAGE_FIND);
	anjuta_message_manager_append (app->messages, _("Finding in Files ....\n"),
			 MESSAGE_FIND);
	anjuta_message_manager_show (app->messages, MESSAGE_FIND);
}

void
find_in_files_mesg_arrived (gchar * mesg)
{
	anjuta_message_manager_append (app->messages, mesg, MESSAGE_FIND);
}

void
find_in_files_terminated (int status, time_t time)
{
	gchar *buff1;

	if (WEXITSTATUS (status))
	{
		anjuta_message_manager_append (app->messages,
				 _
				 ("Find in Files completed...............Unsuccessful\n"),
				 MESSAGE_FIND);
		anjuta_warning (_
				("Find in Files completed ... unsuccessful"));
	}
	else
	{
		anjuta_message_manager_append (app->messages,
				 _
				 ("Find in Files completed...............Successful\n"),
				 MESSAGE_FIND);
		anjuta_status (_("Find in Files completed ... successful"));
	}
	buff1 =
		g_strdup_printf (_("Total time taken: %d secs\n"),
				 (gint) time);
	anjuta_message_manager_append (app->messages, buff1, MESSAGE_FIND);
	if (preferences_get_int (app->preferences, BEEP_ON_BUILD_COMPLETE))
		gdk_beep ();
	g_free (buff1);
	anjuta_update_app_status (TRUE, NULL);
}
