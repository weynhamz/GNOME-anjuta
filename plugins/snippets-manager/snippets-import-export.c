/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-import-export.c
    Copyright (C) Dragos Dena 2010

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, 
	Boston, MA  02110-1301  USA
*/

#include "snippets-import-export.h"
#include "snippets-xml-parser.h"
#include "snippets-group.h"
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libanjuta/anjuta-utils.h>

enum
{
	SNIPPETS_STORE_COL_SNIPPET_OBJECT = 0,
	SNIPPETS_STORE_COL_ACTIVE,
	SNIPPETS_STORE_COL_NO
};

static void
add_or_update_snippet (SnippetsDB *snippets_db,
                       AnjutaSnippet *snippet,
                       const gchar *group_name)
{
	const gchar *trigger = NULL;
	GList *iter = NULL, *languages = NULL;
	gchar *cur_lang = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	g_return_if_fail (ANJUTA_IS_SNIPPET (snippet));

	/* Get the trigger-key and the languages for which the snippet is supported */
	trigger = snippet_get_trigger_key (snippet);
	languages = (GList *)snippet_get_languages (snippet);

	/* Check if each (trigger, language) tuple exists in the database and update it 
	   if yes, or add it if not. */
	for (iter = g_list_first (languages); iter != NULL; iter = g_list_next (iter))
	{
		cur_lang = (gchar *)iter->data;

		/* If there is already an entry for (trigger, cur_lang) we remove it so
		   we can update it. */
		if (snippets_db_get_snippet (snippets_db, trigger, cur_lang))
		{
			snippets_db_remove_snippet (snippets_db, trigger, cur_lang, FALSE);
		}

	}

	snippets_db_add_snippet (snippets_db, snippet, group_name);
}

static void
add_group_list_to_database (SnippetsDB *snippets_db,
                            GList *snippets_groups)
{
	GList *iter = NULL, *iter2 = NULL, *snippets = NULL;
	AnjutaSnippetsGroup *cur_group = NULL;
	const gchar *cur_group_name = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	if (snippets_groups == NULL)
		return;

	for (iter = g_list_first (snippets_groups); iter != NULL; iter = g_list_next (iter))
	{
		if (!ANJUTA_IS_SNIPPETS_GROUP (iter->data))
			continue;
		cur_group = ANJUTA_SNIPPETS_GROUP (iter->data);
		cur_group_name = snippets_group_get_name (cur_group);

		/* If there isn't a snippets group with the same name we just add it */		
		if (!snippets_db_has_snippets_group_name (snippets_db, cur_group_name))
		{
			snippets_db_add_snippets_group (snippets_db, cur_group, TRUE);
			continue;
		}

		/* If there is already a group with the same name, we add the snippets inside
		   the group */
		snippets = snippets_group_get_snippets_list (cur_group);

		for (iter2 = g_list_first (snippets); iter2 != NULL; iter2 = g_list_next (iter2))
		{
			if (!ANJUTA_IS_SNIPPET (iter2->data))
				continue;

			add_or_update_snippet (snippets_db, 
			                       ANJUTA_SNIPPET (iter2->data), 
			                       cur_group_name);
		}
	}
}

static void
add_native_snippets_at_path (SnippetsDB *snippets_db,
                             const gchar *path)
{
	GList *snippets_groups = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));

	if (path == NULL)
		return;

	/* Parse the snippets file */
	snippets_groups = snippets_manager_parse_snippets_xml_file (path, NATIVE_FORMAT);

	/* Add the snippets groups from the file to the database */
	add_group_list_to_database (snippets_db, snippets_groups);

}

static void
add_other_snippets_at_path (SnippetsDB *snippets_db,
                            const gchar *path)
{

	/* TODO - import the snippets. Should try every format known until it matches */
}

void 
snippets_manager_import_snippets (SnippetsDB *snippets_db,
                                  AnjutaShell *anjuta_shell)
{
	GtkWidget *file_chooser = NULL;
	GtkFileFilter *native_filter = NULL, *other_filter = NULL, *cur_filter = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));

	file_chooser = gtk_file_chooser_dialog_new (_("Import Snippets"),
	                                            GTK_WINDOW (anjuta_shell),
	                                            GTK_FILE_CHOOSER_ACTION_OPEN,
	                                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	                                            GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
	                                            NULL);

	/* Set up the filters */
	native_filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (native_filter, "Native format");
	gtk_file_filter_add_pattern (native_filter, "*.anjuta-snippets");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (file_chooser), native_filter);

	other_filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (other_filter, "Other formats");
	gtk_file_filter_add_pattern (other_filter, "*");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (file_chooser), other_filter);

	if (gtk_dialog_run (GTK_DIALOG (file_chooser)) == GTK_RESPONSE_ACCEPT)
	{
		gchar *uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (file_chooser)),
		      *path = anjuta_util_get_local_path_from_uri (uri);
		
		cur_filter = gtk_file_chooser_get_filter (GTK_FILE_CHOOSER (file_chooser));
		if (!g_strcmp0 ("Native format", gtk_file_filter_get_name (cur_filter)))
			add_native_snippets_at_path (snippets_db, path);
		else
			add_other_snippets_at_path (snippets_db, path);

		g_free (path);
		g_free (uri);
	}
	
	gtk_widget_destroy (file_chooser);
}

static void
snippets_view_trigger_data_func (GtkTreeViewColumn *column,
                                 GtkCellRenderer *renderer,
                                 GtkTreeModel *tree_model,
                                 GtkTreeIter *iter, 
                                 gpointer user_data)
{
	const gchar *trigger = NULL;
	GObject *snippet = NULL;

	/* Assertions */
	g_return_if_fail (GTK_IS_CELL_RENDERER_TEXT (renderer));
	g_return_if_fail (GTK_IS_TREE_MODEL (tree_model));

	/* Get trigger key */	
	gtk_tree_model_get (tree_model, iter, 
						SNIPPETS_STORE_COL_SNIPPET_OBJECT, &snippet,
						-1);
	
	if (ANJUTA_IS_SNIPPET (snippet))
	{
		trigger = snippet_get_trigger_key (ANJUTA_SNIPPET (snippet));
		g_object_set (renderer, "text", trigger, NULL);
	}
	else if (ANJUTA_IS_SNIPPETS_GROUP (snippet))
	{
		g_object_set (renderer, "text", "", NULL);
	}
}

static void
snippets_view_languages_data_func (GtkTreeViewColumn *column,
                                   GtkCellRenderer *renderer,
                                   GtkTreeModel *tree_model,
                                   GtkTreeIter *iter, 
                                   gpointer user_data)
{	const gchar *languages = NULL;
	GObject *snippet = NULL;

	/* Assertions */
	g_return_if_fail (GTK_IS_CELL_RENDERER_TEXT (renderer));
	g_return_if_fail (GTK_IS_TREE_MODEL (tree_model));

	/* Get languages */	
	gtk_tree_model_get (tree_model, iter, 
						SNIPPETS_STORE_COL_SNIPPET_OBJECT, &snippet,
						-1);
	
	if (ANJUTA_IS_SNIPPET (snippet))
	{
		languages = snippet_get_languages_string (ANJUTA_SNIPPET (snippet));
		g_object_set (renderer, "text", languages, NULL);
	}
	else if (ANJUTA_IS_SNIPPETS_GROUP (snippet))
	{
		g_object_set (renderer, "text", "", NULL);
	}

}

static void
snippets_view_name_data_func (GtkTreeViewColumn *column,
                              GtkCellRenderer *renderer,
                              GtkTreeModel *tree_model,
                              GtkTreeIter *iter, 
                              gpointer user_data)
{
	const gchar *name = NULL;
	GObject *snippet = NULL;

	/* Assertions */
	g_return_if_fail (GTK_IS_CELL_RENDERER_TEXT (renderer));
	g_return_if_fail (GTK_IS_TREE_MODEL (tree_model));

	/* Get name */
	gtk_tree_model_get (tree_model, iter, 
						SNIPPETS_STORE_COL_SNIPPET_OBJECT, &snippet,
						-1);
	
	if (ANJUTA_IS_SNIPPET (snippet))
	{
		name = snippet_get_name (ANJUTA_SNIPPET (snippet));
		g_object_set (renderer, "text", name, NULL);
	}
	else if (ANJUTA_IS_SNIPPETS_GROUP (snippet))
	{
		name = snippets_group_get_name (ANJUTA_SNIPPETS_GROUP (snippet));
		g_object_set (renderer, "text", name, NULL);
	}
}


static void 
handle_toggle (GtkCellRendererToggle *cell_renderer,
               gchar                 *path,
               gpointer               snippet_tree_store)
{
	gboolean active;
	GtkTreeIter iter, child_iter, parent_iter;
	GObject *snippet;

	/* Assertions */
	g_return_if_fail (GTK_IS_CELL_RENDERER_TOGGLE (cell_renderer));
	g_return_if_fail (GTK_IS_TREE_STORE (snippet_tree_store));

	/* Get the toggled object*/
	gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (snippet_tree_store),
	                                     &iter, path);
	gtk_tree_model_get (GTK_TREE_MODEL (snippet_tree_store), &iter,
	                    SNIPPETS_STORE_COL_ACTIVE, &active,
						SNIPPETS_STORE_COL_SNIPPET_OBJECT, &snippet,
						-1);
	if (active == TRUE)
		active = FALSE;
	else
		active = TRUE;
	
	/* If group toggled, toggle all the snippets */
	if (ANJUTA_IS_SNIPPETS_GROUP (snippet))
	{
		if (gtk_tree_model_iter_children (GTK_TREE_MODEL (snippet_tree_store), 
		                                  &child_iter, &iter))
			{
				do
				{
					gtk_tree_store_set (snippet_tree_store, &child_iter,
					                    SNIPPETS_STORE_COL_ACTIVE, active,
						            -1);
				} 
				while (gtk_tree_model_iter_next (GTK_TREE_MODEL (snippet_tree_store),
				                                 &child_iter));
			}
	}
	/* If snippet toggled, select the snippets group */
	if (ANJUTA_IS_SNIPPET (snippet))
	{
		if (gtk_tree_model_iter_parent (GTK_TREE_MODEL (snippet_tree_store),
		                                &parent_iter, &iter) && active == TRUE)
		{
			gtk_tree_store_set (snippet_tree_store, &parent_iter,
	                            SNIPPETS_STORE_COL_ACTIVE, TRUE,
			                    -1);
		}
	}
		
	gtk_tree_store_set (snippet_tree_store, &iter,
	                    SNIPPETS_STORE_COL_ACTIVE, active,
	                    -1);

	g_print("%s\n", path);
}

static gboolean 
model_foreach_set_store_func (GtkTreeModel *model,
                              GtkTreePath *path,
                              GtkTreeIter *iter,
                              gpointer store)
{
	GObject *obj = NULL;
	GtkTreeIter store_iter;
	GtkTreeStore *tree_store = NULL;
	static GtkTreeIter group_iter;
	/* Assertions */
	g_return_val_if_fail (GTK_IS_TREE_STORE (store), TRUE);

	tree_store = GTK_TREE_STORE (store);

	gtk_tree_model_get (model, iter,
	                    SNIPPETS_DB_MODEL_COL_CUR_OBJECT, &obj,
	                    -1);
	
	if (ANJUTA_IS_SNIPPETS_GROUP (obj))
	{
		gtk_tree_store_append (tree_store, &store_iter, NULL);
		group_iter = store_iter;
		gtk_tree_store_set (tree_store, &store_iter,
		                    SNIPPETS_STORE_COL_SNIPPET_OBJECT, obj,
	                        SNIPPETS_STORE_COL_ACTIVE, TRUE,
	                        -1);
	}
	if (ANJUTA_IS_SNIPPET (obj))
	{
		gtk_tree_store_append (tree_store, &store_iter, &group_iter);
		gtk_tree_store_set (tree_store, &store_iter,
		                    SNIPPETS_STORE_COL_SNIPPET_OBJECT, obj,
							SNIPPETS_STORE_COL_ACTIVE, TRUE,
							-1);
	}

	return FALSE;
}

static GtkTreeStore*
create_snippets_store (SnippetsDB *snippets_db)
{
	GtkTreeStore *store = NULL;
	GtkTreeModel *model = NULL;
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), NULL);
	
	model = gtk_tree_model_filter_new (GTK_TREE_MODEL(snippets_db), NULL);
	store = gtk_tree_store_new (2, G_TYPE_OBJECT, G_TYPE_BOOLEAN);
	gtk_tree_model_foreach (model, model_foreach_set_store_func, store);
	return store;
} 

void snippets_manager_export_snippets (SnippetsDB *snippets_db,
                                       AnjutaShell *anjuta_shell)
{
	GtkWidget               *file_chooser = NULL;
	GtkFileFilter           *filter = NULL;
	GtkWidget               *snippets_chooser = NULL;
	GtkWidget               *snippets_tree_view = NULL;
	GtkWidget               *scrolled_box = NULL;
	GtkCellRenderer         *text_renderer = NULL, *toggle_renderer = NULL;
	GtkTreeViewColumn       *column = NULL;
	GtkTreeStore            *snippet_tree_store = NULL;
	GtkTreeIter             iter, child_iter; 
	GList                   *snippets_group_list = NULL;
	gboolean                active;
	AnjutaSnippet           *snippet = NULL;
	AnjutaSnippetsGroup     *snippets_group = NULL;
	GObject                 *object = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	

	/* Create view and set model */
	snippets_tree_view = gtk_tree_view_new();
	snippet_tree_store = create_snippets_store (snippets_db);
	gtk_tree_view_set_model (GTK_TREE_VIEW (snippets_tree_view),
							 GTK_TREE_MODEL (snippet_tree_store));
	
	/* Column 1 -- Name */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title  (column, "Name");
	gtk_tree_view_append_column     (GTK_TREE_VIEW (snippets_tree_view), 
	                                 column);
	
	toggle_renderer = gtk_cell_renderer_toggle_new();

	g_signal_connect (toggle_renderer, "toggled", G_CALLBACK (handle_toggle), snippet_tree_store);

	gtk_tree_view_column_pack_start (column,
	                                 toggle_renderer,
	                                 TRUE);

	gtk_tree_view_column_add_attribute (column,
	                                    toggle_renderer,
	                                    "active", SNIPPETS_STORE_COL_ACTIVE);

	text_renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start (column, 
	                                 text_renderer,
	                                 TRUE);

	gtk_tree_view_column_set_cell_data_func (column, text_renderer,
	                                         snippets_view_name_data_func,
	                                         snippets_tree_view, NULL);


	/* Column 2 -- Trigger */
	
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title  (column, "Trigger");
	gtk_tree_view_append_column     (GTK_TREE_VIEW (snippets_tree_view), 
	                                 column);
	text_renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start (column, 
	                                 text_renderer,
	                                 TRUE);

	gtk_tree_view_column_set_cell_data_func (column, text_renderer,
	                                         snippets_view_trigger_data_func,
	                                         snippets_tree_view, NULL);

 	/* Column 3 -- Languages */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title  (column, "Languages");
	gtk_tree_view_append_column     (GTK_TREE_VIEW (snippets_tree_view), 
	                                 column);
	text_renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start (column, 
	                                 text_renderer,
	                                 TRUE);

	gtk_tree_view_column_set_cell_data_func (column, text_renderer,
	                                         snippets_view_languages_data_func,
	                                         snippets_tree_view, NULL);

	/* Dialog for snippets chooser */
	snippets_chooser = gtk_dialog_new_with_buttons ("Select Snippets",
	                                                GTK_WINDOW (anjuta_shell),
	                                                GTK_DIALOG_MODAL,
	                                                NULL);
	/* Add the tree-view to dialog */
	
	scrolled_box = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_set_border_width (GTK_CONTAINER(scrolled_box), 0);
	gtk_widget_set_size_request (scrolled_box, 300, 600);
	/* TODO make it resize with the main dialog */
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrolled_box), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC); 	
	
	gtk_container_add (GTK_CONTAINER (scrolled_box),
					   snippets_tree_view);

	gtk_container_add (GTK_CONTAINER ( gtk_dialog_get_content_area (GTK_DIALOG ( snippets_chooser))), 
					   scrolled_box);
	gtk_widget_show (snippets_tree_view);
	gtk_widget_show (scrolled_box);

	gtk_dialog_add_button (GTK_DIALOG (snippets_chooser), "OK", GTK_RESPONSE_DELETE_EVENT);
	
	/* Wait for the dialog to be closed */
	if (gtk_dialog_run (GTK_DIALOG (snippets_chooser)) == GTK_RESPONSE_DELETE_EVENT)
	{
		gtk_widget_destroy (snippets_chooser);
	}

	file_chooser = gtk_file_chooser_dialog_new (_("Export Snippets"),
	                                            GTK_WINDOW (anjuta_shell),
	                                            GTK_FILE_CHOOSER_ACTION_SAVE,
	                                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	                                            GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
	                                            NULL);
	/* Set up the filter */
	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, "Native format");
	gtk_file_filter_add_pattern (filter, "*.anjuta-snippets");
	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (file_chooser), filter);

	if (gtk_dialog_run (GTK_DIALOG (file_chooser)) == GTK_RESPONSE_ACCEPT)
	{
		gchar *uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (file_chooser)),
		      *path = anjuta_util_get_local_path_from_uri (uri);
		
		/* Get the first iter from snippet_tree_store */
		gtk_tree_model_get_iter_first (GTK_TREE_MODEL (snippet_tree_store), 
		                               &iter);	

		/* Select the snippets for exporting */	
		do
		{
			gtk_tree_model_get (GTK_TREE_MODEL (snippet_tree_store), &iter,
			                    SNIPPETS_STORE_COL_ACTIVE, &active,
			                    SNIPPETS_STORE_COL_SNIPPET_OBJECT, &object,
							    -1);

			if (ANJUTA_IS_SNIPPETS_GROUP (object) && active)
			{
				snippets_group = snippets_group_new (snippets_group_get_name (ANJUTA_SNIPPETS_GROUP (object)));
				
				if (gtk_tree_model_iter_children (GTK_TREE_MODEL (snippet_tree_store),
		                                  &child_iter, &iter))
				{
					/* Iterate through snippets from current group */
					do
					{
						gtk_tree_model_get (GTK_TREE_MODEL (snippet_tree_store), &child_iter,
						                    SNIPPETS_STORE_COL_ACTIVE, &active,
						                    SNIPPETS_STORE_COL_SNIPPET_OBJECT, &snippet,
						                    -1);
						if (active)
						{
							snippets_group_add_snippet (snippets_group, snippet);
						}
					} 
					while (gtk_tree_model_iter_next (GTK_TREE_MODEL (snippet_tree_store),
					                                 &child_iter));
				}

				snippets_group_list = g_list_append (snippets_group_list, snippets_group);
			}

		} 
		while (gtk_tree_model_iter_next (GTK_TREE_MODEL (snippet_tree_store), &iter));

		/* Save the snippets */
		snippets_manager_save_snippets_xml_file (NATIVE_FORMAT,
		                                         snippets_group_list,
		                                         path);
		g_free (path);
		g_free (uri);
	}
	g_object_unref (snippet_tree_store);	
	gtk_widget_destroy (file_chooser);

}
