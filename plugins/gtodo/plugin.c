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
#include <libanjuta/resources.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-todo.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>

//#include <libgtodo/main.h>
#include "main.h"
#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-gtodo.ui"
#define ICON_FILE PACKAGE_PIXMAPS_DIR"/anjuta-gtodo-plugin.png"

static gpointer parent_class;

static void
on_hide_completed_action_activate (GtkAction *action, GTodoPlugin *plugin)
{
	gboolean state;
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	gtodo_set_hide_done (state);
}

static void
on_hide_due_date_action_activate (GtkAction *action, GTodoPlugin *plugin)
{
	gboolean state;
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	gtodo_set_hide_due (state);
}

static void
on_hide_end_date_action_activate (GtkAction *action, GTodoPlugin *plugin)
{
	gboolean state;
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	gtodo_set_hide_nodate (state);
}

static GtkActionEntry actions_todo_view[] = {
	{
		"ActionMenuViewTodo",
		NULL,
		N_("_Tasks"),
		NULL, NULL, NULL,
	},
};

static GtkToggleActionEntry actions_view[] = {
	{
		"ActionViewTodoHideCompleted",
		NULL,
		N_("Hide _Completed Items"),
		NULL,
		N_("Hide completed todo items"),
		G_CALLBACK (on_hide_completed_action_activate),
		FALSE
	},
	{
		"ActionViewTodoHideDueDate",
		NULL,
		N_("Hide Items Past _Due Date"),
		NULL,
		N_("Hide items that are past due date"),
		G_CALLBACK (on_hide_due_date_action_activate),
		FALSE
	},
	{
		"ActionViewTodoHideEndDate",
		NULL,
		N_("Hide Items Without _End Date"),
		NULL,
		N_("Hide items without an end date"),
		G_CALLBACK (on_hide_end_date_action_activate),
		FALSE
	}
};

static void
project_root_added (AnjutaPlugin *plugin, const gchar *name,
					const GValue *value, gpointer user_data)
{
	const gchar *root_uri;

	root_uri = g_value_get_string (value);
	
	if (root_uri)
	{
		gchar *todo_file;
		
		todo_file = g_strconcat (root_uri, "/TODO.tasks", NULL);
		gtodo_client_load (cl, todo_file);
		g_free (todo_file);
		category_changed();
	}
}

static void
project_root_removed (AnjutaPlugin *plugin, const gchar *name,
					  gpointer user_data)
{
	const gchar * home;
	gchar *default_todo;
	
	home = g_get_home_dir ();
	default_todo = g_strconcat ("file://", home, "/.gtodo/todos", NULL);
	
	gtodo_client_load (cl, default_todo);
	category_changed();
	g_free (default_todo);
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	GtkWidget *wid;
	AnjutaUI *ui;
	AnjutaPreferences *prefs;
	GTodoPlugin *gtodo_plugin;
	static gboolean initialized;
	
	DEBUG_PRINT ("GTodoPlugin: Activating Task manager plugin ...");
	gtodo_plugin = (GTodoPlugin*) plugin;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	
	if (!initialized)
	{
		gtodo_load_settings();
	}
	wid = gui_create_todo_widget();
	gtk_widget_show_all (wid);
	gtodo_plugin->widget = wid;
	
	/* Add all our editor actions */
	gtodo_plugin->action_group = 
		anjuta_ui_add_action_group_entries (ui, "ActionGroupTodoView",
											_("Tasks manager"),
											actions_todo_view,
											G_N_ELEMENTS (actions_todo_view),
											GETTEXT_PACKAGE, FALSE, plugin);
	gtodo_plugin->action_group2 = 
		anjuta_ui_add_toggle_action_group_entries (ui, "ActionGroupTodoViewOps",
												_("Tasks manager view"),
												actions_view,
												G_N_ELEMENTS (actions_view),
												GETTEXT_PACKAGE, TRUE, plugin);
	gtodo_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);
	anjuta_shell_add_widget (plugin->shell, wid,
							 "AnjutaTodoPlugin", _("Tasks"),
							 "gtodo", /* Icon stock */
							 ANJUTA_SHELL_PLACEMENT_CENTER, NULL);
	/* set up project directory watch */
	gtodo_plugin->root_watch_id = anjuta_plugin_add_watch (plugin,
													"project_root_uri",
													project_root_added,
													project_root_removed, NULL);
	initialized = TRUE;													
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	GTodoPlugin *gplugin = (GTodoPlugin*)plugin;
	AnjutaUI *ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	DEBUG_PRINT ("GTodoPlugin: Dectivating Tasks manager plugin ...");
	
	anjuta_plugin_remove_watch (plugin, gplugin->root_watch_id, TRUE);
	
	/* Container holds the last ref to this widget so it will be destroyed as
	 * soon as removed. No need to separately destroy it. */
	anjuta_shell_remove_widget (plugin->shell, gplugin->widget,
								NULL);
	anjuta_ui_unmerge (ui, gplugin->uiid);
	anjuta_ui_remove_action_group (ui, gplugin->action_group2);
	anjuta_ui_remove_action_group (ui, gplugin->action_group);
	
	
	gplugin->uiid = 0;
	gplugin->widget = NULL;
	gplugin->root_watch_id = 0;
	gplugin->action_group = NULL;
	gplugin->action_group2 = NULL;
	return TRUE;
}

static void
finalize (GObject *obj)
{
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
dispose (GObject *obj)
{
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
gtodo_plugin_instance_init (GObject *obj)
{
	GTodoPlugin *plugin = (GTodoPlugin*)obj;
	plugin->uiid = 0;
}

static void
gtodo_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
	klass->finalize = finalize;
}

static void
itodo_load (IAnjutaTodo *profile, const gchar *filename, GError **err)
{
	g_return_if_fail (filename != NULL);
	gtodo_client_load (cl, filename);
}

static void
itodo_iface_init(IAnjutaTodoIface *iface)
{
	iface->load = itodo_load;
}

static void
ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	/* Add preferences */
	GTodoPlugin* gtodo_plugin = (GTodoPlugin*)(ipref); 
	GdkPixbuf *pixbuf;
	
	pixbuf = gdk_pixbuf_new_from_file (ICON_FILE, NULL);
	gtodo_plugin->prefs = preferences_widget();
	anjuta_preferences_dialog_add_page (ANJUTA_PREFERENCES_DIALOG (prefs), "GTodo",
											_("Todo Manager"),
											pixbuf, gtodo_plugin->prefs);
	g_object_unref (pixbuf);
}

static void
ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	preferences_remove_signals();
	anjuta_preferences_dialog_remove_page(ANJUTA_PREFERENCES_DIALOG(prefs), _("Todo Manager"));
}


static void
ipreferences_iface_init(IAnjutaPreferencesIface* iface)
{
	iface->merge = ipreferences_merge;
	iface->unmerge = ipreferences_unmerge;	
}


ANJUTA_PLUGIN_BEGIN (GTodoPlugin, gtodo_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE (itodo, IANJUTA_TYPE_TODO);
ANJUTA_PLUGIN_ADD_INTERFACE(ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (GTodoPlugin, gtodo_plugin);
