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
#include "find_in_files_cbs.h"
#include "utilities.h"

static void
create_find_in_files_gui (FindInFiles *sf)
{
	GtkWidget *button;
	GtkListStore *list_store;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;
	
	sf->gxml = glade_xml_new (GLADE_FILE_ANJUTA, "find_in_files_dialog", NULL);
	glade_xml_signal_autoconnect (sf->gxml);
	sf->widgets.window =
		glade_xml_get_widget (sf->gxml, "find_in_files_dialog");
	gtk_widget_hide (sf->widgets.window);
	sf->widgets.file_entry =
		glade_xml_get_widget (sf->gxml, "find_in_files_file_entry");
	sf->widgets.clist =
		glade_xml_get_widget (sf->gxml, "find_in_files_clist");
	sf->widgets.case_sensitive_check =
		glade_xml_get_widget (sf->gxml, "find_in_files_case_sensitive");
	sf->widgets.ignore_binary =
		glade_xml_get_widget (sf->gxml, "find_in_files_ignore_binary");
	sf->widgets.nocvs =
		glade_xml_get_widget (sf->gxml, "find_in_files_ignore_cvs_directory");
	sf->widgets.append_messages =
		glade_xml_get_widget (sf->gxml, "find_in_files_append_messages");
	sf->widgets.regexp_entry =
		glade_xml_get_widget (sf->gxml, "find_in_files_regex_entry");
	sf->widgets.regexp_combo =
		glade_xml_get_widget (sf->gxml, "find_in_files_regex_combo");
		
	gtk_combo_set_case_sensitive (GTK_COMBO (sf->widgets.regexp_combo), TRUE);	
	
	/* Set up list of files */
	list_store = gtk_list_store_new (1, G_TYPE_STRING);

	gtk_tree_view_set_model (GTK_TREE_VIEW (sf->widgets.clist),
							 GTK_TREE_MODEL (list_store));
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Files",
													   renderer,
													   "text",
													   0,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (sf->widgets.clist), column);

	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (sf->widgets.clist));
	g_signal_connect (G_OBJECT (select), "changed",
					  G_CALLBACK (on_search_in_files_changed),
					  sf);

	gtk_widget_ref (sf->widgets.window);
	gtk_widget_ref (sf->widgets.file_entry);
	gtk_widget_ref (sf->widgets.clist);
	gtk_widget_ref (sf->widgets.case_sensitive_check);
	gtk_widget_ref (sf->widgets.ignore_binary);
	gtk_widget_ref (sf->widgets.nocvs);
	gtk_widget_ref (sf->widgets.append_messages);
	gtk_widget_ref (sf->widgets.regexp_entry);
	gtk_widget_ref (sf->widgets.regexp_combo);
	
	gtk_window_add_accel_group (GTK_WINDOW (sf->widgets.window), app->accel_group);

	g_signal_connect (G_OBJECT (sf->widgets.window), "delete_event",
					  G_CALLBACK (on_search_in_files_delete_event),
					  sf);
	button = glade_xml_get_widget (sf->gxml, "find_in_files_add");
	g_signal_connect (G_OBJECT (button), "clicked",
					  G_CALLBACK (on_search_in_files_add_clicked),
					  sf);
	button = glade_xml_get_widget (sf->gxml, "find_in_files_remove");
	g_signal_connect (G_OBJECT (button), "clicked",
					  G_CALLBACK (on_search_in_files_remove_clicked),
					  sf);
	button = glade_xml_get_widget (sf->gxml, "find_in_files_clear");
	g_signal_connect (G_OBJECT (button), "clicked",
					  G_CALLBACK (on_search_in_files_clear_clicked),
					  sf);

	g_signal_connect (G_OBJECT (sf->widgets.window), "response",
					  G_CALLBACK (on_search_in_files_response), 
					  sf);

	g_signal_connect (G_OBJECT (sf->widgets.window), "key-press-event",
					  G_CALLBACK (on_search_in_files_key_press),
					  sf);

	gtk_window_set_transient_for (GTK_WINDOW(sf->widgets.window),
	                              GTK_WINDOW(app->widgets.window));
}

FindInFiles *
find_in_files_new ()
{
	FindInFiles *ff;
	ff = g_malloc (sizeof (FindInFiles));
	if (ff)
	{
		ff->regexp_history = NULL;
		ff->cur_row = NULL;
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
	if (ff)
	{
		gtk_widget_unref (ff->widgets.window);
		gtk_widget_unref (ff->widgets.file_entry);
		gtk_widget_unref (ff->widgets.clist);
		gtk_widget_unref (ff->widgets.case_sensitive_check);
        gtk_widget_unref (ff->widgets.append_messages);
		gtk_widget_unref (ff->widgets.regexp_entry);
		gtk_widget_unref (ff->widgets.regexp_combo);

		if (ff->widgets.window)
			gtk_widget_destroy (ff->widgets.window);
		glist_strings_free (ff->regexp_history);
		g_object_unref (ff->gxml);
		g_free (ff);
	}
}

gboolean
find_in_files_save_yourself(FindInFiles* ff, FILE* stream)
{
    /* FIXME: Please save properties here */
    return TRUE;
}

gboolean
find_in_files_load_yourself(FindInFiles* ff, PropsID props)
{
    /* FIXME: Please load properties here */
    return TRUE;
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
	ff->is_showing = FALSE;
	gtk_widget_hide (ff->widgets.window);
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
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	gchar *command, *temp, *file;
	gboolean case_sensitive, ignore_binary, nocvs;

	if (anjuta_is_installed ("grep", TRUE) == FALSE)
		return;
	case_sensitive =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					      (ff->widgets.case_sensitive_check));
	ignore_binary  =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					      (ff->widgets.ignore_binary));
	nocvs  =
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					      (ff->widgets.nocvs));
	temp = (gchar*)gtk_entry_get_text (GTK_ENTRY (ff->widgets.regexp_entry));

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
	if (ignore_binary)
	{
		temp = g_strconcat (command, " -I ", NULL);
		g_free (command);
		command = temp;
	}
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ff->widgets.clist));
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while(valid)
	{
		gtk_tree_model_get (model, &iter, 0, &file, -1);
		valid = gtk_tree_model_iter_next (model, &iter);
		temp = g_strconcat (command, " ", file, " ", NULL);
		g_free (command);
		command = temp;
	}
	if (nocvs)
	{
		temp = g_strconcat(command, " | grep -Fv '/CVS/'", NULL);
		g_free(command);
		command = temp;
	}
#ifdef DEBUG
	g_message("Find: '%s'\n", command);
#endif

	anjuta_clear_execution_dir();
	g_signal_connect (app->launcher, "output-arrived",
					  G_CALLBACK (find_in_files_mesg_arrived), NULL);
	g_signal_connect (app->launcher, "child-exited",
					  G_CALLBACK (find_in_files_terminated), NULL);
	
	if (anjuta_launcher_execute (app->launcher, command) == FALSE)
	{
		g_free (command);
		return;
	}
	anjuta_update_app_status (TRUE, _("Find in Files"));
	if(!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
									  (ff->widgets.append_messages)))
	{
		an_message_manager_clear (app->messages, MESSAGE_FIND);
	}
	an_message_manager_append (app->messages, _("Finding in Files ....\n"),
							   MESSAGE_FIND);
	an_message_manager_show (app->messages, MESSAGE_FIND);
	g_free (command);
}

void
find_in_files_mesg_arrived (AnjutaLauncher *launcher,
							AnjutaLauncherOutputType output_type,
							const gchar * mesg, gpointer data)
{
	an_message_manager_append (app->messages, mesg, MESSAGE_FIND);
}

void
find_in_files_terminated (AnjutaLauncher *launcher,
						  gint child_pid, gint status, gulong time_taken,
						  gpointer data)
{
	gchar *buff1;

	g_signal_handlers_disconnect_by_func (G_OBJECT (launcher),
									  G_CALLBACK (find_in_files_mesg_arrived),
										  NULL);
	g_signal_handlers_disconnect_by_func (G_OBJECT (launcher),
										  G_CALLBACK (find_in_files_terminated),
										  NULL);
	if (status)
	{
		an_message_manager_append (app->messages,
				 _("Find in Files completed...............Unsuccessful\n"),
				 MESSAGE_FIND);
		anjuta_warning (_("Find in Files completed ... unsuccessful"));
	}
	else
	{
		an_message_manager_append (app->messages,
				 _("Find in Files completed...............Successful\n"),
				 MESSAGE_FIND);
		anjuta_status (_("Find in Files completed ... successful"));
	}
	buff1 =	g_strdup_printf (_("Total time taken: %lu secs\n"), time_taken);
	an_message_manager_append (app->messages, buff1, MESSAGE_FIND);
	if (anjuta_preferences_get_int (app->preferences, BEEP_ON_BUILD_COMPLETE))
		gdk_beep ();
	g_free (buff1);
	anjuta_update_app_status (TRUE, NULL);
}
