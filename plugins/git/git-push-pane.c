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

#include "git-push-pane.h"

struct _GitPushPanePriv
{
	GtkBuilder *builder;
	GtkWidget *repository_selector;
};

enum
{
	COL_SELECTED,
	COL_NAME
};

G_DEFINE_TYPE (GitPushPane, git_push_pane, GIT_TYPE_PANE);

/* Foreach function for both the selected branches and tags list. Will generate
 * one list of both types of selected items. */
static gboolean
get_selected_items (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter,
                    GList **list)
{
	gboolean selected;
	gchar *name;

	gtk_tree_model_get (model, iter, COL_SELECTED, &selected, -1);

	if (selected)
	{
		gtk_tree_model_get (model, iter, COL_NAME, &name, -1);
		*list = g_list_append (*list, name);
	}

	return FALSE;
}

static void
on_ok_button_clicked (GtkButton *button, GitPushPane *self)
{
	Git *plugin;
	GtkTreeModel *push_branch_model;
	GtkTreeModel *push_tag_model;
	GtkToggleButton *push_all_tags_check;
	GtkToggleButton *push_all_check;
	GtkToggleButton *force_check;
	GList *selected_items;
	gboolean push_all_tags;
	gboolean push_all;
	gchar *repository;
	GitPushCommand *push_command;

	plugin = ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (self)));
	push_branch_model = GTK_TREE_MODEL (gtk_builder_get_object (self->priv->builder,
	                                                            "push_branch_model"));
	push_tag_model = GTK_TREE_MODEL (gtk_builder_get_object (self->priv->builder,
	                                                         "push_tag_model"));
	push_all_tags_check = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                                 "push_all_tags_check"));
	push_all_check = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                            "push_all_check"));
	force_check = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                         "force_check"));
	selected_items = NULL;
	push_all_tags = gtk_toggle_button_get_active (push_all_tags_check);
	push_all = gtk_toggle_button_get_active (push_all_check);

	/* Check that the user has given a valid repository. In this case we only 
	 * care about checking if the selector widget is in URL mode, because even
	 * if a remote isn't selected, it will default to origin, so we should 
	 * always get something when the selector is in Remote mode. */
	repository = git_repository_selector_get_repository (GIT_REPOSITORY_SELECTOR (self->priv->repository_selector));

	if (!git_pane_check_input (GTK_WIDGET (ANJUTA_PLUGIN (plugin)->shell), 
	                           self->priv->repository_selector, repository,
	                           _("Please enter a URL.")))
	{
		g_free (repository);
		return;
	}

	if (!push_all)
	{
		/* Get selected branches */
		gtk_tree_model_foreach (push_branch_model,
		                        (GtkTreeModelForeachFunc) get_selected_items,
		                        &selected_items);
	}

	/* Don't bother getting selected tags if Push all tags is checked or if 
	 * Push all branches and tags is checked */
	if (!push_all && !push_all_tags)
	{
		/* Get selected tags */
		gtk_tree_model_foreach (push_tag_model,
		                        (GtkTreeModelForeachFunc) get_selected_items,
		                        &selected_items);
	}

	push_command = git_push_command_new (plugin->project_root_directory, 
	                                     repository, selected_items,
	                                     push_all, push_all_tags,
	                                     gtk_toggle_button_get_active (force_check));

	g_free (repository);
	anjuta_util_glist_strings_free (selected_items);

	git_pane_create_message_view (plugin);

	g_signal_connect (G_OBJECT (push_command), "data-arrived",
	                  G_CALLBACK (git_pane_on_command_info_arrived),
	                  plugin);

	g_signal_connect (G_OBJECT (push_command), "command-finished",
	                  G_CALLBACK (git_pane_report_errors),
	                  plugin);
	
	g_signal_connect (G_OBJECT (push_command), "command-finished",
                      G_CALLBACK (git_plugin_status_changed_emit),
                      plugin);

	g_signal_connect (G_OBJECT (push_command), "command-finished",
	                  G_CALLBACK (g_object_unref),
	                  NULL);

	anjuta_command_start (ANJUTA_COMMAND (push_command));

	git_pane_remove_from_dock (GIT_PANE (self));
}

static void
on_selected_renderer_toggled (GtkCellRendererToggle *renderer, gchar *path,
                              GtkTreeModel *model)
{
	GtkTreeIter iter;
	gboolean selected;

	gtk_tree_model_get_iter_from_string (model, &iter, path);

	gtk_tree_model_get (model, &iter, COL_SELECTED, &selected, -1);

	selected = !selected;

	gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_SELECTED, selected, 
	                    -1);
}

static void
on_branch_list_command_data_arrived (AnjutaCommand *command, 
                                     GtkListStore *push_branch_model)
{
	GList *current_branch;
	GitBranch *branch;
	gchar *name;
	GtkTreeIter iter;

	current_branch = git_branch_list_command_get_output (GIT_BRANCH_LIST_COMMAND (command));

	while (current_branch)
	{
		branch = current_branch->data;
		name = git_branch_get_name (branch);

		gtk_list_store_append (push_branch_model, &iter);
		gtk_list_store_set (push_branch_model, &iter,
		                    COL_SELECTED, FALSE,
		                    COL_NAME, name,
		                    -1);

		g_free (name);

		current_branch = g_list_next (current_branch);
	}
}

static void
on_tag_list_command_data_arrived (AnjutaCommand *command,
                                  GtkListStore *push_tag_model)
{
	GQueue *output;
	gchar *name;
	GtkTreeIter iter;

	output = git_raw_output_command_get_output (GIT_RAW_OUTPUT_COMMAND (command));

	while (g_queue_peek_head (output))
	{
		name = g_queue_pop_head (output);

		gtk_list_store_append (push_tag_model, &iter);
		gtk_list_store_set (push_tag_model, &iter, 
		                    COL_SELECTED, FALSE,
		                    COL_NAME, name,
		                    -1);

		g_free (name);
	}
}

static void
on_push_all_check_toggled (GtkToggleButton *button, GtkWidget *widget)
{
	gtk_widget_set_sensitive (widget,
	                          !gtk_toggle_button_get_active (button));
}

static void
on_remote_selected (AnjutaDockPane *pane, 
                    GitRepositorySelector *repository_selector)
{
	gchar *remote;

	remote = git_remotes_pane_get_selected_remote (GIT_REMOTES_PANE (pane));
	git_repository_selector_set_remote (repository_selector, remote);

	g_free (remote);
}

static void
git_push_pane_init (GitPushPane *self)
{
	gchar *objects[] = {"push_pane",
						"push_branch_model",
						"push_tag_model",
						NULL};
	GError *error = NULL;
	GtkWidget *ok_button;
	GtkWidget *cancel_button;
	GtkContainer *repository_alignment;
	GtkWidget *branches_view;
	GtkWidget *tags_view;
	GtkTreeModel *push_branch_model;
	GtkTreeModel *push_tag_model;
	GtkCellRenderer *branches_selected_renderer;
	GtkCellRenderer *tags_selected_renderer;
	GtkWidget *push_all_tags_check;
	GtkWidget *push_all_check;;

	self->priv = g_new0 (GitPushPanePriv, 1);
	self->priv->builder = gtk_builder_new ();
	
	if (!gtk_builder_add_objects_from_file (self->priv->builder, BUILDER_FILE, 
	                                        objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

	ok_button = GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                                "ok_button"));
	cancel_button = GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                                    "cancel_button"));
	repository_alignment = GTK_CONTAINER (gtk_builder_get_object (self->priv->builder,
	                                                              "repository_alignment"));
	self->priv->repository_selector = git_repository_selector_new ();
	branches_view = GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                                    "branches_view"));
	tags_view = GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                                "tags_view"));
	push_branch_model = GTK_TREE_MODEL (gtk_builder_get_object (self->priv->builder,
	                                                            "push_branch_model"));
	push_tag_model = GTK_TREE_MODEL (gtk_builder_get_object (self->priv->builder,
	                                                        "push_tag_model"));
	branches_selected_renderer = GTK_CELL_RENDERER (gtk_builder_get_object (self->priv->builder,
	                                                                        "branches_selected_renderer"));
	tags_selected_renderer = GTK_CELL_RENDERER (gtk_builder_get_object (self->priv->builder,
	                                                                     "tags_selected_renderer"));
	push_all_tags_check = GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                                          "push_all_tags_check"));
	push_all_check = GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                                     "push_all_check"));

	gtk_container_add (repository_alignment, self->priv->repository_selector);

	g_signal_connect (G_OBJECT (ok_button), "clicked",
	                  G_CALLBACK (on_ok_button_clicked),
	                  self);

	g_signal_connect_swapped (G_OBJECT (cancel_button), "clicked",
	                          G_CALLBACK (git_pane_remove_from_dock),
	                          self);

	g_signal_connect (G_OBJECT (branches_selected_renderer), "toggled",
	                  G_CALLBACK (on_selected_renderer_toggled),
	                  push_branch_model);

	g_signal_connect (G_OBJECT (tags_selected_renderer), "toggled",
	                  G_CALLBACK (on_selected_renderer_toggled),
	                  push_tag_model);

	/* Use the push all tags check callback to handle both the Push all tags
	 * and the Push all branches and tags check boxes. The behavior is the same
	 * for both; they just act on different widgets. */
	g_signal_connect (G_OBJECT (push_all_tags_check), "toggled",
	                  G_CALLBACK (on_push_all_check_toggled),
	                  tags_view);

	g_signal_connect (G_OBJECT (push_all_check), "toggled",
	                  G_CALLBACK (on_push_all_check_toggled),
	                  branches_view);

	g_signal_connect (G_OBJECT (push_all_check), "toggled",
	                  G_CALLBACK (on_push_all_check_toggled),
	                  tags_view);
}

static void
git_push_pane_dispose (GObject *object)
{
	GitPushPane *self;
	Git *plugin;

	self = GIT_PUSH_PANE (object);
	plugin = ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (object)));

	g_signal_handlers_disconnect_by_func (plugin->remotes_pane, 
	                                      on_remote_selected,
	                                      self->priv->repository_selector);

	G_OBJECT_CLASS (git_push_pane_parent_class)->dispose (object);
	                                      
}

static void
git_push_pane_finalize (GObject *object)
{
	GitPushPane *self;

	self = GIT_PUSH_PANE (object);

	g_object_unref (self->priv->builder);
	g_free (self->priv);

	G_OBJECT_CLASS (git_push_pane_parent_class)->finalize (object);
}

static GtkWidget *
git_push_pane_get_widget (AnjutaDockPane *pane)
{
	GitPushPane *self;

	self = GIT_PUSH_PANE (pane);

	return GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                           "push_pane"));
}

static void
git_push_pane_class_init (GitPushPaneClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaDockPaneClass* pane_class = ANJUTA_DOCK_PANE_CLASS (klass);

	object_class->finalize = git_push_pane_finalize;
	object_class->dispose = git_push_pane_dispose;
	pane_class->get_widget = git_push_pane_get_widget;
	pane_class->refresh = NULL;
}

AnjutaDockPane *
git_push_pane_new (Git *plugin)
{
	GitPushPane *self;
	GtkListStore *push_branch_model;
	GtkListStore *push_tag_model;
	GitBranchListCommand *branch_list_command;
	GitTagListCommand *tag_list_command;

	self = g_object_new (GIT_TYPE_PUSH_PANE, "plugin", plugin, NULL);
	push_branch_model = GTK_LIST_STORE (gtk_builder_get_object (self->priv->builder,
	                                                            "push_branch_model"));
	push_tag_model = GTK_LIST_STORE (gtk_builder_get_object (self->priv->builder,
	                                                         "push_tag_model"));
	branch_list_command = git_branch_list_command_new (plugin->project_root_directory,
	                                                   GIT_BRANCH_TYPE_LOCAL);
	tag_list_command = git_tag_list_command_new (plugin->project_root_directory);

	g_signal_connect (G_OBJECT (plugin->remotes_pane), "single-selection-changed",
	                  G_CALLBACK (on_remote_selected),
	                  self->priv->repository_selector);

	g_signal_connect (G_OBJECT (branch_list_command), "data-arrived",
	                  G_CALLBACK (on_branch_list_command_data_arrived),
	                  push_branch_model);

	g_signal_connect (G_OBJECT (tag_list_command), "data-arrived",
	                  G_CALLBACK (on_tag_list_command_data_arrived),
	                  push_tag_model);

	g_signal_connect (G_OBJECT (branch_list_command), "command-finished",
	                  G_CALLBACK (g_object_unref),
	                  NULL);

	g_signal_connect (G_OBJECT (tag_list_command), "command-finished",
	                  G_CALLBACK (g_object_unref),
	                  NULL);

	/* Set the contents of the selected remote label */
	on_remote_selected (plugin->remotes_pane, 
	                    GIT_REPOSITORY_SELECTOR (self->priv->repository_selector));
	
	anjuta_command_start (ANJUTA_COMMAND (branch_list_command));
	anjuta_command_start (ANJUTA_COMMAND (tag_list_command));

	return ANJUTA_DOCK_PANE (self);
}

void
on_push_button_clicked (GtkAction *action, Git *plugin)
{
	AnjutaDockPane *pane;

	pane = git_push_pane_new (plugin);

	anjuta_dock_replace_command_pane (ANJUTA_DOCK (plugin->dock), "Push", 
	                                  _("Push"), NULL, pane, GDL_DOCK_BOTTOM,
	                                  NULL, 0, NULL);
}
