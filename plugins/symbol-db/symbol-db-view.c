/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Massimo Cora' 2007-2008 <maxcvs@email.it>
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
#include <libanjuta/resources.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>
#include "symbol-db-view.h"
#include "symbol-db-engine.h"

#define DUMMY_SYMBOL_ID		G_MININT32+1

enum {
	COLUMN_PIXBUF,
	COLUMN_NAME,
	COLUMN_SYMBOL_ID,
	COLUMN_MAX
};

/* positive ids are used in real database */
enum {
	ROOT_GLOBAL = G_MAXINT32
};

struct _SymbolDBViewPriv
{
	gint insert_handler;
	gint remove_handler;	
	gint scan_end_handler;	

	GtkTreeRowReference *row_ref_global;
	GTree *nodes_displayed;
	GTree *waiting_for;
	GTree *expanding_gfunc_ids;
};

typedef struct _WaitingForSymbol {
	gint child_symbol_id;
	gchar *child_symbol_name;
	const GdkPixbuf *pixbuf;
	
} WaitingForSymbol;

typedef struct _NodeIdleExpand  {	
	SymbolDBView *dbv;
	SymbolDBEngineIterator *iterator;
	SymbolDBEngine *dbe;	
	GtkTreePath *expanded_path;
	gint expanded_symbol_id;
	
} NodeIdleExpand;


static GtkTreeViewClass *parent_class = NULL;
static void 
trigger_on_symbol_inserted (SymbolDBView *dbv, gint symbol_id);


static void
waiting_for_symbol_destroy (WaitingForSymbol *wfs)
{	
	g_return_if_fail (wfs != NULL);
	g_free (wfs->child_symbol_name);
	g_free (wfs);
}

static inline gboolean
sdb_view_get_iter_from_row_ref (SymbolDBView *dbv, GtkTreeRowReference *row_ref, 
								GtkTreeIter *OUT_iter)
{
	GtkTreePath *path;
	if (row_ref == NULL) 
	{
		/* no node displayed found */
		return FALSE;
	}
			
	path = gtk_tree_row_reference_get_path (row_ref);
	if (path == NULL) 
	{
		DEBUG_PRINT ("%s", "sdb_view_get_iter_from_row_ref (): path is null, something "
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
	if (value == NULL || key == NULL)
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
	
	/* this will free also the priv->row_ref* instances */	
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
on_scan_end (SymbolDBEngine *dbe, gint process_id, gpointer data)
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
		priv->waiting_for = g_tree_new_full ((GCompareDataFunc)&symbol_db_gtree_compare_func, 
									 NULL,
									 NULL,
									 NULL);

	}
}

static inline GtkTreeRowReference *
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

/* before calling this function be sure that parent_symbol_id is already into
 * nodes_displayed GTree, or this will complain about it and will bail out 
 * with a warning.
 */
static inline GtkTreeRowReference *
do_add_child_symbol_to_view (SymbolDBView *dbv, gint parent_symbol_id,
					   const GdkPixbuf *pixbuf, const gchar* symbol_name,
					   gint symbol_id)
{
	SymbolDBViewPriv *priv;
	GtkTreePath *path;
	GtkTreeStore *store;
	GtkTreeIter iter, child_iter;
	GtkTreeRowReference *row_ref;
	
	priv = dbv->priv;
	
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)));

	/* look up the row ref in the hashtable, then get its associated gtktreeiter */
	row_ref = g_tree_lookup (priv->nodes_displayed, GINT_TO_POINTER (parent_symbol_id));
	
	if (sdb_view_get_iter_from_row_ref (dbv, row_ref, &iter) == FALSE)
	{
		g_warning ("do_add_symbol_to_view (): something went wrong.");
		return NULL;
	}
	
	/* append a new child &child_iter, with a parent of &iter */
	gtk_tree_store_append (store, &child_iter, &iter);
			
	gtk_tree_store_set (store, &child_iter,
		COLUMN_PIXBUF, pixbuf,
		COLUMN_NAME, symbol_name,
		COLUMN_SYMBOL_ID, symbol_id,
		-1);	
	
	/* grab the row ref and return it */
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
	g_tree_insert (priv->nodes_displayed, GINT_TO_POINTER (wfs->child_symbol_id), 
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

	/*DEBUG_PRINT ("trigger_on_symbol_inserted (): triggering %d", symbol_id);*/
	
	/* try to find a waiting for symbol */
	slist = g_tree_lookup (priv->waiting_for, GINT_TO_POINTER (symbol_id));
	
	if (slist == NULL) 
	{
		/* nothing waiting for us */
		/*DEBUG_PRINT ("%s", "trigger_on_symbol_inserted (): no children waiting for us...");*/
		return;
	}
	else {
		gint i;
		gint length = g_slist_length (slist);

		/*DEBUG_PRINT ("trigger_on_symbol_inserted (): consuming slist for parent %d",
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
		g_tree_remove (priv->waiting_for, GINT_TO_POINTER (symbol_id));
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

	/* we don't want a negative parent_symbol_id */
	if (parent_symbol_id < 0)
		return;
	
	/* check if we already have some children waiting for a 
	 * specific father to be inserted, then add this symbol_id to the list 
	 * (or create a new one)
	 */
	WaitingForSymbol *wfs;
			
	wfs = g_new0 (WaitingForSymbol, 1);
	wfs->child_symbol_id = symbol_id;
	wfs->child_symbol_name = g_strdup (symbol_name);
	wfs->pixbuf = pixbuf;
				
	node = g_tree_lookup (priv->waiting_for, GINT_TO_POINTER (parent_symbol_id));
	if (node == NULL) 
	{
		/* no lists already set. Create one. */
		GSList *slist;					
		slist = g_slist_alloc ();			
				
		slist = g_slist_prepend (slist, wfs);

		/* add it to the binary tree. */
		g_tree_insert (priv->waiting_for, GINT_TO_POINTER (parent_symbol_id), 
							   slist);
	}
	else 
	{
		/* found a list */
		GSList *slist;
		slist = (GSList*)node;
		
		slist = g_slist_prepend (slist, wfs);

		g_tree_replace (priv->waiting_for, GINT_TO_POINTER (parent_symbol_id), 
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
	g_tree_remove (priv->nodes_displayed, GINT_TO_POINTER (curr_symbol_id));

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
	g_return_if_fail (kind != NULL);
	priv = dbv->priv;
	
	/* add to root if parent_symbol_id is <= 0 */
	if (parent_symbol_id <= 0)
	{
		GtkTreeRowReference *curr_tree_row_ref = NULL;
		
		/* ok, let's check the kind of the symbol. Based on that we'll retrieve
		 * the row_ref. It's quicker to check onlyl the first char than the whole 
		 * string.
		 */
		switch (kind[0]) 
		{
			case 'n':		/* namespace */
				curr_tree_row_ref = do_add_root_symbol_to_view (dbv, 
																pixbuf, 
																symbol_name, 
																symbol_id);			
				break;

			case 'c':		/* class */
			case 's': 		/* struct */
				curr_tree_row_ref = do_add_child_symbol_to_view (dbv, 
											ROOT_GLOBAL, pixbuf, symbol_name, 
																 symbol_id);
				break;
			
			case 'u':		/* union */
			case 'f':		/* function */
			case 'v':		/* variable */
			case 't':		/* typedef */
			case 'e':		/* enumerator */
			default:
			{
				gpointer node;
				/* Vars/Other may not be displayed already. Check it. */
				node = g_tree_lookup (priv->nodes_displayed, GINT_TO_POINTER (-ROOT_GLOBAL));
		
				if (node != NULL) 
				{
					/* hey we found it */
					/* note the negative: we'll store these under the vars/Other node */
					curr_tree_row_ref = do_add_child_symbol_to_view (dbv, 
											-ROOT_GLOBAL, pixbuf, symbol_name, 
																 symbol_id);
				}
				else 
				{
					/* add it to the waiting_for trigger list */
					add_new_waiting_for (dbv, parent_symbol_id, symbol_name, symbol_id, 
										 pixbuf);
				}				
				break;
			}
		}

		if (curr_tree_row_ref == NULL)
		{
			return;
		}		
		
		/* we'll fake the gpointer to store an int */
		g_tree_insert (priv->nodes_displayed, GINT_TO_POINTER (symbol_id), 
					   curr_tree_row_ref);
				
		/* let's trigger the insertion of the symbol_id, there may be some children
		 * waiting for it.
		 */
		trigger_on_symbol_inserted (dbv, symbol_id);
	}
	else 
	{
		gpointer node;
		
		switch (kind[0]) 
		{
			case 'u':		/* union */
			case 'f':		/* function */
			case 'v':		/* variable */
			case 't':		/* typedef */
			case 'e':		/* enumerator */
				/* switch to negative! i.e. schedule to put it under the
				 * Vars/Others node 
				 */
				parent_symbol_id = -parent_symbol_id;
				break;
			default:		/* let it as it is */
				break;
		}

		
		/* do we already have that parent_symbol displayed in gtktreeview? 
		 * If that's the case add it as children.
		 */
		node = g_tree_lookup (priv->nodes_displayed, GINT_TO_POINTER (parent_symbol_id));
		
		if (node != NULL) 
		{
			/* hey we found it */
			GtkTreeRowReference *child_row_ref;
			child_row_ref = do_add_child_symbol_to_view (dbv, parent_symbol_id,
				   pixbuf, symbol_name, symbol_id);
			
			/* add the children_path to the GTree. */
			g_tree_insert (priv->nodes_displayed, GINT_TO_POINTER (symbol_id), 
						   child_row_ref);
			trigger_on_symbol_inserted (dbv, symbol_id);
		}
		else 
		{
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
	
	/*DEBUG_PRINT ("on_symbol_inserted -global- %d", symbol_id);*/
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)));

	parent_symbol_id = symbol_db_engine_get_parent_scope_id_by_symbol_id (dbe, 
																	symbol_id,
																	NULL);
	
	/*DEBUG_PRINT ("on_symbol_inserted parent_symbol_id detected %d", parent_symbol_id);*/
	
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
		const gchar* symbol_access;
		SymbolDBEngineIterator *iterator_for_children;
	
		iter_node = SYMBOL_DB_ENGINE_ITERATOR_NODE (iterator);
		
		/* check if what we want to add is on a global_scope and not only a local 
		 * file scope e.g. static functions
		 */
		if (symbol_db_engine_iterator_node_get_symbol_is_file_scope (iter_node) == TRUE)
		{
/*			DEBUG_PRINT ("on_symbol_inserted()  -global- symbol %d is not global scope", 
						 symbol_id);*/
			g_object_unref (iterator);
			return;
		}
		
		symbol_kind = symbol_db_engine_iterator_node_get_symbol_extra_string (
							iter_node, SYMINFO_KIND);

		symbol_access = symbol_db_engine_iterator_node_get_symbol_extra_string (
							iter_node, SYMINFO_ACCESS);
		
		pixbuf = symbol_db_util_get_pixbuf (symbol_kind, symbol_access);
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
				SymbolDBEngineIteratorNode *iter_node;

				iter_node = SYMBOL_DB_ENGINE_ITERATOR_NODE (iterator_for_children);

				curr_child_id = 
					symbol_db_engine_iterator_node_get_symbol_id (iter_node);

				row_ref = g_tree_lookup (priv->nodes_displayed,
										 GINT_TO_POINTER (curr_child_id));
				if (row_ref == NULL)
					continue;

				if (sdb_view_get_iter_from_row_ref (dbv, row_ref, &child_iter) == FALSE)
				{
					/* no node displayed found */
					g_warning ("on_symbol_inserted (): row_ref something went wrong ?!");
					continue;
				}
				
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
					 
	while (gtk_tree_model_iter_has_child  (GTK_TREE_MODEL (store), parent_subtree_iter)) 
	{
		GtkTreeIter child;
		gtk_tree_model_iter_children (GTK_TREE_MODEL (store), &child, parent_subtree_iter);
		
		/* recurse */
		do_recurse_subtree_and_remove (dbv, &child);
	}

	gtk_tree_store_remove (store, parent_subtree_iter);
	g_tree_remove (priv->nodes_displayed, GINT_TO_POINTER (curr_symbol_id));

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
	
	dbv = SYMBOL_DB_VIEW (data);

	g_return_if_fail (dbv != NULL);
	priv = dbv->priv;

	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)));

	/*DEBUG_PRINT ("on_symbol_removed (): -global- %d", symbol_id);*/

	row_ref = g_tree_lookup (priv->nodes_displayed, GINT_TO_POINTER (symbol_id));
	if (sdb_view_get_iter_from_row_ref (dbv, row_ref, &iter) == FALSE)
	{
		return;
	}
	
	do_recurse_subtree_and_remove (dbv, &iter);
}

/**
 * Add at most ONE dummy child to the parent_iter. This is done to let the parent_iter
 * node be expandable.
 * @param force If true a dummy symbol will be added even if it has no children on db.
 */
static void
sdb_view_do_add_hidden_dummy_child (SymbolDBView *dbv, SymbolDBEngine *dbe,
						GtkTreeIter *parent_iter,  gint parent_symbol_id,
						gboolean force)
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
	
	if (child_iterator != NULL || force == TRUE)
	{
		/* hey we have something here... */
		GtkTreeIter child_iter;
		
		gtk_tree_store_append (store, &child_iter, parent_iter);			
		gtk_tree_store_set (store, &child_iter,
					COLUMN_PIXBUF, NULL,
					COLUMN_NAME, _("Loading..."),
					COLUMN_SYMBOL_ID, DUMMY_SYMBOL_ID,
					-1);

		if (child_iterator)
			g_object_unref (child_iterator);
	}	
}

static void
sdb_view_row_expanded_idle_destroy (gpointer data)
{
	NodeIdleExpand *node_expand;
	SymbolDBView *dbv;
	SymbolDBEngine *dbe;
	
	g_return_if_fail (data != NULL);
	node_expand = data;	
	DEBUG_PRINT ("%s", "sdb_view_global_row_expanded_idle_destroy ()");
	dbv = node_expand->dbv;
	dbe = node_expand->dbe;
	
	/* remove from the GTree the ids of the func expanding */
	g_tree_remove (dbv->priv->expanding_gfunc_ids, 
				   GINT_TO_POINTER (node_expand->expanded_symbol_id));
	
	if (node_expand->expanded_path != NULL) {
		gtk_tree_path_free (node_expand->expanded_path);
		node_expand->expanded_path = NULL;
	}
	g_object_unref (node_expand->iterator);
	node_expand->iterator = NULL;
	g_free (node_expand);
}

static gboolean
sdb_view_row_expanded_idle (gpointer data)
{
	NodeIdleExpand *node_expand;
	SymbolDBView *dbv;
	SymbolDBEngine *dbe;
	SymbolDBEngineIterator *iterator;
	const GdkPixbuf *pixbuf;
	const gchar* symbol_name;
	const gchar* symbol_kind;
	const gchar* symbol_access;
	GtkTreeIter iter;
	gint curr_symbol_id;
	SymbolDBEngineIteratorNode *iter_node;
	gpointer node;
	GtkTreeRowReference *curr_tree_row_ref;
	SymbolDBViewPriv *priv;
	
	node_expand = data;	
	
	dbv = node_expand->dbv;
	iterator = node_expand->iterator;
	dbe = node_expand->dbe;	
	priv = dbv->priv;

	if (iterator == NULL)
		return FALSE;
	
	iter_node = SYMBOL_DB_ENGINE_ITERATOR_NODE (iterator);			
	curr_symbol_id = symbol_db_engine_iterator_node_get_symbol_id (iter_node);
	node = g_tree_lookup (priv->nodes_displayed, GINT_TO_POINTER (curr_symbol_id));
		
	if (node != NULL) 
	{
		/* already displayed */
		return symbol_db_engine_iterator_move_next (iterator);
	}			
			
	symbol_name = symbol_db_engine_iterator_node_get_symbol_name (iter_node);
	symbol_kind = symbol_db_engine_iterator_node_get_symbol_extra_string (iter_node, 
														SYMINFO_KIND);			
	symbol_access = symbol_db_engine_iterator_node_get_symbol_extra_string (iter_node, 
														SYMINFO_ACCESS);						
	pixbuf = symbol_db_util_get_pixbuf (symbol_kind, symbol_access);
	
	curr_tree_row_ref = do_add_child_symbol_to_view (dbv, 
										node_expand->expanded_symbol_id, pixbuf, 
										symbol_name, curr_symbol_id);
	if (curr_tree_row_ref == NULL)
	{
		return symbol_db_engine_iterator_move_next (iterator);
	}		
		
	/* we'll fake the gpointer to store an int */
	g_tree_insert (priv->nodes_displayed, GINT_TO_POINTER (curr_symbol_id), 
				   curr_tree_row_ref);			
			
	sdb_view_get_iter_from_row_ref (dbv, curr_tree_row_ref, &iter);
			
	/* check for children about this node... */
	sdb_view_do_add_hidden_dummy_child (dbv, dbe, &iter, curr_symbol_id,
										FALSE);
	
	if (node_expand->expanded_path != NULL)
	{		
		gtk_tree_view_expand_row (GTK_TREE_VIEW (dbv),
								  node_expand->expanded_path,
								  FALSE);
		gtk_tree_path_free (node_expand->expanded_path);
		node_expand->expanded_path = NULL;
	}

	if (symbol_db_engine_iterator_move_next (iterator) == TRUE)
	{
		return TRUE;
	}
	else 
	{
		if (g_tree_lookup (priv->nodes_displayed, 
						   GINT_TO_POINTER (-node_expand->expanded_symbol_id)) == NULL)
		{
			GtkTreeRowReference *others_row_ref;
			GtkTreeIter others_dummy_node;
			others_row_ref = do_add_child_symbol_to_view (dbv, 
								node_expand->expanded_symbol_id, 
								symbol_db_util_get_pixbuf ("vars", "others"), 
								"Vars/Others", 
								-node_expand->expanded_symbol_id);
				
			/* insert a negative node ... */
			g_tree_insert (priv->nodes_displayed, 
							   	GINT_TO_POINTER (-node_expand->expanded_symbol_id), 
					   			others_row_ref);		
			
			/* ... and a dummy child */			
			sdb_view_get_iter_from_row_ref (dbv, others_row_ref, &others_dummy_node);
			
			sdb_view_do_add_hidden_dummy_child (dbv, dbe, &others_dummy_node, 0,
													TRUE);
		}
		
		return FALSE;
	}
}

static void
sdb_view_namespace_row_expanded (SymbolDBView *dbv, SymbolDBEngine *dbe, 
							 GtkTreeIter *expanded_iter, gint expanded_symbol_id) 
{
	SymbolDBViewPriv *priv;
	SymbolDBEngineIterator *iterator;	
	GtkTreeStore *store;
	GPtrArray *filter_array;
	gpointer node;
	
	g_return_if_fail (dbv != NULL);
	priv = dbv->priv;

	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)));
	
	DEBUG_PRINT ("%s", "sdb_view_namespace_row_expanded ");

	/* check if there's another expanding idle_func running */
	node = g_tree_lookup (priv->expanding_gfunc_ids, GINT_TO_POINTER (expanded_symbol_id));
	if (node != NULL)
	{
		return;
	}
	
	filter_array = g_ptr_array_new ();
	g_ptr_array_add (filter_array, "class");
	g_ptr_array_add (filter_array, "struct");	
		
	/* get results from database */
	iterator = symbol_db_engine_get_scope_members_by_symbol_id_filtered (dbe, 
									expanded_symbol_id, 
									filter_array,
									TRUE,
									-1,
									-1,
									SYMINFO_SIMPLE| 
									SYMINFO_KIND| 
									SYMINFO_ACCESS
									);

	g_ptr_array_free (filter_array, TRUE);

	if (iterator != NULL)
	{
		NodeIdleExpand *node_expand;
		gint idle_id;
		
		node_expand = g_new0 (NodeIdleExpand, 1);
			
		node_expand->dbv = dbv;
		node_expand->iterator = iterator;
		node_expand->dbe = dbe;
		node_expand->expanded_path =  
			gtk_tree_model_get_path (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)),
  	                                      	expanded_iter);
		node_expand->expanded_symbol_id = expanded_symbol_id;
		
		/* be sure that the expanding process doesn't freeze the gui */
		idle_id = g_idle_add_full (G_PRIORITY_LOW, 
						 (GSourceFunc) sdb_view_row_expanded_idle,
						 (gpointer) node_expand,
						 (GDestroyNotify) sdb_view_row_expanded_idle_destroy);		
		
		/* insert the idle_id into a g_tree */
		g_tree_insert (priv->expanding_gfunc_ids, GINT_TO_POINTER (expanded_symbol_id), 
					   GINT_TO_POINTER (idle_id));
	}
}


static void
sdb_view_global_row_expanded (SymbolDBView *dbv, SymbolDBEngine *dbe, 
							 GtkTreeIter *expanded_iter, gint expanded_symbol_id) 
{
	GtkTreeStore *store;
	SymbolDBViewPriv *priv;
	SymbolDBEngineIterator *iterator;
	GPtrArray *filter_array;
	gpointer node;
	g_return_if_fail (dbv != NULL);
	
	priv = dbv->priv;
	
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)));

	DEBUG_PRINT ("%s", "sdb_view_global_row_expanded ()");
	
	/* check if there's another expanding idle_func running */
	node = g_tree_lookup (priv->expanding_gfunc_ids, GINT_TO_POINTER (expanded_symbol_id));
	if (node != NULL)
	{
		return;
	}

	filter_array = g_ptr_array_new ();
	g_ptr_array_add (filter_array, "class");
	g_ptr_array_add (filter_array, "struct");
	
	/* check for the presence of namespaces. 
	 * If that's the case then populate the root with a 'Global' node.
	 */
	iterator = symbol_db_engine_get_global_members_filtered (dbe, filter_array, 
													TRUE, 
													TRUE, 
													-1,
													-1,
													SYMINFO_SIMPLE |
												  	SYMINFO_ACCESS |
													SYMINFO_KIND);
	g_ptr_array_free (filter_array, TRUE);
	
	if (iterator != NULL)
	{	
		NodeIdleExpand *node_expand;
		gint idle_id;
		
		node_expand = g_new0 (NodeIdleExpand, 1);
		
		node_expand->dbv = dbv;
		node_expand->iterator = iterator;
		node_expand->dbe = dbe;
		node_expand->expanded_path =  gtk_tree_model_get_path (
								gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)),
  	                            expanded_iter);
		node_expand->expanded_symbol_id = expanded_symbol_id;
		
		idle_id = g_idle_add_full (G_PRIORITY_LOW, 
						 (GSourceFunc) sdb_view_row_expanded_idle,
						 (gpointer) node_expand,
						 (GDestroyNotify) sdb_view_row_expanded_idle_destroy);
		
		/* insert the idle_id into a g_tree for (eventually) a later retrieval */
		DEBUG_PRINT ("Inserting into g_tree expanded_symbol_id %d and idle_id %d", 
					 expanded_symbol_id, idle_id);
		g_tree_insert (priv->expanding_gfunc_ids, GINT_TO_POINTER (expanded_symbol_id), 
					   GINT_TO_POINTER (idle_id));
	}	
}

static void
sdb_view_vars_row_expanded (SymbolDBView *dbv, SymbolDBEngine *dbe, 
							 GtkTreeIter *expanded_iter, gint expanded_symbol_id) 
{	
	SymbolDBViewPriv *priv;
	SymbolDBEngineIterator *iterator;
	GtkTreeStore *store;
	GPtrArray *filter_array;	
	gint positive_symbol_expanded;	
	gpointer node;
	
	g_return_if_fail (dbv != NULL);
	priv = dbv->priv;

	positive_symbol_expanded = -expanded_symbol_id;
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)));
	
	DEBUG_PRINT ("%s", "sdb_view_vars_row_expanded ()");

	/* check if there's another expanding idle_func running */
	node = g_tree_lookup (priv->expanding_gfunc_ids, GINT_TO_POINTER (expanded_symbol_id));
	if (node != NULL)
	{
		return;
	}
	
	filter_array = g_ptr_array_new ();
	g_ptr_array_add (filter_array, "class");
	g_ptr_array_add (filter_array, "struct");	
		
	if (positive_symbol_expanded == ROOT_GLOBAL)
	{
		iterator = symbol_db_engine_get_global_members_filtered (dbe, filter_array, 
													FALSE, 
													TRUE, 
													-1,
													-1,
													SYMINFO_SIMPLE |
												  	SYMINFO_ACCESS |
													SYMINFO_KIND);
	}
	else 
	{
		iterator = symbol_db_engine_get_scope_members_by_symbol_id_filtered (dbe, 
									positive_symbol_expanded, 
									filter_array,
									FALSE,
									-1,
									-1,
									SYMINFO_SIMPLE|
									SYMINFO_KIND|
									SYMINFO_ACCESS
									);
	}
	
	g_ptr_array_free (filter_array, TRUE);

	if (iterator != NULL)
	{		
		NodeIdleExpand *node_expand;
		gint idle_id;
		
		node_expand = g_new0 (NodeIdleExpand, 1);
		
		node_expand->dbv = dbv;
		node_expand->iterator = iterator;
		node_expand->dbe = dbe;
		node_expand->expanded_path = gtk_tree_model_get_path (
									gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)),
  	                                      	expanded_iter);
		node_expand->expanded_symbol_id = expanded_symbol_id;
		
		idle_id = g_idle_add_full (G_PRIORITY_LOW, 
						 (GSourceFunc) sdb_view_row_expanded_idle,
						 (gpointer) node_expand,
						 (GDestroyNotify) sdb_view_row_expanded_idle_destroy);		
		
		/* insert the idle_id into a g_tree */
		g_tree_insert (priv->expanding_gfunc_ids, GINT_TO_POINTER (expanded_symbol_id), 
					   GINT_TO_POINTER (idle_id));
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
	
	/* Ok. Is the expanded node a 'namespace' one? A 'global' one? Or a 
	 * 'vars/etc' one? parse here the cases and if we're not there
	 * go on with a classic expanding node algo.
	 */

	/* Case Global */
	if (expanded_symbol_id == ROOT_GLOBAL)
	{
		sdb_view_global_row_expanded (dbv, dbe, expanded_iter, expanded_symbol_id);
		return; 
	}
	
	/* Case vars/etc */
	/* To identify a vars/etc node we'll check if the expanded_symbol_id is negative.
	 * If yes, we can have the parent namespace by making the id positive.
	 */
	if (expanded_symbol_id < 0)
	{
		sdb_view_vars_row_expanded  (dbv, dbe, expanded_iter, expanded_symbol_id);
		return;
	}
	
	
	/* Case namespace */
	iterator = symbol_db_engine_get_symbol_info_by_id (dbe, expanded_symbol_id, 
													   SYMINFO_KIND);
	if (iterator != NULL) 
	{
		SymbolDBEngineIteratorNode *iter_node;
		const gchar* symbol_kind;
	
		iter_node = SYMBOL_DB_ENGINE_ITERATOR_NODE (iterator);
		symbol_kind = symbol_db_engine_iterator_node_get_symbol_extra_string (
							iter_node, SYMINFO_KIND);
		if (g_strcmp0 (symbol_kind, "namespace") == 0)
		{
			sdb_view_namespace_row_expanded (dbv, dbe, expanded_iter, 
											 expanded_symbol_id);
			g_object_unref (iterator);
			return;
		}
		g_object_unref (iterator);
	}
	
	/* Case Normal: go on with usual expanding */	
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
			node = g_tree_lookup (priv->nodes_displayed, GINT_TO_POINTER (curr_symbol_id));
		
			if (node != NULL) 
			{
				continue;
			}
			
			/* Step 3 */
			/* ok we must display this symbol */			
			pixbuf = symbol_db_util_get_pixbuf (
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
			g_tree_insert (priv->nodes_displayed, GINT_TO_POINTER (curr_symbol_id), 
						   child_row_ref);
			
			/* good. Let's check now for a child (B) of the just appended child (A). 
			 * Adding B (a dummy one for now) to A will make A expandable
			 */
			sdb_view_do_add_hidden_dummy_child (dbv, dbe, &child_iter, curr_symbol_id,
												FALSE);
			
		} while (symbol_db_engine_iterator_move_next (iterator) == TRUE);
		
		g_object_unref (iterator);
	}
	
	/* force expand it */
	path =  gtk_tree_model_get_path (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)),
  	                                      	expanded_iter);
	gtk_tree_view_expand_row (GTK_TREE_VIEW (dbv), path, FALSE);
	gtk_tree_path_free (path);	
}

void
symbol_db_view_row_collapsed (SymbolDBView *dbv, SymbolDBEngine *dbe, 
							 GtkTreeIter *collapsed_iter)
{
	GtkTreeStore *store;
	gint collapsed_symbol_id;
	SymbolDBViewPriv *priv;
	gpointer node;

	g_return_if_fail (dbv != NULL);
	priv = dbv->priv;

	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)));
	
	gtk_tree_model_get (GTK_TREE_MODEL (store), collapsed_iter,
						COLUMN_SYMBOL_ID, &collapsed_symbol_id, -1);
	
	/* do a quick check to see if the collapsed_symbol_id is listed into the
	 * currently expanding symbols. If that's the case remove the gsource func.
	 */
	node = g_tree_lookup (priv->expanding_gfunc_ids, GINT_TO_POINTER (collapsed_symbol_id));
	
	if (node == NULL)
	{
		/* no expanding gfunc found. */
		return;
	}
	else {		
		g_source_remove (GPOINTER_TO_INT(node));
		g_tree_remove (priv->expanding_gfunc_ids, GINT_TO_POINTER (collapsed_symbol_id));
	}	
}

static GtkTreeStore *
sdb_view_create_new_store ()
{
	GtkTreeStore *store;
	store = gtk_tree_store_new (COLUMN_MAX, GDK_TYPE_PIXBUF,
				    G_TYPE_STRING, G_TYPE_INT);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
										  COLUMN_NAME,
										  GTK_SORT_ASCENDING);
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
	
	priv->row_ref_global = NULL;
	priv->scan_end_handler = 0;
	priv->remove_handler = 0;
	priv->scan_end_handler = 0;	

	/* create a GTree where to store the row_expanding ids of the gfuncs.
	 * we would be able then to g_source_remove them on row_collapsed
	 */
	priv->expanding_gfunc_ids = g_tree_new_full ((GCompareDataFunc)&symbol_db_gtree_compare_func, 
										 NULL, NULL, NULL);
	
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

	DEBUG_PRINT ("%s", "finalizing symbol_db_view ()");
	
	symbol_db_view_clear_cache (view);
	
	if (priv->expanding_gfunc_ids)
		g_tree_destroy (priv->expanding_gfunc_ids);
	
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
		const gchar* file;
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
			file = symbol_db_engine_iterator_node_get_symbol_extra_string (node,
															SYMINFO_FILE_PATH);
			*OUT_file = g_strdup (file);
			return TRUE;
		}		
	}
	
	return FALSE;
}

static void
sdb_view_build_and_display_base_tree (SymbolDBView *dbv, SymbolDBEngine *dbe)
{
	GtkTreeStore *store;
	SymbolDBViewPriv *priv;
	SymbolDBEngineIterator *iterator;
	gboolean we_have_namespaces;
	GPtrArray *filter_array;
	GtkTreeRowReference *global_tree_row_ref;
	GtkTreeIter global_child_iter;
	const GdkPixbuf *global_pixbuf;
	
	g_return_if_fail (dbv != NULL);
	
	priv = dbv->priv;
	
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)));

	we_have_namespaces = FALSE;

	filter_array = g_ptr_array_new ();
	g_ptr_array_add (filter_array, "namespace");
	
	/* check for the presence of namespaces. 
	 * If that's the case then populate the root with a 'Global' node.
	 */
	iterator = symbol_db_engine_get_global_members_filtered (dbe, filter_array, TRUE, 
													TRUE, 
													-1,
													-1,
													SYMINFO_SIMPLE |
												  	SYMINFO_ACCESS |
													SYMINFO_KIND);
	g_ptr_array_free (filter_array, TRUE);
	
	if (iterator != NULL)
	{
		we_have_namespaces = TRUE;
		
		do {
			gint curr_symbol_id;
			SymbolDBEngineIteratorNode *iter_node;
			const GdkPixbuf *pixbuf;
			const gchar* symbol_name;			
			GtkTreeRowReference *curr_tree_row_ref;
			GtkTreeIter child_iter;

			iter_node = SYMBOL_DB_ENGINE_ITERATOR_NODE (iterator);

			curr_symbol_id = symbol_db_engine_iterator_node_get_symbol_id (iter_node);
			
			pixbuf = symbol_db_util_get_pixbuf (
						symbol_db_engine_iterator_node_get_symbol_extra_string (
							iter_node, SYMINFO_KIND),
						symbol_db_engine_iterator_node_get_symbol_extra_string (
							iter_node, SYMINFO_ACCESS));

			symbol_name = symbol_db_engine_iterator_node_get_symbol_name (iter_node);

			curr_tree_row_ref = do_add_root_symbol_to_view (dbv, pixbuf, 
												symbol_name, curr_symbol_id);
			if (curr_tree_row_ref == NULL)
			{
				continue;				
			}		
		
			/* we'll fake the gpointer to store an int */
			g_tree_insert (priv->nodes_displayed, GINT_TO_POINTER (curr_symbol_id), 
						   curr_tree_row_ref);

			if (sdb_view_get_iter_from_row_ref (dbv, curr_tree_row_ref, 
											&child_iter) == FALSE)
			{
				g_warning ("sdb_view_build_and_display_base_tree (): something "
						   "went wrong");
				continue;
			} 
			/* add a dummy child */
			sdb_view_do_add_hidden_dummy_child (dbv, dbe,
						&child_iter, curr_symbol_id, FALSE);
			
		} while (symbol_db_engine_iterator_move_next (iterator) == TRUE);
		
		g_object_unref (iterator);		
	}

	/*
	 * Good. Add a 'Global' node to the store. 
	 */
	global_pixbuf = symbol_db_util_get_pixbuf ("global", "global");
	
	global_tree_row_ref = do_add_root_symbol_to_view (dbv, global_pixbuf, 
											"Global", ROOT_GLOBAL);
		
	if (global_tree_row_ref == NULL)
	{
		return;
	}		
	g_tree_insert (priv->nodes_displayed, GINT_TO_POINTER (ROOT_GLOBAL), 
					   global_tree_row_ref);

	if (sdb_view_get_iter_from_row_ref (dbv, global_tree_row_ref, 
										&global_child_iter) == FALSE)
	{
		g_warning ("sdb_view_build_and_display_base_tree (): cannot retrieve iter for "
				   "row_ref");
		return;
	}

	/* add a dummy child */
	sdb_view_do_add_hidden_dummy_child (dbv, dbe,
				&global_child_iter, ROOT_GLOBAL, TRUE);
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
		gtk_widget_set_sensitive (GTK_WIDGET (dbv), TRUE);
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
		gtk_widget_set_sensitive (GTK_WIDGET (dbv), FALSE);
		
		if (priv->insert_handler > 0) 
		{
			g_signal_handler_disconnect (G_OBJECT (dbe), priv->insert_handler);
			priv->insert_handler = 0;
		}

		if (priv->remove_handler > 0)
		{
			g_signal_handler_disconnect (G_OBJECT (dbe), priv->remove_handler);
			priv->remove_handler = 0;
		}	

		if (priv->scan_end_handler > 0)
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

	/*DEBUG_PRINT ("%s", "symbol_db_view_open ()");*/
	symbol_db_view_clear_cache (dbv);
	
	store = sdb_view_create_new_store ();
	gtk_tree_view_set_model (GTK_TREE_VIEW (dbv), GTK_TREE_MODEL (store));
	
	priv->nodes_displayed = g_tree_new_full ((GCompareDataFunc)&symbol_db_gtree_compare_func, 
										 NULL,
										 NULL,
										 (GDestroyNotify)&gtk_tree_row_reference_free);

	priv->waiting_for = g_tree_new_full ((GCompareDataFunc)&symbol_db_gtree_compare_func, 
									 NULL,
									 NULL,
									 NULL);
	
	sdb_view_build_and_display_base_tree (dbv, dbe);
}
