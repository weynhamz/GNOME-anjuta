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

#define BROWSE_BUTTON_DIFF_DIALOG "browse_button_diff_dialog"

static void
subversion_show_diff (const gchar *path, gboolean recursive, 
					  gboolean save_files, Subversion *plugin)
{
	IAnjutaDocumentManager *docman;
	gchar *filename;
	gchar *editor_name;
	IAnjutaEditor *editor;
	SvnDiffCommand *diff_command;
	guint pulse_timer_id;
	
	docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										 IAnjutaDocumentManager, NULL);
	filename = get_filename_from_full_path ((gchar *) path);
	editor_name = g_strdup_printf ("%s %s.diff", _("[Head/Working Copy]"),
								   filename);
	editor = ianjuta_document_manager_add_buffer(docman, editor_name, 
												 "", NULL);
	
	g_free (filename);
	g_free (editor_name);
	
	diff_command = svn_diff_command_new ((gchar *) path, 
										 SVN_DIFF_REVISION_NONE,
										 SVN_DIFF_REVISION_NONE,
										 plugin->project_root_dir,
										 recursive);
	
	pulse_timer_id = status_bar_progress_pulse (plugin,
												_("Subversion: " 
												  "Retrieving "
												  "diff..."));
	
	g_signal_connect (G_OBJECT (diff_command), "command-finished",
					  G_CALLBACK (stop_status_bar_progress_pulse),
					  GUINT_TO_POINTER (pulse_timer_id));
	
	
	g_signal_connect (G_OBJECT (diff_command), "command-finished",
					  G_CALLBACK (on_diff_command_finished), 
					  plugin);
	
	g_signal_connect (G_OBJECT (diff_command), "data-arrived",
					  G_CALLBACK (send_diff_command_output_to_editor),
					  editor);
	
	g_object_weak_ref (G_OBJECT (editor), 
					   (GWeakNotify) disconnect_data_arrived_signals,
					   diff_command);
	
	if (save_files)
		ianjuta_file_savable_save (IANJUTA_FILE_SAVABLE (docman), NULL);
	
	anjuta_command_start (ANJUTA_COMMAND (diff_command));
}

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
	
	switch (response)
	{
		case GTK_RESPONSE_OK:
		{
			diff_path_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
													"diff_path_entry"));
			diff_no_recursive_check = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
															"diff_no_recursive_check"));
			diff_revision_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
														"diff_revision_entry"));
			diff_save_open_files_check = GTK_WIDGET (gtk_builder_get_object (data->bxml,
															   "diff_save_open_files_check"));
			path = g_strdup (gtk_entry_get_text (GTK_ENTRY (diff_path_entry)));
			revision_text = gtk_entry_get_text (GTK_ENTRY (diff_revision_entry));
			revision = atol (revision_text);
			
			if (!check_input (GTK_WIDGET (dialog), diff_path_entry, 
							  _("Please enter a path.")))
			{
				break;
			}

			subversion_show_diff (path, 
								  !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (diff_no_recursive_check)),
								  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (diff_save_open_files_check)),
								  data->plugin);
			
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
	GtkBuilder *bxml = gtk_builder_new ();
	GtkWidget *subversion_diff; 
	GtkWidget *diff_path_entry;
	GtkWidget *diff_whole_project_check;
	GtkWidget *button;
	SubversionData *data;
	GError* error = NULL;

	if (!gtk_builder_add_from_file (bxml, GLADE_FILE, &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

	subversion_diff = GTK_WIDGET (gtk_builder_get_object (bxml, "subversion_diff"));
	diff_path_entry = GTK_WIDGET (gtk_builder_get_object (bxml, "diff_path_entry"));
	diff_whole_project_check = GTK_WIDGET (gtk_builder_get_object (bxml, 
													"diff_whole_project_check"));
	data = subversion_data_new (plugin, bxml);
	
	g_object_set_data (G_OBJECT (diff_whole_project_check), "fileentry", 
					   diff_path_entry);
	
	if (filename)
		gtk_entry_set_text (GTK_ENTRY (diff_path_entry), filename);
	
	
	g_signal_connect(G_OBJECT (diff_whole_project_check), "toggled", 
					 G_CALLBACK (on_whole_project_toggled), 
					 plugin);
	init_whole_project (plugin, diff_whole_project_check, !filename);

	button = GTK_WIDGET (gtk_builder_get_object (bxml, BROWSE_BUTTON_DIFF_DIALOG));
	g_signal_connect(G_OBJECT(button), "clicked", 
		G_CALLBACK(on_subversion_browse_button_clicked), diff_path_entry);

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
	subversion_show_diff (plugin->fm_current_filename, TRUE, FALSE, plugin);
}
