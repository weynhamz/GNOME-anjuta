[+ autogen5 template +]
/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
[+IF (=(get "IncludeGNUHeader") "1") +]/*
    main.c
    Copyright (C) [+Author+]

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
*/[+ENDIF+]

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
[+IF (=(get "HaveGlade") "1")+]
#include <glade/glade.h>
[+ENDIF+]

/* FIXME:
 * The .glade filename used by your program. Currently, it is being
 * picked from the source directory. Fix the following line to pick it from
 * program's installation directory.
 */
#define GLADE_FILE PACKAGE_SRC_DIR"/[+NameLower+].glade"
/* #define GLADE_FILE PACKAGE_DATA_DIR"/[+NameLower+]/glade/[+NameLower+].glade" */

int
main (int argc, char *argv[])
{
	GtkWidget *window;
[+IF (=(get "HaveGlade") "1")+]
	GladeXML *gxml;
[+ENDIF+]
[+IF (=(get "HaveI18n") "1")+]
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif
	gtk_set_locale ();
[+ENDIF+]
	gtk_init (&argc, &argv);

[+IF (=(get "HaveGlade") "1")+]
	gxml = glade_xml_new (GLADE_FILE, NULL, NULL);
	
	/* This is important */
	glade_xml_signal_autoconnect (gxml);
	window = glade_xml_get_widget (gxml, "window1");
[+ELSE+]
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size (GTK_WINDOW (window), 500, 400);
	g_signal_connect (G_OBJECT (window), "delete-event",
					  G_CALLBACK (gtk_main_quit), NULL);
[+ENDIF+]
	gtk_widget_show_all (window);
	
	gtk_main ();
	return 0;
}
