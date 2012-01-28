[+ autogen5 template +]
[+INCLUDE (string-append "licenses/" (get "License") ".tpl") \+]
/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.h
 * Copyright (C) [+(shell "date +%Y")+] [+Author+] <[+Email+]>
 * 
[+INVOKE LICENSE-DESCRIPTION PFX=" * " PROGRAM=(get "Name") OWNER=(get "Author") \+]
 */

#ifndef _[+NameCUpper+]_H_
#define _[+NameCUpper+]_H_

#include <libanjuta/anjuta-plugin.h>

typedef struct _[+PluginClass+] [+PluginClass+];
typedef struct _[+PluginClass+]Class [+PluginClass+]Class;

struct _[+PluginClass+]{
	AnjutaPlugin parent;
	[+IF (=(get "HasGladeFile") "1") +]GtkWidget *widget;[+ENDIF+]
	[+IF (=(get "HasUI") "1") +]gint uiid;
	GtkActionGroup *action_group;[+ENDIF+]
};

struct _[+PluginClass+]Class{
	AnjutaPluginClass parent_class;
};

#endif
