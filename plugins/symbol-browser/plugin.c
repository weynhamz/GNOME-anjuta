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
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-help.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>
#include <libanjuta/interfaces/ianjuta-file-manager.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-markable.h>

#include <libanjuta/plugins.h>
#include <libegg/menu/egg-combo-action.h>
#include "plugin.h"
#include "an_symbol_view.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-symbol-browser-plugin.ui"
#define PREFS_GLADE PACKAGE_DATA_DIR"/glade/anjuta-symbol-browser-plugin.glade"
#define ICON_FILE "anjuta-symbol-browser-plugin.png"

static gpointer parent_class = NULL;

static void treeview_signals_block (SymbolBrowserPlugin *sv_plugin);
static void treeview_signals_unblock (SymbolBrowserPlugin *sv_plugin);

static void
goto_file_line (AnjutaPlugin *plugin, const gchar *filename, gint lineno)
{
	gchar *uri;
	IAnjutaFileLoader *loader;
	
	g_return_if_fail (filename != NULL);
		
	/* Go to file and line number */
	loader = anjuta_shell_get_interface (plugin->shell, IAnjutaFileLoader,
										 NULL);
		
	uri = g_strdup_printf ("file:///%s#%d", filename, lineno);
	ianjuta_file_loader_load (loader, uri, FALSE, NULL);
	g_free (uri);
}

static void
goto_file_tag (SymbolBrowserPlugin *sv_plugin, const char *symbol,
			   gboolean prefer_definition)
{
	const gchar *file;
	gint line;
	gboolean ret;
	
	ret = anjuta_symbol_view_get_file_symbol (ANJUTA_SYMBOL_VIEW (sv_plugin->sv),
											  symbol, prefer_definition,
											  &file, &line);
	if (ret)
	{
		goto_file_line (ANJUTA_PLUGIN (sv_plugin), file, line);
	}
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
		goto_file_line (ANJUTA_PLUGIN (sv_plugin), file, line);
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
		goto_file_line (ANJUTA_PLUGIN (sv_plugin), file, line);
	}
}

static void
on_goto_file_tag_decl_activate (GtkAction * action,
								SymbolBrowserPlugin *sv_plugin)
{
	IAnjutaEditor *ed;
	gchar *word;

	if (sv_plugin->current_editor)
	{
		ed = IANJUTA_EDITOR (sv_plugin->current_editor);
		word = ianjuta_editor_get_current_word (ed, NULL);
		if (word)
		{
			goto_file_tag (sv_plugin, word, FALSE);
			g_free (word);
		}
	}
}

static void
on_goto_file_tag_def_activate (GtkAction * action,
								SymbolBrowserPlugin *sv_plugin)
{
	IAnjutaEditor *ed;
	gchar *word;

	if (sv_plugin->current_editor)
	{
		ed = IANJUTA_EDITOR (sv_plugin->current_editor);
		word = ianjuta_editor_get_current_word (ed, NULL);
		if (word)
		{
			goto_file_tag (sv_plugin, word, TRUE);
			g_free (word);
		}
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
	{ "ActionMenuGoto", NULL, N_("_Goto"), NULL, NULL, NULL},
	{
		"ActionSymbolBrowserGotoDef",
		NULL,
		N_("Tag _Definition"),
		"<control>d",
		N_("Goto symbol definition"),
		G_CALLBACK (on_goto_file_tag_def_activate)
	},
	{
		"ActionSymbolBrowserGotoDecl",
		NULL,
		N_("Tag De_claration"),
		"<shift><control>d",
		N_("Goto symbol declaration"),
		G_CALLBACK (on_goto_file_tag_decl_activate)
	},
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

static void
on_symbol_selected (GtkAction *action, SymbolBrowserPlugin *sv_plugin)
{
	GtkTreeIter iter;
	
	if (egg_combo_action_get_active_iter (EGG_COMBO_ACTION (action), &iter))
	{
		gint line;
		line = anjuta_symbol_view_workspace_get_line (ANJUTA_SYMBOL_VIEW
													  (sv_plugin->sv),
													  &iter);
		if (line > 0 && sv_plugin->current_editor)
		{
			/* Goto line number */
			ianjuta_editor_goto_line (IANJUTA_EDITOR (sv_plugin->current_editor),
									  line, NULL);
			if (IANJUTA_IS_MARKABLE (sv_plugin->current_editor))
			{
				ianjuta_markable_delete_all_markers (IANJUTA_MARKABLE (sv_plugin->current_editor),
								IANJUTA_MARKABLE_BASIC, NULL);
				ianjuta_markable_mark (IANJUTA_MARKABLE (sv_plugin->current_editor),
								line, IANJUTA_MARKABLE_BASIC, NULL);
			}
		}
	}
}

static void
on_editor_destroy (SymbolBrowserPlugin *sv_plugin, IAnjutaEditor *editor)
{
	const gchar *uri;
	
	if (!sv_plugin->editor_connected)
		return;
	uri = g_hash_table_lookup (sv_plugin->editor_connected, G_OBJECT (editor));
	if (uri && strlen (uri) > 0)
	{
		DEBUG_PRINT ("Removing file tags of %s", uri);
		anjuta_symbol_view_workspace_remove_file (ANJUTA_SYMBOL_VIEW (sv_plugin->sv),
											   uri);
	}
	g_hash_table_remove (sv_plugin->editor_connected, G_OBJECT (editor));
}

static void
on_editor_saved (IAnjutaEditor *editor, const gchar *saved_uri,
				 SymbolBrowserPlugin *sv_plugin)
{
	const gchar *old_uri;
	gboolean tags_update;
	GtkTreeModel *file_symbol_model;
	GtkAction *action;
	AnjutaUI *ui;
	
	/* FIXME: Do this only if automatic tags update is enabled */
	/* tags_update =
		anjuta_preferences_get_int (te->preferences, AUTOMATIC_TAGS_UPDATE);
	*/
	tags_update = TRUE;
	if (tags_update)
	{
		gchar *local_filename;
		
		/* Verify that it's local file */
		local_filename = gnome_vfs_get_local_path_from_uri (saved_uri);
		g_return_if_fail (local_filename != NULL);
		g_free (local_filename);
		
		if (!sv_plugin->editor_connected)
			return;
	
		old_uri = g_hash_table_lookup (sv_plugin->editor_connected, editor);
		
		if (old_uri && strlen (old_uri) <= 0)
			old_uri = NULL;
		anjuta_symbol_view_workspace_update_file (ANJUTA_SYMBOL_VIEW (sv_plugin->sv),
												  old_uri, saved_uri);
		g_hash_table_insert (sv_plugin->editor_connected, editor,
							 g_strdup (saved_uri));
		
		/* Update File symbol view */
		ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (sv_plugin)->shell, NULL);
		action = anjuta_ui_get_action (ui, "ActionGroupSymbolNavigation",
									   "ActionGotoSymbol");
		file_symbol_model =
			anjuta_symbol_view_get_file_symbol_model (ANJUTA_SYMBOL_VIEW (sv_plugin->sv));
		egg_combo_action_set_model (EGG_COMBO_ACTION (action), file_symbol_model);
		if (gtk_tree_model_iter_n_children (file_symbol_model, NULL) > 0)
			g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
		else
			g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);

#if 0
		/* FIXME: Re-hilite all editors on tags update */
		if(update)
		{
			for (tmp = app->text_editor_list; tmp; tmp = g_list_next(tmp))
			{
				te1 = (TextEditor *) tmp->data;
				text_editor_set_hilite_type(te1);
			}
		}
#endif
	}
}

static void
on_editor_foreach (gpointer key, gpointer value, gpointer user_data)
{
	const gchar *uri;
	SymbolBrowserPlugin *sv_plugin = (SymbolBrowserPlugin *)user_data;
	
	g_signal_handlers_disconnect_by_func (G_OBJECT(key),
										  G_CALLBACK (on_editor_saved),
										  user_data);
	g_object_weak_unref (G_OBJECT(key),
						 (GWeakNotify) (on_editor_destroy),
						 user_data);
	uri = (const gchar *)value;
	if (uri && strlen (uri) > 0)
	{
		DEBUG_PRINT ("Removing file tags of %s", uri);
		anjuta_symbol_view_workspace_remove_file (ANJUTA_SYMBOL_VIEW (sv_plugin->sv),
											   uri);
	}
}

static void
value_added_current_editor (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	AnjutaUI *ui;
	gchar *uri;
	GObject *editor;
	SymbolBrowserPlugin *sv_plugin;
	
	editor = g_value_get_object (value);
	
	sv_plugin = (SymbolBrowserPlugin*)plugin;
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	if (!sv_plugin->editor_connected)
	{
		sv_plugin->editor_connected = g_hash_table_new_full (g_direct_hash,
															 g_direct_equal,
															 NULL, g_free);
	}
	sv_plugin->current_editor = editor;
	
	uri = ianjuta_file_get_uri (IANJUTA_FILE (editor), NULL);
	if (uri)
	{
		gchar *local_filename;
		GtkTreeModel *file_symbol_model;
		GtkAction *action;
		
		/* Verify that it's local file */
		local_filename = gnome_vfs_get_local_path_from_uri (uri);
		g_return_if_fail (local_filename != NULL);
		g_free (local_filename);
		
		anjuta_symbol_view_workspace_add_file (ANJUTA_SYMBOL_VIEW (sv_plugin->sv),
											   uri);
		action = anjuta_ui_get_action (ui, "ActionGroupSymbolNavigation",
									   "ActionGotoSymbol");
		file_symbol_model =
			anjuta_symbol_view_get_file_symbol_model (ANJUTA_SYMBOL_VIEW (sv_plugin->sv));
		egg_combo_action_set_model (EGG_COMBO_ACTION (action), file_symbol_model);
		if (gtk_tree_model_iter_n_children (file_symbol_model, NULL) > 0)
			g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
		else
			g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
	}
	if (g_hash_table_lookup (sv_plugin->editor_connected, editor) == NULL)
	{
		g_object_weak_ref (G_OBJECT (editor),
						   (GWeakNotify) (on_editor_destroy),
						   sv_plugin);
		if (uri)
		{
			g_hash_table_insert (sv_plugin->editor_connected, editor,
								 g_strdup (uri));
		}
		else
		{
			g_hash_table_insert (sv_plugin->editor_connected, editor,
								 g_strdup (""));
		}
		g_signal_connect (G_OBJECT (editor), "saved",
						  G_CALLBACK (on_editor_saved),
						  sv_plugin);
	}
	if (uri)
		g_free (uri);
}

static void
value_removed_current_editor (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	AnjutaUI *ui;
	SymbolBrowserPlugin *sv_plugin;
	GtkAction *action;

	sv_plugin = (SymbolBrowserPlugin*)plugin;
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupSymbolNavigation",
								   "ActionGotoSymbol");
	g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
	sv_plugin->current_editor = NULL;
	
	/* FIXME: Signal should be dis-connected and symbols removed when
	the editor is destroyed */
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	GtkActionGroup *group;
	GtkAction *action;
	SymbolBrowserPlugin *sv_plugin;
	
	DEBUG_PRINT ("SymbolBrowserPlugin: Activating Symbol Manager plugin ...");
	sv_plugin = (SymbolBrowserPlugin*) plugin;
	sv_plugin->ui = anjuta_shell_get_ui (plugin->shell, NULL);
	sv_plugin->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);

	/* Create widgets */
	sv_plugin->sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sv_plugin->sw),
										 GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sv_plugin->sw),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	
	sv_plugin->sv = anjuta_symbol_view_new ();
	
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (sv_plugin->sv), FALSE);
	gtk_container_add (GTK_CONTAINER (sv_plugin->sw), sv_plugin->sv);
	
	g_signal_connect (G_OBJECT (sv_plugin->sv), "event-after",
					  G_CALLBACK (on_treeview_event), plugin);
	g_signal_connect (G_OBJECT (sv_plugin->sv), "row_activated",
					  G_CALLBACK (on_treeview_row_activated), plugin);

	gtk_widget_show_all (sv_plugin->sw);

	/* Add action group */
	sv_plugin->action_group = 
		anjuta_ui_add_action_group_entries (sv_plugin->ui,
											"ActionGroupSymbolBrowser",
											_("Symbol browser popup actions"),
											popup_actions,
											G_N_ELEMENTS (popup_actions),
											plugin);
	group = gtk_action_group_new ("ActionGroupSymbolNavigation");
	action = g_object_new (EGG_TYPE_COMBO_ACTION,
						   "name", "ActionGotoSymbol",
						   "label", _("Goto symbol"),
						   "tooltip", _("Select the symbol to go"),
						   "stock_id", GTK_STOCK_JUMP_TO,
							NULL);
	g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
	g_signal_connect (action, "activate",
					  G_CALLBACK (on_symbol_selected), sv_plugin);
	gtk_action_group_add_action (group, action);
	anjuta_ui_add_action_group (sv_plugin->ui, "ActionGroupSymbolNavigation",
								N_("Symbol navigations"), group);
	sv_plugin->action_group_nav = group;
	
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
	sv_plugin->editor_watch_id = 
		anjuta_plugin_add_watch (plugin, "document_manager_current_editor",
								 value_added_current_editor,
								 value_removed_current_editor, NULL);
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	SymbolBrowserPlugin *sv_plugin;
	sv_plugin = (SymbolBrowserPlugin*) plugin;
	
	/* Ensure all editor cached info are released */
	if (sv_plugin->editor_connected)
	{
		g_hash_table_foreach (sv_plugin->editor_connected, on_editor_foreach, plugin);
		g_hash_table_destroy (sv_plugin->editor_connected);
		sv_plugin->editor_connected = NULL;
	}	
	/* Remove watches */
	anjuta_plugin_remove_watch (plugin, sv_plugin->root_watch_id, FALSE);
	anjuta_plugin_remove_watch (plugin, sv_plugin->editor_watch_id, TRUE);
	
	/* Remove widgets: Widgets will be destroyed when sw is removed */
	anjuta_shell_remove_widget (plugin->shell, sv_plugin->sw, NULL);
	
	/* Remove UI */
	anjuta_ui_unmerge (sv_plugin->ui, sv_plugin->merge_id);
	
	/* Remove action group */
	anjuta_ui_remove_action_group (sv_plugin->ui, sv_plugin->action_group);
	anjuta_ui_remove_action_group (sv_plugin->ui, sv_plugin->action_group_nav);
	
	sv_plugin->root_watch_id = 0;
	sv_plugin->editor_watch_id = 0;
	sv_plugin->merge_id = 0;
	sv_plugin->sw = NULL;
	sv_plugin->sv = NULL;
	return TRUE;
}

static void
dispose (GObject *obj)
{
	/*
	SymbolBrowserPlugin *plugin = (SymbolBrowserPlugin*) obj;
	if (plugin->sw)
	{
		g_object_unref (G_OBJECT (plugin->sw));
		plugin->sw = NULL;
	}
	*/
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
finalize (GObject *obj)
{
	/* SymbolBrowserPlugin *plugin = (SymbolBrowserPlugin *) obj; */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
symbol_browser_plugin_instance_init (GObject *obj)
{
	SymbolBrowserPlugin *plugin = (SymbolBrowserPlugin*) obj;
	plugin->current_editor = NULL;
	plugin->editor_connected = NULL;
	plugin->sw = NULL;
	plugin->sv = NULL;
	
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
