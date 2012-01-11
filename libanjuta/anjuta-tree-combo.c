/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */

/* anjuta-tree-combo.c
 *
 * Copyright (C) 2011  Sébastien Granjoux
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>

#include "anjuta-tree-combo.h"


/* Types
 *---------------------------------------------------------------------------*/

struct _AnjutaTreeComboBoxPrivate
{
	GtkTreeModel *model;

	GtkWidget *cell_view;
	GtkCellArea *area;

	GtkWidget *invalid_notebook;
	GtkWidget *invalid_label;

	GtkWidget *popup_window;
	GtkWidget *scrolled_window;
	GtkWidget *tree_view;

	GdkDevice *grab_pointer;
	GdkDevice *grab_keyboard;

	guint scroll_timer;
	gboolean	auto_scroll;
  	guint resize_idle_id;
 	gboolean popup_in_progress;

	gint active; 						/* Only temporary */
	GtkTreeRowReference *active_row;	/* Current selected row */

	GtkTreeModelFilterVisibleFunc valid_func;
	gpointer valid_data;
	GDestroyNotify valid_destroy;
};

enum {
	INVALID_PAGE = 0,
	VALID_PAGE = 1
};

enum {
	CHANGED,
	POPUP,
	POPDOWN,
	LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_MODEL
};

static guint signals[LAST_SIGNAL] = {0,};

#define SCROLL_TIME  100

static void     anjuta_tree_combo_box_cell_layout_init     (GtkCellLayoutIface *iface);

G_DEFINE_TYPE_WITH_CODE (AnjutaTreeComboBox, anjuta_tree_combo_box, GTK_TYPE_TOGGLE_BUTTON,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_CELL_LAYOUT,
                                                anjuta_tree_combo_box_cell_layout_init))



/* Helpers functions
 *---------------------------------------------------------------------------*/

static gboolean
popup_grab_on_window (GdkWindow *window,
                      GdkDevice *keyboard,
                      GdkDevice *pointer)
{
	if (keyboard &&
	    gdk_device_grab (keyboard, window,
	                     GDK_OWNERSHIP_WINDOW, TRUE,
	                     GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK,
	                     NULL, GDK_CURRENT_TIME) != GDK_GRAB_SUCCESS)
    	return FALSE;

	if (pointer &&
	    gdk_device_grab (pointer, window,
	                     GDK_OWNERSHIP_WINDOW, TRUE,
	                     GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
	                     GDK_POINTER_MOTION_MASK,
	                     NULL, GDK_CURRENT_TIME) != GDK_GRAB_SUCCESS)
	{
		if (keyboard)
			gdk_device_ungrab (keyboard, GDK_CURRENT_TIME);

		return FALSE;
	}

	return TRUE;
}



/* Private functions
 *---------------------------------------------------------------------------*/

static void
anjuta_tree_combo_box_popup_position (AnjutaTreeComboBox *combo,
                                      gint        *x,
                                      gint        *y,
                                      gint        *width,
                                      gint        *height)
{
	AnjutaTreeComboBoxPrivate *priv = combo->priv;
	GtkAllocation allocation;
	GdkScreen *screen;
	gint monitor_num;
	GdkRectangle monitor;
	GtkRequisition popup_req;
	GtkPolicyType hpolicy, vpolicy;
	GdkWindow *window;

	GtkWidget *widget = GTK_WIDGET (combo);

	*x = *y = 0;

	gtk_widget_get_allocation (widget, &allocation);

	if (!gtk_widget_get_has_window (widget))
	{
		*x += allocation.x;
		*y += allocation.y;
	}

	window = gtk_widget_get_window (widget);

	gdk_window_get_root_coords (gtk_widget_get_window (widget),
	                            *x, *y, x, y);

	*width = allocation.width;

	hpolicy = vpolicy = GTK_POLICY_NEVER;
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->scrolled_window),
                                  hpolicy, vpolicy);

	gtk_widget_get_preferred_size (priv->scrolled_window, &popup_req, NULL);

	if (popup_req.width > *width)
	{
		hpolicy = GTK_POLICY_ALWAYS;
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->scrolled_window),
		                                hpolicy, vpolicy);
	}

	*height = popup_req.height;

	screen = gtk_widget_get_screen (GTK_WIDGET (combo));
	monitor_num = gdk_screen_get_monitor_at_window (screen, window);
	gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

	if (gtk_widget_get_direction (GTK_WIDGET (combo)) == GTK_TEXT_DIR_RTL)
		*x = *x + allocation.width - *width;

	if (*x < monitor.x)
		*x = monitor.x;
	else if (*x + *width > monitor.x + monitor.width)
		*x = monitor.x + monitor.width - *width;

	if (*y + allocation.height + *height <= monitor.y + monitor.height)
		*y += allocation.height;
	else if (*y - *height >= monitor.y)
		*y -= *height;
	else if (monitor.y + monitor.height - (*y + allocation.height) > *y - monitor.y)
	{
		*y += allocation.height;
		*height = monitor.y + monitor.height - *y;
	}
	else
	{
		*height = *y - monitor.y;
		*y = monitor.y;
	}

	if (popup_req.height > *height)
	{
		vpolicy = GTK_POLICY_ALWAYS;

		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->scrolled_window),
		                                hpolicy, vpolicy);
	}
}

static void
anjuta_tree_combo_box_popup_for_device (AnjutaTreeComboBox *combo,
                                        GdkDevice   *device)
{
	AnjutaTreeComboBoxPrivate *priv = combo->priv;
	gint x, y, width, height;
	GtkWidget *toplevel;
	GdkDevice *keyboard, *pointer;

	g_return_if_fail (ANJUTA_IS_TREE_COMBO_BOX (combo));
	g_return_if_fail (GDK_IS_DEVICE (device));

	if (!gtk_widget_get_realized (GTK_WIDGET (combo)))
		return;

	if (gtk_widget_get_mapped (priv->popup_window))
		return;

	if (priv->grab_pointer && priv->grab_keyboard)
		return;

	if (gdk_device_get_source (device) == GDK_SOURCE_KEYBOARD)
	{
		keyboard = device;
		pointer = gdk_device_get_associated_device (device);
	}
	else
	{
		pointer = device;
		keyboard = gdk_device_get_associated_device (device);
	}

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (combo));

	if (GTK_IS_WINDOW (toplevel))
		gtk_window_group_add_window (gtk_window_get_group (GTK_WINDOW (toplevel)),
		                             GTK_WINDOW (priv->popup_window));

	gtk_widget_show_all (priv->scrolled_window);
	anjuta_tree_combo_box_popup_position (combo, &x, &y, &width, &height);

	gtk_widget_set_size_request (priv->popup_window, width, height);
	gtk_window_move (GTK_WINDOW (priv->popup_window), x, y);

	gtk_tree_view_set_hover_expand (GTK_TREE_VIEW (priv->tree_view),
	                                TRUE);

	/* popup */
	gtk_widget_show (priv->popup_window);

	gtk_widget_grab_focus (priv->popup_window);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (combo),
	                              TRUE);

	if (!gtk_widget_has_focus (priv->tree_view))
		gtk_widget_grab_focus (priv->tree_view);

	if (!popup_grab_on_window (gtk_widget_get_window (priv->popup_window),
	                           keyboard, pointer))
	{
		gtk_widget_hide (priv->popup_window);
		return;
	}

	gtk_device_grab_add (priv->popup_window, pointer, TRUE);
	priv->grab_pointer = pointer;
	priv->grab_keyboard = keyboard;
}

static void
anjuta_tree_combo_box_popup (AnjutaTreeComboBox *combo)
{
	GdkDevice *device;

	device = gtk_get_current_event_device ();

	if (!device)
	{
		GdkDeviceManager *device_manager;
		GdkDisplay *display;
		GList *devices;

		display = gtk_widget_get_display (GTK_WIDGET (combo));
		device_manager = gdk_display_get_device_manager (display);

		/* No device was set, pick the first master device */
		devices = gdk_device_manager_list_devices (device_manager, GDK_DEVICE_TYPE_MASTER);
		device = devices->data;
		g_list_free (devices);
	}

	anjuta_tree_combo_box_popup_for_device (combo, device);
}

static void
anjuta_tree_combo_box_popdown (AnjutaTreeComboBox *combo)
{
	AnjutaTreeComboBoxPrivate *priv = combo->priv;

	g_return_if_fail (ANJUTA_IS_TREE_COMBO_BOX (combo));

	if (!gtk_widget_get_realized (GTK_WIDGET (combo)))
		return;

	gtk_device_grab_remove (priv->popup_window, priv->grab_pointer);
	gtk_widget_hide (priv->popup_window);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (combo),
	                              FALSE);

	priv->grab_pointer = NULL;
	priv->grab_keyboard = NULL;
}

static void
anjuta_tree_combo_box_unset_model (AnjutaTreeComboBox *combo)
{
	AnjutaTreeComboBoxPrivate *priv = combo->priv;

	if (priv->model != NULL)
	{
		g_object_unref (priv->model);
		priv->model = NULL;
	}

	if (priv->active_row)
    {
		gtk_tree_row_reference_free (priv->active_row);
		priv->active_row = NULL;
	}

	if (priv->cell_view)
	{
		gtk_cell_view_set_model (GTK_CELL_VIEW (priv->cell_view), NULL);
	}

	if (priv->tree_view)
	{
		gtk_tree_view_set_model (GTK_TREE_VIEW (priv->tree_view), NULL);
	}
}

static void
anjuta_tree_combo_box_popup_destroy (AnjutaTreeComboBox *combo)
{
	AnjutaTreeComboBoxPrivate *priv = combo->priv;

	if (priv->scroll_timer)
	{
		g_source_remove (priv->scroll_timer);
		priv->scroll_timer = 0;
	}

	if (priv->resize_idle_id)
	{
		g_source_remove (priv->resize_idle_id);
		priv->resize_idle_id = 0;
	}

	if (priv->popup_window)
    {
		gtk_widget_destroy (priv->popup_window);
		priv->popup_window = NULL;
	}
}

static void
anjuta_tree_combo_box_set_active_path (AnjutaTreeComboBox *combo,
                                       GtkTreePath *path)
{
	AnjutaTreeComboBoxPrivate *priv = combo->priv;
	gboolean valid;

	valid = gtk_tree_row_reference_valid (priv->active_row);

	if (path && valid)
	{
		GtkTreePath *active_path;
		gint cmp;

		active_path = gtk_tree_row_reference_get_path (priv->active_row);
		cmp = gtk_tree_path_compare (path, active_path);
		gtk_tree_path_free (active_path);
		if (cmp == 0) return;
	}

	if (priv->active_row)
	{
		gtk_tree_row_reference_free (priv->active_row);
		priv->active_row = NULL;
	}

	if (!path)
	{
		gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree_view)));
       	gtk_cell_view_set_displayed_row (GTK_CELL_VIEW (priv->cell_view), NULL);
		gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->invalid_notebook), INVALID_PAGE);
		if (!valid)	return;
    }
	else
    {
		GtkTreeIter iter;

		priv->active_row = gtk_tree_row_reference_new (priv->model, path);

		gtk_tree_view_expand_to_path (GTK_TREE_VIEW (priv->tree_view), path);
		gtk_tree_view_set_cursor (GTK_TREE_VIEW (priv->tree_view), path, NULL, FALSE);
		gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (priv->tree_view), path, NULL, TRUE, 0.5, 0.0);

		if ((priv->valid_func == NULL) ||
		    (gtk_tree_model_get_iter (combo->priv->model, &iter, path) &&
			priv->valid_func (combo->priv->model, &iter, priv->valid_data)))
		{
        	gtk_cell_view_set_displayed_row (GTK_CELL_VIEW (priv->cell_view), path);
			gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->invalid_notebook), VALID_PAGE);
		}
		else
		{
        	gtk_cell_view_set_displayed_row (GTK_CELL_VIEW (priv->cell_view), NULL);
			gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->invalid_notebook), INVALID_PAGE);
		}
	}

	g_signal_emit (combo, signals[CHANGED], 0);
}



/* Callbacks functions
 *---------------------------------------------------------------------------*/

static gboolean
anjuta_tree_can_select_node (GtkTreeSelection *selection,
                             GtkTreeModel *model,
                             GtkTreePath *path,
                             gboolean path_currently_selected,
                             gpointer data)
{
	AnjutaTreeComboBoxPrivate *priv = ANJUTA_TREE_COMBO_BOX (data)->priv;
	GtkTreeIter iter;

	return path_currently_selected ||
	    (priv->valid_func == NULL) ||
	    (gtk_tree_model_get_iter (priv->model, &iter, path) &&
	     priv->valid_func (priv->model, &iter, priv->valid_data));
}

static void
anjuta_tree_combo_box_auto_scroll (AnjutaTreeComboBox *combo,
                                   gint         x,
                                   gint         y)
{
	GtkAdjustment *adj;
	GtkAllocation allocation;
	GtkWidget *tree_view = combo->priv->tree_view;
	gdouble value;

	gtk_widget_get_allocation (tree_view, &allocation);

	adj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (combo->priv->scrolled_window));
	if (adj && gtk_adjustment_get_upper (adj) - gtk_adjustment_get_lower (adj) > gtk_adjustment_get_page_size (adj))
	{
		if (x <= allocation.x &&
		    gtk_adjustment_get_lower (adj) < gtk_adjustment_get_value (adj))
		{
			value = gtk_adjustment_get_value (adj) - (allocation.x - x + 1);
			gtk_adjustment_set_value (adj, value);
		}
		else if (x >= allocation.x + allocation.width &&
		         gtk_adjustment_get_upper (adj) - gtk_adjustment_get_page_size (adj) > gtk_adjustment_get_value (adj))
		{
			value = gtk_adjustment_get_value (adj) + (x - allocation.x - allocation.width + 1);
			gtk_adjustment_set_value (adj, MAX (value, 0.0));
		}
	}

	adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (combo->priv->scrolled_window));
	if (adj && gtk_adjustment_get_upper (adj) - gtk_adjustment_get_lower (adj) > gtk_adjustment_get_page_size (adj))
	{
		if (y <= allocation.y &&
		    gtk_adjustment_get_lower (adj) < gtk_adjustment_get_value (adj))
		{
			value = gtk_adjustment_get_value (adj) - (allocation.y - y + 1);
			gtk_adjustment_set_value (adj, value);
		}
		else if (y >= allocation.height &&
		         gtk_adjustment_get_upper (adj) - gtk_adjustment_get_page_size (adj) > gtk_adjustment_get_value (adj))
		{
			value = gtk_adjustment_get_value (adj) + (y - allocation.height + 1);
			gtk_adjustment_set_value (adj, MAX (value, 0.0));
		}
	}
}

static gboolean
anjuta_tree_combo_box_scroll_timeout (AnjutaTreeComboBox *combo)
{
	AnjutaTreeComboBoxPrivate *priv = combo->priv;
	gint x, y;

	if (priv->auto_scroll)
    {
		gdk_window_get_device_position (gtk_widget_get_window (priv->tree_view),
		                                priv->grab_pointer,
		                                &x, &y, NULL);
		anjuta_tree_combo_box_auto_scroll (combo, x, y);
    }

	return TRUE;
}

static gboolean
anjuta_tree_combo_box_key_press (GtkWidget   *widget,
                                 GdkEventKey *event,
                                 gpointer     user_data)
{
	AnjutaTreeComboBox *combo = ANJUTA_TREE_COMBO_BOX (user_data);
	GtkTreeIter iter;

	if (event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_ISO_Enter || event->keyval == GDK_KEY_KP_Enter ||
	    event->keyval == GDK_KEY_space || event->keyval == GDK_KEY_KP_Space)
	{
		GtkTreeModel *model = NULL;

		if (combo->priv->model)
		{
			GtkTreeSelection *sel;

			sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (combo->priv->tree_view));

			if (gtk_tree_selection_get_selected (sel, &model, &iter))
				anjuta_tree_combo_box_set_active_iter (combo, &iter);
		}

		anjuta_tree_combo_box_popdown (combo);

		return TRUE;
	}

	if (!gtk_bindings_activate_event (G_OBJECT (widget), event))
	{
		/* The tree hasn't managed the
		 * event, forward it to the combobox
		 */
		if (gtk_bindings_activate_event (G_OBJECT (combo), event)) return TRUE;
	}

	return FALSE;
}

static gboolean
anjuta_tree_combo_box_enter_notify (GtkWidget        *widget,
                                         GdkEventCrossing *event,
                                         gpointer          data)
{
  AnjutaTreeComboBox *combo = ANJUTA_TREE_COMBO_BOX (data);

  combo->priv->auto_scroll = TRUE;

  return TRUE;
}

static gboolean
anjuta_tree_combo_box_resize_idle (gpointer user_data)
{
	AnjutaTreeComboBox *combo = ANJUTA_TREE_COMBO_BOX (user_data);
	AnjutaTreeComboBoxPrivate *priv = combo->priv;
	gint x, y, width, height;

	if (priv->tree_view && gtk_widget_get_mapped (priv->popup_window))
	{
		anjuta_tree_combo_box_popup_position (combo, &x, &y, &width, &height);

		gtk_widget_set_size_request (priv->popup_window, width, height);
		gtk_window_move (GTK_WINDOW (priv->popup_window), x, y);
	}

	priv->resize_idle_id = 0;

	return FALSE;
}

static void
anjuta_tree_combo_box_popup_resize (AnjutaTreeComboBox *combo)
{
	AnjutaTreeComboBoxPrivate *priv = combo->priv;

	if (!priv->resize_idle_id)
	{
		priv->resize_idle_id = gdk_threads_add_idle (anjuta_tree_combo_box_resize_idle, combo);
	}
}

static void
anjuta_tree_combo_box_row_expanded (GtkTreeModel     *model,
                                          GtkTreePath      *path,
                                          GtkTreeIter      *iter,
                                          gpointer          user_data)
{
	AnjutaTreeComboBox *combo = ANJUTA_TREE_COMBO_BOX (user_data);

	anjuta_tree_combo_box_popup_resize (combo);
}

static gboolean
anjuta_tree_combo_box_button_pressed (GtkWidget      *widget,
                                      GdkEventButton *event,
                                      gpointer        user_data)
{
	AnjutaTreeComboBox *combo = ANJUTA_TREE_COMBO_BOX (user_data);
	AnjutaTreeComboBoxPrivate *priv = combo->priv;

	GtkWidget *ewidget = gtk_get_event_widget ((GdkEvent *)event);

	if (ewidget == priv->popup_window)
		return TRUE;

	if ((ewidget != GTK_WIDGET (combo)) ||
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (combo)))
	{
		return FALSE;
	}

	if (!gtk_widget_has_focus (GTK_WIDGET (combo)))
	{
		gtk_widget_grab_focus (GTK_WIDGET (combo));
	}

	anjuta_tree_combo_box_popup_for_device (combo, event->device);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (combo), TRUE);

	priv->auto_scroll = FALSE;
	if (priv->scroll_timer == 0)
		priv->scroll_timer = gdk_threads_add_timeout (SCROLL_TIME,
		                                              (GSourceFunc) anjuta_tree_combo_box_scroll_timeout,
		                                              combo);

	priv->popup_in_progress = TRUE;

	return TRUE;
}

static gboolean
anjuta_tree_combo_box_button_released (GtkWidget      *widget,
                                       GdkEventButton *event,
                                       gpointer        user_data)
{
	gboolean ret;
	GtkTreePath *path = NULL;
	GtkTreeIter iter;

	AnjutaTreeComboBox *combo = ANJUTA_TREE_COMBO_BOX (user_data);
	AnjutaTreeComboBoxPrivate *priv = combo->priv;

	gboolean popup_in_progress = FALSE;

	GtkWidget *ewidget = gtk_get_event_widget ((GdkEvent *)event);

	if (priv->popup_in_progress)
	{
		popup_in_progress = TRUE;
		priv->popup_in_progress = FALSE;
	}

	gtk_tree_view_set_hover_expand (GTK_TREE_VIEW (priv->tree_view),
	                                FALSE);
	if (priv->scroll_timer)
	{
		g_source_remove (priv->scroll_timer);
		priv->scroll_timer = 0;
	}

	if (ewidget != priv->tree_view)
	{
		if ((ewidget == GTK_WIDGET (combo)) &&
			!popup_in_progress &&
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (combo)))
		{
			anjuta_tree_combo_box_popdown (combo);
			return TRUE;
		}

		/* released outside treeview */
		if (ewidget != GTK_WIDGET (combo))
        {
			anjuta_tree_combo_box_popdown (combo);

			return TRUE;
		}

		return FALSE;
	}

	/* select something cool */
	ret = gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (priv->tree_view),
	                                     event->x, event->y,
	                                     &path,
	                                     NULL, NULL, NULL);

	if (!ret)
		return TRUE; /* clicked outside window? */

	gtk_tree_model_get_iter (priv->model, &iter, path);

	if (anjuta_tree_can_select_node (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree_view)),
	                                 priv->model,
	                                 path,
	                                 FALSE,
	                                 combo))
	{
		anjuta_tree_combo_box_popdown (combo);
		anjuta_tree_combo_box_set_active_iter (combo, &iter);
	}
	gtk_tree_path_free (path);

	return TRUE;
}

static void
anjuta_tree_combo_box_button_toggled (GtkWidget *widget,
                   gpointer   user_data)
{
	AnjutaTreeComboBox *combo = ANJUTA_TREE_COMBO_BOX (user_data);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
	{
        anjuta_tree_combo_box_popup (combo);
    }
	else
	{
		anjuta_tree_combo_box_popdown (combo);
	}
}

static void
anjuta_tree_combo_box_popup_setup (AnjutaTreeComboBox *combo)
{
	AnjutaTreeComboBoxPrivate *priv = combo->priv;
	GtkTreeSelection *sel;
	GtkWidget *toplevel;

	priv->popup_window = gtk_window_new (GTK_WINDOW_POPUP);
	gtk_window_set_type_hint (GTK_WINDOW (priv->popup_window),
	                          GDK_WINDOW_TYPE_HINT_COMBO);

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (combo));
	if (GTK_IS_WINDOW (toplevel))
	{
		gtk_window_group_add_window (gtk_window_get_group (GTK_WINDOW (toplevel)),
		                             GTK_WINDOW (priv->popup_window));
		gtk_window_set_transient_for (GTK_WINDOW (priv->popup_window),
		                              GTK_WINDOW (toplevel));
	}

	gtk_window_set_resizable (GTK_WINDOW (priv->popup_window), FALSE);
	gtk_window_set_screen (GTK_WINDOW (priv->popup_window),
	                       gtk_widget_get_screen (GTK_WIDGET (combo)));

	priv->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->scrolled_window),
	                                GTK_POLICY_NEVER,
	                                GTK_POLICY_NEVER);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (priv->scrolled_window),
	                                     GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (priv->popup_window),
	                   priv->scrolled_window);


	priv->tree_view = gtk_tree_view_new ();
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree_view));
	gtk_tree_selection_set_mode (sel, GTK_SELECTION_BROWSE);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (priv->tree_view), FALSE);
	gtk_tree_view_set_hover_selection (GTK_TREE_VIEW (priv->tree_view), TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (priv->tree_view),
                               gtk_tree_view_column_new_with_area (priv->area));
	gtk_container_add (GTK_CONTAINER (priv->scrolled_window),
	                   priv->tree_view);

	/* set sample/popup widgets */
	g_signal_connect (priv->tree_view, "key-press-event",
	                  G_CALLBACK (anjuta_tree_combo_box_key_press),
	                  combo);
	g_signal_connect (priv->tree_view, "enter-notify-event",
	                  G_CALLBACK (anjuta_tree_combo_box_enter_notify),
	                  combo);
	g_signal_connect (priv->tree_view, "row-expanded",
	                  G_CALLBACK (anjuta_tree_combo_box_row_expanded),
	                  combo);
	g_signal_connect (priv->tree_view, "row-collapsed",
	                  G_CALLBACK (anjuta_tree_combo_box_row_expanded),
	                  combo);
	g_signal_connect (priv->popup_window, "button-press-event",
	                  G_CALLBACK (anjuta_tree_combo_box_button_pressed),
	                  combo);
	g_signal_connect (priv->popup_window, "button-release-event",
	                  G_CALLBACK (anjuta_tree_combo_box_button_released),
	                  combo);

	gtk_widget_show (priv->scrolled_window);
}


/* Public functions
 *---------------------------------------------------------------------------*/

void
anjuta_tree_combo_box_set_model (AnjutaTreeComboBox *combo,
                                 GtkTreeModel *model)
{
	g_return_if_fail (ANJUTA_IS_TREE_COMBO_BOX (combo));

	anjuta_tree_combo_box_unset_model (combo);

	if (model != NULL)
	{
		AnjutaTreeComboBoxPrivate *priv = combo->priv;

		priv->model = g_object_ref (model);
		gtk_cell_view_set_model (GTK_CELL_VIEW (priv->cell_view), model);
		gtk_tree_view_set_model (GTK_TREE_VIEW (priv->tree_view), model);


		if (priv->active != -1)
    	{
     	 	/* If an index was set in advance, apply it now */
      		anjuta_tree_combo_box_set_active (combo, priv->active);
      		priv->active = -1;
    	}
		else
		{
			GtkTreeIter iter;
			gtk_tree_model_get_iter_first (model, &iter);
			anjuta_tree_combo_box_set_active_iter (combo, &iter);
		}
	}
}

GtkTreeModel*
anjuta_tree_combo_box_get_model (AnjutaTreeComboBox *combo)
{
	return combo->priv->model;
}

void
anjuta_tree_combo_box_set_active (AnjutaTreeComboBox *combo,
                              gint index)
{
	GtkTreePath *path = NULL;

	g_return_if_fail (ANJUTA_IS_TREE_COMBO_BOX (combo));
	g_return_if_fail (index >= -1);

	if (combo->priv->model == NULL)
	{
      /* Save index, in case the model is set after the index */
      combo->priv->active = index;
      return;
	}

	if (index != -1) path = gtk_tree_path_new_from_indices (index, -1);

	anjuta_tree_combo_box_set_active_path (combo, path);

	gtk_tree_path_free (path);
}


void
anjuta_tree_combo_box_set_active_iter (AnjutaTreeComboBox *combo,
                                   GtkTreeIter *iter)
{
	GtkTreePath *path = NULL;

	g_return_if_fail (ANJUTA_IS_TREE_COMBO_BOX (combo));

	if (iter)
	{
    	path = gtk_tree_model_get_path (combo->priv->model, iter);
	}

	anjuta_tree_combo_box_set_active_path (combo, path);

	gtk_tree_path_free (path);
}

gboolean
anjuta_tree_combo_box_get_active_iter (AnjutaTreeComboBox *combo,
                                   GtkTreeIter *iter)
{
	AnjutaTreeComboBoxPrivate *priv = combo->priv;
	GtkTreePath *path;
	gboolean valid = FALSE;

	if (priv->active_row != NULL)
	{
		path = gtk_tree_row_reference_get_path (priv->active_row);
		if (path)
		{
			valid = gtk_tree_model_get_iter (priv->model, iter, path);
			gtk_tree_path_free (path);
		}
	}

	return valid;
}

void
anjuta_tree_combo_box_set_valid_function (AnjutaTreeComboBox *combo,
                                          GtkTreeModelFilterVisibleFunc func,
                                          gpointer data,
                                          GDestroyNotify destroy)
{
	AnjutaTreeComboBoxPrivate *priv = combo->priv;
	GtkTreeSelection *selection;

	if (priv->valid_destroy != NULL)
	{
		priv->valid_destroy (priv->valid_data);
	}
	priv->valid_func = func;
	priv->valid_data = data;
	priv->valid_destroy = destroy;


	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree_view));
	gtk_tree_selection_set_select_function (selection, func != NULL ? anjuta_tree_can_select_node : NULL, func != NULL ? combo : NULL, NULL);
}

void
anjuta_tree_combo_box_set_invalid_text (AnjutaTreeComboBox *combo,
                                        const gchar *str)
{
	AnjutaTreeComboBoxPrivate *priv = combo->priv;

	gtk_label_set_text (GTK_LABEL (priv->invalid_label), str);
}


GtkWidget *
anjuta_tree_combo_box_new (void)
{
	return g_object_new (ANJUTA_TYPE_TREE_COMBO_BOX, NULL);
}



/* Implement GtkCellLayout interface
 *---------------------------------------------------------------------------*/

static GtkCellArea *
anjuta_tree_combo_box_cell_layout_get_area (GtkCellLayout *cell_layout)
{
	AnjutaTreeComboBox *combo = ANJUTA_TREE_COMBO_BOX (cell_layout);
	AnjutaTreeComboBoxPrivate *priv = combo->priv;

	return priv->area;
}


static void
anjuta_tree_combo_box_cell_layout_init (GtkCellLayoutIface *iface)
{
  iface->get_area = anjuta_tree_combo_box_cell_layout_get_area;
}



/* Implement GtkWidget functions
 *---------------------------------------------------------------------------*/

static void
anjuta_tree_combo_box_destroy (GtkWidget *widget)
{
	AnjutaTreeComboBox *combo = ANJUTA_TREE_COMBO_BOX (widget);
	AnjutaTreeComboBoxPrivate *priv = combo->priv;

	anjuta_tree_combo_box_popdown (combo);

	GTK_WIDGET_CLASS (anjuta_tree_combo_box_parent_class)->destroy (widget);

	priv->cell_view = NULL;
	priv->tree_view = NULL;
}



/* Implement GObject functions
 *---------------------------------------------------------------------------*/

static void
anjuta_tree_combo_box_dispose (GObject *object)
{
	AnjutaTreeComboBox *combo = ANJUTA_TREE_COMBO_BOX (object);
	AnjutaTreeComboBoxPrivate *priv = combo->priv;

	if (priv->area)
    {
      g_object_unref (priv->area);
      priv->area = NULL;
    }

	if (priv->valid_destroy != NULL)
	{
		priv->valid_destroy (priv->valid_data);
		priv->valid_destroy = NULL;
	}
	anjuta_tree_combo_box_unset_model (combo);

	anjuta_tree_combo_box_popup_destroy (combo);

	G_OBJECT_CLASS (anjuta_tree_combo_box_parent_class)->dispose (object);
}

static void
anjuta_tree_combo_box_init (AnjutaTreeComboBox *combo)
{
  	AnjutaTreeComboBoxPrivate *priv;
	GtkWidget *box, *sep, *arrow;
	GtkWidget *image, *ibox;

	priv = G_TYPE_INSTANCE_GET_PRIVATE (combo,
	                                    ANJUTA_TYPE_TREE_COMBO_BOX,
	                                    AnjutaTreeComboBoxPrivate);
	combo->priv = priv;

	priv->model = NULL;
	priv->active = -1;
	priv->active_row = NULL;

	priv->valid_func = NULL;
	priv->valid_destroy = NULL;

	gtk_widget_push_composite_child ();

	gtk_widget_set_halign (GTK_WIDGET (combo), GTK_ALIGN_FILL);
	gtk_widget_show (GTK_WIDGET (combo));

	g_signal_connect (combo, "button-press-event",
	                  G_CALLBACK (anjuta_tree_combo_box_button_pressed), combo);
	g_signal_connect (combo, "toggled",
	                  G_CALLBACK (anjuta_tree_combo_box_button_toggled), combo);


	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 3);
	gtk_container_add (GTK_CONTAINER (combo), box);
	gtk_widget_show (box);

	priv->invalid_notebook = gtk_notebook_new ();
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (priv->invalid_notebook), FALSE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (priv->invalid_notebook), FALSE);
	gtk_box_pack_start (GTK_BOX (box), priv->invalid_notebook, TRUE, TRUE, 0);

	ibox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_notebook_append_page (GTK_NOTEBOOK (priv->invalid_notebook), ibox, NULL);
	gtk_widget_show (ibox);

	image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_box_pack_start (GTK_BOX (ibox), image, FALSE, FALSE, 0);
	gtk_widget_show (image);

	priv->invalid_label = gtk_label_new (_("<Invalid>"));
	gtk_misc_set_alignment (GTK_MISC (priv->invalid_label), 0, 1);
	gtk_misc_set_padding (GTK_MISC (priv->invalid_label), 3, 3);
	gtk_box_pack_start (GTK_BOX (ibox), priv->invalid_label, TRUE, TRUE, 0);
	gtk_widget_show (priv->invalid_label);

	priv->area = gtk_cell_area_box_new ();
	g_object_ref_sink (priv->area);

	priv->cell_view = gtk_cell_view_new_with_context (priv->area, NULL);
	gtk_notebook_append_page (GTK_NOTEBOOK (priv->invalid_notebook), priv->cell_view, NULL);
	gtk_widget_show (priv->cell_view);

	sep = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
	gtk_box_pack_start (GTK_BOX (box), sep, FALSE, FALSE, 0);
	gtk_widget_show (sep);

	arrow = gtk_arrow_new (GTK_ARROW_DOWN,GTK_SHADOW_NONE);
	gtk_box_pack_start (GTK_BOX (box), arrow, FALSE, FALSE, 0);
	gtk_widget_show (arrow);

	gtk_widget_pop_composite_child ();

	gtk_widget_show_all (GTK_WIDGET (combo));

	anjuta_tree_combo_box_popup_setup (combo);
}

static void
anjuta_tree_combo_box_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
	AnjutaTreeComboBox *combo = ANJUTA_TREE_COMBO_BOX (object);

	switch (prop_id)
	{
    case PROP_MODEL:
		anjuta_tree_combo_box_set_model (combo, g_value_get_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
anjuta_tree_combo_box_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
	AnjutaTreeComboBox *combo = ANJUTA_TREE_COMBO_BOX (object);

	switch (prop_id)
	{
	case PROP_MODEL:
        g_value_set_object (value, combo->priv->model);
        break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
    }
}

static void
anjuta_tree_combo_box_class_init (AnjutaTreeComboBoxClass * class)
{
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;
	GtkBindingSet *binding_set;

	widget_class = GTK_WIDGET_CLASS (class);
	widget_class->destroy = anjuta_tree_combo_box_destroy;

	gobject_class = G_OBJECT_CLASS (class);
	gobject_class->dispose = anjuta_tree_combo_box_dispose;
	gobject_class->set_property = anjuta_tree_combo_box_set_property;
	gobject_class->get_property = anjuta_tree_combo_box_get_property;


	/* Signals */
	signals[CHANGED] = g_signal_new ("changed",
	                                 G_OBJECT_CLASS_TYPE (class),
	                                 G_SIGNAL_RUN_LAST,
	                                 G_STRUCT_OFFSET (AnjutaTreeComboBoxClass, changed),
	                                 NULL, NULL,
	                                 g_cclosure_marshal_VOID__VOID,
	                                 G_TYPE_NONE, 0);

	signals[POPUP] = g_signal_new_class_handler ("popup",
	                                             G_OBJECT_CLASS_TYPE (class),
	                                             G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
	                                             G_CALLBACK (anjuta_tree_combo_box_popup),
	                                             NULL, NULL,
	                                             g_cclosure_marshal_VOID__VOID,
	                                             G_TYPE_NONE, 0);

	signals[POPDOWN] = g_signal_new_class_handler ("popdown",
	                                               G_OBJECT_CLASS_TYPE (class),
	                                               G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
	                                               G_CALLBACK (anjuta_tree_combo_box_popdown),
	                                               NULL, NULL,
	                                               g_cclosure_marshal_VOID__VOID,
	                                               G_TYPE_NONE, 0);

	/**
	 * AnjutaTreeCombo:model:
	 *
	 * The model from which the combo box takes the values shown
	 * in the list.
	 *
	 */
	g_object_class_install_property (gobject_class,
	                                 PROP_MODEL,
	                                 g_param_spec_object ("model",
	                                                      _("ComboBox model"),
	                                                      _("The model for the combo box"),
	                                                      GTK_TYPE_TREE_MODEL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/* Key bindings */
	binding_set = gtk_binding_set_by_class (widget_class);

	gtk_binding_entry_add_signal (binding_set, GDK_KEY_Down, GDK_MOD1_MASK,
	                              "popup", 0);
  	gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Down, GDK_MOD1_MASK,
	                              "popup", 0);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_Up, GDK_MOD1_MASK,
	                              "popdown", 0);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Up, GDK_MOD1_MASK,
	                              "popdown", 0);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_Escape, 0,
	                              "popdown", 0);

	g_type_class_add_private (class, sizeof (AnjutaTreeComboBoxPrivate));
}
