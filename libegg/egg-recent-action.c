/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* egg-recent-action widget
 *
 * Copyright (C) Naba Kumar <naba@gnome.org>
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
 */
#include <stdio.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtktoggletoolbutton.h>
#include <libegg/recent-files/egg-recent-view.h>
#include <libegg/recent-files/egg-recent-view-gtk.h>
#include "egg-recent-action.h"

#ifndef _
# define _(s) (s)
#endif

struct _EggRecentActionPriv
{
	GList *recent_models;
	gint size;
	gchar *selected_uri;
};

static void egg_recent_action_init       (EggRecentAction *action);
static void egg_recent_action_class_init (EggRecentActionClass *class);

static GtkWidget * create_tool_item        (GtkAction *action);
static GtkWidget * create_menu_item        (GtkAction *action);
static void connect_proxy                  (GtkAction *action,
										    GtkWidget *proxy);
static void disconnect_proxy               (GtkAction *action,
										    GtkWidget *proxy);
static void egg_recent_action_finalize      (GObject *object);
static void egg_recent_action_dispose       (GObject *object);
static gboolean on_recent_select (EggRecentView *recent,  EggRecentItem *item,
							  EggRecentAction *plugin);

static GObjectClass *parent_class = NULL;

GType
egg_recent_action_get_type (void)
{
  static GtkType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info =
      {
        sizeof (EggRecentActionClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) egg_recent_action_class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        
        sizeof (EggRecentAction),
        0, /* n_preallocs */
        (GInstanceInitFunc) egg_recent_action_init,
      };

      type = g_type_register_static (GTK_TYPE_ACTION,
                                     "EggRecentAction",
                                     &type_info, 0);
    }
  return type;
}

static void
egg_recent_action_class_init (EggRecentActionClass *class)
{
  GtkActionClass *action_class;
  GObjectClass   *object_class;

  parent_class = g_type_class_peek_parent (class);
  action_class = GTK_ACTION_CLASS (class);
  object_class = G_OBJECT_CLASS (class);

  object_class->finalize     = egg_recent_action_finalize;
  object_class->dispose     = egg_recent_action_dispose;
 
  action_class->connect_proxy = connect_proxy;
  action_class->disconnect_proxy = disconnect_proxy;
  action_class->menu_item_type = GTK_TYPE_MENU_ITEM;
  action_class->toolbar_item_type = GTK_TYPE_TOOL_ITEM;
  action_class->create_tool_item = create_tool_item;
  action_class->create_menu_item = create_menu_item;
}

static void
egg_recent_action_init (EggRecentAction *action)
{
	action->priv = g_new0 (EggRecentActionPriv, 1);
	action->priv->recent_models = NULL;
	action->priv->selected_uri = NULL;
}

static void
egg_recent_action_dispose (GObject *object)
{
	EggRecentActionPriv *priv;
	
	priv = EGG_RECENT_ACTION (object)->priv;
	if (priv->recent_models)
	{
		GList *node;
		node = priv->recent_models;
		while (node)
		{
			g_object_unref (node->data);
			node = g_list_next (node);
		}
		g_list_free (priv->recent_models);
		priv->recent_models = NULL;
	}
  if (parent_class->dispose)
  	parent_class->dispose (object);
}

static void
egg_recent_action_finalize (GObject *object)
{
  g_free (EGG_RECENT_ACTION (object)->priv->selected_uri);
  g_free (EGG_RECENT_ACTION (object)->priv);
  if (parent_class->finalize)
  	parent_class->finalize (object);
}

static gboolean
on_recent_select (EggRecentView *view, EggRecentItem *item,
					 EggRecentAction *action)
{
	gchar *uri;
	gboolean ret = TRUE;

	uri = egg_recent_item_get_uri (item);
	if (uri)
	{
		g_free (action->priv->selected_uri);
		action->priv->selected_uri = g_strdup (uri);
		g_free (uri);
		gtk_action_activate (GTK_ACTION (action));
	}
	return ret;
}

static void
on_recent_files_tooltip (GtkTooltips *tooltips, GtkWidget *menu_item,
						 EggRecentItem *item, gpointer user_data)
{
	char *uri, *tip;

	uri = egg_recent_item_get_uri_for_display (item);
	tip = g_strdup_printf ("Open '%s'", uri);

	gtk_tooltips_set_tip (tooltips, menu_item, tip, NULL);

	g_free (tip);
	g_free (uri);
}

static void
update_recent_submenu (EggRecentAction *action, GtkWidget *submenu,
					   EggRecentModel *model, gint id)
{
	EggRecentViewGtk *recent_view;
	GtkWidget *sep = NULL;
	gchar buff[64];
	
	if (id != 0)
	{
		sep = gtk_menu_item_new ();
		gtk_widget_show (sep);
		gtk_menu_shell_append (GTK_MENU_SHELL (submenu), sep);
	}
	recent_view = egg_recent_view_gtk_new (submenu, sep);
	egg_recent_view_gtk_set_tooltip_func (recent_view,
										  on_recent_files_tooltip, NULL);
	egg_recent_view_set_model (EGG_RECENT_VIEW (recent_view),
							   model);
	snprintf (buff, 64, "recent-view-%d", id);
	g_object_set_data_full (G_OBJECT (submenu), buff, recent_view,
							g_object_unref);
	g_signal_connect (G_OBJECT (recent_view), "activate",
					  G_CALLBACK (on_recent_select), action);
}

static GtkWidget*
create_recent_submenu (EggRecentAction *action)
{
	GtkWidget *submenu;
	GList *node;
	gint id;
	
	submenu = gtk_menu_new ();
	node = action->priv->recent_models;
	id = 0;
	while (node)
	{
		update_recent_submenu (action, submenu, node->data, id);
		id++;
		node = g_list_next (node);
	}
	return submenu;
}

static void
on_reposition_menu (GtkMenu *menu, gint *x, gint *y, gboolean *push_in,
					gpointer data)
{
	gint x1, y1, w, h, d;
	GtkWidget *button = GTK_WIDGET (data);
	gdk_window_get_origin (button->window, x, y);
	gdk_window_get_geometry (button->window, &x1, &y1, &w, &h, &d);
	*y += button->allocation.height;
	if (GTK_WIDGET_NO_WINDOW (button))
	{
		*x += button->allocation.x;
		*y += button->allocation.y;
	}
}

static void
on_selection_done_menu (GtkMenu *menu, GtkWidget *button)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);
}

static void
on_button_pressed (GtkToggleToolButton *button, EggRecentAction *action)
{
	GtkWidget *submenu;

	/* g_message ("Recent toolitem toggled"); */
	submenu = g_object_get_data (G_OBJECT (button), "submenu");
	if (!submenu)
	{
		submenu = create_recent_submenu (action);
		g_object_set_data_full (G_OBJECT (button), "submenu", submenu,
								(GDestroyNotify)gtk_widget_destroy);
		g_object_set_data (G_OBJECT (submenu), "thebutton", button);
		g_signal_connect (G_OBJECT (submenu), "selection-done",
						  G_CALLBACK (on_selection_done_menu), button);
		gtk_widget_show (submenu);
	}
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
	{
		gtk_menu_popup (GTK_MENU (submenu), NULL, NULL, on_reposition_menu,
						button, 0, gtk_get_current_event_time ());
		gtk_menu_reposition (GTK_MENU (submenu));
	}
	else
	{
		gtk_menu_popdown (GTK_MENU (submenu));
	}
}

static GtkWidget *
create_tool_item (GtkAction *action)
{
  GtkWidget *arrow;
  GtkWidget *button;
  GtkToolItem *item;
	
  g_return_val_if_fail (EGG_IS_RECENT_ACTION (action), NULL);
  /* g_message ("Creating recent toolitem"); */
	
  item = gtk_tool_item_new ();
  gtk_widget_show(GTK_WIDGET (item));
  button = gtk_toggle_button_new ();
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_container_add (GTK_CONTAINER (item), button);
  gtk_widget_show(GTK_WIDGET (button));
  arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
  gtk_widget_show (arrow);
  gtk_container_add (GTK_CONTAINER (button), arrow);
  return GTK_WIDGET (item);
}

static GtkWidget *
create_menu_item (GtkAction *action)
{
	GtkWidget *menuitem, *submenu;
	menuitem = (* GTK_ACTION_CLASS (parent_class)->create_menu_item) (action);
	submenu = create_recent_submenu (EGG_RECENT_ACTION (action));
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), submenu);
	return menuitem;
}

static void
connect_proxy (GtkAction *action, GtkWidget *proxy)
{
  EggRecentAction *recent_action;

  recent_action = EGG_RECENT_ACTION (action);

  /* do this before hand, so that we don't call the "activate" handler */
  if (GTK_IS_TOOL_ITEM (proxy))
    {
	  g_signal_connect (G_OBJECT (gtk_bin_get_child(GTK_BIN (proxy))), "toggled",
						G_CALLBACK (on_button_pressed), action);
    }
  (* GTK_ACTION_CLASS (parent_class)->connect_proxy) (action, proxy);
}

static void
disconnect_proxy (GtkAction *action, GtkWidget *proxy)
{
  EggRecentAction *recent_action;

  recent_action = EGG_RECENT_ACTION (action);

  /* do this before hand, so that we don't call the "activate" handler */
  if (GTK_IS_TOOL_ITEM (proxy))
    {
	  g_signal_handlers_disconnect_by_func (G_OBJECT (gtk_bin_get_child(GTK_BIN (proxy))),
						G_CALLBACK (on_button_pressed), action);
    }
  (* GTK_ACTION_CLASS (parent_class)->connect_proxy) (action, proxy);
}

/**
 * egg_recent_action_set_model:
 * @action: the action object
 * @model: a GtkTreeModel
 *
 * Sets a model in the action.
 */
void
egg_recent_action_add_model (EggRecentAction *action, EggRecentModel *model)
{
  GSList *slist;

  g_return_if_fail (EGG_IS_RECENT_ACTION (action));
  g_return_if_fail (EGG_IS_RECENT_MODEL (model));

  g_object_ref (model);
  action->priv->recent_models =
	g_list_append (action->priv->recent_models, model);

  for (slist = gtk_action_get_proxies (GTK_ACTION(action));
	   slist; slist = slist->next)
    {
      GtkWidget *proxy = slist->data;

      gtk_action_block_activate_from (GTK_ACTION (action), proxy);
	  
      if (GTK_IS_MENU_ITEM (proxy))
	  {
		  GtkWidget *submenu;
		  
		  submenu = gtk_menu_item_get_submenu (GTK_MENU_ITEM(proxy));
		  update_recent_submenu (action, submenu, model,
								 g_list_length (action->priv->recent_models)-1);
	  }
	  else if (GTK_IS_TOOL_ITEM (proxy))
	  {
		  GtkWidget *submenu;
		  
		  submenu = g_object_get_data (G_OBJECT (gtk_bin_get_child(GTK_BIN(proxy))), "submenu");
		  update_recent_submenu (action, submenu, model,
								 g_list_length (action->priv->recent_models)-1);
	  }
      else
	  {
			g_warning ("Don't know how to set popdown for `%s' widgets",
				   G_OBJECT_TYPE_NAME (proxy));
      }
      gtk_action_unblock_activate_from (GTK_ACTION (action), proxy);
    }
}

const gchar*
egg_recent_action_get_selected_uri (EggRecentAction *action)
{
	return action->priv->selected_uri;
}
