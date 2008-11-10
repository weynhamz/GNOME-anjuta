/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <config.h>
#include <gtk/gtkactiongroup.h>
#include <glib/gi18n.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/interfaces/ianjuta-wizard.h>

#include "file.h"
#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-file-wizard.ui"
#define ICON_FILE "anjuta-file-wizard-plugin.png"

static gpointer parent_class;

static void
project_root_added (AnjutaPlugin *plugin, const gchar *name,
					const GValue *value, gpointer user_data)
{
	AnjutaFileWizardPlugin *w_plugin;
	const gchar *root_uri;

	w_plugin = ANJUTA_PLUGIN_FILE_WIZARD (plugin);
	root_uri = g_value_get_string (value);
	
	if (root_uri)
	{
		gchar *root_dir = anjuta_util_get_local_path_from_uri (root_uri);
		if (root_dir)
			w_plugin->top_dir = g_strdup(root_dir);
		else
			w_plugin->top_dir = NULL;
		g_free (root_dir);
	}
	else
		w_plugin->top_dir = NULL;
}

static void
project_root_removed (AnjutaPlugin *plugin, const gchar *name,
					  gpointer user_data)
{
	AnjutaFileWizardPlugin *w_plugin;
	w_plugin = ANJUTA_PLUGIN_FILE_WIZARD (plugin);
	
	g_free (w_plugin->top_dir);
	w_plugin->top_dir = NULL;
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaFileWizardPlugin *w_plugin;
	static gboolean initialized = FALSE;
	
	DEBUG_PRINT ("AnjutaFileWizardPlugin: Activating File wizard plugin ...");
	w_plugin = ANJUTA_PLUGIN_FILE_WIZARD (plugin);
	w_plugin->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	
	/* set up project directory watch */
	w_plugin->root_watch_id = anjuta_plugin_add_watch (plugin,
													   IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI,
													   project_root_added,
													   project_root_removed,
													   NULL);
	initialized = TRUE;
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	AnjutaFileWizardPlugin *w_plugin;
	w_plugin = ANJUTA_PLUGIN_FILE_WIZARD (plugin);
	anjuta_plugin_remove_watch (plugin, w_plugin->root_watch_id, TRUE);
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
	/* Finalization codes here */
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
file_wizard_plugin_instance_init (GObject *obj)
{
	AnjutaFileWizardPlugin *plugin = ANJUTA_PLUGIN_FILE_WIZARD (obj);
	plugin->top_dir = NULL;
}

static void
file_wizard_plugin_class_init (GObjectClass *klass) 
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
	AnjutaFileWizardPlugin *plugin;
	IAnjutaDocumentManager *docman;
	
	plugin = ANJUTA_PLUGIN_FILE_WIZARD (wiz);
	docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (wiz)->shell,
										 IAnjutaDocumentManager, NULL);
	display_new_file(plugin, docman);
}

static void
iwizard_iface_init (IAnjutaWizardIface *iface)
{
	iface->activate = iwizard_activate;
}

ANJUTA_PLUGIN_BEGIN (AnjutaFileWizardPlugin, file_wizard_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(iwizard, IANJUTA_TYPE_WIZARD);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (AnjutaFileWizardPlugin, file_wizard_plugin);
