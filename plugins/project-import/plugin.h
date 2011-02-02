/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.h
    Copyright (C) 2000 Naba Kumar, 2005 Johannes Schmid

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

#ifndef __FILE_WIZARD_PLUGIN__
#define __FILE_WIZARD_PLUGIN__

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-ui.h>

extern GType project_import_plugin_get_type (GTypeModule *module);
#define ANJUTA_TYPE_PLUGIN_PROJECT_IMPORT         (project_import_plugin_get_type (NULL))
#define ANJUTA_PLUGIN_PROJECT_IMPORT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PLUGIN_PROJECT_IMPORT, AnjutaProjectImportPlugin))
#define ANJUTA_PLUGIN_PROJECT_IMPORT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), ANJUTA_TYPE_PLUGIN_PROJECT_IMPORT, AnjutaProjectImportPluginClass))
#define ANJUTA_IS_PLUGIN_PROJECT_IMPORT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PLUGIN_PROJECT_IMPORT))
#define ANJUTA_IS_PLUGIN_PROJECT_IMPORT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PLUGIN_PROJECT_IMPORT))
#define ANJUTA_PLUGIN_PROJECT_IMPORT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_PLUGIN_PROJECT_IMPORT, AnjutaProjectImportPluginClass))

typedef struct _AnjutaProjectImportPlugin AnjutaProjectImportPlugin;
typedef struct _AnjutaProjectImportPluginClass AnjutaProjectImportPluginClass;

struct _AnjutaProjectImportPlugin 
{
	AnjutaPlugin parent;
	AnjutaPreferences *prefs;
};

struct _AnjutaProjectImportPluginClass
{
	AnjutaPluginClass parent_class;
};

#endif
