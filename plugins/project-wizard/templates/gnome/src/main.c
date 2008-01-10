[+ autogen5 template +]
/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) [+Author+] [+(shell "date +%Y")+] <[+Email+]>
 * 
[+CASE (get "License") +]
[+ == "BSD"  +][+(bsd  "main.c" (get "Author") " * ")+]
[+ == "LGPL" +][+(lgpl "main.c" (get "Author") " * ")+]
[+ == "GPL"  +][+(gpl  "main.c"                " * ")+]
[+ESAC+] */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <bonobo.h>
#include <gnome.h>

#include "callbacks.h"

[+IF (=(get "HaveGlade") "1")+]
#include <glade/glade.h>

/* For testing propose use the local (not installed) glade file */
/* #define GLADE_FILE PACKAGE_DATA_DIR"/[+NameLower+]/glade/[+NameLower+].glade" */
#define GLADE_FILE "[+NameLower+].glade"
[+ENDIF+]

GtkWidget*
create_window (void)
{
	GtkWidget *window;
[+IF (=(get "HaveGlade") "1")+]
	GladeXML *gxml;
	
	gxml = glade_xml_new (GLADE_FILE, NULL, NULL);
	
	/* This is important */
	glade_xml_signal_autoconnect (gxml);
	window = glade_xml_get_widget (gxml, "window");
[+ELSE+]
 	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
 	gtk_window_set_title (GTK_WINDOW (window), _("window"));
 	gtk_window_set_default_size (GTK_WINDOW (window), 500, 400);
[+ENDIF+]
	
	return window;
}




int
main (int argc, char *argv[])
{
	GtkWidget *window;

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

	window = create_window ();
	gtk_widget_show (window);

	gtk_main ();
	
	return 0;
}
