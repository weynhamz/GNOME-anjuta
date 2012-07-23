/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-language-provider.c
 * Copyright (C) Naba Kumar  <naba@gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include <ctype.h>
#include <string.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-language-provider.h>
#include <libanjuta/interfaces/ianjuta-document.h>
#include <libanjuta/interfaces/ianjuta-editor-assist.h>
#include <libanjuta/interfaces/ianjuta-editor-cell.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-editor-tip.h>
#include <libanjuta/interfaces/ianjuta-language-provider.h>
#include <libanjuta/interfaces/ianjuta-provider.h>

#define SCOPE_BRACE_JUMP_LIMIT 50
#define BRACE_SEARCH_LIMIT 500

G_DEFINE_TYPE (AnjutaLanguageProvider, anjuta_language_provider, G_TYPE_OBJECT);

struct _AnjutaLanguageProviderPriv {
	GSettings* settings;
	IAnjutaEditorAssist* iassist;
	IAnjutaEditorTip* itip;

	/* Autocompletion */
	IAnjutaIterable* start_iter;
};

/**
 * anjuta_language_provider_install:
 * @lang_prov: Self
 * @ieditor: IAnjutaEditor object
 * @settings: the settings
 *
 * Install the settings for AnjutaLanguageProvider
 */
void 
anjuta_language_provider_install (AnjutaLanguageProvider *lang_prov,
                                  IAnjutaEditor *ieditor,
                                  GSettings* settings)
{
	g_return_if_fail (lang_prov->priv->iassist == NULL);

	if (IANJUTA_IS_EDITOR_ASSIST (ieditor))
		lang_prov->priv->iassist = IANJUTA_EDITOR_ASSIST (ieditor);
	else
		lang_prov->priv->iassist = NULL;

	if (IANJUTA_IS_EDITOR_TIP (ieditor))
		lang_prov->priv->itip = IANJUTA_EDITOR_TIP (ieditor);
	else
		lang_prov->priv->itip = NULL;
	
	lang_prov->priv->settings = settings;
}

static void
anjuta_language_provider_uninstall (AnjutaLanguageProvider *lang_prov)
{
	g_return_if_fail (lang_prov->priv->iassist != NULL);

	lang_prov->priv->iassist = NULL;
}

static void
anjuta_language_provider_init (AnjutaLanguageProvider *lang_prov)
{
	lang_prov->priv = g_new0 (AnjutaLanguageProviderPriv, 1);
}

static void
anjuta_language_provider_finalize (GObject *object)
{
	AnjutaLanguageProvider *lang_prov;
	
	lang_prov = ANJUTA_LANGUAGE_PROVIDER (object);
	
	anjuta_language_provider_uninstall (lang_prov);
	g_free (lang_prov->priv);

	G_OBJECT_CLASS (anjuta_language_provider_parent_class)->finalize (object);
}

static void
anjuta_language_provider_class_init (AnjutaLanguageProviderClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = anjuta_language_provider_finalize;
}

/**
 * anjuta_language_provider_find_next_brace:
 * @iter: Iter to start searching at
 *
 * Returns: The position of the brace, if the next non-whitespace character is a
 * opening brace, NULL otherwise
 */
static IAnjutaIterable*
anjuta_language_provider_find_next_brace (IAnjutaIterable* iter)
{
	IAnjutaIterable* current_iter = ianjuta_iterable_clone (iter, NULL);
	gchar ch;
	do
	{
		ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (current_iter),
		                                   0, NULL);
		if (ch == '(')
			return current_iter;
	}
	while (g_ascii_isspace (ch) && ianjuta_iterable_next (current_iter, NULL));
	
	g_object_unref (current_iter);
	return NULL;
}

/**
 * anjuta_language_provider_find_whitespace:
 * @iter: Iter to start searching at
 *
 * Returns: TRUE if the next character is a whitespace character,
 * FALSE otherwise
 */
static gboolean
anjuta_language_provider_find_whitespace (IAnjutaIterable* iter)
{
	gchar ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter),
	                                         0, NULL);
	if (g_ascii_isspace (ch) && ch != '\n'
	    && anjuta_language_provider_find_next_brace (iter))
	{
		return TRUE;
	}
	else
		return FALSE;
}

/**
 * anjuta_language_provider_is_character:
 * @ch: character to check
 * @context_characters: language-specific context characters
 *                      the end is marked with a '0' character
 *
 * Returns: if the current character seperates a scope
 */
static gboolean
anjuta_language_provider_is_character (gchar ch, const gchar* characters)
{
	int i;
	
	if (g_ascii_isspace (ch))
		return FALSE;
	if (g_ascii_isalnum (ch))
		return TRUE;
	for (i = 0; characters[i] != '0'; i++)
	{
		if (ch == characters[i])
			return TRUE;
	}
	
	return FALSE;
}

/**
 * anjuta_language_provider_get_scope_context
 * @editor: current editor
 * @iter: Current cursor position
 * @scope_context_ch: language-specific context characters
 *                    the end is marked with a '0' character
 *
 * Find the scope context for calltips
 */
static gchar*
anjuta_language_provider_get_scope_context (IAnjutaEditor* editor,
                                            IAnjutaIterable *iter,
                                            const gchar* scope_context_ch)
{
	IAnjutaIterable* end;	
	gchar ch, *scope_chars = NULL;
	gboolean out_of_range = FALSE;
	gboolean scope_chars_found = FALSE;
	
	end = ianjuta_iterable_clone (iter, NULL);
	ianjuta_iterable_next (end, NULL);
	
	ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0, NULL);
	
	while (ch)
	{
		if (anjuta_language_provider_is_character (ch, scope_context_ch))
			scope_chars_found = TRUE;
		else if (ch == ')')
		{
			if (!anjuta_util_jump_to_matching_brace (iter, ch,
						                             SCOPE_BRACE_JUMP_LIMIT))
			{
				out_of_range = TRUE;
				break;
			}
		}
		else
			break;
		if (!ianjuta_iterable_previous (iter, NULL))
		{
			out_of_range = TRUE;
			break;
		}		
		ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0, NULL);
	}
	if (scope_chars_found)
	{
		IAnjutaIterable* begin;
		begin = ianjuta_iterable_clone (iter, NULL);
		if (!out_of_range)
			ianjuta_iterable_next (begin, NULL);
		scope_chars = ianjuta_editor_get_text (editor, begin, end, NULL);
		g_object_unref (begin);
	}
	g_object_unref (end);
	return scope_chars;
}

/**
 * anjuta_language_provider_get_calltip_context:
 * @itip: whether a tooltip is crrently shown
 * @iter: current cursor position
 * @scope_context_ch: language-specific context characters
 *                    the end is marked with a '0' character
 *
 * Searches for a calltip context
 *
 * Returns: name of the method to show a calltip for or NULL
 */
gchar*
anjuta_language_provider_get_calltip_context (AnjutaLanguageProvider* lang_prov,
                                              IAnjutaEditorTip* itip,
		                                      IAnjutaIterable* iter,
                                              const gchar* scope_context_ch)
{
	gchar ch;
	gchar *context = NULL;
	
	ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0, NULL);
	if (ch == ')')
	{
		if (!anjuta_util_jump_to_matching_brace (iter, ')', -1))
			return NULL;
		if (!ianjuta_iterable_previous (iter, NULL))
			return NULL;
	}
	if (ch != '(')
	{
		if (!anjuta_util_jump_to_matching_brace (iter, ')', BRACE_SEARCH_LIMIT))
			return NULL;
	}
	
	/* Skip white spaces */
	while (ianjuta_iterable_previous (iter, NULL)
		&& g_ascii_isspace (ianjuta_editor_cell_get_char
								(IANJUTA_EDITOR_CELL (iter), 0, NULL)));
	
	context = anjuta_language_provider_get_scope_context (IANJUTA_EDITOR (itip),
			                                          iter,
													  scope_context_ch);

	/* Point iter to the first character of the scope to align calltip correctly */
	ianjuta_iterable_next (iter, NULL);
	
	return context;
}

/**
 * anjuta_language_provider_get_pre_word:
 * @lang_prov: Self
 * @editor: IAnjutaEditor object
 * @iter: current cursor position
 * @start_iter: return location for the start_iter (if a preword was found)
 *
 * Search for the current typed word
 *
 * Returns: The current word (needs to be freed) or NULL if no word was found
 */
gchar*
anjuta_language_provider_get_pre_word (AnjutaLanguageProvider* lang_prov,
                                       IAnjutaEditor* editor,
                                       IAnjutaIterable* iter,
                                       IAnjutaIterable** start_iter,
                                       const gchar* word_characters)
{
	IAnjutaIterable *end = ianjuta_iterable_clone (iter, NULL);
	IAnjutaIterable *begin = ianjuta_iterable_clone (iter, NULL);
	gchar ch, *preword_chars = NULL;
	gboolean out_of_range = FALSE;
	gboolean preword_found = FALSE;
	
	/* Cursor points after the current characters, move back */
	ianjuta_iterable_previous (begin, NULL);
	
	ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (begin), 0, NULL);
	
	while (ch && anjuta_language_provider_is_character (ch, word_characters))
	{
		preword_found = TRUE;
		if (!ianjuta_iterable_previous (begin, NULL))
		{
			out_of_range = TRUE;
			break;
		}
		ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (begin), 0, NULL);
	}
	
	if (preword_found)
	{
		if (!out_of_range)
			ianjuta_iterable_next (begin, NULL);
		preword_chars = ianjuta_editor_get_text (editor, begin, end, NULL);
		*start_iter = begin;
	}
	else
	{
		g_object_unref (begin);
		*start_iter = NULL;
	}
	
	g_object_unref (end);
	return preword_chars;
}

/**
 * anjuta_language_provider_calltip:
 * @lang_prov: Self
 * @provider: IAnjutaLanguageProvider object
 * 
 * Creates a calltip if there is something to show a tip for
 * Calltips are queried async
 *
 * Returns: TRUE if a calltips was queried, FALSE otherwise
 */
static gboolean
anjuta_language_provider_calltip (AnjutaLanguageProvider* lang_prov,
                                  IAnjutaLanguageProvider* provider)
{
	IAnjutaIterable *iter;
	IAnjutaEditorTip *tip;
	gchar *call_context;
	
	tip = lang_prov->priv->itip;
	iter = ianjuta_editor_get_position (IANJUTA_EDITOR (lang_prov->priv->iassist),
	                                    NULL);
	ianjuta_iterable_previous (iter, NULL);
	
	call_context = ianjuta_language_provider_get_calltip_context (provider,
	                                                              iter, NULL);
	if (call_context)
	{
		DEBUG_PRINT ("Searching calltip for: %s", call_context);
		GList *tips = ianjuta_language_provider_get_calltip_cache (provider,
		                                                           call_context,
		                                                           NULL);
		if (tips)
		{
			/* Continue tip */
			if (!ianjuta_editor_tip_visible (tip, NULL))
				ianjuta_editor_tip_show (tip, tips, iter, NULL);
		}
		else
		{
			/* New tip */
			if (ianjuta_editor_tip_visible (tip, NULL))
				ianjuta_editor_tip_cancel (tip, NULL);
			ianjuta_language_provider_new_calltip (provider, call_context, iter,
			                                       NULL);
		}
		
		g_free (call_context);
		return TRUE;
	}
	else
	{
		if (ianjuta_editor_tip_visible (tip, NULL))
			ianjuta_editor_tip_cancel (tip, NULL);
		
		g_object_unref (iter);
		return FALSE;
	}
}

/**
 * anjuta_language_provider_none:
 * @lang_prov: Self
 * @provider: IAnjutaLanguageProvider object
 *
 * Indicate that there is nothing to autocomplete
 */
static void
anjuta_language_provider_none (AnjutaLanguageProvider* lang_prov,
                               IAnjutaProvider * provider)
{
	ianjuta_editor_assist_proposals (lang_prov->priv->iassist, provider, NULL,
	                                 NULL, TRUE, NULL);
}

/**
 * anjuta_language_provider_activate:
 * @lang_prov: Self
 * @iprov: IAnjutaProvider object
 * @iter: the cursor
 * @data: the ProposalData
 *
 * Complete the function name
 */
void
anjuta_language_provider_activate (AnjutaLanguageProvider* lang_prov,
                                   IAnjutaProvider* iprov,
                                   IAnjutaIterable* iter,
                                   gpointer data)
{
	IAnjutaLanguageProviderProposalData *prop_data;
	GString *assistance;
	IAnjutaEditor *editor = IANJUTA_EDITOR (lang_prov->priv->iassist);
	gboolean add_space_after_func = FALSE;
	gboolean add_brace_after_func = FALSE;
	gboolean add_closebrace_after_func = FALSE;
	
	g_return_if_fail (data != NULL);
	prop_data = data;
	assistance = g_string_new (prop_data->name);
	
	if (prop_data->is_func)
	{
		IAnjutaIterable* next_brace = anjuta_language_provider_find_next_brace (iter);
		add_space_after_func = g_settings_get_boolean (lang_prov->priv->settings,
			IANJUTA_LANGUAGE_PROVIDER_PREF_AUTOCOMPLETE_SPACE_AFTER_FUNC);
		add_brace_after_func = g_settings_get_boolean (lang_prov->priv->settings,
		    IANJUTA_LANGUAGE_PROVIDER_PREF_AUTOCOMPLETE_BRACE_AFTER_FUNC);
		add_closebrace_after_func = g_settings_get_boolean (lang_prov->priv->settings,
		    IANJUTA_LANGUAGE_PROVIDER_PREF_AUTOCOMPLETE_CLOSEBRACE_AFTER_FUNC);

		if (add_space_after_func
			&& !anjuta_language_provider_find_whitespace (iter))
			g_string_append (assistance, " ");
		if (add_brace_after_func && !next_brace)
			g_string_append (assistance, "(");
		else
			g_object_unref (next_brace);
	}
		
	ianjuta_document_begin_undo_action (IANJUTA_DOCUMENT (editor), NULL);
	
	if (ianjuta_iterable_compare (iter, lang_prov->priv->start_iter, NULL) != 0)
	{
		ianjuta_editor_selection_set (IANJUTA_EDITOR_SELECTION (editor),
									  lang_prov->priv->start_iter, iter, FALSE,
									  NULL);
		ianjuta_editor_selection_replace (IANJUTA_EDITOR_SELECTION (editor),
										  assistance->str, -1, NULL);
	}
	else
		ianjuta_editor_insert (editor, iter, assistance->str, -1, NULL);
	
	if (add_brace_after_func && add_closebrace_after_func)
	{
		IAnjutaIterable *next_brace;
		IAnjutaIterable *pos = ianjuta_iterable_clone (iter, NULL);

		ianjuta_iterable_set_position (pos,
		                               ianjuta_iterable_get_position (
									       lang_prov->priv->start_iter, NULL)
									   + strlen (assistance->str),
									   NULL);
		next_brace = anjuta_language_provider_find_next_brace (pos);
		if (!next_brace)
			ianjuta_editor_insert (editor, pos, ")", -1, NULL);
		else
		{
			pos = next_brace;
			ianjuta_iterable_next (pos, NULL);
		}
		
		ianjuta_editor_goto_position (editor, pos, NULL);

		ianjuta_iterable_previous (pos, NULL);
		if (!prop_data->has_para)
		{
			pos = ianjuta_editor_get_position (editor, NULL);
			ianjuta_iterable_next (pos, NULL);
			ianjuta_editor_goto_position (editor, pos, NULL);
		}
		
		g_object_unref (pos);
	}

	ianjuta_document_end_undo_action (IANJUTA_DOCUMENT (editor), NULL);

	/* Show calltip if we completed function */
	if (add_brace_after_func)
	{
		/* Check for calltip */
		if (lang_prov->priv->itip
		    && g_settings_get_boolean (lang_prov->priv->settings,
		                        IANJUTA_LANGUAGE_PROVIDER_PREF_CALLTIP_ENABLE))
		{
			anjuta_language_provider_calltip (
				                lang_prov, IANJUTA_LANGUAGE_PROVIDER (iprov));
		}
	}
	g_string_free (assistance, TRUE);
}

/**
 * anjuta_language_provider_populate:
 * @lang_prov: Self
 * @iprov: IAnjutaProvider object
 * @cursor: the text iter where the provider should be populated
 *
 * Show completion for the context at position @iter. The provider should
 * call ianjuta_editor_assist_proposals here to add proposals to the list.
 */
void
anjuta_language_provider_populate (AnjutaLanguageProvider* lang_prov,
                                   IAnjutaProvider* iprov,
                                   IAnjutaIterable* cursor)
{
	IAnjutaIterable *start_iter;
	
	/* Check if this is a valid text region for completion */
	IAnjutaEditorAttribute attrib = ianjuta_editor_cell_get_attribute (
	                                        IANJUTA_EDITOR_CELL(cursor), NULL);
	if (attrib == IANJUTA_EDITOR_COMMENT || attrib == IANJUTA_EDITOR_STRING)
	{
		anjuta_language_provider_none (lang_prov, iprov);
		return;
	}

	/* Check for calltip */
	if (g_settings_get_boolean (lang_prov->priv->settings,
	                            IANJUTA_LANGUAGE_PROVIDER_PREF_CALLTIP_ENABLE))
	{
		anjuta_language_provider_calltip (lang_prov,
		                                  IANJUTA_LANGUAGE_PROVIDER (iprov));
	}

	/* Check if we actually want autocompletion at all */
	if (!g_settings_get_boolean (lang_prov->priv->settings,
		            IANJUTA_LANGUAGE_PROVIDER_PREF_AUTOCOMPLETE_ENABLE))
	{
		anjuta_language_provider_none (lang_prov, iprov);
		return;
	}
	
	/* Execute language-specific part */
	start_iter = ianjuta_language_provider_populate_language (
	                     IANJUTA_LANGUAGE_PROVIDER (iprov), cursor, NULL);
	if (start_iter)
	{
		if (lang_prov->priv->start_iter)
			g_object_unref (lang_prov->priv->start_iter);
		lang_prov->priv->start_iter = start_iter;
		return;
	}
	
	/* Nothing to propose */
	if (lang_prov->priv->start_iter)
	{
		g_object_unref (lang_prov->priv->start_iter);
		lang_prov->priv->start_iter = NULL;
	}
	anjuta_language_provider_none (lang_prov, iprov);
}

/**
 * anjuta_language_provider_get_start_iter:
 * @lang_prov: Self
 *
 * Returns: (transfer full): the start iter
 */
IAnjutaIterable*
anjuta_language_provider_get_start_iter (AnjutaLanguageProvider* lang_prov)
{
	return lang_prov->priv->start_iter;
}
