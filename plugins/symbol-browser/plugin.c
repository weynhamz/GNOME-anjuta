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
#include <libanjuta/plugins.h>

#include "plugin.h"
#include "an_symbol_view.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-symbol-browser-plugin.ui"
#define PREFS_GLADE PACKAGE_DATA_DIR"/glade/anjuta-symbol-browser-plugin.glade"
#define ICON_FILE "anjuta-symbol-browser-plugin.png"

static gpointer parent_class = NULL;

static void treeview_signals_block (SymbolBrowserPlugin *sv_plugin);
static void treeview_signals_unblock (SymbolBrowserPlugin *sv_plugin);

static void
goto_file_line (const gchar *file, gint line)
{
	g_warning ("TODO: Goto file line unimplemented");
}

static void
on_goto_def_activate (GtkAction *action, SymbolBrowserPlugin *sv_plugin)
{
	const gchar *file;
	gint line;
	gboolean ret;
	
	ret = anjuta_symbol_view_get_current_symbol_def (ANJUTA_SYMBOL_VIEW (sv_plugin->sv),
													 &file, &line);
	if (ret)
	{
		goto_file_line (file, line);
	}
}

static void
on_goto_decl_activate (GtkAction *action, SymbolBrowserPlugin *sv_plugin)
{
	const gchar *file;
	gint line;
	gboolean ret;
	
	ret = anjuta_symbol_view_get_current_symbol_decl (ANJUTA_SYMBOL_VIEW (sv_plugin->sv),
													  &file, &line);
	if (ret)
	{
		goto_file_line (file, line);
	}
}

static void
on_find_activate (GtkAction *action, SymbolBrowserPlugin *sv_plugin)
{
	const gchar *symbol;
	symbol = anjuta_symbol_view_get_current_symbol (ANJUTA_SYMBOL_VIEW (sv_plugin->sv));
	if (symbol)
	{
		g_warning ("TODO: Unimplemented");
	}
}

static void
on_refresh_activate (GtkAction *action, SymbolBrowserPlugin *sv_plugin)
{
	anjuta_symbol_view_update (ANJUTA_SYMBOL_VIEW (sv_plugin->sv));
}

static GtkActionEntry popup_actions[] = 
{
	{
		"ActionPopupSymbolBrowserGotoDef",
		NULL,
		N_("Goto _Definition"),
		NULL,
		N_("Goto symbol definition"),
		G_CALLBACK (on_goto_def_activate)
	},
	{
		"ActionPopupSymbolBrowserGotoDecl",
		NULL,
		N_("Goto De_claration"),
		NULL,
		N_("Goto symbol declaration"),
		G_CALLBACK (on_goto_decl_activate)
	},
	{
		"ActionPopupSymbolBrowserFind",
		GTK_STOCK_FIND,
		N_("_Find Usage"),
		NULL,
		N_("Find usage of symbol in project"),
		G_CALLBACK (on_find_activate)
	},
	{
		"ActionPopupSymbolBrowserRefresh",
		GTK_STOCK_REFRESH,
		N_("_Refresh"),
		NULL,
		N_("Refresh symbol browser tree"),
		G_CALLBACK (on_refresh_activate)
	}
};

static void
project_root_added (AnjutaPlugin *plugin, const gchar *name,
					const GValue *value, gpointer user_data)
{
	SymbolBrowserPlugin *sv_plugin;
	const gchar *root_uri;

	sv_plugin = (SymbolBrowserPlugin *)plugin;
	root_uri = g_value_get_string (value);
	if (root_uri)
	{
		gchar *root_dir = gnome_vfs_get_local_path_from_uri (root_uri);
		if (root_dir)
		{
			treeview_signals_block (sv_plugin);
			anjuta_symbol_view_open (ANJUTA_SYMBOL_VIEW (sv_plugin->sv),
									 root_dir);
			treeview_signals_unblock (sv_plugin);
		}
	}
}

static void
project_root_removed (AnjutaPlugin *plugin, const gchar *name,
					  gpointer user_data)
{
	SymbolBrowserPlugin *sv_plugin;
	
	sv_plugin = (SymbolBrowserPlugin *)plugin;
	anjuta_symbol_view_clear (ANJUTA_SYMBOL_VIEW (sv_plugin->sv));
}

static gboolean
on_treeview_event (GtkWidget *widget,
				   GdkEvent  *event,
				   SymbolBrowserPlugin *sv_plugin)
{
	GtkTreeView *view;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	
	g_return_val_if_fail (GTK_IS_TREE_VIEW (widget), FALSE);

	view = GTK_TREE_VIEW (widget);
	model = gtk_tree_view_get_model (view);
	selection = gtk_tree_view_get_selection (view);

	if (!gtk_tree_selection_get_selected (selection, NULL, &iter) || !event)
		return FALSE;

	if (event->type == GDK_BUTTON_PRESS) {
		GdkEventButton *e = (GdkEventButton *) event;

		if (e->button == 3) {
			GtkWidget *menu;
			
			/* Popup project menu */
			menu = gtk_ui_manager_get_widget (GTK_UI_MANAGER (sv_plugin->ui),
											  "/PopupSymbolBrowser");
			gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
							NULL, NULL, e->button, e->time);
			return TRUE;
		}
	} else if (event->type == GDK_KEY_PRESS) {
		GtkTreePath *path;
		GdkEventKey *e = (GdkEventKey *) event;

		switch (e->keyval) {
			case GDK_Return:
				anjuta_ui_activate_action_by_group (sv_plugin->ui,
													sv_plugin->action_group,
											"ActionPopupSymbolBrowserGotoDef");
				return TRUE;
			case GDK_Left:
				if (gtk_tree_model_iter_has_child (model, &iter))
				{
					path = gtk_tree_model_get_path (model, &iter);
					gtk_tree_view_collapse_row (GTK_TREE_VIEW (view),
												path);
					gtk_tree_path_free (path);
					return TRUE;
				}
			case GDK_Right:
				if (gtk_tree_model_iter_has_child (model, &iter))
				{
					path = gtk_tree_model_get_path (model, &iter);
					gtk_tree_view_expand_row (GTK_TREE_VIEW (view),
											  path, FALSE);
					gtk_tree_path_free (path);
					return TRUE;
				}
			default:
				return FALSE;
		}
	}
	return FALSE;
}

static void
on_treeview_row_activated (GtkTreeView *view, GtkTreePath *arg1,
						   GtkTreeViewColumn *arg2,
						   SymbolBrowserPlugin *sv_plugin)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	
	selection = gtk_tree_view_get_selection (view);
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;
	anjuta_ui_activate_action_by_group (sv_plugin->ui,
										sv_plugin->action_group,
										"ActionPopupSymbolBrowserGotoDef");
}

static void
treeview_signals_block (SymbolBrowserPlugin *sv_plugin)
{
	g_signal_handlers_block_by_func (G_OBJECT (sv_plugin->sv),
									 G_CALLBACK (on_treeview_event), NULL);
}

static void
treeview_signals_unblock (SymbolBrowserPlugin *sv_plugin)
{
	g_signal_handlers_block_by_func (G_OBJECT (sv_plugin->sv),
									 G_CALLBACK (on_treeview_event), NULL);
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	SymbolBrowserPlugin *sv_plugin;
	
	g_message ("SymbolBrowserPlugin: Activating File Manager plugin ...");
	sv_plugin = (SymbolBrowserPlugin*) plugin;
	sv_plugin->ui = anjuta_shell_get_ui (plugin->shell, NULL);
	sv_plugin->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);

	/* Add action group */
	sv_plugin->action_group = 
		anjuta_ui_add_action_group_entries (sv_plugin->ui,
											"ActionGroupSymbolBrowser",
											_("Symbol browser popup actions"),
											popup_actions,
											G_N_ELEMENTS (popup_actions),
											plugin);
	/* Add UI */
	sv_plugin->merge_id = 
		anjuta_ui_merge (sv_plugin->ui, UI_FILE);
	
	/* Added widgets */
	anjuta_shell_add_widget (plugin->shell, sv_plugin->sw,
							 "AnjutaSymbolBrowser", _("Symbols"),
							 GTK_STOCK_OPEN,
							 ANJUTA_SHELL_PLACEMENT_LEFT, NULL);
	
	/* set up project directory watch */
	sv_plugin->root_watch_id = anjuta_plugin_add_watch (plugin,
									"project_root_uri",
									project_root_added,
									project_root_removed, NULL);
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	SymbolBrowserPlugin *sv_plugin;
	sv_plugin = (SymbolBrowserPlugin*) plugin;
	
	/* Remove watches */
	anjuta_plugin_remove_watch (plugin, sv_plugin->root_watch_id, FALSE);
	
	/* Remove widgets */
	anjuta_shell_remove_widget (plugin->shell, sv_plugin->sw, NULL);
	
	/* Remove UI */
	anjuta_ui_unmerge (sv_plugin->ui, sv_plugin->merge_id);
	
	/* Remove action group */
	anjuta_ui_remove_action_group (sv_plugin->ui, sv_plugin->action_group);
	
	sv_plugin->root_watch_id = 0;
	return TRUE;
}

static void
dispose (GObject *obj)
{
	SymbolBrowserPlugin *plugin = (SymbolBrowserPlugin*) obj;
	if (plugin->sv)
	{
		g_object_ref (G_OBJECT (plugin->sv));
		plugin->sv = NULL;
	}
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
finalize (GObject *obj)
{
	SymbolBrowserPlugin *plugin = (SymbolBrowserPlugin *) obj;
	if (!anjuta_plugin_is_active (ANJUTA_PLUGIN (obj)))
	{
		/* widget is no longer in a container */
		gtk_widget_destroy (plugin->sw);
	}
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
symbol_browser_plugin_instance_init (GObject *obj)
{
	SymbolBrowserPlugin *plugin = (SymbolBrowserPlugin*) obj;
	plugin->sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (plugin->sw),
										 GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (plugin->sw),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	
	plugin->sv = anjuta_symbol_view_new ();
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (plugin->sv), FALSE);
	gtk_container_add (GTK_CONTAINER (plugin->sw), plugin->sv);
	
	g_object_ref (G_OBJECT (plugin->sv));
	g_signal_connect (G_OBJECT (plugin->sv), "event-after",
					  G_CALLBACK (on_treeview_event), plugin);
	g_signal_connect (G_OBJECT (plugin->sv), "row_activated",
					  G_CALLBACK (on_treeview_row_activated), plugin);

	gtk_widget_show_all (plugin->sw);
}

static void
symbol_browser_plugin_class_init (SymbolBrowserPluginClass *klass)
{
	GObjectClass *object_class;
	AnjutaPluginClass *plugin_class;
	
	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;
	plugin_class = (AnjutaPluginClass*) klass;
	
	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	
	object_class->dispose = dispose;
	object_class->finalize = finalize;
}

ANJUTA_PLUGIN_BEGIN (SymbolBrowserPlugin, symbol_browser_plugin);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (SymbolBrowserPlugin, symbol_browser_plugin);
