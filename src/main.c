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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>

#include <gnome.h>
#include <gtk/gtkwindow.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libanjuta/resources.h>
#include <libanjuta/plugins.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>

#include "anjuta.h"
#include "bacon-message-connection.h"

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
static gboolean proper_shutdown = 0;
static gchar *anjuta_geometry = NULL;

/* The static variables used in the poptTable.*/
/* anjuta's option table */
static struct 
poptOption anjuta_options[] = {
	{
	 	NULL, '\0', POPT_ARG_INTL_DOMAIN, PACKAGE,
	 	0, NULL, NULL
	},
	{
		"geometry", 'g', POPT_ARG_STRING,
		&anjuta_geometry, 0,
		N_("Specify the size and location of the main window"),
		N_("WIDTHxHEIGHT+XOFF+YOFF")
	},
	{
		"no-splash", 's', POPT_ARG_NONE,
		&no_splash, 0,
		N_("Do not show the splashscreen"),
		NULL
	},
	{
		"proper-shutdown", 'p', POPT_ARG_NONE,
		&proper_shutdown, 0,
		N_("Shutdown anjuta properly releasing all resources (for debugging)"),
		NULL
	},
	POPT_AUTOHELP {NULL}
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
get_command_line_args (GnomeProgram *program)
{
	poptContext ctx;
	gchar **args;
	gint i;
	GValue value = { 0, };

	g_value_init (&value, G_TYPE_POINTER);
	g_object_get_property (G_OBJECT (program),
						   GNOME_PARAM_POPT_CONTEXT, &value);
	ctx = g_value_get_pointer (&value);
	g_value_unset (&value);

	args = (char**) poptGetArgs(ctx);
	if (args) 
		for (i = 0; args[i]; i++) 
			file_list = g_list_append (file_list,
										  get_real_path (args[i]));
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
on_message_received (const char *message,
		     gpointer    data)
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
	gchar *data_dir;
	GList *plugins_dirs = NULL;
	char *im_file;
	
#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif
	
	data_dir = g_strdup (PACKAGE_DATA_DIR);
	data_dir[strlen (data_dir) - strlen (PACKAGE) - 1] = '\0';
	
	/* Initialize gnome program */
	program = gnome_program_init (PACKAGE, VERSION,
								  LIBGNOMEUI_MODULE, argc, argv,
								  GNOME_PARAM_POPT_TABLE, anjuta_options,
								  GNOME_PARAM_HUMAN_READABLE_NAME,
								  _("Integrated Development Environment"),
								  GNOME_PARAM_APP_DATADIR, data_dir,
								  NULL);
	g_free (data_dir);
	
	/* Get the command line files */
	get_command_line_args (program);
	
	connection = bacon_message_connection_new ("anjuta");
	
	if (connection != NULL)
	{
		if (!bacon_message_connection_get_is_server (connection)) 
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
	
	gtk_window_set_default_icon_from_file (PACKAGE_PIXMAPS_DIR"/anjuta_icon.png", NULL);
	gtk_window_set_auto_startup_notification(FALSE);

	im_file = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_SPLASH_SCREEN);
	
	/* Initialize plugins */
	plugins_dirs = g_list_prepend (plugins_dirs, PACKAGE_PLUGIN_DIR);
	anjuta_plugins_init (plugins_dirs);
	
	app = anjuta_new (argv[0], file_list, no_splash, im_file,
					  proper_shutdown, anjuta_geometry);
	
	g_free (im_file);
	gtk_window_set_role (GTK_WINDOW (app), "anjuta-app");
	
	/* Run Anjuta application */
	gtk_window_set_auto_startup_notification(TRUE);
	gtk_widget_show (GTK_WIDGET (app));
	gtk_main();
	
	/* Finalize plugins system */
	anjuta_plugins_finalize ();
	return 0;
}
