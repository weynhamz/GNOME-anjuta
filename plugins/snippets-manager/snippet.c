/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippet.c
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

#include <string.h>
#include "snippet.h"
#include "snippets-db.h"
#include <gio/gio.h>
#include <libanjuta/anjuta-debug.h>

#define SNIPPET_VARIABLE_START(text, index)  (text[index] == '$' && text[index + 1] == '{')
#define SNIPPET_VARIABLE_END(text, index)    (text[index] == '}')

#define STRING_CUR_POSITION(string)          string->len

#define END_CURSOR_VARIABLE_NAME             "END_CURSOR_POSITION"
#define LANGUAGE_SEPARATOR                   '/'

/**
 * SnippetVariable:
 * @variable_name: the name of the variable.
 * @default_value: the default value as it will be inserted in the code.
 * @is_global: if the variable is global accross the SnippetDB. Eg: username or email.
 * @cur_valaue_len: If the snippet was computed recently, it represents the length of the current variable
 *                  (default or global variable)
 * @relative_positions: the relative positions from the start of the snippet code each instance of
 *                      this variable has.
 *
 * The snippet variable structure.
 *
 **/
typedef struct _AnjutaSnippetVariable
{
	gchar* variable_name;
	gchar* default_value;
	gboolean is_global;

	gint cur_value_len;
	GPtrArray* relative_positions;
	
} AnjutaSnippetVariable;


#define ANJUTA_SNIPPET_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ANJUTA_TYPE_SNIPPET, AnjutaSnippetPrivate))

struct _AnjutaSnippetPrivate
{
	gchar* trigger_key;
	GList* snippet_languages;
	gchar* snippet_name;
	
	gchar* snippet_content;
	
	GList* variables;
	GList* keywords;

	gint cur_value_end_position;

	gboolean default_computed;
};


G_DEFINE_TYPE (AnjutaSnippet, snippet, G_TYPE_OBJECT);

static void
snippet_dispose (GObject* snippet)
{
	AnjutaSnippet* anjuta_snippet = ANJUTA_SNIPPET (snippet);
	GList* iter = NULL;
	gpointer p;
	AnjutaSnippetVariable* cur_snippet_var;

	/* Delete the trigger_key, snippet_language, snippet_name and snippet_content fields */
	g_free (anjuta_snippet->priv->trigger_key);
	anjuta_snippet->priv->trigger_key = NULL;
	g_free (anjuta_snippet->priv->snippet_name);
	anjuta_snippet->priv->snippet_name = NULL;
	g_free (anjuta_snippet->priv->snippet_content);
	anjuta_snippet->priv->snippet_content = NULL;

	/* Delete the languages */
	for (iter = g_list_first (anjuta_snippet->priv->snippet_languages); iter != NULL; iter = g_list_next (iter))
	{
		p = iter->data;
		g_free (p);
	}
	g_list_free (anjuta_snippet->priv->snippet_languages);
	anjuta_snippet->priv->snippet_languages = NULL;
	
	/* Delete the keywords */
	for (iter = g_list_first (anjuta_snippet->priv->keywords); iter != NULL; iter = g_list_next (iter))
	{
		p = iter->data;
		g_free (p);
	}
	g_list_free (anjuta_snippet->priv->keywords);
	anjuta_snippet->priv->keywords = NULL;
	
	/* Delete the snippet variables */
	for (iter = g_list_first (anjuta_snippet->priv->variables); iter != NULL; iter = g_list_next (iter))
	{
		cur_snippet_var = (AnjutaSnippetVariable *)iter->data;
		
		g_free (cur_snippet_var->variable_name);
		g_free (cur_snippet_var->default_value);
		g_ptr_array_unref (cur_snippet_var->relative_positions);
		
		g_free (cur_snippet_var);
	}
	g_list_free (anjuta_snippet->priv->variables);

	G_OBJECT_CLASS (snippet_parent_class)->dispose (snippet);
}

static void
snippet_finalize (GObject* snippet)
{
	G_OBJECT_CLASS (snippet_parent_class)->finalize (snippet);
}

static void
snippet_class_init (AnjutaSnippetClass* klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	snippet_parent_class = g_type_class_peek_parent (klass);
	object_class->dispose = snippet_dispose;
	object_class->finalize = snippet_finalize;

	g_type_class_add_private (klass, sizeof (AnjutaSnippetPrivate));
}

static void
snippet_init (AnjutaSnippet* snippet)
{
	AnjutaSnippetPrivate* priv = ANJUTA_SNIPPET_GET_PRIVATE (snippet);

	snippet->priv = priv;
	snippet->parent_snippets_group = NULL;

	/* Initialize the private field */
	snippet->priv->trigger_key = NULL;
	snippet->priv->snippet_languages = NULL;
	snippet->priv->snippet_name = NULL;
	snippet->priv->snippet_content = NULL;
	snippet->priv->variables = NULL;
	snippet->priv->keywords = NULL;

	snippet->priv->cur_value_end_position = -1;
	snippet->priv->default_computed = FALSE;
}

/**
 * snippet_new:
 * @trigger_key: The trigger key of the snippet.
 * @snippet_languages: A list with the languages for which the snippet is meant.
 * @snippet_name: A short, intuitive name of the snippet.
 * @snippet_content: The actual content of the snippet.
 * @variables_names: A #GList with the variable names.
 * @variables_default_values: A #GList with the default values of the variables.
 * @variable_globals: A #GList with gboolean's that state if the variable is global or not.
 * @keywords: A #GList with the keywords of the snippet.
 *
 * Creates a new snippet object. All the data given to this function will be copied.
 *
 * Returns: The new #AnjutaSnippet or NULL on failure.
 **/
AnjutaSnippet* 
snippet_new (const gchar* trigger_key,
             GList* snippet_languages,
             const gchar* snippet_name,
             const gchar* snippet_content,
             GList* variable_names,
             GList* variable_default_values,
             GList* variable_globals,
             GList* keywords)
{
	AnjutaSnippet* snippet = NULL;
	GList *iter1 = NULL, *iter2 = NULL, *iter3 = NULL;
	gchar* temporary_string_holder = NULL;
	AnjutaSnippetVariable* cur_snippet_var = NULL;

	/* Assertions */
	g_return_val_if_fail (trigger_key != NULL, NULL);
	g_return_val_if_fail (snippet_name != NULL, NULL);
	g_return_val_if_fail (snippet_content != NULL, NULL);
	g_return_val_if_fail (g_list_length (variable_names) == g_list_length (variable_default_values),
	                      NULL);
	g_return_val_if_fail (g_list_length (variable_names) == g_list_length (variable_globals),
	                      NULL);
	
	/* Initialize the object */
	snippet = ANJUTA_SNIPPET (g_object_new (snippet_get_type (), NULL));
	
	/* Make a copy of the given strings */
	snippet->priv->trigger_key      = g_strdup (trigger_key);
	snippet->priv->snippet_name     = g_strdup (snippet_name);
	snippet->priv->snippet_content  = g_strdup (snippet_content);

	/* Copy all the snippet languages to a new list */
	snippet->priv->snippet_languages = NULL;
	for (iter1 = g_list_first (snippet_languages); iter1 != NULL; iter1 = g_list_next (iter1))
	{
		temporary_string_holder = g_strdup ((const gchar *)iter1->data);
		snippet->priv->snippet_languages = g_list_append (snippet->priv->snippet_languages,
		                                                  temporary_string_holder);
	}
	
	/* Copy all the keywords to a new list */
	snippet->priv->keywords = NULL;
	for (iter1 = g_list_first (keywords); iter1 != NULL; iter1 = g_list_next (iter1))
	{
		temporary_string_holder = g_strdup ((gchar *)iter1->data);
		snippet->priv->keywords = g_list_append (snippet->priv->keywords, temporary_string_holder);
	}
	
	/* Make a list of variables */
	snippet->priv->variables = NULL;
	iter1 = g_list_first (variable_names);
	iter2 = g_list_first (variable_default_values);
	iter3 = g_list_first (variable_globals);
	while (iter1 != NULL && iter2 != NULL && iter3 != NULL)
	{
		cur_snippet_var = g_malloc (sizeof (AnjutaSnippetVariable));
		
		cur_snippet_var->variable_name = g_strdup ((gchar*)iter1->data);
		cur_snippet_var->default_value = g_strdup ((gchar*)iter2->data);
		cur_snippet_var->is_global = GPOINTER_TO_INT (iter3->data);
		
		cur_snippet_var->cur_value_len = 0;
		cur_snippet_var->relative_positions = g_ptr_array_new ();
		
		snippet->priv->variables = g_list_append (snippet->priv->variables, cur_snippet_var);

		iter1 = g_list_next (iter1);
		iter2 = g_list_next (iter2);
		iter3 = g_list_next (iter3);
	}
	DEBUG_PRINT ("Snippet %s created.\n", snippet_name); 
	return snippet;
}

AnjutaSnippet *
snippet_copy (AnjutaSnippet *snippet)
{
	GList *languages = NULL, *keywords = NULL, *variable_names = NULL, *variable_defaults = NULL,
	      *variable_globals = NULL;
	const gchar *trigger = NULL, *name = NULL, *content = NULL;
	AnjutaSnippet *copied_snippet = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), NULL);

	trigger = snippet_get_trigger_key (snippet);
	name    = snippet_get_name (snippet);
	content = snippet_get_content (snippet);

	keywords          = snippet_get_keywords_list (snippet);
	languages         = (GList *)snippet_get_languages (snippet);
	variable_names    = snippet_get_variable_names_list (snippet);
	variable_defaults = snippet_get_variable_defaults_list (snippet);
	variable_globals  = snippet_get_variable_globals_list (snippet);

	copied_snippet = snippet_new (trigger, languages, name, content,
	                              variable_names, variable_defaults, variable_globals,
	                              keywords);

	g_list_free (keywords);
	g_list_free (variable_names);
	g_list_free (variable_defaults);
	g_list_free (variable_globals);

	copied_snippet->parent_snippets_group = snippet->parent_snippets_group;
	
	return copied_snippet;
}

/**
 * snippet_get_trigger_key:
 * @snippet: A #AnjutaSnippet object.
 *
 * Gets the trigger-key of the snippet.
 *
 * Returns: The snippet-key or NULL if @snippet is invalid.
 **/
const gchar*
snippet_get_trigger_key (AnjutaSnippet* snippet)
{
	AnjutaSnippetPrivate *priv = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), NULL);
	priv = ANJUTA_SNIPPET_GET_PRIVATE (snippet);
	
	return priv->trigger_key;
}

void
snippet_set_trigger_key (AnjutaSnippet *snippet,
                         const gchar *new_trigger_key)
{
	AnjutaSnippetPrivate *priv = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET (snippet));
	g_return_if_fail (new_trigger_key != NULL);
	priv = ANJUTA_SNIPPET_GET_PRIVATE (snippet);

	g_free (priv->trigger_key);
	priv->trigger_key = g_strdup (new_trigger_key);
}

/**
 * snippet_get_language:
 * @snippet: A #AnjutaSnippet object.
 *
 * Gets the supported languages of the snippet.
 *
 * Returns: A GList with strings representing the supported languages or NULL if @snippet is invalid.
 **/
const GList *	
snippet_get_languages (AnjutaSnippet* snippet)
{
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), NULL);

	return snippet->priv->snippet_languages;
}

/**
 * snippet_get_languages_string:
 * @snippet: A #AnjutaSnippets object.
 *
 * Formats a string with the supported languages. For example, if the snippet
 * is supported by C, C++ and Java, the returned string should look like:
 * "C/C++/Java".
 *
 * Returns: The above mentioned string or NULL on error. The memory should be free'd.
 */
gchar*          
snippet_get_languages_string (AnjutaSnippet *snippet)
{
	GString *languages_string = NULL;
	GList *languages = NULL, *iter = NULL;
	gchar *cur_language = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), NULL);
	g_return_val_if_fail (snippet->priv != NULL, NULL);
	languages = snippet->priv->snippet_languages;
	languages_string = g_string_new ("");
	
	for (iter = g_list_first (languages); iter != NULL; iter = g_list_next (iter))
	{
		cur_language = (gchar *)iter->data;
		languages_string = g_string_append (languages_string, cur_language);
		languages_string = g_string_append_c (languages_string, LANGUAGE_SEPARATOR);
	}
	/* We delete the last '/' */
	languages_string = g_string_set_size (languages_string, languages_string->len - 1);

	return g_string_free (languages_string, FALSE);
}

/**
 * snippet_has_language:
 * @snippet: A #AnjutaSnippet object.
 * @language: The language for which the query is done.
 *
 * Tests if the snippet is meant for the given language.
 *
 * Returns: TRUE if the snippet is meant for the given language.
 */
gboolean
snippet_has_language (AnjutaSnippet *snippet,
                      const gchar *language)
{
	GList *languages = NULL, *iter = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), FALSE);
	g_return_val_if_fail (snippet->priv != NULL, FALSE);
	g_return_val_if_fail (language != NULL, FALSE);
	languages = snippet->priv->snippet_languages;

	for (iter = g_list_first (languages); iter != NULL; iter = g_list_next (iter))
	{
		if (!g_strcmp0 ((gchar *)iter->data, language))
			return TRUE;
	}

	return FALSE;
}

/**
 * snippet_get_any_language:
 * @snippet: A #AnjutaSnippet object.
 *
 * Returns any of the languages that is supported by the given snippet. This should
 * be useful for stuff like removing a snippet from a snippets-group.
 *
 * Returns: Any of the languages supported by the snippet (or NULL if an error occured).
 */
const gchar*
snippet_get_any_language (AnjutaSnippet *snippet)
{
	GList *first_language_node = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), NULL);
	g_return_val_if_fail (snippet->priv != NULL, NULL);
	
	first_language_node = g_list_first (snippet->priv->snippet_languages);
	if (first_language_node == NULL)
		return NULL;
	
	return (const gchar *)first_language_node->data;
}

/**
 * snippet_add_language:
 * @snippet: A #AnjutaSnippet object
 * @language: The language for which we want to add support to the snippet.
 *
 * By adding a language to a snippet, we mean that now the snippet can be used in that
 * programming language.
 */
void            
snippet_add_language (AnjutaSnippet *snippet,
                      const gchar *language)
{
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET (snippet));
	g_return_if_fail (snippet->priv != NULL);

	if (snippet_has_language (snippet, language))
		return;

	snippet->priv->snippet_languages = g_list_append (snippet->priv->snippet_languages, 
	                                                  g_strdup (language));
}


/**
 * snippet_remove_language:
 * @snippet: A #AnjutaSnippet object
 * @language: The language for which we want to remove support from the snippet.
 *
 * The snippet won't be able to be used for the removed programming language.
 */                                                   
void
snippet_remove_language (AnjutaSnippet *snippet,
                         const gchar *language)
{
	GList *iter = NULL;
	gpointer p = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET (snippet));
	g_return_if_fail (snippet->priv != NULL);
	g_return_if_fail (language != NULL);

	for (iter = g_list_first (snippet->priv->snippet_languages); iter != NULL; iter = g_list_next (iter))
		if (!g_strcmp0 ((gchar *)iter->data, language))
		{
			p = iter->data;
			snippet->priv->snippet_languages = g_list_remove (snippet->priv->snippet_languages,
			                                                  iter->data);
			g_free (p);
		}
	
}

/**
 * snippet_get_name:
 * @snippet: A #AnjutaSnippet object.
 *
 * Gets the name of the snippet.
 *
 * Returns: The snippet name or NULL if @snippet is invalid.
 **/
const gchar*
snippet_get_name (AnjutaSnippet* snippet)
{
	AnjutaSnippetPrivate *priv = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), NULL);
	priv = ANJUTA_SNIPPET_GET_PRIVATE (snippet);
	
	return priv->snippet_name;
}

/**
 * snippet_set_name:
 * @snippet: An #AnjutaSnippet object
 * @new_name: The new name assigned to the snippet.
 */
void
snippet_set_name (AnjutaSnippet *snippet,
                  const gchar *new_name)
{
	AnjutaSnippetPrivate *priv = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET (snippet));
	g_return_if_fail (new_name != NULL);
	priv = ANJUTA_SNIPPET_GET_PRIVATE (snippet);

	priv->snippet_name = g_strdup (new_name);
}

/**
 * snippet_get_keywords_list:
 * @snippet: A #AnjutaSnippet object.
 *
 * Gets the list with the keywords of the snippet. The GList* should be free'd, but not
 * the containing data.
 *
 * Returns: A #GList of #gchar* with the keywords of the snippet or NULL if 
 *          @snippet is invalid.
 **/
GList*
snippet_get_keywords_list (AnjutaSnippet* snippet)
{
	GList *iter = NULL;
	GList *keywords_copy = NULL;
	gchar *cur_keyword;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), NULL);

	for (iter = g_list_first (snippet->priv->keywords); iter != NULL; iter = g_list_next (iter))
	{
		cur_keyword = (gchar *)iter->data;
		keywords_copy = g_list_append (keywords_copy, cur_keyword);
	}
	
	return keywords_copy;
}

/**
 * snippet_set_keywords_list:
 * @snippet: An #AnjutaSnippet object.
 * @keywords_list: The new list of keywords for the snippet.
 *
 * Sets a new keywords list.
 */
void
snippet_set_keywords_list (AnjutaSnippet *snippet,
                           const GList *keywords_list)
{
	GList *iter = NULL;
	AnjutaSnippetPrivate *priv = NULL;
	gchar *cur_keyword = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET (snippet));
	priv = ANJUTA_SNIPPET_GET_PRIVATE (snippet);

	/* Delete the old keywords list */
	for (iter = g_list_first (priv->keywords); iter != NULL; iter = g_list_next (iter))
	{
		g_free (iter->data);
	}
	g_list_free (g_list_first (priv->keywords));
	priv->keywords = NULL;

	/* Copy over the new list */
	for (iter = g_list_first ((GList *)keywords_list); iter != NULL; iter = g_list_next (iter))
	{
		cur_keyword = g_strdup ((const gchar *)iter->data);
		priv->keywords = g_list_append (priv->keywords, cur_keyword);
	}
	
}

/**
 * snippet_get_variable_names_list:
 * @snippet: A #AnjutaSnippet object.
 *
 * A list with the variables names, in the order they should be edited. The GList*
 * returned should be freed, but not the containing data.
 *
 * Returns: The variable names list or NULL if the @snippet is invalid.
 **/
GList*
snippet_get_variable_names_list (AnjutaSnippet* snippet)
{
	GList *iter = NULL;
	GList *variable_names = NULL;
	AnjutaSnippetVariable *cur_snippet_var = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), NULL);

	for (iter = g_list_first (snippet->priv->variables); iter != NULL; iter = g_list_next (iter))
	{
		cur_snippet_var = (AnjutaSnippetVariable *)iter->data;
		variable_names = g_list_append (variable_names, cur_snippet_var->variable_name);
	}
	
	return variable_names;
}

/**
 * snippet_get_variable_defaults_list:
 * @snippet: A #AnjutaSnippet object.
 *
 * A list with the variables default values, in the order they should be edited.
 * The GList* returned should be freed, but not the containing data.
 *
 * Returns: The variables default values #GList or NULL if @snippet is invalid.
 **/
GList*
snippet_get_variable_defaults_list (AnjutaSnippet* snippet)
{
	GList *iter = NULL;
	GList *variable_defaults = NULL;
	AnjutaSnippetVariable *cur_snippet_var = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), NULL);

	for (iter = g_list_first (snippet->priv->variables); iter != NULL; iter = g_list_next (iter))
	{
		cur_snippet_var = (AnjutaSnippetVariable *)iter->data;
		variable_defaults = g_list_append (variable_defaults, cur_snippet_var->default_value);
	}
	
	return variable_defaults;
}

/**
 * snippet_get_variable_globals_list:
 * @snippet: A #AnjutaSnippet object.
 *
 * A list with the variables global truth value, in the order they should be edited.
 * The GList* returned should be freed.
 *
 * Returns: The variables global truth values #GList or NULL if @snippet is invalid.
 **/
GList* 
snippet_get_variable_globals_list (AnjutaSnippet* snippet)
{
	GList *iter = NULL;
	GList *variable_globals = NULL;
	gboolean temp_holder = FALSE;
	AnjutaSnippetVariable *cur_snippet_var = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), NULL);

	for (iter = g_list_first (snippet->priv->variables); iter != NULL; iter = g_list_next (iter))
	{
		cur_snippet_var = (AnjutaSnippetVariable *)iter->data;
		temp_holder = cur_snippet_var->is_global;
		variable_globals = g_list_append (variable_globals, GINT_TO_POINTER (temp_holder));
	}
	
	return variable_globals;
}

static AnjutaSnippetVariable *
get_snippet_variable (AnjutaSnippet *snippet,
                      const gchar *variable_name)
{
	GList *iter = NULL;
	AnjutaSnippetPrivate *priv = NULL;
	AnjutaSnippetVariable *cur_var = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), NULL);
	priv = ANJUTA_SNIPPET_GET_PRIVATE (snippet);

	for (iter = g_list_first (priv->variables); iter != NULL; iter = g_list_next (iter))
	{
		cur_var = (AnjutaSnippetVariable *)iter->data;
		g_return_val_if_fail (cur_var != NULL, NULL);

		if (!g_strcmp0 (cur_var->variable_name, variable_name))
			return cur_var;
	}

	return NULL;	
}

gboolean        
snippet_has_variable (AnjutaSnippet *snippet,
                      const gchar *variable_name)
{
	return (get_snippet_variable (snippet, variable_name) != NULL);
}

/**
 * snippet_add_variable:
 * @snippet: An #AnjutaSnippet object.
 * @variable_name: The added variable name.
 * @default_value: The default value of the added snippet.
 * @is_global: If the added variable is global.
 *
 * If there is a variable with the same name, it won't add the variable.
 */
void            
snippet_add_variable (AnjutaSnippet *snippet,
                      const gchar *variable_name,
                      const gchar *default_value,
                      gboolean is_global)
{
	AnjutaSnippetPrivate *priv = NULL;
	AnjutaSnippetVariable *added_var = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET (snippet));
	g_return_if_fail (variable_name != NULL);
	g_return_if_fail (default_value != NULL);	
	priv = ANJUTA_SNIPPET_GET_PRIVATE (snippet);

	/* Check if we have a variable with the same name */
	if (snippet_has_variable (snippet, variable_name))
		return;

	/* Create a new variable */
	added_var = g_malloc (sizeof (AnjutaSnippetVariable));
	added_var->variable_name      = g_strdup (variable_name);
	added_var->default_value      = g_strdup (default_value);
	added_var->is_global          = is_global;
	added_var->cur_value_len      = 0;
	added_var->relative_positions = g_ptr_array_new ();

	priv->variables = g_list_prepend (priv->variables, added_var);
}

void            
snippet_remove_variable (AnjutaSnippet *snippet,
                         const gchar *variable_name)
{
	AnjutaSnippetPrivate *priv = NULL;
	AnjutaSnippetVariable *cur_var = NULL;
	GList *iter = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET (snippet));
	g_return_if_fail (variable_name != NULL);
	priv = ANJUTA_SNIPPET_GET_PRIVATE (snippet);

	for (iter = g_list_first (priv->variables); iter != NULL; iter = g_list_next (iter))
	{
		cur_var = (AnjutaSnippetVariable *)iter->data;
		g_return_if_fail (cur_var != NULL);

		if (!g_strcmp0 (cur_var->variable_name, variable_name))
		{
			g_free (cur_var->variable_name);
			g_free (cur_var->default_value);
			g_ptr_array_free (cur_var->relative_positions, TRUE);

			priv->variables = g_list_remove_link (priv->variables, iter);

			g_free (cur_var);
			return;
		}
	}

}

void
snippet_set_variable_name (AnjutaSnippet *snippet,
                           const gchar *variable_name,
                           const gchar *new_variable_name)
{
	AnjutaSnippetVariable *var = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET (snippet));
	g_return_if_fail (variable_name != NULL);
	g_return_if_fail (new_variable_name != NULL);

	var = get_snippet_variable (snippet, variable_name);
	if (var == NULL)
		return;

	g_free (var->variable_name);
	var->variable_name = g_strdup (new_variable_name);
}

const gchar*
snippet_get_variable_default_value (AnjutaSnippet *snippet,
                                    const gchar *variable_name)
{
	AnjutaSnippetVariable *var = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), NULL);
	g_return_val_if_fail (variable_name != NULL, NULL);

	var = get_snippet_variable (snippet, variable_name);
	g_return_val_if_fail (var != NULL, NULL);

	return var->default_value;
}

void            
snippet_set_variable_default_value (AnjutaSnippet *snippet,
                                    const gchar *variable_name,
                                    const gchar *default_value)
{
	AnjutaSnippetVariable *var = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET (snippet));
	g_return_if_fail (variable_name != NULL);
	g_return_if_fail (default_value != NULL);

	var = get_snippet_variable (snippet, variable_name);
	g_return_if_fail (var != NULL);

	g_free (var->default_value);
	var->default_value = g_strdup (default_value);
}

gboolean        
snippet_get_variable_global (AnjutaSnippet *snippet,
                             const gchar *variable_name)
{
	AnjutaSnippetVariable *var = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), FALSE);
	g_return_val_if_fail (variable_name != NULL, FALSE);

	var = get_snippet_variable (snippet, variable_name);
	g_return_val_if_fail (var != NULL, FALSE);

	return var->is_global;
}

void            
snippet_set_variable_global (AnjutaSnippet *snippet,
                             const gchar *variable_name,
                             gboolean global)
{
	AnjutaSnippetVariable *var = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET (snippet));
	g_return_if_fail (variable_name != NULL);

	var = get_snippet_variable (snippet, variable_name);
	g_return_if_fail (var != NULL);

	var->is_global = global;	
}

/**
 * snippet_get_content:
 * @snippet: A #AnjutaSnippet object.
 *
 * The content of the snippet without it being proccesed.
 *
 * Returns: The content of the snippet or NULL if @snippet is invalid.
 **/
const gchar*
snippet_get_content (AnjutaSnippet* snippet)
{
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), NULL);

	return snippet->priv->snippet_content;
}


void
snippet_set_content (AnjutaSnippet *snippet,
                     const gchar *new_content)
{
	AnjutaSnippetPrivate *priv = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET (snippet));
	g_return_if_fail (new_content != NULL);
	priv = ANJUTA_SNIPPET_GET_PRIVATE (snippet);
	
	g_free (priv->snippet_content);
	priv->snippet_content = g_strdup (new_content);
}

static void
reset_variables (AnjutaSnippet *snippet)
{
	GList *iter = NULL;
	AnjutaSnippetVariable *cur_var = NULL;
	
	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPET (snippet));
	g_return_if_fail (snippet->priv != NULL);
	
	for (iter = g_list_first (snippet->priv->variables); iter != NULL; iter = g_list_next (iter))
	{
		cur_var = (AnjutaSnippetVariable *)iter->data;

		cur_var->cur_value_len = 0;
		if (cur_var->relative_positions->len > 0)
			g_ptr_array_remove_range (cur_var->relative_positions, 
				                      0, cur_var->relative_positions->len);
	}

	snippet->priv->cur_value_end_position = -1;
}

static gchar *
expand_global_and_default_variables (AnjutaSnippet *snippet,
                                     gchar *snippet_text,
                                     SnippetsDB *snippets_db)
{
	gint snippet_text_size = 0, i = 0, j = 0;
	GString *buffer = NULL;
	GList *iter = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), NULL);
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), NULL);

	/* We get the snippet_text_size (for iterating) and init the buffer */
	snippet_text_size = strlen (snippet_text);
	buffer = g_string_new ("");

	/* We reset the variable */
	reset_variables (snippet);
	
	/* We expand the variables to the default value or if they are global variables
	   we query the database for their value. If the database can't answer to our
	   request, we fill them also with their default values */
	for (i = 0; i < snippet_text_size; i ++)
	{
		/* If it's the start of a variable name, we look up the end, get the name
		   and evaluate it. */
		if (SNIPPET_VARIABLE_START (snippet_text, i))
		{
			GString *cur_var_name = NULL;
			gchar *cur_var_value = NULL;
			gint cur_var_value_size = 0;
			AnjutaSnippetVariable *cur_var = NULL;
			
			/* We search for the variable end */
			cur_var_name = g_string_new ("");
			for (j = i + 2; j < snippet_text_size && !SNIPPET_VARIABLE_END (snippet_text, j); j ++)
				cur_var_name = g_string_append_c (cur_var_name, snippet_text[j]);

			/* We first see if it's the END_CURSOR_POSITION variable */
			if (!g_strcmp0 (cur_var_name->str, END_CURSOR_VARIABLE_NAME))
			{
				snippet->priv->cur_value_end_position = STRING_CUR_POSITION (buffer);
				g_string_free (cur_var_name, TRUE);

				i = j;
				continue;
			}
			
			/* Look up the variable */
			for (iter = g_list_first (snippet->priv->variables); iter != NULL; iter = g_list_next (iter))
			{
				cur_var = (AnjutaSnippetVariable *)iter->data;
				if (!g_strcmp0 (cur_var->variable_name, cur_var_name->str))
					break;
			}

			/* If iter is NULL, then we didn't found the variable name */
			if (!iter)
			{
				//cur_var_value = g_strconcat ("${", cur_var_name->str, "}", NULL);
				buffer = g_string_append_c (buffer, snippet_text[i]);

				g_string_free (cur_var_name, TRUE);
				continue;
			}
			else
			{
				/* If it's a global variable, we query the database */
				if (cur_var->is_global)
					cur_var_value = snippets_db_get_global_variable (snippets_db, 
					                                                 cur_var_name->str);

				/* If we didn't got an answer from the database or if the variable is not
				   global, we get the default value. */
				if (cur_var_value == NULL)
					cur_var_value = g_strdup (cur_var->default_value);

				i = j;
			}

			/* Update the variable data */
			cur_var_value_size = strlen (cur_var_value);
			cur_var->cur_value_len = cur_var_value_size;
			g_ptr_array_add (cur_var->relative_positions, 
			                 GINT_TO_POINTER (STRING_CUR_POSITION (buffer)));

			/* Append the variable value to the buffer */
			buffer = g_string_append (buffer, cur_var_value);

			g_free (cur_var_value);
			g_string_free (cur_var_name, TRUE);
		}
		else
		{
			buffer = g_string_append_c (buffer, snippet_text[i]);
		}
	}
	
	return g_string_free (buffer, FALSE);
}

static gchar *
get_text_with_indentation (const gchar *text,
                           const gchar *indent)
{
	gint text_size = 0, i = 0;
	GString *text_with_indentation = NULL;
	
	/* Assertions */
	g_return_val_if_fail (text != NULL, NULL);
	g_return_val_if_fail (indent != NULL, NULL);

	/* Init the text_with_indentation string */
	text_with_indentation = g_string_new ("");

	text_size = strlen (text);
	for (i = 0; i < text_size; i ++)
	{
		text_with_indentation = g_string_append_c (text_with_indentation, text[i]);

		/* If we go to a new line, we also add the indentation */
		if (text[i] == '\n')
			text_with_indentation = g_string_append (text_with_indentation, indent);
	}

	return g_string_free (text_with_indentation, FALSE);
}

/**
 * snippet_get_default_content:
 * @snippet: A #AnjutaSnippet object.
 * @snippets_db: A #SnippetsDB object. This is required for filling the global variables.
 *               This can be NULL if the snippet is independent of a #SnippetsDB or if
 *               it doesn't have global variables.
 * @indent: The indentation of the line where the snippet will be inserted.
 *
 * The content of the snippet filled with the default values of the variables.
 * Every '\n' character will be replaced with a string obtained by concatanating
 * "\n" with indent.
 *
 * Returns: The default content of the snippet or NULL if @snippet is invalid.
 **/
gchar*
snippet_get_default_content (AnjutaSnippet *snippet,
                             GObject *snippets_db_obj,
                             const gchar *indent)
{
	gchar* buffer = NULL, *temp = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), NULL);

	/* Get the with indentation */
	temp = get_text_with_indentation (snippet->priv->snippet_content, indent);

	/* If we should expand the global variables */
	if (snippets_db_obj && ANJUTA_IS_SNIPPETS_DB (snippets_db_obj))
	{
		/* Expand the global variables */
		buffer = expand_global_and_default_variables (snippet,
		                                              temp,
		                                              ANJUTA_SNIPPETS_DB (snippets_db_obj));
		g_free (temp);
	}
	else
	{
		buffer = temp;
	}

	snippet->priv->default_computed = TRUE;
	
	return buffer;
}

/**
 * snippet_get_variable_relative_positions:
 * @snippet: A #AnjutaSnippet object.
 *
 * A GList* of GPtrArray* with the relative positions of all snippet variables instances
 * from the start of the last computed default content.
 *
 * For example, the second element of the GPtrArray found in the first GList node has
 * the meaning of: the relative position of the second instance of the first variable
 * in the last computed default content.
 *
 * The values in the GPTrArray are gint's.
 *
 * If the default content was never computed, the method will return NULL.
 *
 * The GList should be free'd, but each GPtrArray should be just unrefed!
 *
 * Returns: A #GList with the positions or NULL on failure.
 **/
GList*	
snippet_get_variable_relative_positions	(AnjutaSnippet* snippet)
{
	GList *relative_positions_list = NULL, *iter = NULL;
	AnjutaSnippetVariable *cur_variable = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), NULL);
	g_return_val_if_fail (snippet->priv != NULL, NULL);
	g_return_val_if_fail (snippet->priv->default_computed, NULL);

	for (iter = g_list_first (snippet->priv->variables); iter != NULL; iter = g_list_next (iter))
	{
		cur_variable = (AnjutaSnippetVariable *)iter->data;

		relative_positions_list = g_list_append (relative_positions_list,
		                                         cur_variable->relative_positions);
		g_ptr_array_ref (cur_variable->relative_positions);
	}
	
	return relative_positions_list;
}

/**
 * snippet_get_variable_cur_values_len:
 * @snippet: A #AnjutaSnippet object
 *
 * Get's a #GList with the length (#gint) of each of the variables last computed
 * default value. The order is the same as for #snippet_get_variable_relative_positions.
 *
 * The #GList should be free'd.
 *
 * Returns: The requested #GList or NULL on failure.
 */
GList*
snippet_get_variable_cur_values_len (AnjutaSnippet *snippet)
{
	GList *cur_values_len_list = NULL, *iter = NULL;
	AnjutaSnippetVariable *cur_variable = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), NULL);
	g_return_val_if_fail (snippet->priv != NULL, NULL);

	for (iter = g_list_first (snippet->priv->variables); iter != NULL; iter = g_list_next (iter))
	{
		cur_variable = (AnjutaSnippetVariable *)iter->data;

		cur_values_len_list = g_list_append (cur_values_len_list,
		                                     GINT_TO_POINTER (cur_variable->cur_value_len));
	
	}

	return cur_values_len_list;	
}

gint
snippet_get_cur_value_end_position (AnjutaSnippet *snippet)
{
	AnjutaSnippetPrivate *priv = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), -1);
	priv = ANJUTA_SNIPPET_GET_PRIVATE (snippet);

	return priv->cur_value_end_position;
}

/**
 * snippet_is_equal:
 * @snippet: A #AnjutaSnippet object.
 * @snippet2: A #AnjutaSnippet object.
 *
 * Compares @snippet with @snippet2 and returns TRUE if they have the same identifier.
 *
 * Returns: TRUE if the snippets have at least one common identifier.
 */
gboolean        
snippet_is_equal (AnjutaSnippet *snippet,
                  AnjutaSnippet *snippet2)
{
	const gchar *trigger = NULL, *trigger2 = NULL, *cur_lang = NULL;
	GList *iter = NULL, *languages = NULL;
	AnjutaSnippetPrivate *priv = NULL;
	
	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), FALSE);
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), FALSE);
	priv = ANJUTA_SNIPPET_GET_PRIVATE (snippet);

	trigger  = snippet_get_trigger_key (snippet);
	trigger2 = snippet_get_trigger_key (snippet2);
	languages  = (GList *)snippet_get_languages (snippet);
	
	if (!g_strcmp0 (trigger, trigger2))
	{
		for (iter = g_list_first (languages); iter != NULL; iter = g_list_next (iter))
		{
			cur_lang = (gchar *)iter->data;
			if (snippet_has_language (snippet2, cur_lang))
				return TRUE;
		}
	}

	return FALSE;
}
