[+ autogen5 template +]
/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * callbacks.c
 * Copyright (C) [+Author+] [+(shell "date +%Y")+] <[+Email+]>
 * 
[+CASE (get "License") +]
[+ == "BSD"  +][+(bsd  "callbacks.c" (get "Author") " * ")+]
[+ == "LGPL" +][+(lgpl "callbacks.c" (get "Author") " * ")+]
[+ == "GPL"  +][+(gpl  "callbacks.c"                " * ")+]
[+ESAC+] */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "callbacks.h"


void
destroy (GtkWidget *widget, gpointer data)
{
	gtk_main_quit ();
}
