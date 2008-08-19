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

#include "git-delete-remote-dialog.h"

static void
on_delete_remote_dialog_response (GtkDialog *dialog, gint response_id, 
								  GitBranchComboData *data)
{
	GtkWidget *delete_remote_branch_combo;
	gchar *branch;
	GtkTreeIter iter;
	GitRemoteDeleteCommand *delete_command;
	
	if (response_id == GTK_RESPONSE_OK)
	{	
		delete_remote_branch_combo = glade_xml_get_widget (data->gxml, 
														   "delete_remote_combo");

		gtk_combo_box_get_active_iter (GTK_COMBO_BOX (delete_remote_branch_combo), 
									   &iter);
		branch = git_branch_combo_model_get_branch (data->model, &iter);
		
		delete_command = git_remote_delete_command_new (data->plugin->project_root_directory,
														branch);
		
		g_free (branch);
		
		git_create_message_view (data->plugin);
		
		g_signal_connect (G_OBJECT (delete_command), "command-finished",
						  G_CALLBACK (on_git_command_finished),
						  data->plugin);
		
		g_signal_connect (G_OBJECT (delete_command), "data-arrived",
						  G_CALLBACK (on_git_command_info_arrived),
						  data->plugin);
		
		anjuta_command_start (ANJUTA_COMMAND (delete_command));
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	git_branch_combo_data_free (data);
}

static void
on_remote_list_command_data_arrived (AnjutaCommand *command, 
									 GtkListStore *model)
{
	GQueue *output_queue;
	gchar *remote_name;
	
	output_queue = git_raw_output_command_get_output (GIT_RAW_OUTPUT_COMMAND (command));
	
	while (g_queue_peek_head (output_queue))
	{
		remote_name = g_queue_pop_head (output_queue);
		git_branch_combo_model_append_remote (model, remote_name);
		g_free (remote_name);
	}
}

static void
on_remote_list_command_finished (AnjutaCommand *command, guint return_code,
								 GitBranchComboData *data)
{
	GtkWidget *delete_remote_ok_button;
	GtkWidget *delete_remote_combo;
	GtkTreeIter iter;
	
	delete_remote_ok_button = glade_xml_get_widget (data->gxml, 
													"delete_remote_ok_button");
	delete_remote_combo = glade_xml_get_widget (data->gxml,
												"delete_remote_combo");
	
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (data->model), 
									   &iter))
	{
		gtk_widget_set_sensitive (delete_remote_ok_button, TRUE);
		gtk_combo_box_set_active (GTK_COMBO_BOX (delete_remote_combo), 0);
	}
}

static void
delete_remote_dialog (Git *plugin)
{
	GladeXML *gxml;
	GtkWidget *dialog;
	GtkWidget *delete_remote_combo;
	GtkListStore *branch_list_store;
	GitBranchComboData *data;
	GitRemoteListCommand *list_command;
	
	gxml = glade_xml_new (GLADE_FILE, "delete_remote_dialog", NULL);
	
	dialog = glade_xml_get_widget (gxml, "delete_remote_dialog");
	delete_remote_combo = glade_xml_get_widget (gxml, 
												"delete_remote_combo");
	branch_list_store = git_branch_combo_model_new ();
	
	gtk_combo_box_set_model (GTK_COMBO_BOX (delete_remote_combo), 
							 GTK_TREE_MODEL (branch_list_store));
	git_branch_combo_model_setup_widget (delete_remote_combo);
	
	data = git_branch_combo_data_new (branch_list_store, 
									  GTK_COMBO_BOX (delete_remote_combo), gxml, 
									  plugin);
	
	list_command = git_remote_list_command_new (plugin->project_root_directory);
	
	g_signal_connect (G_OBJECT (list_command), "data-arrived", 
					  G_CALLBACK (on_remote_list_command_data_arrived), 
					  data->model);
	
	g_signal_connect (G_OBJECT (list_command), "command-finished", 
					  G_CALLBACK (on_remote_list_command_finished), 
					  data);
	
	anjuta_command_start (ANJUTA_COMMAND (list_command));
	
	g_signal_connect (G_OBJECT (dialog), "response", 
					  G_CALLBACK (on_delete_remote_dialog_response), 
					  data);
	
	gtk_widget_show_all (dialog);
}

void
on_menu_git_delete_remote (GtkAction *action, Git *plugin)
{
	delete_remote_dialog (plugin);
}
