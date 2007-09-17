/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * cpp-java-assist.c
 * Copyright (C)  2007 Naba Kumar  <naba@gnome.org>
 *                     Johannes Schmid  <jhs@gnome.org>
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

#include <ctype.h>
#include <string.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-editor-cell.h>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>
#include "cpp-java-assist.h"
#include "cpp-java-utils.h"

#define PREF_AUTOCOMPLETE_ENABLE "language.cpp.code.completion.enable"
#define PREF_AUTOCOMPLETE_CHOICES "language.cpp.code.completion.choices"
#define PREF_CALLTIP_ENABLE "language.cpp.code.calltip.enable"
#define MAX_COMPLETIONS 10

G_DEFINE_TYPE (CppJavaAssist, cpp_java_assist, G_TYPE_OBJECT);

struct _CppJavaAssistPriv {
	AnjutaPreferences *preferences;
	IAnjutaSymbolManager* isymbol_manager;
	IAnjutaEditorAssist* iassist;
	GQueue *assist_queue;
};

static void
add_tags (IAnjutaEditorAssist* iassist, IAnjutaIterable* iter,
		  CppJavaAssistContext *assist_ctx)
{
	GHashTable *suggestions_hash = g_hash_table_new (g_str_hash, g_str_equal);
	GList* suggestions = NULL;
	do
	{
		const gchar* name = ianjuta_symbol_name (IANJUTA_SYMBOL(iter), NULL);
		if (name != NULL)
		{
			if (!g_hash_table_lookup (suggestions_hash, name))
			{
				g_hash_table_insert (suggestions_hash, (gchar*)name,
									 (gchar*)name);
				suggestions = g_list_prepend (suggestions, g_strdup (name));
			}
		}
		else
			break;
	}
	while (ianjuta_iterable_next (iter, NULL));
	
	g_hash_table_destroy (suggestions_hash);
	
	suggestions = g_list_sort (suggestions, (GCompareFunc) strcmp);
	if (assist_ctx->completion == NULL)
		assist_ctx->completion = g_completion_new (NULL);
	g_completion_add_items (assist_ctx->completion, suggestions);
}

static void
on_assist_begin (IAnjutaEditorAssist* iassist, gchar* context,
				 gchar* trigger, CppJavaAssist *assist)
{
	CppJavaAssistContext *assist_ctx;
	gint max_completions;
	gint position = ianjuta_editor_get_position (IANJUTA_EDITOR (iassist), NULL);
	IAnjutaIterable* cell =
		ianjuta_editor_get_cell_iter (IANJUTA_EDITOR(iassist),
									  position, NULL);
	gboolean enable_complete =
		anjuta_preferences_get_int_with_default (assist->priv->preferences,
												 PREF_AUTOCOMPLETE_ENABLE,
												 TRUE);
	gboolean enable_calltips =
		anjuta_preferences_get_int_with_default (assist->priv->preferences,
												 PREF_CALLTIP_ENABLE,
												 TRUE);
	IAnjutaEditorAttribute attribute =
		ianjuta_editor_cell_get_attribute (IANJUTA_EDITOR_CELL (cell), NULL);
	gboolean non_scope = (attribute == IANJUTA_EDITOR_COMMENT ||
						  attribute == IANJUTA_EDITOR_STRING);
	
	
	DEBUG_PRINT("assist-begin: %s", context);
	if (non_scope || context == NULL || strlen(context) < 3)
	{
		return;
	}
	
	assist_ctx = g_new0 (CppJavaAssistContext, 1);
	assist_ctx->completion = NULL;
	assist_ctx->tips = NULL;
	assist_ctx->position = 0;
	
	g_queue_push_head (assist->priv->assist_queue, assist_ctx);
	
	max_completions =
		anjuta_preferences_get_int_with_default (assist->priv->preferences,
												 PREF_AUTOCOMPLETE_CHOICES,
												 MAX_COMPLETIONS);
	/* General word completion */
	if (!trigger && enable_complete)
	{
		IAnjutaIterable* iter = 
			ianjuta_symbol_manager_search (assist->priv->isymbol_manager,
										    IANJUTA_SYMBOL_TYPE_MAX,
											context, TRUE, TRUE, NULL);
		if (iter)
		{
			position = ianjuta_editor_get_position(IANJUTA_EDITOR(iassist), NULL)
				- strlen(context);
			assist_ctx->position = position;
			add_tags (iassist, iter, assist_ctx);
			g_completion_complete (assist_ctx->completion, context, NULL);
			ianjuta_editor_assist_init_suggestions(iassist, position, NULL);
			if (g_list_length (assist_ctx->completion->cache) < max_completions)
				ianjuta_editor_assist_suggest (iassist,
											   assist_ctx->completion->items,
											   position,
											   strlen (context), NULL);
			g_object_unref (iter);
			return;
		}
	}
	
	if (!trigger)
	{
		return;
	}
	else if (enable_complete && g_str_equal(trigger, "::"))
	{
		IAnjutaIterable* iter = 
			ianjuta_symbol_manager_get_members (assist->priv->isymbol_manager,
												context, TRUE, NULL);
		if (iter)
		{
			position = ianjuta_editor_get_position (IANJUTA_EDITOR(iassist), NULL);
			assist_ctx->position = position;
			add_tags (iassist, iter, assist_ctx);
			ianjuta_editor_assist_init_suggestions(iassist, position, NULL);
			if (g_list_length (assist_ctx->completion->items) < max_completions)
				ianjuta_editor_assist_suggest (iassist,
											   assist_ctx->completion->items,
											   position, 0, NULL);
			g_object_unref (iter);
			return;
		}
	}
	else if (enable_complete && (g_str_equal(trigger, ".") ||
								 g_str_equal(trigger, "->")))
	{
		assist_ctx->position = position;
		/* TODO: Find the type of context by parsing the file somehow and
		search for the member as it is done with the :: context */
	}
	else if (g_str_equal(trigger, "(") && enable_calltips && strlen(context) > 2)
	{
		IAnjutaIterable* iter = 
			ianjuta_symbol_manager_search (assist->priv->isymbol_manager,
										   IANJUTA_SYMBOL_TYPE_PROTOTYPE|
										   IANJUTA_SYMBOL_TYPE_FUNCTION|
										   IANJUTA_SYMBOL_TYPE_METHOD|
										   IANJUTA_SYMBOL_TYPE_MACRO_WITH_ARG,
											context, FALSE, TRUE, NULL);
		if (iter)
		{
			do
			{
				IAnjutaSymbol* symbol = IANJUTA_SYMBOL(iter);
				const gchar* name = ianjuta_symbol_name(symbol, NULL);
				if (name != NULL)
				{
					const gchar* args = ianjuta_symbol_args(symbol, NULL);
					gchar* print_args;
					gchar* separator;
					gchar* white_name = g_strnfill (strlen(name) + 1, ' ');
					
					separator = g_strjoin (NULL, ", \n", white_name, NULL);
					DEBUG_PRINT ("Separator: \n%s", separator);
					
					gchar** argv;
					if (!args)
						args = "()";
					
					argv = g_strsplit(args, ",", -1);
					print_args = g_strjoinv(separator, argv);
					
					gchar* tip = g_strdup_printf ("%s %s", name, print_args);
					
					if (!g_list_find_custom (assist_ctx->tips, tip,
											 (GCompareFunc) strcmp))
						assist_ctx->tips = g_list_append (assist_ctx->tips,
														  tip);
					g_strfreev(argv);
					g_free(print_args);
					g_free(separator);
					g_free(white_name);
				}
				else
					break;
			}
			while (ianjuta_iterable_next (iter, NULL));
			
			if (assist_ctx->tips)
			{
				assist_ctx->position =
					ianjuta_editor_get_position (IANJUTA_EDITOR(iassist),
												 NULL) - 1;
				ianjuta_editor_assist_show_tips (iassist, assist_ctx->tips,
												 assist_ctx->position, NULL);
			}
			return;
		}
	}
}

static void
assist_context_free (CppJavaAssistContext *assist_ctx)
{
	if (assist_ctx->completion)
	{
		GList* items = assist_ctx->completion->items;
		if (items)
		{
			g_list_foreach (items, (GFunc) g_free, NULL);
			g_completion_clear_items (assist_ctx->completion);
		}
		g_completion_free (assist_ctx->completion);
	}
	if (assist_ctx->tips)
	{
		g_list_foreach (assist_ctx->tips, (GFunc) g_free, NULL);
		g_list_free (assist_ctx->tips);
	}
	g_free (assist_ctx);
}

static void
assist_context_cleanup (CppJavaAssist* assist,
						CppJavaAssistContext *assist_ctx)
{
	g_queue_remove (assist->priv->assist_queue, assist_ctx);
	assist_context_free (assist_ctx);
}

static void
assist_cleanup (CppJavaAssist* assist)
{
	g_queue_foreach (assist->priv->assist_queue,
					 (GFunc) assist_context_free, NULL);
	g_queue_free (assist->priv->assist_queue);
	assist->priv->assist_queue = g_queue_new ();
}

static void
on_assist_end (IAnjutaEditorAssist* iassist, CppJavaAssist * assist)
{
	CppJavaAssistContext *assist_ctx = 
		(CppJavaAssistContext*) g_queue_peek_head (assist->priv->assist_queue);
	
	DEBUG_PRINT ("assist-end");
	g_return_if_fail (assist_ctx != NULL);
	
	assist_context_cleanup (assist, assist_ctx);
}

static void
on_assist_cancel (IAnjutaEditorAssist* iassist, CppJavaAssist* assist)
{
	DEBUG_PRINT ("assist-cancel");
	assist_cleanup (assist);
}

static void
on_assist_chosen (IAnjutaEditorAssist* iassist, gint selection,
				  CppJavaAssist* assist)
{
	gchar* assistance;
	CppJavaAssistContext *assist_ctx = 
		(CppJavaAssistContext*) g_queue_peek_head (assist->priv->assist_queue);
	
	DEBUG_PRINT ("assist-chosen: %d", selection);
	g_return_if_fail (assist_ctx != NULL);
	
	if (assist_ctx->completion->cache)
	{
		GList* cache = g_list_copy (assist_ctx->completion->cache);
		cache = g_list_sort (cache, (GCompareFunc)strcmp);
		assistance = g_strdup (g_list_nth_data (cache, selection));
		g_list_free (cache);
	}
	else
		assistance = g_strdup (g_list_nth_data (assist_ctx->completion->items,
												selection));
	assist_context_cleanup (assist, assist_ctx);
	ianjuta_editor_assist_react (iassist, selection, assistance, NULL);
	g_free (assistance);
}

static void
on_assist_update (IAnjutaEditorAssist* iassist, gchar* context,
				  CppJavaAssist* assist)
{
	CppJavaAssistContext *assist_ctx = 
		(CppJavaAssistContext*) g_queue_peek_head (assist->priv->assist_queue);
	
	DEBUG_PRINT("assist-update: %s", context);
	
	g_return_if_fail (assist_ctx != NULL);
	
	if (assist_ctx->tips)
	{
		IAnjutaEditor* editor = IANJUTA_EDITOR (iassist);
		IAnjutaIterable* begin = 
			ianjuta_editor_get_cell_iter (editor, assist_ctx->position, NULL);
		IAnjutaIterable* end = 
			ianjuta_editor_get_cell_iter (editor, assist_ctx->position +
										  strlen(context) - 1, NULL);
		char begin_brace = 
			ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (begin), 0, NULL);
		char end_brace =
			ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (end), 0, NULL);
		IAnjutaEditorAttribute attrib =
			ianjuta_editor_cell_get_attribute (IANJUTA_EDITOR_CELL (end), NULL);
		
		DEBUG_PRINT("begin brace: %c", begin_brace);
		DEBUG_PRINT("end brace: %c", end_brace);
		
		if (!strlen (context) || begin_brace != '(')
		{
			ianjuta_editor_assist_cancel_tips (iassist, NULL);
		}
		
		/* Detect if function argument list is closed */
		if (end_brace == ')' &&
			(attrib != IANJUTA_EDITOR_COMMENT &&
			 attrib != IANJUTA_EDITOR_STRING))
		{
			if (cpp_java_util_jump_to_matching_brace (end, end_brace))
			{
				if (ianjuta_iterable_get_position (IANJUTA_ITERABLE (begin), NULL) ==
					ianjuta_iterable_get_position (IANJUTA_ITERABLE (end), NULL))
				{
					ianjuta_editor_assist_cancel_tips (iassist, NULL);
				}
			}
		}
		return;
	}
	
	if (!strlen(context))
	{
		ianjuta_editor_assist_suggest (iassist, NULL, -1, 0, NULL);
		return;
	}
	else
	{
		GList* new_suggestions =
			g_completion_complete (assist_ctx->completion, context, NULL);
		
		gint max_completions =
			anjuta_preferences_get_int_with_default (assist->priv->preferences,
													 PREF_AUTOCOMPLETE_CHOICES,
													 MAX_COMPLETIONS);
		
		if (g_list_length (new_suggestions) < max_completions)
			ianjuta_editor_assist_suggest (iassist, new_suggestions,
										   assist_ctx->position,
										   strlen (context), NULL);
		else
			ianjuta_editor_assist_hide_suggestions (iassist, NULL);
	}
}

static
gboolean context_character (gchar ch)
{	
	if (g_ascii_isspace (ch))
		return FALSE;
	if (g_ascii_isalnum (ch))
		return TRUE;
	if (ch == '_' || ch == '.' || ch == ':' || ch == '>' || ch == '-')
		return TRUE;
	
	return FALSE;
}	

static gchar*
get_context(IAnjutaEditorCell* iter, IAnjutaEditor* editor)
{
	gint end;
	gint begin;
	gchar ch;
	end = ianjuta_iterable_get_position(IANJUTA_ITERABLE(iter), NULL);
	do
	{
		ianjuta_iterable_previous(IANJUTA_ITERABLE(iter), NULL);		
		ch = ianjuta_editor_cell_get_char(iter, 0, NULL);		
	}
	while (ch && context_character(ch));
	begin = ianjuta_iterable_get_position(IANJUTA_ITERABLE(iter), NULL) + 1;
	return ianjuta_editor_get_text(editor, begin, end - begin, NULL);
}

static gchar*
dot_member_parser(IAnjutaEditor* editor, gint position)
{
	IAnjutaEditorCell* iter = IANJUTA_EDITOR_CELL(ianjuta_editor_get_cell_iter(editor, position, NULL));
	if (ianjuta_iterable_previous(IANJUTA_ITERABLE(iter), NULL) &&
		ianjuta_editor_cell_get_char(iter, 0, NULL) == '.')
	{
		return get_context(iter, editor);
	}
	else
	{
		return NULL;
	}
}

static gchar*
pointer_member_parser(IAnjutaEditor* editor, gint position)
{
	IAnjutaEditorCell* iter = IANJUTA_EDITOR_CELL(ianjuta_editor_get_cell_iter(editor, position, NULL));
	if (ianjuta_iterable_previous(IANJUTA_ITERABLE(iter), NULL) &&
		ianjuta_editor_cell_get_char(iter, 0, NULL) == '>' &&
		ianjuta_iterable_previous(IANJUTA_ITERABLE(iter), NULL) &&
		ianjuta_editor_cell_get_char(iter, 0, NULL) == '-')
	{
		return get_context(iter, editor);
	}
	else
	{
		return NULL;
	}
}

static gchar*
function_parser(IAnjutaEditor* editor, gint position)
{
	IAnjutaEditorCell* iter = IANJUTA_EDITOR_CELL(ianjuta_editor_get_cell_iter(editor, position, NULL));
	IAnjutaEditorAttribute attrib = ianjuta_editor_cell_get_attribute(iter, NULL);
	if (attrib == IANJUTA_EDITOR_COMMENT || attrib == IANJUTA_EDITOR_STRING)
		return NULL;
	
	if (ianjuta_iterable_previous(IANJUTA_ITERABLE(iter), NULL) &&
		ianjuta_editor_cell_get_char(iter, 0, NULL) == '(')
	{
		while (ianjuta_iterable_previous(IANJUTA_ITERABLE(iter), NULL)
			&& g_ascii_isspace (ianjuta_editor_cell_get_char (iter, 0, NULL)))
				;
		ianjuta_iterable_next (IANJUTA_ITERABLE(iter), NULL);
		return get_context(iter, editor);
	}
	else
	{
		return NULL;
	}
}

static gchar*
cpp_member_parser (IAnjutaEditor* editor, gint position)
{
	IAnjutaEditorCell* iter = IANJUTA_EDITOR_CELL(ianjuta_editor_get_cell_iter(editor, position, NULL));
	if (ianjuta_iterable_previous(IANJUTA_ITERABLE(iter), NULL) &&
		ianjuta_editor_cell_get_char(iter, 0, NULL) == ':' &&
		ianjuta_iterable_previous(IANJUTA_ITERABLE(iter), NULL) &&
		ianjuta_editor_cell_get_char(iter, 0, NULL) == ':')
	{
		return get_context(iter, editor);
	}
	else
	{
		return NULL;
	}
}

static void
cpp_java_assist_install (CppJavaAssist *assist, IAnjutaEditorAssist *iassist)
{
	DEBUG_PRINT(__FUNCTION__);
	
	assist->priv->iassist = iassist;
	
	ianjuta_editor_assist_add_trigger (iassist, ".", dot_member_parser, NULL);
	ianjuta_editor_assist_add_trigger (iassist, "->", pointer_member_parser, NULL);
	ianjuta_editor_assist_add_trigger (iassist, "::", cpp_member_parser, NULL);	
	ianjuta_editor_assist_add_trigger (iassist, "(", function_parser, NULL);
	g_signal_connect (G_OBJECT(iassist), "assist_begin",
					  G_CALLBACK(on_assist_begin), assist);
	g_signal_connect (G_OBJECT(iassist), "assist_end",
					  G_CALLBACK(on_assist_end), assist);
	g_signal_connect (G_OBJECT(iassist), "assist_update",
					  G_CALLBACK(on_assist_update), assist);
	g_signal_connect (G_OBJECT(iassist), "assist_chosen",
					  G_CALLBACK(on_assist_chosen), assist);
	g_signal_connect (G_OBJECT(iassist), "assist_canceled",
					  G_CALLBACK(on_assist_cancel), assist);
}

static void
cpp_java_assist_uninstall (CppJavaAssist *assist)
{
	/* If there is any assist context alive, make sure to notify everyone
	 * that assist is canceled.
	 */
	if (g_list_length (assist->priv->assist_queue->head) > 0)
		g_signal_emit_by_name (assist->priv->iassist, "assist-canceled");
	
	ianjuta_editor_assist_remove_trigger (assist->priv->iassist, ".", NULL);
	ianjuta_editor_assist_remove_trigger (assist->priv->iassist, "->", NULL);
	ianjuta_editor_assist_remove_trigger (assist->priv->iassist, "::", NULL);
	g_signal_handlers_disconnect_by_func (assist->priv->iassist,
										  G_CALLBACK(on_assist_begin), assist);
	g_signal_handlers_disconnect_by_func (assist->priv->iassist,
										  G_CALLBACK(on_assist_end), assist);
	g_signal_handlers_disconnect_by_func (assist->priv->iassist,
										  G_CALLBACK(on_assist_update), assist);
	g_signal_handlers_disconnect_by_func (assist->priv->iassist,
										  G_CALLBACK(on_assist_cancel), assist);
	g_signal_handlers_disconnect_by_func (assist->priv->iassist,
										  G_CALLBACK(on_assist_chosen), assist);
	assist->priv->iassist = NULL;
}

static void
cpp_java_assist_init (CppJavaAssist *assist)
{
	assist->priv = g_new0 (CppJavaAssistPriv, 1);
	assist->priv->assist_queue = g_queue_new ();
}

static void
cpp_java_assist_finalize (GObject *object)
{
	CppJavaAssist *assist = CPP_JAVA_ASSIST (object);
	cpp_java_assist_uninstall (assist);
	assist_cleanup (assist);
	g_free (assist->priv);
	G_OBJECT_CLASS (cpp_java_assist_parent_class)->finalize (object);
}

static void
cpp_java_assist_class_init (CppJavaAssistClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = cpp_java_assist_finalize;
}

CppJavaAssist *
cpp_java_assist_new (IAnjutaEditorAssist *iassist,
					 IAnjutaSymbolManager *isymbol_manager,
					 AnjutaPreferences *prefs)
{
	CppJavaAssist *assist = g_object_new (TYPE_CPP_JAVA_ASSIST, NULL);
	assist->priv->isymbol_manager = isymbol_manager;
	assist->priv->preferences = prefs;
	cpp_java_assist_install (assist, iassist);
	return assist;
}
