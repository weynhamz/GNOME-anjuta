/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gprof-call-graph.c
 * Copyright (C) James Liggett 2006 <jrliggett@cox.net>
 * 
 * gprof-call-graph.c is free software.
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

#include "gprof-call-graph.h"

#define BLOCK_SEPARATOR "-----------------------------------------------"

struct _GProfCallGraphPriv
{
	GList *blocks;  			 /* Straight list of all call graph blocks */
	GList *root;   				 /* List of strings that refer to 
							 	  * "root functions," or functions without 
								  * parents. */
	GHashTable *lookup_table;    /* Find function blocks by name */
};

static void
gprof_call_graph_init (GProfCallGraph *self)
{
	self->priv = g_new0 (GProfCallGraphPriv, 1);
	
	self->priv->lookup_table = g_hash_table_new (g_str_hash, g_str_equal);
}

static void 
gprof_call_graph_finalize (GObject *obj)
{
	GProfCallGraph *self;
	GList *current;
	
	self = (GProfCallGraph *) obj;
	
	g_hash_table_destroy (self->priv->lookup_table);
	
	current = self->priv->blocks;
	
	while (current)
	{
		gprof_call_graph_block_free (GPROF_CALL_GRAPH_BLOCK (current->data));
		current = g_list_next (current);
	}
	
	g_list_free (self->priv->blocks);
	g_list_free (self->priv->root);
	
	g_free (self->priv);
}

static void 
gprof_call_graph_class_init (GProfCallGraphClass *klass)
{
	GObjectClass *object_class;
	
	object_class = (GObjectClass *) klass;
	
	object_class->finalize = gprof_call_graph_finalize;
}

static gchar **
get_primary_line_fields(gchar *buffer)
{
	gchar **string_table;
	gchar *line;  /* line data without leading index number */
	gchar *calls_field;  /* Pointer to string that begins at the calls field */
	gint i;
	guint pos; /* Where are we in the buffer? */
	
	string_table = g_new0 (gchar *, 6);  /* NULL terminated */
	
	/* Skip over index number */
	line = strchr (buffer, ']') + 1;
	
	/* Read the first 3 fields */
	pos = 0;
	
	for (i = 0; i < 3; i++)
		string_table[i] = read_to_whitespace (&line[pos], &pos, pos);
	
	/* In some cases, uncalled functions will have empty calls field */
	calls_field = strip_whitespace (&line[pos]);
	
	/* If the field begins with a number, we have the field. Otherwise, 
	 * calls_field points to the function name. */
	
	/* In some cases, index numbers at the end of lines will be in parenthesis
	 * instead of square brackets. */
	
	if (g_ascii_isdigit(calls_field[0]))
	{
		string_table[3] = read_to_whitespace (&line[pos], &pos, pos);
		string_table[4] = read_to_delimiter (&line[pos], " [");
		
		if (!string_table[4])
			string_table[4] = read_to_delimiter (&line[pos], " (");
	}
	else
	{
		string_table[3] = g_strdup ("0");
		string_table[4] = read_to_delimiter (calls_field, " [");
		
		if (!string_table[4])
			string_table[4] = read_to_delimiter (calls_field, " (");
	}
	
	g_free (calls_field);
	
	return string_table;
}

static gchar **
get_secondary_line_fields(gchar *buffer)
{
	gchar **string_table;
	gchar *line; /* Pointer to the beginning of the current field */
	gint num_fields;  /* Number of fields read from line */
	gint pos;
	gint i;
	
	/* Secondary lines can have a maximum of 4 fields. In some cases (as in 
	 * recursive functions) lines may have only 2. Continue to read 
	 * fields until we encounter one that doesn't begin with a digit. This 
	 * tells us that we've reached the last field in the line. */
	
	string_table = g_new0 (gchar *, 5); /* NULL terminated */
	
	num_fields = 0;
	pos = 0;
	line = strip_whitespace (buffer);
	
	while (g_ascii_isdigit(line[0]))
	{
		string_table[num_fields] = read_to_whitespace (&buffer[pos], &pos, pos);
		g_free (line); 
		line = strip_whitespace (&buffer[pos]);
		num_fields++;	
	}

	g_free (line);
	
	/* If we only read one field, it refers to the calls field. If we didn't 
     * read any fields here, this tells us that the primary line that this
     * line belongs to refers to an uncalled function	*/
	
	if (num_fields == 1)
	{
		string_table[2] = string_table[0];
		
		for (i = 0; i < 2; i++)
			string_table[i] = g_strdup ("0");
	}
	else if (num_fields == 0)
	{
		g_free (string_table);
		return NULL;
	}
	
	/* In some cases, index numbers at the end of lines will be in parenthesis
	 * instead of square brackets. */
	string_table[3] = read_to_delimiter (&buffer[pos], " [");
	
	if (!string_table[3])
		string_table[3] = read_to_delimiter (&buffer[pos], " (");
		
	return string_table;
}

static void 
gprof_call_graph_add_block (GProfCallGraph *self, GProfCallGraphBlock *block)
{
	GProfCallGraphBlockEntry *entry;
	gchar *name;
	
	entry = gprof_call_graph_block_get_primary_entry (block);
	name = gprof_call_graph_block_entry_get_name (entry);
	
	self->priv->blocks = g_list_append (self->priv->blocks, block);
	g_hash_table_insert (self->priv->lookup_table, name, block);
	
	if (!gprof_call_graph_block_has_parents (block))
		self->priv->root = g_list_append (self->priv->root, block);
}

GType
gprof_call_graph_get_type ()
{
	static GType obj_type = 0;
	
	if (!obj_type)
	{
		static const GTypeInfo obj_info = 
		{
			sizeof (GProfCallGraphClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gprof_call_graph_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,           /* class_data */
			sizeof (GProfCallGraph),
			0,              /* n_preallocs */
			(GInstanceInitFunc) gprof_call_graph_init,
			NULL            /* value_table */
		};
		obj_type = g_type_register_static (G_TYPE_OBJECT,
		                                   "GProfCallGraph", &obj_info, 
											0);
	}
	return obj_type;
}

GProfCallGraph *
gprof_call_graph_new (FILE *stream, GProfFlatProfile *flat_profile)
{
	gchar buffer[PATH_MAX];
	gchar **fields;
	size_t length;
	GProfCallGraphBlock *block;
	GProfCallGraphBlockEntry *entry;
	gchar *entry_name;
	GProfCallGraph *call_graph;
	gboolean found_primary = FALSE;  
	
	call_graph = g_object_new (GPROF_CALL_GRAPH_TYPE, NULL);
	
	/* Find the beginning of the call graph. The call graph begins with
     * "index" */
	do 
	{
		/* Don't loop infinitely if we don't have any data */
		if (!fgets (buffer, PATH_MAX, stream))
			return call_graph;
			
	} while (strncmp ("index", buffer, 5) != 0);
	
	block = NULL; 
	
	while (fgets (buffer, PATH_MAX, stream))
	{
		/* If the first character of the line is 12, that's the end of the call
		 * graph. */
		if (buffer[0] == 12)
			break;
		
		if (!block)
		{
			block = gprof_call_graph_block_new ();
			found_primary = FALSE;
		}
		
		/* Remove the newline from the buffer */
		length = strlen (buffer);
		buffer[length - 1] = 0;
		
		/* If we encounter the block separator, add the block to the graph */
		if (strcmp (BLOCK_SEPARATOR, buffer) == 0)
		{	
			gprof_call_graph_add_block (call_graph, block);
			block = NULL;
		}
		else
		{
			/* If the buffer begins with an index number, treat it as a primary
			 * line */
			if (buffer[0] == '[')
			{
				found_primary = TRUE;
				
				fields = get_primary_line_fields (buffer);
				entry = gprof_call_graph_block_primary_entry_new (fields);

				g_strfreev (fields);
				
				gprof_call_graph_block_add_primary_entry (block, entry);
			}
			else
			{
				fields = get_secondary_line_fields (buffer);
				
				/* If we don't get any fields, that means this is a 
				 * placeholder line.*/
				if (!fields)
					continue;
				
				entry = gprof_call_graph_block_secondary_entry_new (fields);
				entry_name = gprof_call_graph_block_entry_get_name (entry);
				
				g_strfreev (fields);
				
				if (gprof_flat_profile_find_entry(flat_profile, entry_name))
				{
					if (found_primary)
						gprof_call_graph_block_add_child_entry (block, entry);
					else
						gprof_call_graph_block_add_parent_entry (block, entry);
				}
				else
					gprof_call_graph_block_entry_free (entry);
			}
		}
	}
	
	return call_graph;
}

void 
gprof_call_graph_free (GProfCallGraph *self)
{
	g_object_unref (self);
}

GProfCallGraphBlock *
gprof_call_graph_find_block (GProfCallGraph *self, gchar *name)
{
	return GPROF_CALL_GRAPH_BLOCK (g_hash_table_lookup (self->priv->lookup_table, 
														name));
}

GProfCallGraphBlock *
gprof_call_graph_get_first_block (GProfCallGraph *self, GList **iter)
{
	*iter = self->priv->blocks;
	
	if (*iter)
		return GPROF_CALL_GRAPH_BLOCK ((*iter)->data);
	else
		return NULL;
}

GProfCallGraphBlock *
gprof_call_graph_get_root (GProfCallGraph *self, GList **iter)
{
	*iter = self->priv->root;
	
	if (*iter)
		return GPROF_CALL_GRAPH_BLOCK ((*iter)->data);
	else
		return NULL;
}

void
gprof_call_graph_dump (GProfCallGraph *self, FILE *stream)
{
	GProfCallGraphBlockEntry *function;
	GProfCallGraphBlockEntry *current_parent;
	GProfCallGraphBlockEntry *current_child;
	GList *current_block;
	GList *current_child_iter;
	GList *current_parent_iter;
	
	current_block = self->priv->blocks;
	
	while (current_block)
	{
		function = gprof_call_graph_block_get_primary_entry (current_block->data);
		
		fprintf (stream, "Function: %s\n", 
				 gprof_call_graph_block_entry_get_name (function));
		fprintf (stream, "Time: %0.2f\n", 
				 gprof_call_graph_block_entry_get_time_perc (function));
		fprintf (stream, "Self sec: %0.2f\n", 
				 gprof_call_graph_block_entry_get_self_sec (function));
		fprintf (stream, "Child sec: %0.2f\n", 
				 gprof_call_graph_block_entry_get_child_sec (function));
		fprintf (stream, "Calls: %s\n", 
				 gprof_call_graph_block_entry_get_calls (function));
		fprintf (stream, "Recursive: %s\n\n", 
				 gprof_call_graph_block_is_recursive (current_block->data) ? "Yes" : "No");
		
		fprintf (stream, "Called: \n");
		
		current_child = gprof_call_graph_block_get_first_child (current_block->data, 
														  		&current_child_iter);
		
		while (current_child)
		{	
			fprintf (stream, "%s %0.2f, %0.2f, %0.2f, %s\n", 
					 gprof_call_graph_block_entry_get_name (current_child), 
				     gprof_call_graph_block_entry_get_time_perc (current_child), 
					 gprof_call_graph_block_entry_get_self_sec (current_child), 
				     gprof_call_graph_block_entry_get_child_sec (current_child), 
					 gprof_call_graph_block_entry_get_calls (current_child));
			
			current_child = gprof_call_graph_block_entry_get_next (current_child_iter, 
																   &current_child_iter);
		}
		fprintf (stream, "\n");
		
		fprintf (stream, "Called by: \n");
		
		current_parent = gprof_call_graph_block_get_first_parent (current_block->data, 
																  &current_parent_iter);
		while (current_parent)
		{	
			fprintf (stream, "%s %0.2f, %0.2f, %0.2f, %s\n", 
					 gprof_call_graph_block_entry_get_name (current_parent), 
				     gprof_call_graph_block_entry_get_time_perc (current_parent), 
					 gprof_call_graph_block_entry_get_self_sec (current_parent), 
				     gprof_call_graph_block_entry_get_child_sec (current_parent), 
					 gprof_call_graph_block_entry_get_calls (current_parent));
			
			current_parent = gprof_call_graph_block_entry_get_next (current_parent_iter, 
																	&current_parent_iter);
		}		
		fprintf (stream, "\n---\n\n");
		current_block = g_list_next (current_block);
	}
}
