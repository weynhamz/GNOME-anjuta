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
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-file-manager.h>
#include <libanjuta/interfaces/ianjuta-builder.h>
#include <libanjuta/interfaces/ianjuta-project-backend.h>
#include <libanjuta/interfaces/ianjuta-project.h>
#include <libanjuta/anjuta-profile-manager.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-status.h>
#include <libanjuta/anjuta-project.h>

#include "project-util.h"
#include "dialogs.h"
#include "project-chooser.h"

#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR "/ui/anjuta-project-manager.xml"
#define PREFS_GLADE PACKAGE_DATA_DIR "/glade/anjuta-project-manager-plugin.ui"
#define ICON_FILE "anjuta-project-manager-plugin-48.png"
#define DEFAULT_PROFILE "file://"PACKAGE_DATA_DIR "/profiles/default.profile"
#define PROJECT_PROFILE_NAME "project"

#define INT_TO_GBOOLEAN(i) ((i) ? TRUE : FALSE)

typedef struct _PmPropertiesDialogInfo PmPropertiesDialogInfo;

typedef enum _PmPropertiesType
{
	PM_PROJECT_PROPERTIES,
	PM_TARGET_PROPERTIES,
	PM_GROUP_PROPERTIES
} PmPropertiesType;

struct _PmPropertiesDialogInfo
{
	PmPropertiesType type;
	AnjutaProjectNode* id;
	GtkWidget* dialog;
};

static gpointer parent_class;

static gboolean file_is_inside_project (ProjectManagerPlugin *plugin,
									   GFile *uri);
static void project_manager_plugin_close (ProjectManagerPlugin *plugin);

static void
update_title (ProjectManagerPlugin* plugin, const gchar *project_uri)
{
	AnjutaStatus *status;
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell, NULL);
	if (project_uri)
	{
		GFileInfo *file_info;
		GFile *file;

		file = g_file_new_for_uri (project_uri);
		file_info = g_file_query_info (file,
			G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
			G_FILE_QUERY_INFO_NONE,
			NULL,
			NULL);
		if (file_info)
		{
			gchar *dispname;
			gchar *ext;

			dispname = g_strdup (
				g_file_info_get_display_name (file_info));
			ext = strrchr (dispname, '.');
			if (ext)
				*ext = '\0';
			anjuta_status_set_title (status, dispname);
			g_free (dispname);
			g_object_unref (file_info);
		}

		g_object_unref (file);
	}
	else
	{
		anjuta_status_set_title (status, NULL);
	}
}

static gchar*
get_session_dir (ProjectManagerPlugin *plugin)
{
	gchar *session_dir = NULL;
	gchar *local_dir;

	g_return_val_if_fail (plugin->project_root_uri, NULL);

	local_dir = anjuta_util_get_local_path_from_uri (plugin->project_root_uri);
	if (local_dir)
	{
		session_dir = g_build_filename (local_dir, ".anjuta", "session",
										NULL);
	}
	g_free (local_dir);

	return session_dir;
}

static void
project_manager_save_session (ProjectManagerPlugin *plugin)
{
	gchar *session_dir;
	session_dir = get_session_dir (plugin);
	g_return_if_fail (session_dir != NULL);

	plugin->session_by_me = TRUE;
	anjuta_shell_session_save (ANJUTA_PLUGIN (plugin)->shell,
							   session_dir, NULL);
	plugin->session_by_me = FALSE;
	g_free (session_dir);
}

static void
on_session_save (AnjutaShell *shell, AnjutaSessionPhase phase,
				 AnjutaSession *session, ProjectManagerPlugin *plugin)
{
	GList *list;

	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;

	/*
	 * When a project session is being saved (session_by_me == TRUE),
	 * we should not save the current project uri, because project
	 * sessions are loaded when the project has already been loaded.
	 */
	if (plugin->project_file && !plugin->session_by_me)
	{
		list = anjuta_session_get_string_list (session,
												"File Loader",
												"Files");
		list = g_list_append (list, anjuta_session_get_relative_uri_from_file (session, plugin->project_file, NULL));
		anjuta_session_set_string_list (session, "File Loader",
										"Files", list);
		g_list_foreach (list, (GFunc)g_free, NULL);
		g_list_free (list);
	}

	/* Save shortcuts */
	list = gbf_project_view_get_shortcut_list (plugin->view);
	if (list != NULL)
	{
    	anjuta_session_set_string_list (session, "Project Manager", "Shortcut", list);
		g_list_foreach (list, (GFunc)g_free, NULL);
		g_list_free (list);
	}

	/* Save expanded node */
	list = gbf_project_view_get_expanded_list (GBF_PROJECT_VIEW (plugin->view));
	if (list != NULL)
	{
    	anjuta_session_set_string_list (session, "Project Manager", "Expand", list);
		g_list_foreach (list, (GFunc)g_free, NULL);
		g_list_free (list);
	}

}

static void
on_session_load (AnjutaShell *shell, AnjutaSessionPhase phase, AnjutaSession *session, ProjectManagerPlugin *plugin)
{
	GList *list;

	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;

	list = anjuta_session_get_string_list (session, "Project Manager", "Shortcut");
	gbf_project_view_set_shortcut_list (GBF_PROJECT_VIEW (plugin->view), list);
	g_list_foreach (list, (GFunc)g_free, NULL);
	g_list_free (list);

	list = anjuta_session_get_string_list (session, "Project Manager", "Expand");
	gbf_project_view_set_expanded_list (GBF_PROJECT_VIEW (plugin->view), list);
	g_list_foreach (list, (GFunc)g_free, NULL);
	g_list_free (list);
}

static void
on_shell_exiting (AnjutaShell *shell, ProjectManagerPlugin *plugin)
{
	if (plugin->project_file)
	{
		/* Also make sure we save the project session also */
		project_manager_save_session (plugin);
	}
}

static gboolean
on_close_project_idle (gpointer plugin)
{
	project_manager_plugin_close (ANJUTA_PLUGIN_PROJECT_MANAGER (plugin));
	ANJUTA_PLUGIN_PROJECT_MANAGER(plugin)->close_project_idle = -1;

	return FALSE;
}

static void
on_close_project (GtkAction *action, ProjectManagerPlugin *plugin)
{
	if (plugin->project_file)
		plugin->close_project_idle = g_idle_add (on_close_project_idle, plugin);
}

static GList *
find_missing_files (GList *pre, GList *post)
{
	GHashTable *hash;
	GList *ret = NULL;
	GList *node;

	hash = g_hash_table_new (g_file_hash, (GEqualFunc) g_file_equal);
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
	GList *missing_files, *node;

	missing_files = find_missing_files (pre, post);
	node = missing_files;
	while (node)
	{
		DEBUG_PRINT ("URI added emitting: %s", (char*)node->data);
		g_signal_emit_by_name (G_OBJECT (plugin), "element_added",
							   node->data);
		node = g_list_next (node);
	}
	g_list_free (missing_files);

	missing_files = find_missing_files (post, pre);
	node = missing_files;
	while (node)
	{
		DEBUG_PRINT ("URI removed emitting: %s", (char*)node->data);
		g_signal_emit_by_name (G_OBJECT (plugin), "element_removed",
							   node->data);
		node = g_list_next (node);
	}
	g_list_free (missing_files);
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
												  ANJUTA_PROJECT_SOURCE,
												  NULL);
			update_operation_emit_signals (plugin, plugin->pre_update_sources,
										   post_update_sources);
			if (post_update_sources)
			{
				g_list_foreach (post_update_sources, (GFunc)g_object_unref, NULL);
				g_list_free (post_update_sources);
			}
		}
		if (plugin->pre_update_targets)
		{
			GList *post_update_targets =
			ianjuta_project_manager_get_elements (IANJUTA_PROJECT_MANAGER (plugin),
												  ANJUTA_PROJECT_TARGET,
												  NULL);
			update_operation_emit_signals (plugin, plugin->pre_update_targets,
										   post_update_targets);
			if (post_update_targets)
			{
				g_list_foreach (post_update_targets, (GFunc)g_object_unref, NULL);
				g_list_free (post_update_targets);
			}
		}
		if (plugin->pre_update_groups)
		{
			GList *post_update_groups =
			ianjuta_project_manager_get_elements (IANJUTA_PROJECT_MANAGER (plugin),
												  ANJUTA_PROJECT_GROUP,
												  NULL);
			update_operation_emit_signals (plugin, plugin->pre_update_groups,
										   post_update_groups);
			if (post_update_groups)
			{
				g_list_foreach (post_update_groups, (GFunc)g_object_unref, NULL);
				g_list_free (post_update_groups);
			}
		}
	}
	if (plugin->pre_update_sources)
	{
		g_list_foreach (plugin->pre_update_sources, (GFunc)g_object_unref, NULL);
		g_list_free (plugin->pre_update_sources);
		plugin->pre_update_sources = NULL;
	}
	if (plugin->pre_update_targets)
	{
		g_list_foreach (plugin->pre_update_targets, (GFunc)g_object_unref, NULL);
		g_list_free (plugin->pre_update_targets);
		plugin->pre_update_targets = NULL;
	}
	if (plugin->pre_update_groups)
	{
		g_list_foreach (plugin->pre_update_groups, (GFunc)g_object_unref, NULL);
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
										  ANJUTA_PROJECT_SOURCE,
										  NULL);
	plugin->pre_update_targets =
	ianjuta_project_manager_get_elements (IANJUTA_PROJECT_MANAGER (plugin),
										  ANJUTA_PROJECT_TARGET,
										  NULL);
	plugin->pre_update_groups =
	ianjuta_project_manager_get_elements (IANJUTA_PROJECT_MANAGER (plugin),
										  ANJUTA_PROJECT_GROUP,
										  NULL);
}

GtkWindow*
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


/* GUI callbacks
 *---------------------------------------------------------------------------*/

static gboolean
on_refresh_idle (gpointer user_data)
{
	ProjectManagerPlugin *plugin;
	AnjutaStatus *status;
	GError *err = NULL;

	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (user_data);

	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell, NULL);
	anjuta_status_push (status, _("Refreshing symbol tree…"));
	anjuta_status_busy_push (status);

	anjuta_pm_project_refresh (plugin->project, &err);
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
	GtkTreeIter selected;
	gboolean found;

	found = gbf_project_view_get_first_selected (plugin->view, &selected) != NULL;

	anjuta_pm_project_show_properties_dialog (plugin, found ? &selected : NULL);
}

static void
on_new_group (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GFile *group;
	GFile *default_group = NULL;

	if (plugin->current_editor_uri)
	{
		gchar *uri = g_path_get_dirname (plugin->current_editor_uri);
		default_group = g_file_new_for_uri (uri);
		g_free (uri);
	}

	group =
		ianjuta_project_manager_add_group (IANJUTA_PROJECT_MANAGER (plugin),
										   "", default_group,
										   NULL);
	if (group != NULL) g_object_unref (group);
	if (default_group != NULL) g_object_unref (default_group);
}

static void
on_new_package (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GtkTreeIter selected_module;
	GList *new_module;
	GbfTreeData *data;

	update_operation_begin (plugin);
	data = gbf_project_view_get_first_selected (plugin->view, &selected_module);

	new_module = anjuta_pm_project_new_package (plugin,
										   get_plugin_parent_window (plugin),
										   data == NULL ? NULL : &selected_module, NULL);
	g_list_free (new_module);
	update_operation_end (plugin, TRUE);
}

static void
on_add_module (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GtkTreeIter selected_target;
	GList *new_modules;
	GbfTreeData *data;

	update_operation_begin (plugin);
	data = gbf_project_view_get_first_selected (plugin->view, &selected_target);

	new_modules = anjuta_pm_project_new_module (plugin,
										   get_plugin_parent_window (plugin),
										   data == NULL ? NULL : &selected_target, NULL);
	g_list_free (new_modules);
	update_operation_end (plugin, TRUE);
}

static void
on_new_target (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GFile *target;
	GFile *default_group = NULL;

	if (plugin->current_editor_uri)
	{
		gchar *uri = g_path_get_dirname (plugin->current_editor_uri);
		default_group = g_file_new_for_uri (uri);
		g_free (uri);
	}

	target =
		ianjuta_project_manager_add_target (IANJUTA_PROJECT_MANAGER (plugin),
											"", default_group,
											NULL);

	if (target != NULL) g_object_unref (target);
	if (default_group != NULL) g_object_unref (default_group);
}

static void
on_add_source (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GList *new_sources;
	GFile *default_source = NULL;
	GtkTreeIter selected;
	gboolean found;

	if (plugin->current_editor_uri)
	{
		default_source = g_file_new_for_uri (plugin->current_editor_uri);
	}
	found = gbf_project_view_get_first_selected (plugin->view, &selected) != NULL;
	update_operation_begin (plugin);
	new_sources = anjuta_pm_add_source_dialog (plugin,
	                                          get_plugin_parent_window (plugin),
	                                          found ? &selected : NULL,
	                                          default_source);
	update_operation_end (plugin, TRUE);
	g_list_free (new_sources);
	if (default_source) g_object_unref (default_source);
}

static void
on_popup_properties (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GtkTreeIter selected;
	gboolean found;

	/* FIXME: Perhaps it would be better to open a dialog for each
	 * selected node ? */
	found = gbf_project_view_get_first_selected (plugin->view, &selected) != NULL;

	anjuta_pm_project_show_properties_dialog (plugin, found ? &selected : NULL);
}

static void
on_popup_sort_shortcuts (GtkAction *action, ProjectManagerPlugin *plugin)
{
	gbf_project_view_sort_shortcuts (plugin->view);
}

static void
on_popup_new_package (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GtkTreeIter selected_module;
	GList *packages;

	update_operation_begin (plugin);
	gbf_project_view_get_first_selected (plugin->view, &selected_module);

	packages = anjuta_pm_project_new_package (plugin,
										   get_plugin_parent_window (plugin),
										   &selected_module, NULL);
	update_operation_end (plugin, TRUE);
}

static void
on_popup_add_module (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GtkTreeIter selected_target;
	GList *new_modules;

	update_operation_begin (plugin);
	gbf_project_view_get_first_selected (plugin->view, &selected_target);

	new_modules = anjuta_pm_project_new_module (plugin,
										   get_plugin_parent_window (plugin),
										   &selected_target, NULL);
	g_list_free (new_modules);
	update_operation_end (plugin, TRUE);
}

static void
on_popup_new_group (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GtkTreeIter selected_group;
	AnjutaProjectNode *new_group;

	update_operation_begin (plugin);
	gbf_project_view_get_first_selected (plugin->view, &selected_group);

	new_group = anjuta_pm_project_new_group (plugin,
										   get_plugin_parent_window (plugin),
										   &selected_group, NULL);
	update_operation_end (plugin, TRUE);
}

static void
on_popup_new_target (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GtkTreeIter selected_group;
	AnjutaProjectNode *new_target;

	update_operation_begin (plugin);
	gbf_project_view_get_first_selected (plugin->view, &selected_group);

	new_target = anjuta_pm_project_new_target (plugin,
											 get_plugin_parent_window (plugin),
											 &selected_group, NULL);

	update_operation_end (plugin, TRUE);
}

static void
on_popup_add_source (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GList *new_sources;
	GFile *default_source = NULL;
	GtkTreeIter selected;
	gboolean found;

	if (plugin->current_editor_uri)
	{
		default_source = g_file_new_for_uri (plugin->current_editor_uri);
	}
	found = gbf_project_view_get_first_selected (plugin->view, &selected) != NULL;
	update_operation_begin (plugin);
	new_sources = anjuta_pm_add_source_dialog (plugin,
	                                          get_plugin_parent_window (plugin),
	                                          found ? &selected : NULL,
	                                          default_source);

	update_operation_end (plugin, TRUE);
	g_list_free (new_sources);
	if (default_source) g_object_unref (default_source);
}

static gboolean
confirm_removal (ProjectManagerPlugin *plugin, GList *selected)
{
	gboolean answer;
	GString* mesg;
	GList *item;
	GbfTreeNodeType type;
	gboolean group = FALSE;
	gboolean remove_group_file = FALSE;
	gboolean source = FALSE;
	gboolean remove_source_file = FALSE;

	g_return_val_if_fail (selected != NULL, FALSE);

	type = ((GbfTreeData *)selected->data)->type;
	for (item = g_list_first (selected); item != NULL; item = g_list_next (item))
	{
		GbfTreeData *data = (GbfTreeData *)item->data;
		AnjutaProjectNode *node;

		if (data->type == GBF_TREE_NODE_GROUP)
		{
			group = TRUE;
			node = gbf_tree_data_get_node (data);
			remove_group_file = anjuta_project_node_get_state (node) & ANJUTA_PROJECT_REMOVE_FILE;
		}
		if (data->type == GBF_TREE_NODE_SOURCE)
		{
			source = TRUE;
			node = gbf_tree_data_get_node (data);
			remove_source_file = anjuta_project_node_get_state (node) & ANJUTA_PROJECT_REMOVE_FILE;
		}
		if (type != data->type) type = GBF_TREE_NODE_UNKNOWN;
	}

	switch (type)
	{
	case GBF_TREE_NODE_GROUP:
		mesg = g_string_new (_("Are you sure you want to remove the following group from the project?\n\n"));
		break;
	case GBF_TREE_NODE_TARGET:
		mesg = g_string_new (_("Are you sure you want to remove the following target from the project?\n\n"));
		break;
	case GBF_TREE_NODE_SOURCE:
		mesg = g_string_new (_("Are you sure you want to remove the following source file from the project?\n\n"));
		break;
	case GBF_TREE_NODE_PACKAGE:
		mesg = g_string_new (_("Are you sure you want to remove the following package from the project?\n\n"));
		break;
	case GBF_TREE_NODE_MODULE:
		mesg = g_string_new (_("Are you sure you want to remove the following module from the project?\n\n"));
		break;
	case GBF_TREE_NODE_UNKNOWN:
		mesg = g_string_new (_("Are you sure you want to remove the following elements from the project?\n\n"));
		break;
	case GBF_TREE_NODE_SHORTCUT:
		/* Remove shortcut without confirmation */
		return TRUE;
	default:
		g_warn_if_reached ();
		return FALSE;
	}

	for (item = g_list_first (selected); item != NULL; item = g_list_next (item))
	{
		GbfTreeData *data = (GbfTreeData *)item->data;

		switch (data->type)
		{
		case GBF_TREE_NODE_GROUP:
			g_string_append_printf (mesg, _("Group: %s\n"), data->name);
			break;
		case GBF_TREE_NODE_TARGET:
			g_string_append_printf (mesg, _("Target: %s\n"), data->name);
			break;
		case GBF_TREE_NODE_SOURCE:
			g_string_append_printf (mesg, _("Source: %s\n"), data->name);
			break;
		case GBF_TREE_NODE_SHORTCUT:
			g_string_append_printf (mesg, _("Shortcut: %s\n"), data->name);
			return TRUE;
		case GBF_TREE_NODE_MODULE:
			g_string_append_printf (mesg, _("Module: %s\n"), data->name);
			break;
		case GBF_TREE_NODE_PACKAGE:
			g_string_append_printf (mesg, _("Package: %s\n"), data->name);
			break;
		default:
			g_warn_if_reached ();
			return FALSE;
		}
	}

	if (group || source)
	{
		g_string_append (mesg, "\n");
		if (remove_group_file)
			g_string_append (mesg, _("The group will be deleted from the file system."));
		else if (group)
			g_string_append (mesg, _("The group will not be deleted from the file system."));
		if (remove_source_file)
			g_string_append (mesg, _("The source file will be deleted from the file system."));
		else if (source)
			g_string_append (mesg, _("The source file will not be deleted from the file system."));
	}

	answer =
		anjuta_util_dialog_boolean_question (get_plugin_parent_window (plugin),
											 mesg->str, _("Confirm remove"));
	g_string_free (mesg, TRUE);

	return answer;
}

static void
on_popup_remove (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GList *selected;

	selected = gbf_project_view_get_all_selected (plugin->view);

	if (selected != NULL)
	{
		if (confirm_removal (plugin, selected))
		{
			GError *err = NULL;
			GList *item;
			gboolean update = FALSE;

			for (item = g_list_first (selected); item != NULL; item = g_list_next (item))
			{
				GbfTreeData *data = (GbfTreeData *)(item->data);
				AnjutaProjectNode *node;

				switch (data->type)
				{
				case GBF_TREE_NODE_GROUP:
				case GBF_TREE_NODE_TARGET:
				case GBF_TREE_NODE_SOURCE:
				case GBF_TREE_NODE_MODULE:
				case GBF_TREE_NODE_PACKAGE:
					node = gbf_tree_data_get_node (data);
					if (node != NULL)
					{
						if (!update) update_operation_begin (plugin);
						anjuta_pm_project_remove (plugin->project, node, &err);
						if (err)
						{
							const gchar *name;

							update_operation_end (plugin, TRUE);
							update = FALSE;
							name = anjuta_project_node_get_name (node);
							anjuta_util_dialog_error (get_plugin_parent_window (plugin),
										  _("Failed to remove '%s':\n%s"),
										  name, err->message);
							g_error_free (err);
						}
					}
					break;
				case GBF_TREE_NODE_SHORTCUT:
					gbf_project_view_remove_data (plugin->view, data, NULL);
					break;
				default:
					break;
				}
			}
			if (update) update_operation_end (plugin, TRUE);
		}
		g_list_free (selected);
	}
}

static void
on_popup_add_to_project (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GtkWindow *win;
	GFile *file;
	GFileInfo *file_info;
	GFile *parent_directory;
	gchar *filename;
	GError *error = NULL;

	win = get_plugin_parent_window (plugin);

	file = g_file_new_for_uri (plugin->fm_current_uri);
	file_info = g_file_query_info (file,
	                               G_FILE_ATTRIBUTE_STANDARD_TYPE,
								   G_FILE_QUERY_INFO_NONE,
								   NULL,
								   &error);
	if (file_info != NULL)
	{
		parent_directory = g_file_get_parent (file);

		filename = g_file_get_basename (file);
		if (g_file_info_get_file_type (file_info) == G_FILE_TYPE_DIRECTORY)
		{
			GFile *new_file =
			ianjuta_project_manager_add_group (IANJUTA_PROJECT_MANAGER (plugin),
											   filename, parent_directory,
											   NULL);
			g_object_unref (new_file);
		}
		else
		{
			GFile *new_file =
				ianjuta_project_manager_add_source (IANJUTA_PROJECT_MANAGER
													(plugin),
													plugin->fm_current_uri,
													parent_directory,
													NULL);
			g_object_unref (new_file);
		}
		g_object_unref (file_info);
		g_free (filename);
		g_object_unref (parent_directory);
	}
	else
	{
		anjuta_util_dialog_error (win, _("Failed to retrieve URI info of %s: %s"),
					  plugin->fm_current_uri, error->message);
		g_error_free (error);
	}
}

static void
on_node_selected (GtkWidget *widget, AnjutaProjectNode *node,
				  ProjectManagerPlugin *plugin)
{
	IAnjutaFileLoader *loader;

	switch (anjuta_project_node_get_node_type (node))
	{
	case ANJUTA_PROJECT_GROUP:
	case ANJUTA_PROJECT_ROOT:
	case ANJUTA_PROJECT_TARGET:
	case ANJUTA_PROJECT_MODULE:
	case ANJUTA_PROJECT_PACKAGE:
		on_popup_properties (NULL, plugin);
		break;
	case ANJUTA_PROJECT_SOURCE:
		loader = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
											 IAnjutaFileLoader, NULL);
		if (loader)
			ianjuta_file_loader_load (loader, anjuta_project_node_get_file (node), FALSE, NULL);
		break;
	default:
		break;
	}
}

static GtkActionEntry pm_actions[] =
{
	{
		"ActionMenuProject", NULL,
		N_("_Project"), NULL, NULL, NULL
	},
	{
		"ActionProjectNewFolder", GTK_STOCK_ADD,
		N_("New _Folder…"), NULL, N_("Add a new folder to the project"),
		G_CALLBACK (on_new_group)
	},
	{
		"ActionProjectNewTarget", GTK_STOCK_ADD,
		N_("New _Target…"), NULL, N_("Add a new target to the project"),
		G_CALLBACK (on_new_target)
	},
	{
		"ActionProjectAddSource", GTK_STOCK_ADD,
		N_("Add _Source File…"), NULL, N_("Add a source file to a target"),
		G_CALLBACK (on_add_source)
	},
	{
		"ActionProjectAddLibrary", GTK_STOCK_ADD,
		N_("Add _Library…"), NULL, N_("Add a module to a target"),
		G_CALLBACK (on_add_module)
	},
	{
		"ActionProjectNewLibrary", GTK_STOCK_ADD,
		N_("New _Library…"), NULL, N_("Add a new package to the project"),
		G_CALLBACK (on_new_package)
	},
	{
		"ActionProjectProperties", GTK_STOCK_PROPERTIES,
		N_("_Properties"), NULL, N_("Project properties"),
		G_CALLBACK (on_properties)
	},
	{
		"ActionFileCloseProject", NULL,
		N_("Close Pro_ject"), NULL, N_("Close project"),
		G_CALLBACK (on_close_project)
	},
};

static GtkActionEntry popup_actions[] =
{
	{
		"ActionPopupProjectNewFolder", GTK_STOCK_ADD,
		N_("New _Folder"), NULL, N_("Add a new folder to the project"),
		G_CALLBACK (on_popup_new_group)
	},
	{
		"ActionPopupProjectNewTarget", GTK_STOCK_ADD,
		N_("New _Target"), NULL, N_("Add a new target to the project"),
		G_CALLBACK (on_popup_new_target)
	},
	{
		"ActionPopupProjectAddSource", GTK_STOCK_ADD,
		N_("Add _Source File"), NULL, N_("Add a source file to a target"),
		G_CALLBACK (on_popup_add_source)
	},
	{
		"ActionPopupProjectAddLibrary", GTK_STOCK_ADD,
		N_("Add _Library"), NULL, N_("Add a library to a target"),
		G_CALLBACK (on_popup_add_module)
	},
	{
		"ActionPopupProjectNewLibrary", GTK_STOCK_ADD,
		N_("New _Library"), NULL, N_("Add a new library to the project"),
		G_CALLBACK (on_popup_new_package)
	},
	{
		"ActionPopupProjectAddToProject", GTK_STOCK_ADD,
		N_("_Add to Project"), NULL, N_("Add a source file to a target"),
		G_CALLBACK (on_popup_add_to_project)
	},
	{
		"ActionPopupProjectProperties", GTK_STOCK_PROPERTIES,
		N_("_Properties"), NULL, N_("Properties of group/target/source"),
		G_CALLBACK (on_popup_properties)
	},
	{
		"ActionPopupProjectRemove", GTK_STOCK_REMOVE,
		N_("Re_move"), NULL, N_("Remove from project"),
		G_CALLBACK (on_popup_remove)
	},
	{
		"ActionPopupProjectSortShortcut", GTK_STOCK_SORT_ASCENDING,
		N_("_Sort"), NULL, N_("Sort shortcuts"),
		G_CALLBACK (on_popup_sort_shortcuts)
	}
};

static void
update_ui (ProjectManagerPlugin *plugin)
{
	AnjutaUI *ui;
	gint j;
	gint caps;
	gint main_caps;
	gint popup_caps;

	/* Close project is always here */
	main_caps = 0x101;
	popup_caps = 0x100;

	/* Check for supported node */
	caps = anjuta_pm_project_get_capabilities (plugin->project);
	if (caps != 0)
	{
		if (caps & ANJUTA_PROJECT_CAN_ADD_GROUP)
		{
			main_caps |= 0x2;
			popup_caps |= 0x21;
		}
		if (caps & ANJUTA_PROJECT_CAN_ADD_TARGET)
		{
			main_caps |= 0x4;
			popup_caps |= 0x2;
		}
		if (caps & ANJUTA_PROJECT_CAN_ADD_SOURCE)
		{
			main_caps |= 0x8;
			popup_caps |= 0x24;
		}
		if (caps & ANJUTA_PROJECT_CAN_ADD_MODULE)
		{
			main_caps |= 0x10;
			popup_caps |= 0x8;
		}
		if (caps & ANJUTA_PROJECT_CAN_ADD_PACKAGE)
		{
			main_caps |= 0x20;
			popup_caps |= 0x10;
		}
		/* Keep remove if a project is opened */
		popup_caps |= 0x080;
	}
	/* Keep properties and refresh if a project is opened */
	main_caps |= 0x0C0;
	/* Keep properties and remove if a project is opened */
	popup_caps |= 0x040;

	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (plugin)->shell, NULL);

	/* Main menu */
	for (j = 0; j < G_N_ELEMENTS (pm_actions); j++)
	{
		GtkAction *action;

		action = anjuta_ui_get_action (ui, "ActionGroupProjectManager",
										pm_actions[j].name);
		g_object_set (G_OBJECT (action), "visible", INT_TO_GBOOLEAN (main_caps & 0x1), NULL);
		main_caps >>= 1;
	}

	/* Popup menu */
	for (j = 0; j < G_N_ELEMENTS (popup_actions); j++)
	{
		GtkAction *action;

		action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
									popup_actions[j].name);
		g_object_set (G_OBJECT (action), "visible", INT_TO_GBOOLEAN (popup_caps & 0x1), NULL);
		popup_caps >>= 1;
	}
}

static void
on_treeview_selection_changed (GtkTreeSelection *sel,
							   ProjectManagerPlugin *plugin)
{
	AnjutaUI *ui;
	GtkAction *action;
	AnjutaProjectNode *node;
	gint state = 0;
	GFile *selected_file;
	GbfTreeData *data;

	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (plugin)->shell, NULL);
	node = gbf_project_view_find_selected (plugin->view,
										   ANJUTA_PROJECT_UNKNOWN);
	data = gbf_project_view_get_first_selected (plugin->view, NULL);

	if (node != NULL)
	{
		AnjutaProjectNode *parent;

		state = anjuta_project_node_get_state (node);
		/* Allow to select a sibling instead of a parent node */
		parent = anjuta_project_node_parent (node);
		if (parent != NULL)
		{
			state |= anjuta_project_node_get_state (parent);
		}
	}

	/* Popup menu */
	action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
								   "ActionPopupProjectNewFolder");
	g_object_set (G_OBJECT (action), "sensitive", INT_TO_GBOOLEAN (state & ANJUTA_PROJECT_CAN_ADD_GROUP), NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
								   "ActionPopupProjectNewTarget");
	g_object_set (G_OBJECT (action), "sensitive", INT_TO_GBOOLEAN (state & ANJUTA_PROJECT_CAN_ADD_TARGET), NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
								   "ActionPopupProjectAddSource");
	g_object_set (G_OBJECT (action), "sensitive", INT_TO_GBOOLEAN (state & ANJUTA_PROJECT_CAN_ADD_SOURCE), NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
								   "ActionPopupProjectAddLibrary");
	g_object_set (G_OBJECT (action), "sensitive", INT_TO_GBOOLEAN (state & ANJUTA_PROJECT_CAN_ADD_MODULE), NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
								   "ActionPopupProjectNewLibrary");
	g_object_set (G_OBJECT (action), "sensitive", INT_TO_GBOOLEAN (state & ANJUTA_PROJECT_CAN_ADD_PACKAGE), NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
								   "ActionPopupProjectRemove");
	g_object_set (G_OBJECT (action), "sensitive", INT_TO_GBOOLEAN (state & ANJUTA_PROJECT_CAN_REMOVE), NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
								   "ActionPopupProjectSortShortcut");
	g_object_set (G_OBJECT (action), "sensitive", (data != NULL) && (data->type == GBF_TREE_NODE_SHORTCUT), NULL);

	selected_file = node != NULL ? anjuta_project_node_get_file (node) : NULL;
	if (selected_file)
	{
		GValue *value;
		gchar *uri = g_file_get_uri (selected_file);

		value = g_new0 (GValue, 1);
		g_value_init (value, G_TYPE_STRING);
		g_value_set_string (value, uri);
		anjuta_shell_add_value (ANJUTA_PLUGIN(plugin)->shell,
								IANJUTA_PROJECT_MANAGER_CURRENT_URI,
								value, NULL);
		g_signal_emit_by_name (G_OBJECT (plugin), "element_selected",
							   selected_file);
		g_free (uri);
	} else {
		anjuta_shell_remove_value (ANJUTA_PLUGIN(plugin)->shell,
								   IANJUTA_PROJECT_MANAGER_CURRENT_URI, NULL);
	}
}

static gboolean
on_treeview_popup_menu  (GtkWidget *widget,
					 	ProjectManagerPlugin *plugin)
{
	AnjutaUI *ui;
	GtkWidget *popup;

	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (plugin)->shell, NULL);
	popup = gtk_ui_manager_get_widget (GTK_UI_MANAGER (ui), "/PopupProjectManager");
	g_return_val_if_fail (GTK_IS_WIDGET (popup), FALSE);

	gtk_menu_popup (GTK_MENU (popup),
					NULL, NULL, NULL, NULL,
					0,
					gtk_get_current_event_time());

	return TRUE;
}

static gboolean
on_treeview_button_press_event (GtkWidget             *widget,
				 GdkEventButton        *event,
				 ProjectManagerPlugin *plugin)
{
	if (event->button == 3)
	{
		GtkTreePath *path;
		GtkTreeSelection *selection;
		AnjutaUI *ui;
		GtkWidget *popup;

        if (!gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget),
                        event->x,event->y, &path, NULL, NULL, NULL))
			return FALSE;

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (widget));
        if (!gtk_tree_selection_path_is_selected(selection, path))
        {
            gtk_tree_selection_unselect_all(selection);
            gtk_tree_selection_select_path(selection, path);
            gtk_tree_view_set_cursor(GTK_TREE_VIEW(widget),
                                path, NULL, FALSE);
        }
        gtk_tree_path_free (path);

		ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (plugin)->shell, NULL);
		popup = gtk_ui_manager_get_widget (GTK_UI_MANAGER (ui), "/PopupProjectManager");
		g_return_val_if_fail (GTK_IS_WIDGET (popup), FALSE);

		gtk_menu_popup (GTK_MENU (popup),
						NULL, NULL, NULL, NULL,
						((GdkEventButton *) event)->button,
						((GdkEventButton *) event)->time);

        return TRUE;
    }

	return FALSE;
}

/*
 *---------------------------------------------------------------------------*/

static void
register_stock_icons (AnjutaPlugin *plugin)
{
	static gboolean registered = FALSE;

	if (registered)
		return;
	registered = TRUE;

	/* Register stock icons */
	BEGIN_REGISTER_ICON(plugin);
	REGISTER_ICON (ICON_FILE,
				   "project-manager-plugin-icon");
	END_REGISTER_ICON;
}

static void
value_added_fm_current_file (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	AnjutaUI *ui;
	GtkAction *action;
	gchar *uri;
	ProjectManagerPlugin *pm_plugin;

	GFile* file = g_value_get_object (value);
	uri = g_file_get_uri (file);

	pm_plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (plugin);
	ui = anjuta_shell_get_ui (plugin->shell, NULL);

	if (pm_plugin->fm_current_uri)
		g_free (pm_plugin->fm_current_uri);
	pm_plugin->fm_current_uri = g_strdup (uri);

	action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
								   "ActionPopupProjectAddToProject");
	g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
	g_free (uri);
}

static void
value_removed_fm_current_file (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	AnjutaUI *ui;
	GtkAction *action;
	ProjectManagerPlugin *pm_plugin;

	pm_plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (plugin);

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
	GFile* file;

	editor = g_value_get_object (value);
	if (!IANJUTA_IS_EDITOR(editor))
		return;

	pm_plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (plugin);

	if (pm_plugin->current_editor_uri)
		g_free (pm_plugin->current_editor_uri);
	file = ianjuta_file_get_file (IANJUTA_FILE (editor), NULL);
	if (file)
	{
		pm_plugin->current_editor_uri = g_file_get_uri (file);
		g_object_unref (file);
	}
	else
		pm_plugin->current_editor_uri = NULL;

}

static void
value_removed_current_editor (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	ProjectManagerPlugin *pm_plugin;

	pm_plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (plugin);

	if (pm_plugin->current_editor_uri)
		g_free (pm_plugin->current_editor_uri);
	pm_plugin->current_editor_uri = NULL;
}

static void
on_project_loaded (AnjutaPmProject *project, GtkTreeIter *parent, gboolean complete, GError *error, ProjectManagerPlugin *plugin)
{
	AnjutaStatus *status;
	gchar *dirname;

	dirname = anjuta_util_get_local_path_from_uri (plugin->project_root_uri);
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell, NULL);
	if (error)
	{
		if (complete)
		{
			GtkWidget *toplevel;
			GtkWindow *win;

			toplevel = gtk_widget_get_toplevel (plugin->scrolledwindow);
			if (toplevel && GTK_IS_WINDOW (toplevel))
				win = GTK_WINDOW (toplevel);
			else
				win = GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell);

			anjuta_util_dialog_error (win, _("Failed to parse project (the project is opened, but there will be no project view) %s: %s\n"
										 ""),
									  dirname, error->message);
		}
	}

	if (complete)
	{
		gchar *basename = g_path_get_basename (dirname);

		anjuta_status_progress_tick (status, NULL, _("Update project view…"));
		update_ui (plugin);
		anjuta_shell_present_widget (ANJUTA_PLUGIN (plugin)->shell,
									plugin->scrolledwindow,
									NULL);
		anjuta_status_set_default (status, _("Project"), basename);
		g_free (basename);

		if (plugin->busy)
		{
			anjuta_status_pop (status);
			anjuta_status_busy_pop (status);
			plugin->busy = FALSE;
		}

		/* Emit loaded signal for other plugins */
		g_signal_emit_by_name (G_OBJECT (plugin), "project_loaded", error);

	}

	g_free (dirname);
}

static void
project_manager_load_gbf (ProjectManagerPlugin *pm_plugin)
{
	AnjutaStatus *status;
	gchar *dirname;
	GFile *dirfile;
	gchar *basename;
	const gchar *root_uri;
	GError *error = NULL;

	root_uri = pm_plugin->project_root_uri;

	dirname = anjuta_util_get_local_path_from_uri (root_uri);
	dirfile = g_file_new_for_uri (root_uri);

	g_return_if_fail (dirname != NULL);

	status = anjuta_shell_get_status (ANJUTA_PLUGIN (pm_plugin)->shell, NULL);
	anjuta_status_progress_add_ticks (status, 1);
	basename = g_path_get_basename (dirname);
	anjuta_status_push (status, _("Loading project: %s"), basename);
	anjuta_status_busy_push (status);
	pm_plugin->busy = TRUE;

	anjuta_pm_project_unload (pm_plugin->project, NULL);

	DEBUG_PRINT ("loading project %s\n\n", dirname);
	anjuta_pm_project_load (pm_plugin->project, dirfile, &error);
	update_ui (pm_plugin);

	g_free (basename);
	g_free (dirname);
	g_object_unref (dirfile);
}

static void
project_manager_unload_gbf (ProjectManagerPlugin *pm_plugin)
{
	AnjutaStatus *status;

	if (anjuta_pm_project_is_open (pm_plugin->project))
	{
		IAnjutaDocumentManager *docman;

		/* Close files that belong to this project (that are saved) */
		docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (pm_plugin)->shell,
											 IAnjutaDocumentManager, NULL);
		if (docman)
		{
			GList *to_remove = NULL;
			GList *editors;
			GList *node;

			editors =
				ianjuta_document_manager_get_doc_widgets (docman, NULL);
			node = editors;
			while (node)
			{
				if (!IANJUTA_IS_EDITOR(node->data))
				{
					node = g_list_next(node);
					continue;
				}
				GFile* editor_file = ianjuta_file_get_file (IANJUTA_FILE (node->data), NULL);
				gchar *editor_uri = g_file_get_uri (editor_file);
				g_object_unref (editor_file);


				/* Only remove if it does not have unsaved data */
				if (editor_uri && (!IANJUTA_IS_FILE_SAVABLE (node->data) ||
								   !ianjuta_file_savable_is_dirty
										(IANJUTA_FILE_SAVABLE (node->data),
										 NULL)))
				{
					if (strncmp (editor_uri, pm_plugin->project_root_uri,
								 strlen (pm_plugin->project_root_uri)) == 0 &&
						editor_uri[strlen (pm_plugin->project_root_uri)] == '/')
					{
						to_remove = g_list_prepend (to_remove, node->data);
					}
				}
				g_free (editor_uri);
				node = g_list_next (node);
			}
			node = to_remove;
			while (node)
			{
				ianjuta_document_manager_remove_document (docman,
												IANJUTA_DOCUMENT (node->data),
														FALSE,
														NULL);
				node = g_list_next (node);
			}
			if (editors)
				g_list_free (editors);
			if (to_remove)
				g_list_free (to_remove);
		}

		/* Release project */
		anjuta_pm_project_unload (pm_plugin->project, NULL);
		update_ui (pm_plugin);
		status = anjuta_shell_get_status (ANJUTA_PLUGIN (pm_plugin)->shell,
										  NULL);
		anjuta_status_set_default (status, _("Project"), NULL);
	}
}

static void
on_profile_scoped (AnjutaProfileManager *profile_manager,
				   AnjutaProfile *profile, ProjectManagerPlugin *plugin)
{
	gchar *session_dir;
	DEBUG_PRINT ("Profile scoped: %s", anjuta_profile_get_name (profile));
	if (strcmp (anjuta_profile_get_name (profile), PROJECT_PROFILE_NAME) != 0)
		return;

	DEBUG_PRINT ("%s", "Project profile loaded; Restoring project session");

	/* Load gbf project */
	project_manager_load_gbf (plugin);


	update_title (plugin, plugin->project_root_uri);

	/* If profile scoped to "project", restore project session */
	session_dir = get_session_dir (plugin);
	g_return_if_fail (session_dir != NULL);

	/*
	 * If there is a session load already in progress (that is this
	 * project is being opened in session restoration), our session
	 * load would be ignored. Good thing.
	 */
	plugin->session_by_me = TRUE;
	anjuta_shell_session_load (ANJUTA_PLUGIN (plugin)->shell,
							   session_dir, NULL);
	plugin->session_by_me = FALSE;
	g_free (session_dir);
}

static void
on_profile_descoped (AnjutaProfileManager *profile_manager,
					 AnjutaProfile *profile, ProjectManagerPlugin *plugin)
{
	DEBUG_PRINT ("Profile descoped: %s", anjuta_profile_get_name (profile));

	if (strcmp (anjuta_profile_get_name (profile), PROJECT_PROFILE_NAME) != 0)
		return;

	DEBUG_PRINT ("%s", "Project profile descoped; Saving project session");

	/* Save current project session */
	g_return_if_fail (plugin->project_root_uri != NULL);

	/* Save project session */
	project_manager_save_session (plugin);

	/* Close current project */
	project_manager_unload_gbf (plugin);

	g_free (plugin->project_root_uri);
	if (plugin->project_file) g_object_unref (plugin->project_file);
	plugin->project_file = NULL;
	plugin->project_root_uri = NULL;

	update_title (ANJUTA_PLUGIN_PROJECT_MANAGER (plugin), NULL);
	anjuta_shell_remove_value (ANJUTA_PLUGIN (plugin)->shell,
						  IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI, NULL);
}

static void
project_manager_plugin_close (ProjectManagerPlugin *plugin)
{
	AnjutaProfileManager *profile_manager;
	GError *error = NULL;

	/* Remove project profile */
	profile_manager =
		anjuta_shell_get_profile_manager (ANJUTA_PLUGIN (plugin)->shell, NULL);
	anjuta_profile_manager_pop (profile_manager, PROJECT_PROFILE_NAME, &error);
	if (error)
	{
		anjuta_util_dialog_error (get_plugin_parent_window (plugin),
								  _("Error closing project: %s"),
								  error->message);
		g_error_free (error);
	}
}

static gboolean
project_manager_plugin_activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaProfileManager *profile_manager;
	GtkWidget *scrolled_window;
	GtkWidget *view;
	static gboolean initialized = FALSE;
	GtkTreeSelection *selection;
	/* GladeXML *gxml; */
	ProjectManagerPlugin *pm_plugin;

	DEBUG_PRINT ("ProjectManagerPlugin: Activating Project Manager plugin %p…", plugin);

	if (!initialized)
		register_stock_icons (plugin);

	pm_plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (plugin);
	pm_plugin->ui = anjuta_shell_get_ui (plugin->shell, NULL);
	pm_plugin->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);

	/* Create project */
	pm_plugin->project = anjuta_pm_project_new (plugin);

	/* create model & view and bind them */
	view = gbf_project_view_new ();

	/* Add project to view */
	gbf_project_view_set_project (GBF_PROJECT_VIEW (view), pm_plugin->project);
	g_signal_connect (view, "node-loaded", G_CALLBACK (on_project_loaded), plugin);

	/*gtk_tree_view_set_model (GTK_TREE_VIEW (view),
							 GTK_TREE_MODEL (anjuta_pm_project_get_model (pm_plugin->project)));*/

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
	g_signal_connect (view, "node-selected",
					  G_CALLBACK (on_node_selected), plugin);
	g_signal_connect (selection, "changed",
					  G_CALLBACK (on_treeview_selection_changed), plugin);
	g_signal_connect (view, "button-press-event",
		    		  G_CALLBACK (on_treeview_button_press_event), plugin);
	g_signal_connect (view, "popup-menu",
		    		  G_CALLBACK (on_treeview_popup_menu), plugin);

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
	pm_plugin->view = GBF_PROJECT_VIEW (view);

	/* Action groups */
	pm_plugin->pm_action_group =
		anjuta_ui_add_action_group_entries (pm_plugin->ui,
											"ActionGroupProjectManager",
											_("Project manager actions"),
											pm_actions,
											G_N_ELEMENTS(pm_actions),
											GETTEXT_PACKAGE, TRUE,
											plugin);
	pm_plugin->popup_action_group =
		anjuta_ui_add_action_group_entries (pm_plugin->ui,
											"ActionGroupProjectManagerPopup",
											_("Project manager popup actions"),
											popup_actions,
											G_N_ELEMENTS (popup_actions),
											GETTEXT_PACKAGE, TRUE,
											plugin);
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
		anjuta_plugin_add_watch (plugin, IANJUTA_FILE_MANAGER_SELECTED_FILE,
								 value_added_fm_current_file,
								 value_removed_fm_current_file, NULL);
	pm_plugin->editor_watch_id =
		anjuta_plugin_add_watch (plugin, IANJUTA_DOCUMENT_MANAGER_CURRENT_DOCUMENT,
								 value_added_current_editor,
								 value_removed_current_editor, NULL);
	/* Connect to save session */
	g_signal_connect (G_OBJECT (plugin->shell), "save_session",
					  G_CALLBACK (on_session_save), plugin);
	g_signal_connect (G_OBJECT (plugin->shell), "load_session",
					  G_CALLBACK (on_session_load), plugin);
	g_signal_connect (G_OBJECT (plugin->shell), "exiting",
					  G_CALLBACK (on_shell_exiting), plugin);
	profile_manager = anjuta_shell_get_profile_manager (plugin->shell, NULL);

	/* Connect to profile scoping */
	g_signal_connect (profile_manager, "profile-scoped",
					  G_CALLBACK (on_profile_scoped), plugin);
	g_signal_connect (profile_manager, "profile-descoped",
					  G_CALLBACK (on_profile_descoped), plugin);
	return TRUE;
}

static gboolean
project_manager_plugin_deactivate_plugin (AnjutaPlugin *plugin)
{
	AnjutaProfileManager *profile_manager;
	ProjectManagerPlugin *pm_plugin;
	pm_plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (plugin);

	DEBUG_PRINT ("ProjectManagerPlugin: Deactivate Project Manager plugin…");
	if (pm_plugin->close_project_idle > -1)
	{
		g_source_remove (pm_plugin->close_project_idle);
	}

	/* Close project if it's open */
	if (pm_plugin->project_root_uri)
		project_manager_plugin_close (pm_plugin);

	profile_manager = anjuta_shell_get_profile_manager (plugin->shell, NULL);

	/* Disconnect signals */
	g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->shell),
										  G_CALLBACK (on_session_save),
										  plugin);
	g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->shell),
										  G_CALLBACK (on_session_load),
										  plugin);
	g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->shell),
										  G_CALLBACK (on_shell_exiting),
										  plugin);
	g_signal_handlers_disconnect_by_func (G_OBJECT (profile_manager),
										  G_CALLBACK (on_profile_descoped),
										  plugin);
	g_signal_handlers_disconnect_by_func (G_OBJECT (profile_manager),
										  G_CALLBACK (on_profile_scoped),
										  plugin);
	/* Remove watches */
	anjuta_plugin_remove_watch (plugin, pm_plugin->fm_watch_id, TRUE);
	anjuta_plugin_remove_watch (plugin, pm_plugin->editor_watch_id, TRUE);

	/* Widget is removed from the shell when destroyed */
	gtk_widget_destroy (pm_plugin->scrolledwindow);

	anjuta_ui_unmerge (pm_plugin->ui, pm_plugin->merge_id);
	anjuta_ui_remove_action_group (pm_plugin->ui, pm_plugin->pm_action_group);
	anjuta_ui_remove_action_group (pm_plugin->ui,
								   pm_plugin->popup_action_group);

	/* Remove shortcuts list */
	g_list_foreach (pm_plugin->shortcuts, (GFunc)g_free, NULL);
	g_list_free (pm_plugin->shortcuts);
	pm_plugin->shortcuts = NULL;

	/* Destroy project */
	anjuta_pm_project_free (pm_plugin->project);

	return TRUE;
}

static void
project_manager_plugin_finalize (GObject *obj)
{
	/* FIXME: */
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
project_manager_plugin_dispose (GObject *obj)
{
	/* FIXME: */
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
project_manager_plugin_instance_init (GObject *obj)
{
	ProjectManagerPlugin *plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (obj);
	plugin->scrolledwindow = NULL;
	plugin->project = NULL;
	plugin->view = NULL;
	plugin->pre_update_sources = NULL;
	plugin->pre_update_targets = NULL;
	plugin->pre_update_groups = NULL;
	plugin->project_root_uri = NULL;
	plugin->project_file = NULL;
	plugin->fm_current_uri = NULL;
	plugin->current_editor_uri = NULL;
	plugin->session_by_me = FALSE;
	plugin->close_project_idle = -1;
	plugin->shortcuts = NULL;
}

static void
project_manager_plugin_class_init (GObjectClass *klass)
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = project_manager_plugin_activate_plugin;
	plugin_class->deactivate = project_manager_plugin_deactivate_plugin;
	klass->dispose = project_manager_plugin_finalize;
	klass->dispose = project_manager_plugin_dispose;
}

/* IAnjutaProjectManager implementation */

static gboolean
file_is_inside_project (ProjectManagerPlugin *plugin, GFile *file)
{
	gchar *uri = g_file_get_uri (file);
	gboolean inside = FALSE;

	if (plugin->project_root_uri == NULL)
	{
		/* No project open */
		return FALSE;
	}

	if (strncmp (uri, plugin->project_root_uri,
				 strlen (plugin->project_root_uri)) == 0)
	{
		g_free (uri);
		return TRUE;
	}

	if (uri[0] == '/')
	{
		const gchar *project_root_path = strchr (plugin->project_root_uri, ':');
		if (project_root_path)
			project_root_path += 3;
		else
			project_root_path = plugin->project_root_uri;
		inside = strncmp (uri, project_root_path,
						 strlen (project_root_path)) == 0;
	}
	g_free (uri);

	return inside;
}

static GFile*
get_element_file_from_node (ProjectManagerPlugin *plugin, AnjutaProjectNode *node, const gchar *root)
{
	const gchar *project_root = NULL;
	GFile *file = NULL;

	if (!node)
		return NULL;

	anjuta_shell_get (ANJUTA_PLUGIN (plugin)->shell,
					  root, G_TYPE_STRING,
					  &project_root, NULL);
	if (project_root == NULL)
	{
		/* Perhaps missing build URI, use project URI instead */
		anjuta_shell_get (ANJUTA_PLUGIN (plugin)->shell,
					  IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI,
					  G_TYPE_STRING,
					  &project_root,
					  NULL);
	}

	file = g_object_ref (anjuta_project_node_get_file (node));

	if ((file != NULL) && (project_root != NULL))
	{
		gchar *rel_path;

		rel_path = g_file_get_relative_path (anjuta_project_node_get_file (anjuta_pm_project_get_root (plugin->project)), file);

		if (rel_path)
		{
			GFile *node_file = NULL;
			GFile *root_file = NULL;
			root_file = g_file_new_for_uri (project_root);
			node_file = g_file_get_child (root_file, rel_path);
			g_object_unref (root_file);
			g_object_unref (file);

			file = node_file;
			g_free (rel_path);
		}
	}

	return file;
}

static GtkTreeIter*
get_tree_iter_from_file (ProjectManagerPlugin *plugin, GtkTreeIter* iter, GFile *file, GbfTreeNodeType type)
{
	gboolean found;

	found = gbf_project_view_find_file (plugin->view, iter, file, type);

	return found ? iter : NULL;
}

static gboolean
project_node_compare (AnjutaProjectNode *node, gpointer data)
{
	GFile *file = (GFile *)data;

	switch (anjuta_project_node_get_node_type (node))
	{
	case ANJUTA_PROJECT_GROUP:
	case ANJUTA_PROJECT_SOURCE:
	case ANJUTA_PROJECT_OBJECT:
	case ANJUTA_PROJECT_TARGET:
		return g_file_equal (anjuta_project_node_get_file (node), file);
	default:
		return FALSE;
	}
}

static AnjutaProjectNode *
get_node_from_file (const AnjutaProjectNode *root, GFile *file)
{
	AnjutaProjectNode *node;

	node = anjuta_project_node_traverse ((AnjutaProjectNode *)root, G_PRE_ORDER, project_node_compare, file);

	return node;
}

static AnjutaProjectNodeType
get_element_type (ProjectManagerPlugin *plugin, GFile *element)
{
	AnjutaProjectNode *node;

	node = gbf_project_view_get_node_from_file (plugin->view, ANJUTA_PROJECT_UNKNOWN,  element);

	return node == NULL ? ANJUTA_PROJECT_UNKNOWN : anjuta_project_node_get_node_type (node);
}

static GList*
iproject_manager_get_children (IAnjutaProjectManager *project_manager,
							   GFile *parent,
                               gint children_type,
							   GError **err)
{
	ProjectManagerPlugin *plugin;
	GList *children = NULL;

	g_return_val_if_fail (ANJUTA_IS_PLUGIN (project_manager), NULL);

	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));
	if (plugin->project !=  NULL)
	{
		AnjutaProjectNode *root;

		root = anjuta_pm_project_get_root  (plugin->project);
		if (root != NULL)
		{
			/* Get parent */
			if (parent != NULL) root = get_node_from_file (root, parent);
			if (root != NULL)
			{
				/* Get all nodes */
				GList *node;

				children = gbf_project_util_node_all (root, children_type);

				/* Replace all nodes by their corresponding file */
				for (node = g_list_first (children); node != NULL; node = g_list_next (node))
				{
					if (anjuta_project_node_get_node_type (ANJUTA_PROJECT_NODE (node->data)) == ANJUTA_PROJECT_TARGET)
					{
						/* Take care of different build directory */
						node->data = get_element_file_from_node (plugin, ANJUTA_PROJECT_NODE (node->data), IANJUTA_BUILDER_ROOT_URI);
					}
					else
					{
            			node->data = g_object_ref (anjuta_project_node_get_file (ANJUTA_PROJECT_NODE (node->data)));
					}
				}
			}
		}
	}

	return children;
}

static GList*
iproject_manager_get_elements (IAnjutaProjectManager *project_manager,
							   AnjutaProjectNodeType element_type,
							   GError **err)
{
	return ianjuta_project_manager_get_children (project_manager, NULL, element_type, err);
}

static AnjutaProjectNodeType
iproject_manager_get_target_type (IAnjutaProjectManager *project_manager,
								   GFile *target_file,
								   GError **err)
{
	ProjectManagerPlugin *plugin;

	g_return_val_if_fail (ANJUTA_IS_PLUGIN (project_manager),
						  ANJUTA_PROJECT_UNKNOWN);

	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));

	/* Check that file belongs to the project */
	if ((plugin->project != NULL) && file_is_inside_project (plugin, target_file))
	{
		AnjutaProjectNode *node;

		node = anjuta_pm_project_get_root  (plugin->project);
		if (node != NULL)
		{
			node = get_node_from_file (node, target_file);
			if (node != NULL) return anjuta_project_node_get_node_type (node);
		}
	}

	return ANJUTA_PROJECT_UNKNOWN;
}
static GList*
iproject_manager_get_targets (IAnjutaProjectManager *project_manager,
							  AnjutaProjectNodeType target_type,
							  GError **err)
{
	GList *targets, *node;
	ProjectManagerPlugin *plugin;

	g_return_val_if_fail (ANJUTA_IS_PLUGIN (project_manager), NULL);

	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));

	/* Get all targets */
	targets = gbf_project_util_node_all (anjuta_pm_project_get_root (plugin->project), target_type);

	/* Replace all targets by their corresponding URI */
	for (node = g_list_first (targets); node != NULL; node = g_list_next (node))
	{
		node->data = get_element_file_from_node (plugin, node->data, IANJUTA_BUILDER_ROOT_URI);
	}

	return targets;
}

static GFile*
iproject_manager_get_parent (IAnjutaProjectManager *project_manager,
							 GFile *element,
							 GError **err)
{
	ProjectManagerPlugin *plugin;
	GFile *file = NULL;

	g_return_val_if_fail (ANJUTA_IS_PLUGIN (project_manager), NULL);

	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));
	if (plugin->project !=  NULL)
	{
		AnjutaProjectNode *node;

		node = anjuta_pm_project_get_root  (plugin->project);
		if (node != NULL)
		{
			node = get_node_from_file (node, element);
			if (node != NULL) node = anjuta_project_node_parent (node);
			if (node != NULL)
			{
				file = anjuta_project_node_get_file (node);
				if (file != NULL) g_object_ref (file);
			}
		}
	}

	return file;
}

static GFile*
iproject_manager_get_selected (IAnjutaProjectManager *project_manager,
							   GError **err)
{
	AnjutaProjectNode *node;
	ProjectManagerPlugin *plugin;

	g_return_val_if_fail (ANJUTA_IS_PLUGIN (project_manager), NULL);

	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));
	if (!anjuta_pm_project_is_open (plugin->project)) return NULL;

	node = gbf_project_view_find_selected (plugin->view,
										   ANJUTA_PROJECT_SOURCE);
	if (node && anjuta_project_node_get_node_type (node) == ANJUTA_PROJECT_SOURCE)
	{
		return g_object_ref (anjuta_project_node_get_file (node));
	}

	node = gbf_project_view_find_selected (plugin->view,
										   ANJUTA_PROJECT_TARGET);
	if (node && anjuta_project_node_get_node_type (node) == ANJUTA_PROJECT_TARGET)
	{
		return get_element_file_from_node (plugin, node, IANJUTA_BUILDER_ROOT_URI);
	}

	node = gbf_project_view_find_selected (plugin->view,
										   ANJUTA_PROJECT_GROUP);
	if (node && anjuta_project_node_get_node_type (node) == GBF_TREE_NODE_GROUP)
	{
		return g_object_ref (anjuta_project_node_get_file (node));
	}

	return NULL;
}

static guint
iproject_manager_get_capabilities (IAnjutaProjectManager *project_manager,
								   GError **err)
{
	ProjectManagerPlugin *plugin;

	g_return_val_if_fail (ANJUTA_IS_PLUGIN (project_manager), 0);

	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));
	return anjuta_pm_project_get_capabilities (plugin->project);
}

static GFile*
iproject_manager_add_source (IAnjutaProjectManager *project_manager,
							 const gchar *source_uri_to_add,
							 GFile *default_target_file,
							 GError **err)
{
	ProjectManagerPlugin *plugin;
	GtkTreeIter target_iter;
	GtkTreeIter *iter = NULL;
	AnjutaProjectNode *source_id;
	GFile* source;

	g_return_val_if_fail (ANJUTA_IS_PLUGIN (project_manager), FALSE);

	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));

	update_operation_begin (plugin);
	if (default_target_file != NULL)
	{
		iter = get_tree_iter_from_file (plugin, &target_iter, default_target_file, GBF_TREE_NODE_TARGET);
	}
	source_id = anjuta_pm_project_new_source (plugin,
										     get_plugin_parent_window (plugin),
											 iter,
											 source_uri_to_add);
	update_operation_end (plugin, TRUE);

	source = get_element_file_from_node(plugin, source_id, IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI);

	return source;
}

static GFile*
iproject_manager_add_source_quiet (IAnjutaProjectManager *project_manager,
								   const gchar *source_uri_to_add,
								   GFile *location_file,
								   GError **err)
{
	ProjectManagerPlugin *plugin;
	AnjutaProjectNode *source_id;
	AnjutaProjectNode *target = NULL;

	g_return_val_if_fail (ANJUTA_IS_PLUGIN (project_manager), FALSE);

	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));

	target = gbf_project_view_get_node_from_file (plugin->view, ANJUTA_PROJECT_TARGET,  location_file);
	update_operation_begin (plugin);
	source_id = anjuta_pm_project_add_source (plugin->project,
	    								target,
										NULL,
	    								source_uri_to_add,
										err);
	update_operation_end (plugin, TRUE);

	return get_element_file_from_node (plugin, source_id, IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI);
}

static GList*
iproject_manager_add_source_multi (IAnjutaProjectManager *project_manager,
							 GList *source_add_uris,
							 GFile *default_target_file,
							 GError **err)
{
	ProjectManagerPlugin *plugin;
	GtkTreeIter target_iter;
	GtkTreeIter *iter = NULL;
	GList* source_ids;
	GList* source_files = NULL;

	g_return_val_if_fail (ANJUTA_IS_PLUGIN (project_manager), FALSE);

	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));

	update_operation_begin (plugin);
	if (default_target_file != NULL)
	{
		iter = get_tree_iter_from_file (plugin, &target_iter, default_target_file, GBF_TREE_NODE_TARGET);
	}

	source_ids = anjuta_pm_project_new_multiple_source (plugin,
										 get_plugin_parent_window (plugin),
										 iter,
										 source_add_uris);
	update_operation_end (plugin, TRUE);

	while (source_ids)
	{
		source_files = g_list_append (source_files,
									 get_element_file_from_node (plugin,
														  source_ids->data,
														  IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI));
		source_ids = g_list_delete_link (source_ids, source_ids);
	}

	return source_files;
}

static GFile*
iproject_manager_add_target (IAnjutaProjectManager *project_manager,
							 const gchar *target_name_to_add,
							 GFile *default_group_file,
							 GError **err)
{
	ProjectManagerPlugin *plugin;
	GtkTreeIter group_iter;
	GtkTreeIter *iter = NULL;
	GFile *target = NULL;
	AnjutaProjectNode *target_id;

	g_return_val_if_fail (ANJUTA_IS_PLUGIN (project_manager), FALSE);

	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));

	if (default_group_file != NULL)
	{
		iter = get_tree_iter_from_file (plugin, &group_iter, default_group_file, GBF_TREE_NODE_GROUP);
	}

	update_operation_begin (plugin);
	target_id = anjuta_pm_project_new_target (plugin,
											 get_plugin_parent_window (plugin),
											 iter,
											 target_name_to_add);
	update_operation_end (plugin, TRUE);
	target = get_element_file_from_node (plugin, target_id, IANJUTA_BUILDER_ROOT_URI);

	return target;
}

static GFile*
iproject_manager_add_group (IAnjutaProjectManager *project_manager,
							const gchar *group_name_to_add,
							GFile *default_group_file,
							GError **err)
{
	ProjectManagerPlugin *plugin;
	GtkTreeIter group_iter;
	GtkTreeIter *iter = NULL;
	GFile *group = NULL;
	AnjutaProjectNode *group_id;

	g_return_val_if_fail (ANJUTA_IS_PLUGIN (project_manager), FALSE);

	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));

	if (default_group_file != NULL)
	{
		iter = get_tree_iter_from_file (plugin, &group_iter, default_group_file, GBF_TREE_NODE_GROUP);
	}

	update_operation_begin (plugin);
	group_id = anjuta_pm_project_new_group (plugin,
										   get_plugin_parent_window (plugin),
										   iter,
										   group_name_to_add);
	update_operation_end (plugin, TRUE);
	group = get_element_file_from_node (plugin, group_id, IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI);

	return group;
}

static gboolean
iproject_manager_is_open (IAnjutaProjectManager *project_manager, GError **err)
{
	ProjectManagerPlugin *plugin;

	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));

	return anjuta_pm_project_is_open (plugin->project);
}

static GList*
iproject_manager_get_packages (IAnjutaProjectManager *project_manager, GError **err)
{
	ProjectManagerPlugin *plugin;

	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));

	return anjuta_pm_project_get_packages (plugin->project);
}

static IAnjutaProject*
iproject_manager_get_current_project (IAnjutaProjectManager *project_manager, GError **err)
{
	ProjectManagerPlugin *plugin;

	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));

	return anjuta_pm_project_get_project (plugin->project);
}

static void
iproject_manager_iface_init(IAnjutaProjectManagerIface *iface)
{
	iface->get_elements = iproject_manager_get_elements;
	iface->get_target_type = iproject_manager_get_target_type;
	iface->get_targets = iproject_manager_get_targets;
	iface->get_parent = iproject_manager_get_parent;
	iface->get_children = iproject_manager_get_children;
	iface->get_selected = iproject_manager_get_selected;
	iface->get_capabilities = iproject_manager_get_capabilities;
	iface->add_source = iproject_manager_add_source;
	iface->add_source_quiet = iproject_manager_add_source_quiet;
	iface->add_sources = iproject_manager_add_source_multi;
	iface->add_target = iproject_manager_add_target;
	iface->add_group = iproject_manager_add_group;
	iface->is_open = iproject_manager_is_open;
	iface->get_packages = iproject_manager_get_packages;
	iface->get_current_project = iproject_manager_get_current_project;
}

static void
ifile_open (IAnjutaFile *ifile, GFile* file, GError **e)
{
	AnjutaProfile *profile;
	AnjutaProfileManager *profile_manager;
	AnjutaPluginManager *plugin_manager;
	AnjutaStatus *status;
	gchar *session_profile_path, *profile_name;
	GFile *session_profile;
	GFile *default_profile;
	GFile *project_root;
	GFile *tmp;
	ProjectManagerPlugin *plugin;
	GError *error = NULL;

	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (ifile);

	/* If there is already a project loaded, load in separate anjuta window */
	if (plugin->project_root_uri)
	{
		gchar *uri = g_file_get_uri (file);
		gchar *quoted_uri = g_shell_quote (uri);
		gchar *cmd = g_strconcat ("anjuta --no-splash --no-client ", quoted_uri, NULL);
		g_free (quoted_uri);
		anjuta_util_execute_shell (NULL, cmd);
		g_free (cmd);
		g_free (uri);

		return;
	}

	plugin_manager  =
		anjuta_shell_get_plugin_manager (ANJUTA_PLUGIN (ifile)->shell, NULL);
	profile_manager =
		anjuta_shell_get_profile_manager (ANJUTA_PLUGIN (ifile)->shell, NULL);
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (ifile)->shell, NULL);

	anjuta_status_progress_add_ticks (status, 2);
	/* Prepare profile */
	profile = anjuta_profile_new (PROJECT_PROFILE_NAME, plugin_manager);

	/* System default profile */
	default_profile = g_file_new_for_uri (DEFAULT_PROFILE);
	anjuta_profile_add_plugins_from_xml (profile, default_profile,
										 TRUE, &error);
	profile_name = g_file_get_basename (default_profile);
	g_object_unref (default_profile);
	if (error)
	{
		g_propagate_error (e, error);
		g_free (profile_name);
		g_object_unref (profile);

		return;
	}

	/* Project default profile */
	anjuta_profile_add_plugins_from_xml (profile, file, TRUE, &error);
	if (error)
	{
		g_propagate_error (e, error);

		g_free (profile_name);
		g_object_unref (profile);

		return;
	}

	/* Project session profile */
	project_root = g_file_get_parent (file);
	tmp = g_file_get_child (project_root, ".anjuta");
	session_profile = g_file_get_child (tmp, profile_name);
	g_object_unref (tmp);
	g_free (profile_name);

	session_profile_path = g_file_get_path (session_profile);
	DEBUG_PRINT ("Loading project session profile: %s", session_profile_path);
	if (g_file_query_exists (session_profile, NULL))
	{
		anjuta_profile_add_plugins_from_xml (profile, session_profile,
											 FALSE, &error);
		if (error)
		{
			g_propagate_error (e, error);

			g_free (session_profile_path);
			g_object_unref (project_root);
			g_object_unref (profile);
			g_object_unref (session_profile);

			return;
		}
	}
	anjuta_profile_set_sync_file (profile, session_profile);
	g_free (session_profile_path);

	/* Set project uri */
	g_free (plugin->project_root_uri);
	if (plugin->project_file) g_object_unref (plugin->project_file);

	plugin->project_file = g_object_ref (file);
	plugin->project_root_uri = g_file_get_uri (project_root);
	g_object_unref (project_root);

	/* Load profile */
	anjuta_profile_manager_push (profile_manager, profile, &error);
	if (error)
	{
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (ifile)->shell),
								  "%s", error->message);
		g_error_free (error);
		error = NULL;
	}

	anjuta_status_progress_tick (status, NULL,
								 _("Initializing Project…"));
	update_ui (plugin);

	anjuta_status_progress_tick (status, NULL, _("Project Loaded"));
}

static GFile*
ifile_get_file (IAnjutaFile *ifile, GError **e)
{
	ProjectManagerPlugin *plugin;

	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (ifile);
	if (plugin->project_root_uri)
		return g_file_new_for_uri (plugin->project_root_uri);
	else
		return NULL;
}

static void
ifile_iface_init(IAnjutaFileIface *iface)
{
	iface->open = ifile_open;
	iface->get_file = ifile_get_file;
}

ANJUTA_PLUGIN_BEGIN (ProjectManagerPlugin, project_manager_plugin);
	ANJUTA_PLUGIN_ADD_INTERFACE (iproject_manager, IANJUTA_TYPE_PROJECT_MANAGER);
	ANJUTA_PLUGIN_ADD_INTERFACE (ifile, IANJUTA_TYPE_FILE);
	anjuta_pm_chooser_button_register (module);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (ProjectManagerPlugin, project_manager_plugin);

