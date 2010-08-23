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

#include "git-log-pane.h"

enum
{
	BRANCH_COL_ACTIVE,
	BRANCH_COL_ACTIVE_ICON,
	BRANCH_COL_NAME
};

struct _GitLogPanePriv
{
	GtkBuilder *builder;
	GtkListStore *log_model;

	/* This table maps branch names and iters in the branch combo model. When
	 * branches get refreshed, use this to make sure that the same branch the
	 * user was looking at stays selected, unless that branch no longer exists.
	 * In that case the new active branch is selected */
	GHashTable *branches_table;
	gchar *selected_branch;
	gboolean viewing_active_branch;
	GtkTreeIter active_branch_iter;
};

G_DEFINE_TYPE (GitLogPane, git_log_pane, GIT_TYPE_PANE);

static void
on_local_branch_list_command_started (AnjutaCommand *command, 
                                      GitLogPane *self)
{
	GtkComboBox *branch_combo;
	GtkListStore *log_branch_combo_model;

	branch_combo = GTK_COMBO_BOX (gtk_builder_get_object (self->priv->builder,
	                                                       "branch_combo"));
	log_branch_combo_model = GTK_LIST_STORE (gtk_builder_get_object (self->priv->builder,
	                                                                 "log_branch_combo_model"));

	gtk_combo_box_set_model (branch_combo, NULL);
	gtk_list_store_clear (log_branch_combo_model);

	g_hash_table_remove_all (self->priv->branches_table);
}

static void
on_remote_branch_list_command_finished (AnjutaCommand *command,
                                        guint return_code,
                                        GitLogPane *self)
{
	GtkComboBox *branch_combo;
	GtkTreeModel *log_branch_combo_model;
	GtkTreeIter *iter;

	branch_combo = GTK_COMBO_BOX (gtk_builder_get_object (self->priv->builder,
	                                                          "branch_combo"));
	log_branch_combo_model = GTK_TREE_MODEL (gtk_builder_get_object (self->priv->builder,
	                                                                 "log_branch_combo_model"));

	gtk_combo_box_set_model (branch_combo, log_branch_combo_model);

	/* If the user was viewing the active branch, switch to the newly active
	 * branch if it changes. If another branch was being viewed, stay on that
	 * one */
	if ((!self->priv->viewing_active_branch) && 
	    (self->priv->selected_branch && 
	    g_hash_table_lookup_extended (self->priv->branches_table, 
	                                  self->priv->selected_branch, NULL, 
	                                  (gpointer) &iter)))
	{
		gtk_combo_box_set_active_iter (branch_combo, iter);
	}
	else
	{
		gtk_combo_box_set_active_iter (branch_combo, 
		                               &(self->priv->active_branch_iter));
	}
	
}

static void
on_branch_list_command_data_arrived (AnjutaCommand *command,
                                     GitLogPane *self)
{
	GtkListStore *log_branch_combo_model;
	AnjutaPlugin *plugin;
	GList *current_branch;
	GitBranch *branch;
	gchar *name;
	GtkTreeIter iter;

	log_branch_combo_model = GTK_LIST_STORE (gtk_builder_get_object (self->priv->builder,
	                                                                 "log_branch_combo_model"));
	plugin = anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (self));
	current_branch = git_branch_list_command_get_output (GIT_BRANCH_LIST_COMMAND (command));

	while (current_branch)
	{
		branch = current_branch->data;
		name = git_branch_get_name (branch);

		gtk_list_store_append (log_branch_combo_model, &iter);

		if (git_branch_is_active (branch))
		{
			gtk_list_store_set (log_branch_combo_model, &iter,
			                    BRANCH_COL_ACTIVE, TRUE,
			                    BRANCH_COL_ACTIVE_ICON, GTK_STOCK_APPLY,
			                    -1);

			self->priv->active_branch_iter = iter;
		}
		else
		{
			gtk_list_store_set (log_branch_combo_model, &iter,
			                    BRANCH_COL_ACTIVE, FALSE,
			                    BRANCH_COL_ACTIVE_ICON, NULL,
			                    -1);
		}

		gtk_list_store_set (log_branch_combo_model, &iter, 
		                    BRANCH_COL_NAME, name, 
		                    -1);
		g_hash_table_insert (self->priv->branches_table, g_strdup (name), 
		                     g_memdup (&iter, sizeof (GtkTreeIter)));

		g_free (name);
		
		current_branch = g_list_next (current_branch);
	}
	
}

static void
on_branch_combo_changed (GtkComboBox *combo_box, GitLogPane *self)
{
	GtkTreeModel *log_branch_combo_model;
	gchar *branch;
	GtkTreeIter iter;
	gboolean active;

	log_branch_combo_model = gtk_combo_box_get_model (combo_box);

	if (gtk_combo_box_get_active_iter (combo_box, &iter))
	{
		gtk_tree_model_get (log_branch_combo_model, &iter,
		                    BRANCH_COL_ACTIVE, &active, 
							BRANCH_COL_NAME, &branch, 
							-1);

		self->priv->viewing_active_branch = active;

		g_free (self->priv->selected_branch);
		self->priv->selected_branch = g_strdup (branch);

		g_free (branch);
	}
}

static void
git_log_pane_init (GitLogPane *self)
{
	gchar *objects[] = {"log_pane",
						"log_branch_combo_model",
						"log_loading_model",
						"find_button_image",
						NULL};
	GError *error = NULL;
	GtkTreeView *log_view;
	GtkComboBox *branch_combo;

	self->priv = g_new0 (GitLogPanePriv, 1);
	self->priv->builder = gtk_builder_new ();

	if (!gtk_builder_add_objects_from_file (self->priv->builder, BUILDER_FILE, 
	                                        objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

	log_view = GTK_TREE_VIEW (gtk_builder_get_object (self->priv->builder,
	                                                  "log_view"));
	branch_combo = GTK_COMBO_BOX (gtk_builder_get_object (self->priv->builder,
	                                                      "branch_combo"));

	/* Set up the log model */
	self->priv->log_model = gtk_list_store_new (1, GIT_TYPE_REVISION);

	gtk_tree_view_set_model (log_view, GTK_TREE_MODEL (self->priv->log_model));

	/* Branch handling */
	self->priv->branches_table = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                                    g_free, g_free);

	g_signal_connect (G_OBJECT (branch_combo), "changed",
	                  G_CALLBACK (on_branch_combo_changed),
	                  self);
	
}

static void
git_log_pane_finalize (GObject *object)
{
	GitLogPane *self;

	self = GIT_LOG_PANE (object);

	g_object_unref (self->priv->builder);
	g_hash_table_destroy (self->priv->branches_table);
	g_free (self->priv->selected_branch);
	g_free (self->priv);

	G_OBJECT_CLASS (git_log_pane_parent_class)->finalize (object);
}

static GtkWidget *
git_log_pane_get_widget (AnjutaDockPane *pane)
{
	GitLogPane *self;

	self = GIT_LOG_PANE (pane);

	return GTK_WIDGET (gtk_builder_get_object (self->priv->builder, 
	                                           "log_pane"));
}

static void
git_log_pane_class_init (GitLogPaneClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	AnjutaDockPaneClass *pane_class = ANJUTA_DOCK_PANE_CLASS (klass);

	object_class->finalize = git_log_pane_finalize;
	pane_class->get_widget = git_log_pane_get_widget;
	pane_class->refresh = NULL;
}


AnjutaDockPane *
git_log_pane_new (Git *plugin)
{
	GitLogPane *self;

	self = g_object_new (GIT_TYPE_LOG_PANE, "plugin", plugin, NULL);

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
	                  G_CALLBACK (on_branch_list_command_data_arrived),
	                  self);

	g_signal_connect (G_OBJECT (plugin->remote_branch_list_command),
	                  "data-arrived",
	                  G_CALLBACK (on_branch_list_command_data_arrived),
	                  self);

	return ANJUTA_DOCK_PANE (self);
}

void
git_log_pane_set_working_directory (GitLogPane *self, 
                                    const gchar *working_directory)
{
	/* TODO: Add public function implementation here */
}
