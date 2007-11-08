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

#include "subversion-revert-dialog.h"

static void
on_revert_command_finished (AnjutaCommand *command, guint return_code,
							Subversion *plugin)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
									  NULL);
	
	anjuta_status (status, _("Subversion: Revert complete."), 5);
	
	report_errors (command, return_code);
	
	svn_revert_command_destroy (SVN_REVERT_COMMAND (command));
}

static void
on_subversion_revert_response (GtkDialog *dialog, gint response,
							   SubversionData *data)
{
	SvnRevertCommand *revert_command;
	GtkWidget *revert_status_view;
	GList *selected_paths;
	
	if (response == GTK_RESPONSE_OK)
	{
		revert_status_view = glade_xml_get_widget (data->gxml,
												   "revert_status_view");
		selected_paths = anjuta_vcs_status_tree_view_get_selected (ANJUTA_VCS_STATUS_TREE_VIEW (revert_status_view));
		revert_command = svn_revert_command_new (selected_paths, TRUE);
		
		svn_command_free_path_list (selected_paths);
		
		g_signal_connect (G_OBJECT (revert_command), "data-arrived",
						  G_CALLBACK (on_command_info_arrived),
						  data->plugin);
		
		g_signal_connect (G_OBJECT (revert_command), "command-finished",
						  G_CALLBACK (on_revert_command_finished),
						  data->plugin);
		
		create_message_view (data->plugin);
		
		anjuta_command_start (ANJUTA_COMMAND (revert_command));
	}
	
	subversion_data_free (data);
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void 
subversion_revert_dialog (GtkAction *action, Subversion *plugin)
{
	GladeXML *gxml;
	GtkWidget *subversion_revert;
	GtkWidget *revert_select_all_button;
	GtkWidget *revert_clear_button;
	GtkWidget *revert_status_view;
	SvnStatusCommand *status_command;
	SubversionData *data;
	
	gxml = glade_xml_new (GLADE_FILE, "subversion_revert", NULL);
	
	subversion_revert = glade_xml_get_widget (gxml,
											  "subversion_revert");
	revert_select_all_button = glade_xml_get_widget (gxml,
													 "revert_select_all_button");
	revert_clear_button = glade_xml_get_widget (gxml,
												"revert_clear_button");
	revert_status_view = glade_xml_get_widget (gxml,
											   "revert_status_view");
	
	status_command = svn_status_command_new (plugin->project_root_dir, TRUE,
											 FALSE);
	
	data = subversion_data_new (plugin, gxml);
	
	g_signal_connect (G_OBJECT (subversion_revert), "response",
					  G_CALLBACK (on_subversion_revert_response),
					  data);
	
	g_signal_connect (G_OBJECT (revert_select_all_button), "clicked",
					  G_CALLBACK (select_all_status_items),
					  revert_status_view);
	
	g_signal_connect (G_OBJECT (revert_clear_button), "clicked",
					  G_CALLBACK (clear_all_status_selections),
					  revert_status_view);
	
	g_signal_connect (G_OBJECT (status_command), "data-arrived",
					  G_CALLBACK (on_status_command_data_arrived),
					  revert_status_view);
	
	g_signal_connect (G_OBJECT (status_command), "command-finished",
					  G_CALLBACK (on_status_command_finished),
					  NULL);
	
	anjuta_command_start (ANJUTA_COMMAND (status_command));
	
	gtk_dialog_run (GTK_DIALOG (subversion_revert));
}

void 
on_menu_subversion_revert (GtkAction *action, Subversion *plugin)
{
	subversion_revert_dialog (action, plugin);
}
