/*
 * sourceview-tooltip.c (c) 2006 Johannes Schmid
 * Based on the Python-Code from Guillaume Chazarain
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */ 
 
#include "sourceview-tags.h"
#include "sourceview-prefs.h"
#include "tm_tagmanager.h"

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/resources.h>

#include <libgnomevfs/gnome-vfs.h>

static GdkPixbuf **pixbufs = NULL;

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
	pixbufs[(N)] = gdk_pixbuf_new_from_file (pix_file, NULL); \
	g_free (pix_file);

static void
load_pixbufs ()
{
	gchar *pix_file;

	pixbufs = g_new (GdkPixbuf *, sv_max_t + 1);

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
	
	pixbufs[sv_max_t] = NULL;
}

static GdkPixbuf*
get_pixbuf  (const TMTag* tag)
{
	if (pixbufs == NULL)
		load_pixbufs();
	SVNodeType type = anjuta_symbol_info_get_node_type (tag);
	g_return_val_if_fail (type >=0 && type < sv_max_t, NULL);
		
	return pixbufs[type];
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

void sourceview_tags_destroy()
{
	int i;
	for (i=0; i < sv_max_t;i++)
	{
		g_object_unref(pixbufs[i]);
	}
	g_free(pixbufs);
}

#define ENABLE_CODE_COMPLETION "enable.code.completion"

gboolean 
sourceview_tags_update(AnjutaView* view, GtkListStore* store, gchar* current_word)
{
	gint i;
	const GPtrArray* tags =find_tags(current_word);
	GtkSourceBuffer* buffer = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)));
	GtkSourceLanguage* lang = gtk_source_buffer_get_language(buffer);
	GSList* mime_types = gtk_source_language_get_mime_types(lang);
	gboolean is_source = FALSE;
	
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
	
	if (tags == NULL || tags->len <= 0)
		return FALSE;
	
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
	    
	    GdkPixbuf* pixbuf = get_pixbuf  (tag);
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, TAG_WINDOW_COLUMN_SHOW, show,
        												TAG_WINDOW_COLUMN_PIXBUF, pixbuf, 
        												TAG_WINDOW_COLUMN_NAME, tag->name, -1);
        g_free(show);
     }
     return TRUE;
}
