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
#include "splash.h"
#include "anjuta.h"
#include "utilities.h"
#include "fileselection.h"

#ifdef USE_GLADEN
#  include "CORBA-Server.h"
#endif

/* #define DEBUG */

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
	
	config_dir = g_strconcat(g_get_home_dir(), "/.anjuta", NULL);
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

int
main (int argc, char *argv[])
{
	GnomeClient *client;
	GnomeClientFlags flags;
	poptContext context;
	const char** args;

#ifdef USE_GLADEN
	CORBA_Environment ev;
	CORBA_ORB corb ;
#endif /* USE_GLADEN */
	int retCode ;


	/* Before anything starts */
	delete_old_config_file();
	
#ifdef ENABLE_NLS
	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	textdomain (PACKAGE);
#endif
	
	/* Connect the necessary kernal signals */
	anjuta_connect_kernel_signals();

#ifdef USE_GLADEN
	CInitEx( &ev );
	corb = gnome_CORBA_init_with_popt_table(PACKAGE, VERSION, &argc, argv,
					   anjuta_options, 0, &context,
					   GNORBA_INIT_SERVER_FUNC, &ev );
#else /* USE_GLADEN */
	retCode = gnome_init_with_popt_table (PACKAGE, VERSION, argc,
					   argv, anjuta_options, 0, &context);
#endif /*USE_GLADEN */

	/* Session management */
	client = gnome_master_client();
	gtk_signal_connect(GTK_OBJECT(client), "save_yourself",
			   GTK_SIGNAL_FUNC(on_anjuta_session_save_yourself),
			   (gpointer) argv[0]);
	gtk_signal_connect(GTK_OBJECT(client), "die",
			   GTK_SIGNAL_FUNC(on_anjuta_session_die), NULL);

	/* Get the command line files */
	command_args = NULL;
	args = poptGetArgs(context);
	if (args)
	{
		gint i;
		
		for(i = 0; args[i]; ++i)
			command_args = g_list_append (command_args, tm_get_real_path(args[i]));
	}
	poptFreeContext(context);

	/* Libglade stuff */
	glade_gnome_init();
	if (!no_splash)
		splash_screen ();
	
	anjuta_new ();
	anjuta_show ();
	
	flags = gnome_client_get_flags(client);
	if (flags & GNOME_CLIENT_RESTORED) {
		/* Restore session */
		gtk_idle_add(restore_session_on_idle, client);
	} else {
		/* Load commandline args */
		gtk_idle_add(load_command_lines_on_idle, (gpointer)argc);
	}

#ifdef USE_GLADEN
	retCode = MainLoop( &argc, argv, corb );
#else /* USE_GLADEN */
	gtk_main();
#endif /* USE_GLADEN */
	
	anjuta_application_exit();
	write_config();
	return retCode ;
}
