/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2000 Naba Kumar, Johannes Schmid

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

#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>

#include "plugin.h"
#include "cvs-actions.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-cvs.ui"
#define ICON_FILE "anjuta-cvs-plugin.png"

static gpointer parent_class;

static GtkActionEntry actions_cvs[] = {
	{
		"ActionMenuCVS",                       /* Action name */
		NULL,                            /* Stock icon, if any */
		N_("_CVS"),                     /* Display label */
		NULL,                                     /* short-cut */
		NULL,                      /* Tooltip */
		NULL
	},
	{
		"ActionCVSAdd",                       /* Action name */
		GTK_STOCK_ADD,                            /* Stock icon, if any */
		N_("_Add"),                     /* Display label */
		NULL,                                     /* short-cut */
		N_("Add a new file/directory to the CVS tree"),                      /* Tooltip */
		G_CALLBACK (on_cvs_add_activate)    /* action callback */
	},
	{
		"ActionCVSRemove",                       /* Action name */
		GTK_STOCK_REMOVE,                            /* Stock icon, if any */
		N_("_Remove"),                     /* Display label */
		NULL,                                     /* short-cut */
		N_("Remove a file/directory from CVS tree"),                      /* Tooltip */
		G_CALLBACK (on_cvs_remove_activate)    /* action callback */
	},
	{
		"ActionCVSCommit",                       /* Action name */
		GTK_STOCK_YES,                            /* Stock icon, if any */
		N_("_Commit"),                     /* Display label */
		NULL,                                     /* short-cut */
		N_("Commit your changes to the CVS tree"),                      /* Tooltip */
		G_CALLBACK (on_cvs_commit_activate)    /* action callback */
	},
	{
		"ActionCVSUpdate",                       /* Action name */
		GTK_STOCK_REFRESH,                            /* Stock icon, if any */
		N_("_Update"),                     /* Display label */
		NULL,                                     /* short-cut */
		N_("Sync your local copy with the CVS tree"),                      /* Tooltip */
		G_CALLBACK (on_cvs_update_activate)    /* action callback */
	},
	{
		"ActionCVSDiff",                       /* Action name */
		GTK_STOCK_ZOOM_100,                            /* Stock icon, if any */
		N_("_Diff"),                     /* Display label */
		NULL,                                     /* short-cut */
		N_("Show differences between your local copy and the tree"),                      /* Tooltip */
		G_CALLBACK (on_cvs_diff_activate)    /* action callback */
	},
	{
		"ActionCVSStatus",                       /* Action name */
		GTK_STOCK_ZOOM_100,                            /* Stock icon, if any */
		N_("_Show Status"),                     /* Display label */
		NULL,                                     /* short-cut */
		N_("Show the status of a file/directory"),                      /* Tooltip */
		G_CALLBACK (on_cvs_status_activate)    /* action callback */
	},
	{
		"ActionCVSLog",                       /* Action name */
		GTK_STOCK_ZOOM_100,                            /* Stock icon, if any */
		N_("_Show Log"),                     /* Display label */
		NULL,                                     /* short-cut */
		N_("Show the log of a file/directory"),                      /* Tooltip */
		G_CALLBACK (on_cvs_log_activate)    /* action callback */
	},
	{
		"ActionCVSImport",                       /* Action name */
		GTK_STOCK_ADD,                            /* Stock icon, if any */
		N_("_Import Tree"),                     /* Display label */
		NULL,                                     /* short-cut */
		N_("Import a new source tree to CVS"),                      /* Tooltip */
		G_CALLBACK (on_cvs_import_activate)    /* action callback */
	}
};

static void
value_added_fm_current_uri (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	AnjutaUI *ui;
	/* GtkAction *action; */
	const gchar *uri;
	gchar /* *dirname, */ *filename;
	/* gboolean makefile_exists, is_dir; */
	
	uri = g_value_get_string (value);
	filename = gnome_vfs_get_local_path_from_uri (uri);
	g_return_if_fail (filename != NULL);

	CVSPlugin *cvs_plugin = (CVSPlugin*)plugin;
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	if (cvs_plugin->fm_current_filename)
		g_free (cvs_plugin->fm_current_filename);
	cvs_plugin->fm_current_filename = filename;
	
	/*
	is_dir = g_file_test (filename, G_FILE_TEST_IS_DIR);
	if (is_dir)
		dirname = g_strdup (filename);
	else
		dirname = g_path_get_dirname (filename);
	makefile_exists = directory_has_makefile (dirname);
	g_free (dirname);
	
	if (!makefile_exists)
		return;
	
	action = anjuta_ui_get_action (ui, "ActionGroupBuild", "ActionPopupBuild");
	g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupBuild",
										"ActionPopupBuildCompile");
	if (is_dir)
		g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
	else
		g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
	*/
}

static void
value_removed_fm_current_uri (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	/* AnjutaUI *ui;
	GtkAction *action; */
	
	CVSPlugin *cvs_plugin = (CVSPlugin*)plugin;
	
	if (cvs_plugin->fm_current_filename)
		g_free (cvs_plugin->fm_current_filename);
	cvs_plugin->fm_current_filename = NULL;
	/*
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupBuild", "ActionPopupBuild");
	g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
	*/
}

static void
value_added_project_root_uri (AnjutaPlugin *plugin, const gchar *name,
							  const GValue *value, gpointer user_data)
{
	CVSPlugin *bb_plugin;
	const gchar *root_uri;

	bb_plugin = (CVSPlugin *) plugin;
	
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
	CVSPlugin *bb_plugin;

	bb_plugin = (CVSPlugin *) plugin;
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
	
	CVSPlugin *cvs_plugin = (CVSPlugin*)plugin;
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	if (cvs_plugin->current_editor_filename)
		g_free (cvs_plugin->current_editor_filename);
	cvs_plugin->current_editor_filename = NULL;
	
	uri = ianjuta_file_get_uri (IANJUTA_FILE (editor), NULL);
	if (uri)
	{
		gchar *filename;
		
		filename = gnome_vfs_get_local_path_from_uri (uri);
		g_return_if_fail (filename != NULL);
		cvs_plugin->current_editor_filename = filename;
		g_free (uri);
		// update_module_ui (cvs_plugin);
	}
}

static void
value_removed_current_editor (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	CVSPlugin *cvs_plugin = (CVSPlugin*)plugin;
	
	if (cvs_plugin->current_editor_filename)
		g_free (cvs_plugin->current_editor_filename);
	cvs_plugin->current_editor_filename = NULL;
	
	// update_module_ui (cvs_plugin);
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	GladeXML* gxml;
	AnjutaPreferences *prefs;
	AnjutaUI *ui;
	CVSPlugin *cvs_plugin;
	
	g_message ("CVSPlugin: Activating CVS plugin ...");
	cvs_plugin = (CVSPlugin*) plugin;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	/* Create the messages preferences page */
	prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	gxml = glade_xml_new (GLADE_FILE, "cvs", NULL);
	anjuta_preferences_add_page (prefs, gxml, "cvs", ICON_FILE);
	g_object_unref (gxml);
	
	/* Add all our actions */
	anjuta_ui_add_action_group_entries (ui, "ActionGroupCVS",
					_("CVS operations"),
					actions_cvs,
					G_N_ELEMENTS (actions_cvs),
					plugin);
	cvs_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);
	
	/* Add watches */
	cvs_plugin->fm_watch_id = 
		anjuta_plugin_add_watch (plugin, "file_manager_current_uri",
								 value_added_fm_current_uri,
								 value_removed_fm_current_uri, NULL);
	cvs_plugin->project_watch_id = 
		anjuta_plugin_add_watch (plugin, "project_root_uri",
								 value_added_project_root_uri,
								 value_removed_project_root_uri, NULL);
	cvs_plugin->editor_watch_id = 
		anjuta_plugin_add_watch (plugin, "document_manager_current_editor",
								 value_added_current_editor,
								 value_removed_current_editor, NULL);
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui = anjuta_shell_get_ui (plugin->shell, NULL);
	g_message ("CVSPlugin: Dectivating CVS plugin ...");
	anjuta_ui_unmerge (ui, ((CVSPlugin*)plugin)->uiid);
	return TRUE;
}

static void
finalize (GObject *obj)
{
	// CVSPlugin *plugin = (CVSPlugin*)obj;
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (G_OBJECT(obj)));
}

static void
dispose (GObject *obj)
{
	// CVSPlugin *plugin = (CVSPlugin*)obj;
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (G_OBJECT(obj)));
}

static void
cvs_plugin_instance_init (GObject *obj)
{
	CVSPlugin *plugin = (CVSPlugin*)obj;
	plugin->uiid = 0;
	plugin->executing_command = FALSE;
	plugin->mesg_view = NULL;
	plugin->launcher = NULL;
	plugin->diff_editor = NULL;
	plugin->fm_current_filename = NULL;
	plugin->project_root_dir = NULL;
	plugin->current_editor_filename = NULL;
}

static void
cvs_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
	klass->finalize = finalize;
}

ANJUTA_PLUGIN_BOILERPLATE (CVSPlugin, cvs_plugin);
ANJUTA_SIMPLE_PLUGIN (CVSPlugin, cvs_plugin);
