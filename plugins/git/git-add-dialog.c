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

#include "git-add-dialog.h"

static void
on_add_command_finished (AnjutaCommand *command, guint return_code,
						 Git *plugin)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
									  NULL);
	
	anjuta_status (status, _("Git: File staged for add."), 5);
	
	git_report_errors (command, return_code);
	
	g_object_unref (command);
}


static void
on_add_dialog_response (GtkDialog *dialog, gint response_id, 
						GitUIData *data)
{
	GtkWidget *add_file_chooser_button;
	GtkWidget *force_check;
	gchar *filename;
	const gchar *relative_filename;
	GitAddCommand *add_command;
	
	if (response_id == GTK_RESPONSE_OK)
	{
		add_file_chooser_button = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
																	  "add_file_chooser_button"));
		force_check = GTK_WIDGET (gtk_builder_get_object (data->bxml, "force_check"));
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (add_file_chooser_button));
		
		if (git_check_input (GTK_WIDGET (dialog), add_file_chooser_button, filename, 
						     _("Please select a file.")))
		{
			relative_filename = git_get_relative_path (filename,
													   data->plugin->project_root_directory);
			add_command = git_add_command_new_path (data->plugin->project_root_directory,
													relative_filename,
													gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (force_check)));
			
			g_free (filename);
			
			
			g_signal_connect (G_OBJECT (add_command), "command-finished",
							  G_CALLBACK (on_add_command_finished),
							  data->plugin);
			
			anjuta_command_start (ANJUTA_COMMAND (add_command));
		}
		else
			return;
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	git_ui_data_free (data);
}

static void
add_dialog (Git *plugin, const gchar *filename)
{
	GtkBuilder *bxml;
	gchar *objects[] = {"add_dialog", NULL};;
	GError *error;
	GtkWidget *dialog;
	GtkWidget *add_file_chooser_button;
	GitUIData *data;
	
	bxml = gtk_builder_new ();
	error = NULL;

	if (!gtk_builder_add_objects_from_file (bxml, BUILDER_FILE, objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}
	
	dialog = GTK_WIDGET (gtk_builder_get_object (bxml, "add_dialog"));
	add_file_chooser_button = GTK_WIDGET (gtk_builder_get_object (bxml,
	                                                              "add_file_chooser_button"));
	
	if (filename)
	{
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (add_file_chooser_button),
									   filename);
	}
	
	data = git_ui_data_new (plugin, bxml);
	
	g_signal_connect(G_OBJECT (dialog), "response", 
					 G_CALLBACK (on_add_dialog_response), 
					 data);
	
	gtk_widget_show_all (dialog);
}

void
on_menu_git_add (GtkAction *action, Git *plugin)
{
	add_dialog (plugin, plugin->current_editor_filename);
}

void
on_fm_git_add (GtkAction *action, Git *plugin)
{
	add_dialog (plugin, plugin->current_fm_filename);
}
