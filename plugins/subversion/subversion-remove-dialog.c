/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2007 <jrliggett@cox.net>
 *
 * Portions based on the original Subversion plugin 
 * Copyright (C) Johannes Schmid 2005 
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

#include "subversion-remove-dialog.h"

static void
on_remove_command_finished (AnjutaCommand *command, guint return_code, 
							Subversion *plugin)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
									  NULL);
	
	anjuta_status (status, _("Subversion: File will be removed on next "
							 "commit."), 5);
	
	report_errors (command, return_code);
	
	svn_remove_command_destroy (SVN_REMOVE_COMMAND (command));
}

static void
on_remove_path_browse_button_clicked (GtkButton *button, 
										    SubversionData *data)
{
	GtkWidget *subversion_remove;
	GtkWidget *remove_path_entry;
	GtkWidget *file_chooser_dialog;
	gchar *selected_path;
	
	subversion_remove = GTK_WIDGET (gtk_builder_get_object (data->bxml, "subversion_remove"));
	remove_path_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
											  "remove_path_entry"));
	file_chooser_dialog = gtk_file_chooser_dialog_new ("Select file or folder",
													   GTK_WINDOW (subversion_remove),
													   GTK_FILE_CHOOSER_ACTION_OPEN,
													   GTK_STOCK_CANCEL,
													   GTK_RESPONSE_CANCEL,
													   GTK_STOCK_OPEN,
													   GTK_RESPONSE_ACCEPT,
													   NULL);
	
	if (gtk_dialog_run (GTK_DIALOG (file_chooser_dialog)) == GTK_RESPONSE_ACCEPT)
	{
		selected_path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_chooser_dialog));
		gtk_entry_set_text (GTK_ENTRY (remove_path_entry), selected_path);
		g_free (selected_path);
	}
	
	gtk_widget_destroy (GTK_WIDGET (file_chooser_dialog));	
}

static void
on_subversion_remove_response(GtkDialog* dialog, gint response, 
							  SubversionData* data)
{	
	switch (response)
	{
		case GTK_RESPONSE_OK:
		{
			GtkWidget* fileentry = GTK_WIDGET (gtk_builder_get_object (data->bxml, "remove_path_entry"));
			GtkWidget* force = GTK_WIDGET (gtk_builder_get_object (data->bxml, "subversion_force"));
			GtkWidget *remove_log_view;
			const gchar* filename = gtk_entry_get_text(GTK_ENTRY(fileentry));
			gchar *log;
			SvnRemoveCommand *remove_command;
			
			remove_log_view = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
													"remove_log_view"));
			log = get_log_from_textview (remove_log_view);
			
			if (!check_input (GTK_WIDGET (dialog), 
							  fileentry, _("Please enter a path.")))
			{
				break;
			}
			
			
			remove_command = svn_remove_command_new_path ((gchar *) filename, log,
														  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (force)));
			
			g_signal_connect (G_OBJECT (remove_command), "command-finished",
							  G_CALLBACK (on_remove_command_finished),
							  data->plugin);
			
			anjuta_command_start (ANJUTA_COMMAND (remove_command));
			
			subversion_data_free(data);
			gtk_widget_destroy (GTK_WIDGET(dialog));
			break;
		}
		default:
		{
			subversion_data_free(data);
			gtk_widget_destroy (GTK_WIDGET(dialog));
		}
	}
}

static void
subversion_remove_dialog(GtkAction* action, Subversion* plugin, gchar *filename)
{
	GtkBuilder* bxml = gtk_builder_new ();
	GtkWidget* dialog; 
	GtkWidget* remove_path_entry;
	GtkWidget *remove_path_browse_button;
	SubversionData* data;
	GError* error = NULL;

	if (!gtk_builder_add_from_file (bxml, GLADE_FILE, &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}
	
	dialog = GTK_WIDGET (gtk_builder_get_object (bxml, "subversion_remove"));
	remove_path_entry = GTK_WIDGET (gtk_builder_get_object (bxml, "remove_path_entry"));
	remove_path_browse_button = GTK_WIDGET (gtk_builder_get_object (bxml, 
													  "remove_path_browse_button"));
	if (remove_path_entry)
		gtk_entry_set_text(GTK_ENTRY(remove_path_entry), filename);

	data = subversion_data_new(plugin, bxml);
	g_signal_connect(G_OBJECT(dialog), "response", 
		G_CALLBACK(on_subversion_remove_response), data);
	
	g_signal_connect (G_OBJECT (remove_path_browse_button), "clicked",
					  G_CALLBACK (on_remove_path_browse_button_clicked),
					  data);
	
	gtk_widget_show(dialog);
	
}

void 
on_menu_subversion_remove (GtkAction *action, Subversion *plugin)
{
	subversion_remove_dialog (action, plugin, plugin->current_editor_filename);
}


void
on_fm_subversion_remove (GtkAction *action, Subversion *plugin)
{
	subversion_remove_dialog (action, plugin, plugin->fm_current_filename);
}
