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

#include "git-apply-mailbox-dialog.h"

static void
on_apply_mailbox_dialog_response (GtkDialog *dialog, gint response_id, 
								  GitUIData *data)
{
	GtkWidget *apply_mailbox_file_chooser_button;
	GtkWidget *apply_mailbox_signoff_check;
	gchar *path;
	GitApplyMailboxCommand *apply_mailbox_command;
	
	if (response_id == GTK_RESPONSE_OK)
	{
		apply_mailbox_file_chooser_button = glade_xml_get_widget (data->gxml,
		                                                          "apply_mailbox_file_chooser_button");
		apply_mailbox_signoff_check = glade_xml_get_widget (data->gxml,
		                                                    "apply_mailbox_signoff_check");
		path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (apply_mailbox_file_chooser_button));

		if (git_check_input (GTK_WIDGET (dialog), apply_mailbox_file_chooser_button, path, 
							 _("Please select a mailbox file.")))
		{

			apply_mailbox_command = git_apply_mailbox_command_new (data->plugin->project_root_directory,
			                                                       path,
			                                                       gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (apply_mailbox_signoff_check)));

			g_free (path);

			git_create_message_view (data->plugin);

			g_signal_connect (G_OBJECT (apply_mailbox_command), "command-finished",
			                  G_CALLBACK (on_git_command_finished),
			                  data->plugin);

			g_signal_connect (G_OBJECT (apply_mailbox_command), "data-arrived",
			                  G_CALLBACK (on_git_command_info_arrived),
			                  data->plugin);

			anjuta_command_start (ANJUTA_COMMAND (apply_mailbox_command));
		}
		else
			return;
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	git_ui_data_free (data);
}

static void
apply_mailbox_dialog (Git *plugin)
{
	GladeXML *gxml;
	GtkWidget *dialog;
	GitUIData *data;
	
	gxml = glade_xml_new (GLADE_FILE, "apply_mailbox_dialog", NULL);
	data = git_ui_data_new (plugin, gxml);
	
	dialog = glade_xml_get_widget (gxml, "apply_mailbox_dialog");
	
	g_signal_connect (G_OBJECT (dialog), "response", 
					  G_CALLBACK (on_apply_mailbox_dialog_response), 
					  data);
	
	gtk_widget_show_all (dialog);
}

static void
apply_mailbox_continue (Git *plugin, GitApplyMailboxContinueAction action)
{
	GitApplyMailboxContinueCommand *apply_mailbox_command;
	
	git_create_message_view (plugin);
	
	apply_mailbox_command = git_apply_mailbox_continue_command_new (plugin->project_root_directory,
																	action);
	
	g_signal_connect (G_OBJECT (apply_mailbox_command), "command-finished",
					  G_CALLBACK (on_git_command_finished),
					  plugin);
	
	g_signal_connect (G_OBJECT (apply_mailbox_command), "data-arrived",
					  G_CALLBACK (on_git_command_info_arrived),
					  plugin);
	
	anjuta_command_start (ANJUTA_COMMAND (apply_mailbox_command));
}

void
on_menu_git_apply_mailbox_apply (GtkAction *action, Git *plugin)
{
	apply_mailbox_dialog (plugin);
}

void 
on_menu_git_apply_mailbox_resolved (GtkAction *action, Git *plugin)
{
	apply_mailbox_continue (plugin, GIT_APPLY_MAILBOX_CONTINUE_ACTION_RESOLVED);
}

void 
on_menu_git_apply_mailbox_skip (GtkAction *action, Git *plugin)
{
	apply_mailbox_continue (plugin, GIT_APPLY_MAILBOX_CONTINUE_ACTION_SKIP);
}

void 
on_menu_git_apply_mailbox_abort (GtkAction *action, Git *plugin)
{
	apply_mailbox_continue (plugin, GIT_APPLY_MAILBOX_CONTINUE_ACTION_ABORT);
}
