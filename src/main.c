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
#include <libanjuta/e-splash.h>
#include <libanjuta/anjuta-debug.h>

#include "anjuta.h"

#define ANJUTA_PIXMAP_SPLASH_SCREEN       "anjuta_splash.png"

/* Command line options */
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

static gchar *get_real_path(const gchar *file_name)
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
			return g_strdup(path);
		}
		g_free (uri_scheme);
		return g_strdup (file_name);
	}
	else
		return NULL;
}

static GList *
get_command_line_args (GnomeProgram *program)
{
	poptContext ctx;
	gchar **args;
	gint i;
	GValue value = { 0, };
	GList *command_args = NULL;

	g_value_init (&value, G_TYPE_POINTER);
	g_object_get_property (G_OBJECT (program), GNOME_PARAM_POPT_CONTEXT, &value);
	ctx = g_value_get_pointer (&value);
	g_value_unset (&value);

	args = (char**) poptGetArgs(ctx);
	if (args) 
		for (i = 0; args[i]; i++) 
			command_args = g_list_append (command_args,
										  get_real_path (args[i]));
	return command_args;
}

int
main (int argc, char *argv[])
{
	AnjutaApp *app;
	GtkWidget *splash = NULL;
	GnomeProgram *program;
	gchar *data_dir;
	GList *plugins_dirs = NULL;
	GList* command_args;
	GdkGeometry size_hints = {
    	100, 100, 0, 0, 100, 100, 0, 0, 0.0, 0.0, GDK_GRAVITY_NORTH_WEST  
  	};
	
#ifdef ENABLE_NLS
	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(PACKAGE, "UTF-8");
	textdomain (PACKAGE);
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
	
	gtk_window_set_default_icon_from_file (PACKAGE_PIXMAPS_DIR"/anjuta_icon.png", NULL);
	
	/* Get the command line files */
	command_args = get_command_line_args (program);
	gtk_window_set_auto_startup_notification(FALSE);

	if (!no_splash) {
		char *im_file = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_SPLASH_SCREEN);
		if (im_file) {
			if (NULL != (splash = e_splash_new(im_file))) {
				gtk_widget_show (splash);
				g_object_ref (G_OBJECT (splash));
				while (gtk_events_pending ())
					gtk_main_iteration ();
			}
		}
	}
	
	/* Initialize plugins */
	plugins_dirs = g_list_prepend (plugins_dirs, PACKAGE_PLUGIN_DIR);
	anjuta_plugins_init (plugins_dirs);

	/* Create Anjuta application */
	app = anjuta_new (argv[0], command_args, E_SPLASH (splash), proper_shutdown);
	gtk_window_set_role (GTK_WINDOW (app), "anjuta-app");
	if (splash) {
		g_object_unref (splash);
        gtk_widget_destroy (splash);
	}
	
	/* Run Anjuta application */
	gtk_window_set_geometry_hints (GTK_WINDOW (app), GTK_WIDGET (app),
								   &size_hints, GDK_HINT_RESIZE_INC);
	if (anjuta_geometry)
	{
		gint pos_x, pos_y, width, height;
		DEBUG_PRINT ("Setting geometry: %s", anjuta_geometry);
		/* Parse geometry doesn't seem to work here :( */
		/* gtk_window_parse_geometry (GTK_WINDOW (app), anjuta_geometry); */
		if (sscanf (anjuta_geometry, "%dx%d+%d+%d", &width, &height,
			&pos_x, &pos_y) == 4)
		{
			gtk_window_set_default_size (GTK_WINDOW (app), width, height);
			gtk_window_move (GTK_WINDOW (app), pos_x, pos_y);
		}
		else
		{
			g_warning ("Failed to parse geometry: %s", anjuta_geometry);
		}
	}
	else
	{
		gint pos_x, pos_y, width, height;
		
		pos_x = 10;
		pos_y = 10;
		width = gdk_screen_width () - 10;
		height = gdk_screen_height () - 25;
		width = (width < 790)? width : 790;
		height = (height < 575)? width : 575;
		gtk_window_set_default_size (GTK_WINDOW (app), width, height);
		gtk_window_move (GTK_WINDOW (app), pos_x, pos_y);
	}
	gtk_window_set_auto_startup_notification(TRUE);
	gtk_widget_show (GTK_WIDGET (app));
	gtk_main();
	
	/* Finalize plugins system */
	anjuta_plugins_finalize ();
	
	// FIXME: anjuta_application_exit();
	// FIXME: write_config();
	return 0;
}
