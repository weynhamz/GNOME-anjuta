/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Anjuta
 * Copyright 2000 Dave Camp, Naba Kumar  <naba@gnome.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.  
 */

/**
 * SECTION:anjuta-plugin
 * @short_description: Anjuta plugin base class from which all plugins are
 * derived.
 * @see_also: #AnjutaPluginManager, #AnjutaProfileManager
 * @stability: Unstable
 * @include: libanjuta/anjuta-plugin.h
 * 
 * Anjuta plugins are components which are loaded in Anjuta IDE shell
 * either on startup or on demand to perform various subtasks. Plugins are
 * specialized in doing only a very specific task and can let other plugins
 * interract with it using interfaces.
 * 
 * A plugin class is derived from #AnjutaPlugin class and will be used by
 * shell to instanciate any number of plugin objects.
 * 
 * When a plugin class is derived from #AnjutaPlugin, the virtual mehtods 
 * <emphasis>activate</emphasis> and <emphasis>deactivate</emphasis> must 
 * be implemented. The <emphasis>activate</emphasis> method is used to
 * activate the plugin. Note that plugin activation is different from plugin
 * instance initialization. Instance initialization is use to do internal
 * initialization, while <emphasis>activate</emphasis> method is used to
 * setup the plugin in shell, UI and preferences. Other plugins can also
 * be queried in <emphasis>activate</emphasis> method.
 * 
 * Following things should be done in <emphasis>activate</emphasis> method.
 * <orderedlist>
 * 	<listitem>
 * 		<para>
 * 			Register UI Actions: Use anjuta_ui_add_action_group_entries() or 
 * 			anjuta_ui_add_toggle_action_group_entries() to add your action
 * 			groups.
 * 		</para>
 * 	</listitem>
 * 	<listitem>
 * 		<para>
 * 			Merge UI definition file: Use anjuta_ui_merge() to merge a UI
 * 			file. See #AnjutaUI for more detail.
 * 		</para>
 * 	</listitem>
 * 	<listitem>
 * 		<para>
 * 			Add widgets to Anjuta Shell: If the plugin has one or more
 * 			widgets as its User Interface, they can be added with
 * 			anjuta_shell_add_widget().
 * 		</para>
 * 	</listitem>
 * 	<listitem>
 * 		<para>
 * 			Setup value watches with anjuta_plugin_add_watch().
 * 		</para>
 * 	</listitem>
 * </orderedlist>
 * 
 * <emphasis>deactivate</emphasis> method undos all the above. That is, it
 * removes widgets from the shell, unmerges UI and removes the action groups.
 * 
 * Plugins interact with each other using interfaces. A plugin can expose an
 * interface which will let other plugins find it. Any number of interfaces can
 * be exposed by a plugin. These exposed interfaces are called
 * <emphasis>Primary</emphasis> interfaces of the plugin. The condition for
 * the interfaces to be primary is that they should be independent (i.e.
 * an external entity requesting to use a primary interface should not require
 * other primary interfaces). For example, an editor plugin can implement
 * #IAnjutaEditor, #IAnjutaStream and #IAnjutaFile interfaces and expose them
 * as primary interfaces, because they are independent.
 * <emphasis>Primary</emphasis> interfaces exposed by a plugin are exported in
 * its plugin meta-data file, so that plugin manager could register them.
 * 
 * Any other interfaces implemented by the plugin are called
 * <emphasis>Secondary</emphasis> interfaces and they generally depend on
 * one or more primary interfaces.
 * For example, #IAnjutaEditor is the primary interface of anjuta-editor plugin,
 * but it also implements secondary interfaces #IAnjutaEditorGutter and
 * #IAnjutaEditorBuffer. Notice that secondary interfaces #IAnjutaEditorGutter
 * and #IAnjutaEditorBuffer depend on #IAnjutaEditor interface.
 * 
 * The purpose of distinguishing between primary and
 * secondary interfaces is only at plugin level. At code level, they behave
 * just the same and there is no difference.
 * So, a natural sequence for a plugin to communicate with another plugin is:
 * <orderedlist>
 * 	<listitem>
 * 		<para>
 * 			Query the shell for a plugin implemeting the primary interface
 * 			using anjuta_shell_get_interface(). It will return an
 * 			implemetation of the interface (or NULL if not found).
 * 			Do not save this object for longer use, because the implementor
 * 			plugin can change anytime and a different plugin implementing
 * 			the same primary interface may be activated.
 * 				<programlisting>
 * GError *err = NULL;
 * IAnjutaDocumentManager *docman;
 * IAnjutaEditor *editor;
 * 
 * docman = anjuta_shell_get_interface (ANJUTA_PLUGIN(plugin)->shell,
 *                                      IAnjutaDocumentManager, &amp;err);
 * if (err)
 * {
 * 	g_warning ("Error encountered: %s", err->message);
 * 	g_error_free (err);
 * 	return;
 * }
 * 
 * editor = ianjuta_document_manager_get_current_editor (docman, &amp;err);
 * if (err)
 * {
 * 	g_warning ("Error encountered: %s", err->message);
 * 	g_error_free (err);
 * 	return;
 * }
 * 
 * ianjuta_editor_goto_line (editor, 200);
 * ...
 * 			</programlisting>
 * 		</para>
 * </listitem>
 * <listitem>
 * 	<para>
 * 	A primary interface of a plugin can be directly used, but
 * 	to use a secondary interface, make sure to check if the plugin
 * 	object implements it. For example, to check if editor plugin
 * 	implements IAnjutaEditorGutter interface, do something like:
 * 		<programlisting>
 * 
 * if (IANJUTA_IS_EDITOR_GUTTER(editor))
 * {
 * 	ianjuta_editor_gutter_set_marker(IANJUTA_EDITOR_GUTTER (editor),
 * 	                                 ANJUTA_EDITOR_MARKER_1,
 * 	                                 line_number, &amp;err);
 * }
 * 			</programlisting>
 * 		</para>
 * 	</listitem>
 * </orderedlist>
 * 
 * Plugins can also communicate with outside using Shell's <emphasis>Values
 * System</emphasis>. Values are objects exported by plugins to make them
 * available to other plugins. Read #AnjutaShell documentation for more
 * detail on <emphasis>Values System</emphasis>. A plugin can set up watches
 * with anjuta_plugin_add_watch() to get notifications for values exported
 * by other plugins.
 * 
 * Values are very unreliable way of passing objects between plugins, but are
 * nevertheless very useful (and quicker to code). It must be used with care.
 * As a rule of thumb, a plugin should only watch values of other trusted
 * plugins. For example, a group of plugins forming a subsystem can comfortably
 * use values to pass objects and notifications. Use anjuta_plugin_add_watch()
 * and anjuta_plugin_remove_watch() to add or remove value watches.
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

G_DEFINE_TYPE (AnjutaPlugin, anjuta_plugin, G_TYPE_OBJECT);

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
	GObjectClass *parent_class = g_type_class_peek_parent (class);
    
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
				      G_PARAM_READWRITE/* | G_PARAM_CONSTRUCT_ONLY*/)); /* Construct only does not work with libanjutamm */
	plugin_signals[ACTIVATED_SIGNAL] =
		g_signal_new ("activated",
					G_TYPE_FROM_CLASS (object_class),
					G_SIGNAL_RUN_FIRST,
					G_STRUCT_OFFSET (AnjutaPluginClass,
									 activated),
					NULL, NULL,
					anjuta_cclosure_marshal_VOID__VOID,
					G_TYPE_NONE, 0);
	
	plugin_signals[ACTIVATED_SIGNAL] =
		g_signal_new ("deactivated",
					G_TYPE_FROM_CLASS (object_class),
					G_SIGNAL_RUN_FIRST,
					G_STRUCT_OFFSET (AnjutaPluginClass,
									 deactivated),
					NULL, NULL,
					anjuta_cclosure_marshal_VOID__VOID,
					G_TYPE_NONE, 0);
}

static void
anjuta_plugin_init (AnjutaPlugin *plugin)
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
		g_signal_emit_by_name (G_OBJECT (plugin), "activated");

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
