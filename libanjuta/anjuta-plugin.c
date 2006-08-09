/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/* Anjuta
 * Copyright 2000 Dave Camp
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.  
 */

#include <config.h>
#include <string.h>
#include <libanjuta/anjuta-marshal.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include "anjuta-plugin.h"

typedef struct 
{
	guint id;
	char *name;
	AnjutaPluginValueAdded added;
	AnjutaPluginValueRemoved removed;
	gboolean need_remove;
	gpointer user_data;
} Watch;

struct _AnjutaPluginPrivate {
	guint watch_num;

	int added_signal_id;
	int removed_signal_id;

	GList *watches;
	
	gboolean activated;
};

enum {
	PROP_0,
	PROP_SHELL,
};

enum
{
	ACTIVATED_SIGNAL,
	DEACTIVATED_SIGNAL,
	LAST_SIGNAL
};

static guint plugin_signals[LAST_SIGNAL] = { 0 };

static void anjuta_plugin_finalize (GObject *object);
static void anjuta_plugin_class_init (AnjutaPluginClass *class);

GNOME_CLASS_BOILERPLATE (AnjutaPlugin, anjuta_plugin, GObject, G_TYPE_OBJECT);

static void
destroy_watch (Watch *watch)
{
	g_free (watch->name);
	g_free (watch);
}

static void
anjuta_plugin_dispose (GObject *object)
{
	AnjutaPlugin *plugin = ANJUTA_PLUGIN (object);
	
	if (plugin->priv->watches) {
		GList *l;

		for (l = plugin->priv->watches; l != NULL; l = l->next) {
			Watch *watch = (Watch *)l->data;

			if (watch->removed && watch->need_remove) {
				watch->removed (plugin, 
						watch->name, 
						watch->user_data);
			}
			
			destroy_watch (watch);
		}
		g_list_free (plugin->priv->watches);
		plugin->priv->watches = NULL;
	}

	if (plugin->shell) {
		if (plugin->priv->added_signal_id) {
			g_signal_handler_disconnect (G_OBJECT (plugin->shell),
										 plugin->priv->added_signal_id);
			g_signal_handler_disconnect (G_OBJECT (plugin->shell),
										 plugin->priv->removed_signal_id);
			plugin->priv->added_signal_id = 0;
			plugin->priv->removed_signal_id = 0;
		}
		g_object_unref (plugin->shell);
		plugin->shell = NULL;
	}
}

static void
anjuta_plugin_finalize (GObject *object) 
{
	AnjutaPlugin *plugin = ANJUTA_PLUGIN (object);
	g_free (plugin->priv);
}

static void
anjuta_plugin_get_property (GObject *object,
			  guint param_id,
			  GValue *value,
			  GParamSpec *pspec)
{
	AnjutaPlugin *plugin = ANJUTA_PLUGIN (object);
	
	switch (param_id) {
	case PROP_SHELL:
		g_value_set_object (value, plugin->shell);
		break;
	default :
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
anjuta_plugin_set_property (GObject *object,
							guint param_id,
							const GValue *value,
							GParamSpec *pspec)
{
	AnjutaPlugin *plugin = ANJUTA_PLUGIN (object);
	
	switch (param_id) {
	case PROP_SHELL:
		g_return_if_fail (plugin->shell == NULL);
		plugin->shell = g_value_get_object (value);
		g_object_ref (plugin->shell);
		g_object_notify (object, "shell");
		break;
	default :
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}
        
static void
anjuta_plugin_class_init (AnjutaPluginClass *class) 
{
	GObjectClass *object_class = (GObjectClass*) class;
	parent_class = g_type_class_peek_parent (class);
    
	object_class->dispose = anjuta_plugin_dispose;
	object_class->finalize = anjuta_plugin_finalize;
	object_class->get_property = anjuta_plugin_get_property;
	object_class->set_property = anjuta_plugin_set_property;

	class->activate = NULL;
	class->deactivate = NULL;

	g_object_class_install_property
		(object_class,
		 PROP_SHELL,
		 g_param_spec_object ("shell",
				      _("Anjuta Shell"),
				      _("Anjuta shell that will contain the plugin"),
				      ANJUTA_TYPE_SHELL,
				      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	plugin_signals[ACTIVATED_SIGNAL] =
		g_signal_new ("activated",
					G_TYPE_FROM_CLASS (object_class),
					G_SIGNAL_RUN_FIRST,
					G_STRUCT_OFFSET (AnjutaPluginClass,
									 activated_signal),
					NULL, NULL,
					anjuta_cclosure_marshal_VOID__VOID,
					G_TYPE_NONE, 0);
	
	plugin_signals[ACTIVATED_SIGNAL] =
		g_signal_new ("deactivated",
					G_TYPE_FROM_CLASS (object_class),
					G_SIGNAL_RUN_FIRST,
					G_STRUCT_OFFSET (AnjutaPluginClass,
									 deactivated_signal),
					NULL, NULL,
					anjuta_cclosure_marshal_VOID__VOID,
					G_TYPE_NONE, 0);
}

static void
anjuta_plugin_instance_init (AnjutaPlugin *plugin)
{
	plugin->priv = g_new0 (AnjutaPluginPrivate, 1);
}

static void
value_added_cb (AnjutaShell *shell,
		const char *name,
		const GValue *value,
		gpointer user_data)
{
	AnjutaPlugin *plugin = ANJUTA_PLUGIN (user_data);
	GList *l;
	
	for (l = plugin->priv->watches; l != NULL; l = l->next) {
		Watch *watch = (Watch *)l->data;
		if (!strcmp (watch->name, name)) {
			if (watch->added) {
				watch->added (plugin, 
					      name, 
					      value, 
					      watch->user_data);
			}
			
			watch->need_remove = TRUE;
		}
	}
}

static void
value_removed_cb (AnjutaShell *shell,
		  const char *name,
		  gpointer user_data)
{
	AnjutaPlugin *plugin = ANJUTA_PLUGIN (user_data);
	GList *l;

	for (l = plugin->priv->watches; l != NULL; l = l->next) {
		Watch *watch = (Watch *)l->data;
		if (!strcmp (watch->name, name)) {
			if (watch->removed) {
				watch->removed (plugin, name, watch->user_data);
			}
			if (!watch->need_remove) {
				g_warning ("watch->need_remove FALSE when it should be TRUE");
			}
			
			watch->need_remove = FALSE;
		}
	}
}

/**
 * anjuta_plugin_add_watch:
 * @plugin: a #AnjutaPlugin derived class object.
 * @name: Name of the value to watch.
 * @added: Callback to call when the value is added.
 * @removed: Callback to call when the value is removed.
 * @user_data: User data to pass to callbacks.
 * 
 * Adds a watch for @name value. When the value is added in shell, the @added
 * callback will be called and when it is removed, the @removed callback will
 * be called. The returned ID is used to remove the watch later.
 * 
 * Return value: Watch ID.
 */
guint
anjuta_plugin_add_watch (AnjutaPlugin *plugin, 
						 const gchar *name,
						 AnjutaPluginValueAdded added,
						 AnjutaPluginValueRemoved removed,
						 gpointer user_data)
{
	Watch *watch;
	GValue value = {0, };
	GError *error = NULL;

	g_return_val_if_fail (plugin != NULL, -1);
	g_return_val_if_fail (ANJUTA_IS_PLUGIN (plugin), -1);
	g_return_val_if_fail (name != NULL, -1);

	watch = g_new0 (Watch, 1);
	
	watch->id = ++plugin->priv->watch_num;
	watch->name = g_strdup (name);
	watch->added = added;
	watch->removed = removed;
	watch->need_remove = FALSE;
	watch->user_data = user_data;

	plugin->priv->watches = g_list_prepend (plugin->priv->watches, watch);

	anjuta_shell_get_value (plugin->shell, name, &value, &error);
	if (!error) {
		if (added) {
			watch->added (plugin, name, &value, user_data);
			g_value_unset (&value);
		}
		watch->need_remove = TRUE;
	} else {
		/* g_warning ("Error in getting value '%s': %s", name, error->message); */
		g_error_free (error);
	}

	if (!plugin->priv->added_signal_id) {
		plugin->priv->added_signal_id = 
			g_signal_connect (plugin->shell,
					  "value_added",
					  G_CALLBACK (value_added_cb),
					  plugin);

		plugin->priv->removed_signal_id = 
			g_signal_connect (plugin->shell,
					  "value_removed",
					  G_CALLBACK (value_removed_cb),
					  plugin);
	}

	return watch->id;
}

/**
 * anjuta_plugin_remove_watch:
 * @plugin: a #AnjutaPlugin derived class object.
 * @id: Watch id to remove.
 * @send_remove: If true, calls value_removed callback.
 *
 * Removes the watch represented by @id (which was returned by
 * anjuta_plugin_add_watch()).
 */
void
anjuta_plugin_remove_watch (AnjutaPlugin *plugin, guint id,
							gboolean send_remove)
{
	GList *l;
	Watch *watch = NULL;
	
	g_return_if_fail (plugin != NULL);
	g_return_if_fail (ANJUTA_IS_PLUGIN (plugin));

	for (l = plugin->priv->watches; l != NULL; l = l->next) {
		watch = l->data;
		
		if (watch->id == id) {
			break;
		}
	}

	if (!watch) {
		g_warning ("Attempted to remove non-existant watch %d\n", id);
		return;
	}

	if (send_remove && watch->need_remove && watch->removed) {
		watch->removed (plugin, watch->name, watch->user_data);
	}
	
	plugin->priv->watches = g_list_remove (plugin->priv->watches, watch);
	destroy_watch (watch);
}

/**
 * anjuta_plugin_activate:
 * @plugin: a #AnjutaPlugin derived class object.
 *
 * Activates the plugin by calling activate() virtual method. All plugins
 * should derive their classes from this virtual class and implement this
 * method.
 * If the plugin implements IAnjutaPreferences, it is prompted to install
 * it's preferences.
 *
 * Return value: TRUE if sucessfully activated, FALSE otherwise.
 */
gboolean
anjuta_plugin_activate (AnjutaPlugin *plugin)
{
	AnjutaPluginClass *klass;
	
	g_return_val_if_fail (plugin != NULL, FALSE);
	g_return_val_if_fail (ANJUTA_IS_PLUGIN (plugin), FALSE);
	g_return_val_if_fail (plugin->priv->activated == FALSE, FALSE);
	
	klass = ANJUTA_PLUGIN_GET_CLASS(plugin);
	g_return_val_if_fail (klass->activate != NULL, FALSE);
	
	plugin->priv->activated = klass->activate(plugin);
	
	if (plugin->priv->activated)
	{
		if (IANJUTA_IS_PREFERENCES(plugin))
			ianjuta_preferences_merge(IANJUTA_PREFERENCES(plugin),
				ANJUTA_PREFERENCES(anjuta_shell_get_preferences(plugin->shell,
																NULL)), NULL);
		g_signal_emit_by_name (G_OBJECT (plugin), "activated");
	}
	return plugin->priv->activated;
}

/**
 * anjuta_plugin_deactivate:
 * @plugin: a #AnjutaPlugin derived class object.
 *
 * Deactivates the plugin by calling deactivate() virtual method. All plugins
 * should derive their classes from this virtual class and implement this
 * method.
 *
 * Return value: TRUE if sucessfully activated, FALSE otherwise.
 */
gboolean
anjuta_plugin_deactivate (AnjutaPlugin *plugin)
{
	AnjutaPluginClass *klass;
	gboolean success;
	
	g_return_val_if_fail (plugin != NULL, FALSE);
	g_return_val_if_fail (ANJUTA_IS_PLUGIN (plugin), FALSE);
	g_return_val_if_fail (plugin->priv->activated == TRUE, FALSE);
	
	klass = ANJUTA_PLUGIN_GET_CLASS(plugin);
	g_return_val_if_fail (klass->deactivate != NULL, FALSE);
	
	if (IANJUTA_IS_PREFERENCES(plugin))
		ianjuta_preferences_unmerge(IANJUTA_PREFERENCES(plugin),
			ANJUTA_PREFERENCES(anjuta_shell_get_preferences(plugin->shell, NULL)), NULL);
	
	success = klass->deactivate(plugin);
	plugin->priv->activated = !success;
	if (!plugin->priv->activated)
		g_signal_emit_by_name (G_OBJECT (plugin), "deactivated");
	return success;
}

/**
 * anjuta_plugin_is_active:
 * @plugin: a #AnjutaPlugin derived class object.
 *
 * Returns TRUE if the plugin has been activated.
 *
 * Return value: TRUE if activated, FALSE otherwise.
 */
gboolean
anjuta_plugin_is_active (AnjutaPlugin *plugin)
{
	return plugin->priv->activated;
}
