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

static void
on_refresh (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GError *err = NULL;
	gbf_project_refresh (GBF_PROJECT (plugin->project), &err);
	if (err)
	{
		GtkWindow *win;
		win = GTK_WINDOW (gtk_widget_get_toplevel (plugin->scrolledwindow));
		anjuta_util_dialog_error (win, "Failed to refresh project: %s",
								  err->message);
		g_error_free (err);
	}
}

static void
on_properties (GtkAction *action, ProjectManagerPlugin *plugin)
{
	/* FIXME: */
}

static void
on_add_group (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GtkWidget *win = gtk_widget_get_toplevel (plugin->scrolledwindow);
	gbf_project_util_new_group (plugin->model,
								GTK_WINDOW (win), NULL);
}
 
static void
on_add_target (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GtkWidget *win = gtk_widget_get_toplevel (plugin->scrolledwindow);
	gbf_project_util_new_target (plugin->model,
								 GTK_WINDOW (win), NULL);
}

static void
on_add_source (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GtkWidget *win = gtk_widget_get_toplevel (plugin->scrolledwindow);
	gbf_project_util_add_source (plugin->model,
								 GTK_WINDOW (win), NULL,
								 plugin->current_editor_uri);
}

static void
on_popup_properties (GtkAction *action, ProjectManagerPlugin *plugin)
{
	/* FIXME: */
}

static void
on_popup_add_group (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GbfTreeData *data;
	const gchar *selected_group;
	GtkWidget *win;

	win = gtk_widget_get_toplevel (plugin->scrolledwindow);
	data = gbf_project_view_find_selected (GBF_PROJECT_VIEW (plugin->view),
										   GBF_TREE_NODE_GROUP);
	selected_group = NULL;
	if (data)
		selected_group = data->id;
	gbf_project_util_new_group (plugin->model, GTK_WINDOW (win),
								selected_group);
	if (data)
		gbf_tree_data_free (data);
}

static void
on_popup_add_target (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GbfTreeData *data;
	const gchar *selected_group;
	GtkWidget *win;

	win = gtk_widget_get_toplevel (plugin->scrolledwindow);
	data = gbf_project_view_find_selected (GBF_PROJECT_VIEW (plugin->view),
										   GBF_TREE_NODE_GROUP);
	selected_group = NULL;
	if (data)
		selected_group = data->id;
	gbf_project_util_new_target (plugin->model,
								 GTK_WINDOW (win), selected_group);
	if (data)
		gbf_tree_data_free (data);
}

static void
on_popup_add_source (GtkAction *action, ProjectManagerPlugin *plugin)
{
	GbfTreeData *data;
	const gchar *selected_target;
	GtkWidget *win;

	win = gtk_widget_get_toplevel (plugin->scrolledwindow);
	data = gbf_project_view_find_selected (GBF_PROJECT_VIEW (plugin->view),
										   GBF_TREE_NODE_TARGET);
	selected_target = NULL;
	if (data)
		selected_target = data->id;
	gbf_project_util_add_source (plugin->model,
								 GTK_WINDOW (win), selected_target, NULL);
	if (data)
		gbf_tree_data_free (data);
}

static gboolean
confirm_removal (ProjectManagerPlugin *plugin, GbfTreeData *data)
{
	GtkWidget *win;
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
	win = gtk_widget_get_toplevel (plugin->scrolledwindow);
	answer = anjuta_util_dialog_boolean_question (GTK_WINDOW (win),
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
			if (err)
			{
				GtkWindow *win;
				win = GTK_WINDOW (gtk_widget_get_toplevel (plugin->scrolledwindow));
				anjuta_util_dialog_error (win, "Failed to remove '%s':\n%s",
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
	GtkWidget *win;
	GnomeVFSFileInfo info;
	GnomeVFSResult res;
	
	win = gtk_widget_get_toplevel (plugin->scrolledwindow);
	res = gnome_vfs_get_file_info (plugin->fm_current_uri, &info,
								   GNOME_VFS_FILE_INFO_DEFAULT |
								   GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
	if (res == GNOME_VFS_OK)
	{
		if (info.type == GNOME_VFS_FILE_TYPE_DIRECTORY)
		{
			/* FIXME: Should select current directory */
			gbf_project_util_new_group (plugin->model,
										GTK_WINDOW (win), NULL);
		}
		else
		{
			gbf_project_util_add_source (plugin->model,
										 GTK_WINDOW (win), NULL,
										 plugin->fm_current_uri);
		}
	}
	else
	{
		const gchar *mesg;
		
		mesg = gnome_vfs_result_to_string (res);
		anjuta_util_dialog_error (GTK_WINDOW (win),
								  "Failed to retried URI info of %s: %s",
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
	gchar *target, *ptr;
	
#if 0
	GList *list;
	
	gbf_project_configure_target (GBF_PROJECT (plugin->project), target_id, NULL);
	list = gbf_project_get_build_targets (GBF_PROJECT (plugin->project), NULL);
	while (list)
	{
		GbfBuildTarget *t = list->data;
		g_message ("Build target: %s, %s, %s", t->id, t->label, t->description);
		list = g_list_next (list);
	}
	g_list_free (list);
#endif
	
	DEBUG_PRINT ("Target activated: %s", target_id);
	target = g_strdup (target_id);
	ptr = strchr (target, ':');
	if (ptr)
	{
		*ptr = '\0';
		ptr++;
		
		if (strcmp (ptr, "static_lib") == 0 ||
			strcmp (ptr, "shared_lib") == 0 ||
			strcmp (ptr, "program") == 0 ||
			strcmp (ptr, "configure_generated_file") == 0)
		{
			IAnjutaFileLoader *loader;
			gchar *uri;
			const gchar *project_root;
			
			anjuta_shell_get (ANJUTA_PLUGIN (plugin)->shell,
							  "project_root_uri", G_TYPE_STRING,
							  &project_root, NULL);
			uri = g_build_filename (project_root, target, NULL);
			loader = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
												 IAnjutaFileLoader, NULL);
			if (loader)
				ianjuta_file_loader_load (loader, uri, FALSE, NULL);
			g_free (uri);
		}
	}
	g_free (target);
}

static void
on_close_project (GtkAction *action, ProjectManagerPlugin *plugin)
{
	AnjutaStatus *status;
	
	if (plugin->project) {
		g_object_unref (plugin->project);
		plugin->project = NULL;
		g_object_set (G_OBJECT (plugin->model), "project", NULL, NULL);
		update_ui (plugin);
		anjuta_shell_remove_value (ANJUTA_PLUGIN (plugin)->shell,
								   "project_root_uri", NULL);
		status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell, NULL);
		anjuta_status_set_default (status, _("Project"), NULL);
	}
}

static GtkActionEntry pm_actions[] = 
{
	{
		"ActionMenuProject", NULL,
		N_("_Project"), NULL, NULL, NULL
	},
	{
		"ActionProjectCloseProject", GTK_STOCK_CLOSE,
		N_("Close Pro_ject"), NULL, N_("Close project"),
		G_CALLBACK (on_close_project)
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
		return;
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
		return;
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
		return;
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

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	GtkWidget *view, *scrolled_window;
	GbfProjectModel *model;
	static gboolean initialized = FALSE;
	GtkTreeSelection *selection;
	// GladeXML *gxml;
	ProjectManagerPlugin *pm_plugin;
	
	g_message ("ProjectManagerPlugin: Activating Project Manager plugin ...");
	
	if (!initialized)
		register_stock_icons (plugin);
	
	pm_plugin = (ProjectManagerPlugin*) plugin;
	pm_plugin->ui = anjuta_shell_get_ui (plugin->shell, NULL);
	pm_plugin->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);

	/* create model & view and bind them */
	model = gbf_project_model_new (NULL);
	/* We already get a ref on model */
	view = gbf_project_view_new ();
	g_object_ref (view);
	gtk_tree_view_set_model (GTK_TREE_VIEW (view),
							 GTK_TREE_MODEL (model));
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
	g_signal_connect (view, "uri-activated",
					  G_CALLBACK (on_uri_activated), plugin);
	g_signal_connect (view, "target-selected",
					  G_CALLBACK (on_target_activated), plugin);
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
											G_N_ELEMENTS(pm_actions), plugin);
	pm_plugin->popup_action_group = 
		anjuta_ui_add_action_group_entries (pm_plugin->ui,
											"ActionGroupProjectManagerPopup",
											_("Project manager popup actions"),
											popup_actions,
											G_N_ELEMENTS (popup_actions),
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
		anjuta_plugin_add_watch (plugin, "file_manager_current_uri",
								 value_added_fm_current_uri,
								 value_removed_fm_current_uri, NULL);
	pm_plugin->editor_watch_id = 
		anjuta_plugin_add_watch (plugin, "document_manager_current_editor",
								 value_added_current_editor,
								 value_removed_current_editor, NULL);
	initialized = TRUE;
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
	
	// pm_finalize(pm_plugin);
	anjuta_shell_remove_widget (plugin->shell, pm_plugin->scrolledwindow, NULL);
	anjuta_ui_unmerge (pm_plugin->ui, pm_plugin->merge_id);
	anjuta_ui_remove_action_group (pm_plugin->ui, pm_plugin->pm_action_group);
	anjuta_ui_remove_action_group (pm_plugin->ui, pm_plugin->popup_action_group);
	g_object_unref (G_OBJECT (pm_plugin->model));
	g_object_unref (G_OBJECT (pm_plugin->view));
	gtk_widget_destroy (pm_plugin->scrolledwindow);
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

static void
ifile_open (IAnjutaFile *project_manager,
			const gchar *filename, GError **err)
{
	AnjutaPlugin *plugin;
	ProjectManagerPlugin *pm_plugin;
	AnjutaStatus *status;
	GtkWidget *progress_win;
	GnomeVFSURI *vfs_uri;
	gchar *dirname, *vfs_dir;
	GSList *l;
	GValue *value;
	GError *error = NULL;
	GbfBackend *backend = NULL;
	
	g_return_if_fail (filename != NULL);
	
	plugin = ANJUTA_PLUGIN (project_manager);
	pm_plugin = (ProjectManagerPlugin*)(plugin);
	progress_win = show_loading_progress (plugin);
	
	vfs_uri = gnome_vfs_uri_new (filename);
	dirname = gnome_vfs_uri_extract_dirname (vfs_uri);
	gnome_vfs_uri_unref (vfs_uri);
	
	if (pm_plugin->project != NULL)
			g_object_unref (pm_plugin->project);
	
	DEBUG_PRINT ("initializing gbf backend...\n");
	gbf_backend_init ();
	
	for (l = gbf_backend_get_backends (); l; l = l->next) {
			backend = l->data;
			if (!strcmp (backend->id, "gbf-am:GbfAmProject"))
					break;
			backend = NULL;
	}
	
	if (!backend)
	{
			/* FIXME: Set err */
			g_warning ("no automake/autoconf backend available\n");
			g_free (dirname);
			gtk_widget_destroy (progress_win);
			return;
	}
	
	DEBUG_PRINT ("Creating new gbf-am project\n");
	pm_plugin->project = gbf_backend_new_project (backend->id);
	if (!pm_plugin->project)
	{
			/* FIXME: Set err */
			g_warning ("project creation failed\n");
			g_free (dirname);
			gtk_widget_destroy (progress_win);
			return;
	}
	
	status = anjuta_shell_get_status (plugin->shell, NULL);
	anjuta_status_push (status, _("Loading project: %s"), g_basename (dirname));
	anjuta_status_busy_push (status);
	
	DEBUG_PRINT ("loading project %s\n\n", dirname);
	/* FIXME: use the error parameter to determine if the project
	 * was loaded successfully */
	gbf_project_load (pm_plugin->project, dirname, &error);
	if (error)
	{
		GtkWindow *win;
		win = GTK_WINDOW (gtk_widget_get_toplevel (pm_plugin->scrolledwindow));
		anjuta_util_dialog_error (win, _("Failed to load project %s: %s"),
								  filename, error->message);
		g_propagate_error (err, error);
		g_object_unref (pm_plugin->project);
		pm_plugin->project = NULL;
		g_free (dirname);
		gtk_widget_destroy (progress_win);
		anjuta_status_pop (status);
		anjuta_status_busy_pop (status);
		return;
	}
	g_object_set (G_OBJECT (pm_plugin->model), "project",
				  pm_plugin->project, NULL);
	
	/* Set project root directory */
	vfs_dir = gnome_vfs_get_uri_from_local_path (dirname);

	value = g_new0 (GValue, 1);
	g_value_init (value, G_TYPE_STRING);
	g_value_take_string (value, vfs_dir);
	
	update_ui (pm_plugin);
	anjuta_shell_present_widget (ANJUTA_PLUGIN (pm_plugin)->shell,
								 pm_plugin->scrolledwindow,
								 NULL);
	
	anjuta_shell_add_value (ANJUTA_PLUGIN(pm_plugin)->shell,
							"project_root_uri",
							value, NULL);
	gtk_widget_destroy (progress_win);
	anjuta_status_set_default (status, _("Project"), g_basename (dirname));
	anjuta_status_pop (status);
	anjuta_status_busy_pop (status);
	g_free (dirname);
}

static void
iproject_manager_configure (IAnjutaProjectManager *project_manager,
							GError **err)
{
}

#if 0
static gchar*
iproject_manager_get_selected (IAnjutaProjectManager *file_manager, GError **err)
{
	return pm_get_selected_file_path((ProjectManagerPlugin*) file_manager);
}
#endif

static void
ifile_iface_init(IAnjutaFileIface *iface)
{
	iface->open = ifile_open;
}

static void
iproject_manager_iface_init(IAnjutaProjectManagerIface *iface)
{
	iface->configure = iproject_manager_configure;
}

ANJUTA_PLUGIN_BEGIN (ProjectManagerPlugin, project_manager_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE (ifile, IANJUTA_TYPE_FILE);
ANJUTA_PLUGIN_ADD_INTERFACE (iproject_manager, IANJUTA_TYPE_PROJECT_MANAGER);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (ProjectManagerPlugin, project_manager_plugin);
