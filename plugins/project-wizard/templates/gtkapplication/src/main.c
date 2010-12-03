[+ autogen5 template +]
/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) [+Author+] [+(shell "date +%Y")+] <[+Email+]>
 * 
[+CASE (get "License") +]
[+ == "BSD"  +][+(bsd  (get "Name") (get "Author") " * ")+]
[+ == "LGPL" +][+(lgpl (get "Name") (get "Author") " * ")+]
[+ == "GPL"  +][+(gpl  (get "Name")                " * ")+]
[+ESAC+] */

#include <config.h>
#include <gtk/gtk.h>
#include "[+NameLower+].h"

[+IF (=(get "HaveI18n") "1")+]
#include <glib/gi18n.h>
[+ENDIF+]

/* For testing propose use the local (not installed) ui file */
/* #define UI_FILE PACKAGE_DATA_DIR"/[+NameHLower+]/ui/[+NameHLower+].ui" */
#define UI_FILE "src/[+NameHLower+].ui"

int
main (int argc, char *argv[])
{
	[+NameCClass+] *app;
	int status;

[+IF (=(get "HaveI18n") "1")+]
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif
[+ENDIF+]
	
  app = [+NameCLower+]_pad_new ();
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
