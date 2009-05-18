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

#include "git-add-remote-dialog.h"

static void
on_remote_add_command_finished (AnjutaCommand *command, guint return_code,
								Git *plugin)
{
	AnjutaStatus *status;
	gchar *branch_name;
	gchar *status_message;
	
	if (return_code == 0)
	{
		status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
										  NULL);
		
		branch_name = git_remote_add_command_get_branch_name (GIT_REMOTE_ADD_COMMAND (command));
		status_message = g_strdup_printf (_("Git: Added remote branch \"%s\"."), 
										  branch_name);
		anjuta_status (status, status_message, 5);
		
		g_free (branch_name);
		g_free (status_message);
	}
	
	git_report_errors (command, return_code);
	
	g_object_unref (command);
}


static void
on_add_remote_dialog_response (GtkDialog *dialog, gint response_id, 
							   GitUIData *data)
{
	GtkWidget *add_remote_name_entry;
	GtkWidget *add_remote_url_entry;
	GtkWidget *add_remote_fetch_check;
	gchar *branch_name;
	gchar *url;
	gboolean fetch;
	GitRemoteAddCommand *remote_add_command;
	GitProgressData *progress_data;
	
	if (response_id == GTK_RESPONSE_OK)
	{	
		add_remote_name_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
																	"add_remote_name_entry"));
		add_remote_url_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
																   "add_remote_url_entry"));
		add_remote_fetch_check = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
																	 "add_remote_fetch_check"));
	
		branch_name = gtk_editable_get_chars (GTK_EDITABLE (add_remote_name_entry),
											  0, -1);
		
		if (!git_check_input (GTK_WIDGET (dialog), add_remote_name_entry, 
							  branch_name, _("Please enter a branch name.")))
		{
			g_free (branch_name);
			return;
		}
		
		url = gtk_editable_get_chars (GTK_EDITABLE (add_remote_url_entry),
									  0, -1);
		
		if (!git_check_input (GTK_WIDGET (dialog), add_remote_url_entry, 
							  url, _("Please enter a URL.")))
		{
			g_free (branch_name);
			g_free (url);
			return;
		}
		
		fetch = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (add_remote_fetch_check));
		remote_add_command = git_remote_add_command_new (data->plugin->project_root_directory,
														branch_name,
														url,
														fetch);
		
		g_free (branch_name);
		g_free (url);
		
		
		if (fetch)
		{
			progress_data = git_progress_data_new (data->plugin, 
												   _("Git: Fetching..."));
			
			git_create_message_view (data->plugin);
			
			g_signal_connect (G_OBJECT (remote_add_command), "data-arrived",
							  G_CALLBACK (on_git_command_info_arrived),
							  data->plugin);
			
			g_signal_connect (G_OBJECT (remote_add_command), "progress",
							  G_CALLBACK (on_git_command_progress),
							  progress_data);
			
			g_signal_connect_swapped (G_OBJECT (remote_add_command), 
									  "command-finished",
									  G_CALLBACK (git_progress_data_free),
									  progress_data);
			
		}
		
		g_signal_connect (G_OBJECT (remote_add_command), "command-finished",
						  G_CALLBACK (on_remote_add_command_finished),
						  data->plugin);
		
		
		
		anjuta_command_start (ANJUTA_COMMAND (remote_add_command));
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	git_ui_data_free (data);
}

static void
add_remote_dialog (Git *plugin)
{
	GtkBuilder *bxml;
	gchar *objects[] = {"add_remote_dialog", NULL};
	GError *error;
	GtkWidget *dialog;
	GitUIData *data;

	bxml = gtk_builder_new ();
	error = NULL;

	if (!gtk_builder_add_objects_from_file (bxml, BUILDER_FILE, objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

	dialog = GTK_WIDGET (gtk_builder_get_object (bxml, "add_remote_dialog"));

	data = git_ui_data_new (plugin, bxml);
	
	g_signal_connect (G_OBJECT (dialog), "response", 
					  G_CALLBACK (on_add_remote_dialog_response), 
					  data);
	
	gtk_widget_show_all (dialog);
}

void
on_menu_git_add_remote (GtkAction *action, Git *plugin)
{
	add_remote_dialog (plugin);
}

