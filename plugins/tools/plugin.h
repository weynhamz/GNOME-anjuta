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

#ifndef __TOOLS_PLUGIN__
#define __TOOLS_PLUGIN__

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>
#include <libanjuta/anjuta-launcher.h>

#include <gtk/gtk.h>

/*---------------------------------------------------------------------------*/

#define MENU_PLACEHOLDER "/MenuMain/PlaceHolderToolMenus/MenuTools"

#define ANJUTA_TOOLS_DIRECTORY PACKAGE_DATA_DIR"/tools"
#define LOCAL_ANJUTA_TOOLS_DIRECTORY "/.anjuta"
#define TOOLS_FILE	"tools-2.xml"
#define LOCAL_ANJUTA_SCRIPT_DIRECTORY "/.anjuta/script"

/*---------------------------------------------------------------------------*/

extern GType atp_plugin_get_type (AnjutaGluePlugin *plugin);
#define ANJUTA_TYPE_PLUGIN_ATP         (atp_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_ATP(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_ATP, ATPPlugin))
#define ANJUTA_PLUGIN_ATP_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_ATP, ATPPluginClass))
#define ANJUTA_IS_PLUGIN_ATP(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_ATP))
#define ANJUTA_IS_PLUGIN_ATP_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_ATP))
#define ANJUTA_PLUGIN_ATP_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_ATP, ATPPluginClass))

typedef struct _ATPPlugin ATPPlugin;
typedef struct _ATPPluginClass ATPPluginClass;

struct _ATPToolList* atp_plugin_get_tool_list (const ATPPlugin *this);
struct _ATPToolDialog* atp_plugin_get_tool_dialog (const ATPPlugin *this);
struct _ATPVariable* atp_plugin_get_variable (const ATPPlugin *this);
struct _ATPContextList* atp_plugin_get_context_list (const ATPPlugin *this);
GtkWindow* atp_plugin_get_app_window (const ATPPlugin *this);

#endif
