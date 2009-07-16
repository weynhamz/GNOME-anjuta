/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2009 <jrliggett@cox.net>
 * 
 * anjuta is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include "git-stash-changes-dialog.h"

static void
on_stash_command_finished (AnjutaCommand *command, guint return_code,
						   Git *plugin)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
									  NULL);
	
	anjuta_status (status, _("Git: Changes stored in a stash."), 5);
	
	git_report_errors (command, return_code);
	
	g_object_unref (command);
}

static void
on_stash_changes_dialog_response (GtkDialog *dialog, gint response, 
								  GitUIData *data)
{
	GtkWidget *stash_changes_message_view;
	GtkWidget *stash_changes_keep_index_check;
	gchar *message;
	GitStashSaveCommand *stash_command;

	if (response == GTK_RESPONSE_OK)
	{
		stash_changes_message_view = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																		 "stash_changes_message_view"));
		stash_changes_keep_index_check = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																			 "stash_changes_keep_index_check"));

		message = git_get_log_from_textview (stash_changes_message_view);

		/* Don't pass empty messages */
		if (!g_utf8_strlen (message, -1))
		{
			g_free (message);
			message = NULL;
		}

		stash_command = git_stash_save_command_new (data->plugin->project_root_directory,
													gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (stash_changes_keep_index_check)),
													message);

		git_create_message_view (data->plugin);

		g_signal_connect (G_OBJECT (stash_command), "command-finished",
						  G_CALLBACK (on_stash_command_finished),
						  data->plugin);

		g_signal_connect (G_OBJECT (stash_command), "data-arrived",
						  G_CALLBACK (on_git_command_info_arrived),
						  data->plugin);

		anjuta_command_start (ANJUTA_COMMAND (stash_command));
	}

	git_ui_data_free (data);
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
stash_changes_dialog (Git *plugin)
{
	GtkBuilder *bxml;
	gchar *objects[] = {"stash_changes_dialog", NULL};
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

	dialog = GTK_WIDGET (gtk_builder_get_object (bxml, "stash_changes_dialog"));

	data = git_ui_data_new (plugin, bxml);

	g_signal_connect (G_OBJECT (dialog), "response",
					  G_CALLBACK (on_stash_changes_dialog_response),
					  data);

	gtk_widget_show_all (dialog);
}

void
on_menu_git_stash_changes (GtkAction *action, Git *plugin)
{
	stash_changes_dialog (plugin);
}