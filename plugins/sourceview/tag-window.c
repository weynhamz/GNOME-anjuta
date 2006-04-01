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

#include <gtk/gtk.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/resources.h>

#include <string.h>

static void tag_window_class_init(TagWindowClass *klass);
static void tag_window_init(TagWindow *sp);
static void tag_window_finalize(GObject *object);

struct _TagWindowPrivate {
	GtkTreeView* view;
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



static void
tag_window_class_init(TagWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
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
	
	/* Free private members, etc. */
	
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
	gtk_tree_model_get(model, &iter, TAG_WINDOW_COLUMN_NAME, &tag_name, -1);
	
	g_signal_emit(window, tag_window_signals[SIGNAL_TYPE_SELECTED], 0, 
					tag_name);
	gtk_widget_hide(window);
}

GtkWidget *
tag_window_new()
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
	
	model = gtk_list_store_new(TAG_WINDOW_N_COLUMNS, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
	
   	renderer_pixbuf = gtk_cell_renderer_pixbuf_new();
   	column_pixbuf = gtk_tree_view_column_new_with_attributes ("Pixbuf",
                                                   renderer_pixbuf, "pixbuf", TAG_WINDOW_COLUMN_PIXBUF, NULL);
   	renderer_text = gtk_cell_renderer_text_new();
	column_show = gtk_tree_view_column_new_with_attributes ("Show",
                                                   renderer_text, "text", TAG_WINDOW_COLUMN_SHOW, NULL);
                                                   
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
	
	gtk_window_set_decorated(GTK_WINDOW(obj), FALSE);
	gtk_window_set_type_hint(GTK_WINDOW(obj), GDK_WINDOW_TYPE_HINT_MENU);
	gtk_widget_set_size_request(view, -1, 150);
	gtk_widget_show_all(scroll);
	
	return obj;
}

gboolean tag_window_up(TagWindow* tagwin)
{
	GtkTreeIter iter;
	GtkTreePath* path;
	GtkTreeModel* model;
	GtkTreeSelection* selection;
	
	if (!GTK_WIDGET_VISIBLE(GTK_WIDGET(tagwin)))
		return FALSE;
	
	selection = gtk_tree_view_get_selection(tagwin->priv->view);
	
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

gboolean tag_window_down(TagWindow* tagwin)
{
	GtkTreeIter iter;
	GtkTreeModel* model;
	GtkTreeSelection* selection;
	
	if (!GTK_WIDGET_VISIBLE(GTK_WIDGET(tagwin)))
		return FALSE;
	
	selection = gtk_tree_view_get_selection(tagwin->priv->view);
	
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
	return TRUE;
}

gboolean tag_window_select(TagWindow* tagwin)
{
	GtkTreeIter iter;
	GtkTreeModel* model;
	GtkTreeSelection* selection;
	
	if (!GTK_WIDGET_VISIBLE(GTK_WIDGET(tagwin)))
		return FALSE;
	
	selection = gtk_tree_view_get_selection(tagwin->priv->view);
	
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gchar* tag_name;
		gtk_tree_model_get(model, &iter, TAG_WINDOW_COLUMN_NAME, &tag_name, -1);
		g_signal_emit(tagwin, tag_window_signals[SIGNAL_TYPE_SELECTED], 0, 
						tag_name);
		gtk_widget_hide(GTK_WIDGET(tagwin));
		return TRUE;
	}
	else
		return FALSE;
}

GtkListStore*
tag_window_get_model(TagWindow* tag_window)
{
	return GTK_LIST_STORE(gtk_tree_view_get_model(tag_window->priv->view));
}
