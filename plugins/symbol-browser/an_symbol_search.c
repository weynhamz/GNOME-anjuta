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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
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
#include <libgnome/gnome-i18n.h>


#include "an_symbol_search.h"
#include "an_symbol_info.h"
#include "an_symbol_view.h"

#include "plugin.h"


// private class
struct _AnjutaSymbolSearchPriv {

	GtkTreeModel *model;			// AnSymbolView's one [gtk_tree_store]

	GtkWidget *entry;				// entrybox
	GtkWidget *hitlist;			// treeview
	
	GCompletion *completion;

	guint idle_complete;
	guint idle_filter;

	GdkPixbuf ** pixbufs;		// pixbufs stuff
	
	GList *original_list;
   GList *keyword_words;
};


static void  an_symbol_search_init                       (AnjutaSymbolSearch *search);
static void  an_symbol_search_class_init                 (AnjutaSymbolSearchClass *klass);
static void  an_symbol_search_finalize                   (GObject *object);
static void  an_symbol_search_on_selection_changed       (GtkTreeSelection *selection, AnjutaSymbolSearch *content);
static gboolean an_symbol_search_on_tree_button_press    (GtkTreeView *view, GdkEventButton *event, AnjutaSymbolSearch *search);

static gboolean an_symbol_search_on_entry_key_press_event (GtkEntry *entry,GdkEventKey *event, AnjutaSymbolSearch *search);

static void  an_symbol_search_on_entry_changed (GtkEntry *entry, AnjutaSymbolSearch *search);
static void  an_symbol_search_on_entry_activated (GtkEntry *entry, AnjutaSymbolSearch *search);
static void  an_symbol_search_on_entry_text_inserted (GtkEntry *entry,
						const gchar      *text,
						gint              length,
						gint             *position,
						AnjutaSymbolSearch         *search);
static gboolean an_symbol_search_complete_idle (AnjutaSymbolSearch *search);
static gboolean an_symbol_search_filter_idle (AnjutaSymbolSearch *search);
static gchar *  an_symbol_search_complete_func (AnjutaSymbolInfo *sym);


static AnjutaSymbolInfo *an_symbol_search_model_filter (AnjutaSymbolSearch *model, const gchar *string);
static gint an_symbol_search_symbolfileinfo_compare  (gconstpointer a, gconstpointer b);



enum {
        SYM_SELECTED,
        LAST_SIGNAL
};


enum {
	PIXBUF_COLUMN,
	NAME_COLUMN,
	SVFILE_ENTRY_COLUMN,
	COLUMNS_NB
};

// max hits to display on the search tab
#define MAX_HITS	100

static GtkVBox *parent_class;
static gint     signals[LAST_SIGNAL] = { 0 };

GType
anjuta_symbol_search_get_type (void)
{
	static GType type = 0;

   if (!type) {
   	static const GTypeInfo info =  	{
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
                
         type = g_type_register_static (GTK_TYPE_VBOX, "AnjutaSymbolSearch",  &info, 0);
	}
   return type;
}

static void
an_symbol_search_class_init (AnjutaSymbolSearchClass *klass)
{
	GObjectClass   *object_class;
	
   object_class = (GObjectClass *) klass;
   parent_class = g_type_class_peek_parent (klass);
	
	object_class->finalize = an_symbol_search_finalize;

   signals[SYM_SELECTED] =
               g_signal_new ("symbol_selected",
			      	G_TYPE_FROM_CLASS (klass),
			      	G_SIGNAL_RUN_LAST,
			      	G_STRUCT_OFFSET (AnjutaSymbolSearchClass, symbol_selected),
			      	NULL, NULL,
			      	g_cclosure_marshal_VOID__POINTER,
			      	G_TYPE_NONE,
			      	1, G_TYPE_POINTER);
}


//------------------------------------------------------------------------------
//

static void
an_symbol_search_init (AnjutaSymbolSearch *search) {
	
	AnjutaSymbolSearchPriv *priv;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	
 
	// allocate space for a AnjutaSymbolSearchPriv class.
	priv = g_new0 (AnjutaSymbolSearchPriv, 1);
	search->priv = priv;
	
	priv->idle_complete = 0;
	priv->idle_filter   = 0;

	priv->completion = g_completion_new ((GCompletionFunc) an_symbol_search_complete_func);

	priv->hitlist = gtk_tree_view_new ();

	priv->model = GTK_TREE_MODEL( gtk_tree_store_new (	COLUMNS_NB,
																		GDK_TYPE_PIXBUF,
																		G_TYPE_STRING,
																		ANJUTA_TYPE_SYMBOL_INFO));
														

	gtk_tree_view_set_model (GTK_TREE_VIEW (priv->hitlist), GTK_TREE_MODEL (priv->model));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (priv->hitlist), TRUE);
	
	// column initialization
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "pixbuf", PIXBUF_COLUMN);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text", NAME_COLUMN);

	gtk_tree_view_append_column (GTK_TREE_VIEW (priv->hitlist), column);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (priv->hitlist), column);

	gtk_box_set_spacing (GTK_BOX (search), 2);
}

static void
an_symbol_search_finalize (GObject *object)
{
	if (G_OBJECT_CLASS (parent_class)->finalize) {
		G_OBJECT_CLASS (parent_class)->finalize (object);
	}
}

static void
an_symbol_search_on_selection_changed (GtkTreeSelection *selection, AnjutaSymbolSearch *search)
{
	AnjutaSymbolSearchPriv *priv;
 	GtkTreeIter   iter;
g_message( " IN an_symbol_search_on_selection_changed, need some more tests" );
	g_return_if_fail (GTK_IS_TREE_SELECTION (selection));
	g_return_if_fail (ANJUTA_SYMBOL_IS_SEARCH (search));

	priv = search->priv;

	// now that we've chosen the symbol we must emit a signal
	if (gtk_tree_selection_get_selected (selection, NULL, &iter)) {

		AnjutaSymbolInfo *sym;
		
		gtk_tree_model_get (GTK_TREE_MODEL (priv->model), &iter,
				    SVFILE_ENTRY_COLUMN, &sym,
				    -1);

 		g_message("Emiting signal with symbol to: %s \n", sym->sym_name);
		g_signal_emit (search, signals[SYM_SELECTED], 0, sym);
	}
g_message( " IN an_symbol_search_on_selection_changed, need some test" );	
}

static gboolean
an_symbol_search_on_tree_button_press (GtkTreeView    *view,
			     GdkEventButton *event,
			     AnjutaSymbolSearch *search)
{
	GtkTreePath  *path = NULL;
	GtkTreeIter   iter;
	AnjutaSymbolSearchPriv *priv;
	AnjutaSymbolInfo *sym;

	priv = search->priv;

	gtk_tree_view_get_path_at_pos (view, event->x, event->y, &path, NULL, NULL, NULL);

	if (!path) {
		return FALSE;
	}

	gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->model), &iter, path);

	// free the GtkTreePath
	gtk_tree_path_free (path);

	gtk_tree_model_get (GTK_TREE_MODEL (priv->model),
			    &iter,
			    SVFILE_ENTRY_COLUMN, &sym,
			    -1);
	
	g_signal_emit (search, signals[SYM_SELECTED], 0, sym);
	return TRUE;
}

static gboolean
an_symbol_search_on_entry_key_press_event (GtkEntry    *entry,
				 GdkEventKey *event,
				 AnjutaSymbolSearch    *search)
{
	AnjutaSymbolSearchPriv *priv;
g_message("key_press event" );	
	priv = search->priv;
	
	if (event->keyval == GDK_Tab) {
		g_message("tab key pressed" );	
		if (event->state & GDK_CONTROL_MASK) {
			gtk_widget_grab_focus (priv->hitlist);
		} else {
			gtk_editable_set_position (GTK_EDITABLE (entry), -1);
			gtk_editable_select_region (GTK_EDITABLE (entry), -1, -1);
		}
		return TRUE;
	}

	if (event->keyval == GDK_Return ||
	    event->keyval == GDK_KP_Enter) {
		GtkTreeIter  iter;
		AnjutaSymbolInfo *sym;
		gchar       *name;
			 
		g_message("enter key pressed: getting the first entry found" );
			 
		/* Get the first entry found. */
		// FIXME		
		if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->model), &iter)) {
			
			gtk_tree_model_get (GTK_TREE_MODEL (priv->model),
					    &iter,
			 		    NAME_COLUMN, &name,
					    SVFILE_ENTRY_COLUMN, &sym,
					    -1);
			
			g_return_val_if_fail (&iter != NULL, FALSE);

			g_message( "got -----> sym_name: %s ", sym->sym_name );
			gtk_entry_set_text (GTK_ENTRY (entry), name);
			g_free (name);
			
			gtk_editable_set_position (GTK_EDITABLE (entry), -1);
			gtk_editable_select_region (GTK_EDITABLE (entry), -1, -1);
			
			g_signal_emit (search, signals[SYM_SELECTED], 0, sym);
			
			return TRUE;
		}

	}
	return FALSE;
}

static void
an_symbol_search_on_entry_changed (GtkEntry *entry, AnjutaSymbolSearch *search)
{
	AnjutaSymbolSearchPriv *priv;
	
	g_return_if_fail (GTK_IS_ENTRY (entry));
	g_return_if_fail (ANJUTA_SYMBOL_IS_SEARCH (search));
	
	priv = search->priv;
 
	g_message("Entry changed\n");

	if (!priv->idle_filter) {
		priv->idle_filter =
			g_idle_add ((GSourceFunc) an_symbol_search_filter_idle, search);
	}
}

static void
an_symbol_search_on_entry_activated (GtkEntry *entry, AnjutaSymbolSearch *search)
{
	AnjutaSymbolSearchPriv *priv;
	gchar             *str;

	g_return_if_fail (GTK_IS_ENTRY (entry));
	g_return_if_fail (ANJUTA_SYMBOL_IS_SEARCH (search));

	priv = search->priv;
	
	str = (gchar *) gtk_entry_get_text (GTK_ENTRY (priv->entry));

	// parse the string typed in the entry	
	an_symbol_search_model_filter (search, str);
}


static void
an_symbol_search_on_entry_text_inserted (GtkEntry    *entry,
			       const gchar *text,
			       gint         length,
			       gint        *position,
			       AnjutaSymbolSearch    *search)
{
	AnjutaSymbolSearchPriv *priv;
 	g_return_if_fail (ANJUTA_SYMBOL_IS_SEARCH (search));
	
	priv = search->priv;
	
	if (!priv->idle_complete) {
		priv->idle_complete = 
			g_idle_add ((GSourceFunc) an_symbol_search_complete_idle, search);
	}
}


static gboolean
an_symbol_search_complete_idle (AnjutaSymbolSearch *search)
{
	AnjutaSymbolSearchPriv *priv;
	const gchar       *text;
	gchar             *completed = NULL;
	GList             *list;
	gint               text_length;

	g_return_val_if_fail (ANJUTA_SYMBOL_IS_SEARCH (search), FALSE);
	
	priv = search->priv;
	
	text = gtk_entry_get_text (GTK_ENTRY (priv->entry));
	list = g_completion_complete (priv->completion, (gchar *)text, &completed);

	if (completed) {
		text_length = strlen (text);
		gtk_entry_set_text (GTK_ENTRY (priv->entry), completed);

 		gtk_editable_set_position (GTK_EDITABLE (priv->entry),
 					   text_length);
		gtk_editable_select_region (GTK_EDITABLE (priv->entry),
					    text_length, -1);
	}
	
	priv->idle_complete = 0;
	return FALSE;
}


static gboolean
an_symbol_search_filter_idle (AnjutaSymbolSearch *search)
{
	AnjutaSymbolSearchPriv *priv;
	gchar        *str;
	AnjutaSymbolInfo *sym;

	g_return_val_if_fail (ANJUTA_SYMBOL_IS_SEARCH (search), FALSE);

	priv = search->priv;
	
	str = (gchar *) gtk_entry_get_text (GTK_ENTRY (priv->entry));

	sym = an_symbol_search_model_filter( search, str);
	
	priv->idle_filter = 0;

	if (sym) {
		g_signal_emit (search, signals[SYM_SELECTED], 0, sym);
	}

	return FALSE;
}


//------------------------------------------------------------------------------
// this should return the string from a GList of objects. In this case they are
// AnjutaSymbolInfo. String will be used by autocompletion

static gchar *an_symbol_search_complete_func (AnjutaSymbolInfo *sym) {

	return sym->sym_name;
	
}




//------------------------------------------------------------------------------
//


gint an_symbol_search_symbolfileinfo_compare  (gconstpointer a, gconstpointer b) {
	
	return strcmp (((AnjutaSymbolInfo *)a)->sym_name, ((AnjutaSymbolInfo *)b)->sym_name);
	
}

//------------------------------------------------------------------------------
//

AnjutaSymbolInfo *
an_symbol_search_model_filter (AnjutaSymbolSearch *search, const gchar *string)
{
	AnjutaSymbolSearchPriv  *priv;
	AnjutaSymbolInfo      *sym;
	GList               *node;
	GList               *new_list = NULL;
	gint                 new_length, old_length;
	gint                 i;
	gint                 hits = 0;
	AnjutaSymbolInfo		  *exactsym = NULL;
	gboolean	    			found;
	gchar              **stringv;
	GtkTreeStore	*store;
	
g_message( "an_symbol_search_model_filter : string is %s", string);
	
	g_return_val_if_fail (ANJUTA_SYMBOL_IS_SEARCH(search), NULL);
	g_return_val_if_fail (string != NULL, NULL);

	priv = search->priv;

	// get the tree store model
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (priv->hitlist)));

	/* here we want to change the contents of keyword_words,
	   call update on all rows that is included in the new 
	   list and remove on all outside it */
	
	old_length = g_list_length (priv->keyword_words);

	g_message( "an_symbol_search_model_filter: original_list %d, GList_old length is %d", g_list_length(priv->original_list), old_length);

	if (!strcmp ("", string)) {
		new_list = NULL;
	} else {

		stringv = g_strsplit (string, " ", -1);
		
		// go throught the original list searching for matches
		for (node = priv->original_list; node && hits < MAX_HITS; node = node->next) {

			sym = (AnjutaSymbolInfo*)(node->data);
			found = TRUE;
			for (i = 0; stringv[i] != NULL; i++) {
				if (!g_strrstr (sym->sym_name, stringv[i])) {
	    			found = FALSE;
					break;
				}
			}
				
			if (found) {
				/* Include in the new list */
				new_list = g_list_prepend (new_list, sym);
				hits++;
			}
			
			if (strcmp (sym->sym_name, string) == 0) {
				g_message( "matched iter " );				
				exactsym= sym;
			}
		}

		//	compare the list and sort it following the function_for_comparison
		new_list = g_list_sort (new_list, an_symbol_search_symbolfileinfo_compare );
		g_strfreev (stringv);
	}

	new_length = g_list_length (new_list);
	
	g_message( "new_lisr length is %d", new_length);	
	
	if (priv->keyword_words != priv->original_list) {
		g_message( "freeing old_list" );		
		/* Only remove the old list if it's not pointing at the 
		   original list */
 		g_list_free (priv->keyword_words);
		
	}

	priv->keyword_words = new_list;

	// let's clean up rows from store model
	gtk_tree_store_clear( GTK_TREE_STORE(store));

	// Update rows 0 - new_length 
	node = new_list;

	g_message( "adding up to %d items", new_length );
	for (i = 0; i < new_length; ++i) {
		GtkTreeIter iter;	
		AnjutaSymbolInfo *sym = NULL;
		
		node = g_list_nth (new_list, i);
		sym = node->data;

		// add a new iter
		gtk_tree_store_append ( GTK_TREE_STORE(store), &iter, NULL);	

		gtk_tree_store_set (GTK_TREE_STORE(store), &iter,
				    PIXBUF_COLUMN, priv->pixbufs[sym->node_type],
				    NAME_COLUMN, sym->sym_name,
				    SVFILE_ENTRY_COLUMN, sym,
				    -1);
	}

	if (hits == 1) {
		g_message( "just one item remained" );		
		return (AnjutaSymbolInfo*)(priv->keyword_words->data);
	}

	return exactsym;
}


//------------------------------------------------------------------------------
//

void anjuta_symbol_search_set_keywords_symbols( AnjutaSymbolSearch *search, GList *keywords_symbols ) {

	AnjutaSymbolSearchPriv *priv;
	
	g_message( "Setting keywords on AnjutaSymbolSearch: GList length is %d ", g_list_length(keywords_symbols) );
	
	priv = search->priv;
	
	g_completion_add_items ( priv->completion, keywords_symbols );
	
	// pay attention: we set the original list as a symbol-keyword. The "keywords_symbols" one will be
	// used by an_symbol_search_model_filter to store partial results	
	priv->original_list = keywords_symbols;
	
	// set the entrybox to editable: we can type in out keywords from now
	gtk_editable_set_editable( GTK_EDITABLE(priv->entry), TRUE );
}


//------------------------------------------------------------------------------
// 

void anjuta_symbol_search_set_pixbufs( AnjutaSymbolSearch *search, GdkPixbuf ** pix ) {
	
	AnjutaSymbolSearchPriv      *priv;

	priv = search->priv;
	priv->pixbufs = pix;	
	
}


//------------------------------------------------------------------------------
// 

GtkWidget *anjuta_symbol_search_new( ) { 
	
	AnjutaSymbolSearch          *search;
	AnjutaSymbolSearchPriv      *priv;
	GtkTreeSelection  *selection;
   GtkWidget         *list_sw;
	GtkWidget         *frame;

	// create a new object	
	search = g_object_new (ANJUTA_TYPE_SYMBOL_SEARCH, NULL);
	priv = search->priv;

	// setting some variables to NULL
	priv->original_list = NULL;
	priv->keyword_words = NULL;
	priv->pixbufs = NULL;
	
	gtk_container_set_border_width (GTK_CONTAINER (search), 2);

	// creating entry box, where we'll type the keyword to look for
	priv->entry = gtk_entry_new ();

	// set entry to not-editable till we'll load a project
	gtk_editable_set_editable( GTK_EDITABLE(priv->entry), FALSE );
	
	// set up some signals
	g_signal_connect (priv->entry, "key_press_event",
			  G_CALLBACK (an_symbol_search_on_entry_key_press_event),
			  search);

	g_signal_connect (priv->hitlist, "button_press_event",
			  G_CALLBACK (an_symbol_search_on_tree_button_press),
			  search);

	g_signal_connect (priv->entry, "changed", 
			  G_CALLBACK (an_symbol_search_on_entry_changed),
			  search);

	g_signal_connect (priv->entry, "activate",
			  G_CALLBACK (an_symbol_search_on_entry_activated),
			  search);
	
	g_signal_connect (priv->entry, "insert_text",
			  G_CALLBACK (an_symbol_search_on_entry_text_inserted),
			  search);

	gtk_box_pack_start (GTK_BOX (search), priv->entry, FALSE, FALSE, 0);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	
   list_sw = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (list_sw),
                                   GTK_POLICY_AUTOMATIC, 
                                   GTK_POLICY_AUTOMATIC);

	gtk_container_add (GTK_CONTAINER (frame), list_sw);
	
	gtk_tree_view_insert_column_with_attributes (
		GTK_TREE_VIEW (priv->hitlist), -1,
		_("Section"), gtk_cell_renderer_text_new (),
		"text", 0,
		NULL);

	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (priv->hitlist),
					   FALSE);

	selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW (priv->hitlist) );
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);


	g_signal_connect (selection, "changed",
			  G_CALLBACK (an_symbol_search_on_selection_changed),
			  search);
	
	gtk_container_add (GTK_CONTAINER (list_sw), priv->hitlist);

	gtk_box_pack_end_defaults (GTK_BOX (search), frame);
	

	gtk_widget_show_all (GTK_WIDGET (search));
	
	return GTK_WIDGET (search);
}

void
anjuta_symbol_search_set_search_string (AnjutaSymbolSearch *search, const gchar *str)
{ 
	// FIXME: untested function. Leave this here for a future feature?
	AnjutaSymbolSearchPriv *priv;
	
	g_return_if_fail (ANJUTA_SYMBOL_IS_SEARCH (search));

	priv = search->priv;

	gtk_entry_set_text (GTK_ENTRY (priv->entry), str);
	
	gtk_editable_set_position (GTK_EDITABLE (priv->entry), -1);
	gtk_editable_select_region (GTK_EDITABLE (priv->entry), -1, -1);
}

void
anjuta_symbol_search_grab_focus (AnjutaSymbolSearch *search)
{
	// FIXME: untested function. Leave this here for a future feature?
	AnjutaSymbolSearchPriv *priv;
	
	g_return_if_fail (ANJUTA_SYMBOL_IS_SEARCH (search));

	priv = search->priv;

	gtk_widget_grab_focus (priv->entry);
}
