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

#include "git-create-patch-series-dialog.h"

static void
on_create_patch_series_dialog_response (GtkDialog *dialog, gint response_id, 
										GitBranchComboData *data)
{
	GtkWidget *patch_series_origin_check;
	GtkWidget *patch_series_branch_combo;
	GtkWidget *patch_series_file_chooser_button;
	GtkWidget *patch_series_signoff_check;
	gchar *branch;
	gchar *output_directory;
	GtkTreeIter iter;
	GitFormatPatchCommand *format_patch_command;
	
	if (response_id == GTK_RESPONSE_OK)
	{	
		patch_series_origin_check = glade_xml_get_widget (data->gxml, 
		                                                  "patch_series_origin_check");
		patch_series_branch_combo = glade_xml_get_widget (data->gxml, 
														  "patch_series_branch_combo");
		patch_series_file_chooser_button = glade_xml_get_widget (data->gxml, 
																 "patch_series_file_chooser_button");
		patch_series_signoff_check = glade_xml_get_widget (data->gxml, 
														   "patch_series_signoff_check");
		
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (patch_series_origin_check)))
		    branch = g_strdup ("origin");
		else
		{
			gtk_combo_box_get_active_iter (GTK_COMBO_BOX (patch_series_branch_combo), 
									   &iter);
			branch = git_branch_combo_model_get_branch (data->model, &iter);
		}
		    
		output_directory = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (patch_series_file_chooser_button));
		
		format_patch_command = git_format_patch_command_new (data->plugin->project_root_directory,
															 output_directory,
															 branch,
															 gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (patch_series_signoff_check)));
		
		g_free (branch);
		g_free (output_directory);
		
		git_create_message_view (data->plugin);
		
		g_signal_connect (G_OBJECT (format_patch_command), "command-finished",
						  G_CALLBACK (on_git_command_finished),
						  data->plugin);
		
		g_signal_connect (G_OBJECT (format_patch_command), "data-arrived",
						  G_CALLBACK (on_git_command_info_arrived),
						  data->plugin);
		
		anjuta_command_start (ANJUTA_COMMAND (format_patch_command));
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	git_branch_combo_data_free (data);
}

static void
create_patch_series_dialog (Git *plugin)
{
	GladeXML *gxml;
	GtkWidget *dialog;
	GtkWidget *patch_series_origin_check;
	GtkWidget *patch_series_branch_combo;
	GtkListStore *branch_list_store;
	GitBranchComboData *data;
	GitBranchListCommand *list_command;
	
	gxml = glade_xml_new (GLADE_FILE, "patch_series_dialog", NULL);
	
	dialog = glade_xml_get_widget (gxml, "patch_series_dialog");
	patch_series_origin_check = glade_xml_get_widget (gxml, 
	                                                  "patch_series_origin_check");
	patch_series_branch_combo = glade_xml_get_widget (gxml, 
													  "patch_series_branch_combo");
	branch_list_store = git_branch_combo_model_new ();
	
	gtk_combo_box_set_model (GTK_COMBO_BOX (patch_series_branch_combo), 
							 GTK_TREE_MODEL (branch_list_store));
	git_branch_combo_model_setup_widget (patch_series_branch_combo);
	
	data = git_branch_combo_data_new (branch_list_store, 
									  GTK_COMBO_BOX (patch_series_branch_combo),  
									  gxml, plugin);
	
	list_command = git_branch_list_command_new (plugin->project_root_directory,
												GIT_BRANCH_TYPE_ALL);
	
	g_signal_connect (G_OBJECT (list_command), "data-arrived", 
					  G_CALLBACK (on_git_list_branch_command_data_arrived), 
					  data);
	
	g_signal_connect (G_OBJECT (list_command), "command-finished", 
					  G_CALLBACK (on_git_list_branch_command_finished), 
					  data);
	
	anjuta_command_start (ANJUTA_COMMAND (list_command));
	
	g_signal_connect (G_OBJECT (dialog), "response", 
					  G_CALLBACK (on_create_patch_series_dialog_response), 
					  data);

	g_signal_connect (G_OBJECT (patch_series_origin_check), "toggled",
	                  G_CALLBACK (on_git_origin_check_toggled),
	                  patch_series_branch_combo);
	
	gtk_widget_show_all (dialog);
}

void
on_menu_git_create_patch_series (GtkAction *action, Git *plugin)
{
	create_patch_series_dialog (plugin);
}
