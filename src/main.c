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

#include <gnome.h>
#include <gtk/gtkwindow.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libanjuta/resources.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>

#include "anjuta.h"
#include "bacon-message-connection.h"

#ifdef ENABLE_NLS
#include <locale.h>
#endif

#define ANJUTA_PIXMAP_SPLASH_SCREEN       "anjuta_splash.png"

/* App */
static AnjutaApp *app = NULL;

/* Bacon */
static guint32 startup_timestamp = 0;
static BaconMessageConnection *connection;

/* Command line options */
/* command line */
static gint line_position = 0;
static GList *file_list = NULL;
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

static gchar*
get_real_path (const gchar *file_name)
{
	if (file_name)
	{
		gchar path[PATH_MAX+1];
		gchar *uri_scheme;
		
		uri_scheme = gnome_vfs_get_uri_scheme (file_name);
		if (!uri_scheme)
		{
			memset(path, '\0', PATH_MAX+1);
			realpath(file_name, path);
			return g_strdup (path);
		}
		g_free (uri_scheme);
		return g_strdup (file_name);
	}
	else
		return NULL;
}

static void
get_command_line_args ()
{
	int i;
	if (anjuta_filenames) 
		for (i = 0; anjuta_filenames[i]; i++) 
 			file_list = g_list_append (file_list,
										  get_real_path (anjuta_filenames[i]));
}

static GdkDisplay *
display_open_if_needed (const gchar *name)
{
	GSList *displays;
	GSList *l;
	GdkDisplay *display = NULL;

	displays = gdk_display_manager_list_displays (gdk_display_manager_get ());

	for (l = displays; l != NULL; l = l->next)
	{
		if (strcmp (gdk_display_get_name ((GdkDisplay *) l->data), name) == 0)
		{
			display = l->data;
			break;
		}
	}

	g_slist_free (displays);

	return display != NULL ? display : gdk_display_open (name);
}

/* serverside */
static void
on_message_received (const char *message, gpointer    data)
{
	gchar **commands;
	gchar **params;
	gchar *display_name;
	gint screen_number;
	gint i;
	GdkDisplay *display;
	GdkScreen *screen;

	g_return_if_fail (message != NULL);

	commands = g_strsplit (message, "\v", -1);

	/* header */
	params = g_strsplit (commands[0], "\t", 4);
	startup_timestamp = atoi (params[0]); /* CHECK if this is safe */
	display_name = g_strdup (params[1]);
	screen_number = atoi (params[2]);

	display = display_open_if_needed (display_name);
	screen = gdk_display_get_screen (display, screen_number);
	g_free (display_name);

	g_strfreev (params);

	/* body */
	gtk_window_present (GTK_WINDOW (app));
	i = 1;
	while (commands[i])
	{
		params = g_strsplit (commands[i], "\t", -1);

		if (strcmp (params[0], "OPEN-URIS") == 0)
		{
			gint n_uris, i;
			gchar **uris;

			line_position = atoi (params[1]);

			n_uris = atoi (params[2]);
			uris = g_strsplit (params[3], " ", n_uris);

			for (i = 0; i < n_uris; i++)
				file_list = g_list_prepend (file_list, uris[i]);
			file_list = g_list_reverse (file_list);

			/* the list takes ownerhip of the strings,
			 * only free the array */
			g_free (uris);
		}
		else
		{
			g_warning ("Unexpected bacon command");
		}

		g_strfreev (params);
		++i;
	}

	g_strfreev (commands);

	/* execute the commands */

	while (file_list != NULL)
	{
		IAnjutaFileLoader* file =
			anjuta_shell_get_interface(ANJUTA_SHELL(app), IAnjutaFileLoader, NULL);
		gchar* uri = g_strdup_printf("%s:%d", (gchar*) file_list->data, line_position);
		
		ianjuta_file_loader_load(file, file_list->data, FALSE, NULL);
		g_free(uri);
		file_list = g_list_next(file_list);
	}

	/* free the file list and reset to default */
	g_list_foreach (file_list, (GFunc) g_free, NULL);
	g_list_free (file_list);
	file_list = NULL;

	line_position = 0;
}

/* clientside */
static void
send_bacon_message (void)
{
	GdkScreen *screen;
	GdkDisplay *display;
	const gchar *display_name;
	gint screen_number;
	GString *command;

	screen = gdk_screen_get_default ();
	display = gdk_screen_get_display (screen);

	display_name = gdk_display_get_name (display);
	screen_number = gdk_screen_get_number (screen);

	command = g_string_new (NULL);

	/* header */
	g_string_append_printf (command,
				"%" G_GUINT32_FORMAT "\t%s\t%d",
				startup_timestamp,
				display_name,
				screen_number);

	/* OPEN_URIS command, optionally specify line_num and encoding */
	if (file_list)
	{
		GList *l;

		command = g_string_append_c (command, '\v');
		command = g_string_append (command, "OPEN-URIS");

		g_string_append_printf (command,
					"\t%d\t%d\t",
					line_position,
					g_list_length (file_list));

		for (l = file_list; l != NULL; l = l->next)
		{
			command = g_string_append (command, l->data);
			if (l->next != NULL)
				command = g_string_append_c (command, ' ');
		}
	}
	
	bacon_message_connection_send (connection,
				       command->str);

	g_string_free (command, TRUE);
}

int
main (int argc, char *argv[])
{
	GnomeProgram *program;
	GOptionContext *context;
	gchar *data_dir;
	char *im_file;
	
	context = g_option_context_new (_("- Integrated Development Environment"));
#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
	g_option_context_add_main_entries (context, anjuta_options, PACKAGE);	
#else
	g_option_context_add_main_entries (context, anjuta_options, NULL);
#endif
	
	data_dir = g_strdup (PACKAGE_DATA_DIR);
	data_dir[strlen (data_dir) - strlen (PACKAGE) - 1] = '\0';
	
    /* Initialize threads, if possible */
#ifdef G_THREADS_ENABLED    
    if (!g_thread_supported()) g_thread_init(NULL);
#else
#warning "Some plugins won't work without thread support"
#endif
    
	/* Initialize gnome program */
	g_option_context_parse (context, &argc, &argv, NULL);
	program = gnome_program_init (PACKAGE, VERSION,
								  LIBGNOMEUI_MODULE, argc, argv,
								  GNOME_PARAM_GOPTION_CONTEXT, context,
								  GNOME_PARAM_HUMAN_READABLE_NAME,
								  _("Integrated Development Environment"),
								  GNOME_PARAM_APP_DATADIR, data_dir,
								  GNOME_PARAM_NONE);
	g_free (data_dir);
	
	/* Get the command line files */
	get_command_line_args ();
	
	connection = bacon_message_connection_new ("anjuta");
	
	if (connection != NULL)
	{
		if (!bacon_message_connection_get_is_server (connection) &&
			 no_client ==FALSE) 
		{
			DEBUG_PRINT("Client");
			send_bacon_message ();

			/* we never popup a window... tell startup-notification
			 * that we are done.
			 */
			gdk_notify_startup_complete ();

			bacon_message_connection_free (connection);

			exit (0);
		}
		else 
		{
			DEBUG_PRINT("Server");
			bacon_message_connection_set_callback (connection,
							       on_message_received,
							       NULL);
		}
	}
	else
		g_warning ("Cannot create the 'anjuta' connection.");
	
	g_set_application_name (_("Anjuta"));
	gtk_window_set_default_icon_name ("anjuta");
	gtk_window_set_auto_startup_notification(FALSE);

	im_file = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_SPLASH_SCREEN);
	
	/* Initialize applicatoin */
	app = anjuta_new (argv[0], file_list, no_splash, no_session, no_files,
					  im_file, proper_shutdown, anjuta_geometry);
	
	g_free (im_file);
	gtk_window_set_role (GTK_WINDOW (app), "anjuta-app");
	
	/* Run Anjuta application */
	gtk_window_set_auto_startup_notification(TRUE);
	gtk_widget_show (GTK_WIDGET (app));
	gtk_main();
	
	gnome_vfs_shutdown();
	
	return 0;
}
