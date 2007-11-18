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

#include "subversion-diff-dialog.h"

static void
on_subversion_diff_response(GtkDialog* dialog, gint response, SubversionData* data)
{	
	switch (response)
	{
		case GTK_RESPONSE_OK:
		{
			IAnjutaDocumentManager *docman;
			gchar *filename;
			gchar *editor_name;
			IAnjutaEditor *editor;
			const gchar* revision_text;
			glong revision;
		
			GtkWidget* norecurse;
			GtkWidget* revisionentry;
			GtkWidget* fileentry = glade_xml_get_widget(data->gxml, "subversion_filename");
			const gchar* path = g_strdup(gtk_entry_get_text(GTK_ENTRY(fileentry)));
			SvnDiffCommand *diff_command;
			guint pulse_timer_id;
			
			norecurse = glade_xml_get_widget(data->gxml, "subversion_norecurse");
			revisionentry = glade_xml_get_widget(data->gxml, "subversion_revision");
			revision_text = gtk_entry_get_text(GTK_ENTRY(revisionentry));
			revision = atol (revision_text);
			
			if (!check_filename(dialog, path))
				break;	
				
			docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (data->plugin)->shell,
	                                     IAnjutaDocumentManager, NULL);
			filename = get_filename_from_full_path ((gchar *) path);
			editor_name = g_strdup_printf ("[Head/Working Copy] %s.diff", 
										   filename);
			editor = ianjuta_document_manager_add_buffer(docman, editor_name, 
														 "", NULL);
			
			g_free (filename);
			g_free (editor_name);
			
			diff_command = svn_diff_command_new ((gchar *) path, 
												 SVN_DIFF_REVISION_NONE,
												 SVN_DIFF_REVISION_NONE,
												 !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(norecurse)));
			
			pulse_timer_id = status_bar_progress_pulse (data->plugin,
														_("Subversion: " 
														  "Retrieving "
														  "diff..."));
			
			g_signal_connect (G_OBJECT (diff_command), "command-finished",
							  G_CALLBACK (stop_status_bar_progress_pulse),
							  GUINT_TO_POINTER (pulse_timer_id));
			
			
			g_signal_connect (G_OBJECT (diff_command), "command-finished",
							  G_CALLBACK (on_diff_command_finished), 
							  data->plugin);
			
			g_signal_connect (G_OBJECT (diff_command), "data-arrived",
							  G_CALLBACK (send_diff_command_output_to_editor),
							  editor);
			
			anjuta_command_start (ANJUTA_COMMAND (diff_command));
			
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
subversion_diff_dialog (GtkAction* action, Subversion* plugin, gchar *filename)
{
	GladeXML* gxml;
	GtkWidget* dialog; 
	GtkWidget* fileentry;
	GtkWidget* project;
	SubversionData* data;
	
	gxml = glade_xml_new(GLADE_FILE, "subversion_diff", NULL);
	
	dialog = glade_xml_get_widget(gxml, "subversion_diff");
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
		G_CALLBACK(on_subversion_diff_response), data);
	
	gtk_widget_show(dialog);
}

void 
on_menu_subversion_diff (GtkAction* action, Subversion* plugin)
{
	subversion_diff_dialog (action, plugin, NULL);
}

void 
on_fm_subversion_diff (GtkAction *action, Subversion *plugin)
{
	subversion_diff_dialog (action, plugin, plugin->fm_current_filename);
}
