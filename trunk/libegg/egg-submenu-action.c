/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* egg-submenu-action widget
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
#include "egg-submenu-action.h"

#ifndef _
# define _(s) (s)
#endif

struct _EggSubmenuActionPriv
{
	EggSubmenuFactory factory;
	gpointer user_data;
};

static void egg_submenu_action_init       (EggSubmenuAction *action);
static void egg_submenu_action_class_init (EggSubmenuActionClass *kclass);

static GtkWidget *create_tool_item         (GtkAction *action);
static GtkWidget *create_menu_item         (GtkAction *action);
static void connect_proxy                  (GtkAction *action,
										    GtkWidget *proxy);
static void disconnect_proxy               (GtkAction *action,
										    GtkWidget *proxy);
static void egg_submenu_action_finalize    (GObject *object);

static GObjectClass *parent_class = NULL;

GType
egg_submenu_action_get_type (void)
{
	static GtkType type = 0;
	
	if (!type)
    {
		static const GTypeInfo type_info =
		{
			sizeof (EggSubmenuActionClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) egg_submenu_action_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			
			sizeof (EggSubmenuAction),
        0, /* n_preallocs */
		(GInstanceInitFunc) egg_submenu_action_init,
      };

      type = g_type_register_static (GTK_TYPE_ACTION,
                                     "EggSubmenuAction",
                                     &type_info, 0);
  }
  return type;
}

static void
egg_submenu_action_class_init (EggSubmenuActionClass *kclass)
{
	GtkActionClass *action_class;
	GObjectClass   *object_class;
	
	parent_class = g_type_class_peek_parent (kclass);
	action_class = GTK_ACTION_CLASS (kclass);
	object_class = G_OBJECT_CLASS (kclass);
	
	object_class->finalize = egg_submenu_action_finalize;
	
	action_class->connect_proxy = connect_proxy;
	action_class->disconnect_proxy = disconnect_proxy;
	action_class->menu_item_type = GTK_TYPE_MENU_ITEM;
	action_class->toolbar_item_type = GTK_TYPE_TOOL_ITEM;
	action_class->create_tool_item = create_tool_item;
	action_class->create_menu_item = create_menu_item;
}

static void
egg_submenu_action_init (EggSubmenuAction *action)
{
	action->priv = g_new0 (EggSubmenuActionPriv, 1);
	action->priv->factory = NULL;
	action->priv->user_data = NULL;
}

static void
egg_submenu_action_finalize (GObject *object)
{
	g_free (EGG_SUBMENU_ACTION (object)->priv);
	if (parent_class->finalize)
		parent_class->finalize (object);
}

static GtkWidget*
create_submenu_submenu (EggSubmenuAction *action)
{
	GtkWidget *submenu;
	
	if (action->priv->factory)
	{
		submenu = (*action->priv->factory)(action->priv->user_data);
	}
	else
	{
		submenu = gtk_menu_new ();
		gtk_widget_show (submenu);
	}
	gtk_widget_show (submenu);
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
on_button_pressed (GtkToggleToolButton *button, EggSubmenuAction *action)
{
	GtkWidget *submenu;

	/* g_message ("Submenu toolitem toggled"); */
	submenu = g_object_get_data (G_OBJECT (button), "submenu");
	if (!submenu)
	{
		submenu = create_submenu_submenu (action);
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
	
	g_return_val_if_fail (EGG_IS_SUBMENU_ACTION (action), NULL);
	/* g_message ("Creating submenu toolitem"); */
	
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
	submenu = create_submenu_submenu (EGG_SUBMENU_ACTION (action));
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), submenu);
	return menuitem;
}

static void
connect_proxy (GtkAction *action, GtkWidget *proxy)
{
	EggSubmenuAction *submenu_action;
	
	submenu_action = EGG_SUBMENU_ACTION (action);
	
	/* do this before hand, so that we don't call the "activate" handler */
	if (GTK_IS_TOOL_ITEM (proxy))
    {
		g_signal_connect (G_OBJECT (gtk_bin_get_child(GTK_BIN (proxy))),
						  "toggled",
						  G_CALLBACK (on_button_pressed), action);
    }
	(* GTK_ACTION_CLASS (parent_class)->connect_proxy) (action, proxy);
}

static void
disconnect_proxy (GtkAction *action, GtkWidget *proxy)
{
	EggSubmenuAction *submenu_action;
	
	submenu_action = EGG_SUBMENU_ACTION (action);
	
	/* do this before hand, so that we don't call the "activate" handler */
	if (GTK_IS_TOOL_ITEM (proxy))
    {
		g_signal_handlers_disconnect_by_func (G_OBJECT (gtk_bin_get_child(GTK_BIN (proxy))),
											  G_CALLBACK (on_button_pressed),
											  action);
    }
	(* GTK_ACTION_CLASS (parent_class)->connect_proxy) (action, proxy);
}

void
egg_submenu_action_set_menu_factory (EggSubmenuAction *action,
									 EggSubmenuFactory factory,
									 gpointer user_data)
{
	g_return_if_fail (EGG_IS_SUBMENU_ACTION (action));
	
	action->priv->factory = factory;
	action->priv->user_data = user_data;
}
