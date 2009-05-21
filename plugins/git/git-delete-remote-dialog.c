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
								  GitUIData *data)
{
	GtkWidget *delete_remote_branch_combo;
	GtkTreeModel *branch_combo_model;
	gchar *branch;
	GtkTreeIter iter;
	GitRemoteDeleteCommand *delete_command;
	
	if (response_id == GTK_RESPONSE_OK)
	{	
		delete_remote_branch_combo = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
																		 "delete_remote_combo"));
		branch_combo_model = GTK_TREE_MODEL (gtk_builder_get_object (data->bxml,
		                                                             "branch_combo_model"));

		gtk_combo_box_get_active_iter (GTK_COMBO_BOX (delete_remote_branch_combo), 
									   &iter);
		gtk_tree_model_get (branch_combo_model, &iter, 0, &branch, -1);
		
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
	git_ui_data_free (data);
}

static void
on_remote_list_command_data_arrived (AnjutaCommand *command, 
									 GtkListStore *model)
{
	GQueue *output_queue;
	gchar *remote_name;
	GtkTreeIter iter;
	
	output_queue = git_raw_output_command_get_output (GIT_RAW_OUTPUT_COMMAND (command));
	
	while (g_queue_peek_head (output_queue))
	{
		remote_name = g_queue_pop_head (output_queue);

		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter, 0, remote_name, -1);
		
		g_free (remote_name);
	}
}

static void
on_remote_list_command_finished (AnjutaCommand *command, guint return_code,
								 GtkBuilder *bxml)
{
	GtkWidget *delete_remote_ok_button;
	GtkWidget *delete_remote_combo;
	GtkTreeModel *branch_combo_model;
	GtkTreeIter iter;
	
	delete_remote_ok_button = GTK_WIDGET (gtk_builder_get_object (bxml, 
																  "delete_remote_ok_button"));
	delete_remote_combo = GTK_WIDGET (gtk_builder_get_object (bxml,
															  "delete_remote_combo"));
	branch_combo_model = GTK_TREE_MODEL (gtk_builder_get_object (bxml,
	                                                             "branch_combo_model"));
	
	if (gtk_tree_model_get_iter_first (branch_combo_model, &iter))
	{
		gtk_widget_set_sensitive (delete_remote_ok_button, TRUE);
		gtk_combo_box_set_active (GTK_COMBO_BOX (delete_remote_combo), 0);
	}
}

static void
delete_remote_dialog (Git *plugin)
{
	GtkBuilder *bxml;
	gchar *objects[] = {"delete_remote_dialog", "branch_combo_model", NULL};
	GError *error;
	GtkWidget *dialog;
	GtkWidget *delete_remote_combo;
	GtkListStore *branch_combo_model;
	GitUIData *data;
	GitRemoteListCommand *list_command;
	
	bxml = gtk_builder_new ();
	error = NULL;

	if (!gtk_builder_add_objects_from_file (bxml, BUILDER_FILE, objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}
	
	dialog = GTK_WIDGET (gtk_builder_get_object (bxml, "delete_remote_dialog"));
	delete_remote_combo = GTK_WIDGET (gtk_builder_get_object (bxml, 
															  "delete_remote_combo"));
	branch_combo_model = GTK_LIST_STORE (gtk_builder_get_object (bxml,
	                                                             "branch_combo_model"));
	
	data = git_ui_data_new (plugin, bxml);
	
	list_command = git_remote_list_command_new (plugin->project_root_directory);
	
	g_signal_connect (G_OBJECT (list_command), "data-arrived", 
					  G_CALLBACK (on_remote_list_command_data_arrived), 
					  branch_combo_model);
	
	g_signal_connect (G_OBJECT (list_command), "command-finished", 
					  G_CALLBACK (on_remote_list_command_finished), 
					  bxml);
	
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
