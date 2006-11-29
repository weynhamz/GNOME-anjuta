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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __PLUGIN_H__
#define __PLUGIN_H__


#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-debugger.h>
#include <libanjuta/interfaces/ianjuta-editor.h>

#define GLADE_FILE PACKAGE_DATA_DIR"/glade/anjuta-debug-manager.glade"

extern GType dma_plugin_get_type (GluePlugin *plugin);
#define ANJUTA_TYPE_PLUGIN_DEBUG_MANAGER         (dma_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_DEBUG_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_DEBUG_MANAGER, DebugManagerPlugin))
#define ANJUTA_PLUGIN_DEBUG_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_DEBUG_MANAGER, DebugManagerPluginClass))
#define ANJUTA_IS_PLUGIN_DEBUG_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_DEBUG_MANAGER))
#define ANJUTA_IS_PLUGIN_DEBUG_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_DEBUG_MANAGER))
#define ANJUTA_PLUGIN_DEBUG_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_DEBUG_MANAGER, DebugManagerPluginClass))

typedef struct _DebugManagerPlugin DebugManagerPlugin;
typedef struct _DebugManagerPluginClass DebugManagerPluginClass;

enum {
ACTION_GROUP_DEFAULT = 0,
ACTION_GROUP_EXECUTE = 1,
LAST_ACTION_GROUP };

#endif
