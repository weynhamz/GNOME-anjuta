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
#include <libanjuta/glue-plugin.h>

//#include <bonobo/bonobo-ui-component.h>

G_BEGIN_DECLS

typedef struct _AnjutaPlugin        AnjutaPlugin;
typedef struct _AnjutaPluginClass   AnjutaPluginClass;
typedef struct _AnjutaPluginPrivate AnjutaPluginPrivate;
typedef void (*AnjutaPluginValueAdded) (AnjutaPlugin *tool, 
				      const char *name,
				      const GValue *value,
				      gpointer data);
typedef void (*AnjutaPluginValueRemoved) (AnjutaPlugin *tool, 
					const char *name,
					gpointer data);


#define ANJUTA_TYPE_PLUGIN         (anjuta_plugin_get_type ())
#define ANJUTA_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN, AnjutaPlugin))
#define ANJUTA_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), ANJUTA_TYPE_PLUGIN, AnjutaPluginClass))
#define ANJUTA_IS_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN))
#define ANJUTA_IS_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN))
#define ANJUTA_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN, AnjutaPluginClass))

struct _AnjutaPlugin {
	GObject parent;	

	AnjutaShell *shell;
	// BonoboUIComponent *uic;
	AnjutaUI *ui;
	AnjutaPreferences *prefs;
	
	AnjutaPluginPrivate *priv;
};

struct _AnjutaPluginClass {
	GObjectClass parent_class;

	void (*shell_set) (AnjutaPlugin *tool);
  void (*prefs_set) (AnjutaPlugin *tool);
  void (*ui_set) (AnjutaPlugin *tool);
	gboolean (*shutdown) (AnjutaPlugin *tool);
};

GType anjuta_plugin_get_type   (void);

/*
void  anjuta_plugin_merge_ui   (AnjutaPlugin   *tool,
			      const char   *name,
			      const char   *datadir,
			      const char   *xmlfile,
			      BonoboUIVerb *verbs,
			      gpointer      user_data);
void  anjuta_plugin_unmerge_ui (AnjutaPlugin   *tool);
*/
guint anjuta_plugin_add_watch (AnjutaPlugin *tool, 
			     const char *name,
			     AnjutaPluginValueAdded added,
			     AnjutaPluginValueRemoved removed,
			     gpointer user_data);
void anjuta_plugin_remove_watch (AnjutaPlugin *tool,
			       guint id,
			       gboolean send_remove);

#define ANJUTA_PLUGIN_BOILERPLATE(class_name, prefix) \
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
						    &type_info, 0); \
	} \
	return type; \
}

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
