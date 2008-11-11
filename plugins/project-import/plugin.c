/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
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

#include <config.h>
#include <gtk/gtkactiongroup.h>
#include <glib/gi18n.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-wizard.h>
#include <libanjuta/interfaces/ianjuta-file.h>

#include "plugin.h"
#include "project-import.h"

#define ICON_FILE "anjuta-project-import-plugin-48.png"

static gpointer parent_class;

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaProjectImportPlugin *iplugin;
	
	DEBUG_PRINT ("%s", "AnjutaProjectImportPlugin: Activating Project Import Plugin ...");
	
	iplugin = ANJUTA_PLUGIN_PROJECT_IMPORT (plugin);
	iplugin->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	AnjutaProjectImportPlugin *iplugin;
	iplugin = ANJUTA_PLUGIN_PROJECT_IMPORT (plugin);
	return TRUE;
}

static void
dispose (GObject *obj)
{
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
finalize (GObject *obj)
{
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
project_import_plugin_instance_init (GObject *obj)
{
	// AnjutaFileWizardPlugin *plugin = ANJUTA_PLUGIN_PROJECT_IMPORT (obj);
}

static void
project_import_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
	klass->finalize = finalize;
}

static void
iwizard_activate (IAnjutaWizard *wiz, GError **err)
{
	AnjutaProjectImportPlugin* plugin = ANJUTA_PLUGIN_PROJECT_IMPORT (wiz);
	ProjectImport* pi;
	
	pi = project_import_new(ANJUTA_PLUGIN(plugin));
}

static void
iwizard_iface_init (IAnjutaWizardIface *iface)
{
	iface->activate = iwizard_activate;
}

static void
ifile_open (IAnjutaFile *ifile, GFile* file, GError **err)
{
	gchar *dir, *ext, *project_name;
	ProjectImport* pi;
	AnjutaProjectImportPlugin* plugin = ANJUTA_PLUGIN_PROJECT_IMPORT (ifile);
	gchar* uri = g_file_get_uri (file);
	
	g_return_if_fail (uri != NULL && strlen (uri) > 0);
	
	dir = g_path_get_dirname (uri);
	project_name = g_path_get_basename (uri);
	ext = strrchr (project_name, '.');
	if (ext)
		*ext = '\0';
	
	pi = project_import_new(ANJUTA_PLUGIN(plugin));
	project_import_set_name (pi, project_name);
	project_import_set_directory (pi, dir);
	g_free (uri);
}

static GFile*
ifile_get_file (IAnjutaFile *file, GError **err)
{
	g_warning ("Unsupported operation");
	return NULL;
}

static void
ifile_iface_init (IAnjutaFileIface *iface)
{
	iface->open = ifile_open;
	iface->get_file = ifile_get_file;
}

ANJUTA_PLUGIN_BEGIN (AnjutaProjectImportPlugin, project_import_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(iwizard, IANJUTA_TYPE_WIZARD);
ANJUTA_PLUGIN_ADD_INTERFACE(ifile, IANJUTA_TYPE_FILE);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (AnjutaProjectImportPlugin, project_import_plugin);
