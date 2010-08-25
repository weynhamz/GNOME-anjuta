/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippet-variables-store.c
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

#include "snippet-variables-store.h"

#define ANJUTA_SNIPPET_VARS_STORE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ANJUTA_TYPE_SNIPPET_VARS_STORE, SnippetVarsStorePrivate))

struct _SnippetVarsStorePrivate
{
	SnippetsDB *snippets_db;
	AnjutaSnippet *snippet;

	/* Handler id's */
	gulong row_inserted_handler_id;
	gulong row_changed_handler_id;
	gulong row_deleted_handler_id;
	
};

G_DEFINE_TYPE (SnippetVarsStore, snippet_vars_store, GTK_TYPE_LIST_STORE);

static void
snippet_vars_store_init (SnippetVarsStore *snippet_vars_store)
{
	SnippetVarsStorePrivate *priv = ANJUTA_SNIPPET_VARS_STORE_GET_PRIVATE (snippet_vars_store);
	
	snippet_vars_store->priv = priv;

	/* Initialize the private field */
	priv->snippets_db = NULL;
	priv->snippet     = NULL;
	
}

static void
snippet_vars_store_class_init (SnippetVarsStoreClass *snippet_vars_store_class)
{
	snippet_vars_store_parent_class = g_type_class_peek_parent (snippet_vars_store_class);

	g_type_class_add_private (snippet_vars_store_class, sizeof (SnippetVarsStorePrivate));
}

/**
 * snippet_vars_store_new:
 *
 * Returns: A new #SnippetVarsStore object with empty fields, but with columns
 *          initialized.
 */
SnippetVarsStore* 
snippet_vars_store_new ()
{
	GObject* obj = g_object_new (snippet_vars_store_get_type (), NULL);
	SnippetVarsStore *vars_store = ANJUTA_SNIPPET_VARS_STORE (obj);
	GType types[VARS_STORE_COL_N] = {G_TYPE_STRING, 
	                                 G_TYPE_UINT, 
	                                 G_TYPE_STRING,
	                                 G_TYPE_STRING,
	                                 G_TYPE_BOOLEAN, 
	                                 G_TYPE_BOOLEAN};

	gtk_list_store_set_column_types (GTK_LIST_STORE (vars_store),
	                                 VARS_STORE_COL_N, types);

	return vars_store;
}

static void
add_snippet_variable (SnippetVarsStore *vars_store,
                      const gchar *variable_name,
                      const gchar *default_value,
                      gboolean is_global)
{
	SnippetVarsStorePrivate *priv = NULL;
	gchar *instant_value = NULL;
	gboolean undefined = FALSE;
	GtkTreeIter iter;
	SnippetVariableType type;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET_VARS_STORE (vars_store));
	g_return_if_fail (variable_name != NULL);
	g_return_if_fail (default_value != NULL);
	priv = ANJUTA_SNIPPET_VARS_STORE_GET_PRIVATE (vars_store);
	
	if (is_global)
	{
		instant_value = snippets_db_get_global_variable (priv->snippets_db, variable_name);
		if (instant_value == NULL)
		{
			undefined = TRUE;
			instant_value = g_strdup (default_value);
		}
		
		type = SNIPPET_VAR_TYPE_GLOBAL;
	}
	else
	{
		instant_value = g_strdup (default_value);
		type = SNIPPET_VAR_TYPE_LOCAL;
	}

	gtk_list_store_append (GTK_LIST_STORE (vars_store), &iter);

	gtk_list_store_set (GTK_LIST_STORE (vars_store), &iter,
	                    VARS_STORE_COL_NAME, variable_name,
	                    VARS_STORE_COL_TYPE, type,
	                    VARS_STORE_COL_DEFAULT_VALUE, default_value,
	                    VARS_STORE_COL_INSTANT_VALUE, instant_value,
	                    VARS_STORE_COL_IN_SNIPPET, TRUE,
	                    VARS_STORE_COL_UNDEFINED, undefined,
	                    -1);

	g_free (instant_value);
}

static void
add_global_variables (SnippetVarsStore *vars_store)
{
	SnippetVarsStorePrivate *priv = NULL;
	GtkTreeIter iter, iter_to_add;
	gchar *cur_var_name = NULL;
	GtkTreeModel *global_vars_model = NULL;
	gchar *instant_value = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET_VARS_STORE (vars_store));
	priv = ANJUTA_SNIPPET_VARS_STORE_GET_PRIVATE (vars_store);
	global_vars_model = snippets_db_get_global_vars_model (priv->snippets_db);
	g_return_if_fail (GTK_IS_TREE_MODEL (global_vars_model));
	
	if (gtk_tree_model_get_iter_first (global_vars_model, &iter))
	{
		do
		{
			gtk_tree_model_get (global_vars_model, &iter,
			                    GLOBAL_VARS_MODEL_COL_NAME, &cur_var_name,
			                    -1);

			/* If the snippet holds this global variable, it was already added to the
			   store.*/
			if (snippet_has_variable (priv->snippet, cur_var_name))
			{
				g_free (cur_var_name);
				continue;
			}
			instant_value = snippets_db_get_global_variable (priv->snippets_db, cur_var_name);

			gtk_list_store_append (GTK_LIST_STORE (vars_store), &iter_to_add);
			gtk_list_store_set (GTK_LIST_STORE (vars_store), &iter_to_add,
					            VARS_STORE_COL_NAME, cur_var_name,
					            VARS_STORE_COL_TYPE, SNIPPET_VAR_TYPE_GLOBAL,
					            VARS_STORE_COL_DEFAULT_VALUE, g_strdup (""),
					            VARS_STORE_COL_INSTANT_VALUE, instant_value,
					            VARS_STORE_COL_IN_SNIPPET, FALSE,
					            VARS_STORE_COL_UNDEFINED, FALSE,
					            -1);
			
			g_free (cur_var_name);
			g_free (instant_value);
			
		} while (gtk_tree_model_iter_next (global_vars_model, &iter));
	}
}

/**
 * reload_vars_store:
 * @vars_store: A #SnippetVarsStore object.
 *
 * Reloads the GtkListStore with the current values of the variables in the internal
 * snippet and snippets-db. If priv->snippet or priv->snippets_db is NULL, it will clear the 
 * GtkListStore.
 */
static void
reload_vars_store (SnippetVarsStore *vars_store)
{
	SnippetVarsStorePrivate *priv = NULL;
	GList *snippet_vars_names = NULL, *snippet_vars_defaults = NULL, *snippet_vars_globals = NULL,
	      *iter1 = NULL, *iter2 = NULL, *iter3 = NULL;
	gchar *cur_var_name = NULL, *cur_var_default = NULL;
	gboolean cur_var_global = FALSE;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET_VARS_STORE (vars_store));
	priv = ANJUTA_SNIPPET_VARS_STORE_GET_PRIVATE (vars_store);

	/* Clear the GtkListStore */
	gtk_list_store_clear (GTK_LIST_STORE (vars_store));

	/* Add new items */
	if (ANJUTA_IS_SNIPPET (priv->snippet) && ANJUTA_IS_SNIPPETS_DB (priv->snippets_db))
	{
		/* Add the snippet variables to the store */
		snippet_vars_names    = snippet_get_variable_names_list (priv->snippet);
		snippet_vars_defaults = snippet_get_variable_defaults_list (priv->snippet);
		snippet_vars_globals  = snippet_get_variable_globals_list (priv->snippet);

		g_return_if_fail (g_list_length (snippet_vars_names) == g_list_length (snippet_vars_defaults));
		g_return_if_fail (g_list_length (snippet_vars_names) == g_list_length (snippet_vars_globals));

		iter1 = g_list_first (snippet_vars_names);
		iter2 = g_list_first (snippet_vars_defaults);
		iter3 = g_list_first (snippet_vars_globals);
		while (iter1 != NULL && iter2 != NULL && iter3 != NULL)
		{
			cur_var_name    = (gchar *)iter1->data;
			cur_var_default = (gchar *)iter2->data;
			cur_var_global  = GPOINTER_TO_INT (iter3->data);
			add_snippet_variable (vars_store, cur_var_name, cur_var_default, cur_var_global);
					
			iter1 = g_list_next (iter1);
			iter2 = g_list_next (iter2);
			iter3 = g_list_next (iter3);
		}
		g_list_free (snippet_vars_names);
		g_list_free (snippet_vars_defaults);
		g_list_free (snippet_vars_globals);

		/* Add the global variables to the store which aren't in the snippet */
		add_global_variables (vars_store);
	
	}

}

static void
on_global_vars_model_row_changed (GtkTreeModel *tree_model,
                                  GtkTreePath *path,
                                  GtkTreeIter *iter,
                                  gpointer user_data)
{
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET_VARS_STORE (user_data));

	reload_vars_store (ANJUTA_SNIPPET_VARS_STORE (user_data));
}


static void
on_global_vars_model_row_inserted (GtkTreeModel *tree_model,
                                   GtkTreePath *path,
                                   GtkTreeIter *iter,
                                   gpointer user_data)
{
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET_VARS_STORE (user_data));

	reload_vars_store (ANJUTA_SNIPPET_VARS_STORE (user_data));
}

static void
on_global_vars_model_row_deleted (GtkTreeModel *tree_model,
                                  GtkTreePath *path,
                                  gpointer user_data)
{
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET_VARS_STORE (user_data));

	reload_vars_store (ANJUTA_SNIPPET_VARS_STORE (user_data));
}

void                       
snippet_vars_store_load (SnippetVarsStore *vars_store,
                         SnippetsDB *snippets_db,
                         AnjutaSnippet *snippet)
{
	SnippetVarsStorePrivate *priv = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET_VARS_STORE (vars_store));
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db));
	g_return_if_fail (ANJUTA_IS_SNIPPET (snippet));
	priv = ANJUTA_SNIPPET_VARS_STORE_GET_PRIVATE (vars_store);
	
	priv->snippets_db = snippets_db;
	priv->snippet = snippet;

	/* This will fill the GtkListStore with values of variables from snippets_db 
	   and snippet */
	reload_vars_store (vars_store);

	/* We connect to the signals that change the GtkTreeModel of the global variables.
	   This is to make sure our store is synced with the global variables model. */
	priv->row_inserted_handler_id = 
		g_signal_connect (G_OBJECT (snippets_db_get_global_vars_model (snippets_db)),
		                  "row-inserted",
		                  G_CALLBACK (on_global_vars_model_row_inserted),
		                  vars_store);
	priv->row_changed_handler_id =
		g_signal_connect (G_OBJECT (snippets_db_get_global_vars_model (snippets_db)),
		                  "row-changed",
		                  G_CALLBACK (on_global_vars_model_row_changed),
		                  vars_store);
	priv->row_deleted_handler_id =
		g_signal_connect (G_OBJECT (snippets_db_get_global_vars_model (snippets_db)),
		                  "row-deleted",
		                  G_CALLBACK (on_global_vars_model_row_deleted),
		                  vars_store);
}

void
snippet_vars_store_unload (SnippetVarsStore *vars_store)
{
	SnippetVarsStorePrivate *priv = NULL;
	GtkTreeModel *global_vars_model = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET_VARS_STORE (vars_store));
	priv = ANJUTA_SNIPPET_VARS_STORE_GET_PRIVATE (vars_store);

	/* If we don't have a snippet or a snippets-db we just return */
	if (!ANJUTA_IS_SNIPPETS_DB (priv->snippets_db))
		return;

	global_vars_model = snippets_db_get_global_vars_model (priv->snippets_db);
	g_return_if_fail (GTK_IS_TREE_MODEL (global_vars_model));

	/* Disconnect the handlers */
	g_signal_handler_disconnect (global_vars_model, priv->row_inserted_handler_id);
	g_signal_handler_disconnect (global_vars_model, priv->row_changed_handler_id);
	g_signal_handler_disconnect (global_vars_model, priv->row_deleted_handler_id);
	
	priv->snippets_db = NULL;
	priv->snippet = NULL;

	/* This will clear the GtkListStore */
	reload_vars_store (vars_store);
}

static gboolean
get_iter_at_variable (SnippetVarsStore *vars_store,
                      GtkTreeIter *iter,
                      const gchar *variable_name,
                      SnippetVariableType type,
                      gboolean in_snippet_only)
{
	SnippetVarsStorePrivate *priv = NULL;
	gchar *cur_var_name = NULL;
	gboolean cur_var_in_snippet = FALSE;
	SnippetVariableType cur_type = SNIPPET_VAR_TYPE_ANY;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPET_VARS_STORE (vars_store), FALSE);
	priv = ANJUTA_SNIPPET_VARS_STORE_GET_PRIVATE (vars_store);

	if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (vars_store), iter))
		return FALSE;

	do
	{
		gtk_tree_model_get (GTK_TREE_MODEL (vars_store), iter,
		                    VARS_STORE_COL_NAME, &cur_var_name,
		                    VARS_STORE_COL_IN_SNIPPET, &cur_var_in_snippet,
		                    VARS_STORE_COL_TYPE, &cur_type,
		                    -1);

		if (!g_strcmp0 (variable_name, cur_var_name))
		{
			g_free (cur_var_name);
			if (type != SNIPPET_VAR_TYPE_ANY && cur_type != type)
				continue;
			if (in_snippet_only && !cur_var_in_snippet)
				continue;
			
			return TRUE;
		}
		g_free (cur_var_name);
		
	} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (vars_store), iter));
	
	return FALSE;
}

/**
 * snippet_vars_store_set_variable_name:
 * @vars_store: A #SnippetVarsStore object.
 * @old_variable_name: The name of the variable which should be renamed.
 * @new_variable_name: The new name for the variable.
 *
 * Changes the name of a variable which is already added to the snippet (so it won't
 * work for global variables that aren't added to the snippet). If the type of the 
 * variable is global, it will remove it from the snippet and add a new global variable
 * with the @new_variable_name. If no global variable with the new name is found, it 
 * will be marked as undefined. If the type of the variable is local, it will still
 * remove/add again with a new name, but won't do the check if it's defined.
 */
void
snippet_vars_store_set_variable_name (SnippetVarsStore *vars_store,
                                      const gchar *old_variable_name,
                                      const gchar *new_variable_name)
{
	SnippetVarsStorePrivate *priv = NULL;
	GtkTreeIter iter;
	gchar *default_value = NULL, *instant_value = NULL;
	SnippetVariableType type;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET_VARS_STORE (vars_store));
	g_return_if_fail (old_variable_name != NULL);
	g_return_if_fail (new_variable_name != NULL);
	priv = ANJUTA_SNIPPET_VARS_STORE_GET_PRIVATE (vars_store);
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (priv->snippets_db));
	g_return_if_fail (ANJUTA_IS_SNIPPET (priv->snippet));

	/* We check that the new name isn't already in the snippet */
	if (snippet_has_variable (priv->snippet, new_variable_name))
		return;

	/* We get the iter at the requested variable */
	if (!get_iter_at_variable (vars_store, &iter, old_variable_name, SNIPPET_VAR_TYPE_ANY, TRUE))
		return;

	/* We get the type and default value (as we may need to change it) */
	gtk_tree_model_get (GTK_TREE_MODEL (vars_store), &iter,
	                    VARS_STORE_COL_DEFAULT_VALUE, &default_value,
	                    VARS_STORE_COL_TYPE, &type,
	                    -1);	

	/* Remove the old variable */
	snippet_vars_store_remove_variable_from_snippet (vars_store, old_variable_name);
	snippet_vars_store_add_variable_to_snippet (vars_store, new_variable_name,
	                                            type == SNIPPET_VAR_TYPE_GLOBAL);

	/* Get the iter at the newly added variable */
	if (!get_iter_at_variable (vars_store, &iter, new_variable_name, type, TRUE))
		g_return_if_reached ();

	/* We compute the instant_value:
	   * if it's global and defined we get the value from the snippets_db
	   * if it's global but undefined or if it's local, we save the default_value
	 */
	if (type == SNIPPET_VAR_TYPE_GLOBAL)
	{
		instant_value = snippets_db_get_global_variable (priv->snippets_db, new_variable_name);
	}
	if (instant_value == NULL)
	{	
		instant_value = g_strdup (default_value);
	}

	/* Save the the list store the changes */
	gtk_list_store_set (GTK_LIST_STORE (vars_store), &iter,
	                    VARS_STORE_COL_DEFAULT_VALUE, default_value,
	                    VARS_STORE_COL_INSTANT_VALUE, instant_value,
	                    -1);

	/* Save the change to the snippet */
	snippet_set_variable_name (priv->snippet, old_variable_name, new_variable_name);
	snippet_set_variable_default_value (priv->snippet, new_variable_name, default_value);
	snippet_set_variable_global (priv->snippet, new_variable_name, type == SNIPPET_VAR_TYPE_GLOBAL);
	
	g_free (default_value);
	g_free (instant_value);

}

/**
 * snippet_vars_store_set_variable_type:
 * @vars_store: A #SnippetVarsStore object.
 * @variable_name: The name of the variable to have it's type changed.
 * @new_type: The new type of the variable.
 *
 * Sets a new type for a varible that already is in the snippet (so you can't set
 * types for global variables that aren't inserted in the snippet). This should be used
 * for changing already inserted variables types. Changing to global will try to get the
 * global variable with the given name and if not found it will be marked as undefined.
 * Changing to local, should actually add a new local variable with the given name and 
 * remove the global variable from the snippet without deleting it it's defined, but 
 * deleting it if it's undefined.
 */
void 
snippet_vars_store_set_variable_type (SnippetVarsStore *vars_store,
                                      const gchar *variable_name,
                                      SnippetVariableType new_type)
{
	SnippetVarsStorePrivate *priv = NULL;
	GtkTreeIter iter;
	gchar *default_value = NULL;
	SnippetVariableType old_type;
	gboolean undefined = FALSE;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET_VARS_STORE (vars_store));
	g_return_if_fail (variable_name != NULL);
	priv = ANJUTA_SNIPPET_VARS_STORE_GET_PRIVATE (vars_store);
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (priv->snippets_db));
	g_return_if_fail (ANJUTA_IS_SNIPPET (priv->snippet));

	old_type = (new_type == SNIPPET_VAR_TYPE_LOCAL) ? SNIPPET_VAR_TYPE_GLOBAL : SNIPPET_VAR_TYPE_LOCAL;

	/* We get the iter at the requested variable */
	if (!get_iter_at_variable (vars_store, &iter, variable_name, old_type, TRUE))
		return;

	/* Get the default value as it was saved by the user */
	gtk_tree_model_get (GTK_TREE_MODEL (vars_store), &iter,
	                    VARS_STORE_COL_DEFAULT_VALUE, &default_value,
	                    -1);

	/* We remove from the snippet the old variable entry and we add a new "clean" one*/
	snippet_vars_store_remove_variable_from_snippet (vars_store, variable_name);
	snippet_vars_store_add_variable_to_snippet (vars_store, variable_name,
	                                            new_type == SNIPPET_VAR_TYPE_GLOBAL);

	/* Get a iter at the new variable */
	if (!get_iter_at_variable (vars_store, &iter, variable_name, new_type, TRUE))
		g_return_if_reached ();

	/* Change it's default value to what the user used to have for it */
	gtk_list_store_set (GTK_LIST_STORE (vars_store), &iter,
	                    VARS_STORE_COL_DEFAULT_VALUE, default_value,
	                    -1);

	snippet_set_variable_global (priv->snippet, variable_name, new_type == SNIPPET_VAR_TYPE_GLOBAL);
	snippet_set_variable_default_value (priv->snippet, variable_name, default_value);

	/* If the newly added variable is local or undefined, we update it's instant value */
	gtk_tree_model_get (GTK_TREE_MODEL (vars_store), &iter,
	                    VARS_STORE_COL_UNDEFINED, &undefined,
	                    -1);
	if (new_type == SNIPPET_VAR_TYPE_LOCAL || undefined)
		gtk_list_store_set (GTK_LIST_STORE (vars_store), &iter,
		                    VARS_STORE_COL_INSTANT_VALUE, default_value,
		                    -1);

	g_free (default_value);

}

/**
 * snippet_vars_store_set_variable_default:
 * @vars_store: A #SnippetVarsStore object.
 * @variable_name: The name of the variable which will have it's default value changed.
 * @default_value: The new default value.
 *
 * This will only work if the variable is in the snippet (so we can't set default values
 * for global variables which aren't in the snippet).
 */
void
snippet_vars_store_set_variable_default (SnippetVarsStore *vars_store,
                                         const gchar *variable_name,
                                         const gchar *default_value)
{
	SnippetVarsStorePrivate *priv = NULL;
	GtkTreeIter iter;
	SnippetVariableType type;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET_VARS_STORE (vars_store));
	g_return_if_fail (variable_name != NULL);
	g_return_if_fail (default_value != NULL);
	priv = ANJUTA_SNIPPET_VARS_STORE_GET_PRIVATE (vars_store);
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (priv->snippets_db));
	g_return_if_fail (ANJUTA_IS_SNIPPET (priv->snippet));

	/* We get the iter at the requested variable */
	if (!get_iter_at_variable (vars_store, &iter, variable_name, SNIPPET_VAR_TYPE_ANY, TRUE))
		return;	

	gtk_list_store_set (GTK_LIST_STORE (vars_store), &iter,
	                    VARS_STORE_COL_DEFAULT_VALUE, default_value,
	                    -1);

	/* If the variable is local, we also set the instant value */
	gtk_tree_model_get (GTK_TREE_MODEL (vars_store), &iter,
	                    VARS_STORE_COL_TYPE, &type,
	                    -1);
	if (type == SNIPPET_VAR_TYPE_LOCAL)
	{
		gtk_list_store_set (GTK_LIST_STORE (vars_store), &iter,
		                    VARS_STORE_COL_INSTANT_VALUE, default_value,
		                    -1);
	}

	/* Save the changes to the snippet */
	snippet_set_variable_default_value (priv->snippet, variable_name, default_value);
}

/**
 * snippet_vars_store_add_variable_to_snippet:
 * @vars_store: A #SnippetVarsStore object.
 * @variable_name: The name of the variable to be added to the snippet.
 * @get_global: If it should add the global variable with the given name.
 *
 * Adds a variable to the snippet. If @get_global is TRUE, but no global variable
 * with the given name is found, the added variable will be marked as undefined.
 * If it's FALSE it will add it as local.
 */
void
snippet_vars_store_add_variable_to_snippet (SnippetVarsStore *vars_store,
                                            const gchar *variable_name,
                                            gboolean get_global)
{
	SnippetVarsStorePrivate *priv = NULL;
	GtkTreeIter iter;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET_VARS_STORE (vars_store));
	g_return_if_fail (variable_name != NULL);
	priv = ANJUTA_SNIPPET_VARS_STORE_GET_PRIVATE (vars_store);
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (priv->snippets_db));
	g_return_if_fail (ANJUTA_IS_SNIPPET (priv->snippet));

	/* We check to see if there is a variable with the same name in the snippet*/
	if (snippet_has_variable (priv->snippet, variable_name))
		return;		

	/* If we should get the global variable, we just change it's status to TRUE */
	if (get_global)
	{
		if (!get_iter_at_variable (vars_store, &iter, variable_name, SNIPPET_VAR_TYPE_GLOBAL, FALSE))
		{
			/* If we didn't found a global variable with the given name, add one as
			   undefined. */
			gtk_list_store_prepend (GTK_LIST_STORE (vars_store), &iter);
			gtk_list_store_set (GTK_LIST_STORE (vars_store), &iter,
			                    VARS_STORE_COL_NAME, variable_name,
			                    VARS_STORE_COL_TYPE, SNIPPET_VAR_TYPE_GLOBAL,
			                    VARS_STORE_COL_DEFAULT_VALUE, "",
			                    VARS_STORE_COL_INSTANT_VALUE, "",
			                    VARS_STORE_COL_IN_SNIPPET, TRUE,
			                    VARS_STORE_COL_UNDEFINED, TRUE,
			                    -1);
		}
		else
		{
			gtk_list_store_set (GTK_LIST_STORE (vars_store), &iter,
		                        VARS_STORE_COL_IN_SNIPPET, TRUE,
		                        -1);		
		}	
	}
	/* If not, we just add a local variable */
	else
	{
		gtk_list_store_prepend (GTK_LIST_STORE (vars_store), &iter);
		gtk_list_store_set (GTK_LIST_STORE (vars_store), &iter,
			                VARS_STORE_COL_NAME, variable_name,
			                VARS_STORE_COL_TYPE, SNIPPET_VAR_TYPE_LOCAL,
			                VARS_STORE_COL_DEFAULT_VALUE, "",
			                VARS_STORE_COL_INSTANT_VALUE, "",
			                VARS_STORE_COL_IN_SNIPPET, TRUE,
			                VARS_STORE_COL_UNDEFINED, FALSE,
			                -1);
	}

	snippet_add_variable (priv->snippet, variable_name, "", get_global);
}

/**
 * snippet_vars_store_remove_variable_from_snippet:
 * @vars_store: A #SnippetVarsStore object.
 * @variable_name: The name of the variable to be removed from the snippet.
 *
 * If the variable to be removed is global, it's in_snippet field will just be set
 * to FALSE. If the variable to be removed is local, we will also delete the row
 * from the store. If the variable to be removed is global, but undefined, it will also
 * delete the row.
 */
void
snippet_vars_store_remove_variable_from_snippet (SnippetVarsStore *vars_store,
                                                 const gchar *variable_name)
{
	SnippetVarsStorePrivate *priv = NULL;
	GtkTreeIter iter;
	SnippetVariableType type;
	gboolean undefined = FALSE;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET_VARS_STORE (vars_store));
	g_return_if_fail (variable_name != NULL);
	priv = ANJUTA_SNIPPET_VARS_STORE_GET_PRIVATE (vars_store);
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (priv->snippets_db));
	g_return_if_fail (ANJUTA_IS_SNIPPET (priv->snippet));

	/* We get the iter at the requested variable */
	if (!get_iter_at_variable (vars_store, &iter, variable_name, SNIPPET_VAR_TYPE_ANY, TRUE))
		return;
	
	gtk_tree_model_get (GTK_TREE_MODEL (vars_store), &iter,
	                    VARS_STORE_COL_TYPE, &type,
	                    VARS_STORE_COL_UNDEFINED, &undefined,
	                    -1);

	/* If it's local or global, but undefined, we remove the entry from the store */
	if (type == SNIPPET_VAR_TYPE_LOCAL || undefined)	
	{
		gtk_list_store_remove (GTK_LIST_STORE (vars_store), &iter);
	}
	/* If it's global we just set it's in_snippet field to FALSE */
	else
	{
		g_return_if_fail (type == SNIPPET_VAR_TYPE_GLOBAL);
		
		gtk_list_store_set (GTK_LIST_STORE (vars_store), &iter,
		                    VARS_STORE_COL_IN_SNIPPET, FALSE,
		                    VARS_STORE_COL_DEFAULT_VALUE, "",
		                    -1);	
	}

	/* Finally, remove the variable from the snippet. */
	snippet_remove_variable (priv->snippet, variable_name);
}
