/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2008 <jrliggett@cox.net>
 * 
 * anjuta is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include "git-delete-branch-dialog.h"

static void
on_delete_command_finished (AnjutaCommand *command, guint return_code,
							Git *plugin)
{
	AnjutaStatus *status;

	if (return_code == 0)
	{
		status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
										  NULL);
		anjuta_status (status, _("Git: Deleted selected branches."), 5);
	}
	
	git_report_errors (command, return_code);
	
	g_object_unref (command);
}

static void
on_list_branch_command_data_arrived (AnjutaCommand *command,
                                     GtkListStore *branch_list_model)
{
	GQueue *output_queue;
	GitBranch *branch;
	GtkTreeIter iter;
	gchar *name;
	
	output_queue = git_branch_list_command_get_output (GIT_BRANCH_LIST_COMMAND (command));

	while (g_queue_peek_head (output_queue))
	{
		branch = g_queue_pop_head (output_queue);
		name = git_branch_get_name (branch);

		if (!git_branch_is_active (branch))
		{
			gtk_list_store_append (branch_list_model, &iter);
			gtk_list_store_set (branch_list_model, &iter, 1, name, -1);
		}

		g_object_unref (branch);
		g_free (name);
	}
}

static void
on_delete_branch_dialog_response (GtkDialog *dialog, gint response_id, 
								  GitUIData *data)
{
	GtkWidget *delete_branch_remote_toggle;
	GtkWidget *delete_branch_view;
	GtkWidget *require_merged_check;
	GtkTreeModel *model;
	GList *selected_branches;
	GitBranchDeleteCommand *delete_command;
	GtkWidget *error_dialog;
	
	if (response_id == GTK_RESPONSE_OK)
	{	
		delete_branch_remote_toggle = GTK_WIDGET (gtk_builder_get_object (data->bxml,
		                                                                  "delete_branch_remote_toggle"));
		delete_branch_view = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
		                                                         "delete_branch_view"));
		require_merged_check = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																   "require_merged_check"));
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (delete_branch_view));

		selected_branches = NULL;
		gtk_tree_model_foreach (model, 
		                        (GtkTreeModelForeachFunc) git_get_selected_refs,
		                        &selected_branches);

		if (!selected_branches)
		{
			error_dialog = gtk_message_dialog_new (GTK_WINDOW (dialog),
			                                       GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			                                       GTK_MESSAGE_WARNING,
			                                       GTK_BUTTONS_CLOSE,
			                                       "%s",
			                                       _("Please select branches to delete"));

			gtk_dialog_run (GTK_DIALOG (error_dialog));
			gtk_widget_destroy (error_dialog);

			return;
			                                 
		}
		                      
		
		delete_command = git_branch_delete_command_new (data->plugin->project_root_directory,
														selected_branches,
		                                                gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (delete_branch_remote_toggle)),
														gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (require_merged_check)));
		
		git_command_free_string_list (selected_branches);
		
		git_create_message_view (data->plugin);
		
		g_signal_connect (G_OBJECT (delete_command), "command-finished",
						  G_CALLBACK (on_delete_command_finished),
						  data->plugin);
		
		g_signal_connect (G_OBJECT (delete_command), "data-arrived",
						  G_CALLBACK (on_git_command_info_arrived),
						  data->plugin);
		
		anjuta_command_start (ANJUTA_COMMAND (delete_command));
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	git_ui_data_free (data);
}

static void
on_branch_type_toggle_toggled (GtkToggleButton *toggle_button,
                               GitUIData *data)
{
	GtkWidget *delete_branch_view;
	GtkTreeModel *old_model;
	GtkTreeModel *new_model;
	GtkCellRenderer *delete_branch_selected_renderer;

	delete_branch_view = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
	                                                         "delete_branch_view"));
	old_model = gtk_tree_view_get_model (GTK_TREE_VIEW (delete_branch_view));
	new_model = g_object_get_data (G_OBJECT (toggle_button), "model");
	delete_branch_selected_renderer = GTK_CELL_RENDERER (gtk_builder_get_object (data->bxml,
																				 "delete_branch_selected_renderer"));
	/* Disconnect the toggled signal on the cell render and then reconnect it
	 * with the new model */
	g_signal_handlers_disconnect_by_func (delete_branch_selected_renderer,
	                                      on_git_selected_column_toggled,
	                                      old_model);

	gtk_tree_view_set_model (GTK_TREE_VIEW (delete_branch_view), new_model);

	g_signal_connect (G_OBJECT (delete_branch_selected_renderer), "toggled",
	                  G_CALLBACK (on_git_selected_column_toggled),
	                  new_model);
}

static void
delete_branch_dialog (Git *plugin)
{
	GtkBuilder *bxml;
	gchar *objects[] = {"delete_branch_dialog", "branch_list_model",
						"remote_branch_list_model", NULL};
	GError *error;
	GtkWidget *dialog;
	GtkWidget *delete_branch_local_toggle;
	GtkWidget *delete_branch_remote_toggle;
	GtkWidget *delete_branch_view;
	GtkListStore *branch_list_model;
	GtkListStore *remote_branch_list_model;
	GtkCellRenderer *delete_branch_selected_renderer;
	GitUIData *data;
	GitBranchListCommand *local_list_command;
	GitBranchListCommand *remote_list_command;
	
	bxml = gtk_builder_new ();
	error = NULL;

	if (!gtk_builder_add_objects_from_file (bxml, BUILDER_FILE, objects,
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}
	
	dialog = GTK_WIDGET (gtk_builder_get_object (bxml, "delete_branch_dialog"));
	delete_branch_local_toggle = GTK_WIDGET (gtk_builder_get_object (bxml,
	                                                                 "delete_branch_local_toggle"));
	delete_branch_remote_toggle = GTK_WIDGET (gtk_builder_get_object (bxml,
	                                                                  "delete_branch_remote_toggle"));
	delete_branch_view = GTK_WIDGET (gtk_builder_get_object (bxml, "delete_branch_view"));
	branch_list_model = GTK_LIST_STORE (gtk_builder_get_object (bxml, 
	                                                            "branch_list_model"));
	remote_branch_list_model = GTK_LIST_STORE (gtk_builder_get_object (bxml,
	                                                                   "remote_branch_list_model"));
	delete_branch_selected_renderer = GTK_CELL_RENDERER (gtk_builder_get_object (bxml,
																				 "delete_branch_selected_renderer"));
	
	data = git_ui_data_new (plugin, bxml);

	local_list_command = git_branch_list_command_new (plugin->project_root_directory,
	                                                  GIT_BRANCH_TYPE_LOCAL);
	remote_list_command = git_branch_list_command_new (plugin->project_root_directory,
	                                                   GIT_BRANCH_TYPE_REMOTE);
	
	g_signal_connect (G_OBJECT (local_list_command), "data-arrived", 
					  G_CALLBACK (on_list_branch_command_data_arrived), 
					  branch_list_model);
	
	g_signal_connect (G_OBJECT (local_list_command), "command-finished", 
					  G_CALLBACK (on_git_command_finished), 
					  NULL);

	g_signal_connect (G_OBJECT (remote_list_command), "data-arrived", 
					  G_CALLBACK (on_list_branch_command_data_arrived), 
					  remote_branch_list_model);
	
	g_signal_connect (G_OBJECT (remote_list_command), "command-finished", 
					  G_CALLBACK (on_git_command_finished), 
					  NULL);
	
	anjuta_command_start (ANJUTA_COMMAND (local_list_command));
	anjuta_command_start (ANJUTA_COMMAND (remote_list_command));
	
	g_signal_connect (G_OBJECT (dialog), "response", 
					  G_CALLBACK (on_delete_branch_dialog_response), 
					  data);

	g_signal_connect (G_OBJECT (delete_branch_selected_renderer), "toggled",
					  G_CALLBACK (on_git_selected_column_toggled),
					  branch_list_model);

	g_object_set_data (G_OBJECT (delete_branch_local_toggle), "model", 
	                   branch_list_model);
	g_object_set_data (G_OBJECT (delete_branch_remote_toggle), "model",
	                   remote_branch_list_model);

	g_signal_connect (G_OBJECT (delete_branch_local_toggle), "toggled",
	                  G_CALLBACK (on_branch_type_toggle_toggled),
	                  data);

	g_signal_connect (G_OBJECT (delete_branch_remote_toggle), "toggled",
	                  G_CALLBACK (on_branch_type_toggle_toggled),
	                  data);
	
	gtk_widget_show_all (dialog);
}

void
on_menu_git_delete_branch (GtkAction *action, Git *plugin)
{
	delete_branch_dialog (plugin);
}
