[+ autogen5 template +]
[+INCLUDE (string-append "licenses/" (get "License") ".tpl") \+]
/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) [+(shell "date +%Y")+] [+Author+] <[+Email+]>
 * 
[+INVOKE LICENSE-DESCRIPTION PFX=" * " PROGRAM=(get "Name") OWNER=(get "Author") \+]
 */

#include <config.h>
#include <gtk/gtk.h>

[+IF (=(get "HaveI18n") "1")+]
#include <glib/gi18n.h>
[+ENDIF+]

[+IF (=(get "HaveBuilderUI") "1")+]
typedef struct _Private Private;
static struct _Private
{
	/* ANJUTA: Widgets declaration - DO NOT REMOVE */
};

static struct Private* priv = NULL;

/* For testing propose use the local (not installed) ui file */
/* #define UI_FILE PACKAGE_DATA_DIR"/ui/[+NameHLower+].ui" */
#define UI_FILE "src/[+NameHLower+].ui"
#define TOP_WINDOW "window"

/* Signal handlers */
/* Note: These may not be declared static because signal autoconnection
 * only works with non-static methods
 */

/* Called when the window is closed */
void
destroy (GtkWidget *widget, gpointer data)
{
	gtk_main_quit ();
}

static GtkWidget*
create_window (void)
{
	GtkWidget *window;
	GtkBuilder *builder;
	GError* error = NULL;

	/* Load UI from file */
	builder = gtk_builder_new ();
	if (!gtk_builder_add_from_file (builder, UI_FILE, &error))
	{
		g_critical ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

	/* Auto-connect signal handlers */
	gtk_builder_connect_signals (builder, NULL);

	/* Get the window object from the ui file */
	window = GTK_WIDGET (gtk_builder_get_object (builder, TOP_WINDOW));
        if (!window)
        {
                g_critical ("Widget \"%s\" is missing in file %s.",
				TOP_WINDOW,
				UI_FILE);
        }

	priv = g_malloc (sizeof (struct _Private));
	/* ANJUTA: Widgets initialization - DO NOT REMOVE */

	g_object_unref (builder);
	
	return window;
}
[+ELSE+]
static GtkWidget*
create_window (void)
{
	GtkWidget *window;

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "[+Name+]");

	/* Exit when the window is closed */
	g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
	
	return window;
}
[+ENDIF+]

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
	
	gtk_init (&argc, &argv);

	window = create_window ();
	gtk_widget_show (window);

	gtk_main ();
[+IF (=(get "HaveBuilderUI") "1")+]
	g_free (priv);
[+ENDIF+]
	return 0;
}
