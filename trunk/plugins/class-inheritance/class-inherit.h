/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 *  Copyright (C) Massimo Cora' 2005 <maxcvs@email.it>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
 
#ifndef _CLASS_INHERIT_H
#define _CLASS_INHERIT_H

#include "plugin.h"

G_BEGIN_DECLS

#define NODE_FONT_DEFAULT "-*-clean-medium-r-normal-*-10-*-*-*-*-*-*"
#define NODE_HALF_DISPLAY_ELEM_NUM			10
#define NODE_SHOW_ALL_MEMBERS_STR			N_("Show all members...")
#define NODE_SHOW_NORMAL_VIEW_STR			N_("Normal view")

typedef struct _NodeData {
	GnomeCanvasItem *canvas_item;  /* item itself */
	gchar *name;                   /* the name of the class */
	gchar *sub_item;					 /* case of an expanded node */
	gboolean anchored;             /* should it be anchored [e.g. paint all the 
	                                * data to the graph?]
	                                */
	GtkWidget *menu;
	AnjutaClassInheritance *plugin;
} NodeData;

typedef struct _NodeExpansionStatus {
	gchar *name;
	gint expansion_status;
} NodeExpansionStatus;


enum {
	NODE_NOT_EXPANDED,
	NODE_HALF_EXPANDED,
	NODE_FULL_EXPANDED
};

	
void class_inheritance_base_gui_init (AnjutaClassInheritance *plugin);
void class_inheritance_update_graph (AnjutaClassInheritance *plugin);
void class_inheritance_clean_canvas (AnjutaClassInheritance *plugin);
void class_inheritance_show_dynamic_class_popup_menu (GdkEvent *event,
										   NodeData* nodedata);
void class_inheritance_hash_table_clear (AnjutaClassInheritance *plugin);

G_END_DECLS
 
#endif /* _CLASS_INHERIT_H */
