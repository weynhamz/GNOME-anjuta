/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    main.c
    Copyright (C) 2000  Naba Kumar  <naba@gnome.org>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef ENABLE_NLS
#  include <locale.h>
#endif

#include <sys/stat.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <unique/unique.h>
#include <libanjuta/resources.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>

#include <gnome.h>

#include "anjuta.h"

#ifdef ENABLE_NLS
#include <locale.h>
#endif

#define ANJUTA_PIXMAP_SPLASH_SCREEN       "anjuta_splash.png"

/* App */
static AnjutaApp *app = NULL;

/* Command line options */
static gboolean no_splash = 0;
static gboolean no_client = 0;
static gboolean no_session = 0;
static gboolean no_files = 0;
static gboolean proper_shutdown = 0;
static gchar *anjuta_geometry = NULL;
static gchar **anjuta_filenames = NULL;

static const GOptionEntry anjuta_options[] = {
	{
		"geometry", 'g', 0, G_OPTION_ARG_STRING,
		&anjuta_geometry,
		N_("Specify the size and location of the main window"),
		/* This is the format you can specify the size andposition
		 * of the window on command line */
		N_("WIDTHxHEIGHT+XOFF+YOFF")
	},
	{
		"no-splash", 's', 0, G_OPTION_ARG_NONE,
		&no_splash,
		N_("Do not show the splashscreen"),
		NULL
	},
	{
		"no-client", 'c', 0, G_OPTION_ARG_NONE,
		&no_client,
		N_("Start a new instance and do not open the file in a existing"),
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
		&proper_shutdown,
		N_("Shutdown anjuta properly releasing all resources (for debugging)"),
		NULL
	},
	{
		G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY,
		&anjuta_filenames,
		NULL,
		NULL
	},
	{NULL}
};

static UniqueResponse
message_received_cb (UniqueApp         *unique,
					 UniqueCommand      command,
					 UniqueMessageData *message,
					 guint              time_,
					 gpointer           user_data)
{
	UniqueResponse res;
	
	switch (command)
	{
		case UNIQUE_ACTIVATE:
			/* move the main window to the screen that sent us the command */
			gtk_window_set_screen (GTK_WINDOW (app), unique_message_data_get_screen (message));
			gtk_window_present (GTK_WINDOW (app));
			res = UNIQUE_RESPONSE_OK;
			break;
		case UNIQUE_OPEN:
		{
			gchar** uris = unique_message_data_get_uris(message);
			gchar** uri;
			for (uri = uris; *uri != NULL; uri++)
			{	
				IAnjutaFileLoader* loader =
					anjuta_shell_get_interface(ANJUTA_SHELL(app), IAnjutaFileLoader, NULL);
				GFile* gfile = g_file_new_for_commandline_arg(*uri);
				ianjuta_file_loader_load(loader, gfile, FALSE, NULL);
				g_object_unref (gfile);
			}
			g_strfreev(uris);
			res = UNIQUE_RESPONSE_OK;
			break;
		}
		default:
			res = UNIQUE_RESPONSE_OK;
			break;
	}
	
	return res;
}


int
main (int argc, char *argv[])
{
	GOptionContext *context;
	GError* error = NULL;
	gchar* im_file;
	UniqueApp* unique;
	
	context = g_option_context_new (_("- Integrated Development Environment"));
#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
	g_option_context_add_main_entries (context, anjuta_options, GETTEXT_PACKAGE);	
#else
	g_option_context_add_main_entries (context, anjuta_options, NULL);
#endif
	
	/* gnome_program_init() does this for us - enable when it's removed */
	/* g_option_context_add_group (context, gtk_get_option_group (TRUE));*/
	/* Init Gnome - remove once we get rid of libgnome */
	gnome_program_init (PACKAGE, VERSION,
	                    LIBGNOMEUI_MODULE, argc, argv,
	                    GNOME_PARAM_GOPTION_CONTEXT, context,
	                    GNOME_PARAM_HUMAN_READABLE_NAME,
	                    _("Integrated Development Environment"),
	                    GNOME_PARAM_NONE);
	
    /* Initialize threads, if possible */
#ifdef G_THREADS_ENABLED    
    if (!g_thread_supported()) g_thread_init(NULL);
#else
#warning "Some plugins won't work without thread support"
#endif
    
	/* Initialize gnome program */
	if (!g_option_context_parse (context, &argc, &argv, &error))
	{
		DEBUG_PRINT ("Option parsing failed: %s", error->message);
		exit(1);
	}
	
	/* Init debug helpers */
	anjuta_debug_init ();
	
	unique = unique_app_new ("org.gnome.anjuta", NULL);
	
	if (unique_app_is_running(unique))
	{
		UniqueResponse response;
		
		if (!no_client)
		{
			if (anjuta_filenames)
			{
				UniqueMessageData* message = unique_message_data_new();
				if (!unique_message_data_set_uris (message, anjuta_filenames))
					g_warning("Set uris failed");
				response = unique_app_send_message (unique, UNIQUE_OPEN, message);
				unique_message_data_free(message);
			}
			response = unique_app_send_message (unique, UNIQUE_ACTIVATE, NULL);
			
			/* we don't need the application instance anymore */
			g_object_unref (unique);
			
			if (response == UNIQUE_RESPONSE_OK)
				return 0;
			else
				DEBUG_PRINT("Faild to contact first instance, starting up normally");
		}
	}
	
	/* Init gtk+ */
	gtk_init (&argc, &argv);
	g_set_application_name (_("Anjuta"));
	gtk_window_set_default_icon_name ("anjuta");
	gtk_window_set_auto_startup_notification(FALSE);
	
	/* Initialize application */
	im_file = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_SPLASH_SCREEN);
	app = anjuta_new (argv[0], anjuta_filenames, no_splash, no_session, no_files,
					  im_file, proper_shutdown, anjuta_geometry);
	g_signal_connect (unique, "message-received", G_CALLBACK (message_received_cb), NULL);
	
	g_free (im_file);
	gtk_window_set_role (GTK_WINDOW (app), "anjuta-app");
	
	/* Run Anjuta application */
	gtk_window_set_auto_startup_notification(TRUE);
	gtk_widget_show (GTK_WIDGET (app));
	gtk_main();
	
	return 0;
}
