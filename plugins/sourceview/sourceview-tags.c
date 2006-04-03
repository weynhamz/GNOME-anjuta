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
#include <libanjuta/resources.h>

#include <libgnomevfs/gnome-vfs.h>
 
#define ENABLE_CODE_COMPLETION "enable.code.completion"

static void sourceview_tags_class_init(SourceviewTagsClass *klass);
static void sourceview_tags_init(SourceviewTags *st);
static void sourceview_tags_finalize(GObject *object);

enum
{
	COLUMN_SHOW,
	COLUMN_PIXBUF,
	COLUMN_NAME,
	N_COLUMNS
};

typedef enum
{
	sv_none_t,
	sv_class_t,
	sv_struct_t,
	sv_union_t,
	sv_typedef_t,
	sv_function_t,
	sv_variable_t,
	sv_enumerator_t,
	sv_macro_t,
	sv_private_func_t,
	sv_private_var_t,
	sv_protected_func_t,
	sv_protected_var_t,
	sv_public_func_t,
	sv_public_var_t,
	sv_max_t
} SVNodeType;

struct _SourceviewTagsPrivate {
	GdkPixbuf** pixbufs;
};

G_DEFINE_TYPE(SourceviewTags, sourceview_tags, TAG_TYPE_WINDOW);

static SVNodeType
anjuta_symbol_info_get_node_type ( const TMTag *tag)
{
	TMTagType t_type;
	SVNodeType type;
	char access;
	
	if ( tag == NULL)
		return sv_none_t;

	t_type = tag->type;
	
	if (t_type == tm_tag_file_t)
		return sv_none_t;
	
	access = tag->atts.entry.access;
	
	switch (t_type)
	{
	case tm_tag_class_t:
		type = sv_class_t;
		break;
	case tm_tag_struct_t:
		type = sv_struct_t;
		break;
	case tm_tag_union_t:
		type = sv_union_t;
		break;
	case tm_tag_function_t:
	case tm_tag_prototype_t:
	case tm_tag_method_t:
		switch (access)
		{
		case TAG_ACCESS_PRIVATE:
			type = sv_private_func_t;
			break;
		case TAG_ACCESS_PROTECTED:
			type = sv_protected_func_t;
			break;
		case TAG_ACCESS_PUBLIC:
			type = sv_public_func_t;
			break;
		default:
			type = sv_function_t;
			break;
		}
		break;
	case tm_tag_member_t:
	case tm_tag_field_t:
		switch (access)
		{
		case TAG_ACCESS_PRIVATE:
			type = sv_private_var_t;
			break;
		case TAG_ACCESS_PROTECTED:
			type = sv_protected_var_t;
			break;
		case TAG_ACCESS_PUBLIC:
			type = sv_public_var_t;
			break;
		default:
			type = sv_variable_t;
			break;
		}
		break;
	case tm_tag_externvar_t:
	case tm_tag_variable_t:
		type = sv_variable_t;
		break;
	case tm_tag_macro_t:
	case tm_tag_macro_with_arg_t:
		type = sv_macro_t;
		break;
	case tm_tag_typedef_t:
		type = sv_typedef_t;
		break;
	case tm_tag_enumerator_t:
		type = sv_enumerator_t;
		break;
	default:
		type = sv_none_t;
		break;
	}
	return type;
}

#define CREATE_SV_ICON(N, F) \
	pix_file = anjuta_res_get_pixmap_file (F); \
	st->priv->pixbufs[(N)] = gdk_pixbuf_new_from_file (pix_file, NULL); \
	g_free (pix_file);

static void
sourceview_tags_load_pixbufs (SourceviewTags* st)
{
	gchar *pix_file;

	st->priv->pixbufs = g_new (GdkPixbuf *, sv_max_t + 1);

	CREATE_SV_ICON (sv_none_t,              "Icons.16x16.Literal");
	CREATE_SV_ICON (sv_class_t,             "Icons.16x16.Class");
	CREATE_SV_ICON (sv_struct_t,            "Icons.16x16.ProtectedStruct");
	CREATE_SV_ICON (sv_union_t,             "Icons.16x16.PrivateStruct");
	CREATE_SV_ICON (sv_typedef_t,           "Icons.16x16.Reference");
	CREATE_SV_ICON (sv_function_t,          "Icons.16x16.Method");
	CREATE_SV_ICON (sv_variable_t,          "Icons.16x16.Literal");
	CREATE_SV_ICON (sv_enumerator_t,        "Icons.16x16.Enum");
	CREATE_SV_ICON (sv_macro_t,             "Icons.16x16.Field");
	CREATE_SV_ICON (sv_private_func_t,      "Icons.16x16.PrivateMethod");
	CREATE_SV_ICON (sv_private_var_t,       "Icons.16x16.PrivateProperty");
	CREATE_SV_ICON (sv_protected_func_t,    "Icons.16x16.ProtectedMethod");
	CREATE_SV_ICON (sv_protected_var_t,     "Icons.16x16.ProtectedProperty");
	CREATE_SV_ICON (sv_public_func_t,       "Icons.16x16.InternalMethod");
	CREATE_SV_ICON (sv_public_var_t,        "Icons.16x16.InternalProperty");
	
	st->priv->pixbufs[sv_max_t] = NULL;
}

static GdkPixbuf*
sourceview_tags_get_pixbuf  (SourceviewTags* st, const TMTag* tag)
{
	if (st->priv->pixbufs == NULL)
		sourceview_tags_load_pixbufs(st);
	SVNodeType type = anjuta_symbol_info_get_node_type (tag);
	g_return_val_if_fail (type >=0 && type < sv_max_t, NULL);
		
	return st->priv->pixbufs[type];
}

static const GPtrArray*
find_tags (const gchar* current_word)
{
	TMTagAttrType attrs[] =
	{
		tm_tag_attr_name_t, tm_tag_attr_type_t, tm_tag_attr_none_t
	};
	
	g_return_val_if_fail(current_word != NULL, NULL);
	
	if (strlen(current_word) <= 3)
		return NULL;
	
	return tm_workspace_find(current_word,
											  tm_tag_enumerator_t |
											  tm_tag_struct_t |
											  tm_tag_class_t |
											  tm_tag_prototype_t |
											  tm_tag_function_t |
											  tm_tag_macro_with_arg_t,
											  attrs, TRUE, TRUE);
}

static gboolean
sourceview_tags_update(TagWindow* tagwin, GtkWidget* view)
{
	gint i;
	const GPtrArray* tags;
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
	
	tags = find_tags(current_word);
	
	if (tags == NULL || tags->len <= 0)
		return FALSE;
	
	g_object_get(G_OBJECT(st), "view", &tag_view, NULL);
	store = GTK_LIST_STORE(gtk_tree_view_get_model(tag_view));
	gtk_list_store_clear(store);
	
	for (i = 0; i < tags->len; i++)
	{
	    GtkTreeIter iter;
	    gchar* show;
	    TMTag* tag = TM_TAG(g_ptr_array_index(tags, i));
	    switch (tag->type)
	    {
	    	case tm_tag_function_t:
	    	case tm_tag_method_t:
	    	case tm_tag_prototype_t:
	    		/* Nice but too long
	    		show = g_strdup_printf("%s %s %s", tag->atts.entry.var_type, tag->name, tag->atts.entry.arglist);*/
	    		show = g_strdup_printf("%s %s ()", tag->atts.entry.var_type, tag->name);
	    		break;
	    	case tm_tag_macro_with_arg_t:
	    	 	show = g_strdup_printf("%s %s", tag->name, tag->atts.entry.arglist);
	    	 	break;
	    	 default:
	    	 	show = g_strdup(tag->name);
	    }
	    
	    GdkPixbuf* pixbuf = sourceview_tags_get_pixbuf  (st, tag);
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, COLUMN_SHOW, show,
        												COLUMN_PIXBUF, pixbuf, 
        												COLUMN_NAME, tag->name, -1);
        g_free(show);
     }
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
	int i;
	
	if (cobj->priv->pixbufs != NULL)
	{
		for (i=0; i < sv_max_t; i++)
		{
			g_object_unref(cobj->priv->pixbufs[i]);
		}
	}
	g_free(cobj->priv->pixbufs);
	g_free(cobj->priv);
	
	(* G_OBJECT_CLASS (sourceview_tags_parent_class)->finalize) (object);
}

SourceviewTags *
sourceview_tags_new()
{
	SourceviewTags *obj;
	GtkCellRenderer* renderer_text;
	GtkCellRenderer* renderer_pixbuf;
	GtkTreeViewColumn* column_show;
	GtkTreeViewColumn* column_pixbuf;
	GtkListStore* model;
	GtkTreeView* view;
	
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
	
	return obj;
}
