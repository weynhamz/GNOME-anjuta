/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-application.c
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <config.h>

#include <string.h>
#include <stdlib.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-save-prompt.h>
#include <libanjuta/anjuta-plugin-manager.h>
#include <libanjuta/resources.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>
#include <libanjuta/interfaces/ianjuta-file.h>

#include "anjuta-application.h"

#define ANJUTA_REMEMBERED_PLUGINS "remembered-plugins"
#define ANJUTA_SESSION_SKIP_LAST "session-skip-last"
#define ANJUTA_SESSION_SKIP_LAST_FILES "session-skip-last-files"
#define USER_SESSION_PATH_NEW (anjuta_util_get_user_cache_file_path ("session/", NULL))

#define DEFAULT_PROFILE "file://"PACKAGE_DATA_DIR"/profiles/default.profile"
#define USER_PROFILE_NAME "user"

#define ANJUTA_PIXMAP_SPLASH_SCREEN       "anjuta_splash.png"


G_DEFINE_TYPE (AnjutaApplication, anjuta_application, GTK_TYPE_APPLICATION)

static gchar *system_restore_session = NULL;

struct _AnjutaApplicationPrivate {
	gboolean no_splash;
	gboolean proper_shutdown;
};

static gboolean
on_anjuta_delete_event (AnjutaWindow *win, GdkEvent *event, gpointer user_data)
{
	AnjutaApplication *app = ANJUTA_APPLICATION (user_data);
	AnjutaPluginManager *plugin_manager;
	AnjutaProfileManager *profile_manager;
	AnjutaProfile *current_profile;
	AnjutaSavePrompt *save_prompt;
	gchar *remembered_plugins;

	DEBUG_PRINT ("%s", "AnjutaWindow delete event");

	plugin_manager = anjuta_shell_get_plugin_manager (ANJUTA_SHELL (win), NULL);
	profile_manager = anjuta_shell_get_profile_manager (ANJUTA_SHELL (win), NULL);

	/* Save remembered plugins */
	remembered_plugins =
		anjuta_plugin_manager_get_remembered_plugins (plugin_manager);
	g_settings_set_string (win->settings,
	                       ANJUTA_REMEMBERED_PLUGINS,
	                       remembered_plugins);
	g_free (remembered_plugins);

	/* Check for unsaved data */
	save_prompt = anjuta_save_prompt_new (GTK_WINDOW (win));
	anjuta_shell_save_prompt (ANJUTA_SHELL (win), save_prompt, NULL);

	if (anjuta_save_prompt_get_items_count (save_prompt) > 0)
	{
		switch (gtk_dialog_run (GTK_DIALOG (save_prompt)))
		{
			case GTK_RESPONSE_DELETE_EVENT:
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
	/* Wait for files to be really saved (asyncronous operation) */
	if (win->save_count > 0)
	{
		g_message ("Waiting for %d file(s) to be saved!", win->save_count);
		while (win->save_count > 0)
		{
			g_main_context_iteration (NULL, TRUE);
		}
	}

	/* If current active profile is "user", save current session as
	 * default session
	 */
	current_profile = anjuta_profile_manager_get_current (profile_manager);
	if (strcmp (anjuta_profile_get_name (current_profile), "user") == 0)
	{
		gchar *session_dir;
		session_dir = USER_SESSION_PATH_NEW;
		anjuta_shell_session_save (ANJUTA_SHELL (win), session_dir, NULL);
		g_free (session_dir);
	}

	anjuta_shell_notify_exit (ANJUTA_SHELL (win), NULL);

	gtk_widget_destroy (GTK_WIDGET (save_prompt));

	/* Shutdown */
	if (anjuta_application_get_proper_shutdown (app))
	{
		gtk_widget_hide (GTK_WIDGET (win));
		anjuta_plugin_manager_unload_all_plugins
			(anjuta_shell_get_plugin_manager (ANJUTA_SHELL (win), NULL));
	}

	return FALSE;
}

static void
on_profile_scoped (AnjutaProfileManager *profile_manager,
				   AnjutaProfile *profile, AnjutaWindow *win)
{
	gchar *session_dir;
	static gboolean first_time = TRUE;

	DEBUG_PRINT ("Profile scoped: %s", anjuta_profile_get_name (profile));
	if (strcmp (anjuta_profile_get_name (profile), USER_PROFILE_NAME) != 0)
		return;

	DEBUG_PRINT ("%s", "User profile loaded; Restoring user session");

	/* If profile scoped to "user", restore user session */
	if (system_restore_session)
	{
		session_dir = system_restore_session;
		system_restore_session = NULL;
	}
	else
	{
		session_dir = USER_SESSION_PATH_NEW;
	}

	if (first_time)
	{
		first_time = FALSE;
	}
	else
	{
		/* Clear the files list since we don't want to load them later */
		AnjutaSession *session;
		session = anjuta_session_new (session_dir);
		anjuta_session_set_string_list (session, "File Loader",
										"Files", NULL);
		anjuta_session_sync (session);
		g_object_unref (session);
	}

	/* Restore session */
	anjuta_shell_session_load (ANJUTA_SHELL (win), session_dir, NULL);
	g_free (session_dir);
}

static void
on_profile_descoped (AnjutaProfileManager *profile_manager,
					 AnjutaProfile *profile, AnjutaWindow *win)
{
	gchar *session_dir;

	DEBUG_PRINT ("Profile descoped: %s", anjuta_profile_get_name (profile));
	if (strcmp (anjuta_profile_get_name (profile), USER_PROFILE_NAME) != 0)
		return;

	DEBUG_PRINT ("%s", "User profile descoped; Saving user session");

	/* If profile descoped from is "user", save user session */
	session_dir = USER_SESSION_PATH_NEW;

	/* Save current session */
	anjuta_shell_session_save (ANJUTA_SHELL (win), session_dir, NULL);
	g_free (session_dir);
}

static AnjutaWindow*
anjuta_application_create_window (AnjutaApplication *app,
                                  GFile **files, int n_files, gboolean no_splash,
                                  gboolean no_session, gboolean no_files,
                                  const gchar *geometry)
{
	AnjutaPluginManager *plugin_manager;
	AnjutaProfileManager *profile_manager;
	AnjutaWindow *win;
	AnjutaStatus *status;
	AnjutaProfile *profile;
	GFile *session_profile;
	gchar *remembered_plugins;
	gchar *profile_name = NULL;
	GError *error = NULL;

	/* Initialize application */
	win = ANJUTA_WINDOW (anjuta_window_new ());
	status = anjuta_shell_get_status (ANJUTA_SHELL (win), NULL);
	anjuta_status_progress_add_ticks (status, 1);

	gtk_window_set_role (GTK_WINDOW (win), "anjuta-app");
	gtk_window_set_auto_startup_notification(TRUE);
	gtk_window_set_default_icon_name ("anjuta");
	gtk_window_set_auto_startup_notification(FALSE);

	if (app->priv->no_splash)
	{
		anjuta_status_disable_splash (status, TRUE);
	}
	else
	{
		gchar *im_file = NULL;
		im_file = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_SPLASH_SCREEN);
		if (im_file)
		{
			anjuta_status_set_splash (status, im_file, 100);
			g_free (im_file);
		}
	}

	g_signal_connect (G_OBJECT (win), "delete_event",
					  G_CALLBACK (on_anjuta_delete_event), app);

	/* Setup application framework */
	plugin_manager  = anjuta_shell_get_plugin_manager (ANJUTA_SHELL (win),
													   NULL);
	profile_manager = anjuta_shell_get_profile_manager (ANJUTA_SHELL (win),
														NULL);

	/* Restore remembered plugins */
	remembered_plugins =
		g_settings_get_string (win->settings, ANJUTA_REMEMBERED_PLUGINS);
	if (remembered_plugins)
		anjuta_plugin_manager_set_remembered_plugins (plugin_manager,
													  remembered_plugins);
	g_free (remembered_plugins);

	/* Prepare profile */
	profile = anjuta_profile_new (USER_PROFILE_NAME, plugin_manager);
	session_profile = g_file_new_for_uri (DEFAULT_PROFILE);
	anjuta_profile_add_plugins_from_xml (profile, session_profile,
										 TRUE, &error);
	if (error)
	{
		anjuta_util_dialog_error (GTK_WINDOW (win), "%s", error->message);
		g_error_free (error);
		error = NULL;
	}
	g_object_unref (session_profile);

	/* Load user session profile */
	profile_name = g_path_get_basename (DEFAULT_PROFILE);
	session_profile = anjuta_util_get_user_cache_file (profile_name, NULL);
	if (g_file_query_exists (session_profile, NULL))
	{
		anjuta_profile_add_plugins_from_xml (profile, session_profile,
											 FALSE, &error);
		if (error)
		{
			anjuta_util_dialog_error (GTK_WINDOW (win), "%s", error->message);
			g_error_free (error);
			error = NULL;
		}
	}
	anjuta_profile_set_sync_file (profile, session_profile);
	g_object_unref (session_profile);
	g_free (profile_name);

	/* Load profile */
	anjuta_profile_manager_freeze (profile_manager);
	anjuta_profile_manager_push (profile_manager, profile, &error);
	if (error)
	{
		anjuta_util_dialog_error (GTK_WINDOW (win), "%s", error->message);
		g_error_free (error);
		error = NULL;
	}

	if (files || geometry)
	{
		gchar *session_dir;
		AnjutaSession *session;
		gint i;
		GList *files_load = NULL;

		/* Reset default session */
		session_dir = USER_SESSION_PATH_NEW;

		session = anjuta_session_new (session_dir);
		anjuta_session_clear (session);
		if (geometry)
			anjuta_session_set_string (session, "Anjuta", "Geometry",
									   geometry);

		/* Identify non-project files and set them for loading in session */
		for (i = 0; i < n_files; i++)
		{
			gchar* file = g_file_get_path (files[i]);
			if (!file)
				continue;
			files_load = g_list_prepend (files_load, g_file_get_uri (files[i]));
		}
		if (files_load)
		{
			anjuta_session_set_string_list (session, "File Loader", "Files",
											files_load);
			g_list_foreach (files_load, (GFunc)g_free, NULL);
			g_list_free (files_load);
		}
		anjuta_session_sync (session);
		g_object_unref (session);
		g_free (session_dir);
	}
	else
	{
		gchar *session_dir;
		AnjutaSession *session;

		/* Load user session */
		session_dir = USER_SESSION_PATH_NEW;

		/* If preferences is set to not load last session, clear it */
		if (no_session ||
			g_settings_get_boolean (win->settings,
			                        ANJUTA_SESSION_SKIP_LAST))
		{
			/* Reset default session */
			session = anjuta_session_new (session_dir);
			anjuta_session_clear (session);
			g_object_unref (session);
		}
		/* If preferences is set to not load last project, clear it */
		else if (no_files ||
				g_settings_get_boolean (win->settings,
				                        ANJUTA_SESSION_SKIP_LAST_FILES))
		{
			session = anjuta_session_new (session_dir);
			anjuta_session_set_string_list (session, "File Loader",
											"Files", NULL);
			anjuta_session_sync (session);
			g_object_unref (session);
		}
		g_free (session_dir);
	}

	/* Prepare for session save and load on profile change */
	g_signal_connect (profile_manager, "profile-scoped",
					  G_CALLBACK (on_profile_scoped), win);
	anjuta_profile_manager_thaw (profile_manager, &error);

	if (error)
	{
		anjuta_util_dialog_error (GTK_WINDOW (win), "%s", error->message);
		g_error_free (error);
		error = NULL;
	}
	g_signal_connect (profile_manager, "profile-descoped",
					  G_CALLBACK (on_profile_descoped), win);

	anjuta_status_progress_tick (status, NULL, _("Loaded Session…"));
	anjuta_status_disable_splash (status, TRUE);

	return win;
}


/* GApplication implementation
 *---------------------------------------------------------------------------*/

static gboolean
anjuta_application_local_command_line (GApplication *application,
                                       gchar ***arguments,
                                       gint *exit_status)
{
	/* Command line options */
	gboolean no_splash = FALSE;
	gboolean no_client = FALSE;
	gboolean no_session = FALSE;
	gboolean no_files = FALSE;
	gboolean version = FALSE;
	gchar *geometry = NULL;
	gchar **filenames = NULL;

	const GOptionEntry anjuta_options[] = {
		{
			"geometry", 'g', 0, G_OPTION_ARG_STRING,
			&geometry,
			N_("Specify the size and location of the main window"),
			/* This is the format you can specify the size andposition
		 	 * of the window on command line */
			N_("WIDTHxHEIGHT+XOFF+YOFF")
		},
		{
			"no-splash", 's', 0, G_OPTION_ARG_NONE,
			&no_splash,
			N_("Do not show the splash screen"),
			NULL
		},
		{
			"no-client", 'c', 0, G_OPTION_ARG_NONE,
			&no_client,
			N_("Start a new instance and do not open the file in an existing instance"),
			NULL
		},
		{
			"no-session", 'n', 0, G_OPTION_ARG_NONE,
			&no_session,
			N_("Do not open last session on startup"),
			NULL
		},
		{
			"no-files", 'f', 0, G_OPTION_ARG_NONE,
			&no_files,
			N_("Do not open last project and files on startup"),
			NULL
		},
		{
			"proper-shutdown", 'p', 0, G_OPTION_ARG_NONE,
			&((ANJUTA_APPLICATION (application))->priv->proper_shutdown),
			N_("Shut down Anjuta properly, releasing all resources (for debugging)"),
			NULL
		},
		{
			"version", 'v', 0, G_OPTION_ARG_NONE,
			&version,
			N_("Display program version"),
			NULL
		},
		{
			G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY,
			&filenames,
			NULL,
			NULL
		},
		{NULL}
	};
	GOptionContext *context;
	gint argc = 0;
	gchar **argv = NULL;
	GError *error = NULL;
	GPtrArray *files;

	context = g_option_context_new (_("- Integrated Development Environment"));
#ifdef ENABLE_NLS
	g_option_context_add_main_entries (context, anjuta_options, GETTEXT_PACKAGE);
#else
	g_option_context_add_main_entries (context, anjuta_options, NULL);
#endif
	g_option_context_add_group (context, gtk_get_option_group (TRUE));

	/* Parse arguments */
	argv = *arguments;
	argc = g_strv_length (argv);
	if (!g_option_context_parse (context, &argc, &argv, &error))
	{
		g_printerr ("Could not parse arguments: %s\n", error->message);
		g_error_free (error);
		g_option_context_free (context);

		*exit_status = EXIT_FAILURE;
		return TRUE;
	}
	*exit_status = EXIT_SUCCESS;
	g_option_context_free (context);


	/* Display version */
	if (version)
	{
		g_print ("%s\n", PACKAGE_STRING);
		return TRUE;
	}

	/* Register application */
	if (no_client) g_application_set_flags (application, G_APPLICATION_NON_UNIQUE | g_application_get_flags (application));
	g_application_register (G_APPLICATION (application), NULL, NULL);

	/* Convert all file names to GFile, so an already existing instance of
	 * Anjuta having another current directory can still open the files */
	files = g_ptr_array_new ();
	if (filenames)
	{
		gchar** filename;

		for (filename = filenames; *filename != NULL; filename++)
		{
			GFile* file = g_file_new_for_commandline_arg(*filename);
			if (file != NULL) g_ptr_array_add (files, file);
		}
	}

	if (g_application_get_is_remote (application))
	{
		if (files)
		{
			g_application_open (application, (GFile **)files->pdata, files->len, "");
		}
	}
	else
	{
		AnjutaWindow *win = anjuta_application_create_window (ANJUTA_APPLICATION (application),
		                                                      (GFile **)files->pdata, files->len,
		                                                      no_splash, no_session, no_files,
		                                                      geometry);
		gtk_window_set_application (GTK_WINDOW (win), GTK_APPLICATION (application));
		gtk_widget_show (GTK_WIDGET (win));
	}
	g_ptr_array_foreach (files, (GFunc)g_object_unref, NULL);
	g_ptr_array_free (files, TRUE);

	return TRUE;
}

static void
anjuta_application_open (GApplication *application,
                         GFile **files,
                         gint n_files,
                         const gchar* hint)
{
	int i;
	GList* windows = gtk_application_get_windows (GTK_APPLICATION (application));
	AnjutaWindow *win = ANJUTA_WINDOW (windows->data);
	IAnjutaFileLoader* loader =
		anjuta_shell_get_interface(ANJUTA_SHELL(win), IAnjutaFileLoader, NULL);

	if (!loader)
		return;

	for (i = 0; i < n_files; i++)
	{
		ianjuta_file_loader_load(loader, files[i], FALSE, NULL);
	}
}



/* GObject implementation
 *---------------------------------------------------------------------------*/

static void
anjuta_application_finalize (GObject *object)
{
  G_OBJECT_CLASS (anjuta_application_parent_class)->finalize (object);
}

static void
anjuta_application_init (AnjutaApplication *application)
{
	application->priv = G_TYPE_INSTANCE_GET_PRIVATE (application,
	                                                 ANJUTA_TYPE_APPLICATION,
	                                                 AnjutaApplicationPrivate);
	application->priv->proper_shutdown = FALSE;
	application->priv->no_splash = FALSE;
}

static void
anjuta_application_class_init (AnjutaApplicationClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GApplicationClass* app_class = G_APPLICATION_CLASS (klass);

	object_class->finalize = anjuta_application_finalize;

	app_class->open = anjuta_application_open;
	app_class->local_command_line = anjuta_application_local_command_line;

	g_type_class_add_private (klass, sizeof (AnjutaApplicationPrivate));
}



/* Public functions
 *---------------------------------------------------------------------------*/

AnjutaApplication*
anjuta_application_new (void)
{
  g_type_init ();

  return g_object_new (anjuta_application_get_type (),
                       "application-id", "org.gnome.anjuta",
                       "flags", G_APPLICATION_HANDLES_OPEN,
                       NULL);
}

gboolean
anjuta_application_get_proper_shutdown (AnjutaApplication *app)
{
	return app->priv->proper_shutdown;
}
