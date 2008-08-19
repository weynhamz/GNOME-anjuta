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

#include "git-reset-dialog.h"

static void
on_reset_dialog_response (GtkDialog *dialog, gint response_id, 
						  GitUIData *data)
{
	GtkWidget *reset_revision_radio;
	GtkWidget *reset_revision_entry;
	GtkWidget *reset_soft_radio;
	GtkWidget *reset_hard_radio;
	gchar *revision;
	GitResetTreeMode mode;
	GitResetTreeCommand *reset_command;
	
	if (response_id == GTK_RESPONSE_OK)
	{	
		reset_revision_radio = glade_xml_get_widget (data->gxml, 
													 "reset_revision_radio");
		reset_revision_entry = glade_xml_get_widget (data->gxml, 
													 "reset_revision_entry");
		reset_soft_radio = glade_xml_get_widget (data->gxml,
												 "reset_soft_radio");
		reset_hard_radio = glade_xml_get_widget (data->gxml,
												 "reset_hard_radio");
		revision = NULL;
		mode = GIT_RESET_TREE_MODE_MIXED;
		
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (reset_revision_radio)))
		{
			revision = gtk_editable_get_chars (GTK_EDITABLE (reset_revision_entry),
											   0, -1);
			if (!git_check_input (GTK_WIDGET (dialog), reset_revision_entry, 
								  revision, _("Please enter a revision.")))
			{
				g_free (revision);
				return;
			}
		}
		
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (reset_soft_radio)))
			mode = GIT_RESET_TREE_MODE_SOFT;
		else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (reset_hard_radio)))
			mode = GIT_RESET_TREE_MODE_HARD;
		
		if (revision)
		{
			reset_command = git_reset_tree_command_new (data->plugin->project_root_directory,
														revision, mode);
		}
		else
		{
			reset_command = git_reset_tree_command_new (data->plugin->project_root_directory,
														GIT_RESET_TREE_PREVIOUS, 
														mode);
		}
		
		
		
		git_create_message_view (data->plugin);
		
		g_signal_connect (G_OBJECT (reset_command), "command-finished",
						  G_CALLBACK (on_git_command_finished),
						  data->plugin);
		
		g_signal_connect (G_OBJECT (reset_command), "data-arrived",
						  G_CALLBACK (on_git_command_info_arrived),
						  data->plugin);
		
		anjuta_command_start (ANJUTA_COMMAND (reset_command));
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	git_ui_data_free (data);
}

static void
on_reset_revision_radio_toggled (GtkToggleButton *toggle_button, 
								 GitUIData *data)
{
	GtkWidget *reset_dialog;
	GtkWidget *reset_revision_entry;
	gboolean active;
	
	reset_dialog = glade_xml_get_widget (data->gxml, 
										 "reset_dialog");
	reset_revision_entry = glade_xml_get_widget (data->gxml,
												 "reset_revision_entry");
	
	active = gtk_toggle_button_get_active (toggle_button);
	gtk_widget_set_sensitive (reset_revision_entry, active);
	
	if (active)
	{
		gtk_window_set_focus (GTK_WINDOW (reset_dialog),
							  reset_revision_entry);
	}
	
}

static void
reset_dialog (Git *plugin, const gchar *revision)
{
	GladeXML *gxml;
	GtkWidget *dialog;
	GtkWidget *reset_revision_radio;
	GtkWidget *reset_revision_entry;
	GitUIData *data;
	
	gxml = glade_xml_new (GLADE_FILE, "reset_dialog", NULL);
	dialog = glade_xml_get_widget (gxml, "reset_dialog");
	reset_revision_radio = glade_xml_get_widget (gxml, 
												 "reset_revision_radio");
	reset_revision_entry = glade_xml_get_widget (gxml, 
												 "reset_revision_entry");
	data = git_ui_data_new (plugin, gxml);
	
	g_signal_connect (G_OBJECT (dialog), "response", 
					  G_CALLBACK (on_reset_dialog_response), 
					  data);
	
	g_signal_connect (G_OBJECT (reset_revision_radio), "toggled",
					  G_CALLBACK (on_reset_revision_radio_toggled),
					  data);
	
	if (revision)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (reset_revision_radio), TRUE);
		gtk_entry_set_text (GTK_ENTRY (reset_revision_entry), revision);
	}
	
	gtk_widget_show_all (dialog);
}

void
on_menu_git_reset (GtkAction *action, Git *plugin)
{
	reset_dialog (plugin, NULL);
}

void
on_log_menu_git_reset (GtkAction *action, Git *plugin)
{
	GitRevision *revision;
	gchar *sha;
	
	revision = git_log_get_selected_revision (plugin);
	
	if (revision)
	{
		sha = git_revision_get_sha (revision);
		
		reset_dialog (plugin, sha);
		g_free (sha);
	}
}
