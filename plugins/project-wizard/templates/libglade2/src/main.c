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
#include <glade/glade.h>

#define GLADE_FILE PACKAGE_DATA_DIR"/[+NameLower+]/glade/[+NameLower+].glade"


int main (int argc, char *argv[])
{
	GtkWidget *window1;
	GladeXML *xml;

[+IF (=(get "HaveI18n") "1")+]
	#ifdef ENABLE_NLS
		bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
		textdomain (PACKAGE);
	#endif
[+ENDIF+]
	
	gnome_init (PACKAGE, VERSION, argc, argv);
	glade_gnome_init ();
	/*
	 * The .glade filename should be on the next line.
	 */
	xml = glade_xml_new (GLADE_FILE, NULL, NULL);

	/* This is important */
	glade_xml_signal_autoconnect (xml);
	window1 = glade_xml_get_widget (xml, "window1");
	gtk_widget_show (window1);

	gtk_main ();
	return 0;
}
