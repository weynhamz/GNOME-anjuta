/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gprof-call-graph-block.h
 * Copyright (C) James Liggett 2006 <jrliggett@cox.net>
 * 
 * gprof-call-graph-block.h is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2, or (at your option) any later version.
 * 
 * plugin.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.c.  See the file "COPYING".  If not,
 * write to:  The Free Software Foundation, Inc.,
 *            59 Temple Place - Suite 330,
 *            Boston,  MA  02111-1307, USA.
 */

#ifndef _GPROF_CALL_GRAPH_BLOCK_H
#define _GPROF_CALL_GRAPH_BLOCK_H

#include <glib.h>
#include <glib-object.h>
#include "gprof-call-graph-block-entry.h"

G_BEGIN_DECLS

typedef struct _GProfCallGraphBlock         GProfCallGraphBlock;
typedef struct _GProfCallGraphBlockClass    GProfCallGraphBlockClass;
typedef struct _GProfCallGraphBlockPriv     GProfCallGraphBlockPriv;

#define GPROF_CALL_GRAPH_BLOCK_TYPE            (gprof_call_graph_block_get_type ())
#define GPROF_CALL_GRAPH_BLOCK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPROF_CALL_GRAPH_BLOCK_TYPE, GProfCallGraphBlock))
#define GPROF_CALL_GRAPH_BLOCK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GPROF_CALL_GRAPH_BLOCK_TYPE, GProfCallGraphBlockClass))
#define IS_GPROF_CALL_GRAPH_BLOCK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPROF_CALL_GRAPH_BLOCK_TYPE))
#define IS_GPROF_CALL_GRAPH_BLOCK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPROF_CALL_GRAPH_BLOCK_TYPE))

struct  _GProfCallGraphBlock
{
	GObject parent;
	GProfCallGraphBlockPriv *priv;
};

struct _GProfCallGraphBlockClass
{
	GObjectClass parent_class;
};

GType gprof_call_graph_block_get_type ();

GProfCallGraphBlock *gprof_call_graph_block_new ();
void gprof_call_graph_block_free (GProfCallGraphBlock *self);

void gprof_call_graph_block_add_primary_entry (GProfCallGraphBlock *self,
											   GProfCallGraphBlockEntry *entry);
void gprof_call_graph_block_add_child_entry (GProfCallGraphBlock *self,
											 GProfCallGraphBlockEntry *entry);
void gprof_call_graph_block_add_parent_entry (GProfCallGraphBlock *self,
											  GProfCallGraphBlockEntry *entry);

GProfCallGraphBlockEntry *gprof_call_graph_block_get_primary_entry (GProfCallGraphBlock *self);
gboolean gprof_call_graph_block_has_parents (GProfCallGraphBlock *self);
gboolean gprof_call_graph_block_has_children (GProfCallGraphBlock *self);
GProfCallGraphBlockEntry *gprof_call_graph_block_get_first_parent (GProfCallGraphBlock *self,
																   GList **iter);
GProfCallGraphBlockEntry *gprof_call_graph_block_get_first_child (GProfCallGraphBlock *self,
																  GList **iter);

gboolean gprof_call_graph_block_is_recursive (GProfCallGraphBlock *self);

GProfCallGraphBlock *gprof_call_graph_block_get_next (GList *current_iter,
													  GList **next_iter);

G_END_DECLS

#endif
