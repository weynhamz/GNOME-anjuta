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
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/plugins.h>
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
	gtk_widget_hide (w);
	gtk_main_quit ();
}

static gint
on_anjuta_load_session_on_idle (gpointer data)
{
	GQueue *commandline_args;
	GList *args, *node;
	AnjutaApp *app = (AnjutaApp*)data;
	
	args = g_object_get_data (G_OBJECT(app), "command-args");
	if (!args)
		return FALSE;
	
	commandline_args = g_queue_new ();
	
	node = args;
	while (node)
	{
		g_queue_push_head (commandline_args, node->data);
		node = g_list_next (node);
	}
	while (gtk_events_pending ())
	{
		gtk_main_iteration ();
	}
	g_signal_emit_by_name (G_OBJECT (app), "load_session", commandline_args);
	anjuta_util_glist_strings_free (args);
	g_queue_free (commandline_args);
	return FALSE;
}

/* Saves the current anjuta session */
static gint
on_anjuta_session_save_yourself (GnomeClient * client, gint phase,
									  GnomeSaveStyle s_style, gint shutdown,
									  GnomeInteractStyle i_style, gint fast,
									  gpointer app)
{
	gchar *argv[] = { "rm",	"-rf", NULL};
	gchar geometry[256];
	const gchar *prefix;
	gchar **res_argv;
	gint res_argc;
	GQueue *commandline_args;
	gint i, width, height, posx, posy;
	
	DEBUG_PRINT ("Going to save session ...");

	prefix = gnome_client_get_config_prefix (client);
	argv[2] = gnome_config_get_real_path (prefix);
	gnome_client_set_discard_command (client, 3, argv);
	gnome_client_set_restart_style  (client, GNOME_RESTART_IF_RUNNING);
	
	/* We want to be somewhere at last to start, otherwise bonobo-activation and
	   gnome-vfs gets screwed up at start up */
	gnome_client_set_priority (client, 80);
	
	commandline_args = g_queue_new ();
	g_signal_emit_by_name (G_OBJECT (app), "save_session", commandline_args);

	/* Prepare restart command */
	res_argc = g_queue_get_length(commandline_args) + 2;
	res_argv = g_malloc (res_argc * sizeof (char*));
	res_argv[0] = "anjuta";
	
	/* Get window geometry */
	gtk_window_get_size (GTK_WINDOW (app), &width, &height);
	posx = posy = 500;
	if (GTK_WIDGET(app)->window)
		gdk_window_get_origin (GTK_WIDGET(app)->window, &posx, &posy);
	snprintf (geometry, 256, "--geometry=%dx%d+%d+%d", width, height,
			  posx, posy);
	res_argv[1] = geometry;
	
	/* Prepare other commandline args */
	for (i = 2; i < res_argc; i++)
	{
		res_argv[i] = g_queue_peek_nth (commandline_args, i - 2);
	}
	gnome_client_set_restart_command(client, res_argc, res_argv);
	g_free (res_argv);
	// anjuta_save_settings ();	
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
			   app);
	gtk_signal_connect(GTK_OBJECT(client), "die",
			   GTK_SIGNAL_FUNC(on_anjuta_session_die), app);

	flags = gnome_client_get_flags(client);

	if (!prog_args)
	{
		/* Restore from last state */
	}
	if (prog_args)
	{
		/* Restore session */
		g_object_set_data (G_OBJECT (app), "command-args", prog_args); 
		gtk_idle_add (on_anjuta_load_session_on_idle, app);
	}
	/* Set layout */
	anjuta_app_load_layout (app, NULL);
	return app;
}
