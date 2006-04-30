/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.h
 * Copyright (C) Johannes Schmid 2006 <jhs@cvs.gnome.org>
 * 
 * plugin.h is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * plugin.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.h.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _DEVHELP_H_
#define _DEVHELP_H_

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-editor.h>

#include <devhelp/dh-base.h>

typedef struct _AnjutaDevhelp AnjutaDevhelp;
typedef struct _AnjutaDevhelpClass AnjutaDevhelpClass;

struct _AnjutaDevhelp{
	AnjutaPlugin parent;
	
	DhBase         *base;
	GtkWidget     *htmlview;    
	GtkWidget      *control_notebook; 
	GtkWidget      *book_tree;
	GtkWidget      *search;
	
	IAnjutaEditor	*editor;
	guint editor_watch_id;
	
	GtkActionGroup* action_group;
	gint uiid;
};

struct _AnjutaDevhelpClass{
	AnjutaPluginClass parent_class;
};

void anjuta_devhelp_check_history(AnjutaDevhelp* devhelp);

#endif
