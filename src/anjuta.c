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

static void
anjuta_save_default_session (GtkWidget * w)
{
	GQueue *args;
	gint width, height, posx, posy;
	gint args_len, i;
	GString *pref_val;
	gchar *pref_val_str;
	gboolean first_entry;
	AnjutaPreferences *prefs;
	
	/* Save window geometry */
	width = height = posx = posy = 0;
	if (GTK_WIDGET(w)->window)
	{
		gtk_window_get_size (GTK_WINDOW (w), &width, &height);
		gdk_window_get_origin (GTK_WIDGET(w)->window, &posx, &posy);
	}
	prefs = ANJUTA_APP (w)->preferences;
	anjuta_preferences_set_int (prefs, "anjuta_window_width", width);
	anjuta_preferences_set_int (prefs, "anjuta_window_height", height);
	anjuta_preferences_set_int (prefs, "anjuta_window_posx", posx);
	anjuta_preferences_set_int (prefs, "anjuta_window_posy", posy);
	
	/* Save session args */
	args = g_queue_new ();
	g_signal_emit_by_name (G_OBJECT (w), "save_session", args);
	args_len = g_queue_get_length(args);
	
	pref_val = g_string_new("");
	first_entry = TRUE;
	for (i = 0; i < args_len; i++)
	{
		gchar *arg;
		arg = g_queue_peek_nth (args, i);
		if (arg && strlen(arg) > 0)
		{
			if (first_entry)
				first_entry = FALSE;
			else
				pref_val = g_string_append (pref_val, "%%%");
			pref_val = g_string_append (pref_val, arg);
		}
		g_free (arg);
	}
	pref_val_str = g_string_free (pref_val, FALSE);
	anjuta_preferences_set (prefs, "anjuta_last_opened_files", pref_val_str);
	g_queue_free (args);
	g_free (pref_val_str);
}

static gboolean
on_anjuta_delete_event (GtkWidget *w, GdkEvent *event, gpointer data)
{
	const gchar *proper_shutdown;
	
	DEBUG_PRINT ("AnjutaApp delete event");

	anjuta_save_default_session (w);
	proper_shutdown = g_object_get_data (G_OBJECT (w), "__proper_shutdown");
	
	if (proper_shutdown)
	{
		gtk_widget_hide (w);
		anjuta_plugins_unload_all (ANJUTA_SHELL (w));
	}
	
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
	
	/* We want to be somewhere at last to start, otherwise bonobo-activation
	 * and gnome-vfs gets screwed up at start up
	 */
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
		g_object_set_data (G_OBJECT (app), "__proper_shutdown", "1");
	}
	g_signal_connect (G_OBJECT (app), "delete_event",
					  G_CALLBACK (on_anjuta_delete_event), NULL);
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

	if (prog_args)
	{
		/* Restore session */
		g_object_set_data (G_OBJECT (app), "command-args", prog_args);
		gtk_idle_add (on_anjuta_load_session_on_idle, app);
	}
	else
	{
		/* Restore from last default session */
		gchar *saved_args;
		gchar **argv, **arg;
		GList *args_list;
		
		args_list = NULL;
		saved_args = anjuta_preferences_get (app->preferences,
											 "anjuta_last_opened_files");
		if (saved_args && strlen (saved_args) > 0)
		{
			argv = g_strsplit (saved_args, "%%%", -1);
			arg = argv;
			while (*arg)
			{
				if (*arg && strlen (*arg) > 0)
					args_list = g_list_prepend (args_list, g_strdup (*arg));
				arg++;
			}
			g_strfreev (argv);
			args_list = g_list_reverse (args_list);
		}
		if (args_list)
		{
			g_object_set_data (G_OBJECT (app), "command-args", args_list);
			gtk_idle_add (on_anjuta_load_session_on_idle, app);
		}
	}
	/* Set layout */
	anjuta_app_load_layout (app, NULL);
	return app;
}

void
anjuta_set_window_geometry (AnjutaApp *app, const gchar *anjuta_geometry)
{
	gint pos_x, pos_y, width, height;
	gboolean geometry_set = FALSE;
	
	AnjutaPreferences *prefs;
	GdkGeometry size_hints = {
    	100, 100, 0, 0, 100, 100, 0, 0, 0.0, 0.0, GDK_GRAVITY_NORTH_WEST  
  	};
	
	prefs = app->preferences;
	
	gtk_window_set_geometry_hints (GTK_WINDOW (app), GTK_WIDGET (app),
								   &size_hints, GDK_HINT_RESIZE_INC);
	if (anjuta_geometry)
	{
		DEBUG_PRINT ("Setting geometry: %s", anjuta_geometry);
		/* Parse geometry doesn't seem to work here :( */
		/* gtk_window_parse_geometry (GTK_WINDOW (app), anjuta_geometry); */
		if (sscanf (anjuta_geometry, "%dx%d+%d+%d", &width, &height,
			&pos_x, &pos_y) == 4)
		{
			gtk_window_set_default_size (GTK_WINDOW (app), width, height);
			gtk_window_move (GTK_WINDOW (app), pos_x, pos_y);
			geometry_set = TRUE;
		}
		else
		{
			g_warning ("Failed to parse geometry: %s", anjuta_geometry);
		}
	}
	
	if (!geometry_set &&
		anjuta_preferences_get_int (prefs, "anjuta_window_width") > 0)
	{
		width = anjuta_preferences_get_int (prefs, "anjuta_window_width");
		height = anjuta_preferences_get_int (prefs, "anjuta_window_height");
		pos_x = anjuta_preferences_get_int (prefs, "anjuta_window_posx");
		pos_y = anjuta_preferences_get_int (prefs, "anjuta_window_posy");
		
		gtk_window_set_default_size (GTK_WINDOW (app), width, height);
		gtk_window_move (GTK_WINDOW (app), pos_x, pos_y);
		geometry_set = TRUE;
	}
	
	if (!geometry_set)
	{
		pos_x = 10;
		pos_y = 10;
		width = gdk_screen_width () - 10;
		height = gdk_screen_height () - 25;
		width = (width < 790)? width : 790;
		height = (height < 575)? width : 575;
		gtk_window_set_default_size (GTK_WINDOW (app), width, height);
		gtk_window_move (GTK_WINDOW (app), pos_x, pos_y);
	}
}
