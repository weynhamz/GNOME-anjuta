/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2004 Naba Kumar, Johannes Schmid

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

#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs.h>

#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-vcs.h>

#include "plugin.h"
#include "subversion-actions.h"
#include "svn-backend.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-subversion.ui"
#define ICON_FILE "anjuta-subversion.png"

static gpointer parent_class;

static GtkActionEntry actions_subversion[] = {
	{
		"ActionMenuSubversion",                       /* Action name */
		NULL,                            /* Stock icon, if any */
		N_("_Subversion"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		NULL
	},
	{
		"ActionSubversionAdd",                       /* Action name */
		GTK_STOCK_ADD,                            /* Stock icon, if any */
		N_("_Add"),                     /* Display label */
		NULL,                                     /* short-cut */
		N_("Add a new file/directory to the Subversion tree"),                      /* Tooltip */
		G_CALLBACK (on_menu_subversion_add)    /* action callback */
	},
	{
		"ActionSubversionRemove",                       /* Action name */
		GTK_STOCK_REMOVE,                            /* Stock icon, if any */
		N_("_Remove"),                     /* Display label */
		NULL,                                     /* short-cut */
		N_("Remove a file/directory from Subversion tree"),                      /* Tooltip */
		G_CALLBACK (on_menu_subversion_remove)    /* action callback */
	},
	{
		"ActionSubversionCommit",                       /* Action name */
		GTK_STOCK_YES,                            /* Stock icon, if any */
		N_("_Commit"),                     /* Display label */
		NULL,                                     /* short-cut */
		N_("Commit your changes to the Subversion tree"),                      /* Tooltip */
		G_CALLBACK (on_menu_subversion_commit)    /* action callback */
	},
	{
		"ActionSubversionUpdate",                       /* Action name */
		GTK_STOCK_REFRESH,                            /* Stock icon, if any */
		N_("_Update"),                     /* Display label */
		NULL,                                     /* short-cut */
		N_("Sync your local copy with the Subversion tree"),                      /* Tooltip */
		G_CALLBACK (on_menu_subversion_update)    /* action callback */
	},
	{
		"ActionPopupSubversion",                       /* Action name */
		NULL,                            /* Stock icon, if any */
		N_("_Subversion"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		NULL
	},
		{
		"ActionPopupSubversionCommit",                       /* Action name */
		GTK_STOCK_YES,                            /* Stock icon, if any */
		N_("_Commit"),                     /* Display label */
		NULL,                                     /* short-cut */
		N_("Commit your changes to the Subversion tree"),                      /* Tooltip */
		G_CALLBACK (on_fm_subversion_commit)    /* action callback */
	},
	{
		"ActionPopupSubversionUpdate",                       /* Action name */
		GTK_STOCK_REFRESH,                            /* Stock icon, if any */
		N_("_Update"),                     /* Display label */
		NULL,                                     /* short-cut */
		N_("Sync your local copy with the Subversion tree"),                      /* Tooltip */
		G_CALLBACK (on_fm_subversion_update)    /* action callback */
	}
};

static void
value_added_fm_current_uri (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	AnjutaUI *ui;
	GtkAction *subversion_menu_action;
	const gchar *uri;
	GnomeVFSURI *subversion_uri = NULL;
	gchar *subversion_text_uri = NULL;
	gchar *subversion_dir;
	gchar *filename;
	GnomeVFSDirectoryHandle* handle;
	GnomeVFSFileInfo info;
	GnomeVFSResult result;
	
	uri = g_value_get_string (value);
	filename = gnome_vfs_get_local_path_from_uri (uri);
	g_return_if_fail (filename != NULL);

	Subversion *subversion = (Subversion*)plugin;
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	if (subversion->fm_current_filename)
		g_free (subversion->fm_current_filename);
	subversion->fm_current_filename = filename;
	
	/* Show popup menu if Subversion directory exists */
	subversion_menu_action = anjuta_ui_get_action (ui, "ActionGroupSubversion", "ActionPopupSubversion");
	
	/* If a directory is selected we check if it contains a "Subversion" directory,
	if it is a file we check if it's directory contains a "Subversion" directory */
	result = gnome_vfs_get_file_info(uri, &info, 
		GNOME_VFS_FILE_INFO_DEFAULT);
	if (result == GNOME_VFS_OK)
	{
		if (info.type == GNOME_VFS_FILE_TYPE_DIRECTORY)
		{
			subversion_dir = g_strconcat(uri, "/Subversion", NULL);
		}
		
		else
		{
			subversion_uri = gnome_vfs_uri_new (uri);
			subversion_text_uri = gnome_vfs_uri_extract_dirname(subversion_uri);
			subversion_dir = g_strconcat(subversion_text_uri, "/Subversion", NULL);
			g_free(subversion_text_uri);
			gnome_vfs_uri_unref(subversion_uri);
		}	
	}
	else
		return; /* Strange... */
	if (gnome_vfs_directory_open(&handle, subversion_dir, 
		GNOME_VFS_FILE_INFO_DEFAULT) == GNOME_VFS_OK) 
	{
		
		g_object_set (G_OBJECT (subversion_menu_action), "sensitive", TRUE, NULL);
	}
	else
	{
		g_object_set (G_OBJECT (subversion_menu_action), "sensitive", FALSE, NULL);
	}
	g_free (subversion_dir);
}

static void
value_removed_fm_current_uri (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	AnjutaUI *ui;
	GtkAction *action;
	
	Subversion *subversion = (Subversion*)plugin;
	
	if (subversion->fm_current_filename)
		g_free (subversion->fm_current_filename);
	subversion->fm_current_filename = NULL;

	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupSubversion", "ActionPopupSubversion");
	g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
	
}

static void
value_added_project_root_uri (AnjutaPlugin *plugin, const gchar *name,
							  const GValue *value, gpointer user_data)
{
	Subversion *bb_plugin;
	const gchar *root_uri;

	bb_plugin = (Subversion *) plugin;
	
	g_message ("Project added");
	
	if (bb_plugin->project_root_dir)
		g_free (bb_plugin->project_root_dir);
	bb_plugin->project_root_dir = NULL;
	
	root_uri = g_value_get_string (value);
	if (root_uri)
	{
		bb_plugin->project_root_dir =
			gnome_vfs_get_local_path_from_uri (root_uri);
		if (bb_plugin->project_root_dir)
		{
			// update_project_ui (bb_plugin);
		}
	}
}

static void
value_removed_project_root_uri (AnjutaPlugin *plugin, const gchar *name,
								gpointer user_data)
{
	Subversion *bb_plugin;

	bb_plugin = (Subversion *) plugin;
	if (bb_plugin->project_root_dir)
		g_free (bb_plugin->project_root_dir);
	bb_plugin->project_root_dir = NULL;
	// update_project_ui (bb_plugin);
}

static void
value_added_current_editor (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	AnjutaUI *ui;
	gchar *uri;
	GObject *editor;
	
	editor = g_value_get_object (value);
	
	Subversion *subversion = (Subversion*)plugin;
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	if (subversion->current_editor_filename)
		g_free (subversion->current_editor_filename);
	subversion->current_editor_filename = NULL;
	
	uri = ianjuta_file_get_uri (IANJUTA_FILE (editor), NULL);
	if (uri)
	{
		gchar *filename;
		
		filename = gnome_vfs_get_local_path_from_uri (uri);
		g_return_if_fail (filename != NULL);
		subversion->current_editor_filename = filename;
		g_free (uri);
		// update_module_ui (subversion);
	}
}

static void
value_removed_current_editor (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	Subversion *subversion = (Subversion*)plugin;
	
	if (subversion->current_editor_filename)
		g_free (subversion->current_editor_filename);
	subversion->current_editor_filename = NULL;
	
	// update_module_ui (subversion);
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	GladeXML* gxml;
	AnjutaPreferences *prefs;
	AnjutaUI *ui;
	Subversion *subversion;
	
	g_message ("Subversion: Activating Subversion plugin ...");
	subversion = (Subversion*) plugin;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	/* Create the messages preferences page */
	prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	gxml = glade_xml_new (GLADE_FILE, "subversion", NULL);
	anjuta_preferences_add_page (prefs, gxml, "subversion", ICON_FILE);
	g_object_unref (gxml);
	
	/* Add all our actions */
	anjuta_ui_add_action_group_entries (ui, "ActionGroupSubversion",
					_("Subversion operations"),
					actions_subversion,
					G_N_ELEMENTS (actions_subversion),
					plugin);
	subversion->uiid = anjuta_ui_merge (ui, UI_FILE);
	
	/* Add watches */
	subversion->fm_watch_id = 
		anjuta_plugin_add_watch (plugin, "file_manager_current_uri",
								 value_added_fm_current_uri,
								 value_removed_fm_current_uri, NULL);
	subversion->project_watch_id = 
		anjuta_plugin_add_watch (plugin, "project_root_uri",
								 value_added_project_root_uri,
								 value_removed_project_root_uri, NULL);
	subversion->editor_watch_id = 
		anjuta_plugin_add_watch (plugin, "document_manager_current_editor",
								 value_added_current_editor,
								 value_removed_current_editor, NULL);
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui = anjuta_shell_get_ui (plugin->shell, NULL);
	g_message ("Subversion: Dectivating Subversion plugin ...");
	anjuta_ui_unmerge (ui, ((Subversion*)plugin)->uiid);
	return TRUE;
}

static void
finalize (GObject *obj)
{
	// Subversion *plugin = (Subversion*)obj;
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (G_OBJECT(obj)));
}

static void
dispose (GObject *obj)
{
	// Subversion *plugin = (Subversion*)obj;
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (G_OBJECT(obj)));
}

static void
subversion_instance_init (GObject *obj)
{
	Subversion *plugin = (Subversion*)obj;
	plugin->uiid = 0;
	plugin->mesg_view = NULL;
	plugin->launcher = NULL;
	plugin->diff_editor = NULL;
	plugin->fm_current_filename = NULL;
	plugin->project_root_dir = NULL;
	plugin->current_editor_filename = NULL;
	plugin->backend = svn_backend_new(plugin);
}

static void
subversion_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
	klass->finalize = finalize;
}

/* Interface */

static void
ianjuta_subversion_add (IAnjutaVcs *obj, const gchar* filename, 
	GError **err)
{
	Subversion* plugin = (Subversion*)obj;
	svn_backend_add(plugin->backend, filename, FALSE, FALSE);
}
	
static void
ianjuta_subversion_commit (IAnjutaVcs *obj, const gchar* filename, const gchar* log, 
						 gboolean recurse, GError **err)
{
	Subversion* plugin = (Subversion*)obj;
	svn_backend_commit(plugin->backend, filename, log, recurse);
}

static void
ianjuta_subversion_remove (IAnjutaVcs *obj, const gchar* filename, GError **err)
{
	Subversion* plugin = (Subversion*)obj;
	svn_backend_remove(plugin->backend, filename, FALSE);
}


static void
ianjuta_subversion_update (IAnjutaVcs *obj, const gchar* filename, gboolean recurse, GError **err)
{
	Subversion* plugin = (Subversion*)obj;
	svn_backend_update(plugin->backend, filename, NULL, recurse);
}

static void
ianjuta_vcs_iface_init (IAnjutaVcsIface *iface)
{
	iface->add = ianjuta_subversion_add;
	iface->remove = ianjuta_subversion_remove;
	iface->update = ianjuta_subversion_update;
	iface->commit = ianjuta_subversion_commit;	
}

ANJUTA_PLUGIN_BEGIN (Subversion, subversion);
ANJUTA_PLUGIN_ADD_INTERFACE(ianjuta_vcs, IANJUTA_TYPE_VCS);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (Subversion, subversion);
