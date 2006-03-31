/***************************************************************************
 *            tag-window.c
 *
 *  Mi Mär 29 23:23:09 2006
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

#include "tag-window.h"

#include "tm_tagmanager.h"

#include <gtk/gtk.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/resources.h>

#include <string.h>

enum
{
	COLUMN_SHOW = 0,
	COLUMN_PIXBUF,
	COLUMN_NAME,
	N_COLUMNS
};

static void tag_window_class_init(TagWindowClass *klass);
static void tag_window_init(TagWindow *sp);
static void tag_window_finalize(GObject *object);

struct _TagWindowPrivate {
	GtkTreeView* view;
	GdkPixbuf **pixbufs;
};

typedef struct _TagWindowSignal TagWindowSignal;
typedef enum _TagWindowSignalType TagWindowSignalType;

enum _TagWindowSignalType {
	/* Place Signal Types Here */
	SIGNAL_TYPE_SELECTED,
	LAST_SIGNAL
};

struct _TagWindowSignal {
	TagWindow *object;
};

static guint tag_window_signals[LAST_SIGNAL] = { 0 };
static GtkWindowClass *parent_class = NULL;

G_DEFINE_TYPE(TagWindow, tag_window, GTK_TYPE_WINDOW);

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
	tag_window->priv->pixbufs[(N)] = gdk_pixbuf_new_from_file (pix_file, NULL); \
	g_free (pix_file);

static void
tag_window_load_pixbufs (TagWindow *tag_window)
{
	gchar *pix_file;

	tag_window->priv->pixbufs = g_new (GdkPixbuf *, sv_max_t + 1);

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
	
	tag_window->priv->pixbufs[sv_max_t] = NULL;
}

static GdkPixbuf*
tag_window_get_pixbuf  (TagWindow* tag_window, const TMTag* tag)
{
	SVNodeType type = anjuta_symbol_info_get_node_type (tag);
	g_return_val_if_fail (type >=0 && type < sv_max_t, NULL);
		
	return tag_window->priv->pixbufs[type];
}

static void
tag_window_class_init(TagWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass   *widget_class = GTK_WIDGET_CLASS (klass);
	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = tag_window_finalize;
	
	tag_window_signals[SIGNAL_TYPE_SELECTED] = g_signal_new ("selected",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TagWindowClass, selected),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_STRING);
}

static void
tag_window_init(TagWindow *obj)
{
	obj->priv = g_new0(TagWindowPrivate, 1);
	/* Initialize private members, etc. */
}

static void
tag_window_finalize(GObject *object)
{
	TagWindow *cobj;
	cobj = TAG_WINDOW(object);
	gint i;
	
	/* Free private members, etc. */
	for (i=0; i < sv_max_t; i++)
		g_object_unref(cobj->priv->pixbufs[i]);
	g_free(cobj->priv->pixbufs);
	
	g_free(cobj->priv);
	G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void
tag_activated(GtkTreeView* view, GtkTreePath* path, GtkTreeViewColumn* column,
			  GtkWidget* window)
{
	GtkTreeIter iter;
	gchar* tag_name;
	GtkTreeModel* model = gtk_tree_view_get_model(view);
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, COLUMN_NAME, &tag_name, -1);
	
	g_signal_emit(window, tag_window_signals[SIGNAL_TYPE_SELECTED], 0, 
					tag_name);
	gtk_widget_hide(window);
	gtk_widget_destroy(window);
}

void 
tag_window_update(TagWindow* tagwin, const GPtrArray* tags)
{
	gint i;
	GtkTreeModel* model = gtk_tree_view_get_model(tagwin->priv->view);
	gtk_list_store_clear(GTK_LIST_STORE(model));
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
	    GdkPixbuf* pixbuf = tag_window_get_pixbuf  (tagwin, tag);
        gtk_list_store_append(GTK_LIST_STORE(model), &iter);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, COLUMN_SHOW, show,
        	COLUMN_PIXBUF, pixbuf, COLUMN_NAME, tag->name, -1);
        g_free(show);
     }
}

GtkWidget *
tag_window_new(const GPtrArray* tags)
{
	GtkWidget* view;
	GtkWidget* scroll;
	GtkCellRenderer* renderer_text;
	GtkCellRenderer* renderer_pixbuf;
	GtkTreeViewColumn* column_show;
	GtkTreeViewColumn* column_pixbuf;
	GtkListStore* model;
	GtkWidget *obj;
	
	obj = GTK_WIDGET(
		g_object_new(TAG_TYPE_WINDOW, "type", GTK_WINDOW_POPUP, NULL));
	
	tag_window_load_pixbufs(TAG_WINDOW(obj));
	
	model = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
	
   	renderer_pixbuf = gtk_cell_renderer_pixbuf_new();
   	column_pixbuf = gtk_tree_view_column_new_with_attributes ("Pixbuf",
                                                   renderer_pixbuf, "pixbuf", COLUMN_PIXBUF, NULL);
   	renderer_text = gtk_cell_renderer_text_new();
	column_show = gtk_tree_view_column_new_with_attributes ("Show",
                                                   renderer_text, "text", COLUMN_SHOW, NULL);
                                                   
	gtk_tree_view_append_column (GTK_TREE_VIEW (view), column_pixbuf);
	gtk_tree_view_append_column (GTK_TREE_VIEW (view), column_show);
	
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(view), FALSE);
	
	g_signal_connect(G_OBJECT(view), "row-activated", G_CALLBACK(tag_activated),
					  obj);
	
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), 
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	
	gtk_container_set_border_width(GTK_CONTAINER(obj), 2);
	gtk_container_add(GTK_CONTAINER(obj), scroll);
	gtk_container_add(GTK_CONTAINER(scroll), view);
	
	TAG_WINDOW(obj)->priv->view = GTK_TREE_VIEW(view);
	
	tag_window_update(TAG_WINDOW(obj), tags);
	
	gtk_window_set_decorated(GTK_WINDOW(obj), FALSE);
	gtk_window_set_type_hint(GTK_WINDOW(obj), GDK_WINDOW_TYPE_HINT_MENU);
	gtk_widget_set_size_request(view, -1, 150);
	gtk_widget_show_all(obj);
	
	return obj;
}

gboolean tag_window_up(TagWindow* tagwin)
{
	GtkTreeIter iter;
	GtkTreePath* path;
	GtkTreeModel* model;
	GtkTreeSelection* selection = gtk_tree_view_get_selection(tagwin->priv->view);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_path_prev(path);
		
		if (gtk_tree_model_get_iter(model, &iter, path))
		{
			gtk_tree_selection_select_iter(selection, &iter);
			gtk_tree_view_scroll_to_cell(tagwin->priv->view, path, NULL, FALSE, 0, 0);
		}
		gtk_tree_path_free(path);
		return TRUE;
	}
	return FALSE;
}

void tag_window_down(TagWindow* tagwin)
{
	GtkTreeIter iter;
	GtkTreeModel* model;
	GtkTreeSelection* selection = gtk_tree_view_get_selection(tagwin->priv->view);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		if (gtk_tree_model_iter_next(model, &iter))
		{
			GtkTreePath* path;
			gtk_tree_selection_select_iter(selection, &iter);
			path = gtk_tree_model_get_path(model, &iter);
			gtk_tree_view_scroll_to_cell(tagwin->priv->view, path, NULL, FALSE, 0, 0);
			gtk_tree_path_free(path);
		}
	}
	else
	{
		gtk_tree_model_get_iter_first(model, &iter);
		gtk_tree_selection_select_iter(selection, &iter);
	}
}

gboolean tag_window_select(TagWindow* tagwin)
{
	GtkTreeIter iter;
	GtkTreeModel* model;
	GtkTreeSelection* selection = gtk_tree_view_get_selection(tagwin->priv->view);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gchar* tag_name;
		gtk_tree_model_get(model, &iter, COLUMN_NAME, &tag_name, -1);
		g_signal_emit(tagwin, tag_window_signals[SIGNAL_TYPE_SELECTED], 0, 
						tag_name);
		gtk_widget_hide(GTK_WIDGET(tagwin));
		gtk_widget_destroy(GTK_WIDGET(tagwin));
		return TRUE;
	}
	else
		return FALSE;
}
	
