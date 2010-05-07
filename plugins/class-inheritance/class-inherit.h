/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 *  Copyright (C) Massimo Cora' 2005 <maxcvs@email.it>,
 *                2010 Naba Kumar <naba@gnome.org>
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

#include <libanjuta/interfaces/ianjuta-symbol-manager.h>
#include <libgnomecanvas/libgnomecanvas.h>

#include "plugin.h"

G_BEGIN_DECLS

#define NODE_HALF_DISPLAY_ELEM_NUM			30
#define NODE_SHOW_ALL_MEMBERS_STR			N_("Show all members...")

typedef enum {
	CLS_NODE_COLLAPSED,
	CLS_NODE_SEMI_EXPANDED,
	CLS_NODE_FULL_EXPANDED
} ClsNodeExpansionType;

typedef struct
{
	AnjutaClassInheritance *plugin; /* fixme: remove this */
	Agraph_t *graph;
	GtkWidget* canvas;

	IAnjutaSymbolManager *sym_manager;
	gint klass_id;
	gchar *sym_name;
	
	Agnode_t *agnode;

	/* Node's expansion status */
	ClsNodeExpansionType expansion_status;

	/* Holds canvas group item for either collapsed or expanded node */
	GnomeCanvasItem* canvas_group;
	
	/* What expansion type is currently drawn */
	ClsNodeExpansionType drawn_expansion_status;
		
	 /* Absolute coords in canvas. Only valid after node draw is ensured */
	gint width, height, x1, y1, x2, y2;

	/* Class members. Empty for collapsed nodes */
	GHashTable *members;

	/* All outgoing edges from this node */
	GHashTable *edges_to;

	/* All inbound edges to this node */
	GHashTable *edges_from;

	/* Used during update scan */
	gboolean marked_for_deletion;
} ClsNode;

typedef struct {
	Agedge_t *agedge;
	GnomeCanvasItem *canvas_line;
	GnomeCanvasItem *canvas_arrow;
	ClsNode *cls_node_from;
	ClsNode *cls_node_to;
} ClsNodeEdge;

typedef struct {
	ClsNode *cls_node;
	GnomeCanvasItem* canvas_node_item;

	gint sym_id;
	gchar *label;
	gchar *args;
	gchar *type_name;
	GFile *file;
	gint line;
	GdkPixbuf *icon;

	/* The order in which this item is shown in the list */
	gint order;

	/* Tooltip shown on item hover */
	GnomeCanvasItem *tooltip;

	/* Tooltip timout id */
	guint tooltip_timeout;
	
} ClsNodeItem;

gboolean cls_node_collapse (ClsNode *cls_node);
gboolean cls_node_expand (ClsNode *cls_node, ClsNodeExpansionType expand_type);

void cls_inherit_init (AnjutaClassInheritance *plugin);
void cls_inherit_draw (AnjutaClassInheritance *plugin);
void cls_inherit_update (AnjutaClassInheritance *plugin);
void cls_inherit_clear (AnjutaClassInheritance *plugin);
void cls_inherit_free (AnjutaClassInheritance *plugin);

G_END_DECLS
 
#endif /* _CLASS_INHERIT_H */
