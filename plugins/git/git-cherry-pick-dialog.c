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

#include "git-cherry-pick-dialog.h"

static void
on_cherry_pick_dialog_response (GtkDialog *dialog, gint response_id, 
                                GitUIData *data)
{
	GtkWidget *cherry_pick_revision_entry;
	GtkWidget *cherry_pick_no_commit_check;
	GtkWidget *cherry_pick_show_source_check;
	GtkWidget *cherry_pick_signoff_check;
	gchar *revision;
	GitCherryPickCommand *cherry_pick_command;
	
	if (response_id == GTK_RESPONSE_OK)
	{
		cherry_pick_revision_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
		                                                                 "cherry_pick_revision_entry"));
		cherry_pick_no_commit_check = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
		                                                                  "cherry_pick_no_commit_check"));
		cherry_pick_show_source_check = GTK_WIDGET (gtk_builder_get_object (data->bxml,
		                                                                    "cherry_pick_show_source_check"));
		cherry_pick_signoff_check = GTK_WIDGET (gtk_builder_get_object (data->bxml,
		                                                                "cherry_pick_signoff_check"));
		revision = gtk_editable_get_chars (GTK_EDITABLE (cherry_pick_revision_entry),
										   0, -1);
		
		if (git_check_input (GTK_WIDGET (dialog), cherry_pick_revision_entry, revision, 
							 _("Please enter a revision.")))
		{
			cherry_pick_command = git_cherry_pick_command_new (data->plugin->project_root_directory,
			                                                   revision,
			                                                   gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cherry_pick_no_commit_check)),
			                                                   gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cherry_pick_show_source_check)),
			                                                   gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cherry_pick_signoff_check)));
												
			
			g_free (revision);
			
			git_create_message_view (data->plugin);
			
			g_signal_connect (G_OBJECT (cherry_pick_command), "command-finished",
							  G_CALLBACK (on_git_command_finished),
							  data->plugin);
			
			g_signal_connect (G_OBJECT (cherry_pick_command), "data-arrived",
							  G_CALLBACK (on_git_command_info_arrived),
							  data->plugin);
			
			anjuta_command_start (ANJUTA_COMMAND (cherry_pick_command));
		}
		else
		{
			g_free (revision);
			return;
		}
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	git_ui_data_free (data);
}

static void
on_cherry_pick_no_commit_check_toggled (GtkToggleButton *toggle_button,
                                        GitUIData *data)
{
	GtkWidget *cherry_pick_show_source_check;
	GtkWidget *cherry_pick_signoff_check;
	gboolean active;

	cherry_pick_show_source_check = GTK_WIDGET (gtk_builder_get_object (data->bxml,
		                                                                    "cherry_pick_show_source_check"));
	cherry_pick_signoff_check = GTK_WIDGET (gtk_builder_get_object (data->bxml,
	                                                                "cherry_pick_signoff_check"));
	active = gtk_toggle_button_get_active (toggle_button);

	/* It doesn't make much sense to show a source revision or add a signed off
	 * line when there's no commit to be made. */
	if (active)
	{
		gtk_widget_set_sensitive (cherry_pick_show_source_check, FALSE);
		gtk_widget_set_sensitive (cherry_pick_signoff_check, FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cherry_pick_show_source_check), FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cherry_pick_signoff_check), FALSE);
		
	}
	else
	{
		gtk_widget_set_sensitive (cherry_pick_show_source_check, TRUE);
		gtk_widget_set_sensitive (cherry_pick_signoff_check, TRUE);
	}
}

static void
cherry_pick_dialog (Git *plugin, const gchar *revision)
{
	GtkBuilder *bxml;
	gchar *objects[] = {"cherry_pick_dialog", NULL};
	GError *error;
	GtkWidget *dialog;
	GtkWidget *cherry_pick_revision_entry;
	GtkWidget *cherry_pick_no_commit_check;
	GitUIData *data;
	
	bxml = gtk_builder_new ();
	error = NULL;

	if (!gtk_builder_add_objects_from_file (bxml, BUILDER_FILE, objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}
	
	dialog = GTK_WIDGET (gtk_builder_get_object (bxml, "cherry_pick_dialog"));
	cherry_pick_revision_entry = GTK_WIDGET (gtk_builder_get_object (bxml, 
	                                                                 "cherry_pick_revision_entry"));
	cherry_pick_no_commit_check = GTK_WIDGET (gtk_builder_get_object (bxml, 
	                                                                  "cherry_pick_no_commit_check"));
	
	data = git_ui_data_new (plugin, bxml);
	
	if (revision)
		gtk_entry_set_text (GTK_ENTRY (cherry_pick_revision_entry), revision);
	
	g_signal_connect (G_OBJECT (dialog), "response", 
					  G_CALLBACK (on_cherry_pick_dialog_response), 
					  data);

	g_signal_connect (G_OBJECT (cherry_pick_no_commit_check), "toggled",
	                  G_CALLBACK (on_cherry_pick_no_commit_check_toggled),
	                  data);
	
	gtk_widget_show_all (dialog);
}

void
on_menu_git_cherry_pick (GtkAction *action, Git *plugin)
{
	cherry_pick_dialog (plugin, NULL);
}

void
on_log_menu_git_cherry_pick (GtkAction *action, Git *plugin)
{
	GitRevision *revision;
	gchar *sha;
	
	revision = git_log_get_selected_revision (plugin);
	
	if (revision)
	{
		sha = git_revision_get_sha (revision);
		
		cherry_pick_dialog (plugin, sha);
		g_free (sha);
	}
}
