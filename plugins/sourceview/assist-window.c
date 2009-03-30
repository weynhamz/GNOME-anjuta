/***************************************************************************
 *            assist-window.c
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "assist-window.h"
#include "anjuta-view.h"

#include <gtk/gtk.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/resources.h>

#include <string.h>

/* Properties */
enum
{
	ASSIST_WINDOW_COLUMN,
	ASSIST_WINDOW_COLUMN_NUM,
	ASSIST_WINDOW_COLUMN_END
};

static void assist_window_finalize(GObject *object);

struct _AssistWindowPrivate {
	GtkTreeView* view;
	GtkTreeModel* suggestions;
	GtkWidget* scrolled_window;
	GtkTextView* text_view;
	gchar* trigger;
	gint pos;
};

typedef struct _AssistWindowSignal AssistWindowSignal;
typedef enum _AssistWindowSignalType AssistWindowSignalType;

enum _AssistWindowSignalType {
	/* Place Signal Types Here */
	SIGNAL_TYPE_CHOSEN,
	SIGNAL_TYPE_CANCEL,
	LAST_SIGNAL
};

static guint assist_window_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(AssistWindow, assist_window, GTK_TYPE_WINDOW);

static gboolean 
assist_window_expose(GtkWidget* widget, GdkEventExpose* event)
{
	GtkWidget *text_view;
	gint width;
	gint total_items, items, height;
	GdkScreen *screen;
	gint monitor_num;
	GdkRectangle monitor;
	GtkRequisition popup_req;
	gint vert_separator;
  AssistWindow* assistwin = ASSIST_WINDOW(widget);
	GtkTreeModel* model = gtk_tree_view_get_model(assistwin->priv->view);
	GtkTreeViewColumn* column = gtk_tree_view_get_column(assistwin->priv->view, 0);

	g_return_val_if_fail (assistwin->priv->text_view != NULL, FALSE); 
	text_view = GTK_WIDGET(assistwin->priv->text_view);

	total_items = gtk_tree_model_iter_n_children (model, NULL);
	items = MIN (total_items, 5);

	gtk_tree_view_column_cell_get_size (column, NULL,
                                        NULL, NULL, NULL, &height);

	screen = gtk_widget_get_screen (text_view);
	monitor_num = gdk_screen_get_monitor_at_window (screen, text_view->window);
	gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

	gtk_widget_style_get (GTK_WIDGET (assistwin->priv->view),
                          "vertical-separator", &vert_separator,
                          NULL);

	gtk_widget_size_request (GTK_WIDGET (assistwin->priv->view), &popup_req);
	width = popup_req.width;

	if (total_items > items)
	{
		int scrollbar_spacing;
		GtkRequisition scrollbar_req;
		gtk_widget_size_request (GTK_SCROLLED_WINDOW(assistwin->priv->scrolled_window)->vscrollbar,
														 &scrollbar_req);
		gtk_widget_style_get (GTK_WIDGET (assistwin->priv->scrolled_window),
													"scrollbar-spacing", &scrollbar_spacing, NULL);
		width += scrollbar_req.width + scrollbar_spacing;
	}

	width = MAX (width, 100);
	width = MIN (monitor.width, width);
	height = items * (height + vert_separator);
	
	gtk_widget_set_size_request (GTK_WIDGET (assistwin->priv->view),
															 -1, height);
	gtk_widget_set_size_request (GTK_WIDGET (assistwin->priv->scrolled_window),
															 width, -1);

	gtk_window_resize(GTK_WINDOW(assistwin), width, height);

	return (* GTK_WIDGET_CLASS (assist_window_parent_class)->expose_event)(widget, event);
}

static void
assist_window_set_property (GObject * object,
			   guint property_id,
			   const GValue * value, GParamSpec * pspec)
{
//	AssistWindow *self = ASSIST_WINDOW (object);
	g_return_if_fail(value != NULL);
	g_return_if_fail(pspec != NULL);
	
	switch (property_id)
	{
		default:
		{
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
			break;
		}
	}
}

static void
assist_window_get_property (GObject * object,
			   guint property_id,
			   GValue * value, GParamSpec * pspec)
{
//	AssistWindow *self = ASSIST_WINDOW (object);
	
	g_return_if_fail(value != NULL);
	g_return_if_fail(pspec != NULL);
	
	switch (property_id)
	{
		default:
		{
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
			break;
		}
	}
}

static void
assist_window_class_init(AssistWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
	
	object_class->finalize = assist_window_finalize;
	object_class->set_property = assist_window_set_property;
	object_class->get_property = assist_window_get_property;
	
	widget_class->expose_event = assist_window_expose;
	
	assist_window_signals[SIGNAL_TYPE_CHOSEN] = g_signal_new ("chosen",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AssistWindowClass, chosen),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_INT);
	assist_window_signals[SIGNAL_TYPE_CANCEL] = g_signal_new ("cancel",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AssistWindowClass, cancel),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}

static void
assist_activated(GtkTreeView* view, GtkTreePath* path, GtkTreeViewColumn* column,
			  GtkWidget* window)
{
	GtkTreeIter iter;
	gint num;
	GtkTreeModel* model = gtk_tree_view_get_model(view);
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, ASSIST_WINDOW_COLUMN_NUM, &num, -1);
	
	g_signal_emit_by_name(G_OBJECT(window), "chosen", num);
}



static void
assist_window_init(AssistWindow *obj)
{
	GtkWidget* view;
	GtkWidget* scroll;
  GtkTreeViewColumn* column;
  GtkCellRenderer* renderer;
	
	obj->priv = g_new0(AssistWindowPrivate, 1);
	
	view = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(view), FALSE);
	
	g_signal_connect(G_OBJECT(view), "row-activated", G_CALLBACK(assist_activated),
					  obj);
	
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), 
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	
	gtk_container_set_border_width(GTK_CONTAINER(obj), 2);
	gtk_container_add(GTK_CONTAINER(obj), scroll);
	gtk_container_add(GTK_CONTAINER(scroll), view);
	
	obj->priv->view = GTK_TREE_VIEW(view);
	obj->priv->scrolled_window = scroll;
  
  obj->priv->suggestions = GTK_TREE_MODEL(gtk_list_store_new(ASSIST_WINDOW_COLUMN_END,
                                              G_TYPE_STRING, G_TYPE_INT));
  gtk_tree_view_set_model(obj->priv->view, obj->priv->suggestions);
  renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Suggestions", 
                                                    renderer, "text", ASSIST_WINDOW_COLUMN, NULL);
  gtk_tree_view_append_column(obj->priv->view, column);
  
	gtk_window_set_decorated(GTK_WINDOW(obj), FALSE);
	gtk_window_set_type_hint(GTK_WINDOW(obj), GDK_WINDOW_TYPE_HINT_MENU);
	
	gtk_widget_show_all(scroll);
}

static void
assist_window_finalize(GObject *object)
{
	AssistWindow *cobj;
	cobj = ASSIST_WINDOW(object);
	
	/* Free private members, etc. */
	
	g_free(cobj->priv);
	(* G_OBJECT_CLASS (assist_window_parent_class)->finalize) (object);
}

static gboolean assist_window_select(AssistWindow* assistwin)
{
	GtkTreeIter iter;
	GtkTreeModel* model;
	GtkTreeSelection* selection;
	
	if (!GTK_WIDGET_VISIBLE(GTK_WIDGET(assistwin)))
		return FALSE;
	
	selection = gtk_tree_view_get_selection(assistwin->priv->view);
	
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gint num;
		gtk_tree_model_get(model, &iter, ASSIST_WINDOW_COLUMN_NUM, &num, -1);
		g_signal_emit_by_name(assistwin, "chosen", 
						num);
		return TRUE;
	}
	else
		return FALSE;
}

static gboolean assist_window_first(AssistWindow* assistwin)
{
	GtkTreeIter iter;
	GtkTreePath* path;
	GtkTreeModel* model;
	GtkTreeSelection* selection;
	
	if (!GTK_WIDGET_VISIBLE(GTK_WIDGET(assistwin)))
		return FALSE;
	
	selection = gtk_tree_view_get_selection(assistwin->priv->view);
	
	if (gtk_tree_selection_get_mode(selection) == GTK_SELECTION_NONE)
		return FALSE;
	
	model = gtk_tree_view_get_model(assistwin->priv->view);
		
	gtk_tree_model_get_iter_first(model, &iter);
	gtk_tree_selection_select_iter(selection, &iter);
	path = gtk_tree_model_get_path(model, &iter);
	gtk_tree_view_scroll_to_cell(assistwin->priv->view, path, NULL, FALSE, 0, 0);
	gtk_tree_path_free(path);
	return TRUE;
}

static gboolean assist_window_last(AssistWindow* assistwin)
{
	GtkTreeIter iter;
	GtkTreeModel* model;
	GtkTreeSelection* selection;
	GtkTreePath* path;
	gint children;
	
	if (!GTK_WIDGET_VISIBLE(GTK_WIDGET(assistwin)))
		return FALSE;
	
	selection = gtk_tree_view_get_selection(assistwin->priv->view);
	model = gtk_tree_view_get_model(assistwin->priv->view);
	
	if (gtk_tree_selection_get_mode(selection) == GTK_SELECTION_NONE)
		return FALSE;
	
	children = gtk_tree_model_iter_n_children(model, NULL);
	if (children > 0)
	{
		gtk_tree_model_iter_nth_child(model, &iter, NULL, children - 1);
	
		gtk_tree_selection_select_iter(selection, &iter);
		path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_view_scroll_to_cell(assistwin->priv->view, path, NULL, FALSE, 0, 0);
		gtk_tree_path_free(path);
		return TRUE;
	}
	return FALSE;
}

static gboolean assist_window_up(AssistWindow* assistwin, gint rows)
{
	GtkTreeIter iter;
	GtkTreePath* path;
	GtkTreeModel* model;
	GtkTreeSelection* selection;
	
	if (!GTK_WIDGET_VISIBLE(GTK_WIDGET(assistwin)))
		return FALSE;
	
	selection = gtk_tree_view_get_selection(assistwin->priv->view);
	
	if (gtk_tree_selection_get_mode(selection) == GTK_SELECTION_NONE)
		return FALSE;
	
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gint i;
		path = gtk_tree_model_get_path(model, &iter);
		for (i=0; i  < rows; i++)
			gtk_tree_path_prev(path);
		
		if (gtk_tree_model_get_iter(model, &iter, path))
		{
			gtk_tree_selection_select_iter(selection, &iter);
			gtk_tree_view_scroll_to_cell(assistwin->priv->view, path, NULL, FALSE, 0, 0);
		}
		gtk_tree_path_free(path);
		return TRUE;
	}
	return FALSE;
}

static gboolean assist_window_down(AssistWindow* assistwin, gint rows)
{
	GtkTreeIter iter;
	GtkTreeModel* model;
	GtkTreeSelection* selection;
	
	if (!GTK_WIDGET_VISIBLE(GTK_WIDGET(assistwin)))
		return FALSE;
	
	selection = gtk_tree_view_get_selection(assistwin->priv->view);
	
	if (gtk_tree_selection_get_mode(selection) == GTK_SELECTION_NONE)
		return FALSE;
	
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gint i;
		GtkTreePath* path;
		for (i = 0; i < rows; i++)
		{
			if (!gtk_tree_model_iter_next(model, &iter))
				return assist_window_last(assistwin);
		}
			
		gtk_tree_selection_select_iter(selection, &iter);
		path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_view_scroll_to_cell(assistwin->priv->view, path, NULL, FALSE, 0, 0);
		gtk_tree_path_free(path);
		return TRUE;
	}
	else
	{	
		gtk_tree_model_get_iter_first(model, &iter);
		gtk_tree_selection_select_iter(selection, &iter);
	}
	return TRUE;
}

/* Return a tuple containing the (x, y) position of the cursor + 1 line */
static void
get_coordinates(AnjutaView* view, int offset, int* x, int* y)
{
	int xor, yor;
	/* We need to Rectangles because if we step to the next line
	the x position is lost */
	GdkRectangle rectx;
	GdkRectangle recty;
	GdkWindow* window;
	GtkTextIter iter;
	GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	
	gtk_text_buffer_get_iter_at_offset(buffer, &iter, 
									   offset); 
	gtk_text_view_get_iter_location(GTK_TEXT_VIEW(view), &iter, &rectx);
	gtk_text_iter_forward_lines(&iter, 1);
	gtk_text_view_get_iter_location(GTK_TEXT_VIEW(view), &iter, &recty);
	window = gtk_text_view_get_window(GTK_TEXT_VIEW(view), GTK_TEXT_WINDOW_TEXT);
	gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(view), GTK_TEXT_WINDOW_TEXT, 
		rectx.x + rectx.width, recty.y, x, y);
	
	gdk_window_get_origin(window, &xor, &yor);
	*x = *x + xor;
	*y = *y + yor;
}

void
assist_window_move(AssistWindow* assist_win, int offset)
{
	int x,y;
	get_coordinates(ANJUTA_VIEW(assist_win->priv->text_view), offset, &x, &y);	
	gtk_window_move(GTK_WINDOW(assist_win), x, y);
}

 
void assist_window_update(AssistWindow* assistwin, GList* suggestions)
{
	g_return_if_fail(assistwin != NULL);
	GtkListStore* list = GTK_LIST_STORE(assistwin->priv->suggestions);
	GtkTreeIter iter;
	GtkTreeSelection* selection;
	GList* node;
	gtk_list_store_clear(list);
	int i = 0;
	for (node = suggestions; node != NULL; node = g_list_next(node))
	{
		gtk_list_store_append(list, &iter);
		gtk_list_store_set(list, &iter, ASSIST_WINDOW_COLUMN, (gchar*) node->data,
                       ASSIST_WINDOW_COLUMN_NUM, i++, -1);
	}
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(assistwin->priv->view));
	gtk_tree_selection_unselect_all(selection);
	gtk_tree_model_get_iter_first (GTK_TREE_MODEL(list), &iter);
	gtk_tree_selection_select_iter (selection, &iter);
	gtk_widget_queue_draw(GTK_WIDGET(assistwin));
}

/* 
 * Return true if the key was processed and does not need to be passed to the textview,
 * otherwise FALSE
 */
gboolean
assist_window_filter_keypress(AssistWindow* assist_window, guint keyval)
{	
	if (assist_window_is_active(assist_window))
	{
		switch (keyval)
	 	{
	 		case GDK_Down:
			case GDK_Page_Down:
			 {
				return assist_window_down(assist_window, 1);
			}
			case GDK_Up:
			case GDK_Page_Up:
			 {
				return assist_window_up(assist_window, 1);
			}
			case GDK_Home:
			{
				return assist_window_first(assist_window);
			}
			case GDK_End:
			{
				return assist_window_last(assist_window);
			}
			case GDK_Return:
			case GDK_Tab:
			{
				return assist_window_select(assist_window);
			}
			case GDK_Escape:
			{
				 g_signal_emit_by_name(G_OBJECT(assist_window), "cancel");
				 return TRUE;
			}
			case GDK_Right:
			case GDK_KP_Right:
			case GDK_Left:
			case GDK_KP_Left:
			{
				 g_signal_emit_by_name(G_OBJECT(assist_window), "cancel");
				 return FALSE;
			}
			default:
				 return FALSE;
		}
	}
	return FALSE;
}

gboolean assist_window_is_active(AssistWindow* assistwin)
{
	return GTK_WIDGET_VISIBLE(GTK_WIDGET(assistwin));
}

AssistWindow*
assist_window_new(GtkTextView* view, gchar* trigger, gint position)
{
	GtkTextIter iter;
	AssistWindow* assist_win = ASSIST_WINDOW(g_object_new(ASSIST_TYPE_WINDOW, 
																												"type", GTK_WINDOW_POPUP, NULL));
	assist_win->priv->text_view = view;
	if (position == -1)
	{
		gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(view),
                                   &iter,
                                   gtk_text_buffer_get_insert(gtk_text_view_get_buffer(view)));
		assist_win->priv->pos = gtk_text_iter_get_offset(&iter);
	}
	else
		assist_win->priv->pos = position;
	assist_win->priv->trigger = trigger;
	
	assist_window_move(assist_win, assist_win->priv->pos);
	return assist_win;
}


const gchar* 
assist_window_get_trigger(AssistWindow* assist_win)
{
  return assist_win->priv->trigger;
}
gint assist_window_get_position(AssistWindow* assist_win)
{
  return assist_win->priv->pos;
}
