[+ autogen5 template +]
/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */

[+IF (=(get "IncludeGNUHeader") "1") +]/*
	callbacks.c
	Copyright (C) [+Author+] [+(shell "date +%Y")+] <[+Email+]>

[+(gpl "callbacks.c"  "\t")+]
*/[+ENDIF+]

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
