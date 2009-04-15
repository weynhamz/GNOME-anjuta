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
 * You should have received a switch of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include "subversion-switch-dialog.h"

#define BROWSE_BUTTON_SWITCH_DIALOG "browse_button_switch_dialog"

static void
on_switch_other_revision_radio_toggled (GtkToggleButton *toggle_button,
									    SubversionData *data)
{
	GtkWidget *switch_revision_entry;
	GtkWidget *subversion_switch;
	gboolean active;
	
	switch_revision_entry = glade_xml_get_widget (data->gxml, 
												  "switch_revision_entry");
	subversion_switch = glade_xml_get_widget (data->gxml,
											  "subversion_switch");
	active = gtk_toggle_button_get_active (toggle_button);
	gtk_widget_set_sensitive (switch_revision_entry, active);
	
	if (active)
	{
		gtk_window_set_focus (GTK_WINDOW (subversion_switch), 
							  switch_revision_entry);
	}
}

static void
on_switch_command_finished (AnjutaCommand *command, guint return_code,
						    Subversion *plugin)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
									  NULL);
	
	anjuta_status (status, _("Subversion: Switch complete."), 5);
	
	report_errors (command, return_code);
	
	svn_switch_command_destroy (SVN_SWITCH_COMMAND (command));
}

static void
on_subversion_switch_response (GtkDialog *dialog, gint response,
							 SubversionData *data)
{
	GtkWidget *switch_working_copy_entry;
	GtkWidget *switch_url_entry;
	GtkWidget *switch_head_revision_radio;
	GtkWidget *switch_other_revision_radio;
	GtkWidget *switch_revision_entry;
	GtkWidget *switch_no_recursive_check;
	gchar *working_copy_path;
	gchar *branch_url;
	gchar *revision_text;
	glong revision;
	SvnSwitchCommand *switch_command;
	
	if (response == GTK_RESPONSE_OK)
	{
		switch_working_copy_entry = glade_xml_get_widget (data->gxml, 
												  		  "switch_working_copy_entry");
		switch_url_entry = glade_xml_get_widget (data->gxml,
												 "switch_url_entry");
		switch_head_revision_radio = glade_xml_get_widget (data->gxml,
														   "switch_head_revision_radio");
		switch_other_revision_radio = glade_xml_get_widget (data->gxml,
														    "switch_other_revision_radio");
		switch_no_recursive_check = glade_xml_get_widget (data->gxml,
														  "switch_no_recursive_check");
		
		working_copy_path = gtk_editable_get_chars (GTK_EDITABLE (switch_working_copy_entry),
											  		0, -1);
		branch_url = gtk_editable_get_chars (GTK_EDITABLE (switch_url_entry),
											 0, -1);
		
		if (!check_input (GTK_WIDGET (dialog), switch_working_copy_entry,
						  _("Please enter a working copy path.")))
		{
			return;
		}
		
		if (!check_input (GTK_WIDGET (dialog), switch_url_entry,
						  _("Please enter a branch/tag URL.")))
		{
			return;
		}
		
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (switch_head_revision_radio)))
			revision = SVN_SWITCH_REVISION_HEAD;
		
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (switch_other_revision_radio)))
		{
			switch_revision_entry = glade_xml_get_widget (data->gxml, 
														  "switch_revision_entry");
			
			if (!check_input (GTK_WIDGET (dialog), switch_revision_entry,
						  	  _("Please enter a revision.")))
			{
				return;
			}
			
			revision_text = gtk_editable_get_chars (GTK_EDITABLE (switch_revision_entry),
													0, -1);
			revision = atol (revision_text);
			
			g_free (revision_text);
		}
		
		create_message_view (data->plugin);

		switch_command = svn_switch_command_new (working_copy_path, branch_url,
												 revision,
												 !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (switch_no_recursive_check)));
			
		g_signal_connect (G_OBJECT (switch_command), "command-finished",
						  G_CALLBACK (on_switch_command_finished),
						  data->plugin);
			
		g_signal_connect (G_OBJECT (switch_command), "data-arrived",
						  G_CALLBACK (on_command_info_arrived),
						  data->plugin);
		
		anjuta_command_start (ANJUTA_COMMAND (switch_command));
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	subversion_data_free (data);
}

static void
subversion_switch_dialog (GtkAction *action, Subversion *plugin)
{
	GladeXML *gxml;
	GtkWidget *subversion_switch;
	GtkWidget *switch_working_copy_entry;
	GtkWidget *switch_other_revision_radio;
	GtkWidget *button;
	SubversionData *data;
	
	gxml = glade_xml_new (GLADE_FILE, "subversion_switch", NULL);
	subversion_switch = glade_xml_get_widget (gxml, "subversion_switch");
	switch_working_copy_entry = glade_xml_get_widget (gxml,
													  "switch_working_copy_entry");
	switch_other_revision_radio = glade_xml_get_widget (gxml,
													    "switch_other_revision_radio");
	
	data = subversion_data_new (plugin, gxml);

	button = glade_xml_get_widget(gxml, BROWSE_BUTTON_SWITCH_DIALOG);
	g_signal_connect(G_OBJECT(button), "clicked", 
		G_CALLBACK(on_subversion_browse_button_clicked), switch_working_copy_entry);

	g_signal_connect (G_OBJECT (subversion_switch), "response",
					  G_CALLBACK (on_subversion_switch_response),
					  data);
	
	g_signal_connect (G_OBJECT (switch_other_revision_radio), "toggled",
					  G_CALLBACK (on_switch_other_revision_radio_toggled),
					  data);
	
	if (plugin->project_root_dir)
	{
		gtk_entry_set_text (GTK_ENTRY (switch_working_copy_entry),
							plugin->project_root_dir);
	}
	
	gtk_widget_show (subversion_switch);
}

void
on_menu_subversion_switch (GtkAction *action, Subversion *plugin)
{
	subversion_switch_dialog (action, plugin);
}
