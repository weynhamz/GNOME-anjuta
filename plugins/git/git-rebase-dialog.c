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

#include "git-rebase-dialog.h"

static void
on_rebase_dialog_response (GtkDialog *dialog, gint response_id, 
						   GitUIData *data)
{
	GtkWidget *rebase_branch_combo;
	GtkWidget *rebase_origin_check;
	GtkTreeModel *branch_combo_model;
	gchar *branch;
	GtkTreeIter iter;
	GitRebaseStartCommand *rebase_command;
	GitProgressData *progress_data;
	
	if (response_id == GTK_RESPONSE_OK)
	{	
		rebase_branch_combo = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
																  "rebase_branch_combo"));
		rebase_origin_check = GTK_WIDGET (gtk_builder_get_object (data->bxml,
		                                        				  "rebase_origin_check"));
		branch_combo_model = GTK_TREE_MODEL (gtk_builder_get_object (data->bxml,
		                                                             "branch_combo_model"));

		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (rebase_origin_check)))
		    branch = g_strdup ("origin");
		else
		{
			gtk_combo_box_get_active_iter (GTK_COMBO_BOX (rebase_branch_combo), 
										   &iter);
			gtk_tree_model_get (branch_combo_model, &iter, 0, &branch, -1);
		}

		rebase_command = git_rebase_start_command_new (data->plugin->project_root_directory,
													   branch);
		progress_data = git_progress_data_new (data->plugin,
											   _("Git: Rebasing"));
		
		g_free (branch);
		
		git_create_message_view (data->plugin);
		
		g_signal_connect (G_OBJECT (rebase_command), "command-finished",
						  G_CALLBACK (on_git_command_finished),
						  data->plugin);
		
		g_signal_connect (G_OBJECT (rebase_command), "data-arrived",
						  G_CALLBACK (on_git_command_info_arrived),
						  data->plugin);
		
		g_signal_connect (G_OBJECT (rebase_command), "progress",
						  G_CALLBACK (on_git_command_progress),
						  data->plugin);
		
		g_signal_connect_swapped (G_OBJECT (rebase_command), 
								  "command-finished",
								  G_CALLBACK (git_progress_data_free),
								  progress_data);
		
		anjuta_command_start (ANJUTA_COMMAND (rebase_command));
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	git_ui_data_free (data);
}

static void
rebase_dialog (Git *plugin)
{
	GtkBuilder *bxml;
	gchar *objects[] = {"rebase_dialog", "branch_combo_model", NULL};
	GError *error;
	GtkWidget *dialog;
	GtkWidget *rebase_branch_combo;
	GtkWidget *rebase_origin_check;
	GtkListStore *branch_combo_model;
	GitUIData *data;
	GitBranchListCommand *list_command;
	
	bxml = gtk_builder_new ();
	error = NULL;

	if (!gtk_builder_add_objects_from_file (bxml, BUILDER_FILE, objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}
	
	dialog = GTK_WIDGET (gtk_builder_get_object (bxml, "rebase_dialog"));
	rebase_branch_combo = GTK_WIDGET (gtk_builder_get_object (bxml, 
	                                                          "rebase_branch_combo"));
	rebase_origin_check = GTK_WIDGET (gtk_builder_get_object (bxml, 
	                                                          "rebase_origin_check"));
	branch_combo_model = GTK_LIST_STORE (gtk_builder_get_object (bxml,
	                                                             "branch_combo_model"));
	
	data = git_ui_data_new (plugin, bxml);
	
	list_command = git_branch_list_command_new (plugin->project_root_directory,
												GIT_BRANCH_TYPE_REMOTE);
	
	g_signal_connect (G_OBJECT (list_command), "data-arrived", 
					  G_CALLBACK (on_git_list_branch_combo_command_data_arrived), 
					  branch_combo_model);
	
	g_signal_connect (G_OBJECT (list_command), "command-finished", 
					  G_CALLBACK (on_git_list_branch_combo_command_finished), 
					  rebase_branch_combo);
	
	anjuta_command_start (ANJUTA_COMMAND (list_command));
	
	g_signal_connect (G_OBJECT (dialog), "response", 
					  G_CALLBACK (on_rebase_dialog_response), 
					  data);

	g_signal_connect (G_OBJECT (rebase_origin_check), "toggled",
	                  G_CALLBACK (on_git_origin_check_toggled),
	                  rebase_branch_combo);
	
	gtk_widget_show_all (dialog);
}

static void
rebase_continue (Git *plugin, GitRebaseContinueAction action)
{
	GitRebaseContinueCommand *rebase_command;
	
	git_create_message_view (plugin);
	
	rebase_command = git_rebase_continue_command_new (plugin->project_root_directory,
													  action);
	
	g_signal_connect (G_OBJECT (rebase_command), "command-finished",
					  G_CALLBACK (on_git_command_finished),
					  plugin);
	
	g_signal_connect (G_OBJECT (rebase_command), "data-arrived",
					  G_CALLBACK (on_git_command_info_arrived),
					  plugin);
	
	anjuta_command_start (ANJUTA_COMMAND (rebase_command));
}

void
on_menu_git_rebase_start (GtkAction *action, Git *plugin)
{
	rebase_dialog (plugin);
}

void 
on_menu_git_rebase_continue (GtkAction *action, Git *plugin)
{
	rebase_continue (plugin, GIT_REBASE_CONTINUE_ACTION_CONTINUE);
}

void 
on_menu_git_rebase_skip (GtkAction *action, Git *plugin)
{
	rebase_continue (plugin, GIT_REBASE_CONTINUE_ACTION_SKIP);
}

void 
on_menu_git_rebase_abort (GtkAction *action, Git *plugin)
{
	rebase_continue (plugin, GIT_REBASE_CONTINUE_ACTION_ABORT);
}
