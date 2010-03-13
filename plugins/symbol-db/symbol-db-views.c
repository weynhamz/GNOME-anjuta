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
#include "symbol-db-model-global.h"
#include "symbol-db-engine.h"

GtkWidget*
symbol_db_view_global_new (SymbolDBEngine *dbe)
{
	GtkWidget *sw;
	GtkTreeModel *model;
	GtkWidget *dbv;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	model = symbol_db_model_global_new (dbe);
	
	dbv = gtk_tree_view_new_with_model (model);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (dbv), FALSE);
	gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (dbv), TRUE);
	
	/* Columns */
	/* Note: Fixed sized columns are mandatory for high performance */
	column = gtk_tree_view_column_new ();

	gtk_tree_view_column_set_fixed_width (column, 300);
	gtk_tree_view_column_set_title (column, _("Symbol"));
	gtk_tree_view_column_set_sizing (column,
	                                 GTK_TREE_VIEW_COLUMN_FIXED);

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "pixbuf",
	                                    SYMBOL_DB_MODEL_GLOBAL_COL_PIXBUF);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
	                                    SYMBOL_DB_MODEL_GLOBAL_COL_LABEL);

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
