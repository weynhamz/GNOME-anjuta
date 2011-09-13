[+ autogen5 template +]
/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * [+NameLower+].c
 * Copyright (C) [+Author+] [+(shell "date +%Y")+] <[+Email+]>
 * 
[+CASE (get "License") +]
[+ == "BSD"  +][+(bsd  (get "Name") (get "Author") " * ")+]
[+ == "LGPL" +][+(lgpl (get "Name") (get "Author") " * ")+]
[+ == "GPL"  +][+(gpl  (get "Name")                " * ")+]
[+ESAC+] */
#include "[+NameLower+].h"
[+IF (=(get "HaveI18n") "1")+]
#include <glib/gi18n.h>
[+ENDIF+]

[+IF (=(get "HaveBuilderUI") "1")+]
/* For testing propose use the local (not installed) ui file */
/* #define UI_FILE PACKAGE_DATA_DIR"/[+NameHLower+]/ui/[+NameHLower+].ui" */
#define UI_FILE "src/[+NameHLower+].ui"
#define TOP_WINDOW "window"
[+ENDIF+]

G_DEFINE_TYPE ([+NameCClass+], [+NameCLower+], GTK_TYPE_APPLICATION);

/* Create a new window loading a file */
static void
[+NameCLower+]_new_window (GApplication *app,
                           GFile        *file)
{
	GtkWidget *window;
[+IF (=(get "HaveBuilderUI") "1")+]
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
	g_object_unref (builder);
[+ELSE+]
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "[+Name+]");
[+ENDIF+]	
	
	gtk_window_set_application (GTK_WINDOW (window), GTK_APPLICATION (app));
	if (file != NULL)
	{
		/* TODO: Add code here to open the file in the new window */
	}
	gtk_widget_show_all (GTK_WIDGET (window));
}


/* GApplication implementation */
static void
[+NameCLower+]_activate (GApplication *application)
{
  [+NameCLower+]_new_window (application, NULL);
}

static void
[+NameCLower+]_open (GApplication  *application,
                     GFile        **files,
                     gint           n_files,
                     const gchar   *hint)
{
  gint i;

  for (i = 0; i < n_files; i++)
    [+NameCLower+]_new_window (application, files[i]);
}

static void
[+NameCLower+]_init ([+NameCClass+] *object)
{

}

static void
[+NameCLower+]_finalize (GObject *object)
{

	G_OBJECT_CLASS ([+NameCLower+]_parent_class)->finalize (object);
}

static void
[+NameCLower+]_class_init ([+NameCClass+]Class *klass)
{
	G_APPLICATION_CLASS (klass)->activate = [+NameCLower+]_activate;
	G_APPLICATION_CLASS (klass)->open = [+NameCLower+]_open;

	G_OBJECT_CLASS (klass)->finalize = [+NameCLower+]_finalize;
}

[+NameCClass+] *
[+NameCLower+]_new (void)
{
	g_type_init ();

	return g_object_new ([+NameCLower+]_get_type (),
	                     "application-id", "org.gnome.[+NameCLower+]",
	                     "flags", G_APPLICATION_HANDLES_OPEN,
	                     NULL);
}
