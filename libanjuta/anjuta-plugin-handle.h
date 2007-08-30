/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-plugin-handle.h
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

#ifndef _ANJUTA_PLUGIN_HANDLE_H_
#define _ANJUTA_PLUGIN_HANDLE_H_

#include <glib-object.h>
#include <libanjuta/anjuta-plugin-description.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_PLUGIN_HANDLE             (anjuta_plugin_handle_get_type ())
#define ANJUTA_PLUGIN_HANDLE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_PLUGIN_HANDLE, AnjutaPluginHandle))
#define ANJUTA_PLUGIN_HANDLE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_PLUGIN_HANDLE, AnjutaPluginHandleClass))
#define ANJUTA_IS_PLUGIN_HANDLE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_PLUGIN_HANDLE))
#define ANJUTA_IS_PLUGIN_HANDLE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_PLUGIN_HANDLE))
#define ANJUTA_PLUGIN_HANDLE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_PLUGIN_HANDLE, AnjutaPluginHandleClass))

typedef struct _AnjutaPluginHandleClass AnjutaPluginHandleClass;
typedef struct _AnjutaPluginHandlePriv AnjutaPluginHandlePriv;
typedef struct _AnjutaPluginHandle AnjutaPluginHandle;

struct _AnjutaPluginHandleClass
{
	GObjectClass parent_class;
};

struct _AnjutaPluginHandle
{
	GObject parent_instance;
	AnjutaPluginHandlePriv *priv;
};

GType anjuta_plugin_handle_get_type (void) G_GNUC_CONST;
AnjutaPluginHandle* anjuta_plugin_handle_new (const gchar *plugin_desc_path);
const char* anjuta_plugin_handle_get_id (AnjutaPluginHandle *plugin_handle);
const char* anjuta_plugin_handle_get_name (AnjutaPluginHandle *plugin_handle);
const char* anjuta_plugin_handle_get_about (AnjutaPluginHandle *plugin_handle);
const char* anjuta_plugin_handle_get_icon_path (AnjutaPluginHandle *plugin_handle);
gboolean anjuta_plugin_handle_get_user_activatable (AnjutaPluginHandle *plugin_handle);
gboolean anjuta_plugin_handle_get_resident (AnjutaPluginHandle *plugin_handle);
const char* anjuta_plugin_handle_get_language (AnjutaPluginHandle *plugin_handle);
AnjutaPluginDescription* anjuta_plugin_handle_get_description (AnjutaPluginHandle *plugin_handle);
GList* anjuta_plugin_handle_get_dependency_names (AnjutaPluginHandle *plugin_handle);
GHashTable* anjuta_plugin_handle_get_dependencies (AnjutaPluginHandle *plugin_handle);
GHashTable* anjuta_plugin_handle_get_dependents (AnjutaPluginHandle *plugin_handle);
GList* anjuta_plugin_handle_get_interfaces (AnjutaPluginHandle *plugin_handle);
gboolean anjuta_plugin_handle_get_can_load (AnjutaPluginHandle *plugin_handle);
gboolean anjuta_plugin_handle_get_checked (AnjutaPluginHandle *plugin_handle);
gint anjuta_plugin_handle_get_resolve_pass (AnjutaPluginHandle *plugin_handle);
void anjuta_plugin_handle_set_can_load (AnjutaPluginHandle *plugin_handle,
										gboolean can_load);
void anjuta_plugin_handle_set_checked (AnjutaPluginHandle *plugin_handle,
									   gboolean checked);
void anjuta_plugin_handle_set_resolve_pass (AnjutaPluginHandle *plugin_handle,
											gboolean resolve_pass);
void anjuta_plugin_handle_unresolve_dependencies (AnjutaPluginHandle *plugin_handle);

G_END_DECLS

#endif /* _ANJUTA_PLUGIN_HANDLE_H_ */
