/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * symbol-db-model.c
 * Copyright (C) Naba Kumar 2010 <naba@gnome.org>
 * 
 * anjuta is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libgda/libgda.h>
#include "symbol-db-marshal.h"
#include "symbol-db-model.h"

#define SYMBOL_DB_MODEL_STAMP 5364558

#define GET_PRIV(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE((o), SYMBOL_DB_TYPE_MODEL, SymbolDBModelPriv))

/* Constants */

#define SYMBOL_DB_MODEL_PAGE_SIZE 50
#define SYMBOL_DB_MODEL_ENSURE_CHILDREN_BATCH_SIZE 10

typedef struct _SymbolDBModelPage SymbolDBModelPage;
struct _SymbolDBModelPage
{
	gint begin_offset, end_offset;
	SymbolDBModelPage *prev;
	SymbolDBModelPage *next;
};

typedef struct _SymbolDBModelNode SymbolDBModelNode;
struct _SymbolDBModelNode {

	/* Column values of the node. This is an array of GValues of length
	 * n_column. and holds the values in order of columns given at
	 * object initialized.
	 */
	GValue *values;

	/* List of currently active (cached) pages */
	SymbolDBModelPage *pages;
	
	/* Data structure */
	gint level;
	SymbolDBModelNode *parent;
	gint offset;

	/* Children states */
	gint children_ref_count;
	gboolean children_ensured;
	guint n_children;
	SymbolDBModelNode **children;
};

typedef struct {
	/* Keeps track of model freeze count. When the model is frozen, it
	 * avoid retreiving data from backend. It does not freeze the frontend
	 * view at all and instead use empty data for the duration of freeze.
	 */
	gint freeze_count;
	
	gint n_columns;      /* Number of columns in the model */
	GType *column_types; /* Type of each column in the model */
	gint *query_columns; /* Corresponding GdaDataModel column */
	
	/* Idle child-ensure queue */
	GQueue *ensure_children_queue;
	guint ensure_children_idle_id;
	
	SymbolDBModelNode *root;
} SymbolDBModelPriv;

enum {
	SIGNAL_GET_HAS_CHILD,
	SIGNAL_GET_N_CHILDREN,
	SIGNAL_GET_CHILDREN,
	LAST_SIGNAL
};

static guint symbol_db_model_signals[LAST_SIGNAL] = { 0 };

/* Declarations */

static void symbol_db_model_node_free (SymbolDBModelNode *node, gboolean force);

static void symbol_db_model_tree_model_init (GtkTreeModelIface *iface);

static gboolean symbol_db_model_get_query_value_at (SymbolDBModel *model,
                                                    GdaDataModel *data_model,
                                                    gint position, gint column,
                                                    GValue *value);

static gboolean symbol_db_model_get_query_value (SymbolDBModel *model,
                                                 GdaDataModel *data_model,
                                                 GdaDataModelIter *iter,
                                                 gint column,
                                                 GValue *value);

static gint symbol_db_model_get_n_children (SymbolDBModel *model,
                                            gint tree_level,
                                            GValue column_values[]);

static GdaDataModel* symbol_db_model_get_children (SymbolDBModel *model,
                                                   gint tree_level,
                                                   GValue column_values[],
                                                   gint offset, gint limit);

static void symbol_db_model_queue_ensure_node_children (SymbolDBModel *model,
                                                        SymbolDBModelNode *parent);

static void symbol_db_model_ensure_node_children (SymbolDBModel *model,
                                                  SymbolDBModelNode *parent,
                                                  gboolean emit_has_child);

/* Class definition */
G_DEFINE_TYPE_WITH_CODE (SymbolDBModel, symbol_db_model, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
                                                symbol_db_model_tree_model_init))
/* Node */

static GNUC_INLINE SymbolDBModelNode*
symbol_db_model_node_get_child (SymbolDBModelNode *node, gint child_offset)
{
	g_return_val_if_fail (node != NULL, NULL);
	g_return_val_if_fail (child_offset >= 0 && child_offset < node->n_children, NULL);
	if(node->children)
		return node->children[child_offset];
	return NULL;
}

static void
symbol_db_model_node_set_child (SymbolDBModelNode *node, gint child_offset,
                                SymbolDBModelNode *val)
{
	g_return_if_fail (node != NULL);
	g_return_if_fail (node->children_ensured == TRUE);
	g_return_if_fail (child_offset >= 0 && child_offset < node->n_children);

	/* If children nodes array hasn't been allocated, now is the time */
	if (!node->children)
	{
		node->children = g_new0 (SymbolDBModelNode*,
			                                 node->n_children);
	}
	node->children[child_offset] = val;
}

static gboolean
symbol_db_model_node_cleanse (SymbolDBModelNode *node, gboolean force)
{
	SymbolDBModelPage *page, *next;
	gint i;
	
	g_return_val_if_fail (node != NULL, FALSE);

	/* Can not cleanse a node if there are refed children */
	if (!force)
	{
		g_return_val_if_fail (node->children_ref_count == 0, FALSE);
	}
	
	if (node->children)
	{
		/* There should be no children with any ref. Children with ref count 0
		 * are floating children and can be destroyed.
		 */
		for (i = 0; i < node->n_children; i++)
		{
			SymbolDBModelNode *child = symbol_db_model_node_get_child (node, i);
			if (child)
			{
				if (!force)
				{
					/* Assert on nodes with ref count > 0 */
					g_warn_if_fail (child->children_ref_count == 0);
				}
				symbol_db_model_node_free (child, force);
				symbol_db_model_node_set_child (node, i, NULL);
			}
		}
	}
	
	/* Reset cached pages */
	page = node->pages;
	while (page)
	{
		next = page->next;
		g_free (page);
		page = next;
	}
	node->pages = NULL;
	node->children_ensured = FALSE;
	node->n_children = 0;

	/* Destroy arrays */
	g_free (node->children);
	node->children = NULL;

	return TRUE;
}

static void
symbol_db_model_node_free (SymbolDBModelNode *node, gboolean force)
{
	if (!symbol_db_model_node_cleanse (node, force))
	    return;
	
	g_free (node->values);
	g_free (node);
}

static void
symbol_db_model_node_remove_page (SymbolDBModelNode *node,
                                  SymbolDBModelPage *page)
{
	if (page->prev)
		page->prev->next = page->next;
	else
		node->pages = page->next;
		
	if (page->next)
		page->next->prev = page->prev;
}

static void
symbol_db_model_node_insert_page (SymbolDBModelNode *node,
                                  SymbolDBModelPage *page,
                                  SymbolDBModelPage *after)
{
	
	/* Insert the new page after "after" page */
	if (after)
	{
		page->next = after->next;
		after->next = page;
	}
	else /* Insert at head */
	{
		page->next = node->pages;
		node->pages = page;
	}
}

static SymbolDBModelPage*
symbol_db_model_node_find_child_page (SymbolDBModelNode *node,
                                      gint child_offset,
                                      SymbolDBModelPage **prev_page)
{
	SymbolDBModelPage *page;
	
	page = node->pages;
	
	*prev_page = NULL;
	
	/* Find the page which holds result for given child_offset */
	while (page)
	{
		if (child_offset >= page->begin_offset &&
		    child_offset < page->end_offset)
		{
			/* child_offset has associated page */
			return page;
		}
		if (child_offset < page->begin_offset)
		{
			/* Insert point is here */
			break;
		}
		*prev_page = page;
		page = page->next;
	}
	return NULL;
}

static void
symbol_db_model_node_ref_child (SymbolDBModelNode *node)
{
	g_return_if_fail (node != NULL);

	node->children_ref_count++;

	if (node->parent)
	{
		/* Increate associated ref count on parent */
		symbol_db_model_node_ref_child (node->parent);
	}
}

static void
symbol_db_model_node_unref_child (SymbolDBModelNode *node, gint child_offset)
{
	g_return_if_fail (node != NULL);
	g_return_if_fail (node->children_ref_count > 0);
	
	node->children_ref_count--;

	/* If ref count reaches 0, cleanse this node */
	if (node->children_ref_count <= 0)
	{
		symbol_db_model_node_cleanse (node, FALSE);
	}

	if (node->parent)
	{
		/* Reduce ref count on parent as well */
		symbol_db_model_node_unref_child (node->parent, node->offset);
	}
}

static SymbolDBModelNode *
symbol_db_model_node_new (SymbolDBModel *model, SymbolDBModelNode *parent,
                          gint child_offset, GdaDataModel *data_model,
                          GdaDataModelIter *data_iter)
{
	gint i;
	SymbolDBModelPriv *priv = GET_PRIV (model);
	SymbolDBModelNode* node = g_new0 (SymbolDBModelNode, 1);
	node->values = g_new0 (GValue, priv->n_columns);
	for (i = 0; i < priv->n_columns; i++)
	{
		g_value_init (&(node->values[i]), priv->column_types[i]);
		symbol_db_model_get_query_value (model,
		                                 data_model,
		                                 data_iter,
		                                 i, &(node->values[i]));
	}
	node->offset = child_offset;
	node->parent = parent;
	node->level = parent->level + 1;
	return node;
}

/* SymbolDBModel implementation */

static gboolean
symbol_db_model_iter_is_valid (GtkTreeModel *model, GtkTreeIter *iter)
{
	SymbolDBModelNode *parent_node;
	gint offset;

	g_return_val_if_fail (SYMBOL_DB_IS_MODEL (model), FALSE);
	g_return_val_if_fail (iter->stamp == SYMBOL_DB_MODEL_STAMP, FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);

	parent_node = (SymbolDBModelNode*) iter->user_data;
	offset = GPOINTER_TO_INT (iter->user_data2);
	
	g_return_val_if_fail (parent_node != NULL, FALSE);
	g_return_val_if_fail (offset >= 0 && offset < parent_node->n_children,
	                      FALSE);
	return TRUE;
}

static SymbolDBModelPage*
symbol_db_model_page_fault (SymbolDBModel *model,
                            SymbolDBModelNode *parent_node,
                            gint child_offset)
{
	SymbolDBModelPriv *priv;
	SymbolDBModelPage *page, *prev_page, *page_found;
	gint i;
	GdaDataModelIter *data_iter;
	GdaDataModel *data_model = NULL;

	/* Insert after prev_page */
	page_found = symbol_db_model_node_find_child_page (parent_node,
	                                                   child_offset,
	                                                   &prev_page);

	g_return_val_if_fail (page_found == NULL, page_found);

	/* If model is frozen, can't fetch data from backend */
	priv = GET_PRIV (model);
	if (priv->freeze_count > 0)
		return NULL;
	
	/* New page to cover current child_offset */
	page = g_new0 (SymbolDBModelPage, 1);

	/* Define page range */
	page->begin_offset = child_offset - SYMBOL_DB_MODEL_PAGE_SIZE;
	page->end_offset = child_offset + SYMBOL_DB_MODEL_PAGE_SIZE;

	symbol_db_model_node_insert_page (parent_node, page, prev_page);
	
	/* Adjust boundries not to overlap with preceeding or following page */
	if (prev_page && prev_page->end_offset > page->begin_offset)
		page->begin_offset = prev_page->end_offset;

	if (page->next && page->end_offset >= page->next->begin_offset)
		page->end_offset = page->next->begin_offset;

	/* Adjust boundries not to preceed 0 index */
	if (page->begin_offset < 0)
		page->begin_offset = 0;
	
	/* Load a page from database */
	data_model = symbol_db_model_get_children (model, parent_node->level,
	                                           parent_node->values,
	                                           page->begin_offset,
	                                           page->end_offset
	                                       			- page->begin_offset);

	/* Fill up the page */
	data_iter = gda_data_model_create_iter (data_model);
	if (gda_data_model_iter_move_to_row (data_iter, 0))
	{
		for (i = page->begin_offset; i < page->end_offset; i++)
		{
			if (i >= parent_node->n_children)
			{
				/* FIXME: There are more rows in DB. Extend node */
				break;
			}
			SymbolDBModelNode *node =
				symbol_db_model_node_new (model, parent_node, i,
				                          data_model, data_iter);
			g_assert (symbol_db_model_node_get_child (parent_node, i) == NULL);
			symbol_db_model_node_set_child (parent_node, i, node);
			if (!gda_data_model_iter_move_next (data_iter))
			{
				if (i < (page->end_offset - 1))
				{
					/* FIXME: There are fewer rows in DB. Shrink node */
				}
				break;
			}
		}
	}
	
	g_object_unref (data_iter);
	g_object_unref (data_model);
	return page;
}

/* GtkTreeModel implementation */

static GtkTreeModelFlags
symbol_db_model_get_flags (GtkTreeModel *tree_model)
{
	return 0;
}

static GType
symbol_db_model_get_column_type (GtkTreeModel *tree_model,
                                 gint index)
{
	SymbolDBModelPriv *priv = GET_PRIV (tree_model);
	g_return_val_if_fail (index < priv->n_columns, G_TYPE_INVALID);
	return priv->column_types [index];
}

static gint
symbol_db_model_get_n_columns (GtkTreeModel *tree_model)
{
	SymbolDBModelPriv *priv;
	priv = GET_PRIV (tree_model);
	return priv->n_columns;
}

static gboolean
symbol_db_model_get_iter (GtkTreeModel *tree_model,
                          GtkTreeIter *iter,
                          GtkTreePath *path)
{
	gint i;
	SymbolDBModelNode *node, *parent_node;
	gint depth, *indx;
	SymbolDBModelPriv *priv;
	
	g_return_val_if_fail (SYMBOL_DB_IS_MODEL(tree_model), FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);
	g_return_val_if_fail (path != NULL, FALSE);

	gchar *path_str = gtk_tree_path_to_string (path);
	g_free (path_str);
	
	depth = gtk_tree_path_get_depth (path);
	g_return_val_if_fail (depth > 0, FALSE);

	priv = GET_PRIV (tree_model);
	indx = gtk_tree_path_get_indices (path);

	parent_node = NULL;
	node = priv->root;
	for (i = 0; i < depth; i++)
	{
		parent_node = node;
		if (!node->children_ensured)
			symbol_db_model_ensure_node_children (SYMBOL_DB_MODEL (tree_model),
				                                  node, FALSE);
		if (node->n_children <= 0)
		{
			/* No child available. View thinks there is child.
			 * It's an inconsistent state. Do something fancy to fix it.
			 */
			symbol_db_model_update (SYMBOL_DB_MODEL (tree_model)); /* Untested path */
			break;
		}
		if (indx[i] >= node->n_children)
		{
			g_warning ("Invalid path to iter conversion; no children list found at depth %d", i);
			break;
		}
		node = symbol_db_model_node_get_child (node, indx[i]);
	}
	if (i != depth)
	{
		return FALSE;
	}
	iter->stamp = SYMBOL_DB_MODEL_STAMP;
	iter->user_data = parent_node;
	iter->user_data2 = GINT_TO_POINTER (indx[i-1]);

	g_assert (symbol_db_model_iter_is_valid (tree_model, iter));

	return TRUE;
}

static GtkTreePath*
symbol_db_model_get_path (GtkTreeModel *tree_model,
                          GtkTreeIter *iter)
{
	SymbolDBModelNode *node;
	GtkTreePath* path;
	SymbolDBModelPriv *priv;
	gint offset;
	
	g_return_val_if_fail (symbol_db_model_iter_is_valid (tree_model, iter),
	                      NULL);

	priv = GET_PRIV (tree_model);
	path = gtk_tree_path_new ();

	node = (SymbolDBModelNode*)iter->user_data;
	offset = GPOINTER_TO_INT (iter->user_data2);
	do {
		gtk_tree_path_prepend_index (path, offset);
		node = node->parent;
		if (node)
			offset = node->offset;
	} while (node);
	return path;
}

static void
symbol_db_model_get_value (GtkTreeModel *tree_model,
                           GtkTreeIter *iter, gint column,
                           GValue *value)
{
	SymbolDBModelPriv *priv;
	SymbolDBModelNode *parent_node, *node;
	SymbolDBModelPage *page;
	gint offset;
	
	g_return_if_fail (symbol_db_model_iter_is_valid (tree_model, iter));

	priv = GET_PRIV (tree_model);
	
	g_return_if_fail (column >= 0);
	g_return_if_fail (column < priv->n_columns);
	
	parent_node = (SymbolDBModelNode*) iter->user_data;
	offset = GPOINTER_TO_INT (iter->user_data2);

	if (symbol_db_model_node_get_child (parent_node, offset) == NULL)
		page = symbol_db_model_page_fault (SYMBOL_DB_MODEL (tree_model),
		                                   parent_node, offset);
	node = symbol_db_model_node_get_child (parent_node, offset);
	g_value_init (value, priv->column_types[column]);

	/* If model is frozen, we don't expect the page fault to work */
	if (priv->freeze_count > 0 && node == NULL)
		return;
	
	g_return_if_fail (node != NULL);

	/* View accessed the node, so update any pending has-child status */
	if (!node->children_ensured)
		symbol_db_model_queue_ensure_node_children (SYMBOL_DB_MODEL (tree_model),
		                                            node);
	g_value_copy (&(node->values[column]), value);
}

static gboolean
symbol_db_model_iter_next (GtkTreeModel *tree_model,
                           GtkTreeIter *iter)
{
	SymbolDBModelNode *node;
	gint offset;
	
	g_return_val_if_fail (iter != NULL, FALSE);
	g_return_val_if_fail (iter->stamp == SYMBOL_DB_MODEL_STAMP, FALSE);
	g_return_val_if_fail (iter->user_data != NULL, FALSE);
	

	node = (SymbolDBModelNode*)(iter->user_data);
	offset = GPOINTER_TO_INT (iter->user_data2);
	offset++;
	
	if (offset >= node->n_children)
		return FALSE;
	iter->user_data2 = GINT_TO_POINTER (offset);

	g_assert (symbol_db_model_iter_is_valid (tree_model, iter));

	return TRUE;
}

static gboolean
symbol_db_model_iter_children (GtkTreeModel *tree_model,
                               GtkTreeIter *iter,
                               GtkTreeIter *parent)
{
	SymbolDBModelNode *node, *parent_node;
	SymbolDBModelPriv *priv;

	if (parent)
	{
		g_return_val_if_fail (symbol_db_model_iter_is_valid (tree_model,
		                                                     parent),
		                      FALSE);
	}
	
	g_return_val_if_fail (SYMBOL_DB_IS_MODEL(tree_model), FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);

	priv = GET_PRIV (tree_model);
	if (parent == NULL)
	{
		node = priv->root;
	}
	else
	{
		gint offset;
		parent_node = (SymbolDBModelNode*) parent->user_data;
		offset = GPOINTER_TO_INT (parent->user_data2);
		node = symbol_db_model_node_get_child (parent_node, offset);
		if (!node)
		{
			symbol_db_model_page_fault (SYMBOL_DB_MODEL (tree_model),
			                            parent_node, offset);
			node = symbol_db_model_node_get_child (parent_node, offset);
			if (node)
				symbol_db_model_ensure_node_children (SYMBOL_DB_MODEL (tree_model),
						                              node, FALSE);
		}
		g_return_val_if_fail (node != NULL, FALSE);
	}

	/* View trying to access children of childless node seems typical */
	if (node->n_children <= 0)
		return FALSE;
	
	iter->user_data = node;
	iter->user_data2 = GINT_TO_POINTER (0);
	iter->stamp = SYMBOL_DB_MODEL_STAMP;

	g_assert (symbol_db_model_iter_is_valid (tree_model, iter));

	return TRUE;
}

static gboolean
symbol_db_model_iter_has_child (GtkTreeModel *tree_model,
                                GtkTreeIter *iter)
{
	gint offset;
	SymbolDBModelNode *node, *parent_node;

	g_return_val_if_fail (symbol_db_model_iter_is_valid (tree_model, iter),
	                      FALSE);
	
	parent_node = (SymbolDBModelNode*) iter->user_data;
	offset = GPOINTER_TO_INT (iter->user_data2);

	node = symbol_db_model_node_get_child (parent_node, offset);

	/* If node is not loaded, has-child is defered. See get_value() */
	if (node == NULL)
		return FALSE;
	return (node->n_children > 0);
}

static gint
symbol_db_model_iter_n_children (GtkTreeModel *tree_model,
                                 GtkTreeIter *iter)
{
	gint offset;
	SymbolDBModelNode *node, *parent_node;
	
	g_return_val_if_fail (symbol_db_model_iter_is_valid (tree_model, iter),
	                      FALSE);
	
	parent_node = (SymbolDBModelNode*) iter->user_data;
	offset = GPOINTER_TO_INT (iter->user_data2);
	
	node = symbol_db_model_node_get_child (parent_node, offset);
	if (node == NULL)
		return 0;
	return node->n_children;
}

static gboolean
symbol_db_model_iter_nth_child (GtkTreeModel *tree_model,
                                GtkTreeIter *iter,
                                GtkTreeIter *parent,
                                gint n)
{
	SymbolDBModelNode *node;
	SymbolDBModelPriv *priv;
	                           
	g_return_val_if_fail (SYMBOL_DB_IS_MODEL(tree_model), FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);
	g_return_val_if_fail (n >= 0, FALSE);

	if (!symbol_db_model_iter_children (tree_model, iter, parent))
		return FALSE;
	
	priv = GET_PRIV (tree_model);
	node = (SymbolDBModelNode*) (iter->user_data);

	g_return_val_if_fail (n < node->n_children, FALSE);
	
	iter->user_data2 = GINT_TO_POINTER (n);

	g_assert (symbol_db_model_iter_is_valid (tree_model, iter));

	return TRUE;
}

static gboolean
symbol_db_model_iter_parent (GtkTreeModel *tree_model,
                             GtkTreeIter *iter,
                             GtkTreeIter *child)
{
	SymbolDBModelNode *parent_node;
	
	g_return_val_if_fail (symbol_db_model_iter_is_valid (tree_model, child),
	                      FALSE);
	
	g_return_val_if_fail (iter != NULL, FALSE);
	
	parent_node = (SymbolDBModelNode*) child->user_data;
	g_return_val_if_fail (parent_node->parent != NULL, FALSE);
	
	iter->user_data = parent_node->parent;
	iter->user_data2 = GINT_TO_POINTER (parent_node->offset);
	iter->stamp = SYMBOL_DB_MODEL_STAMP;

	g_assert (symbol_db_model_iter_is_valid (tree_model, iter));

	return TRUE;
}

static void
symbol_db_model_iter_ref (GtkTreeModel *tree_model, GtkTreeIter  *iter)
{
	SymbolDBModelNode *parent_node;
	gint child_offset;
	
	g_return_if_fail (symbol_db_model_iter_is_valid (tree_model, iter));

	parent_node = (SymbolDBModelNode*) iter->user_data;
	child_offset = GPOINTER_TO_INT (iter->user_data2);

	symbol_db_model_node_ref_child (parent_node);
}

static void
symbol_db_model_iter_unref (GtkTreeModel *tree_model, GtkTreeIter  *iter)
{
	SymbolDBModelNode *parent_node;
	gint child_offset;
	
	g_return_if_fail (symbol_db_model_iter_is_valid (tree_model, iter));

	parent_node = (SymbolDBModelNode*) iter->user_data;
	child_offset = GPOINTER_TO_INT (iter->user_data2);

	symbol_db_model_node_unref_child (parent_node, child_offset);
}

static void
symbol_db_model_tree_model_init (GtkTreeModelIface *iface)
{
	iface->get_flags = symbol_db_model_get_flags;
	iface->get_n_columns = symbol_db_model_get_n_columns;
	iface->get_column_type = symbol_db_model_get_column_type;
	iface->get_iter = symbol_db_model_get_iter;
	iface->get_path = symbol_db_model_get_path;
	iface->get_value = symbol_db_model_get_value;
	iface->iter_next = symbol_db_model_iter_next;
	iface->iter_children = symbol_db_model_iter_children;
	iface->iter_has_child = symbol_db_model_iter_has_child;
	iface->iter_n_children = symbol_db_model_iter_n_children;
	iface->iter_nth_child = symbol_db_model_iter_nth_child;
	iface->iter_parent = symbol_db_model_iter_parent;
	iface->ref_node = symbol_db_model_iter_ref;
	iface->unref_node = symbol_db_model_iter_unref;
}

/* SymbolDBModel implementation */

static void
symbol_db_model_ensure_node_children (SymbolDBModel *model,
                                      SymbolDBModelNode *node,
                                      gboolean emit_has_child)
{
	SymbolDBModelPriv *priv;
	
	g_return_if_fail (node->n_children == 0);
	g_return_if_fail (node->children == NULL);
	g_return_if_fail (node->children_ensured == FALSE);

	priv = GET_PRIV (model);

	/* Can not ensure if model is frozen */
	if (priv->freeze_count > 0)
		return;
	
	/* Initialize children array and count */
	node->n_children = 
		symbol_db_model_get_n_children (model, node->level,
		                                node->values);

	node->children_ensured = TRUE;

	if (emit_has_child && node->n_children > 0)
	{
		GtkTreePath *path;
		GtkTreeIter iter = {0};

		iter.stamp = SYMBOL_DB_MODEL_STAMP;
		iter.user_data = node->parent;
		iter.user_data2 = GINT_TO_POINTER (node->offset);
		
		path = symbol_db_model_get_path (GTK_TREE_MODEL (model), &iter);
		gtk_tree_model_row_has_child_toggled (GTK_TREE_MODEL (model),
		                                      path, &iter);
		gtk_tree_path_free (path);
	}
}

static gboolean
on_symbol_db_ensure_node_children_idle (SymbolDBModel *model)
{
	gint count;
	SymbolDBModelNode *node;
	SymbolDBModelPriv *priv;
	
	priv = GET_PRIV (model);

	for (count = 0; count < SYMBOL_DB_MODEL_ENSURE_CHILDREN_BATCH_SIZE; count++)
	{
		node = g_queue_pop_head (priv->ensure_children_queue);
		symbol_db_model_ensure_node_children (model, node, TRUE);
		if (g_queue_get_length (priv->ensure_children_queue) <= 0)
		{
			priv->ensure_children_idle_id = 0;
			return FALSE;
		}
	}
	return TRUE;
}

static void
symbol_db_model_queue_ensure_node_children (SymbolDBModel *model,
                                            SymbolDBModelNode *node)
{
	SymbolDBModelPriv *priv;
	
	g_return_if_fail (node->children == NULL);
	g_return_if_fail (node->children_ensured == FALSE);

	priv = GET_PRIV (model);
	if (!g_queue_find (priv->ensure_children_queue, node))
	{
		g_queue_push_tail (priv->ensure_children_queue, node);
		if (!priv->ensure_children_idle_id)
			priv->ensure_children_idle_id =
				g_idle_add ((GSourceFunc)on_symbol_db_ensure_node_children_idle,
				            model);
	}
}

static void
symbol_db_model_update_node_children (SymbolDBModel *model,
                                      SymbolDBModelNode *node,
                                      gboolean emit_has_child)
{
	SymbolDBModelPriv *priv;

	priv = GET_PRIV (model);
	
	/* Delete all nodes */
	if (node->n_children > 0)
	{
		GtkTreePath *path;
		GtkTreeIter iter = {0};
		gint i;

		/* Set the iter to first child */
		iter.stamp = SYMBOL_DB_MODEL_STAMP;
		iter.user_data = node;
		iter.user_data2 = GINT_TO_POINTER (0);

		/* Get path to it */
		path = symbol_db_model_get_path (GTK_TREE_MODEL (model), &iter);

		/* Delete all children */
		for (i = 0; i < node->n_children; i++)
			gtk_tree_model_row_deleted (GTK_TREE_MODEL (model), path);
		gtk_tree_path_free (path);
	}

	symbol_db_model_node_cleanse (node, TRUE);
	symbol_db_model_ensure_node_children (model, node, emit_has_child);
	
	/* Add all new nodes */
	if (node->n_children > 0)
	{
		GtkTreePath *path;
		GtkTreeIter iter = {0};
		gint i;
		
		iter.stamp = SYMBOL_DB_MODEL_STAMP;
		iter.user_data = node;
		iter.user_data2 = 0;
		path = symbol_db_model_get_path (GTK_TREE_MODEL (model), &iter);
		if (path == NULL)
			path = gtk_tree_path_new_first ();
		for (i = 0; i < node->n_children; i++)
		{
			iter.user_data2 = GINT_TO_POINTER (i);
			gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), path, &iter);
			gtk_tree_path_next (path);
		}
		gtk_tree_path_free (path);
	}
}

static gboolean
symbol_db_model_get_query_value_real (SymbolDBModel *model,
                                      GdaDataModel *data_model,
                                      GdaDataModelIter *iter, gint column,
                                      GValue *value)
{
	const GValue *ret;
	SymbolDBModelPriv *priv = GET_PRIV (model);
	gint query_column = priv->query_columns[column];

	if (query_column < 0)
		return FALSE;

	ret = gda_data_model_iter_get_value_at (iter, query_column);
	if (!ret || !G_IS_VALUE (ret))
		return FALSE;

	g_value_copy (ret, value);
	return TRUE;
}

static gboolean
symbol_db_model_get_query_value (SymbolDBModel *model,
                                 GdaDataModel *data_model,
                                 GdaDataModelIter *iter, gint column,
                                 GValue *value)
{
	return SYMBOL_DB_MODEL_GET_CLASS(model)->get_query_value(model, data_model,
	                                                         iter, column,
	                                                         value);
}

static gboolean
symbol_db_model_get_query_value_at_real (SymbolDBModel *model,
                                         GdaDataModel *data_model,
                                         gint position, gint column,
                                         GValue *value)
{
	const GValue *ret;
	SymbolDBModelPriv *priv = GET_PRIV (model);
	gint query_column = priv->query_columns[column];
	g_value_init (value, priv->column_types[column]);

	if (query_column < 0)
		return FALSE;

	ret = gda_data_model_get_value_at (data_model, query_column, position,
	                                   NULL);
	if (!ret || !G_IS_VALUE (ret))
		return FALSE;

	g_value_copy (ret, value);
	return TRUE;
}

static gboolean
symbol_db_model_get_query_value_at (SymbolDBModel *model,
                                    GdaDataModel *data_model,
                                    gint position, gint column, GValue *value)
{
	return SYMBOL_DB_MODEL_GET_CLASS(model)->get_query_value_at (model,
	                                                             data_model,
	                                                             position,
	                                                             column,
	                                                             value);
}

static gint
symbol_db_model_get_n_children_real (SymbolDBModel *model, gint tree_level,
                                     GValue column_values[])
{
	gint n_children;
	g_signal_emit_by_name (model, "get-n-children", tree_level, column_values,
	                       &n_children);
	return n_children;
}

static gint
symbol_db_model_get_n_children (SymbolDBModel *model, gint tree_level,
                                GValue column_values[])
{
	return SYMBOL_DB_MODEL_GET_CLASS(model)->get_n_children (model, tree_level,
	                                                         column_values);
}

static GdaDataModel*
symbol_db_model_get_children_real (SymbolDBModel *model, gint tree_level,
                                   GValue column_values[], gint offset,
                                   gint limit)
{
	GdaDataModel *data_model;
	g_signal_emit_by_name (model, "get-children", tree_level,
	                       column_values, offset, limit, &data_model);
	return data_model;
}

static GdaDataModel*
symbol_db_model_get_children (SymbolDBModel *model, gint tree_level,
                              GValue column_values[], gint offset,
                              gint limit)
{
	return SYMBOL_DB_MODEL_GET_CLASS(model)->
		get_children (model, tree_level, column_values, offset, limit);
}

/* Object implementation */

static void
symbol_db_model_finalize (GObject *object)
{
	SymbolDBModelPriv *priv;
	/* FIXME */

	priv = GET_PRIV (object);
	if (priv->ensure_children_idle_id)
		g_source_remove (priv->ensure_children_idle_id);
	g_queue_free (priv->ensure_children_queue);
	
	G_OBJECT_CLASS (symbol_db_model_parent_class)->finalize (object);
}

static void
symbol_db_model_set_property (GObject *object, guint prop_id,
                              const GValue *value, GParamSpec *pspec)
{
	g_return_if_fail (SYMBOL_DB_IS_MODEL (object));
	/* SymbolDBModel* model = SYMBOL_DB_MODEL(object);
	SymbolDBModelPriv* priv = GET_PRIV (model); */
	
	switch (prop_id)
	{
	}
}

static void
symbol_db_model_get_property (GObject *object, guint prop_id, GValue *value,
                              GParamSpec *pspec)
{
	g_return_if_fail (SYMBOL_DB_IS_MODEL (object));
	/* SymbolDBModel* model = SYMBOL_DB_MODEL(object);
	SymbolDBModelPriv* priv = GET_PRIV (model); */
	
	switch (prop_id)
	{
	}
}

static void
symbol_db_model_init (SymbolDBModel *object)
{
	SymbolDBModelPriv *priv = GET_PRIV (object);
	priv->root = g_new0 (SymbolDBModelNode, 1);
	priv->freeze_count = 0;
	priv->n_columns = 0;
	priv->column_types = NULL;
	priv->query_columns = NULL;
	priv->ensure_children_queue = g_queue_new ();
}

static void
symbol_db_model_class_init (SymbolDBModelClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	klass->get_query_value = symbol_db_model_get_query_value_real;
	klass->get_query_value_at = symbol_db_model_get_query_value_at_real;
	klass->get_n_children = symbol_db_model_get_n_children_real;
	klass->get_children = symbol_db_model_get_children_real;
	
	object_class->finalize = symbol_db_model_finalize;
	object_class->set_property = symbol_db_model_set_property;
	object_class->get_property = symbol_db_model_get_property;
	
	g_type_class_add_private (object_class, sizeof(SymbolDBModelPriv));

	/* Signals */
	symbol_db_model_signals[SIGNAL_GET_HAS_CHILD] =
		g_signal_new ("get-has-child",
		              G_TYPE_FROM_CLASS (klass),
		              G_SIGNAL_RUN_LAST,
		              0, NULL, NULL,
					  symbol_db_cclosure_marshal_BOOLEAN__INT_POINTER,
		              G_TYPE_BOOLEAN,
		              2,
		              G_TYPE_INT,
		              G_TYPE_POINTER);
	symbol_db_model_signals[SIGNAL_GET_N_CHILDREN] =
		g_signal_new ("get-n-children",
		              G_TYPE_FROM_CLASS (klass),
		              G_SIGNAL_RUN_LAST,
		              0, NULL, NULL,
					  symbol_db_cclosure_marshal_INT__INT_POINTER,
		              G_TYPE_INT,
		              2,
		              G_TYPE_INT,
		              G_TYPE_POINTER);
	symbol_db_model_signals[SIGNAL_GET_CHILDREN] =
		g_signal_new ("get-children",
		              G_TYPE_FROM_CLASS (klass),
		              G_SIGNAL_RUN_LAST,
		              0, NULL, NULL,
					  symbol_db_cclosure_marshal_OBJECT__INT_POINTER_INT_INT,
		              GDA_TYPE_DATA_MODEL,
		              4,
		              G_TYPE_INT,
		              G_TYPE_POINTER,
		              G_TYPE_INT,
		              G_TYPE_INT);
}

void
symbol_db_model_set_columns (SymbolDBModel *model, gint n_columns,
                             GType *types, gint *query_columns)
{
	SymbolDBModelPriv *priv;

	g_return_if_fail (n_columns > 0);
	g_return_if_fail (SYMBOL_DB_IS_MODEL (model));

	priv = GET_PRIV (model);
	
	g_return_if_fail (priv->n_columns <= 0);
	g_return_if_fail (priv->column_types == NULL);
	g_return_if_fail (priv->query_columns == NULL);
	
	priv->n_columns = n_columns;
	priv->column_types = g_new0(GType, n_columns);
	priv->query_columns = g_new0(gint, n_columns);
	memcpy (priv->column_types, types, n_columns * sizeof (GType));
	memcpy (priv->query_columns, query_columns, n_columns * sizeof (gint));
}

GtkTreeModel *
symbol_db_model_new (gint n_columns, ...)
{
	GtkTreeModel *model;
	SymbolDBModelPriv *priv;
	va_list args;
	gint i;

	g_return_val_if_fail (n_columns > 0, NULL);

	model = g_object_new (SYMBOL_DB_TYPE_MODEL, NULL);
	priv = GET_PRIV (model);
	
	priv->n_columns = n_columns;
	priv->column_types = g_new0(GType, n_columns);
	priv->query_columns = g_new0(gint, n_columns);
	
	va_start (args, n_columns);

	for (i = 0; i < n_columns; i++)
	{
		priv->column_types[i] = va_arg (args, GType);
		priv->query_columns[i] = va_arg (args, gint);
	}
	va_end (args);

	return model;
}

GtkTreeModel*
symbol_db_model_newv (gint n_columns, GType *types, gint *query_columns)
{
	GtkTreeModel *model;
	g_return_val_if_fail (n_columns > 0, NULL);
	model = g_object_new (SYMBOL_DB_TYPE_MODEL, NULL);
	symbol_db_model_set_columns (SYMBOL_DB_MODEL (model), n_columns,
	                             types, query_columns);
	return model;
}

void
symbol_db_model_update (SymbolDBModel *model)
{
	SymbolDBModelPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_MODEL (model));

	priv = GET_PRIV (model);

	symbol_db_model_update_node_children (model, priv->root, FALSE);
}

void
symbol_db_model_freeze (SymbolDBModel *model)
{
	SymbolDBModelPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_MODEL (model));
	
	priv = GET_PRIV (model);
	priv->freeze_count++;
}

void
symbol_db_model_thaw (SymbolDBModel *model)
{
	SymbolDBModelPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_MODEL (model));

	priv = GET_PRIV (model);

	if (priv->freeze_count > 0)
		priv->freeze_count--;
	
	if (priv->freeze_count <= 0)
		symbol_db_model_update (model);
}
