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

#include "git-pull-pane.h"

struct _GitPullPanePriv
{
	GtkBuilder *builder;
	GtkWidget *repository_selector;
};

G_DEFINE_TYPE (GitPullPane, git_pull_pane, GIT_TYPE_PANE);

static void
on_ok_button_clicked (GtkButton *button, GitPullPane *self)
{
	Git *plugin;
	GtkToggleButton *rebase_check;
	GtkToggleButton *no_commit_check;
	GtkToggleButton *squash_check;
	GtkToggleButton *append_fetch_data_check;
	GtkToggleButton *fast_forward_commit_check;
	GtkToggleButton *force_check;
	GtkToggleButton *no_follow_tags_check;
	gchar *repository;
	GitPullCommand *pull_command;

	plugin = ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (self)));
	rebase_check = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                          "rebase_check"));
	no_commit_check = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                             "no_commit_check"));
	squash_check = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                          "squash_check"));
	append_fetch_data_check = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                                     "append_fetch_data_check"));
	fast_forward_commit_check = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                                         "fast_forward_commit_check"));
	force_check = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                         "force_check"));
	no_follow_tags_check = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                                  "no_follow_tags_check"));

	repository = git_repository_selector_get_repository (GIT_REPOSITORY_SELECTOR (self->priv->repository_selector));

	/* Check input. Input can only be bad if the selector is in URL mode */
	if (!git_pane_check_input (GTK_WIDGET (ANJUTA_PLUGIN (plugin)->shell),
	                           self->priv->repository_selector,
	                           repository,
	                           _("Please enter a URL.")))
	{
		g_free (repository);
		return;
	}

	pull_command = git_pull_command_new (plugin->project_root_directory,
	                                     repository,
	                                     gtk_toggle_button_get_active (rebase_check),
	                                     gtk_toggle_button_get_active (no_commit_check),
	                                     gtk_toggle_button_get_active (squash_check),
	                                     gtk_toggle_button_get_active (append_fetch_data_check),
	                                     gtk_toggle_button_get_active (fast_forward_commit_check),
	                                     gtk_toggle_button_get_active (force_check),
	                                     gtk_toggle_button_get_active (no_follow_tags_check));

	g_free (repository);

	git_pane_create_message_view (plugin);

	g_signal_connect (G_OBJECT (pull_command), "data_arrived",
	                  G_CALLBACK (git_pane_on_command_info_arrived),
	                  plugin);

	g_signal_connect (G_OBJECT (pull_command), "command-finished",
	                  G_CALLBACK (g_object_unref),
	                  NULL);

	anjuta_command_start (ANJUTA_COMMAND (pull_command));
	
	anjuta_dock_remove_pane (ANJUTA_DOCK (plugin->dock), 
	                         ANJUTA_DOCK_PANE (self));
}

static void
on_cancel_button_clicked (GtkButton *button, GitPullPane *self)
{
	Git *plugin;

	plugin = ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (self)));

	anjuta_dock_remove_pane (ANJUTA_DOCK (plugin->dock), 
	                         ANJUTA_DOCK_PANE (self));
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
git_pull_pane_init (GitPullPane *self)
{
	gchar *objects[] = {"pull_pane",
						NULL};
	GError *error = NULL;
	GtkWidget *ok_button;
	GtkWidget *cancel_button;
	GtkContainer *repository_alignment;

	self->priv = g_new0 (GitPullPanePriv, 1);
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

	gtk_container_add (repository_alignment, self->priv->repository_selector);

	g_signal_connect (G_OBJECT (ok_button), "clicked",
	                  G_CALLBACK (on_ok_button_clicked),
	                  self);

	g_signal_connect (G_OBJECT (cancel_button), "clicked",
	                  G_CALLBACK (on_cancel_button_clicked),
	                  self);
}

static void
git_pull_pane_finalize (GObject *object)
{
	GitPullPane *self;

	self = GIT_PULL_PANE (object);

	g_object_unref (self->priv->builder);
	g_free (self->priv);

	G_OBJECT_CLASS (git_pull_pane_parent_class)->finalize (object);
}

static void
git_pull_pane_dispose (GObject *object)
{
	GitPullPane *self;
	Git *plugin;

	self = GIT_PULL_PANE (object);
	plugin = ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (object)));

	g_signal_handlers_disconnect_by_func (plugin->remotes_pane, 
	                                      on_remote_selected,
	                                      self->priv->repository_selector);


	G_OBJECT_CLASS (git_pull_pane_parent_class)->dispose (object);
}

static GtkWidget *
git_pull_pane_get_widget (AnjutaDockPane *pane)
{
	GitPullPane *self;

	self = GIT_PULL_PANE (pane);

	return GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                           "pull_pane"));
}

static void
git_pull_pane_class_init (GitPullPaneClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaDockPaneClass *pane_class = ANJUTA_DOCK_PANE_CLASS (klass);

	object_class->finalize = git_pull_pane_finalize;
	object_class->dispose = git_pull_pane_dispose;
	pane_class->get_widget = git_pull_pane_get_widget;
}


AnjutaDockPane *
git_pull_pane_new (Git *plugin)
{
	GitPullPane *self;

	self = g_object_new (GIT_TYPE_PULL_PANE, "plugin", plugin, NULL);

	g_signal_connect (G_OBJECT (plugin->remotes_pane), "single_selection_changed",
	                  G_CALLBACK (on_remote_selected),
	                  self->priv->repository_selector);

	/* Set the contents of the selected remote label */
	on_remote_selected (plugin->remotes_pane, 
	                    GIT_REPOSITORY_SELECTOR (self->priv->repository_selector));

	return ANJUTA_DOCK_PANE (self);
}

void
on_pull_button_clicked (GtkAction *action, Git *plugin)
{
	AnjutaDockPane *pane;

	pane = git_pull_pane_new (plugin);

	anjuta_dock_add_pane (ANJUTA_DOCK (plugin->dock), "Pull", 
	                      _("Pull"), NULL, pane, GDL_DOCK_BOTTOM,
	                      NULL, 0, NULL);
}
