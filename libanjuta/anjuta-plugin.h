/* Anjuta
 * Copyright (C) 2000 Dave Camp
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

#ifndef ANJUTA_PLUGIN_H
#define ANJUTA_PLUGIN_H

#include <glib.h>
#include <glib-object.h>

#include <string.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-ui.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/glue-plugin.h>

G_BEGIN_DECLS

typedef struct _AnjutaPlugin        AnjutaPlugin;
typedef struct _AnjutaPluginClass   AnjutaPluginClass;
typedef struct _AnjutaPluginPrivate AnjutaPluginPrivate;

/**
 * AnjutaPluginValueAdded:
 * @plugin: The #AnjutaPlugin based plugin
 * @name: name of value being added.
 * @value: value of value being added.
 * @user_data: User data set during anjuta_plugin_add_watch()
 *
 * The callback to pass to anjuta_plugin_add_watch(). When a @name value
 * is added to shell by another plugin, this callback will be called.
 */
typedef void (*AnjutaPluginValueAdded) (AnjutaPlugin *plugin, 
										const char *name,
										const GValue *value,
										gpointer user_data);

/**
 * AnjutaPluginValueRemoved:
 * @plugin: The #AnjutaPlugin based plugin
 * @name: name of value being added.
 * @user_data: User data set during anjuta_plugin_add_watch()
 *
 * The callback to pass to anjuta_plugin_add_watch(). When the @name value
 * is removed from the shell (by the plugin exporting this value), this
 * callback will be called.
 */
typedef void (*AnjutaPluginValueRemoved) (AnjutaPlugin *plugin, 
										  const char *name,
										  gpointer user_data);


#define ANJUTA_TYPE_PLUGIN         (anjuta_plugin_get_type ())
#define ANJUTA_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN, AnjutaPlugin))
#define ANJUTA_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), ANJUTA_TYPE_PLUGIN, AnjutaPluginClass))
#define ANJUTA_IS_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN))
#define ANJUTA_IS_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN))
#define ANJUTA_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN, AnjutaPluginClass))

struct _AnjutaPlugin {
	GObject parent;	

	/* The shell in which the plugin has been added */
	AnjutaShell *shell;
	
	/*< private >*/
	AnjutaPluginPrivate *priv;
};

struct _AnjutaPluginClass {
	GObjectClass parent_class;

	/* Virtual functions */
	gboolean (*activate) (AnjutaPlugin *plugin);
	gboolean (*deactivate) (AnjutaPlugin *plugin);
};

GType anjuta_plugin_get_type   (void);

gboolean anjuta_plugin_activate (AnjutaPlugin *plugin);

gboolean anjuta_plugin_deactivate (AnjutaPlugin *plugin);

gboolean anjuta_plugin_is_active (AnjutaPlugin *plugin);

guint anjuta_plugin_add_watch (AnjutaPlugin *plugin, 
							   const gchar *name,
							   AnjutaPluginValueAdded added,
							   AnjutaPluginValueRemoved removed,
							   gpointer user_data);

void anjuta_plugin_remove_watch (AnjutaPlugin *plugin, guint id,
								 gboolean send_remove);

/**
 * ANJUTA_PLUGIN_BEGIN:
 * @class_name: Name of the class. e.g. EditorPlugin
 * @prefix: prefix of member function names. e.g. editor_plugin
 * 
 * This is a convienient macro defined to make it easy to write plugin
 * classes . This macro begins the class type definition. member function
 * @prefix _class_init and @prefix _instance_init should be statically defined
 * before using this macro.
 *
 * The class type definition is finished with ANJUTA_PLUGIN_END() macro. In
 * between which any number of interface definitions could be added with
 * ANJUTA_INTERFACE_MACRO().
 */
#define ANJUTA_PLUGIN_BEGIN(class_name, prefix) \
static GType \
prefix##_get_type (GluePlugin *plugin) \
{ \
	static GType type = 0; \
	if (!type) { \
		static const GTypeInfo type_info = { \
			sizeof (class_name##Class), \
			NULL, \
			NULL, \
			(GClassInitFunc)prefix##_class_init, \
			NULL, \
			NULL, \
			sizeof (class_name), \
			0, \
			(GInstanceInitFunc)prefix##_instance_init \
		}; \
		type = g_type_module_register_type (G_TYPE_MODULE (plugin), \
						    ANJUTA_TYPE_PLUGIN, \
						    #class_name, \
						    &type_info, 0);
/**
 * ANJUTA_PLUGIN_END:
 * 
 * Ends the plugin class type definition started with ANJUTA_PLUGIN_BEGIN()
 */
#define ANJUTA_PLUGIN_END \
	} \
	return type; \
}

/**
 * ANJUTA_PLUGIN_BOILERPLATE:
 * @class_name: Name of the class. e.g EditorPlugin
 * @prefix: prefix of member function names. e.g. editor_plugin
 * 
 * This macro is similar to using ANJUTA_PLUGIN_BEGIN() and then immediately
 * using ANJUTA_PLUGIN_END(). It is basically a plugin type definition macro
 * that does not have any interface implementation.
 */
#define ANJUTA_PLUGIN_BOILERPLATE(class_name, prefix) \
ANJUTA_PLUGIN_BEGIN(class_name, prefix); \
ANJUTA_PLUGIN_END

/**
 * ANJUTA_SIMPLE_PLUGIN:
 * @class_name: Name of the class. e.g. EditorPlugin
 * @prefix: prefix of member function names. e.g. editor_plugin
 * 
 * Sets up necessary codes for the plugin factory to know the class type of
 * of the plugin. This macro is generally used at the end of plugin class
 * and member functions definitions. 
 */
#define ANJUTA_SIMPLE_PLUGIN(class_name, prefix) \
G_MODULE_EXPORT void glue_register_components (GluePlugin *plugin); \
G_MODULE_EXPORT GType glue_get_component_type (GluePlugin *plugin, const char *name); \
G_MODULE_EXPORT void \
glue_register_components (GluePlugin *plugin) \
{ \
	prefix##_get_type (plugin); \
} \
G_MODULE_EXPORT GType \
glue_get_component_type (GluePlugin *plugin, const char *name) \
{ \
	if (!strcmp (name, #class_name)) { \
		return prefix##_get_type (plugin); \
	} else { \
		return G_TYPE_INVALID;  \
	} \
}

G_END_DECLS

#endif
