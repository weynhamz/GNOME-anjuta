/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * git-shell-test
 * Copyright (C) James Liggett 2009 <jrliggett@cox.net>
 * 
 * git-shell-test is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * git-shell-test is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "anjuta-dock.h"

/**
 * SECTION: AnjutaDock
 * @short_description: Docking system for context-driven user interfaces.
 * @see_also: #AnjutaCommandBar
 * @include; libanjuta/anjuta-dock.h
 *
 * AnjutaDock provides an alternative to the traditional menu and toolbar based
 * methodologies used by most GNOME programs. Instead, it focuses on tasks, or 
 * related sets of tasks. Each pane in the dock corresponds to a different set
 * of related tasks. 
 * 
 * Optionally, you can also associate an #AnjutaCommandBar with an AnjutaDock to
 * provide contextually appropriate sets of commands depending on the currently
 * visible pane
 */

struct _AnjutaDockPriv
{
	GHashTable *panes;
	GHashTable *dock_items;
	GtkWidget *command_bar;

	AnjutaDockPane* command_pane;
};

G_DEFINE_TYPE (AnjutaDock, anjuta_dock, GDL_TYPE_DOCK);

static void
anjuta_dock_init (AnjutaDock *self)
{
	self->priv = g_new0 (AnjutaDockPriv, 1);
	self->priv->panes = g_hash_table_new_full (g_str_hash, g_str_equal, 
	                                           NULL, g_object_unref);
	self->priv->dock_items = g_hash_table_new (g_str_hash, g_str_equal);
	self->priv->command_pane = 0;
}

static void
anjuta_dock_finalize (GObject *object)
{
	AnjutaDock *self;

	self = ANJUTA_DOCK (object);

	g_hash_table_destroy (self->priv->panes);
	g_hash_table_destroy (self->priv->dock_items);
	g_free (self->priv);

	G_OBJECT_CLASS (anjuta_dock_parent_class)->finalize (object);
}

static void
on_pane_selected (GdlDockItem *item, AnjutaCommandBar *command_bar)
{
	const gchar *pane_name;

	pane_name = (const gchar *) g_object_get_data (G_OBJECT (item), 
	                                               "pane-name");
	anjuta_command_bar_show_action_group (command_bar, pane_name);
}
	

static void
disconnect_select_signals (gpointer key, GdlDockItem *value, 
                           AnjutaCommandBar *command_bar)
{
	g_signal_handlers_disconnect_by_func (value, G_CALLBACK (on_pane_selected),
	                                      command_bar);
}

static void
anjuta_dock_dispose (GObject *object)
{
	AnjutaDock *self;

	self = ANJUTA_DOCK (object);

	if (self->priv->command_bar)
	{
		/* Disconnect pane selection signals before dropping the command bar 
		 * reference. It is possible that we may be holding the last reference
		 * of the bar, creating the possibility that the parent implementation
		 * of dispose might do something that would call the signal handler that
		 * would reference the already destroyed command bar. */
		g_hash_table_foreach (self->priv->dock_items, 
		                      (GHFunc) disconnect_select_signals,
		                      self->priv->command_bar);

		g_object_unref (self->priv->command_bar);
		self->priv->command_bar = NULL;
	}

	G_OBJECT_CLASS (anjuta_dock_parent_class)->dispose (object);
}

static void
anjuta_dock_class_init (AnjutaDockClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = anjuta_dock_finalize;
	object_class->dispose = anjuta_dock_dispose;
}

/**
 * anjuta_dock_new:
 * 
 * Creates a new AnjutaDock.
 */
GtkWidget *
anjuta_dock_new (void)
{
	return g_object_new (ANJUTA_TYPE_DOCK, NULL);
}

/**
 * anjuta_dock_add_pane:
 * @self: An AnjutaDock
 * @pane_name: A unique name for this pane
 * @pane_label: Label to display in this pane's grip 
 * @pane: The #AnjutaDockPane to add to the dock. The dock takes ownership of
 *		  the pane object.
 * @stock_id: Stock icon to display in this pane's grip
 * @placement: A #GdlDockPlacement value indicating where the pane should be
 *			   placed
 * @entries: #AnjutaCommandBar entries for this pane. Can be %NULL
 * @num_entries: The number of entries pointed to by entries, or 0.
 * @user_data: User data to pass to the entry callback
 *
 * Adds a pane, with optional #AnjutaCommandBar entries, to an AnjutaDock. This
 * method adds a pane with no grip that cannot be closed, floating or iconified.
 */
void 
anjuta_dock_add_pane (AnjutaDock *self, const gchar *pane_name,
                      const gchar *pane_label, const gchar *stock_icon,
                      AnjutaDockPane *pane, GdlDockPlacement placement, 
                      AnjutaCommandBarEntry *entries, int num_entries,
                      gpointer user_data)
{
	int behavior;

	behavior = 0;
	behavior |= GDL_DOCK_ITEM_BEH_NO_GRIP;
	behavior |= GDL_DOCK_ITEM_BEH_CANT_CLOSE;
	behavior |= GDL_DOCK_ITEM_BEH_CANT_ICONIFY;
	behavior |= GDL_DOCK_ITEM_BEH_NEVER_FLOATING;
	
	anjuta_dock_add_pane_full (self, pane_name, pane_label, stock_icon,
	                           pane, placement, entries, num_entries, 
	                           user_data, behavior);
}

/**
 * anjuta_dock_add_pane_full:
 * @self: An AnjutaDock
 * @pane_name: A unique name for this pane
 * @pane_label: Label to display in this pane's grip 
 * @stock_id: Stock icon to display in this pane's grip
 * @pane: The #AnjutaDockPane to add to the dock. The dock takes ownership of
 *		  the pane object.
 * @placement: A #GdlDockPlacement value indicating where the pane should be
 *			   placed
 * @entries: #AnjutaCommandBar entries for this pane. Can be %NULL
 * @num_entries: The number of entries pointed to by entries, or 0.
 * @user_data: User data to pass to the entry callback
 * @behavior: Any combination of #GdlDockItemBehavior flags
 *
 * Does the same thing as anjuta_dock_add_pane, but allows GDL dock behavior 
 * flags to be specified.
 */
void 
anjuta_dock_add_pane_full (AnjutaDock *self, const gchar *pane_name,
                           const gchar *pane_label, const gchar *stock_icon,   
                           AnjutaDockPane *pane,
                           GdlDockPlacement placement,
                           AnjutaCommandBarEntry *entries, int num_entries,
                           gpointer user_data,
                           GdlDockItemBehavior behavior)
{
	GtkWidget *dock_item;
	GtkWidget *child;

	dock_item = gdl_dock_item_new (pane_name, pane_label, behavior);
	child = anjuta_dock_pane_get_widget (pane);
	g_object_set_data (G_OBJECT (child), "dock-item", dock_item);

	/* Make sure there isn't another dock with the same name */
	if (!g_hash_table_lookup_extended (self->priv->panes, pane_name, NULL, 
	                                   NULL))
	{
		/* Take ownership of the pane object */
		g_hash_table_insert (self->priv->panes, (gchar *) pane_name, pane);

		gtk_container_add (GTK_CONTAINER (dock_item), child);
		gdl_dock_add_item (GDL_DOCK (self), GDL_DOCK_ITEM (dock_item), placement);

		g_object_set_data (G_OBJECT (dock_item), "pane-name", (gchar *) pane_name);

		/* Don't add anything to the action bar if there are no entries */
		if (self->priv->command_bar && entries)
		{
			anjuta_command_bar_add_action_group (ANJUTA_COMMAND_BAR (self->priv->command_bar),
			                                     pane_name, entries, num_entries, 
			                                     user_data);

			g_signal_connect (G_OBJECT (dock_item), "selected",
			                  G_CALLBACK (on_pane_selected),
			                  self->priv->command_bar);

			g_hash_table_insert (self->priv->dock_items, (gchar *) pane_name,
			                     dock_item);

			/* Show the new pane's commands in the command bar */
			anjuta_command_bar_show_action_group (ANJUTA_COMMAND_BAR (self->priv->command_bar),
			                                      pane_name);
		}
	}
}

/**
 * anjuta_dock_replace_command_pane:
 * @self: An AnjutaDock
 * @pane_name: A unique name for this pane
 * @pane_label: Label to display in this pane's grip 
 * @pane: The #AnjutaDockPane to add to the dock. The dock takes ownership of
 *		  the pane object.
 * @stock_id: Stock icon to display in this pane's grip
 * @placement: A #GdlDockPlacement value indicating where the pane should be
 *			   placed
 * @entries: #AnjutaCommandBar entries for this pane. Can be %NULL
 * @num_entries: The number of entries pointed to by entries, or 0.
 * @user_data: User data to pass to the entry callback
 *
 * Adds a pane, with optional #AnjutaCommandBar entries, to an AnjutaDock. This
 * method adds a pane with no grip that cannot be closed, floating or iconified.
 * If there was an old command pane, that pane is removed in favour of the new pane.
 */
void 
anjuta_dock_replace_command_pane (AnjutaDock *self,
                                  const gchar *pane_name,
                                  const gchar *pane_label, const gchar *stock_icon,
                                  AnjutaDockPane *pane, GdlDockPlacement placement, 
                                  AnjutaCommandBarEntry *entries, int num_entries,
                                  gpointer user_data)
{
	if (self->priv->command_pane)
	{
		anjuta_dock_remove_pane (self, self->priv->command_pane);
	}

	anjuta_dock_add_pane (self, pane_name, pane_label, stock_icon,
	                      pane, placement, entries, num_entries, user_data);
	
	self->priv->command_pane = pane;
}

/**
 * anjuta_dock_remove_pane:
 * @self An AnjutaDock
 * @pane_name: Name of the pane to remove
 *
 * Removes a pane from a dock
 */
void
anjuta_dock_remove_pane (AnjutaDock *self, AnjutaDockPane *pane)
{
	GtkWidget *child;
	GtkContainer *dock_item;

	child = anjuta_dock_pane_get_widget (pane);

	if (self->priv->command_pane == pane)
	{
		self->priv->command_pane = NULL;
	}
	
	if (child)
	{
		/* Remove the child from its dock item and destroy it */
		dock_item = g_object_get_data (G_OBJECT (child), "dock-item");
		g_hash_table_remove (self->priv->panes, 
		                     g_object_get_data (G_OBJECT (dock_item), 
		                                        "pane-name"));
		gtk_container_remove (dock_item, child);

		gdl_dock_item_unbind (GDL_DOCK_ITEM (dock_item));
	}
}

/**
 * anjuta_dock_show_pane:
 * @self: An AnjutaDock
 * @pane_name: Name of the pane to show
 *
 * Makes the given pane visible
 */
void
anjuta_dock_show_pane (AnjutaDock *self, AnjutaDockPane *pane)
{
	GtkWidget *child;
	GdlDockItem *dock_item;

	child = anjuta_dock_pane_get_widget (pane);

	if (child)
	{
		dock_item = g_object_get_data (G_OBJECT (child), "dock-item");
		gdl_dock_item_show_item (dock_item);
	}
}

/**
 * anjuta_dock_hide_pane:
 * @self: An AnjutaDock
 * @pane_name: Name of the pane to hide
 *
 * Makes the given pane invisible
 */
void
anjuta_dock_hide_pane (AnjutaDock *self, AnjutaDockPane *pane)
{
	GtkWidget *child;
	GdlDockItem *dock_item;

	child = anjuta_dock_pane_get_widget (pane);

	if (child)
	{
		dock_item = g_object_get_data (G_OBJECT (child), "dock-item");
		gdl_dock_item_hide_item (dock_item);
	}
}

/**
 * anjuta_dock_set_command_bar:
 * @self: An AnjutaDock
 * @command_bar: An #AnjutaCommandBar to associate with this dock
 *
 * Associates an #AnjutaCommandBar with this dock. Command bars can be used to 
 * provide different sets of commands based on the currently visible pane.
 */
void
anjuta_dock_set_command_bar (AnjutaDock *self, AnjutaCommandBar *command_bar)
{
	if (self->priv->command_bar)
		g_object_unref (self->priv->command_bar);

	self->priv->command_bar = g_object_ref (command_bar);
}

/**
 * anjuta_dock_get_command_bar:
 * @self: An AnjutaDock
 *
 * Returns: the #AnjutaCommandBar associated with this dock or %NULL.
 */
AnjutaCommandBar *
anjuta_dock_get_command_bar (AnjutaDock *self)
{
	return ANJUTA_COMMAND_BAR (self->priv->command_bar);
}