/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * symbol-db-views.c
 * Copyright (C) Naba Kumar 2010 <naba@gnome.org>
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
#include <gtk/gtktreeview.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-markable.h>
#include "symbol-db-model-project.h"
#include "symbol-db-model-file.h"
#include "symbol-db-engine.h"
#include "symbol-db-views.h"
#include "plugin.h"

static void
on_treeview_row_activated (GtkTreeView *view, GtkTreePath *arg1,
                           GtkTreeViewColumn *arg2,
                           SymbolDBPlugin *plugin)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	IAnjutaDocumentManager *docman;
	GFile* file;
	gchar *filename, *full_path;
	gint line;

	AnjutaShell *shell = ANJUTA_PLUGIN (plugin)->shell;
	selection = gtk_tree_view_get_selection (view);
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
	    return;

	gtk_tree_model_get (model, &iter,
	                    SYMBOL_DB_MODEL_PROJECT_COL_FILE, &filename,
	                    SYMBOL_DB_MODEL_PROJECT_COL_LINE, &line,
	                    -1);
	g_return_if_fail (filename != NULL);

	docman = anjuta_shell_get_interface (shell, IAnjutaDocumentManager,
	                                     NULL);
	g_return_if_fail (docman != NULL);

	full_path = g_build_filename (plugin->project_root_dir, filename, NULL);
	file = g_file_new_for_path (full_path);
	ianjuta_document_manager_goto_file_line (docman, file, line, NULL);
	g_free (full_path);
	g_free (filename);
	g_object_unref (file);

	if (IANJUTA_IS_MARKABLE (plugin->current_editor))
	{
		ianjuta_markable_delete_all_markers (IANJUTA_MARKABLE (plugin->current_editor),
		                                     IANJUTA_MARKABLE_LINEMARKER,
		                                     NULL);

		ianjuta_markable_mark (IANJUTA_MARKABLE (plugin->current_editor),
		                       line, IANJUTA_MARKABLE_LINEMARKER, NULL);
	}
}

static void
on_treeview_row_expanded (GtkTreeView *view, GtkTreeIter *iter,
                          GtkTreePath *path, SymbolDBPlugin *plugin)
{
	gint symbol_id;
	GtkTreeModel *model;
	GHashTable *expanded_nodes =
		g_object_get_data (G_OBJECT (view), "__expanded_nodes__");

	model = gtk_tree_view_get_model (view);
	gtk_tree_model_get (model, iter, SYMBOL_DB_MODEL_PROJECT_COL_SYMBOL_ID,
	                    &symbol_id, -1);
	g_hash_table_insert (expanded_nodes, GINT_TO_POINTER (symbol_id),
	                     GINT_TO_POINTER (symbol_id));
}

static void
on_treeview_row_collapsed (GtkTreeView *view, GtkTreeIter *iter,
                           GtkTreePath *path, SymbolDBPlugin *plugin)
{
	gint symbol_id;
	GtkTreeModel *model;
	
	GHashTable *expanded_nodes =
		g_object_get_data (G_OBJECT (view), "__expanded_nodes__");

	model = gtk_tree_view_get_model (view);
	gtk_tree_model_get (model, iter, SYMBOL_DB_MODEL_PROJECT_COL_SYMBOL_ID,
	                    &symbol_id, -1);
	g_hash_table_remove (expanded_nodes, GINT_TO_POINTER (symbol_id));
}

static void
on_treeview_has_child_toggled (GtkTreeModel *model,
                               GtkTreePath  *path,
                               GtkTreeIter  *iter,
                               GtkTreeView  *view)
{
	gint symbol_id;
	
	GHashTable *expanded_nodes =
		g_object_get_data (G_OBJECT (view), "__expanded_nodes__");
	
	gtk_tree_model_get (model, iter, SYMBOL_DB_MODEL_PROJECT_COL_SYMBOL_ID,
	                    &symbol_id, -1);
	if (g_hash_table_lookup (expanded_nodes, GINT_TO_POINTER (symbol_id)))
		gtk_tree_view_expand_row (view, path, FALSE);
}

GtkWidget*
symbol_db_view_new (SymbolViewType view_type,
                    SymbolDBEngine *dbe, SymbolDBPlugin *plugin)
{
	GtkWidget *sw;
	GtkTreeModel *model;
	GtkWidget *dbv;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	switch (view_type)
	{
		case SYMBOL_DB_VIEW_FILE:
			model = symbol_db_model_file_new (dbe);
			break;
		default:
			model = symbol_db_model_project_new (dbe);
	}
	
	dbv = gtk_tree_view_new_with_model (model);
	g_object_unref (model);

	g_signal_connect (G_OBJECT (dbv), "row-activated",
					  G_CALLBACK (on_treeview_row_activated), plugin);
	g_signal_connect (G_OBJECT (dbv), "row-expanded",
					  G_CALLBACK (on_treeview_row_expanded), plugin);
	g_signal_connect (G_OBJECT (dbv), "row-collapsed",
					  G_CALLBACK (on_treeview_row_collapsed), plugin);
	g_signal_connect (G_OBJECT (model), "row-has-child-toggled",
					  G_CALLBACK (on_treeview_has_child_toggled), dbv);

	g_object_set_data_full (G_OBJECT (dbv), "__expanded_nodes__",
	                        g_hash_table_new (g_direct_hash, g_direct_equal),
	                        (GDestroyNotify)g_hash_table_destroy);
	
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (dbv), FALSE);
	gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (dbv), TRUE);
	gtk_tree_view_set_tooltip_column (GTK_TREE_VIEW (dbv),
	                                  SYMBOL_DB_MODEL_PROJECT_COL_ARGS);
	/* Columns */
	/* Note: Fixed sized columns are mandatory for high performance */
	column = gtk_tree_view_column_new ();

	gtk_tree_view_column_set_fixed_width (column, 400);
	gtk_tree_view_column_set_title (column, _("Symbol"));
	gtk_tree_view_column_set_sizing (column,
	                                 GTK_TREE_VIEW_COLUMN_FIXED);

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_renderer_set_fixed_size (renderer, 16, -1);
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "pixbuf",
	                                    SYMBOL_DB_MODEL_PROJECT_COL_PIXBUF);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "markup",
	                                    SYMBOL_DB_MODEL_PROJECT_COL_LABEL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (dbv), column);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (dbv), column);

	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
	                                     GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
	                                GTK_POLICY_AUTOMATIC,
	                                GTK_POLICY_AUTOMATIC);
	gtk_widget_show (dbv);
	gtk_container_add (GTK_CONTAINER (sw), dbv);
	gtk_widget_show (sw);
	return sw;
}
