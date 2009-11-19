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

#include "git-delete-tag-dialog.h"

static void
on_delete_command_finished (AnjutaCommand *command, guint return_code,
							Git *plugin)
{
	AnjutaStatus *status;

	if (return_code == 0)
	{
		status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
										  NULL);
		anjuta_status (status, _("Git: Deleted selected tags."), 5);
	}
	
	git_report_errors (command, return_code);
	
	g_object_unref (command);
}

static void
on_delete_tag_dialog_response (GtkDialog *dialog, gint response_id, 
							   GitUIData *data)
{
	GtkWidget *delete_tag_view;
	GtkWidget *require_merged_check;
	GtkTreeModel *tag_list_model;
	GList *selected_tags;
	GitTagDeleteCommand *delete_command;
	
	if (response_id == GTK_RESPONSE_OK)
	{	
		delete_tag_view = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
		                                                      "delete_tag_view"));
		require_merged_check = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																   "require_merged_check"));
		tag_list_model = GTK_TREE_MODEL (gtk_builder_get_object (data->bxml,
		                                                         "tag_list_model"));

		selected_tags = NULL;
		gtk_tree_model_foreach (tag_list_model, 
								(GtkTreeModelForeachFunc) git_get_selected_refs,
								&selected_tags);
		
		delete_command = git_tag_delete_command_new (data->plugin->project_root_directory,
													 selected_tags);
		
		git_command_free_string_list (selected_tags);
		
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
delete_tag_dialog (Git *plugin)
{
	GtkBuilder *bxml;
	gchar *objects[] = {"delete_tag_dialog", "tag_list_model", NULL};
	GError *error;
	GtkWidget *dialog;
	GtkWidget *delete_tag_view;
	GtkListStore *tag_list_model;
	GtkCellRenderer *delete_tag_selected_renderer;
	GitUIData *data;
	GitTagListCommand *list_command;
	
	bxml = gtk_builder_new ();
	error = NULL;

	if (!gtk_builder_add_objects_from_file (bxml, BUILDER_FILE, objects,
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}
	
	dialog = GTK_WIDGET (gtk_builder_get_object (bxml, "delete_tag_dialog"));
	delete_tag_view = GTK_WIDGET (gtk_builder_get_object (bxml, "delete_tag_view"));
	tag_list_model = GTK_LIST_STORE (gtk_builder_get_object (bxml, 
	                                                         "tag_list_model"));
	delete_tag_selected_renderer = GTK_CELL_RENDERER (gtk_builder_get_object (bxml,
																			  "delete_tag_selected_renderer")),
	
	data = git_ui_data_new (plugin, bxml);
	                                     
	list_command = git_tag_list_command_new (plugin->project_root_directory);
	
	g_signal_connect (G_OBJECT (list_command), "data-arrived", 
					  G_CALLBACK (on_git_list_tag_command_data_arrived), 
					  tag_list_model);
	
	g_signal_connect (G_OBJECT (list_command), "command-finished", 
					  G_CALLBACK (on_git_command_finished), 
					  NULL);
	
	anjuta_command_start (ANJUTA_COMMAND (list_command));
	
	g_signal_connect (G_OBJECT (dialog), "response", 
					  G_CALLBACK (on_delete_tag_dialog_response), 
					  data);

	g_signal_connect (G_OBJECT (delete_tag_selected_renderer), "toggled",
					  G_CALLBACK (on_git_selected_column_toggled),
					  tag_list_model);
	
	gtk_widget_show_all (dialog);
}

void
on_menu_git_delete_tag (GtkAction *action, Git *plugin)
{
	delete_tag_dialog (plugin);
}
