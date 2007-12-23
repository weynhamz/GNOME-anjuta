/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Massimo Cora' 2007 <maxcvs@email.it>
 * 
 * anjuta is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include <glib.h>
#include <gdl/gdl-icons.h>
#include <libanjuta/resources.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>
#include "symbol-db-view.h"
#include "symbol-db-engine.h"
#include "symbol-db-engine-iterator.h"
#include "symbol-db-engine-iterator-node.h"


enum {
	COLUMN_PIXBUF,
	COLUMN_NAME,
	COLUMN_SYMBOL_ID,
	COLUMN_MAX
};

/* positive ids are used in real database */
enum {
	ROOT_NAMESPACE = -2,
	ROOT_CLASS = -3,
	ROOT_STRUCT = -4,
	ROOT_UNION = -5,
	ROOT_FUNCTION = -6,
	ROOT_VARIABLE = -7,
	ROOT_MACRO = -8,
	ROOT_TYPEDEF = -9,
	ROOT_ENUMERATOR = -10
};

#define DUMMY_SYMBOL_ID		-333

struct _SymbolDBViewPriv
{
	gint insert_handler;
	gint remove_handler;	
	gint scan_end_handler;	
	
	GtkTreeRowReference *row_ref_namespace;
	GtkTreeRowReference *row_ref_class;
	GtkTreeRowReference *row_ref_struct;
	GtkTreeRowReference *row_ref_union;
	GtkTreeRowReference *row_ref_function;
	GtkTreeRowReference *row_ref_variable;
	GtkTreeRowReference *row_ref_macro;
	GtkTreeRowReference *row_ref_typedef;
	GtkTreeRowReference *row_ref_enumerator;

	GTree *nodes_displayed;
	GTree *waiting_for;
};

typedef struct _WaitingForSymbol {
	gint child_symbol_id;
	gchar *child_symbol_name;
	const GdkPixbuf *pixbuf;
	
} WaitingForSymbol;

static GtkTreeViewClass *parent_class = NULL;
static GHashTable *pixbufs_hash = NULL;
static void 
trigger_on_symbol_inserted (SymbolDBView *dbv, gint symbol_id);


static gint
gtree_compare_func (gconstpointer a, gconstpointer b, gpointer user_data)
{
	return (gint)a - (gint)b;
}

static void
waiting_for_symbol_destroy (WaitingForSymbol *wfs)
{
	g_return_if_fail (wfs != NULL);
	g_free (wfs->child_symbol_name);
	g_free (wfs);
}

static gboolean
sdb_view_get_iter_from_row_ref (SymbolDBView *dbv, GtkTreeRowReference *row_ref, 
								GtkTreeIter *OUT_iter)
{
	GtkTreePath *path;
	if (row_ref == NULL) 
	{
		/* no node displayed found */
		DEBUG_PRINT ("sdb_view_get_iter_from_row_ref (): row_ref == NULL");
		return FALSE;
	}
			
	path = gtk_tree_row_reference_get_path (row_ref);
	if (path == NULL) 
	{
		DEBUG_PRINT ("sdb_view_get_iter_from_row_ref (): path is null, something "
					 "went wrong ?!");
		return FALSE;
	}
		
	if (gtk_tree_model_get_iter (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)),
                             OUT_iter, path) == FALSE) 
	{
		gtk_tree_path_free (path);
		return FALSE;
	}
	gtk_tree_path_free (path);	
	
	return TRUE;
}


static gboolean
traverse_free_waiting_for (gpointer key, gpointer value, gpointer data)
{
	if (value == NULL)
		return FALSE;

	g_slist_foreach ((GSList*)value, (GFunc)waiting_for_symbol_destroy, NULL);
	g_slist_free ((GSList*)value);	
	return FALSE;
}

void
symbol_db_view_clear_cache (SymbolDBView *dbv)
{
	SymbolDBViewPriv *priv;
	GtkTreeStore *store;
	
	g_return_if_fail (dbv != NULL);
	
	priv = dbv->priv;
	
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)));
	if (store != NULL)
		g_object_unref (store);	
	
	/* this will free alto the priv->row_ref* instances */	
	if (priv->nodes_displayed)
	{
		g_tree_destroy (priv->nodes_displayed);
		priv->nodes_displayed = NULL;
	}
	
	/* free the waiting_for structs before destroying the tree itself */
	if (priv->waiting_for)
	{
		g_tree_foreach (priv->waiting_for, traverse_free_waiting_for, NULL);
		g_tree_destroy (priv->waiting_for);
		priv->waiting_for = NULL;		
	}
	
	gtk_tree_view_set_model (GTK_TREE_VIEW (dbv), NULL);
}

static void
on_scan_end (SymbolDBEngine *dbe, gpointer data)
{
	SymbolDBView *dbv;
	SymbolDBViewPriv *priv;

	dbv = SYMBOL_DB_VIEW (data);
	g_return_if_fail (dbv != NULL);	
	priv = dbv->priv;

	/* void the waiting_for symbols */
	/* free the waiting_for structs before destroying the tree itself */
	if (priv->waiting_for)
	{
		g_tree_foreach (priv->waiting_for, traverse_free_waiting_for, data);
		g_tree_destroy (priv->waiting_for);
		
		/* recreate it because there's a free_all_items function. And the
		 * one proposed by the doc is too complex.. create a list of the items
		 * and reparse them with g_tree_remove...
		 */
		priv->waiting_for = g_tree_new_full ((GCompareDataFunc)&gtree_compare_func, 
									 NULL,
									 NULL,
									 NULL);

	}
}

static GtkTreeRowReference *
do_add_root_symbol_to_view (SymbolDBView *dbv, const GdkPixbuf *pixbuf, 
							const gchar* symbol_name, gint symbol_id)
{
	SymbolDBViewPriv *priv;
	GtkTreeStore *store;
	GtkTreeIter child_iter;
	GtkTreePath *path;
	GtkTreeRowReference *row_ref;
	
	g_return_val_if_fail (dbv != NULL, NULL);
	
	priv = dbv->priv;	
	
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)));
 	
	gtk_tree_store_append (store, &child_iter, NULL);
			
	gtk_tree_store_set (store, &child_iter,
		COLUMN_PIXBUF, pixbuf,
		COLUMN_NAME, symbol_name,
		COLUMN_SYMBOL_ID, symbol_id,
		-1);	

	path = gtk_tree_model_get_path (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)),
                                          &child_iter);	
	row_ref = gtk_tree_row_reference_new (
						gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)), path);
	gtk_tree_path_free (path);
	
	return row_ref;
}

static GtkTreeRowReference *
do_add_child_symbol_to_view (SymbolDBView *dbv, gint parent_symbol_id,
					   const GdkPixbuf *pixbuf, const gchar* symbol_name,
					   gint symbol_id)
{
	SymbolDBViewPriv *priv;
	GtkTreePath *path;
	GtkTreeStore *store;
	GtkTreeIter iter, child_iter;
	GtkTreeRowReference *row_ref;
	
	g_return_val_if_fail (dbv != NULL, NULL);
	
	priv = dbv->priv;	
	
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)));
	
	/* look into nodes_displayed g_tree for the gtktreepath of the parent iter,
	 * get the gtktreeiter, and add a child 
	 */
	row_ref = g_tree_lookup (priv->nodes_displayed, (gpointer)parent_symbol_id);
	path = gtk_tree_row_reference_get_path (row_ref);
	
	if (path == NULL) {
		DEBUG_PRINT ("do_add_symbol_to_view (): something went wrong.");
		return NULL;		
	}
	
	if (gtk_tree_model_get_iter (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)),
                                 &iter, path) == FALSE) {									 
		DEBUG_PRINT ("do_add_symbol_to_view (): iter was not set ?![%s %d] parent %d",
					 symbol_name, symbol_id, parent_symbol_id);
		return NULL;
	}

	gtk_tree_path_free (path);
	
	gtk_tree_store_append (store, &child_iter, &iter);
			
	gtk_tree_store_set (store, &child_iter,
		COLUMN_PIXBUF, pixbuf,
		COLUMN_NAME, symbol_name,
		COLUMN_SYMBOL_ID, symbol_id,
		-1);	
	
	gchar *tmp_str = gtk_tree_path_to_string (
					gtk_tree_model_get_path (
						gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)),
                                          &child_iter));

	g_free (tmp_str);
	
	path = gtk_tree_model_get_path (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)),
                                          &child_iter);
	row_ref = gtk_tree_row_reference_new (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)), 
										  path);
	gtk_tree_path_free (path);
	
	return row_ref;
}


static void
add_waiting_for_symbol_to_view (SymbolDBView *dbv, WaitingForSymbol *wfs,
								gint parent_symbol_id)
{
	SymbolDBViewPriv *priv;
	gint symbol_id_added;
	GtkTreeRowReference *child_tree_row_ref;
	
	g_return_if_fail (dbv != NULL);
	g_return_if_fail (wfs != NULL);
	
	priv = dbv->priv;	

	child_tree_row_ref = do_add_child_symbol_to_view (dbv, parent_symbol_id,
					   wfs->pixbuf, wfs->child_symbol_name, wfs->child_symbol_id);
			
	symbol_id_added = wfs->child_symbol_id;
	
	/* add a new entry on gtree 'nodes_displayed' */
	g_tree_insert (priv->nodes_displayed, (gpointer)wfs->child_symbol_id, 
				   child_tree_row_ref);	
	
	/* and now trigger the inserted symbol... (recursive function). */
	if (wfs->child_symbol_id != parent_symbol_id)
		trigger_on_symbol_inserted  (dbv, wfs->child_symbol_id);
}


static void
trigger_on_symbol_inserted (SymbolDBView *dbv, gint symbol_id)
{
	SymbolDBViewPriv *priv;
	GSList *slist;
	WaitingForSymbol *wfs;
	
	g_return_if_fail (dbv != NULL);
	
	priv = dbv->priv;

/*	DEBUG_PRINT ("trigger_on_symbol_inserted (): triggering %d", symbol_id);*/
	
	/* try to find a waiting for symbol */
	slist = g_tree_lookup (priv->waiting_for, (gpointer)symbol_id);
	
	if (slist == NULL) 
	{
		/* nothing waiting for us */
		/*DEBUG_PRINT ("trigger_on_symbol_inserted (): no children waiting for us...");*/
		return;
	}
	else {
		gint i;
		gint length = g_slist_length (slist);

/*		DEBUG_PRINT ("trigger_on_symbol_inserted (): consuming slist for parent %d",
					 symbol_id);*/

		for (i=0; i < length-1; i++)
		{
			wfs = g_slist_nth_data (slist, 0);
				
			slist = g_slist_remove (slist, wfs);

			add_waiting_for_symbol_to_view (dbv, wfs, symbol_id);

			/* destroy the data structure */
			waiting_for_symbol_destroy (wfs);
		}
		
		/* remove the waiting for key/value */
		g_tree_remove (priv->waiting_for, (gpointer)symbol_id);		
		g_slist_free (slist);
	}
}


static void
add_new_waiting_for (SymbolDBView *dbv, gint parent_symbol_id, 
					 const gchar* symbol_name, 
					 gint symbol_id, const GdkPixbuf *pixbuf)
{
	SymbolDBViewPriv *priv;
	gpointer node;
	
	g_return_if_fail (dbv != NULL);	
	priv = dbv->priv;

	/* check if we already have some children waiting for a 
	 * specific father to be inserted, then add this symbol_id to the list 
	 * (or create a new one)
	 */
	WaitingForSymbol *wfs;
			
	wfs = g_new0 (WaitingForSymbol, 1);
	wfs->child_symbol_id = symbol_id;
	wfs->child_symbol_name = g_strdup (symbol_name);
	wfs->pixbuf = pixbuf;
				
/*	DEBUG_PRINT ("add_new_waiting_for (): looking up waiting_for %d", 
				 parent_symbol_id);*/
	node = g_tree_lookup (priv->waiting_for, (gpointer)parent_symbol_id);
	if (node == NULL) 
	{
		/* no lists already set. Create one. */
		GSList *slist;					
		slist = g_slist_alloc ();			
				
		slist = g_slist_prepend (slist, wfs);
					
/*		DEBUG_PRINT ("add_new_waiting_for (): NEW adding to "
					 "waiting_for [%d]", parent_symbol_id);*/
				
		/* add it to the binary tree. */
		g_tree_insert (priv->waiting_for, (gpointer)parent_symbol_id, 
							   slist);
	}
	else 
	{
		/* found a list */
		GSList *slist;
		slist = (GSList*)node;
		
		/*DEBUG_PRINT ("prepare_for_adding (): NEW adding to "
					 "parent_waiting_for_list [%d] %s",
				 	parent_symbol_id, symbol_name);*/
		slist = g_slist_prepend (slist, wfs);
				
		g_tree_replace (priv->waiting_for, (gpointer)parent_symbol_id, 
						slist);
	}	
}


/* Put every GtkTreeView node of the subtree headed by 'parent_subtree_iter'
 * into a waiting_for GTree.
 * It's a recursive function.
 */
static void
do_recurse_subtree_and_invalidate (SymbolDBView *dbv, 
								   GtkTreeIter *parent_subtree_iter,
								   gint parent_id_to_wait_for)
{
	gint curr_symbol_id;
	const GdkPixbuf *curr_pixbuf;
	GtkTreeStore *store;
	gchar *curr_symbol_name;

	SymbolDBViewPriv *priv;
	
	g_return_if_fail (dbv != NULL);
	
	priv = dbv->priv;
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)));	
	
	gtk_tree_model_get (GTK_TREE_MODEL (store), parent_subtree_iter,
				COLUMN_SYMBOL_ID, &curr_symbol_id,
			    COLUMN_PIXBUF, &curr_pixbuf, 
				COLUMN_NAME, &curr_symbol_name,	/* no strdup required */
				-1);
	
	 /*DEBUG_PRINT ("do_recurse_subtree_and_invalidate (): curr_symbol_id %d,"
				"parent_id_to_wait_for %d", curr_symbol_id, parent_id_to_wait_for);*/
				 
	while (gtk_tree_model_iter_has_child  (GTK_TREE_MODEL (store), 
										   parent_subtree_iter)) 
	{
		GtkTreeIter child;
		gtk_tree_model_iter_children (GTK_TREE_MODEL (store), &child, 
									  parent_subtree_iter);
		
		/* recurse */
		do_recurse_subtree_and_invalidate (dbv, &child, curr_symbol_id);
	}

	/* add to waiting for */
	add_new_waiting_for (dbv, parent_id_to_wait_for, curr_symbol_name,
						 curr_symbol_id, curr_pixbuf);
	
	gtk_tree_store_remove (store, parent_subtree_iter);
	g_tree_remove (priv->nodes_displayed, (gpointer) curr_symbol_id);

	/* don't forget to free this gchar */				   
	g_free (curr_symbol_name);
}

/* Add promptly a symbol to the gtktreeview or add it for a later add (waiting
 * for trigger).
 */
static void
prepare_for_adding (SymbolDBView *dbv, gint parent_symbol_id, 
					const gchar* symbol_name, gint symbol_id,
					const GdkPixbuf *pixbuf, const gchar* kind)
{
	SymbolDBViewPriv *priv;
	
	g_return_if_fail (dbv != NULL);	
	priv = dbv->priv;
	
	/* add to root if parent_symbol_id is <= 0 */
	if (parent_symbol_id <= 0)
	{			
		GtkTreeRowReference *curr_tree_row_ref;

		/* ok, let's check the kind of the symbol. Based on that we'll retrieve
		 * the row_ref
		 */
		if (strcmp (kind, "namespace") == 0)
		{
			curr_tree_row_ref = do_add_child_symbol_to_view (dbv, ROOT_NAMESPACE,
				   pixbuf, symbol_name, symbol_id);			
		}
		else if (strcmp (kind, "class") == 0)
		{
			curr_tree_row_ref = do_add_child_symbol_to_view (dbv, ROOT_CLASS,
				   pixbuf, symbol_name, symbol_id);
		}
		else if (strcmp (kind, "struct") == 0)
		{
			curr_tree_row_ref = do_add_child_symbol_to_view (dbv, ROOT_STRUCT,
				   pixbuf, symbol_name, symbol_id);
		}
		else if (strcmp (kind, "union") == 0)
		{
			curr_tree_row_ref = do_add_child_symbol_to_view (dbv, ROOT_UNION,
				   pixbuf, symbol_name, symbol_id);
		}
		else if (strcmp (kind, "function") == 0)
		{
			curr_tree_row_ref = do_add_child_symbol_to_view (dbv, ROOT_FUNCTION,
				   pixbuf, symbol_name, symbol_id);
		}
		else if (strcmp (kind, "variable") == 0)
		{
			curr_tree_row_ref = do_add_child_symbol_to_view (dbv, ROOT_VARIABLE,
				   pixbuf, symbol_name, symbol_id);
		}
		else if (strcmp (kind, "macro") == 0)
		{
			curr_tree_row_ref = do_add_child_symbol_to_view (dbv, ROOT_MACRO,
				   pixbuf, symbol_name, symbol_id);
		}
		else if (strcmp (kind, "typedef") == 0)
		{
			curr_tree_row_ref = do_add_child_symbol_to_view (dbv, ROOT_TYPEDEF,
				   pixbuf, symbol_name, symbol_id);
		}
		else if (strcmp (kind, "enumerator") == 0)
		{
			curr_tree_row_ref = do_add_child_symbol_to_view (dbv, ROOT_ENUMERATOR,
				   pixbuf, symbol_name, symbol_id);
		}
		else 
		{
			/* unknown row_ref... */
			g_warning ("prepare_for_adding (): unknown ref found. Adding to root.");
			curr_tree_row_ref = do_add_root_symbol_to_view (dbv, pixbuf, symbol_name,
													 symbol_id);
		}

		if (curr_tree_row_ref == NULL)
		{
			g_warning ("prepare_for_adding (): row_ref == NULL");
			return;
		}		
		
		/* we'll fake the gpointer to store an int */
		g_tree_insert (priv->nodes_displayed, (gpointer)symbol_id, 
					   curr_tree_row_ref);
				
		/* let's trigger the insertion of the symbol_id, there may be some children
		 * waiting for it.
		 */
		trigger_on_symbol_inserted (dbv, symbol_id);
	}
	else 
	{
		gpointer node;
		/* do we already have that parent_symbol displayed in gtktreeview? 
		 * If that's the case add it as children.
		 */
		node = g_tree_lookup (priv->nodes_displayed, (gpointer)parent_symbol_id);
		
		if (node != NULL) 
		{
			/* hey we found it */
			GtkTreeRowReference *child_row_ref;
/*			DEBUG_PRINT ("prepare_for_adding(): found node already displayed %d",
						 parent_symbol_id);*/
			
			child_row_ref = do_add_child_symbol_to_view (dbv, parent_symbol_id,
				   pixbuf, symbol_name, symbol_id);
			
			/* add the children_path to the GTree. */
			g_tree_insert (priv->nodes_displayed, (gpointer)symbol_id, 
						   child_row_ref);
			trigger_on_symbol_inserted (dbv, symbol_id);
		}
		else 
		{
/*			DEBUG_PRINT ("prepare_for_adding(): gonna pass parent: %d name: %s "
						 "id: %d to add_new_waiting_for", parent_symbol_id,
						 symbol_name, symbol_id);*/
			
			/* add it to the waiting_for trigger list */
			add_new_waiting_for (dbv, parent_symbol_id, symbol_name, symbol_id, 
								 pixbuf);
		}
	}
}


static void 
on_symbol_inserted (SymbolDBEngine *dbe, 
					gint symbol_id, gpointer data)
{
	SymbolDBEngineIterator *iterator;
	GtkTreeStore *store;

	/* it's not obligatory referred to a class inheritance */
	gint parent_symbol_id;
	SymbolDBView *dbv;
	SymbolDBViewPriv *priv;
	
	dbv = SYMBOL_DB_VIEW (data);

	g_return_if_fail (dbv != NULL);
	
	priv = dbv->priv;
	
/*	DEBUG_PRINT ("on_symbol_inserted -global- %d", symbol_id);*/
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)));

	parent_symbol_id = symbol_db_engine_get_parent_scope_id_by_symbol_id (dbe, 
																	symbol_id,
																	NULL);
	
/*	DEBUG_PRINT ("parent_symbol_id %d", parent_symbol_id);*/
	
	/* get the original symbol infos */
	iterator = symbol_db_engine_get_symbol_info_by_id (dbe, symbol_id, 
													   SYMINFO_SIMPLE |
													   SYMINFO_ACCESS |
													   SYMINFO_KIND);
	
	if (iterator != NULL) 
	{
		SymbolDBEngineIteratorNode *iter_node;
		const GdkPixbuf *pixbuf;
		const gchar* symbol_name;
		const gchar* symbol_kind;
		SymbolDBEngineIterator *iterator_for_children;
	
		iter_node = SYMBOL_DB_ENGINE_ITERATOR_NODE (iterator);
		symbol_kind = symbol_db_engine_iterator_node_get_symbol_extra_string (
							iter_node, SYMINFO_KIND);

		pixbuf = symbol_db_view_get_pixbuf (symbol_kind,
						symbol_db_engine_iterator_node_get_symbol_extra_string (
							iter_node, SYMINFO_ACCESS));
		symbol_name = symbol_db_engine_iterator_node_get_symbol_name (iter_node);
		
		/* check if one of the children [if they exist] of symbol_id are already 
		 * displayed. In that case we'll invalidate all of them.
		 * i.e. we're in an updating insertion.
		 */
		iterator_for_children = 
			symbol_db_engine_get_scope_members_by_symbol_id (dbe, symbol_id, -1,
															 -1,
															 SYMINFO_SIMPLE);

		if (iterator_for_children == NULL) 
		{
			/* we don't have children */
/*			DEBUG_PRINT ("on_symbol_inserted (): %d has no children.", symbol_id);*/
		}
		else 
		{
			/* hey there are some children here.. kill 'em all and put them on
			 * a waiting_for list 
			 */			
			do
			{
				gint curr_child_id;
				GtkTreeIter child_iter;
				GtkTreeRowReference *row_ref;
				GtkTreePath *path;
				SymbolDBEngineIteratorNode *iter_node;

				iter_node = SYMBOL_DB_ENGINE_ITERATOR_NODE (iterator_for_children);

				curr_child_id = 
					symbol_db_engine_iterator_node_get_symbol_id (iter_node);

/*				DEBUG_PRINT ("on_symbol_inserted (): %d has child %d",
							 symbol_id, curr_child_id);*/
				row_ref = g_tree_lookup (priv->nodes_displayed,
										 (gpointer)curr_child_id);

				if (row_ref == NULL) 
				{
					/* no node displayed found */
					continue;
				}
				
				path = gtk_tree_row_reference_get_path (row_ref);
				if (path == NULL) 
				{
					DEBUG_PRINT ("on_symbol_inserted (): path is null, something "
								 "went wrong ?!");
					continue;
				}
		
				if (gtk_tree_model_get_iter (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)),
                                 &child_iter, path) == FALSE) 
				{
					gtk_tree_path_free (path);
					continue;		
				}
				gtk_tree_path_free (path);
				
				/* put on waiting_for the subtree */
				do_recurse_subtree_and_invalidate (dbv, &child_iter, symbol_id);
			} while (symbol_db_engine_iterator_move_next (iterator_for_children) 
					 == TRUE);
			
			g_object_unref (iterator_for_children);
		}		
		
		prepare_for_adding (dbv, parent_symbol_id, symbol_name, symbol_id, pixbuf,
							symbol_kind);
		g_object_unref (iterator);
	}
}

static void
do_recurse_subtree_and_remove (SymbolDBView *dbv, 
							   GtkTreeIter *parent_subtree_iter)
{
	gint curr_symbol_id;
	const GdkPixbuf *curr_pixbuf;
	GtkTreeStore *store;
	gchar *curr_symbol_name;

	SymbolDBViewPriv *priv;
	
	g_return_if_fail (dbv != NULL);
	
	priv = dbv->priv;
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)));	
	
	gtk_tree_model_get (GTK_TREE_MODEL (store), parent_subtree_iter,
				COLUMN_SYMBOL_ID, &curr_symbol_id,
			    COLUMN_PIXBUF, &curr_pixbuf, 
				COLUMN_NAME, &curr_symbol_name,	/* no strdup required */
				-1);
	
	/*DEBUG_PRINT ("do_recurse_subtree_and_remove (): curr_symbol_id %d", 
				 curr_symbol_id);*/
				 
	while (gtk_tree_model_iter_has_child  (GTK_TREE_MODEL (store), parent_subtree_iter)) 
	{
		GtkTreeIter child;
		gtk_tree_model_iter_children (GTK_TREE_MODEL (store), &child, parent_subtree_iter);
		
		/* recurse */
		do_recurse_subtree_and_remove (dbv, &child);
	}

	gtk_tree_store_remove (store, parent_subtree_iter);
	g_tree_remove (priv->nodes_displayed, (gpointer) curr_symbol_id);

	/* don't forget to free this gchar */				   
	g_free (curr_symbol_name);
}


static void 
on_symbol_removed (SymbolDBEngine *dbe, gint symbol_id, gpointer data)
{
	GtkTreeStore *store;
	SymbolDBView *dbv;
	SymbolDBViewPriv *priv;
    GtkTreeIter  iter;	
	GtkTreeRowReference *row_ref;
	GtkTreePath *path;
	
	dbv = SYMBOL_DB_VIEW (data);

	g_return_if_fail (dbv != NULL);
	priv = dbv->priv;

	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)));

	DEBUG_PRINT ("on_symbol_removed (): -global- %d", symbol_id);

	row_ref = g_tree_lookup (priv->nodes_displayed, (gpointer)symbol_id);
	if (row_ref == NULL) 
	{
		DEBUG_PRINT ("on_symbol_removed (): ERROR: cannot remove %d", symbol_id);
		return;
	}
 
	path = gtk_tree_row_reference_get_path (row_ref);
	if (path == NULL) 
	{
		DEBUG_PRINT ("on_symbol_removed (): ERROR2: cannot remove %d", symbol_id);
		return;
	}
	
	if (gtk_tree_model_get_iter (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)),
                                 &iter, path) == FALSE) 
	{
		DEBUG_PRINT ("on_symbol_removed (): iter was not set ?![%d]",
					 symbol_id);
		gtk_tree_path_free (path);
		return;
	}
	gtk_tree_path_free (path);

	do_recurse_subtree_and_remove (dbv, &iter);
}

/**
 * Add at most ONE dummy child to the parent_iter. This is done to let the parent_iter
 * node be expandable.
 */
static void
sdb_view_do_add_hidden_dummy_child (SymbolDBView *dbv, SymbolDBEngine *dbe,
						GtkTreeIter *parent_iter,  gint parent_symbol_id)
{
	SymbolDBEngineIterator *child_iterator;
	GtkTreeStore *store;
	SymbolDBViewPriv *priv;
	
	g_return_if_fail (dbv != NULL);
	priv = dbv->priv;
	
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)));
	child_iterator = symbol_db_engine_get_scope_members_by_symbol_id (dbe, 
						parent_symbol_id, 
						1, 
						-1, 
						SYMINFO_SIMPLE | SYMINFO_ACCESS | SYMINFO_KIND);
	
	if (child_iterator != NULL)
	{
		/* hey we have something here... */
		GtkTreeIter child_iter;
		
		gtk_tree_store_append (store, &child_iter, parent_iter);			
		gtk_tree_store_set (store, &child_iter,
					COLUMN_PIXBUF, NULL,
					COLUMN_NAME, _("Loading..."),
					COLUMN_SYMBOL_ID, DUMMY_SYMBOL_ID,
					-1);

		g_object_unref (child_iterator);
	}	
}

/**
 * Usually on a row expanded event we should perform the following steps:
 * 1. retrieve a list of scoped children.
 * 2. check if the nth children has already been displayed or not.
 * 3. if it isn't then append a child *and* check if that child has children itself.
 *    using a dummy node we can achieve a performant population while setting an expand
 *    mark on the child firstly appended.
 */
void
symbol_db_view_row_expanded (SymbolDBView *dbv, SymbolDBEngine *dbe, 
							 GtkTreeIter *expanded_iter)
{
	GtkTreeStore *store;
	gint expanded_symbol_id;
	SymbolDBViewPriv *priv;
	SymbolDBEngineIterator *iterator;	
	GtkTreePath *path;
		
	g_return_if_fail (dbv != NULL);
	priv = dbv->priv;

	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)));
	
	gtk_tree_model_get (GTK_TREE_MODEL (store), expanded_iter,
						COLUMN_SYMBOL_ID, &expanded_symbol_id, -1);
	
	/* remove the dummy item, if present */
	if (gtk_tree_model_iter_has_child  (GTK_TREE_MODEL (store), expanded_iter)) 
	{
		GtkTreeIter child;
		gint dummy_symbol;
		gtk_tree_model_iter_children (GTK_TREE_MODEL (store), &child, expanded_iter);

		gtk_tree_model_get (GTK_TREE_MODEL (store), &child,
						COLUMN_SYMBOL_ID, &dummy_symbol, -1);
		
		if (dummy_symbol == DUMMY_SYMBOL_ID)
			gtk_tree_store_remove (store, &child);		
	}
	
	DEBUG_PRINT ("symbol_db_view_row_expanded (): expanded %d", expanded_symbol_id);
	
	/* Step 1 */
	iterator = symbol_db_engine_get_scope_members_by_symbol_id (dbe, 
									expanded_symbol_id, 
									-1,
									-1,
									SYMINFO_SIMPLE|
									SYMINFO_KIND|
									SYMINFO_ACCESS);

	if (iterator != NULL)
	{
		do {
			gint curr_symbol_id;
			SymbolDBEngineIteratorNode *iter_node;
			const GdkPixbuf *pixbuf;
			const gchar* symbol_name;			
			GtkTreeIter child_iter;
			GtkTreePath *path;
			GtkTreeRowReference *child_row_ref;
			gpointer node;

			iter_node = SYMBOL_DB_ENGINE_ITERATOR_NODE (iterator);

			curr_symbol_id = symbol_db_engine_iterator_node_get_symbol_id (iter_node);

			/* Step 2:
			 * check if the curr_symbol_id is already displayed. In that case
			 * skip to the next symbol 
			 */
			node = g_tree_lookup (priv->nodes_displayed, (gpointer)curr_symbol_id);
		
			if (node != NULL) 
			{
				continue;
			}
			
			/* Step 3 */
			/* ok we must display this symbol */			
			pixbuf = symbol_db_view_get_pixbuf (
						symbol_db_engine_iterator_node_get_symbol_extra_string (
							iter_node, SYMINFO_KIND),
						symbol_db_engine_iterator_node_get_symbol_extra_string (
							iter_node, SYMINFO_ACCESS));

			symbol_name = symbol_db_engine_iterator_node_get_symbol_name (iter_node);

			gtk_tree_store_append (store, &child_iter, expanded_iter);
			
			gtk_tree_store_set (store, &child_iter,
				COLUMN_PIXBUF, pixbuf,
				COLUMN_NAME, symbol_name,
				COLUMN_SYMBOL_ID, curr_symbol_id, 
				-1);	

			path = gtk_tree_model_get_path (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)),
                                          	&child_iter);	
			child_row_ref = gtk_tree_row_reference_new (
						gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)), path);
			gtk_tree_path_free (path);
			
			/* insert the just append row_ref into the GTree for a quick retrieval
			 * later 
			 */
			g_tree_insert (priv->nodes_displayed, (gpointer)curr_symbol_id, 
						   child_row_ref);			
			
			/* good. Let's check now for a child (B) of the just appended child (A). 
			 * Adding B (a dummy one for now) to A will make A expandable
			 */
			sdb_view_do_add_hidden_dummy_child (dbv, dbe, &child_iter, curr_symbol_id);
			
		} while (symbol_db_engine_iterator_move_next (iterator) == TRUE);
	}
	

	path =  gtk_tree_model_get_path (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)),
  	                                      	expanded_iter);	

	gtk_tree_view_expand_row (GTK_TREE_VIEW (dbv), path, FALSE);
	gtk_tree_path_free (path);	
	
}

static GtkTreeStore *
sdb_view_locals_create_new_store ()
{
	GtkTreeStore *store;
	store = gtk_tree_store_new (COLUMN_MAX, GDK_TYPE_PIXBUF,
				    G_TYPE_STRING, G_TYPE_INT);	
	return store;
}

static void
sdb_view_init (SymbolDBView *object)
{
	SymbolDBView *dbv;
	GtkTreeStore *store;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
	SymbolDBViewPriv *priv;
	
	dbv = SYMBOL_DB_VIEW (object);
	dbv->priv = g_new0 (SymbolDBViewPriv, 1);
	
	priv = dbv->priv;
	
	/* initialize some priv data */
	priv->insert_handler = 0;
	priv->remove_handler = 0;
	priv->nodes_displayed = NULL;
	priv->waiting_for = NULL;
	priv->row_ref_namespace = NULL;
	priv->row_ref_class = NULL;
	priv->row_ref_struct = NULL;
	priv->row_ref_union = NULL;
	priv->row_ref_function = NULL;
	priv->row_ref_variable = NULL;
	priv->row_ref_macro = NULL;
	priv->row_ref_typedef = NULL;
	priv->row_ref_enumerator = NULL;
	priv->scan_end_handler = 0;
	priv->remove_handler = 0;
	priv->scan_end_handler = 0;	

	/* Tree and his model */
	store = NULL;

	gtk_tree_view_set_model (GTK_TREE_VIEW (dbv), GTK_TREE_MODEL (store));	
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (dbv), FALSE);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dbv));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	/* search through the tree interactively */
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (dbv), COLUMN_NAME);	
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW (dbv), TRUE);	

	/* Columns */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing (column,
					 GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Symbol"));

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "pixbuf",
					    	COLUMN_PIXBUF);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
					    	COLUMN_NAME);

	gtk_tree_view_append_column (GTK_TREE_VIEW (dbv), column);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (dbv), column);
}


static void
sdb_view_finalize (GObject *object)
{
	SymbolDBView *view = SYMBOL_DB_VIEW (object);
	SymbolDBViewPriv *priv = view->priv;

	DEBUG_PRINT ("finalizing symbol_db_view ()");
	
	symbol_db_view_clear_cache (view);
	
	g_free (priv);
	
	/* dbe must be freed outside. */	
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
sdb_view_class_init (SymbolDBViewClass *klass)
{
	SymbolDBViewClass *sdbc;
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	parent_class = g_type_class_peek_parent (klass);

	sdbc = SYMBOL_DB_VIEW_CLASS (klass);
	object_class->finalize = sdb_view_finalize;
}

GType
symbol_db_view_get_type (void)
{
	static GType obj_type = 0;

	if (!obj_type)
	{
		static const GTypeInfo obj_info = {
			sizeof (SymbolDBViewClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) sdb_view_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,	/* class_data */
			sizeof (SymbolDBViewClass),
			0,	/* n_preallocs */
			(GInstanceInitFunc) sdb_view_init,
			NULL	/* value_table */
		};
		obj_type = g_type_register_static (GTK_TYPE_TREE_VIEW,
						   "SymbolDBView",
						   &obj_info, 0);
	}
	return obj_type;
}


#define CREATE_SYM_ICON(N, F) \
	pix_file = anjuta_res_get_pixmap_file (F); \
	g_hash_table_insert (pixbufs_hash, \
					   N, \
					   gdk_pixbuf_new_from_file (pix_file, NULL)); \
	g_free (pix_file);

static void
sdb_view_load_symbol_pixbufs ()
{
	gchar *pix_file;
	
	if (pixbufs_hash != NULL) 
	{
		/* we already have loaded it */
		return;
	}

	pixbufs_hash = g_hash_table_new (g_str_hash, g_str_equal);

	CREATE_SYM_ICON ("class",             "Icons.16x16.Class");	
	CREATE_SYM_ICON ("enum",     	  	  "Icons.16x16.Enum");		
	CREATE_SYM_ICON ("enumerator",     	  "Icons.16x16.Enum");	
	CREATE_SYM_ICON ("function",          "Icons.16x16.Method");	
	CREATE_SYM_ICON ("interface",         "Icons.16x16.Interface");	
	CREATE_SYM_ICON ("macro",             "Icons.16x16.Field");	
	CREATE_SYM_ICON ("namespace",         "Icons.16x16.NameSpace");
	CREATE_SYM_ICON ("none",              "Icons.16x16.Literal");
	CREATE_SYM_ICON ("struct",            "Icons.16x16.ProtectedStruct");
	CREATE_SYM_ICON ("typedef",           "Icons.16x16.Reference");
	CREATE_SYM_ICON ("union",             "Icons.16x16.PrivateStruct");
	CREATE_SYM_ICON ("variable",          "Icons.16x16.Literal");
	
	CREATE_SYM_ICON ("privateclass",      "Icons.16x16.PrivateClass");
	CREATE_SYM_ICON ("privateenum",   	  "Icons.16x16.PrivateEnum");
	CREATE_SYM_ICON ("privatefield",   	  "Icons.16x16.PrivateField");
	CREATE_SYM_ICON ("privatefunction",   "Icons.16x16.PrivateMethod");
	CREATE_SYM_ICON ("privateinterface",  "Icons.16x16.PrivateInterface");	
	CREATE_SYM_ICON ("privatemember",     "Icons.16x16.PrivateProperty");	
	CREATE_SYM_ICON ("privatemethod",     "Icons.16x16.PrivateMethod");
	CREATE_SYM_ICON ("privateproperty",   "Icons.16x16.PrivateProperty");
	CREATE_SYM_ICON ("privatestruct",     "Icons.16x16.PrivateStruct");

	CREATE_SYM_ICON ("protectedclass",    "Icons.16x16.ProtectedClass");	
	CREATE_SYM_ICON ("protectedenum",     "Icons.16x16.ProtectedEnum");
	CREATE_SYM_ICON ("protectedfield",    "Icons.16x16.ProtectedField");	
	CREATE_SYM_ICON ("protectedmember",   "Icons.16x16.ProtectedProperty");
	CREATE_SYM_ICON ("protectedmethod",   "Icons.16x16.ProtectedMethod");
	CREATE_SYM_ICON ("protectedproperty", "Icons.16x16.ProtectedProperty");
	
	CREATE_SYM_ICON ("publicclass",    	  "Icons.16x16.Class");	
	CREATE_SYM_ICON ("publicenum",    	  "Icons.16x16.Enum");	
	CREATE_SYM_ICON ("publicfunction",    "Icons.16x16.Method");
	CREATE_SYM_ICON ("publicmember",      "Icons.16x16.InternalMethod");
	CREATE_SYM_ICON ("publicproperty",    "Icons.16x16.InternalProperty");
	CREATE_SYM_ICON ("publicstruct",      "Icons.16x16.ProtectedStruct");
}

/**
 * @return The pixbufs. It will initialize pixbufs first if they weren't before
 * @param node_access can be NULL.
 */
const GdkPixbuf*
symbol_db_view_get_pixbuf  (const gchar *node_type, const gchar *node_access)
{
	gchar *search_node;
	GdkPixbuf *pix;
	if (!pixbufs_hash)
		sdb_view_load_symbol_pixbufs ();
	
	g_return_val_if_fail (node_type != NULL, NULL);

	/* is there a better/quicker method to retrieve pixbufs? */
	if (node_access != NULL)
		search_node = g_strdup_printf ("%s%s", node_access, node_type);
	else 
	{ 
		/* we will not free search_node gchar, so casting here is ok. */
		search_node = (gchar*)node_type;
	}
	pix = GDK_PIXBUF (g_hash_table_lookup (pixbufs_hash, search_node));
	
	if (node_access)
		g_free (search_node);
	
	if (pix == NULL)
		DEBUG_PRINT ("symbol_db_view_get_pixbuf (): no pixbuf for %s %s",
					 node_type, node_access);
	
	return pix;
}


GtkWidget* 
symbol_db_view_new (void)
{
	return GTK_WIDGET (g_object_new (SYMBOL_TYPE_DB_VIEW, NULL));
}

gboolean
symbol_db_view_get_file_and_line (SymbolDBView *dbv, SymbolDBEngine *dbe,
							GtkTreeIter * iter, gint *OUT_line, gchar **OUT_file) 
{
	GtkTreeStore *store;
		
	g_return_val_if_fail (dbv != NULL, FALSE);
	g_return_val_if_fail (dbe != NULL, FALSE);	
	g_return_val_if_fail (iter != NULL, FALSE);
	
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)));
	
	if (store)
	{
		gint symbol_id;
		const gchar* relative_file;
		SymbolDBEngineIteratorNode *node;
		
		gtk_tree_model_get (GTK_TREE_MODEL
				    (store), iter,
				    COLUMN_SYMBOL_ID, &symbol_id, -1);
		
		/* getting line at click time with a query is faster than updating every 
		 * entry in the gtktreeview. We can be sure that the db is in a consistent 
		 * state and has all the last infos 
		 */
		node = SYMBOL_DB_ENGINE_ITERATOR_NODE (
					symbol_db_engine_get_symbol_info_by_id (dbe, symbol_id, 
															SYMINFO_SIMPLE |
															SYMINFO_FILE_PATH));
		if (node != NULL) 
		{
			*OUT_line = symbol_db_engine_iterator_node_get_symbol_file_pos (node);
			relative_file = 
				symbol_db_engine_iterator_node_get_symbol_extra_string (node,
															SYMINFO_FILE_PATH);
			*OUT_file = symbol_db_engine_get_full_local_path (dbe, relative_file);
			return TRUE;
		}		
	}
	
	return FALSE;
}								


/**
 * @param row_ref One of the base root symbols, like namespaces, classes etc.
 * 		  You can find them inside SymbolDBViewPriv struct.
 * @param root_kind Simple name to indicate the kind saved on database. It should
 * 		  be something like "namespace", "class", "struct" and so on.
 */
static void
sdb_view_populate_base_root (SymbolDBView *dbv, SymbolDBEngine *dbe,
							 GtkTreeRowReference *row_ref,
							 const gchar* root_kind)
{
	SymbolDBViewPriv *priv;
	GtkTreeIter root_iter;	
	SymbolDBEngineIterator *iterator;
	GtkTreeStore *store;
	
	g_return_if_fail (dbv != NULL);
	
	priv = dbv->priv;
	
	DEBUG_PRINT ("sdb_view_populate_base_root ()");
	if (sdb_view_get_iter_from_row_ref (dbv, row_ref, &root_iter) == FALSE)
	{
		DEBUG_PRINT ("sdb_view_populate_base_root (): root_iter == NULL");
		return;
	}		

	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)));
	iterator = symbol_db_engine_get_global_members (dbe, root_kind, TRUE, 
													20,
													-1,
													SYMINFO_SIMPLE |
												  	SYMINFO_ACCESS |
													SYMINFO_KIND);
	if (iterator != NULL)
	{
		do {
			gint curr_symbol_id;
			SymbolDBEngineIteratorNode *iter_node;
			const GdkPixbuf *pixbuf;
			const gchar* symbol_name;			
			GtkTreeIter child_iter;
			GtkTreePath *path;
			GtkTreeRowReference *child_row_ref;

			iter_node = SYMBOL_DB_ENGINE_ITERATOR_NODE (iterator);

			curr_symbol_id = symbol_db_engine_iterator_node_get_symbol_id (iter_node);

			pixbuf = symbol_db_view_get_pixbuf (
						symbol_db_engine_iterator_node_get_symbol_extra_string (
							iter_node, SYMINFO_KIND),
						symbol_db_engine_iterator_node_get_symbol_extra_string (
							iter_node, SYMINFO_ACCESS));

			symbol_name = symbol_db_engine_iterator_node_get_symbol_name (iter_node);

			gtk_tree_store_append (store, &child_iter, &root_iter);			
			gtk_tree_store_set (store, &child_iter,
				COLUMN_PIXBUF, pixbuf,
				COLUMN_NAME, symbol_name,
				COLUMN_SYMBOL_ID, curr_symbol_id, 
				-1);	

			path = gtk_tree_model_get_path (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)),
                                          	&child_iter);	
			child_row_ref = gtk_tree_row_reference_new (
						gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)), path);
			gtk_tree_path_free (path);
			
			/* insert the just append row_ref into the GTree for a quick retrieval
			 * later 
			 */
			g_tree_insert (priv->nodes_displayed, (gpointer)curr_symbol_id, 
						   child_row_ref);

			/* add a dummy child */
			sdb_view_do_add_hidden_dummy_child (dbv, dbe,
						&child_iter, curr_symbol_id);
			
		} while (symbol_db_engine_iterator_move_next (iterator) == TRUE);
		
		g_object_unref (iterator);
	}	
}

static void
sdb_view_build_and_display_base_tree (SymbolDBView *dbv, SymbolDBEngine *dbe)
{
	GtkTreeStore *store;
	GtkTreePath *path;
	SymbolDBViewPriv *priv;
	GtkTreeIter iter;
	
	g_return_if_fail (dbv != NULL);
	
	priv = dbv->priv;
	
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)));
	
	gtk_tree_store_append (store, &iter, NULL);
	gtk_tree_store_set (store, &iter,
				COLUMN_PIXBUF, symbol_db_view_get_pixbuf ("namespace", NULL),
				COLUMN_NAME, _("Namespaces"),
				COLUMN_SYMBOL_ID, ROOT_NAMESPACE,
				-1);
	path = gtk_tree_model_get_path (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)),
                                          &iter);	
	priv->row_ref_namespace = gtk_tree_row_reference_new (
						gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)), path);
	gtk_tree_path_free (path);
	sdb_view_populate_base_root (dbv, dbe, priv->row_ref_namespace, "namespace");
	/* don't forget to insert also the 'fake' symbol id into the binary tree */
	g_tree_insert (priv->nodes_displayed, (gpointer)ROOT_NAMESPACE, priv->row_ref_namespace);	
	

	gtk_tree_store_append (store, &iter, NULL);
	gtk_tree_store_set (store, &iter,
				COLUMN_PIXBUF, symbol_db_view_get_pixbuf ("class", NULL),
				COLUMN_NAME, _("Classes"),
				COLUMN_SYMBOL_ID, ROOT_CLASS,
				-1);
	path = gtk_tree_model_get_path (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)),
                                          &iter);	
	priv->row_ref_class = gtk_tree_row_reference_new (
						gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)), path);
	gtk_tree_path_free (path);
	sdb_view_populate_base_root (dbv, dbe, priv->row_ref_class, "class");
	g_tree_insert (priv->nodes_displayed, (gpointer)ROOT_CLASS, priv->row_ref_class);	
	
	
	gtk_tree_store_append (store, &iter, NULL);
	gtk_tree_store_set (store, &iter,
				COLUMN_PIXBUF, symbol_db_view_get_pixbuf ("struct", NULL),
				COLUMN_NAME, _("Structs"),
				COLUMN_SYMBOL_ID, ROOT_STRUCT,
				-1);
	path = gtk_tree_model_get_path (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)),
                                          &iter);	
	priv->row_ref_struct = gtk_tree_row_reference_new (
						gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)), path);
	gtk_tree_path_free (path);
	sdb_view_populate_base_root (dbv, dbe, priv->row_ref_struct, "struct");
	g_tree_insert (priv->nodes_displayed, (gpointer)ROOT_STRUCT, priv->row_ref_struct);	
	
	
	gtk_tree_store_append (store, &iter, NULL);
	gtk_tree_store_set (store, &iter,
				COLUMN_PIXBUF, symbol_db_view_get_pixbuf ("union", NULL),
				COLUMN_NAME, _("Unions"),
				COLUMN_SYMBOL_ID, ROOT_UNION,
				-1);
	path = gtk_tree_model_get_path (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)),
                                          &iter);	
	priv->row_ref_union = gtk_tree_row_reference_new (
						gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)), path);
	gtk_tree_path_free (path);	
	sdb_view_populate_base_root (dbv, dbe, priv->row_ref_union, "union");
	g_tree_insert (priv->nodes_displayed, (gpointer)ROOT_UNION, priv->row_ref_union);	
	
	
	gtk_tree_store_append (store, &iter, NULL);
	gtk_tree_store_set (store, &iter,
				COLUMN_PIXBUF, symbol_db_view_get_pixbuf ("function", NULL),
				COLUMN_NAME, _("Functions"),
				COLUMN_SYMBOL_ID, ROOT_FUNCTION,
				-1);
	path = gtk_tree_model_get_path (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)),
                                          &iter);	
	priv->row_ref_function = gtk_tree_row_reference_new (
						gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)), path);
	gtk_tree_path_free (path);
	sdb_view_populate_base_root (dbv, dbe, priv->row_ref_function, "function");
	g_tree_insert (priv->nodes_displayed, (gpointer)ROOT_FUNCTION, priv->row_ref_function);	
	
	
	gtk_tree_store_append (store, &iter, NULL);
	gtk_tree_store_set (store, &iter,
				COLUMN_PIXBUF, symbol_db_view_get_pixbuf ("variable", NULL),
				COLUMN_NAME, _("Variables"),
				COLUMN_SYMBOL_ID, ROOT_VARIABLE,
				-1);
	path = gtk_tree_model_get_path (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)),
                                          &iter);	
	priv->row_ref_variable = gtk_tree_row_reference_new (
						gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)), path);
	gtk_tree_path_free (path);
	sdb_view_populate_base_root (dbv, dbe, priv->row_ref_variable, "variable");
	g_tree_insert (priv->nodes_displayed, (gpointer)ROOT_VARIABLE, priv->row_ref_variable);
	
	
	gtk_tree_store_append (store, &iter, NULL);
	gtk_tree_store_set (store, &iter,
				COLUMN_PIXBUF, symbol_db_view_get_pixbuf ("macro", NULL),
				COLUMN_NAME, _("Macros"),
				COLUMN_SYMBOL_ID, ROOT_MACRO,
				-1);
	path = gtk_tree_model_get_path (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)),
                                          &iter);	
	priv->row_ref_macro = gtk_tree_row_reference_new (
						gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)), path);
	gtk_tree_path_free (path);
	sdb_view_populate_base_root (dbv, dbe, priv->row_ref_macro, "macro");
	g_tree_insert (priv->nodes_displayed, (gpointer)ROOT_MACRO, priv->row_ref_macro);
	
	
	gtk_tree_store_append (store, &iter, NULL);
	gtk_tree_store_set (store, &iter,
				COLUMN_PIXBUF, symbol_db_view_get_pixbuf ("typedef", NULL),
				COLUMN_NAME, _("Typedefs"),
				COLUMN_SYMBOL_ID, ROOT_TYPEDEF,
				-1);
	path = gtk_tree_model_get_path (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)),
                                          &iter);	
	priv->row_ref_typedef = gtk_tree_row_reference_new (
						gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)), path);
	gtk_tree_path_free (path);
	sdb_view_populate_base_root (dbv, dbe, priv->row_ref_typedef, "typedef");
	g_tree_insert (priv->nodes_displayed, (gpointer)ROOT_TYPEDEF, priv->row_ref_typedef);
	
	
	gtk_tree_store_append (store, &iter, NULL);
	gtk_tree_store_set (store, &iter,
				COLUMN_PIXBUF, symbol_db_view_get_pixbuf ("enumerator", NULL),
				COLUMN_NAME, _("Enumerators"),
				COLUMN_SYMBOL_ID, ROOT_ENUMERATOR,
				-1);
	path = gtk_tree_model_get_path (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)),
                                          &iter);	
	priv->row_ref_enumerator = gtk_tree_row_reference_new (
						gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)), path);
	gtk_tree_path_free (path);
	sdb_view_populate_base_root (dbv, dbe, priv->row_ref_enumerator, "enumerator");
	g_tree_insert (priv->nodes_displayed, (gpointer)ROOT_ENUMERATOR, priv->row_ref_enumerator);
}

void
symbol_db_view_recv_signals_from_engine (SymbolDBView *dbv, SymbolDBEngine *dbe,
										 gboolean enable_status)
{
	SymbolDBViewPriv *priv;

	g_return_if_fail (dbv != NULL);
	priv = dbv->priv;		
	
	if (enable_status == TRUE) 
	{
		/* connect some signals */
		if (priv->insert_handler <= 0) 
		{
			priv->insert_handler = 	g_signal_connect (G_OBJECT (dbe), "symbol-inserted",
						  G_CALLBACK (on_symbol_inserted), dbv);
		}

		if (priv->remove_handler <= 0)
		{
			priv->remove_handler = g_signal_connect (G_OBJECT (dbe), "symbol-removed",
						  G_CALLBACK (on_symbol_removed), dbv);
		}	

		if (priv->scan_end_handler <= 0)
		{
			priv->scan_end_handler = g_signal_connect (G_OBJECT (dbe), "scan-end",
						  G_CALLBACK (on_scan_end), dbv);
		}
	}
	else		/* disconnect them, if they were ever connected before */
	{
		if (priv->insert_handler >= 0) 
		{
			g_signal_handler_disconnect (G_OBJECT (dbe), priv->insert_handler);
			priv->insert_handler = 0;
		}

		if (priv->remove_handler >= 0)
		{
			g_signal_handler_disconnect (G_OBJECT (dbe), priv->remove_handler);
			priv->remove_handler = 0;
		}	

		if (priv->scan_end_handler >= 0)
		{
			g_signal_handler_disconnect (G_OBJECT (dbe), priv->scan_end_handler);
			priv->scan_end_handler = 0;
		}
	}
}

void
symbol_db_view_open (SymbolDBView *dbv, SymbolDBEngine *dbe)
{
	SymbolDBViewPriv *priv;
	GtkTreeStore *store;
	
	g_return_if_fail (dbv != NULL);
	
	priv = dbv->priv;

	symbol_db_view_clear_cache (dbv);
	
	store = sdb_view_locals_create_new_store ();
	gtk_tree_view_set_model (GTK_TREE_VIEW (dbv), GTK_TREE_MODEL (store));
	
	priv->nodes_displayed = g_tree_new_full ((GCompareDataFunc)&gtree_compare_func, 
										 NULL,
										 NULL,
										 (GDestroyNotify)&gtk_tree_row_reference_free);

	priv->waiting_for = g_tree_new_full ((GCompareDataFunc)&gtree_compare_func, 
									 NULL,
									 NULL,
									 NULL);
	
	sdb_view_build_and_display_base_tree (dbv, dbe);
}
