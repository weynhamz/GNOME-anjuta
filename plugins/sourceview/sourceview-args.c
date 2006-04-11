/***************************************************************************
 *            sourceview-args.c
 *
 *  Di Apr  4 17:09:23 2006
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

#include "sourceview-args.h"
#include "sourceview-prefs.h"
#include "anjuta-document.h"
#include "anjuta-view.h"

#include <gtk/gtktreeview.h>
#include <libanjuta/interfaces/ianjuta-iterable.h>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>
#include <libanjuta/interfaces/ianjuta-symbol.h>
#include <libanjuta/anjuta-debug.h>

#include <pcre.h>

static void sourceview_args_finalize(GObject *object);

enum
{
	COLUMN_SHOW,
	COLUMN_PIXBUF,
	COLUMN_NAME,
	N_COLUMNS
};

struct _SourceviewArgsPrivate {
	gint brace_count;
	IAnjutaSymbolManager* browser;
	AnjutaView* view;
	gchar* current_word;
};

G_DEFINE_TYPE(SourceviewArgs, sourceview_args, TAG_TYPE_WINDOW);

#define WORD_REGEX "[^ \\t&*(]+$"
static gchar* get_current_word(AnjutaDocument* doc)
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
	gtk_text_iter_set_line_offset(line_iter, 0);
	gtk_text_iter_backward_char(&cursor_iter);
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
  	
  	return word;
}

/* Return a tuple containing the (x, y) position of the cursor - 1 line */
static void
get_coordinates(AnjutaView* view, int* x, int* y, const gchar* current_word)
{
	int xor, yor;
	/* We need to Rectangles because if we step to the next line
	the x position is lost */
	GdkRectangle rectx;
	GdkRectangle recty;
	GdkWindow* window;
	GtkTextIter cursor;
	GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	
	gtk_text_buffer_get_iter_at_mark(buffer, &cursor, gtk_text_buffer_get_insert(buffer)); 
	gtk_text_iter_backward_chars(&cursor, g_utf8_strlen(current_word, -1));
	gtk_text_view_get_iter_location(GTK_TEXT_VIEW(view), &cursor, &rectx);
	gtk_text_iter_forward_line(&cursor);
	gtk_text_view_get_iter_location(GTK_TEXT_VIEW(view), &cursor, &recty);
	window = gtk_text_view_get_window(GTK_TEXT_VIEW(view), GTK_TEXT_WINDOW_TEXT);
	gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(view), GTK_TEXT_WINDOW_TEXT, 
		rectx.x + rectx.width, recty.y, x, y);
	
	gdk_window_get_origin(window, &xor, &yor);
	*x = *x + xor;
	*y = *y + yor;
}

static gboolean
sourceview_args_update(TagWindow* tagwin, GtkWidget* view)
{
	gint i;
	GtkTreeIter iter;
	IAnjutaIterable* tags;
	gchar* current_word;
	GtkSourceBuffer* buffer = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)));
	GtkSourceLanguage* lang = gtk_source_buffer_get_language(buffer);
	GSList* mime_types = gtk_source_language_get_mime_types(lang);
	GtkListStore* store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING,
											 GDK_TYPE_PIXBUF, G_TYPE_STRING);
	GtkTreeView* tag_view;
	gboolean is_source = FALSE;
	SourceviewArgs* st = SOURCEVIEW_ARGS(tagwin);
	
	if (!anjuta_preferences_get_int (sourceview_get_prefs(), "enable.code.completion" ))
		return FALSE;
		
	if (tag_window_is_active(tagwin))
	{
		/* User types inside (...) */
		return TRUE;
	}
	
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
	{
		st->priv->brace_count = 0;
		return FALSE;
	}
	current_word = get_current_word(ANJUTA_DOCUMENT(buffer));
	if (current_word == NULL || strlen(current_word) <= 3)
	{
		st->priv->brace_count = 0;
		return FALSE;
	}
	
	g_return_val_if_fail(st->priv->browser != NULL, FALSE);
	
	tags = ianjuta_symbol_manager_search (st->priv->browser, 
											IANJUTA_SYMBOL_TYPE_PROTOTYPE |
											IANJUTA_SYMBOL_TYPE_MACRO_WITH_ARG,
											current_word,
											 FALSE, TRUE, NULL);
	
	if  (!ianjuta_iterable_get_length(tags, NULL))
	{
		st->priv->brace_count = 0;
		g_object_unref(tags);
		return FALSE;
	}
	
	g_object_get(G_OBJECT(st), "view", &tag_view, NULL);
	store = GTK_LIST_STORE(gtk_tree_view_get_model(tag_view));
	gtk_list_store_clear(store);
	
	for (i = 0; i < ianjuta_iterable_get_length(tags, NULL); i++)
	{
	    gchar* show = NULL;
	    IAnjutaSymbol* tag = ianjuta_iterable_get_nth(tags, IANJUTA_TYPE_SYMBOL, i, NULL); 
	    switch (ianjuta_symbol_type(tag, NULL))
	    {
	    	case IANJUTA_SYMBOL_TYPE_FUNCTION:
	    	case IANJUTA_SYMBOL_TYPE_PROTOTYPE:
	    	case IANJUTA_SYMBOL_TYPE_METHOD:
	    		show = g_strdup_printf("%s %s %s", ianjuta_symbol_var_type(tag, NULL),
	    			 ianjuta_symbol_name(tag, NULL), ianjuta_symbol_args(tag, NULL));
	    		break;
	    	case IANJUTA_SYMBOL_TYPE_MACRO_WITH_ARG:
	    	 	show = g_strdup_printf("%s %s",  ianjuta_symbol_name(tag, NULL), 
	    	 		 ianjuta_symbol_args(tag, NULL));
	    	 	break;
	    	 default:
	    	 	continue;
	    }
	    
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, COLUMN_SHOW, show,
        												COLUMN_PIXBUF,  ianjuta_symbol_icon(tag, NULL), 
        												COLUMN_NAME,"", -1);
        g_free(show);
     }
     g_object_unref(tags);
     g_free(current_word);
     if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
     {
     	int x,y;
     	get_coordinates(ANJUTA_VIEW(view), &x,&y, current_word);
     	gtk_window_move(GTK_WINDOW(st), x, y);
     	return TRUE;
     }
     else
     {
     	st->priv->brace_count = 0;
     	return FALSE;
     }
}

static gboolean
sourceview_args_filter_keypress(TagWindow* tags, guint keyval)
{
	SourceviewArgs* args = SOURCEVIEW_ARGS(tags);
	switch (keyval)
	{
		case GDK_Escape:
		{
			args->priv->brace_count = 0;
			return FALSE;
		}
		case GDK_BackSpace:
		{
			GtkTextBuffer* buffer = 
				gtk_text_view_get_buffer(GTK_TEXT_VIEW(SOURCEVIEW_ARGS(tags)->priv->view));
			GtkTextIter cursor;
			GtkTextIter start;
			gchar* text;
			
			gtk_text_buffer_get_iter_at_mark(buffer, &cursor, gtk_text_buffer_get_insert(buffer));
			gtk_text_buffer_get_iter_at_mark(buffer, &start, gtk_text_buffer_get_insert(buffer));			
			gtk_text_iter_backward_char(&start);
			
			text = gtk_text_buffer_get_text(buffer, &start, &cursor, FALSE);
			if (g_str_equal(text, "(") && args->priv->brace_count == 1)
			{
				g_free(text);
				return FALSE;
			}
			g_free(text);
			return TRUE;
		}
		case GDK_parenleft:
		{
			args->priv->brace_count++;
			return TRUE;
		}
		case GDK_parenright:
		{
			if (tag_window_is_active(tags) &&keyval == GDK_parenright)
			{
				if (args->priv->brace_count >= 0)
					args->priv->brace_count--;
			}
		}
		default:
			return args->priv->brace_count;
	}
}

static void sourceview_args_move(TagWindow* tagwin,GtkWidget* view)
{
	/* Do nothing here, we move on update */
}

static void
sourceview_args_class_init(SourceviewArgsClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	TagWindowClass* tag_window_class = TAG_WINDOW_CLASS(klass);

	tag_window_class->update_tags = sourceview_args_update;
	tag_window_class->filter_keypress = sourceview_args_filter_keypress;
	tag_window_class->move = sourceview_args_move;
	object_class->finalize = sourceview_args_finalize;
}

static void
sourceview_args_init(SourceviewArgs *obj)
{
	obj->priv = g_new0(SourceviewArgsPrivate, 1);
	obj->priv->brace_count = 0;
	/* Initialize private members, etc. */
}

static void
sourceview_args_finalize(GObject *object)
{
	SourceviewArgs *cobj;
	cobj = SOURCEVIEW_ARGS(object);
	
	/* Free private members, etc. */
		
	g_free(cobj->priv);
	G_OBJECT_CLASS(sourceview_args_parent_class)->finalize(object);
}

static void
sourceview_args_hide(SourceviewArgs* args)
{
	args->priv->brace_count = 0;
}

SourceviewArgs*
sourceview_args_new(AnjutaPlugin* plugin, AnjutaView* aview)
{
	SourceviewArgs *obj;
	GtkCellRenderer* renderer_text;
	GtkCellRenderer* renderer_pixbuf;
	GtkTreeViewColumn* column_show;
	GtkTreeViewColumn* column_pixbuf;
	GtkListStore* model;
	GtkTreeView* view;
	GtkTreeSelection* selection;
	AnjutaShell* shell;
	gint height;
	
	obj = SOURCEVIEW_ARGS(g_object_new(SOURCEVIEW_TYPE_ARGS, NULL));
	
	g_object_get(G_OBJECT(obj), "view", &view, NULL);
	g_object_set(G_OBJECT(obj), "column", COLUMN_NAME, NULL);
	
	/* Do not allow any selection */
	selection = gtk_tree_view_get_selection(view);
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_NONE);
	
	model = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	gtk_tree_view_set_model(view, GTK_TREE_MODEL(model));
	
	renderer_pixbuf = gtk_cell_renderer_pixbuf_new();
   	column_pixbuf = gtk_tree_view_column_new_with_attributes ("Pixbuf",
                                                   renderer_pixbuf, "pixbuf", COLUMN_PIXBUF, NULL);
    
   	renderer_text = gtk_cell_renderer_text_new();
   	g_object_set(G_OBJECT(renderer_text), "wrap-width", 400, "wrap-mode", PANGO_WRAP_WORD, NULL); 
	column_show = gtk_tree_view_column_new_with_attributes ("Show",
                                                   renderer_text, "text", COLUMN_SHOW, NULL);
                                                   
	gtk_tree_view_append_column (GTK_TREE_VIEW (view), column_pixbuf);
	gtk_tree_view_append_column (GTK_TREE_VIEW (view), column_show);
	
	g_object_get(G_OBJECT(plugin), "shell", &shell, NULL);
	obj->priv->browser = anjuta_shell_get_interface(shell, IAnjutaSymbolManager, NULL);
	obj->priv->view = aview;
	
	g_signal_connect(G_OBJECT(obj), "hide", G_CALLBACK(sourceview_args_hide), NULL);
	
 	g_object_get(G_OBJECT(renderer_text), "height", &height, NULL);
	gtk_widget_set_size_request(GTK_WIDGET(view), -1, height);
	
	return obj;
}
