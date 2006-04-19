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
#include "anjuta-view.h"

#include <gtk/gtk.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/resources.h>

#include <string.h>

/* Properties */
enum
{
	TAG_WINDOW_VIEW = 1,
	TAG_WINDOW_COLUMN,
	TAG_WINDOW_END
};

static void tag_window_finalize(GObject *object);

struct _TagWindowPrivate {
	GtkTreeView* view;
	GtkWidget* scrolled_window;
	GtkWidget* text_view;
	gint	column;
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

G_DEFINE_TYPE(TagWindow, tag_window, GTK_TYPE_WINDOW);

static void
tag_window_move(TagWindow* tag_win, GtkWidget* view);

static gboolean 
tag_window_expose(GtkWidget* widget, GdkEventExpose* event)
{
	GtkWidget *text_view;
    gint width;
    gint total_items, items, height;
    GdkScreen *screen;
    gint monitor_num;
    GdkRectangle monitor;
    GtkRequisition popup_req;
    gint vert_separator;
    TagWindow* tagwin = TAG_WINDOW(widget);
	GtkTreeModel* model = gtk_tree_view_get_model(tagwin->priv->view);
	GtkTreeViewColumn* column = gtk_tree_view_get_column(tagwin->priv->view, 0);

    g_return_val_if_fail (tagwin->priv->text_view != NULL, FALSE); 
    text_view = tagwin->priv->text_view;

    total_items = gtk_tree_model_iter_n_children (model, NULL);
    items = MIN (total_items, 8);

    gtk_tree_view_column_cell_get_size (column, NULL,
                                        NULL, NULL, NULL, &height);

    screen = gtk_widget_get_screen (text_view);
    monitor_num = gdk_screen_get_monitor_at_window (screen, text_view->window);
    gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

    gtk_widget_style_get (GTK_WIDGET (tagwin->priv->view),
                          "vertical-separator", &vert_separator,
                          NULL);

    gtk_widget_size_request (GTK_WIDGET (tagwin->priv->view), &popup_req);
    width = popup_req.width;

    if (total_items > items)
    {
        int scrollbar_spacing;
        GtkRequisition scrollbar_req;
        gtk_widget_size_request (GTK_SCROLLED_WINDOW(tagwin->priv->scrolled_window)->vscrollbar,
                                 &scrollbar_req);
        gtk_widget_style_get (GTK_WIDGET (tagwin->priv->scrolled_window),
                              "scrollbar-spacing", &scrollbar_spacing, NULL);
        width += scrollbar_req.width + scrollbar_spacing;
    }

    width = MAX (width, 100);
    width = MIN (monitor.width, width);

    gtk_widget_set_size_request (GTK_WIDGET (tagwin->priv->view),
                                 -1, items * (height + vert_separator));
    gtk_widget_set_size_request (GTK_WIDGET (tagwin->priv->scrolled_window),
                                 width, -1);

    gtk_widget_set_size_request (widget, -1, -1);
    gtk_widget_size_request (widget, &popup_req);

	return (* GTK_WIDGET_CLASS (tag_window_parent_class)->expose_event)(widget, event);
}

static void
tag_window_set_property (GObject * object,
			   guint property_id,
			   const GValue * value, GParamSpec * pspec)
{
	TagWindow *self = TAG_WINDOW (object);
	g_return_if_fail(value != NULL);
	g_return_if_fail(pspec != NULL);
	
	switch (property_id)
	{
		case TAG_WINDOW_VIEW:
		{
			g_assert("Property view is read-only!");
			break;
		}
		case TAG_WINDOW_COLUMN:
		{
			 self->priv->column = g_value_get_int(value);
			break;
		}
		default:
		{
			g_assert ("Unknown property");
			break;
		}
	}
}

static void
tag_window_get_property (GObject * object,
			   guint property_id,
			   GValue * value, GParamSpec * pspec)
{
	TagWindow *self = TAG_WINDOW (object);
	
	g_return_if_fail(value != NULL);
	g_return_if_fail(pspec != NULL);
	
	switch (property_id)
	{
		case TAG_WINDOW_VIEW:
		{
			g_value_set_object (value, self->priv->view);
			break;
		}
		case TAG_WINDOW_COLUMN:
		{
			g_value_set_int(value, self->priv->column);
			break;
		}
		default:
		{
			g_assert ("Unknown property");
			break;
		}
	}
}

static void
tag_window_class_init(TagWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
	GParamSpec *tag_window_spec_view;
	GParamSpec *tag_window_spec_column;
	
	object_class->finalize = tag_window_finalize;
	object_class->set_property = tag_window_set_property;
	object_class->get_property = tag_window_get_property;
	
	widget_class->expose_event = tag_window_expose;
	
	klass->update_tags = NULL;
	klass->filter_keypress = NULL;
	klass->move = tag_window_move;
	
	tag_window_spec_view = g_param_spec_object ("view",
						       "GtkTreeView of the window",
						       "Add column, etc here",
						       GTK_TYPE_TREE_VIEW,
						       G_PARAM_READABLE);
	g_object_class_install_property (object_class,
					 TAG_WINDOW_VIEW,
					 tag_window_spec_view);
	
	tag_window_spec_column = g_param_spec_int ("column",
						       "Number of TreeViewColumn",
						       "The string to insert on activation should be found there",
								0, 100, 0, G_PARAM_READWRITE);
	g_object_class_install_property (object_class,
					 TAG_WINDOW_COLUMN,
					 tag_window_spec_column);
	
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
tag_activated(GtkTreeView* view, GtkTreePath* path, GtkTreeViewColumn* column,
			  GtkWidget* window)
{
	GtkTreeIter iter;
	gchar* tag_name;
	GtkTreeModel* model = gtk_tree_view_get_model(view);
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, TAG_WINDOW(window)->priv->column, &tag_name, -1);
	
	g_signal_emit(window, tag_window_signals[SIGNAL_TYPE_SELECTED], 0, 
					tag_name);
	gtk_widget_hide(window);
}


static void
tag_window_init(TagWindow *obj)
{
	GtkWidget* view;
	GtkWidget* scroll;
	
	obj->priv = g_new0(TagWindowPrivate, 1);
	
	g_object_set(G_OBJECT(obj), "type", GTK_WINDOW_POPUP, NULL);
	
	view = gtk_tree_view_new();
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
	
	obj->priv->view = GTK_TREE_VIEW(view);
	obj->priv->scrolled_window = scroll;
	
	gtk_window_set_decorated(GTK_WINDOW(obj), FALSE);
	gtk_window_set_type_hint(GTK_WINDOW(obj), GDK_WINDOW_TYPE_HINT_MENU);
	
	gtk_widget_show_all(scroll);
}

static void
tag_window_finalize(GObject *object)
{
	TagWindow *cobj;
	cobj = TAG_WINDOW(object);
	
	/* Free private members, etc. */
	
	g_free(cobj->priv);
	(* G_OBJECT_CLASS (tag_window_parent_class)->finalize) (object);
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
	
	if (gtk_tree_selection_get_mode(selection) == GTK_SELECTION_NONE)
		return FALSE;
	
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
	
	if (gtk_tree_selection_get_mode(selection) == GTK_SELECTION_NONE)
		return FALSE;
	
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
		gtk_tree_model_get(model, &iter, tagwin->priv->column, &tag_name, -1);
		g_signal_emit(tagwin, tag_window_signals[SIGNAL_TYPE_SELECTED], 0, 
						tag_name);
		gtk_widget_hide(GTK_WIDGET(tagwin));
		return TRUE;
	}
	else
		return FALSE;
}

/* Return a tuple containing the (x, y) position of the cursor + 1 line */
static void
get_coordinates(AnjutaView* view, int* x, int* y)
{
	int xor, yor;
	/* We need to Rectangles because if we step to the next line
	the x position is lost */
	GdkRectangle rectx;
	GdkRectangle recty;
	GdkWindow* window;
	GtkTextIter cursor;
	GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	gchar* current_word = anjuta_document_get_current_word(ANJUTA_DOCUMENT(buffer));
	
	g_return_if_fail(current_word != NULL);
	
	gtk_text_buffer_get_iter_at_mark(buffer, &cursor, gtk_text_buffer_get_insert(buffer)); 
	gtk_text_iter_backward_chars(&cursor, g_utf8_strlen(current_word, -1));
	gtk_text_view_get_iter_location(GTK_TEXT_VIEW(view), &cursor, &rectx);
	gtk_text_iter_forward_lines(&cursor, 1);
	gtk_text_view_get_iter_location(GTK_TEXT_VIEW(view), &cursor, &recty);
	window = gtk_text_view_get_window(GTK_TEXT_VIEW(view), GTK_TEXT_WINDOW_TEXT);
	gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(view), GTK_TEXT_WINDOW_TEXT, 
		rectx.x + rectx.width, recty.y, x, y);
	
	gdk_window_get_origin(window, &xor, &yor);
	*x = *x + xor;
	*y = *y + yor;
	g_free(current_word);
}

static void
tag_window_move(TagWindow* tag_win, GtkWidget* view)
{
	int x,y;
	get_coordinates(ANJUTA_VIEW(view), &x, &y);	
	gtk_window_move(GTK_WINDOW(tag_win), x, y);
}

gboolean tag_window_update(TagWindow* tagwin, GtkWidget* view)
{
	TagWindowClass* klass = TAG_WINDOW_GET_CLASS (tagwin);
	
	g_return_val_if_fail(klass != NULL, FALSE);
	g_return_val_if_fail(klass->update_tags != NULL, FALSE);
	
	if (klass->update_tags(tagwin, view))
	{	
		if (!tag_window_is_active(tagwin))
		{	
			tagwin->priv->text_view = view;
			klass->move(tagwin, view);
			gtk_widget_show(GTK_WIDGET(tagwin));
		}
		return TRUE;
	}
	else
	{
		gtk_widget_hide(GTK_WIDGET(tagwin));
		return FALSE;
	}
}

TagWindowKeyPress tag_window_filter_keypress(TagWindow* tag_window, guint keyval)
{
	TagWindowClass* klass = TAG_WINDOW_GET_CLASS (tag_window);
	
	g_return_val_if_fail(klass != NULL, FALSE);
	g_return_val_if_fail(klass->filter_keypress != NULL, FALSE);
	
	
	if (tag_window_is_active(tag_window))
	{
		switch (keyval)
	 	{
	 		case GDK_Down:
	 		case GDK_Page_Down:
			{
				if (tag_window_down(tag_window))
					return TAG_WINDOW_KEY_CONTROL;
				return TAG_WINDOW_KEY_SKIP;
			}
			case GDK_Up:
			case GDK_Page_Up:
			{
				if (tag_window_up(tag_window))
					return TAG_WINDOW_KEY_CONTROL;
				return TAG_WINDOW_KEY_SKIP;
			}
			case GDK_Return:
			case GDK_Tab:
			{
				if (tag_window_select(tag_window))
					return TAG_WINDOW_KEY_CONTROL;
				return TAG_WINDOW_KEY_SKIP;
			}
		}
	}
	if (klass->filter_keypress (tag_window, keyval))
		return TAG_WINDOW_KEY_UPDATE;
	else
		return TAG_WINDOW_KEY_SKIP;
}

gboolean tag_window_is_active(TagWindow* tagwin)
{
	return GTK_WIDGET_VISIBLE(GTK_WIDGET(tagwin));
}
