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
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/interfaces/ianjuta-project-chooser.h>

#define BUILDER_FILE PACKAGE_DATA_DIR"/glade/anjuta-document-manager.ui"

struct _SearchFilesPrivate
{
	GtkBuilder* builder;

	GtkWidget* main_box;
	
	GtkWidget* search_button;
	GtkWidget* replace_button;
	GtkWidget* find_files_button;
	
	GtkWidget* search_entry;
	GtkWidget* replace_entry;
	
	GtkWidget* files_combo;
	GtkWidget* project_combo;
	GtkWidget* file_type_combo;

	GtkWidget* files_tree;
	GtkTreeModel* files_model;

	GtkWidget* files_tree_check;
	
	AnjutaDocman* docman;
	GtkWidget* dialog;
};

enum
{
	COLUMN_SELECTED,
	COLUMN_FILENAME,
	COLUMN_COUNT,
	COLUMN_PULSE,
	COLUMN_SPINNER,
	COLUMN_FILE
};

G_DEFINE_TYPE (SearchFiles, search_files, G_TYPE_OBJECT);

void search_files_search_clicked (SearchFiles* sf);
void search_files_replace_clicked (SearchFiles* sf);
void search_files_find_files_clicked (SearchFiles* sf);

static void
search_files_get_files (GFile* parent, GList** files, IAnjutaProjectManager* pm)
{
	GList* node;
	GList* children = ianjuta_project_manager_get_children(pm, parent, NULL);
	for (node = children;node != NULL; node = g_list_next(node))
	{
		search_files_get_files(G_FILE(node), files, pm);
		g_message (g_file_get_path (G_FILE(node)));
	}
	g_list_foreach (children, (GFunc)g_object_unref, NULL);
	g_list_free(children);
}

static void
search_files_check_column_clicked (SearchFiles* sf,
                                   GtkCheckButton* button)
{

}

static void
search_files_check_column_toggled (SearchFiles* sf,
                                   GtkCellRenderer* renderer)
{

}

void
search_files_search_clicked (SearchFiles* sf)
{

}

void
search_files_replace_clicked (SearchFiles* sf)
{

}

void
search_files_find_files_clicked (SearchFiles* sf)
{
	GFile* selected;
	IAnjutaProjectManager* pm;
	GList* files;
	GList* file;

	gchar* project_uri = NULL;
	GFile* project_file = NULL;
	
	g_return_if_fail (sf != NULL && SEARCH_IS_FILES (sf));
	
	gtk_list_store_clear(GTK_LIST_STORE (sf->priv->files_model));

	pm = anjuta_shell_get_interface (sf->priv->docman->shell,
	                                 IAnjutaProjectManager,
	                                 NULL);
	files = ianjuta_project_manager_get_elements(pm,
	                                             ANJUTA_PROJECT_SOURCE,
	                                             NULL);
	anjuta_shell_get (sf->priv->docman->shell,
	                  IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI,
	                  G_TYPE_STRING,
	                  &project_uri, NULL);
	
	if (project_uri)
		project_file = g_file_new_for_uri (project_uri);
	g_free (project_uri);
	
	for (file = files; file != NULL; file = g_list_next (file))
	{
		GtkTreeIter iter;

		gchar* display_name = NULL;

		if (project_file)
		{
			display_name = g_file_get_relative_path (project_file,
			                                         G_FILE (file->data));
			if (!display_name)
				continue;
		}
#if 0
		if (!display_name)
			display_name = g_file_get_path (G_FILE (file->data));
		if (!display_name)
			display_name = g_file_get_uri (G_FILE (file->data));
#endif	
		gtk_list_store_append(GTK_LIST_STORE (sf->priv->files_model),
		                      &iter);
		gtk_list_store_set (GTK_LIST_STORE (sf->priv->files_model), &iter,
		                    COLUMN_SELECTED, TRUE,
		                    COLUMN_FILENAME, display_name,
		                    COLUMN_FILE, file->data,
		                    COLUMN_COUNT, 0,
		                    COLUMN_SPINNER, FALSE,
		                    COLUMN_PULSE, FALSE, -1);
	}
	g_object_unref (project_file);
	
	g_list_foreach (files, (GFunc) g_object_unref, NULL);
	g_list_free (files);
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
search_files_update_ui (SearchFiles* sf)
{
	GtkTreeIter iter;
	gboolean file_available = FALSE;
	
	if (gtk_tree_model_get_iter_first(sf->priv->files_model, &iter))
	{
		do
		{
			gboolean selected;
			gtk_tree_model_get (sf->priv->files_model, &iter,
			                    COLUMN_SELECTED, &selected, -1);
			if (selected)
			{
				file_available = TRUE;
				break;
			}
		}
		while (gtk_tree_model_iter_next(sf->priv->files_model, &iter));
	}

	gtk_widget_set_sensitive (sf->priv->search_button, file_available);
	gtk_widget_set_sensitive (sf->priv->replace_button, file_available);	
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
	
	column_select = gtk_tree_view_column_new();
	sf->priv->files_tree_check = gtk_check_button_new();
	gtk_widget_show (sf->priv->files_tree_check);
	gtk_tree_view_column_set_widget(column_select,
	                                sf->priv->files_tree_check);
	g_signal_connect_swapped (sf->priv->files_tree_check, "clicked",
	                          G_CALLBACK(search_files_check_column_clicked), sf);
	selection_renderer = gtk_cell_renderer_toggle_new ();
	gtk_tree_view_column_pack_start(column_select,
	                                selection_renderer,
	                                FALSE);
	gtk_tree_view_column_add_attribute(column_select,
	                                   selection_renderer,
	                                   "active",
	                                   COLUMN_SELECTED);
	g_signal_connect_swapped (selection_renderer, "toggled",
	                          G_CALLBACK(search_files_check_column_toggled), sf);
	
	column_filename = gtk_tree_view_column_new();
	gtk_tree_view_column_set_expand(column_filename,
	                                TRUE);
	gtk_tree_view_column_set_title (column_filename, _("Filename"));
	filename_renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column_filename,
	                                filename_renderer,
	                                TRUE);
	gtk_tree_view_column_add_attribute(column_filename,
	                                   filename_renderer,
	                                   "text",
	                                   COLUMN_FILENAME);

	column_count = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title (column_count, "#");
	count_renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column_count,
	                                count_renderer,
	                                TRUE);
	gtk_tree_view_column_set_cell_data_func(column_count,
	                                        count_renderer,
	                                        search_files_render_count,
	                                        NULL,
	                                        NULL);

	sf->priv->files_model = GTK_TREE_MODEL (gtk_list_store_new (6,
	                                                            G_TYPE_BOOLEAN,
	                                                            G_TYPE_STRING,
	                                                            G_TYPE_INT,
	                                                            G_TYPE_BOOLEAN,
	                                                            G_TYPE_BOOLEAN,
	                                                            G_TYPE_FILE));

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
}

static void
search_files_init (SearchFiles* sf)
{
	GError* error = NULL;
	GtkCellRenderer* combo_renderer;
	
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

	combo_renderer = gtk_cell_renderer_text_new();
	
	sf->priv->main_box = GTK_WIDGET (gtk_builder_get_object(sf->priv->builder,
	                                                         "main_box"));
	sf->priv->search_button = GTK_WIDGET (gtk_builder_get_object(sf->priv->builder,
	                                                         "search_button"));
	sf->priv->replace_button = GTK_WIDGET (gtk_builder_get_object(sf->priv->builder,
	                                                         "replace_button"));
	sf->priv->find_files_button = GTK_WIDGET (gtk_builder_get_object(sf->priv->builder,
	                                                                 "find_files_button"));
	sf->priv->search_entry = GTK_WIDGET (gtk_builder_get_object(sf->priv->builder,
	                                                            "search_entry"));
	sf->priv->replace_entry = GTK_WIDGET (gtk_builder_get_object(sf->priv->builder,
	                                                             "replace_entry"));
	sf->priv->project_combo = GTK_WIDGET (gtk_builder_get_object(sf->priv->builder,
	                                                             "project_combo"));
	sf->priv->file_type_combo = GTK_WIDGET (gtk_builder_get_object(sf->priv->builder,
	                                                             "file_type_combo"));
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT (sf->priv->file_type_combo),
	                           combo_renderer, TRUE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (sf->priv->file_type_combo),
	                               combo_renderer, "text", 0);
	sf->priv->files_tree = GTK_WIDGET (gtk_builder_get_object(sf->priv->builder,
	                                                          "files_tree"));	

	search_files_init_tree(sf);
	
	gtk_builder_connect_signals(sf->priv->builder, sf);
	
	g_object_ref (sf->priv->main_box);
	gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (sf->priv->main_box)),
	                      sf->priv->main_box);

	search_files_update_ui(sf);
}

static void
search_files_finalize (GObject* object)
{
	SearchFiles* sf = SEARCH_FILES(object);

	g_object_unref (sf->priv->main_box);
	g_object_unref (sf->priv->builder);
	
	G_OBJECT_CLASS (search_files_parent_class)->finalize (object);
}

static void
search_files_class_init (SearchFilesClass* klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GObjectClass* parent_class = G_OBJECT_CLASS (klass);

	object_class->finalize = search_files_finalize;

	g_type_class_add_private(klass, sizeof(SearchFilesPrivate));
}

static void
search_files_project_loaded (SearchFiles* sf, IAnjutaProjectManager *pm, GError* e)
{
	if (!e)
	{
		ianjuta_project_chooser_set_project_model(IANJUTA_PROJECT_CHOOSER(sf->priv->project_combo),
		                                          pm,
		                                          ANJUTA_PROJECT_GROUP,
		                                          NULL);
	}
}

SearchFiles* 
search_files_new (AnjutaDocman* docman)
{
	AnjutaShell* shell = docman->shell;
	GObject* obj = g_object_new (SEARCH_TYPE_FILES, NULL);
	SearchFiles* sf = SEARCH_FILES(obj);
	IAnjutaProjectManager* pm = anjuta_shell_get_interface(shell,
	                                                       IAnjutaProjectManager,
	                                                       NULL);
	search_files_project_loaded(sf, pm, NULL);
	g_signal_connect_swapped (pm, "project-loaded",
	                          G_CALLBACK (search_files_project_loaded), NULL);

	anjuta_shell_add_widget(shell, sf->priv->main_box,
	                        "search_files",
	                        _("Find in files"),
	                        GTK_STOCK_FIND_AND_REPLACE,
	                        ANJUTA_SHELL_PLACEMENT_BOTTOM, NULL);

	sf->priv->docman = docman;
	
	gtk_widget_show_all (sf->priv->main_box);
	
	return sf; 
}

void search_files_present (SearchFiles* sf)
{
	anjuta_shell_present_widget(sf->priv->docman->shell,
	                            sf->priv->main_box,
	                            NULL);
}