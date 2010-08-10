/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-db.c
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

#include "snippets-db.h"
#include "snippets-xml-parser.h"
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-document.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-language.h>
#include <libanjuta/interfaces/ianjuta-language.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#define DEFAULT_SNIPPETS_FILE               "snippets.anjuta-snippets"
#define DEFAULT_GLOBAL_VARS_FILE            "snippets-global-variables.xml"
#define USER_SNIPPETS_DB_DIR                "snippets-database"

#define SNIPPETS_DB_MODEL_DEPTH             2

/* Internal global variables */
#define GLOBAL_VAR_FILE_NAME       "filename"
#define GLOBAL_VAR_USER_NAME       "username"
#define GLOBAL_VAR_USER_FULL_NAME  "userfullname"
#define GLOBAL_VAR_HOST_NAME       "hostname"

static gchar *default_files[] = {
	DEFAULT_SNIPPETS_FILE,
	DEFAULT_GLOBAL_VARS_FILE
};


#define ANJUTA_SNIPPETS_DB_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ANJUTA_TYPE_SNIPPETS_DB, SnippetsDBPrivate))

/**
 * SnippetsDBPrivate:
 * @snippets_groups: A #GList where the #AnjutaSnippetsGroup objects are loaded.
 * @snippet_keys_map: A #GHashTable with strings representing the snippet-key as keys and pointers
 *                    to #AnjutaSnippet objects as values.
 *                    Important: One should not try to delete anything. The #GHashTable was 
 *                    constructed with destroy functions passed to the #GHashTable that will 
 *                    free the memory.
 * @global_variables: A #GtkListStore where the static and command-based global variables are stored.
 *                    See snippets-db.h for details about columns.
 *                    Important: Only static and command-based global variables are stored here!
 *                    The internal global variables are computed when #snippets_db_get_global_variable
 *                    is called.
 *
 * The private field for the SnippetsDB object.
 */
struct _SnippetsDBPrivate
{	
	GList* snippets_groups;

	GHashTable* snippet_keys_map;
	
	GtkListStore* global_variables;
};


/* GObject methods declaration */
static void              snippets_db_dispose         (GObject* obj);

static void              snippets_db_finalize        (GObject* obj);

static void              snippets_db_class_init      (SnippetsDBClass* klass);

static void              snippets_db_init            (SnippetsDB *snippets_db);


/* GtkTreeModel methods declaration */
static void              snippets_db_tree_model_init (GtkTreeModelIface *iface);

static GtkTreeModelFlags snippets_db_get_flags       (GtkTreeModel *tree_model);

static gint              snippets_db_get_n_columns   (GtkTreeModel *tree_model);

static GType             snippets_db_get_column_type (GtkTreeModel *tree_model,
                                                      gint index);

static gboolean          snippets_db_get_iter        (GtkTreeModel *tree_model,
                                                      GtkTreeIter *iter,
                                                      GtkTreePath *path);

static GtkTreePath*      snippets_db_get_path        (GtkTreeModel *tree_model,
                                                      GtkTreeIter *iter);

static void              snippets_db_get_value       (GtkTreeModel *tree_model,
                                                      GtkTreeIter *iter,
                                                      gint column,
                                                      GValue *value);

static gboolean          snippets_db_iter_next       (GtkTreeModel *tree_model,
                                                      GtkTreeIter *iter);

static gboolean          snippets_db_iter_children   (GtkTreeModel *tree_model,
                                                      GtkTreeIter *iter,
                                                      GtkTreeIter *parent);

static gboolean          snippets_db_iter_has_child  (GtkTreeModel *tree_model,
                                                      GtkTreeIter  *iter);

static gint              snippets_db_iter_n_children (GtkTreeModel *tree_model,
                                                      GtkTreeIter *iter);

static gboolean          snippets_db_iter_nth_child  (GtkTreeModel *tree_model,
                                                      GtkTreeIter *iter,
                                                      GtkTreeIter *parent,
                                                      gint n);

static gboolean          snippets_db_iter_parent     (GtkTreeModel *tree_model,
                                                      GtkTreeIter *iter,
                                                      GtkTreeIter *child);


G_DEFINE_TYPE_WITH_CODE (SnippetsDB, snippets_db, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
                                                snippets_db_tree_model_init));

/* SnippetsDB private methods */

static GtkTreePath *
get_tree_path_for_snippets_group (SnippetsDB *snippets_db,
                                  AnjutaSnippetsGroup *snippets_group);
static GtkTreePath *
get_tree_path_for_snippet (SnippetsDB *snippets_db,
                           AnjutaSnippet *snippet);

static gchar *
get_snippet_key_from_trigger_and_language (const gchar *trigger_key,
                                           const gchar *language)
{
	gchar *snippet_key = NULL;
	
	/* Assertions */
	g_return_val_if_fail (trigger_key != NULL, NULL);

	snippet_key = g_strconcat (trigger_key, ".", language, NULL);

	return snippet_key;
}

static void
add_snippet_to_hash_table (SnippetsDB *snippets_db,
                           AnjutaSnippet *snippet)
{
	GList *iter = NULL, *languages = NULL;
	gchar *snippet_key = NULL;
	const gchar *trigger_key = NULL, *lang = NULL;
	SnippetsDBPrivate *priv = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	g_return_if_fail (ANJUTA_IS_SNIPPET (snippet));
	priv = ANJUTA_SNIPPETS_DB_GET_PRIVATE (snippets_db);
	
	languages = (GList *)snippet_get_languages (snippet);
	trigger_key = snippet_get_trigger_key (snippet);

	for (iter = g_list_first (languages); iter != NULL; iter = g_list_next (iter))
	{
		lang = (const gchar *)iter->data;
		
		snippet_key = get_snippet_key_from_trigger_and_language (trigger_key, lang);

		g_hash_table_insert (priv->snippet_keys_map, snippet_key, snippet);

	}

}

static void
remove_snippet_from_hash_table (SnippetsDB *snippets_db,
                                AnjutaSnippet *snippet)
{
	GList *languages = NULL, *iter = NULL;
	gchar *cur_language = NULL, *cur_snippet_key = NULL, *trigger_key = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	g_return_if_fail (ANJUTA_IS_SNIPPET (snippet));
	
	languages = (GList *)snippet_get_languages (snippet);
	trigger_key = (gchar *)snippet_get_trigger_key (snippet);
	
	for (iter = g_list_first (languages); iter != NULL; iter = g_list_next (iter))
	{
		cur_language = (gchar *)iter->data;
		cur_snippet_key = get_snippet_key_from_trigger_and_language (trigger_key, cur_language);

		if (cur_snippet_key == NULL)
			continue;

		g_hash_table_remove (snippets_db->priv->snippet_keys_map, cur_snippet_key);
	}
}

static void
remove_snippets_group_from_hash_table (SnippetsDB *snippets_db,
                                       AnjutaSnippetsGroup *snippets_group)
{
	GList *snippets = NULL, *iter = NULL;
	AnjutaSnippet *cur_snippet = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	g_return_if_fail (ANJUTA_IS_SNIPPETS_GROUP (snippets_group));

	snippets = (GList *)snippets_group_get_snippets_list (snippets_group);

	/* Iterate over all the snippets in the group, and remove all the snippet keys
	   a snippet has stored. */
	for (iter = g_list_first (snippets); iter != NULL; iter = g_list_next (iter))
	{
		cur_snippet = (AnjutaSnippet *)iter->data;
		g_return_if_fail (ANJUTA_IS_SNIPPET (cur_snippet));

		remove_snippet_from_hash_table (snippets_db, cur_snippet);
	}
}

static void
copy_default_files_to_user_folder (SnippetsDB *snippets_db)
{
	/* In this function we should copy the default snippets file and the global variables
	   files in the user folder if there aren't already files with the same name in that
	   folder */
	gchar *cur_user_path = NULL, *cur_installation_path = NULL,
	      *user_snippets_db_path = NULL;
	GFile *cur_user_file = NULL, *cur_installation_file = NULL;
	gboolean cur_file_exists = FALSE, copy_success = FALSE;
	gint i = 0;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));

	/* Compute the user_snippets_db file paths */
	user_snippets_db_path = anjuta_util_get_user_data_file_path (USER_SNIPPETS_DB_DIR, "/", NULL);

	for (i = 0; i < G_N_ELEMENTS (default_files); i ++)
	{
		cur_user_path         = g_strconcat (user_snippets_db_path, "/", default_files[i], NULL);
		cur_installation_path = g_strconcat (PACKAGE_DATA_DIR, "/", default_files[i], NULL);

		cur_file_exists = g_file_test (cur_user_path, G_FILE_TEST_EXISTS);
		if (!cur_file_exists)
		{
			cur_installation_file = g_file_new_for_path (cur_installation_path);
			cur_user_file         = g_file_new_for_path (cur_user_path);

			copy_success = g_file_copy (cur_installation_file, cur_user_file,
			                            G_FILE_COPY_NONE,
			                            NULL, NULL, NULL, NULL);

			if (!copy_success)
				DEBUG_PRINT ("Copying of %s failed.", default_files[i]);

			g_object_unref (cur_installation_file);
			g_object_unref (cur_user_file);
		}

		g_free (cur_user_path);
		g_free (cur_installation_path);
	}

	g_free (user_snippets_db_path);

}

static void
load_internal_global_variables (SnippetsDB *snippets_db)
{
	GtkTreeIter iter_added;
	GtkListStore *global_vars_store = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	g_return_if_fail (snippets_db->priv != NULL);
	g_return_if_fail (GTK_IS_LIST_STORE (snippets_db->priv->global_variables));
	global_vars_store = snippets_db->priv->global_variables;

	/* Add the filename global variable */
	gtk_list_store_prepend (global_vars_store, &iter_added);
	gtk_list_store_set (global_vars_store, &iter_added,
	                    GLOBAL_VARS_MODEL_COL_NAME, GLOBAL_VAR_FILE_NAME,
	                    GLOBAL_VARS_MODEL_COL_VALUE, "",
	                    GLOBAL_VARS_MODEL_COL_IS_COMMAND, FALSE,
	                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, TRUE,
	                    -1);

	/* Add the username global variable */
	gtk_list_store_prepend (global_vars_store, &iter_added);
	gtk_list_store_set (global_vars_store, &iter_added,
	                    GLOBAL_VARS_MODEL_COL_NAME, GLOBAL_VAR_USER_NAME,
	                    GLOBAL_VARS_MODEL_COL_VALUE, "",
	                    GLOBAL_VARS_MODEL_COL_IS_COMMAND, FALSE,
	                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, TRUE,
	                    -1);

	/* Add the userfullname global variable */
	gtk_list_store_prepend (global_vars_store, &iter_added);
	gtk_list_store_set (global_vars_store, &iter_added,
	                    GLOBAL_VARS_MODEL_COL_NAME, GLOBAL_VAR_USER_FULL_NAME,
	                    GLOBAL_VARS_MODEL_COL_VALUE, "",
	                    GLOBAL_VARS_MODEL_COL_IS_COMMAND, FALSE,
	                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, TRUE,
	                    -1);

	/* Add the hostname global variable*/
	gtk_list_store_prepend (global_vars_store, &iter_added);
	gtk_list_store_set (global_vars_store, &iter_added,
	                    GLOBAL_VARS_MODEL_COL_NAME, GLOBAL_VAR_HOST_NAME,
	                    GLOBAL_VARS_MODEL_COL_VALUE, "",
	                    GLOBAL_VARS_MODEL_COL_IS_COMMAND, FALSE,
	                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, TRUE,
	                    -1);
}

static void
load_global_variables (SnippetsDB *snippets_db)
{
	gchar *global_vars_user_path = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));

	/* Load the internal global variables */
	load_internal_global_variables (snippets_db);

	global_vars_user_path = 
		anjuta_util_get_user_data_file_path (USER_SNIPPETS_DB_DIR, "/", 
		                                     DEFAULT_GLOBAL_VARS_FILE, NULL);
	snippets_manager_parse_variables_xml_file (global_vars_user_path, snippets_db);

	g_free (global_vars_user_path);	
}

static void
load_snippets (SnippetsDB *snippets_db)
{
	gchar *user_snippets_path;
	GList *snippets_groups = NULL, *iter = NULL;
	AnjutaSnippetsGroup *snippets_group = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));

	user_snippets_path = 
		anjuta_util_get_user_data_file_path (USER_SNIPPETS_DB_DIR, "/", 
		                                     DEFAULT_SNIPPETS_FILE, NULL);

	snippets_groups = snippets_manager_parse_snippets_xml_file (user_snippets_path, NATIVE_FORMAT);

	for (iter = g_list_first (snippets_groups); iter != NULL; iter = g_list_next (iter))
	{
		snippets_group = ANJUTA_SNIPPETS_GROUP (iter->data);
		if (!ANJUTA_IS_SNIPPETS_GROUP (snippets_group))
			continue;

		snippets_db_add_snippets_group (snippets_db, snippets_group, TRUE);
	}

	g_free (user_snippets_path);
}

static gchar*
get_internal_global_variable_value (AnjutaShell *shell,
                                    const gchar* variable_name)
{
	/* Assertions */
	g_return_val_if_fail (variable_name != NULL, NULL);

	/* Check manually what variable is requested */
	if (!g_strcmp0 (variable_name, GLOBAL_VAR_FILE_NAME))
	{
		IAnjutaDocumentManager *anjuta_docman = NULL;
		IAnjutaDocument *anjuta_cur_doc = NULL;

		anjuta_docman = anjuta_shell_get_interface (shell,
		                                            IAnjutaDocumentManager,
		                                            NULL);
		if (anjuta_docman)
		{
			anjuta_cur_doc = ianjuta_document_manager_get_current_document (anjuta_docman, NULL);
			if (!anjuta_cur_doc)
				return g_strdup ("");

			return g_strdup (ianjuta_document_get_filename (anjuta_cur_doc, NULL));
		}
		else
			return g_strdup ("");
	}
	
	if (!g_strcmp0 (variable_name, GLOBAL_VAR_USER_NAME))
		return g_strdup (g_get_user_name ());

	if (!g_strcmp0 (variable_name, GLOBAL_VAR_USER_FULL_NAME))
		return g_strdup (g_get_real_name ());

	if (!g_strcmp0 (variable_name, GLOBAL_VAR_HOST_NAME))
		return g_strdup (g_get_host_name ());
	
	return NULL;
}

static GtkTreeIter*
get_iter_at_global_variable_name (GtkListStore *global_vars_store,
                                  const gchar *variable_name)
{
	GtkTreeIter iter;
	gchar *stored_name = NULL;
	gboolean iter_is_set = FALSE;
	
	/* Assertions */
	g_return_val_if_fail (GTK_IS_LIST_STORE (global_vars_store), NULL);

	iter_is_set = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (global_vars_store), &iter);
	while (iter_is_set) 
	{
		gtk_tree_model_get (GTK_TREE_MODEL (global_vars_store), &iter,
		                    GLOBAL_VARS_MODEL_COL_NAME, &stored_name, 
		                    -1);

		/* If we found the name in the database */
		if (!g_strcmp0 (stored_name, variable_name) )
		{
			g_free (stored_name);
			return gtk_tree_iter_copy (&iter);
		}

		iter_is_set = gtk_tree_model_iter_next (GTK_TREE_MODEL (global_vars_store), &iter);
		g_free (stored_name);
	}
	
	return NULL;
}

static gint
compare_snippets_groups_by_name (gconstpointer a,
                                 gconstpointer b)
{
	AnjutaSnippetsGroup *group1 = (AnjutaSnippetsGroup *)a;
	AnjutaSnippetsGroup *group2 = (AnjutaSnippetsGroup *)b;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_GROUP (group1), 0);
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_GROUP (group2), 0);

	return g_utf8_collate (snippets_group_get_name (group1),
	                       snippets_group_get_name (group2));
}

static void
snippets_db_dispose (GObject* obj)
{
	/* Important: This does not free the memory in the internal structures. You first
	   must use snippets_db_close before disposing the snippets-database. */
	SnippetsDB *snippets_db = NULL;
	
	DEBUG_PRINT ("%s", "Disposing SnippetsDB …");

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	snippets_db = ANJUTA_SNIPPETS_DB (obj);
	g_return_if_fail (snippets_db->priv != NULL);
	
	g_list_free (snippets_db->priv->snippets_groups);
	g_hash_table_destroy (snippets_db->priv->snippet_keys_map);

	snippets_db->priv->snippets_groups   = NULL;
	snippets_db->priv->snippet_keys_map  = NULL;
	
	G_OBJECT_CLASS (snippets_db_parent_class)->dispose (obj);
}

static void
snippets_db_finalize (GObject* obj)
{
	DEBUG_PRINT ("%s", "Finalizing SnippetsDB …");
	
	G_OBJECT_CLASS (snippets_db_parent_class)->finalize (obj);
}

static void
snippets_db_class_init (SnippetsDBClass* klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	snippets_db_parent_class = g_type_class_peek_parent (klass);
	object_class->dispose = snippets_db_dispose;
	object_class->finalize = snippets_db_finalize;
	g_type_class_add_private (klass, sizeof (SnippetsDBPrivate));
}

static void
snippets_db_tree_model_init (GtkTreeModelIface *iface)
{
	iface->get_flags       = snippets_db_get_flags;
	iface->get_n_columns   = snippets_db_get_n_columns;
	iface->get_column_type = snippets_db_get_column_type;
	iface->get_iter        = snippets_db_get_iter;
	iface->get_path        = snippets_db_get_path;
	iface->get_value       = snippets_db_get_value;
	iface->iter_next       = snippets_db_iter_next;
	iface->iter_children   = snippets_db_iter_children;
	iface->iter_has_child  = snippets_db_iter_has_child;
	iface->iter_n_children = snippets_db_iter_n_children;
	iface->iter_nth_child  = snippets_db_iter_nth_child;
	iface->iter_parent     = snippets_db_iter_parent;
}

static void
snippets_db_init (SnippetsDB *snippets_db)
{
	SnippetsDBPrivate* priv = ANJUTA_SNIPPETS_DB_GET_PRIVATE (snippets_db);
	snippets_db->priv = priv;
	snippets_db->anjuta_shell = NULL;
	snippets_db->stamp = g_random_int ();

	/* Initialize the private fields */
	snippets_db->priv->snippets_groups = NULL;
	snippets_db->priv->snippet_keys_map = g_hash_table_new_full (g_str_hash, 
	                                                             g_str_equal, 
	                                                             g_free, 
	                                                             NULL);
	snippets_db->priv->global_variables = gtk_list_store_new (GLOBAL_VARS_MODEL_COL_N,
	                                                          G_TYPE_STRING,
	                                                          G_TYPE_STRING,
	                                                          G_TYPE_BOOLEAN,
	                                                          G_TYPE_BOOLEAN);
}

/* SnippetsDB public methods */

/**
 * snippets_db_new:
 *
 * A new #SnippetDB object.
 *
 * Returns: A new #SnippetsDB object.
 **/
SnippetsDB*	
snippets_db_new ()
{
	return ANJUTA_SNIPPETS_DB (g_object_new (snippets_db_get_type (), NULL));
}

/**
 * snippets_db_load:
 * @snippets_db: A #SnippetsDB object
 *
 * Loads the given @snippets_db with snippets/global-variables loaded from the default
 * folder.
 */
void                       
snippets_db_load (SnippetsDB *snippets_db)
{
	gchar *user_snippets_path = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));

	/* Make sure we have the user folder */
	user_snippets_path = anjuta_util_get_user_data_file_path (USER_SNIPPETS_DB_DIR, "/", NULL);
	g_mkdir_with_parents (user_snippets_path, 0755);

	/* Check if the default snippets file is in the user directory and copy them
	   over from the installation folder if they aren't*/
	copy_default_files_to_user_folder (snippets_db);

	/* Load the snippets and global variables */
	load_global_variables (snippets_db);
	load_snippets (snippets_db);
}

/**
 * snippets_db_close:
 * @snippets_db: A #SnippetsDB object.
 *
 * Saves the snippets and free's the loaded data from the internal structures (not the 
 * internal structures themselvs, so after calling snippets_db_load, the snippets_db
 * will be functional).
 */
void                       
snippets_db_close (SnippetsDB *snippets_db)
{
	GList *iter = NULL;
	AnjutaSnippetsGroup *cur_snippets_group = NULL;
	SnippetsDBPrivate *priv = NULL;
	GtkTreePath *path = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	g_return_if_fail (snippets_db->priv != NULL);
	priv = ANJUTA_SNIPPETS_DB_GET_PRIVATE (snippets_db);

	/* Free the memory for the snippets-groups in the SnippetsDB */
	for (iter = g_list_first (priv->snippets_groups); iter != NULL; iter = g_list_next (iter))
	{
		cur_snippets_group = (AnjutaSnippetsGroup *)iter->data;
		g_return_if_fail (ANJUTA_IS_SNIPPETS_GROUP (cur_snippets_group));

		/* Emit the signal that the snippet was deleted */
		path = get_tree_path_for_snippets_group (snippets_db, cur_snippets_group);
		gtk_tree_model_row_deleted (GTK_TREE_MODEL (snippets_db), path);
		gtk_tree_path_free (path);

		g_object_unref (cur_snippets_group);

	}
	g_list_free (priv->snippets_groups);
	priv->snippets_groups = NULL;

	/* Unload the global variables */
	gtk_list_store_clear (priv->global_variables);

	/* Free the hash-table memory */
	g_hash_table_ref (priv->snippet_keys_map);
	g_hash_table_destroy (priv->snippet_keys_map);

}

void
snippets_db_debug (SnippetsDB *snippets_db)
{
	SnippetsDBPrivate *priv = ANJUTA_SNIPPETS_DB_GET_PRIVATE (snippets_db);
	GList *iter = NULL, *iter2 = NULL;

	for (iter = g_list_first (priv->snippets_groups); iter != NULL; iter = g_list_next (iter))
	{
		if (ANJUTA_IS_SNIPPETS_GROUP (iter->data))
		{
			AnjutaSnippetsGroup *group = ANJUTA_SNIPPETS_GROUP (iter->data);
			printf ("%s\n", snippets_group_get_name (group));
			for (iter2 = g_list_first (snippets_group_get_snippets_list (group)); iter2 != NULL; iter2 = g_list_next (iter2))
			{
				if (ANJUTA_IS_SNIPPET (iter2->data))
				{
					AnjutaSnippet *s = ANJUTA_SNIPPET (iter2->data);
					printf ("\t[%s | %s | %s]\n", snippet_get_name (s), snippet_get_trigger_key (s), snippet_get_languages_string (s));
				}
				else
					printf ("\t(Invalid snippet)\n");
			}
		}
		else
			printf ("(Invalid Snippets Group)\n");
	}
}

/**
 * snippets_db_get_path_at_object:
 * @snippets_db: A #SnippetsDB object.
 * @obj: An #AnjutaSnippetsGroup or #AnjutaSnippet object.
 * 
 * Gets the GtkTreePath at given snippet or snippets group.
 *
 * Returns: The GtkTreePath or NULL if the given object was not found.
 */
GtkTreePath *
snippets_db_get_path_at_object (SnippetsDB *snippets_db,
                                GObject *obj)
{

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), NULL);

	if (ANJUTA_IS_SNIPPET (obj))
		return get_tree_path_for_snippet (snippets_db, ANJUTA_SNIPPET (obj));

	if (ANJUTA_IS_SNIPPETS_GROUP (obj))
		return get_tree_path_for_snippets_group (snippets_db, ANJUTA_SNIPPETS_GROUP (obj));

	g_return_val_if_reached (NULL);
}


void
snippets_db_save_snippets (SnippetsDB *snippets_db)
{
	SnippetsDBPrivate *priv = NULL;
	gchar *user_file_path = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	priv = ANJUTA_SNIPPETS_DB_GET_PRIVATE (snippets_db);

	user_file_path = 
		anjuta_util_get_user_data_file_path (USER_SNIPPETS_DB_DIR, "/", 
		                                     DEFAULT_SNIPPETS_FILE, NULL);

	snippets_manager_save_snippets_xml_file (NATIVE_FORMAT, priv->snippets_groups, user_file_path);

	g_free (user_file_path);
}

void
snippets_db_save_global_vars (SnippetsDB *snippets_db)
{
	SnippetsDBPrivate *priv = NULL;
	gchar *user_file_path = NULL;
	GList *vars_names = NULL, *vars_values = NULL, *vars_comm = NULL, *l_iter = NULL;
	GtkTreeIter iter;
	gchar *name = NULL, *value = NULL;
	gboolean is_command = FALSE, is_internal = FALSE;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	priv = ANJUTA_SNIPPETS_DB_GET_PRIVATE (snippets_db);

	user_file_path =
		anjuta_util_get_user_data_file_path (USER_SNIPPETS_DB_DIR, "/",
		                                     DEFAULT_GLOBAL_VARS_FILE, NULL);

	if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->global_variables), &iter))
		return;

	do
	{
		gtk_tree_model_get (GTK_TREE_MODEL (priv->global_variables), &iter,
		                    GLOBAL_VARS_MODEL_COL_NAME, &name,
		                    GLOBAL_VARS_MODEL_COL_VALUE, &value,
		                    GLOBAL_VARS_MODEL_COL_IS_COMMAND, &is_command,
		                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, &is_internal,
		                    -1);

		if (!is_internal)
		{
			vars_names  = g_list_append (vars_names, name);
			vars_values = g_list_append (vars_values, value);
			vars_comm   = g_list_append (vars_comm, GINT_TO_POINTER (is_command));
		}

	} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->global_variables), &iter));

	snippets_manager_save_variables_xml_file (user_file_path, vars_names, vars_values, vars_comm);

	/* Free the data */
	for (l_iter = g_list_first (vars_names); l_iter != NULL; l_iter = g_list_next (l_iter))
		g_free (l_iter->data);
	g_list_free (vars_names);
	for (l_iter = g_list_first (vars_values); l_iter != NULL; l_iter = g_list_next (l_iter))
		g_free (l_iter->data);
	g_list_free (vars_values);
	g_list_free (vars_comm);
	g_free (user_file_path);
}

/**
 * snippets_db_add_snippet:
 * @snippets_db: A #SnippetsDB object
 * @added_snippet: A #Snippet object
 * @group_name: The name of the group where the snippet should be added.
 * 
 * Adds the @added_snippet to the @snippets_db. If the user is conflicting, it will
 * fail (or if the group wasn't found).
 *
 * Returns: TRUE on success.
 **/
gboolean	
snippets_db_add_snippet (SnippetsDB* snippets_db,
                         AnjutaSnippet* added_snippet,
                         const gchar* group_name)
{
	GList *iter = NULL;
	AnjutaSnippetsGroup *cur_snippets_group = NULL;
	const gchar *cur_snippets_group_name = NULL;
	GtkTreePath *path;
	GtkTreeIter tree_iter;
	SnippetsDBPrivate *priv = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (added_snippet), FALSE);
	priv = ANJUTA_SNIPPETS_DB_GET_PRIVATE (snippets_db);

	/* Check that the snippet is not conflicting */
	if (snippets_db_has_snippet (snippets_db, added_snippet))
		return FALSE;

	/* Lookup the AnjutaSnippetsGroup with the given group_name */
	for (iter = g_list_first (priv->snippets_groups); iter != NULL; iter = g_list_next (iter))
	{
		cur_snippets_group = ANJUTA_SNIPPETS_GROUP (iter->data);
		g_return_val_if_fail (ANJUTA_IS_SNIPPETS_GROUP (cur_snippets_group), FALSE);
		cur_snippets_group_name = snippets_group_get_name (cur_snippets_group);

		/* We found the group */
		if (!g_strcmp0 (cur_snippets_group_name, group_name))
		{
			/* Add the snippet to the group */
			snippets_group_add_snippet (cur_snippets_group, added_snippet);

			/* Add to the Hashtable */
			add_snippet_to_hash_table (snippets_db, added_snippet);

			/* Emit the signal that the database was changed */
			path = get_tree_path_for_snippet (snippets_db, added_snippet);
			snippets_db_get_iter (GTK_TREE_MODEL (snippets_db), &tree_iter, path);
			gtk_tree_model_row_inserted (GTK_TREE_MODEL (snippets_db), path, &tree_iter);
			gtk_tree_path_free (path);
			return TRUE;
		}
	}
	
	return FALSE;
}


gboolean
snippets_db_has_snippet (SnippetsDB *snippets_db,
                         AnjutaSnippet *snippet)
{
	GtkTreePath *path = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), FALSE);
	
	path = get_tree_path_for_snippet (snippets_db, snippet);
	if (path != NULL)
	{
		gtk_tree_path_free (path);
		return TRUE;
	}

	return FALSE;
}

/**
 * snippets_db_get_snippet:
 * @snippets_db: A #SnippetsDB object
 * @trigger_key: The trigger-key of the requested snippet
 * @language: The snippet language. NULL for auto-detection.
 *
 * Gets a snippet from the database for the given trigger-key. If language is NULL,
 * it will get the snippet for the current editor language.
 *
 * Returns: The requested snippet (not a copy, should not be freed) or NULL if not found.
 **/
AnjutaSnippet*	
snippets_db_get_snippet (SnippetsDB* snippets_db,
                         const gchar* trigger_key,
                         const gchar* language)
{
	AnjutaSnippet *snippet = NULL;
	gchar *snippet_key = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), NULL);
	g_return_val_if_fail (trigger_key != NULL, NULL);

	/* Get the editor language if not provided */
	if (language == NULL)
	{
		IAnjutaDocumentManager *docman = NULL;
		IAnjutaDocument *doc = NULL;
		IAnjutaEditorLanguage *ieditor_language = NULL;
		IAnjutaLanguage *ilanguage = NULL;
		
		docman = anjuta_shell_get_interface (snippets_db->anjuta_shell, 
		                                     IAnjutaDocumentManager, 
		                                     NULL);
		ilanguage = anjuta_shell_get_interface (snippets_db->anjuta_shell,
		                                        IAnjutaLanguage,
		                                        NULL);
		g_return_val_if_fail (IANJUTA_IS_DOCUMENT_MANAGER (docman), NULL);
		g_return_val_if_fail (IANJUTA_IS_LANGUAGE (ilanguage), NULL);

		/* Get the current doc and make sure it's an editor */
		doc = ianjuta_document_manager_get_current_document (docman, NULL);
		if (!IANJUTA_IS_EDITOR_LANGUAGE (doc))
			return NULL;
		ieditor_language = IANJUTA_EDITOR_LANGUAGE (doc);
		

		language = ianjuta_language_get_name_from_editor (ilanguage, ieditor_language, NULL);
	}

	/* Calculate the snippet-key */
	snippet_key = get_snippet_key_from_trigger_and_language (trigger_key, language);
	if (snippet_key == NULL)
		return NULL;
	
	/* Look up the the snippet in the hashtable */
	snippet = g_hash_table_lookup (snippets_db->priv->snippet_keys_map, snippet_key);

	g_free (snippet_key);
	return snippet;
}

/**
 * snippets_db_remove_snippet:
 * @snippets_db: A #SnippetsDB object.
 * @trigger-key: The snippet to be removed trigger-key.
 * @language: The language of the snippet. This must not be NULL, as it won't take
 *            the document language.
 * @remove_all_languages_support: If this is FALSE, it won't actually remove the snippet,
 *                                but remove the given language support for the snippet.
 *
 * Removes a snippet from the #SnippetDB (or removes it's language support).
 *
 * Returns: TRUE on success.
 **/
gboolean	
snippets_db_remove_snippet (SnippetsDB* snippets_db,
                            const gchar* trigger_key,
                            const gchar* language,
                            gboolean remove_all_languages_support)
{
	AnjutaSnippet *deleted_snippet = NULL;
	AnjutaSnippetsGroup *deleted_snippet_group = NULL;
	gchar *snippet_key = NULL;
	GtkTreePath *path = NULL;
	SnippetsDBPrivate *priv = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);
	priv = ANJUTA_SNIPPETS_DB_GET_PRIVATE (snippets_db);

	/* Get the snippet to be deleted */
	snippet_key = get_snippet_key_from_trigger_and_language (trigger_key, language);
	if (snippet_key == NULL)
		return FALSE;

	deleted_snippet = g_hash_table_lookup (priv->snippet_keys_map, snippet_key);
	g_free (snippet_key);
	if (!ANJUTA_IS_SNIPPET (deleted_snippet))
		return FALSE;

	if (remove_all_languages_support)
	{
		remove_snippet_from_hash_table (snippets_db, deleted_snippet);
	}
	else
	{
		/* We remove just the current language support from the database */
		g_hash_table_remove (priv->snippet_keys_map, snippet_key);
	}

	/* Emit the signal that the snippet was deleted */
	path = get_tree_path_for_snippet (snippets_db, deleted_snippet);
	gtk_tree_model_row_deleted (GTK_TREE_MODEL (snippets_db), path);
	gtk_tree_path_free (path);
	
	/* Remove it from the snippets group */
	deleted_snippet_group = ANJUTA_SNIPPETS_GROUP (deleted_snippet->parent_snippets_group);
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_GROUP (deleted_snippet_group), FALSE);
	snippets_group_remove_snippet (deleted_snippet_group, 
	                               trigger_key, 
	                               language,
	                               remove_all_languages_support);

	return TRUE;
}

/**
 * snippets_db_add_snippets_group:
 * @snippets_db: A #SnippetsDB object
 * @snippets_group: A #AnjutaSnippetsGroup object
 * @overwrite_group: If a #AnjutaSnippetsGroup with the same name exists it will 
 *                   be overwriten.
 *
 * Adds an #AnjutaSnippetsGroup to the database, checking for conflicts. The @snippets_group
 * passed as argument will have it's reference increased by one. The snippets which are conflicting
 * won't be added.
 *
 * Returns: TRUE on success
 **/
gboolean	
snippets_db_add_snippets_group (SnippetsDB* snippets_db,
                                AnjutaSnippetsGroup* snippets_group,
                                gboolean overwrite_group)
{
	AnjutaSnippet *cur_snippet = NULL;
	GList *iter = NULL, *snippets_list = NULL;
	SnippetsDBPrivate *priv = NULL;
	GtkTreeIter tree_iter;
	GtkTreePath *path;
	const gchar *group_name = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_GROUP (snippets_group), FALSE);
	priv = ANJUTA_SNIPPETS_DB_GET_PRIVATE (snippets_db);

	/* If we should overwrite an existing group, we remove it. */
	group_name = snippets_group_get_name (snippets_group);
	if (overwrite_group)
		snippets_db_remove_snippets_group (snippets_db, group_name);
	else 
	if (snippets_db_has_snippets_group_name (snippets_db, group_name))
		return FALSE;

	/* Check for conflicts */
	snippets_list = snippets_group_get_snippets_list (snippets_group);
	for (iter = g_list_first (snippets_list); iter != NULL; iter = g_list_next (iter))
	{
		cur_snippet = ANJUTA_SNIPPET (iter->data);
		if (!ANJUTA_IS_SNIPPET (cur_snippet))
			continue;

		if (snippets_db_has_snippet (snippets_db, cur_snippet))
		{
			snippets_group_remove_snippet (snippets_group,
			                               snippet_get_trigger_key (cur_snippet),
			                               snippet_get_any_language (cur_snippet),
			                               TRUE);
		}
		else
		{
			add_snippet_to_hash_table (snippets_db, cur_snippet);
		}
	}

	/* Add the snippets_group to the database keeping sorted the list by the
	   group name. */
	priv->snippets_groups = g_list_insert_sorted (priv->snippets_groups,
	                                              snippets_group,
	                                              compare_snippets_groups_by_name);
	g_object_ref (snippets_group);

	/* Emit the signal that the database was changed */
	path = get_tree_path_for_snippets_group (snippets_db, snippets_group);
	snippets_db_get_iter (GTK_TREE_MODEL (snippets_db), &tree_iter, path);
	gtk_tree_model_row_inserted (GTK_TREE_MODEL (snippets_db), path, &tree_iter);
	gtk_tree_path_free (path);
	
	return TRUE;
}

/**
 * snippets_db_remove_snippets_group:
 * @snippets_db: A #SnippetsDB object.
 * @group_name: The name of the group to be removed.
 * 
 * Removes a snippet group (and it's containing snippets) from the #SnippetsDB
 *
 * Returns: TRUE on success.
 **/
gboolean	
snippets_db_remove_snippets_group (SnippetsDB* snippets_db,
                                   const gchar* group_name)
{
	GList *iter = NULL;
	AnjutaSnippetsGroup *snippets_group = NULL;
	GtkTreePath *path = NULL;
	SnippetsDBPrivate *priv = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);
	g_return_val_if_fail (group_name != NULL, FALSE);
	priv = ANJUTA_SNIPPETS_DB_GET_PRIVATE (snippets_db);
	
	for (iter = g_list_first (priv->snippets_groups); iter != NULL; iter = g_list_next (iter))
	{
		snippets_group = ANJUTA_SNIPPETS_GROUP (iter->data);
		g_return_val_if_fail (ANJUTA_IS_SNIPPETS_GROUP (snippets_group), FALSE);
	
		if (!g_strcmp0 (group_name, snippets_group_get_name (snippets_group)))
		{
			/* Remove the snippets in the group from the hash-table */
			remove_snippets_group_from_hash_table (snippets_db, snippets_group);

			/* Emit the signal that it was deleted */
			path = get_tree_path_for_snippets_group (snippets_db, snippets_group);
			gtk_tree_model_row_deleted (GTK_TREE_MODEL (snippets_db), path);
			gtk_tree_path_free (path);

			/* Destroy the snippets-group object */
			g_object_unref (snippets_group);

			/* Delete the list node */
			iter->data = NULL;
			priv->snippets_groups = g_list_delete_link (priv->snippets_groups, iter);
			return TRUE;
		}
	}

	return FALSE;
}

/**
 * snippets_db_get_snippets_group:
 * @snippets_db: A #SnippetsDB object.
 * @group_name: The name of the #AnjutaSnippetsGroup requested object.
 *
 * Returns: The requested #AnjutaSnippetsGroup object or NULL on failure.
 */
AnjutaSnippetsGroup*
snippets_db_get_snippets_group (SnippetsDB* snippets_db,
                                const gchar* group_name)
{
	AnjutaSnippetsGroup *snippets_group = NULL;
	SnippetsDBPrivate *priv = NULL;
	GList *iter = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), NULL);

	/* Look up the AnjutaSnippetsGroup object with the name being group_name */
	priv = snippets_db->priv;
	for (iter = g_list_first (priv->snippets_groups); iter != NULL; iter = g_list_next (iter))
	{
		snippets_group = (AnjutaSnippetsGroup *)iter->data;
		if (!g_strcmp0 (snippets_group_get_name (snippets_group), group_name))
		{
			return snippets_group;
		}
	}
	
	return NULL;
}


void
snippets_db_set_snippets_group_name (SnippetsDB *snippets_db,
                                     const gchar *old_group_name,
                                     const gchar *new_group_name)
{
	AnjutaSnippetsGroup *snippets_group = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));

	/* Make sure the new name doesen't cause a conflict */
	if (snippets_db_has_snippets_group_name (snippets_db, new_group_name))
		return;

	snippets_group = snippets_db_get_snippets_group (snippets_db, old_group_name);
	if (!ANJUTA_IS_SNIPPETS_GROUP (snippets_group))
		return;

	/* Remove the group, but don't destroy it and add it with the new name */
	g_object_ref (snippets_group);
	snippets_db_remove_snippets_group (snippets_db, old_group_name);
	snippets_group_set_name (snippets_group, new_group_name);
	snippets_db_add_snippets_group (snippets_db, snippets_group, TRUE);
	g_object_unref (snippets_group);
}

gboolean
snippets_db_has_snippets_group_name (SnippetsDB *snippets_db,
                                     const gchar *group_name)
{
	return ANJUTA_IS_SNIPPETS_GROUP (snippets_db_get_snippets_group (snippets_db, group_name));
}

/**
 * snippets_db_get_global_variable_text:
 * @snippets_db: A #SnippetsDB object.
 * @variable_name: The variable name.
 *
 * Gets the actual text of the variable. If it's a command it will return the command,
 * not the output. If it's static it will have the same result as #snippets_db_get_global_variable.
 * If it's internal it will return an empty string.
 *
 * Returns: The actual text of the global variable.
 */
gchar*
snippets_db_get_global_variable_text (SnippetsDB* snippets_db,
                                      const gchar* variable_name)
{
	GtkTreeIter *iter = NULL;
	GtkListStore *global_vars_store = NULL;
	gboolean is_internal = FALSE;
	gchar *value = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), NULL);
	g_return_val_if_fail (snippets_db->priv != NULL, NULL);
	g_return_val_if_fail (GTK_IS_LIST_STORE (snippets_db->priv->global_variables), NULL);
	global_vars_store = snippets_db->priv->global_variables;

	/* Search for the variable */
	iter = get_iter_at_global_variable_name (global_vars_store, variable_name);
	if (iter)
	{
		/* Check if it's internal or not */
		gtk_tree_model_get (GTK_TREE_MODEL (global_vars_store), iter,
		                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, &is_internal, 
		                    -1);

		/* If it's internal we return an empty string */
		if (is_internal)
		{
			return g_strdup("");
		}
		else
		{
			/* If it's a command we launch that command and return the output */
			gtk_tree_model_get (GTK_TREE_MODEL (global_vars_store), iter,
				                GLOBAL_VARS_MODEL_COL_VALUE, &value, 
				                -1);
			return value;
		}
	}

	return NULL;

}

/**
 * snippets_db_get_global_variable:
 * @snippets_db: A #SnippetsDB object.
 * @variable_name: The name of the global variable.
 *
 * Gets the value of a global variable. A global variable value can be static,the output of a 
 * command or internal.
 *
 * Returns: The value of the global variable, or NULL if the variable wasn't found.
 */
gchar* 
snippets_db_get_global_variable (SnippetsDB* snippets_db,
                                 const gchar* variable_name)
{
	GtkTreeIter *iter = NULL;
	GtkListStore *global_vars_store = NULL;
	gboolean is_command = FALSE, is_internal = FALSE, command_success = FALSE;
	gchar *value = NULL, *command_line = NULL, *command_output = NULL, *command_error = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), NULL);
	g_return_val_if_fail (snippets_db->priv != NULL, NULL);
	g_return_val_if_fail (GTK_IS_LIST_STORE (snippets_db->priv->global_variables), NULL);
	global_vars_store = snippets_db->priv->global_variables;

	/* Search for the variable */
	iter = get_iter_at_global_variable_name (global_vars_store, variable_name);
	if (iter)
	{
		/* Check if it's a command/internal or not */
		gtk_tree_model_get (GTK_TREE_MODEL (global_vars_store), iter,
		                    GLOBAL_VARS_MODEL_COL_IS_COMMAND, &is_command, 
		                    -1);
		gtk_tree_model_get (GTK_TREE_MODEL (global_vars_store), iter,
		                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, &is_internal, 
		                    -1);

		/* If it's internal we call a function defined above to compute the value */
		if (is_internal)
		{
			return get_internal_global_variable_value (snippets_db->anjuta_shell,
			                                           variable_name);
		}
		/* If it's a command we launch that command and return the output */
		else if (is_command)
		{
			gtk_tree_model_get (GTK_TREE_MODEL (global_vars_store), iter,
			                    GLOBAL_VARS_MODEL_COL_VALUE, &command_line, 
			                    -1);
			command_success = g_spawn_command_line_sync (command_line,
			                                             &command_output,
			                                             &command_error,
			                                             NULL,
			                                             NULL);
			g_free (command_line);
			g_free (command_error);
			if (command_success)
			{
				/* If the last character is a newline we eliminate it */
				gint command_output_size = 0;
				while (command_output[command_output_size] != 0)
					command_output_size ++;
				if (command_output[command_output_size - 1] == '\n')
					command_output[command_output_size - 1] = 0;
					
				return command_output;
			}
		}
		/* If it's static just return the value stored */
		else
		{
			gtk_tree_model_get (GTK_TREE_MODEL (global_vars_store), iter,
			                    GLOBAL_VARS_MODEL_COL_VALUE, &value, 
			                    -1);
			return value;
		
		}		
	}

	return NULL;
}

/**
 * snippets_db_has_global_variable:
 * @snippets_db: A #SnippetsDB object.
 * @variable_name: A variable name.
 *
 * Checks if the Snippet Database has an entry with a variable name as requested.
 *
 * Returns: TRUE if the global variable exists.
 */
gboolean 
snippets_db_has_global_variable (SnippetsDB* snippets_db,
                                 const gchar* variable_name)
{
	GtkListStore *global_vars_store = NULL;
	GtkTreeIter *iter = NULL;
	gboolean found = FALSE;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);
	g_return_val_if_fail (snippets_db->priv != NULL, FALSE);
	g_return_val_if_fail (GTK_IS_LIST_STORE (snippets_db->priv->global_variables), FALSE);
	global_vars_store = snippets_db->priv->global_variables;
	
	/* Locate the variable in the GtkListStore */
	iter = get_iter_at_global_variable_name (global_vars_store, variable_name);
	found = (iter != NULL);
	if (iter)
		gtk_tree_iter_free (iter);

	return found;
}

/**
 * snippets_db_add_global_variable:
 * @snippets_db: A #SnippetsDB object.
 * @variable_name: A variable name.
 * @variable_value: The global variable value.
 * @variable_is_command: If the variable is the output of a command.
 * @overwrite: If a global variable with the same name exists, it should be overwriten.
 *
 * Adds a global variable to the Snippets Database.
 *
 * Returns: TRUE on success.
 */
gboolean
snippets_db_add_global_variable (SnippetsDB* snippets_db,
                                 const gchar* variable_name,
                                 const gchar* variable_value,
                                 gboolean variable_is_command,
                                 gboolean overwrite)
{
	GtkTreeIter *iter = NULL, iter_to_add;
	GtkListStore *global_vars_store = NULL;
	gboolean is_internal = FALSE;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);
	g_return_val_if_fail (snippets_db->priv != NULL, FALSE);
	g_return_val_if_fail (GTK_IS_LIST_STORE (snippets_db->priv->global_variables), FALSE);
	global_vars_store = snippets_db->priv->global_variables;

	/* Check to see if there is a global variable with the same name in the database */
	iter = get_iter_at_global_variable_name (global_vars_store, variable_name);
	if (iter)
	{
		gtk_tree_model_get (GTK_TREE_MODEL (global_vars_store), iter,
		                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, &is_internal,
		                    -1);

		/* If it's internal it can't be overwriten */
		if (overwrite && !is_internal)
		{
			gtk_list_store_set (global_vars_store, iter,
			                    GLOBAL_VARS_MODEL_COL_NAME, variable_name,
			                    GLOBAL_VARS_MODEL_COL_VALUE, variable_value,
			                    GLOBAL_VARS_MODEL_COL_IS_COMMAND, variable_is_command,
			                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, FALSE,
			                    -1);
			gtk_tree_iter_free (iter);
			return TRUE;	
		}
		else
		{
			gtk_tree_iter_free (iter);
			return FALSE;
		}
	}
	else
	{
		/* Add the the global_vars_store */
		gtk_list_store_append (global_vars_store, &iter_to_add);
		gtk_list_store_set (global_vars_store, &iter_to_add,
		                    GLOBAL_VARS_MODEL_COL_NAME, variable_name,
		                    GLOBAL_VARS_MODEL_COL_VALUE, variable_value,
		                    GLOBAL_VARS_MODEL_COL_IS_COMMAND, variable_is_command,
		                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, FALSE,
		                    -1);
	}
	return TRUE;
}

/**
 * snippets_db_set_global_variable_name:
 * @snippets_db: A #SnippetsDB object.
 * @variable_old_name: The old name of the variable
 * @variable_new_name: The name with which it should be changed.
 *
 * Changed the Global Variable name.
 *
 * Returns: TRUE on success.
 */
gboolean
snippets_db_set_global_variable_name (SnippetsDB* snippets_db,
                                      const gchar* variable_old_name,
                                      const gchar* variable_new_name)
{
	GtkListStore *global_vars_store = NULL;
	GtkTreeIter *iter = NULL;
	gboolean is_internal = FALSE;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);
	g_return_val_if_fail (snippets_db->priv != NULL, FALSE);
	g_return_val_if_fail (GTK_IS_LIST_STORE (snippets_db->priv->global_variables), FALSE);
	global_vars_store = snippets_db->priv->global_variables;

	/* Test if the variable_new_name is already in the database */
	iter = get_iter_at_global_variable_name (global_vars_store, variable_new_name);
	if (iter)
	{
		gtk_tree_iter_free (iter);
		return FALSE;
	}

	/* Get a GtkTreeIter pointing at the global variable to be updated */
	iter = get_iter_at_global_variable_name (global_vars_store, variable_old_name);
	if (iter)
	{
		gtk_tree_model_get (GTK_TREE_MODEL (global_vars_store), iter,
		                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, &is_internal,
		                    -1);
		                    
		if (!is_internal)
		{
			gtk_list_store_set (global_vars_store, iter,
			                    GLOBAL_VARS_MODEL_COL_NAME, variable_new_name,
			                    -1);
			gtk_tree_iter_free (iter);
			return TRUE;
		}
		else
		{
			gtk_tree_iter_free (iter);
			return FALSE;
		}
	}

	return FALSE;	
}

/**
 * snippets_db_set_global_variable_value:
 * @snippets_db: A #SnippetsDB value.
 * @variable_name: The name of the global variable to be updated.
 * @variable_new_value: The new value to be set to the variable.
 *
 * Returns: TRUE on success.
 */
gboolean
snippets_db_set_global_variable_value (SnippetsDB* snippets_db,
                                       const gchar* variable_name,
                                       const gchar* variable_new_value)
{
	GtkListStore *global_vars_store = NULL;
	GtkTreeIter *iter = NULL;
	gboolean is_internal = FALSE;
	gchar *stored_value = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);
	g_return_val_if_fail (snippets_db->priv != NULL, FALSE);
	g_return_val_if_fail (GTK_IS_LIST_STORE (snippets_db->priv->global_variables), FALSE);
	global_vars_store = snippets_db->priv->global_variables;
	
	/* Get a GtkTreeIter pointing at the global variable to be updated */
	iter = get_iter_at_global_variable_name (global_vars_store, variable_name);
	if (iter)
	{
		gtk_tree_model_get (GTK_TREE_MODEL (global_vars_store), iter,
		                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, &is_internal,
		                    -1);
		gtk_tree_model_get (GTK_TREE_MODEL (global_vars_store), iter,
		                    GLOBAL_VARS_MODEL_COL_VALUE, &stored_value,
		                    -1);
		                    
		if (!is_internal)
		{
			gtk_list_store_set (global_vars_store, iter,
			                    GLOBAL_VARS_MODEL_COL_VALUE, variable_new_value,
			                    -1);
			                    
			g_free (stored_value);
			gtk_tree_iter_free (iter);

			return TRUE;
		}
		else
		{
			g_free (stored_value);
			gtk_tree_iter_free (iter);
			
			return FALSE;
		}
	}

	return FALSE;
}

/**
 * snippets_db_set_global_variable_type:
 * @snippets_db: A #SnippetsDB value.
 * @variable_name: The name of the global variable to be updated.
 * @is_command: TRUE if after the update the global variable should be considered a command.
 *
 * Returns: TRUE on success.
 */
gboolean
snippets_db_set_global_variable_type (SnippetsDB *snippets_db,
                                      const gchar* variable_name,
                                      gboolean is_command)
{
	GtkListStore *global_vars_store = NULL;
	GtkTreeIter *iter = NULL;
	gboolean is_internal = FALSE;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);
	g_return_val_if_fail (snippets_db->priv != NULL, FALSE);
	g_return_val_if_fail (GTK_IS_LIST_STORE (snippets_db->priv->global_variables), FALSE);
	global_vars_store = snippets_db->priv->global_variables;
	
	/* Get a GtkTreeIter pointing at the global variable to be updated */
	iter = get_iter_at_global_variable_name (global_vars_store, variable_name);
	if (iter)
	{
		gtk_tree_model_get (GTK_TREE_MODEL (global_vars_store), iter,
		                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, &is_internal,
		                    -1);

		if (!is_internal)
		{
			gtk_list_store_set (global_vars_store, iter,
			                    GLOBAL_VARS_MODEL_COL_IS_COMMAND, is_command,
			                    -1);
			gtk_tree_iter_free (iter);
			return TRUE;
		}
		else
		{
			gtk_tree_iter_free (iter);
			return FALSE;
		}
	}

	return FALSE;
}

/**
 * snippets_db_remove_global_variable:
 * @snippets_db: A #SnippetsDB object
 * @group_name: The name of the global variable to be removed.
 *
 * Removes a global variable from the database. Only works for static or command
 * based variables.
 *
 * Returns: TRUE on success.
 */
gboolean
snippets_db_remove_global_variable (SnippetsDB* snippets_db, 
                                    const gchar* variable_name)
{
	GtkListStore *global_vars_store = NULL;
	GtkTreeIter *iter = NULL;
	gboolean is_internal = FALSE;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);
	g_return_val_if_fail (snippets_db->priv != NULL, FALSE);
	g_return_val_if_fail (GTK_IS_LIST_STORE (snippets_db->priv->global_variables), FALSE);
	global_vars_store = snippets_db->priv->global_variables;
	
	/* Get a GtkTreeIter pointing at the global variable to be removed */
	iter = get_iter_at_global_variable_name (global_vars_store, variable_name);
	if (iter)
	{
		gtk_tree_model_get (GTK_TREE_MODEL (global_vars_store), iter,
		                    GLOBAL_VARS_MODEL_COL_IS_INTERNAL, &is_internal, 
		                    -1);

		if (!is_internal)
		{
			gtk_list_store_remove (global_vars_store, iter);
			gtk_tree_iter_free (iter);
			return TRUE;
		}
		else
		{
			gtk_tree_iter_free (iter);
			return FALSE;
		}
	}
		
	return FALSE;
}

/**
 * snippets_db_get_global_vars_model:
 * snippets_db: A #SnippetsDB object.
 *
 * The returned GtkTreeModel is basically a list with the global variables that
 * should be used for displaying the variables. At the moment, it's used for
 * displaying the variables in the preferences.
 *
 * Returns: The #GtkTreeModel of the global variables list. 
 *          Warning: This isn't a copy, shouldn't be freed or unrefed.
 */
GtkTreeModel*
snippets_db_get_global_vars_model (SnippetsDB* snippets_db)
{
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), NULL);
	g_return_val_if_fail (snippets_db->priv != NULL, NULL);
	g_return_val_if_fail (GTK_IS_LIST_STORE (snippets_db->priv->global_variables), NULL);
	
	return GTK_TREE_MODEL (snippets_db->priv->global_variables);
}

/* GtkTreeModel methods definition */

static GObject *
iter_get_data (GtkTreeIter *iter)
{
	GList *cur_node = NULL;

	g_return_val_if_fail (iter != NULL, NULL);
	if (iter->user_data == NULL)
		return NULL;
	cur_node = (GList *)iter->user_data;
	if (cur_node == NULL)
		return NULL;
	if (!G_IS_OBJECT (cur_node->data))
		return NULL;
		
	return G_OBJECT (cur_node->data);
}

static gboolean
iter_is_snippets_group_node (GtkTreeIter *iter)
{
	GObject *data = iter_get_data (iter);
	
	return ANJUTA_IS_SNIPPETS_GROUP (data);
}

static gboolean
iter_is_snippet_node (GtkTreeIter *iter)
{
	GObject *data = iter_get_data (iter);

	return ANJUTA_IS_SNIPPET (data);
}

static gboolean
iter_get_first_snippets_db_node (GtkTreeIter *iter,
                                 SnippetsDB *snippets_db)
{
	SnippetsDBPrivate *priv = NULL;
	
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), FALSE);
	priv = ANJUTA_SNIPPETS_DB_GET_PRIVATE (snippets_db);

	iter->user_data  = g_list_first (priv->snippets_groups);
	iter->user_data2 = NULL;
	iter->user_data3 = NULL;
	iter->stamp      = snippets_db->stamp;

	return iter->user_data != NULL;
}

static gboolean
iter_nth (GtkTreeIter *iter, gint n)
{
	g_return_val_if_fail (iter != NULL, FALSE);
	
	iter->user_data = g_list_nth ((GList *)iter->user_data, n);

	return (iter->user_data != NULL);
}

static GList *
iter_get_list_node (GtkTreeIter *iter)
{
	g_return_val_if_fail (iter != NULL, NULL);

	return iter->user_data;
}

static GtkTreeModelFlags 
snippets_db_get_flags (GtkTreeModel *tree_model)
{
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (tree_model), (GtkTreeModelFlags)0);

	return GTK_TREE_MODEL_ITERS_PERSIST;
}

static gint
snippets_db_get_n_columns (GtkTreeModel *tree_model)
{
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (tree_model), 0);

	return SNIPPETS_DB_MODEL_COL_N;
}

static GType
snippets_db_get_column_type (GtkTreeModel *tree_model,
                             gint index)
{
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (tree_model), G_TYPE_INVALID);
	g_return_val_if_fail (index >= 0 && index < SNIPPETS_DB_MODEL_COL_N, G_TYPE_INVALID);

	if (index == 0)
		return G_TYPE_OBJECT;
	else
		return G_TYPE_STRING;
}

static gboolean
snippets_db_get_iter (GtkTreeModel *tree_model,
                      GtkTreeIter *iter,
                      GtkTreePath *path)
{
	SnippetsDB *snippets_db = NULL;
	gint *indices = NULL, depth = 0, db_count = 0, group_count = 0;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (tree_model), FALSE);
	g_return_val_if_fail (path != NULL, FALSE);
	snippets_db = ANJUTA_SNIPPETS_DB (tree_model);
	
	/* Get the indices and depth of the path */
	indices = gtk_tree_path_get_indices (path);
	depth   = gtk_tree_path_get_depth (path);
	if (depth > SNIPPETS_DB_MODEL_DEPTH)
		return FALSE;

	db_count = indices[0];
	if (depth == 2)
		group_count = indices[1];

	/* Get the top-level iter with the count being db_count */
	iter_get_first_snippets_db_node (iter, snippets_db);
	if (!iter_nth (iter, db_count))
		return FALSE;

	/* If depth is SNIPPETS_DB_MODEL_DEPTH, get the group_count'th child */
	if (depth == SNIPPETS_DB_MODEL_DEPTH)
		return snippets_db_iter_nth_child (tree_model, iter, iter, group_count);
	
	return TRUE;
}

static GtkTreePath*
snippets_db_get_path (GtkTreeModel *tree_model,
                      GtkTreeIter *iter)
{
	GtkTreePath *path = NULL;
	GtkTreeIter *iter_copy = NULL;
	SnippetsDB *snippets_db = NULL;
	gint count = 0;
	GList *l_iter = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (tree_model), NULL);
	g_return_val_if_fail (iter != NULL, NULL);
	snippets_db = ANJUTA_SNIPPETS_DB (tree_model);

	/* Make a new GtkTreePath object */
	path = gtk_tree_path_new ();

	/* Get the first index */
	l_iter = iter_get_list_node (iter);
	while (l_iter != NULL)
	{
		count ++;
		l_iter = g_list_previous (l_iter);
	}
	gtk_tree_path_append_index (path, count);
	
	/* If it's a snippet node, then it has a parent */
	count = 0;
	if (iter_is_snippet_node (iter))
	{
		iter_copy = gtk_tree_iter_copy (iter);

		snippets_db_iter_parent (tree_model, iter_copy, iter);
		l_iter = iter_get_list_node (iter_copy);
		while (l_iter != NULL)
		{
			count ++;
			l_iter = g_list_previous (l_iter);
		}
		gtk_tree_iter_free (iter);
	}

	return path;
}

static void
snippets_db_get_value (GtkTreeModel *tree_model,
                       GtkTreeIter *iter,
                       gint column,
                       GValue *value)
{
	SnippetsDB *snippets_db = NULL;
	GObject *cur_object = NULL;
	gchar *cur_string = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (tree_model));
	g_return_if_fail (iter != NULL);
	g_return_if_fail (column >= 0 && column < SNIPPETS_DB_MODEL_COL_N);
	snippets_db = ANJUTA_SNIPPETS_DB (tree_model);

	/* Initializations */
	g_value_init (value, snippets_db_get_column_type (tree_model, column));
	cur_object = iter_get_data (iter);
	g_return_if_fail (ANJUTA_IS_SNIPPETS_GROUP (cur_object) || ANJUTA_IS_SNIPPET (cur_object));
	
	/* Get the data in the node */
	switch (column)
	{
		case SNIPPETS_DB_MODEL_COL_CUR_OBJECT:
			g_value_set_object (value, cur_object);
			return;

		case SNIPPETS_DB_MODEL_COL_NAME:
			if (ANJUTA_IS_SNIPPET (cur_object))
				cur_string = g_strdup (snippet_get_name (ANJUTA_SNIPPET (cur_object)));
			else
				cur_string = g_strdup (snippets_group_get_name (ANJUTA_SNIPPETS_GROUP (cur_object)));
			g_value_set_string (value, cur_string);
			return;

		case SNIPPETS_DB_MODEL_COL_TRIGGER:
			if (ANJUTA_IS_SNIPPET (cur_object))
				cur_string = g_strdup (snippet_get_trigger_key (ANJUTA_SNIPPET (cur_object)));
			else
				cur_string = g_strdup ("");
			g_value_set_string (value, cur_string);
			return;
			
		case SNIPPETS_DB_MODEL_COL_LANGUAGES:
			if (ANJUTA_IS_SNIPPET (cur_object))
				cur_string = g_strdup (snippet_get_languages_string (ANJUTA_SNIPPET (cur_object)));
			else
				cur_string = g_strdup ("");
			g_value_set_string (value, cur_string);		
	}
}

static gboolean
snippets_db_iter_next (GtkTreeModel *tree_model,
                       GtkTreeIter *iter)
{
	SnippetsDB* snippets_db = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (tree_model), FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);
	snippets_db = ANJUTA_SNIPPETS_DB (tree_model);

	/* Check if the stamp is correct */
	g_return_val_if_fail (snippets_db->stamp == iter->stamp,
	                      FALSE);

	/* Update the iter and return TRUE if it didn't reached the end*/
	iter->user_data = g_list_next ((GList *)iter->user_data);

	return (iter->user_data != NULL);
}

static gboolean
snippets_db_iter_children (GtkTreeModel *tree_model,
                           GtkTreeIter *iter,
                           GtkTreeIter *parent)
{
	return snippets_db_iter_nth_child (tree_model, iter, parent, 0);
}

static gboolean
snippets_db_iter_has_child (GtkTreeModel *tree_model,
                            GtkTreeIter  *iter)
{
	GList *snippets_list = NULL;
	AnjutaSnippetsGroup *snippets_group = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (tree_model), FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);

	/* If the parent field is NULL then it's the 1st level so it has children */
	if (iter_is_snippets_group_node (iter))
	{
		snippets_group = ANJUTA_SNIPPETS_GROUP (iter_get_data (iter));
		snippets_list = (GList *)snippets_group_get_snippets_list (snippets_group);
		return (g_list_length (snippets_list) != 0);
	}
	else
		return FALSE;
}

static gint
snippets_db_iter_n_children (GtkTreeModel *tree_model,
                             GtkTreeIter *iter)
{
	const GList* snippets_list = NULL;
	SnippetsDB *snippets_db = NULL;
	AnjutaSnippetsGroup *snippets_group = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (tree_model), -1);
	snippets_db = ANJUTA_SNIPPETS_DB (tree_model);
	
	/* If a top-level count is requested */
	if (iter == NULL)
	{
		return (gint)g_list_length (snippets_db->priv->snippets_groups);
	}

	/* If iter points to a SnippetsGroup node */
	if (iter_is_snippets_group_node (iter))
	{
		/* Get the Snippets Group object and assert it */
		snippets_group = ANJUTA_SNIPPETS_GROUP (iter_get_data (iter));
		g_return_val_if_fail (ANJUTA_IS_SNIPPETS_GROUP (snippets_group),
		                      -1);

		snippets_list = snippets_group_get_snippets_list (snippets_group);
		return (gint)g_list_length ((GList *)snippets_list);
	}

	/* If iter points to a Snippet node than it has no children. */
	return 0;
}

static gboolean
snippets_db_iter_nth_child (GtkTreeModel *tree_model,
                            GtkTreeIter *iter,
                            GtkTreeIter *parent,
                            gint n)
{
	SnippetsDB *snippets_db = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (tree_model), FALSE);
	snippets_db = ANJUTA_SNIPPETS_DB (tree_model);
	
	/* If it's a top level request */
	if (parent == NULL)
	{
		iter_get_first_snippets_db_node (iter, snippets_db);
		return iter_nth (iter, n);
	}

	if (iter_is_snippets_group_node (parent))
	{
		AnjutaSnippetsGroup *snippets_group = ANJUTA_SNIPPETS_GROUP (iter_get_data (parent));
		GList *snippets_list = (GList *)snippets_group_get_snippets_list (snippets_group);

		iter->user_data2 = parent->user_data;
		iter->user_data  = g_list_first (snippets_list);
		iter->stamp      = parent->stamp;
		
		return iter_nth (iter, n);
	}

	/* If we got to this point, it's a snippet node, so it doesn't have a child*/
	return FALSE;
}

static gboolean
snippets_db_iter_parent (GtkTreeModel *tree_model,
                         GtkTreeIter *iter,
                         GtkTreeIter *child)
{
	SnippetsDB *snippets_db = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (tree_model), FALSE);
	g_return_val_if_fail (child != NULL, FALSE);
	snippets_db = ANJUTA_SNIPPETS_DB (tree_model);

	/* If it's a snippets group node, it doesn't have a parent */
	if (iter_is_snippets_group_node (child))
		return FALSE;
	
	/* Fill the iter fields */
	iter->user_data  = child->user_data2;
	iter->user_data2 = NULL;
	iter->stamp      = child->stamp;
	
	return TRUE;
}

static GtkTreePath *
get_tree_path_for_snippets_group (SnippetsDB *snippets_db,
                                  AnjutaSnippetsGroup *snippets_group)
{
	GtkTreeIter iter;
	gint index = 0;
	const gchar *group_name = NULL;
	AnjutaSnippetsGroup *cur_group = NULL;
	GtkTreePath *path = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), NULL);
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_GROUP (snippets_group), NULL);
	                      
	group_name = snippets_group_get_name (snippets_group);
	path = gtk_tree_path_new ();
	
	if (!iter_get_first_snippets_db_node (&iter, snippets_db))
		return NULL;
		
	do 
	{
		cur_group = ANJUTA_SNIPPETS_GROUP (iter_get_data (&iter));
		if (!ANJUTA_IS_SNIPPETS_GROUP (cur_group))
		{
			index ++;
			continue;
		}
		
		if (!g_strcmp0 (snippets_group_get_name (cur_group), group_name))
		{
			gtk_tree_path_append_index (path, index);		
			return path;
		}
		index ++;
		
	} while (snippets_db_iter_next (GTK_TREE_MODEL (snippets_db), &iter));

	gtk_tree_path_free (path);
	return NULL;
}

static GtkTreePath *
get_tree_path_for_snippet (SnippetsDB *snippets_db,
                           AnjutaSnippet *snippet)
{
	GtkTreePath *path = NULL;
	gint index1 = 0, index2 = 0;
	GtkTreeIter iter1, iter2;
	AnjutaSnippet *cur_snippet = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), NULL);
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), NULL);

	path = gtk_tree_path_new ();

	if (!iter_get_first_snippets_db_node (&iter1, snippets_db))
		return NULL;

	do
	{
		index2 = 0;
		snippets_db_iter_nth_child (GTK_TREE_MODEL (snippets_db), &iter2, &iter1, 0);
		
		do
		{
			cur_snippet = ANJUTA_SNIPPET (iter_get_data (&iter2));
			if (!ANJUTA_IS_SNIPPET (cur_snippet))
			{
				index2 ++;
				continue;
			}
			if (snippet_is_equal (snippet, cur_snippet))
			{
				gtk_tree_path_append_index (path, index1);
				gtk_tree_path_append_index (path, index2);
				return path;
			}
			
			index2 ++;
			
		} while (snippets_db_iter_next (GTK_TREE_MODEL (snippets_db), &iter2));
		
		index1 ++;
		
	} while (snippets_db_iter_next (GTK_TREE_MODEL (snippets_db), &iter1));

	gtk_tree_path_free (path);
	return NULL;
}
