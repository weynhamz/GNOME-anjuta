/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* eggcomboselect widget
 *
 * Copyright (C) Naba Kumar <naba@gnome.org>
 *
 * Codes taken from:
 * gtkcombo_select - combo_select widget for gtk+
 * Copyright 1999-2001 Adrian E. Feiguin <feiguin@ifir.edu.ar>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <string.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkarrow.h>
#include <gtk/gtkeventbox.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkwindow.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkcelllayout.h>
#include "gtkcellview.h"

#include <libegg/menu/eggcomboselect.h>

static void         egg_combo_select_class_init      (EggComboSelectClass *klass);
static void         egg_combo_select_init            (EggComboSelect      *combo_select);
static void         egg_combo_select_dispose         (GObject     *combo_select);
static void         egg_combo_select_finalize        (GObject     *combo_select);
static void         egg_combo_select_get_pos         (EggComboSelect      *combo_select, 
                                               	  gint          *x, 
                                                  gint          *y, 
                                                  gint          *height, 
                                                  gint          *width);
static gint         egg_combo_select_button_toggled    (GtkWidget * widget, 
					  	  EggComboSelect * combo_select);
static void         egg_combo_select_arrow_clicked   (GtkWidget * widget, 
					  	  EggComboSelect * combo_select);
static gint         egg_combo_select_button_press    (GtkWidget     *widget,
				                  GdkEvent      *event,
                                  gpointer data);
static gboolean     egg_combo_select_key_press (GtkWidget * widget, GdkEventKey * event, gpointer data);
static void         egg_combo_select_size_allocate   (GtkWidget     *widget,
					          GtkAllocation *allocation);
static void         egg_combo_select_size_request    (GtkWidget     *widget,
					          GtkRequisition *requisition);
static void         egg_combo_select_sync_cells (EggComboSelect   *combo_select,
							 GtkCellLayout *cell_layout);

struct _EggComboSelectPriv {	
	GtkWidget *arrow;
	GtkWidget *popup;
	GtkWidget *popwin;
	GtkWidget *button;
	GtkWidget *frame;
	GtkWidget *treeview;
	GtkTreeModel *model;
	GtkTreeViewColumn *column;
	GSList *cells;
	GtkWidget *cell_view;
	GtkWidget *cell_frame;
	GtkTreeRowReference *active_row;
	gchar *title;
};

typedef struct _ComboSelectCellInfo ComboSelectCellInfo;
struct _ComboSelectCellInfo
{
  GtkCellRenderer *cell;
  GSList *attributes;

  GtkCellLayoutDataFunc func;
  gpointer func_data;
  GDestroyNotify destroy;

  guint expand : 1;
  guint pack : 1;
};

enum {
  CHANGED,
  LAST_SIGNAL
};

static GtkHBoxClass *parent_class = NULL;
static guint         action_signals[LAST_SIGNAL] = { 0 };

/* Celllayout interface implementation */

static ComboSelectCellInfo *
cell_get_info (EggComboSelect     *combo_select,
                             GtkCellRenderer *cell)
{
  GSList *i;

  for (i = combo_select->priv->cells; i; i = i->next)
    {
      ComboSelectCellInfo *info = (ComboSelectCellInfo *)i->data;

      if (info && info->cell == cell)
        return info;
    }

  return NULL;
}

static void
cell_layout_pack_start (GtkCellLayout   *layout,
						  GtkCellRenderer *cell,
						  gboolean         expand)
{
  ComboSelectCellInfo *info;
  EggComboSelect *combo_select;

  g_return_if_fail (EGG_IS_COMBO_SELECT (layout));
  g_return_if_fail (GTK_IS_CELL_RENDERER (cell));

  combo_select = EGG_COMBO_SELECT (layout);

  g_object_ref (cell);
  gtk_object_sink (GTK_OBJECT (cell));

  info = g_new0 (ComboSelectCellInfo, 1);
  info->cell = cell;
  info->expand = expand;
  info->pack = GTK_PACK_START;

  combo_select->priv->cells = g_slist_append (combo_select->priv->cells, info);

  if (combo_select->priv->cell_view)
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_select->priv->cell_view),
                                cell, expand);

  if (combo_select->priv->column)
    gtk_tree_view_column_pack_start (combo_select->priv->column, cell, expand);
}

static void
cell_layout_pack_end (GtkCellLayout   *layout,
                                    GtkCellRenderer *cell,
                                    gboolean         expand)
{
  ComboSelectCellInfo *info;
  EggComboSelect *combo_select;

  g_return_if_fail (EGG_IS_COMBO_SELECT (layout));
  g_return_if_fail (GTK_IS_CELL_RENDERER (cell));

  combo_select = EGG_COMBO_SELECT (layout);

  g_object_ref (cell);
  gtk_object_sink (GTK_OBJECT (cell));

  info = g_new0 (ComboSelectCellInfo, 1);
  info->cell = cell;
  info->expand = expand;
  info->pack = GTK_PACK_END;

  combo_select->priv->cells = g_slist_append (combo_select->priv->cells, info);

  if (combo_select->priv->cell_view)
    gtk_cell_layout_pack_end (GTK_CELL_LAYOUT (combo_select->priv->cell_view),
                              cell, expand);

  if (combo_select->priv->column)
    gtk_tree_view_column_pack_end (combo_select->priv->column, cell, expand);
}

static void
cell_layout_clear_attributes (GtkCellLayout   *layout,
                                            GtkCellRenderer *cell)
{
  ComboSelectCellInfo *info;
  EggComboSelect *combo_select;
  GSList *list;

  g_return_if_fail (EGG_IS_COMBO_SELECT (layout));
  g_return_if_fail (GTK_IS_CELL_RENDERER (cell));

  combo_select = EGG_COMBO_SELECT (layout);

  info = cell_get_info (combo_select, cell);
  g_return_if_fail (info != NULL);

  list = info->attributes;
  while (list && list->next)
    {
      g_free (list->data);
      list = list->next->next;
    }
  g_slist_free (info->attributes);
  info->attributes = NULL;

  if (combo_select->priv->cell_view)
    gtk_cell_layout_clear_attributes (GTK_CELL_LAYOUT (combo_select->priv->cell_view), cell);

  if (combo_select->priv->column)
    gtk_cell_layout_clear_attributes (GTK_CELL_LAYOUT (combo_select->priv->column), cell);

  gtk_widget_queue_resize (GTK_WIDGET (combo_select));
}

static void
cell_layout_clear (GtkCellLayout *layout)
{
  EggComboSelect *combo_select;
  GSList *i;
  
  g_return_if_fail (EGG_IS_COMBO_SELECT (layout));

  combo_select = EGG_COMBO_SELECT (layout);
 
  if (combo_select->priv->cell_view)
    gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo_select->priv->cell_view));

  if (combo_select->priv->column)
    gtk_tree_view_column_clear (combo_select->priv->column);

  for (i = combo_select->priv->cells; i; i = i->next)
    {
     ComboSelectCellInfo *info = (ComboSelectCellInfo *)i->data;

      cell_layout_clear_attributes (layout, info->cell);
      g_object_unref (info->cell);
      g_free (info);
      i->data = NULL;
    }
  g_slist_free (combo_select->priv->cells);
  combo_select->priv->cells = NULL;
}

static void
cell_layout_add_attribute (GtkCellLayout   *layout,
                                         GtkCellRenderer *cell,
                                         const gchar     *attribute,
                                         gint             column)
{
  ComboSelectCellInfo *info;
  EggComboSelect *combo_select;

  g_return_if_fail (EGG_IS_COMBO_SELECT (layout));
  g_return_if_fail (GTK_IS_CELL_RENDERER (cell));

  combo_select = EGG_COMBO_SELECT (layout);

  info = cell_get_info (combo_select, cell);

  info->attributes = g_slist_prepend (info->attributes,
                                      GINT_TO_POINTER (column));
  info->attributes = g_slist_prepend (info->attributes,
                                      g_strdup (attribute));

  if (combo_select->priv->cell_view)
    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo_select->priv->cell_view),
                                   cell, attribute, column);

  if (combo_select->priv->column)
    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo_select->priv->column),
                                   cell, attribute, column);
  gtk_widget_queue_resize (GTK_WIDGET (combo_select));
}

static void
combo_cell_data_func (GtkCellLayout   *cell_layout,
		      GtkCellRenderer *cell,
		      GtkTreeModel    *tree_model,
		      GtkTreeIter     *iter,
		      gpointer         data)
{
  ComboSelectCellInfo *info = (ComboSelectCellInfo *)data;
  GtkWidget *parent = NULL;
  
  if (!info->func)
    return;

  (*info->func) (cell_layout, cell, tree_model, iter, info->func_data);
}

static void
cell_layout_set_cell_data_func (GtkCellLayout         *layout,
                                              GtkCellRenderer       *cell,
                                              GtkCellLayoutDataFunc  func,
                                              gpointer               func_data,
                                              GDestroyNotify         destroy)
{
  ComboSelectCellInfo *info;
  EggComboSelect *combo_select;

  g_return_if_fail (EGG_IS_COMBO_SELECT (layout));

  combo_select = EGG_COMBO_SELECT (layout);

  info = cell_get_info (combo_select, cell);
  g_return_if_fail (info != NULL);
  
  if (info->destroy)
    {
      GDestroyNotify d = info->destroy;

      info->destroy = NULL;
      d (info->func_data);
    }

  info->func = func;
  info->func_data = func_data;
  info->destroy = destroy;

  if (combo_select->priv->cell_view)
    gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (combo_select->priv->cell_view), cell, func, func_data, NULL);

  if (combo_select->priv->column)
    gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (combo_select->priv->column), cell, func, func_data, NULL);

  gtk_widget_queue_resize (GTK_WIDGET (combo_select));
}

static void
cell_layout_reorder (GtkCellLayout   *layout,
                                   GtkCellRenderer *cell,
                                   gint             position)
{
  ComboSelectCellInfo *info;
  EggComboSelect *combo_select;
  GSList *link;

  g_return_if_fail (EGG_IS_COMBO_SELECT (layout));
  g_return_if_fail (GTK_IS_CELL_RENDERER (cell));

  combo_select = EGG_COMBO_SELECT (layout);

  info = cell_get_info (combo_select, cell);

  g_return_if_fail (info != NULL);
  g_return_if_fail (position >= 0);

  link = g_slist_find (combo_select->priv->cells, info);

  g_return_if_fail (link != NULL);

  combo_select->priv->cells = g_slist_remove_link (combo_select->priv->cells, link);
  combo_select->priv->cells = g_slist_insert (combo_select->priv->cells, info,
                                           position);

  if (combo_select->priv->cell_view)
    gtk_cell_layout_reorder (GTK_CELL_LAYOUT (combo_select->priv->cell_view),
                             cell, position);

  if (combo_select->priv->column)
    gtk_cell_layout_reorder (GTK_CELL_LAYOUT (combo_select->priv->column),
                             cell, position);

  gtk_widget_queue_draw (GTK_WIDGET (combo_select));
}

static void
cell_layout_init (GtkCellLayoutIface *iface)
{
	iface->pack_start = cell_layout_pack_start;
	iface->pack_end = cell_layout_pack_end;
	iface->reorder = cell_layout_reorder;
	iface->clear = cell_layout_clear;
	iface->add_attribute = cell_layout_add_attribute;
	iface->set_cell_data_func = cell_layout_set_cell_data_func;
	iface->clear_attributes = cell_layout_clear_attributes;
}

/* EggComboSelect class */
static void
egg_combo_select_class_init (EggComboSelectClass * klass)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;

  parent_class = g_type_class_peek_parent (klass);
  object_class = (GObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;

  object_class->dispose = egg_combo_select_dispose;
  object_class->finalize = egg_combo_select_finalize;
  
  widget_class->size_allocate = egg_combo_select_size_allocate;
  widget_class->size_request = egg_combo_select_size_request;

  action_signals[CHANGED] =
	g_signal_new ("changed",
				  G_OBJECT_CLASS_TYPE (klass),
				  G_SIGNAL_RUN_FIRST,
				  G_STRUCT_OFFSET (EggComboSelectClass, changed),
				  NULL, NULL,
				  g_cclosure_marshal_VOID__VOID,
				  G_TYPE_NONE, 0);
}

static void
egg_combo_select_dispose (GObject * combo_select)
{
  if (EGG_COMBO_SELECT (combo_select)->priv->popwin)
  {
	egg_combo_select_popdown (EGG_COMBO_SELECT (combo_select));
  }
  if (EGG_COMBO_SELECT (combo_select)->priv->model)
  {
	  g_object_unref (EGG_COMBO_SELECT (combo_select)->priv->model);
	  EGG_COMBO_SELECT (combo_select)->priv->model = NULL;
  }
  if (G_OBJECT_CLASS (parent_class)->dispose)
    (*G_OBJECT_CLASS (parent_class)->dispose) (combo_select);
}

static void
egg_combo_select_finalize (GObject * combo_select)
{
  g_free (EGG_COMBO_SELECT(combo_select)->priv->title);
  g_free (EGG_COMBO_SELECT(combo_select)->priv);
  if (G_OBJECT_CLASS (parent_class)->finalize)
    (*G_OBJECT_CLASS (parent_class)->finalize) (combo_select);
}

/***************************************************************/

static void
egg_combo_select_get_position (EggComboSelect *combo_select, 
			     gint        *x, 
			     gint        *y, 
			     gint        *width,
			     gint        *height)
{
  GtkWidget *sample;
  GdkScreen *screen;
  gint monitor_num;
  GdkRectangle monitor;
  GtkRequisition popup_req;
  GtkPolicyType hpolicy, vpolicy;
  
  // sample = GTK_BIN (combo_select->priv->button)->child;
  sample = combo_select->priv->button;

  gdk_window_get_origin (sample->window, x, y);

  if (GTK_WIDGET_NO_WINDOW (sample))
    {
      *x += sample->allocation.x;
      *y += sample->allocation.y;
    }
  
  *width = sample->allocation.width;
  
/*       *x -= GTK_CONTAINER (combo_select->priv->button)->border_width +
	     GTK_WIDGET (combo_select->priv->button)->style->xthickness;
       *width += 2 * (GTK_CONTAINER (combo_select->priv->button)->border_width +
            GTK_WIDGET (combo_select->priv->button)->style->xthickness);
*/
  hpolicy = vpolicy = GTK_POLICY_NEVER;
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (combo_select->priv->frame),
				  hpolicy, vpolicy);
  gtk_widget_size_request (combo_select->priv->frame, &popup_req);
/*
  if (popup_req.width > *width)
    {
      hpolicy = GTK_POLICY_ALWAYS;
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (combo_select->priv->frame),
				      hpolicy, vpolicy);
      gtk_widget_size_request (combo_select->priv->frame, &popup_req);
    }*/
  *width = popup_req.width;
  *height = popup_req.height;

  screen = gtk_widget_get_screen (GTK_WIDGET (combo_select));
  monitor_num = gdk_screen_get_monitor_at_window (screen, 
						  GTK_WIDGET (combo_select)->window);
  gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

  if (*x < monitor.x)
    *x = monitor.x;
  else if (*x + *width > monitor.x + monitor.width)
    *x = monitor.x + monitor.width - *width;
  
  if (*y + sample->allocation.height + *height <= monitor.y + monitor.height)
    *y += sample->allocation.height;
  else if (*y - *height >= monitor.y)
    *y -= *height;
  else if (monitor.y + monitor.height - (*y + sample->allocation.height) > *y - monitor.y)
    {
      *y += sample->allocation.height;
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
      
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (combo_select->priv->frame),
				      hpolicy, vpolicy);
    }
} 

static gboolean
list_popup_resize_idle (gpointer user_data)
{
  EggComboSelect *combo_select;
  gint x, y, width, height;

  GDK_THREADS_ENTER ();

  combo_select = EGG_COMBO_SELECT (user_data);

  if (combo_select->priv->treeview &&
      GTK_WIDGET_MAPPED (combo_select->priv->popwin))
    {
      egg_combo_select_get_position (combo_select, &x, &y, &width, &height);
  
      gtk_widget_set_size_request (combo_select->priv->popwin, width, height);
      gtk_window_move (GTK_WINDOW (combo_select->priv->popwin), x, y);
    }

  GDK_THREADS_LEAVE ();

  return FALSE;
}

static void
egg_combo_select_popup_resize (EggComboSelect *combo_select)
{
  g_idle_add (list_popup_resize_idle, combo_select);
}

static void
on_treeview_selection_changed (GtkTreeSelection *selection,
								EggComboSelect *combo_select)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	
	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		egg_combo_select_set_active_iter (combo_select, &iter);
	}
	egg_combo_select_popdown (combo_select);
}

void
egg_combo_select_popup (EggComboSelect * combo_select)
{
  gint height, width, x, y;
  gint old_width, old_height;
  GtkWidget *event_box;
  GdkCursor *cursor;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
	
  combo_select->priv->popwin = gtk_window_new (GTK_WINDOW_POPUP);

  gtk_widget_ref (combo_select->priv->popwin);
  gtk_window_set_policy (GTK_WINDOW (combo_select->priv->popwin), 1, 1, 0);
  gtk_widget_set_events (combo_select->priv->popwin, GDK_KEY_PRESS_MASK);
  g_signal_connect (G_OBJECT (combo_select->priv->popwin), "button_press_event",
		      G_CALLBACK (egg_combo_select_button_press), combo_select);
  g_signal_connect (G_OBJECT (combo_select->priv->popwin), "key_press_event",
		      G_CALLBACK (egg_combo_select_key_press), combo_select);
 
  event_box = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (combo_select->priv->popwin), event_box);
  gtk_widget_show (event_box);

  gtk_widget_realize (event_box);
  cursor = gdk_cursor_new (GDK_TOP_LEFT_ARROW);
  gdk_window_set_cursor (event_box->window, cursor);
  gdk_cursor_unref (cursor);

  combo_select->priv->frame = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (event_box), combo_select->priv->frame);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (combo_select->priv->frame),
									   GTK_SHADOW_OUT);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (combo_select->priv->frame),
  								  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_show (combo_select->priv->frame);
  
  combo_select->priv->treeview = gtk_tree_view_new ();
  if (combo_select->priv->model)
	  gtk_tree_view_set_model (GTK_TREE_VIEW (combo_select->priv->treeview),
							   combo_select->priv->model);

  gtk_widget_show (combo_select->priv->treeview);
  gtk_container_add (GTK_CONTAINER (combo_select->priv->frame), combo_select->priv->treeview);
  
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (combo_select->priv->treeview));
  
  combo_select->priv->column = gtk_tree_view_column_new ();
  if (combo_select->priv->title)
	  gtk_tree_view_column_set_title (combo_select->priv->column,
  									  combo_select->priv->title);
  g_object_ref (combo_select->priv->column);
  egg_combo_select_sync_cells (combo_select,
						GTK_CELL_LAYOUT (combo_select->priv->column));
  
  gtk_tree_view_append_column (GTK_TREE_VIEW (combo_select->priv->treeview),
  							   combo_select->priv->column);

  old_width = combo_select->priv->popwin->allocation.width;
  old_height  = combo_select->priv->popwin->allocation.height;

  egg_combo_select_get_position (combo_select, &x, &y, &width, &height);
  gtk_widget_set_size_request (combo_select->priv->popwin, width, height);
  gtk_window_move (GTK_WINDOW (combo_select->priv->popwin), x, y);
  gtk_widget_show (combo_select->priv->popwin);
  
  gtk_grab_add (combo_select->priv->popwin);
  gdk_pointer_grab (combo_select->priv->popwin->window, TRUE,
		    GDK_BUTTON_PRESS_MASK | 
		    GDK_BUTTON_RELEASE_MASK |
		    GDK_POINTER_MOTION_MASK, 
		    NULL, NULL, GDK_CURRENT_TIME);
  if (egg_combo_select_get_active_iter (combo_select, &iter))
	  gtk_tree_selection_select_iter (selection, &iter);
  g_signal_connect (G_OBJECT (selection), "changed",
  					G_CALLBACK (on_treeview_selection_changed), combo_select);
}

void
egg_combo_select_popdown (EggComboSelect *combo_select)
{
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(combo_select->priv->button), FALSE);

  gtk_grab_remove(combo_select->priv->popwin);
  gdk_pointer_ungrab(GDK_CURRENT_TIME);
  gtk_widget_hide(combo_select->priv->popwin);

  g_object_unref (combo_select->priv->column);
  gtk_widget_destroy (combo_select->priv->popwin);

  combo_select->priv->popwin = NULL;
  combo_select->priv->treeview = NULL;
  combo_select->priv->column = NULL;
  combo_select->priv->frame = NULL;
}

static gint
egg_combo_select_button_toggled (GtkWidget * widget, EggComboSelect * combo_select)
{
  GtkToggleButton *button;

  button = GTK_TOGGLE_BUTTON(widget);

  if(!button->active){
     gtk_widget_hide (combo_select->priv->popwin);
     gtk_grab_remove (combo_select->priv->popwin);
     gdk_pointer_ungrab (GDK_CURRENT_TIME);
     return TRUE;
  }

  egg_combo_select_popup (combo_select);
  return TRUE;
}

static void
egg_combo_select_arrow_clicked (GtkWidget *widget, EggComboSelect * combo_select)
{
	g_signal_emit_by_name (G_OBJECT (combo_select), "changed");
}

static void
egg_combo_select_sync_cells (EggComboSelect   *combo_select,
							 GtkCellLayout *cell_layout)
{
  GSList *k;

  for (k = combo_select->priv->cells; k; k = k->next)
    {
      GSList *j;
      ComboSelectCellInfo *info = (ComboSelectCellInfo *)k->data;

      if (info->pack == GTK_PACK_START)
        gtk_cell_layout_pack_start (cell_layout,
                                    info->cell, info->expand);
      else if (info->pack == GTK_PACK_END)
        gtk_cell_layout_pack_end (cell_layout,
                                  info->cell, info->expand);

      gtk_cell_layout_set_cell_data_func (cell_layout,
                                          info->cell,
                                          combo_cell_data_func, info, NULL);

      for (j = info->attributes; j; j = j->next->next)
        {
          gtk_cell_layout_add_attribute (cell_layout,
                                         info->cell,
                                         j->data,
                                         GPOINTER_TO_INT (j->next->data));
        }
    }
}

static void
egg_combo_select_init (EggComboSelect * combo_select)
{
  GtkWidget *widget;
  GtkWidget *arrow;

  widget=GTK_WIDGET(combo_select);

  combo_select->priv = g_new0 (EggComboSelectPriv, 1);
  combo_select->priv->model = NULL;
  combo_select->priv->popwin = NULL;
  combo_select->priv->frame = NULL;
  combo_select->priv->column = NULL;
  combo_select->priv->active_row = NULL;
  combo_select->priv->title = NULL;

  gtk_box_set_spacing (GTK_BOX (widget), 5);
  gtk_box_set_homogeneous (GTK_BOX (widget), FALSE);

  /* Create button and cell view */
  combo_select->priv->button = gtk_toggle_button_new ();
  gtk_widget_show (combo_select->priv->button);
  gtk_box_pack_start (GTK_BOX (combo_select), combo_select->priv->button, FALSE, FALSE, 0);

  combo_select->priv->cell_view = gtk_cell_view_new ();
  gtk_widget_show (combo_select->priv->cell_view);
  gtk_container_add (GTK_CONTAINER (combo_select->priv->button),
					 combo_select->priv->cell_view);

  /* Create arrow widget */
  combo_select->priv->arrow = gtk_button_new ();
  gtk_widget_show (combo_select->priv->arrow);
  arrow = gtk_arrow_new (GTK_ARROW_LEFT, GTK_SHADOW_NONE);
  gtk_widget_show (arrow);
  gtk_container_add (GTK_CONTAINER (combo_select->priv->arrow), arrow);
  gtk_box_pack_start (GTK_BOX (combo_select), combo_select->priv->arrow, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (combo_select->priv->button), "toggled",
		      	G_CALLBACK (egg_combo_select_button_toggled), combo_select);
  g_signal_connect (G_OBJECT (combo_select->priv->arrow), "clicked",
		        G_CALLBACK (egg_combo_select_arrow_clicked), combo_select);
}

GType
egg_combo_select_get_type ()
{
  static GType combo_select_type = 0;

  if (!combo_select_type)
    {
       static const GTypeInfo combo_select_info =
       {
          sizeof (EggComboSelectClass),
          NULL, /* base_init */
          NULL, /* base_finalize */
          (GClassInitFunc) egg_combo_select_class_init,
          NULL, /* class_finalize */
          NULL, /* class_data */
          sizeof (EggComboSelect),
          0,
          (GInstanceInitFunc) egg_combo_select_init
      };
	  static const GInterfaceInfo cell_layout_info =
      {
          (GInterfaceInitFunc) cell_layout_init,
          NULL,
          NULL
      };
      combo_select_type = g_type_register_static (GTK_TYPE_HBOX,
                                               "EggComboSelect",
                                               &combo_select_info,
                                               0);
      g_type_add_interface_static (combo_select_type,
                                   GTK_TYPE_CELL_LAYOUT,
                                   &cell_layout_info);
    }
  return combo_select_type;
}

GtkWidget *
egg_combo_select_new ()
{
  EggComboSelect *combo_select;

  combo_select = gtk_type_new (egg_combo_select_get_type ());

  return(GTK_WIDGET(combo_select));

}

typedef struct {
  EggComboSelect *combo;
  GtkTreePath *path;
  GtkTreeIter iter;
  gboolean found;
  gboolean set;
  gboolean visible;
} SearchData;

static gboolean
path_visible (GtkTreeView *view,
	      GtkTreePath *path)
{
  /* Note that we assume all paths visible. FIXME.
   */
  return TRUE;
}

static gboolean
tree_column_row_is_sensitive (EggComboSelect *combo_select,
			      GtkTreeIter *iter)
{
  return TRUE;
}

static gboolean
tree_next_func (GtkTreeModel *model,
		GtkTreePath  *path,
		GtkTreeIter  *iter,
		gpointer      data)
{
  SearchData *search_data = (SearchData *)data;

  if (search_data->found) 
    {
      if (!tree_column_row_is_sensitive (search_data->combo, iter))
	return FALSE;
      
      if (search_data->visible &&
	  !path_visible (GTK_TREE_VIEW (search_data->combo->priv->treeview), path))
	return FALSE;

      search_data->set = TRUE;
      search_data->iter = *iter;

      return TRUE;
    }
 
  if (gtk_tree_path_compare (path, search_data->path) == 0)
    search_data->found = TRUE;
  
  return FALSE;
}

static gboolean
tree_next (EggComboSelect  *combo,
	   GtkTreeModel *model,
	   GtkTreeIter  *iter,
	   GtkTreeIter  *next,
	   gboolean      visible)
{
  SearchData search_data;

  search_data.combo = combo;
  search_data.path = gtk_tree_model_get_path (model, iter);
  search_data.visible = visible;
  search_data.found = FALSE;
  search_data.set = FALSE;

  gtk_tree_model_foreach (model, tree_next_func, &search_data);
  
  *next = search_data.iter;

  gtk_tree_path_free (search_data.path);

  return search_data.set;
}

static gboolean
tree_prev_func (GtkTreeModel *model,
		GtkTreePath  *path,
		GtkTreeIter  *iter,
		gpointer      data)
{
  SearchData *search_data = (SearchData *)data;

  if (gtk_tree_path_compare (path, search_data->path) == 0)
    {
      search_data->found = TRUE;
      return TRUE;
    }
  
  if (!tree_column_row_is_sensitive (search_data->combo, iter))
    return FALSE;
      
  if (search_data->visible &&
      !path_visible (GTK_TREE_VIEW (search_data->combo->priv->treeview), path))
    return FALSE; 
  
  search_data->set = TRUE;
  search_data->iter = *iter;
  
  return FALSE; 
}

static gboolean
tree_prev (EggComboSelect  *combo,
	   GtkTreeModel *model,
	   GtkTreeIter  *iter,
	   GtkTreeIter  *prev,
	   gboolean      visible)
{
  SearchData search_data;

  search_data.combo = combo;
  search_data.path = gtk_tree_model_get_path (model, iter);
  search_data.visible = visible;
  search_data.found = FALSE;
  search_data.set = FALSE;

  gtk_tree_model_foreach (model, tree_prev_func, &search_data);
  
  *prev = search_data.iter;

  gtk_tree_path_free (search_data.path);

  return search_data.set;
}

static gboolean
tree_last_func (GtkTreeModel *model,
		GtkTreePath  *path,
		GtkTreeIter  *iter,
		gpointer      data)
{
  SearchData *search_data = (SearchData *)data;

  if (!tree_column_row_is_sensitive (search_data->combo, iter))
    return FALSE;
      
  /* Note that we rely on the fact that collapsed rows don't have nodes 
   */
  if (search_data->visible &&
      !path_visible (GTK_TREE_VIEW (search_data->combo->priv->treeview), path))
    return FALSE; 
  
  search_data->set = TRUE;
  search_data->iter = *iter;
  
  return FALSE; 
}

static gboolean
tree_last (EggComboSelect  *combo,
	   GtkTreeModel *model,
	   GtkTreeIter  *last,
	   gboolean      visible)
{
  SearchData search_data;

  search_data.combo = combo;
  search_data.visible = visible;
  search_data.set = FALSE;

  gtk_tree_model_foreach (model, tree_last_func, &search_data);
  
  *last = search_data.iter;

  return search_data.set;  
}


static gboolean
tree_first_func (GtkTreeModel *model,
		 GtkTreePath  *path,
		 GtkTreeIter  *iter,
		 gpointer      data)
{
  SearchData *search_data = (SearchData *)data;

  if (!tree_column_row_is_sensitive (search_data->combo, iter))
    return FALSE;
  
  if (search_data->visible &&
      !path_visible (GTK_TREE_VIEW (search_data->combo->priv->treeview), path))
    return FALSE;
  
  search_data->set = TRUE;
  search_data->iter = *iter;
  
  return TRUE;
}

static gboolean
tree_first (EggComboSelect  *combo,
	    GtkTreeModel *model,
	    GtkTreeIter  *first,
	    gboolean      visible)
{
  SearchData search_data;
  
  search_data.combo = combo;
  search_data.visible = visible;
  search_data.set = FALSE;

  gtk_tree_model_foreach (model, tree_first_func, &search_data);
  
  *first = search_data.iter;

  return search_data.set;  
}

static gboolean
egg_combo_select_key_press (GtkWidget * widget, GdkEventKey * event, gpointer data)
{
  EggComboSelect *combo_select = EGG_COMBO_SELECT (data);
  guint state = event->state & gtk_accelerator_get_default_mod_mask ();
  gboolean found;
  GtkTreeIter iter;
  GtkTreeIter new_iter;
  GtkTreeSelection *selection;

  if (combo_select->priv->model == NULL)
    return FALSE;

  if ((event->keyval == GDK_Down || event->keyval == GDK_KP_Down) && 
      state == GDK_MOD1_MASK)
    {
      egg_combo_select_popup (combo_select);

      return TRUE;
    }
  if (event->keyval == GDK_Escape || event->keyval == GDK_Return || 
      event->keyval == GDK_space)
    {
      egg_combo_select_popdown (combo_select);

      return TRUE;
    }

  switch (event->keyval) 
    {
    case GDK_Down:
    case GDK_KP_Down:
      if (egg_combo_select_get_active_iter (combo_select, &iter))
	{
	  found = tree_next (combo_select, combo_select->priv->model, 
			     &iter, &new_iter, FALSE);
	  break;
	}
      /* else fall through */
    case GDK_Page_Up:
    case GDK_KP_Page_Up:
    case GDK_Home: 
    case GDK_KP_Home:
      found = tree_first (combo_select, combo_select->priv->model, &new_iter, FALSE);
      break;

    case GDK_Up:
    case GDK_KP_Up:
      if (egg_combo_select_get_active_iter (combo_select, &iter))
	{
	  found = tree_prev (combo_select, combo_select->priv->model, 
			     &iter, &new_iter, FALSE);
	  break;
	}
      /* else fall through */      
    case GDK_Page_Down:
    case GDK_KP_Page_Down:
    case GDK_End: 
    case GDK_KP_End:
      found = tree_last (combo_select, combo_select->priv->model, &new_iter, FALSE);
      break;
    default:
      return FALSE;
    }

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (combo_select->priv->treeview));
  g_signal_handlers_block_by_func (G_OBJECT (selection),
					G_CALLBACK (on_treeview_selection_changed), combo_select);	
  if (found)
    egg_combo_select_set_active_iter (combo_select, &new_iter);
  g_signal_handlers_unblock_by_func (G_OBJECT (selection),
					G_CALLBACK (on_treeview_selection_changed), combo_select);	
  
  return TRUE;
}


static gint
egg_combo_select_button_press (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  GtkWidget *child;

  child = gtk_get_event_widget (event);

  if (child != widget)
    {
      while (child)
		{
		  if (child == widget)
			return FALSE;
		  child = child->parent;
		}
  }
  egg_combo_select_popdown (EGG_COMBO_SELECT (data));
  /*
  gtk_widget_hide (widget);
  gtk_grab_remove (widget);
  gdk_pointer_ungrab (GDK_CURRENT_TIME);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(EGG_COMBO_SELECT(data)->priv->button), FALSE);
  */
  return TRUE;
}

static void
egg_combo_select_size_request (GtkWidget *widget,
			   GtkRequisition *requisition)
{
  EggComboSelect *combo_select;
  GtkRequisition box_requisition;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (EGG_IS_COMBO_SELECT (widget));
  g_return_if_fail (requisition != NULL);

  GTK_WIDGET_CLASS (parent_class)->size_request (widget, &box_requisition);

  combo_select=EGG_COMBO_SELECT(widget);
/*
  size = MIN(box_requisition.width, box_requisition.height);
  size = MIN(combo_select->priv->button->requisition.width, box_requisition.height);
  size = MIN(combo_select->priv->button->requisition.width, box_requisition.height);

  widget->requisition.height = size;
  widget->requisition.width = size + combo_select->priv->arrow->requisition.width;
*/
  widget->requisition.height = box_requisition.height;
  widget->requisition.width = box_requisition.width;
}

static void
egg_combo_select_size_allocate (GtkWidget     *widget,
			 GtkAllocation *allocation)
{
  EggComboSelect *combo_select;
  GtkAllocation button_allocation;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (EGG_IS_COMBO_SELECT (widget));
  g_return_if_fail (allocation != NULL);

  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  combo_select = EGG_COMBO_SELECT (widget);

  button_allocation = combo_select->priv->button->allocation;
/*
  button_allocation.width = MIN(button_allocation.width, 
                                combo_select->priv->button->requisition.width);
  button_allocation.height = MIN(button_allocation.height, 
                               combo_select->priv->button->requisition.height);
  button_allocation.x += (combo_select->priv->button->allocation.width-
                        button_allocation.width) / 2;
  button_allocation.y += (combo_select->priv->button->allocation.height-
                        button_allocation.height) / 2;
*/

  gtk_widget_size_allocate (combo_select->priv->button, &button_allocation);

  button_allocation.x=combo_select->priv->button->allocation.x +
                      combo_select->priv->button->allocation.width;
  button_allocation.width=combo_select->priv->arrow->requisition.width;
  gtk_widget_size_allocate (combo_select->priv->arrow, &button_allocation);
}

void
egg_combo_select_set_model (EggComboSelect *combo_select, GtkTreeModel *model)
{
	g_return_if_fail (EGG_IS_COMBO_SELECT (combo_select));
	g_return_if_fail (GTK_IS_TREE_MODEL (model));
	
	if (combo_select->priv->model == NULL)
		egg_combo_select_sync_cells (combo_select,
						GTK_CELL_LAYOUT (combo_select->priv->cell_view));
	
	g_object_ref (model);
	
	/* Unset model */
	if (combo_select->priv->active_row)
	{
		gtk_tree_row_reference_free (combo_select->priv->active_row);
		combo_select->priv->active_row = NULL;
	}

	if (combo_select->priv->model)
	{
		g_object_unref (combo_select->priv->model);
		combo_select->priv->model = NULL;
	}

	/* Set model */
	combo_select->priv->model = model;
	if (combo_select->priv->treeview)
		gtk_tree_view_set_model (GTK_TREE_VIEW (combo_select->priv->treeview),
								 model);
    gtk_cell_view_set_model (GTK_CELL_VIEW (combo_select->priv->cell_view),
			    			 model);
	/*if (combo_select->priv->cell_view)
		gtk_cell_view_set_displayed_row (GTK_CELL_VIEW (combo_select->priv->cell_view),
		NULL);
	*/
}

static void
egg_combo_select_set_active_internal (EggComboSelect *combo_select,  GtkTreePath *path)
{
  GtkTreePath *active_path;
  gint path_cmp;
	
  if (combo_select->priv->model == NULL)
	  return;

  if (path && combo_select->priv->active_row)
    {
      active_path = gtk_tree_row_reference_get_path (combo_select->priv->active_row);
      if (active_path)
	{
	  path_cmp = gtk_tree_path_compare (path, active_path);
	  gtk_tree_path_free (active_path);
	  if (path_cmp == 0)
	    return;
	}
  }

  if (combo_select->priv->active_row)
    {
      gtk_tree_row_reference_free (combo_select->priv->active_row);
      combo_select->priv->active_row = NULL;
    }
  
  if (!path)
    {
      if (combo_select->priv->treeview)
        gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (GTK_TREE_VIEW (combo_select->priv->treeview)));

      if (combo_select->priv->cell_view)
        gtk_cell_view_set_displayed_row (GTK_CELL_VIEW (combo_select->priv->cell_view), NULL);
    }
  else
    {
      combo_select->priv->active_row = 
	gtk_tree_row_reference_new (combo_select->priv->model, path);

      if (combo_select->priv->treeview)
		{
		  gtk_tree_view_set_cursor (GTK_TREE_VIEW (combo_select->priv->treeview), 
						path, NULL, FALSE);
		}

      if (combo_select->priv->cell_view)
		gtk_cell_view_set_displayed_row (GTK_CELL_VIEW (combo_select->priv->cell_view), 
						 path);
    }
  g_signal_emit_by_name (combo_select, "changed", NULL, NULL);
}

void
egg_combo_select_set_active (EggComboSelect *combo_select, gint iter_index)
{
  GtkTreePath *path = NULL;
  g_return_if_fail (EGG_IS_COMBO_SELECT (combo_select));
  g_return_if_fail (iter_index >= -1);

  if (combo_select->priv->model == NULL)
	  return;

  if (iter_index != -1)
    path = gtk_tree_path_new_from_indices (iter_index, -1);
   
  egg_combo_select_set_active_internal (combo_select, path);

  if (path)
    gtk_tree_path_free (path);
}

gint
egg_combo_select_get_active (EggComboSelect *combo_select)
{
  gint result;
  g_return_val_if_fail (EGG_IS_COMBO_SELECT (combo_select), 0);

  if (combo_select->priv->active_row)
    {
      GtkTreePath *path;
      path = gtk_tree_row_reference_get_path (combo_select->priv->active_row);
      if (path == NULL)
	result = -1;
      else
	{
	  result = gtk_tree_path_get_indices (path)[0];
	  gtk_tree_path_free (path);
	}
    }
  else
    result = -1;

  return result;
}

void
egg_combo_select_set_active_iter (EggComboSelect *combo_select,
								  GtkTreeIter *iter)
{
  GtkTreePath *path;

  g_return_if_fail (EGG_IS_COMBO_SELECT (combo_select));
  if (combo_select->priv->model == NULL)
	  return;
  path = gtk_tree_model_get_path (combo_select->priv->model, iter);
  egg_combo_select_set_active_internal (combo_select, path);
  gtk_tree_path_free (path);
}

gboolean
egg_combo_select_get_active_iter (EggComboSelect *combo_select,
								  GtkTreeIter *iter)
{
  GtkTreePath *path;
  gboolean result;

  g_return_val_if_fail (EGG_IS_COMBO_SELECT (combo_select), FALSE);

  if (!combo_select->priv->active_row)
    return FALSE;
  path = gtk_tree_row_reference_get_path (combo_select->priv->active_row);
  if (!path)
    return FALSE;
  result = gtk_tree_model_get_iter (combo_select->priv->model, iter, path);
  gtk_tree_path_free (path);

  return result;
}

void
egg_combo_select_set_title (EggComboSelect *combo_select, const gchar *title)
{
	g_free (combo_select->priv->title);
	combo_select->priv->title = g_strdup (title);
	if (combo_select->priv->column)
		gtk_tree_view_column_set_title (combo_select->priv->column, title);
}
