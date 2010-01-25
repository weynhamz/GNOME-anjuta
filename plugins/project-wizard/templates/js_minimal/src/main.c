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

#include <glib.h>
#include <gjs/gjs.h>
#include <girepository.h>

#include "debug.h"

int main(int argc, char **argv)
{
	GjsContext *gjs_context;
 	const char *js_dir;
	char **search_path;
	GError *error = NULL;
	int status;

	g_type_init ();
	g_irepository_prepend_search_path (TYPELIBDIR);

	js_dir = JSDIR":.";

	search_path = g_strsplit(js_dir, ":", -1);
	gjs_context = gjs_context_new_with_search_path(search_path);

	debug_init (&argc, &argv, gjs_context);

	if (!gjs_context_eval (gjs_context,
                         "const Main = imports.main; Main.start();",
                         -1, "<main>", &status, &error))
 	{
 		g_warning ("Evaling main.js failed: %s", error->message);
 		g_error_free (error);
    }
	g_strfreev (search_path);
	return 0;
}
