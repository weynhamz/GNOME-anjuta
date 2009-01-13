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
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-file-manager.h>
#include <libanjuta/interfaces/ianjuta-builder.h>
#include <libanjuta/interfaces/ianjuta-project-backend.h>
#include <libanjuta/anjuta-profile-manager.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-status.h>

#include "gbf-project-util.h"

#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-project-manager.ui"
#define PREFS_GLADE PACKAGE_DATA_DIR"/glade/anjuta-project-manager-plugin.glade"
#define ICON_FILE "anjuta-project-manager-plugin-48.png"
#define DEFAULT_PROFILE "file://"PACKAGE_DATA_DIR"/profiles/default.profile"
#define PROJECT_PROFILE_NAME "project"

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
	const gchar* id;
	GtkWidget* dialog;
};

static gpointer parent_class;

static gboolean uri_is_inside_project (ProjectManagerPlugin *plugin,
									   const gchar *uri);
static void project_manager_plugin_close (ProjectManagerPlugin *plugin);

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

static void
update_title (ProjectManagerPlugin* plugin, const gchar *project_uri)
{
	AnjutaStatus *status;
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell, NULL);
	if (project_uri)
	{
		gchar* uri_basename;
		gchar* unescape_basename;
		gchar* ext;
		
		uri_basename = g_path_get_basename (project_uri);
		unescape_basename = gnome_vfs_unescape_string (uri_basename, "");
		ext = strrchr (unescape_basename, '.');
		if (ext)
			*ext = '\0';
		anjuta_status_set_title (status, unescape_basename);
		g_free (unescape_basename);
		g_free (uri_basename);
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
	GList *files;

	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;
	
	/*
	 * When a project session is being saved (session_by_me == TRUE),
	 * we should not save the current project uri, because project 
	 * sessions are loaded when the project has already been loaded.
	 */
	if (plugin->project_uri && !plugin->session_by_me)
	{
		files = anjuta_session_get_string_list (session,
												"File Loader",
												"Files");
		files = g_list_append (files, g_strdup (plugin->project_uri));
		anjuta_session_set_string_list (session, "File Loader",
										"Files", files);
		g_list_foreach (files, (GFunc)g_free, NULL);
		g_list_free (files);
	}
}

static void
on_shell_exiting (AnjutaShell *shell, ProjectManagerPlugin *plugin)
{
	if (plugin->project_uri)
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
	if (plugin->project_uri)
		plugin->close_project_idle = g_idle_add (on_close_project_idle, plugin);
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

/* Properties dialogs functions
 *---------------------------------------------------------------------------*/

static void
properties_dialog_info_free (PmPropertiesDialogInfo *info)
{
	gtk_widget_destroy (info->dialog);
	g_free (info);
}

static gint
compare_properties_widget (PmPropertiesDialogInfo *info, GtkWidget *widget)
{
	return info->dialog == widget ? 0 : 1;
}

static gint
compare_properties_id (PmPropertiesDialogInfo *info, PmPropertiesDialogInfo *info1)
{
	/* project properties have a NULL id */
	return (info->type == info1->type) &&
			((info1->id == NULL) ||
			 ((info->id != NULL) &&
			  (strcmp(info->id, info1->id) == 0))) ? 0 : 1;
}

static void
on_properties_dialog_response (GtkDialog *win,
							   gint id,
							   ProjectManagerPlugin *plugin)
{
	GList *prop;
	
	prop = g_list_find_custom (plugin->prop_dialogs,
							   win,
							   (GCompareFunc)compare_properties_widget);
	if (prop != NULL)
	{
		properties_dialog_info_free ((PmPropertiesDialogInfo *)prop->data);
		plugin->prop_dialogs = g_list_delete_link (plugin->prop_dialogs, prop);
	}
}

/* Display properties dialog. These dialogs are not modal, so keep a list of
 * them to be able to destroy them if the project is closed. This list is
 * useful to put the dialog at the top if the same target is selected while
 * the corresponding dialog already exist instead of creating two times the
 * same dialog */

static void
project_manager_show_properties_dialog (ProjectManagerPlugin *plugin,
										PmPropertiesType type,
										const gchar *id)
{
	PmPropertiesDialogInfo info;
	GList* prop;
		
	info.type = type;
	info.id = id; 
	prop = g_list_find_custom (plugin->prop_dialogs,
							   &info,
							   (GCompareFunc)compare_properties_id);
	if (prop != NULL)
	{
		/* Show already existing dialog */

		gtk_window_present (GTK_WINDOW (((PmPropertiesDialogInfo *)prop->data)->dialog));
	}
	else
	{
		/* Create new dialog */
		GtkWidget *win;
		GtkWidget *properties = NULL;
		const char* title = NULL;
		
		switch (type)
		{
			case PM_PROJECT_PROPERTIES:
				properties = gbf_project_configure (plugin->project, NULL);
				title = _("Project properties");
				break;
			case PM_TARGET_PROPERTIES:		
				properties = gbf_project_configure_target (plugin->project,
														   id, NULL);
				title = _("Target properties");
				break;
			case PM_GROUP_PROPERTIES:
				properties = gbf_project_configure_group (plugin->project,
														   id, NULL);
				title = _("Group properties");
				break;
		}
			
		if (properties)
		{
			win = gtk_dialog_new_with_buttons (title,
									   GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell),
									   GTK_DIALOG_DESTROY_WITH_PARENT,
									   GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL, NULL);
			info.dialog = win;
			plugin->prop_dialogs = g_list_prepend (plugin->prop_dialogs, g_memdup (&info, sizeof (info)));
			g_signal_connect (win, "response",
							  G_CALLBACK (on_properties_dialog_response),
							  plugin);
			
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
	anjuta_status_push (status, "Refreshing symbol tree...");
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
	project_manager_show_properties_dialog (plugin, PM_PROJECT_PROPERTIES, NULL); 
}

static void
on_add_group (GtkAction *action, ProjectManagerPlugin *plugin)
{
	gchar *group_uri, *default_group_uri = NULL;
	if (plugin->current_editor_uri)
		default_group_uri = g_path_get_dirname (plugin->current_editor_uri);
	else
		default_group_uri = g_strdup ("");
	group_uri =
		ianjuta_project_manager_add_group (IANJUTA_PROJECT_MANAGER (plugin),
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
	target_uri =
		ianjuta_project_manager_add_target (IANJUTA_PROJECT_MANAGER (plugin),
											"", default_group_uri,
											NULL);
	g_free (target_uri);
	g_free (default_group_uri);
}

static void
on_add_source (GtkAction *action, ProjectManagerPlugin *plugin)
{
	gchar *default_group_uri = NULL, *default_source_uri;
	gchar* source_uri;
	
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
	source_uri =
		ianjuta_project_manager_add_source (IANJUTA_PROJECT_MANAGER (plugin),
											default_source_uri,
											default_group_uri, NULL);
	g_free (source_uri);
	g_free (default_group_uri);
}

static void
on_popup_properties (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GbfTreeData *data;
	
	data = gbf_project_view_find_selected (GBF_PROJECT_VIEW (plugin->view),
										   GBF_TREE_NODE_TARGET);
	if (data)
	{
		project_manager_show_properties_dialog (plugin, PM_TARGET_PROPERTIES, data->id);
		return;
	}
		
	data = gbf_project_view_find_selected (GBF_PROJECT_VIEW (plugin->view),
										   GBF_TREE_NODE_GROUP);
	if (data)
	{
		project_manager_show_properties_dialog (plugin, PM_GROUP_PROPERTIES, data->id);
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
	gchar* question;
	gchar* full_mesg;

	switch (data->type)
	{
		case GBF_TREE_NODE_GROUP:
			question = _("Are you sure you want to remove the following group from project?\n\n");
			mesg = _("Group: %s\n\nThe group will not be deleted from file system.");
			break;
		case GBF_TREE_NODE_TARGET:
			question = _("Are you sure you want to remove the following target from project?\n\n");
			mesg = _("Target: %s");
			break;
		case GBF_TREE_NODE_TARGET_SOURCE:
			question = _("Are you sure you want to remove the following source file from project?\n\n");
			mesg = _("Source: %s\n\nThe source file will not be deleted from file system.");
			break;
		default:
			g_warning ("Unknown node");
			return FALSE;
	}
	full_mesg = g_strconcat (question, mesg, NULL);
	answer =
		anjuta_util_dialog_boolean_question (get_plugin_parent_window (plugin),
											 full_mesg, data->name);
	g_free (full_mesg);
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
										  data->name, err->message);
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
				ianjuta_project_manager_add_source (IANJUTA_PROJECT_MANAGER
													(plugin),
													plugin->fm_current_uri,
													parent_directory,
													NULL);
			g_free(source_id);
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
			mesg = _("The file you selected is a link and can't be added to the project");
		anjuta_util_dialog_error (win,
								  _("Failed to retrieve URI info of %s: %s"),
								  plugin->fm_current_uri, mesg);
	}
}

static void
on_uri_activated (GtkWidget *widget, const gchar *uri,
				  ProjectManagerPlugin *plugin)
{
	IAnjutaFileLoader *loader;
	GFile* file = g_file_new_for_uri (uri);
	
	loader = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										 IAnjutaFileLoader, NULL);
	if (loader)
		ianjuta_file_loader_load (loader, file, FALSE, NULL);
	g_object_unref (file);
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
		N_("Add _Group..."), NULL, N_("Add a group to project"),
		G_CALLBACK (on_add_group)
	},
	{
		"ActionProjectAddTarget", GTK_STOCK_ADD,
		N_("Add _Target..."), NULL, N_("Add a target to project"),
		G_CALLBACK (on_add_target)
	},
	{
		"ActionProjectAddSource", GTK_STOCK_ADD,
		N_("Add _Source File..."), NULL, N_("Add a source file to project"),
		G_CALLBACK (on_add_source)
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
	GbfProjectCapabilities caps = GBF_PROJECT_CAN_ADD_NONE;
	
	if (plugin->project)
		caps = gbf_project_get_capabilities (plugin->project, NULL);
	
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (plugin)->shell, NULL);
	for (j = 0; j < G_N_ELEMENTS (pm_actions); j++)
	{
		action = anjuta_ui_get_action (ui, "ActionGroupProjectManager",
									   pm_actions[j].name);
		if (pm_actions[j].callback &&
			strcmp (pm_actions[j].name, "ActionFileCloseProject") != 0)
		{
			/* 'close' menuitem is never disabled */
			g_object_set (G_OBJECT (action), "sensitive",
						  (plugin->project != NULL), NULL);
		}
	}
	
	/* Main menu */
	action = anjuta_ui_get_action (ui, "ActionGroupProjectManager",
								   "ActionProjectAddGroup");
	g_object_set (G_OBJECT (action), "sensitive",
				  ((plugin->project != NULL) &&
				   (caps & GBF_PROJECT_CAN_ADD_GROUP)), NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupProjectManager",
								   "ActionProjectAddTarget");
	g_object_set (G_OBJECT (action), "sensitive",
				  ((plugin->project != NULL) &&
				   (caps & GBF_PROJECT_CAN_ADD_TARGET)), NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupProjectManager",
								   "ActionProjectAddSource");
	g_object_set (G_OBJECT (action), "sensitive",
				  ((plugin->project != NULL) &&
				   (caps & GBF_PROJECT_CAN_ADD_SOURCE)), NULL);

	/* Popup menus */
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
	GbfProjectCapabilities caps = GBF_PROJECT_CAN_ADD_NONE;
	
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (plugin)->shell, NULL);
	/* Popup menu */
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
	
	if (plugin->project)
		caps = gbf_project_get_capabilities (plugin->project, NULL);
	data = gbf_project_view_find_selected (GBF_PROJECT_VIEW (plugin->view),
										   GBF_TREE_NODE_TARGET_SOURCE);
	if (data && data->type == GBF_TREE_NODE_TARGET_SOURCE)
	{
		if (caps & GBF_PROJECT_CAN_ADD_SOURCE)
		{
			action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
										   "ActionPopupProjectAddSource");
			g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
		}
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
		if (caps & GBF_PROJECT_CAN_ADD_SOURCE)
		{
			action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
										   "ActionPopupProjectAddSource");
			g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
		}
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
		if (caps & GBF_PROJECT_CAN_ADD_GROUP)
		{
			action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
										   "ActionPopupProjectAddGroup");
			g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
		}
		if (caps & GBF_PROJECT_CAN_ADD_TARGET)
		{
			action = anjuta_ui_get_action (ui, "ActionGroupProjectManagerPopup",
										   "ActionPopupProjectAddTarget");
			g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
		}
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
								IANJUTA_PROJECT_MANAGER_CURRENT_URI,
								value, NULL);
		g_signal_emit_by_name (G_OBJECT (plugin), "element_selected",
							   selected_uri);
		g_free (selected_uri);
	} else {
		anjuta_shell_remove_value (ANJUTA_PLUGIN(plugin)->shell,
								   IANJUTA_PROJECT_MANAGER_CURRENT_URI, NULL);
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
project_manager_load_gbf (ProjectManagerPlugin *pm_plugin)
{
	AnjutaPluginManager *plugin_manager;
	AnjutaStatus *status;
	gchar *dirname;
	const gchar *root_uri;
	GError *error = NULL;
	GList *descs = NULL;
	GList *desc;
	
	root_uri = pm_plugin->project_root_uri;
	
	dirname = gnome_vfs_get_local_path_from_uri (root_uri);
	
	g_return_if_fail (dirname != NULL);
	
	if (pm_plugin->project != NULL)
			g_object_unref (pm_plugin->project);
	
	DEBUG_PRINT ("loading gbf backend...\n");
	plugin_manager = anjuta_shell_get_plugin_manager (ANJUTA_PLUGIN(pm_plugin)->shell, NULL);
	descs = anjuta_plugin_manager_query (plugin_manager,
										 "Anjuta Plugin",
										 "Interfaces",
										 "IAnjutaProjectBackend",
										 NULL);
	for (desc = g_list_first (descs); desc != NULL; desc = g_list_next (desc)) {
		AnjutaPluginDescription *backend;
		IAnjutaProjectBackend *plugin;
		gchar *location = NULL;
		
		backend = (AnjutaPluginDescription *)desc->data;
		anjuta_plugin_description_get_string (backend, "Anjuta Plugin", "Location", &location);
		plugin = (IAnjutaProjectBackend *)anjuta_plugin_manager_get_plugin_by_id (plugin_manager, location);
		g_free (location);
			
		pm_plugin->project = ianjuta_project_backend_new_project (plugin, NULL);
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
		plugin = NULL;
	}
	g_list_free (descs);
	
	if (!pm_plugin->project)
	{
		/* FIXME: Set err */
		g_warning ("no backend available for this project\n");
		g_free (dirname);
		return;
	}
	
	DEBUG_PRINT ("%s", "Creating new gbf project\n");
	
	/* pm_plugin->project = gbf_backend_new_project (backend->id); */
	if (!pm_plugin->project)
	{
		/* FIXME: Set err */
		g_warning ("project creation failed\n");
		g_free (dirname);
		return;
	}
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (pm_plugin)->shell, NULL);
	anjuta_status_progress_add_ticks (status, 1);
	anjuta_status_push (status, _("Loading project: %s"), g_basename (dirname));
	anjuta_status_busy_push (status);
	
	DEBUG_PRINT ("loading project %s\n\n", dirname);
	/* FIXME: use the error parameter to determine if the project
	 * was loaded successfully */
	gbf_project_load (pm_plugin->project, dirname, &error);
	
	anjuta_status_progress_tick (status, NULL, _("Created project view..."));
	
	if (error)
	{
		GtkWidget *toplevel;
		GtkWindow *win;
		
		toplevel = gtk_widget_get_toplevel (pm_plugin->scrolledwindow);
		if (toplevel && GTK_IS_WINDOW (toplevel))
			win = GTK_WINDOW (toplevel);
		else
			win = GTK_WINDOW (ANJUTA_PLUGIN (pm_plugin)->shell);
		
		anjuta_util_dialog_error (win, _("Failed to parse project (the project is opened, but there will be no project view) %s: %s\n"
										 ""),
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
	
	anjuta_status_set_default (status, _("Project"), g_basename (dirname));
	anjuta_status_pop (status);
	anjuta_status_busy_pop (status);
	g_free (dirname);
}

static void
project_manager_unload_gbf (ProjectManagerPlugin *pm_plugin)
{
	AnjutaStatus *status;
	
	if (pm_plugin->project)
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
		
		/* Remove remaining properties dialogs */
		g_list_foreach (pm_plugin->prop_dialogs, (GFunc)properties_dialog_info_free, NULL);
		g_list_free (pm_plugin->prop_dialogs);
		pm_plugin->prop_dialogs = NULL;
		
		/* Release project */
		g_object_unref (pm_plugin->project);
		pm_plugin->project = NULL;
		g_object_set (G_OBJECT (pm_plugin->model), "project", NULL, NULL);
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
	GValue *value;
	gchar *session_dir;
	DEBUG_PRINT ("Profile scoped: %s", anjuta_profile_get_name (profile));
	if (strcmp (anjuta_profile_get_name (profile), PROJECT_PROFILE_NAME) != 0)
		return;
	
	DEBUG_PRINT ("%s", "Project profile loaded; Restoring project session");
	
	/* Load gbf project */
	project_manager_load_gbf (plugin);
	
	/* Export project */
	value = g_new0 (GValue, 1);
	g_value_init (value, G_TYPE_STRING);
	g_value_set_string (value, plugin->project_root_uri);
	
	update_title (plugin, plugin->project_root_uri);
	anjuta_shell_add_value (ANJUTA_PLUGIN(plugin)->shell,
							IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI,
							value, NULL);
	
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
	g_free (plugin->project_uri);
	plugin->project_uri = NULL;
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
	GtkWidget *view, *scrolled_window;
	GbfProjectModel *model;
	static gboolean initialized = FALSE;
	GtkTreeSelection *selection;
	/* GladeXML *gxml; */
	ProjectManagerPlugin *pm_plugin;
	
	DEBUG_PRINT ("ProjectManagerPlugin: Activating Project Manager plugin %p...", plugin);
	
	if (!initialized)
		register_stock_icons (plugin);
	
	pm_plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (plugin);
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
	pm_plugin->prop_dialogs = NULL;
	
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

	DEBUG_PRINT ("ProjectManagerPlugin: Deactivate Project Manager plugin...");
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
	
	g_object_unref (G_OBJECT (pm_plugin->model));
	
	/* Widget is removed from the shell when destroyed */
	gtk_widget_destroy (pm_plugin->scrolledwindow);
	
	anjuta_ui_unmerge (pm_plugin->ui, pm_plugin->merge_id);
	anjuta_ui_remove_action_group (pm_plugin->ui, pm_plugin->pm_action_group);
	anjuta_ui_remove_action_group (pm_plugin->ui,
								   pm_plugin->popup_action_group);
	
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
	plugin->model = NULL;
	plugin->pre_update_sources = NULL;
	plugin->pre_update_targets = NULL;
	plugin->pre_update_groups = NULL;
	plugin->project_root_uri = NULL;
	plugin->project_uri = NULL;
	plugin->fm_current_uri = NULL;
	plugin->current_editor_uri = NULL;
	plugin->session_by_me = FALSE;
	plugin->close_project_idle = -1;
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
	if (strncmp (uri, plugin->project_root_uri,
				 strlen (plugin->project_root_uri)) == 0)
		return TRUE;
	
	if (uri[0] == '/')
	{
		const gchar *project_root_path = strchr (plugin->project_root_uri, ':');
		if (project_root_path)
			project_root_path += 3;
		else
			project_root_path = plugin->project_root_uri;
		return (strncmp (uri, project_root_path,
						 strlen (project_root_path)) == 0);
	}
	return FALSE;
}

static gchar *
get_element_uri_from_id (ProjectManagerPlugin *plugin, const gchar *id, const gchar *root)
{
	gchar *path, *ptr;
	gchar *uri;
	const gchar *project_root = NULL;
	
	if (!id)
		return NULL;
	
	path = g_strdup (id);
	ptr = strrchr (path, ':');
	if (ptr)
	{
		*ptr = '\0';
		if (ptr[1] == '/')
		{
			 /* ID is source ID, extract source uri */
			uri = strrchr (path, ':');		/* keep uri scheme */
			*ptr = ':';
			return g_strdup (uri+1);
		}
	}
	
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
	uri = g_build_filename (project_root, path, NULL);
	/* DEBUG_PRINT ("Converting id: %s to %s", id, uri); */
	g_free (path);
	return uri;
}

static const gchar *
get_element_relative_path (ProjectManagerPlugin *plugin, const gchar *uri, const gchar *root)
{
	const gchar *project_root = NULL;
	
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
	
	rel_path = get_element_relative_path (plugin, uri, IANJUTA_BUILDER_ROOT_URI);
	
	if (!rel_path)
		return NULL;
	
	/* FIXME: More target types should be handled */
	/* Test for shared lib */
	test_id = g_strconcat (rel_path, ":shared_lib", NULL);
	data = gbf_project_get_target (GBF_PROJECT (plugin->project),
								   test_id, NULL);
	g_free (test_id);
	
	if (!data)
	{
		/* Test for static lib */
		test_id = g_strconcat (rel_path, ":static_lib", NULL);
		data = gbf_project_get_target (GBF_PROJECT (plugin->project),
									   test_id, NULL);
		g_free (test_id);
	}
	if (!data)
	{
		/* Test for program */
		test_id = g_strconcat (rel_path, ":program", NULL);
		data = gbf_project_get_target (GBF_PROJECT (plugin->project),
									   test_id, NULL);
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
		id = g_strconcat (get_element_relative_path (plugin, uri, IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI), "/", NULL);
	}
	else
	{
		id = strdup (get_element_relative_path (plugin, uri, IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI));
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
	
	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));
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
	
	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));
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
												(const gchar *)node->data,
												IANJUTA_BUILDER_ROOT_URI));
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
												(const gchar *)node->data,
										        IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI));
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
	
	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));
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
	GList *target_types = NULL;
	
	g_return_val_if_fail (ANJUTA_IS_PLUGIN (project_manager), NULL);
	
	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));
	g_return_val_if_fail (GBF_IS_PROJECT (plugin->project), NULL);
	
	switch (target_type)
	{
		case IANJUTA_PROJECT_MANAGER_TARGET_SHAREDLIB:
			target_types = g_list_append(target_types, "shared_lib");
			break;
		case IANJUTA_PROJECT_MANAGER_TARGET_STATICLIB:
			target_types = g_list_append(target_types, "static_lib");
			break;
		case IANJUTA_PROJECT_MANAGER_TARGET_EXECUTABLE:
			target_types = g_list_append(target_types, "program");
			target_types = g_list_append(target_types, "script");
			break;
		default:
			/* FIXME: there are some more target types */
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
			GList* type_node;
			t_type++;
			for (type_node = target_types; type_node != NULL;
				 type_node = type_node->next)
			{
				if (strcmp (t_type, type_node->data) == 0)
				{
					gchar *target_uri = get_element_uri_from_id (plugin,
																 target_id,
																 IANJUTA_BUILDER_ROOT_URI);
					elements = g_list_prepend (elements, target_uri);
				}
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
	
	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));
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
	
	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));
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
	
	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));
	if (plugin->project == NULL) return NULL;
	
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
		uri = get_element_uri_from_id (plugin, data->id, IANJUTA_BUILDER_ROOT_URI);
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

static IAnjutaProjectManagerCapabilities
iproject_manager_get_capabilities (IAnjutaProjectManager *project_manager,
								   GError **err)
{
	ProjectManagerPlugin *plugin;
	
	g_return_val_if_fail (ANJUTA_IS_PLUGIN (project_manager),
						  IANJUTA_PROJECT_MANAGER_CAN_ADD_NONE);
	
	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));
	return gbf_project_get_capabilities (plugin->project, NULL);
}

static gchar*
iproject_manager_add_source (IAnjutaProjectManager *project_manager,
							 const gchar *source_uri_to_add,
							 const gchar *default_location_uri,
							 GError **err)
{
	ProjectManagerPlugin *plugin;
	IAnjutaProjectManagerElementType default_location_type;
	gchar *location_id = NULL;
	gchar* source_id;
	gchar* source_uri;
	
	g_return_val_if_fail (ANJUTA_IS_PLUGIN (project_manager), FALSE);
	
	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));
	g_return_val_if_fail (GBF_IS_PROJECT (plugin->project), FALSE);

	update_operation_begin (plugin);
	if (default_location_uri == NULL)
	{
		default_location_type = IANJUTA_PROJECT_MANAGER_UNKNOWN;
	}
	else
	{
		default_location_type =
			ianjuta_project_manager_get_element_type (project_manager,
													  default_location_uri, NULL);
		location_id = get_element_id_from_uri (plugin, default_location_uri);
	}
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
	
	source_uri = get_element_uri_from_id(plugin, source_id, IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI);
	g_free(source_id);
	
	return source_uri;
}

static GList*
iproject_manager_add_source_multi (IAnjutaProjectManager *project_manager,
							 GList *source_add_uris,
							 const gchar *default_location_uri,
							 GError **err)
{
	ProjectManagerPlugin *plugin;
	IAnjutaProjectManagerElementType default_location_type;
	gchar *location_id = NULL;
	GList* source_ids;
	GList* source_uris = NULL;
	
	g_return_val_if_fail (ANJUTA_IS_PLUGIN (project_manager), FALSE);
	
	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));
	g_return_val_if_fail (GBF_IS_PROJECT (plugin->project), FALSE);

	update_operation_begin (plugin);
	if (default_location_uri == NULL)
	{
		default_location_type = IANJUTA_PROJECT_MANAGER_UNKNOWN;
	}
	else
	{
		default_location_type =
			ianjuta_project_manager_get_element_type (project_manager,
													  default_location_uri, NULL);
		location_id = get_element_id_from_uri (plugin, default_location_uri);
	}
	if (default_location_type == IANJUTA_PROJECT_MANAGER_GROUP)
	{
		source_ids = gbf_project_util_add_source_multi (plugin->model,
											 get_plugin_parent_window (plugin),
												 NULL, location_id,
												 source_add_uris);
	}
	else if (default_location_type == IANJUTA_PROJECT_MANAGER_TARGET)
	{
		source_ids =
			gbf_project_util_add_source_multi (plugin->model,
										   get_plugin_parent_window (plugin),
										   location_id, NULL,
										   source_add_uris);
	}
	else
	{
		source_ids =
			gbf_project_util_add_source_multi (plugin->model,
										   get_plugin_parent_window (plugin),
										   NULL, NULL,
										   source_add_uris);
	}
	update_operation_end (plugin, TRUE);
	
	while (source_ids)
	{
		source_uris = g_list_append (source_uris,
									 get_element_uri_from_id (plugin,
														  source_ids->data,
														  IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI));
		g_free (source_ids->data);
		source_ids = g_list_next(source_ids);
	}
	g_list_free (source_ids);
	return source_uris;
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
	
	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));
	
	g_return_val_if_fail (GBF_IS_PROJECT (plugin->project), FALSE);

	default_group_id = get_element_id_from_uri (plugin, default_group_uri);
	
	update_operation_begin (plugin);
	target_id = gbf_project_util_new_target (plugin->model,
											 get_plugin_parent_window (plugin),
											 default_group_id,
											 target_name_to_add);
	update_operation_end (plugin, TRUE);
	target_uri = get_element_uri_from_id (plugin, target_id, IANJUTA_BUILDER_ROOT_URI);
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
	
	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));
	g_return_val_if_fail (GBF_IS_PROJECT (plugin->project), FALSE);

	default_group_id = get_element_id_from_uri (plugin, default_group_uri);
	
	update_operation_begin (plugin);
	group_id = gbf_project_util_new_group (plugin->model,
										   get_plugin_parent_window (plugin),
										   default_group_id,
										   group_name_to_add);
	update_operation_end (plugin, TRUE);
	group_uri = get_element_uri_from_id (plugin, group_id, IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI);
	g_free (group_id);
	g_free (default_group_id);
	return group_uri;
}

static gboolean
iproject_manager_is_open (IAnjutaProjectManager *project_manager, GError **err)
{
	ProjectManagerPlugin *plugin;

	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));

	return GBF_IS_PROJECT (plugin->project);
}

static GList*
iproject_manager_get_packages (IAnjutaProjectManager *project_manager, GError **err)
{
	ProjectManagerPlugin *plugin;
	GList *modules = NULL;
	GList *packages = NULL;
	GList* node;
	
	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (G_OBJECT (project_manager));
	
	modules = gbf_project_get_config_modules (plugin->project, NULL);
	for (node = modules; node != NULL; node = g_list_next (node))
	{
		GList* mod_pkgs = gbf_project_get_config_packages (plugin->project,
														   node->data, NULL);
		packages = g_list_concat (packages, mod_pkgs);
	}
	g_list_foreach (modules, (GFunc)g_free, NULL);
	g_list_free (modules);
	return packages;
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
	iface->get_capabilities = iproject_manager_get_capabilities;
	iface->add_source = iproject_manager_add_source;
	iface->add_sources = iproject_manager_add_source_multi;
	iface->add_target = iproject_manager_add_target;
	iface->add_group = iproject_manager_add_group;
	iface->is_open = iproject_manager_is_open;
	iface->get_packages = iproject_manager_get_packages;
}

static void
ifile_open (IAnjutaFile *ifile, GFile* file, GError **e)
{
	AnjutaProfile *profile;
	AnjutaProfileManager *profile_manager;
	AnjutaPluginManager *plugin_manager;
	AnjutaStatus *status;
	gchar *dirname, *dirname_tmp, *vfs_dir;
	gchar *session_profile_path, *profile_name;
	GFile *session_profile;
	ProjectManagerPlugin *plugin;
	GError *error = NULL;
	gchar* uri = g_file_get_uri (file);
	GnomeVFSURI* vfs_uri;

	plugin = ANJUTA_PLUGIN_PROJECT_MANAGER (ifile);
	
	/* If there is already a project loaded, load in separate anjuta window */
	if (plugin->project_root_uri)
	{
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
	session_profile = g_file_new_for_uri (DEFAULT_PROFILE);
	anjuta_profile_add_plugins_from_xml (profile, session_profile,
										 TRUE, &error);
	g_object_unref (session_profile);
	if (error)
	{
		g_propagate_error (e, error);

		g_free(uri);
		g_object_unref (profile);
		
		return;
	}
	
	/* Project default profile */
	session_profile = g_file_new_for_uri (uri);
	anjuta_profile_add_plugins_from_xml (profile, session_profile, TRUE, &error);
	g_object_unref (session_profile);
	if (error)
	{
		g_propagate_error (e, error);
		
		g_free(uri);
		g_object_unref (profile);

		return;
	}
	
	/* Project session profile */
	vfs_uri = gnome_vfs_uri_new (uri);
	
	dirname_tmp = gnome_vfs_uri_extract_dirname (vfs_uri);
	dirname = gnome_vfs_unescape_string (dirname_tmp, "");
	g_free (dirname_tmp);
	
	profile_name = g_path_get_basename (DEFAULT_PROFILE);
	gnome_vfs_uri_unref (vfs_uri);
	
	session_profile_path = g_build_filename (dirname, ".anjuta",
										profile_name, NULL);
	DEBUG_PRINT ("Loading project session profile: %s", session_profile_path);
	session_profile = g_file_new_for_path (session_profile_path);
	if (g_file_query_exists (session_profile, NULL))
	{
		anjuta_profile_add_plugins_from_xml (profile, session_profile,
											 FALSE, &error);
		if (error)
		{
			g_propagate_error (e, error);
			
			g_free(uri);
			g_object_unref (profile);
			g_object_unref (session_profile);
			
			return;
		}
	}
	anjuta_profile_set_sync_file (profile, session_profile); 
	g_free (session_profile_path);
	g_free (profile_name);
	
	/* Set project uri */
	g_free (plugin->project_root_uri);
	g_free (plugin->project_uri);
	
	vfs_dir = gnome_vfs_get_uri_from_local_path (dirname);
	plugin->project_uri = g_strdup (uri);
	plugin->project_root_uri = vfs_dir;
	g_free (dirname);
	
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
								 _("Initializing Project..."));
	update_ui (plugin);

	anjuta_status_progress_tick (status, NULL, _("Project Loaded"));
	g_free (uri);
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
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (ProjectManagerPlugin, project_manager_plugin);
