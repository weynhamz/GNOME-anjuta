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
 
#include <pcre.h> 
 
#define ENABLE_CODE_COMPLETION "enable.code.completion"

static void sourceview_scope_finalize(GObject *object);

G_DEFINE_TYPE(SourceviewScope, sourceview_scope, TAG_TYPE_WINDOW);

typedef enum 
{
	PERIOD,
	ARROW,
	DOUBLECOLON
} ScopeType;

struct _SourceviewScopePrivate
{
	IAnjutaSymbolManager* browser;
	ScopeType type;
	AnjutaView* view;
};

enum
{
	COLUMN_SHOW,
	COLUMN_PIXBUF,
	COLUMN_NAME,
	COLUMN_VISIBLE,
	N_COLUMNS
};


#define WORD_REGEX "[^ \\t!&*]+$"
static gchar* get_current_word(AnjutaDocument* doc, ScopeType type)
{
	pcre *re;
    gint err_offset;
	const gchar* err;
	gint rc, start, end;
	int ovector[2];
	gchar* line, *word;
	GtkTextIter *line_iter, cursor_iter;
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(doc);
	gtk_text_buffer_get_iter_at_mark(buffer, &cursor_iter, 
								 gtk_text_buffer_get_insert(buffer));
	line_iter = gtk_text_iter_copy(&cursor_iter);
	
	/* Check if we have "." or "->" or a "::" */
	switch (type)
	{
		case PERIOD:
			gtk_text_iter_backward_char(&cursor_iter);
			break;
		case ARROW:
			gtk_text_iter_backward_chars(&cursor_iter, 2);
			break;
		case DOUBLECOLON:
			gtk_text_iter_backward_chars(&cursor_iter, 2);
			break;
	}
			
	gtk_text_iter_set_line_offset(line_iter, 0);
	line = gtk_text_buffer_get_text(buffer, line_iter, &cursor_iter, FALSE);
	gtk_text_iter_free(line_iter);
	
	/* Create regular expression */
	re = pcre_compile(WORD_REGEX, 0, &err, &err_offset, NULL);
	
	if (NULL == re)
	{
		/* Compile failed - check error message */
		DEBUG_PRINT("Regex compile failed! %s at position %d", err, err_offset);
		return FALSE;
	}
	
	/* Run the next matching operation */
	rc = pcre_exec(re, NULL, line, strlen(line), 0,
					   0, ovector, 2);

	 if (rc == PCRE_ERROR_NOMATCH)
    {
    	DEBUG_PRINT("No match");
    	return NULL;
    }
 	 else if (rc < 0)
    {
    	DEBUG_PRINT("Matching error %d\n", rc);
    	return NULL;
    }
	else if (rc == 0)
    {
    	DEBUG_PRINT("ovector too small");
    	return NULL;
    }
    start = ovector[0];
    end = ovector[1];
    	    	
    word = g_new0(gchar, end - start + 1);
    strncpy(word, line + start, end - start);
  	
  	DEBUG_PRINT("Found: %s", word);
  	
  	return word;
}

static gboolean
sourceview_scope_create_list(TagWindow* tagwin, GtkWidget* view)
{
	gint i;
	IAnjutaIterable* tags;
	gchar* current_word;
	GtkSourceBuffer* buffer = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)));
	GtkSourceLanguage* lang = gtk_source_buffer_get_language(buffer);
	GSList* mime_types = gtk_source_language_get_mime_types(lang);
	GtkListStore* store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING,
											 GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_BOOLEAN);
	GtkTreeView* tag_view;
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	
	GtkTextBuffer* text_buffer = GTK_TEXT_BUFFER(buffer);	
	gchar *final_text = NULL;
	GtkTextIter cursor_iter;
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
	
	current_word = get_current_word(ANJUTA_DOCUMENT(buffer), st->priv->type);
	DEBUG_PRINT(current_word);
	if (current_word == NULL || strlen(current_word) <= 2)
		return FALSE;
	
	g_return_val_if_fail(st->priv->browser != NULL, FALSE);
	

	gtk_text_buffer_get_iter_at_mark(text_buffer, &cursor_iter, 
								 gtk_text_buffer_get_insert(text_buffer));

	gtk_text_buffer_get_iter_at_offset(text_buffer,
								   &start_iter, 0);
	gtk_text_buffer_get_iter_at_offset(text_buffer,
								   &end_iter, -1);
	
	final_text = gtk_text_buffer_get_text (text_buffer, &start_iter, &end_iter, FALSE);
	
	gchar * file_uri = anjuta_document_get_uri (ANJUTA_DOCUMENT(text_buffer));
	
	tags = ianjuta_symbol_manager_get_completions_at_position (st->priv->browser, 
												file_uri,
												final_text,
												strlen (final_text),
												gtk_text_iter_get_offset (&cursor_iter),
												NULL);
	g_free (final_text);
	
	if (!tags) {
		return FALSE;
	}
		
	if (!ianjuta_iterable_get_length(tags, NULL))
	{
		DEBUG_PRINT ("some went wrong with tags from scope_update, returning.");
		g_object_unref(tags);
		return FALSE;		
	}

	DEBUG_PRINT ("lenght of tags discovered %d",ianjuta_iterable_get_length(tags, NULL));
	
	g_object_get(G_OBJECT(st), "view", &tag_view, NULL);
	store = GTK_LIST_STORE(gtk_tree_view_get_model(tag_view));
	gtk_list_store_clear(store);
	
	for (i = 0; i < ianjuta_iterable_get_length(tags, NULL); i++)
	{
	    GtkTreeIter iter;
	    gchar* show;
	    gchar* name = NULL;
	    IAnjutaSymbol* tag = ianjuta_iterable_get_nth(tags, IANJUTA_TYPE_SYMBOL, i, NULL); 
	    switch (ianjuta_symbol_type(tag, NULL))
	    {
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
	    
	    switch (st->priv->type)
	    {
	    	case PERIOD:
	    		name = g_strdup_printf("%s.%s", current_word,  ianjuta_symbol_name(tag, NULL));
	    		break;
	    	case ARROW:
	    		name = g_strdup_printf("%s->%s", current_word,  ianjuta_symbol_name(tag, NULL));	    	
	    		break;
	    	case DOUBLECOLON:
	    		name = g_strdup_printf("%s::%s", current_word,  ianjuta_symbol_name(tag, NULL));	    	
	    		break;
	    }
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, COLUMN_SHOW, show,
        												COLUMN_PIXBUF,  ianjuta_symbol_icon(tag, NULL), 
        												COLUMN_NAME,name, COLUMN_VISIBLE, TRUE, -1);
        g_free(name);
        g_free(show);
     }
     g_object_unref(tags);
     g_free(current_word);
	 
     return TRUE;
}

static gboolean
sourceview_scope_update_list(TagWindow* tagwin, GtkWidget* view)
{
	gboolean still_items = FALSE;
	gchar* current_word;
	GtkTreeModel* model;
	GtkTreeView* tag_view;
	GtkTreeIter iter;
	
	GtkTextBuffer* text_buffer =gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));	
	SourceviewScope* st = SOURCEVIEW_SCOPE(tagwin);
	
	g_object_get(G_OBJECT(st), "view", &tag_view, NULL);
	model = gtk_tree_view_get_model(tag_view);
	gtk_tree_model_get_iter_first(model, &iter);
	
	current_word = anjuta_document_get_current_word(ANJUTA_DOCUMENT(text_buffer));
	
	while (TRUE)
	{
		gchar* tag;
		gtk_tree_model_get(model, &iter, COLUMN_NAME, &tag, -1);
		if (!g_str_has_prefix(tag, current_word))
		{
			gtk_list_store_set(GTK_LIST_STORE(model), &iter, COLUMN_VISIBLE, FALSE, -1);
			if (!gtk_list_store_remove(GTK_LIST_STORE(model), &iter))
				break;
		}
		else
		{
			still_items = TRUE;
			if (!gtk_tree_model_iter_next(model, &iter))
				break;
		}
	}
	
	return still_items;
}

static gboolean
sourceview_scope_update(TagWindow* tagwin, GtkWidget* view)
{
	if (!tag_window_is_active(tagwin))
		return sourceview_scope_create_list(tagwin, view);
	else
		return sourceview_scope_update_list(tagwin, view);
}

static gboolean
sourceview_scope_filter_keypress(TagWindow* tags, guint keyval)
{
	SourceviewScope* scope = SOURCEVIEW_SCOPE(tags);
	if (tag_window_is_active(tags))
	{
		/* Assume keyval is something like [A-Za-z0-9_]+ */
		if  ((GDK_A <= keyval && keyval <= GDK_Z)
			|| (GDK_a <= keyval && keyval <= GDK_z)
			|| (GDK_0 <= keyval && keyval <= GDK_9)		
			|| GDK_underscore == keyval
			|| GDK_Shift_L == keyval
			|| GDK_Shift_R == keyval)
			{
				return TRUE;
			}
		else
			return FALSE;
	}
	else
	{
		if (keyval == GDK_period)
		{
			scope->priv->type = PERIOD;
			return TRUE;
		}
		else if (keyval == GDK_greater)
		{
			GtkTextBuffer* buffer = 
				gtk_text_view_get_buffer(GTK_TEXT_VIEW(SOURCEVIEW_SCOPE(tags)->priv->view));
			GtkTextIter cursor;
			GtkTextIter start;
			gchar* text;
			
			gtk_text_buffer_get_iter_at_mark(buffer, &cursor, gtk_text_buffer_get_insert(buffer));
			gtk_text_buffer_get_iter_at_mark(buffer, &start, gtk_text_buffer_get_insert(buffer));			
			gtk_text_iter_backward_char(&start);
			
			text = gtk_text_buffer_get_text(buffer, &start, &cursor, FALSE);
			if (g_str_equal(text, "-"))
			{
				g_free(text);
				scope->priv->type = ARROW;
				return TRUE;
			}
			g_free(text);
		}
		else if (keyval == GDK_colon)
		{
			GtkTextBuffer* buffer = 
				gtk_text_view_get_buffer(GTK_TEXT_VIEW(SOURCEVIEW_SCOPE(tags)->priv->view));
			GtkTextIter cursor;
			GtkTextIter start;
			gchar* text;
			
			gtk_text_buffer_get_iter_at_mark(buffer, &cursor, gtk_text_buffer_get_insert(buffer));
			gtk_text_buffer_get_iter_at_mark(buffer, &start, gtk_text_buffer_get_insert(buffer));			
			gtk_text_iter_backward_char(&start);
			
			text = gtk_text_buffer_get_text(buffer, &start, &cursor, FALSE);
			if (g_str_equal(text, ":"))
			{
				g_free(text);
				scope->priv->type = DOUBLECOLON;
				return TRUE;
			}
			g_free(text);
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
	
	model = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_STRING,
												G_TYPE_BOOLEAN);
	gtk_tree_view_set_model(view, GTK_TREE_MODEL(model));
	
	renderer_pixbuf = gtk_cell_renderer_pixbuf_new();
   	column_pixbuf = gtk_tree_view_column_new_with_attributes ("Pixbuf",
                                                   renderer_pixbuf, "pixbuf", COLUMN_PIXBUF, 
                                                   "visible", COLUMN_VISIBLE, NULL);
   	renderer_text = gtk_cell_renderer_text_new();
	column_show = gtk_tree_view_column_new_with_attributes ("Show",
                                                   renderer_text, "text", COLUMN_SHOW, 
                                                   "visible", COLUMN_VISIBLE, NULL);
                                                   
	gtk_tree_view_append_column (GTK_TREE_VIEW (view), column_pixbuf);
	gtk_tree_view_append_column (GTK_TREE_VIEW (view), column_show);
	
	g_object_get(G_OBJECT(plugin), "shell", &shell, NULL);
	obj->priv->browser = anjuta_shell_get_interface(shell, IAnjutaSymbolManager, NULL);
	obj->priv->view = aview;
	
	return obj;
}
