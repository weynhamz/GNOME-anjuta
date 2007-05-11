/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gprof-call-graph-block.c
 * Copyright (C) James Liggett 2006 <jrliggett@cox.net>
 * 
 * gprof-call-graph-block.c is free software.
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

#include "gprof-call-graph-block.h"

struct _GProfCallGraphBlockPriv
{
	GProfCallGraphBlockEntry *primary_entry;  /* Function this block refers to */
	GList *parents;					  		  /* Functions that call this one */
	GList *children;						  /* Function that this one calls */
	GHashTable *lookup_set;					  /* Used to determine if this  
									 		   * function is recursive */
};

static void
gprof_call_graph_block_init (GProfCallGraphBlock *self)
{
	self->priv = g_new0 (GProfCallGraphBlockPriv, 1);
	
	self->priv->lookup_set = g_hash_table_new (g_str_hash, g_str_equal);
}

static void
gprof_call_graph_block_finalize (GObject *obj)
{
	GProfCallGraphBlock *self;
	GList *current;
	
	self = (GProfCallGraphBlock *) obj;
	
	gprof_call_graph_block_entry_free (self->priv->primary_entry);
	
	current = self->priv->parents;
	
	while (current)
	{
		gprof_call_graph_block_entry_free (GPROF_CALL_GRAPH_BLOCK_ENTRY (current->data));
		current = g_list_next (current);
	}
	
	g_list_free (self->priv->parents);
	
	current = self->priv->children;
	
	while (current)
	{
		gprof_call_graph_block_entry_free (GPROF_CALL_GRAPH_BLOCK_ENTRY (current->data));
		current = g_list_next (current);
	}	
	
	g_list_free (self->priv->children);
	
	g_hash_table_destroy (self->priv->lookup_set);
	
	g_free (self->priv);
}

static void 
gprof_call_graph_block_class_init (GProfCallGraphBlockClass *klass)
{
	GObjectClass *object_class;
	
	object_class = (GObjectClass *) klass;
	
	object_class->finalize = gprof_call_graph_block_finalize;
}

GType
gprof_call_graph_block_get_type (void)
{
	static GType obj_type = 0;
	
	if (!obj_type)
	{
		static const GTypeInfo obj_info = 
		{
			sizeof (GProfCallGraphBlockClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gprof_call_graph_block_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,           /* class_data */
			sizeof (GProfCallGraphBlock),
			0,              /* n_preallocs */
			(GInstanceInitFunc) gprof_call_graph_block_init,
			NULL            /* value_table */
		};
		obj_type = g_type_register_static (G_TYPE_OBJECT,
		                                   "GProfCallGraphBlock", &obj_info, 
											0);
	}
	return obj_type;
}

GProfCallGraphBlock *
gprof_call_graph_block_new (void)
{
	return g_object_new (GPROF_CALL_GRAPH_BLOCK_TYPE, NULL);
}

void
gprof_call_graph_block_free (GProfCallGraphBlock *self)
{
	g_object_unref (self);
}

void
gprof_call_graph_block_add_primary_entry (GProfCallGraphBlock *self,
										  GProfCallGraphBlockEntry *entry)
{
	if (!self->priv->primary_entry)
		self->priv->primary_entry = entry;
}

void 
gprof_call_graph_block_add_parent_entry (GProfCallGraphBlock *self,
										 GProfCallGraphBlockEntry *entry)
{
	self->priv->parents = g_list_append (self->priv->parents, entry);
	g_hash_table_insert (self->priv->lookup_set, 
						 gprof_call_graph_block_entry_get_name (entry), NULL);
}

void 
gprof_call_graph_block_add_child_entry (GProfCallGraphBlock *self,
										GProfCallGraphBlockEntry *entry)
{
	self->priv->children = g_list_append (self->priv->children, entry);
	g_hash_table_insert (self->priv->lookup_set, 
						 gprof_call_graph_block_entry_get_name (entry), NULL);
}

GProfCallGraphBlockEntry *
gprof_call_graph_block_get_primary_entry (GProfCallGraphBlock *self)
{
	return self->priv->primary_entry;
}

gboolean
gprof_call_graph_block_has_parents (GProfCallGraphBlock *self)
{
	return (self->priv->parents != NULL);
}

gboolean
gprof_call_graph_block_has_children (GProfCallGraphBlock *self)
{
	return (self->priv->children != NULL);
}

GProfCallGraphBlockEntry *
gprof_call_graph_block_get_first_child (GProfCallGraphBlock *self,
										GList **iter)
{
	*iter = self->priv->children;
	
	if (self->priv->children)
		return GPROF_CALL_GRAPH_BLOCK_ENTRY ((*iter)->data);
	else
		return NULL;
}

GProfCallGraphBlockEntry *
gprof_call_graph_block_get_first_parent (GProfCallGraphBlock *self,
										 GList **iter)
{
	*iter = self->priv->parents;
	
	if (self->priv->parents)
		return GPROF_CALL_GRAPH_BLOCK_ENTRY ((*iter)->data);
	else
		return NULL;
}

gboolean
gprof_call_graph_block_is_recursive (GProfCallGraphBlock *self)
{
	gchar *name;
	
	name = gprof_call_graph_block_entry_get_name (self->priv->primary_entry);
	
	/* Determine if this block refers to a recursive function. 
	 * Recursive blocks are those whose primary entry also is a parent
	 * or child of the primary. */
	return g_hash_table_lookup_extended (self->priv->lookup_set, name, NULL, 
										 NULL);
}

GProfCallGraphBlock *
gprof_call_graph_block_get_next (GList *current_iter, GList **next_iter)
{
	*next_iter = g_list_next (current_iter);
	
	if (*next_iter)
		return GPROF_CALL_GRAPH_BLOCK ((*next_iter)->data);
	else 
		return NULL;
}
