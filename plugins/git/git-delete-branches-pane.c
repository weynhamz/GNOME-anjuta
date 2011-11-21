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

#include "git-delete-branches-pane.h"

struct _GitDeleteBranchesPanePriv
{
	GtkBuilder *builder;
};

G_DEFINE_TYPE (GitDeleteBranchesPane, git_delete_branches_pane, GIT_TYPE_PANE);

static void
on_ok_button_clicked (GtkButton *button, GitDeleteBranchesPane *self)
{
	Git *plugin;
	GtkToggleButton *require_merged_check;
	GList *selected_local_branches;
	GList *selected_remote_branches;
	GitBranchDeleteCommand *local_delete_command;
	GitBranchDeleteCommand *remote_delete_command;
	AnjutaCommandQueue *queue;

	plugin = ANJUTA_PLUGIN_GIT (anjuta_dock_pane_get_plugin (ANJUTA_DOCK_PANE (self)));
	require_merged_check = GTK_TOGGLE_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                                  "require_merged_check"));
	selected_local_branches = git_branches_pane_get_selected_local_branches (GIT_BRANCHES_PANE (plugin->branches_pane));
	selected_remote_branches = git_branches_pane_get_selected_remote_branches (GIT_BRANCHES_PANE (plugin->branches_pane));

	/* The user might not have selected anything */
	if (git_branches_pane_count_selected_items (GIT_BRANCHES_PANE (plugin->branches_pane)) > 0)
	{
		queue = anjuta_command_queue_new (ANJUTA_COMMAND_QUEUE_EXECUTE_MANUAL);
		
		if (selected_local_branches)
		{
			local_delete_command = git_branch_delete_command_new (plugin->project_root_directory,
			                                                      selected_local_branches,
			                                                      FALSE,
			                                                      gtk_toggle_button_get_active (require_merged_check));

			anjuta_util_glist_strings_free (selected_local_branches);

			g_signal_connect (G_OBJECT (local_delete_command), 
			                  "command-finished",
			                  G_CALLBACK (git_pane_report_errors),
			                  plugin);
			

			g_signal_connect (G_OBJECT (local_delete_command), 
			                  "command-finished",
			                  G_CALLBACK (g_object_unref),
			                  NULL);

			anjuta_command_queue_push (queue, 
			                           ANJUTA_COMMAND (local_delete_command));
		}

		if (selected_remote_branches)
		{
			remote_delete_command = git_branch_delete_command_new (plugin->project_root_directory,
			                                                       selected_remote_branches,
			                                                       TRUE,
			                                                       gtk_toggle_button_get_active (require_merged_check));

			anjuta_util_glist_strings_free (selected_remote_branches);

			g_signal_connect (G_OBJECT (remote_delete_command), 
			                  "command-finished",
			                  G_CALLBACK (git_pane_report_errors),
			                  plugin);


			g_signal_connect (G_OBJECT (remote_delete_command), "command-finished",
			                  G_CALLBACK (g_object_unref),
			                  NULL);
			
			anjuta_command_queue_push (queue, 
			                           ANJUTA_COMMAND (remote_delete_command));
		}

		/* Run the commands */
		g_signal_connect (G_OBJECT (queue), "finished",
		                  G_CALLBACK (g_object_unref),
		                  NULL);

		anjuta_command_queue_start (queue);
	}


	git_pane_remove_from_dock (GIT_PANE (self));
	                            
}

static void
git_delete_branches_pane_init (GitDeleteBranchesPane *self)
{
	gchar *objects[] = {"delete_branches_pane",
						NULL};
	GError *error = NULL;
	GtkButton *ok_button;
	GtkButton *cancel_button;

	self->priv = g_new0 (GitDeleteBranchesPanePriv, 1);
	self->priv->builder = gtk_builder_new ();

	if (!gtk_builder_add_objects_from_file (self->priv->builder, BUILDER_FILE, 
	                                        objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

	ok_button = GTK_BUTTON (gtk_builder_get_object (self->priv->builder, 
	                                                "ok_button"));
	cancel_button = GTK_BUTTON (gtk_builder_get_object (self->priv->builder,
	                                                    "cancel_button"));

	g_signal_connect (G_OBJECT (ok_button), "clicked",
	                  G_CALLBACK (on_ok_button_clicked),
	                  self);

	g_signal_connect_swapped (G_OBJECT (cancel_button), "clicked",
	                          G_CALLBACK (git_pane_remove_from_dock),
	                          self);
}

static void
git_delete_branches_pane_finalize (GObject *object)
{
	GitDeleteBranchesPane *self;

	self = GIT_DELETE_BRANCHES_PANE (object);

	g_object_unref (self->priv->builder);
	g_free (self->priv);

	G_OBJECT_CLASS (git_delete_branches_pane_parent_class)->finalize (object);
}

static GtkWidget *
get_widget (AnjutaDockPane *pane)
{
	GitDeleteBranchesPane *self;

	self = GIT_DELETE_BRANCHES_PANE (pane);

	return GTK_WIDGET (gtk_builder_get_object (self->priv->builder,
	                                           "delete_branches_pane"));
}

static void
git_delete_branches_pane_class_init (GitDeleteBranchesPaneClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaDockPaneClass* pane_class = ANJUTA_DOCK_PANE_CLASS (klass);

	object_class->finalize = git_delete_branches_pane_finalize;
	pane_class->get_widget = get_widget;
	pane_class->refresh = NULL;
}


AnjutaDockPane *
git_delete_branches_pane_new (Git *plugin)
{
	return g_object_new (GIT_TYPE_DELETE_BRANCHES_PANE, "plugin", plugin, NULL);
}

void
on_delete_branches_button_clicked (GtkAction *action, Git *plugin)
{
	AnjutaDockPane *delete_branches_pane;

	delete_branches_pane = git_delete_branches_pane_new (plugin);

	anjuta_dock_replace_command_pane (ANJUTA_DOCK (plugin->dock), "DeleteBranches",
	                                  "Delete Branches", NULL, delete_branches_pane,
	                                  GDL_DOCK_BOTTOM, NULL, 0, NULL);
}
