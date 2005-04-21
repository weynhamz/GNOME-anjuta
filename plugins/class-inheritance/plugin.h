/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.h
 * Copyright (C) Massimo Cora' 2005 <maxcvs@email.it>
 * 
 * plugin.h is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2, or (at your option) any later version.
 * 
 * plugin.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.h.  See the file "COPYING".  If not,
 * write to:  The Free Software Foundation, Inc.,
 *            59 Temple Place - Suite 330,
 *            Boston,  MA  02111-1307, USA.
 */

#ifndef _CLASS_INHERITANCE_H_
#define _CLASS_INHERITANCE_H_

#include <libanjuta/anjuta-plugin.h>

#include <graphviz/dotneato.h>	/* libgraph */
#include <graphviz/graph.h>


typedef struct _AnjutaClassInheritance AnjutaClassInheritance;
typedef struct _AnjutaClassInheritanceClass AnjutaClassInheritanceClass;

struct _AnjutaClassInheritance {
	AnjutaPlugin parent;
	
	GtkWidget *widget;			/* a vbox */
	GtkWidget *update_button;	
	GtkWidget *canvas;
	GList *item_list;
	
	Agraph_t *graph;	
	
	gchar *top_dir;
	guint root_watch_id;
	gint uiid;
};

struct _AnjutaClassInheritanceClass {
	AnjutaPluginClass parent_class;
};

#endif
