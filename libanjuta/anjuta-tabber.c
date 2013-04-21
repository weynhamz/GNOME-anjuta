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

/**
 * SECTION:anjuta-tabber
 * @title: AnjutaTabber
 * @short_description: Tab widget
 * @see_also:
 * @stability: Unstable
 * @include: libanjuta/anjuta-tabber.h
 */

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
	AnjutaTabberPriv* priv;

	GtkStyleContext* context;

	tabber->priv = ANJUTA_TABBER_GET_PRIVATE (tabber);
	priv = tabber->priv;

	priv->children = NULL;
	priv->active_page = 0;

	gtk_widget_set_has_window (GTK_WIDGET(tabber), FALSE);

	context = gtk_widget_get_style_context (GTK_WIDGET (tabber));
	gtk_style_context_add_class (context, GTK_STYLE_CLASS_NOTEBOOK);
	gtk_style_context_add_class (context, GTK_STYLE_CLASS_TOP);
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

static void
anjuta_tabber_finalize (GObject *object)
{
	G_OBJECT_CLASS (anjuta_tabber_parent_class)->finalize (object);
}

static void
anjuta_tabber_dispose (GObject *object)
{
	AnjutaTabber* tabber = ANJUTA_TABBER (object);
	g_signal_handlers_disconnect_by_func (tabber->priv->notebook,
	                                      anjuta_tabber_notebook_page_removed,
	                                      object);
	g_signal_handlers_disconnect_by_func (tabber->priv->notebook,
	                                      anjuta_tabber_notebook_switch_page,
	                                      object);
	
	G_OBJECT_CLASS (anjuta_tabber_parent_class)->dispose (object);
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

static GtkRegionFlags
anjuta_tabber_get_region_flags (AnjutaTabber* tabber, gint page_num)
{
	GtkRegionFlags flags = 0;

	if ((page_num + 1) % 2 == 0)
		flags |= GTK_REGION_EVEN;
	else
		flags |= GTK_REGION_ODD;

	if (page_num == 1)
		flags |= GTK_REGION_FIRST;

	if (page_num == (g_list_length (tabber->priv->children) - 1))
		flags |= GTK_REGION_LAST;
	
	return flags;
}

static void
anjuta_tabber_setup_style_context (AnjutaTabber* tabber, GtkStyleContext* context,
                                   GList* child, GtkStateFlags* state_flags,
                                   GtkRegionFlags* region_flags)
{
	gint page_num;
	gboolean current;
	GtkRegionFlags region;
	GtkStateFlags state;

	page_num = g_list_position (tabber->priv->children, child);
	current = page_num == tabber->priv->active_page;

	region = anjuta_tabber_get_region_flags (tabber, page_num);

	gtk_style_context_add_region (context, GTK_STYLE_REGION_TAB, region);

	if (gtk_widget_get_direction (GTK_WIDGET (tabber)) == GTK_TEXT_DIR_LTR)
		gtk_style_context_set_junction_sides (context,
		                                      GTK_JUNCTION_CORNER_TOPLEFT);
	else
		gtk_style_context_set_junction_sides (context,
		                                      GTK_JUNCTION_CORNER_TOPRIGHT);

	state = gtk_style_context_get_state (context);
	if (current)
		state |= GTK_STATE_FLAG_ACTIVE;
	else
		state &= ~GTK_STATE_FLAG_ACTIVE;
	gtk_style_context_set_state (context, state);

	if (state_flags)
		*state_flags = state;
	if (region_flags)
		*region_flags = region;
}

static void
anjuta_tabber_get_preferred_width (GtkWidget* widget, 
                                    gint* minimum,
                                    gint* preferred)
{
	g_return_if_fail (ANJUTA_IS_TABBER (widget));

	AnjutaTabber* tabber = ANJUTA_TABBER (widget);
	GtkStyleContext* context;
	GList* child;
	gint focus_width;
	gint focus_pad;
	gint tab_curvature;
	gint tab_overlap;

	*minimum = 0;
	*preferred = 0;
	
	gtk_widget_style_get (GTK_WIDGET (tabber->priv->notebook),
	                      "focus-line-width", &focus_width,
	                      "focus-padding", &focus_pad,
	                      "tab-curvature", &tab_curvature,
	                      "tab-overlap", &tab_overlap,
	                      NULL);

	context = gtk_widget_get_style_context (widget);

	for (child = tabber->priv->children; child != NULL; child = g_list_next (child))
	{
		GtkStateFlags state;
		GtkBorder tab_padding;
		gint xpadding;
		gint child_min;
		gint child_preferred;
		gint extra_space = 2 * (tab_curvature - tab_overlap);

		/* Get the padding of the tab */
		gtk_style_context_save (context);
		anjuta_tabber_setup_style_context (tabber, context, child, &state, NULL);
		gtk_style_context_get_padding (context, state, &tab_padding);
		gtk_style_context_restore (context);

		xpadding =  2 * (focus_width + focus_pad) + tab_padding.left + tab_padding.right;
		
		if (child->prev == NULL)
			extra_space += tab_overlap;
		if (child->next == NULL)
			extra_space += tab_overlap;
		
		gtk_widget_get_preferred_width (GTK_WIDGET (child->data), &child_min, &child_preferred);
		if (minimum)
		{
			*minimum += child_min + xpadding + extra_space;
		}
		if (preferred)
		{
			*preferred += child_preferred + xpadding + extra_space;
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
	GtkStyleContext* context;
	GList* child;
	gint focus_width;
	gint focus_pad;
	
	gtk_widget_style_get (GTK_WIDGET (tabber),
	                      "focus-line-width", &focus_width,
	                      "focus-padding", &focus_pad,
	                      NULL);

	context = gtk_widget_get_style_context (widget);

	for (child = tabber->priv->children; child != NULL; child = g_list_next (child))
	{
		GtkStateFlags state;
		GtkBorder tab_padding;
		gint ypadding;
		gint child_min;
		gint child_preferred;

		/* Get the padding of the tab */
		gtk_style_context_save (context);
		anjuta_tabber_setup_style_context (tabber, context, child, &state, NULL);
		gtk_style_context_get_padding (context, state, &tab_padding);
		gtk_style_context_restore (context);

		ypadding =  2 * (focus_width + focus_pad) + tab_padding.top + tab_padding.bottom;

		gtk_widget_get_preferred_height (GTK_WIDGET (child->data), &child_min, &child_preferred);
		if (minimum)
		{
			*minimum = MAX(*minimum, child_min + ypadding);
		}
		if (preferred)
		{
			*preferred = MAX(*preferred, child_preferred + ypadding);
		}
	}
}


static void
anjuta_tabber_size_allocate(GtkWidget* widget, GtkAllocation* allocation)
{
	g_return_if_fail (ANJUTA_IS_TABBER (widget));
	AnjutaTabber* tabber = ANJUTA_TABBER (widget);

	GtkStyleContext* context;
	GList* child;
	gint focus_width;
	gint focus_pad;
	gint tab_curvature;
	gint tab_overlap;
	gint n_children = g_list_length (tabber->priv->children);
	gint x;
	gint focus_space;
	gint tab_space;

	context = gtk_widget_get_style_context (widget);

	gtk_widget_style_get (GTK_WIDGET (tabber),
	                      "focus-line-width", &focus_width,
	                      "focus-padding", &focus_pad,
	                      "tab-curvature", &tab_curvature,
	                      "tab-overlap", &tab_overlap,
	                      NULL);

	focus_space = focus_width + focus_pad;
	tab_space = tab_curvature - tab_overlap;
	
	gtk_widget_set_allocation (widget, allocation);

	switch (gtk_widget_get_direction (widget))
	{
		case GTK_TEXT_DIR_RTL:
			x = allocation->x + allocation->width;
			break;
		case GTK_TEXT_DIR_LTR:
		default:
			x = allocation->x;
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
		gint total_space;
		gint total_width;
		gboolean use_natural = FALSE;
		gint child_equal;
		gint extra_space = 0;
		gint real_width = allocation->width;

		/* Calculate the total space that is used for padding/overlap */
		total_space = 2 * tab_overlap + n_children * 2 * focus_space +
			(n_children - 1) * 2 * tab_space;
		for (child = tabber->priv->children; child != NULL; child = g_list_next (child))
		{
			GtkStateFlags state;
			GtkBorder tab_padding;

			/* Get the padding of the tab */
			gtk_style_context_save (context);
			anjuta_tabber_setup_style_context (tabber, context, child, &state, NULL);
			gtk_style_context_get_padding (context, state, &tab_padding);
			gtk_style_context_restore (context);

			total_space += tab_padding.left + tab_padding.right;
		}

		/* Check if we have enough space for all widgets natural size */
		child_equal = (real_width - total_space) / n_children;

		if (child_equal < 0)
			return;

		/* Calculate the total width of the tabs */
		total_width = 2 * tab_overlap + n_children * 2 * focus_space +
			(n_children - 1) * 2 * tab_space;
		for (child = tabber->priv->children; child != NULL; child = g_list_next (child))
		{
			GtkWidget* child_widget = GTK_WIDGET (child->data);
			GtkStateFlags state;
			GtkBorder tab_padding;
			gint natural;

			/* Get the padding of the tab */
			gtk_style_context_save (context);
			anjuta_tabber_setup_style_context (tabber, context, child, &state, NULL);
			gtk_style_context_get_padding (context, state, &tab_padding);
			gtk_style_context_restore (context);

			gtk_widget_get_preferred_width (child_widget, NULL,
			                                &natural);

			total_width += natural + tab_padding.left + tab_padding.right;

			if (natural < child_equal)
				extra_space += child_equal - natural;
		}
		use_natural = (total_width <= real_width);
		child_equal += extra_space / n_children;
		
		for (child = tabber->priv->children; child != NULL; child = g_list_next (child))
		{
			GtkWidget* child_widget = GTK_WIDGET (child->data);
			GtkStateFlags state;
			GtkBorder tab_padding, active_padding;
			GtkAllocation child_alloc;
			gint natural;
			gint minimal;
			gint begin_tab = tab_space;
			gint end_tab = tab_space;

			/* Get the padding of the tab */
			gtk_style_context_save (context);
			anjuta_tabber_setup_style_context (tabber, context, child, &state, NULL);
			gtk_style_context_get_padding (context, state, &tab_padding);
			gtk_style_context_get_padding (context, state | GTK_STATE_ACTIVE, &active_padding);
			gtk_style_context_restore (context);

			if (child->prev == NULL)
				begin_tab += tab_overlap;
			if (child->next == NULL)
				end_tab += tab_overlap;
			
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
			/* The active pad is by definition at least the same height
			 * as the inactive one. Therefore we always use the padding of the
			 * active tab to calculate the height and y position of the child.
			 */
			child_alloc.height = allocation->height - 2 * focus_space
				- active_padding.top - active_padding.bottom;
			child_alloc.y = allocation->y + focus_space + active_padding.top;

			switch (gtk_widget_get_direction (widget))
			{
				case GTK_TEXT_DIR_RTL:
					child_alloc.x = x - focus_space - tab_padding.right
						- begin_tab - child_alloc.width;
					x = child_alloc.x - focus_space - tab_padding.left - end_tab;
					break;
				case GTK_TEXT_DIR_LTR:
				default:
					child_alloc.x = x + focus_space + tab_padding.left + begin_tab;
					x = child_alloc.x + child_alloc.width + focus_space
						+ tab_padding.right + end_tab;
			}

			gtk_widget_size_allocate (GTK_WIDGET (child->data), &child_alloc);
		}
	}
}

static void
anjuta_tabber_draw_tab (AnjutaTabber* tabber, cairo_t* cr, GList* child)
{
	GtkWidget* widget = GTK_WIDGET (tabber);
	GtkWidget* tab = GTK_WIDGET (child->data);

	gboolean current;
	gint focus_width;
	gint focus_pad;
	gint tab_curvature;
	gint tab_overlap;
	gint focus_space;
	gint tab_begin;
	gint tab_end;

	GtkStateFlags state;
	GtkRegionFlags region_flags;
	GtkBorder tab_padding;
	GtkAllocation alloc;
	GtkAllocation widget_alloc;

	GtkStyleContext* context = gtk_widget_get_style_context (widget);

	current = g_list_position (tabber->priv->children, child) == tabber->priv->active_page;

	gtk_widget_style_get (widget,
	                      "focus-line-width", &focus_width,
	                      "focus-padding", &focus_pad,
	                      "tab-curvature", &tab_curvature,
	                      "tab-overlap", &tab_overlap,
	                      NULL);

	focus_space = focus_pad + focus_width;

	/* Get border/padding for tab */
	gtk_style_context_save (context);
	anjuta_tabber_setup_style_context (tabber, context, child, &state, &region_flags);
	gtk_style_context_get_padding (context, state, &tab_padding);
		
	gtk_widget_get_allocation (widget, &widget_alloc);
	gtk_widget_get_allocation (tab, &alloc);

	tab_begin = tab_curvature - tab_overlap;
	tab_end = tab_curvature - tab_overlap;

	if (region_flags | GTK_REGION_FIRST)
		tab_begin += tab_overlap;
	if (region_flags | GTK_REGION_LAST)
		tab_end += tab_overlap;
	
	alloc.x -= widget_alloc.x;
	alloc.x -= tab_begin;
	alloc.x -= focus_space + tab_padding.left;
	alloc.y -= widget_alloc.y;
	alloc.y -= focus_space + tab_padding.top;
	alloc.width += 2 * focus_space + tab_padding.left + tab_padding.right
		+ tab_begin + tab_end;
	alloc.height += 2 * focus_space + tab_padding.top + tab_padding.bottom;
	
	gtk_render_extension (context,
	                      cr,
	                      alloc.x, 
	                      alloc.y,
	                      alloc.width, 
	                      alloc.height,
	                      GTK_POS_BOTTOM);

	if (gtk_widget_has_focus (widget) &&
	    current)
	{
		GtkAllocation allocation;
		
		gtk_widget_get_allocation (tab, &allocation);

		gtk_render_focus (context, cr,
		                  allocation.x - focus_space,
		                  allocation.y - focus_space,
		                  allocation.width + 2 * focus_space,
		                  allocation.height + 2 * focus_space);
	}
	
	gtk_style_context_restore (context);
}

static gboolean
anjuta_tabber_draw (GtkWidget* widget, cairo_t* cr)
{
	AnjutaTabber* tabber;
	GList* current_tab;
	GList* child;

	g_return_val_if_fail (ANJUTA_IS_TABBER (widget), FALSE);

	tabber = ANJUTA_TABBER (widget);
	
	if (!tabber->priv->children)
		return TRUE;

	current_tab = g_list_nth (tabber->priv->children, tabber->priv->active_page);

	/* Draw the current tab last since it overlaps the others */
	for (child = tabber->priv->children; child != current_tab; child = g_list_next (child))
	{
		anjuta_tabber_draw_tab (tabber, cr, child);
	}
	for (child = g_list_last (tabber->priv->children); child != current_tab; child = g_list_previous (child))
	{
		anjuta_tabber_draw_tab (tabber, cr, child);
	}
	anjuta_tabber_draw_tab (tabber, cr, current_tab);

	return GTK_WIDGET_CLASS (anjuta_tabber_parent_class)->draw (widget, cr);
}

/**
 * anjuta_tabber_get_widget_coordinates:
 * @widget: widget for the coordinates
 * @event: event to get coordinates from
 * @x: return location for x coordinate
 * @y: return location for y coordinate
 *
 * Returns: %TRUE if coordinates were set, %FALSE otherwise
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
	{
		gtk_container_resize_children (GTK_CONTAINER (tabber));
		gtk_widget_queue_resize (widget);
	}
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

	if (tabber->priv->active_page > 0)
		tabber->priv->active_page--;
	
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

static GtkWidgetPath*
anjuta_tabber_get_path_for_child (GtkContainer* container,
                                  GtkWidget* widget)
{
	AnjutaTabber* tabber = ANJUTA_TABBER (container);

	GtkWidgetPath* path;
	gint page_num;;
	gint tabber_pos;

	path = GTK_CONTAINER_CLASS (anjuta_tabber_parent_class)->get_path_for_child (container, widget);

	page_num = g_list_index (tabber->priv->children, widget);
	if (page_num == -1)
		return path;

	tabber_pos = gtk_widget_path_length (path) - 2;
	gtk_widget_path_iter_add_region (path, tabber_pos, GTK_STYLE_REGION_TAB,
	                                 anjuta_tabber_get_region_flags (tabber, page_num));
	return path;
}

static void
anjuta_tabber_class_init (AnjutaTabberClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	GtkContainerClass* container_class = GTK_CONTAINER_CLASS (klass);
	
	object_class->finalize = anjuta_tabber_finalize;
	object_class->dispose = anjuta_tabber_dispose;
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
	container_class->get_path_for_child = anjuta_tabber_get_path_for_child;
	
	g_object_class_install_property (object_class,
	                                 PROP_NOTEBOOK,
	                                 g_param_spec_object ("notebook",
	                                                      "a GtkNotebook",
	                                                      "GtkNotebook the tabber is associated with",
	                                                      G_TYPE_OBJECT,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE));

	/* Install some notebook properties */
	gtk_widget_class_install_style_property (widget_class,
	                                         g_param_spec_int ("tab-overlap", "", "",
	                                                           G_MININT,
	                                                           G_MAXINT,
	                                                           4,
	                                                           G_PARAM_READABLE));
	gtk_widget_class_install_style_property (widget_class,
	                                         g_param_spec_int ("tab-curvature", "", "",
	                                                           0,
	                                                           G_MAXINT,
	                                                           6,
	                                                           G_PARAM_READABLE));	
	
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
