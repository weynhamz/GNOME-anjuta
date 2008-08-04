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
	GtkWidget *diff_path_entry;
	GtkWidget *diff_no_recursive_check;
	GtkWidget *diff_revision_entry;
	GtkWidget *diff_save_open_files_check;
	const gchar *path;
	const gchar *revision_text;
	glong revision;
	IAnjutaDocumentManager *docman;
	gchar *filename;
	gchar *editor_name;
	IAnjutaEditor *editor;
	SvnDiffCommand *diff_command;
	guint pulse_timer_id;
	
	switch (response)
	{
		case GTK_RESPONSE_OK:
		{
			gchar *aux;

			diff_path_entry = glade_xml_get_widget (data->gxml, 
													"diff_path_entry");
			diff_no_recursive_check = glade_xml_get_widget (data->gxml, 
															"diff_no_recursive_check");
			diff_revision_entry = glade_xml_get_widget (data->gxml, 
														"diff_revision_entry");
			diff_save_open_files_check = glade_xml_get_widget (data->gxml,
															   "diff_save_open_files_check");
			path = g_strdup (gtk_entry_get_text (GTK_ENTRY (diff_path_entry)));
			revision_text = gtk_entry_get_text (GTK_ENTRY (diff_revision_entry));
			revision = atol (revision_text);
			
			if (!check_input (GTK_WIDGET (dialog), diff_path_entry, 
							  _("Please enter a path.")))
			{
				break;
			}
				
			docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (data->plugin)->shell,
	                                     IAnjutaDocumentManager, NULL);
			filename = get_filename_from_full_path ((gchar *) path);
			
			aux = g_strdup (_("[Head/Working Copy]"));
			editor_name = g_strdup_printf ("%s %s.diff", aux, filename);
			g_free (aux);
			editor = ianjuta_document_manager_add_buffer(docman, editor_name, 
														 "", NULL);
			
			g_free (filename);
			g_free (editor_name);
			
			diff_command = svn_diff_command_new ((gchar *) path, 
												 SVN_DIFF_REVISION_NONE,
												 SVN_DIFF_REVISION_NONE,
												 !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (diff_no_recursive_check)));
			
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
			
			g_object_weak_ref (G_OBJECT (editor), 
							   (GWeakNotify) disconnect_data_arrived_signals,
							   diff_command);
			
			if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (diff_save_open_files_check)))
				ianjuta_file_savable_save (IANJUTA_FILE_SAVABLE (docman), NULL);
			
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
subversion_diff_dialog (GtkAction *action, Subversion *plugin, gchar *filename)
{
	GladeXML *gxml;
	GtkWidget *subversion_diff; 
	GtkWidget *diff_path_entry;
	GtkWidget *diff_whole_project_check;
	SubversionData *data;
	
	gxml = glade_xml_new (GLADE_FILE, "subversion_diff", NULL);
	subversion_diff = glade_xml_get_widget(gxml, "subversion_diff");
	diff_path_entry = glade_xml_get_widget(gxml, "diff_path_entry");
	diff_whole_project_check = glade_xml_get_widget(gxml, 
													"diff_whole_project_check");
	data = subversion_data_new (plugin, gxml);
	
	g_object_set_data (G_OBJECT (diff_whole_project_check), "fileentry", 
					   diff_path_entry);
	
	if (filename)
		gtk_entry_set_text (GTK_ENTRY (diff_path_entry), filename);
	
	
	g_signal_connect(G_OBJECT (diff_whole_project_check), "toggled", 
					 G_CALLBACK (on_whole_project_toggled), 
					 plugin);
	init_whole_project (plugin, diff_whole_project_check, !filename);
	
	
	g_signal_connect (G_OBJECT (subversion_diff), "response", 
					  G_CALLBACK (on_subversion_diff_response), 
					  data);
	
	gtk_widget_show (subversion_diff);
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
