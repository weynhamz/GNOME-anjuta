/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gprof-call-graph-block-entry.c
 * Copyright (C) James Liggett 2006 <jrliggett@cox.net>
 * 
 * gprof-call-graph-block-entry.c is free software.
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
 *            51 Franklin Street, Fifth Floor,
 *            Boston,  MA  02110-1301, USA.
 */

#include "gprof-call-graph-block-entry.h"

struct _GProfCallGraphBlockEntryPriv
{
	gfloat time_perc;  /* Percent of total execution */
	gfloat self_sec;   /* Time spent executing this function */
	gfloat child_sec;  /* Time spent on this function's children */
	gchar *calls;      /* Calls from parents and total */
	gchar *name;       /* Name of the function */
};

static void
gprof_call_graph_block_entry_init (GProfCallGraphBlockEntry *self)
{
	self->priv = g_new0 (GProfCallGraphBlockEntryPriv, 1);
}

static void
gprof_call_graph_block_entry_finalize (GObject *obj)
{
	GProfCallGraphBlockEntry *self;
	
	self = (GProfCallGraphBlockEntry *) obj;
	
	g_free (self->priv->name);
	g_free (self->priv->calls);
	g_free (self->priv);
}

static void 
gprof_call_graph_block_entry_class_init (GProfCallGraphBlockEntryClass *klass)
{
	GObjectClass *object_class;
	
	object_class = (GObjectClass *) klass;
	
	object_class->finalize = gprof_call_graph_block_entry_finalize;
}

GType
gprof_call_graph_block_entry_get_type (void)
{
	static GType obj_type = 0;
	
	if (!obj_type)
	{
		static const GTypeInfo obj_info = 
		{
			sizeof (GProfCallGraphBlockEntryClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gprof_call_graph_block_entry_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,           /* class_data */
			sizeof (GProfCallGraphBlockEntry),
			0,              /* n_preallocs */
			(GInstanceInitFunc) gprof_call_graph_block_entry_init,
			NULL            /* value_table */
		};
		obj_type = g_type_register_static (G_TYPE_OBJECT,
		                                   "GProfCallGraphBlockEntry", &obj_info, 
											0);
	}
	return obj_type;
}

GProfCallGraphBlockEntry *
gprof_call_graph_block_primary_entry_new (gchar **fields)
{
	GProfCallGraphBlockEntry *entry;
	
	entry = g_object_new (GPROF_CALL_GRAPH_BLOCK_ENTRY_TYPE, NULL);
	
	entry->priv->time_perc = atof (fields[0]);
	entry->priv->self_sec = atof (fields[1]);
	entry->priv->child_sec = atof (fields[2]);
	entry->priv->calls = g_strdup (fields[3]);
	entry->priv->name = g_strdup (fields[4]);

	return entry;	
}

GProfCallGraphBlockEntry *
gprof_call_graph_block_secondary_entry_new (gchar **fields)
{
	GProfCallGraphBlockEntry *entry;
	
	entry = g_object_new (GPROF_CALL_GRAPH_BLOCK_ENTRY_TYPE, NULL);
	
	entry->priv->time_perc = 0.0f;
	entry->priv->self_sec = atof (fields[0]);
	entry->priv->child_sec = atof (fields[1]);
	entry->priv->calls = g_strdup (fields[2]);
	entry->priv->name = g_strdup (fields[3]);

	return entry;	
}

void 
gprof_call_graph_block_entry_free (GProfCallGraphBlockEntry *self)
{
	g_object_unref (self);
}

gfloat
gprof_call_graph_block_entry_get_time_perc (GProfCallGraphBlockEntry *self)
{
	return self->priv->time_perc;
}


gfloat
gprof_call_graph_block_entry_get_self_sec (GProfCallGraphBlockEntry *self)
{
	return self->priv->self_sec;
}

gfloat
gprof_call_graph_block_entry_get_child_sec (GProfCallGraphBlockEntry *self)
{
	return self->priv->child_sec;
}

gchar *
gprof_call_graph_block_entry_get_calls (GProfCallGraphBlockEntry *self)
{
	return self->priv->calls;
}

gchar *
gprof_call_graph_block_entry_get_name (GProfCallGraphBlockEntry *self)
{
	return self->priv->name;
}

GProfCallGraphBlockEntry *
gprof_call_graph_block_entry_get_next (GList *current_iter, GList **next_iter)
{
	*next_iter = g_list_next (current_iter);
	
	if (*next_iter)
		return GPROF_CALL_GRAPH_BLOCK_ENTRY ((*next_iter)->data);
	else
		return NULL;
}
