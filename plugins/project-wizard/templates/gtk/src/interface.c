[+ autogen5 template +]
/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */

[+IF (=(get "IncludeGNUHeader") "1") +]/*
	interface.c
	Copyright (C) [+Author+] [+(shell "date +%Y")+] <[+Email+]>

[+(gpl "interface.c"  "\t")+]
*/[+ENDIF+]

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

[+IF (=(get "HaveGlade") "1")+]
#define GLADE_HOOKUP_OBJECT(component,widget,name) \
		g_object_set_data_full (G_OBJECT (component), name, \
		gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
	g_object_set_data (G_OBJECT (component), name, widget)
[+ENDIF+]

GtkWidget*
create_window1 (void)
{
	GtkWidget *window1;
[+IF (=(get "HaveGlade") "1")+]
	GladeXML *gxml;
	
	gxml = glade_xml_new (GLADE_FILE, NULL, NULL);
	
	/* This is important */
	glade_xml_signal_autoconnect (gxml);
	window1 = glade_xml_get_widget (gxml, "window1");
	/* Store pointers to all widgets, for use by lookup_widget(). */
 	GLADE_HOOKUP_OBJECT_NO_REF (window1, window1, "window1");
[+ELSE+]
	window1 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window1), _("window1"));
	gtk_window_set_default_size (GTK_WINDOW (window1), 500, 400);
[+ENDIF+];
	
	return window1;
}
