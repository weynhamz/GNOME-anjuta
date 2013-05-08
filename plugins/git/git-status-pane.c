/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2010 <jrliggett@cox.net>
 * 
 * anjuta is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "git-status-pane.h"

enum
{
	COL_SELECTED,
	COL_STATUS,
	COL_PATH,
	COL_TYPE
};

/* Status item type flags. These help reliably determine which section a status
 * item belongs to */
typedef enum
{
	STATUS_TYPE_NONE,
	STATUS_TYPE_COMMIT,
	STATUS_TYPE_NOT_UPDATED
} StatusType;

/* Data for generating lists of selected items */
typedef struct
{
	AnjutaVcsStatus status_codes;
	GList *list;
} StatusSelectionData;

/* DND drag targets */
static GtkTargetEntry drag_target_targets[] =
{
	{
		"text/uri-list",
		0,
		0
	}
};


struct _GitStatusPanePriv
{
	GtkBuilder *builder;

	/* Iters for the two sections: Changes to be committed and Changed but not
	 * updated. Status items will be children of these two iters. */
	GtkTreeIter commit_iter;
	GtkTreeIter not_updated_iter;

	/* Hash tables that show which items are selected in each section */
	GHashTable *selected_commit_items;
	GHashTable *selected_not_updated_items;
};

G_DEFINE_TYPE (GitStatusPane, git_status_pane, GIT_TYPE_PANE);

static void
selected_renderer_data_func (GtkTreeViewColumn *tree_column,
                             GtkCellRenderer *renderer,
                             GtkTreeModel *model,
                             GtkTreeIter *iter,
                             gpointer user_data)
{
	gboolean selected;

	/* Don't show the checkbox on the toplevel items--these are supposed to 
	 * be placeholders to show the two sections, Changes to be committed and
	 * Changeed but not updated. */
	gtk_cell_renderer_set_visible (renderer, 
	                               gtk_tree_store_iter_depth (GTK_TREE_STORE (model), 
	                                                          iter) > 0);

	gtk_tree_model_get (model, iter, COL_SELECTED, &selected, -1);

	gtk_cell_renderer_toggle_set_active (GTK_CELL_RENDERER_TOGGLE (renderer),
	                                     selected);
}

static void
status_icon_renderer_data_func (GtkTreeViewColumn *tree_column,
                                GtkCellRenderer *renderer,
                                GtkTreeModel *model,
                                GtkTreeIter *iter,
                                gpointer user_data)
{
	
	AnjutaVcsStatus status;

	/* Don't show this renderer on placeholders */
	gtk_cell_renderer_set_visible (renderer, 
	                               gtk_tree_store_iter_depth (GTK_TREE_STORE (model), 
	                                                          iter) > 0);

	gtk_tree_model_get (model, iter, COL_STATUS, &status, -1);

	switch (status)
	{
		case ANJUTA_VCS_STATUS_MODIFIED:
			g_object_set (G_OBJECT (renderer), "stock-id", GTK_STOCK_EDIT, 
						  NULL);
			break;
		case ANJUTA_VCS_STATUS_ADDED:
			g_object_set (G_OBJECT (renderer), "stock-id", GTK_STOCK_ADD, 
						  NULL);
			break;
		case ANJUTA_VCS_STATUS_DELETED:
			g_object_set (G_OBJECT (renderer), "stock-id", GTK_STOCK_REMOVE, 
						  NULL);
			break;
		case ANJUTA_VCS_STATUS_CONFLICTED:
			g_object_set (G_OBJECT (renderer), "stock-id",  
						  GTK_STOCK_DIALOG_WARNING, NULL);
			break;
		case ANJUTA_VCS_STATUS_UPTODATE:
			g_object_set (G_OBJECT (renderer), "stock-id", GTK_STOCK_APPLY, 
						  NULL);
			break;
		case ANJUTA_VCS_STATUS_LOCKED:
			g_object_set (G_OBJECT (renderer), "stock-id",  
						  GTK_STOCK_DIALOG_AUTHENTICATION, NULL);
			break;	
		case ANJUTA_VCS_STATUS_MISSING:
			g_object_set (G_OBJECT (renderer), "stock-id",  
						  GTK_STOCK_MISSING_IMAGE, NULL);
			break;
		case ANJUTA_VCS_STATUS_UNVERSIONED:
			g_object_set (G_OBJECT (renderer), "stock-id",  
						  GTK_STOCK_DIALOG_QUESTION, NULL);
			break;
		case ANJUTA_VCS_STATUS_IGNORED:
			g_object_set (G_OBJECT (renderer), "stock-id", GTK_STOCK_STOP, 
						  NULL);
			break;
		case ANJUTA_VCS_STATUS_NONE:
		default:
			break;
	}
}

static void
status_name_renderer_data_func (GtkTreeViewColumn *tree_column,
                                GtkCellRenderer *renderer,
                                GtkTreeModel *model,
                                GtkTreeIter *iter,
                                gpointer user_data)
{
	AnjutaVcsStatus status;

	gtk_tree_model_get (model, iter, COL_STATUS, &status, -1);

	/* Don't show this renderer on placeholders */
	gtk_cell_renderer_set_visible (renderer, 
	                               gtk_tree_store_iter_depth (GTK_TREE_STORE (model), 
	                                                          iter) > 0);

	switch (status)
	{
		case ANJUTA_VCS_STATUS_MODIFIED:
			g_object_set (G_OBJECT (renderer), "text", _("Modified"), NULL);
			break;
		case ANJUTA_VCS_STATUS_ADDED:
			g_object_set (G_OBJECT (renderer), "text", _("Added"), NULL);
			break;
		case ANJUTA_VCS_STATUS_DELETED:
			g_object_set (G_OBJECT (renderer), "text", _("Deleted"), NULL);
			break;
		case ANJUTA_VCS_STATUS_CONFLICTED:
			g_object_set (G_OBJECT (renderer), "text", _("Conflicted"), 
									NULL);
			break;
		case ANJUTA_VCS_STATUS_UPTODATE:
			g_object_set (G_OBJECT (renderer), "text", _("Up-to-date"), 
						  NULL);
			break;
		case ANJUTA_VCS_STATUS_LOCKED:
			g_object_set (G_OBJECT (renderer), "text", _("Locked"), NULL);
			break;	
		case ANJUTA_VCS_STATUS_MISSING:
			g_object_set (G_OBJECT (renderer), "text", _("Missing"), NULL);
			break;
		case ANJUTA_VCS_STATUS_UNVERSIONED:
			g_object_set (G_OBJECT (renderer), "text", _("Unversioned"), 
									NULL);
			break;
		case ANJUTA_VCS_STATUS_IGNORED:
			g_object_set (G_OBJECT (renderer), "text", _("Ignored"),
						  NULL);
			break;
		case ANJUTA_VCS_STATUS_NONE:
		default:
			break;
	}
}

static void
path_renderer_data_func (GtkTreeViewColumn *tree_column,
                         GtkCellRenderer *renderer,
                         GtkTreeModel *model,
                         GtkTreeIter *iter,
                         gpointer user_data)
{
	gchar *path;
	gchar *placeholder;

	gtk_tree_model_get (model, iter, COL_PATH, &path, -1);

	/* Use the path column to show placeholders as well */
	if (gtk_tree_store_iter_depth (GTK_TREE_STORE (model), iter) == 0)
	{
		placeholder = g_strdup_printf ("<b>%s</b>", path); 
		
		g_object_set (G_OBJECT (renderer), "markup", placeholder, NULL);

		g_free (placeholder);
	}
	else
		g_object_set (G_OBJECT (renderer), "text", path, NULL);

	g_free (path);
	
}

static void
git_status_pane_set_path_selection_state (GitStatusPane *self,  
                                          const gchar *path, 
                                          AnjutaVcsStatus status, 
                                          StatusType type, gboolean state)
{
	GHashTable *selection_table;

	switch (type)
	{
		case STATUS_TYPE_COMMIT:
			selection_table = self->priv->selected_commit_items;
			break;
		case STATUS_TYPE_NOT_UPDATED:
			selection_table = self->priv->selected_not_updated_items;
			break;
		default:
			return;
			break;
	}

	if (state)
	{
		g_hash_table_insert (selection_table, g_strdup (path), 
		                     GINT_TO_POINTER (status));
	}
	else
		g_hash_table_remove (selection_table, path);
}

static void
on_selected_renderer_toggled (GtkCellRendererToggle *renderer, gchar *tree_path,
                              GitStatusPane *self)
{
	GtkTreeModel *status_model;
	GtkTreeIter iter;
	gboolean selected;
	AnjutaVcsStatus status;
	gchar *path;
	StatusType type;
	
	status_model = GTK_TREE_MODEL (gtk_builder_get_object (self->priv->builder,
	                                                       "status_model"));

	gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (status_model), &iter,
	                                     tree_path);
	gtk_tree_model_get (status_model, &iter, 
	                    COL_SELECTED, &selected,
	                    COL_STATUS, &status,
	                    COL_PATH, &path,
	                    COL_TYPE, &type,
	                    -1);

	selected = !selected;

	gtk_tree_store_set (GTK_TREE_STORE (status_model), &iter,
	                    COL_SELECTED, selected,
	                    -1);

	git_status_pane_set_path_selection_state (self, path, status, type, 
	                                          selected);

	g_free (path);
}

static void
add_status_items (GQueue *output, GtkTreeStore *status_model, 
                  GtkTreeIter *parent_iter, StatusType type)
{
	GitStatus *status_object;
	AnjutaVcsStatus status;
	gchar *path;
	GtkTreeIter iter;

	while (g_queue_peek_head (output))
	{
		status_object = g_queue_pop_head (output);
		status = git_status_get_vcs_status (status_object);
		path = git_status_get_path (status_object);

		gtk_tree_store_append (status_model, &iter, parent_iter);
		gtk_tree_store_set (status_model, &iter,
		                    COL_SELECTED, FALSE,
		                    COL_STATUS, status,
		                    COL_PATH, path,
		                    COL_TYPE, type,
		                    -1);

		g_free (path);
		g_object_unref (status_object);
	}
}

static void
on_commit_status_data_arrived (AnjutaCommand *command, 
                               GitStatusPane *self)
{
	GtkTreeStore *status_model;
	GQueue *output;

	status_model = GTK_TREE_STORE (gtk_builder_get_object (self->priv->builder,
	                                                       "status_model"));
	output = git_status_command_get_status_queue (GIT_STATUS_COMMAND (command));

	add_status_items (output, status_model, &(self->priv->commit_iter),
	                  STATUS_TYPE_COMMIT);

}

static void
on_not_updated_status_data_arrived (AnjutaCommand *command,
                                    GitStatusPane *self)
{
	GtkTreeStore *status_model;
	GQueue *output;

	status_model = GTK_TREE_STORE (gtk_builder_get_object (self->priv->builder,
	                                                       "status_model"));
	output = git_status_command_get_status_queue (GIT_STATUS_COMMAND (command));

	add_status_items (output, status_model, &(self->priv->not_updated_iter),
	                  STATUS_TYPE_NOT_UPDATED);
}

static void
git_status_pane_clear (GitStatusPane *self)
{
	GtkTreeStore *status_model;

	status_model = GTK_TREE_STORE (gtk_builder_get_object (self->priv->builder,	
	                                                       "status_model"));

	/* Clear any existing model data and create the placeholders */
	gtk_tree_store_clear (status_model);
	
	gtk_tree_store_append (status_model, &(self->priv->commit_iter), NULL);
	gtk_tree_store_set (status_model, &(self->priv->commit_iter), 
	                    COL_PATH, _("Changes to be committed"), 
	                    COL_SELECTED, FALSE,
	                    COL_STATUS, ANJUTA_VCS_STATUS_NONE,
	                    COL_TYPE, STATUS_TYPE_NONE,
	                    -1);

	gtk_tree_store_append (status_model, &(self->priv->not_updated_iter), 
	                       NULL);
	gtk_tree_store_set (status_model, &(self->priv->not_updated_iter), 
	                    COL_PATH, _("Changed but not updated"), 
	                    COL_SELECTED, FALSE, 
	                    COL_STATUS, ANJUTA_VCS_STATUS_NONE,
	                    COL_TYPE, STATUS_TYPE_NONE,
	                    -1);

	g_hash_table_remove_all (self->priv->selected_commit_items);
	g_hash_table_remove_all (self->priv->selected_not_updated_items);
}

static void
git_status_pane_set_selected_column_state (GitStatusPane *self, 
                                           StatusType type, 
                                           gboolean state)
{
	GtkTreeModel *status_model;
	GtkTreeIter *parent_iter;
	gint i;
	GtkTreeIter iter;
	gchar *path;
	AnjutaVcsStatus status;

	status_model = GTK_TREE_MODEL (gtk_builder_get_object (self->priv->builder,
	                                                       "status_model"));
	switch (type)
	{
		case STATUS_TYPE_COMMIT:
			parent_iter = &(self->priv->commit_iter);
			break;
		case STATUS_TYPE_NOT_UPDATED:
			parent_iter = &(self->priv->not_updated_iter);
			break;
		default:
			return;
			break;
	}
	
	i = 0;

	while (gtk_tree_model_iter_nth_child (status_model, &iter, parent_iter,
	                                      i++))
	{
		gtk_tree_store_set (GTK_TREE_STORE (status_model), &iter, 
		                    COL_SELECTED, state, 
		                    -1);

		gtk_tree_model_get (status_model, &iter,
		                    COL_PATH, &path,
		                    COL_STATUS, &status,
		                    -1);

		git_status_pane_set_path_selection_state (self, path, status, type,
		                                          state);

		g_free (path);
	}
}

static void
on_select_all_button_clicked (GtkButton *button, GitStatusPane *self)
{
	git_status_pane_set_selected_column_state (self, STATUS_TYPE_COMMIT, TRUE);
	git_status_pane_set_selected_column_state (self, STATUS_TYPE_NOT_UPDATED, 
	                                           TRUE);
}

static void
on_clear_button_clicked (GtkButton *button, GitStatusPane *self)
{
	git_status_pane_set_selected_column_state (self, STATUS_TYPE_COMMIT, FALSE);
	git_status_pane_set_selected_column_state (self, STATUS_TYPE_NOT_UPDATED, 
	                                           FALSE);
}

static void
on_status_view_drag_data_received (GtkWidget *widget,
                            	   GdkDragContext *context, gint x, gint y,
                                   GtkSelectionData *data, guint target_type,
                                   guint time, GitStatusPane *self)
{
	Git *plugin;
	gboolean success;
	gchar **uri_list;
	int i;
	GFile *file;
	gchar *path;
	GList *paths;
	GitAddCommand *add_command;

	plugin = ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (self)));
	success = FALSE;
	path = NULL;
	paths = NULL;

	if ((data != NULL) && 
	    (gtk_selection_data_get_length (data) >= 0))
	{
		if (target_type == 0)
		{
			uri_list = gtk_selection_data_get_uris (data);

			for (i = 0; uri_list[i]; i++)
			{
				file = g_file_new_for_uri (uri_list[i]);
				path = g_file_get_path (file);

				if (path && !g_file_test (path, G_FILE_TEST_IS_DIR))
				{
					paths = g_list_append (paths, 
					                       g_strdup (path +
					                                 strlen (plugin->project_root_directory) + 1));
				}

				g_free (path);
				g_object_unref (file);
			}


			add_command = git_add_command_new_list (plugin->project_root_directory,
			                                        paths, FALSE);

			g_signal_connect (G_OBJECT (add_command), "command-finished",
			                  G_CALLBACK (g_object_unref),
			                  NULL);

			anjuta_command_start (ANJUTA_COMMAND (add_command));
			success = TRUE;

			anjuta_util_glist_strings_free (paths);
			g_strfreev (uri_list);
		}
	}

	/* Do not delete source data */
	gtk_drag_finish (context, success, FALSE, time);
}

static gboolean
on_status_view_drag_drop (GtkWidget *widget, GdkDragContext *context, 
                   		  gint x, gint y, guint time,
                   		  GitStatusPane *self)
{
	GdkAtom target_type;

	target_type = gtk_drag_dest_find_target (widget, context, NULL);

	if (target_type != GDK_NONE)
		gtk_drag_get_data (widget, context, target_type, time);
	else
		gtk_drag_finish (context, FALSE, FALSE, time);

	return TRUE;
}

static gboolean
on_status_view_button_press_event (GtkWidget *widget, GdkEvent *event,
                                   GitStatusPane *self)
{
	GdkEventButton *button_event;
	AnjutaPlugin *plugin;
	AnjutaUI *ui;
	GtkTreeView *status_view;
	GtkTreeSelection *selection;
	GtkTreeModel *status_model;
	GtkTreeIter iter;
	StatusType status_type;
	GtkMenu *menu;

	button_event = (GdkEventButton *) event;
	menu = NULL;
	
	if (button_event->type == GDK_BUTTON_PRESS && button_event->button == 3)
	{
		plugin = anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (self));
		ui = anjuta_shell_get_ui (plugin->shell, NULL);
		status_view = GTK_TREE_VIEW (gtk_builder_get_object (self->priv->builder,
		                                                     "status_view"));
		selection = gtk_tree_view_get_selection (status_view);

		if (gtk_tree_selection_get_selected (selection, &status_model, &iter))
		{
			gtk_tree_model_get (status_model, &iter, COL_TYPE, &status_type, 
			                    -1);

			if (status_type == STATUS_TYPE_COMMIT)
			{
				menu = GTK_MENU (gtk_ui_manager_get_widget (GTK_UI_MANAGER (ui),
				                              			    "/GitStatusCommitPopup"));
			}
			else if (status_type == STATUS_TYPE_NOT_UPDATED)
			{
				menu = GTK_MENU (gtk_ui_manager_get_widget (GTK_UI_MANAGER (ui),
				                              				"/GitStatusNotUpdatedPopup"));
			}

			if (menu)
			{
				gtk_menu_popup (menu, NULL, NULL, NULL, NULL, button_event->button, 
				                button_event->time);
			}
		}
	}

	return FALSE;
}

static void
git_status_pane_init (GitStatusPane *self)
{
	gchar *objects[] = {"status_pane",
						"status_model",
						NULL};
	GError *error = NULL;
	GtkTreeView *status_view;
	GtkTreeViewColumn *status_column;
	GtkCellRenderer *selected_renderer;
	GtkCellRenderer *status_icon_renderer;
	GtkCellRenderer *status_name_renderer;
	GtkCellRenderer *path_renderer;
	GtkWidget *refresh_button;
	GtkWidget *select_all_button;
	GtkWidget *clear_button;

	self->priv = g_new0 (GitStatusPanePriv, 1);
	self->priv->builder = gtk_builder_new ();
	self->priv->selected_commit_items = g_hash_table_new_full (g_str_hash, 
	                                                           g_str_equal,
	                                                           g_free, 
	                                                           NULL);
	self->priv->selected_not_updated_items = g_hash_table_new_full (g_str_hash,
	                                                                g_str_equal,
	                                                                g_free,
	                                                                NULL);
	
	if (!gtk_builder_add_objects_from_file (self->priv->builder, BUILDER_FILE, 
	                                        objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

	status_view = GTK_TREE_VIEW (gtk_builder_get_object (self->priv->builder,
	                                                     "status_view"));
	status_column = GTK_TREE_VIEW_COLUMN (gtk_builder_get_object (self->priv->builder,
	                                                              "status_column"));
	selected_renderer = GTK_CELL_RENDERER (gtk_builder_get_object (self->priv->builder,
	                                   							   "selected_renderer"));
	status_icon_renderer = GTK_CELL_RENDERER (gtk_builder_get_object (self->priv->builder,
	                                                                  "status_icon_renderer"));
	status_name_renderer = GTK_CELL_RENDERER (gtk_builder_get_object (self->priv->builder,
	                                                                  "status_name_renderer"));
	path_renderer = GTK_CELL_RENDERER (gtk_builder_get_object (self->priv->builder,
	                                                           "path_renderer"));
	refresh_button = GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                                     "refresh_button"));
	select_all_button = GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                                        "select_all_button"));
	clear_button = GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                                   "clear_button"));
										   
	gtk_tree_view_column_set_cell_data_func (status_column, selected_renderer,
	                                         (GtkTreeCellDataFunc) selected_renderer_data_func,
	                                         NULL, NULL);

	gtk_tree_view_column_set_cell_data_func (status_column, status_icon_renderer,
	                                         (GtkTreeCellDataFunc) status_icon_renderer_data_func,
	                                         NULL, NULL);

	gtk_tree_view_column_set_cell_data_func (status_column, status_name_renderer,
	                                         (GtkTreeCellDataFunc) status_name_renderer_data_func,
	                                         NULL, NULL);

	gtk_tree_view_column_set_cell_data_func (status_column, path_renderer,
	                                         (GtkTreeCellDataFunc) path_renderer_data_func,
	                                         NULL, NULL);

	g_signal_connect (G_OBJECT (selected_renderer), "toggled",
	                  G_CALLBACK (on_selected_renderer_toggled),
	                  self);

	g_signal_connect_swapped (G_OBJECT (refresh_button), "clicked",
	                          G_CALLBACK (anjuta_dock_pane_refresh),
	                          self);

	g_signal_connect (G_OBJECT (select_all_button), "clicked",
	                  G_CALLBACK (on_select_all_button_clicked),
	                  self);

	g_signal_connect (G_OBJECT (clear_button), "clicked",
	                  G_CALLBACK (on_clear_button_clicked),
	                  self);

	/* DND */
	gtk_drag_dest_set (GTK_WIDGET (status_view), 
	                   GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT, 
	                   drag_target_targets,
	                   G_N_ELEMENTS (drag_target_targets), 
	                   GDK_ACTION_COPY | GDK_ACTION_MOVE);
	
	g_signal_connect (G_OBJECT (status_view), "drag-drop",
	                  G_CALLBACK (on_status_view_drag_drop),
	                  self);

	g_signal_connect (G_OBJECT (status_view), "drag-data-received",
	                  G_CALLBACK (on_status_view_drag_data_received),
	                  self);

	/* Popup menus */
	g_signal_connect (G_OBJECT (status_view), "button-press-event",
	                  G_CALLBACK (on_status_view_button_press_event),
	                  self);
}

static void
git_status_pane_finalize (GObject *object)
{
	GitStatusPane *self;

	self = GIT_STATUS_PANE (object);

	g_object_unref (self->priv->builder);
	g_hash_table_destroy (self->priv->selected_commit_items);
	g_hash_table_destroy (self->priv->selected_not_updated_items);
	g_free (self->priv);

	G_OBJECT_CLASS (git_status_pane_parent_class)->finalize (object);
}

static void
git_status_pane_refresh (AnjutaDockPane *pane)
{
	Git *plugin;

	plugin = ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (pane));

	anjuta_command_start (ANJUTA_COMMAND (plugin->commit_status_command));
	
}

static GtkWidget *
git_status_pane_get_widget (AnjutaDockPane *pane)
{
	GitStatusPane *self;

	self = GIT_STATUS_PANE (pane);

	return GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                           "status_pane"));
}

static void
git_status_pane_class_init (GitStatusPaneClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaDockPaneClass* pane_class = ANJUTA_DOCK_PANE_CLASS (klass);

	object_class->finalize = git_status_pane_finalize;
	pane_class->refresh = git_status_pane_refresh;
	pane_class->get_widget = git_status_pane_get_widget;
}

AnjutaDockPane *
git_status_pane_new (Git *plugin)
{
	GitStatusPane *self;
	GtkTreeView *status_view;

	self = g_object_new (GIT_TYPE_STATUS_PANE, "plugin", plugin, NULL);
	status_view = GTK_TREE_VIEW (gtk_builder_get_object (self->priv->builder,
														 "status_view"));

	g_signal_connect_swapped (G_OBJECT (plugin->commit_status_command), 
	                          "command-started",
	                          G_CALLBACK (git_status_pane_clear),
	                          self);

	/* Expand the placeholders so something is visible to the user after 
	 * refreshing */
	g_signal_connect_swapped (G_OBJECT (plugin->not_updated_status_command),
	                          "command-finished",
	                          G_CALLBACK (gtk_tree_view_expand_all),
	                          status_view);

	g_signal_connect (G_OBJECT (plugin->commit_status_command),
	                  "data-arrived",
	                  G_CALLBACK (on_commit_status_data_arrived),
	                  self);

	g_signal_connect (G_OBJECT (plugin->not_updated_status_command),
	                  "data-arrived",
	                  G_CALLBACK (on_not_updated_status_data_arrived),
	                  self);

	return ANJUTA_DOCK_PANE (self);
}

static void
selected_items_table_foreach (gchar *path, gpointer status, 
                              StatusSelectionData *data)
{
	if (GPOINTER_TO_INT (status) & data->status_codes)
		data->list = g_list_append (data->list, g_strdup (path));
}

GList *
git_status_pane_get_checked_commit_items (GitStatusPane *self,
                                          AnjutaVcsStatus status_codes)
{
	StatusSelectionData data;

	data.status_codes = status_codes;
	data.list = NULL;

	g_hash_table_foreach (self->priv->selected_commit_items, 
	                      (GHFunc) selected_items_table_foreach,
	                      &data);

	return data.list;
}

GList *
git_status_pane_get_checked_not_updated_items (GitStatusPane *self,
                                               AnjutaVcsStatus status_codes)
{
	StatusSelectionData data;

	data.status_codes = status_codes;
	data.list = NULL;

	g_hash_table_foreach (self->priv->selected_not_updated_items, 
	                      (GHFunc) selected_items_table_foreach,
	                      &data);

	return data.list;
}

GList *
git_status_pane_get_all_checked_items (GitStatusPane *self,
                                       AnjutaVcsStatus status_codes)
{
	StatusSelectionData data;

	data.status_codes = status_codes;
	data.list = NULL;

	g_hash_table_foreach (self->priv->selected_commit_items, 
	                      (GHFunc) selected_items_table_foreach,
	                      &data);

	g_hash_table_foreach (self->priv->selected_not_updated_items, 
	                      (GHFunc) selected_items_table_foreach,
	                      &data);

	return data.list;
}

static gchar *
git_status_pane_get_selected_path (GitStatusPane *self, StatusType type)
{
	gchar *path;
	GtkTreeView *status_view;
	GtkTreeSelection *selection;
	GtkTreeModel *status_model;
	GtkTreeIter iter;
	StatusType selected_type;

	path = NULL;
	status_view = GTK_TREE_VIEW (gtk_builder_get_object (self->priv->builder,
	                                                     "status_view"));
	selection = gtk_tree_view_get_selection (status_view);

	if (gtk_tree_selection_get_selected (selection, &status_model, &iter))
	{
		gtk_tree_model_get (status_model, &iter, COL_TYPE, &selected_type, -1);

		if (type == selected_type)
			gtk_tree_model_get (status_model, &iter, COL_PATH, &path, -1);
	}

	return path;
}

gchar *
git_status_pane_get_selected_commit_path (GitStatusPane *self)
{
	return git_status_pane_get_selected_path (self, STATUS_TYPE_COMMIT);
}

gchar *
git_status_pane_get_selected_not_updated_path (GitStatusPane *self)
{
	return git_status_pane_get_selected_path (self, STATUS_TYPE_NOT_UPDATED);
}
