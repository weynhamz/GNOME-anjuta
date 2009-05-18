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

#include "git-pull-dialog.h"

static void
on_pull_command_finished (AnjutaCommand *command, guint return_code,
						  Git *plugin)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
									  NULL);
	
	anjuta_status (status, _("Git: Pull complete."), 5);
	
	git_report_errors (command, return_code);
	
	g_object_unref (command);
}


static void
on_pull_dialog_response (GtkDialog *dialog, gint response_id, 
						 GitUIData *data)
{
	GtkWidget *pull_origin_check;
	GtkWidget *pull_rebase_check;
	GtkWidget *pull_url_entry;
	GtkWidget *pull_no_commit_check;
	GtkWidget *pull_squash_check;
	GtkWidget *pull_fast_forward_commit_check;
	GtkWidget *pull_append_fetch_data_check;
	GtkWidget *pull_force_check;
	GtkWidget *pull_no_follow_tags_check;
	gchar *url;
	GitPullCommand *pull_command;
	
	if (response_id == GTK_RESPONSE_OK)
	{	
		pull_origin_check = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
		                                      					"pull_origin_check"));
		pull_rebase_check = GTK_WIDGET (gtk_builder_get_object (data->bxml,
		                                      					"pull_rebase_check"));
		pull_url_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
		                                                     "pull_url_entry"));
		pull_no_commit_check = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
																   "pull_no_commit_check"));
		pull_squash_check = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																"pull_squash_check"));
		pull_fast_forward_commit_check = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																			 "pull_fast_forward_commit_check"));
		pull_append_fetch_data_check = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																			"pull_append_fetch_data_check"));
		pull_force_check = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
															   "pull_force_check"));
		pull_no_follow_tags_check = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																		"pull_no_follow_tags_check"));
		
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pull_origin_check)))
		    url = g_strdup ("origin");
		else
		    url = gtk_editable_get_chars (GTK_EDITABLE (pull_url_entry), 0, -1);
		
		if (!git_check_input (GTK_WIDGET (dialog), pull_url_entry, url,
							  _("Please enter the URL of the repository to pull" 
								" from.")))
		{
			g_free (url);
			return;
		}
		
		pull_command = git_pull_command_new (data->plugin->project_root_directory,
											 url,
		                                     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pull_rebase_check)),
											 gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pull_no_commit_check)),
											 gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pull_squash_check)),
											 gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pull_fast_forward_commit_check)),
											 gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pull_append_fetch_data_check)),
											 gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pull_force_check)),
											 gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pull_no_follow_tags_check)));
		
		g_free (url);
		
		git_create_message_view (data->plugin);
		
		g_signal_connect (G_OBJECT (pull_command), "command-finished",
						  G_CALLBACK (on_pull_command_finished),
						  data->plugin);
		
		g_signal_connect (G_OBJECT (pull_command), "data-arrived",
						  G_CALLBACK (on_git_command_info_arrived),
						  data->plugin);
		
		anjuta_command_start (ANJUTA_COMMAND (pull_command));
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	git_ui_data_free (data);
}

static void
pull_dialog (Git *plugin)
{
	GtkBuilder *bxml;
	gchar *objects[] = {"pull_dialog", NULL};
	GError *error;
	GtkWidget *dialog;
	GtkWidget *pull_origin_check;
	GtkWidget *pull_url_entry;
	GitUIData *data;
	
	bxml = gtk_builder_new ();
	error = NULL;

	if (!gtk_builder_add_objects_from_file (bxml, BUILDER_FILE, objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}
	
	dialog = GTK_WIDGET (gtk_builder_get_object (bxml, "pull_dialog"));
	pull_origin_check = GTK_WIDGET (gtk_builder_get_object (bxml, 
	                                                        "pull_origin_check"));
	pull_url_entry = GTK_WIDGET (gtk_builder_get_object (bxml, 
	                                                     "pull_url_entry"));
	data = git_ui_data_new (plugin, bxml);
	
	g_signal_connect (G_OBJECT (dialog), "response", 
					  G_CALLBACK (on_pull_dialog_response), 
					  data);

	g_signal_connect (G_OBJECT (pull_origin_check), "toggled",
	                  G_CALLBACK (on_git_origin_check_toggled),
	                  pull_url_entry);
	
	gtk_widget_show_all (dialog);
}

void
on_menu_git_pull (GtkAction *action, Git *plugin)
{
	pull_dialog (plugin);
}
