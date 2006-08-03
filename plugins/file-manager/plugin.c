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
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/interfaces/ianjuta-help.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>
#include <libanjuta/interfaces/ianjuta-file-manager.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include <libanjuta/plugins.h>
#include <libanjuta/anjuta-debug.h>

#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-file-manager.ui"
#define PREFS_GLADE PACKAGE_DATA_DIR"/glade/anjuta-file-manager-plugin.glade"
#define ICON_FILE "anjuta-file-manager-plugin.png"

static gpointer parent_class;

static void refresh (GtkAction *action, FileManagerPlugin *plugin)
{
	fv_refresh (plugin);
}

static GtkActionEntry popup_actions[] = 
{
	{
		"ActionPopupFileManagerRefresh", GTK_STOCK_REFRESH,
		N_("_Refresh"), NULL, N_("Refresh file manager tree"),
		G_CALLBACK (refresh)
	}
};

static void
set_default_root_directory (FileManagerPlugin *fv)
{
	gchar* root = anjuta_preferences_get(fv->prefs, "root.dir");
	if (root)
	{
		fv_set_root (fv, root);
		g_free (root);
	}
	else
	{
		fv_set_root (fv, "/");
	}
	fv_refresh (fv);
}

static void
on_gconf_notify_prefs (GConfClient *gclient, guint cnxn_id,
					   GConfEntry *entry, gpointer user_data)
{
	FileManagerPlugin *fv = (FileManagerPlugin*)user_data;
	if (fv->project_is_loaded == FALSE)
		set_default_root_directory (fv);
	else
		fv_refresh (fv);
}

#define REGISTER_NOTIFY(key, func) \
	notify_id = anjuta_preferences_notify_add (fv->prefs, \
											   key, func, fv, NULL); \
	fv->gconf_notify_ids = g_list_prepend (fv->gconf_notify_ids, \
										   (gpointer)(notify_id));
static void
prefs_init (FileManagerPlugin *fv)
{
	guint notify_id;
	REGISTER_NOTIFY ("root.dir", on_gconf_notify_prefs);
	REGISTER_NOTIFY ("filter.dir.ignore.hidden", on_gconf_notify_prefs);
	REGISTER_NOTIFY ("filter.dir.match", on_gconf_notify_prefs);
	REGISTER_NOTIFY ("filter.dir.unmatch", on_gconf_notify_prefs);
	REGISTER_NOTIFY ("filter.file.ignore.hidden", on_gconf_notify_prefs);
	REGISTER_NOTIFY ("filter.file.match", on_gconf_notify_prefs);
	REGISTER_NOTIFY ("filter.file.unmatch", on_gconf_notify_prefs);
	REGISTER_NOTIFY ("filter.file.ignore.nonrepo", on_gconf_notify_prefs);
}

static void
prefs_finalize (FileManagerPlugin *fv)
{
	GList *node;
	node = fv->gconf_notify_ids;
	while (node)
	{
		anjuta_preferences_notify_remove (fv->prefs, (guint)node->data);
		node = g_list_next (node);
	}
	g_list_free (fv->gconf_notify_ids);
	fv->gconf_notify_ids = NULL;
}

static void
project_root_added (AnjutaPlugin *plugin, const gchar *name,
					const GValue *value, gpointer user_data)
{
	FileManagerPlugin *fm_plugin;
	const gchar *root_uri;

	fm_plugin = (FileManagerPlugin *)plugin;
	root_uri = g_value_get_string (value);
	if (root_uri)
	{
		gchar *root_dir = gnome_vfs_get_local_path_from_uri (root_uri);
		if (root_dir)
		{
			fv_set_root (fm_plugin, root_dir);
			fv_refresh (fm_plugin);
			((FileManagerPlugin *)plugin)->project_is_loaded = TRUE;
		}
		else
			set_default_root_directory (fm_plugin);
		g_free (root_dir);
	}
	else
		set_default_root_directory (fm_plugin);
}

static void
project_root_removed (AnjutaPlugin *plugin, const gchar *name,
					  gpointer user_data)
{
	set_default_root_directory ((FileManagerPlugin *)plugin);
	((FileManagerPlugin *)plugin)->project_is_loaded = FALSE;
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	FileManagerPlugin *fm_plugin;
	gboolean initialized = FALSE;
	
	DEBUG_PRINT ("FileManagerPlugin: Activating File Manager plugin ...");
	
	fm_plugin = (FileManagerPlugin*) plugin;
	fm_plugin->ui = anjuta_shell_get_ui (plugin->shell, NULL);
	fm_plugin->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	fv_init (fm_plugin);
	
	/* Add action group */
	fm_plugin->action_group = 
		anjuta_ui_add_action_group_entries (fm_plugin->ui,
											"ActionGroupFileManager",
											_("File manager popup actions"),
											popup_actions, 1,
											GETTEXT_PACKAGE, plugin);
	/* Add UI */
	fm_plugin->merge_id = 
		anjuta_ui_merge (fm_plugin->ui, UI_FILE);
	
	/* Added widgets */
	anjuta_shell_add_widget (plugin->shell, fm_plugin->scrolledwindow,
							 "AnjutaFileManager", _("Files"), GTK_STOCK_OPEN,
							 ANJUTA_SHELL_PLACEMENT_LEFT, NULL);
	
	/* set up project directory watch */
	fm_plugin->root_watch_id = anjuta_plugin_add_watch (plugin,
									"project_root_uri",
									project_root_added,
									project_root_removed, NULL);
	initialized = FALSE;
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	GtkWidget *widget_to_remove;
	FileManagerPlugin *fm_plugin;
	
	fm_plugin = (FileManagerPlugin*) plugin;
	
	/* Remove watches */
	anjuta_plugin_remove_watch (plugin, fm_plugin->root_watch_id, FALSE);
	
	/* Finalize it first so that we release our refs */
	widget_to_remove = fm_plugin->scrolledwindow;
	fv_finalize(fm_plugin);
	
	/* Remove widgets */
	anjuta_shell_remove_widget (plugin->shell, widget_to_remove, NULL);
	
	/* Remove UI */
	anjuta_ui_unmerge (fm_plugin->ui, fm_plugin->merge_id);
	
	/* Remove action group */
	anjuta_ui_remove_action_group (fm_plugin->ui, fm_plugin->action_group);
	
	fm_plugin->root_watch_id = 0;
	return TRUE;
}

static void
dispose (GObject *obj)
{
	/* FIXME: */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
file_manager_plugin_instance_init (GObject *obj)
{
	FileManagerPlugin *plugin = (FileManagerPlugin*) obj;
	plugin->tree = NULL;
	plugin->scrolledwindow = NULL;
	plugin->top_dir = NULL;
	plugin->root_watch_id = 0;
	plugin->gconf_notify_ids = NULL;
	plugin->project_is_loaded = FALSE;
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

static void
ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	FileManagerPlugin* fm_plugin = (FileManagerPlugin*) ipref;
	/* Add preferences */
	GladeXML *gxml = glade_xml_new (PREFS_GLADE, "dialog.file.filter", NULL);
		
	anjuta_preferences_add_page (prefs,
									gxml, "File Manager", _("File Manager"), ICON_FILE);
	g_object_unref (G_OBJECT (gxml));
	prefs_init (fm_plugin);
	on_gconf_notify_prefs (NULL, 0, NULL, fm_plugin);
}

static void
ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	FileManagerPlugin* fm_plugin = (FileManagerPlugin*) ipref;
	prefs_finalize (fm_plugin);
	anjuta_preferences_dialog_remove_page(ANJUTA_PREFERENCES_DIALOG(prefs), _("File Manager"));
}

static void
ipreferences_iface_init(IAnjutaPreferencesIface* iface)
{
	iface->merge = ipreferences_merge;
	iface->unmerge = ipreferences_unmerge;	
}

ANJUTA_PLUGIN_BEGIN (FileManagerPlugin, file_manager_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE (ifile_manager, IANJUTA_TYPE_FILE_MANAGER);
ANJUTA_PLUGIN_ADD_INTERFACE (ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (FileManagerPlugin, file_manager_plugin);
