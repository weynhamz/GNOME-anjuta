/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-plugin-manager.h
 * Copyright (C) Naba Kumar  <naba@gnome.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _ANJUTA_PLUGIN_MANAGER_H_
#define _ANJUTA_PLUGIN_MANAGER_H_

#include <glib-object.h>
#include <libanjuta/anjuta-status.h>
#include <libanjuta/anjuta-plugin-description.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_PLUGIN_MANAGER             (anjuta_plugin_manager_get_type ())
#define ANJUTA_PLUGIN_MANAGER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_PLUGIN_MANAGER, AnjutaPluginManager))
#define ANJUTA_PLUGIN_MANAGER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_PLUGIN_MANAGER, AnjutaPluginManagerClass))
#define ANJUTA_IS_PLUGIN_MANAGER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_PLUGIN_MANAGER))
#define ANJUTA_IS_PLUGIN_MANAGER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_PLUGIN_MANAGER))
#define ANJUTA_PLUGIN_MANAGER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_PLUGIN_MANAGER, AnjutaPluginManagerClass))
#define ANJUTA_PLUGIN_MANAGER_ERROR            (anjuta_plugin_manager_error_quark())

typedef enum
{
	ANJUTA_PLUGIN_MANAGER_MISSING_FACTORY,
	ANJUTA_PLUGIN_MANAGER_ERROR_UNKNOWN
} AnjutaPluginManagerError;


typedef struct _AnjutaPluginManagerClass AnjutaPluginManagerClass;
typedef struct _AnjutaPluginManager AnjutaPluginManager;
typedef struct _AnjutaPluginManagerPriv AnjutaPluginManagerPriv;

struct _AnjutaPluginManagerClass
{
	GObjectClass parent_class;

	/* Signals */
	void(* plugin_activated) (AnjutaPluginManager *self,
							  AnjutaPluginDescription* plugin_desc,
							  GObject *plugin);
	void(* plugin_deactivated) (AnjutaPluginManager *self,
								AnjutaPluginDescription* plugin_desc,
								GObject *plugin);
};

struct _AnjutaPluginManager
{
	GObject parent_instance;
	AnjutaPluginManagerPriv *priv;
};

GQuark anjuta_plugin_manager_error_quark (void);
GType anjuta_plugin_manager_get_type (void) G_GNUC_CONST;
AnjutaPluginManager* anjuta_plugin_manager_new (GObject *shell,
												AnjutaStatus *status,
												GList* plugin_search_paths);

/* Plugin activation, deactivation and retrival */
GObject* anjuta_plugin_manager_get_plugin (AnjutaPluginManager *plugin_manager,
										   const gchar *iface_name);
GObject* anjuta_plugin_manager_get_plugin_by_id (AnjutaPluginManager *plugin_manager,
												 const gchar *plugin_id);
gboolean anjuta_plugin_manager_unload_plugin (AnjutaPluginManager *plugin_manager,
											  GObject *plugin);
gboolean anjuta_plugin_manager_unload_plugin_by_id (AnjutaPluginManager *plugin_manager,
													const gchar *plugin_id);
GList* anjuta_plugin_manager_get_active_plugins (AnjutaPluginManager *plugin_manager);
GList* anjuta_plugin_manager_get_active_plugin_objects (AnjutaPluginManager *plugin_manager);

/* Selection dialogs */
GtkWidget* anjuta_plugin_manager_get_plugins_page (AnjutaPluginManager *plugin_manager);
GtkWidget* anjuta_plugin_manager_get_remembered_plugins_page (AnjutaPluginManager *plugin_manager);

/* Plugin queries based on meta-data */
/* Returns a list of plugin Descriptions */
GList* anjuta_plugin_manager_query (AnjutaPluginManager *plugin_manager,
									const gchar *section_name,
									const gchar *attribute_name,
									const gchar *attribute_value,
									...);

/* Returns the plugin description that has been selected from the list */
AnjutaPluginDescription* anjuta_plugin_manager_select (AnjutaPluginManager *plugin_manager,
													   gchar *title, gchar *description,
													   GList *plugin_descriptions);

/* Returns the plugin that has been selected and activated */
GObject* anjuta_plugin_manager_select_and_activate (AnjutaPluginManager *plugin_manager,
													gchar *title,
													gchar *description,
													GList *plugin_descriptions);

void anjuta_plugin_manager_activate_plugins (AnjutaPluginManager *plugin_manager,
											 GList *plugin_descs);

void anjuta_plugin_manager_unload_all_plugins (AnjutaPluginManager *plugin_manager);

gchar* anjuta_plugin_manager_get_remembered_plugins (AnjutaPluginManager *plugin_manager);
void anjuta_plugin_manager_set_remembered_plugins (AnjutaPluginManager *plugin_manager,
												   const gchar *remembered_plugins);

/**
 * anjuta_plugin_manager_get_interface:
 * @plugin_manager: A #AnjutaPluginManager object
 * @iface_type: The interface type implemented by the object to be found
 * @error: Error propagation object.
 *
 * Equivalent to anjuta_plugin_manager_get_object(), but additionally
 * typecasts returned object to the interface type. It also takes
 * interface type directly. A usage of this function is:
 * <programlisting>
 * IAnjutaDocumentManager *docman =
 *     anjuta_plugin_manager_get_interface (plugin_manager, IAnjutaDocumentManager, error);
 * </programlisting>
 */
#define anjuta_plugin_manager_get_interface(plugin_manager, iface_type, error) \
	(((iface_type*) anjuta_plugin_manager_get_plugin((plugin_manager), #iface_type, (error)))

G_END_DECLS

#endif /* _ANJUTA_PLUGIN_MANAGER_H_ */
