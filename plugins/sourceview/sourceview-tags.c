/***************************************************************************
 *            sourceview-tags.c
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

#include "sourceview-tags.h"
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

static void sourceview_tags_finalize(GObject *object);

G_DEFINE_TYPE(SourceviewTags, sourceview_tags, TAG_TYPE_WINDOW);

struct _SourceviewTagsPrivate
{
	IAnjutaSymbolManager* browser;
};

enum
{
	COLUMN_SHOW,
	COLUMN_PIXBUF,
	COLUMN_NAME,
	N_COLUMNS
};

static gboolean
sourceview_tags_update(TagWindow* tagwin, GtkWidget* view)
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
	SourceviewTags* st = SOURCEVIEW_TAGS(tagwin);
	
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
	current_word = anjuta_document_get_current_word(ANJUTA_DOCUMENT(buffer));
	if (current_word == NULL || strlen(current_word) <= 3)
		return FALSE;
	
	g_return_val_if_fail(st->priv->browser != NULL, FALSE);
	
	tags = ianjuta_symbol_manager_search (st->priv->browser, 
											IANJUTA_SYMBOL_TYPE_ENUM |
											IANJUTA_SYMBOL_TYPE_STRUCT |
											IANJUTA_SYMBOL_TYPE_CLASS |
											IANJUTA_SYMBOL_TYPE_PROTOTYPE |
											IANJUTA_SYMBOL_TYPE_FUNCTION |
											IANJUTA_SYMBOL_TYPE_MACRO_WITH_ARG,
											current_word,
											 TRUE, TRUE, NULL);
	
	if  (!ianjuta_iterable_get_length(tags, NULL))
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
sourceview_tags_filter_keypress(TagWindow* tags, guint keyval)
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

static void
sourceview_tags_class_init(SourceviewTagsClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	TagWindowClass *tag_window_class = TAG_WINDOW_CLASS(klass);
	
	object_class->finalize = sourceview_tags_finalize;
	
	tag_window_class->update_tags = sourceview_tags_update;
	tag_window_class->filter_keypress = sourceview_tags_filter_keypress;
}

static void
sourceview_tags_init(SourceviewTags *obj)
{
	obj->priv = g_new0(SourceviewTagsPrivate, 1);
	/* Initialize private members, etc. */
}

static void
sourceview_tags_finalize(GObject *object)
{
	SourceviewTags *cobj;
	cobj = SOURCEVIEW_TAGS(object);
	
	g_free(cobj->priv);
	
	(* G_OBJECT_CLASS (sourceview_tags_parent_class)->finalize) (object);
}

SourceviewTags *
sourceview_tags_new(AnjutaPlugin* plugin)
{
	SourceviewTags *obj;
	GtkCellRenderer* renderer_text;
	GtkCellRenderer* renderer_pixbuf;
	GtkTreeViewColumn* column_show;
	GtkTreeViewColumn* column_pixbuf;
	GtkListStore* model;
	GtkTreeView* view;
	AnjutaShell* shell;
	
	obj = SOURCEVIEW_TAGS(g_object_new(SOURCEVIEW_TYPE_TAGS, NULL));
	
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
	
	return obj;
}
