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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <config.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/interfaces/ianjuta-help.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>
#include <libanjuta/interfaces/ianjuta-file-manager.h>
#include <libanjuta/plugins.h>

#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-file-manager.ui"
#define PREFS_GLADE PACKAGE_DATA_DIR"/glade/anjuta-file-manager-plugin.glade"
#define ICON_FILE "anjuta-file-manager-plugin.png"

gpointer parent_class;

static void
preferences_changed (AnjutaPreferences *prefs, FileManagerPlugin *fv)
{
	fv_refresh (fv);
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	GladeXML *gxml;
	FileManagerPlugin *fm_plugin;
	
	g_message ("FileManagerPlugin: Activating File Manager plugin ...");
	fm_plugin = (FileManagerPlugin*) plugin;
	fm_plugin->ui = anjuta_shell_get_ui (plugin->shell, NULL);
	fm_plugin->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	fv_init (fm_plugin);
	
	/* Added widget in shell */
	anjuta_shell_add_widget (plugin->shell, fm_plugin->scrolledwindow,
							 "AnjutaFileManager", _("File"),
							 ANJUTA_SHELL_PLACEMENT_LEFT, NULL);
	
	/* Add preferences page */
	gxml = glade_xml_new (PREFS_GLADE, "dialog.file.filter", NULL);
	
	anjuta_preferences_add_page (fm_plugin->prefs,
								gxml, "File Manager", ICON_FILE);
	g_signal_connect (G_OBJECT (fm_plugin->prefs), "changed",
					  G_CALLBACK (preferences_changed), fm_plugin);
	g_object_unref (G_OBJECT (gxml));
	
	/* FIXME: For now just load '/' */
	fv_set_root (fm_plugin, "/");
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	FileManagerPlugin *fm_plugin;
	fm_plugin = (FileManagerPlugin*) plugin;
	
	g_signal_handlers_disconnect_by_func (G_OBJECT (fm_plugin->prefs),
										  G_CALLBACK (preferences_changed),
										  fm_plugin);
	fv_finalize(fm_plugin);
	return TRUE;
}

static void
dispose (GObject *obj)
{
}

static void
file_manager_plugin_instance_init (GObject *obj)
{
	FileManagerPlugin *plugin = (FileManagerPlugin*) obj;
	plugin->tree = NULL;
	plugin->scrolledwindow = NULL;
	plugin->top_dir = NULL;
	plugin->root_watch_id = 0;
}

static void
file_manager_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
}

static void
ifile_manager_set_root (IAnjutaFileManager *file_manager,
						const gchar *root, GError **err)
{
	fv_set_root ((FileManagerPlugin*) file_manager, root);
}

static void
ifile_manager_set_selected (IAnjutaFileManager *file_manager,
							const gchar *root, GError **err)
{
}

static gchar*
ifile_manager_get_selected (IAnjutaFileManager *file_manager, GError **err)
{
	return fv_get_selected_file_path((FileManagerPlugin*) file_manager);
}

static void
ifile_manager_iface_init(IAnjutaFileManagerIface *iface)
{
	iface->set_root = ifile_manager_set_root;
	iface->get_selected = ifile_manager_get_selected;
	iface->set_selected = ifile_manager_set_selected;
}

ANJUTA_PLUGIN_BEGIN (FileManagerPlugin, file_manager_plugin);
ANJUTA_INTERFACE (ifile_manager, IANJUTA_TYPE_FILE_MANAGER);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (FileManagerPlugin, file_manager_plugin);
