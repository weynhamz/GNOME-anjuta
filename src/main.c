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

#include <gnome.h>
#include "splash.h"
#include "anjuta.h"
#include "utilities.h"
#include "fileselection.h"
#include "CORBA-Server.h"


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

int
main (int argc, char *argv[])
{
	GnomeClient *client;
	GnomeClientFlags flags;
	poptContext context;
	const char** args;
	CORBA_Environment	ev;
	CORBA_ORB			corb ;
	int		retCode ;


#ifdef ENABLE_NLS
	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	textdomain (PACKAGE);
#endif
	
	/* Connect the necessary kernal signals */
	anjuta_connect_kernel_signals();
	
	CInitEx( &ev );
	corb = gnome_CORBA_init_with_popt_table("anjuta", VERSION, &argc, argv,
					   anjuta_options, 0, &context,
					   GNORBA_INIT_SERVER_FUNC, &ev );
	/*corb = gnome_CORBA_init( "anjuta", VERSION, &argc, argv, 
								GNORBA_INIT_SERVER_FUNC, &ev );*/


	//gnome_init_with_popt_table("anjuta", VERSION, argc, argv,
	//			   anjuta_options, 0, &context);
	
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
		i = 0;
		while (args[i])
		{
			command_args = g_list_append (command_args, g_strdup(args[i]));
			i++;
		}
	}
	poptFreeContext(context);

	if (!no_splash)
		splash_screen ();
	
	anjuta_new ();
	anjuta_show ();

	/* Restore session */
	flags = gnome_client_get_flags(client);
	if (flags & GNOME_CLIENT_RESTORED)
		anjuta_session_restore(client);
	else
		anjuta_load_cmdline_files();
	//gtk_main ();
	retCode = MainLoop( &argc, argv, corb );
	anjuta_application_exit();
	write_config();
	return retCode ;
//	return 0;
}

