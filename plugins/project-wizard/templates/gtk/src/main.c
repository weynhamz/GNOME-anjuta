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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <config.h>

#include <gtk/gtk.h>
#include <glade/glade.h>


[+IF (=(get "HaveI18n") "1")+]
/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif
[+ENDIF+]


#include "callbacks.h"

#define GLADE_FILE PACKAGE_DATA_DIR"/[+NameLower+]/glade/[+NameLower+].glade"

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
		g_object_set_data_full (G_OBJECT (component), name, \
		gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
	g_object_set_data (G_OBJECT (component), name, widget)
	
	
GtkWidget*
create_window1 (void)
{
	GtkWidget *window1;
	GladeXML *gxml;
	
	gxml = glade_xml_new (GLADE_FILE, NULL, NULL);
	
	/* This is important */
	glade_xml_signal_autoconnect (gxml);
	window1 = glade_xml_get_widget (gxml, "window1");
	/* Store pointers to all widgets, for use by lookup_widget(). */
 	GLADE_HOOKUP_OBJECT_NO_REF (window1, window1, "window1");
	
	return window1;
}


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
	
	gtk_set_locale ();
	gtk_init (&argc, &argv);

	window1 = create_window1 ();
	gtk_widget_show (window1);

	gtk_main ();
	return 0;
}
