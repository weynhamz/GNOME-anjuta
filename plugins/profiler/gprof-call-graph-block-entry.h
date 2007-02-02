/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gprof-call-graph-block-entry.h
 * Copyright (C) James Liggett 2006 <jrliggett@cox.net>
 * 
 * gprof-call-graph-block-entry.h is free software.
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

#ifndef _GPROF_CALL_GRAPH_BLOCK_ENTRY
#define _GPROF_CALL_GRAPH_BLOCK_ENTRY

#include <glib.h>
#include <glib-object.h>
#include <stdlib.h>

G_BEGIN_DECLS

typedef struct _GProfCallGraphBlockEntry         GProfCallGraphBlockEntry;
typedef struct _GProfCallGraphBlockEntryClass    GProfCallGraphBlockEntryClass;
typedef struct _GProfCallGraphBlockEntryPriv     GProfCallGraphBlockEntryPriv;

#define GPROF_CALL_GRAPH_BLOCK_ENTRY_TYPE            (gprof_call_graph_block_entry_get_type ())
#define GPROF_CALL_GRAPH_BLOCK_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPROF_CALL_GRAPH_BLOCK_ENTRY_TYPE, GProfCallGraphBlockEntry))
#define GPROF_CALL_GRAPH_BLOCK_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GPROF_CALL_GRAPH_BLOCK_ENTRY_TYPE, GProfCallGraphBlockEntryClass))
#define IS_GPROF_CALL_GRAPH_BLOCK_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPROF_CALL_GRAPH_BLOCK_ENTRY_TYPE))
#define IS_GPROF_CALL_GRAPH_BLOCK_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPROF_CALL_GRAPH_BLOCK_ENTRY_TYPE))

struct  _GProfCallGraphBlockEntry
{
	GObject parent;
	GProfCallGraphBlockEntryPriv *priv;
};

struct _GProfCallGraphBlockEntryClass
{
	GObjectClass parent_class;
};

GType gprof_call_graph_block_entry_get_type ();

GProfCallGraphBlockEntry *gprof_call_graph_block_primary_entry_new (gchar **fields);
GProfCallGraphBlockEntry *gprof_call_graph_block_secondary_entry_new (gchar **fields);
void gprof_call_graph_block_entry_free (GProfCallGraphBlockEntry *self);

gfloat gprof_call_graph_block_entry_get_time_perc (GProfCallGraphBlockEntry *self);
gfloat gprof_call_graph_block_entry_get_self_sec (GProfCallGraphBlockEntry *self);
gfloat gprof_call_graph_block_entry_get_child_sec (GProfCallGraphBlockEntry *self);
gchar *gprof_call_graph_block_entry_get_calls (GProfCallGraphBlockEntry *self);
gchar *gprof_call_graph_block_entry_get_name (GProfCallGraphBlockEntry *self);

GProfCallGraphBlockEntry *gprof_call_graph_block_entry_get_next (GList *current_iter,
														   		 GList **next_iter);

G_END_DECLS

#endif
