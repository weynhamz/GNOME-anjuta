/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2003 CodeFactory AB
 * Copyright (C) 2001-2003 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2005 		Massimo Corà <maxcvs@email.it>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gtk/gtkaccessible.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtktreeselection.h>
#include <glib/gi18n.h>
#include <libanjuta/anjuta-debug.h>

#include "an_symbol_search.h"
#include "an_symbol_info.h"
#include "an_symbol_view.h"

#include "plugin.h"

/* private class */
struct _AnjutaSymbolSearchPriv
{
	AnjutaSymbolView *sv;
	GtkTreeModel *model;	/* AnSymbolView's one [gtk_tree_store] */

	GtkWidget *entry;	/* entrybox */
	GtkWidget *hitlist;	/* treeview */

	GCompletion *completion;

	guint idle_complete;
	guint idle_filter;
};

static void an_symbol_search_init (AnjutaSymbolSearch * search);
static void an_symbol_search_class_init (AnjutaSymbolSearchClass * klass);
static void an_symbol_search_finalize (GObject * object);
static gboolean an_symbol_search_on_tree_row_activate(GtkTreeView * view,
				       GtkTreePath * arg1,
				       GtkTreeViewColumn * arg2,
				       AnjutaSymbolSearch * search);

static gboolean an_symbol_search_on_entry_key_press_event (GtkEntry * entry,
							   GdkEventKey *
							   event,
							   AnjutaSymbolSearch
							   * search);

static void an_symbol_search_on_entry_changed (GtkEntry * entry,
					       AnjutaSymbolSearch * search);
static void an_symbol_search_on_entry_activated (GtkEntry * entry,
						 AnjutaSymbolSearch * search);
static void an_symbol_search_on_entry_text_inserted (GtkEntry * entry,
						     const gchar * text,
						     gint length,
						     gint * position,
						     AnjutaSymbolSearch *
						     search);
static gboolean an_symbol_search_complete_idle (AnjutaSymbolSearch * search);
static gboolean an_symbol_search_filter_idle (AnjutaSymbolSearch * search);

static AnjutaSymbolInfo *an_symbol_search_model_filter (AnjutaSymbolSearch *
							model,
							const gchar * string);

enum
{
	SYM_SELECTED,
	LAST_SIGNAL
};

enum
{
	PIXBUF_COLUMN,
	NAME_COLUMN,
	SVFILE_ENTRY_COLUMN,
	COLUMNS_NB
};

/* max hits to display on the search tab */
#define MAX_HITS	100

static GtkVBox *parent_class;
static gint signals[LAST_SIGNAL] = { 0 };

/*---------------------------------------------------------------------------*/
static void
an_symbol_search_dispose (GObject * obj)
{
	AnjutaSymbolSearch *search = ANJUTA_SYMBOL_SEARCH (obj);
	AnjutaSymbolSearchPriv *priv = search->priv;
	
	DEBUG_PRINT("%s", "Destroying symbolsearch");
	
	/* anjuta_symbol_view's dispose should manage it's freeing */
	if (priv->model)
	{
		anjuta_symbol_search_clear(search);
		g_object_unref (priv->model);
		priv->model = NULL;
	}	
	
	if (priv->entry)
		priv->entry = NULL;
	
	if (priv->hitlist)
		priv->hitlist = NULL;
	
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

/*---------------------------------------------------------------------------*/
static void
an_symbol_search_finalize (GObject * obj)
{
	AnjutaSymbolSearch *search = ANJUTA_SYMBOL_SEARCH (obj);
	AnjutaSymbolSearchPriv *priv = search->priv;

	DEBUG_PRINT ("%s", "Finalizing symbolsearch widget");
	
	g_list_foreach (priv->completion->items, (GFunc)g_free, NULL);
	g_completion_free (priv->completion);
	g_free (priv);
			
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

/*-----------------------------------------------------------------------------
 * Cleaning issues. This function must be called when a project is removed.
 */
void anjuta_symbol_search_clear (AnjutaSymbolSearch *search) {
	
	AnjutaSymbolSearchPriv *priv;
	priv=search->priv;
	
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
anjuta_symbol_search_get_type (void)
{
	static GType type = 0;

	if (!type)
	{
		static const GTypeInfo info = {
			sizeof (AnjutaSymbolSearchClass),
			NULL,
			NULL,
			(GClassInitFunc) an_symbol_search_class_init,
			NULL,
			NULL,
			sizeof (AnjutaSymbolSearch),
			0,
			(GInstanceInitFunc) an_symbol_search_init,
		};

		type = g_type_register_static (GTK_TYPE_VBOX,
					       "AnjutaSymbolSearch", &info,
					       0);
	}
	return type;
}

static void
an_symbol_search_class_init (AnjutaSymbolSearchClass * klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = an_symbol_search_finalize;
	object_class->dispose = an_symbol_search_dispose;

	signals[SYM_SELECTED] =
		g_signal_new ("symbol-selected",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AnjutaSymbolSearchClass,
					       symbol_selected), NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE,
			      1, G_TYPE_POINTER);
}

/*--------------------------------------------------------------------------*/
static void
an_symbol_search_init (AnjutaSymbolSearch * search)
{

	AnjutaSymbolSearchPriv *priv;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkWidget *frame, *list_sw;

	/* allocate space for a AnjutaSymbolSearchPriv class. */
	priv = g_new0 (AnjutaSymbolSearchPriv, 1);
	search->priv = priv;

	priv->idle_complete = 0;
	priv->idle_filter = 0;
	priv->sv = NULL;
	
	priv->completion =
		g_completion_new (NULL);

	priv->hitlist = gtk_tree_view_new ();

	priv->model = GTK_TREE_MODEL (gtk_tree_store_new (COLUMNS_NB,
							  GDK_TYPE_PIXBUF,
							  G_TYPE_STRING,
							  ANJUTA_TYPE_SYMBOL_INFO));

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
					    PIXBUF_COLUMN);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
					    NAME_COLUMN);

	gtk_tree_view_append_column (GTK_TREE_VIEW (priv->hitlist), column);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (priv->hitlist),
					   column);

	gtk_box_set_spacing (GTK_BOX (search), 2);
	
	gtk_container_set_border_width (GTK_CONTAINER (search), 2);

	/* creating entry box, where we'll type the keyword to look for */
	priv->entry = gtk_entry_new ();

	/* set up some signals */
	g_signal_connect (priv->entry, "key_press_event",
			  G_CALLBACK (an_symbol_search_on_entry_key_press_event),
			  search);

	g_signal_connect (priv->hitlist, "row_activated",
			  G_CALLBACK (an_symbol_search_on_tree_row_activate),
			  search);

	g_signal_connect (priv->entry, "changed",
			  G_CALLBACK (an_symbol_search_on_entry_changed),
			  search);

	g_signal_connect (priv->entry, "activate",
			  G_CALLBACK (an_symbol_search_on_entry_activated),
			  search);

	g_signal_connect (priv->entry, "insert_text",
			  G_CALLBACK (an_symbol_search_on_entry_text_inserted), search);

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

static gboolean
an_symbol_search_on_tree_row_activate (GtkTreeView * view,
				       GtkTreePath * arg1,
				       GtkTreeViewColumn * arg2,
				       AnjutaSymbolSearch * search)
{

	GtkTreeIter iter;
	AnjutaSymbolSearchPriv *priv;
	AnjutaSymbolInfo *sym;
	GtkTreeSelection *selection;

	priv = search->priv;

	selection = gtk_tree_view_get_selection (view);

	if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		DEBUG_PRINT
			("%s", "an_symbol_search_on_tree_row_activate: error getting selected row");
		return FALSE;
	}

	gtk_tree_model_get (GTK_TREE_MODEL (priv->model),
						&iter, SVFILE_ENTRY_COLUMN, &sym, -1);

	g_signal_emit (search, signals[SYM_SELECTED], 0, sym);
	anjuta_symbol_info_free (sym);
	/* Always return FALSE so the tree view gets the event and can update
	 * the selection etc.
	 */
	return FALSE;
}

static gboolean
an_symbol_search_on_entry_key_press_event (GtkEntry * entry,
					   GdkEventKey * event,
					   AnjutaSymbolSearch * search)
{
	AnjutaSymbolSearchPriv *priv;

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
		AnjutaSymbolInfo *sym;
		gchar *name;

		DEBUG_PRINT("%s", "enter key pressed: getting the first entry found");

		/* Get the first entry found. */
		if (gtk_tree_model_get_iter_first
		    (GTK_TREE_MODEL (priv->model), &iter))
		{

			gtk_tree_model_get (GTK_TREE_MODEL (priv->model),
								&iter,
								NAME_COLUMN, &name,
								SVFILE_ENTRY_COLUMN, &sym, -1);

			g_return_val_if_fail (&iter != NULL, FALSE);

			DEBUG_PRINT ("got -----> sym_name: %s ", sym->sym_name);
			gtk_entry_set_text (GTK_ENTRY (entry), name);
			
			gtk_editable_set_position (GTK_EDITABLE (entry), -1);
			gtk_editable_select_region (GTK_EDITABLE (entry), -1, -1);

			g_signal_emit (search, signals[SYM_SELECTED], 0, sym);
			
			anjuta_symbol_info_free (sym);
			g_free (name);

			return TRUE;
		}
	}
	return FALSE;
}

static void
an_symbol_search_on_entry_changed (GtkEntry * entry,
				   AnjutaSymbolSearch * search)
{
	AnjutaSymbolSearchPriv *priv;

	g_return_if_fail (GTK_IS_ENTRY (entry));
	g_return_if_fail (ANJUTA_SYMBOL_IS_SEARCH (search));

	priv = search->priv;

	DEBUG_PRINT("%s", "Entry changed");

	if (!priv->idle_filter)
	{
		priv->idle_filter =
			g_idle_add ((GSourceFunc)
				    an_symbol_search_filter_idle, search);
	}
}

static void
an_symbol_search_on_entry_activated (GtkEntry * entry,
				     AnjutaSymbolSearch * search)
{
	AnjutaSymbolInfo *info;
	AnjutaSymbolSearchPriv *priv;
	gchar *str;

	g_return_if_fail (GTK_IS_ENTRY (entry));
	g_return_if_fail (ANJUTA_SYMBOL_IS_SEARCH (search));

	priv = search->priv;

	str = (gchar *) gtk_entry_get_text (GTK_ENTRY (priv->entry));

	/* parse the string typed in the entry   */
	info = an_symbol_search_model_filter (search, str);
	if (info)
		anjuta_symbol_info_free (info);
}

static void
an_symbol_search_on_entry_text_inserted (GtkEntry * entry,
					 const gchar * text,
					 gint length,
					 gint * position,
					 AnjutaSymbolSearch * search)
{
	AnjutaSymbolSearchPriv *priv;
	g_return_if_fail (ANJUTA_SYMBOL_IS_SEARCH (search));

	priv = search->priv;

	if (!priv->idle_complete)
	{
		priv->idle_complete =
			g_idle_add ((GSourceFunc)
				    an_symbol_search_complete_idle, search);
	}
}

static gboolean
an_symbol_search_complete_idle (AnjutaSymbolSearch * search)
{
	AnjutaSymbolSearchPriv *priv;
	const gchar *text;
	gchar *completed = NULL;
	GList *list;
	gint text_length;

	g_return_val_if_fail (ANJUTA_SYMBOL_IS_SEARCH (search), FALSE);

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

static gboolean
an_symbol_search_filter_idle (AnjutaSymbolSearch * search)
{
	AnjutaSymbolSearchPriv *priv;
	gchar *str;
	AnjutaSymbolInfo *sym;

	g_return_val_if_fail (ANJUTA_SYMBOL_IS_SEARCH (search), FALSE);

	priv = search->priv;

	str = (gchar *) gtk_entry_get_text (GTK_ENTRY (priv->entry));
	sym = an_symbol_search_model_filter (search, str);

	priv->idle_filter = 0;

	/* if you wanna that on word completion event we [open file->goto symbol line]
	just uncomment this part. Anyway this could cause some involountary editing/tampering 
	with just opened files */
	
	if (sym)
	{
		/* g_signal_emit (search, signals[SYM_SELECTED], 0, sym); */
		anjuta_symbol_info_free (sym);
	}
	return FALSE;
}

AnjutaSymbolInfo *
an_symbol_search_model_filter (AnjutaSymbolSearch * search,
							   const gchar * string)
{
	AnjutaSymbolSearchPriv *priv;
	gint i;
	GtkTreeStore *store;
	const GPtrArray *tags;
	AnjutaSymbolInfo *exactsym = NULL;
	gint hits = 0;

	g_return_val_if_fail (ANJUTA_SYMBOL_IS_SEARCH (search), NULL);
	g_return_val_if_fail (string != NULL, NULL);

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
		tags = tm_workspace_find (string, tm_tag_max_t, NULL, TRUE, TRUE);
		if (tags && (tags->len > 0))
		{
			GList *completion_list;
			gint max_hits;
			
			hits = tags->len;
			max_hits = (tags->len < MAX_HITS)? tags->len : MAX_HITS;
			
			completion_list = NULL;
			
			for (i = 0; i < max_hits; ++i)
			{
				TMSymbol *symbol;
				GtkTreeIter iter;
				TMTag *tag;
				AnjutaSymbolInfo *sym = NULL;
				
				tag = (TMTag *) tags->pdata[i];
				symbol = g_new0 (TMSymbol, 1);
				symbol->tag = tag;
				
				sym = anjuta_symbol_info_new (symbol,
							  anjuta_symbol_info_get_node_type (symbol, NULL));
				
				if (sym->sym_name)
				{
					/* add a new iter */
					gtk_tree_store_append (GTK_TREE_STORE (store), &iter, NULL);
					
					gtk_tree_store_set (GTK_TREE_STORE (store), &iter,
								PIXBUF_COLUMN,
								anjuta_symbol_info_get_pixbuf (sym->node_type),
								NAME_COLUMN, tag->name,
								SVFILE_ENTRY_COLUMN, sym, -1);
					
					completion_list = g_list_prepend (completion_list,
													  g_strdup (sym->sym_name));
					
					if ((hits == 1) ||
						(!exactsym && strcmp (tag->name, string) == 0))
					{
						if (exactsym)
							anjuta_symbol_info_free (exactsym);
						exactsym = sym;
					}
				}
				g_free (symbol);
				if (exactsym != sym)
					anjuta_symbol_info_free (sym);
			}
			if (completion_list)
			{
				completion_list = g_list_reverse (completion_list);
				g_completion_add_items (priv->completion, completion_list);
				g_list_free (completion_list);
			}
		}
	}
	return exactsym;
}

/*--------------------------------------------------------------------------*/
GtkWidget *
anjuta_symbol_search_new (AnjutaSymbolView *symbol_view)
{
	AnjutaSymbolSearch *search;
	/* create a new object   */
	search = g_object_new (ANJUTA_TYPE_SYMBOL_SEARCH, NULL);
	search->priv->sv = symbol_view;
	return GTK_WIDGET (search);
}

#if 0
void
anjuta_symbol_search_set_search_string (AnjutaSymbolSearch * search,
					const gchar * str)
{
	/* FIXME: untested function. Leave this here for a future feature? */
	AnjutaSymbolSearchPriv *priv;

	g_return_if_fail (ANJUTA_SYMBOL_IS_SEARCH (search));

	priv = search->priv;

	gtk_entry_set_text (GTK_ENTRY (priv->entry), str);

	gtk_editable_set_position (GTK_EDITABLE (priv->entry), -1);
	gtk_editable_select_region (GTK_EDITABLE (priv->entry), -1, -1);
}

void
anjuta_symbol_search_grab_focus (AnjutaSymbolSearch * search)
{
	/* FIXME: untested function. Leave this here for a future feature? */
	AnjutaSymbolSearchPriv *priv;

	g_return_if_fail (ANJUTA_SYMBOL_IS_SEARCH (search));

	priv = search->priv;

	gtk_widget_grab_focus (priv->entry);
}
#endif
