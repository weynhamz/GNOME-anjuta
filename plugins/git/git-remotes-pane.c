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

#include "git-remotes-pane.h"

struct _GitRemotesPanePriv
{
	GtkBuilder *builder;
	gchar *selected_remote;
};

G_DEFINE_TYPE (GitRemotesPane, git_remotes_pane, GIT_TYPE_PANE);

static gboolean
on_remote_selected (GtkTreeSelection *selection, GtkTreeModel *model,
                    GtkTreePath *path, gboolean path_currently_selected,
                    GitRemotesPane *self)
{
	GtkTreeIter iter;

	if (!path_currently_selected)
	{
		gtk_tree_model_get_iter (model, &iter, path);

		g_free (self->priv->selected_remote);
		gtk_tree_model_get (model, &iter, 0, &(self->priv->selected_remote), -1);
		
		anjuta_dock_pane_notify_single_selection_changed (ANJUTA_DOCK_PANE (self));
	}

	return TRUE;
}

static void
git_remotes_pane_init (GitRemotesPane *self)
{
	gchar *objects[] = {"remotes_pane",
						"remotes_list_model",
						NULL};
	GError *error = NULL;
	GtkTreeView *remotes_view;
	GtkTreeSelection *selection;

	self->priv = g_new0 (GitRemotesPanePriv, 1);
	self->priv->builder = gtk_builder_new ();

	if (!gtk_builder_add_objects_from_file (self->priv->builder, BUILDER_FILE, 
	                                        objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

	remotes_view = GTK_TREE_VIEW (gtk_builder_get_object (self->priv->builder,
	                                                      "remotes_view"));
	selection = gtk_tree_view_get_selection (remotes_view);


	gtk_tree_selection_set_select_function (selection,
	                                        (GtkTreeSelectionFunc) on_remote_selected,
	                                        self, NULL);
}

static void
git_remotes_pane_finalize (GObject *object)
{
	GitRemotesPane *self;

	self = GIT_REMOTES_PANE (object);

	g_object_unref (self->priv->builder);
	g_free (self->priv->selected_remote);
	g_free (self->priv);

	G_OBJECT_CLASS (git_remotes_pane_parent_class)->finalize (object);
}

static GtkWidget *
git_remotes_pane_get_widget (AnjutaDockPane *pane)
{
	GitRemotesPane *self;

	self = GIT_REMOTES_PANE (pane);

	return GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                           "remotes_pane"));
}

static void
git_remotes_pane_class_init (GitRemotesPaneClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaDockPaneClass* pane_class = ANJUTA_DOCK_PANE_CLASS (klass);

	object_class->finalize = git_remotes_pane_finalize;
	pane_class->get_widget = git_remotes_pane_get_widget;
	pane_class->refresh = NULL;
}

static void
on_remote_list_command_data_arrived (AnjutaCommand *command, 
                                     GitRemotesPane *self)
{
	GtkListStore *remotes_list_model;
	GQueue *output;
	gchar *remote;
	GtkTreeIter iter;

	remotes_list_model = GTK_LIST_STORE (gtk_builder_get_object (self->priv->builder,
	                                                             "remotes_list_model"));
	output = git_raw_output_command_get_output (GIT_RAW_OUTPUT_COMMAND (command));

	while (g_queue_peek_head (output))
	{
		remote = g_queue_pop_head (output);

		gtk_list_store_append (remotes_list_model, &iter);
		gtk_list_store_set (remotes_list_model, &iter, 0, remote, -1);

		g_free (remote);
	}
}

AnjutaDockPane *
git_remotes_pane_new (Git *plugin)
{
	GitRemotesPane *self;
	GtkListStore *remotes_list_model;

	self = g_object_new (GIT_TYPE_REMOTES_PANE, "plugin", plugin, NULL);
	remotes_list_model = GTK_LIST_STORE (gtk_builder_get_object (self->priv->builder,
	                                                             "remotes_list_model"));

	g_signal_connect_swapped (G_OBJECT (plugin->remote_list_command), 
	                          "command-started",
	                          G_CALLBACK (gtk_list_store_clear),
	                          remotes_list_model);

	g_signal_connect (G_OBJECT (plugin->remote_list_command), "data-arrived",
	                  G_CALLBACK (on_remote_list_command_data_arrived),
	                  self);

	return ANJUTA_DOCK_PANE (self);
}

gchar *
git_remotes_pane_get_selected_remote (GitRemotesPane *self)
{
	return self->priv->selected_remote;
}
