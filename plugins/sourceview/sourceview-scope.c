/***************************************************************************
 *            sourceview-scope.c
 *
 *  So Apr  2 10:56:47 2006
 *  Copyright  2006  Johannes Schmid
 *  jhs@gnome.org
 ***************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "sourceview-scope.h"
#include "sourceview-prefs.h"
#include "anjuta-document.h"

#include <gtk/gtktreeview.h>
#include "tm_tagmanager.h"

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-iterable.h>
#include <libanjuta/interfaces/ianjuta-symbol.h>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>
#include <libanjuta/resources.h>

#include <libgnomevfs/gnome-vfs.h>
 
#define ENABLE_CODE_COMPLETION "enable.code.completion"

static void sourceview_scope_finalize(GObject *object);

G_DEFINE_TYPE(SourceviewScope, sourceview_scope, TAG_TYPE_WINDOW);

struct _SourceviewScopePrivate
{
	IAnjutaSymbolManager* browser;
	AnjutaView* view;
};

enum
{
	COLUMN_SHOW,
	COLUMN_PIXBUF,
	COLUMN_NAME,
	N_COLUMNS
};

static gchar*
get_current_word(GtkTextBuffer* buffer)
{
	GtkTextIter word_end;
	GtkTextIter* word_start;
	gchar* current_word;
	gunichar character;
			
	gtk_text_buffer_get_iter_at_mark(buffer, &word_end, gtk_text_buffer_get_insert(buffer));
	character = gtk_text_iter_get_char(&word_end);
	gtk_text_iter_backward_char(&word_end);
		
	word_start = gtk_text_iter_copy(&word_end);
	gtk_text_iter_backward_word_start(word_start);
	current_word = gtk_text_buffer_get_text(buffer, word_start, &word_end, FALSE);
	gtk_text_iter_free(word_start);
	return current_word;
}

static gboolean
sourceview_scope_update(TagWindow* tagwin, GtkWidget* view)
{
	gint i;
	IAnjutaIterable* tags;
	gchar* current_word;
	GtkSourceBuffer* buffer = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)));
	GtkSourceLanguage* lang = gtk_source_buffer_get_language(buffer);
	GSList* mime_types = gtk_source_language_get_mime_types(lang);
	GtkListStore* store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING,
											 GDK_TYPE_PIXBUF, G_TYPE_STRING);
	GtkTreeView* tag_view;
	gboolean is_source = FALSE;
	SourceviewScope* st = SOURCEVIEW_SCOPE(tagwin);
	
	if (!anjuta_preferences_get_int (sourceview_get_prefs(), "enable.code.completion" ))
		return FALSE;
	
	while(mime_types)
	{
		if (g_str_equal(mime_types->data, "text/x-c")
			|| g_str_equal(mime_types->data, "text/x-c++"))
		{
			is_source = TRUE;
			break;
		}
		mime_types = g_slist_next(mime_types);
	}
	if (!is_source)
		return FALSE;
	
	current_word = get_current_word(GTK_TEXT_BUFFER(buffer));
	DEBUG_PRINT(current_word);
	if (current_word == NULL || strlen(current_word) <= 2)
		return FALSE;
	
	g_return_val_if_fail(st->priv->browser != NULL, FALSE);
	
	tags = ianjuta_symbol_manager_get_members (st->priv->browser,
											current_word,
											 TRUE, NULL);
	
	if  (!tags || !ianjuta_iterable_get_length(tags, NULL))
	{
		return FALSE;
		g_object_unref(tags);
	}
	
	g_object_get(G_OBJECT(st), "view", &tag_view, NULL);
	store = GTK_LIST_STORE(gtk_tree_view_get_model(tag_view));
	gtk_list_store_clear(store);
	
	for (i = 0; i < ianjuta_iterable_get_length(tags, NULL); i++)
	{
	    GtkTreeIter iter;
	    gchar* show;
	    IAnjutaSymbol* tag = ianjuta_iterable_get_nth(tags, IANJUTA_TYPE_SYMBOL, i, NULL); 
	    switch (ianjuta_symbol_type(tag, NULL))
	    {
	    	case IANJUTA_SYMBOL_TYPE_FUNCTION:
	    	case IANJUTA_SYMBOL_TYPE_METHOD:
	    	case IANJUTA_SYMBOL_TYPE_PROTOTYPE:
	    		show = g_strdup_printf("%s %s ()", ianjuta_symbol_var_type(tag, NULL),
	    			 ianjuta_symbol_name(tag, NULL));
	    		break;
	    	case IANJUTA_SYMBOL_TYPE_MACRO_WITH_ARG:
	    	 	show = g_strdup_printf("%s %s",  ianjuta_symbol_name(tag, NULL), 
	    	 		 ianjuta_symbol_args(tag, NULL));
	    	 	break;
	    	 default:
	    	 	show = g_strdup(ianjuta_symbol_name(tag, NULL));
	    }
	    
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, COLUMN_SHOW, show,
        												COLUMN_PIXBUF,  ianjuta_symbol_icon(tag, NULL), 
        												COLUMN_NAME, ianjuta_symbol_name(tag, NULL), -1);
        g_free(show);
     }
     g_object_unref(tags);
     g_free(current_word);
     return TRUE;
}

static gboolean
sourceview_scope_filter_keypress(TagWindow* tags, guint keyval)
{
	if (tag_window_is_active(tags))
	{
			return FALSE;
	}
	else
	{
		if (keyval == GDK_period)
			return TRUE;
		else if (keyval == GDK_rightarrow)
		{
			GtkTextBuffer* buffer = 
				gtk_text_view_get_buffer(GTK_TEXT_VIEW(SOURCEVIEW_SCOPE(tags)->priv->view));
			GtkTextIter cursor;
			gunichar character;
			
			gtk_text_buffer_get_iter_at_mark(buffer, &cursor, gtk_text_buffer_get_insert(buffer));
			character = gtk_text_iter_get_char(&cursor);
			if (character == '-')
				return TRUE;
		}
	}
	return FALSE;
}

static void
sourceview_scope_class_init(SourceviewScopeClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	TagWindowClass *tag_window_class = TAG_WINDOW_CLASS(klass);
	
	object_class->finalize = sourceview_scope_finalize;
	
	tag_window_class->update_tags = sourceview_scope_update;
	tag_window_class->filter_keypress = sourceview_scope_filter_keypress;
}

static void
sourceview_scope_init(SourceviewScope *obj)
{
	obj->priv = g_new0(SourceviewScopePrivate, 1);
	/* Initialize private members, etc. */
}

static void
sourceview_scope_finalize(GObject *object)
{
	SourceviewScope *cobj;
	cobj = SOURCEVIEW_SCOPE(object);
	
	g_free(cobj->priv);
	
	(* G_OBJECT_CLASS (sourceview_scope_parent_class)->finalize) (object);
}

SourceviewScope *
sourceview_scope_new(AnjutaPlugin* plugin, AnjutaView* aview)
{
	SourceviewScope *obj;
	GtkCellRenderer* renderer_text;
	GtkCellRenderer* renderer_pixbuf;
	GtkTreeViewColumn* column_show;
	GtkTreeViewColumn* column_pixbuf;
	GtkListStore* model;
	GtkTreeView* view;
	AnjutaShell* shell;
	
	obj = SOURCEVIEW_SCOPE(g_object_new(SOURCEVIEW_TYPE_SCOPE, NULL));
	
	g_object_get(G_OBJECT(obj), "view", &view, NULL);
	g_object_set(G_OBJECT(obj), "column", COLUMN_NAME, NULL);
	
	model = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	gtk_tree_view_set_model(view, GTK_TREE_MODEL(model));
	
	renderer_pixbuf = gtk_cell_renderer_pixbuf_new();
   	column_pixbuf = gtk_tree_view_column_new_with_attributes ("Pixbuf",
                                                   renderer_pixbuf, "pixbuf", COLUMN_PIXBUF, NULL);
   	renderer_text = gtk_cell_renderer_text_new();
	column_show = gtk_tree_view_column_new_with_attributes ("Show",
                                                   renderer_text, "text", COLUMN_SHOW, NULL);
                                                   
	gtk_tree_view_append_column (GTK_TREE_VIEW (view), column_pixbuf);
	gtk_tree_view_append_column (GTK_TREE_VIEW (view), column_show);
	
	g_object_get(G_OBJECT(plugin), "shell", &shell, NULL);
	obj->priv->browser = anjuta_shell_get_interface(shell, IAnjutaSymbolManager, NULL);
	obj->priv->view = aview;
	
	return obj;
}
