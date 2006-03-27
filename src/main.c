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

#include "anjuta.h"

#define ANJUTA_PIXMAP_SPLASH_SCREEN       "anjuta_splash.png"

/* Command line options */
static gboolean no_splash = 0;
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

static GList *
get_command_line_args ()
{
	gint i;
	GList *command_args = NULL;

if (anjuta_filenames) 
		for (i = 0; anjuta_filenames[i]; i++) 
			command_args = g_list_append (command_args,
										  get_real_path (get_real_path (anjuta_filenames[i])));
	return command_args;
}

int
main (int argc, char *argv[])
{
	AnjutaApp *app;
	GnomeProgram *program;
	GOptionContext *context;
	gchar *data_dir;
	GList *plugins_dirs = NULL;
	GList* command_args = NULL;
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
	
	g_option_context_parse (context, &argc, &argv, NULL);
	/* Initialize gnome program */
	program = gnome_program_init (PACKAGE, VERSION,
								  LIBGNOMEUI_MODULE, argc, argv,
								  GNOME_PARAM_GOPTION_CONTEXT, context,
								  GNOME_PARAM_HUMAN_READABLE_NAME,
								  _("Integrated Development Environment"),
								  GNOME_PARAM_APP_DATADIR, data_dir,
								  NULL);
	g_free (data_dir);
	
	gtk_window_set_default_icon_from_file (PACKAGE_PIXMAPS_DIR"/anjuta_icon.png", NULL);
	
	/* Get the command line files */
	command_args = get_command_line_args ();
	gtk_window_set_auto_startup_notification(FALSE);

	im_file = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_SPLASH_SCREEN);
	
	/* Initialize plugins */
	plugins_dirs = g_list_prepend (plugins_dirs, PACKAGE_PLUGIN_DIR);
	anjuta_plugins_init (plugins_dirs);

	/* Create Anjuta application */
	app = anjuta_new (argv[0], command_args, no_splash, im_file,
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
