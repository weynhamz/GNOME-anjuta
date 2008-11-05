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

#include <string.h>

#include <gtk/gtk.h>
#include <panel-applet.h>

static gboolean
applet_factory (PanelApplet *applet, const gchar *iid, gpointer user_data)
{
	GtkWidget *label;

	if (strcmp (iid, "OAFIID:[+Name+]Applet") != 0)
		return FALSE;

	label = gtk_label_new ("Hello World");
	gtk_container_add (GTK_CONTAINER (applet), label);

	gtk_widget_show_all (GTK_WIDGET (applet));

	return TRUE;
}

PANEL_APPLET_BONOBO_FACTORY ("OAFIID:[+Name+]Applet_Factory",
                             PANEL_TYPE_APPLET,
                             "[+Title+]",
                             "0",
                             applet_factory,
                             NULL);

