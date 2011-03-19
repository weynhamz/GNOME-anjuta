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

#define EXPORT_UI PACKAGE_DATA_DIR"/glade/snippets-export-dialog.ui"

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

static gboolean
snippets_store_unref_foreach_func (GtkTreeModel *model,
                                   GtkTreePath *path,
                                   GtkTreeIter *iter,
                                   gpointer data)
{
	GObject *obj = NULL;
	gtk_tree_model_get (model, iter,
	                    SNIPPETS_STORE_COL_SNIPPET_OBJECT, &obj,
	                    -1);
	g_object_unref (obj);
	return FALSE;
}

static gboolean 
save_snippets_to_path (GtkTreeStore *snippets_tree_store,
                       gchar *path,
                       gboolean overwrite)
{
	AnjutaSnippet           *snippet = NULL;
	AnjutaSnippetsGroup     *snippets_group = NULL;
	GObject                 *object = NULL;
	GtkTreeIter             iter, child_iter; 
	GList                   *snippets_group_list = NULL, *element = NULL;
	gboolean                active;
	
	/* Assertions */
	g_return_val_if_fail (GTK_IS_TREE_STORE (snippets_tree_store), TRUE);
	
	if (g_file_test (path, G_FILE_TEST_EXISTS) &&
		!overwrite)
		return FALSE;

	/* Save snippets */
	/* Get the first iter from snippets_tree_store */
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (snippets_tree_store), 
		                               &iter) == FALSE)
		return TRUE;
	/* Select the snippets for exporting */	
	do
	{	
		/* Get current group */
		gtk_tree_model_get (GTK_TREE_MODEL (snippets_tree_store), &iter,
		                    SNIPPETS_STORE_COL_ACTIVE, &active,
		                    SNIPPETS_STORE_COL_SNIPPET_OBJECT, &object,
		                    -1);
		g_object_ref (object);

		if (ANJUTA_IS_SNIPPETS_GROUP (object) && active)
		{
			snippets_group = snippets_group_new (snippets_group_get_name (ANJUTA_SNIPPETS_GROUP (object)));
			g_object_unref (object);

			if (gtk_tree_model_iter_children (GTK_TREE_MODEL (snippets_tree_store),
	                                  &child_iter, &iter))
			{
				/* Iterate through snippets from current group */
				do
				{
					gtk_tree_model_get (GTK_TREE_MODEL (snippets_tree_store), &child_iter,
					                    SNIPPETS_STORE_COL_ACTIVE, &active,
					                    SNIPPETS_STORE_COL_SNIPPET_OBJECT, &snippet,
					                    -1);
					if (active)
					{
						snippets_group_add_snippet (snippets_group, snippet);
					}
				} 
				while (gtk_tree_model_iter_next (GTK_TREE_MODEL (snippets_tree_store),
				                                 &child_iter));
			}

			snippets_group_list = g_list_append (snippets_group_list, snippets_group);
		}

	} 
	while (gtk_tree_model_iter_next (GTK_TREE_MODEL (snippets_tree_store), &iter));
	
	snippets_manager_save_snippets_xml_file (NATIVE_FORMAT,
	                                         snippets_group_list,
	                                         path);
	/* Destroy created snippets groups */
	for (element = g_list_first (snippets_group_list); element; element = g_list_next (element))
		g_object_unref (element->data);
	g_list_free (snippets_group_list);
	return TRUE;
}


static void 
handle_toggle (GtkCellRendererToggle *cell_renderer,
               gchar                 *path,
               gpointer               snippets_tree_store)
{
	gboolean active;
	GtkTreeIter iter, child_iter, parent_iter;
	GObject *snippet;

	/* Assertions */
	g_return_if_fail (GTK_IS_CELL_RENDERER_TOGGLE (cell_renderer));
	g_return_if_fail (GTK_IS_TREE_STORE (snippets_tree_store));

	/* Get the toggled object*/
	gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (snippets_tree_store),
	                                     &iter, path);
	gtk_tree_model_get (GTK_TREE_MODEL (snippets_tree_store), &iter,
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
		if (gtk_tree_model_iter_children (GTK_TREE_MODEL (snippets_tree_store), 
		                                  &child_iter, &iter))
			{
				do
				{
					gtk_tree_store_set (snippets_tree_store, &child_iter,
					                    SNIPPETS_STORE_COL_ACTIVE, active,
						                -1);
				} 
				while (gtk_tree_model_iter_next (GTK_TREE_MODEL (snippets_tree_store),
				                                 &child_iter));
			}
	}

	/* If snippet toggled, select the snippets group */
	if (ANJUTA_IS_SNIPPET (snippet))
	{
		if (gtk_tree_model_iter_parent (GTK_TREE_MODEL (snippets_tree_store),
		                                &parent_iter, &iter) && active == TRUE)
		{
			gtk_tree_store_set (snippets_tree_store, &parent_iter,
	                            SNIPPETS_STORE_COL_ACTIVE, TRUE,
			                    -1);
		}
	}
		
	gtk_tree_store_set (snippets_tree_store, &iter,
	                    SNIPPETS_STORE_COL_ACTIVE, active,
	                    -1);
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
	
	g_object_ref (obj);	
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

static GtkWidget*
create_snippets_tree_view (SnippetsDB *snippets_db,
                           GtkTreeStore *snippets_tree_store)
{
	GtkWidget *snippets_tree_view = NULL;
	GtkTreeViewColumn *column = NULL;
	GtkCellRenderer   *text_renderer = NULL, *toggle_renderer = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), NULL);

	/* Create view and set model */
	snippets_tree_view = gtk_tree_view_new();
	gtk_tree_view_set_model (GTK_TREE_VIEW (snippets_tree_view),
	                         GTK_TREE_MODEL (snippets_tree_store));

	/* Column 1 -- Name */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title  (column, "Name");
	gtk_tree_view_append_column     (GTK_TREE_VIEW (snippets_tree_view), 
	                                 column);
	toggle_renderer = gtk_cell_renderer_toggle_new();

	g_signal_connect (toggle_renderer, "toggled", G_CALLBACK (handle_toggle), snippets_tree_store);

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
	return snippets_tree_view;
}

void snippets_manager_export_snippets (SnippetsDB *snippets_db,
                                       AnjutaShell *anjuta_shell)
{
	GtkWidget               *tree_view_window = NULL;
	GtkDialog               *snippets_export_dialog = NULL;
	GtkDialog               *file_conflict_dialog = NULL;
	GtkWidget               *snippets_tree_view = NULL;
	GtkTreeStore            *snippets_tree_store = NULL;
	GtkBuilder              *dialog_builder = NULL;
	GError                  *error = NULL;
	gint					export_dialog_result;
	GtkFileChooserButton    *folder_selector_button = NULL;
	GtkEntry                *name_entry = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	
	/* Set set snippets store and model */	
	snippets_tree_store = create_snippets_store (snippets_db);
	snippets_tree_view = create_snippets_tree_view (snippets_db, snippets_tree_store);

	dialog_builder = gtk_builder_new ();
	if (!gtk_builder_add_from_file (dialog_builder, EXPORT_UI, &error))
	{
		g_warning ("Couldn't load export ui file: %s", error->message);
		g_error_free (error);
	}

	snippets_export_dialog = GTK_DIALOG (gtk_builder_get_object (dialog_builder, "export_dialog"));
	tree_view_window = GTK_WIDGET (gtk_builder_get_object (dialog_builder, "tree_view_window"));
	
	gtk_container_add (GTK_CONTAINER (tree_view_window), snippets_tree_view);
	gtk_widget_show (snippets_tree_view);
	
	const gchar *name = NULL;
	gchar *uri = NULL, *path = NULL;
	folder_selector_button = GTK_FILE_CHOOSER_BUTTON (gtk_builder_get_object (dialog_builder, "folder_selector"));
	name_entry = GTK_ENTRY (gtk_builder_get_object (dialog_builder, "name_entry"));

	while (TRUE)
	{
		/* Run the dialog until cancel or export */
		export_dialog_result = gtk_dialog_run (GTK_DIALOG (snippets_export_dialog));
		if (export_dialog_result == GTK_RESPONSE_ACCEPT)
		{
			g_free (uri);
			g_free (path);
			/* Save button was pressed, export snippets */
			name = gtk_entry_get_text (name_entry);
			uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (folder_selector_button));
			
			if (!g_strcmp0 (name, ""))
				/* Void name, re-run the export dialog */
				continue;

			if (g_strrstr (name, "."))
				uri = g_strconcat (uri,  "/", name, NULL);
			else
				uri = g_strconcat (uri, "/", name, ".anjuta-snippets", NULL);

			path = anjuta_util_get_local_path_from_uri (uri);
	
			if (!save_snippets_to_path (snippets_tree_store, path, FALSE))
			{
				/* Filename is conflicting */
				file_conflict_dialog = GTK_DIALOG (gtk_message_dialog_new (GTK_WINDOW (snippets_export_dialog),
				                                                           GTK_DIALOG_MODAL,
				                                                           GTK_MESSAGE_ERROR,
				                                                           GTK_BUTTONS_YES_NO,
				                                                           "Path %s exists. Overwrite?", path));

				if (gtk_dialog_run (file_conflict_dialog) == GTK_RESPONSE_YES)
				{
					/* Overwrite file */
					save_snippets_to_path (snippets_tree_store, path, TRUE);
					gtk_widget_destroy (GTK_WIDGET (file_conflict_dialog));
					break;
				}
				else
				{
					/* Re-run the export dialog */
					gtk_widget_destroy (GTK_WIDGET (file_conflict_dialog));
					continue;
				}
	
			}
			else
			{
				/* Snippets were saved */
				break;
			}
		}
		else
		{
			/* Save cancelled */
			break;
		}
	}

	gtk_widget_destroy (GTK_WIDGET (snippets_export_dialog));
	g_free (path);
	g_free (uri);
	gtk_tree_model_foreach (GTK_TREE_MODEL (snippets_tree_store),
	                        snippets_store_unref_foreach_func,
	                        NULL);
	g_object_unref (dialog_builder);
	g_object_unref (snippets_tree_store);	
}
