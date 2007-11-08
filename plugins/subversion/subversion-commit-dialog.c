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

#include "subversion-commit-dialog.h"

static void
on_commit_command_finished (AnjutaCommand *command, guint return_code,
							Subversion *plugin)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
									  NULL);
	
	anjuta_status (status, _("Subversion: Commit complete."), 5);
	
	report_errors (command, return_code);
	
	svn_commit_command_destroy (SVN_COMMIT_COMMAND (command));
}

static void
on_subversion_commit_response(GtkDialog* dialog, gint response, 
							  SubversionData* data)
{	
	switch (response)
	{
		case GTK_RESPONSE_OK:
		{
			gchar* log;
			GtkWidget* logtext;
			GtkWidget* norecurse;
			GtkWidget *commit_status_view;
			GList *selected_paths;
			SvnCommitCommand *commit_command;
			guint pulse_timer_id;
			
			logtext = glade_xml_get_widget(data->gxml, "subversion_log_view");
			log = get_log_from_textview(logtext);
			if (!g_utf8_strlen(log, -1))
			{
				gint result;
				GtkWidget* dlg = gtk_message_dialog_new(GTK_WINDOW(dialog), 
														GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO,
														GTK_BUTTONS_YES_NO, 
														_("Are you sure that you do not want a log message?"));
				result = gtk_dialog_run(GTK_DIALOG(dlg));
				gtk_widget_destroy(dlg);
				if (result == GTK_RESPONSE_NO)
					break;
			}
			
			norecurse = glade_xml_get_widget(data->gxml, "subversion_norecurse");
			
			commit_status_view = glade_xml_get_widget (data->gxml, 
													   "commit_status_view");
			
			selected_paths = anjuta_vcs_status_tree_view_get_selected (ANJUTA_VCS_STATUS_TREE_VIEW (commit_status_view));

			commit_command = svn_commit_command_new (selected_paths, 
													 (gchar *) log,
													 !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(norecurse)));
			
			svn_command_free_path_list (selected_paths);
			
			create_message_view (data->plugin);
		
			pulse_timer_id = status_bar_progress_pulse (data->plugin,
														_("Subversion: " 
														  "Committing changes  "
														  "to the "
														  "repository..."));
			
			g_signal_connect (G_OBJECT (commit_command), "command-finished",
							  G_CALLBACK (stop_status_bar_progress_pulse),
							  GUINT_TO_POINTER (pulse_timer_id));
			
			g_signal_connect (G_OBJECT (commit_command), "command-finished",
							  G_CALLBACK (on_commit_command_finished),
							  data->plugin);
			
			g_signal_connect (G_OBJECT (commit_command), "data-arrived",
							  G_CALLBACK (on_command_info_arrived),
							  data->plugin);
			
			anjuta_command_start (ANJUTA_COMMAND (commit_command));
		
			subversion_data_free(data);
			gtk_widget_destroy (GTK_WIDGET(dialog));
			break;
		}
		default:
		{
			subversion_data_free(data);
			gtk_widget_destroy(GTK_WIDGET(dialog));
			break;
		}
	}
}

static void
subversion_commit_dialog (GtkAction* action, Subversion* plugin, 
						  gchar *filename)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget *commit_select_all_button;
	GtkWidget *commit_clear_button;
	GtkWidget *commit_status_view;
	SvnStatusCommand *status_command;
	SubversionData* data;
	
	gxml = glade_xml_new(GLADE_FILE, "subversion_commit", NULL);
	
	dialog = glade_xml_get_widget(gxml, "subversion_commit");
	commit_select_all_button = glade_xml_get_widget (gxml, 
													 "commit_select_all_button");
	commit_clear_button = glade_xml_get_widget (gxml,
												"commit_clear_button");
	commit_status_view = glade_xml_get_widget (gxml, "commit_status_view");
	
	status_command = svn_status_command_new (plugin->project_root_dir, 
											 TRUE, TRUE);
	
	g_signal_connect (G_OBJECT (commit_select_all_button), "clicked",
					  G_CALLBACK (select_all_status_items),
					  commit_status_view);
	
	g_signal_connect (G_OBJECT (commit_clear_button), "clicked",
					  G_CALLBACK (clear_all_status_selections),
					  commit_status_view);
	
	g_signal_connect (G_OBJECT (status_command), "command-finished",
					  G_CALLBACK (on_status_command_finished),
					  NULL);
	
	g_signal_connect (G_OBJECT (status_command), "data-arrived",
					  G_CALLBACK (on_status_command_data_arrived),
					  commit_status_view);
	
	anjuta_command_start (ANJUTA_COMMAND (status_command));
	
	data = subversion_data_new(plugin, gxml);
	g_signal_connect(G_OBJECT(dialog), "response", 
		G_CALLBACK(on_subversion_commit_response), data);
	
	gtk_widget_show_all (dialog);
	
}

void 
on_menu_subversion_commit (GtkAction* action, Subversion* plugin)
{
	subversion_commit_dialog(action, plugin, plugin->current_editor_filename);
}

void 
on_fm_subversion_commit (GtkAction* action, Subversion* plugin)
{
	subversion_commit_dialog(action, plugin, plugin->fm_current_filename);
}
