/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2010 <jrliggett@cox.net>
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

#include "anjuta-dock-pane.h"

/**
 * SECTION: anjuta-dock-pane
 * @short_description: Wrapper class for #AnjutaDock panes
 * @see_also: #AnjutaDock
 * @include: libanjuta/anjuta-dock-pane.h
 *
 * AnjutaDockPane is an abstract wrapper class for panes in an 
 * #AnjutaDock.
 *
 * Using AnjutaDockPane is especially helpful for those panes that show data
 * from extenal sources that must be refreshed frequently, or panes that are
 * exceptionally complex. 
 */

enum
{
	ANJUTA_DOCK_PANE_PLUGIN = 1
};

enum
{
	SINGLE_SELECTION_CHANGED,
	MULTIPLE_SELECTION_CHANGED,

	LAST_SIGNAL
};

static guint anjuta_dock_pane_signals[LAST_SIGNAL] = { 0 };

struct _AnjutaDockPanePriv
{
	AnjutaPlugin *plugin;
};

G_DEFINE_ABSTRACT_TYPE (AnjutaDockPane, anjuta_dock_pane, G_TYPE_OBJECT);

static void
anjuta_dock_pane_init (AnjutaDockPane *self)
{
	self->priv = g_new0 (AnjutaDockPanePriv, 1);
}

static void
anjuta_dock_pane_finalize (GObject *object)
{
	AnjutaDockPane *self;

	self = ANJUTA_DOCK_PANE (object);

	g_free (self->priv);

	G_OBJECT_CLASS (anjuta_dock_pane_parent_class)->finalize (object);
}

static void
anjuta_dock_pane_set_property (GObject *object, guint property_id, 
                               const GValue *value, GParamSpec *param_spec)
{
	AnjutaDockPane *self;

	self = ANJUTA_DOCK_PANE (object);

	switch (property_id)
	{
		case ANJUTA_DOCK_PANE_PLUGIN:
			self->priv->plugin = ANJUTA_PLUGIN (g_value_get_object (value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, param_spec);
			break;
	}
}

static void
anjuta_dock_pane_get_property (GObject *object, guint property_id,
                               GValue *value, GParamSpec *param_spec)
{
	AnjutaDockPane *self;

	self = ANJUTA_DOCK_PANE (object);

	switch (property_id)
	{
		case ANJUTA_DOCK_PANE_PLUGIN:
			g_value_set_object (value, self->priv->plugin);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, param_spec);
			break;
	}
}

static void
anjuta_dock_pane_single_selection_changed (AnjutaDockPane *pane)
{
}

static void
anjuta_dock_pane_multiple_selection_changed (AnjutaDockPane *pane)
{
}

static void
anjuta_dock_pane_class_init (AnjutaDockPaneClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GParamSpec *param_spec;

	object_class->finalize = anjuta_dock_pane_finalize;
	object_class->set_property = anjuta_dock_pane_set_property;
	object_class->get_property = anjuta_dock_pane_get_property;
	klass->refresh = NULL;
	klass->get_widget = NULL;
	klass->single_selection_changed = anjuta_dock_pane_single_selection_changed;
	klass->multiple_selection_changed = anjuta_dock_pane_multiple_selection_changed;

	param_spec = g_param_spec_object ("plugin", "Plugin", 
	                                  "Plugin object associated with this pane.",
	                                  ANJUTA_TYPE_PLUGIN,
	                                  G_PARAM_READWRITE);
	g_object_class_install_property (object_class, ANJUTA_DOCK_PANE_PLUGIN, 
	                                 param_spec);

	/** 
	 * AnjutaDockPane::single-selection-changed
	 * @pane: An AnjutaDockPane
	 *
	 * This signal is emitted by pane subclasses to notify clients that
	 * the user has selected an item. This signal should be used when users are
	 * expected to only select one item at a time.
	 */
	anjuta_dock_pane_signals[SINGLE_SELECTION_CHANGED] = 
		g_signal_new ("single-selection-changed",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (AnjutaDockPaneClass, single_selection_changed),
		              NULL, 
		              NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE,
		              0);

	/**
	 * AnjutaDockPane::multiple-selection-changed
	 * @pane: An AnjutaDockPane
	 *
	 * This signal is emitted by pane subclasses to notify clients that the set
	 * of selected items in the pane has changed. 
	 */
	anjuta_dock_pane_signals[MULTIPLE_SELECTION_CHANGED] =
		g_signal_new ("multiple-selection-changed",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (AnjutaDockPaneClass, multiple_selection_changed),
		              NULL, 
		              NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE,
		              0);
	                                                                   
}

/**
 * anjuta_dock_pane_refresh:
 * @self: An AnjutaDockPane
 *
 * Refreshes the given pane. Subclasses only need to override this method if 
 * needed. 
 */
void
anjuta_dock_pane_refresh (AnjutaDockPane *self)
{
	AnjutaDockPaneClass *klass;

	klass = ANJUTA_DOCK_PANE_GET_CLASS (self);
	
	if (klass->refresh)
		klass->refresh (self);
}

/**
 * anjuta_dock_pane_get_widget:
 * @self: An AnjutaDockPane
 *
 * Returns the widget associated with the given pane. The returned widget is 
 * owned by the pane and should not be destroyed or modified.
 */
GtkWidget *
anjuta_dock_pane_get_widget (AnjutaDockPane *self)
{
	return ANJUTA_DOCK_PANE_GET_CLASS (self)->get_widget (self);
}


/**
 * anjuta_dock_pane_get_plugin:
 * @self: An AnjutaDockPane
 *
 * Returns the plugin object associated with this pane. 
 */
AnjutaPlugin *
anjuta_dock_pane_get_plugin (AnjutaDockPane *self)
{
	return self->priv->plugin;
}

/**
 * anjuta_dock_pane_notify_single_selection_changed:
 * @self: An AnjutaDockPane
 *
 * Emits the single-selection-changed signal.
 */
void
anjuta_dock_pane_notify_single_selection_changed (AnjutaDockPane *self)
{
	g_signal_emit_by_name (self, "single-selection-changed", NULL);
}

/**
 * anjuta_dock_pane_notify_multiple_selection_changed:
 * @pane: An AnjutaDockPane 
 *
 * Emits the multiple-selection-changed signal.
 */
void
anjuta_dock_pane_notify_multiple_selection_changed (AnjutaDockPane *self)
{
	g_signal_emit_by_name (self, "multiple-selection-changed", NULL);
}