/*
    main.c
    Copyright (C) 2000  Kh. Naba Kumar Singh

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

#include "anjuta.h"
#include "utilities.h"
#include "fileselection.h"

#include "pixmaps.h"
#include "resources.h"
#include "e-splash.h"

/* One and only one instance of AnjutaApp. */
AnjutaApp *app;			

gboolean no_splash = 0;
GList* command_args;

/* The static variables used in the poptTable.*/
/* anjuta's option table */
static struct 
poptOption anjuta_options[] = {
	{
	 	NULL, '\0', POPT_ARG_INTL_DOMAIN, PACKAGE,
	 	0, NULL, NULL
	},
	{"no-splash", 's', POPT_ARG_NONE, &no_splash, 0, N_("Do not show the splashscreen"), NULL},
	POPT_AUTOHELP {NULL}
};

/*
 * This is work around function to make sure that the users get a 
 * smooth transition from 0.1.8 to 0.1.9 version. It works by
 * detecting the version of the old config file and deleting
 * ~/.gnome/Anjuta if it is lesser than 0.1.9.
*/
static void delete_old_config_file (void)
{
	gchar *config_file, *config_dir;
	gchar *config_version;
	PropsID prop;
	
	config_dir = g_strconcat(g_get_home_dir(), "/.anjuta" PREF_SUFFIX, NULL);
	config_file = g_strconcat(config_dir, "/session.properties", NULL);
	
	prop = prop_set_new();
	prop_read (prop, config_file, config_dir);
	g_free (config_dir);
	g_free (config_file);
	
	config_version = prop_get(prop, "anjuta.version");
	if (config_version) {
		gint last_ver;
		if(1 > sscanf(config_version, "0.1.%d", &last_ver))
			last_ver = 10;
#ifdef DEBUG
		g_message ("Old Version = %d", last_ver);
#endif
		if (last_ver < 9) {
			gchar* conf_file;
			conf_file = g_strconcat (g_get_home_dir(), "/.gnome/Anjuta", NULL);
#ifdef DEBUG
			g_message("Old config file %s found: Removing it", conf_file);
#endif
			remove(conf_file);
			g_free(conf_file);
		}
		g_free(config_version);
	}
}

static gint
restore_session_on_idle (gpointer data)
{
	GnomeClient* client = data;
	while (gtk_events_pending ())
	{
		gtk_main_iteration ();
	}
	anjuta_session_restore(client);
	return FALSE;
}

static gint
load_command_lines_on_idle(gpointer data)
{
	int argc = (int)data;
	while (gtk_events_pending ())
	{
		gtk_main_iteration ();
	}
	anjuta_load_cmdline_files();
	if( ( 1 == argc ) &&	app->b_reload_last_project )
	{
		anjuta_load_last_project();
	}
	return FALSE;
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
										  tm_get_real_path (args[i]));
	return command_args;
}

int
main (int argc, char *argv[])
{
	GtkWidget *splash = NULL;
	GnomeProgram *program;
	GnomeClient *client;
	GnomeClientFlags flags;
	gchar *data_dir;

	/* Before anything starts */
	delete_old_config_file();
	
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
#ifdef DEBUG
	g_message ("Anjuta data directory set to: %s", data_dir);
#endif
	g_free (data_dir);
	
	/* Session management */
	client = gnome_master_client();
	gtk_signal_connect(GTK_OBJECT(client), "save_yourself",
			   GTK_SIGNAL_FUNC(on_anjuta_session_save_yourself),
			   (gpointer) argv[0]);
	gtk_signal_connect(GTK_OBJECT(client), "die",
			   GTK_SIGNAL_FUNC(on_anjuta_session_die), NULL);

	/* Get the command line files */
	command_args = get_command_line_args (program);

	if (!no_splash) {
		char *im_file = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_SPLASH_SCREEN);
		if (im_file) {
			if (NULL != (splash = e_splash_new(im_file))) {
				gtk_window_set_auto_startup_notification(FALSE);
				gtk_widget_show (splash);
				gtk_window_set_auto_startup_notification(TRUE);
			        g_object_ref (G_OBJECT (splash));
				while (gtk_events_pending ())
					gtk_main_iteration ();
			}
		}
	}
	
	anjuta_new ();
	while (gtk_events_pending ())
		gtk_main_iteration ();
	anjuta_show ();

	if (splash) {
		gtk_widget_unref (splash);
        	gtk_widget_destroy (splash);
	}

	flags = gnome_client_get_flags(client);
	if (flags & GNOME_CLIENT_RESTORED) {
		/* Restore session */
		gtk_idle_add(restore_session_on_idle, client);
	} else {
		/* Load commandline args */
		gtk_idle_add(load_command_lines_on_idle, (gpointer)argc);
	}
	
	/* Connect the necessary kernal signals */
	anjuta_kernel_signals_connect ();

	gtk_main();
	
	anjuta_application_exit();
	write_config();
	return 0;
}
