[+ autogen5 template +]
/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.h
    Copyright (C) [+Author+]
[+IF (=(get "IncludeGNUHeader") "1") +]
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
[+ENDIF+]
*/

#ifndef _[+NameCUpper+]_H_
#define _[+NameCUpper+]_H_

#include <libanjuta/anjuta-plugin.h>

typedef struct _[+PluginClass+] [+PluginClass+];
typedef struct _[+PluginClass+]Class [+PluginClass+]Class;

struct _[+PluginClass+]{
	AnjutaPlugin parent;
	[+IF (=(get "HasGladeFile") "1") +]GtkWidget *widget;[+ENDIF+]
	[+IF (=(get "HasUI") "1") +]gint uiid;[+ENDIF+]
};

struct _[+PluginClass+]Class{
	AnjutaPluginClass parent_class;
};

#endif
