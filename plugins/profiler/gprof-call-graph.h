/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gprof-call-graph.h
 * Copyright (C) James Liggett 2006 <jrliggett@cox.net>
 * 
 * gprof-call-graph.h is free software.
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

#ifndef _GPROF_CALL_GRAPH_H
#define _GPROF_CALL_GRAPH_H

#include <glib.h>
#include <glib-object.h>
#include "gprof-call-graph-block-entry.h"
#include "gprof-call-graph-block.h"
#include "gprof-flat-profile.h"
#include "string-utils.h"

G_BEGIN_DECLS

typedef struct _GProfCallGraph         GProfCallGraph;
typedef struct _GProfCallGraphClass    GProfCallGraphClass;
typedef struct _GProfCallGraphPriv     GProfCallGraphPriv;

#define GPROF_CALL_GRAPH_TYPE            (gprof_call_graph_get_type ())
#define GPROF_CALL_GRAPH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPROF_CALL_GRAPH_TYPE, GProfCallGraph))
#define GPROF_CALL_GRAPH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GPROF_CALL_GRAPH_TYPE, GProfCallGraphClass))
#define IS_GPROF_CALL_GRAPH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPROF_CALL_GRAPH_TYPE))
#define IS_GPROF_CALL_GRAPH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPROF_CALL_GRAPH_TYPE))

struct  _GProfCallGraph
{
	GObject parent;
	GProfCallGraphPriv *priv;
};

struct _GProfCallGraphClass
{
	GObjectClass parent_class;
};

GType gprof_call_graph_get_type (void);

GProfCallGraph *gprof_call_graph_new (FILE *stream, 
									  GProfFlatProfile *flat_profile);
void gprof_call_graph_free (GProfCallGraph *self);

GProfCallGraphBlock *gprof_call_graph_find_block (GProfCallGraph *self, 
												  gchar *name);
GProfCallGraphBlock *gprof_call_graph_get_first_block (GProfCallGraph *self,
													   GList **iter);
GProfCallGraphBlock *gprof_call_graph_get_root (GProfCallGraph *self, 
												GList **iter);

void gprof_call_graph_dump (GProfCallGraph *self, FILE *stream);

G_END_DECLS

#endif
