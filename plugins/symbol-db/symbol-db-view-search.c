/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) 2001-2003 CodeFactory AB
 * Copyright (C) 2001-2003 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2005-2009 Massimo Cora' <maxcvs@email.it>
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

#include <config.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-marshal.h>

#include "symbol-db-view-search.h"
#include "symbol-db-engine.h"
#include "symbol-db-view.h"

/* private class */
struct _SymbolDBViewSearchPriv
{
	SymbolDBEngine *sdbe;
	GtkTreeModel *model;	

	GtkWidget *entry;	/* entrybox */
	GtkWidget *hitlist;	/* treeview */

	GCompletion *completion;

	guint idle_complete;
	guint idle_filter;
};

enum
{
	SYM_SELECTED,
	LAST_SIGNAL
};

enum {
	COLUMN_PIXBUF,
	COLUMN_DESC,
	COLUMN_NAME,
	COLUMN_LINE,
	COLUMN_FILE,
	COLUMN_SYMBOL_ID,
	COLUMN_MAX
};

/* max hits to display on the search tab */
#define MAX_HITS	100

static GtkVBox *parent_class;
static gint signals[LAST_SIGNAL] = { 0 };

static void
sdb_view_search_model_filter (SymbolDBViewSearch * search,
							   const gchar * string)
{
	SymbolDBViewSearchPriv *priv;
	gint i;
	GtkTreeStore *store;
	SymbolDBEngineIterator *iterator;								   
	gint hits = 0;

	g_return_if_fail (SYMBOL_IS_DB_VIEW_SEARCH (search));
	g_return_if_fail (string != NULL);

	priv = search->priv;

	/* get the tree store model */
	store = GTK_TREE_STORE (gtk_tree_view_get_model
				(GTK_TREE_VIEW (priv->hitlist)));
	
	/* let's clean up rows from store model */
	g_list_foreach (priv->completion->items, (GFunc)g_free, NULL);
	g_completion_clear_items (priv->completion);
	
	gtk_tree_store_clear (GTK_TREE_STORE (store));

	if (strlen (string))
	{
		gchar *pattern;
		pattern = g_strdup_printf ("%%%s%%", string);
			
		iterator = symbol_db_engine_find_symbol_by_name_pattern (priv->sdbe, 
										pattern, FALSE, SYMINFO_SIMPLE| SYMINFO_FILE_PATH |
												SYMINFO_ACCESS | SYMINFO_KIND);
		g_free (pattern);
		
		if (iterator)
		{
			GList *completion_list;
			gint max_hits;
			SymbolDBEngineIteratorNode *iter_node;
			
			/* max number of hits to take care of */
			hits = symbol_db_engine_iterator_get_n_items (iterator);
			max_hits = (hits < MAX_HITS)? hits : MAX_HITS;
			
			completion_list = NULL;
			
			for (i = 0; i < max_hits; ++i)
			{
				GtkTreeIter iter;				
				iter_node = SYMBOL_DB_ENGINE_ITERATOR_NODE (iterator);
				
				const gchar *sym_name = 
					symbol_db_engine_iterator_node_get_symbol_name (iter_node);

				/* get the full file path instead of a database-oriented one. */					
				const gchar *file_path = 
					symbol_db_engine_iterator_node_get_symbol_extra_string (
									iter_node, SYMINFO_FILE_PATH);
				
				if (sym_name && file_path)
				{
					gchar *display;
					gchar *db_file_path;
					gint pos;
					
					/* and now get the relative one */
					db_file_path = symbol_db_util_get_file_db_path (priv->sdbe,
																	  file_path);
					
					pos = symbol_db_engine_iterator_node_get_symbol_file_pos (iter_node);
					display = g_markup_printf_escaped("<b>%s</b>\n"
										"<small><tt>%s:%d</tt></small>",
										sym_name,
										db_file_path,
										pos);
					g_free (db_file_path);
					
					/* add a new iter */
					gtk_tree_store_append (GTK_TREE_STORE (store), &iter, NULL);
					

					gtk_tree_store_set (GTK_TREE_STORE (store), &iter,
							COLUMN_PIXBUF, symbol_db_util_get_pixbuf (
								symbol_db_engine_iterator_node_get_symbol_extra_string (
									iter_node, SYMINFO_KIND),
									symbol_db_engine_iterator_node_get_symbol_extra_string (
									iter_node, SYMINFO_ACCESS)
								),
							COLUMN_DESC, display,
							COLUMN_NAME, sym_name,
							COLUMN_LINE, 
								symbol_db_engine_iterator_node_get_symbol_file_pos (
													iter_node),
							COLUMN_FILE, file_path,
							COLUMN_SYMBOL_ID,
								symbol_db_engine_iterator_node_get_symbol_id (iter_node),
							-1);
					
					completion_list = g_list_prepend (completion_list,
							g_strdup (sym_name));
					
					g_free (display);
				}

				symbol_db_engine_iterator_move_next (iterator);
			}
			if (completion_list)
			{
				completion_list = g_list_reverse (completion_list);
				g_completion_add_items (priv->completion, completion_list);
				g_list_free (completion_list);
			}
		}
		
		if (iterator)
			g_object_unref (iterator);
	}
}

static gboolean
sdb_view_search_filter_idle (SymbolDBViewSearch * search)
{
	SymbolDBViewSearchPriv *priv;
	gchar *str;

	g_return_val_if_fail (SYMBOL_IS_DB_VIEW_SEARCH (search), FALSE);

	priv = search->priv;

	str = (gchar *) gtk_entry_get_text (GTK_ENTRY (priv->entry));
	sdb_view_search_model_filter (search, str);

	priv->idle_filter = 0;
	return FALSE;
}

static void
sdb_view_search_on_entry_changed (GtkEntry * entry,
				   SymbolDBViewSearch * search)
{
	SymbolDBViewSearchPriv *priv;

	g_return_if_fail (GTK_IS_ENTRY (entry));
	g_return_if_fail (SYMBOL_IS_DB_VIEW_SEARCH (search));

	priv = search->priv;

	DEBUG_PRINT("%s", "Entry changed");

	if (!priv->idle_filter)
	{
		priv->idle_filter =
			g_idle_add ((GSourceFunc)
				    sdb_view_search_filter_idle, search);
	}
}

static void
sdb_view_search_on_entry_activated (GtkEntry * entry,
				     SymbolDBViewSearch * search)
{
	SymbolDBViewSearchPriv *priv;
	gchar *str;

	g_return_if_fail (GTK_IS_ENTRY (entry));
	g_return_if_fail (SYMBOL_IS_DB_VIEW_SEARCH (search));

	priv = search->priv;

	str = (gchar *) gtk_entry_get_text (GTK_ENTRY (priv->entry));

	/* parse the string typed in the entry   */
	sdb_view_search_model_filter (search, str);
}

static gboolean
sdb_view_search_on_tree_row_activate (GtkTreeView * view,
				       GtkTreePath * arg1,
				       GtkTreeViewColumn * arg2,
				       SymbolDBViewSearch * search)
{

	GtkTreeIter iter;
	SymbolDBViewSearchPriv *priv;
	gint line;
	gchar *file;
	GtkTreeSelection *selection;

	priv = search->priv;

	selection = gtk_tree_view_get_selection (view);

	if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		DEBUG_PRINT
			("%s", "sdb_view_search_on_tree_row_activate: error getting selected row");
		return FALSE;
	}

	gtk_tree_model_get (GTK_TREE_MODEL (priv->model),
						&iter, 
						COLUMN_LINE, &line,
						COLUMN_FILE, &file,
						-1);

	/*DEBUG_PRINT ("sdb_view_search_on_tree_row_activate: file %s", file);*/
						   
	g_signal_emit (search, signals[SYM_SELECTED], 0, line, file);
	
	g_free (file);
						   
	/* Always return FALSE so the tree view gets the event and can update
	 * the selection etc.
	 */
	return FALSE;
}

static gboolean
sdb_view_search_on_entry_key_press_event (GtkEntry * entry,
					   GdkEventKey * event,
					   SymbolDBViewSearch * search)
{
	SymbolDBViewSearchPriv *priv;

	priv = search->priv;

	DEBUG_PRINT ("%s", "key_press event");
	if (event->keyval == GDK_Tab)
	{
		DEBUG_PRINT ("%s", "tab key pressed");
		if (event->state & GDK_CONTROL_MASK)
		{
			gtk_widget_grab_focus (priv->hitlist);
		}
		else
		{
			gtk_editable_set_position (GTK_EDITABLE (entry), -1);
			gtk_editable_select_region (GTK_EDITABLE (entry), -1,
						    -1);
		}
		return TRUE;
	}

	if (event->keyval == GDK_Return || event->keyval == GDK_KP_Enter)
	{
		GtkTreeIter iter;
		gchar *name;
		gint line;
		gchar *file;

		/*DEBUG_PRINT("enter key pressed: getting the first entry found");*/

		/* Get the first entry found. */
		if (gtk_tree_model_get_iter_first
		    (GTK_TREE_MODEL (priv->model), &iter))
		{

			gtk_tree_model_get (GTK_TREE_MODEL (priv->model),
								&iter,
								COLUMN_NAME, &name,
								COLUMN_LINE, &line,
								COLUMN_FILE, &file,
								-1);

			g_return_val_if_fail (&iter != NULL, FALSE);
			
			gtk_entry_set_text (GTK_ENTRY (entry), name);
			
			gtk_editable_set_position (GTK_EDITABLE (entry), -1);
			gtk_editable_select_region (GTK_EDITABLE (entry), -1, -1);

			g_signal_emit (search, signals[SYM_SELECTED], 0, line, file);
			
			g_free (name);
			g_free (file);

			return TRUE;
		}
	}
	return FALSE;
}

static gboolean
sdb_view_search_complete_idle (SymbolDBViewSearch * search)
{
	SymbolDBViewSearchPriv *priv;
	const gchar *text;
	gchar *completed = NULL;
	GList *list;
	gint text_length;

	g_return_val_if_fail (SYMBOL_IS_DB_VIEW_SEARCH (search), FALSE);

	priv = search->priv;

	text = gtk_entry_get_text (GTK_ENTRY (priv->entry));

	list = g_completion_complete (priv->completion, (gchar *) text,
								  &completed);

	if (completed)
	{
		text_length = strlen (text);
		gtk_entry_set_text (GTK_ENTRY (priv->entry), completed);

		gtk_editable_set_position (GTK_EDITABLE (priv->entry),
					   text_length);

		gtk_editable_select_region (GTK_EDITABLE (priv->entry),
					    text_length, -1);
		g_free (completed);
	}
	priv->idle_complete = 0;
	return FALSE;
}

static void
sdb_view_search_on_entry_text_inserted (GtkEntry * entry,
					 const gchar * text,
					 gint length,
					 gint * position,
					 SymbolDBViewSearch * search)
{
	SymbolDBViewSearchPriv *priv;
	g_return_if_fail (SYMBOL_IS_DB_VIEW_SEARCH (search));

	priv = search->priv;

	if (!priv->idle_complete)
	{
		priv->idle_complete =
			g_idle_add ((GSourceFunc)
				    sdb_view_search_complete_idle, search);
	}
}

static gint
sdb_view_search_sort_iter_compare_func (GtkTreeModel *model, GtkTreeIter  *a,
		GtkTreeIter  *b, gpointer userdata)
{
	gchar *name1, *name2;
	gchar *file1, *file2;
	gint ret;
	
	gtk_tree_model_get (model, a, COLUMN_NAME, &name1, COLUMN_FILE, &file1, -1);
    gtk_tree_model_get (model, b, COLUMN_NAME, &name2, COLUMN_FILE, &file2, -1);

	if (name1 == NULL || name2 == NULL || file1 == NULL || file2 == NULL)
    {
		ret = 0;
	}
	else if ((ret = g_strcmp0 (name1, name2)) == 0)		/* they're equal */
	{
		/* process file name */		
		ret = g_strcmp0 (file1, file2);
	}

	g_free (name1);
	g_free (name2);
	g_free (file1);
	g_free (file2);
	
	return ret;
}


static void
sdb_view_search_init (SymbolDBViewSearch * search)
{
	SymbolDBViewSearchPriv *priv;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkWidget *frame, *list_sw;
	
	/* allocate space for a SymbolDBViewSearchPriv class. */
	priv = g_new0 (SymbolDBViewSearchPriv, 1);
	search->priv = priv;

	priv->idle_complete = 0;
	priv->idle_filter = 0;	
	priv->completion = g_completion_new (NULL);
	priv->hitlist = gtk_tree_view_new ();
	
	priv->model = GTK_TREE_MODEL (gtk_tree_store_new (COLUMN_MAX, GDK_TYPE_PIXBUF,
				    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT));
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (priv->model),
										  COLUMN_NAME,
										  GTK_SORT_ASCENDING);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (priv->model),
				COLUMN_NAME,
				(GtkTreeIterCompareFunc )sdb_view_search_sort_iter_compare_func,
				NULL,
				NULL);
									 
	gtk_tree_view_set_model (GTK_TREE_VIEW (priv->hitlist), GTK_TREE_MODEL (priv->model));	
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (priv->hitlist), FALSE);
	
	gtk_tree_view_set_model (GTK_TREE_VIEW (priv->hitlist),
				 GTK_TREE_MODEL (priv->model));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (priv->hitlist), TRUE);

	/* column initialization */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing (column,
					 GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "pixbuf",
					    COLUMN_PIXBUF);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
					    COLUMN_DESC);
	gtk_tree_view_column_set_attributes (column, renderer,
										 "markup", COLUMN_DESC, NULL);	

	gtk_tree_view_append_column (GTK_TREE_VIEW (priv->hitlist), column);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (priv->hitlist),
					   column);

	gtk_box_set_spacing (GTK_BOX (search), 2);
	
	gtk_container_set_border_width (GTK_CONTAINER (search), 2);

	/* creating entry box, where we'll type the keyword to look for */
	priv->entry = gtk_entry_new ();

	/* set up some signals */
	g_signal_connect (priv->entry, "key_press_event",
			  G_CALLBACK (sdb_view_search_on_entry_key_press_event),
			  search);

	g_signal_connect (priv->hitlist, "row_activated",
			  G_CALLBACK (sdb_view_search_on_tree_row_activate),
			  search);

	g_signal_connect (priv->entry, "changed",
			  G_CALLBACK (sdb_view_search_on_entry_changed),
			  search);

	g_signal_connect (priv->entry, "activate",
			  G_CALLBACK (sdb_view_search_on_entry_activated),
			  search);

	g_signal_connect (priv->entry, "insert_text",
			  G_CALLBACK (sdb_view_search_on_entry_text_inserted), search);

	gtk_box_pack_start (GTK_BOX (search), priv->entry, FALSE, FALSE, 0);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);

	list_sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (list_sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);

	gtk_container_add (GTK_CONTAINER (frame), list_sw);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (priv->hitlist), FALSE);

	gtk_container_add (GTK_CONTAINER (list_sw), priv->hitlist);
	gtk_box_pack_end_defaults (GTK_BOX (search), frame);

	gtk_widget_show_all (GTK_WIDGET (search));
}

static void
sdb_view_search_dispose (GObject * obj)
{
	SymbolDBViewSearch *search = SYMBOL_DB_VIEW_SEARCH (obj);
	SymbolDBViewSearchPriv *priv = search->priv;
	
	DEBUG_PRINT("%s", "Destroying symbolsearch");

	if (priv->entry)
		g_signal_handlers_disconnect_by_func (G_OBJECT(priv->entry),
						  G_CALLBACK (sdb_view_search_on_entry_key_press_event),
						  search);
	
	if (priv->hitlist)
		g_signal_handlers_disconnect_by_func (G_OBJECT(priv->hitlist),
						  G_CALLBACK (sdb_view_search_on_tree_row_activate),
						  search);

	if (priv->entry)
		g_signal_handlers_disconnect_by_func (G_OBJECT(priv->entry),
						  G_CALLBACK (sdb_view_search_on_entry_changed),
						  search);
	
	if (priv->entry)
		g_signal_handlers_disconnect_by_func (G_OBJECT(priv->entry),
						  G_CALLBACK (sdb_view_search_on_entry_activated),
						  search);

	if (priv->entry)
		g_signal_handlers_disconnect_by_func (G_OBJECT(priv->entry),
						  G_CALLBACK (sdb_view_search_on_entry_text_inserted),
						  search);
	
	/* anjuta_symbol_view's dispose should manage it's freeing */
	if (priv->model)
	{
		symbol_db_view_search_clear(search);
		g_object_unref (priv->model);
		priv->model = NULL;
	}	
	
	if (priv->entry)
		priv->entry = NULL;
	
	if (priv->hitlist)
		priv->hitlist = NULL;
	
	G_OBJECT_CLASS (parent_class)->dispose (obj);	
}


static void
sdb_view_search_finalize (GObject * obj)
{
	SymbolDBViewSearch *search = SYMBOL_DB_VIEW_SEARCH (obj);
	SymbolDBViewSearchPriv *priv = search->priv;

	DEBUG_PRINT ("%s", "Finalizing symbolsearch widget");
	
	g_list_foreach (priv->completion->items, (GFunc)g_free, NULL);
	g_completion_free (priv->completion);
	g_free (priv);
			
	G_OBJECT_CLASS (parent_class)->finalize (obj);		
}


static void
sdb_view_search_class_init (SymbolDBViewSearchClass * klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = sdb_view_search_finalize;
	object_class->dispose = sdb_view_search_dispose;

	signals[SYM_SELECTED] =
		g_signal_new ("symbol-selected",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (SymbolDBViewSearchClass,
					       symbol_selected), NULL, NULL,
			      anjuta_cclosure_marshal_VOID__INT_STRING, G_TYPE_NONE,
			      2, G_TYPE_INT, G_TYPE_STRING);
}

/**
 * Cleaning issues. This function must be called when a project is removed.
 */
void 
symbol_db_view_search_clear (SymbolDBViewSearch *search) 
{	
	SymbolDBViewSearchPriv *priv;
	priv = search->priv;
	
	/* set entry text to a NULL string */
	gtk_entry_set_text (GTK_ENTRY (priv->entry), "");
	
	/* thrown away the g_completion words */
	g_list_foreach (priv->completion->items, (GFunc)g_free, NULL);
	g_completion_clear_items (priv->completion);	
	
	/* clean the gtk_tree_store */
	gtk_tree_store_clear (GTK_TREE_STORE(gtk_tree_view_get_model
				(GTK_TREE_VIEW (priv->hitlist))));
}

GType
sdb_view_search_get_type (void)
{
	static GType type = 0;

	if (!type)
	{
		static const GTypeInfo info = {
			sizeof (SymbolDBViewSearchClass),
			NULL,
			NULL,
			(GClassInitFunc) sdb_view_search_class_init,
			NULL,
			NULL,
			sizeof (SymbolDBViewSearch),
			0,
			(GInstanceInitFunc) sdb_view_search_init,
		};

		type = g_type_register_static (GTK_TYPE_VBOX,
					       "SymbolDBViewSearch", &info, 0);
	}
	return type;
}

GtkWidget *
symbol_db_view_search_new (SymbolDBEngine *dbe)
{
	SymbolDBViewSearch *search;
	SymbolDBViewSearchPriv *priv;
	
	/* create a new object   */
	search = g_object_new (SYMBOL_TYPE_DB_VIEW_SEARCH, NULL);
	
	/* store the engine pointer */
	priv = search->priv;
	priv->sdbe = dbe;
	
	return GTK_WIDGET (search);
}

GtkEntry *
symbol_db_view_search_get_entry (SymbolDBViewSearch *search)
{
	SymbolDBViewSearchPriv *priv;
	priv = search->priv;

	g_return_val_if_fail (search != NULL, NULL);
	
	return GTK_ENTRY (priv->entry);
}
