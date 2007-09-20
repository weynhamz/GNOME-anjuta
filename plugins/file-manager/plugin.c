/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.c
 * Copyright (C) Johannes Schmid 2007 <jhs@gnome.org>
 * 
 * plugin.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * plugin.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.c.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include <config.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>
#include <libanjuta/interfaces/ianjuta-file-manager.h>
#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/file-manager.ui"

static gpointer parent_class;

typedef struct _ScrollPosition ScrollPosition;
struct _ScrollPosition
{
	GtkWidget* window;
	gdouble hvalue;
	gdouble vvalue;
};

static void
on_file_manager_refresh (GtkAction* action, AnjutaFileManager* file_manager)
{	
	file_view_refresh (file_manager->fv, TRUE);
}

static GtkActionEntry popup_actions[] = 
{
	{
		"ActionPopupFileManagerRefresh", GTK_STOCK_REFRESH,
		N_("_Refresh"), NULL, N_("Refresh file manager tree"),
		G_CALLBACK (on_file_manager_refresh)
	}
};

static void
file_manager_set_default_uri (AnjutaFileManager* file_manager)
{
	const gchar* home_dir = g_get_home_dir();
	gchar* home_uri = g_strconcat ("file://", home_dir, NULL);
	g_object_set (G_OBJECT (file_manager->fv), "base_uri", home_uri, NULL);
	g_free (home_uri);
}

static void
project_root_added (AnjutaPlugin *plugin, const gchar *name,
					const GValue *value, gpointer user_data)
{
	AnjutaFileManager* file_manager;
	const gchar *root_uri;

	file_manager = (AnjutaFileManager*) plugin;
	root_uri = g_value_get_string (value);
	if (root_uri)
	{
		g_object_set (G_OBJECT(file_manager->fv), "base_uri", root_uri, NULL);
		file_view_refresh (file_manager->fv, FALSE);
	}
	else
	{
		file_manager_set_default_uri (file_manager);
		file_view_refresh (file_manager->fv, FALSE);
	}
}

static void
project_root_removed (AnjutaPlugin *plugin, const gchar *name,
					  gpointer user_data)
{
	AnjutaFileManager* file_manager = (AnjutaFileManager*) plugin;
	file_manager_set_default_uri (file_manager);
	file_view_refresh (file_manager->fv, FALSE);
}

static void
on_file_view_current_uri_changed (AnjutaFileView* view, gchar* uri,
								  AnjutaFileManager* file_manager)
{
	if (uri)
	{
		GValue* value;
		value = g_new0 (GValue, 1);
		g_value_init (value, G_TYPE_STRING);
		g_value_take_string (value, uri);
		anjuta_shell_add_value (ANJUTA_PLUGIN (file_manager)->shell,
								"file_manager_current_uri", value, NULL);
	}
	else
	{
		anjuta_shell_remove_value (ANJUTA_PLUGIN(file_manager)->shell,
								   "file_manager_current_uri", NULL);		
	}
}

static void
on_file_view_open_file (AnjutaFileView* view, const char *uri,
						AnjutaFileManager* file_manager)
{
	GObject *obj;
	
	IAnjutaFileLoader *loader;
	g_return_if_fail (uri != NULL);
	loader = anjuta_shell_get_interface (ANJUTA_PLUGIN (file_manager)->shell,
										 IAnjutaFileLoader, NULL);
	g_return_if_fail (loader != NULL);
		
	ianjuta_file_loader_load (loader, uri, FALSE, NULL);
}

static void
on_file_view_show_popup_menu (AnjutaFileView* view, const gchar* uri,
							  gboolean is_dir,guint button,
							  guint32 time, AnjutaFileManager* file_manager)
{
	GtkWidget *popup;
	AnjutaUI* ui = anjuta_shell_get_ui (ANJUTA_PLUGIN(file_manager)->shell, 
										NULL);
	popup = gtk_ui_manager_get_widget (GTK_UI_MANAGER (ui),
									   "/PopupFileManager");
	g_return_if_fail (GTK_IS_WIDGET (popup));
	gtk_menu_popup (GTK_MENU (popup), NULL, NULL, NULL, NULL, button, time);
}

static gboolean
file_manager_activate (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	AnjutaFileManager *file_manager;
	
	DEBUG_PRINT ("AnjutaFileManager: Activating AnjutaFileManager plugin ...");
	file_manager = (AnjutaFileManager*) plugin;

	/* Add all UI actions and merge UI */
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	/* Add action group */
	file_manager->action_group = 
		anjuta_ui_add_action_group_entries (ui, "ActionGroupFileManager",
											_("File manager popup actions"),
											popup_actions, 1,
											GETTEXT_PACKAGE, FALSE,
											plugin);
	
	file_manager->uiid = anjuta_ui_merge (ui, UI_FILE);
	
	file_manager->sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (file_manager->sw),
								    GTK_POLICY_AUTOMATIC,
								    GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (file_manager->sw),
										 GTK_SHADOW_IN);
	
	file_manager->fv = ANJUTA_FILE_VIEW (file_view_new ());
	
	g_signal_connect (G_OBJECT (file_manager->fv), "file-open",
					  G_CALLBACK (on_file_view_open_file), file_manager);
	g_signal_connect (G_OBJECT(file_manager->fv), "show-popup-menu",
					  G_CALLBACK (on_file_view_show_popup_menu), file_manager);
	g_signal_connect (G_OBJECT(file_manager->fv), "current-uri-changed",
					  G_CALLBACK (on_file_view_current_uri_changed),
					  file_manager);
	file_manager_set_default_uri (file_manager);
	file_view_refresh (file_manager->fv, FALSE);
	
	gtk_container_add (GTK_CONTAINER (file_manager->sw), 
					   GTK_WIDGET (file_manager->fv));
	
	gtk_widget_show_all (file_manager->sw);
	
	anjuta_shell_add_widget (plugin->shell, file_manager->sw, "file-manager",
							 _("Files"), GTK_STOCK_OPEN,
							 ANJUTA_SHELL_PLACEMENT_LEFT, NULL);
	
	file_manager->root_watch_id =
		anjuta_plugin_add_watch (plugin, "project_root_uri",
								 project_root_added,
								 project_root_removed, NULL);
	
	return TRUE;
}

static gboolean
file_manager_deactivate (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	AnjutaFileManager *file_manager;

	DEBUG_PRINT ("AnjutaFileManager: Dectivating AnjutaFileManager plugin ...");
	
	file_manager = (AnjutaFileManager*) plugin;
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	anjuta_plugin_remove_watch (plugin, file_manager->root_watch_id, TRUE);
	anjuta_ui_remove_action_group (ui, ((AnjutaFileManager*)plugin)->action_group);
	anjuta_ui_unmerge (ui, ((AnjutaFileManager*)plugin)->uiid);
	
	anjuta_shell_remove_widget (plugin->shell, file_manager->sw, NULL);
	return TRUE;
}

static void
file_manager_finalize (GObject *obj)
{
	/* Finalization codes here */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
file_manager_dispose (GObject *obj)
{
	/* Disposition codes */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
file_manager_instance_init (GObject *obj)
{
	AnjutaFileManager *plugin = (AnjutaFileManager*)obj;

	plugin->uiid = 0;
}

static void
file_manager_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = file_manager_activate;
	plugin_class->deactivate = file_manager_deactivate;
	
	klass->finalize = file_manager_finalize;
	klass->dispose = file_manager_dispose;
}

static void
ifile_manager_set_root (IAnjutaFileManager *ifile_manager, const gchar *root,
						GError **err)
{
	AnjutaFileManager* file_manager = (AnjutaFileManager*) ifile_manager;
	g_object_set (G_OBJECT(file_manager->fv), "base_uri", root, NULL);
}

static void
ifile_manager_set_selected (IAnjutaFileManager *file_manager,
							const gchar *root, GError **err)
{
	/* TODO */
}

static gchar*
ifile_manager_get_selected (IAnjutaFileManager *ifile_manager, GError **err)
{
	AnjutaFileManager* file_manager = (AnjutaFileManager*) ifile_manager;
	return file_view_get_selected (file_manager->fv);
}

static void
ifile_manager_iface_init (IAnjutaFileManagerIface *iface)
{
	iface->set_root = ifile_manager_set_root;
	iface->get_selected = ifile_manager_get_selected;
	iface->set_selected = ifile_manager_set_selected;
}

ANJUTA_PLUGIN_BEGIN (AnjutaFileManager, file_manager);
ANJUTA_PLUGIN_ADD_INTERFACE (ifile_manager, IANJUTA_TYPE_FILE_MANAGER);
ANJUTA_PLUGIN_END

ANJUTA_SIMPLE_PLUGIN (AnjutaFileManager, file_manager);
