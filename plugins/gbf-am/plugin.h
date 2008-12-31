/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.h
    Copyright (C) 2008 Sébastien Granjoux

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

#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include <libanjuta/anjuta-plugin.h>

extern GType gbf_am_plugin_get_type (GTypeModule *module);
#define GBF_TYPE_PLUGIN_AM         (gbf_am_plugin_get_type (NULL))
#define GBF_PLUGIN_AM(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GBF_TYPE_PLUGIN_AM, GbfAmPlugin))
#define GBF_PLUGIN_AM_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GBF_TYPE_PLUGIN_AM, GbfAmPluginClass))
#define GBF_IS_PLUGIN_AM(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GBF_TYPE_PLUGIN_AM))
#define GBF_IS_PLUGIN_AM_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GBF_TYPE_PLUGIN_AM))
#define GBF_PLUGIN_AM_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GBF_TYPE_PLUGIN_AM, GbfAmPluginClass))

typedef struct _GbfAmPlugin GbfAmPlugin;
typedef struct _GbfAmPluginClass GbfAmPluginClass;

struct _GbfAmPlugin 
{
	AnjutaPlugin parent;
};

struct _GbfAmPluginClass
{
	AnjutaPluginClass parent_class;
};

#endif
