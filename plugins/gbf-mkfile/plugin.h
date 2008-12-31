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

extern GType gbf_mkfile_plugin_get_type (GTypeModule *module);
#define GBF_TYPE_PLUGIN_MKFILE         (gbf_mkfile_plugin_get_type (NULL))
#define GBF_PLUGIN_MKFILE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GBF_TYPE_PLUGIN_MKFILE, GbfMkfilePlugin))
#define GBF_PLUGIN_MKFILE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GBF_TYPE_PLUGIN_MKFILE, GbfMkfilePluginClass))
#define GBF_IS_PLUGIN_MKFILE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GBF_TYPE_PLUGIN_MKFILE))
#define GBF_IS_PLUGIN_MKFILE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GBF_TYPE_PLUGIN_MKFILE))
#define GBF_PLUGIN_MKFILE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GBF_TYPE_PLUGIN_MKFILE, GbfMkfilePluginClass))

typedef struct _GbfMkfilePlugin GbfMkfilePlugin;
typedef struct _GbfMkfilePluginClass GbfMkfilePluginClass;

struct _GbfMkfilePlugin 
{
	AnjutaPlugin parent;
};

struct _GbfMkfilePluginClass
{
	AnjutaPluginClass parent_class;
};

#endif
