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
#include <libanjuta/anjuta-debug.h>
#include <libxml/parser.h>

#include "anjuta.h"

#ifdef ENABLE_NLS
#include <locale.h>
#endif

/* Command line options */
static gboolean no_splash = 0;
static gboolean no_client = 0;
static gboolean no_session = 0;
static gboolean no_files = 0;
static gboolean proper_shutdown = 0;
static gchar *anjuta_geometry = NULL;
static gchar **anjuta_filenames = NULL;

static gboolean
show_version_cb (const char *option_name,
				const char *value,
				gpointer data,
				GError **error)
{
	g_print ("%s\n", PACKAGE_STRING);
	exit (0);

	return TRUE;
}

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
		&proper_shutdown,
		N_("Shut down Anjuta properly, releasing all resources (for debugging)"),
		NULL
	},
	{
		"version", 'v', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
		&show_version_cb,
		("Display program version"),
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

static void
free_files (GFile** files, gint n_files)
{
	gint i;
	for (i = 0; i < n_files; i++)
	{
		g_object_unref (files[i]);
	}
	g_free (files);
}

int
main (int argc, char *argv[])
{
	GOptionContext *context;
	GError* error = NULL;
	Anjuta* anjuta;
	GFile** files = NULL;
	gint n_files = 0;
	gint status;

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
	
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
    
	/* Initialize gnome program */
	if (!g_option_context_parse (context, &argc, &argv, &error))
	{
		g_debug ("Option parsing failed: %s", error->message);
		exit(1);
	}
	
	/* Init gtk+ */
	gtk_init (&argc, &argv);
    /* Initialize threads */
	g_thread_init(NULL);
	
	/* Init debug helpers */
	anjuta_debug_init ();

	/* Convert all file names to URI */
	/* So an already existing instance of Anjuta having another current
	 * directory can still open the files */
	if (anjuta_filenames)
	{
		files = g_new0 (GFile*, 1);
		gchar** filename;
		for (filename = anjuta_filenames; *filename != NULL; filename++)
		{
			GFile* file = g_file_new_for_commandline_arg(*filename);
			files = g_realloc (files, (n_files + 1) * sizeof (GFile*));
			files[n_files++] = file;
		}
	}

	g_set_application_name (_("Anjuta"));
	anjuta = anjuta_new ();
	if (no_client) g_application_set_flags (G_APPLICATION (anjuta), G_APPLICATION_NON_UNIQUE);
	g_application_register (G_APPLICATION (anjuta), NULL, NULL);

	
	if (g_application_get_is_remote (G_APPLICATION (anjuta)))
	{	
		if (files)
		{
			g_application_open (G_APPLICATION (anjuta), files, n_files, "");
			free_files (files, n_files);
		}
	}
	else
	{
		AnjutaApp *app = create_window (files, n_files,
										no_splash, no_session, no_files,
										proper_shutdown, anjuta_geometry);
		gtk_window_set_application (GTK_WINDOW (app), GTK_APPLICATION (anjuta));
		gtk_widget_show (GTK_WIDGET (app));
		
		free_files (files, n_files);
	}
	
	status = g_application_run (G_APPLICATION (anjuta), argc, argv);
	g_object_unref (anjuta);

	/* xmlCleanupParser must be called only one time in the application */
	if (proper_shutdown) xmlCleanupParser ();

	return status;
}
