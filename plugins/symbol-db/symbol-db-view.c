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


enum {
	COLUMN_PIXBUF,
	COLUMN_NAME,
	COLUMN_LINE,
	COLUMN_FILE,
	COLUMN_SYMBOL_ID,
	COLUMN_MAX
};

struct _SymbolDBViewPriv
{
	SymbolDBEngine *dbe;
	
	gint insert_handler;
	gint remove_handler;	
	gint update_handler;
};

static GtkTreeViewClass *parent_class = NULL;
static GHashTable *pixbufs_hash = NULL;


static void 
on_symbol_updated (SymbolDBEngine *dbe, 
					gint symbol_id, gpointer data)
{
	SymbolDBEngineIterator *iterator;
	GtkTreeStore *store;
	gint i;	
	gint parent_symbol_id;
	SymbolDBView *dbv;
	SymbolDBViewPriv *priv;
	
	dbv = SYMBOL_DB_VIEW (data);

	g_return_if_fail (dbv != NULL);
	
	priv = dbv->priv;
	
	DEBUG_PRINT ("on_symbol_updated -global- !!!!! %d", symbol_id);
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)));	
}

static gboolean
do_add_hidden_children (SymbolDBEngine *dbe, GtkTreeStore *store,
						GtkTreeIter *parent_iter,  gint parent_symbol_id)
{
	SymbolDBEngineIterator *child_iterator;
	child_iterator = symbol_db_engine_get_scope_members_by_symbol_id (dbe, 
						parent_symbol_id, SYMINFO_SIMPLE | SYMINFO_FILE_PATH | 
							SYMINFO_ACCESS | SYMINFO_KIND);
	if (child_iterator != NULL)
	{
		do {
			/* hey we have something here... */
			GtkTreeIter child_iter;
			gtk_tree_store_append (store, &child_iter, parent_iter);
			gchar *file_path;
			const gchar *sym_name;
	
			sym_name = 
				symbol_db_engine_iterator_get_symbol_name (child_iterator);
			
			/* DEBUG_PRINT ("child iterator for %s",  sym_name); */
			
			/* get the full file path instead of a database-oriented one. */
			file_path = 
				symbol_db_engine_get_full_local_path (dbe, 
					symbol_db_engine_iterator_get_symbol_extra_string (child_iterator,
											SYMINFO_FILE_PATH));
			
			gtk_tree_store_set (store, &child_iter,
					COLUMN_PIXBUF, symbol_db_view_get_pixbuf (
						symbol_db_engine_iterator_get_symbol_extra_string (
								child_iterator, SYMINFO_KIND),
						symbol_db_engine_iterator_get_symbol_extra_string (
								child_iterator, SYMINFO_ACCESS)
					),
					COLUMN_NAME, sym_name,
					COLUMN_LINE, 
						symbol_db_engine_iterator_get_symbol_file_pos (child_iterator),
					COLUMN_FILE, file_path,
					COLUMN_SYMBOL_ID, 
							symbol_db_engine_iterator_get_symbol_id (child_iterator),
					-1);
			
			g_free (file_path);
		} while (symbol_db_engine_iterator_move_next (child_iterator) == TRUE);
		g_object_unref (child_iterator);
	}
}


static gboolean
do_recurse_and_add_new_sym (gint parent_symbol_id, SymbolDBEngineIterator *iterator, 
					GtkTreeStore *store, GtkTreeIter *iter)
{
	gint sym_id;
	gboolean valid;

	gtk_tree_model_get (GTK_TREE_MODEL (store),
				iter, COLUMN_SYMBOL_ID, &sym_id, -1);

	if (sym_id == parent_symbol_id) {
		GtkTreeIter child_iter;
		
		gtk_tree_store_append (store, &child_iter, iter);
		gtk_tree_store_set (store, &child_iter,
			COLUMN_PIXBUF, symbol_db_view_get_pixbuf (
					symbol_db_engine_iterator_get_symbol_extra_string (
						iterator, SYMINFO_KIND),
					symbol_db_engine_iterator_get_symbol_extra_string (
						iterator, SYMINFO_ACCESS)),
			COLUMN_NAME, symbol_db_engine_iterator_get_symbol_name (iterator),
			COLUMN_LINE, symbol_db_engine_iterator_get_symbol_file_pos (iterator),
			COLUMN_SYMBOL_ID, symbol_db_engine_iterator_get_symbol_id (iterator),								
			-1);
			
		DEBUG_PRINT ("[do_recurse_and_add_new_sym] inserted into -global- tab: %s [%s]",
					 symbol_db_engine_iterator_get_symbol_name (iterator),
					 symbol_db_engine_iterator_get_symbol_extra_string (
						iterator, SYMINFO_KIND));	
		return TRUE;
	} 
	
	if (gtk_tree_model_iter_has_child  (GTK_TREE_MODEL (store), iter)) 
	{
		GtkTreeIter child;
		gtk_tree_model_iter_children (GTK_TREE_MODEL (store), &child, iter);
		
		DEBUG_PRINT ("[do_recurse_and_add_new_sym] recurse for child");
		if (do_recurse_and_add_new_sym (parent_symbol_id, iterator, store, &child) 
			== TRUE)
			return TRUE;
	}
	
	while ((valid = 
			gtk_tree_model_iter_next (GTK_TREE_MODEL (store), iter)) == TRUE)
	{
		if (do_recurse_and_add_new_sym (parent_symbol_id, iterator, store, iter)
			== TRUE)
			return TRUE;
	}
	
	return FALSE;
}


static void 
on_symbol_inserted (SymbolDBEngine *dbe, 
					gint symbol_id, gpointer data)
{
	SymbolDBEngineIterator *iterator;
	GtkTreeStore *store;
	gint i;	
	/* it's not obligatory referred to a class inheritance */
	gint parent_symbol_id;
	SymbolDBView *dbv;
	SymbolDBViewPriv *priv;
	
	dbv = SYMBOL_DB_VIEW (data);

	g_return_if_fail (dbv != NULL);
	
	priv = dbv->priv;
	
	DEBUG_PRINT ("on_symbol_inserted -global- !!!!! %d", symbol_id);
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)));

	/* again we use a little trick to insert symbols here. First of all forget chars
	 * and symbol names. They are too much cpu-intensive. We'll use symbol-ids instead.
	 *
	 * Suppose we have a symbol_id X to insert. Where should we put it into the
	 * gtktree? Well.. look at its parent! By knowing its parent we're able to
	 * know the right place where to store this child, being it on the root /
	 * or under some path. Please note this: the whole path isn't computed at once
	 * when the global gtk tree view is loaded, but it's incremental. So we can
	 * have a case where our symbol X has a parent Y, but that parent isn't already
	 * mapped into the gtktreestore: we'll just avoid to insert 'visually' the 
	 * symbol.	 
	 *
	 */
	parent_symbol_id = symbol_db_engine_get_parent_scope_id_by_symbol_id (dbe, 
																	symbol_id);
	
	DEBUG_PRINT ("parent_symbol_id %d", parent_symbol_id);
	
	/* get the original symbol infos */
	iterator = symbol_db_engine_get_symbol_info_by_id (dbe, symbol_id, 
													   SYMINFO_SIMPLE |
													   SYMINFO_FILE_PATH |
													   SYMINFO_ACCESS |
													   SYMINFO_KIND);	
	
	if (iterator != NULL) 
	{
		GtkTreeIter iter, child;
		
		/* add to root if parent_symbol_id is <= 0 */
		if (parent_symbol_id <= 0)
		{
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
			
			DEBUG_PRINT ("[on_symbol_inserted] inserted into -global- tab: %s [%s]",
						 symbol_db_engine_iterator_get_symbol_name (iterator),
						 symbol_db_engine_iterator_get_symbol_extra_string (
							iterator, SYMINFO_KIND));	
			
			do_add_hidden_children (dbe, store, &iter, 
							symbol_db_engine_iterator_get_symbol_id (iterator));
		}
		else 
		{
			gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);			
			do_recurse_and_add_new_sym (parent_symbol_id, iterator, store, &iter);
		}
		
		
		symbol_db_engine_iterator_move_next (iterator);
		g_object_unref (iterator);
	}	
}

static gboolean
do_recurse_and_remove (gint symbol_id, GtkTreeStore *store, GtkTreeIter *iter)
{
	gint sym_id;
	gboolean valid;

	gtk_tree_model_get (GTK_TREE_MODEL (store),
				iter, COLUMN_SYMBOL_ID, &sym_id, -1);

	if (sym_id == symbol_id) {
		DEBUG_PRINT ("removing -global- this %d!", sym_id);
   		gtk_tree_store_remove (store, iter);
		
		/* found and removed */
		return TRUE;
	} 
	
	if (gtk_tree_model_iter_has_child  (GTK_TREE_MODEL (store), iter)) 
	{
		GtkTreeIter child;
		gtk_tree_model_iter_children (GTK_TREE_MODEL (store), &child, iter);
		
		DEBUG_PRINT ("[do_recurse_and_remove] recurse for child");
		if (do_recurse_and_remove (symbol_id, store, &child) == TRUE)
			return TRUE;
	}
	
	while ((valid = 
			gtk_tree_model_iter_next (GTK_TREE_MODEL (store), iter)) == TRUE)
	{
		if (do_recurse_and_remove (symbol_id, store, iter)== TRUE)
			return TRUE;
	}
	
	return FALSE;
}

static void 
on_symbol_removed (SymbolDBEngine *dbe, gint symbol_id, gpointer data)
{
	SymbolDBEngineIterator *iterator;
	GtkTreeStore *store;
	gint i;	
	SymbolDBView *dbv;
	SymbolDBViewPriv *priv;
    GtkTreeIter  iter;	
	gboolean valid;
	
	dbv = SYMBOL_DB_VIEW (data);

	g_return_if_fail (dbv != NULL);
	
	priv = dbv->priv;

	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)));

	DEBUG_PRINT ("[on_symbol_removed] on_symbol_removed -global- %d!!!!!", 
				 symbol_id);

    /* NULL means the parent is the virtual root node, so the
     *  n-th top-level element is returned in iter, which is
     *  the n-th row in a list store (as a list store only has
     *  top-level elements, and no children) */
	valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);
	
	
	do_recurse_and_remove (symbol_id, store, &iter);
}

static void
on_symbol_db_view_row_expanded (GtkTreeView * view,
							 GtkTreeIter * iter, GtkTreePath *iter_path,
							 SymbolDBView *sdbv)
{
	GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (view));
	GtkTreeIter child;
	SymbolDBViewPriv *priv;
	
	g_return_if_fail (sdbv != NULL);
	priv = sdbv->priv;
	
	if (gtk_tree_model_iter_children (GTK_TREE_MODEL (store), &child, iter))
	{
		do
		{
			gint curr_symbol_id;
			/* Make sure the symbol children are not yet created */
			gtk_tree_model_get (GTK_TREE_MODEL (store), &child,
								COLUMN_SYMBOL_ID, &curr_symbol_id, -1);
		
			DEBUG_PRINT ("expanded %d", curr_symbol_id);

			/* cannot be a wrong symbol id */
			if (curr_symbol_id <=0 )
				return;

			/* hey if we've a node with some children that means that this path
			 * has already been parsed 
			 */
			if (gtk_tree_model_iter_has_child (GTK_TREE_MODEL (store), &child) == TRUE)
				return;
			
			/* ok you won. Let's have your children right now... */
			do_add_hidden_children (priv->dbe, store, &child, curr_symbol_id);
		}
		while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &child));

	}
	else 
	{
		/* Has no children */
		return;
	}
}

static void
sdb_view_init (SymbolDBView *object)
{
	SymbolDBView *dbv;
	GtkTreeStore *store;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
	
	dbv = SYMBOL_DB_VIEW (object);
	dbv->priv = g_new0 (SymbolDBViewPriv, 1);
	
	/* initialize some priv data */
	dbv->priv->insert_handler = 0;
	dbv->priv->remove_handler = 0;
	dbv->priv->update_handler = 0;
	

	/* Tree and his model */
	store = gtk_tree_store_new (COLUMN_MAX, GDK_TYPE_PIXBUF,
				    G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT);

	gtk_tree_view_set_model (GTK_TREE_VIEW (dbv), GTK_TREE_MODEL (store));
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (dbv), FALSE);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dbv));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	/* search through the tree interactively */
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (dbv), COLUMN_NAME);	
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW (dbv), TRUE);	

	g_signal_connect (G_OBJECT (dbv), "row_expanded",
			  G_CALLBACK (on_symbol_db_view_row_expanded), dbv);
#if 0	
	g_signal_connect (G_OBJECT (sv), "row_collapsed",
			  G_CALLBACK (on_symbol_view_row_collapsed), sv);
			  
	/* Tooltip signals */
	g_signal_connect (G_OBJECT (sv), "motion-notify-event",
					  G_CALLBACK (tooltip_motion_cb), sv);
	g_signal_connect (G_OBJECT (sv), "leave-notify-event",
					  G_CALLBACK (tooltip_leave_cb), sv);
#endif
	g_object_unref (G_OBJECT (store));

	
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

	CREATE_SYM_ICON ("none",              "Icons.16x16.Literal");
	CREATE_SYM_ICON ("namespace",         "Icons.16x16.NameSpace");
	CREATE_SYM_ICON ("class",             "Icons.16x16.Class");
	CREATE_SYM_ICON ("struct",            "Icons.16x16.ProtectedStruct");
	CREATE_SYM_ICON ("union",             "Icons.16x16.PrivateStruct");
	CREATE_SYM_ICON ("typedef",           "Icons.16x16.Reference");
	CREATE_SYM_ICON ("function",          "Icons.16x16.Method");
	CREATE_SYM_ICON ("variable",          "Icons.16x16.Literal");
	CREATE_SYM_ICON ("enumerator",     	  "Icons.16x16.Enum");
	CREATE_SYM_ICON ("macro",             "Icons.16x16.Field");
	CREATE_SYM_ICON ("privatemethod",     "Icons.16x16.PrivateMethod");
	CREATE_SYM_ICON ("privateproperty",   "Icons.16x16.PrivateProperty");
	CREATE_SYM_ICON ("protectedmethod",   "Icons.16x16.ProtectedMethod");
	CREATE_SYM_ICON ("protectedproperty", "Icons.16x16.ProtectedProperty");
	CREATE_SYM_ICON ("publicmember",      "Icons.16x16.InternalMethod");
	CREATE_SYM_ICON ("publicproperty",    "Icons.16x16.InternalProperty");

/*	
	sv_symbol_pixbufs[sv_cfolder_t] = gdl_icons_get_mime_icon (icon_set,
							    "application/directory-normal");
	sv_symbol_pixbufs[sv_ofolder_t] = gdl_icons_get_mime_icon (icon_set,
							    "application/directory-normal");
	sv_symbol_pixbufs[sv_max_t] = NULL;
*/	
}

/**
 * return the pixbufs. It will initialize pixbufs first if they weren't before
 * node_access: can be NULL.
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
	return pix;
}


GtkWidget* 
symbol_db_view_new (void)
{
	return gtk_widget_new (SYMBOL_TYPE_DB_VIEW, NULL);	
}



void 
symbol_db_view_open (SymbolDBView *dbv, SymbolDBEngine *dbe)
{
	SymbolDBEngineIterator *iterator;
	GtkTreeStore *store;
	SymbolDBViewPriv *priv;
	gint i, j;
	
	g_return_if_fail (dbv != NULL);
	
	priv = dbv->priv;
	priv->dbe = dbe;
	
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbv)));
	
	/* Make sure the model stays with us after the tree view unrefs it */
	g_object_ref(store); 
	/* Detach model from view */
	gtk_tree_view_set_model(GTK_TREE_VIEW(dbv), NULL); 
	
	/* when opening the project just add the global members, being them 
	 * classes, namespaces, global functions etc
	 * We'll add two levels, so that children can be browsed.
	 */
	iterator = symbol_db_engine_get_global_members (dbe, NULL, SYMINFO_SIMPLE| 
										SYMINFO_FILE_PATH | SYMINFO_ACCESS |
										SYMINFO_KIND);
	
	if (iterator != NULL)
	{
		do {
			GtkTreeIter iter;
			const gchar *sym_name;
			gint curr_symbol_id;
			gchar *file_path;

			/* store the parent */
			gtk_tree_store_append (store, &iter, NULL);			
			
			sym_name = symbol_db_engine_iterator_get_symbol_name (iterator);			
			curr_symbol_id = symbol_db_engine_iterator_get_symbol_id (iterator);
/*			DEBUG_PRINT ("father for %s [%d]", sym_name, count++);*/
			/* get the full file path instead of a database-oriented one. */
			file_path = 
				symbol_db_engine_get_full_local_path (dbe, 
					symbol_db_engine_iterator_get_symbol_extra_string (iterator,
													SYMINFO_FILE_PATH));
			
			gtk_tree_store_set (store, &iter,
						COLUMN_PIXBUF, symbol_db_view_get_pixbuf (
							symbol_db_engine_iterator_get_symbol_extra_string (
									iterator, SYMINFO_KIND),
							symbol_db_engine_iterator_get_symbol_extra_string (
									iterator, SYMINFO_ACCESS)
						),
						COLUMN_NAME, sym_name,
						COLUMN_LINE, 
							symbol_db_engine_iterator_get_symbol_file_pos (iterator),
						COLUMN_FILE, file_path,
						COLUMN_SYMBOL_ID, curr_symbol_id,
						-1);
			
			g_free (file_path);	
			
			/* go on with the checking of children... 
			 * we can speed up the queries just using symbols' ids
			 */
			do_add_hidden_children (dbe, store, &iter, curr_symbol_id);
			
		} while (symbol_db_engine_iterator_move_next (iterator) == TRUE);
		
		g_object_unref (iterator);
	}
	
	if (priv->insert_handler <= 0) 
	{
		priv->insert_handler = 	g_signal_connect (G_OBJECT (dbe), "symbol_inserted",
					  G_CALLBACK (on_symbol_inserted), dbv);
	}
/*
	if (priv->update_handler <= 0)
	{
		
		priv->update_handler = g_signal_connect (G_OBJECT (dbe), "symbol_updated",
					  G_CALLBACK (on_symbol_updated), dbv);
	}
*/
	if (priv->remove_handler <= 0)
	{
		priv->remove_handler = g_signal_connect (G_OBJECT (dbe), "symbol_removed",
					  G_CALLBACK (on_symbol_removed), dbv);
	}
	
	/* Re-attach model to view */
	gtk_tree_view_set_model (GTK_TREE_VIEW (dbv), GTK_TREE_MODEL (store)); 
	g_object_unref(store);	
}
