/* -*- Mode: C; indent-spaces-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.h
 * Copyright (C) 2012 Carl-Anton Ingmarsson <carlantoni@src.gnome.org>
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

#ifndef _PLUGIN_H_
#define _PLUGIN_H_

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>

#include "quick-open-dialog.h"


#define ANJUTA_TYPE_PLUGIN_QUICK_OPEN           (quick_open_plugin_get_type(NULL))
#define ANJUTA_PLUGIN_QUICK_OPEN(o)             (G_TYPE_CHECK_INSTANCE_CAST((o), ANJUTA_TYPE_PLUGIN_QUICK_OPEN, QuickOpenPlugin))
#define ANJUTA_PLUGIN_QUICK_OPEN_CLASS(k)       (G_TYPE_CHECK_CLASS_CAST((k), ANJUTA_TYPE_PLUGIN_QUICK_OPEN, QuickOpenPluginClass))
#define ANJUTA_IS_PLUGIN_QUICK_OPEN(o)          (G_TYPE_CHECK_INSTANCE_TYPE((o), ANJUTA_TYPE_PLUGIN_QUICK_OPEN))
#define ANJUTA_IS_PLUGIN_QUICK_OPEN_CLASS(k)    (G_TYPE_CHECK_CLASS_TYPE((k), ANJUTA_TYPE_PLUGIN_QUICK_OPEN))
#define ANJUTA_PLUGIN_QUICK_OPEN_GET_CLASS(o)   (G_TYPE_INSTANCE_GET_CLASS((o), ANJUTA_TYPE_PLUGIN_QUICK_OPEN, QuickOpenPluginClass))

typedef struct _QuickOpenPlugin QuickOpenPlugin;
typedef struct _QuickOpenPluginClass QuickOpenPluginClass;

struct _QuickOpenPlugin
{
    AnjutaPlugin parent;

    /* Menu item */
    gint uiid;
    GtkActionGroup* action_group;

    IAnjutaProjectManager* project_manager;
    guint project_watch_id;

    IAnjutaDocumentManager* docman;

    QuickOpenDialog* dialog;
};


extern GType quick_open_plugin_get_type(GTypeModule *module);

#endif
