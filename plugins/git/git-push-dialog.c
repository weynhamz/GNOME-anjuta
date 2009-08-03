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

#include "git-push-dialog.h"

static void
on_push_command_finished (AnjutaCommand *command, guint return_code,
						  Git *plugin)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
									  NULL);
	
	anjuta_status (status, _("Git: Push complete."), 5);
	
	git_report_errors (command, return_code);
	
	g_object_unref (command);
}

static void
on_push_tags_check_toggled (GtkToggleButton *toggle_button,
							GitUIData *data)
{
	GtkWidget *push_tags_scrolled_window;
	GtkWidget *push_all_check;
	gboolean active;

	push_tags_scrolled_window = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																	"push_tags_scrolled_window"));
	push_all_check = GTK_WIDGET (gtk_builder_get_object (data->bxml,
														 "push_all_check"));
	active = gtk_toggle_button_get_active (toggle_button);

	/* Leave the widget insensitive if the push all refs check is ticked. */
	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (push_all_check)))
		gtk_widget_set_sensitive (push_tags_scrolled_window, !active);
}

static void
on_push_all_check_toggled (GtkToggleButton *toggle_button,
						   GitUIData *data)
{
	GtkWidget *push_branches_scrolled_window;
	GtkWidget *push_tags_scrolled_window;
	GtkWidget *push_tags_check;
	gboolean active;

	push_branches_scrolled_window = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																		"push_branches_scrolled_window"));
	push_tags_scrolled_window = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																	"push_tags_scrolled_window"));
	push_tags_check = GTK_WIDGET (gtk_builder_get_object (data->bxml,
														  "push_tags_check"));

	active = gtk_toggle_button_get_active (toggle_button);

	if (active)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (push_tags_check), 
									  FALSE);
	}

	gtk_widget_set_sensitive (push_branches_scrolled_window, !active);
	gtk_widget_set_sensitive (push_tags_scrolled_window, !active);
	gtk_widget_set_sensitive (push_tags_check, !active);
}

static void
on_push_dialog_response (GtkDialog *dialog, gint response_id, 
						 GitUIData *data)
{
	GtkWidget *push_remote_toggle;
	GtkWidget *push_url_toggle;
	GtkWidget *push_remote_view;
	GtkWidget *push_origin_check;
	GtkWidget *push_url_entry;
	GtkWidget *push_all_check;
	GtkWidget *push_tags_check;
	GtkTreeModel *remote_list_model;
	GtkTreeModel *branch_list_model;
	GtkTreeModel *tag_list_model;
	GtkWidget *input_widget;
	const gchar *input_error;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gchar *url;
	gboolean push_all_tags;
	gboolean push_all_refs;
	GList *selected_refs;
	GitPushCommand *push_command;
	GitProgressData *progress_data;
	
	if (response_id == GTK_RESPONSE_OK)
	{
		push_remote_toggle = GTK_WIDGET (gtk_builder_get_object (data->bxml,
		                                                         "push_remote_toggle"));
		push_url_toggle = GTK_WIDGET (gtk_builder_get_object (data->bxml,
		                                                      "push_url_toggle"));
		push_remote_view = GTK_WIDGET (gtk_builder_get_object (data->bxml,
		                                                       "push_remote_view"));
		push_origin_check = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
		                                      					"push_origin_check"));
		push_url_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
		                                                     "push_url_entry"));
		push_all_check = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
		                                                     "push_all_check"));
		push_tags_check = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
		                                                      "push_tags_check"));
		remote_list_model = GTK_TREE_MODEL (gtk_builder_get_object (data->bxml,
		                                                    		"remote_list_model"));
		branch_list_model = GTK_TREE_MODEL (gtk_builder_get_object (data->bxml,
																	"branch_list_model"));
		tag_list_model = GTK_TREE_MODEL (gtk_builder_get_object (data->bxml,
																  "tag_list_model"));

		/* The "input widget" is the widget that should receive focus if the
		 * user does not properly enter anything */
		input_error = _("Please select a remote to push to.");
		input_widget = push_remote_view;

		url = NULL;

		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (push_origin_check)))
			url = g_strdup ("origin");
		else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (push_remote_toggle)))
		{
			selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (push_remote_view));

			if (gtk_tree_selection_get_selected (selection, NULL, &iter))
				gtk_tree_model_get (remote_list_model, &iter, 0, &url, -1);
		}
		else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (push_url_toggle)))
		{
			url = gtk_editable_get_chars (GTK_EDITABLE (push_url_entry), 0, -1);
			input_widget = push_url_entry;
			input_error = _("Please enter the URL of the repository to push to.");
		}

		if (!git_check_input (GTK_WIDGET (dialog), input_widget, url, 
							  input_error))
		{
			g_free (url);
			return;
		}

		push_all_tags = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (push_all_check));
		push_all_refs = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (push_tags_check));

		selected_refs = NULL;

		if (!push_all_refs)
		{
			gtk_tree_model_foreach (branch_list_model, 
									(GtkTreeModelForeachFunc) git_get_selected_refs,
									&selected_refs);

			if (!push_all_tags)
			{
				gtk_tree_model_foreach (tag_list_model, 
										(GtkTreeModelForeachFunc) git_get_selected_refs,
										&selected_refs);
			}
		}
		
		push_command = git_push_command_new (data->plugin->project_root_directory,
		                                     url,
											 selected_refs,
		                                     push_all_tags,
		                                     push_all_refs);
		progress_data = git_progress_data_new (data->plugin, _("Git: Pushing..."));

		g_free (url);
		git_command_free_string_list (selected_refs);

		git_create_message_view (data->plugin);


		g_signal_connect (G_OBJECT (push_command), "data-arrived",
		                  G_CALLBACK (on_git_command_info_arrived),
		                  data->plugin);

		g_signal_connect (G_OBJECT (push_command), "progress",
		                  G_CALLBACK (on_git_command_progress),
		                  progress_data);

		g_signal_connect (G_OBJECT (push_command), "command-finished",
		                  G_CALLBACK (on_push_command_finished),
		                  data->plugin);
		
		g_signal_connect_swapped (G_OBJECT (push_command), "command-finished",
								  G_CALLBACK (git_progress_data_free),
								  progress_data);

		anjuta_command_start (ANJUTA_COMMAND (push_command));
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	git_ui_data_free (data);
}

static void
push_dialog (Git *plugin)
{
	GtkBuilder *bxml;
	gchar *objects[] = {"push_dialog", "remote_list_model", "branch_list_model", 
						"tag_list_model", NULL};
	GError *error;
	GtkWidget *dialog;
	GtkWidget *push_repository_notebook;
	GtkWidget *push_remote_toggle;
	GtkWidget *push_url_toggle;
	GtkWidget *push_origin_check;
	GtkWidget *push_repository_vbox;
	GtkWidget *push_tags_check;
	GtkWidget *push_all_check;
	GtkListStore *remote_list_model;
	GtkListStore *branch_list_model;
	GtkListStore *tag_list_model;
	GtkCellRenderer *push_branches_view_selected_renderer;
	GtkCellRenderer *push_tags_view_selected_renderer;
	GitUIData *data;
	GitRemoteListCommand *remote_list_command;
	GitBranchListCommand *branch_list_command;
	GitTagListCommand *tag_list_command;
	
	
	bxml = gtk_builder_new ();
	error = NULL;

	if (!gtk_builder_add_objects_from_file (bxml, BUILDER_FILE, objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}
	
	dialog = GTK_WIDGET (gtk_builder_get_object (bxml, "push_dialog"));
	push_repository_notebook = GTK_WIDGET (gtk_builder_get_object (bxml,
	                                                               "push_repository_notebook"));
	push_remote_toggle = GTK_WIDGET (gtk_builder_get_object (bxml,
	                                                         "push_remote_toggle"));
	push_url_toggle = GTK_WIDGET (gtk_builder_get_object (bxml,
	                                                      "push_url_toggle"));
	push_origin_check = GTK_WIDGET (gtk_builder_get_object (bxml, 
	                                                        "push_origin_check"));
	push_repository_vbox = GTK_WIDGET (gtk_builder_get_object (bxml, 
	                                                 		   "push_repository_vbox"));
	push_all_check = GTK_WIDGET (gtk_builder_get_object (bxml, 
		                                                 "push_all_check"));
	push_tags_check = GTK_WIDGET (gtk_builder_get_object (bxml, 
		                                                  "push_tags_check"));
	remote_list_model = GTK_LIST_STORE (gtk_builder_get_object (bxml,
	                                                            "remote_list_model"));
	branch_list_model = GTK_LIST_STORE (gtk_builder_get_object (bxml,
																"branch_list_model"));
	tag_list_model = GTK_LIST_STORE (gtk_builder_get_object (bxml,
															 "tag_list_model"));
	
	push_branches_view_selected_renderer = GTK_CELL_RENDERER (gtk_builder_get_object (bxml,
																					  "push_branches_view_selected_renderer"));
	push_tags_view_selected_renderer = GTK_CELL_RENDERER (gtk_builder_get_object (bxml,
																				  "push_tags_view_selected_renderer"));

	data = git_ui_data_new (plugin, bxml);

	remote_list_command = git_remote_list_command_new (plugin->project_root_directory);
	branch_list_command = git_branch_list_command_new (plugin->project_root_directory,
													   GIT_BRANCH_TYPE_LOCAL);
	tag_list_command = git_tag_list_command_new (plugin->project_root_directory);

	g_object_set_data (G_OBJECT (remote_list_command), "origin-check", 
	                   push_origin_check);

	g_signal_connect (G_OBJECT (remote_list_command), "data-arrived",
	                  G_CALLBACK (on_git_remote_list_command_data_arrived),
	                  remote_list_model);

	g_signal_connect (G_OBJECT (remote_list_command), "command-finished",
	                  G_CALLBACK (on_git_command_finished),
	                  NULL);
	
	g_signal_connect (G_OBJECT (branch_list_command), "data-arrived",
					  G_CALLBACK (on_git_list_branch_command_data_arrived),
					  branch_list_model);

	g_signal_connect (G_OBJECT (branch_list_command), "command-finished",
					  G_CALLBACK (git_report_errors),
					  NULL);

	g_signal_connect (G_OBJECT (tag_list_command), "data-arrived",
					  G_CALLBACK (on_git_list_tag_command_data_arrived),
					  tag_list_model);

	g_signal_connect (G_OBJECT (tag_list_command), "command-finished",
					  G_CALLBACK (git_report_errors),
					  NULL);

	anjuta_command_start (ANJUTA_COMMAND (remote_list_command));
	anjuta_command_start (ANJUTA_COMMAND (branch_list_command));
	anjuta_command_start (ANJUTA_COMMAND (tag_list_command));
	
	
	g_signal_connect (G_OBJECT (dialog), "response", 
					  G_CALLBACK (on_push_dialog_response), 
					  data);

	g_object_set_data (G_OBJECT (push_remote_toggle), "tab-index",
	                   GINT_TO_POINTER (0));
	g_object_set_data (G_OBJECT (push_url_toggle), "tab-index",
	                   GINT_TO_POINTER (1));

	g_signal_connect (G_OBJECT (push_remote_toggle), "toggled",
	                  G_CALLBACK (on_git_notebook_button_toggled),
	                  push_repository_notebook);

	g_signal_connect (G_OBJECT (push_url_toggle), "toggled",
	                  G_CALLBACK (on_git_notebook_button_toggled),
	                  push_repository_notebook);

	g_signal_connect (G_OBJECT (push_origin_check), "toggled",
	                  G_CALLBACK (on_git_origin_check_toggled),
	                  push_repository_vbox);

	g_signal_connect (G_OBJECT (push_tags_check), "toggled",
					  G_CALLBACK (on_push_tags_check_toggled),
					  data);

	g_signal_connect (G_OBJECT (push_all_check), "toggled",
					  G_CALLBACK (on_push_all_check_toggled),
					  data);

	g_signal_connect (G_OBJECT (push_branches_view_selected_renderer), "toggled",
					  G_CALLBACK (on_git_selected_column_toggled),
					  branch_list_model);

	g_signal_connect (G_OBJECT (push_tags_view_selected_renderer), "toggled",
					  G_CALLBACK (on_git_selected_column_toggled),
					  tag_list_model);
	
	gtk_widget_show_all (dialog);
}

void
on_menu_git_push (GtkAction *action, Git *plugin)
{
	push_dialog (plugin);
}