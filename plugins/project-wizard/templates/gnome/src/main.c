[+ autogen5 template +]
/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */

[+IF (=(get "IncludeGNUHeader") "1") +]/*
	main.c
	Copyright (C) [+Author+] [+(shell "date +%Y")+] <[+Email+]>

[+(gpl "main.c"  "\t")+]
*/[+ENDIF+]

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>

#include "interface.h"
#include "support.h"

int
main (int argc, char *argv[])
{
	GtkWidget *window1;

[+IF (=(get "HaveI18n") "1")+]
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif
[+ENDIF+]
	gnome_program_init (PACKAGE, VERSION, LIBGNOMEUI_MODULE,
                      argc, argv,
                      GNOME_PARAM_APP_DATADIR, PACKAGE_DATA_DIR,
                      NULL);

	window1 = create_window1 ();
	gtk_widget_show (window1);

	gtk_main ();
	
	return 0;
}
