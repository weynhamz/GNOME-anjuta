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
#include "resources.h"
#include "anjuta.h"
#include "utilities.h"
#include "fileselection.h"

/* One and only one instance of AnjutaApp. */
AnjutaApp *app;			

gboolean no_splash = 0;

/* command line options table */
const struct poptOption anjuta_options[] = {
	{"no-splash", 's', POPT_ARG_NONE, &no_splash, 0, N_("Don't show the splashscreen"), NULL},
	POPT_AUTOHELP {NULL}
};

int
main (int argc, char *argv[])
{
	poptContext context;
	char *filename;

#ifdef ENABLE_NLS
	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	textdomain (PACKAGE);
#endif

	gnome_init_with_popt_table ("anjuta", VERSION, argc, argv,
				    anjuta_options, 0, &context);
	
	/* Session management is under development */
	gnome_client_disable_master_connection ();

	if (!no_splash)
		splash_screen ();

	anjuta_new ();
	anjuta_show ();

	/* Open the files given on the command line */
	while ((filename = poptGetArg (context)) != NULL)
		switch (get_file_ext_type (filename)) {
			case FILE_TYPE_IMAGE:
				break;
				
			case FILE_TYPE_PROJECT: 
				if (!app->project_dbase->project_is_open) {
					fileselection_set_filename (app->project_dbase->fileselection_open, filename);
					project_dbase_load_project (app->project_dbase, TRUE);
				}
				
				break;

			default:
				anjuta_goto_file_line (filename, -1);
		}
		
	poptFreeContext (context);
	
	gtk_main ();
	
	return 0;
}
