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

#include "git-log-dialog.h"
#include "git-cat-file-menu.h"

enum
{
	COL_REVISION,
	
	NUM_COLS
};

typedef struct
{
	Git *plugin;
	GtkBuilder *bxml;
	GtkListStore *list_store;
	GtkCellRenderer *graph_renderer;
	gchar *path;
	GHashTable *refs;
	GHashTable *filters;
} LogData;

static void
author_cell_function (GtkTreeViewColumn *column, GtkCellRenderer *renderer,
					  GtkTreeModel *model, GtkTreeIter *iter, 
					  gpointer user_data)
{
	GitRevision *revision;
	gchar *author;
	
	gtk_tree_model_get (model, iter, COL_REVISION, &revision, -1);
	author = git_revision_get_author (revision);
	
	g_object_unref (revision);
	
	g_object_set (renderer, "text", author, NULL);
	
	g_free (author);
}

static void
date_cell_function (GtkTreeViewColumn *column, GtkCellRenderer *renderer,
					GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
	GitRevision *revision;
	gchar *date;
	
	gtk_tree_model_get (model, iter, COL_REVISION, &revision, -1);
	date = git_revision_get_formatted_date (revision);
	
	g_object_unref (revision);
	
	g_object_set (renderer, "text", date, NULL);
	
	g_free (date);
}

static void
short_log_cell_function (GtkTreeViewColumn *column, GtkCellRenderer *renderer,
						 GtkTreeModel *model, GtkTreeIter *iter, 
						 gpointer user_data)
{
	GitRevision *revision;
	gchar *short_log;
	
	gtk_tree_model_get (model, iter, COL_REVISION, &revision, -1);
	short_log = git_revision_get_short_log (revision);
	
	g_object_unref (revision);
	
	g_object_set (renderer, "text", short_log, NULL);
	
	g_free (short_log);
}

static void
ref_icon_cell_function (GtkTreeViewColumn *column, GtkCellRenderer *renderer,
						GtkTreeModel *model, GtkTreeIter *iter, 
						LogData *data)
{
	GitRevision *revision;
	gchar *sha;
	
	gtk_tree_model_get (model, iter, COL_REVISION, &revision, -1);
	sha = git_revision_get_sha (revision);
	
	g_object_unref (revision);
	
	if (g_hash_table_lookup_extended (data->refs, sha, NULL, NULL))
		g_object_set (renderer, "stock-id", GTK_STOCK_INFO, NULL);
	else
		g_object_set (renderer, "stock-id", "", NULL);
	
	g_free (sha);
}

static void
create_columns (LogData *data)
{
	GtkWidget *log_changes_view;
	gint font_size;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	
	log_changes_view = GTK_WIDGET (gtk_builder_get_object (data->bxml, "log_changes_view"));
	font_size = PANGO_PIXELS (pango_font_description_get_size (GTK_WIDGET (log_changes_view)->style->font_desc));
	
	/* Ref info */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_append_column (GTK_TREE_VIEW (log_changes_view), column);
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column, renderer,
											 (GtkTreeCellDataFunc) ref_icon_cell_function,
											 data, NULL);
	
	
	
	/* Graph */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_append_column (GTK_TREE_VIEW (log_changes_view), column);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_min_width (column, font_size * 10);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_pack_start (column, data->graph_renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, data->graph_renderer, 
										"revision", COL_REVISION);
	gtk_tree_view_column_set_title (column, _("Graph"));
	
	/* Short log */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_append_column (GTK_TREE_VIEW (log_changes_view), column);
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_min_width (column, font_size * 10);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column, renderer,
											 (GtkTreeCellDataFunc) short_log_cell_function,
											 NULL, NULL);
	gtk_tree_view_column_set_title (column, _("Short log"));
	
	/* Author */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_append_column (GTK_TREE_VIEW (log_changes_view), column);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column, renderer,
											 (GtkTreeCellDataFunc) author_cell_function,
											 NULL, NULL);
	gtk_tree_view_column_set_title (column, _("Author"));
	
	/* Date */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_append_column (GTK_TREE_VIEW (log_changes_view), column);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column, renderer,
											 (GtkTreeCellDataFunc) date_cell_function,
											 NULL, NULL);
	gtk_tree_view_column_set_title (column, _("Date"));
	
	gtk_tree_view_set_model (GTK_TREE_VIEW (log_changes_view),
							 GTK_TREE_MODEL (data->list_store));
	g_object_unref (data->list_store);
	
}

static void
on_log_command_finished (AnjutaCommand *command, guint return_code, 
						 LogData *data)
{
	GtkWidget *log_changes_view;
	GQueue *queue;
	GtkTreeIter iter;
	GitRevision *revision;
	
	if (return_code != 0)
	{
		git_report_errors (command, return_code);
		g_object_unref (command);
		
		return;
	}
	
	log_changes_view = GTK_WIDGET (gtk_builder_get_object (data->bxml, "log_changes_view"));
	
	g_object_ref (data->list_store);
	gtk_tree_view_set_model (GTK_TREE_VIEW (log_changes_view), NULL);
	
	queue = git_log_command_get_output_queue (GIT_LOG_COMMAND (command));
	
	while (g_queue_peek_head (queue))
	{
		revision = g_queue_pop_head (queue);
		
		gtk_list_store_append (data->list_store, &iter);
		gtk_list_store_set (data->list_store, &iter, COL_REVISION, revision, -1);
		g_object_unref (revision);
	}
	
	giggle_graph_renderer_validate_model (GIGGLE_GRAPH_RENDERER (data->graph_renderer),
										  GTK_TREE_MODEL (data->list_store),
										  COL_REVISION);
	gtk_tree_view_set_model (GTK_TREE_VIEW (log_changes_view), 
							 GTK_TREE_MODEL (data->list_store));
	g_object_unref (data->list_store);
	
	g_object_unref (command);
}

static void
on_ref_command_finished (AnjutaCommand *command, guint return_code, 
						 LogData *data)
{
	gchar *path;
	const gchar *relative_path;
	GtkWidget *log_changes_view;
	GtkTreeViewColumn *graph_column;
	gchar *author;
	gchar *grep;
	gchar *since_date;
	gchar *until_date;
	gchar *since_commit;
	gchar *until_commit;
	GitLogCommand *log_command;
	gint pulse_timer_id;
	
	path = g_object_get_data (G_OBJECT (command), "path");
	relative_path = NULL;
	
	if (return_code != 0)
	{
		git_report_errors (command, return_code);
		g_object_unref (command);
		
		return;
	}
	
	if (path)
	{
		relative_path = git_get_relative_path (path, 
											   data->plugin->project_root_directory);
	}
	
	/* If the user is using any filters or getting the log of an individual,
	 * file or folder, hide the graph column, because we can't be assured that  
	 * the graph will be correct in these cases */
	log_changes_view = GTK_WIDGET (gtk_builder_get_object (data->bxml, "log_changes_view"));
	graph_column = gtk_tree_view_get_column (GTK_TREE_VIEW (log_changes_view),
											 1);
	
	if (g_hash_table_size (data->filters) > 0 || path)
		gtk_tree_view_column_set_visible (graph_column, FALSE);
	else
		gtk_tree_view_column_set_visible (graph_column, TRUE);
	
	/* Get the filter data */
	author = g_hash_table_lookup (data->filters, "author");
	grep = g_hash_table_lookup (data->filters, "grep");
	since_date = g_hash_table_lookup (data->filters, "since-date");
	until_date = g_hash_table_lookup (data->filters, "until-date");
	since_commit = g_hash_table_lookup (data->filters, "since-commit");
	until_commit = g_hash_table_lookup (data->filters, "until-commit");
	
	if (data->refs)
		g_hash_table_unref (data->refs);
	
	data->refs = git_ref_command_get_refs (GIT_REF_COMMAND (command));
	log_command = git_log_command_new (data->plugin->project_root_directory,
									   relative_path,
									   author, grep, since_date, until_date,
									   since_commit, until_commit);
	
	gtk_list_store_clear (data->list_store);
	
	pulse_timer_id = git_status_bar_progress_pulse (data->plugin,
													_("Git: Retrieving"
													  " log..."));
	
	g_signal_connect (G_OBJECT (log_command), "command-finished",
					  G_CALLBACK (git_stop_status_bar_progress_pulse),
					  GUINT_TO_POINTER (pulse_timer_id)); 
	
	g_signal_connect (G_OBJECT (log_command), "command-finished",
					  G_CALLBACK (on_log_command_finished),
					  data);
	
	anjuta_command_start (ANJUTA_COMMAND (log_command));
	
	g_object_unref (command);
}

static void
on_log_view_button_clicked (GtkButton *button, LogData *data)
{
	gchar *path;
	AnjutaShell *shell;
	GtkWidget *log_whole_project_check;
	GtkWidget *log_path_entry;
	GitRefCommand *ref_command;
	
	path = NULL;
	
	log_whole_project_check = GTK_WIDGET (gtk_builder_get_object (data->bxml,
										  "log_whole_project_check"));
	
	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (log_whole_project_check)))
	{
		log_path_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
		                                                     "log_path_entry"));
		path = gtk_editable_get_chars (GTK_EDITABLE (log_path_entry), 0, -1);
		
		/* Log widget belongs to the shell at this point. */
		shell = ANJUTA_PLUGIN (data->plugin)->shell;
		
		if (!git_check_input (GTK_WIDGET (shell), log_path_entry, path, 
							  _("Please enter a path.")))
		{
			g_free (path);
			return;
		}
		
		/* Don't allow the user to try to view revisions of directories */
		git_cat_file_menu_set_sensitive (data->plugin, 
										 !g_file_test (path, 
													   G_FILE_TEST_IS_DIR));
	}
	else
	{
		/* Users can't get individual files if they're viewing the whole 
		 * project log. */
		git_cat_file_menu_set_sensitive (data->plugin, FALSE);
	}
	
	ref_command = git_ref_command_new (data->plugin->project_root_directory);
	
	g_signal_connect (G_OBJECT (ref_command), "command-finished",
					  G_CALLBACK (on_ref_command_finished),
					  data);
	
	
	/* Attach path to this command so it can be passed to the log command. */
	g_object_set_data_full (G_OBJECT (ref_command), "path", 
							g_strdup (path), g_free);
	
	g_free (path);
	
	anjuta_command_start (ANJUTA_COMMAND (ref_command));
}

static void
on_log_browse_button_clicked (GtkButton *button, LogData *data)
{
	GtkWidget *log_file_entry;
	GtkWidget *file_chooser_dialog;
	gchar *filename;
	gint response;

	log_file_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
	                                                     "log_path_entry"));

	file_chooser_dialog = gtk_file_chooser_dialog_new (_("Select a file"),
	                                                   NULL,
	                                                   GTK_FILE_CHOOSER_ACTION_OPEN,
				  									   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				  									   GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				  									   NULL);

	response = gtk_dialog_run (GTK_DIALOG (file_chooser_dialog));

	if (response == GTK_RESPONSE_ACCEPT)
	{
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_chooser_dialog));
		gtk_entry_set_text (GTK_ENTRY (log_file_entry), filename);

		g_free (filename);
	}

	gtk_widget_destroy (file_chooser_dialog);
}

static void
on_log_vbox_destroy (GtkObject *log_vbox, LogData *data)
{
	g_free (data->path);
	g_object_unref (data->bxml);
	
	if (data->refs)
		g_hash_table_unref (data->refs);
	
	g_hash_table_destroy (data->filters);
	
	g_free (data);
}

static void
on_log_message_command_finished (AnjutaCommand *command, guint return_code,
								 LogData *data)
{
	GtkWidget *log_text_view;
	GtkTextBuffer *buffer;
	gchar *log_message;
	
	log_text_view = GTK_WIDGET (gtk_builder_get_object (data->bxml, "log_text_view"));
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (log_text_view));
	log_message = git_log_message_command_get_message (GIT_LOG_MESSAGE_COMMAND (command));
	
	gtk_text_buffer_set_text (buffer, log_message, strlen (log_message));
	
	g_free (log_message);
	g_object_unref (command);
}

static gboolean
on_log_changes_view_row_selected (GtkTreeSelection *selection, 
								  GtkTreeModel *model,
						  		  GtkTreePath *path, 
							  	  gboolean path_currently_selected,
							  	  LogData *data)
{
	GtkTreeIter iter;
	GitRevision *revision;
	gchar *sha;
	GitLogMessageCommand *log_message_command;
	
	if (!path_currently_selected)
	{
		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_model_get (model, &iter, COL_REVISION, &revision, -1);
		sha = git_revision_get_sha (revision);
		
		log_message_command = git_log_message_command_new (data->plugin->project_root_directory,
														   sha);
		
		g_free (sha);
		g_object_unref (revision);
		
		g_signal_connect (G_OBJECT (log_message_command), "command-finished",
						  G_CALLBACK (on_log_message_command_finished),
						  data);
		
		anjuta_command_start (ANJUTA_COMMAND (log_message_command));
	}
	
	return TRUE;
}

static gboolean
on_log_changes_view_query_tooltip (GtkWidget *log_changes_view, gint x, gint y,
								   gboolean keyboard_mode, GtkTooltip *tooltip,
								   LogData *data)
{
	gboolean ret;
	GtkTreeViewColumn *ref_column;
	gint bin_x;
	gint bin_y;
	GtkTreeViewColumn *current_column;
	GtkTreePath *path;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GitRevision *revision;
	gchar *sha;
	GList *ref_list;
	GList *current_ref;
	GString *tooltip_string;
	gchar *ref_name;
	GitRefType ref_type;
	
	ret = FALSE;
	
	ref_column = gtk_tree_view_get_column (GTK_TREE_VIEW (log_changes_view), 0);
	
	gtk_tree_view_convert_widget_to_bin_window_coords (GTK_TREE_VIEW (log_changes_view),
													   x, y, &bin_x, &bin_y);
	gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (log_changes_view), bin_x, 
								   bin_y, &path, &current_column, NULL, NULL);
	
	/* We need to be in the ref icon column */
	if (current_column == ref_column)
	{
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (log_changes_view));
		gtk_tree_model_get_iter (model, &iter, path);
		
		gtk_tree_model_get (model, &iter, COL_REVISION, &revision, -1);
		sha = git_revision_get_sha (revision);
		
		g_object_unref (revision);
		
		ref_list = g_hash_table_lookup (data->refs, sha);
		g_free (sha);
		
		if (ref_list)
		{
			current_ref = ref_list;
			tooltip_string = g_string_new ("");
			
			while (current_ref)
			{
				ref_name = git_ref_get_name (GIT_REF (current_ref->data));
				ref_type = git_ref_get_ref_type (GIT_REF (current_ref->data));
				
				if (tooltip_string->len > 0)
					g_string_append (tooltip_string, "\n");
				
				switch (ref_type)
				{
					case GIT_REF_TYPE_BRANCH:
						g_string_append_printf (tooltip_string,
												_("<b>Branch:</b> %s"),
												ref_name);
						break;
					case GIT_REF_TYPE_TAG:
						g_string_append_printf (tooltip_string,
												_("<b>Tag:</b> %s"),
												ref_name);
						break;
					case GIT_REF_TYPE_REMOTE:
						g_string_append_printf (tooltip_string,
												_("<b>Remote:</b> %s"),
												ref_name);
						break;
					default:
						break;
				}
				
				g_free (ref_name);
				current_ref = g_list_next (current_ref);
			}
			
			gtk_tooltip_set_markup (tooltip, tooltip_string->str);
			g_string_free (tooltip_string, TRUE);
			
			ret = TRUE;
		}
	}
	
	gtk_tree_path_free (path);
	return ret;
	
	
}

static gboolean
on_log_changes_view_button_press_event (GtkWidget *log_changes_view, 
										GdkEventButton *event,
										Git *plugin)
{
	GtkTreeSelection *selection;
	
	if (event->type == GDK_BUTTON_PRESS)
	{
		if (event->button == 3)
		{
			selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (log_changes_view));
			
			if (gtk_tree_selection_count_selected_rows (selection) > 0)
			{
				gtk_menu_popup (GTK_MENU(plugin->log_popup_menu), NULL, NULL,  
								NULL, NULL,  event->button, event->time);
			}
		}
	}
	return FALSE;
}

static void
on_log_filter_entry_changed (GtkEditable *editable, LogData *data)
{
	gchar *filter_name;
	gchar *text;
	
	filter_name = g_object_get_data (G_OBJECT (editable), "filter-name");
	text = gtk_editable_get_chars (editable, 0, -1);
	
	if (strlen (text) > 0)
		g_hash_table_insert (data->filters, filter_name, g_strdup (text));
	else
		g_hash_table_remove (data->filters, filter_name);
	
	g_free (text);
}

static void
on_log_filter_date_changed (GtkCalendar *calendar, LogData *data)
{
	gchar *filter_name;
	guint year;
	guint month;
	guint day;
	gchar *date;
	
	filter_name = g_object_get_data (G_OBJECT (calendar), "filter-name");
	
	gtk_calendar_get_date (calendar, &year, &month, &day);
	date = g_strdup_printf ("%i-%02i-%02i", year, (month + 1), day);
	g_hash_table_insert (data->filters, filter_name, g_strdup (date));
	
	g_free (date);	
}

static void
on_log_filter_date_check_toggled (GtkToggleButton *toggle_button, LogData *data)
{
	GtkCalendar *calendar;
	gchar *filter_name;
	
	calendar = g_object_get_data (G_OBJECT (toggle_button), "calendar");
	
	if (gtk_toggle_button_get_active (toggle_button))
	{
		gtk_widget_set_sensitive (GTK_WIDGET (calendar), TRUE);
		
		/* Treat the currently selected date as a date to filter on */
		on_log_filter_date_changed (calendar, data);
	}
	else
	{
		filter_name = g_object_get_data (G_OBJECT (calendar), "filter-name");
		
		gtk_widget_set_sensitive (GTK_WIDGET (calendar), FALSE);
		g_hash_table_remove (data->filters, filter_name);
	}
}

static void
on_log_filter_clear_button_clicked (GtkButton *button, LogData *data)
{
	GtkWidget *log_filter_author_entry;
	GtkWidget *log_filter_grep_entry;
	GtkWidget *log_filter_from_check;
	GtkWidget *log_filter_to_check;
	GtkWidget *log_filter_from_entry;
	GtkWidget *log_filter_to_entry;
	
	log_filter_author_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																  "log_filter_author_entry"));
	log_filter_grep_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																"log_filter_grep_entry"));
	log_filter_from_check = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																"log_filter_from_check"));
	log_filter_to_check = GTK_WIDGET (gtk_builder_get_object (data->bxml,
															  "log_filter_to_check"));
	log_filter_from_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																"log_filter_from_entry"));
	log_filter_to_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml,
															  "log_filter_to_entry"));
	
	gtk_entry_set_text (GTK_ENTRY (log_filter_author_entry), "");
	gtk_entry_set_text (GTK_ENTRY (log_filter_grep_entry), "");
	gtk_entry_set_text (GTK_ENTRY (log_filter_from_entry), "");
	gtk_entry_set_text (GTK_ENTRY (log_filter_from_entry), "");
	gtk_entry_set_text (GTK_ENTRY (log_filter_to_entry), "");
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (log_filter_from_check), 
								  FALSE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (log_filter_to_check), 
								  FALSE);
	
}

static void
setup_filters (LogData *data)
{
	GtkWidget *log_filter_author_entry;
	GtkWidget *log_filter_grep_entry;
	GtkWidget *log_filter_from_check;
	GtkWidget *log_filter_to_check;
	GtkWidget *log_filter_from_calendar;
	GtkWidget *log_filter_to_calendar;
	GtkWidget *log_filter_from_entry;
	GtkWidget *log_filter_to_entry;
	GtkWidget *log_filter_clear_button;
	
	data->filters = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, 
										   g_free);
	
	log_filter_author_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																  "log_filter_author_entry"));
	log_filter_grep_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																"log_filter_grep_entry"));
	log_filter_from_check = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																"log_filter_from_check"));
	log_filter_to_check = GTK_WIDGET (gtk_builder_get_object (data->bxml,
															  "log_filter_to_check"));
	log_filter_from_calendar = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																   "log_filter_from_calendar"));
	log_filter_to_calendar = GTK_WIDGET (gtk_builder_get_object (data->bxml,
															     "log_filter_to_calendar"));
	log_filter_from_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																"log_filter_from_entry"));
	log_filter_to_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml,
															  "log_filter_to_entry"));
	log_filter_clear_button = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																  "log_filter_clear_button"));
	
	/* Each widget that has some kind of filter must have a "filter name"
	 * associated with it so that we can generically see how many filters 
	 * the user asked for, so we can hide the graph column, and to get this 
	 * information in a somewhat generic way. */
	g_object_set_data (G_OBJECT (log_filter_author_entry), "filter-name",
					   "author");
	g_object_set_data (G_OBJECT (log_filter_grep_entry), "filter-name",
					   "grep");
	g_object_set_data (G_OBJECT (log_filter_from_calendar), "filter-name",
					   "since-date");
	g_object_set_data (G_OBJECT (log_filter_to_calendar), "filter-name",
					   "until-date");
	g_object_set_data (G_OBJECT (log_filter_from_entry), "filter-name",
					   "since-commit");
	g_object_set_data (G_OBJECT (log_filter_to_entry), "filter-name",
					   "until-commit");
	
	g_object_set_data (G_OBJECT (log_filter_from_check), "calendar",
					   log_filter_from_calendar);
	g_object_set_data (G_OBJECT (log_filter_to_check), "calendar",
					   log_filter_to_calendar);
	
	/* Each widget should have one generic handler that inserts its changes
	 * into the filter hash table as needed. */
	g_signal_connect (G_OBJECT (log_filter_author_entry), "changed",
					  G_CALLBACK (on_log_filter_entry_changed),
					  data);
	
	g_signal_connect (G_OBJECT (log_filter_grep_entry), "changed",
					  G_CALLBACK (on_log_filter_entry_changed),
					  data);
	
	g_signal_connect (G_OBJECT (log_filter_from_entry), "changed",
					  G_CALLBACK (on_log_filter_entry_changed),
					  data);
	
	g_signal_connect (G_OBJECT (log_filter_to_entry), "changed",
					  G_CALLBACK (on_log_filter_entry_changed),
					  data);
	
	/* Calendars don't have one catch all signal that handles both month and 
	 * day changes, so we have to connect to both signals. */
	g_signal_connect (G_OBJECT (log_filter_from_calendar), "day-selected",
					  G_CALLBACK (on_log_filter_date_changed),
					  data);
	
	g_signal_connect (G_OBJECT (log_filter_from_calendar), "month-changed",
					  G_CALLBACK (on_log_filter_date_changed),
					  data);
	
	g_signal_connect (G_OBJECT (log_filter_to_calendar), "day-selected",
					  G_CALLBACK (on_log_filter_date_changed),
					  data);
	
	g_signal_connect (G_OBJECT (log_filter_to_calendar), "month-changed",
					  G_CALLBACK (on_log_filter_date_changed),
					  data);
	
	g_signal_connect (G_OBJECT (log_filter_from_check), "toggled",
					  G_CALLBACK (on_log_filter_date_check_toggled),
					  data);
	
	g_signal_connect (G_OBJECT (log_filter_to_check), "toggled",
					  G_CALLBACK (on_log_filter_date_check_toggled),
					  data);
	
	g_signal_connect (G_OBJECT (log_filter_clear_button), "clicked",
					  G_CALLBACK (on_log_filter_clear_button_clicked),
					  data);
}

GtkWidget *
git_log_window_create (Git *plugin)
{
	LogData *data;
	gchar *objects[] = {"log_window", NULL};
	GError *error;
	GtkWidget *log_window;
	GtkWidget *log_vbox;
	GtkWidget *log_changes_view;
	GtkWidget *log_view_button;
	GtkWidget *log_browse_button;
	GtkWidget *log_whole_project_check;
	GtkWidget *log_path_entry;
	GtkWidget *log_path_entry_hbox;
	GtkTreeSelection *selection;
	
	data = g_new0 (LogData, 1);
	data->bxml = gtk_builder_new ();
	error = NULL;

	if (!gtk_builder_add_objects_from_file (data->bxml, BUILDER_FILE, objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}
	
	data->plugin = plugin;
	data->path = NULL;
	data->graph_renderer = giggle_graph_renderer_new ();
	
	log_window = GTK_WIDGET (gtk_builder_get_object (data->bxml, "log_window"));
	log_vbox = GTK_WIDGET (gtk_builder_get_object (data->bxml, "log_vbox"));
	log_changes_view = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
	                                                       "log_changes_view"));
	log_view_button = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
														  "log_view_button"));
	log_browse_button = GTK_WIDGET (gtk_builder_get_object (data->bxml,
	                                                        "log_browse_button"));
	log_whole_project_check = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
																  "log_whole_project_check"));
	log_path_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
	                                                     "log_path_entry"));
	log_path_entry_hbox = GTK_WIDGET (gtk_builder_get_object (data->bxml,
	                                                          "log_path_entry_hbox"));
	
	g_object_set_data (G_OBJECT (log_vbox), "log-data", data);
	
	setup_filters (data);
	
	g_signal_connect (G_OBJECT (log_changes_view), "query-tooltip",
					  G_CALLBACK (on_log_changes_view_query_tooltip),
					  data);
	
	g_signal_connect (G_OBJECT (log_changes_view), "button-press-event",
					  G_CALLBACK (on_log_changes_view_button_press_event),
					  plugin);
	
	
	g_signal_connect (G_OBJECT (log_view_button), "clicked",
					  G_CALLBACK (on_log_view_button_clicked),
					  data);

	g_signal_connect (G_OBJECT (log_browse_button), "clicked",
	                  G_CALLBACK (on_log_browse_button_clicked),
	                  data);
	
	g_object_set_data (G_OBJECT (log_whole_project_check), "file-entry",
					   log_path_entry_hbox);
	                  
	g_signal_connect (G_OBJECT (log_whole_project_check), "toggled",
					  G_CALLBACK (on_git_whole_project_toggled), plugin);
	
	data->list_store = gtk_list_store_new (NUM_COLS,
										   G_TYPE_OBJECT);
	create_columns (data);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (log_changes_view));
	gtk_tree_selection_set_select_function (selection, 
											(GtkTreeSelectionFunc) on_log_changes_view_row_selected,
											data, NULL);
	
	git_cat_file_menu_set_sensitive (plugin, FALSE);
	
	g_signal_connect (G_OBJECT (log_vbox), "destroy",
					  G_CALLBACK (on_log_vbox_destroy), 
					  data);
	
	g_object_ref (log_vbox);
	gtk_container_remove (GTK_CONTAINER (log_window), log_vbox);
	gtk_widget_destroy (log_window);
	
	return log_vbox;
}

void
on_menu_git_log (GtkAction *action, Git *plugin)
{
	anjuta_shell_present_widget (ANJUTA_PLUGIN (plugin)->shell, 
								 plugin->log_viewer, NULL);
}

void
on_fm_git_log (GtkAction *action, Git *plugin)
{
	LogData *data;
	GtkWidget *log_path_entry;
	GtkWidget *log_whole_project_check;
	
	data = g_object_get_data (G_OBJECT (plugin->log_viewer), "log-data");
	log_path_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
														 "log_path_entry"));
	log_whole_project_check = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																  "log_whole_project_check"));
	
	gtk_entry_set_text (GTK_ENTRY (log_path_entry), 
						plugin->current_fm_filename);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (log_whole_project_check),
								  FALSE);
	
	anjuta_shell_present_widget (ANJUTA_PLUGIN (plugin)->shell, 
								 plugin->log_viewer, NULL);
}

void
git_log_window_clear (Git *plugin)
{
	LogData *data;
	GtkWidget *log_text_view;
	GtkTextBuffer *buffer;
	
	data = g_object_get_data (G_OBJECT (plugin->log_viewer), "log-data");
	log_text_view = GTK_WIDGET (gtk_builder_get_object (data->bxml, "log_text_view"));
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (log_text_view));
	
	gtk_list_store_clear (data->list_store);
	gtk_text_buffer_set_text (buffer, "", 0);
}

GitRevision *
git_log_get_selected_revision (Git *plugin)
{
	LogData *data;
	GtkWidget *log_changes_view;
	GitRevision *revision;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	data = g_object_get_data (G_OBJECT (plugin->log_viewer), "log-data");
	log_changes_view = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
	                                                       "log_changes_view"));
	revision = NULL;
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (log_changes_view));
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (log_changes_view));
	
	gtk_tree_selection_get_selected (selection, NULL, &iter);
	gtk_tree_model_get (model, &iter, COL_REVISION, &revision, -1);
	
	return revision;
}

gchar *
git_log_get_path (Git *plugin)
{
	LogData *data;
	GtkWidget *log_path_entry;
	
	data = g_object_get_data (G_OBJECT (plugin->log_viewer), "log-data");
	log_path_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
	                                                     "log_path_entry"));
	
	return gtk_editable_get_chars (GTK_EDITABLE (log_path_entry), 0, -1);
}
