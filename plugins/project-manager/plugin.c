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
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-status.h>

#include <gbf/gbf-project-util.h>
#include <gbf/gbf-backend.h>

#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-project-manager.ui"
#define PREFS_GLADE PACKAGE_DATA_DIR"/glade/anjuta-project-manager-plugin.glade"
#define ICON_FILE "anjuta-project-manager-plugin.png"

static gpointer parent_class;

static void update_ui (ProjectManagerPlugin *plugin);
static gboolean uri_is_inside_project (ProjectManagerPlugin *plugin,
									   const gchar *uri);

static GtkWindow*
get_plugin_parent_window (ProjectManagerPlugin *plugin)
{
	GtkWindow *win;
	GtkWidget *toplevel;
	
	toplevel = gtk_widget_get_toplevel (plugin->scrolledwindow);
	if (toplevel && GTK_IS_WINDOW (toplevel))
		win = GTK_WINDOW (toplevel);
	else
		win = GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell);
	return win;
}

static GList *
find_missing_uris (GList *pre, GList *post)
{
	GHashTable *hash;
	GList *ret = NULL;
	GList *node;
	
	hash = g_hash_table_new (g_str_hash, g_str_equal);
	node = pre;
	while (node)
	{
		g_hash_table_insert (hash, node->data, node->data);
		node = g_list_next (node);
	}
	
	node = post;
	while (node)
	{
		if (g_hash_table_lookup (hash, node->data) == NULL)
			ret = g_list_prepend (ret, node->data);
		node = g_list_next (node);
	}
	g_hash_table_destroy (hash);
	return g_list_reverse (ret);
}

static void
update_operation_emit_signals (ProjectManagerPlugin *plugin, GList *pre,
							   GList *post)
{
	GList *missing_uris, *node;
	
	missing_uris = find_missing_uris (pre, post);
	node = missing_uris;
	while (node)
	{
		DEBUG_PRINT ("URI added emitting: %s", (char*)node->data);
		g_signal_emit_by_name (G_OBJECT (plugin), "element_added",
							   node->data);
		node = g_list_next (node);
	}
	g_list_free (missing_uris);
	
	missing_uris = find_missing_uris (post, pre);
	node = missing_uris;
	while (node)
	{
		DEBUG_PRINT ("URI removed emitting: %s", (char*)node->data);
		g_signal_emit_by_name (G_OBJECT (plugin), "element_removed",
							   node->data);
		node = g_list_next (node);
	}
	g_list_free (missing_uris);
}

static void
update_operation_end (ProjectManagerPlugin *plugin, gboolean emit_signal)
{
	if (emit_signal)
	{
		if (plugin->pre_update_sources)
		{
			GList *post_update_sources =
			ianjuta_project_manager_get_elements (IANJUTA_PROJECT_MANAGER (plugin),
												  IANJUTA_PROJECT_MANAGER_SOURCE,
												  NULL);
			update_operation_emit_signals (plugin, plugin->pre_update_sources,
										   post_update_sources);
			if (post_update_sources)
			{
				g_list_foreach (post_update_sources, (GFunc)g_free, NULL);
				g_list_free (post_update_sources);
			}
		}
		if (plugin->pre_update_targets)
		{
			GList *post_update_targets =
			ianjuta_project_manager_get_elements (IANJUTA_PROJECT_MANAGER (plugin),
												  IANJUTA_PROJECT_MANAGER_TARGET,
												  NULL);
			update_operation_emit_signals (plugin, plugin->pre_update_targets,
										   post_update_targets);
			if (post_update_targets)
			{
				g_list_foreach (post_update_targets, (GFunc)g_free, NULL);
				g_list_free (post_update_targets);
			}
		}
		if (plugin->pre_update_groups)
		{
			GList *post_update_groups =
			ianjuta_project_manager_get_elements (IANJUTA_PROJECT_MANAGER (plugin),
												  IANJUTA_PROJECT_MANAGER_GROUP,
												  NULL);
			update_operation_emit_signals (plugin, plugin->pre_update_groups,
										   post_update_groups);
			if (post_update_groups)
			{
				g_list_foreach (post_update_groups, (GFunc)g_free, NULL);
				g_list_free (post_update_groups);
			}
		}
	}
	if (plugin->pre_update_sources)
	{
		g_list_foreach (plugin->pre_update_sources, (GFunc)g_free, NULL);
		g_list_free (plugin->pre_update_sources);
		plugin->pre_update_sources = NULL;
	}
	if (plugin->pre_update_targets)
	{
		g_list_foreach (plugin->pre_update_targets, (GFunc)g_free, NULL);
		g_list_free (plugin->pre_update_targets);
		plugin->pre_update_targets = NULL;
	}
	if (plugin->pre_update_groups)
	{
		g_list_foreach (plugin->pre_update_groups, (GFunc)g_free, NULL);
		g_list_free (plugin->pre_update_groups);
		plugin->pre_update_groups = NULL;
	}
}

static void
update_operation_begin (ProjectManagerPlugin *plugin)
{
	update_operation_end (plugin, FALSE);
	plugin->pre_update_sources =
	ianjuta_project_manager_get_elements (IANJUTA_PROJECT_MANAGER (plugin),
										  IANJUTA_PROJECT_MANAGER_SOURCE,
										  NULL);
	plugin->pre_update_targets =
	ianjuta_project_manager_get_elements (IANJUTA_PROJECT_MANAGER (plugin),
										  IANJUTA_PROJECT_MANAGER_TARGET,
										  NULL);
	plugin->pre_update_groups =
	ianjuta_project_manager_get_elements (IANJUTA_PROJECT_MANAGER (plugin),
										  IANJUTA_PROJECT_MANAGER_GROUP,
										  NULL);
}

static gboolean
on_refresh_idle (gpointer user_data)
{
	ProjectManagerPlugin *plugin;
	AnjutaStatus *status;
	GError *err = NULL;
	
	plugin = (ProjectManagerPlugin *)user_data;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell, NULL);
	anjuta_status_push (status, "Refreshing symbol tree ...");
	anjuta_status_busy_push (status);
	
	gbf_project_refresh (GBF_PROJECT (plugin->project), &err);
	if (err)
	{
		anjuta_util_dialog_error (get_plugin_parent_window (plugin),
								  _("Failed to refresh project: %s"),
								  err->message);
		g_error_free (err);
	}
	anjuta_status_busy_pop (status);
	anjuta_status_pop (status);
	return FALSE;
}

static void
on_refresh (GtkAction *action, ProjectManagerPlugin *plugin)
{
	g_idle_add (on_refresh_idle, plugin);
}

static void
on_properties (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GtkWidget *win, *properties;
	
	properties = gbf_project_configure (plugin->project, NULL);
	if (properties)
	{
		win = gtk_dialog_new_with_buttons (_("Project properties"),
										   GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
										   GTK_DIALOG_DESTROY_WITH_PARENT,
										   GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL, NULL);
		g_signal_connect_swapped (win, "response",
								  G_CALLBACK (gtk_widget_destroy), win);
		
		gtk_container_add (GTK_CONTAINER (GTK_DIALOG(win)->vbox),
						   properties);
		gtk_window_set_default_size (GTK_WINDOW (win), 450, -1);
		gtk_widget_show (win);
	}
	else
	{
		anjuta_util_dialog_info (GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
								 _("No properties available for this target"));
	}
}

static void
on_add_group (GtkAction *action, ProjectManagerPlugin *plugin)
{
	gchar *group_uri, *default_group_uri = NULL;
	if (plugin->current_editor_uri)
		default_group_uri = g_path_get_dirname (plugin->current_editor_uri);
	else
		default_group_uri = g_strdup ("");
	group_uri = ianjuta_project_manager_add_group (IANJUTA_PROJECT_MANAGER (plugin),
												   "", default_group_uri,
												   NULL);
	g_free (group_uri);
	g_free (default_group_uri);
}

static void
on_add_target (GtkAction *action, ProjectManagerPlugin *plugin)
{
	gchar *target_uri, *default_group_uri = NULL;
	if (plugin->current_editor_uri)
		default_group_uri = g_path_get_dirname (plugin->current_editor_uri);
	else
		default_group_uri = g_strdup ("");
	target_uri = ianjuta_project_manager_add_target (IANJUTA_PROJECT_MANAGER (plugin),
													 "", default_group_uri,
													 NULL);
	g_free (target_uri);
	g_free (default_group_uri);
}

static void
on_add_source (GtkAction *action, ProjectManagerPlugin *plugin)
{
	gchar *source_uri, *default_group_uri = NULL, *default_source_uri;
	
	if (plugin->current_editor_uri)
	{
		default_group_uri = g_path_get_dirname (plugin->current_editor_uri);
		default_source_uri = plugin->current_editor_uri;
	}
	else
	{
		default_group_uri = g_strdup ("");
		default_source_uri = "";
	}
	source_uri = ianjuta_project_manager_add_source (IANJUTA_PROJECT_MANAGER (plugin),
													 default_source_uri,
													 default_group_uri, NULL);
	g_free (source_uri);
	g_free (default_group_uri);
}

static void
on_popup_properties (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GbfTreeData *data;
	const gchar *selected_target;
	GtkWidget *properties, *win;
	
	data = gbf_project_view_find_selected (GBF_PROJECT_VIEW (plugin->view),
										   GBF_TREE_NODE_TARGET);
	selected_target = NULL;
	if (data)
	{
		selected_target = data->id;
		properties = gbf_project_configure_target (plugin->project,
												   selected_target, NULL);
		if (properties)
		{
			win = gtk_dialog_new_with_buttons (_("Target properties"),
											   GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
											   GTK_DIALOG_DESTROY_WITH_PARENT,
											   GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL, NULL);
			g_signal_connect_swapped (win, "response",
									  G_CALLBACK (gtk_widget_destroy), win);
			
			gtk_container_add (GTK_CONTAINER (GTK_DIALOG(win)->vbox),
							   properties);
			gtk_window_set_default_size (GTK_WINDOW (win), 450, -1);
			gtk_widget_show (win);
		}
		else
		{
			anjuta_util_dialog_info (GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
									 _("No properties available for this target"));
		}
		return;
	}
	data = gbf_project_view_find_selected (GBF_PROJECT_VIEW (plugin->view),
										   GBF_TREE_NODE_GROUP);
	selected_target = NULL;
	if (data)
	{
		selected_target = data->id;
		properties = gbf_project_configure_group (plugin->project,
												  selected_target, NULL);
		if (properties)
		{
			win = gtk_dialog_new_with_buttons (_("Group properties"),
											   GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
											   GTK_DIALOG_DESTROY_WITH_PARENT,
											   _("Close"), GTK_RESPONSE_CANCEL, NULL);
			g_signal_connect_swapped (win, "response",
									  G_CALLBACK (gtk_widget_destroy), win);
			
			gtk_container_add (GTK_CONTAINER (GTK_DIALOG(win)->vbox),
							   properties);
			gtk_window_set_default_size (GTK_WINDOW (win), 450, -1);
			gtk_widget_show (win);
		}
		else
		{
			anjuta_util_dialog_info (GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
									 _("No properties available for this group"));
		}
		return;
	}
	/* FIXME: */
}

static void
on_popup_add_group (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GbfTreeData *data;
	const gchar *selected_group;
	gchar *group_id;
	
	update_operation_begin (plugin);
	data = gbf_project_view_find_selected (GBF_PROJECT_VIEW (plugin->view),
										   GBF_TREE_NODE_GROUP);
	selected_group = NULL;
	if (data)
		selected_group = data->id;
	group_id = gbf_project_util_new_group (plugin->model,
										   get_plugin_parent_window (plugin),
										   selected_group, NULL);
	if (data)
		gbf_tree_data_free (data);
	update_operation_end (plugin, TRUE);
	g_free (group_id);
}

static void
on_popup_add_target (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GbfTreeData *data;
	const gchar *selected_group;
	gchar *target_id;

	update_operation_begin (plugin);
	data = gbf_project_view_find_selected (GBF_PROJECT_VIEW (plugin->view),
										   GBF_TREE_NODE_GROUP);
	selected_group = NULL;
	if (data)
		selected_group = data->id;
	target_id = gbf_project_util_new_target (plugin->model,
											 get_plugin_parent_window (plugin),
											 selected_group, NULL);
	if (data)
		gbf_tree_data_free (data);
	update_operation_end (plugin, TRUE);
	g_free (target_id);
}

static void
on_popup_add_source (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GbfTreeData *data;
	const gchar *selected_target;
	gchar *source_id;
	
	update_operation_begin (plugin);
	data = gbf_project_view_find_selected (GBF_PROJECT_VIEW (plugin->view),
										   GBF_TREE_NODE_TARGET);
	selected_target = NULL;
	if (data)
		selected_target = data->id;
	source_id = gbf_project_util_add_source (plugin->model,
											 get_plugin_parent_window (plugin),
											 selected_target, NULL, NULL);
	if (data)
		gbf_tree_data_free (data);
	update_operation_end (plugin, TRUE);
	g_free (source_id);
}

static gboolean
confirm_removal (ProjectManagerPlugin *plugin, GbfTreeData *data)
{
	gboolean answer;
	gchar *mesg;

	switch (data->type)
	{
		case GBF_TREE_NODE_GROUP:
			mesg = _("%sGroup: %s\n\nThe group will not be deleted from file system.");
			break;
		case GBF_TREE_NODE_TARGET:
			mesg = _("%sTarget: %s");
			break;
		case GBF_TREE_NODE_TARGET_SOURCE:
			mesg = _("%sSource: %s\n\nThe source file will not be deleted from file system.");
			break;
		default:
			g_warning ("Unknow node");
			return FALSE;
	}
	answer =
		anjuta_util_dialog_boolean_question (get_plugin_parent_window (plugin),
											 mesg,
		_("Are you sure you want to remove the following from project?\n\n"),
											 data->name);
	return answer;
}

static void
on_popup_remove (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GbfTreeData *data;
	
	data = gbf_project_view_find_selected (GBF_PROJECT_VIEW (plugin->view),
										   GBF_TREE_NODE_TARGET_SOURCE);
	if (data == NULL)
	{
		data = gbf_project_view_find_selected (GBF_PROJECT_VIEW (plugin->view),
											   GBF_TREE_NODE_TARGET);
	}
	if (data == NULL)
	{
		data = gbf_project_view_find_selected (GBF_PROJECT_VIEW (plugin->view),
											   GBF_TREE_NODE_GROUP);
	}
	if (data)
	{
		if (confirm_removal (plugin, data))
		{
			GError *err = NULL;
			update_operation_begin (plugin);
			switch (data->type)
			{
				case GBF_TREE_NODE_GROUP:
					gbf_project_remove_group (plugin->project, data->id, &err);
					break;
				case GBF_TREE_NODE_TARGET:
					gbf_project_remove_target (plugin->project, data->id, &err);
					break;
				case GBF_TREE_NODE_TARGET_SOURCE:
					gbf_project_remove_source (plugin->project, data->id, &err);
					break;
				default:
					g_warning ("Should not reach here!!!");
			}
			update_operation_end (plugin, TRUE);
			if (err)
			{
				anjuta_util_dialog_error (get_plugin_parent_window (plugin),
										  _("Failed to remove '%s':\n%s"),
										  err->message);
				g_error_free (err);
			}
		}
		gbf_tree_data_free (data);
	}
}

static void
on_popup_add_to_project (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GtkWindow *win;
	GnomeVFSFileInfo info;
	GnomeVFSResult res;
	gchar *parent_directory, *filename;
	
	win = get_plugin_parent_window (plugin);
	res = gnome_vfs_get_file_info (plugin->fm_current_uri, &info,
								   GNOME_VFS_FILE_INFO_DEFAULT |
								   GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
	if (res == GNOME_VFS_OK)
	{
		parent_directory = g_path_get_dirname (plugin->fm_current_uri);
		if (!parent_directory)
			parent_directory = g_strdup ("");
		
		filename = g_path_get_basename (plugin->fm_current_uri);
		if (info.type == GNOME_VFS_FILE_TYPE_DIRECTORY)
		{
			gchar *group_id =
			ianjuta_project_manager_add_group (IANJUTA_PROJECT_MANAGER (plugin),
											   filename, parent_directory,
											   NULL);
			g_free (group_id);
		}
		else
		{
			gchar *source_id =
			ianjuta_project_manager_add_source (IANJUTA_PROJECT_MANAGER (plugin),
												plugin->fm_current_uri,
												parent_directory,
												NULL);
			g_free (source_id);
		}
		g_free (filename);
		g_free (parent_directory);
	}
	else
	{
		const gchar *mesg;
		
		if (res != GNOME_VFS_OK)
			mesg = gnome_vfs_result_to_string (res);
		else
			mesg = _("URI is link");
		anjuta_util_dialog_error (win,
								  _("Failed to retried URI info of %s: %s"),
								  plugin->fm_current_uri, mesg);
	}
}

static void
on_uri_activated (GtkWidget *widget, const gchar *uri,
				  ProjectManagerPlugin *plugin)
{
	IAnjutaFileLoader *loader;
	
	loader = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										 IAnjutaFileLoader, NULL);
	if (loader)
		ianjuta_file_loader_load (loader, uri, FALSE, NULL);
}

static void
on_target_activated (GtkWidget *widget, const gchar *target_id,
					 ProjectManagerPlugin *plugin)
{
	on_popup_properties (NULL, plugin);
}

static void
on_group_activated (GtkWidget *widget, const gchar *group_id,
					 ProjectManagerPlugin *plugin)
{
	on_popup_properties (NULL, plugin);
}

static GtkActionEntry pm_actions[] = 
{
	{
		"ActionMenuProject", NULL,
		N_("_Project"), NULL, NULL, NULL
	},
	{
		"ActionProjectProperties", GTK_STOCK_PROPERTIES,
		N_("_Properties"), NULL, N_("Project properties"),
		G_CALLBACK (on_properties)
	},
	{
		"ActionProjectRefresh", GTK_STOCK_REFRESH,
		N_("_Refresh"), NULL, N_("Refresh project manager tree"),
		G_CALLBACK (on_refresh)
	},
	{
		"ActionProjectAddGroup", GTK_STOCK_ADD,
		N_("Add _Group"), NULL, N_("Add a group to project"),
		G_CALLBACK (on_add_group)
	},
	{
		"ActionProjectAddTarget", GTK_STOCK_ADD,
		N_("Add _Target"), NULL, N_("Add a target to project"),
		G_CALLBACK (on_add_target)
	},
	{
		"ActionProjectAddSource", GTK_STOCK_ADD,
		N_("Add _Source File"), NULL, N_("Add a source file to project"),
		G_CALLBACK (on_add_source)
	}
};

static GtkActionEntry popup_actions[] = 
{
	{
		"ActionPopupProjectProperties", GTK_STOCK_PROPERTIES,
		N_("_Properties"), NULL, N_("Properties of group/target/source"),
		G_CALLBACK (on_popup_properties)
	},
	{
		"ActionPopupProjectAddToProject", GTK_STOCK_ADD,
		N_("_Add To Project"), NULL, N_("Add a source file to project"),
		G_CALLBACK (on_popup_add_to_project)
	},
	{
		"ActionPopupProjectAddGroup", GTK_STOCK_ADD,
		N_("Add _Group"), NULL, N_("Add a group to project"),
		G_CALLBACK (on_popup_add_group)
	},
	{
		"ActionPopupProjectAddTarget", GTK_STOCK_ADD,
		N_("Add _Target"), NULL, N_("Add a target to project"),
		G_CALLBACK (on_popup_add_target)
	},
	{
		"ActionPopupProjectAddSource", GTK_STOCK_ADD,
		N_("Add _Source File"), NULL, N_("Add a source file to project"),
		G_CALLBACK (on_popup_add_source)
	},
	{
		"ActionPopupProjectRemove", GTK_STOCK_REMOVE,
		N_("Re_move"), NULL, N_("Remove from project"),
		G_CALLBACK (on_popup_remove)
	}
};

static void
update_ui (ProjectManagerPlugin *plugin)
{
	AnjutaUI *ui;
	gint j;
	GtkAction *action;
			
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (plugin)->shell, NULL);
	for (j = 0; j < G_N_ELEMENTS (pm_actions); j++)
	{
		action = anjuta_ui_get_action (ui, "ActionGroupProjectManager",
									   pm_actions[j].name);
		if (pm_actions[j].callback)
		{
			g_object_set (G_OBJECT (action), "sensitive",
						  (plugin->project != NULL), NULL);
		}
	}
	for (j = 0; j < G_N_ELEMENTS (popup_actions); j++)
	{
		action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
									   popup_actions[j].name);
		if (popup_actions[j].callback)
		{
			g_object_set (G_OBJECT (action), "sensitive",
						  (plugin->project != NULL), NULL);
		}
	}
}

static void
on_treeview_selection_changed (GtkTreeSelection *sel,
							   ProjectManagerPlugin *plugin)
{
	AnjutaUI *ui;
	GtkAction *action;
	GbfTreeData *data;
	gchar *selected_uri;
	
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (plugin)->shell, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
								   "ActionPopupProjectAddGroup");
	g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
								   "ActionPopupProjectAddTarget");
	g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
								   "ActionPopupProjectAddSource");
	g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
								   "ActionPopupProjectRemove");
	g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
	
	data = gbf_project_view_find_selected (GBF_PROJECT_VIEW (plugin->view),
										   GBF_TREE_NODE_TARGET_SOURCE);
	if (data && data->type == GBF_TREE_NODE_TARGET_SOURCE)
	{
		action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
									   "ActionPopupProjectAddSource");
		g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
		action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
									   "ActionPopupProjectRemove");
		g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
		gbf_tree_data_free (data);
		goto finally;
	}
	
	gbf_tree_data_free (data);
	data = gbf_project_view_find_selected (GBF_PROJECT_VIEW (plugin->view),
										   GBF_TREE_NODE_TARGET);
	if (data && data->type == GBF_TREE_NODE_TARGET)
	{
		action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
									   "ActionPopupProjectAddSource");
		g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
		action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
									   "ActionPopupProjectRemove");
		g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
		gbf_tree_data_free (data);
		goto finally;
	}
	
	gbf_tree_data_free (data);
	data = gbf_project_view_find_selected (GBF_PROJECT_VIEW (plugin->view),
										   GBF_TREE_NODE_GROUP);
	if (data && data->type == GBF_TREE_NODE_GROUP)
	{
		action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
									   "ActionPopupProjectAddGroup");
		g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
		action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
									   "ActionPopupProjectAddTarget");
		g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
		action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
									   "ActionPopupProjectRemove");
		g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
		gbf_tree_data_free (data);
		goto finally;
	}
finally:
	selected_uri =
		ianjuta_project_manager_get_selected (IANJUTA_PROJECT_MANAGER (plugin),
											  NULL);
	if (selected_uri)
	{
		GValue *value;
		value = g_new0 (GValue, 1);
		g_value_init (value, G_TYPE_STRING);
		g_value_set_string (value, selected_uri);
		anjuta_shell_add_value (ANJUTA_PLUGIN(plugin)->shell,
								"project_manager_current_uri",
								value, NULL);
		g_signal_emit_by_name (G_OBJECT (plugin), "element_selected",
							   selected_uri);
		g_free (selected_uri);
	} else {
		anjuta_shell_remove_value (ANJUTA_PLUGIN(plugin)->shell,
								   "project_manager_current_uri", NULL);
	}
}

static gboolean
on_treeview_event  (GtkWidget *widget,
					 GdkEvent  *event,
					 ProjectManagerPlugin *plugin)
{
	AnjutaUI *ui;
	
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (plugin)->shell, NULL);
	if (event->type == GDK_BUTTON_PRESS) {
		GdkEventButton *e = (GdkEventButton *) event;
		if (e->button == 3) {
			GtkWidget *popup;
			popup = gtk_ui_manager_get_widget (GTK_UI_MANAGER (ui),
											   "/PopupProjectManager");
			g_return_val_if_fail (GTK_IS_WIDGET (popup), FALSE);
			gtk_menu_popup (GTK_MENU (popup),
					NULL, NULL, NULL, NULL,
					((GdkEventButton *) event)->button,
					((GdkEventButton *) event)->time);
			return TRUE;
		}
	}
	return FALSE;
}

#define REGISTER_ICON(icon, stock_id) \
	pixbuf = gdk_pixbuf_new_from_file (icon, NULL); \
	icon_set = gtk_icon_set_new_from_pixbuf (pixbuf); \
	gtk_icon_factory_add (icon_factory, stock_id, icon_set); \
	g_object_unref (pixbuf);

static void
register_stock_icons (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	GtkIconFactory *icon_factory;
	GtkIconSet *icon_set;
	GdkPixbuf *pixbuf;
	static gboolean registered = FALSE;

	if (registered)
		return;
	registered = TRUE;

	/* Register stock icons */
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	icon_factory = anjuta_ui_get_icon_factory (ui);
	REGISTER_ICON (PACKAGE_PIXMAPS_DIR"/"ICON_FILE,
				   "project-manager-plugin-icon");
}

static void
value_added_fm_current_uri (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	AnjutaUI *ui;
	GtkAction *action;
	const gchar *uri;
	ProjectManagerPlugin *pm_plugin;
	
	uri = g_value_get_string (value);

	pm_plugin = (ProjectManagerPlugin*)plugin;
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	if (pm_plugin->fm_current_uri)
		g_free (pm_plugin->fm_current_uri);
	pm_plugin->fm_current_uri = g_strdup (uri);
	
	action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
								   "ActionPopupProjectAddToProject");
	g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
}

static void
value_removed_fm_current_uri (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	AnjutaUI *ui;
	GtkAction *action;
	ProjectManagerPlugin *pm_plugin;

	pm_plugin = (ProjectManagerPlugin*)plugin;
	
	if (pm_plugin->fm_current_uri)
		g_free (pm_plugin->fm_current_uri);
	pm_plugin->fm_current_uri = NULL;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
								   "ActionPopupProjectAddToProject");
	g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
}

static void
value_added_current_editor (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	GObject *editor;
	ProjectManagerPlugin *pm_plugin;
	
	editor = g_value_get_object (value);
	pm_plugin = (ProjectManagerPlugin*)plugin;
	
	if (pm_plugin->current_editor_uri)
		g_free (pm_plugin->current_editor_uri);
	pm_plugin->current_editor_uri =
		ianjuta_file_get_uri (IANJUTA_FILE (editor), NULL);
}

static void
value_removed_current_editor (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	ProjectManagerPlugin *pm_plugin;
	
	pm_plugin = (ProjectManagerPlugin*)plugin;
	
	if (pm_plugin->current_editor_uri)
		g_free (pm_plugin->current_editor_uri);
	pm_plugin->current_editor_uri = NULL;
}

#if 0
static GtkWidget *
show_loading_progress (AnjutaPlugin *plugin)
{
	GtkWidget *win, *label;
	label = gtk_label_new (_("Loading project. Please wait ..."));
	win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_transient_for (GTK_WINDOW (win), GTK_WINDOW (plugin->shell));
	gtk_window_set_position (GTK_WINDOW (win), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_container_add (GTK_CONTAINER (win), label);
	gtk_container_set_border_width (GTK_CONTAINER (win), 20);
	gtk_widget_show_all (win);
	while (gtk_events_pending ())
		gtk_main_iteration ();
	return win;
}
#endif

static void
value_added_project_root_uri (AnjutaPlugin *plugin, const gchar *name,
							  const GValue *value, gpointer data)
{
	ProjectManagerPlugin *pm_plugin;
	AnjutaStatus *status;
	/* GtkWidget *progress_win; */
	gchar *dirname;
	const gchar *root_uri;
	GSList *l;
	GError *error = NULL;
	GbfBackend *backend = NULL;
	
	root_uri = g_value_get_string (value);
	
	pm_plugin = (ProjectManagerPlugin*)(plugin);
	/* progress_win = show_loading_progress (plugin); */
	
	dirname = gnome_vfs_get_local_path_from_uri (root_uri);
	
	g_return_if_fail (dirname != NULL);
	
	if (pm_plugin->project != NULL)
			g_object_unref (pm_plugin->project);
	
	DEBUG_PRINT ("initializing gbf backend...\n");
	gbf_backend_init ();
	for (l = gbf_backend_get_backends (); l; l = l->next) {
		backend = l->data;
		
		pm_plugin->project = gbf_backend_new_project (backend->id);
		if (pm_plugin->project)
		{
			if (gbf_project_probe (pm_plugin->project, dirname, NULL))
			{
				/* Backend found */
				break;
			}
			g_object_unref (pm_plugin->project);
			pm_plugin->project = NULL;
		}
		/*
		if (!strcmp (backend->id, "gbf-am:GbfAmProject"))
			break;
		*/
		backend = NULL;
	}
	
	if (!backend)
	{
		/* FIXME: Set err */
		g_warning ("no backend available for this project\n");
		g_free (dirname);
		return;
	}
	
	DEBUG_PRINT ("Creating new gbf-am project\n");
	
	/* pm_plugin->project = gbf_backend_new_project (backend->id); */
	if (!pm_plugin->project)
	{
		/* FIXME: Set err */
		g_warning ("project creation failed\n");
		g_free (dirname);
		return;
	}
	
	status = anjuta_shell_get_status (plugin->shell, NULL);
	anjuta_status_progress_add_ticks (status, 1);
	anjuta_status_push (status, _("Loading project: %s"), g_basename (dirname));
	anjuta_status_busy_push (status);
	
	DEBUG_PRINT ("loading project %s\n\n", dirname);
	/* FIXME: use the error parameter to determine if the project
	 * was loaded successfully */
	gbf_project_load (pm_plugin->project, dirname, &error);
	
	anjuta_status_progress_tick (status, NULL, _("Created project view ..."));
	
	if (error)
	{
		GtkWidget *toplevel;
		GtkWindow *win;
		
		toplevel = gtk_widget_get_toplevel (pm_plugin->scrolledwindow);
		if (toplevel && GTK_IS_WINDOW (toplevel))
			win = GTK_WINDOW (toplevel);
		else
			win = GTK_WINDOW (plugin->shell);
		
		anjuta_util_dialog_error (win, _("Failed to load project %s: %s"),
								  dirname, error->message);
		/* g_propagate_error (err, error); */
		g_object_unref (pm_plugin->project);
		pm_plugin->project = NULL;
		g_free (dirname);
		/* gtk_widget_destroy (progress_win); */
		anjuta_status_pop (status);
		anjuta_status_busy_pop (status);
		return;
	}
	g_object_set (G_OBJECT (pm_plugin->model), "project",
				  pm_plugin->project, NULL);
	
	update_ui (pm_plugin);
	anjuta_shell_present_widget (ANJUTA_PLUGIN (pm_plugin)->shell,
								 pm_plugin->scrolledwindow,
								 NULL);
	/* gtk_widget_destroy (progress_win); */
	
	anjuta_status_set_default (status, _("Project"), g_basename (dirname));
	anjuta_status_pop (status);
	anjuta_status_busy_pop (status);
	g_free (dirname);
}

static void
value_removed_project_root_uri (AnjutaPlugin *plugin, const gchar *name,
								gpointer data)
{
	ProjectManagerPlugin *pm_plugin;
	AnjutaStatus *status;
	
	pm_plugin = (ProjectManagerPlugin*)plugin;
	if (pm_plugin->project) {
		g_object_unref (pm_plugin->project);
		pm_plugin->project = NULL;
		g_object_set (G_OBJECT (pm_plugin->model), "project", NULL, NULL);
		update_ui (pm_plugin);
		status = anjuta_shell_get_status (plugin->shell, NULL);
		anjuta_status_set_default (status, _("Project"), NULL);
	}
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	GtkWidget *view, *scrolled_window;
	GbfProjectModel *model;
	static gboolean initialized = FALSE;
	GtkTreeSelection *selection;
	/* GladeXML *gxml; */
	ProjectManagerPlugin *pm_plugin;
	
	DEBUG_PRINT ("ProjectManagerPlugin: Activating Project Manager plugin ...");
	
	if (!initialized)
		register_stock_icons (plugin);
	
	pm_plugin = (ProjectManagerPlugin*) plugin;
	pm_plugin->ui = anjuta_shell_get_ui (plugin->shell, NULL);
	pm_plugin->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);

	/* create model & view and bind them */
	model = gbf_project_model_new (NULL);
	view = gbf_project_view_new ();
	
	gtk_tree_view_set_model (GTK_TREE_VIEW (view),
							 GTK_TREE_MODEL (model));
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
	g_signal_connect (view, "uri-activated",
					  G_CALLBACK (on_uri_activated), plugin);
	g_signal_connect (view, "target-selected",
					  G_CALLBACK (on_target_activated), plugin);
	g_signal_connect (view, "group-selected",
					  G_CALLBACK (on_group_activated), plugin);
	g_signal_connect (selection, "changed",
					  G_CALLBACK (on_treeview_selection_changed), plugin);
	g_signal_connect (view, "event",
					  G_CALLBACK (on_treeview_event), plugin);
	
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
										 GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (scrolled_window), view);
	gtk_widget_show (view);
	gtk_widget_show (scrolled_window);
	
	pm_plugin->scrolledwindow = scrolled_window;
	pm_plugin->view = view;
	pm_plugin->model = model;
	
	/* Action groups */
	pm_plugin->pm_action_group = 
		anjuta_ui_add_action_group_entries (pm_plugin->ui,
											"ActionGroupProjectManager",
											_("Project manager actions"),
											pm_actions,
											G_N_ELEMENTS(pm_actions),
											GETTEXT_PACKAGE, plugin);
	pm_plugin->popup_action_group = 
		anjuta_ui_add_action_group_entries (pm_plugin->ui,
											"ActionGroupProjectManagerPopup",
											_("Project manager popup actions"),
											popup_actions,
											G_N_ELEMENTS (popup_actions),
											GETTEXT_PACKAGE, plugin);
	/* Merge UI */
	pm_plugin->merge_id = 
		anjuta_ui_merge (pm_plugin->ui, UI_FILE);
	
	update_ui (pm_plugin);
	
	/* Added widget in shell */
	anjuta_shell_add_widget (plugin->shell, pm_plugin->scrolledwindow,
							 "AnjutaProjectManager", _("Project"),
							 "project-manager-plugin-icon",
							 ANJUTA_SHELL_PLACEMENT_LEFT, NULL);
#if 0
	/* Add preferences page */
	gxml = glade_xml_new (PREFS_GLADE, "dialog.project.manager", NULL);
	
	anjuta_preferences_add_page (pm_plugin->prefs,
								gxml, "Project Manager", ICON_FILE);
	preferences_changed(pm_plugin->prefs, pm_plugin);
	g_object_unref (G_OBJECT (gxml));
#endif
	
	/* Add watches */
	pm_plugin->fm_watch_id =
		anjuta_plugin_add_watch (plugin, "file_manager_current_uri",
								 value_added_fm_current_uri,
								 value_removed_fm_current_uri, NULL);
	pm_plugin->editor_watch_id = 
		anjuta_plugin_add_watch (plugin, "document_manager_current_editor",
								 value_added_current_editor,
								 value_removed_current_editor, NULL);
	pm_plugin->project_watch_id = 
		anjuta_plugin_add_watch (plugin, "project_root_uri",
								 value_added_project_root_uri,
								 value_removed_project_root_uri, NULL);
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	ProjectManagerPlugin *pm_plugin;
	pm_plugin = (ProjectManagerPlugin*) plugin;
	
	/* Remove watches */
	anjuta_plugin_remove_watch (plugin, pm_plugin->fm_watch_id, TRUE);
	anjuta_plugin_remove_watch (plugin, pm_plugin->editor_watch_id, TRUE);
	
	/* Project is also closed with this */
	anjuta_plugin_remove_watch (plugin, pm_plugin->project_watch_id, TRUE);
	
	g_object_unref (G_OBJECT (pm_plugin->model));
	
	/* Widget is removed from the shell when destroyed */
	gtk_widget_destroy (pm_plugin->scrolledwindow);
	
	anjuta_ui_unmerge (pm_plugin->ui, pm_plugin->merge_id);
	anjuta_ui_remove_action_group (pm_plugin->ui, pm_plugin->pm_action_group);
	anjuta_ui_remove_action_group (pm_plugin->ui, pm_plugin->popup_action_group);
	
	return TRUE;
}

static void
finalize (GObject *obj)
{
	/* FIXME: */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
dispose (GObject *obj)
{
	/* FIXME: */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
project_manager_plugin_instance_init (GObject *obj)
{
	ProjectManagerPlugin *plugin = (ProjectManagerPlugin*) obj;
	plugin->scrolledwindow = NULL;
	plugin->project = NULL;
	plugin->view = NULL;
	plugin->model = NULL;
	plugin->pre_update_sources = NULL;
	plugin->pre_update_targets = NULL;
	plugin->pre_update_groups = NULL;
}

static void
project_manager_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = finalize;
	klass->dispose = dispose;
}

/* IAnjutaProjectManager implementation */

static GnomeVFSFileType
get_uri_vfs_type (const gchar *uri)
{
	GnomeVFSFileInfo info;
	gnome_vfs_get_file_info (uri, &info, 0);
	return info.type;
}

static gboolean
uri_is_inside_project (ProjectManagerPlugin *plugin, const gchar *uri)
{
	const gchar *root_uri;

	/* DEBUG_PRINT ("Is '%s' inside project", uri); */
	anjuta_shell_get (ANJUTA_PLUGIN (plugin)->shell, "project_root_uri",
					  G_TYPE_STRING, &root_uri, NULL);

	if (strncmp (uri, root_uri, strlen (root_uri)) == 0)
		return TRUE;
	
	if (uri[0] == '/')
	{
		const gchar *project_root_path = strchr (root_uri, ':');
		if (project_root_path)
			project_root_path += 3;
		else
			project_root_path = root_uri;
		return (strncmp (uri, project_root_path, strlen (project_root_path)) == 0);
	}
	return FALSE;
}

static gchar *
get_element_uri_from_id (ProjectManagerPlugin *plugin, const gchar *id)
{
	gchar *path, *ptr;
	gchar *uri;
	const gchar *project_root;
	
	if (!id)
		return NULL;
	
	path = g_strdup (id);
	ptr = strrchr (path, ':');
	if (ptr)
	{
		if (ptr[1] == '/')
		{
			 /* ID is source ID, extract source uri */
			ptr = strrchr (ptr, ':');
			return g_strdup (ptr+1);
		}
		*ptr = '\0';
	}
	
	anjuta_shell_get (ANJUTA_PLUGIN (plugin)->shell,
					  "project_root_uri", G_TYPE_STRING,
					  &project_root, NULL);
	uri = g_build_filename (project_root, path, NULL);
	/* DEBUG_PRINT ("Converting id: %s to %s", id, uri); */
	g_free (path);
	return uri;
}

static const gchar *
get_element_relative_path (ProjectManagerPlugin *plugin, const gchar *uri)
{
	const gchar *project_root = NULL;
	
	anjuta_shell_get (ANJUTA_PLUGIN (plugin)->shell,
					  "project_root_uri", G_TYPE_STRING,
					  &project_root, NULL);
	if (project_root)
	{
		if (uri[0] != '/')
		{
			return uri + strlen (project_root);
		}
		else
		{
			const gchar *project_root_path = strchr (project_root, ':');
			if (project_root_path)
				project_root_path += 3;
			else
				project_root_path = project_root;
			return uri + strlen (project_root_path);
		}
	}
	return NULL;
}

static GbfProjectTarget*
get_target_from_uri (ProjectManagerPlugin *plugin, const gchar *uri)
{
	GbfProjectTarget *data;
	const gchar *rel_path;
	gchar *test_id;
	
	rel_path = get_element_relative_path (plugin, uri);
	
	if (!rel_path)
		return NULL;
	
	/* FIXME: More target types should be handled */
	/* Test for shared lib */
	test_id = g_strconcat (rel_path, ":shared_lib", NULL);
	data = gbf_project_get_target (GBF_PROJECT (plugin->project), test_id, NULL);
	g_free (test_id);
	
	if (!data)
	{
		/* Test for static lib */
		test_id = g_strconcat (rel_path, ":static_lib", NULL);
		data = gbf_project_get_target (GBF_PROJECT (plugin->project), test_id, NULL);
		g_free (test_id);
	}
	if (!data)
	{
		/* Test for program */
		test_id = g_strconcat (rel_path, ":program", NULL);
		data = gbf_project_get_target (GBF_PROJECT (plugin->project), test_id, NULL);
		g_free (test_id);
	}
	return data;
}

static gchar *
get_element_id_from_uri (ProjectManagerPlugin *plugin, const gchar *uri)
{
	GbfProjectTarget *target;
	gchar *id;
	
	if (!uri_is_inside_project (plugin, uri))
		return NULL;
	
	target = get_target_from_uri (plugin, uri);
	if (target)
	{
		id = g_strdup (target->id);
		gbf_project_target_free (target);
	}
	else if (get_uri_vfs_type (uri) | GNOME_VFS_FILE_TYPE_DIRECTORY)
	{
		id = g_strconcat (get_element_relative_path (plugin, uri), "/", NULL);
	}
	else
	{
		id = strdup (get_element_relative_path (plugin, uri));
	}
	return id;
}

static IAnjutaProjectManagerElementType
iproject_manager_get_element_type (IAnjutaProjectManager *project_manager,
								   const gchar *element_uri,
								   GError **err)
{
	GnomeVFSFileType ftype;
	ProjectManagerPlugin *plugin;
	
	g_return_val_if_fail (ANJUTA_IS_PLUGIN (project_manager),
						  IANJUTA_PROJECT_MANAGER_UNKNOWN);
	
	plugin = (ProjectManagerPlugin*) G_OBJECT (project_manager);
	g_return_val_if_fail (GBF_IS_PROJECT (plugin->project),
						  IANJUTA_PROJECT_MANAGER_UNKNOWN);
	g_return_val_if_fail (uri_is_inside_project (plugin, element_uri),
						  IANJUTA_PROJECT_MANAGER_UNKNOWN);
	
	ftype = get_uri_vfs_type (element_uri);
	if (ftype | GNOME_VFS_FILE_TYPE_DIRECTORY)
	{
		return IANJUTA_PROJECT_MANAGER_GROUP;
	}
	else if (ianjuta_project_manager_get_target_type (project_manager,
													  element_uri, NULL) !=
				IANJUTA_PROJECT_MANAGER_TARGET_UNKNOWN)
	{
		return IANJUTA_PROJECT_MANAGER_TARGET;
	}
	else if (ftype | GNOME_VFS_FILE_TYPE_REGULAR)
	{
		return IANJUTA_PROJECT_MANAGER_SOURCE;
	}
	return IANJUTA_PROJECT_MANAGER_UNKNOWN;
}

static GList*
iproject_manager_get_elements (IAnjutaProjectManager *project_manager,
							   IAnjutaProjectManagerElementType element_type,
							   GError **err)
{
	GList *elements;
	ProjectManagerPlugin *plugin;
	
	g_return_val_if_fail (ANJUTA_IS_PLUGIN (project_manager), NULL);
	
	plugin = (ProjectManagerPlugin*) G_OBJECT (project_manager);
	g_return_val_if_fail (GBF_IS_PROJECT (plugin->project), NULL);
	
	elements = NULL;
	switch (element_type)
	{
		case IANJUTA_PROJECT_MANAGER_SOURCE:
		{
			GList *sources, *node;
			GbfProjectTargetSource *source;
			sources = gbf_project_get_all_sources (plugin->project, NULL);
			node = sources;
			while (node)
			{
				source = gbf_project_get_source (plugin->project,
												 (const gchar *) node->data,
												 NULL);
				if (source)
					elements = g_list_prepend (elements,
											   g_strdup (source->source_uri));
				gbf_project_target_source_free (source);
				g_free (node->data);
				node = node->next;
			}
			g_list_free (sources);
			break;
		}
		case IANJUTA_PROJECT_MANAGER_TARGET:
		{
			GList *targets, *node;
			targets = gbf_project_get_all_targets (plugin->project, NULL);
			node = targets;
			while (node)
			{
				elements = g_list_prepend (elements,
										   get_element_uri_from_id (plugin,
																	(const gchar *)node->data));
				g_free (node->data);
				node = node->next;
			}
			g_list_free (targets);
			break;
		}
		case IANJUTA_PROJECT_MANAGER_GROUP:
		{
			GList *groups, *node;
			groups = gbf_project_get_all_groups (plugin->project, NULL);
			node = groups;
			while (node)
			{
				elements = g_list_prepend (elements,
										   get_element_uri_from_id (plugin,
																	(const gchar *)node->data));
				g_free (node->data);
				node = node->next;
			}
			g_list_free (groups);
			break;
		}
		default:
			elements = NULL;
	}
	return g_list_reverse (elements);
}

static IAnjutaProjectManagerTargetType
iproject_manager_get_target_type (IAnjutaProjectManager *project_manager,
								   const gchar *target_uri,
								   GError **err)
{
	IAnjutaProjectManagerElementType element_type;
	IAnjutaProjectManagerTargetType target_type;
	ProjectManagerPlugin *plugin;
	GbfProjectTarget *data;
	
	g_return_val_if_fail (ANJUTA_IS_PLUGIN (project_manager),
						  IANJUTA_PROJECT_MANAGER_TARGET_UNKNOWN);
	
	plugin = (ProjectManagerPlugin*) G_OBJECT (project_manager);
	g_return_val_if_fail (GBF_IS_PROJECT (plugin->project),
						  IANJUTA_PROJECT_MANAGER_TARGET_UNKNOWN);
	
	element_type = ianjuta_project_manager_get_element_type (project_manager,
															 target_uri, NULL);
	
	g_return_val_if_fail (uri_is_inside_project (plugin, target_uri),
						  IANJUTA_PROJECT_MANAGER_TARGET_UNKNOWN);
	
	data = get_target_from_uri (plugin, target_uri);
	
	if (data && data->type && strcmp (data->type, "shared_lib") == 0)
	{
		target_type = IANJUTA_PROJECT_MANAGER_TARGET_SHAREDLIB;
	}
	else if (data && data->type && strcmp (data->type, "static_lib") == 0)
	{
		target_type = IANJUTA_PROJECT_MANAGER_TARGET_STATICLIB;
	}
	else if (data && data->type && strcmp (data->type, "program") == 0)
	{
		target_type = IANJUTA_PROJECT_MANAGER_TARGET_STATICLIB;
	}
	else
	{
		target_type = IANJUTA_PROJECT_MANAGER_TARGET_UNKNOWN;
	}
	if (data)
		gbf_project_target_free (data);
	return target_type;
}

static GList*
iproject_manager_get_targets (IAnjutaProjectManager *project_manager,
							  IAnjutaProjectManagerTargetType target_type,
							  GError **err)
{
	GList *targets, *node;
	const gchar *target_id;
	GList *elements;
	ProjectManagerPlugin *plugin;
	const gchar *target_type_str;
	
	g_return_val_if_fail (ANJUTA_IS_PLUGIN (project_manager), NULL);
	
	plugin = (ProjectManagerPlugin*) G_OBJECT (project_manager);
	g_return_val_if_fail (GBF_IS_PROJECT (plugin->project), NULL);
	
	switch (target_type)
	{
		case IANJUTA_PROJECT_MANAGER_TARGET_SHAREDLIB:
			target_type_str = "shared_lib";
			break;
		case IANJUTA_PROJECT_MANAGER_TARGET_STATICLIB:
			target_type_str = "static_lib";
			break;
		case IANJUTA_PROJECT_MANAGER_TARGET_EXECUTABLE:
			target_type_str = "program";
			break;
		default:
			g_warning ("Unsupported target type");
			return NULL;
	}
	
	elements = NULL;
	targets = gbf_project_get_all_targets (plugin->project, NULL);
	node = targets;
	while (node)
	{
		const gchar *t_type;
		
		target_id = (const gchar*) node->data;
		
		t_type = strrchr (target_id, ':');
		if (t_type && strlen (t_type) > 2)
		{
			t_type++;
			if (strcmp (t_type, target_type_str) == 0)
			{
				gchar *target_uri = get_element_uri_from_id (plugin, target_id);
				elements = g_list_prepend (elements, target_uri);
			}
		}
		g_free (node->data);
		node = node->next;
	}
	g_list_free (targets);
	return g_list_reverse (elements);
}

static gchar*
iproject_manager_get_parent (IAnjutaProjectManager *project_manager,
							 const gchar *element_uri,
							 GError **err)
{
	IAnjutaProjectManagerElementType type;
	ProjectManagerPlugin *plugin;
	
	g_return_val_if_fail (ANJUTA_IS_PLUGIN (project_manager), NULL);
	
	plugin = (ProjectManagerPlugin*) G_OBJECT (project_manager);
	g_return_val_if_fail (GBF_IS_PROJECT (plugin->project), NULL);
	
	type = ianjuta_project_manager_get_element_type (project_manager,
													 element_uri, NULL);
	/* FIXME: */
	return NULL;
}

static GList*
iproject_manager_get_children (IAnjutaProjectManager *project_manager,
							   const gchar *element_uri,
							   GError **err)
{
	ProjectManagerPlugin *plugin;
	
	g_return_val_if_fail (ANJUTA_IS_PLUGIN (project_manager), NULL);
	
	plugin = (ProjectManagerPlugin*) G_OBJECT (project_manager);
	g_return_val_if_fail (GBF_IS_PROJECT (plugin->project), NULL);
	/* FIXME: */
	return NULL;
}

static gchar*
iproject_manager_get_selected (IAnjutaProjectManager *project_manager,
							   GError **err)
{
	gchar *uri;
	GbfTreeData *data;
	ProjectManagerPlugin *plugin;
	
	g_return_val_if_fail (ANJUTA_IS_PLUGIN (project_manager), NULL);
	
	plugin = (ProjectManagerPlugin*) G_OBJECT (project_manager);
	g_return_val_if_fail (GBF_IS_PROJECT (plugin->project), NULL);
	
	data = gbf_project_view_find_selected (GBF_PROJECT_VIEW (plugin->view),
										   GBF_TREE_NODE_TARGET_SOURCE);
	if (data && data->type == GBF_TREE_NODE_TARGET_SOURCE)
	{
		uri = g_strdup (data->uri);
		gbf_tree_data_free (data);
		return uri;
	}
	
	if (data)
		gbf_tree_data_free (data);

	data = gbf_project_view_find_selected (GBF_PROJECT_VIEW (plugin->view),
										   GBF_TREE_NODE_TARGET);
	if (data && data->type == GBF_TREE_NODE_TARGET)
	{
		uri = get_element_uri_from_id (plugin, data->id);
		gbf_tree_data_free (data);
		return uri;
	}
	
	if (data)
		gbf_tree_data_free (data);

	data = gbf_project_view_find_selected (GBF_PROJECT_VIEW (plugin->view),
										   GBF_TREE_NODE_GROUP);
	if (data && data->type == GBF_TREE_NODE_GROUP)
	{
		uri = g_strdup (data->uri);
		gbf_tree_data_free (data);
		return uri;;
	}

	if (data)
		gbf_tree_data_free (data);
	
	return NULL;
}

static gchar*
iproject_manager_add_source (IAnjutaProjectManager *project_manager,
							 const gchar *source_uri_to_add,
							 const gchar *default_location_uri,
							 GError **err)
{
	ProjectManagerPlugin *plugin;
	IAnjutaProjectManagerElementType default_location_type;
	gchar *source_id, *location_id;
	gchar *source_uri = NULL;
	
	g_return_val_if_fail (ANJUTA_IS_PLUGIN (project_manager), FALSE);
	
	plugin = (ProjectManagerPlugin*) G_OBJECT (project_manager);
	g_return_val_if_fail (GBF_IS_PROJECT (plugin->project), FALSE);

	update_operation_begin (plugin);
	default_location_type =
		ianjuta_project_manager_get_element_type (project_manager,
												  default_location_uri, NULL);
	location_id = get_element_id_from_uri (plugin, default_location_uri);
	if (default_location_type == IANJUTA_PROJECT_MANAGER_GROUP)
	{
		source_id = gbf_project_util_add_source (plugin->model,
												 get_plugin_parent_window (plugin),
												 NULL, location_id,
												 source_uri_to_add);
	}
	else if (default_location_type == IANJUTA_PROJECT_MANAGER_TARGET)
	{
		source_id = gbf_project_util_add_source (plugin->model,
												 get_plugin_parent_window (plugin),
												 location_id, NULL,
												 source_uri_to_add);
	}
	else
	{
		source_id = gbf_project_util_add_source (plugin->model,
												 get_plugin_parent_window (plugin),
												 NULL, NULL,
												 source_uri_to_add);
	}
	update_operation_end (plugin, TRUE);
	source_uri = get_element_uri_from_id (plugin, source_id);
	g_free (source_id);
	g_free (location_id);
	return source_uri;
}

static gchar*
iproject_manager_add_target (IAnjutaProjectManager *project_manager,
							 const gchar *target_name_to_add,
							 const gchar *default_group_uri,
							 GError **err)
{
	ProjectManagerPlugin *plugin;
	gchar *default_group_id, *target_id, *target_uri = NULL;
	
	g_return_val_if_fail (ANJUTA_IS_PLUGIN (project_manager), FALSE);
	
	plugin = (ProjectManagerPlugin*) G_OBJECT (project_manager);
	
	g_return_val_if_fail (GBF_IS_PROJECT (plugin->project), FALSE);

	default_group_id = get_element_id_from_uri (plugin, default_group_uri);
	
	update_operation_begin (plugin);
	target_id = gbf_project_util_new_target (plugin->model,
											 get_plugin_parent_window (plugin),
											 default_group_id,
											 target_name_to_add);
	update_operation_end (plugin, TRUE);
	target_uri = get_element_uri_from_id (plugin, target_id);
	g_free (target_id);
	g_free (default_group_id);
	return target_uri;
}

static gchar*
iproject_manager_add_group (IAnjutaProjectManager *project_manager,
							const gchar *group_name_to_add,
							const gchar *default_group_uri,
							GError **err)
{
	ProjectManagerPlugin *plugin;
	gchar *group_id, *group_uri = NULL;
	gchar *default_group_id;
	
	g_return_val_if_fail (ANJUTA_IS_PLUGIN (project_manager), FALSE);
	
	plugin = (ProjectManagerPlugin*) G_OBJECT (project_manager);
	g_return_val_if_fail (GBF_IS_PROJECT (plugin->project), FALSE);

	default_group_id = get_element_id_from_uri (plugin, default_group_uri);
	
	update_operation_begin (plugin);
	group_id = gbf_project_util_new_group (plugin->model,
										   get_plugin_parent_window (plugin),
										   default_group_id,
										   group_name_to_add);
	update_operation_end (plugin, TRUE);
	group_uri = get_element_uri_from_id (plugin, group_id);
	g_free (group_id);
	g_free (default_group_id);
	return group_uri;
}

static gboolean
iproject_manager_is_open (IAnjutaProjectManager *project_manager, GError **err)
{
	ProjectManagerPlugin *plugin;

	plugin = (ProjectManagerPlugin*) G_OBJECT (project_manager);

	return GBF_IS_PROJECT (plugin->project);
}

static void
iproject_manager_iface_init(IAnjutaProjectManagerIface *iface)
{
	iface->get_element_type = iproject_manager_get_element_type;
	iface->get_elements = iproject_manager_get_elements;
	iface->get_target_type = iproject_manager_get_target_type;
	iface->get_targets = iproject_manager_get_targets;
	iface->get_parent = iproject_manager_get_parent;
	iface->get_children = iproject_manager_get_children;
	iface->get_selected = iproject_manager_get_selected;
	iface->add_source = iproject_manager_add_source;
	iface->add_target = iproject_manager_add_target;
	iface->add_group = iproject_manager_add_group;
	iface->is_open = iproject_manager_is_open;
}

ANJUTA_PLUGIN_BEGIN (ProjectManagerPlugin, project_manager_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE (iproject_manager, IANJUTA_TYPE_PROJECT_MANAGER);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (ProjectManagerPlugin, project_manager_plugin);
