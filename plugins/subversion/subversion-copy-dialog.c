/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2007 <jrliggett@cox.net>
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

#include "subversion-copy-dialog.h"

static void
on_copy_other_revision_radio_toggled (GtkToggleButton *toggle_button,
									  SubversionData *data)
{
	GtkWidget *copy_revision_entry;
	GtkWidget *subversion_copy;
	gboolean active;
	
	copy_revision_entry = glade_xml_get_widget (data->gxml, 
												"copy_revision_entry");
	subversion_copy = glade_xml_get_widget (data->gxml,
											"subversion_copy");
	active = gtk_toggle_button_get_active (toggle_button);
	gtk_widget_set_sensitive (copy_revision_entry, active);
	
	if (active)
	{
		gtk_window_set_focus (GTK_WINDOW (subversion_copy), 
							  copy_revision_entry);
	}
}
									  

static void
on_copy_browse_button_clicked (GtkButton *button, SubversionData *data)
{
	GtkWidget *subversion_copy;
	GtkWidget *copy_source_entry;
	GtkWidget *file_chooser_dialog;
	gchar *selected_path;
	
	subversion_copy = glade_xml_get_widget (data->gxml, "subversion_copy");
	copy_source_entry = glade_xml_get_widget (data->gxml, "copy_source_entry");
	file_chooser_dialog = gtk_file_chooser_dialog_new ("Select file or folder",
													   GTK_WINDOW (subversion_copy),
													   GTK_FILE_CHOOSER_ACTION_OPEN,
													   GTK_STOCK_CANCEL,
													   GTK_RESPONSE_CANCEL,
													   GTK_STOCK_OPEN,
													   GTK_RESPONSE_ACCEPT,
													   NULL);
	
	if (gtk_dialog_run (GTK_DIALOG (file_chooser_dialog)) == GTK_RESPONSE_ACCEPT)
	{
		selected_path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_chooser_dialog));
		gtk_entry_set_text (GTK_ENTRY (copy_source_entry), selected_path);
		g_free (selected_path);
	}
	
	gtk_widget_destroy (GTK_WIDGET (file_chooser_dialog));													   
}

static gboolean
on_copy_dest_entry_focus_in (GtkWidget *widget, GdkEventFocus *event, 
							 SubversionData *data)
{
	GtkWidget *copy_source_entry;
	GtkWidget *copy_dest_entry;
	gchar *source_path;
	gchar *dest_path;
	gchar *stripped_source_path;
	gchar *stripped_dest_path;
	gchar *last_slash;
	gchar *source_path_parent;
	gchar *suggested_path;  /* source_path_parent with a slash (/) after it */
	
	copy_source_entry = glade_xml_get_widget (data->gxml, "copy_source_entry");
	copy_dest_entry = glade_xml_get_widget (data->gxml, "copy_dest_entry");
	source_path = gtk_editable_get_chars (GTK_EDITABLE (copy_source_entry), 0, 
										  -1);
	dest_path = gtk_editable_get_chars (GTK_EDITABLE (copy_dest_entry), 0, 
										  -1);
	stripped_source_path = g_strstrip (source_path);
	stripped_dest_path = g_strstrip (dest_path);
	
	/* Make sure not to overwrite a path the user has already put in. */
	if (strlen (stripped_source_path) > 0 &&
		strlen (stripped_dest_path) == 0)
	{
		last_slash = strrchr (stripped_source_path, '/');
		
		if (last_slash)
		{ 
			source_path_parent = g_strndup (stripped_source_path,
											(last_slash - 
											 stripped_source_path));
			suggested_path = g_strconcat (source_path_parent, "/", NULL);
			
			gtk_entry_set_text (GTK_ENTRY (widget), 
								suggested_path);
			gtk_editable_set_position (GTK_EDITABLE (widget), -1);
			
			g_free (source_path_parent);
			g_free (suggested_path);
		}
	}
	
	g_free (source_path);
	
	return TRUE;
}

static void
on_copy_command_finished (AnjutaCommand *command, guint return_code,
						  Subversion *plugin)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
									  NULL);
	
	anjuta_status (status, _("Subversion: Copy complete."), 5);
	
	report_errors (command, return_code);
	
	svn_copy_command_destroy (SVN_COPY_COMMAND (command));
}

static void
on_subversion_copy_response (GtkDialog *dialog, gint response,
							 SubversionData *data)
{
	GtkWidget *copy_source_entry;
	GtkWidget *copy_dest_entry;
	GtkWidget *copy_working_copy_radio;
	GtkWidget *copy_repository_head_radio;
	GtkWidget *copy_other_revision_radio;
	GtkWidget *copy_revision_entry;
	GtkWidget *copy_log_view;
	gchar *source_path;
	gchar *dest_path;
	gchar *revision_text;
	glong revision;
	gchar *log;
	SvnCopyCommand *copy_command;
	
	if (response == GTK_RESPONSE_OK)
	{
		copy_source_entry = glade_xml_get_widget (data->gxml, 
												  "copy_source_entry");
		copy_dest_entry = glade_xml_get_widget (data->gxml,
												"copy_dest_entry");
		copy_working_copy_radio = glade_xml_get_widget (data->gxml,
														"copy_working_copy_radio");
		copy_repository_head_radio = glade_xml_get_widget (data->gxml,
														   "copy_repository_head_radio");
		copy_other_revision_radio = glade_xml_get_widget (data->gxml,
														  "copy_other_revision_radio");
		copy_log_view = glade_xml_get_widget (data->gxml,
											  "copy_log_view");
		
		source_path = gtk_editable_get_chars (GTK_EDITABLE (copy_source_entry),
											  0, -1);
		dest_path = gtk_editable_get_chars (GTK_EDITABLE (copy_dest_entry),
											0, -1);
		
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (copy_working_copy_radio)))
			revision = SVN_COPY_REVISION_WORKING;
		
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (copy_repository_head_radio)))
			revision = SVN_COPY_REVISION_HEAD;
		
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (copy_other_revision_radio)))
		{
			copy_revision_entry = glade_xml_get_widget (data->gxml, 
														"copy_revision_entry");
			revision_text = gtk_editable_get_chars (GTK_EDITABLE (copy_revision_entry),
													0, -1);
			revision = atol (revision_text);
			
			g_free (revision_text);
		}
		
		log = get_log_from_textview (copy_log_view);
		
		create_message_view (data->plugin);
		
		g_print ("Source path: %s\n", source_path);
		g_print ("Destination path: %s\n", dest_path);
		
		copy_command = svn_copy_command_new (source_path, revision, dest_path,
											 log);
		
		g_signal_connect (G_OBJECT (copy_command), "command-finished",
						  G_CALLBACK (on_copy_command_finished),
						  data->plugin);
			
		g_signal_connect (G_OBJECT (copy_command), "data-arrived",
						  G_CALLBACK (on_command_info_arrived),
						  data->plugin);
		
		anjuta_command_start (ANJUTA_COMMAND (copy_command));
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	subversion_data_free (data);
}

static void
subversion_copy_dialog (GtkAction *action, Subversion *plugin, gchar *filename)
{
	GladeXML *gxml;
	GtkWidget *subversion_copy;
	GtkWidget *copy_source_entry;
	GtkWidget *copy_dest_entry;
	GtkWidget *copy_browse_button;
	GtkWidget *copy_other_revision_radio;
	SubversionData *data;
	
	gxml = glade_xml_new (GLADE_FILE, "subversion_copy", NULL);
	subversion_copy = glade_xml_get_widget (gxml, "subversion_copy");
	copy_source_entry = glade_xml_get_widget (gxml, "copy_source_entry");
	copy_dest_entry = glade_xml_get_widget (gxml, "copy_dest_entry");
	copy_browse_button = glade_xml_get_widget (gxml, "copy_browse_button");
	copy_other_revision_radio = glade_xml_get_widget (gxml,
													  "copy_other_revision_radio");
	
	data = subversion_data_new (plugin, gxml);
	
	g_signal_connect (G_OBJECT (subversion_copy), "response",
					  G_CALLBACK (on_subversion_copy_response),
					  data);
	
	g_signal_connect (G_OBJECT (copy_dest_entry), "focus-in-event",
					  G_CALLBACK (on_copy_dest_entry_focus_in),
					  data);
	
	g_signal_connect (G_OBJECT (copy_browse_button), "clicked",
					  G_CALLBACK (on_copy_browse_button_clicked),
					  data);
	
	g_signal_connect (G_OBJECT (copy_other_revision_radio), "toggled",
					  G_CALLBACK (on_copy_other_revision_radio_toggled),
					  data);
	
	gtk_entry_set_text (GTK_ENTRY (copy_source_entry), filename);
	
	gtk_widget_show (subversion_copy);
}

void
on_menu_subversion_copy (GtkAction *action, Subversion *plugin)
{
	subversion_copy_dialog (action, plugin, NULL);
}

void
on_fm_subversion_copy (GtkAction *action, Subversion *plugin)
{
	subversion_copy_dialog (action, plugin, plugin->fm_current_filename);
}
