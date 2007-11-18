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

#include "subversion-update-dialog.h"

static void
on_update_command_finished (AnjutaCommand *command, guint return_code,
							Subversion *plugin)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
									  NULL);
	
	anjuta_status (status, _("Subversion: Update complete."), 5);
	
	report_errors (command, return_code);
	
	svn_update_command_destroy (SVN_UPDATE_COMMAND (command));	
}

static void
on_subversion_update_response(GtkDialog* dialog, gint response, SubversionData* data)
{	
	switch (response)
	{
		case GTK_RESPONSE_OK:
		{
			const gchar* revision;
		
			GtkWidget* norecurse;
			GtkWidget* revisionentry;
			GtkWidget* fileentry = glade_xml_get_widget(data->gxml, "subversion_filename");
			const gchar* filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(fileentry)));
			SvnUpdateCommand *update_command;
			
			norecurse = glade_xml_get_widget(data->gxml, "subversion_norecurse");
			revisionentry = glade_xml_get_widget(data->gxml, "subversion_revision");
			revision = gtk_entry_get_text(GTK_ENTRY(revisionentry));
			
			if (!check_filename(dialog, filename))
				break;	
			
			update_command = svn_update_command_new ((gchar *) filename, 
													 (gchar *) revision, 
													 !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(norecurse)));
			create_message_view (data->plugin);
			
			g_signal_connect (G_OBJECT (update_command), "command-finished",
							  G_CALLBACK (on_update_command_finished),
							  data->plugin);
			
			g_signal_connect (G_OBJECT (update_command), "data-arrived",
							  G_CALLBACK (on_command_info_arrived),
							  data->plugin);
			
			anjuta_command_start (ANJUTA_COMMAND (update_command));
			
			subversion_data_free(data);
			gtk_widget_destroy(GTK_WIDGET(dialog));
			break;
		}
		default:
		{
			gtk_widget_destroy(GTK_WIDGET(dialog));
			subversion_data_free(data);
			break;
		}
	}
}

static void
subversion_update_dialog (GtkAction* action, Subversion* plugin, gchar *filename)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	GtkWidget* project;
	SubversionData* data;
	
	gxml = glade_xml_new(GLADE_FILE, "subversion_update", NULL);
	
	dialog = glade_xml_get_widget(gxml, "subversion_update");
	fileentry = glade_xml_get_widget(gxml, "subversion_filename");
	if (filename)
		gtk_entry_set_text(GTK_ENTRY(fileentry), filename);
	
	project = glade_xml_get_widget(gxml, "subversion_project");
	g_object_set_data (G_OBJECT (project), "fileentry", fileentry);
	g_signal_connect(G_OBJECT(project), "toggled", 
		G_CALLBACK(on_whole_project_toggled), plugin);
	init_whole_project(plugin, project, !filename);
	
	data = subversion_data_new(plugin, gxml);
	g_signal_connect(G_OBJECT(dialog), "response", 
		G_CALLBACK(on_subversion_update_response), data);
	
	gtk_widget_show(dialog);	
}

void 
on_menu_subversion_update (GtkAction* action, Subversion* plugin)
{
	subversion_update_dialog(action, plugin, NULL);
}

void 
on_fm_subversion_update (GtkAction* action, Subversion* plugin)
{
	subversion_update_dialog(action, plugin, plugin->fm_current_filename);
}
