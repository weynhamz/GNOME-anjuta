/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * git-shell-test
 * Copyright (C) James Liggett 2010 <jrliggett@cox.net>
 * 
 * git-shell-test is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * git-shell-test is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "git-branches-pane.h"

enum
{
	COL_SELECTED,
	COL_ACTIVE,
	COL_REMOTE,
	COL_NAME
};

static GtkTargetEntry drag_targets[] =
{
	{
		"STRING",
		0,
		0
	}
};

struct _GitBranchesPanePriv
{
	GtkBuilder *builder;
	GHashTable *selected_local_branches;
	GHashTable *selected_remote_branches;
};


G_DEFINE_TYPE (GitBranchesPane, git_branches_pane, GIT_TYPE_PANE);

/* The local branch command is started first, usually automatically. Then the 
 * remote branches are listed. We need to have both in the same model, so
 * the model isn't restored until the remote list command fisishes. */

static void
on_local_branch_list_command_started (AnjutaCommand *command, 
                                      GitBranchesPane *self)
{
	GtkTreeView *branches_view;
	GtkListStore *branches_list_model;

	branches_view = GTK_TREE_VIEW (gtk_builder_get_object (self->priv->builder,
	                                                            "branches_view"));
	branches_list_model = GTK_LIST_STORE (gtk_builder_get_object (self->priv->builder,
	                                                              "branches_list_model"));
	
	
	gtk_tree_view_set_model (branches_view, NULL);
	gtk_list_store_clear (branches_list_model);
	g_hash_table_remove_all (self->priv->selected_local_branches);
	g_hash_table_remove_all (self->priv->selected_remote_branches);
}

static void
on_remote_branch_list_command_finished (AnjutaCommand *command, 
                                        guint return_code,
                                        GitBranchesPane *self)
{
	GtkTreeView *branches_view;
	GtkListStore *branches_list_model;

	branches_view = GTK_TREE_VIEW (gtk_builder_get_object (self->priv->builder,
	                                                            "branches_view"));
	branches_list_model = GTK_LIST_STORE (gtk_builder_get_object (self->priv->builder,
	                                                              "branches_list_model"));
	
	gtk_tree_view_set_model (branches_view, 
	                         GTK_TREE_MODEL (branches_list_model));
}

static void
on_local_branch_list_command_data_arrived (AnjutaCommand *command,
                                           GitBranchesPane *self)
{
	GtkListStore *branches_list_model;
	GList *current_branch;
	GitBranch *branch;
	gboolean active;
	gchar *name;
	GtkTreeIter iter;

	branches_list_model = GTK_LIST_STORE (gtk_builder_get_object (self->priv->builder,
	                                                              "branches_list_model"));
	current_branch = git_branch_list_command_get_output (GIT_BRANCH_LIST_COMMAND (command)); 

	while (current_branch)
	{
		branch = current_branch->data;
		name = git_branch_get_name (branch);
		active = git_branch_is_active (branch);

		gtk_list_store_append (branches_list_model, &iter);
		gtk_list_store_set (branches_list_model, &iter, 
		                    COL_SELECTED, FALSE,
		                    COL_ACTIVE, active, 
		                    COL_REMOTE, FALSE, 
		                    COL_NAME, name, 
		                    -1);

		g_free (name);

		current_branch = g_list_next (current_branch);
	}
}

static void
on_remote_branch_list_command_data_arrived (AnjutaCommand *command,
                                            GitBranchesPane *self)
{
	GtkListStore *branches_list_model;
	GList *current_branch;
	GitBranch *branch;
	gboolean active;
	gchar *name;
	GtkTreeIter iter;

	branches_list_model = GTK_LIST_STORE (gtk_builder_get_object (self->priv->builder,
	                                                              "branches_list_model"));
	current_branch = git_branch_list_command_get_output (GIT_BRANCH_LIST_COMMAND (command)); 

	while (current_branch)
	{
		branch = current_branch->data;
		name = git_branch_get_name (branch);
		active = git_branch_is_active (branch);

		/* Make sure these entries are treated as remotes */

		gtk_list_store_append (branches_list_model, &iter);
		gtk_list_store_set (branches_list_model, &iter, 
		                    COL_SELECTED, FALSE,
		                    COL_ACTIVE, active, 
		                    COL_REMOTE, TRUE, 
		                    COL_NAME, name, 
		                    -1);

		g_free (name);

		current_branch = g_list_next (current_branch);
	}
}

static void
selected_column_data_func (GtkTreeViewColumn *column, GtkCellRenderer *renderer,
                           GtkTreeModel *model, GtkTreeIter *iter, 
                           GitBranchesPane *self)
{
	gboolean selected;
	gboolean active;

	gtk_tree_model_get (model, iter, COL_SELECTED, &selected, COL_ACTIVE, 
	                    &active, -1);

	gtk_cell_renderer_toggle_set_active (GTK_CELL_RENDERER_TOGGLE (renderer), 
	                                     selected);
	gtk_cell_renderer_toggle_set_activatable (GTK_CELL_RENDERER_TOGGLE (renderer),
	                                          !active);
}

static void
active_icon_data_func (GtkTreeViewColumn *column, GtkCellRenderer *renderer,
                       GtkTreeModel *model, GtkTreeIter *iter,
                       GitBranchesPane *self)
{
	gboolean active;

	gtk_tree_model_get (model, iter, COL_ACTIVE, &active, -1);

	if (active)
		g_object_set (renderer, "stock-id", GTK_STOCK_APPLY, NULL);
	else
		g_object_set (renderer, "stock-id", NULL, NULL);
}

static void
on_branch_selected_renderer_toggled (GtkCellRendererToggle *renderer, 
                                     gchar *path, GitBranchesPane *self)
{
	GtkTreeModel *branches_list_model;
	GtkTreeIter iter;
	gboolean selected;
	gboolean remote;
	gchar *name;
	GHashTable *selection_table;

	branches_list_model = GTK_TREE_MODEL (gtk_builder_get_object (self->priv->builder,
	                                                              "branches_list_model"));

	gtk_tree_model_get_iter_from_string (branches_list_model, &iter, path);
	gtk_tree_model_get (branches_list_model, &iter, 
	                    COL_SELECTED, &selected, 
	                    COL_REMOTE, &remote, 
	                    COL_NAME, &name,
	                    -1);

	selected = !selected;

	if (remote)
		selection_table = self->priv->selected_remote_branches;
	else
		selection_table = self->priv->selected_local_branches;

	/* The selection tables are hash sets of each type of selected branch 
	 * (local or remote.) The hash table takes ownership of the name string. */ 
	if (selected)
		g_hash_table_insert (selection_table, name, NULL);
	else
		g_hash_table_remove (selection_table, name);

	gtk_list_store_set (GTK_LIST_STORE (branches_list_model), &iter, 0, selected,
	                    -1);
}

static void
on_branches_list_view_drag_data_get (GtkWidget *branches_list_view, 
                                     GdkDragContext *drag_context,
                                     GtkSelectionData *data,
                                     guint info, guint time,
                                     GitBranchesPane *self)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeModel *branches_list_model;
	gchar *name;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (branches_list_view));

	if (gtk_tree_selection_count_selected_rows (selection) > 0)
	{
		gtk_tree_selection_get_selected (selection, &branches_list_model, 
		                                 &iter);

		gtk_tree_model_get (branches_list_model, &iter, COL_NAME, &name, -1);

		gtk_selection_data_set_text (data, name, -1);

		g_free (name);
	}
}

static void
selected_branches_table_foreach (gchar *name, gpointer value, 
                                 GList **list)
{
	*list = g_list_append (*list, g_strdup (name));
}

static void
on_branches_view_row_activated (GtkTreeView *branches_view, GtkTreePath *path,
                                GtkTreeViewColumn *column, GitBranchesPane *self)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *branch;
	Git *plugin;
	GitBranchCheckoutCommand *checkout_command;
	
	model = gtk_tree_view_get_model (branches_view);
	gtk_tree_model_get_iter (model, &iter, path);

	gtk_tree_model_get (model, &iter, COL_NAME, &branch, -1);

	plugin = ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (self)));
	checkout_command = git_branch_checkout_command_new (plugin->project_root_directory,
	                                                    branch);

	g_signal_connect (G_OBJECT (checkout_command), "command-finished",
	                  G_CALLBACK (git_pane_report_errors),
	                  plugin);

	g_signal_connect (G_OBJECT (checkout_command), "command-finished",
	                  G_CALLBACK (g_object_unref), 
	                  NULL);

	g_free (branch);

	anjuta_command_start (ANJUTA_COMMAND (checkout_command));
}

static void
git_branches_pane_init (GitBranchesPane *self)
{
	gchar *objects[] = {"branches_pane",
						"branches_list_model",
						NULL};
	GError *error = NULL;
	GtkTreeView *branches_view;
	GtkTreeViewColumn *branch_selected_column;
	GtkCellRenderer *branch_selected_renderer;
	GtkTreeViewColumn *branch_name_column;
	GtkCellRenderer *branch_active_icon_renderer;
	
	self->priv = g_new0 (GitBranchesPanePriv, 1);
	self->priv->builder = gtk_builder_new ();
	self->priv->selected_local_branches = g_hash_table_new_full (g_str_hash,
	                                                             g_str_equal,
	                                                             g_free,
	                                                             NULL);
	self->priv->selected_remote_branches = g_hash_table_new_full (g_str_hash,
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

	branches_view = GTK_TREE_VIEW (gtk_builder_get_object (self->priv->builder,
	                                                       "branches_view"));
	branch_selected_column = GTK_TREE_VIEW_COLUMN (gtk_builder_get_object (self->priv->builder,
	                                               "branch_selected_column"));
	branch_selected_renderer = GTK_CELL_RENDERER (gtk_builder_get_object (self->priv->builder,
	                                              "branch_selected_renderer"));
	branch_name_column = GTK_TREE_VIEW_COLUMN (gtk_builder_get_object (self->priv->builder,
	                                           "branch_name_column"));
	branch_active_icon_renderer = GTK_CELL_RENDERER (gtk_builder_get_object (self->priv->builder,
	                                                                         "branch_active_icon_renderer"));

	/* DND */
	gtk_tree_view_enable_model_drag_source (branches_view,
	                                        GDK_BUTTON1_MASK,
	                                        drag_targets,
	                                        G_N_ELEMENTS (drag_targets),
	                                        GDK_ACTION_COPY);

	g_signal_connect (G_OBJECT (branches_view), "drag-data-get",
	                  G_CALLBACK (on_branches_list_view_drag_data_get),
	                  self);

	gtk_tree_view_column_set_cell_data_func (branch_selected_column,
	                                         branch_selected_renderer,
	                                         (GtkTreeCellDataFunc) selected_column_data_func,
	                                         self, 
	                                         NULL);

	gtk_tree_view_column_set_cell_data_func (branch_name_column,
	                                         branch_active_icon_renderer,
	                                         (GtkTreeCellDataFunc) active_icon_data_func,
	                                         self, 
	                                         NULL);

	g_signal_connect (G_OBJECT (branch_selected_renderer), "toggled",
	                  G_CALLBACK (on_branch_selected_renderer_toggled),
	                  self);

	/* Switch branches on double-click */
	g_signal_connect (G_OBJECT (branches_view), "row-activated",
	                  G_CALLBACK (on_branches_view_row_activated),
	                  self);
}

static void
git_branches_pane_finalize (GObject *object)
{
	GitBranchesPane *self;

	self = GIT_BRANCHES_PANE (object);

	g_object_unref (self->priv->builder);
	g_hash_table_destroy (self->priv->selected_local_branches);
	g_hash_table_destroy (self->priv->selected_remote_branches);
	g_free (self->priv);

	G_OBJECT_CLASS (git_branches_pane_parent_class)->finalize (object);
}

static GtkWidget *
git_branches_pane_get_widget (AnjutaDockPane *pane)
{
	GitBranchesPane *self;

	self = GIT_BRANCHES_PANE (pane);

	return GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                           "branches_pane"));
}

static void
git_branches_pane_refresh (AnjutaDockPane *pane)
{
	/* TODO: Add private function implementation here */
}

static void
git_branches_pane_class_init (GitBranchesPaneClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaDockPaneClass *pane_class = ANJUTA_DOCK_PANE_CLASS (klass);

	object_class->finalize = git_branches_pane_finalize;
	pane_class->get_widget = git_branches_pane_get_widget;
	pane_class->refresh = git_branches_pane_refresh;
}


AnjutaDockPane *
git_branches_pane_new (Git *plugin)
{
	GitBranchesPane *self;
	
	self = g_object_new (GIT_TYPE_BRANCHES_PANE, "plugin", plugin, NULL);

	g_signal_connect (G_OBJECT (plugin->local_branch_list_command), 
	                  "command-started",
	                  G_CALLBACK (on_local_branch_list_command_started),
	                  self);

	g_signal_connect (G_OBJECT (plugin->remote_branch_list_command), 
	                  "command-finished",
	                  G_CALLBACK (on_remote_branch_list_command_finished),
	                  self);

	g_signal_connect (G_OBJECT (plugin->local_branch_list_command), 
	                  "data-arrived",
	                  G_CALLBACK (on_local_branch_list_command_data_arrived),
	                  self);

	g_signal_connect (G_OBJECT (plugin->remote_branch_list_command),
	                  "data-arrived",
	                  G_CALLBACK (on_remote_branch_list_command_data_arrived),
	                  self);

	return ANJUTA_DOCK_PANE (self);
}

GList *
git_branches_pane_get_selected_local_branches (GitBranchesPane *self)
{
	GList *list;

	list = NULL;

	g_hash_table_foreach (self->priv->selected_local_branches, 
	                      (GHFunc) selected_branches_table_foreach, 
	                      &list);

	return list;
}

GList *
git_branches_pane_get_selected_remote_branches (GitBranchesPane *self)
{
	GList *list;

	list = NULL;

	g_hash_table_foreach (self->priv->selected_remote_branches, 
	                      (GHFunc) selected_branches_table_foreach, 
	                      &list);

	return list;
}

gsize
git_branches_pane_count_selected_items (GitBranchesPane *self)
{
	return (g_hash_table_size (self->priv->selected_local_branches)) +
		   (g_hash_table_size (self->priv->selected_remote_branches));
}

gchar *
git_branches_pane_get_selected_branch (GitBranchesPane *self)
{
	gchar *selected_branch;
	GtkTreeView *branches_view;
	GtkTreeSelection *selection;
	GtkTreeModel *branches_list_model;
	GtkTreeIter iter;

	selected_branch = NULL;
	branches_view = GTK_TREE_VIEW (gtk_builder_get_object (self->priv->builder,
	                                                       "branches_view"));
	selection = gtk_tree_view_get_selection (branches_view);

	if (gtk_tree_selection_count_selected_rows (selection) > 0)
	{
		gtk_tree_selection_get_selected (selection, &branches_list_model,
		                                 &iter);

		gtk_tree_model_get (branches_list_model, &iter, COL_NAME, 
		                    &selected_branch, -1);
	}

	return selected_branch;
}

static gboolean
clear_branch_selections (GtkTreeModel *model, GtkTreePath *path, 
                         GtkTreeIter *iter, gpointer data)
{
	gtk_list_store_set (GTK_LIST_STORE (model), iter, COL_SELECTED, FALSE, -1);
	
	return FALSE;
}

void
git_branches_pane_set_select_column_visible (GitBranchesPane *self,
                                             gboolean visible)
{
	GtkTreeViewColumn *branch_selected_column;
	GtkTreeModel *branches_list_model;

	branch_selected_column = GTK_TREE_VIEW_COLUMN (gtk_builder_get_object (self->priv->builder,
	                                                                       "branch_selected_column"));

	gtk_tree_view_column_set_visible (branch_selected_column, visible);

	/* Clear branch selections when the column becomes invisible again, because
	 * selections have no meaning once an operation that needs these selections
	 * has either been completed or cancelled */
	if (!visible)
	{
		branches_list_model = GTK_TREE_MODEL (gtk_builder_get_object (self->priv->builder,
		                                                              "branches_list_model"));

		gtk_tree_model_foreach (branches_list_model,
		                        (GtkTreeModelForeachFunc) clear_branch_selections,
		                        NULL);

		g_hash_table_remove_all (self->priv->selected_local_branches);
		g_hash_table_remove_all (self->priv->selected_remote_branches);
	}
}

