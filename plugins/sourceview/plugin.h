/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.h
 * Copyright (C) Johannes Schmid 2005 <jhs@gnome.org>
 * 
 * plugin.h is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * plugin.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.h.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _SOURCEVIEW_H_
#define _SOURCEVIEW_H_

#include <libanjuta/anjuta-plugin.h>

extern GType sourceview_plugin_get_type (GluePlugin *plugin);
#define ANJUTA_TYPE_PLUGIN_SOURCEVIEW         (sourceview_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_SOURCEVIEW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_SOURCEVIEW, SourceviewPlugin))
#define ANJUTA_PLUGIN_SOURCEVIEW_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_SOURCEVIEW, SourceviewPluginClass))
#define ANJUTA_IS_PLUGIN_SOURCEVIEW(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_SOURCEVIEW))
#define ANJUTA_IS_PLUGIN_SOURCEVIEW_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_SOURCEVIEW))
#define ANJUTA_PLUGIN_SOURCEVIEW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_SOURCEVIEW, SourceviewPluginClass))

typedef struct _SourceviewPlugin SourceviewPlugin;
typedef struct _SourceviewPluginClass SourceviewPluginClass;

struct _SourceviewPlugin{
	AnjutaPlugin parent;
	
	GtkWidget* check_color;
	GtkWidget* check_font;
};

struct _SourceviewPluginClass{
	AnjutaPluginClass parent_class;
};

#endif
