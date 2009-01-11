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

#include "subversion-log-dialog.h"

typedef struct
{
	glong revisions[2];
	gint pos;
} SelectedDiffData;

typedef struct
{
	GladeXML *gxml;
	Subversion *plugin;
	gchar *path;
	GtkListStore *log_store;
	GHashTable *selected_diff_revisions;
} LogData;

enum
{
	COL_DIFF_SELECTED,
	COL_AUTHOR,
	COL_DATE,
	COL_REVISION,
	COL_SHORT_LOG,
	COL_FULL_LOG,
	NUM_COLS
};

static void
on_diff_selected_column_toggled (GtkCellRendererToggle *renderer,
								 gchar *tree_path,
								 LogData *data)
{
	GtkTreeIter iter;
	glong revision;
	GtkWidget *log_diff_selected_button;
	gboolean selected;
	
	gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (data->log_store),
										&iter, tree_path);
	gtk_tree_model_get (GTK_TREE_MODEL (data->log_store), &iter,
						COL_DIFF_SELECTED, &selected,
						COL_REVISION, &revision,
						-1);
	
	log_diff_selected_button = glade_xml_get_widget (data->gxml,
													 "log_diff_selected_button");
		
	selected = !selected;
		
	if (selected)
	{
		/* Only allow two log items to be selected at once. */
		if (g_hash_table_size (data->selected_diff_revisions) < 2)
		{
			g_hash_table_insert (data->selected_diff_revisions, 
							 	 GINT_TO_POINTER (revision), NULL);
			
			gtk_list_store_set (data->log_store, &iter, 
								COL_DIFF_SELECTED, TRUE,
								-1);
		}
	}
	else
	{
		g_hash_table_remove (data->selected_diff_revisions, 
							 GINT_TO_POINTER (revision));
		
		gtk_list_store_set (data->log_store, &iter, 
							COL_DIFF_SELECTED, FALSE,
							-1);
	}
		
	/* For diff selected to work, exactly 2 revisions must be selected. */
	gtk_widget_set_sensitive (log_diff_selected_button,
							  g_hash_table_size (data->selected_diff_revisions) == 2);

}

static void
create_columns (LogData *data)
{
	GtkWidget *log_changes_view;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	
	log_changes_view = glade_xml_get_widget (data->gxml, "log_changes_view");
	
	/* Selected for diff  */
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_toggle_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_title (column, _("Diff"));
	gtk_tree_view_append_column (GTK_TREE_VIEW (log_changes_view), column);
	gtk_tree_view_column_add_attribute (column, renderer, "active", 
										COL_DIFF_SELECTED);
	
	g_signal_connect (G_OBJECT (renderer), "toggled", 
					  G_CALLBACK (on_diff_selected_column_toggled),
					  data);
	
	/* Author */
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_title (column, _("Author"));
	gtk_tree_view_append_column (GTK_TREE_VIEW (log_changes_view), column);
	gtk_tree_view_column_add_attribute (column, renderer, "text", 
										COL_AUTHOR);
	
	/* Date */
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_title (column, _("Date"));
	gtk_tree_view_append_column (GTK_TREE_VIEW (log_changes_view), column);
	gtk_tree_view_column_add_attribute (column, renderer, "text", 
										COL_DATE);
	
	
	/* Revision */
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_title (column, _("Revision"));
	gtk_tree_view_append_column (GTK_TREE_VIEW (log_changes_view), column);
	gtk_tree_view_column_add_attribute (column, renderer, "text", 
										COL_REVISION);
	
	/* Short log */
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_title (column, _("Short Log"));
	gtk_tree_view_append_column (GTK_TREE_VIEW (log_changes_view), column);
	gtk_tree_view_column_add_attribute (column, renderer, "text", 
										COL_SHORT_LOG);
	
}

static void
on_log_command_finished (AnjutaCommand *command, guint return_code, 
						 LogData *data)
{
	GtkWidget *log_changes_view;
	GQueue *log_queue;
	SvnLogEntry *log_entry;
	GtkTreeIter iter;
	gchar *author;
	gchar *date;
	glong revision;
	gchar *short_log;
	gchar *full_log;
		
	g_object_ref (data->log_store);
	log_changes_view = glade_xml_get_widget (data->gxml, "log_changes_view");
	gtk_tree_view_set_model (GTK_TREE_VIEW (log_changes_view), NULL);
	g_hash_table_remove_all (data->selected_diff_revisions);
	
	log_queue = svn_log_command_get_entry_queue (SVN_LOG_COMMAND (command));
	
	while (g_queue_peek_tail (log_queue))
	{
		log_entry = g_queue_pop_tail (log_queue);
		
		author = svn_log_entry_get_author (log_entry);
		date = svn_log_entry_get_date (log_entry);
		revision = svn_log_entry_get_revision (log_entry);
		short_log = svn_log_entry_get_short_log (log_entry);
		full_log = svn_log_entry_get_full_log (log_entry);
		
		gtk_list_store_prepend (data->log_store, &iter);
		gtk_list_store_set (data->log_store, &iter,
							COL_DIFF_SELECTED, FALSE,
							COL_AUTHOR, author,
							COL_DATE, date,
							COL_REVISION, revision,
							COL_SHORT_LOG, short_log,
							COL_FULL_LOG, full_log,
							-1);
		
		g_free (author);
		g_free (date);
		g_free (short_log);
		g_free (full_log);
		svn_log_entry_destroy (log_entry);
	}
	
	report_errors (command, return_code);
	
	svn_log_command_destroy (SVN_LOG_COMMAND (command));
	
	gtk_tree_view_set_model (GTK_TREE_VIEW (log_changes_view), 
							 GTK_TREE_MODEL (data->log_store));
	g_object_unref (data->log_store);
}

static void
on_log_view_button_clicked (GtkButton *button, LogData *data)
{
	GtkWidget *log_changes_view;
	GtkWidget *log_file_entry;
	GtkWidget *log_diff_previous_button;
	GtkWidget *log_diff_selected_button;
	GtkWidget *log_view_selected_button;
	const gchar *path;
	SvnLogCommand *log_command;
	guint pulse_timer_id;
	
	log_changes_view = glade_xml_get_widget (data->gxml, "log_changes_view");
	log_file_entry = glade_xml_get_widget (data->gxml, "log_file_entry");
	log_diff_previous_button = glade_xml_get_widget (data->gxml,
													 "log_diff_previous_button");
	log_diff_selected_button = glade_xml_get_widget (data->gxml,
													 "log_diff_selected_button");
	log_view_selected_button = glade_xml_get_widget (data->gxml,
													 "log_view_selected_button");
	path = gtk_entry_get_text (GTK_ENTRY (log_file_entry));
	
	if (data->path)
		g_free (data->path);
	
	data->path = g_strdup (path);
	
	if (strlen (path) > 0)
	{

		log_command = svn_log_command_new ((gchar *) path);
		
		pulse_timer_id = status_bar_progress_pulse (data->plugin,
													  _("Subversion: Retrieving"
														" log..."));
		
		g_signal_connect (G_OBJECT (log_command), "command-finished",
						  G_CALLBACK (stop_status_bar_progress_pulse),
						  GUINT_TO_POINTER (pulse_timer_id)); 
		
		g_signal_connect (G_OBJECT (log_command), "command-finished",
						  G_CALLBACK (on_log_command_finished),
						  data);
		
		anjuta_command_start (ANJUTA_COMMAND (log_command));
	}
	
	gtk_widget_set_sensitive (log_diff_previous_button, FALSE);
	gtk_widget_set_sensitive (log_diff_selected_button, FALSE);
	gtk_widget_set_sensitive (log_view_selected_button, FALSE);
	
	gtk_list_store_clear (data->log_store);
}

static void
on_cat_command_data_arrived (AnjutaCommand *command, IAnjutaEditor *editor)
{
	GQueue *output;
	gchar *line;
	
	output = svn_cat_command_get_output (SVN_CAT_COMMAND (command));
	
	while (g_queue_peek_head (output))
	{
		line = g_queue_pop_head (output);
		ianjuta_editor_append (editor, line, strlen (line), NULL);
		g_free (line);
	}
}

static void
on_cat_command_finished (AnjutaCommand *command, guint return_code,
						 Subversion *plugin)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
									  NULL);
	
	anjuta_status (status, _("Subversion: File retrieved."), 5);
	
	report_errors (command, return_code);
	
	svn_cat_command_destroy (SVN_CAT_COMMAND (command));	
}

static void
on_log_view_selected_button_clicked (GtkButton *button, LogData *data)
{
	GtkWidget *log_changes_view;
	GtkTreeModel *log_store;
	GtkTreeSelection *selection;
	GtkTreeIter selected_iter;
	glong revision;
	SvnCatCommand *cat_command;;
	gchar *filename;
	gchar *editor_name;
	IAnjutaDocumentManager *docman;
	IAnjutaEditor *editor;
	guint pulse_timer_id;
	
	log_changes_view = glade_xml_get_widget (data->gxml, "log_changes_view");
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (log_changes_view));
	gtk_tree_selection_get_selected (selection, &log_store, &selected_iter);
	
	gtk_tree_model_get (log_store, &selected_iter,
						COL_REVISION, &revision,
						-1);
	
	cat_command = svn_cat_command_new (data->path, revision);
	
	filename = get_filename_from_full_path (data->path);
	docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (data->plugin)->shell,
	                                     	 IAnjutaDocumentManager, NULL);
	editor_name = g_strdup_printf ("[Revision %ld] %s", revision, filename);
	editor = ianjuta_document_manager_add_buffer(docman, editor_name, "", 
												 NULL);
	g_free (filename);
	g_free (editor_name);
	
	pulse_timer_id = status_bar_progress_pulse (data->plugin,
												_("Subversion: Retrieving "
												  "file..."));
	
	g_signal_connect (G_OBJECT (cat_command), "command-finished",
					  G_CALLBACK (stop_status_bar_progress_pulse),
					  GUINT_TO_POINTER (pulse_timer_id));
	
	g_signal_connect (G_OBJECT (cat_command), "data-arrived",
					  G_CALLBACK (on_cat_command_data_arrived),
					  editor);
	
	g_signal_connect (G_OBJECT (cat_command), "command-finished",
					  G_CALLBACK (on_cat_command_finished),
					  data->plugin);
	
	g_object_weak_ref (G_OBJECT (editor), 
					   (GWeakNotify) disconnect_data_arrived_signals,
					   cat_command);
	
	anjuta_command_start (ANJUTA_COMMAND (cat_command));
}

static void
get_selected_revisions (gpointer revision, gpointer value, 
						SelectedDiffData *data)
{
	data->revisions[data->pos] = GPOINTER_TO_INT (revision);
	data->pos++;
}

static void
on_log_diff_selected_button_clicked (GtkButton *button, LogData *data)
{
	SelectedDiffData *selected_diff_data;
	SvnDiffCommand *diff_command;
	glong revision1;
	glong revision2;
	IAnjutaDocumentManager *docman;
	gchar *filename;
	gchar *editor_name;
	IAnjutaEditor *editor;
	guint pulse_timer_id;
	
	if (g_hash_table_size (data->selected_diff_revisions) == 2)
	{
		selected_diff_data = g_new0 (SelectedDiffData, 1);
		
		g_hash_table_foreach (data->selected_diff_revisions, 
							  (GHFunc) get_selected_revisions, 
							  selected_diff_data);
		
		revision1 = MIN (selected_diff_data->revisions[0],
						 selected_diff_data->revisions[1]);
		revision2 = MAX (selected_diff_data->revisions[0],
						 selected_diff_data->revisions[1]);
		
		diff_command = svn_diff_command_new (data->path, 
											 revision1,
											 revision2,
											 data->plugin->project_root_dir,
											 TRUE);
		
		docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (data->plugin)->shell,
	                                     	 IAnjutaDocumentManager, NULL);
		filename = get_filename_from_full_path (data->path);
		editor_name = g_strdup_printf ("[Revisions %ld/%ld] %s.diff", 
									   revision1, revision2, filename);
		
		editor = ianjuta_document_manager_add_buffer(docman, editor_name, "", 
												 	 NULL);
		
		g_free (filename);
		g_free (editor_name);
		
		pulse_timer_id = status_bar_progress_pulse (data->plugin,
													_("Subversion: Retrieving " 
													  "diff..."));
		
		g_signal_connect (G_OBJECT (diff_command), "command-finished",
						  G_CALLBACK (stop_status_bar_progress_pulse),
						  GUINT_TO_POINTER (pulse_timer_id));
		
		g_signal_connect (G_OBJECT (diff_command), "data-arrived", 
					  	  G_CALLBACK (send_diff_command_output_to_editor),
					  	  editor);
	
		g_signal_connect (G_OBJECT (diff_command), "command-finished",
					  	  G_CALLBACK (on_diff_command_finished),
					  	  data->plugin);
		
		g_object_weak_ref (G_OBJECT (editor), 
						   (GWeakNotify) disconnect_data_arrived_signals,
						   diff_command);
	
		anjuta_command_start (ANJUTA_COMMAND (diff_command));
		
		g_free (selected_diff_data);

	}
}

static void
on_log_diff_previous_button_clicked (GtkButton *button, LogData *data)
{
	GtkWidget *log_changes_view;
	GtkTreeModel *log_store;
	GtkTreeSelection *selection;
	GtkTreeIter selected_iter;
	glong revision;
	SvnDiffCommand *diff_command;
	IAnjutaDocumentManager *docman;
	gchar *filename;
	gchar *editor_name;
	IAnjutaEditor *editor;
	guint pulse_timer_id;
	
	log_changes_view = glade_xml_get_widget (data->gxml, "log_changes_view");
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (log_changes_view));
	gtk_tree_selection_get_selected (selection, &log_store, &selected_iter);
	
	gtk_tree_model_get (log_store, &selected_iter,
						COL_REVISION, &revision,
						-1);
	
	diff_command = svn_diff_command_new (data->path,
										 SVN_DIFF_REVISION_PREVIOUS,
										 revision,
										 data->plugin->project_root_dir,
										 TRUE);
	
	docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (data->plugin)->shell,
	                                     IAnjutaDocumentManager, NULL);
	filename = get_filename_from_full_path (data->path);
	editor_name = g_strdup_printf ("[Revisions %ld/%ld] %s.diff", 
								   revision - 1, revision, filename);
	editor = ianjuta_document_manager_add_buffer(docman, editor_name, "", 
												 NULL);
	
	g_free (filename);
	g_free (editor_name);
	
	pulse_timer_id = status_bar_progress_pulse (data->plugin,
												_("Subversion: Retrieving " 
												  "diff..."));
	
	g_signal_connect (G_OBJECT (diff_command), "command-finished",
					  G_CALLBACK (stop_status_bar_progress_pulse),
					  GUINT_TO_POINTER (pulse_timer_id));
	
	g_signal_connect (G_OBJECT (diff_command), "data-arrived", 
					  G_CALLBACK (send_diff_command_output_to_editor),
					  editor);
	
	g_signal_connect (G_OBJECT (diff_command), "command-finished",
					  G_CALLBACK (on_diff_command_finished),
					  data->plugin);
	
	g_object_weak_ref (G_OBJECT (editor), 
					   (GWeakNotify) disconnect_data_arrived_signals,
					   diff_command);
	
	anjuta_command_start (ANJUTA_COMMAND (diff_command));
}

static void
on_subversion_log_vbox_destroy (GtkObject *subversion_log, LogData *data)
{
	g_hash_table_destroy (data->selected_diff_revisions);
	g_free (data->path);
	g_free (data);
}

static gboolean
on_log_changes_view_row_selected (GtkTreeSelection *selection, 
								  GtkTreeModel *model,
						  		  GtkTreePath *path, 
							  	  gboolean path_currently_selected,
							  	  LogData *data)
{
	GtkTreeIter iter;
	GtkWidget *log_message_text;
	GtkWidget *log_diff_previous_button;
	GtkWidget *log_view_selected_button;
	GtkTextBuffer *buffer;
	gchar *log_message;
	
	gtk_tree_model_get_iter (model, &iter, path);
	log_message_text = glade_xml_get_widget (data->gxml, "log_message_text");
	log_diff_previous_button = glade_xml_get_widget (data->gxml, 
													 "log_diff_previous_button");
	log_view_selected_button = glade_xml_get_widget (data->gxml,
													 "log_view_selected_button");
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (log_message_text));
	gtk_tree_model_get (model, &iter, 
						COL_FULL_LOG, &log_message,
						-1);
	
	gtk_text_buffer_set_text (buffer, log_message, strlen (log_message));
	gtk_widget_set_sensitive (log_diff_previous_button, 
							  TRUE);
	if (data->path)
	{
		gtk_widget_set_sensitive (log_view_selected_button,
								  !g_file_test (data->path, 
												G_FILE_TEST_IS_DIR));
	}
	
	return TRUE;
}

GtkWidget *
subversion_log_window_create (Subversion *plugin)
{
	LogData *data;
	GtkWidget *subversion_log;
	GtkWidget *subversion_log_vbox;
	GtkWidget *log_changes_view;
	GtkWidget *log_whole_project_check;
	GtkWidget *log_file_entry;
	GtkWidget *log_view_button;
	GtkWidget *log_diff_previous_button;
	GtkWidget *log_diff_selected_button;
	GtkWidget *log_view_selected_button;
	GtkListStore *log_list_store;
	GtkTreeSelection *selection;
	
	data = g_new0 (LogData, 1);
	data->gxml = plugin->log_gxml;
	data->selected_diff_revisions = g_hash_table_new (g_direct_hash, 
													  g_direct_equal);
	data->plugin = plugin;
	data->path = NULL;
	
	subversion_log = glade_xml_get_widget (plugin->log_gxml, "subversion_log");
	subversion_log_vbox = glade_xml_get_widget (plugin->log_gxml, "subversion_log_vbox");
	log_changes_view = glade_xml_get_widget (plugin->log_gxml, "log_changes_view");
	log_whole_project_check = glade_xml_get_widget (plugin->log_gxml, 
													"log_whole_project_check");
	log_file_entry = glade_xml_get_widget (plugin->log_gxml, "log_file_entry");
	log_view_button = glade_xml_get_widget (plugin->log_gxml, "log_view_button");
	log_diff_previous_button = glade_xml_get_widget (plugin->log_gxml, 
													 "log_diff_previous_button");
	log_diff_selected_button = glade_xml_get_widget (plugin->log_gxml,
													 "log_diff_selected_button");
	log_view_selected_button = glade_xml_get_widget (plugin->log_gxml,
													 "log_view_selected_button");
	
	g_signal_connect (G_OBJECT (log_view_button), "clicked",
					  G_CALLBACK (on_log_view_button_clicked),
					  data);
	
	g_signal_connect (G_OBJECT (log_diff_previous_button), "clicked",
					  G_CALLBACK (on_log_diff_previous_button_clicked),
					  data);
	
	g_signal_connect (G_OBJECT (log_diff_selected_button), "clicked",
					  G_CALLBACK (on_log_diff_selected_button_clicked),
					  data);
	
	g_signal_connect (G_OBJECT (log_view_selected_button), "clicked",
					  G_CALLBACK (on_log_view_selected_button_clicked),
					  data);
	
	
	g_object_set_data (G_OBJECT (log_whole_project_check), "fileentry",
					   log_file_entry);
	g_signal_connect (G_OBJECT (log_whole_project_check), "toggled",
					  G_CALLBACK (on_whole_project_toggled), plugin);
	init_whole_project (plugin, log_whole_project_check, FALSE);
	
	log_list_store = gtk_list_store_new (NUM_COLS,
										 G_TYPE_BOOLEAN,
										 G_TYPE_STRING,
										 G_TYPE_STRING,
										 G_TYPE_LONG,
										 G_TYPE_STRING,
										 G_TYPE_STRING);
	create_columns (data);
	gtk_tree_view_set_model (GTK_TREE_VIEW (log_changes_view),
							 GTK_TREE_MODEL (log_list_store));
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (log_changes_view));
	gtk_tree_selection_set_select_function (selection, 
											(GtkTreeSelectionFunc) on_log_changes_view_row_selected,
											data, NULL);
	
	data->log_store = log_list_store;
	g_object_unref (log_list_store);
	
	g_signal_connect (G_OBJECT (subversion_log_vbox), "destroy",
					  G_CALLBACK (on_subversion_log_vbox_destroy), 
					  data);
	
	g_object_ref (subversion_log_vbox);
	gtk_container_remove (GTK_CONTAINER (subversion_log), subversion_log_vbox);
	gtk_widget_destroy (subversion_log);
	
	return subversion_log_vbox;
}

void
on_menu_subversion_log (GtkAction *action, Subversion *plugin)
{
	anjuta_shell_present_widget (ANJUTA_PLUGIN (plugin)->shell, 
								 plugin->log_viewer, NULL);
}

void
on_fm_subversion_log (GtkAction *action, Subversion *plugin)
{
	GtkWidget *log_file_entry;
	
	log_file_entry = glade_xml_get_widget (plugin->log_gxml, "log_file_entry");
	
	gtk_entry_set_text (GTK_ENTRY (log_file_entry), 
						plugin->fm_current_filename);
	
	anjuta_shell_present_widget (ANJUTA_PLUGIN (plugin)->shell, 
								 plugin->log_viewer, NULL);
}

void
subversion_log_set_whole_project_sensitive (GladeXML *log_gxml, 
											gboolean sensitive)
{
	GtkWidget *log_whole_project_check;
	
	log_whole_project_check = glade_xml_get_widget (log_gxml,
													"log_whole_project_check");
	
	gtk_widget_set_sensitive (log_whole_project_check, sensitive);
	
	if (!sensitive)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (log_whole_project_check),
									  FALSE);
	}
}
