/* -*- Mode: C; indent-spaces-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.h
    Copyright (C) 2012 Carl-Anton Ingmarsson

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

#ifndef _PLUGIN_H_
#define _PLUGIN_H_

#include <libanjuta/anjuta-plugin.h>

#define ANJUTA_TYPE_PLUGIN_JHBUILD          (jhbuild_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_JHBUILD(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_JHBUILD, JHBuildPlugin))
#define ANJUTA_PLUGIN_JHBUILD_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_JHBUILD, JHBuildPluginClass))
#define ANJUTA_IS_PLUGIN_JHBUILD(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_JHBUILD))
#define ANJUTA_IS_PLUGIN_JHBUILD_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_JHBUILD))
#define ANJUTA_PLUGIN_JHBUILD_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_JHBUILD, JHBuildPluginClass))

typedef struct _JHBuildPlugin JHBuildPlugin;
typedef struct _JHBuildPluginClass JHBuildPluginClass;

struct _JHBuildPlugin
{
    AnjutaPlugin parent;

    AnjutaShell* shell;

    char* prefix;
    char* libdir;
};

extern GType jhbuild_plugin_get_type(GTypeModule *module);

#endif
