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

#include <string.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-save-prompt.h>
#include <libanjuta/anjuta-plugin-manager.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>
#include <libanjuta/interfaces/ianjuta-file.h>

#include "anjuta.h"

#define ANJUTA_REMEMBERED_PLUGINS "anjuta.remembered.plugins"
#define ANJUTA_SESSION_SKIP_LAST "anjuta.session.skip.last"
#define ANJUTA_SESSION_SKIP_LAST_FILES "anjuta.session.skip.last.files"
#define USER_SESSION_PATH_NEW (g_build_filename (g_get_home_dir (), ".anjuta", \
												 "session", NULL))

#define DEFAULT_PROFILE "file://"PACKAGE_DATA_DIR"/profiles/default.profile"
#define USER_PROFILE_NAME "user"

static gboolean
on_anjuta_delete_event (AnjutaApp *app, GdkEvent *event, gpointer data)
{
	AnjutaPluginManager *plugin_manager;
	AnjutaSavePrompt *save_prompt;
	gchar *session_dir;
	gchar *remembered_plugins;
	
	DEBUG_PRINT ("AnjutaApp delete event");
	
	plugin_manager = anjuta_shell_get_plugin_manager (ANJUTA_SHELL (app), NULL);
	
	/* Save remembered plugins */
	remembered_plugins =
		anjuta_plugin_manager_get_remembered_plugins (plugin_manager);
	anjuta_preferences_set (app->preferences,
							ANJUTA_REMEMBERED_PLUGINS,
							remembered_plugins);
	g_free (remembered_plugins);
	
	/* Check for unsaved data */
	save_prompt = anjuta_save_prompt_new (GTK_WINDOW (app));
	anjuta_shell_save_prompt (ANJUTA_SHELL (app), save_prompt, NULL);

	if (anjuta_save_prompt_get_items_count (save_prompt) > 0)
	{
		switch (gtk_dialog_run (GTK_DIALOG (save_prompt)))
		{
			case ANJUTA_SAVE_PROMPT_RESPONSE_CANCEL:
				gtk_widget_destroy (GTK_WIDGET (save_prompt));
				/* Do not exit now */
				return TRUE;
			case ANJUTA_SAVE_PROMPT_RESPONSE_DISCARD:
			case ANJUTA_SAVE_PROMPT_RESPONSE_SAVE_CLOSE:
				/* exit now */
				break;
		}
	}
	
	/* Save current session as default session */
	session_dir = USER_SESSION_PATH_NEW;
	anjuta_shell_session_save (ANJUTA_SHELL (app), session_dir, NULL);
	g_free (session_dir);

	gtk_widget_destroy (GTK_WIDGET (save_prompt));
	
	/* Shutdown */
	if (g_object_get_data (G_OBJECT (app), "__proper_shutdown"))
	{
		gtk_widget_hide (GTK_WIDGET (app));
		anjuta_plugin_manager_unload_all_plugins
			(anjuta_shell_get_plugin_manager (ANJUTA_SHELL (app), NULL));
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
	
	DEBUG_PRINT ("Going to save session...");

	prefix = gnome_client_get_config_prefix (client);
	argv[2] = gnome_config_get_real_path (prefix);
	gnome_client_set_discard_command (client, 3, argv);
	
	argv[0] = "anjuta";
	argv[1] = "--no-client";
	gnome_client_set_restart_command (client, 2, argv);
	gnome_client_set_restart_style (client, GNOME_RESTART_IF_RUNNING);
	
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

static void
on_profile_scoped (AnjutaProfileManager *profile_manager,
				   AnjutaProfile *profile, AnjutaApp *app)
{
	gchar *session_dir;
	AnjutaSession *session;
	
	DEBUG_PRINT ("Profile scoped: %s", anjuta_profile_get_name (profile));
	if (strcmp (anjuta_profile_get_name (profile), USER_PROFILE_NAME) != 0)
		return;
	
	DEBUG_PRINT ("User profile loaded; Restoring user session");
	
	/* If profile scoped to "user", restore user session */
	session_dir = USER_SESSION_PATH_NEW;
	
	/* Clear the files list since we don't want to load them */
	{
		session = anjuta_session_new (session_dir);
		anjuta_session_set_string_list (session, "File Loader",
										"Files", NULL);
		anjuta_session_sync (session);
		g_object_unref (session);
	}
	
	/* Restore session */
	anjuta_shell_session_load (ANJUTA_SHELL (app), session_dir, NULL);
	g_free (session_dir);
}

static void
on_profile_descoped (AnjutaProfileManager *profile_manager,
					 AnjutaProfile *profile, AnjutaApp *app)
{
	gchar *session_dir;
	
	DEBUG_PRINT ("Profile descoped: %s", anjuta_profile_get_name (profile));
	if (strcmp (anjuta_profile_get_name (profile), USER_PROFILE_NAME) != 0)
		return;
	
	DEBUG_PRINT ("User profile descoped; Saving user session");
	
	/* If profile descoped from is "user", save user session */
	session_dir = USER_SESSION_PATH_NEW;
	
	/* Save current session */
	anjuta_shell_session_save (ANJUTA_SHELL (app), session_dir, NULL);
	g_free (session_dir);
}

AnjutaApp*
anjuta_new (gchar *prog_name, GList *prog_args, gboolean no_splash,
			gboolean no_session, gboolean no_files,
			const gchar *im_file,
			gboolean proper_shutdown, const gchar *geometry)
{
	AnjutaPluginManager *plugin_manager;
	AnjutaProfileManager *profile_manager;
	gchar *session_dir;
	AnjutaApp *app;
	AnjutaStatus *status;
	GnomeClient *client;
	GnomeClientFlags flags;
	AnjutaProfile *profile;
	gchar *session_profile;
	gchar *remembered_plugins;
	const gchar *project_file = NULL;
	gchar *profile_name = NULL;
	GError *error = NULL;
	
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
	plugin_manager  = anjuta_shell_get_plugin_manager (ANJUTA_SHELL (app),
													   NULL);
	profile_manager = anjuta_shell_get_profile_manager (ANJUTA_SHELL (app),
														NULL);
	
	/* Restore remembered plugins */
	remembered_plugins =
		anjuta_preferences_get (app->preferences, ANJUTA_REMEMBERED_PLUGINS);
	if (remembered_plugins)
		anjuta_plugin_manager_set_remembered_plugins (plugin_manager,
													  remembered_plugins);
	g_free (remembered_plugins);
	
	/* Prepare profile */
	profile = anjuta_profile_new (USER_PROFILE_NAME, plugin_manager);
	anjuta_profile_add_plugins_from_xml (profile, DEFAULT_PROFILE,
										 TRUE, &error);
	if (error)
	{
		anjuta_util_dialog_error (GTK_WINDOW (app), "%s", error->message);
		g_error_free (error);
		error = NULL;
	}
	
	/* Load user session profile */
	profile_name = g_path_get_basename (DEFAULT_PROFILE);
	session_profile = g_build_filename (g_get_home_dir(), ".anjuta",
										profile_name, NULL);
	if (g_file_test (session_profile, G_FILE_TEST_EXISTS))
	{
		anjuta_profile_add_plugins_from_xml (profile, session_profile,
											 FALSE, &error);
		if (error)
		{
			anjuta_util_dialog_error (GTK_WINDOW (app), "%s", error->message);
			g_error_free (error);
			error = NULL;
		}
	}
	anjuta_profile_set_sync_uri (profile, session_profile); 
	g_free (session_profile);
	g_free (profile_name);
	
	/* Load profile */
	anjuta_profile_manager_freeze (profile_manager);
	anjuta_profile_manager_push (profile_manager, profile, &error);
	if (error)
	{
		anjuta_util_dialog_error (GTK_WINDOW (app), "%s", error->message);
		g_error_free (error);
		error = NULL;
	}
	
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
		GList *node, *files_load = NULL;
		
		/* Reset default session */
		session_dir = USER_SESSION_PATH_NEW;
		session = anjuta_session_new (session_dir);
		anjuta_session_clear (session);
		if (geometry)
			anjuta_session_set_string (session, "Anjuta", "Geometry",
									   geometry);
		
		/* Identify non-project files and set them for loading in session */
		node = prog_args;
		while (node)
		{
			const gchar *ext;
			const gchar *filename = node->data;
			
			ext = strrchr (filename, '.');
			
			if (!ext ||
				(strcmp (ext, ".anjuta") != 0 &&
				 strcmp (ext, ".prj") != 0))
			{
				files_load = g_list_prepend (files_load, node->data);
			}
			else if (project_file == NULL)
			{
				/* Pick up the first project file for loading later */
				project_file = filename;
			}
			node = g_list_next (node);
		}
		if (files_load)
		{
			anjuta_session_set_string_list (session, "File Loader", "Files",
											files_load);
			g_list_free (files_load);
		}
		anjuta_session_sync (session);
		g_object_unref (session);
	}
	else
	{
		AnjutaSession *session;
		
		/* Load user session */
		session_dir = USER_SESSION_PATH_NEW;
		
		/* If preferences is set to not load last session, clear it */
		if (no_session ||
			anjuta_preferences_get_int (app->preferences,
										ANJUTA_SESSION_SKIP_LAST))
		{
			/* Reset default session */
			session = anjuta_session_new (session_dir);
			anjuta_session_clear (session);
			g_object_unref (session);
		}
		/* If preferences is set to not load last project, clear it */
		else if (no_files ||
				 anjuta_preferences_get_int (app->preferences,
											 ANJUTA_SESSION_SKIP_LAST_FILES))
		{
			session = anjuta_session_new (session_dir);
			anjuta_session_set_string_list (session, "File Loader",
											"Files", NULL);
			anjuta_session_sync (session);
			g_object_unref (session);
		}
	}
	
	/* Restore session */
	anjuta_shell_session_load (ANJUTA_SHELL (app), session_dir, NULL);
	g_free (session_dir);

	/* Prepare for session save and load on profile change */
	g_signal_connect (profile_manager, "profile-scoped",
					  G_CALLBACK (on_profile_scoped), app);
	g_signal_connect (profile_manager, "profile-descoped",
					  G_CALLBACK (on_profile_descoped), app);
	
	/* Load project file */
	if (project_file)
	{
		IAnjutaFileLoader *loader;
		loader = anjuta_shell_get_interface (ANJUTA_SHELL (app),
											 IAnjutaFileLoader, NULL);
		ianjuta_file_loader_load (loader, project_file, FALSE, NULL);
	}
	anjuta_profile_manager_thaw (profile_manager, &error);
	if (error)
	{
		anjuta_util_dialog_error (GTK_WINDOW (app), "%s", error->message);
		g_error_free (error);
		error = NULL;
	}
	anjuta_status_progress_tick (status, NULL, _("Loaded Session..."));
	return app;
}
