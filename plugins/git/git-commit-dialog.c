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

#include "git-commit-dialog.h"

static void
on_commit_command_finished (AnjutaCommand *command, guint return_code,
							Git *plugin)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
									  NULL);
	
	anjuta_status (status, _("Git: Commit complete."), 5);
	
	git_report_errors (command, return_code);
	
	g_object_unref (command);
}


static void
on_commit_dialog_response (GtkDialog *dialog, gint response_id, 
						   GitUIData *data)
{
	GtkWidget *commit_log_view;
	gchar *log;
	GtkWidget *log_prompt_dialog;
	gint prompt_response;
	GtkWidget *commit_status_view;
	GtkWidget *commit_custom_author_info_check;
	GtkWidget *commit_author_info_alignment;
	GtkWidget *commit_author_name_entry;
	GtkWidget *commit_author_email_entry;
	gchar *author_name;
	gchar *author_email;
	GtkWidget *resolve_check;
	GList *selected_paths;
	GitCommitCommand *commit_command;
	
	if (response_id == GTK_RESPONSE_OK)
	{
		commit_log_view = glade_xml_get_widget (data->gxml, "commit_log_view");
		log = git_get_log_from_textview (commit_log_view);
		
		if (!g_utf8_strlen(log, -1))
		{
			log_prompt_dialog = gtk_message_dialog_new(GTK_WINDOW(dialog), 
													   GTK_DIALOG_DESTROY_WITH_PARENT, 
													   GTK_MESSAGE_INFO,
													   GTK_BUTTONS_YES_NO, 
													   _("Are you sure that you want to pass an empty log message?"));
			
			prompt_response = gtk_dialog_run(GTK_DIALOG (log_prompt_dialog));
			gtk_widget_destroy (log_prompt_dialog);
			
			if (prompt_response == GTK_RESPONSE_NO)
				return;
		}
		
		commit_custom_author_info_check = glade_xml_get_widget (data->gxml, 
		                                                        "commit_custom_author_info_check");
		commit_author_info_alignment = glade_xml_get_widget (data->gxml,
		                                                     "commit_author_info_alignment");
		commit_author_name_entry = glade_xml_get_widget (data->gxml,
		                                                 "commit_author_name_entry");
		commit_author_email_entry = glade_xml_get_widget (data->gxml,
		                                                  "commit_author_email_entry");

		author_name = NULL;
		author_email = NULL;

		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (commit_custom_author_info_check)))
		{
			author_name = gtk_editable_get_chars (GTK_EDITABLE (commit_author_name_entry), 0, -1);
			if (!git_check_input (GTK_WIDGET (dialog), commit_author_name_entry,
			                      author_name,
			                      _("Please enter the commit author's name.")))
			{
				g_free (log);
				return;
			}

			author_email = gtk_editable_get_chars (GTK_EDITABLE (commit_author_email_entry), 0, -1);

			if (!git_check_input (GTK_WIDGET (dialog), commit_author_email_entry,
			                      author_email,
			                      _("Please enter the commit author's e-mail address.")))
			{
				g_free (log);
				g_free (author_name);
				return;
			}
		}
		   

		commit_status_view = glade_xml_get_widget (data->gxml, "commit_status_view");
		resolve_check = glade_xml_get_widget (data->gxml, "resolve_check");
		selected_paths = anjuta_vcs_status_tree_view_get_selected (ANJUTA_VCS_STATUS_TREE_VIEW (commit_status_view));
		commit_command = git_commit_command_new (data->plugin->project_root_directory,
												 gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (resolve_check)),
												 log,
		                                         author_name,
		                                         author_email,
												 selected_paths);
		
		g_free (log);
		git_command_free_path_list (selected_paths);
		
		git_create_message_view (data->plugin);
		
		g_signal_connect (G_OBJECT (commit_command), "command-finished",
						  G_CALLBACK (on_commit_command_finished),
						  data->plugin);
		
		g_signal_connect (G_OBJECT (commit_command), "data-arrived",
						  G_CALLBACK (on_git_command_info_arrived),
						  data->plugin);
		
		anjuta_command_start (ANJUTA_COMMAND (commit_command));
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	git_ui_data_free (data);
}

static void
select_all_files (AnjutaCommand *command, guint return_code, 
				  AnjutaVcsStatusTreeView *commit_status_view)
{
	anjuta_vcs_status_tree_view_select_all (commit_status_view);
}

static void
set_author_info_alignment_sensitive (GtkToggleButton *button,
                            		 GtkWidget *commit_author_info_alignment)
{
	gtk_widget_set_sensitive (commit_author_info_alignment,
	                          gtk_toggle_button_get_active (button));
}

static void
commit_dialog (Git *plugin)
{
	GladeXML *gxml;
	GtkWidget *dialog;
	GtkWidget *commit_custom_author_info_check;
	GtkWidget *commit_author_info_alignment;
	GtkWidget *commit_select_all_button;
	GtkWidget *commit_clear_button;
	GtkWidget *commit_status_view;
	GtkWidget *commit_status_progress_bar;
	GitStatusCommand *status_command;
	GitUIData *data;
	
	gxml = glade_xml_new (GLADE_FILE, "commit_dialog", NULL);
	
	dialog = glade_xml_get_widget (gxml, "commit_dialog");
	commit_custom_author_info_check = glade_xml_get_widget (gxml, "commit_custom_author_info_check");
	commit_author_info_alignment = glade_xml_get_widget (gxml, "commit_author_info_alignment");
	commit_select_all_button = glade_xml_get_widget (gxml, "commit_select_all_button");
	commit_clear_button = glade_xml_get_widget (gxml, "commit_clear_button");
	commit_status_view = glade_xml_get_widget (gxml, "commit_status_view");
	commit_status_progress_bar = glade_xml_get_widget (gxml, "commit_status_progress_bar");
	
	status_command = git_status_command_new (plugin->project_root_directory,
											 GIT_STATUS_SECTION_MODIFIED);
	
	g_signal_connect (G_OBJECT (commit_custom_author_info_check), "toggled",
	                  G_CALLBACK (set_author_info_alignment_sensitive),
	                  commit_author_info_alignment);

	g_signal_connect (G_OBJECT (commit_select_all_button), "clicked",
					  G_CALLBACK (git_select_all_status_items),
					  commit_status_view);
	
	g_signal_connect (G_OBJECT (commit_clear_button), "clicked",
					  G_CALLBACK (git_clear_all_status_selections),
					  commit_status_view);
	
	g_signal_connect (G_OBJECT (status_command), "command-finished",
					  G_CALLBACK (select_all_files),
					  commit_status_view);
	
	git_pulse_progress_bar (GTK_PROGRESS_BAR (commit_status_progress_bar));
	
	g_signal_connect (G_OBJECT (status_command), "command-finished",
					  G_CALLBACK (git_cancel_data_arrived_signal_disconnect),
					  commit_status_view);
	
	g_signal_connect (G_OBJECT (status_command), "command-finished",
					  G_CALLBACK (git_hide_pulse_progress_bar),
					  commit_status_progress_bar);
	
	g_signal_connect (G_OBJECT (status_command), "command-finished",
					  G_CALLBACK (on_git_command_finished),
					  NULL);
	
	g_signal_connect (G_OBJECT (status_command), "data-arrived",
					  G_CALLBACK (on_git_status_command_data_arrived),
					  commit_status_view);
	
	g_object_weak_ref (G_OBJECT (commit_status_view),
					   (GWeakNotify) git_disconnect_data_arrived_signals,
					   status_command);

	
	
	anjuta_command_start (ANJUTA_COMMAND (status_command));
	
	data = git_ui_data_new (plugin, gxml);
	
	g_signal_connect(G_OBJECT (dialog), "response", 
					 G_CALLBACK (on_commit_dialog_response), 
					 data);
	
	gtk_widget_show_all (dialog);
}

void
on_menu_git_commit (GtkAction *action, Git *plugin)
{
	commit_dialog (plugin);
}
