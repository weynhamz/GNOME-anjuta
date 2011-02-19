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

	/* Style information (taken from GtkNotebook) */
	gint tab_hborder;
	gint tab_vborder;

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
	AnjutaTabberPriv* priv;

	tabber->priv = ANJUTA_TABBER_GET_PRIVATE (tabber);
	priv = tabber->priv;

	priv->children = NULL;
	priv->active_page = 0;

	priv->tab_hborder = 2;
	priv->tab_vborder = 2;

	gtk_widget_set_has_window (GTK_WIDGET(tabber), FALSE);
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
                                    GtkWidget* page,
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

static void
anjuta_tabber_get_preferred_width (GtkWidget* widget, 
                                    gint* minimum,
                                    gint* preferred)
{
	g_return_if_fail (ANJUTA_IS_TABBER (widget));

	AnjutaTabber* tabber = ANJUTA_TABBER (widget);
	GList* child;
	gint xthickness;
	gint focus_width;
	GtkStyle* style = gtk_widget_get_style (widget);

	xthickness = style->xthickness;
	
	gtk_widget_style_get (GTK_WIDGET (tabber->priv->notebook),
	                      "focus-line-width", &focus_width,
	                      NULL);
	
	for (child = tabber->priv->children; child != NULL; child = g_list_next (child))
	{
		gint child_min;
		gint child_preferred;		
		GtkStyle *style;

		gtk_widget_get_preferred_width (GTK_WIDGET (child->data), &child_min, &child_preferred);
		style = gtk_widget_get_style (widget);
		if (minimum)
		{
			*minimum += child_min + 2 * (xthickness + focus_width + tabber->priv->tab_hborder);
		}
		if (preferred)
		{
			*preferred += child_preferred + 2 * (xthickness + focus_width + tabber->priv->tab_hborder);
		}
	}
}

static void
anjuta_tabber_get_preferred_height (GtkWidget* widget, 
                                    gint* minimum,
                                    gint* preferred)
{
	g_return_if_fail (ANJUTA_IS_TABBER (widget));

	AnjutaTabber* tabber = ANJUTA_TABBER (widget);
	GList* child;
	gint ythickness;
	gint focus_width;
	GtkStyle* style = gtk_widget_get_style (widget);

	ythickness = style->ythickness;
	
	gtk_widget_style_get (GTK_WIDGET (tabber->priv->notebook),
	                      "focus-line-width", &focus_width,
	                      NULL);

	
	for (child = tabber->priv->children; child != NULL; child = g_list_next (child))
	{
		gint child_min;
		gint child_preferred;		
		GtkStyle *style;

		gtk_widget_get_preferred_height (GTK_WIDGET (child->data), &child_min, &child_preferred);
		style = gtk_widget_get_style (widget);
		if (minimum)
		{
			*minimum = MAX(*minimum, child_min + 
			               2 * (ythickness + focus_width + tabber->priv->tab_vborder));
		}
		if (preferred)
		{
			*preferred = MAX(*preferred, child_preferred + 
			                 2 * (ythickness + focus_width + tabber->priv->tab_vborder));
		}
	}
}


static void
anjuta_tabber_size_allocate(GtkWidget* widget, GtkAllocation* allocation)
{
	g_return_if_fail (ANJUTA_IS_TABBER (widget));
	AnjutaTabber* tabber = ANJUTA_TABBER (widget);

	GList* child;
	gint focus_width;
	gint tab_curvature;
	gint n_children = g_list_length (tabber->priv->children);
	gint xthickness;
	gint ythickness;
	gint x;
	GtkStyle* style = gtk_widget_get_style (widget);

	xthickness = style->xthickness;
	ythickness = style->ythickness;
	
	gtk_widget_style_get (GTK_WIDGET (tabber->priv->notebook),
	                      "focus-line-width", &focus_width,
	                      "tab-curvature", &tab_curvature,
	                      NULL);
	
	gtk_widget_set_allocation (widget, allocation);

	switch (gtk_widget_get_direction (widget))
	{
		case GTK_TEXT_DIR_RTL:
			x = allocation->x + allocation->width - tabber->priv->tab_hborder;
			break;
		case GTK_TEXT_DIR_LTR:
		default:
			x = allocation->x; // + tabber->priv->tab_hborder;
	}

	if (gtk_widget_get_realized (widget))
	{
		gdk_window_move_resize (tabber->priv->event_window,
		                        allocation->x, allocation->y,
		                        allocation->width, allocation->height);
		if (gtk_widget_get_mapped (widget))
			gdk_window_show_unraised (tabber->priv->event_window);
	}

	if (n_children > 0)
	{
		gint total_width = 0;
		gboolean use_natural = FALSE;
		gint child_equal;
		gint extra_space = 0;

		/* Check if we have enough space for all widgets natural size */
		child_equal = allocation->width / n_children - 
			2 * (xthickness + focus_width + tabber->priv->tab_hborder);
		for (child = tabber->priv->children; child != NULL; child = g_list_next (child))
		{
			GtkWidget* child_widget = GTK_WIDGET (child->data);
			gint natural;
			gtk_widget_get_preferred_width (child_widget, NULL,
			                                &natural);

			total_width += natural;
			if (natural < child_equal)
				extra_space += child_equal - natural;
		}
		use_natural = (total_width +  
		               2 * (xthickness + focus_width + tabber->priv->tab_hborder) <= allocation->width);
		child_equal += extra_space / n_children;
		
		for (child = tabber->priv->children; child != NULL; child = g_list_next (child))
		{
			GtkWidget* child_widget = GTK_WIDGET (child->data);
			GtkRequisition child_req;
			GtkAllocation child_alloc;
			gtk_widget_get_child_requisition (child_widget, &child_req);
			gint natural;
			gint minimal;			

			gtk_widget_get_preferred_width (child_widget, &minimal,
				                            &natural);

			if (use_natural)
			{
				child_alloc.width = natural;
			}
			else
			{
				if (natural < child_equal)
					child_alloc.width = natural;
				else
					child_alloc.width = child_equal;
			}
			child_alloc.height = child_req.height - tabber->priv->tab_vborder;

			switch (gtk_widget_get_direction (widget))
			{
				case GTK_TEXT_DIR_RTL:
					child_alloc.x = x - xthickness - focus_width;
					child_alloc.width += xthickness + focus_width + tabber->priv->tab_hborder; 
					x = child_alloc.x - child_alloc.width;
					break;
				case GTK_TEXT_DIR_LTR:
				default:
					child_alloc.x = x + xthickness + focus_width;
					child_alloc.width += xthickness + focus_width + tabber->priv->tab_hborder; 
					x = child_alloc.x + child_alloc.width;
			}
			child_alloc.y = allocation->y + allocation->height
				- child_alloc.height - tabber->priv->tab_vborder - focus_width - ythickness;

			gtk_widget_size_allocate (GTK_WIDGET (child->data), &child_alloc);
		}
	}
}

static void
anjuta_tabber_draw_tab (GtkWidget* widget,
                        GtkWidget* tab,
                        cairo_t* cr,
                        GtkStateType state,
                        gboolean first)
{
	AnjutaTabber* tabber = ANJUTA_TABBER (widget);
	GtkAllocation alloc;
	GtkAllocation widget_alloc;
	gint focus_width;
	gint tab_curvature;
	gint xthickness;
	gint ythickness;
	GtkStyle* style = gtk_widget_get_style (widget);

	xthickness = style->xthickness;
	ythickness = style->ythickness;
	
	gtk_widget_style_get (GTK_WIDGET (tabber->priv->notebook),
	                      "focus-line-width", &focus_width,
	                      "tab-curvature", &tab_curvature,
	                      NULL);

	
	gtk_widget_get_allocation (widget, &widget_alloc);
	gtk_widget_get_allocation (tab, &alloc);

	style = gtk_widget_get_style (widget);

	alloc.x -= widget_alloc.x 
		+ xthickness + focus_width;// + tabber->priv->tab_hborder;
	alloc.y -= widget_alloc.y + focus_width + ythickness;
	alloc.width += xthickness + focus_width; //+ tabber->priv->tab_hborder;;
	alloc.height = widget_alloc.height;

	if (state == GTK_STATE_NORMAL)
	{
		alloc.y -= tabber->priv->tab_vborder;
		alloc.height += tabber->priv->tab_vborder;
		if (!first)
		{
			switch (gtk_widget_get_direction (widget))
			{
				case GTK_TEXT_DIR_RTL:
					alloc.x += tabber->priv->tab_hborder;
				case GTK_TEXT_DIR_LTR:
				default:
					alloc.x -= tabber->priv->tab_hborder;
			}
			alloc.width += tabber->priv->tab_hborder;
		}
		alloc.width += tabber->priv->tab_hborder;
	}
	
	gtk_paint_extension (style,
	                     cr,
	                     state, GTK_SHADOW_OUT,
	                     widget, "tab",
	                     alloc.x, 
	                     alloc.y,
	                     alloc.width, 
	                     alloc.height,
	                     GTK_POS_BOTTOM);
}

static gboolean
anjuta_tabber_draw (GtkWidget* widget, cairo_t* cr)
{
	g_return_val_if_fail (ANJUTA_IS_TABBER (widget), FALSE);

	AnjutaTabber* tabber = ANJUTA_TABBER (widget);
	GList* child;
	GtkWidget* current_tab = g_list_nth_data (tabber->priv->children, tabber->priv->active_page);
	GtkWidget* first = NULL;

	if (tabber->priv->children) 
		first = tabber->priv->children->data;

	for (child = tabber->priv->children; child != NULL; child = g_list_next (child))
	{
		GtkWidget* tab = GTK_WIDGET (child->data);
		if (tab != current_tab)
			anjuta_tabber_draw_tab (widget, tab, cr, GTK_STATE_ACTIVE, current_tab == first);
	}
	anjuta_tabber_draw_tab (widget, current_tab, cr, GTK_STATE_NORMAL, current_tab == first);
	
	return GTK_WIDGET_CLASS (anjuta_tabber_parent_class)->draw (widget, cr);
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

  while (window && window != gtk_widget_get_window (widget))
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
			
			if (alloc.x <= x && (alloc.x + alloc.width) >= x &&
			    alloc.y <= y && (alloc.y + alloc.height) >= y)
			{
				gint page = g_list_position (tabber->priv->children, child);
				gtk_notebook_set_current_page (tabber->priv->notebook, page);
				return TRUE;
			}
		}
	}
		
	return FALSE;
}

static void
anjuta_tabber_realize (GtkWidget *widget)
{
	GdkWindow*    window;
	GdkWindowAttr attributes;
	GtkAllocation allocation;
	AnjutaTabber* tabber = ANJUTA_TABBER (widget);
	
	gtk_widget_set_realized (widget, TRUE);

	window = gtk_widget_get_parent_window (widget);
	gtk_widget_set_window (widget, window);
	g_object_ref (window);

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

	gtk_widget_style_attach (widget);
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
	gtk_widget_set_mapped (widget, TRUE);

	gdk_window_show_unraised (tabber->priv->event_window);

	GTK_WIDGET_CLASS (anjuta_tabber_parent_class)->map (widget);
}

static void
anjuta_tabber_unmap (GtkWidget* widget)
{
	AnjutaTabber* tabber = ANJUTA_TABBER (widget);

	gtk_widget_set_mapped (widget, FALSE);
	gdk_window_hide (tabber->priv->event_window);

	GTK_WIDGET_CLASS (anjuta_tabber_parent_class)->unmap (widget);

}

static void
anjuta_tabber_add (GtkContainer* container, GtkWidget* widget)
{
	g_return_if_fail (ANJUTA_IS_TABBER (container));
	g_return_if_fail (GTK_IS_WIDGET (widget));
	
	AnjutaTabber* tabber = ANJUTA_TABBER (container);
	gboolean visible = gtk_widget_get_visible (widget);

	tabber->priv->children = g_list_append (tabber->priv->children, widget);
	gtk_widget_set_parent (widget, GTK_WIDGET (tabber));
	if (visible)
		gtk_widget_queue_resize (widget);
}

static void
anjuta_tabber_remove (GtkContainer* container, GtkWidget* widget)
{
	g_return_if_fail (ANJUTA_IS_TABBER (container));
	g_return_if_fail (GTK_IS_WIDGET (widget));

	AnjutaTabber* tabber = ANJUTA_TABBER (container);
	gboolean visible = gtk_widget_get_visible (widget);
	
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

	widget_class->get_preferred_height = anjuta_tabber_get_preferred_height;
	widget_class->get_preferred_width = anjuta_tabber_get_preferred_width;
	widget_class->size_allocate = anjuta_tabber_size_allocate;
	widget_class->draw = anjuta_tabber_draw;
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
