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
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <gbf/gbf-project-util.h>
#include <gbf/gbf-backend.h>

#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-project-manager.ui"
#define PREFS_GLADE PACKAGE_DATA_DIR"/glade/anjuta-project-manager-plugin.glade"
#define ICON_FILE "anjuta-project-manager-plugin.png"

gpointer parent_class;

static void update_ui (ProjectManagerPlugin *plugin);

static void refresh (GtkAction *action, ProjectManagerPlugin *plugin)
{
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
								 GTK_WINDOW (win), NULL, NULL);
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
on_target_selected (GtkWidget *widget, const gchar *target_id,
					ProjectManagerPlugin *plugin)
{
#if 0
	GList *list;
	
	g_message ("Target selected: %s", target_id);
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
}

static void
on_close_project (GtkAction *action, ProjectManagerPlugin *plugin)
{
	if (plugin->project) {
		g_object_unref (plugin->project);
		plugin->project = NULL;
		g_object_set (G_OBJECT (plugin->model), "project", NULL, NULL);
		update_ui (plugin);
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
		"ActionProjectAddGroup", NULL,
		N_("Add Targets _Group"), NULL, N_("Add a targets group to project"),
		G_CALLBACK (on_add_group)
	},
	{
		"ActionProjectAddTarget", NULL,
		N_("Add Build _Target"), NULL, N_("Add a target to project"),
		G_CALLBACK (on_add_target)
	},
	{
		"ActionProjectAddSource", NULL,
		N_("Add _Source File"), NULL, N_("Add a source file to project"),
		G_CALLBACK (on_add_source)
	}
};

static GtkActionEntry popup_actions[] = 
{
	{
		"ActionPopupProjectManagerRefresh", GTK_STOCK_REFRESH,
		N_("_Refresh"), NULL, N_("Refresh project manager tree"),
		G_CALLBACK (refresh)
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

#if 0
static void
preferences_changed (AnjutaPreferences *prefs, ProjectManagerPlugin *fv)
{
	gchar* root = anjuta_preferences_get(prefs, "root.dir");
	if (root)
	{
		pm_set_root (fv, root);
	}
	else
		pm_set_root (fv, "/");
	pm_refresh (fv);
}
#endif

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


static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	GtkWidget *view, *scrolled_window;
	GbfProjectModel *model;
	
	// GladeXML *gxml;
	ProjectManagerPlugin *pm_plugin;
	
	g_message ("ProjectManagerPlugin: Activating Project Manager plugin ...");
	
	register_stock_icons (plugin);
	
	pm_plugin = (ProjectManagerPlugin*) plugin;
	pm_plugin->ui = anjuta_shell_get_ui (plugin->shell, NULL);
	pm_plugin->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);

	/* create model & view and bind them */
	model = gbf_project_model_new (NULL);
	view = gbf_project_view_new ();
	gtk_tree_view_set_model (GTK_TREE_VIEW (view),
							 GTK_TREE_MODEL (model));
	g_object_unref (model);
	g_signal_connect (view, "uri-activated",
					  G_CALLBACK (on_uri_activated), plugin);
	g_signal_connect (view, "target-selected",
					  G_CALLBACK (on_target_selected), plugin);
	
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
	g_signal_connect (G_OBJECT (pm_plugin->prefs), "changed",
					  G_CALLBACK (preferences_changed), pm_plugin);
	g_object_unref (G_OBJECT (gxml));
#endif	
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	ProjectManagerPlugin *pm_plugin;
	pm_plugin = (ProjectManagerPlugin*) plugin;
	
	// g_signal_handlers_disconnect_by_func (G_OBJECT (pm_plugin->prefs),
	//									  G_CALLBACK (preferences_changed),
	//									  pm_plugin);
	// pm_finalize(pm_plugin);
	anjuta_ui_unmerge (pm_plugin->ui, pm_plugin->merge_id);
	anjuta_ui_remove_action_group (pm_plugin->ui, pm_plugin->pm_action_group);
	anjuta_ui_remove_action_group (pm_plugin->ui, pm_plugin->popup_action_group);
	return TRUE;
}

static void
dispose (GObject *obj)
{
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
	klass->dispose = dispose;
}

static void
ifile_open (IAnjutaFile *project_manager,
			const gchar *filename, GError **err)
{
	GnomeVFSURI *vfs_uri;
	gchar *dirname, *vfs_dir;
	GSList *l;
	GValue *value;
	
	GbfBackend *backend = NULL;
	ProjectManagerPlugin *pm_plugin;
	
	g_return_if_fail (filename != NULL);
	
	pm_plugin = (ProjectManagerPlugin*)(ANJUTA_PLUGIN (project_manager));
	
	vfs_uri = gnome_vfs_uri_new (filename);
	dirname = gnome_vfs_uri_extract_dirname (vfs_uri);
	gnome_vfs_uri_unref (vfs_uri);
	
	if (pm_plugin->project != NULL)
			g_object_unref (pm_plugin->project);
	
	g_print ("initializing gbf backend...\n");
	gbf_backend_init ();
	
	for (l = gbf_backend_get_backends (); l; l = l->next) {
			backend = l->data;
			if (!strcmp (backend->id, "gbf-am:GbfAmProject"))
					break;
			backend = NULL;
	}
	
	if (!backend) {
			g_print ("no automake/autoconf backend available\n");
			return;
	}
	
	g_print ("creating new gbf-am project\n");
	pm_plugin->project = gbf_backend_new_project (backend->id);
	if (!pm_plugin->project) {
			g_print ("project creation failed\n");
			return;
	}
	
	g_print ("loading project %s\n\n", dirname);
	/* FIXME: use the error parameter to determine if the project
	 * was loaded successfully */
	gbf_project_load (pm_plugin->project, dirname, NULL);
	
	g_object_set (G_OBJECT (pm_plugin->model), "project",
				  pm_plugin->project, NULL);
	
	/* Set project root directory */
	vfs_dir = gnome_vfs_get_uri_from_local_path (dirname);
	g_free (dirname);

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
ANJUTA_INTERFACE (ifile, IANJUTA_TYPE_FILE);
ANJUTA_INTERFACE (iproject_manager, IANJUTA_TYPE_PROJECT_MANAGER);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (ProjectManagerPlugin, project_manager_plugin);
