/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2008 <jrliggett@cox.net>
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

#include "git-branch-combo-model.h"

enum
{
	COL_ACTIVE,
	COL_NAME,
	
	NUM_COLS
};

GitBranchComboData *
git_branch_combo_data_new (GtkListStore *model, GtkComboBox *combo_box,
						   GtkBuilder *bxml, Git *plugin)
{
	GitBranchComboData *data;
	
	data = g_new0 (GitBranchComboData, 1);
	data->model = model;
	data->combo_box = combo_box;
	data->bxml = bxml;
	data->plugin = plugin;
	
	return data;
}

void
git_branch_combo_data_free (GitBranchComboData *data)
{
	g_object_unref (data->bxml);
	g_free (data);
}

GtkListStore *
git_branch_combo_model_new (void)
{
	return gtk_list_store_new (NUM_COLS, G_TYPE_STRING, G_TYPE_STRING);
}

void
git_branch_combo_model_setup_widget (GtkWidget *widget)
{
	GtkCellRenderer *renderer;
	
	/* Active column */
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget), renderer, FALSE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (widget), renderer, 
								   "stock-id", COL_ACTIVE);
	
	/* Name column */
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget), renderer, FALSE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (widget), renderer, 
								   "text", COL_NAME);
}

void
git_branch_combo_model_append (GtkListStore *model, GitBranch *branch)
{
	gchar *name;
	GtkTreeIter iter;
	
	name = git_branch_get_name (branch);
	
	gtk_list_store_append (model, &iter);
	gtk_list_store_set (model, &iter, COL_NAME, name, -1);
	
	if (git_branch_is_active (branch))
		gtk_list_store_set (model, &iter, COL_ACTIVE, GTK_STOCK_YES, -1);
	
	g_free (name);
}

void
git_branch_combo_model_append_remote (GtkListStore *model, const gchar *name)
{
	GtkTreeIter iter;
	
	gtk_list_store_append (model, &iter);
	gtk_list_store_set (model, &iter, COL_NAME, name, -1);
}

gchar *
git_branch_combo_model_get_branch (GtkListStore *model, GtkTreeIter *iter)
{
	gchar *branch;
	
	gtk_tree_model_get (GTK_TREE_MODEL (model), iter, COL_NAME, &branch, -1);
	
	return branch;
}
