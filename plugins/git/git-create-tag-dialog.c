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

#include "git-create-tag-dialog.h"

static void
on_create_command_finished (AnjutaCommand *command, guint return_code,
							Git *plugin)
{
	AnjutaStatus *status;
	gchar *tag_name;
	gchar *status_message;
	
	if (return_code == 0)
	{
		status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
										  NULL);
		
		tag_name = git_tag_create_command_get_tag_name (GIT_TAG_CREATE_COMMAND (command));
		status_message = g_strdup_printf (_("Git: Created tag \"%s\"."), 
										  tag_name);
		anjuta_status (status, status_message, 5);
		
		g_free (tag_name);
		g_free (status_message);
	}
	
	
	
	git_report_errors (command, return_code);
	
	g_object_unref (command);
}


static void
on_create_tag_dialog_response (GtkDialog *dialog, gint response_id, 
							   GitUIData *data)
{
	GtkWidget *tag_name_entry;
	GtkWidget *tag_revision_radio;
	GtkWidget *tag_revision_entry;
	GtkWidget *tag_force_check;
	GtkWidget *tag_annotate_check;
	GtkWidget *tag_log_view;
	gchar *log;
	GtkWidget *log_prompt_dialog;
	gint prompt_response;
	gchar *tag_name;
	gchar *revision;
	GitTagCreateCommand *create_command;
	
	if (response_id == GTK_RESPONSE_OK)
	{	
		tag_name_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
															 "tag_name_entry"));
		tag_revision_radio = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
															     "tag_revision_radio"));
		tag_revision_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
																 "tag_revision_entry"));
		tag_force_check = GTK_WIDGET (gtk_builder_get_object (data->bxml,
															  "tag_force_check"));
		tag_annotate_check = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																 "tag_annotate_check"));
		tag_log_view = GTK_WIDGET (gtk_builder_get_object (data->bxml,
														   "tag_log_view"));
		
		tag_name = gtk_editable_get_chars (GTK_EDITABLE (tag_name_entry),
										   0, -1);
		revision = NULL;
		log = NULL;
		
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (tag_revision_radio)))
		{
			revision = gtk_editable_get_chars (GTK_EDITABLE (tag_revision_entry),
											   0, -1);
			if (!git_check_input (GTK_WIDGET (dialog), tag_revision_entry, 
								  revision, _("Please enter a revision.")))
			{
				g_free (revision);
				g_free (tag_name);
				return;
			}
		}
		
		if (!git_check_input (GTK_WIDGET (dialog), tag_revision_entry, 
							  tag_name, _("Please enter a tag name.")))
		{
			
			g_free (revision);
			g_free (tag_name);
			return;
		}
		
		
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (tag_annotate_check)))
		{
			log = git_get_log_from_textview (tag_log_view);
			
			if (!g_utf8_strlen (log, -1))
			{
				log_prompt_dialog = gtk_message_dialog_new (GTK_WINDOW(dialog), 
															GTK_DIALOG_DESTROY_WITH_PARENT, 
															GTK_MESSAGE_INFO,
															GTK_BUTTONS_YES_NO, 
															_("Are you sure that you want to pass an empty log message?"));
				
				prompt_response = gtk_dialog_run(GTK_DIALOG (log_prompt_dialog));
				gtk_widget_destroy (log_prompt_dialog);
				
				if (prompt_response == GTK_RESPONSE_NO)
				{
					g_free (revision);
					g_free (tag_name);
					g_free (log);
					return;
				}
			}
		}
		
		create_command = git_tag_create_command_new (data->plugin->project_root_directory,
													 tag_name,
													 revision,
													 log,
													 gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (tag_force_check)));
		
		g_free (tag_name);
		g_free (revision);
		g_free (log);
		
		g_signal_connect (G_OBJECT (create_command), "command-finished",
						  G_CALLBACK (on_create_command_finished),
						  data->plugin);
		
		anjuta_command_start (ANJUTA_COMMAND (create_command));
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	git_ui_data_free (data);
}

static void
on_tag_revision_radio_toggled (GtkToggleButton *toggle_button, 
							   GitUIData *data)
{
	GtkWidget *create_tag_dialog;
	GtkWidget *tag_revision_entry;
	gboolean active;
	
	create_tag_dialog = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
														    "create_tag_dialog"));
	tag_revision_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml,
															 "tag_revision_entry"));
	
	active = gtk_toggle_button_get_active (toggle_button);
	gtk_widget_set_sensitive (tag_revision_entry, active);
	
	if (active)
	{
		gtk_window_set_focus (GTK_WINDOW (create_tag_dialog),
							  tag_revision_entry);
	}
	
}

static void
on_tag_annotate_check_toggled (GtkToggleButton *toggle_button, 
							   GtkWidget *tag_log_view)
{
	gtk_widget_set_sensitive (tag_log_view,
							  gtk_toggle_button_get_active (toggle_button));
}

static void
create_tag_dialog (Git *plugin, const gchar *revision)
{
	GtkBuilder *bxml;
	gchar *objects[] = {"create_tag_dialog", NULL};
	GError *error;
	GtkWidget *dialog;
	GtkWidget *tag_revision_radio;
	GtkWidget *tag_revision_entry;
	GtkWidget *tag_annotate_check;
	GtkWidget *tag_log_view;
	GitUIData *data;
	
	bxml = gtk_builder_new ();
	error = NULL;

	if (!gtk_builder_add_objects_from_file (bxml, BUILDER_FILE, objects,
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}
	
	dialog = GTK_WIDGET (gtk_builder_get_object (bxml, "create_tag_dialog"));
	tag_revision_radio = GTK_WIDGET (gtk_builder_get_object (bxml, 
														     "tag_revision_radio"));
	tag_revision_entry = GTK_WIDGET (gtk_builder_get_object (bxml, 
														     "tag_revision_entry"));
	tag_annotate_check = GTK_WIDGET (gtk_builder_get_object (bxml, 
														     "tag_annotate_check"));
	tag_log_view = GTK_WIDGET (gtk_builder_get_object (bxml,
													   "tag_log_view"));
	data = git_ui_data_new (plugin, bxml);
	
	g_signal_connect (G_OBJECT (dialog), "response", 
					  G_CALLBACK (on_create_tag_dialog_response), 
					  data);
	
	g_signal_connect (G_OBJECT (tag_revision_radio), "toggled",
					  G_CALLBACK (on_tag_revision_radio_toggled),
					  data);
	
	g_signal_connect (G_OBJECT (tag_annotate_check), "toggled",
					  G_CALLBACK (on_tag_annotate_check_toggled),
					  tag_log_view);
	
	if (revision)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tag_revision_radio), TRUE);
		gtk_entry_set_text (GTK_ENTRY (tag_revision_entry), revision);
	}
	
	gtk_widget_show_all (dialog);
}

void
on_menu_git_create_tag (GtkAction *action, Git *plugin)
{
	create_tag_dialog (plugin, NULL);
}

void
on_log_menu_git_create_tag (GtkAction *action, Git *plugin)
{
	GitRevision *revision;
	gchar *sha;
	
	revision = git_log_get_selected_revision (plugin);
	
	if (revision)
	{
		sha = git_revision_get_sha (revision);
		
		create_tag_dialog (plugin, sha);
		g_free (sha);
	}
}
