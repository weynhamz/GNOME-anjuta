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
						   GitBranchComboData *data)
{
	GtkWidget *rebase_branch_combo;
	gchar *branch;
	GtkTreeIter iter;
	GitRebaseStartCommand *rebase_command;
	GitProgressData *progress_data;
	
	if (response_id == GTK_RESPONSE_OK)
	{	
		rebase_branch_combo = glade_xml_get_widget (data->gxml, 
													"rebase_branch_combo");

		gtk_combo_box_get_active_iter (GTK_COMBO_BOX (rebase_branch_combo), 
									   &iter);
		branch = git_branch_combo_model_get_branch (data->model, &iter);
		
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
	git_branch_combo_data_free (data);
}

static void
rebase_dialog (Git *plugin)
{
	GladeXML *gxml;
	GtkWidget *dialog;
	GtkWidget *rebase_branch_combo;
	GtkListStore *branch_list_store;
	GitBranchComboData *data;
	GitBranchListCommand *list_command;
	
	gxml = glade_xml_new (GLADE_FILE, "rebase_dialog", NULL);
	
	dialog = glade_xml_get_widget (gxml, "rebase_dialog");
	rebase_branch_combo = glade_xml_get_widget (gxml, "rebase_branch_combo");
	branch_list_store = git_branch_combo_model_new ();
	
	gtk_combo_box_set_model (GTK_COMBO_BOX (rebase_branch_combo), 
							 GTK_TREE_MODEL (branch_list_store));
	git_branch_combo_model_setup_widget (rebase_branch_combo);
	
	data = git_branch_combo_data_new (branch_list_store, 
									  GTK_COMBO_BOX (rebase_branch_combo), gxml, 
									  plugin);
	
	list_command = git_branch_list_command_new (plugin->project_root_directory,
												GIT_BRANCH_TYPE_REMOTE);
	
	g_signal_connect (G_OBJECT (list_command), "data-arrived", 
					  G_CALLBACK (on_git_list_branch_command_data_arrived), 
					  data);
	
	g_signal_connect (G_OBJECT (list_command), "command-finished", 
					  G_CALLBACK (on_git_list_branch_command_finished), 
					  data);
	
	anjuta_command_start (ANJUTA_COMMAND (list_command));
	
	g_signal_connect (G_OBJECT (dialog), "response", 
					  G_CALLBACK (on_rebase_dialog_response), 
					  data);
	
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
