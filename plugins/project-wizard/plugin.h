/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.h
    Copyright (C) 2000 Naba Kumar

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef __PROJECT_WIZARD_PLUGIN__
#define __PROJECT_WIZARD_PLUGIN__

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>
#include <libanjuta/anjuta-preferences.h>

extern GType npw_plugin_get_type (GTypeModule *module);
#define ANJUTA_TYPE_PLUGIN_NPW         (npw_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_NPW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_NPW, NPWPlugin))
#define ANJUTA_PLUGIN_NPW_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_NPW, NPWPluginClass))
#define ANJUTA_IS_PLUGIN_NPW(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_NPW))
#define ANJUTA_IS_PLUGIN_NPW_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_NPW))
#define ANJUTA_PLUGIN_NPW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_NPW, NPWPluginClass))

typedef struct _NPWPlugin NPWPlugin;
typedef struct _NPWPluginClass NPWPluginClass;

struct _NPWPlugin {
	AnjutaPlugin parent;

	struct _NPWDruid* druid;
	struct _NPWInstall* install;
	IAnjutaMessageView* view;
};

struct _NPWPluginClass {
	AnjutaPluginClass parent_class;
};

IAnjutaMessageView* npw_plugin_create_view (NPWPlugin* plugin);
void npw_plugin_append_view (NPWPlugin* plugin, const gchar* text);
void npw_plugin_print_view (NPWPlugin* plugin, IAnjutaMessageViewType type, const gchar* summary, const gchar* details);

#endif
