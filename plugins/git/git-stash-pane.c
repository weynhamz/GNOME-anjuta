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

#include "git-stash-pane.h"

enum
{
	COL_NUMBER,
	COL_MESSAGE,
	COL_ID
};

struct _GitStashPanePriv
{
	GtkBuilder *builder;
};

G_DEFINE_TYPE (GitStashPane, git_stash_pane, GIT_TYPE_PANE);

static void
on_stash_list_command_started (AnjutaCommand *command, GitStashPane *self)
{
	GtkTreeView *stash_view;
	GtkListStore *stash_list_model;

	stash_view = GTK_TREE_VIEW (gtk_builder_get_object (self->priv->builder,
	                                                    "stash_view"));
	stash_list_model = GTK_LIST_STORE (gtk_builder_get_object (self->priv->builder,
	                                                           "stash_list_model"));

	gtk_tree_view_set_model (stash_view, NULL);
	gtk_list_store_clear (stash_list_model);
}

static void
on_stash_list_command_finished (AnjutaCommand *command, guint return_code,
                                GitStashPane *self)
{
	GtkTreeView *stash_view;
	GtkTreeModel *stash_list_model;

	stash_view = GTK_TREE_VIEW (gtk_builder_get_object (self->priv->builder,
	                                                    "stash_view"));
	stash_list_model = GTK_TREE_MODEL (gtk_builder_get_object (self->priv->builder,
	                                                           "stash_list_model"));

	gtk_tree_view_set_model (stash_view, stash_list_model);
}

static void
on_stash_list_command_data_arrived (AnjutaCommand *command, 
                                    GtkListStore *stash_list_model)
{
	GQueue *output;
	GtkTreeIter iter;
	GitStash *stash;
	guint number;
	gchar *message;
	gchar *id;
	
	output = git_stash_list_command_get_output (GIT_STASH_LIST_COMMAND (command));

	while (g_queue_peek_head (output))
	{
		gtk_list_store_append (stash_list_model, &iter);

		stash = g_queue_pop_head (output);
		number = git_stash_get_number (stash);
		message = git_stash_get_message (stash);
		id = git_stash_get_id (stash);

		gtk_list_store_set (stash_list_model, &iter, 
		                    COL_NUMBER, number,
		                    COL_MESSAGE, message,
		                    COL_ID, id,
		                    -1);

		g_object_unref (stash);
		g_free (message);
		g_free (id);
	}
}

static void
git_stash_pane_init (GitStashPane *self)
{
	gchar *objects[] = {"stash_pane",
						"stash_list_model",
						NULL};
	GError *error = NULL;
	
	self->priv = g_new0 (GitStashPanePriv, 1);
	self->priv->builder = gtk_builder_new ();
	

	if (!gtk_builder_add_objects_from_file (self->priv->builder, BUILDER_FILE, 
	                                        objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}
}

static void
git_stash_pane_finalize (GObject *object)
{
	GitStashPane *self;

	self = GIT_STASH_PANE (object);

	g_object_unref (self->priv->builder);
	g_free (self->priv);

	G_OBJECT_CLASS (git_stash_pane_parent_class)->finalize (object);
}

static GtkWidget *
git_stash_pane_get_widget (AnjutaDockPane *pane)
{
	GitStashPane *self;

	self = GIT_STASH_PANE (pane);

	return GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                           "stash_pane"));
}

static void
git_stash_pane_class_init (GitStashPaneClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaDockPaneClass *pane_class = ANJUTA_DOCK_PANE_CLASS (klass);

	object_class->finalize = git_stash_pane_finalize;
	pane_class->get_widget = git_stash_pane_get_widget;
	pane_class->refresh = NULL;
}


AnjutaDockPane *
git_stash_pane_new (Git *plugin)
{
	GitStashPane *self;
	GtkListStore *stash_list_model;

	self = g_object_new (GIT_TYPE_STASH_PANE, "plugin", plugin, NULL);
	stash_list_model = GTK_LIST_STORE (gtk_builder_get_object (self->priv->builder,
	                                                           "stash_list_model"));

	g_signal_connect (G_OBJECT (plugin->stash_list_command), "command-started",
	                  G_CALLBACK (on_stash_list_command_started),
	                  self);

	g_signal_connect (G_OBJECT (plugin->stash_list_command), "command-finished",
	                  G_CALLBACK (on_stash_list_command_finished),
	                  self);

	g_signal_connect (G_OBJECT (plugin->stash_list_command), "data-arrived",
	                  G_CALLBACK (on_stash_list_command_data_arrived),
	                  stash_list_model);

	return ANJUTA_DOCK_PANE (self);
}

gchar *
git_stash_pane_get_selected_stash_id (GitStashPane *self)
{
	GtkTreeView *stash_view;
	GtkTreeSelection *selection;
	GtkTreeModel *stash_list_model;
	GtkTreeIter iter;
	gchar *id;

	stash_view = GTK_TREE_VIEW (gtk_builder_get_object (self->priv->builder,
	                                                    "stash_view"));
	selection = gtk_tree_view_get_selection (stash_view);

	gtk_tree_selection_get_selected (selection, &stash_list_model, &iter);
	gtk_tree_model_get (stash_list_model, &iter, COL_ID, &id, -1);

	return id;
}

guint
git_stash_pane_get_selected_stash_number (GitStashPane *self)
{
	GtkTreeView *stash_view;
	GtkTreeSelection *selection;
	GtkTreeModel *stash_list_model;
	GtkTreeIter iter;
	guint number;

	stash_view = GTK_TREE_VIEW (gtk_builder_get_object (self->priv->builder,
	                                                    "stash_view"));
	selection = gtk_tree_view_get_selection (stash_view);

	gtk_tree_selection_get_selected (selection, &stash_list_model, &iter);
	gtk_tree_model_get (stash_list_model, &iter, COL_NUMBER, &number, -1);

	return number;
}
