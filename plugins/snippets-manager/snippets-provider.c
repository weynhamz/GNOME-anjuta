/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    snippets-provider.c
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

#include <libanjuta/interfaces/ianjuta-provider.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-document.h>
#include <libanjuta/interfaces/ianjuta-language.h>
#include <libanjuta/interfaces/ianjuta-indicable.h>
#include "snippets-provider.h"
#include "snippet.h"
#include "snippets-group.h"


#define TRIGGER_RELEVANCE        1000
#define NAME_RELEVANCE           1000
#define FIRST_KEYWORD_RELEVANCE  100
#define KEYWORD_RELEVANCE_DEC    5
#define START_MATCH_BONUS        1.7

#define RELEVANCE(search_str_len, key_len)  ((gdouble)(search_str_len)/(key_len - search_str_len + 1))

#define IS_SEPARATOR(c)          ((c == ' ') || (c == '\n') || (c == '\t'))

#define ANJUTA_SNIPPETS_PROVIDER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
                                                   ANJUTA_TYPE_SNIPPETS_PROVIDER,\
                                                   SnippetsProviderPrivate))

struct _SnippetsProviderPrivate
{
	SnippetsDB *snippets_db;
	SnippetsInteraction *snippets_interaction;

	IAnjutaEditorAssist *editor_assist;

	gboolean request;
	gboolean listening;
	IAnjutaIterable *start_iter;
	GList *suggestions_list;

};

typedef struct _SnippetEntry
{
	AnjutaSnippet *snippet;
	gdouble relevance;
} SnippetEntry;

/* IAnjutaProvider methods declaration */

static void             snippets_provider_iface_init     (IAnjutaProviderIface* iface);
static void             snippets_provider_populate       (IAnjutaProvider *self,
                                                          IAnjutaIterable *cursor,
                                                          GError **error);
static IAnjutaIterable* snippets_provider_get_start_iter (IAnjutaProvider *self,
                                                          GError **error);
static void             snippets_provider_activate       (IAnjutaProvider *self,
                                                          IAnjutaIterable *iter,
                                                          gpointer data,
                                                          GError **error);
static const gchar*     snippets_provider_get_name       (IAnjutaProvider *self,
                                                          GError **error);

G_DEFINE_TYPE_WITH_CODE (SnippetsProvider,
			 snippets_provider,
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (IANJUTA_TYPE_PROVIDER, snippets_provider_iface_init))


static void
snippets_provider_init (SnippetsProvider *obj)
{
	SnippetsProviderPrivate *priv = ANJUTA_SNIPPETS_PROVIDER_GET_PRIVATE (obj);

	priv->snippets_db          = NULL;
	priv->snippets_interaction = NULL;

	priv->editor_assist = NULL;

	priv->request          = FALSE;
	priv->listening        = FALSE;
	priv->start_iter       = NULL;
	priv->suggestions_list = NULL;

	obj->anjuta_shell = NULL;

}

static void
snippets_provider_class_init (SnippetsProviderClass *klass)
{
	snippets_provider_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (SnippetsProviderPrivate));	

}

static void 
snippets_provider_iface_init (IAnjutaProviderIface* iface)
{
	iface->populate       = snippets_provider_populate;
	iface->get_start_iter = snippets_provider_get_start_iter;
	iface->activate       = snippets_provider_activate;
	iface->get_name       = snippets_provider_get_name;

}

/* Private methods */

static gdouble
get_relevance_for_word (const gchar *search_word,
                        const gchar *key_word)
{
	gint i = 0, search_word_len = 0, key_word_len = 0;
	gdouble relevance = 0.0, cur_relevance = 0.0;

	search_word_len = strlen (search_word);
	key_word_len    = strlen (key_word);

	for (i = 0; i < key_word_len - search_word_len + 1; i ++)
	{
		if (g_str_has_prefix (key_word + i, search_word))
		{
			cur_relevance = RELEVANCE (search_word_len, key_word_len);
			if (i == 0)
				cur_relevance *= START_MATCH_BONUS;

			relevance += cur_relevance;
		}
	}

	return relevance;
}

static gdouble
get_relevance_for_snippet (AnjutaSnippet *snippet,
                           GList *words_list)
{
	gchar *cur_word = NULL, *name = NULL, *trigger = NULL, *cur_keyword = NULL;
	gdouble relevance = 0.0, cur_relevance = 0.0, cur_keyword_relevance = 0.0;
	GList *iter = NULL, *keywords = NULL, *keywords_down = NULL, *iter2 = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), 0.0);

	/* If the user hasn't typed anything we just consider all snippets relevant */
	if (words_list == NULL)
		return 1.0;

	/* Get the snippet data */
	trigger  = g_utf8_strdown (snippet_get_trigger_key (snippet), -1);
	name     = g_utf8_strdown (snippet_get_name (snippet), -1);
	keywords = snippet_get_keywords_list (snippet);
	for (iter = g_list_first (keywords); iter != NULL; iter = g_list_next (iter))
	{
		keywords_down = g_list_append (keywords_down, 
		                               g_utf8_strdown ((gchar *)iter->data, -1));
	}
	g_list_free (keywords);

	/* We iterate over all the words */
	for (iter = g_list_first (words_list); iter != NULL; iter = g_list_next (iter))
	{
		cur_word = (gchar *)iter->data;

		/* Check the trigger-key */
		cur_relevance = get_relevance_for_word (cur_word, trigger);
		cur_relevance *= TRIGGER_RELEVANCE;
		relevance += cur_relevance;

		/* Check the name */
		cur_relevance = get_relevance_for_word (cur_word, name);
		cur_relevance *= NAME_RELEVANCE;
		relevance += cur_relevance;

		/* Check each keyword */
		cur_keyword_relevance = FIRST_KEYWORD_RELEVANCE;
		for (iter2 = g_list_first (keywords_down); iter2 != NULL; iter2 = g_list_next (iter2))
		{
			/* If we have too many keywords */
			if (cur_keyword_relevance < 0.0)
				break;

			cur_keyword = (gchar *)iter2->data;

			cur_relevance = get_relevance_for_word (cur_word, cur_keyword);
			cur_relevance *= cur_keyword_relevance;
			relevance += cur_relevance;

			cur_keyword_relevance -= KEYWORD_RELEVANCE_DEC; 
		}
	}

	return relevance;
}

static gint
snippets_relevance_sort_func (gconstpointer a,
                              gconstpointer b)
{
	IAnjutaEditorAssistProposal *proposal1 = (IAnjutaEditorAssistProposal *)a,
	                            *proposal2 = (IAnjutaEditorAssistProposal *)b;
	SnippetEntry *entry1 = (SnippetEntry *)proposal1->data,
	             *entry2 = (SnippetEntry *)proposal2->data;

	if (entry1->relevance - entry2->relevance == 0.0)
		return g_strcmp0 (snippet_get_name (entry1->snippet),
		                  snippet_get_name (entry2->snippet));

	return (gint)(entry2->relevance - entry1->relevance);
}

static IAnjutaEditorAssistProposal*
get_proposal_for_snippet (AnjutaSnippet *snippet,
                          SnippetsDB *snippets_db,
                          GList *words_list)
{
	IAnjutaEditorAssistProposal *proposal = NULL;
	SnippetEntry *entry = NULL;
	gchar *name_with_trigger = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPET (snippet), NULL);

	/* Make a new proposal object */
	proposal = g_new0 (IAnjutaEditorAssistProposal, 1);
	entry = g_new0 (SnippetEntry, 1);

	/* Fill the markup field */
	name_with_trigger = g_strconcat (snippet_get_name (snippet), " (<b>",
	                                 snippet_get_trigger_key (snippet), "</b>)",
	                                 NULL);
	proposal->markup = name_with_trigger;

#if 0
	/* Fill the info field */
	proposal->info = snippet_get_default_content (snippet, G_OBJECT (snippets_db), "");
#endif

	/* Fill the data field */
	entry->snippet   = snippet;
	entry->relevance = get_relevance_for_snippet (snippet, words_list);
	proposal->data = entry;

	return proposal;
}

static void
clear_suggestions_list (SnippetsProvider *snippets_provider)
{
	SnippetsProviderPrivate *priv = NULL;
	GList *iter = NULL;
	IAnjutaEditorAssistProposal *cur_proposal = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_PROVIDER (snippets_provider));
	priv = ANJUTA_SNIPPETS_PROVIDER_GET_PRIVATE (snippets_provider);

	for (iter = g_list_first (priv->suggestions_list); iter != NULL; iter = g_list_next (iter))
	{
		cur_proposal = (IAnjutaEditorAssistProposal *)iter->data;

		g_free (cur_proposal->markup);
		g_free (cur_proposal->data);
		g_free (cur_proposal);		
	}
	g_list_free (priv->suggestions_list);
	priv->suggestions_list = NULL;

}

static const gchar *
get_current_editor_language (SnippetsProvider *snippets_provider)
{

	IAnjutaDocumentManager *docman = NULL;
	IAnjutaDocument *doc = NULL;
	IAnjutaLanguage *ilanguage = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_PROVIDER (snippets_provider), NULL);
	g_return_val_if_fail (ANJUTA_IS_SHELL (snippets_provider->anjuta_shell), NULL);

	/* Get the document manager and assert it */
	docman = anjuta_shell_get_interface (snippets_provider->anjuta_shell, 
	                                     IAnjutaDocumentManager, 
	                                     NULL);
	g_return_val_if_fail (IANJUTA_IS_DOCUMENT_MANAGER (docman), NULL);

	/* Get the language manager and assert it */
	ilanguage = anjuta_shell_get_interface (snippets_provider->anjuta_shell,
	                                        IAnjutaLanguage,
	                                        NULL);
	g_return_val_if_fail (IANJUTA_IS_LANGUAGE (ilanguage), NULL);

	/* Get the current editor and assert it */
	doc = ianjuta_document_manager_get_current_document (docman, NULL);
	g_return_val_if_fail (IANJUTA_IS_EDITOR (doc), NULL);

	return ianjuta_language_get_name_from_editor (ilanguage,
	                                              IANJUTA_EDITOR_LANGUAGE (doc),
	                                              NULL);
}

static void
stop_listening (SnippetsProvider *snippets_provider)
{
	SnippetsProviderPrivate *priv = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_PROVIDER (snippets_provider));
	priv = ANJUTA_SNIPPETS_PROVIDER_GET_PRIVATE (snippets_provider);

	if (IANJUTA_IS_ITERABLE (priv->start_iter))
		g_object_unref (priv->start_iter);
	priv->start_iter = NULL;

	priv->request   = FALSE;
	priv->listening = FALSE;

	clear_suggestions_list (snippets_provider);

}


static void
build_suggestions_list (SnippetsProvider *snippets_provider,
                        IAnjutaIterable *cur_cursor_position)
{
	SnippetsProviderPrivate *priv = NULL;
	GtkTreeIter iter, iter2;
	GObject *cur_object = NULL;
	IAnjutaEditorAssistProposal *cur_proposal = NULL;
	SnippetEntry *cur_entry = NULL;
	gchar *search_string = NULL, **words = NULL;
	gboolean show_all_languages = FALSE;
	const gchar *language = NULL;
	gint i = 0;
	GList *words_list = NULL, *l_iter = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_PROVIDER (snippets_provider));
	g_return_if_fail (IANJUTA_IS_ITERABLE (cur_cursor_position));
	priv = ANJUTA_SNIPPETS_PROVIDER_GET_PRIVATE (snippets_provider);
	g_return_if_fail (IANJUTA_IS_ITERABLE (cur_cursor_position));
	g_return_if_fail (ANJUTA_IS_SNIPPETS_DB (priv->snippets_db));
	g_return_if_fail (priv->suggestions_list == NULL);

	if (ianjuta_iterable_diff (priv->start_iter, cur_cursor_position, NULL) < 0)
	{
		stop_listening (snippets_provider);
		return;
	}

	/* Get the language of the current editor */
	language = get_current_editor_language (snippets_provider);
	if (!language)
		show_all_languages = TRUE;

	/* Get the current searching string */
	search_string = ianjuta_editor_get_text (IANJUTA_EDITOR (priv->editor_assist),
	                                         priv->start_iter,
	                                         cur_cursor_position,
	                                         NULL);
	if (search_string == NULL)
		search_string = g_strdup ("");

	/* Split the search string into words and build the words list with non empty
	   words */
	words = g_strsplit (search_string, " ", 0);
	while (words[i])
	{
		if (g_strcmp0 (words[i], ""))
			words_list = g_list_append (words_list, g_utf8_strdown (words[i], -1));

		i ++;
	}
	g_strfreev (words);

	if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->snippets_db), &iter))
		return;

	do
	{
		/* Get the children for the current snippets group */
		if (!gtk_tree_model_iter_children (GTK_TREE_MODEL (priv->snippets_db), &iter2, &iter))
			continue;

		/* Iterate over the snippets */
		do
		{
			gtk_tree_model_get (GTK_TREE_MODEL (priv->snippets_db), &iter2,
			                    SNIPPETS_DB_MODEL_COL_CUR_OBJECT, &cur_object,
			                    -1);
			g_object_unref (cur_object);

			if (!ANJUTA_IS_SNIPPET (cur_object))
				continue;

			/* If the current snippet isn't meant for the current editor language, we ignore it */
			if (!show_all_languages && !snippet_has_language (ANJUTA_SNIPPET (cur_object), language))
				continue;
			
			/* Build a proposal for the current snippet based on the words the user typed */
			cur_proposal = get_proposal_for_snippet (ANJUTA_SNIPPET (cur_object),
			                                         priv->snippets_db,
			                                         words_list);

			/* If the snippet isn't relevant for the typed text, we neglect this proposal */
			cur_entry = (SnippetEntry *)cur_proposal->data;
			if (cur_entry->relevance == 0.0)
			{
				g_free (cur_entry);
				g_free (cur_proposal->markup);
				g_free (cur_proposal);

				continue;
			}

			/* We add the proposal to the suggestions list */
			priv->suggestions_list = g_list_insert_sorted (priv->suggestions_list,
			                                               cur_proposal,
			                                               snippets_relevance_sort_func);

		} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->snippets_db), &iter2));

	} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->snippets_db), &iter));

	/* Free the data */
	g_free (search_string);
	for (l_iter = g_list_first (words_list); l_iter != NULL; l_iter = g_list_next (l_iter))
		g_free (l_iter->data);
	g_list_free (words_list);
	
}

static gchar
char_before_iterator (IAnjutaEditor *editor,
                      IAnjutaIterable *iter)
{
	IAnjutaIterable *prev = NULL;
	gchar *text = NULL, returned_char = 0;
	
	/* Assertions */
	g_return_val_if_fail (IANJUTA_IS_EDITOR (editor), 0);
	g_return_val_if_fail (IANJUTA_IS_ITERABLE (iter), 0);

	prev = ianjuta_iterable_clone (iter, NULL);
	ianjuta_iterable_previous (prev, NULL);
	
	text = ianjuta_editor_get_text (editor, prev, iter, NULL);
	if (text == NULL)
		return 0;

	returned_char = text[0];
	g_free (text);
	g_object_unref (prev);

	return returned_char;	
}

static IAnjutaIterable *
get_start_iter_for_cursor (IAnjutaEditor *editor,
                           IAnjutaIterable *cursor)
{
	IAnjutaIterable *iter = NULL;
	gchar cur_char;

	/* Assertions */
	g_return_val_if_fail (IANJUTA_IS_EDITOR (editor), NULL);
	g_return_val_if_fail (IANJUTA_IS_ITERABLE (cursor), NULL);

	/* Go backwards in the text until we get to the start or find a separator */
	iter = ianjuta_iterable_clone (cursor, NULL);
	cur_char = char_before_iterator (editor, iter);
	while (cur_char && !IS_SEPARATOR (cur_char))
	{
		if (!ianjuta_iterable_previous (iter, NULL))
			break;

		cur_char = char_before_iterator (editor, iter);
	}

	return iter;
}

/* Public methods */

SnippetsProvider*
snippets_provider_new (SnippetsDB *snippets_db,
                       SnippetsInteraction *snippets_interaction)
{
	SnippetsProvider *snippets_provider = NULL;
	SnippetsProviderPrivate *priv = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_DB (snippets_db), NULL);
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_INTERACTION (snippets_interaction), NULL);

	snippets_provider = ANJUTA_SNIPPETS_PROVIDER (g_object_new (snippets_provider_get_type (), NULL));
	priv = ANJUTA_SNIPPETS_PROVIDER_GET_PRIVATE (snippets_provider);

	priv->snippets_db          = snippets_db;
	priv->snippets_interaction = snippets_interaction;

	return snippets_provider;
}


void
snippets_provider_load (SnippetsProvider *snippets_provider,
                        IAnjutaEditorAssist *editor_assist)
{
	SnippetsProviderPrivate *priv = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_PROVIDER (snippets_provider));
	g_return_if_fail (IANJUTA_IS_EDITOR_ASSIST (editor_assist));
	priv = ANJUTA_SNIPPETS_PROVIDER_GET_PRIVATE (snippets_provider);
	/* Should be removed with unload */
	g_return_if_fail (!IANJUTA_IS_EDITOR_ASSIST (priv->editor_assist)); 

	/* Add the snippets_provider to the editor assist */
	ianjuta_editor_assist_add (editor_assist, 
	                           IANJUTA_PROVIDER (snippets_provider), 
	                           NULL);
	priv->editor_assist = editor_assist;

	priv->request = FALSE;
	priv->listening = FALSE;

}

void
snippets_provider_unload (SnippetsProvider *snippets_provider)
{
	SnippetsProviderPrivate *priv = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_PROVIDER (snippets_provider));
	priv = ANJUTA_SNIPPETS_PROVIDER_GET_PRIVATE (snippets_provider);

	/* If we don't have anything to unload, we just return. This isn't an assertion
	   because it's possible to call this function if the previous document wasn't
	   an editor. */
	if (!IANJUTA_IS_EDITOR_ASSIST (priv->editor_assist))
		return;

	/* Remove the snippets_provider from the editor assist and mark as NULL the one
	   saved internal. */
	ianjuta_editor_assist_remove (priv->editor_assist,
	                              IANJUTA_PROVIDER (snippets_provider),
	                              NULL);
	priv->editor_assist = NULL;

	/* Stop listening if necessary */
	stop_listening (snippets_provider);

}

void
snippets_provider_request (SnippetsProvider *snippets_provider)
{
	SnippetsProviderPrivate *priv = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_PROVIDER (snippets_provider));
	priv = ANJUTA_SNIPPETS_PROVIDER_GET_PRIVATE (snippets_provider);
	g_return_if_fail (ANJUTA_IS_SHELL (snippets_provider->anjuta_shell));

	/* If we don't have an editor assist loaded we just return. */
	if (!IANJUTA_IS_EDITOR_ASSIST (priv->editor_assist))
		return;

	/* We just made the request and should listen with the populate method. */
	priv->request   = TRUE;
	priv->listening = TRUE;

	/* Clear the iter if need */
	if (IANJUTA_IS_ITERABLE (priv->start_iter))
		g_object_unref (priv->start_iter);
	priv->start_iter = NULL;

	/* Show the auto-complete widget */
	ianjuta_editor_assist_invoke (priv->editor_assist,
	                              IANJUTA_PROVIDER (snippets_provider),
	                              NULL);

}


/* IAnjutaProvider methods declarations */

static void
snippets_provider_populate (IAnjutaProvider *self,
                            IAnjutaIterable *cursor,
                            GError **error)
{
	SnippetsProviderPrivate *priv = NULL;
	SnippetsProvider *snippets_provider = NULL;

	/* Assertions */
	snippets_provider = ANJUTA_SNIPPETS_PROVIDER (self);
	g_return_if_fail (ANJUTA_IS_SNIPPETS_PROVIDER (snippets_provider));
	priv = ANJUTA_SNIPPETS_PROVIDER_GET_PRIVATE (snippets_provider);
	g_return_if_fail (ANJUTA_IS_SHELL (snippets_provider->anjuta_shell));

	/* If we aren't listening */
	if (!priv->listening)
	{
		ianjuta_editor_assist_proposals (priv->editor_assist, self, NULL, TRUE, NULL);
		return;
	}

	/* If the request just happened */
	if (priv->request)
	{
		/* Save the new cursor as the starting one */
/*		priv->start_iter = get_start_iter_for_cursor (IANJUTA_EDITOR (priv->editor_assist),*/
/*		                                              cursor);*/
		/* TODO - seems to feel better if it starts at the current cursor position.
		   Keeping the old method also if it will be decided to use that method.
		   Note: get_start_iter_for_cursor goes back in the text until it finds a
		   separator. */
		priv->start_iter = ianjuta_iterable_clone (cursor, NULL);
		priv->request = FALSE;
	}

	clear_suggestions_list (snippets_provider);
	build_suggestions_list (snippets_provider, cursor);

	/* Clear the previous indicator */
	if (IANJUTA_IS_INDICABLE (priv->editor_assist))
		ianjuta_indicable_clear (IANJUTA_INDICABLE (priv->editor_assist), NULL);

	if (priv->suggestions_list == NULL)
	{
		stop_listening (snippets_provider);
		ianjuta_editor_assist_proposals (priv->editor_assist, self, NULL, TRUE, NULL);
		return;
	}

	/* Highlight the search string */
	if (IANJUTA_IS_INDICABLE (priv->editor_assist))
	{
		ianjuta_indicable_set (IANJUTA_INDICABLE (priv->editor_assist),
		                       priv->start_iter,
		                       cursor,
		                       IANJUTA_INDICABLE_IMPORTANT,
		                       NULL);
	}

	ianjuta_editor_assist_proposals (priv->editor_assist,
	                                 self,
	                                 priv->suggestions_list,
	                                 TRUE, NULL);

}

static IAnjutaIterable*
snippets_provider_get_start_iter (IAnjutaProvider *self,
                                  GError **error)
{
	SnippetsProviderPrivate *priv = NULL;

	/* Assertions */
	g_return_val_if_fail (ANJUTA_IS_SNIPPETS_PROVIDER (self), NULL);
	priv = ANJUTA_SNIPPETS_PROVIDER_GET_PRIVATE (self);

	return priv->start_iter;
}

static void
snippets_provider_activate (IAnjutaProvider *self,
                            IAnjutaIterable *iter,
                            gpointer data,
                            GError **error)
{
	SnippetsProviderPrivate *priv = NULL;
	AnjutaSnippet *snippet = NULL;

	/* Assertions */
	g_return_if_fail (ANJUTA_IS_SNIPPETS_PROVIDER (self));
	g_return_if_fail (IANJUTA_IS_ITERABLE (iter));
	priv = ANJUTA_SNIPPETS_PROVIDER_GET_PRIVATE (self);
	g_return_if_fail (IANJUTA_IS_ITERABLE (priv->start_iter));
	g_return_if_fail (IANJUTA_IS_EDITOR (priv->editor_assist));

	/* Get the Snippet and assert it */
	snippet = ((SnippetEntry *)data)->snippet;
	g_return_if_fail (ANJUTA_IS_SNIPPET (snippet));

	/* Erase the text */
	ianjuta_editor_erase (IANJUTA_EDITOR (priv->editor_assist),
	                      priv->start_iter,
	                      iter,
	                      NULL);

	/* Set the cursor at the start iter */
	ianjuta_editor_goto_position (IANJUTA_EDITOR (priv->editor_assist),
	                              priv->start_iter,
	                              NULL);

	/* Clear the indicator */
	if (IANJUTA_IS_INDICABLE (priv->editor_assist))
		ianjuta_indicable_clear (IANJUTA_INDICABLE (priv->editor_assist), NULL);

	/* Insert the snippet */
	snippets_interaction_insert_snippet (priv->snippets_interaction,
	                                     priv->snippets_db,
	                                     snippet);

	stop_listening (ANJUTA_SNIPPETS_PROVIDER (self));
}

static const gchar*
snippets_provider_get_name (IAnjutaProvider *self,
                            GError **error)
{
	return _("Code Snippets");
}

