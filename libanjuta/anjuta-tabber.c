/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-notebook-tabber
 * Copyright (C) Johannes Schmid 2010 <jhs@gnome.org>
 * 
 * anjuta-notebook-tabber is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta-notebook-tabber is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "anjuta-tabber.h"

struct _AnjutaTabberPriv
{
	GtkNotebook* notebook;
	GList* children;
	gint active_page;

	GdkWindow* event_window;
};

enum
{
	PROP_0,
	PROP_NOTEBOOK
};

#define ANJUTA_TABBER_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), ANJUTA_TYPE_TABBER, AnjutaTabberPriv))


G_DEFINE_TYPE (AnjutaTabber, anjuta_tabber, GTK_TYPE_CONTAINER);

static void
anjuta_tabber_init (AnjutaTabber *object)
{
	AnjutaTabber* tabber = ANJUTA_TABBER (object);
	tabber->priv = ANJUTA_TABBER_GET_PRIVATE (tabber);
	tabber->priv->children = NULL;
	tabber->priv->active_page = 0;

	GTK_WIDGET_SET_FLAGS (GTK_WIDGET(tabber), GTK_NO_WINDOW);
}

static void
anjuta_tabber_finalize (GObject *object)
{
	G_OBJECT_CLASS (anjuta_tabber_parent_class)->finalize (object);
}

/**
 * anjuta_tabber_notebook_page_removed:
 *
 * Called when a page is removed from the associated notebook. Removes
 * the tab for this page
 */
static void
anjuta_tabber_notebook_page_removed (GtkNotebook* notebook,
                                     GtkWidget* notebook_child,
                                     gint page_num,
                                     AnjutaTabber* tabber)
{
	GtkWidget* child = g_list_nth_data (tabber->priv->children, page_num);
	gtk_container_remove (GTK_CONTAINER (tabber), child);
}

static void
anjuta_tabber_notebook_switch_page (GtkNotebook* notebook,
                                    GtkNotebookPage* page,
                                    guint page_num,
                                    AnjutaTabber* tabber)
{
	tabber->priv->active_page = page_num;
	gtk_widget_queue_draw (GTK_WIDGET (tabber));
}

/**
 * anjuta_tabber_connect_notebook:
 *
 * Connect signals to associated notebook
 *
 */
static void
anjuta_tabber_connect_notebook (AnjutaTabber* tabber)
{
	g_signal_connect (tabber->priv->notebook, "page-removed", 
	                  G_CALLBACK(anjuta_tabber_notebook_page_removed), tabber);
	g_signal_connect (tabber->priv->notebook, "switch-page", 
	                  G_CALLBACK(anjuta_tabber_notebook_switch_page), tabber);
	
}

static void
anjuta_tabber_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{	
	g_return_if_fail (ANJUTA_IS_TABBER (object));

	AnjutaTabber* tabber = ANJUTA_TABBER (object);

	switch (prop_id)
	{
	case PROP_NOTEBOOK:
		tabber->priv->notebook = g_value_get_object (value);
		anjuta_tabber_connect_notebook (tabber);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
anjuta_tabber_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	g_return_if_fail (ANJUTA_IS_TABBER (object));

	switch (prop_id)
	{
	case PROP_NOTEBOOK:
		g_value_set_object (value, object);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/**
 * anjuta_tabber_get_padding:
 * @widget: a AnjutaTabber widget
 *
 * Receives the padding from the widget style
 *
 * Returns: the padding used for the tabs
 */
static gint
anjuta_tabber_get_padding(GtkWidget* widget)
{
	gint focus_width;
	gint tab_border = gtk_container_get_border_width (GTK_CONTAINER(widget));
	gint padding;

	gtk_widget_style_get (widget,
	                      "focus-line-width", &focus_width,
	                      NULL);

	padding = 2 * (focus_width + tab_border);
	return padding;
}

static void
anjuta_tabber_size_request(GtkWidget* widget, GtkRequisition* req)
{
	g_return_if_fail (ANJUTA_IS_TABBER (widget));

	AnjutaTabber* tabber = ANJUTA_TABBER (widget);
	GList* child;
	gint padding = anjuta_tabber_get_padding (widget);
	
	for (child = tabber->priv->children; child != NULL; child = g_list_next (child))
	{
		GtkRequisition child_req;
		gtk_widget_size_request (GTK_WIDGET (child->data), &child_req);
		req->width += child_req.width + 2 * widget->style->xthickness + padding;
		req->height = MAX(req->height, child_req.height + 2 * widget->style->ythickness);
	}
}

static void
anjuta_tabber_size_allocate(GtkWidget* widget, GtkAllocation* allocation)
{
	g_return_if_fail (ANJUTA_IS_TABBER (widget));
	AnjutaTabber* tabber = ANJUTA_TABBER (widget);

	GList* child;
	gint x = allocation->x;
	gint y = allocation->y;
	gint padding = anjuta_tabber_get_padding (widget);
	gint child_width;
	gint n_children = g_list_length (tabber->priv->children);

	widget->allocation = *allocation;


	if (GTK_WIDGET_REALIZED (widget))
	{
		gdk_window_move_resize (tabber->priv->event_window,
		                        allocation->x, allocation->y,
		                        allocation->width, allocation->height);
		if (GTK_WIDGET_MAPPED (widget))
			gdk_window_show_unraised (tabber->priv->event_window);
	}

	if (n_children > 0)
	{
		child_width = allocation->width / n_children - padding - 
			(2 * widget->style->xthickness);

		for (child = tabber->priv->children; child != NULL; child = g_list_next (child))
		{
			GtkRequisition child_req;
			GtkAllocation child_alloc;
			gtk_widget_get_child_requisition (GTK_WIDGET (child->data), &child_req);

			child_alloc.width = child_width;
			child_alloc.height = MAX(child_req.height, allocation->height);
			child_alloc.x = x + widget->style->xthickness;
			child_alloc.y = y + widget->style->ythickness;

			gtk_widget_size_allocate (GTK_WIDGET (child->data), &child_alloc);
			x += child_alloc.width + 2 * widget->style->xthickness + padding;
		}
	}
}

static gboolean
anjuta_tabber_expose_event (GtkWidget* widget, GdkEventExpose *event)
{
	g_return_val_if_fail (ANJUTA_IS_TABBER (widget), FALSE);
	AnjutaTabber* tabber = ANJUTA_TABBER (widget);
	GList* child;
	gint padding = anjuta_tabber_get_padding (widget);
	
	for (child = tabber->priv->children; child != NULL; child = g_list_next (child))
	{
		GtkAllocation alloc;
		GtkStateType state = (g_list_nth (tabber->priv->children, tabber->priv->active_page) == child) ? 
			GTK_STATE_NORMAL : GTK_STATE_ACTIVE;
		gtk_widget_get_allocation (GTK_WIDGET (child->data), &alloc);

		alloc.x -= widget->style->xthickness;
		alloc.y -= widget->style->ythickness;
		alloc.width += widget->style->xthickness + padding;
		alloc.height += widget->style->ythickness;

		gtk_paint_extension (widget->style, widget->window,
		                     state, GTK_SHADOW_OUT,
		                     NULL, widget, "tab",
		                     alloc.x, 
		                     alloc.y,
		                     alloc.width, 
		                     alloc.height,
		                     GTK_POS_BOTTOM);
		gtk_container_propagate_expose (GTK_CONTAINER (tabber),
		                                GTK_WIDGET(child->data), event);
	}
	return FALSE;
}

/**
 * anjuta_tabber_get_widget_coordintes
 * @widget: widget for the coordinates
 * @event: event to get coordinates from
 * @x: return location for x coordinate
 * @y: return location for y coordinate
 *
 * Returns: TRUE if coordinates were set, FALSE otherwise
 */
static gboolean
anjuta_tabber_get_widget_coordinates (GtkWidget *widget,
			GdkEvent  *event,
			gint      *x,
			gint      *y)
{
  GdkWindow *window = ((GdkEventAny *)event)->window;
  gdouble tx, ty;

  if (!gdk_event_get_coords (event, &tx, &ty))
    return FALSE;

  while (window && window != widget->window)
    {
      gint window_x, window_y;

      gdk_window_get_position (window, &window_x, &window_y);
      tx += window_x;
      ty += window_y;

      window = gdk_window_get_parent (window);
    }

  if (window)
    {
      *x = tx;
      *y = ty;

      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
anjuta_tabber_button_press_event (GtkWidget* widget, GdkEventButton* event)
{
	AnjutaTabber* tabber = ANJUTA_TABBER (widget);
	GList* child;

	if (event->button == 1)
	{
		gint x, y;
		if (!anjuta_tabber_get_widget_coordinates (widget, (GdkEvent*) event, &x, &y))
			return FALSE;
		
		for (child = tabber->priv->children; child != NULL; child = g_list_next (child))
		{
			GtkAllocation alloc;
			gtk_widget_get_allocation (GTK_WIDGET (child->data), &alloc);

			g_message ("Event: %d %d", x, y);
			g_message ("alloc: %d %d %d %d", alloc.x, alloc.y, alloc.width, alloc.height);			
			
			if (alloc.x <= event->x && (alloc.x + alloc.width) >= event->x &&
			    alloc.y <= event->y && (alloc.y + alloc.height) >= event->y)
			{
				gint page = g_list_position (tabber->priv->children, child);
				gtk_notebook_set_current_page (tabber->priv->notebook, page);
				g_message ("Switched to page %d!", page);
				return TRUE;
			}
		}
	}
		
	return FALSE;
}

static void
anjuta_tabber_realize (GtkWidget *widget)
{
	GdkWindowAttr attributes;
	GtkAllocation allocation;
	AnjutaTabber* tabber = ANJUTA_TABBER (widget);
	
	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

	widget->window = gtk_widget_get_parent_window (widget);
	g_object_ref (widget->window);

	gtk_widget_get_allocation (widget, &allocation);
	
	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = allocation.x;
	attributes.y = allocation.y;
	attributes.width = allocation.width;
	attributes.height = allocation.height;
	attributes.wclass = GDK_INPUT_ONLY;
	attributes.event_mask = gtk_widget_get_events (widget);
	attributes.event_mask |= (GDK_BUTTON_PRESS_MASK);

	tabber->priv->event_window = gdk_window_new (gtk_widget_get_parent_window (widget), 
	                                             &attributes, GDK_WA_X | GDK_WA_Y);
	gdk_window_set_user_data (tabber->priv->event_window, tabber);

	widget->style = gtk_style_attach (widget->style, widget->window);
}

static void
anjuta_tabber_unrealize (GtkWidget *widget)
{
	AnjutaTabber* tabber = ANJUTA_TABBER (widget);
	gdk_window_set_user_data (tabber->priv->event_window, NULL);
	gdk_window_destroy (tabber->priv->event_window);
	tabber->priv->event_window = NULL;

	GTK_WIDGET_CLASS (anjuta_tabber_parent_class)->unrealize (widget);
}

static void
anjuta_tabber_map (GtkWidget* widget)
{
	AnjutaTabber* tabber = ANJUTA_TABBER (widget);
	GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);

	gdk_window_show_unraised (tabber->priv->event_window);

	GTK_WIDGET_CLASS (anjuta_tabber_parent_class)->map (widget);
}

static void
anjuta_tabber_unmap (GtkWidget* widget)
{
	AnjutaTabber* tabber = ANJUTA_TABBER (widget);
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);

	gdk_window_hide (tabber->priv->event_window);

	GTK_WIDGET_CLASS (anjuta_tabber_parent_class)->unmap (widget);

}

static void
anjuta_tabber_add (GtkContainer* container, GtkWidget* widget)
{
	g_return_if_fail (ANJUTA_IS_TABBER (container));
	g_return_if_fail (GTK_IS_WIDGET (widget));
	
	AnjutaTabber* tabber = ANJUTA_TABBER (container);

	tabber->priv->children = g_list_append (tabber->priv->children, widget);
	gtk_widget_set_parent (widget, GTK_WIDGET (tabber));
}

static void
anjuta_tabber_remove (GtkContainer* container, GtkWidget* widget)
{
	g_return_if_fail (ANJUTA_IS_TABBER (container));
	g_return_if_fail (GTK_IS_WIDGET (widget));

	AnjutaTabber* tabber = ANJUTA_TABBER (container);
	gboolean visible = GTK_WIDGET_VISIBLE (widget);
	
	gtk_widget_unparent (widget);
	tabber->priv->children = g_list_remove (tabber->priv->children, widget);

	if (visible)
		gtk_widget_queue_resize (GTK_WIDGET (tabber));
}

static void
anjuta_tabber_forall (GtkContainer* container, 
                      gboolean include_internals,
                      GtkCallback callback,
                      gpointer callback_data)
{
	g_return_if_fail (ANJUTA_IS_TABBER (container));
	AnjutaTabber* tabber = ANJUTA_TABBER (container);
	GList* child;
	for (child = tabber->priv->children; child != NULL; child = g_list_next (child))
	{
		(* callback) (GTK_WIDGET(child->data), callback_data);
    }
}

static void
anjuta_tabber_class_init (AnjutaTabberClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	GtkContainerClass* container_class = GTK_CONTAINER_CLASS (klass);
	
	object_class->finalize = anjuta_tabber_finalize;
	object_class->set_property = anjuta_tabber_set_property;
	object_class->get_property = anjuta_tabber_get_property;

	widget_class->size_request = anjuta_tabber_size_request;
	widget_class->size_allocate = anjuta_tabber_size_allocate;
	widget_class->expose_event = anjuta_tabber_expose_event;
	widget_class->button_press_event = anjuta_tabber_button_press_event;
	widget_class->realize = anjuta_tabber_realize;
	widget_class->unrealize = anjuta_tabber_unrealize;
	widget_class->map = anjuta_tabber_map;
	widget_class->unmap = anjuta_tabber_unmap;

	container_class->add = anjuta_tabber_add;
	container_class->remove = anjuta_tabber_remove;
	container_class->forall = anjuta_tabber_forall;
	
	g_object_class_install_property (object_class,
	                                 PROP_NOTEBOOK,
	                                 g_param_spec_object ("notebook",
	                                                      "a GtkNotebook",
	                                                      "GtkNotebook the tabber is associated with",
	                                                      G_TYPE_OBJECT,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE));

	g_type_class_add_private (klass, sizeof (AnjutaTabberPriv));
}

/**
 * anjuta_tabber_new:
 * @notebook: the GtkNotebook the tabber should be associated with
 *
 * Creates a new AnjutaTabber widget
 *
 * Returns: newly created AnjutaTabber widget
 */
GtkWidget* anjuta_tabber_new (GtkNotebook* notebook)
{
	GtkWidget* tabber;
	tabber = GTK_WIDGET (g_object_new (ANJUTA_TYPE_TABBER, "notebook", notebook, NULL));

	return tabber;
}

/**
 * anjuta_tabber_add_tab:
 * @tabber: a AnjutaTabber widget
 * @tab_label: widget used as tab label
 *
 * Adds a tab to the AnjutaTabber widget
 */
void anjuta_tabber_add_tab (AnjutaTabber* tabber, GtkWidget* tab_label)
{
	gtk_container_add (GTK_CONTAINER (tabber), tab_label);
}
