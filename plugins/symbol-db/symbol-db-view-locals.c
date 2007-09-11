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
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>
#include <libegg/menu/egg-combo-action.h>

#include "symbol-db-view-locals.h"
#include "symbol-db-engine.h"
#include "symbol-db-engine-iterator.h"

enum {
	COLUMN_PIXBUF,
	COLUMN_NAME,
	COLUMN_LINE,
	COLUMN_SYMBOL_ID,
	COLUMN_MAX
};

static GtkTreeViewClass *parent_class = NULL;

struct _SymbolDBViewLocalsPriv
{
	gchar *current_file;
	gint insert_handler;
	gint update_handler;
	gint remove_handler;
};

static void
sdb_view_locals_init (SymbolDBViewLocals *dbvl)
{
	DEBUG_PRINT ("sdb_view_locals_init  ()");
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
	GtkTreeStore *store;
	SymbolDBViewLocalsPriv *priv;
	
	g_return_if_fail (dbvl != NULL);
	
	dbvl->priv = g_new0 (SymbolDBViewLocalsPriv, 1);		
	priv = dbvl->priv;
	
	priv->current_file = NULL;
	priv->insert_handler = 0;
	priv->update_handler = 0;
	priv->remove_handler = 0;
	
	store = gtk_tree_store_new (COLUMN_MAX, GDK_TYPE_PIXBUF,
				    G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);

	gtk_tree_view_set_model (GTK_TREE_VIEW (dbvl), GTK_TREE_MODEL (store));	
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (dbvl), FALSE);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dbvl));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	/* search through the tree interactively */
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW (dbvl), TRUE);
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (dbvl), COLUMN_NAME);
	
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

	gtk_tree_view_append_column (GTK_TREE_VIEW (dbvl), column);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (dbvl), column);	
}

static void
sdb_view_locals_finalize (GObject *object)
{
	SymbolDBViewLocals *locals = SYMBOL_DB_VIEW_LOCALS (object);
	SymbolDBViewLocalsPriv *priv = locals->priv;

	DEBUG_PRINT ("finalizing symbol_db_view_locals ()");	
	
	
	if (priv->current_file)
		g_free (priv->current_file);

	g_free (priv);
	
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
sdb_view_locals_class_init (SymbolDBViewLocalsClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = sdb_view_locals_finalize;
}

GType
symbol_db_view_locals_get_type (void) 
{
	static GType our_type = 0;

	if(our_type == 0)
	{
		static const GTypeInfo our_info =
		{
			sizeof (SymbolDBViewLocalsClass), /* class_size */
			(GBaseInitFunc) NULL, /* base_init */
			(GBaseFinalizeFunc) NULL, /* base_finalize */
			(GClassInitFunc) sdb_view_locals_class_init, /* class_init */
			(GClassFinalizeFunc) NULL, /* class_finalize */
			NULL /* class_data */,
			sizeof (SymbolDBViewLocals), /* instance_size */
			0, /* n_preallocs */
			(GInstanceInitFunc) sdb_view_locals_init, /* instance_init */
			NULL /* value_table */
		};

		our_type = g_type_register_static (GTK_TYPE_TREE_VIEW, "SymbolDBViewLocals",
		                                   &our_info, 0);
	}

	return our_type;
}

GtkWidget *
symbol_db_view_locals_new (void)
{
	return GTK_WIDGET (g_object_new (SYMBOL_TYPE_DB_VIEW_LOCALS, NULL));
}


static void 
on_symbol_removed (SymbolDBEngine *dbe, gint symbol_id, gpointer data)
{
	SymbolDBEngineIterator *iterator;
	GtkTreeStore *store;
	gint i;	
	SymbolDBViewLocals *dbvl;
	SymbolDBViewLocalsPriv *priv;
    GtkTreeIter  iter;	
	
	dbvl = SYMBOL_DB_VIEW_LOCALS (data);

	g_return_if_fail (dbvl != NULL);
	
	priv = dbvl->priv;

	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbvl)));		

	DEBUG_PRINT ("on_symbol_removed -local- %d!!!!!", symbol_id);		

	
	/* Make sure the model stays with us after the tree view unrefs it */
	g_object_ref(store); 
	/* Detach model from view */
	gtk_tree_view_set_model(GTK_TREE_VIEW(dbvl), NULL); 
	
	
    /* NULL means the parent is the virtual root node, so the
     *  n-th top-level element is returned in iter, which is
     *  the n-th row in a list store (as a list store only has
     *  top-level elements, and no children) */
	for (i=0; i < gtk_tree_model_iter_n_children (GTK_TREE_MODEL(store), NULL); i++)
	{
	    if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, i))
    	{
			gint sym_id;
			gtk_tree_model_get (GTK_TREE_MODEL (store),
						&iter, COLUMN_SYMBOL_ID, &sym_id, -1);
 
			if (sym_id == symbol_id) {
				DEBUG_PRINT ("removing this -local- %d!", sym_id);
	    		gtk_tree_store_remove (store, &iter);
			}
		}
	}
	
	/* Re-attach model to view */
	gtk_tree_view_set_model (GTK_TREE_VIEW (dbvl), GTK_TREE_MODEL (store)); 
	g_object_unref(store);
	
}

static void 
on_symbol_updated (SymbolDBEngine *dbe, gint symbol_id, gpointer data)
{
	SymbolDBEngineIterator *iterator;
	GtkTreeStore *store;
	gint i;	
	SymbolDBViewLocals *dbvl;
	SymbolDBViewLocalsPriv *priv;
	
	dbvl = SYMBOL_DB_VIEW_LOCALS (data);

	g_return_if_fail (dbvl != NULL);	
	priv = dbvl->priv;
	
	
	DEBUG_PRINT ("on_symbol_updated !!!!!");	
}

static void 
on_symbol_inserted (SymbolDBEngine *dbe, gint symbol_id, gpointer data)
{
	SymbolDBEngineIterator *iterator;
	GtkTreeStore *store;
	gint i;	
	SymbolDBViewLocals *dbvl;
	SymbolDBViewLocalsPriv *priv;
	
	dbvl = SYMBOL_DB_VIEW_LOCALS (data);

	g_return_if_fail (dbvl != NULL);
	
	priv = dbvl->priv;	
	
	DEBUG_PRINT ("on_symbol_inserted -local- !!!!! %d", symbol_id);
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbvl)));	
	
	/* try to filter out the symbols inserted that don't belong to
	 * the current viewed file
	 */
	iterator = symbol_db_engine_get_symbol_info_by_id (dbe, symbol_id, 
													   SYMINFO_SIMPLE |
													   SYMINFO_FILE_PATH |
													   SYMINFO_ACCESS |
													   SYMINFO_KIND);	
	
	if (iterator != NULL) 
	{
		gchar *tmp = symbol_db_engine_get_full_local_path (dbe, 
						symbol_db_engine_iterator_get_symbol_extra_string (iterator,
															SYMINFO_FILE_PATH));
		/* it's not our case, just bail out. */
		if (strcmp (tmp, priv->current_file) != 0) 
		{
			g_free (tmp);
			g_object_unref (iterator);
			return;
		}
  
		/* Make sure the model stays with us after the tree view unrefs it */
		g_object_ref(store); 
		/* Detach model from view */
		gtk_tree_view_set_model(GTK_TREE_VIEW(dbvl), NULL); 

  		/* ... insert a couple of thousand rows ...*/		
		for (i=0; i < symbol_db_engine_iterator_get_n_items (iterator); i++) 
		{
			GtkTreeIter iter;
			gtk_tree_store_append (store, &iter, NULL);
			gtk_tree_store_set (store, &iter,
				COLUMN_PIXBUF, symbol_db_view_get_pixbuf (
						symbol_db_engine_iterator_get_symbol_extra_string (
							iterator, SYMINFO_KIND),
						symbol_db_engine_iterator_get_symbol_extra_string (
							iterator, SYMINFO_ACCESS)),
				COLUMN_NAME, symbol_db_engine_iterator_get_symbol_name (iterator),
				COLUMN_LINE, symbol_db_engine_iterator_get_symbol_file_pos (iterator),
				COLUMN_SYMBOL_ID, symbol_db_engine_iterator_get_symbol_id (iterator),								
				-1);	
			
			DEBUG_PRINT ("inserted into locals tab: %s [%s]",
						 symbol_db_engine_iterator_get_symbol_name (iterator),
						 symbol_db_engine_iterator_get_symbol_extra_string (
							iterator, SYMINFO_KIND));	
			
			symbol_db_engine_iterator_move_next (iterator);
		}
		g_free (tmp);		
		g_object_unref (iterator);
		
		/* Re-attach model to view */
		gtk_tree_view_set_model (GTK_TREE_VIEW (dbvl), GTK_TREE_MODEL (store)); 
  		g_object_unref(store);
	}	
}

gint
symbol_db_view_locals_get_line (SymbolDBViewLocals *dbvl,
								GtkTreeIter * iter) 
{
	GtkTreeStore *store;
	
	g_return_if_fail (dbvl != NULL);
	g_return_if_fail (iter != NULL);
	
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbvl)));	
	
	if (store)
	{
		gint line;
		gtk_tree_model_get (GTK_TREE_MODEL
				    (store), iter,
				    COLUMN_LINE, &line, -1);
		return line;
	}
	return -1;
}								

void
symbol_db_view_locals_update_list (SymbolDBViewLocals *dbvl, SymbolDBEngine *dbe,
							  const gchar* filepath)
{
	SymbolDBViewLocalsPriv *priv;

	SymbolDBEngineIterator *iterator;
	GtkTreeStore *store;
	gint i;
	
	g_return_if_fail (dbvl != NULL);
	g_return_if_fail (filepath != NULL);
	g_return_if_fail (dbe != NULL);
		
	DEBUG_PRINT ("symbol_db_view_locals_update_list  () for %s", filepath);
	
	priv = dbvl->priv;
	if (priv->current_file)
		g_free (priv->current_file);
	priv->current_file = g_strdup (filepath);

	if (priv->insert_handler <= 0) 
	{
		priv->insert_handler = 	g_signal_connect (G_OBJECT (dbe), "symbol_inserted",
					  G_CALLBACK (on_symbol_inserted), dbvl);
	}
/*
	if (priv->update_handler <= 0)
	{
		
		priv->update_handler = g_signal_connect (G_OBJECT (dbe), "symbol_updated",
					  G_CALLBACK (on_symbol_updated), dbvl);
	}
*/	
	if (priv->remove_handler <= 0)
	{
		priv->remove_handler = g_signal_connect (G_OBJECT (dbe), "symbol_removed",
					  G_CALLBACK (on_symbol_removed), dbvl);
	}
	
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbvl)));	
	
	/* Removes all rows from tree_store */
	gtk_tree_store_clear (store);

	iterator = symbol_db_engine_get_file_symbols (dbe, filepath, SYMINFO_SIMPLE |
												  	SYMINFO_ACCESS |
													SYMINFO_KIND);	
	if (iterator != NULL) 
	{
		for (i=0; i < symbol_db_engine_iterator_get_n_items (iterator); i++) 
		{
			GtkTreeIter iter, child;
			gtk_tree_store_append (store, &iter, NULL);
			
			DEBUG_PRINT ("gonna add [id: %d] %s %s [%s]",
				symbol_db_engine_iterator_get_symbol_id (iterator),
				symbol_db_engine_iterator_get_symbol_name (iterator),
				symbol_db_engine_iterator_get_symbol_extra_string (
							iterator, SYMINFO_KIND),
				symbol_db_engine_iterator_get_symbol_extra_string (
							iterator, SYMINFO_ACCESS));
			
			gtk_tree_store_set (store, &iter,
				COLUMN_PIXBUF, symbol_db_view_get_pixbuf (
						symbol_db_engine_iterator_get_symbol_extra_string (
							iterator, SYMINFO_KIND),
						symbol_db_engine_iterator_get_symbol_extra_string (
							iterator, SYMINFO_ACCESS)),
				COLUMN_NAME, symbol_db_engine_iterator_get_symbol_name (iterator),
				COLUMN_LINE, symbol_db_engine_iterator_get_symbol_file_pos (iterator),
				COLUMN_SYMBOL_ID, symbol_db_engine_iterator_get_symbol_id (iterator),
				-1);
			
			symbol_db_engine_iterator_move_next (iterator);
		}
		g_object_unref (iterator);
	}
}

