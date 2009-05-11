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
on_push_dialog_response (GtkDialog *dialog, gint response_id, 
						 GitUIData *data)
{
	GtkWidget *push_origin_check;
	GtkWidget *push_url_entry;
	GtkWidget *push_all_check;
	GtkWidget *push_tags_check;
	gchar *url;
	GitPushCommand *push_command;
	GitProgressData *progress_data;
	
	if (response_id == GTK_RESPONSE_OK)
	{
		push_origin_check = glade_xml_get_widget (data->gxml, 
		                                          "push_origin_check");
		push_url_entry = glade_xml_get_widget (data->gxml, "push_url_entry");
		push_all_check = glade_xml_get_widget (data->gxml, "push_all_check");
		push_tags_check = glade_xml_get_widget (data->gxml, "push_tags_check");

		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (push_origin_check)))
			url = g_strdup ("origin");
		else 
		{
			url = gtk_editable_get_chars (GTK_EDITABLE (push_url_entry), 0, -1);
		}

		if (!git_check_input (GTK_WIDGET (dialog), push_url_entry, url, 
							  _("Please enter the URL of the repository to push to.")))
		{
			g_free (url);
			return;
		}
		
		push_command = git_push_command_new (data->plugin->project_root_directory,
		                                     url,
		                                     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (push_all_check)),
		                                     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (push_tags_check)));
		progress_data = git_progress_data_new (data->plugin, _("Git: Pushing..."));

		g_free (url);

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

		anjuta_command_start (ANJUTA_COMMAND (push_command));
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	git_ui_data_free (data);
}

static void
push_dialog (Git *plugin)
{
	GladeXML *gxml;
	GtkWidget *dialog;
	GtkWidget *push_origin_check;
	GtkWidget *push_url_entry;
	GitUIData *data;
	
	gxml = glade_xml_new (GLADE_FILE, "push_dialog", NULL);
	
	dialog = glade_xml_get_widget (gxml, "push_dialog");
	push_origin_check = glade_xml_get_widget (gxml, "push_origin_check");
	push_url_entry = glade_xml_get_widget (gxml, "push_url_entry");
	
	data = git_ui_data_new (plugin, gxml);

	g_signal_connect (G_OBJECT (dialog), "response", 
					  G_CALLBACK (on_push_dialog_response), 
					  data);

	g_signal_connect (G_OBJECT (push_origin_check), "toggled",
	                  G_CALLBACK (on_git_origin_check_toggled),
	                  push_url_entry);
	
	gtk_widget_show_all (dialog);
}

void
on_menu_git_push (GtkAction *action, Git *plugin)
{
	push_dialog (plugin);
}