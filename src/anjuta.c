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

#define ANJUTA_SESSION_SKIP_LAST "anjuta.session.skip.last"

static gboolean
on_anjuta_delete_event (AnjutaApp *app, GdkEvent *event, gpointer data)
{
	gchar *session_dir;
	DEBUG_PRINT ("AnjutaApp delete event");
	
	/* FIXME: Check for unsaved data */

	/* Save current session as default session */
	session_dir = g_build_filename (g_get_home_dir (), ".anjuta",
									"session", NULL);
	anjuta_shell_session_save (ANJUTA_SHELL (app), session_dir, NULL);
	g_free (session_dir);

	/* Shutdown */
	if (g_object_get_data (G_OBJECT (app), "__proper_shutdown"))
	{
		gtk_widget_hide (GTK_WIDGET (app));
		anjuta_plugins_unload_all (ANJUTA_SHELL (app));
	}
	return FALSE;
}

static void
on_anjuta_destroy (GtkWidget * w, gpointer data)
{
	DEBUG_PRINT ("AnjutaApp destroy event");
	
	gtk_widget_hide (w);
	gtk_main_quit ();
}

/* Saves the current anjuta session */
static gint
on_anjuta_session_save_yourself (GnomeClient * client, gint phase,
								 GnomeSaveStyle s_style, gint shutdown,
								 GnomeInteractStyle i_style, gint fast,
								 gpointer app)
{
	gchar *argv[] = { "rm",	"-rf", NULL};
	const gchar *prefix;
	
	DEBUG_PRINT ("Going to save session ...");

	prefix = gnome_client_get_config_prefix (client);
	argv[2] = gnome_config_get_real_path (prefix);
	gnome_client_set_discard_command (client, 3, argv);
	
	argv[0] = "anjuta";
	gnome_client_set_restart_command(client, 1, argv);
	gnome_client_set_restart_style  (client, GNOME_RESTART_IF_RUNNING);
	
	/*
	 * We want to be somewhere at last to start, otherwise bonobo-activation
	 * and gnome-vfs gets screwed up at start up
	 */
	gnome_client_set_priority (client, 80);
	
	/* Save current session */
	anjuta_shell_session_save (ANJUTA_SHELL (app), argv[2], NULL);
	g_free (argv[2]);
	
	return TRUE;
}

static gint
on_anjuta_session_die (GnomeClient * client, gpointer data)
{
	gtk_main_quit();
	return FALSE;
}

AnjutaApp*
anjuta_new (gchar *prog_name, GList *prog_args, gboolean no_splash,
			const gchar *im_file,
			gboolean proper_shutdown, const gchar *geometry)
{
	gchar *session_dir;
	AnjutaApp *app;
	AnjutaStatus *status;
	GnomeClient *client;
	GnomeClientFlags flags;
	IAnjutaProfile *profile;
	
	/* Initialize application */
	app = ANJUTA_APP (anjuta_app_new ());
	status = anjuta_shell_get_status (ANJUTA_SHELL (app), NULL);
	anjuta_status_progress_add_ticks (status, 1);
	
	if (im_file)
		anjuta_status_progress_set_splash (status, im_file, 100);
	if (no_splash)
		anjuta_status_progress_disable_splash (status, no_splash);

	if (proper_shutdown)
	{
		g_object_set_data (G_OBJECT (app), "__proper_shutdown", "1");
	}
	g_signal_connect (G_OBJECT (app), "delete_event",
					  G_CALLBACK (on_anjuta_delete_event), NULL);
	g_signal_connect (G_OBJECT (app), "destroy",
					  G_CALLBACK (on_anjuta_destroy), NULL);
	
	/* Load plugins */
	profile = anjuta_shell_get_interface (ANJUTA_SHELL(app),
										  IAnjutaProfile, NULL);
	if (!profile)
	{
		g_warning ("No profile could be loaded");
		exit(1);
	}
	ianjuta_profile_load (profile, NULL);
	
	/* Session management */
	client = gnome_master_client();
	gtk_signal_connect(GTK_OBJECT(client), "save_yourself",
			   GTK_SIGNAL_FUNC(on_anjuta_session_save_yourself),
			   app);
	gtk_signal_connect(GTK_OBJECT(client), "die",
			   GTK_SIGNAL_FUNC(on_anjuta_session_die), app);
	
	flags = gnome_client_get_flags(client);
	
	if (flags & GNOME_CLIENT_RESTORED)
	{
		/* Load the last session */
		const gchar *prefix;
		prefix = gnome_client_get_config_prefix (client);
		session_dir = gnome_config_get_real_path (prefix);
	}
	else if (prog_args || geometry)
	{
		AnjutaSession *session;
		
		/* Reset default session */
		session_dir = g_build_filename (g_get_home_dir (), ".anjuta",
										"session", NULL);
		session = anjuta_session_new (session_dir);
		anjuta_session_clear (session);
		if (geometry)
			anjuta_session_set_string (session, "Anjuta", "Geometry",
									   geometry);
		if (prog_args)
			anjuta_session_set_string_list (session, "File Loader", "Files",
											prog_args);
	}
	else
	{
		AnjutaSession *session;
		
		/* Load default session */
		session_dir = g_build_filename (g_get_home_dir (), ".anjuta",
										"session", NULL);
		/* If preferences is not set to load last session, clear it */
		if (anjuta_preferences_get_int (app->preferences,
										ANJUTA_SESSION_SKIP_LAST))
		{
			/* Reset default session */
			session_dir = g_build_filename (g_get_home_dir (), ".anjuta",
											"session", NULL);
			session = anjuta_session_new (session_dir);
			anjuta_session_clear (session);
		}
	}
	
	/* Restore session */
	anjuta_shell_session_load (ANJUTA_SHELL (app), session_dir, NULL);
	g_free (session_dir);
	anjuta_status_progress_tick (status, NULL, _("Loaded Session ..."));
	return app;
}
