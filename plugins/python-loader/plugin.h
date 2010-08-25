/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.h
    Copyright (C) 2007 Sebastien Granjoux

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

#ifndef __PYTHON_LOADER_PLUGIN__
#define __PYTHON_LOADER_PLUGIN__

#include <libanjuta/anjuta-plugin.h>


extern GType pyl_plugin_get_type (AnjutaGluePlugin *plugin);
#define ANJUTA_TYPE_PYTHON_LOADER_PLUGIN         (pyl_plugin_get_type (NULL))
#define ANJUTA_PYTHON_LOADER_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PYTHON_LOADER_PLUGIN, PythonLoaderPlugin))
#define ANJUTA_PYTHON_LOADER_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PYTHON_LOADER_PLUGIN, PythonLoaderPluginClass))
#define ANJUTA_IS_PYTHON_LOADER_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PYTHON_LOADER_PLUGIN))
#define ANJUTA_IS_PYTHON_LOADER_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PYTHON_LOADER_PLUGIN))
#define ANJUTA_PYTHON_LOADER_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PYTHON_LOADER_PLUGIN, PythonLoaderPluginClass))

typedef struct _PythonLoaderPlugin PythonLoaderPlugin;
typedef struct _PythonLoaderPluginClass PythonLoaderPluginClass;

struct _PythonLoaderPlugin {
	AnjutaPlugin parent;
};

struct _PythonLoaderPluginClass {
	AnjutaPluginClass parent_class;
};

#endif
