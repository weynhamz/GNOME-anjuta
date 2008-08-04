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

#include "git-switch-dialog.h"

static void
on_checkout_command_finished (AnjutaCommand *command, guint return_code,
						   Git *plugin)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
									  NULL);
	
	anjuta_status (status, _("Git: Branch checkout complete."), 5);
	
	git_report_errors (command, return_code);
	
	g_object_unref (command);
}


static void
on_switch_dialog_response (GtkDialog *dialog, gint response_id, 
						   GitBranchComboData *data)
{
	GtkWidget *switch_branch_combo;
	gchar *branch;
	GtkTreeIter iter;
	GitBranchCheckoutCommand *checkout_command;
	
	if (response_id == GTK_RESPONSE_OK)
	{	
		switch_branch_combo = glade_xml_get_widget (data->gxml, "switch_branch_combo");

		gtk_combo_box_get_active_iter (GTK_COMBO_BOX (switch_branch_combo), &iter);
		branch = git_branch_combo_model_get_branch (data->model, &iter);
		
		checkout_command = git_branch_checkout_command_new (data->plugin->project_root_directory,
															branch);
		
		g_free (branch);
		
		git_create_message_view (data->plugin);
		
		g_signal_connect (G_OBJECT (checkout_command), "command-finished",
						  G_CALLBACK (on_checkout_command_finished),
						  data->plugin);
		
		g_signal_connect (G_OBJECT (checkout_command), "data-arrived",
						  G_CALLBACK (on_git_command_info_arrived),
						  data->plugin);
		
		anjuta_command_start (ANJUTA_COMMAND (checkout_command));
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	git_branch_combo_data_free (data);
}

static void
switch_dialog (Git *plugin)
{
	GladeXML *gxml;
	GtkWidget *dialog;
	GtkWidget *switch_branch_combo;
	GtkListStore *branch_list_store;
	GitBranchComboData *data;
	GitBranchListCommand *list_command;
	
	gxml = glade_xml_new (GLADE_FILE, "switch_dialog", NULL);
	
	dialog = glade_xml_get_widget (gxml, "switch_dialog");
	switch_branch_combo = glade_xml_get_widget (gxml, "switch_branch_combo");
	branch_list_store = git_branch_combo_model_new ();
	
	gtk_combo_box_set_model (GTK_COMBO_BOX (switch_branch_combo), 
							 GTK_TREE_MODEL (branch_list_store));
	git_branch_combo_model_setup_widget (switch_branch_combo);
	
	data = git_branch_combo_data_new (branch_list_store, 
									  GTK_COMBO_BOX (switch_branch_combo), gxml, 
									  plugin);
	
	list_command = git_branch_list_command_new (plugin->project_root_directory,
												GIT_BRANCH_TYPE_LOCAL);
	
	g_signal_connect (G_OBJECT (list_command), "data-arrived", 
					  G_CALLBACK (on_git_list_branch_command_data_arrived), 
					  data);
	
	g_signal_connect (G_OBJECT (list_command), "command-finished", 
					  G_CALLBACK (on_git_list_branch_command_finished), 
					  data);
	
	anjuta_command_start (ANJUTA_COMMAND (list_command));
	
	g_signal_connect (G_OBJECT (dialog), "response", 
					  G_CALLBACK (on_switch_dialog_response), 
					  data);
	
	gtk_widget_show_all (dialog);
}

void
on_menu_git_switch (GtkAction *action, Git *plugin)
{
	switch_dialog (plugin);
}
