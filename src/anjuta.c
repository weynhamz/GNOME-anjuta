/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta.c
 * Copyright (C) 2000 Naba Kumar  <naba@gnome.org>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59 
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-profile.h>

#include "anjuta.h"

static gboolean
on_anjuta_delete_event (GtkWidget *w, GdkEvent *event, gpointer data)
{
	DEBUG_PRINT ("AnjutaApp delete event");

	gtk_widget_hide (w);
	anjuta_plugins_unload_all (ANJUTA_SHELL (w));
	return FALSE;

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
}

static void
on_anjuta_destroy (GtkWidget * w, gpointer data)
{
	DEBUG_PRINT ("AnjutaApp destroy event");
	/* Nothing to be done here */
	gtk_main_quit ();
}

static gint
on_anjuta_restore_session_on_idle (gpointer data)
{
	GList *args;
	AnjutaApp *app = (AnjutaApp*)data;
	GnomeClient* client = gnome_master_client();
	
	args = g_object_get_data (G_OBJECT(app), "command-args");
	if (!args)
		return FALSE;
	while (gtk_events_pending ())
	{
		gtk_main_iteration ();
	}
	// FIXME: anjuta_session_restore(client);
	anjuta_util_glist_strings_free (args);
	return FALSE;
}

static gint
on_anjuta_load_command_lines_on_idle(gpointer data)
{
	GList *args;
	AnjutaApp *app = (AnjutaApp*)data;
	args = g_object_get_data (G_OBJECT(app), "command-args");
	
	if (!args)
		return FALSE;
	// int argc = (int)data;
	// while (gtk_events_pending ())
	//{
	//	gtk_main_iteration ();
	//}
	// FIXME: anjuta_load_cmdline_files();
	// FIXME: if( ( 1 == argc ) &&	app->b_reload_last_project )
	//{
		// FIXME: anjuta_load_last_project();
	//}
	anjuta_util_glist_strings_free (args);
	return FALSE;
}

/* Saves the current anjuta session */
static gint
on_anjuta_session_save_yourself (GnomeClient * client, gint phase,
									  GnomeSaveStyle s_style, gint shutdown,
									  GnomeInteractStyle i_style, gint fast,
									  gpointer data)
{
#if 0
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
#endif
	return TRUE;
}

static gint
on_anjuta_session_die(GnomeClient * client, gpointer data)
{
	gtk_main_quit();
	return FALSE;
}

AnjutaApp*
anjuta_new (gchar *prog_name, GList *prog_args, ESplash *splash,
			gboolean proper_shutdown)
{
	AnjutaApp *app;
	GnomeClient *client;
	GnomeClientFlags flags;
	IAnjutaProfile *profile;
	
	/* Initialize application */
	app = ANJUTA_APP (anjuta_app_new ());

	if (proper_shutdown)
	{
		g_signal_connect (G_OBJECT (app), "delete_event",
						  G_CALLBACK (on_anjuta_delete_event), NULL);
	}
	g_signal_connect (G_OBJECT (app), "destroy",
					  G_CALLBACK (on_anjuta_destroy), NULL);
	
	/* Load plugins */
	profile = anjuta_shell_get_interface (ANJUTA_SHELL(app), IAnjutaProfile, NULL);
	if (!profile)
	{
		g_warning ("No profile could be loaded");
		exit(1);
	}
	ianjuta_profile_load (profile, splash, NULL);
	
	/* Session management */
	client = gnome_master_client();
	gtk_signal_connect(GTK_OBJECT(client), "save_yourself",
			   GTK_SIGNAL_FUNC(on_anjuta_session_save_yourself),
			   (gpointer) prog_name);
	gtk_signal_connect(GTK_OBJECT(client), "die",
			   GTK_SIGNAL_FUNC(on_anjuta_session_die), NULL);

	flags = gnome_client_get_flags(client);
	if (flags & GNOME_CLIENT_RESTORED) {
		/* Restore session */
		if (prog_args)
			g_object_set_data (G_OBJECT (app), "command-args", prog_args); 
		gtk_idle_add(on_anjuta_restore_session_on_idle, app);
	} else {
		/* Load commandline args */
		if (prog_args)
			g_object_set_data (G_OBJECT (app), "command-args", prog_args); 
		gtk_idle_add(on_anjuta_load_command_lines_on_idle, app);
	}
	/* Set layout */
	anjuta_app_load_layout (app, NULL);

	/* Connect the necessary kernal signals */
	// FIXME: anjuta_kernel_signals_connect ();
	return app;
}
