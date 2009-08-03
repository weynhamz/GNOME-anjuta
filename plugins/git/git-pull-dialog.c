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
on_remote_list_command_finished (AnjutaCommand *command, guint return_code,
                                 GitUIData *data)
{
	GtkWidget *pull_origin_check;
	gboolean sensitive;

	pull_origin_check = GTK_WIDGET (gtk_builder_get_object (data->bxml,
	                                                        "pull_origin_check"));

	g_object_get (G_OBJECT (pull_origin_check), "sensitive", &sensitive, NULL);

	/* The repository origin check should be on by default, but since we're
	 * trying to intelligently detect if there is even an origin to be used,
	 * don't try to make it active if if will be disabled anyway. */
	if (sensitive)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pull_origin_check), 
		                              TRUE);
	}

	git_report_errors (command, return_code);

	g_object_unref (command);
}

static void
on_pull_dialog_response (GtkDialog *dialog, gint response_id, 
						 GitUIData *data)
{
	GtkWidget *pull_remote_toggle;
	GtkWidget *pull_url_toggle;
	GtkWidget *pull_remote_view;
	GtkWidget *pull_origin_check;
	GtkWidget *pull_rebase_check;
	GtkWidget *pull_url_entry;
	GtkWidget *pull_no_commit_check;
	GtkWidget *pull_squash_check;
	GtkWidget *pull_fast_forward_commit_check;
	GtkWidget *pull_append_fetch_data_check;
	GtkWidget *pull_force_check;
	GtkWidget *pull_no_follow_tags_check;
	GtkTreeModel *remote_list_model;
	GtkWidget *input_widget;
	const gchar *input_error;
	gchar *url;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GitPullCommand *pull_command;
	
	if (response_id == GTK_RESPONSE_OK)
	{	
		pull_remote_toggle = GTK_WIDGET (gtk_builder_get_object (data->bxml,
		                                                         "pull_remote_toggle"));
		pull_url_toggle = GTK_WIDGET (gtk_builder_get_object (data->bxml,
		                                                      "pull_url_toggle"));
		pull_remote_view = GTK_WIDGET (gtk_builder_get_object (data->bxml,
		                                                       "pull_remote_view"));
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
		remote_list_model = GTK_TREE_MODEL (gtk_builder_get_object (data->bxml,
		                                                            "remote_list_model"));
		
		/* The "input widget" is the widget that should receive focus if the
		 * user does not properly enter anything */
		input_error = _("Please select a remote to pull from.");
		input_widget = pull_remote_view;

		url = NULL;

		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pull_origin_check)))
			url = g_strdup ("origin");
		else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pull_remote_toggle)))
		{
			selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (pull_remote_view));

			if (gtk_tree_selection_get_selected (selection, NULL, &iter))
				gtk_tree_model_get (remote_list_model, &iter, 0, &url, -1);
		}
		else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pull_url_toggle)))
		{
			url = gtk_editable_get_chars (GTK_EDITABLE (pull_url_entry), 0, -1);
			input_widget = pull_url_entry;
			input_error = _("Please enter the URL of the repository to pull from.");
		}
		
		if (!git_check_input (GTK_WIDGET (dialog), input_widget, url,
							  input_error))
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
	gchar *objects[] = {"pull_dialog", "remote_list_model", NULL};
	GError *error;
	GtkWidget *dialog;
	GtkWidget *pull_repository_vbox;
	GtkWidget *pull_remote_toggle;
	GtkWidget *pull_url_toggle;
	GtkWidget *pull_repository_notebook;
	GtkWidget *pull_origin_check;
	GtkListStore *remote_list_model;
	GitUIData *data;
	GitRemoteListCommand *remote_list_command;
	
	bxml = gtk_builder_new ();
	error = NULL;

	if (!gtk_builder_add_objects_from_file (bxml, BUILDER_FILE, objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}
	
	dialog = GTK_WIDGET (gtk_builder_get_object (bxml, "pull_dialog"));
	pull_repository_vbox = GTK_WIDGET (gtk_builder_get_object (bxml,
	                                                           "pull_repository_vbox"));
	pull_remote_toggle = GTK_WIDGET (gtk_builder_get_object (bxml,
	                                                         "pull_remote_toggle"));
	pull_url_toggle = GTK_WIDGET (gtk_builder_get_object (bxml,
	                                                      "pull_url_toggle"));
	pull_repository_notebook = GTK_WIDGET (gtk_builder_get_object (bxml,
	                                                               "pull_repository_notebook"));
	pull_origin_check = GTK_WIDGET (gtk_builder_get_object (bxml, 
	                                                        "pull_origin_check"));
	remote_list_model = GTK_LIST_STORE (gtk_builder_get_object (bxml,
	                                                            "remote_list_model"));
	data = git_ui_data_new (plugin, bxml);

	remote_list_command = git_remote_list_command_new (plugin->project_root_directory);

	g_object_set_data (G_OBJECT (remote_list_command), "origin-check", 
	                   pull_origin_check);

	g_signal_connect (G_OBJECT (remote_list_command), "data-arrived",
	                  G_CALLBACK (on_git_remote_list_command_data_arrived),
	                  remote_list_model);

	g_signal_connect (G_OBJECT (remote_list_command), "command-finished",
	                  G_CALLBACK (on_remote_list_command_finished),
	                  data);

	anjuta_command_start (ANJUTA_COMMAND (remote_list_command));
	
	g_signal_connect (G_OBJECT (dialog), "response", 
					  G_CALLBACK (on_pull_dialog_response), 
					  data);

	g_object_set_data (G_OBJECT (pull_remote_toggle), "tab-index",
	                   GINT_TO_POINTER (0));
	g_object_set_data (G_OBJECT (pull_url_toggle), "tab-index",
	                   GINT_TO_POINTER (1));

	g_signal_connect (G_OBJECT (pull_remote_toggle), "toggled",
	                  G_CALLBACK (on_git_notebook_button_toggled),
	                  pull_repository_notebook);

	g_signal_connect (G_OBJECT (pull_url_toggle), "toggled",
	                  G_CALLBACK (on_git_notebook_button_toggled),
	                  pull_repository_notebook);

	g_signal_connect (G_OBJECT (pull_origin_check), "toggled",
	                  G_CALLBACK (on_git_origin_check_toggled),
	                  pull_repository_vbox);
	
	gtk_widget_show_all (dialog);
}

void
on_menu_git_pull (GtkAction *action, Git *plugin)
{
	pull_dialog (plugin);
}
