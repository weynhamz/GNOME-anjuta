/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*  plugin.h (c) Johannes Schmid 2004
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
 
#ifndef _MV_PLUGIN_H
#define _MV_PLUGIN_H

#include <libanjuta/anjuta-plugin.h>

extern GType message_view_plugin_get_type (AnjutaGluePlugin *plugin);
#define ANJUTA_TYPE_PLUGIN_MESSAGE_VIEW         (message_view_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_MESSAGE_VIEW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_MESSAGE_VIEW, MessageViewPlugin))
#define ANJUTA_PLUGIN_MESSAGE_VIEW_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_MESSAGE_VIEW, MessageViewPluginClass))
#define ANJUTA_IS_PLUGIN_MESSAGE_VIEW(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_MESSAGE_VIEW))
#define ANJUTA_IS_PLUGIN_MESSAGE_VIEW_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_MESSAGE_VIEW))
#define ANJUTA_PLUGIN_MESSAGE_VIEW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_MESSAGE_VIEW, MessageViewPluginClass))

typedef struct _MessageViewPlugin MessageViewPlugin;
typedef struct _MessageViewPluginClass MessageViewPluginClass;

struct _MessageViewPlugin {
	AnjutaPlugin parent;
	GtkWidget* msgman;
	GtkActionGroup *action_group;
	gint uiid;
};

struct _MessageViewPluginClass {
	AnjutaPluginClass parent_class;
};

#endif /* _MV_PLUGIN_H */
