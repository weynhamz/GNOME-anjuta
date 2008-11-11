/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    debug_tree.c
    Copyright (C) 2006  Sébastien Granjoux

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

// TODO:
//   Alloc string in DmaVariableData object, so treeview can use POINTER
//     => no need to free data after getting the value
//     => value can be access more easily if we have the tree viex
//     => do the same for variable name
//
//   Add a function to get all arguments

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "debug_tree.h"

#include "plugin.h"
#include "memory.h"

/*#define DEBUG*/
#include <libanjuta/anjuta-debug.h>

#include <glib/gi18n.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

/* Types
 *---------------------------------------------------------------------------*/

enum _DataType
{
	TYPE_ROOT,
	TYPE_UNKNOWN,
	TYPE_POINTER,
	TYPE_ARRAY,
	TYPE_STRUCT,
	TYPE_VALUE,
	TYPE_REFERENCE,
	TYPE_NAME
};

typedef enum _DataType DataType;

enum {AUTO_UPDATE_WATCH = 1 << 0};	

/* Common data */
typedef struct _CommonDebugTree CommonDebugTree;
typedef struct _DmaVariablePacket DmaVariablePacket;
typedef struct _DmaVariableData DmaVariableData;

struct _CommonDebugTree {
	guint initialized;
	DebugTree* current;
	DebugTree* user;
	GList *tree_list;
};

/* The debug tree object */
struct _DebugTree {
	DmaDebuggerQueue *debugger;
	AnjutaPlugin *plugin;
	GtkWidget* view;        /* the tree widget */
	gboolean auto_expand;
};

struct _DmaVariablePacket {
	DmaVariableData *data;
	GtkTreeModel *model;
	GtkTreeRowReference* reference;
	DebugTree *tree;
	DmaVariablePacket* next;
};

struct _DmaVariableData {
	guchar modified;    /* Set by tree update */
	guchar changed;     /* Set by global update */

	gboolean exited;	/* variable outside scope */
	gboolean deleted;	/* variable should be deleted */
	
	gboolean auto_update;
	
	DmaVariablePacket* packet;
		
	gchar* name;
};

/* Constant
 *---------------------------------------------------------------------------*/

#define UNKNOWN_VALUE "???"
#define UNKNOWN_TYPE "?"
#define AUTO_UPDATE 'U'

enum {
	VARIABLE_COLUMN,
	VALUE_COLUMN,
	TYPE_COLUMN,
	ROOT_COLUMN,
	DTREE_ENTRY_COLUMN,
	N_COLUMNS
};

static gchar *tree_title[] = {
	N_("Variable"), N_("Value"), N_("Type")
};

/* Global variable
 *---------------------------------------------------------------------------*/

static GList* gTreeList = NULL;

/* Helper functions
 *---------------------------------------------------------------------------*/

static void
my_gtk_tree_model_foreach_child (GtkTreeModel *const model,
                           GtkTreeIter *const parent,
                           GtkTreeModelForeachFunc func,
                           gpointer user_data)
{
  GtkTreeIter iter;
  gboolean success = gtk_tree_model_iter_children(model, &iter, parent);
							   
  while(success)
  {
    	success = (!func(model, NULL, &iter, user_data) &&
               gtk_tree_model_iter_next (model, &iter));
  }
}

static gboolean 
get_current_iter (GtkTreeView *view, GtkTreeIter* iter)
{
	GtkTreeSelection *selection;
	
	selection = gtk_tree_view_get_selection (view);
	return gtk_tree_selection_get_selected (selection, NULL, iter);
}

static DmaVariableData *
dma_variable_data_new(const gchar *const name, gboolean auto_update)
{
	DmaVariableData *data;
		
	data = g_new0 (DmaVariableData, 1);
	if (name != NULL)
	{
		data->name = g_strdup (name);
	}

	data->changed = TRUE;
	data->auto_update = auto_update;
		
	return data;
}

static void 
dma_variable_data_free(DmaVariableData * data)
{
	DmaVariablePacket* pack;
	
	/* Mark the data as invalid, the packet structure will
	 * be free later in the callback */
	for (pack = data->packet; pack != NULL; pack = pack->next)
	{
		pack->data= NULL;
	}
	
	if (data->name != NULL)
	{
		g_free (data->name);
	}
	
	g_free(data);
}

/* ------------------------------------------------------------------ */

static DmaVariablePacket *
dma_variable_packet_new(GtkTreeModel *model,
                     GtkTreeIter *iter,
					 DebugTree *tree,					 
					 DmaVariableData *data)
{
	GtkTreePath *path;
						 
	g_return_val_if_fail (model, NULL);
	g_return_val_if_fail (iter, NULL);

	DmaVariablePacket *pack = g_new (DmaVariablePacket, 1);

	pack->data = data;
	pack->model = GTK_TREE_MODEL (model);
	path = gtk_tree_model_get_path(model, iter); 
	pack->reference = gtk_tree_row_reference_new (model, path);
	gtk_tree_path_free (path);
	pack->tree = tree;
	pack->next = data->packet;
	data->packet = pack;

	return pack;
}

static void
dma_variable_packet_free (DmaVariablePacket* pack)
{
	if (pack->data != NULL)
	{
		/* Remove from packet data list */
		DmaVariablePacket **find;
		
		for (find = &pack->data->packet; *find != NULL; find = &(*find)->next)
		{
			if (*find == pack)
			{
				*find = pack->next;
				break;
			}
		}
	}
	
	gtk_tree_row_reference_free (pack->reference);
	
	g_free (pack);
}

static gboolean
dma_variable_packet_get_iter (DmaVariablePacket* pack, GtkTreeIter *iter)
{
	GtkTreePath *path;
	gboolean ok;
	
	path = gtk_tree_row_reference_get_path (pack->reference);
	ok = gtk_tree_model_get_iter (pack->model, iter, path);
	gtk_tree_path_free (path);
	
	return ok;
}

/* DebugTree private functions
 *---------------------------------------------------------------------------*/

static void
debug_tree_cell_data_func (GtkTreeViewColumn *tree_column,
				GtkCellRenderer *cell, GtkTreeModel *tree_model,
					GtkTreeIter *iter, gpointer data)
{
	gchar *value;
	static const gchar *colors[] = {"black", "red"};
	GValue gvalue = {0, };
	DmaVariableData *node = NULL;

	gtk_tree_model_get (tree_model, iter, VALUE_COLUMN, &value, -1);
	g_value_init (&gvalue, G_TYPE_STRING);
	g_value_set_static_string (&gvalue, value);
	g_object_set_property (G_OBJECT (cell), "text", &gvalue);

	gtk_tree_model_get (tree_model, iter, DTREE_ENTRY_COLUMN, &node, -1);
	
	if (node)
	{
		g_value_reset (&gvalue);
		g_value_set_static_string (&gvalue, 
                        colors[(node && node->modified ? 1 : 0)]);

		g_object_set_property (G_OBJECT (cell), "foreground", &gvalue);
	}
	g_free (value);
}

static gboolean
delete_child(GtkTreeModel *model, GtkTreePath* path,
			GtkTreeIter* iter, gpointer user_data)
{
	DmaVariableData *data;
	DebugTree* tree = (DebugTree *)user_data;

	g_return_val_if_fail (model,TRUE);
	g_return_val_if_fail (iter,TRUE);

	gtk_tree_model_get(model, iter, DTREE_ENTRY_COLUMN, &data, -1);	
	
	/* Dummy node (data == NULL) are used when child are not known */				
	if (data != NULL)
	{
		dma_variable_data_free(data);
	  	my_gtk_tree_model_foreach_child (model, iter, delete_child, tree);
	}
	
	return FALSE;
}

static gboolean
delete_parent(GtkTreeModel *model, GtkTreePath* path,
			GtkTreeIter* iter, gpointer user_data)
{
	DmaVariableData *data;
	DebugTree* tree = (DebugTree *)user_data;

	g_return_val_if_fail (model,TRUE);
	g_return_val_if_fail (iter,TRUE);

	gtk_tree_model_get(model, iter, DTREE_ENTRY_COLUMN, &data, -1);	

	/* Dummy node (data == NULL) are used as a place holder in watch box */				
	if (data != NULL)
	{
		if (tree->debugger)
		{
			if (data->name)
			{
				/* Object has been created in debugger and is not a child
			 	* (destroyed with their parent) */
				dma_queue_delete_variable (tree->debugger, data->name);
			}
		}				

		dma_variable_data_free(data);
				
  		my_gtk_tree_model_foreach_child (model, iter, delete_child, tree);
	}
	
	return FALSE;
}


static gboolean
set_deleted(GtkTreeModel *model, GtkTreePath* path,
				 GtkTreeIter* iter, gpointer user_data)
{
	DmaVariableData *data;

	g_return_val_if_fail (model,TRUE);
	g_return_val_if_fail (iter,TRUE);
	
	gtk_tree_model_get(model, iter, DTREE_ENTRY_COLUMN, &data, -1);
	g_return_val_if_fail (data, TRUE);	/* Use on root node only, data != NULL */

	data->deleted= TRUE;

	return FALSE;	
}

static void
debug_tree_remove_children (DebugTree *tree, GtkTreeIter* parent, GtkTreeIter* first)
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree->view));
	gboolean child;
 	GtkTreeIter iter;
	
	if (first != NULL)
	{
		/* Start with first child */
		child = TRUE;
		iter = *first;
	}
	else
	{
		/* Remove all children */
		child  = gtk_tree_model_iter_children(model, &iter, parent);
	}

	while (child)
	{
		delete_child (model, NULL, &iter, tree);
		
		child = gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
	}
}

static void
debug_tree_add_children (DebugTree *tree, GtkTreeIter* parent, const GList *children)
{
	GList *child;
	
	child = g_list_first ((GList *)children);
	
	if (child == NULL)
	{
		/* Clear all children if they exist */
		debug_tree_remove_children (tree, parent, NULL);
	}
	else
	{
		GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree->view));
		GtkTreeIter iter;
		gboolean valid_iter;
		
		valid_iter = gtk_tree_model_iter_children (model, &iter, parent); 
		
		for (; child != NULL; child = g_list_next (child))
		{
			IAnjutaDebuggerVariableObject *var = (IAnjutaDebuggerVariableObject *)child->data;
			DmaVariableData *data;
			
			if (!valid_iter)
			{
				/* Add new tree node */
				gtk_tree_store_append(GTK_TREE_STORE(model), &iter, parent);
				gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
					   TYPE_COLUMN, var->type == NULL ? UNKNOWN_TYPE : var->type,
					   VALUE_COLUMN, var->value == NULL ? UNKNOWN_VALUE : var->value,
					   VARIABLE_COLUMN, var->expression,
					   ROOT_COLUMN, FALSE,
					   DTREE_ENTRY_COLUMN, NULL,-1);
				data = NULL;
			}
			else
			{
				/* Update tree node */
				if (var->type != NULL)
					gtk_tree_store_set(GTK_TREE_STORE(model), &iter, TYPE_COLUMN, var->type, -1);
				if (var->value != NULL)
					gtk_tree_store_set(GTK_TREE_STORE(model), &iter, VALUE_COLUMN, var->value, -1);
				if (var->expression != NULL)
					gtk_tree_store_set(GTK_TREE_STORE(model), &iter, VARIABLE_COLUMN, var->expression, -1);
				gtk_tree_model_get(model, &iter, DTREE_ENTRY_COLUMN, &data, -1);	
			}
				
			if (data == NULL)
			{
				/* Create new data */
				data = dma_variable_data_new(var->name, FALSE);
				gtk_tree_store_set(GTK_TREE_STORE(model), &iter, DTREE_ENTRY_COLUMN, data, -1);				
			}
			else
			{
				data->changed = TRUE;
			}
		
			if (var->children == 0)
			{
				/* Clear all children if they exist */
				debug_tree_remove_children (tree, &iter, NULL);
			}
			else
			{
				/* Add dummy children */
				debug_tree_add_dummy (tree, &iter);
			}
			
			valid_iter = gtk_tree_model_iter_next (model, &iter); 
		}
		if (valid_iter)
		{
			/* Remove trailing children */
			debug_tree_remove_children (tree, parent, &iter);
		}
	}	
}

static void
destroy_deleted (DebugTree *tree, GtkTreeModel* model)
{
	DmaVariableData *data;
	GtkTreeIter iter;
	gboolean success;
	
	g_return_if_fail (model);

	for (success = gtk_tree_model_get_iter_first (model, &iter); success == TRUE; )
	{
		gtk_tree_model_get(model, &iter, DTREE_ENTRY_COLUMN, &data, -1);
		if ((data != NULL) && (data->deleted == TRUE))
		{
			success = debug_tree_remove (tree, &iter);
		}
		else
		{
			success = gtk_tree_model_iter_next (model, &iter);
		}
	};
}

/*---------------------------------------------------------------------------*/

static void
gdb_var_evaluate_expression (const gchar *value,
                        gpointer user_data, GError* err)
{
	DmaVariablePacket *pack = (DmaVariablePacket *) user_data;
	GtkTreeIter iter;
							
	g_return_if_fail (pack != NULL);

	if ((err != NULL)
		|| (pack->data == NULL)
		|| !dma_variable_packet_get_iter (pack, &iter))
	{
		/* Command failed or item has been deleted */
		dma_variable_packet_free (pack);
		
		return;
	}

	pack->data->changed = FALSE;
	gtk_tree_store_set(GTK_TREE_STORE (pack->model), &iter, VALUE_COLUMN, value, -1);
	dma_variable_packet_free (pack);
}

static void
gdb_var_list_children (const GList *children, gpointer user_data, GError *err)
{
	DmaVariablePacket *pack = (DmaVariablePacket *) user_data;
	GtkTreeIter iter;
	
	g_return_if_fail (pack != NULL);

	if ((err != NULL)
		|| (pack->data == NULL)
		|| !dma_variable_packet_get_iter (pack, &iter))
	{
		/* Command failed or item has been deleted */
		dma_variable_packet_free (pack);
		
		return;
	}

	debug_tree_add_children (pack->tree, &iter, children);
		  
	dma_variable_packet_free (pack);
}

static void
gdb_var_create (IAnjutaDebuggerVariableObject *variable, gpointer user_data, GError *err)
{
	DmaVariablePacket *pack = (DmaVariablePacket *) user_data;
	GtkTreeIter iter;
	
	g_return_if_fail (pack != NULL);

	if (err != NULL)
	{
		/* Command failed */
		dma_variable_packet_free (pack);
		
		return;
	}
	if ((pack->data == NULL)
		|| !dma_variable_packet_get_iter (pack, &iter))
	{
		/* Item has been deleted, but not removed from debugger as it was not
		 * created at this time, so remove it now */
		if ((pack->tree->debugger) && (variable->name))
		{
			dma_queue_delete_variable (pack->tree->debugger, variable->name);
		}
		dma_variable_packet_free (pack);
		
		return;
	}

	DmaVariableData *data = pack->data;
	
	if ((variable->name != NULL) && (data->name == NULL))
	{
		data->name = strdup (variable->name);
	}
    data->changed = TRUE;
	
	gtk_tree_store_set(GTK_TREE_STORE(pack->model), &iter,
					   TYPE_COLUMN, variable->type,
					   VALUE_COLUMN, variable->value, -1);
	

	if (variable->children == -1)
	{
		/* Find the number of children */
		DmaVariablePacket *pack_child =
                  dma_variable_packet_new(pack->model, &iter, pack->tree, data);

		dma_queue_list_children (
				pack_child->tree->debugger,
				variable->name,
				(IAnjutaDebuggerCallback)gdb_var_list_children,
				pack_child);
	}
	else if (variable->children > 0)
	{
		debug_tree_add_dummy (pack->tree, &iter);
	}
	else
	{
		debug_tree_remove_children (pack->tree, &iter, NULL);
	}
	
	
	/* Request value and/or children if they are missing
	 * reusing same packet if possible */
	if (variable->value == NULL)
	{
		dma_queue_evaluate_variable (
				pack->tree->debugger,
				variable->name,
				(IAnjutaDebuggerCallback)gdb_var_evaluate_expression,
				pack);
	}
	else
	{
		dma_variable_packet_free (pack);
	}
}

/* ------------------------------------------------------------------ */

static void
on_treeview_row_expanded       (GtkTreeView     *treeview,
                                 GtkTreeIter     *iter,
                                 GtkTreePath     *path,
                                 gpointer         user_data)
{
	DebugTree *tree = (DebugTree *)user_data;
	GtkTreeModel *const model = gtk_tree_view_get_model (treeview);
	DmaVariableData *data;
									 
	gtk_tree_model_get (model, iter, DTREE_ENTRY_COLUMN, &data, -1);

	if ((data != NULL) && (data->name != NULL) && (tree->debugger != NULL))
	{
		DmaVariablePacket *pack;
					
		pack = dma_variable_packet_new(model, iter, tree, data);
		dma_queue_list_children (
						tree->debugger,
						data->name,
						(IAnjutaDebuggerCallback)gdb_var_list_children,
						pack);
	}
								 
	return;
}

static void
on_debug_tree_variable_changed (GtkCellRendererText *cell,
						  gchar *path_string,
                          gchar *text,
                          gpointer user_data)
{
	DebugTree *tree = (DebugTree *)user_data;
	GtkTreeIter iter;
	GtkTreeModel * model;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree->view));
	if (gtk_tree_model_get_iter_from_string (model, &iter, path_string))
	{
		debug_tree_remove (tree, &iter);
		
		if ((text != NULL) && (*text != '\0'))
		{
		    IAnjutaDebuggerVariableObject var = {NULL, NULL, NULL, NULL, FALSE, FALSE, FALSE, -1};
			
			var.expression = text;
			debug_tree_add_watch (tree, &var, TRUE);
		}
	}
}

static void
on_debug_tree_value_changed (GtkCellRendererText *cell,
						  gchar *path_string,
                          gchar *text,
                          gpointer user_data)
{
	DebugTree *tree = (DebugTree *)user_data;
	GtkTreeIter iter;
	GtkTreeModel * model;
	
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree->view));

	if (gtk_tree_model_get_iter_from_string (model, &iter, path_string))
	{
		DmaVariableData *item;
		DmaVariablePacket *tran;

		gtk_tree_model_get (model, &iter, DTREE_ENTRY_COLUMN, &item, -1);
		if ((item != NULL) && (item->name != NULL) && (tree->debugger != NULL))
		{
			/* Variable is valid */
			dma_queue_assign_variable (tree->debugger, item->name, text);
			tran = dma_variable_packet_new(model, &iter, tree, item);
			dma_queue_evaluate_variable (
					tree->debugger,
					item->name,
					(IAnjutaDebuggerCallback) gdb_var_evaluate_expression,
					tran);
		}
	}
}

static GtkWidget *
debug_tree_create (DebugTree *tree, GtkTreeView *view)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeModel * model = GTK_TREE_MODEL (gtk_tree_store_new
			                     (N_COLUMNS, 
	                                          G_TYPE_STRING, 
	                                          G_TYPE_STRING,
                                              G_TYPE_STRING,
								              G_TYPE_BOOLEAN,
			                      G_TYPE_POINTER));
	
	if (view == NULL)
	{
		view = GTK_TREE_VIEW (gtk_tree_view_new ());
	}
	
    gtk_tree_view_set_model (view, GTK_TREE_MODEL (model));
  
	GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	g_object_unref (G_OBJECT (model));

	/* Columns */
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text", VARIABLE_COLUMN);
	gtk_tree_view_column_add_attribute (column, renderer, "editable", ROOT_COLUMN);
	g_signal_connect(renderer, "edited", (GCallback) on_debug_tree_variable_changed, tree);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _(tree_title[0]));
	gtk_tree_view_append_column (view, column);
	gtk_tree_view_set_expander_column (view, column);

	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column, renderer,
                               debug_tree_cell_data_func, NULL, NULL);
	gtk_tree_view_column_add_attribute (column, renderer, "text", VALUE_COLUMN);
	g_object_set(renderer, "editable", TRUE, NULL);	
	g_signal_connect(renderer, "edited", (GCallback) on_debug_tree_value_changed, tree);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _(tree_title[1]));
	gtk_tree_view_append_column (view, column);


    column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text", TYPE_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _(tree_title[2]));
	gtk_tree_view_append_column (view, column);

    return GTK_WIDGET (view);
}

/* Public functions
 *---------------------------------------------------------------------------*/

/* clear the display of the debug tree and reset the title */
void
debug_tree_remove_all (DebugTree *tree)
{
	GtkTreeModel *model;

	g_return_if_fail (tree);
	g_return_if_fail (tree->view);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree->view));
	debug_tree_remove_model (tree, model);
}

static gboolean
debug_tree_find_name (const GtkTreeModel *model, GtkTreeIter *iter, const gchar *name)
{
	size_t len = 0;
	GtkTreeIter parent_iter;
	GtkTreeIter* parent = NULL;
	
	for (;;)
	{
		const gchar *ptr;
		
		/* Check if we look for a child variable */
		ptr = strchr(name + len + 1, '.');
		if (ptr != NULL)
		{
			/* Child variable */
			gboolean search;
			
			len = ptr - name;
			for (search = gtk_tree_model_iter_children (GTK_TREE_MODEL (model), iter, parent);
				search != FALSE;
	    		search = gtk_tree_model_iter_next (GTK_TREE_MODEL (model), iter))
			{
				DmaVariableData *iData;
        		gtk_tree_model_get (GTK_TREE_MODEL (model), iter, DTREE_ENTRY_COLUMN, &iData, -1);
			
				if ((iData != NULL) && (iData->name != NULL) && (name[len] == '.') && (strncmp (name, iData->name, len) == 0))
				{
					break;
				}
			}
			
			if (search == TRUE)
			{
				parent_iter = *iter;
				parent = &parent_iter;
				continue;
			}
			else
			{
				return FALSE;
			}
		}
		else
		{
			/* Variable without any child */
			gboolean search;

			for (search = gtk_tree_model_iter_children (GTK_TREE_MODEL (model), iter, parent);
				search != FALSE;
				search = gtk_tree_model_iter_next (GTK_TREE_MODEL (model), iter))
			{
				DmaVariableData *iData;
				gtk_tree_model_get (GTK_TREE_MODEL (model), iter, DTREE_ENTRY_COLUMN, &iData, -1);
			
				if ((iData != NULL) && (iData->name != NULL) && (strcmp (name, iData->name) == 0))
				{
					return TRUE;
				}
			}
			
			return FALSE;
		}
	}
}

static gboolean
debug_tree_find_expression (const GtkTreeModel *model, GtkTreeIter *iter, const gchar *expression, const gchar *type)
{
	gboolean search;
	
	for (search = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), iter);
		search;
	    search = gtk_tree_model_iter_next (GTK_TREE_MODEL (model), iter))
	{
		gchar *exp;
		gchar *typ;
		DmaVariableData *node;
		gboolean found;
		
        gtk_tree_model_get (GTK_TREE_MODEL (model), iter, TYPE_COLUMN, &typ,
							VARIABLE_COLUMN, &exp,
							DTREE_ENTRY_COLUMN, &node, -1);

		found = ((type == NULL) || (strcmp (typ, type) == 0))
			&& ((expression == NULL) || (strcmp (exp, expression) == 0))
			&& (node->exited == FALSE);
		if (typ != NULL) g_free (typ);
		if (exp != NULL) g_free (exp);
		
		if (found) return TRUE;
	}
	
	return FALSE;
}

static void
on_replace_watch (gpointer data, gpointer user_data)
{
	DebugTree* tree = (DebugTree *)user_data;
	const gchar *expression = (const gchar *)data;
	GtkTreeModel*const model = gtk_tree_view_get_model (GTK_TREE_VIEW(tree->view));
	IAnjutaDebuggerVariableObject var = {NULL, NULL, NULL, NULL, FALSE, FALSE, FALSE, -1};
	GtkTreeIter iter;

	if (debug_tree_find_expression (model, &iter, expression, NULL))
	{
		DmaVariableData *data;

        	gtk_tree_model_get (model, &iter, DTREE_ENTRY_COLUMN, &data, -1);
		if (data != NULL) data->deleted = FALSE;
	}
	else
	{
		var.expression = (gchar *)expression;
		debug_tree_add_watch (tree, &var, TRUE);
	}
}

void
debug_tree_replace_list (DebugTree *tree, const GList *expressions)
{
	GtkTreeModel* model = gtk_tree_view_get_model (GTK_TREE_VIEW(tree->view));
	
	/* Mark variables as deleted */
	my_gtk_tree_model_foreach_child(model, NULL, set_deleted, NULL);	

	g_list_foreach ((GList *)expressions, on_replace_watch, tree);	

	destroy_deleted (tree, model);	
}

void
debug_tree_add_dummy (DebugTree *tree, GtkTreeIter *parent)
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree->view));
	GtkTreeIter iter;
	
	gtk_tree_store_append(GTK_TREE_STORE(model), &iter, parent);
	gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
					   VARIABLE_COLUMN, "",
					   VALUE_COLUMN, "",
					   TYPE_COLUMN, "",
					   ROOT_COLUMN, parent == NULL ? TRUE : FALSE,
					   DTREE_ENTRY_COLUMN, NULL, -1);
}

/* Get a IAnjutaVariable as argument in order to use the same function without
 * variable object, currently only the expression field is set */

void
debug_tree_add_watch (DebugTree *tree, const IAnjutaDebuggerVariableObject* var, gboolean auto_update)
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree->view));
	GtkTreeIter iter;
	DmaVariableData *data;
	
	/* Allocate data */
	data = dma_variable_data_new(var->name, auto_update);
	
	/* Add node in tree */
	gtk_tree_store_append(GTK_TREE_STORE(model), &iter, NULL);
	gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
					   TYPE_COLUMN, var->type == NULL ? UNKNOWN_TYPE : var->type,
					   VALUE_COLUMN, var->value == NULL ? UNKNOWN_VALUE : var->value,
					   VARIABLE_COLUMN, var->expression,
					   ROOT_COLUMN, TRUE,
					   DTREE_ENTRY_COLUMN, data, -1);

	if (tree->debugger != NULL)
	{
		if ((var->value == NULL) || (var->children == -1))
		{
			if (var->name == NULL)
			{
				/* Need to create variable before to get value */
				DmaVariablePacket *pack;
		
				pack = dma_variable_packet_new(model, &iter, tree, data);
				dma_queue_create_variable (
						tree->debugger,
						var->expression,
						(IAnjutaDebuggerCallback)gdb_var_create,
						pack);
			}
			else
			{
				DEBUG_PRINT("%s", "You shouldn't read this, debug_tree_add_watch");
				if (var->value == NULL)
				{
					/* Get value */
					DmaVariablePacket *pack =
					
					pack = dma_variable_packet_new(model, &iter, tree, data);
					dma_queue_evaluate_variable (
							tree->debugger,
							var->name,
							(IAnjutaDebuggerCallback)gdb_var_evaluate_expression,
							pack);
				}
				if (var->children == -1)
				{
					/* Get number of children */
					DmaVariablePacket *pack =
					
					pack = dma_variable_packet_new(model, &iter, tree, data);
					dma_queue_list_children (
							tree->debugger,
							var->name,
							(IAnjutaDebuggerCallback)gdb_var_list_children,
							pack);
				}
			}
		}
	}
}

static void
on_add_watch (gpointer data, gpointer user_data)
{
	DebugTree* this = (DebugTree *)user_data;
	gboolean auto_update = ((const gchar *)data)[0] & AUTO_UPDATE_WATCH ? TRUE : FALSE;
	IAnjutaDebuggerVariableObject var = {NULL, NULL, NULL, NULL, FALSE, FALSE, FALSE, -1};

	var.expression = &((gchar *)data)[1];
	debug_tree_add_watch (this, &var, auto_update);
}

void
debug_tree_add_full_watch_list (DebugTree *this, GList *expressions)
{
	g_list_foreach (expressions, on_add_watch, this);
}

static void
on_add_manual_watch (gpointer data, gpointer user_data)
{
	DebugTree* this = (DebugTree *)user_data;
	IAnjutaDebuggerVariableObject var = {NULL, NULL, NULL, NULL, FALSE, FALSE, FALSE, -1};

	var.expression = &((gchar *)data)[0];
	debug_tree_add_watch (this, &var, FALSE);
}

static void
on_add_auto_watch (gpointer data, gpointer user_data)
{
	DebugTree* this = (DebugTree *)user_data;
	IAnjutaDebuggerVariableObject var = {NULL, NULL, NULL, NULL, FALSE, FALSE, -1};
	
	var.expression = &((gchar *)data)[0];
	debug_tree_add_watch (this, &var, TRUE);
}

void
debug_tree_add_watch_list (DebugTree *this, GList *expressions, gboolean auto_update)
{
	g_list_foreach (expressions, auto_update ? on_add_auto_watch : on_add_manual_watch, this);
}

static void
on_debug_tree_changed (gpointer data, gpointer user_data)
{
	IAnjutaDebuggerVariableObject *var = (IAnjutaDebuggerVariableObject *)data;

	if (var->name != NULL)
	{
		/* Search corresponding variable in one tree */
		GList *tree;
		
		for (tree = g_list_first (gTreeList); tree != NULL; tree = g_list_next (tree))
		{
			GtkTreeIter iter;
			GtkTreeModel *model;
			
			model =  GTK_TREE_MODEL (tree->data);
			
			if (debug_tree_find_name (model, &iter, var->name))
			{
				DmaVariableData *data;
				gtk_tree_model_get (model, &iter, DTREE_ENTRY_COLUMN, &data, -1);
		
				if (data != NULL)
				{	
					data->changed = var->changed;
					data->exited = var->exited;
					data->deleted = var->deleted;
				}
				
				return;
			}
		}
	}
}

static gboolean
on_debug_tree_modified (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data)
{
	DmaVariableData *data = NULL;
	
	gtk_tree_model_get (model, iter, DTREE_ENTRY_COLUMN, &data, -1);
	
	if (data != NULL)	/* Avoid dummy node */
	{
		if (data->modified != data->changed)
		{
			data->modified = data->changed;
			gtk_tree_model_row_changed (model, path, iter);
		}
		data->changed = FALSE;
	}

	return FALSE;
}

static gboolean
debug_tree_update_real (DebugTree* tree, GtkTreeIter* iter, gboolean force)
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree->view));
	DmaVariableData *data = NULL;
	GtkTreeIter child;
	gboolean search;
	gboolean refresh = TRUE;

	gtk_tree_model_get (model, iter, DTREE_ENTRY_COLUMN, &data, -1);
	if (data == NULL) return FALSE;

	if (data->name == NULL)
	{
		/* Check is the variable creation is not pending */
		if (data->packet == NULL)
		{
			/* Variable need to be created first */
			gchar *exp;
			DmaVariablePacket *pack;
		
			gtk_tree_model_get (model, iter, VARIABLE_COLUMN, &exp, -1);
			pack = dma_variable_packet_new(model, iter, tree, data);
			data->modified = TRUE;
			dma_queue_create_variable (
					tree->debugger,
					exp,
					(IAnjutaDebuggerCallback)gdb_var_create,
					pack);
			g_free (exp);
		}
		
		return FALSE;
	}	
	else if (force || (data->auto_update && data->changed))
	{
		DmaVariablePacket *pack = dma_variable_packet_new(model, iter, tree, data);
		refresh = data->modified != (data->changed != FALSE);
		data->modified = (data->changed != FALSE);
		dma_queue_evaluate_variable (
				tree->debugger,
				data->name,
				(IAnjutaDebuggerCallback)gdb_var_evaluate_expression,
				pack);
	}
	else
	{
		refresh = data->modified;
		data->modified = FALSE;
	}
	
	/* update children */
	for (search = gtk_tree_model_iter_children(model, &child, iter);
		search == TRUE;
	    search = gtk_tree_model_iter_next (model, &child))
	{
		if (debug_tree_update_real (tree, &child, force))
		{
			refresh = data->modified == TRUE;
			data->modified = TRUE;
		}
	}

	if (refresh)
	{
		GtkTreePath *path;
		path = gtk_tree_model_get_path (model, iter);
		gtk_tree_model_row_changed (model, path, iter);
		gtk_tree_path_free (path);
	}
	
	return data->modified;
}

static void
on_debug_tree_update_all (const GList *change, gpointer user_data, GError* err)
{
	DebugTree *tree = (DebugTree *)user_data;
	GtkTreeModel *model;
	GtkTreeIter iter;

	if (err != NULL) return;

	// Mark all modified variables
	g_list_foreach ((GList *)change, on_debug_tree_changed, NULL);

	// Update this tree
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree->view));
	if (gtk_tree_model_get_iter_first (model, &iter) == TRUE)
	{
		do
		{
			DmaVariableData *data = NULL;
			
			gtk_tree_model_get (model, &iter, DTREE_ENTRY_COLUMN, &data, -1);
	
			debug_tree_update_real (tree, &iter, FALSE);
		} while (gtk_tree_model_iter_next (model, &iter) == TRUE);
	}
	
	// Update modified mark
	gtk_tree_model_foreach (model, on_debug_tree_modified, NULL);
}

void
debug_tree_update_all (DebugTree* tree)
{
	if (tree->debugger != NULL)
	{
		/* Update if debugger is connected */
		dma_queue_update_variable (
				tree->debugger,
				(IAnjutaDebuggerCallback)on_debug_tree_update_all,
				tree);
	}
}

GList*
debug_tree_get_full_watch_list (DebugTree *this)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GList* list = NULL;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (this->view));
	
	if (gtk_tree_model_get_iter_first (model, &iter) == TRUE)
	{
		do
		{
			DmaVariableData *data;
			gchar *exp;
			gchar *exp_with_flag;
	
			gtk_tree_model_get(model, &iter, DTREE_ENTRY_COLUMN, &data, 
							                 VARIABLE_COLUMN, &exp, -1);	

			if (data != NULL)
			{
				exp_with_flag = g_strconcat (" ", exp, NULL); 
				exp_with_flag[0] = data->auto_update ? AUTO_UPDATE_WATCH : ' ';
				list = g_list_prepend (list, exp_with_flag);
			}
			g_free (exp);
		} while (gtk_tree_model_iter_next (model, &iter) == TRUE);
	}
	
	list = g_list_reverse (list);
	
	return list;
}


GtkWidget *
debug_tree_get_tree_widget (DebugTree *this)
{
	return this->view;
}

gboolean
debug_tree_get_current (DebugTree *tree, GtkTreeIter* iter)
{
	return get_current_iter (GTK_TREE_VIEW (tree->view), iter);
}

/* Return TRUE if iter is still valid (point to next item) */
gboolean
debug_tree_remove (DebugTree *tree, GtkTreeIter* iter)
{
	GtkTreeModel *model;

	g_return_val_if_fail (tree, FALSE);
	g_return_val_if_fail (tree->view, FALSE);
	g_return_val_if_fail (iter, FALSE);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree->view));
	
	delete_parent (model, NULL, iter, tree);
	return gtk_tree_store_remove (GTK_TREE_STORE (model), iter);
}

gboolean
debug_tree_update (DebugTree* tree, GtkTreeIter* iter, gboolean force)
{
	if (tree->debugger != NULL)
	{
		return debug_tree_update_real (tree, iter, force);
	}
	else
	{
		return FALSE;
	}
}

void
debug_tree_set_auto_update (DebugTree* this, GtkTreeIter* iter, gboolean state)
{
	GtkTreeModel *model;
	DmaVariableData *data;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (this->view));
	gtk_tree_model_get(model, iter, DTREE_ENTRY_COLUMN, &data, -1);	
	if (data != NULL)
	{
		data->auto_update = state;
	}
}

gboolean
debug_tree_get_auto_update (DebugTree* this, GtkTreeIter* iter)
{
	GtkTreeModel *model;
	DmaVariableData *data;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (this->view));
	gtk_tree_model_get(model, iter, DTREE_ENTRY_COLUMN, &data, -1);	
	
	if (data != NULL)
	{
		return data->auto_update;
	}
	else
	{
		return FALSE;
	}
}

void
debug_tree_connect (DebugTree *this, DmaDebuggerQueue* debugger)
{
	this->debugger = debugger;
}

void
debug_tree_disconnect (DebugTree *this)
{
	this->debugger = NULL;
}

GtkTreeModel *
debug_tree_get_model (DebugTree *tree)
{
	return gtk_tree_view_get_model (GTK_TREE_VIEW (tree->view));
}

void
debug_tree_set_model (DebugTree *tree, GtkTreeModel *model)
{
	gtk_tree_view_set_model (GTK_TREE_VIEW (tree->view), model);
}

void
debug_tree_new_model (DebugTree *tree)
{
	GtkTreeModel * model = GTK_TREE_MODEL (gtk_tree_store_new
		                     (N_COLUMNS, 
	                          G_TYPE_STRING, 
	                          G_TYPE_STRING,
                              G_TYPE_STRING,
				              G_TYPE_BOOLEAN,
			                  G_TYPE_POINTER));

	gtk_tree_view_set_model (GTK_TREE_VIEW (tree->view), model);
}

void
debug_tree_remove_model (DebugTree *tree, GtkTreeModel *model)
{
	my_gtk_tree_model_foreach_child (model, NULL, delete_parent, tree);
	gtk_tree_store_clear (GTK_TREE_STORE (model));
}

gchar *
debug_tree_get_selected (DebugTree *tree)
{
	GtkTreeIter iter;
	gchar *exp = NULL;

	if (get_current_iter (GTK_TREE_VIEW (tree->view), &iter))
	{
		GtkTreeModel *model;

		model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree->view));
		if (model != NULL)
		{
			gtk_tree_model_get(model, &iter, VARIABLE_COLUMN, &exp, -1);
		}
	}
	
	return exp;
}

gchar *
debug_tree_get_first (DebugTree *tree)
{
	GtkTreeIter iter;
	gchar *exp = NULL;
	GtkTreeModel *model;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree->view));
	if (model != NULL)
	{
		if (gtk_tree_model_get_iter_first (model, &iter))
		{
			gtk_tree_model_get(model, &iter, VARIABLE_COLUMN, &exp, -1);
		}
	}
	
	return exp;
}

gchar*
debug_tree_find_variable_value (DebugTree *tree, const gchar *name)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree->view));
	
	if (gtk_tree_model_get_iter_first (model, &iter))
	{
		gchar *exp;
		gchar *value;
		do
		{
			gtk_tree_model_get(model, &iter, VARIABLE_COLUMN, &exp, 
							                 VALUE_COLUMN, &value, -1);
			
			if (strcmp (exp, name) == 0)
			{
				return value;
			}
		} while (gtk_tree_model_iter_next (model, &iter));
	}
	
	return NULL;
}

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

/* return a pointer to a newly allocated DebugTree object */
DebugTree *
debug_tree_new_with_view (AnjutaPlugin *plugin, GtkTreeView *view)
{
	DebugTree *tree = g_new0 (DebugTree, 1);
	GtkTreeModel *model;	

	tree->plugin = plugin;
	tree->view = debug_tree_create(tree, view);
	tree->auto_expand = FALSE;

	/* Add this model in list */
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree->view));
	gTreeList = g_list_prepend (gTreeList, model);
	
	/* Connect signal */
    g_signal_connect(GTK_TREE_VIEW (tree->view), "row_expanded", G_CALLBACK (on_treeview_row_expanded), tree);
	

	return tree;
}


/* return a pointer to a newly allocated DebugTree object */
DebugTree *
debug_tree_new (AnjutaPlugin* plugin)
{
	return debug_tree_new_with_view (plugin, NULL);
}

/* DebugTree destructor */
void
debug_tree_free (DebugTree * tree)
{
	GtkTreeModel *model;
	
	g_return_if_fail (tree);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree->view));
	debug_tree_remove_all (tree);

	/* Remove from list */
	gTreeList = g_list_remove (gTreeList, model);
	
	g_signal_handlers_disconnect_by_func (GTK_TREE_VIEW (tree->view),
				  G_CALLBACK (on_treeview_row_expanded), tree);
	
	gtk_widget_destroy (tree->view);
	
	
	g_free (tree);
}
