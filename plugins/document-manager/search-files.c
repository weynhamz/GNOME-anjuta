/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Johannes Schmid 2012 <jhs@gnome.org>
 *
 * anjuta is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "search-files.h"
#include "search-file-command.h"
#include "search-filter-file-command.h"
#include <libanjuta/anjuta-command-queue.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/interfaces/ianjuta-project-chooser.h>
#include <libanjuta/interfaces/ianjuta-language.h>

#define BUILDER_FILE PACKAGE_DATA_DIR"/glade/anjuta-document-manager.ui"

#define TEXT_MIME_TYPE "text/*"

struct _SearchFilesPrivate
{
	GtkBuilder* builder;

	GtkWidget* main_box;

	GtkWidget* search_button;
	GtkWidget* replace_button;

	GtkWidget* search_entry;
	GtkWidget* replace_entry;

	GtkWidget* files_combo;
	GtkWidget* project_combo;
	GtkWidget* file_type_combo;

	GtkWidget* case_check;
	GtkWidget* regex_check;

	GtkWidget* spinner_busy;

	GtkWidget* files_tree;
	GtkTreeModel* files_model;
	GtkWidget* scrolled_window;

	GtkWidget* files_tree_check;

	AnjutaDocman* docman;
	SearchBox* search_box;
	GtkWidget* dialog;

	/* Saved from last search */
	gboolean case_sensitive;
	gboolean regex;
	gchar* last_search_string;
	gchar* last_replace_string;

	/* Project uri of last search */
	GFile* project_file;

	gboolean busy;
};

enum
{
	COMBO_LANG_NAME,
	COMBO_LANG_TYPES,
	COMBO_N_COLUMNS
};

enum
{
	COLUMN_SELECTED,
	COLUMN_FILENAME,
	COLUMN_COUNT,
	COLUMN_PULSE,
	COLUMN_SPINNER,
	COLUMN_FILE,
	COLUMN_ERROR_TOOLTIP,
	COLUMN_ERROR_CODE,
	N_COLUMNS
};

G_DEFINE_TYPE (SearchFiles, search_files, G_TYPE_OBJECT);

G_MODULE_EXPORT void search_files_search_clicked (SearchFiles* sf);
G_MODULE_EXPORT void search_files_replace_clicked (SearchFiles* sf);
G_MODULE_EXPORT void search_files_update_ui (SearchFiles* sf);

void
search_files_update_ui (SearchFiles* sf)
{
	GtkTreeIter iter;
	gboolean can_replace = FALSE;
	gboolean can_search = FALSE;

	if (!sf->priv->busy)
	{
		gtk_spinner_stop(GTK_SPINNER (sf->priv->spinner_busy));
		gtk_widget_hide (sf->priv->spinner_busy);

		can_search = 
			strlen(gtk_entry_get_text (GTK_ENTRY (sf->priv->search_entry))) > 0;
		
		if (gtk_tree_model_get_iter_first(sf->priv->files_model, &iter))
		{
			do
			{
				gboolean selected;
				gtk_tree_model_get (sf->priv->files_model, &iter,
				                    COLUMN_SELECTED, &selected, -1);
				if (selected)
				{
					can_replace = TRUE;
					break;
				}
			}
			while (gtk_tree_model_iter_next(sf->priv->files_model, &iter));
		}
	}
	else
	{
		gtk_spinner_start(GTK_SPINNER (sf->priv->spinner_busy));
		gtk_widget_show (sf->priv->spinner_busy);
	}

	gtk_widget_set_sensitive (sf->priv->search_button, can_search);
	gtk_widget_set_sensitive (sf->priv->replace_button, can_replace);
	gtk_widget_set_sensitive (sf->priv->search_entry, !sf->priv->busy);
	gtk_widget_set_sensitive (sf->priv->replace_entry, !sf->priv->busy);
}

static void
search_files_check_column_toggled (GtkCellRendererToggle* renderer,
                                   gchar* path,
                                   SearchFiles* sf)
{
	GtkTreePath* tree_path;
	GtkTreeIter iter;
	gboolean state;

	if (sf->priv->busy)
		return;

	tree_path = gtk_tree_path_new_from_string(path);
	gtk_tree_model_get_iter (sf->priv->files_model, &iter, tree_path);

	gtk_tree_path_free(tree_path);

	gtk_tree_model_get (sf->priv->files_model, &iter,
	                    COLUMN_SELECTED, &state, -1);

	gtk_list_store_set (GTK_LIST_STORE (sf->priv->files_model), &iter,
	                    COLUMN_SELECTED, !state,
	                    -1);
}

static void
search_files_finished (SearchFiles* sf, AnjutaCommandQueue* queue)
{
	GtkAdjustment* h_adj;
	GtkAdjustment* v_adj;

	g_object_unref (queue);
	sf->priv->busy = FALSE;

	/* Scroll to first item */
	h_adj = 
		gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (sf->priv->scrolled_window));
	v_adj = 
		gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (sf->priv->scrolled_window));
	gtk_adjustment_set_value (h_adj, 0.0);
	gtk_adjustment_set_value (v_adj, 0.0);

	search_files_update_ui(sf);
}

static void
search_files_command_finished (SearchFileCommand* cmd,
                               guint return_code,
                               SearchFiles* sf)
{
	GtkTreeIter iter;
	GtkTreeRowReference* tree_ref;
	GtkTreePath* path;

	tree_ref = g_object_get_data (G_OBJECT (cmd),
	                              "__tree_ref");
	path = gtk_tree_row_reference_get_path(tree_ref);

	gtk_tree_model_get_iter(sf->priv->files_model, &iter, path);
	gtk_list_store_set (GTK_LIST_STORE (sf->priv->files_model),
	                    &iter,
	                    COLUMN_COUNT, search_file_command_get_n_matches(cmd),
	                    COLUMN_ERROR_CODE, return_code,
	                    COLUMN_ERROR_TOOLTIP, NULL,
	                    -1);
	gtk_tree_row_reference_free(tree_ref);
	gtk_tree_path_free(path);

	if (return_code)
	{
		gtk_list_store_set (GTK_LIST_STORE (sf->priv->files_model),
		                    &iter,
		                    COLUMN_ERROR_CODE, return_code,
		                    COLUMN_ERROR_TOOLTIP,
		                    anjuta_command_get_error_message(ANJUTA_COMMAND(cmd)),
		                    -1);
	}

	g_object_unref (cmd);
}

static void
search_files_search (SearchFiles* sf)
{
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter_first(sf->priv->files_model, &iter))
	{
		AnjutaCommandQueue* queue = anjuta_command_queue_new(ANJUTA_COMMAND_QUEUE_EXECUTE_MANUAL);
		const gchar* pattern =
			gtk_entry_get_text (GTK_ENTRY (sf->priv->search_entry));
		do
		{
			GFile* file;
			gboolean selected;

			/* Save the current values */
			sf->priv->regex =
				gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (sf->priv->regex_check));
			sf->priv->case_sensitive =
				gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (sf->priv->case_check));

			g_free (sf->priv->last_search_string);
			sf->priv->last_search_string = g_strdup(pattern);
			g_free (sf->priv->last_replace_string);
			sf->priv->last_replace_string = NULL;

			gtk_tree_model_get (sf->priv->files_model, &iter,
			                    COLUMN_FILE, &file,
			                    COLUMN_SELECTED, &selected, -1);
			if (selected)
			{
				GtkTreePath* path;
				GtkTreeRowReference* ref;

				path = gtk_tree_model_get_path(sf->priv->files_model, &iter);
				ref = gtk_tree_row_reference_new(sf->priv->files_model,
				                                 path);
				gtk_tree_path_free(path);


				SearchFileCommand* cmd = search_file_command_new(file,
				                                                 pattern,
				                                                 NULL,
				                                                 sf->priv->case_sensitive,
				                                                 sf->priv->regex);
				g_object_set_data (G_OBJECT (cmd), "__tree_ref",
				                   ref);

				g_signal_connect (cmd, "command-finished",
				                  G_CALLBACK(search_files_command_finished), sf);

				anjuta_command_queue_push(queue,
				                          ANJUTA_COMMAND(cmd));
			}
			g_object_unref (file);
		}
		while (gtk_tree_model_iter_next(sf->priv->files_model, &iter));

		g_signal_connect_swapped (queue, "finished", G_CALLBACK (search_files_finished), sf);

		anjuta_command_queue_start (queue);
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE (sf->priv->files_model),
		                                     COLUMN_COUNT,
		                                     GTK_SORT_DESCENDING);

		sf->priv->busy = TRUE;
		search_files_update_ui(sf);
	}
}

void
search_files_replace_clicked (SearchFiles* sf)
{
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter_first(sf->priv->files_model, &iter))
	{
		AnjutaCommandQueue* queue = anjuta_command_queue_new(ANJUTA_COMMAND_QUEUE_EXECUTE_MANUAL);
		const gchar* pattern =
			gtk_entry_get_text (GTK_ENTRY (sf->priv->search_entry));
		const gchar* replace =
			gtk_entry_get_text (GTK_ENTRY (sf->priv->replace_entry));
		do
		{
			GFile* file;
			gboolean selected;

			/* Save the current values */
			sf->priv->regex =
				gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (sf->priv->regex_check));
			sf->priv->case_sensitive =
				gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (sf->priv->case_check));

			g_free (sf->priv->last_search_string);
			sf->priv->last_search_string = g_strdup(pattern);
			g_free (sf->priv->last_replace_string);
			sf->priv->last_replace_string = g_strdup(replace);

			gtk_tree_model_get (sf->priv->files_model, &iter,
			                    COLUMN_FILE, &file,
			                    COLUMN_SELECTED, &selected, -1);
			if (selected)
			{
				GtkTreePath* path;
				GtkTreeRowReference* ref;

				path = gtk_tree_model_get_path(sf->priv->files_model, &iter);
				ref = gtk_tree_row_reference_new(sf->priv->files_model,
				                                 path);
				gtk_tree_path_free(path);


				SearchFileCommand* cmd = search_file_command_new(file,
				                                                 pattern,
				                                                 replace,
				                                                 sf->priv->case_sensitive,
				                                                 sf->priv->regex);
				g_object_set_data (G_OBJECT (cmd), "__tree_ref",
				                   ref);

				g_signal_connect (cmd, "command-finished",
				                  G_CALLBACK(search_files_command_finished), sf);

				anjuta_command_queue_push(queue,
				                          ANJUTA_COMMAND(cmd));
			}
			g_object_unref (file);
		}
		while (gtk_tree_model_iter_next(sf->priv->files_model, &iter));

		g_signal_connect_swapped (queue, "finished", G_CALLBACK (search_files_finished), sf);

		anjuta_command_queue_start (queue);
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE (sf->priv->files_model),
		                                     COLUMN_COUNT,
		                                     GTK_SORT_DESCENDING);

		sf->priv->busy = TRUE;
		search_files_update_ui(sf);
	}
}

static void
search_files_filter_command_finished (SearchFilterFileCommand* cmd,
                                      guint return_code,
                                      SearchFiles* sf)
{
	GFile* file;
	GtkTreeIter iter;
	gchar* display_name = NULL;

	if (return_code)
		return;

	g_object_get (cmd, "file", &file, NULL);

	if (sf->priv->project_file)
	{
		display_name = g_file_get_relative_path (sf->priv->project_file,
		                                         G_FILE (file));
	}
	if (!display_name)
		display_name = g_file_get_path (G_FILE(file));

	gtk_list_store_append(GTK_LIST_STORE (sf->priv->files_model),
	                      &iter);
	gtk_list_store_set (GTK_LIST_STORE (sf->priv->files_model), &iter,
	                    COLUMN_SELECTED, TRUE,
	                    COLUMN_FILENAME, display_name,
	                    COLUMN_FILE, file,
	                    COLUMN_COUNT, 0,
	                    COLUMN_SPINNER, FALSE,
	                    COLUMN_PULSE, FALSE, -1);

	g_object_unref (file);
	g_free (display_name);
}

static void
search_files_filter_finished (AnjutaCommandQueue* queue,
                              SearchFiles* sf)
{
	g_object_unref (queue);
	search_files_search (sf);
}

void
search_files_search_clicked (SearchFiles* sf)
{
	GFile* selected;
	IAnjutaProjectManager* pm;
	GList* files = NULL;
	GList* file;
	gchar* project_uri;
	AnjutaCommandQueue* queue;
	gchar* mime_types;
	GtkComboBox* type_combo;
	GtkTreeIter iter;

	g_return_if_fail (sf != NULL && SEARCH_IS_FILES (sf));

	/* Clear store */
	gtk_list_store_clear(GTK_LIST_STORE (sf->priv->files_model));
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE (sf->priv->files_model),
	                                     COLUMN_FILENAME,
	                                     GTK_SORT_DESCENDING);

	/* Get file and type selection */
	selected =
		ianjuta_project_chooser_get_selected(IANJUTA_PROJECT_CHOOSER (sf->priv->project_combo),
		                                     NULL);
	type_combo = GTK_COMBO_BOX (sf->priv->file_type_combo);
	gtk_combo_box_get_active_iter(type_combo, &iter);
	gtk_tree_model_get (gtk_combo_box_get_model (type_combo),
	                    &iter,
	                    COMBO_LANG_TYPES, &mime_types,
	                    -1);

	/* Get files from project manager */
	pm = anjuta_shell_get_interface (sf->priv->docman->shell,
	                                 IAnjutaProjectManager,
	                                 NULL);
	files = ianjuta_project_manager_get_children(pm, selected, ANJUTA_PROJECT_SOURCE, NULL);

	/* Query project root uri */
	anjuta_shell_get (sf->priv->docman->shell,
	                  IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI,
	                  G_TYPE_STRING,
	                  &project_uri, NULL);
	if (sf->priv->project_file)
		g_object_unref (sf->priv->project_file);

	if (project_uri)
	{
		sf->priv->project_file = g_file_new_for_uri (project_uri);
	}
	g_free (project_uri);


	/* Check that there are some files to process */
	if (files != NULL)
	{
		/* Queue file filtering */
		queue = anjuta_command_queue_new(ANJUTA_COMMAND_QUEUE_EXECUTE_MANUAL);
		g_signal_connect (queue, "finished",
	    	              G_CALLBACK (search_files_filter_finished), sf);
		for (file = files; file != NULL; file = g_list_next (file))
		{	
			SearchFilterFileCommand* cmd =
				search_filter_file_command_new(G_FILE (file->data),
			    	                           mime_types);

			/* Check we are searching in project files only */
			if (!g_file_has_prefix (G_FILE (file->data), sf->priv->project_file))
				continue;
			
			g_signal_connect (cmd, "command-finished",
		    	              G_CALLBACK (search_files_filter_command_finished), sf);
			anjuta_command_queue_push(queue, ANJUTA_COMMAND(cmd));
		}
		sf->priv->busy = TRUE;
		search_files_update_ui(sf);
		anjuta_command_queue_start (queue);

		g_list_foreach (files, (GFunc) g_object_unref, NULL);
		g_list_free (files);
	}

	g_free (mime_types);
}

static void
search_files_render_count (GtkTreeViewColumn *tree_column,
                           GtkCellRenderer *cell,
                           GtkTreeModel *tree_model,
                           GtkTreeIter *iter,
                           gpointer data)
{
	int count;
	gchar* count_str;

	gtk_tree_model_get (tree_model, iter,
	                    COLUMN_COUNT, &count,
	                    -1);
	count_str = g_strdup_printf("%d", count);
	g_object_set (cell, "text", count_str, NULL);
	g_free (count_str);
}

static void
search_files_editor_loaded (SearchFiles* sf, IAnjutaEditor* editor)
{
	search_box_set_search_string(sf->priv->search_box,
	                             sf->priv->last_search_string);
	if (sf->priv->last_replace_string)
	{
		search_box_set_replace_string(sf->priv->search_box,
		                              sf->priv->last_replace_string);
		search_box_set_replace(sf->priv->search_box,
		                       TRUE);
	}
	else
	{
		search_box_set_replace(sf->priv->search_box,
		                       FALSE);
	}
	search_box_toggle_case_sensitive(sf->priv->search_box,
	                                 sf->priv->case_sensitive);
	search_box_toggle_highlight(sf->priv->search_box,
	                            TRUE);
	search_box_toggle_regex(sf->priv->search_box,
	                        sf->priv->regex);
	search_box_search_highlight_all(sf->priv->search_box, TRUE);
	search_box_incremental_search(sf->priv->search_box, TRUE, FALSE);

	gtk_widget_show (GTK_WIDGET(sf->priv->search_box));
}


static void
search_files_result_activated (GtkTreeView* files_tree,
                               GtkTreePath* path,
                               GtkTreeViewColumn* column,
                               SearchFiles* sf)
{
	IAnjutaDocument* editor;
	GFile* file;
	GtkTreeIter iter;

	gtk_tree_model_get_iter (sf->priv->files_model, &iter, path);
	gtk_tree_model_get (sf->priv->files_model, &iter,
	                    COLUMN_FILE, &file, -1);

	/* Check if document is open */
	editor = anjuta_docman_get_document_for_file(sf->priv->docman, file);

	if (editor && IANJUTA_IS_EDITOR(editor))
	{
		anjuta_docman_present_notebook_page(sf->priv->docman,
		                                    editor);
		search_files_editor_loaded (sf, IANJUTA_EDITOR(editor));
	}
	else
	{
		IAnjutaEditor* real_editor =
			anjuta_docman_goto_file_line(sf->priv->docman, file, 0);
		if (real_editor)
			g_signal_connect_swapped (real_editor, "opened",
			                          G_CALLBACK (search_files_editor_loaded), sf);
	}

	g_object_unref (file);
}

static void
search_files_init_tree (SearchFiles* sf)
{
	GtkTreeViewColumn* column_select;
	GtkTreeViewColumn* column_filename;
	GtkTreeViewColumn* column_count;

	GtkCellRenderer* selection_renderer;
	GtkCellRenderer* filename_renderer;
	GtkCellRenderer* count_renderer;
	GtkCellRenderer* error_renderer;

	column_select = gtk_tree_view_column_new();
	sf->priv->files_tree_check = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sf->priv->files_tree_check),
	                             TRUE);
	gtk_widget_show (sf->priv->files_tree_check);
	gtk_tree_view_column_set_widget(column_select,
	                                sf->priv->files_tree_check);
	selection_renderer = gtk_cell_renderer_toggle_new ();
	gtk_tree_view_column_pack_start(column_select,
	                                selection_renderer,
	                                FALSE);
	gtk_tree_view_column_add_attribute(column_select,
	                                   selection_renderer,
	                                   "active",
	                                   COLUMN_SELECTED);
	g_signal_connect (selection_renderer, "toggled",
	                  G_CALLBACK(search_files_check_column_toggled), sf);
	gtk_tree_view_column_set_sort_column_id(column_select,
	                                        COLUMN_SELECTED);

	column_filename = gtk_tree_view_column_new();
	gtk_tree_view_column_set_expand(column_filename,
	                                TRUE);
	gtk_tree_view_column_set_title (column_filename, _("Filename"));
	filename_renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column_filename,
	                                filename_renderer,
	                                TRUE);
	gtk_tree_view_column_add_attribute (column_filename,
	                                    filename_renderer,
	                                    "text",
	                                    COLUMN_FILENAME);
	gtk_tree_view_column_add_attribute (column_filename,
	                                    filename_renderer,
	                                    "sensitive",
	                                    COLUMN_COUNT);
	gtk_tree_view_column_set_sort_column_id(column_filename,
	                                        COLUMN_FILENAME);
	error_renderer = gtk_cell_renderer_pixbuf_new();
	g_object_set (error_renderer, "stock-id", GTK_STOCK_DIALOG_ERROR, NULL);
	gtk_tree_view_column_pack_start(column_filename,
	                                error_renderer,
	                                FALSE);
	gtk_tree_view_column_add_attribute (column_filename,
	                                    error_renderer,
	                                    "visible",
	                                    COLUMN_ERROR_CODE);

	column_count = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title (column_count, "#");
	count_renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column_count,
	                                count_renderer,
	                                TRUE);
	gtk_tree_view_column_add_attribute (column_count,
	                                    count_renderer,
	                                    "sensitive",
	                                    COLUMN_COUNT);
	gtk_tree_view_column_set_cell_data_func(column_count,
	                                        count_renderer,
	                                        search_files_render_count,
	                                        NULL,
	                                        NULL);
	gtk_tree_view_column_set_sort_column_id(column_count,
	                                        COLUMN_COUNT);

	sf->priv->files_model = GTK_TREE_MODEL (gtk_list_store_new (N_COLUMNS,
	                                                            G_TYPE_BOOLEAN,
	                                                            G_TYPE_STRING,
	                                                            G_TYPE_INT,
	                                                            G_TYPE_BOOLEAN,
	                                                            G_TYPE_BOOLEAN,
	                                                            G_TYPE_FILE,
	                                                            G_TYPE_STRING,
	                                                            G_TYPE_INT));
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE (sf->priv->files_model),
	                                     COLUMN_FILENAME,
	                                     GTK_SORT_DESCENDING);

	g_signal_connect_swapped (sf->priv->files_model,
	                          "row-inserted",
	                          G_CALLBACK (search_files_update_ui),
	                          sf);
	g_signal_connect_swapped (sf->priv->files_model,
	                          "row-deleted",
	                          G_CALLBACK (search_files_update_ui),
	                          sf);
	g_signal_connect_swapped (sf->priv->files_model,
	                          "row-changed",
	                          G_CALLBACK (search_files_update_ui),
	                          sf);

	gtk_tree_view_set_model (GTK_TREE_VIEW (sf->priv->files_tree), sf->priv->files_model);
	gtk_tree_view_append_column(GTK_TREE_VIEW (sf->priv->files_tree),
	                            column_select);
	gtk_tree_view_append_column(GTK_TREE_VIEW (sf->priv->files_tree),
	                            column_filename);
	gtk_tree_view_append_column(GTK_TREE_VIEW (sf->priv->files_tree),
	                            column_count);
	gtk_tree_view_set_tooltip_column(GTK_TREE_VIEW (sf->priv->files_tree),
	                                 COLUMN_ERROR_TOOLTIP);
	g_signal_connect (sf->priv->files_tree, "row-activated",
	                  G_CALLBACK (search_files_result_activated), sf);
}

static void
search_files_type_combo_init (SearchFiles* sf)
{
	GtkCellRenderer* combo_renderer = gtk_cell_renderer_text_new();

	GtkTreeIter iter;
	GtkListStore* store;

	IAnjutaLanguage* lang_manager =
		anjuta_shell_get_interface (sf->priv->docman->shell,
		                            IAnjutaLanguage,
		                            NULL);

	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT (sf->priv->file_type_combo),
	                           combo_renderer, TRUE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (sf->priv->file_type_combo),
	                               combo_renderer, "text", 0);

	store = gtk_list_store_new (COMBO_N_COLUMNS,
	                            G_TYPE_STRING,
	                            G_TYPE_STRING);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE (store),
	                                     COMBO_LANG_NAME,
	                                     GTK_SORT_DESCENDING);

	gtk_combo_box_set_model(GTK_COMBO_BOX(sf->priv->file_type_combo),
	                        GTK_TREE_MODEL (store));

	gtk_list_store_append(store, &iter);
	gtk_list_store_set (store, &iter,
	                    COMBO_LANG_NAME, _("All text files"),
	                    COMBO_LANG_TYPES, TEXT_MIME_TYPE,
	                    -1);
	gtk_combo_box_set_active_iter(GTK_COMBO_BOX(sf->priv->file_type_combo),
	                              &iter);

	if (lang_manager)
	{
		GList* languages =
			ianjuta_language_get_languages(lang_manager, NULL);
		GList* lang;
		for (lang = languages; lang != NULL; lang = g_list_next (lang))
		{
			GList* mime_type;
			GString* type_string = g_string_new(NULL);

			GList* mime_types = ianjuta_language_get_mime_types (lang_manager,
			                                                     GPOINTER_TO_INT (lang->data),
			                                                     NULL);
			const gchar* name = ianjuta_language_get_name (lang_manager,
			                                               GPOINTER_TO_INT (lang->data),
			                                               NULL);

			for (mime_type = mime_types; mime_type != NULL; mime_type = g_list_next (mime_type))
			{
				if (type_string->len)
				{
					g_string_append_c (type_string, ',');
				}
				g_string_append (type_string, mime_type->data);
			}
			gtk_list_store_append(store, &iter);
			gtk_list_store_set (store, &iter,
			                    COMBO_LANG_NAME, name,
			                    COMBO_LANG_TYPES, type_string->str,
			                    -1);

			g_string_free (type_string, TRUE);
		}
	}
}

static void
search_files_init (SearchFiles* sf)
{
	GError* error = NULL;

	sf->priv =
		G_TYPE_INSTANCE_GET_PRIVATE (sf, SEARCH_TYPE_FILES, SearchFilesPrivate);

	sf->priv->builder = gtk_builder_new();
	gtk_builder_add_from_file(sf->priv->builder, BUILDER_FILE, &error);

	if (error)
	{
		g_warning("Could load ui file for search files: %s",
		          error->message);
		g_error_free(error);
		return;
	}

	sf->priv->main_box = GTK_WIDGET (gtk_builder_get_object(sf->priv->builder,
	                                                         "main_box"));
	sf->priv->search_button = GTK_WIDGET (gtk_builder_get_object(sf->priv->builder,
	                                                         "search_button"));
	sf->priv->replace_button = GTK_WIDGET (gtk_builder_get_object(sf->priv->builder,
	                                                         "replace_button"));
	sf->priv->search_entry = GTK_WIDGET (gtk_builder_get_object(sf->priv->builder,
	                                                            "search_entry"));
	sf->priv->replace_entry = GTK_WIDGET (gtk_builder_get_object(sf->priv->builder,
	                                                             "replace_entry"));
	sf->priv->project_combo = GTK_WIDGET (gtk_builder_get_object(sf->priv->builder,
	                                                             "project_combo"));
	sf->priv->file_type_combo = GTK_WIDGET (gtk_builder_get_object(sf->priv->builder,
	                                                             "file_type_combo"));

	sf->priv->case_check = GTK_WIDGET (gtk_builder_get_object(sf->priv->builder,
	                                                             "case_check"));
	sf->priv->regex_check = GTK_WIDGET (gtk_builder_get_object(sf->priv->builder,
	                                                           "regex_check"));
	sf->priv->spinner_busy = GTK_WIDGET (gtk_builder_get_object(sf->priv->builder,
	                                                            "spinner_busy"));

	sf->priv->files_tree = GTK_WIDGET (gtk_builder_get_object(sf->priv->builder,
	                                                          "files_tree"));
	sf->priv->scrolled_window = GTK_WIDGET (gtk_builder_get_object (sf->priv->builder,
	                                                                "scrolled_window"));

	search_files_init_tree(sf);

	gtk_builder_connect_signals(sf->priv->builder, sf);

	g_object_ref (sf->priv->main_box);
	gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (sf->priv->main_box)),
	                      sf->priv->main_box);
}

static void
search_files_finalize (GObject* object)
{
	SearchFiles* sf = SEARCH_FILES(object);

	g_object_unref (sf->priv->main_box);
	g_object_unref (sf->priv->builder);
	if (sf->priv->project_file)
		g_object_unref (sf->priv->project_file);
	g_free (sf->priv->last_search_string);
	g_free (sf->priv->last_replace_string);

	G_OBJECT_CLASS (search_files_parent_class)->finalize (object);
}

static void
search_files_class_init (SearchFilesClass* klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = search_files_finalize;

	g_type_class_add_private(klass, sizeof(SearchFilesPrivate));
}

void 
search_files_update_project (SearchFiles* sf)
{
	IAnjutaProjectManager* pm = anjuta_shell_get_interface(sf->priv->docman->shell,
	                                                       IAnjutaProjectManager,
	                                                       NULL);
	ianjuta_project_chooser_set_project_model(IANJUTA_PROJECT_CHOOSER(sf->priv->project_combo),
	                                          pm,
	                                          ANJUTA_PROJECT_GROUP,
	                                          NULL);
}

SearchFiles*
search_files_new (AnjutaDocman* docman, SearchBox* search_box)
{
	AnjutaShell* shell = docman->shell;
	GObject* obj = g_object_new (SEARCH_TYPE_FILES, NULL);
	SearchFiles* sf = SEARCH_FILES(obj);

	anjuta_shell_add_widget(shell, sf->priv->main_box,
	                        "search_files",
	                        _("Find in files"),
	                        GTK_STOCK_FIND_AND_REPLACE,
	                        ANJUTA_SHELL_PLACEMENT_BOTTOM, NULL);

	sf->priv->docman = docman;
	sf->priv->search_box = search_box;

	gtk_widget_show (sf->priv->main_box);

	search_files_type_combo_init(sf);
	search_files_update_ui(sf);
	search_files_update_project (sf);

	return sf;
}

void search_files_present (SearchFiles* sf)
{
	g_return_if_fail (sf != NULL && SEARCH_IS_FILES(sf));

	gtk_entry_set_text (GTK_ENTRY (sf->priv->search_entry),
	                    search_box_get_search_string(sf->priv->search_box));
	gtk_entry_set_text (GTK_ENTRY (sf->priv->replace_entry),
	                    search_box_get_replace_string(sf->priv->search_box));

	anjuta_shell_present_widget(sf->priv->docman->shell,
	                            sf->priv->main_box,
	                            NULL);

	gtk_widget_grab_focus (sf->priv->search_entry);
}
