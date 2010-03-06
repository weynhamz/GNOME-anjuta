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

#include "cxxparser/engine-parser.h"

#include <ctype.h>
#include <string.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-editor-cell.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-editor-assist.h>
#include <libanjuta/interfaces/ianjuta-editor-tip.h>
#include <libanjuta/interfaces/ianjuta-provider.h>
#include <libanjuta/interfaces/ianjuta-document.h>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>
#include "cpp-java-assist.h"
#include "cpp-java-utils.h"

#define PREF_AUTOCOMPLETE_ENABLE "language.cpp.code.completion.enable"
#define PREF_AUTOCOMPLETE_SPACE_AFTER_FUNC "language.cpp.code.completion.space.after.func"
#define PREF_AUTOCOMPLETE_BRACE_AFTER_FUNC "language.cpp.code.completion.brace.after.func"
#define PREF_CALLTIP_ENABLE "language.cpp.code.calltip.enable"
#define BRACE_SEARCH_LIMIT 500

static void cpp_java_assist_iface_init(IAnjutaProviderIface* iface);

G_DEFINE_TYPE_WITH_CODE (CppJavaAssist,
			 cpp_java_assist,
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (IANJUTA_TYPE_PROVIDER,
			                        cpp_java_assist_iface_init))

typedef struct
{
	gchar *name;
	gboolean is_func;
	GdkPixbuf* icon;
	IAnjutaSymbolType type;
} CppJavaAssistTag;

struct _CppJavaAssistPriv {
	AnjutaPreferences *preferences;
	IAnjutaSymbolManager* isymbol_manager;
	IAnjutaEditorAssist* iassist;
	IAnjutaEditorTip* itip;

	GCompletion *completion_cache;

	/* Calltips */
	gboolean calltip_active;
	gchar* calltip_context;
	GList* tips;
	IAnjutaIterable* calltip_iter;

	gboolean async_calltip_file;
	gboolean async_calltip_system;
	gboolean async_calltip_project;

	GCancellable* cancel_calltip_system;
	GCancellable* cancel_calltip_file;
	GCancellable* cancel_calltip_project;

	/* Autocompletion */
	gboolean member_completion;
	gboolean autocompletion;
	IAnjutaIterable* start_iter;
	gchar* pre_word;

	gboolean async_file;
	gboolean async_system;
	gboolean async_project;

	GCancellable* cancel_system;
	GCancellable* cancel_file;
	GCancellable* cancel_project;	
};

typedef struct
{
	gboolean is_func;
	gchar* name;
} ProposalData;

/**
 * cpp_java_assist_proposal_new:
 * @symbol: IAnjutaSymbol to create the proposal for
 *
 * Creates a new IAnjutaEditorAssistProposal for symbol
 *
 * Returns: a newly allocated IAnjutaEditorAssistProposal
 */
static IAnjutaEditorAssistProposal*
cpp_java_assist_proposal_new (IAnjutaSymbol* symbol)
{
	IAnjutaEditorAssistProposal* proposal = g_new0 (IAnjutaEditorAssistProposal, 1);
	IAnjutaSymbolType type = ianjuta_symbol_get_sym_type (symbol, NULL);
	ProposalData* data = g_new0 (ProposalData, 1);

	data->name = g_strdup (ianjuta_symbol_get_name (symbol, NULL));
	switch (type)
	{
		case IANJUTA_SYMBOL_TYPE_PROTOTYPE:
		case IANJUTA_SYMBOL_TYPE_FUNCTION:
		case IANJUTA_SYMBOL_TYPE_METHOD:
		case IANJUTA_SYMBOL_TYPE_MACRO_WITH_ARG:
			proposal->label = g_strdup_printf ("%s()", ianjuta_symbol_get_name (symbol, NULL));
			data->is_func = TRUE;
			break;
		default:
			proposal->label = g_strdup (ianjuta_symbol_get_name (symbol, NULL));
			data->is_func = FALSE;
	}
	proposal->data = data;
	/* Icons are lifetime object of the symbol-db so we can cast here */
	proposal->icon = (GdkPixbuf*) ianjuta_symbol_get_icon (symbol, NULL);
	return proposal;
}

/**
 * cpp_java_assist_proposal_free:
 * @proposal: the proposal to free
 * 
 * Frees the proposal
 */
static void
cpp_java_assist_proposal_free (IAnjutaEditorAssistProposal* proposal)
{
	ProposalData* data = proposal->data;
	g_free (data->name);
	g_free (data);
	g_free (proposal->label);
	g_free (proposal);
}

/**
 * anjuta_propsal_completion_func:
 * @data: an IAnjutaEditorAssistProposal
 *
 * Returns: the name of the completion func
 */
static gchar*
anjuta_proposal_completion_func (gpointer data)
{
	IAnjutaEditorAssistProposal* proposal = data;
	ProposalData* prop_data = proposal->data;
	
	return prop_data->name;
}

/**
 * cpp_java_assist_create_completion_from_symbols:
 * @symbols: Symbol iteration
 * 
 * Create a list of IAnjutaEditorAssistProposals from a list of symbols
 *
 * Returns: a newly allocated GList of newly allocated proposals. Free 
 * with cpp_java_assist_proposal_free()
 */
static GList*
cpp_java_assist_create_completion_from_symbols (IAnjutaIterable* symbols)
{
	GList* list = NULL;
	do
	{
		IAnjutaSymbol* symbol = IANJUTA_SYMBOL (symbols);
		IAnjutaEditorAssistProposal* proposal = cpp_java_assist_proposal_new (symbol);	

		list = g_list_append (list, proposal);
	}
	while (ianjuta_iterable_next (symbols, NULL));

	return list;
}

/**
 * cpp_java_assist_is_word_character:
 * @ch: character to check
 *
 * Returns: TRUE if ch is a valid word character, FALSE otherwise
 */
 
static gboolean
cpp_java_assist_is_word_character (gchar ch)
{
	if (g_ascii_isspace (ch))
		return FALSE;
	if (g_ascii_isalnum (ch))
		return TRUE;
	if (ch == '_')
		return TRUE;
	
	return FALSE;
}	

/**
 * cpp_java_assist_get_pre_word:
 * @editor: Editor object
 * @iter: current cursor position
 * @start_iter: return location for the start_iter (if a preword was found)
 *
 * Search for the current typed word
 *
 * Returns: The current word (needs to be freed) or NULL if no word was found
 */
static gchar*
cpp_java_assist_get_pre_word (IAnjutaEditor* editor, IAnjutaIterable *iter, IAnjutaIterable** start_iter)
{
	IAnjutaIterable *end = ianjuta_iterable_clone (iter, NULL);
	IAnjutaIterable *begin = ianjuta_iterable_clone (iter, NULL);
	gchar ch, *preword_chars = NULL;
	gboolean out_of_range = FALSE;
	gboolean preword_found = FALSE;
	
	/* Cursor points after the current characters, move back */
	ianjuta_iterable_previous (begin, NULL);
	
	ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (begin), 0, NULL);
	
	while (ch && cpp_java_assist_is_word_character (ch))
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
 * cpp_java_assist_update_pre_word:
 * @assist: self
 * @pre_word: new pre_word
 *
 * Updates the current pre_word
 */
static void
cpp_java_assist_update_pre_word (CppJavaAssist* assist, const gchar* pre_word)
{
	g_free (assist->priv->pre_word);
	if (pre_word == NULL) pre_word = "";
	assist->priv->pre_word = g_strdup (pre_word);
}

/**
 * cpp_java_assist_is_expression_separator:
 * @c: character to check
 * @skip_braces: whether to skip closing braces
 * @iter: current cursor position
 *
 * Checks if a character seperates a C/C++ expression. It can skip brances
 * because they might not really end the expression
 *
 * Returns: TRUE if the characters seperates an expression, FALSE otherwise
 */
static gboolean
cpp_java_assist_is_expression_separator (gchar c, gboolean skip_braces, IAnjutaIterable* iter)
{
	IAnjutaEditorAttribute attrib = ianjuta_editor_cell_get_attribute (IANJUTA_EDITOR_CELL(iter),
	                                                                   NULL);
	if (attrib == IANJUTA_EDITOR_STRING ||
	    attrib == IANJUTA_EDITOR_COMMENT)
	{
		return FALSE;
	}
	
	if (c == ')' && skip_braces)
	{
		cpp_java_util_jump_to_matching_brace (iter, c, BRACE_SEARCH_LIMIT);
		return TRUE;
	}
	else if (c == ')' && !skip_braces)
		return FALSE;
	
	if (c == ',' || c == ';' || c == '\n' || c == '\r' || c == '\t' || c == '(' ||
	    c == '{' || c == '}' || c == '=' || c == '<' || c == '\v' || c == '!')
	{
		return TRUE;
	}	

	return FALSE;
}

/**
 * cpp_java_assist_parse_expression:
 * @assist: self,
 * @iter: current cursor position
 * @start_iter: return location for the start of the completion
 * 
 * Returns: An iter of a list of IAnjutaSymbols or NULL
 */
static IAnjutaIterable*
cpp_java_assist_parse_expression (CppJavaAssist* assist, IAnjutaIterable* iter, IAnjutaIterable** start_iter)
{
	IAnjutaEditor* editor = IANJUTA_EDITOR (assist->priv->iassist);
	IAnjutaIterable* res = NULL;
	IAnjutaIterable* cur_pos = ianjuta_iterable_clone (iter, NULL);
	gboolean op_start = FALSE;
	gboolean ref_start = FALSE;
	gchar* stmt = NULL;

	/* Cursor points after the current characters, move back */
	ianjuta_iterable_previous (cur_pos, NULL);
	
	/* Search for a operator in the current line */
	do 
	{
		gchar ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL(cur_pos), 0, NULL);
		
		if (cpp_java_assist_is_expression_separator(ch, FALSE, iter)) {
			break;
		}

		if (ch == '.' || (op_start && ch == '-') || (ref_start && ch == ':'))
		{
			/* Found an operator, get the statement and the pre_word */
			IAnjutaIterable* pre_word_start = ianjuta_iterable_clone (cur_pos, NULL);
			IAnjutaIterable* pre_word_end = ianjuta_iterable_clone (iter, NULL);
			IAnjutaIterable* stmt_end = ianjuta_iterable_clone (pre_word_start, NULL);

			
			/* we need to pass to the parser all the statement included the last operator,
			 * being it "." or "->" or "::"
			 * Increase the end bound of the statement.
			 */
			ianjuta_iterable_next (stmt_end, NULL);
			if (op_start == TRUE || ref_start == TRUE)
				ianjuta_iterable_next (stmt_end, NULL);
				
			
			/* Move one character forward so we have the start of the pre_word and
			 * not the last operator */
			ianjuta_iterable_next (pre_word_start, NULL);
			/* If this is a two character operator, skip the second character */
			if (op_start)
			{
				ianjuta_iterable_next (pre_word_start, NULL);
			}
			
			cpp_java_assist_update_pre_word (assist, 
			                                 ianjuta_editor_get_text (editor,
			                                                          pre_word_start, pre_word_end, NULL));

			/* Try to get the name of the variable */
			while (ianjuta_iterable_previous (cur_pos, NULL))
			{
				gchar word_ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL(cur_pos), 0, NULL);
				
				if (cpp_java_assist_is_expression_separator(word_ch, FALSE, cur_pos)) 
					break;				
			}
			ianjuta_iterable_next (cur_pos, NULL);
			stmt = ianjuta_editor_get_text (editor,
			                                cur_pos, stmt_end, NULL);
			*start_iter = pre_word_start;
			g_object_unref (stmt_end);
			g_object_unref (pre_word_end);
			break;
		}
		else if (ch == '>')
			op_start = TRUE;
		else if (ch == ':')
			ref_start = TRUE;
		else
		{
			op_start = FALSE;
			ref_start = FALSE;
		}
	}
	while (ianjuta_iterable_previous (cur_pos, NULL));

	if (stmt)
	{
		gint lineno;
		gchar *filename = NULL;
		gchar *above_text;
		IAnjutaIterable* start;

		if (IANJUTA_IS_FILE (assist->priv->iassist))
		{
			GFile *file = ianjuta_file_get_file (IANJUTA_FILE (assist->priv->iassist), NULL);
			if (file != NULL)
			{
				filename = g_file_get_path (file);
				g_object_unref (file);
			}
			else
			{
				g_free (stmt);
				return NULL;
			}
		}
		start = ianjuta_editor_get_start_position (editor, NULL);
		above_text = ianjuta_editor_get_text (editor, start, iter, NULL);
		g_object_unref (start);
		
		lineno = ianjuta_editor_get_lineno (editor, NULL);

		/* the parser works even for the "Gtk::" like expressions, so it shouldn't be 
		 * created a specific case to handle this.
		 */
		res = engine_parser_process_expression (stmt,
		                                        above_text,
		                                        filename,
		                                        lineno);
		g_free (filename);
		g_free (stmt);
	}
	g_object_unref (cur_pos);
	return res;
}

/** 
 * cpp_java_assist_create_completion_cache:
 * @assist: self
 *
 * Create a new completion_cache object
 */
static void
cpp_java_assist_create_completion_cache (CppJavaAssist* assist)
{
	g_assert (assist->priv->completion_cache == NULL);
	assist->priv->completion_cache = 
		g_completion_new (anjuta_proposal_completion_func);
}

/**
 * cpp_java_assist_clear_completion_cache
 * @assist: self
 *
 * Clear the completion cache, aborting all async operations
 */
static void
cpp_java_assist_clear_completion_cache (CppJavaAssist* assist)
{
	g_cancellable_cancel (assist->priv->cancel_file);
	g_cancellable_cancel (assist->priv->cancel_project);	
	g_cancellable_cancel (assist->priv->cancel_system);
	if (assist->priv->completion_cache)
	{	
		g_list_foreach (assist->priv->completion_cache->items, (GFunc) cpp_java_assist_proposal_free, NULL);
		g_completion_free (assist->priv->completion_cache);
	}
	assist->priv->completion_cache = NULL;
}

/**
 * cpp_java_assist_populate_real:
 * @assist: self
 * @finished: TRUE if no more proposals are expected, FALSE otherwise
 *
 * Really invokes the completion interfaces and adds completions. Might be called
 * from an async context
 */
static void
cpp_java_assist_populate_real (CppJavaAssist* assist, gboolean finished)
{
	g_assert (assist->priv->pre_word != NULL);
	gchar* prefix;
	GList* proposals = g_completion_complete (assist->priv->completion_cache,
	                                          assist->priv->pre_word,
	                                          &prefix);
	if (g_list_length (proposals) == 1)
	{
		IAnjutaEditorAssistProposal* proposal = proposals->data;
		ProposalData* data = proposal->data;
		if (g_str_equal (assist->priv->pre_word, data->name))
		{
			ianjuta_editor_assist_proposals (assist->priv->iassist,
			                                 IANJUTA_PROVIDER(assist),
			                                 NULL, finished, NULL);
			return;
		}
	}

	ianjuta_editor_assist_proposals (assist->priv->iassist,
	                                 IANJUTA_PROVIDER(assist),
	                                 proposals, finished, NULL);
}

/**
 * cpp_java_assist_create_member_completion_cache
 * @assist: self
 * @cursor: Current cursor position
 * 
 * Create the completion_cache for member completion if possible
 *
 * Returns: TRUE if a completion cache was build, FALSE otherwise
 */
static gboolean
cpp_java_assist_create_member_completion_cache (CppJavaAssist* assist, IAnjutaIterable* cursor)
{
	IAnjutaIterable* symbol = NULL;
	IAnjutaIterable* start_iter = NULL;
	symbol = cpp_java_assist_parse_expression (assist, cursor, &start_iter);

	if (symbol)
	{
		gint retval = FALSE;
		/* Query symbol children */
		IAnjutaIterable *children = 
			ianjuta_symbol_manager_get_members (assist->priv->isymbol_manager,
			                                    IANJUTA_SYMBOL(symbol),
			                                    IANJUTA_SYMBOL_FIELD_SIMPLE |
			                                    IANJUTA_SYMBOL_FIELD_KIND |
			                                    IANJUTA_SYMBOL_FIELD_ACCESS |
			                                    IANJUTA_SYMBOL_FIELD_TYPE,
			                                    NULL);
		if (children)
		{
			GList* proposals = 
				cpp_java_assist_create_completion_from_symbols (children);
			cpp_java_assist_create_completion_cache (assist);
			g_completion_add_items (assist->priv->completion_cache, proposals);

			assist->priv->start_iter = start_iter;

			cpp_java_assist_populate_real (assist, TRUE);
			g_list_free (proposals);
			g_object_unref (children);
			retval = TRUE;
		}
		g_object_unref (symbol);
		return retval;
	}
	else if (start_iter)
		g_object_unref (start_iter);
	return FALSE;
}

/**
 * on_symbol_search_complete:
 * @search_id: id of this search
 * @symbols: the returned symbols
 * @assist: self
 *
 * Called by the async search method when it found symbols
 */
static void
on_symbol_search_complete (gint search_id, IAnjutaIterable* symbols, CppJavaAssist* assist)
{
	GList* proposals = cpp_java_assist_create_completion_from_symbols (symbols);
	g_completion_add_items (assist->priv->completion_cache, proposals);
	gboolean running = assist->priv->async_system || assist->priv->async_file ||
		assist->priv->async_project;
	cpp_java_assist_populate_real (assist, !running);
	g_list_free (proposals);
	g_object_unref (symbols);
}

/**
 * notify_system_finished:
 * @notify: the notifycation object
 * @assist: self
 *
 * Called when the system search was finished
 */
static void
notify_system_finished (AnjutaAsyncNotify* notify, CppJavaAssist* assist)
{
	assist->priv->async_system = FALSE;
}

/**
 * notify_file_finished:
 * @notify: the notifycation object
 * @assist: self
 *
 * Called when the file search was finished
 */
static void
notify_file_finished (AnjutaAsyncNotify* notify, CppJavaAssist* assist)
{
	assist->priv->async_file = FALSE;
}

/**
 * notify_project_finished:
 * @notify: the notifycation object
 * @assist: self
 *
 * Called when the file search was finished
 */
static void
notify_project_finished (AnjutaAsyncNotify* notify, CppJavaAssist* assist)
{
	assist->priv->async_project = FALSE;
}


/**
 * cpp_java_assist_create_autocompletion_cache:
 * @assist: self
 * @cursor: Current cursor position
 * 
 * Create completion cache for autocompletion. This is done async.
 *
 * Returns: TRUE if a preword was detected, FALSE otherwise
 */ 
static gboolean
cpp_java_assist_create_autocompletion_cache (CppJavaAssist* assist, IAnjutaIterable* cursor)
{
	IAnjutaIterable* start_iter;
	gchar* pre_word = 
		cpp_java_assist_get_pre_word (IANJUTA_EDITOR (assist->priv->iassist), cursor, &start_iter);
	if (!pre_word || strlen (pre_word) <= 3)
	{
		if (start_iter)
			g_object_unref (start_iter);
		return FALSE;
	}
	else
	{
		gchar *pattern = g_strconcat (pre_word, "%", NULL);
		AnjutaAsyncNotify* notify = NULL;
		IAnjutaSymbolType match_types = IANJUTA_SYMBOL_TYPE_MAX;
		IAnjutaSymbolField fields = IANJUTA_SYMBOL_FIELD_SIMPLE |
			IANJUTA_SYMBOL_FIELD_TYPE |
			IANJUTA_SYMBOL_FIELD_ACCESS |
			IANJUTA_SYMBOL_FIELD_KIND;
		
		cpp_java_assist_create_completion_cache (assist);
		cpp_java_assist_update_pre_word (assist, pre_word);

		if (IANJUTA_IS_FILE (assist->priv->iassist))
		{
			GFile *file = ianjuta_file_get_file (IANJUTA_FILE (assist->priv->iassist), NULL);
			if (file != NULL)
			{
				notify = anjuta_async_notify_new();
				g_signal_connect (notify, "finished", G_CALLBACK(notify_file_finished), assist);
				assist->priv->async_file = TRUE;
				g_cancellable_reset (assist->priv->cancel_file);
				ianjuta_symbol_manager_search_file_async (assist->priv->isymbol_manager,
				                                          match_types,
				                                          TRUE,
				                                          fields,
				                                          pattern, file, -1, -1, 
				                                          assist->priv->cancel_file,
				                                          notify, 
				                                          (IAnjutaSymbolManagerSearchCallback) on_symbol_search_complete,
				                                          assist,
				                                          NULL);
				g_object_unref (file);
			}
		}
		/* This will avoid duplicates of FUNCTION and PROTOTYPE */
		match_types &= ~IANJUTA_SYMBOL_TYPE_FUNCTION;
		notify = anjuta_async_notify_new();
		g_signal_connect (notify, "finished", G_CALLBACK(notify_project_finished), assist);
		assist->priv->async_project = TRUE;
		g_cancellable_reset (assist->priv->cancel_project);
		ianjuta_symbol_manager_search_project_async (assist->priv->isymbol_manager,
		                                             match_types,
		                                             TRUE,
		                                             fields,
		                                             pattern, 
		                                             IANJUTA_SYMBOL_MANAGER_SEARCH_FS_PUBLIC, -1, -1, 
		                                             assist->priv->cancel_project,
		                                             notify, 
		                                             (IAnjutaSymbolManagerSearchCallback) on_symbol_search_complete, 
		                                             assist,
		                                             NULL);


		notify = anjuta_async_notify_new();
		g_signal_connect (notify, "finished", G_CALLBACK(notify_system_finished), assist);
		assist->priv->async_system = TRUE;
		g_cancellable_reset (assist->priv->cancel_system);
		ianjuta_symbol_manager_search_system_async (assist->priv->isymbol_manager,
		                                            match_types,
		                                            TRUE,
		                                            fields,
		                                            pattern, IANJUTA_SYMBOL_MANAGER_SEARCH_FS_PUBLIC, -1, -1,
		                                            assist->priv->cancel_system,
		                                            notify, 
		                                            (IAnjutaSymbolManagerSearchCallback) on_symbol_search_complete, 
		                                            assist,
		                                            NULL);
		g_free (pre_word);
		g_free (pattern);

		assist->priv->start_iter = start_iter;
		
		return TRUE;
	}
}


/**
 * cpp_java_assist_create_calltips:
 * @iter: List of symbols
 * @merge: list of calltips to merge or NULL
 *
 * Create a list of Calltips (string) from a list of symbols
 *
 * A newly allocated GList* with newly allocated strings
 */
static GList*
cpp_java_assist_create_calltips (IAnjutaIterable* iter, GList* merge)
{
	GList* tips = merge;
	if (iter)
	{
		do
		{
			IAnjutaSymbol* symbol = IANJUTA_SYMBOL(iter);
			const gchar* name = ianjuta_symbol_get_name(symbol, NULL);
			if (name != NULL)
			{
				const gchar* args = ianjuta_symbol_get_args(symbol, NULL);
				const gchar* rettype = ianjuta_symbol_get_returntype (symbol, NULL);
				gchar* print_args;
				gchar* separator;
				gchar* white_name;
				gint white_count = 0;

				if (!rettype)
					rettype = "";
				else
					white_count += strlen(rettype) + 1;
				
				white_count += strlen(name) + 1;
				
				white_name = g_strnfill (white_count, ' ');
				separator = g_strjoin (NULL, ", \n", white_name, NULL);
				
				gchar** argv;
				if (!args)
					args = "()";
				
				argv = g_strsplit (args, ",", -1);
				print_args = g_strjoinv (separator, argv);
				
				gchar* tip = g_strdup_printf ("%s %s %s", rettype, name, print_args);
				
				if (!g_list_find_custom (tips, tip, (GCompareFunc) strcmp))
					tips = g_list_append (tips, tip);
				
				g_strfreev (argv);
				g_free (print_args);
				g_free (separator);
				g_free (white_name);
			}
			else
				break;
		}
		while (ianjuta_iterable_next (iter, NULL));
	}
	return tips;
}

/**
 * on_calltip_search_complete:
 * @search_id: id of this search
 * @symbols: the returned symbols
 * @assist: self
 *
 * Called by the async search method when it found calltips
 */
static void
on_calltip_search_complete (gint search_id, IAnjutaIterable* symbols, CppJavaAssist* assist)
{
	assist->priv->tips = cpp_java_assist_create_calltips (symbols, assist->priv->tips);
	gboolean running = !(assist->priv->async_calltip_system || assist->priv->async_calltip_file ||
		assist->priv->async_calltip_project);

	DEBUG_PRINT ("Calltip search finished with %d items", g_list_length (assist->priv->tips));
	
	if (!running && assist->priv->tips)
	{
		ianjuta_editor_tip_show (IANJUTA_EDITOR_TIP(assist->priv->itip), assist->priv->tips,
		                         assist->priv->calltip_iter, 0,
		                         NULL);
	}
	g_object_unref (symbols);
}

/**
 * notify_system_calltips_finished:
 * @notify: the notifycation object
 * @assist: self
 *
 * Called when the system search was finished
 */
static void
notify_system_calltips_finished (AnjutaAsyncNotify* notify, CppJavaAssist* assist)
{
	assist->priv->async_calltip_system = FALSE;
}

/**
 * notify_file_calltips_finished:
 * @notify: the notifycation object
 * @assist: self
 *
 * Called when the file search was finished
 */
static void
notify_file_calltips_finished (AnjutaAsyncNotify* notify, CppJavaAssist* assist)
{
	assist->priv->async_calltip_file = FALSE;
}

/**
 * notify_project_calltips_finished:
 * @notify: the notifycation object
 * @assist: self
 *
 * Called when the file search was finished
 */
static void
notify_project_calltips_finished (AnjutaAsyncNotify* notify, CppJavaAssist* assist)
{
	assist->priv->async_calltip_project = FALSE;
}

/**
 * cpp_java_assist_query_calltip:
 * @assist: self
 * @call_context: name of method/function
 *
 * Starts an async query for the calltip
 */
static void
cpp_java_assist_query_calltip (CppJavaAssist *assist, const gchar *call_context)
{	
	IAnjutaSymbolType types = 
		IANJUTA_SYMBOL_TYPE_PROTOTYPE |
		IANJUTA_SYMBOL_TYPE_FUNCTION |
		IANJUTA_SYMBOL_TYPE_METHOD |
		IANJUTA_SYMBOL_TYPE_MACRO_WITH_ARG;
	IAnjutaSymbolField fields = 
		IANJUTA_SYMBOL_FIELD_SIMPLE |
		IANJUTA_SYMBOL_FIELD_TYPE |
		IANJUTA_SYMBOL_FIELD_ACCESS |
		IANJUTA_SYMBOL_FIELD_KIND;
	AnjutaAsyncNotify* notify;
	/* Search file */
	if (IANJUTA_IS_FILE (assist->priv->itip))
	{
		GFile *file = ianjuta_file_get_file (IANJUTA_FILE (assist->priv->itip), NULL);


		if (file != NULL)
		{
			notify = anjuta_async_notify_new();
			g_signal_connect (notify, "finished", G_CALLBACK(notify_file_calltips_finished), assist);
			assist->priv->async_calltip_file = TRUE;
			g_cancellable_reset (assist->priv->cancel_calltip_file);
			ianjuta_symbol_manager_search_file_async (assist->priv->isymbol_manager,
			                                          types,
			                                          TRUE, 
			                                          fields,
			                                          call_context, file, -1, -1,
			                                          assist->priv->cancel_calltip_file,
			                                          notify,
			                                          (IAnjutaSymbolManagerSearchCallback) on_calltip_search_complete,
			                                          assist,
			                                          NULL);			
			g_object_unref (file);
		}
	}

	types &= ~IANJUTA_SYMBOL_TYPE_FUNCTION;
	/* Search Project */
	notify = anjuta_async_notify_new();
	g_signal_connect (notify, "finished", G_CALLBACK(notify_project_calltips_finished), assist);
	assist->priv->async_calltip_project = TRUE;
	g_cancellable_reset (assist->priv->cancel_calltip_file);
	ianjuta_symbol_manager_search_project_async (assist->priv->isymbol_manager,
	                                             types,
	                                             TRUE, 
	                                             fields,
	                                             call_context, 
	                                             IANJUTA_SYMBOL_MANAGER_SEARCH_FS_PUBLIC, -1, -1,
	                                             assist->priv->cancel_calltip_project,
	                                             notify,
	                                             (IAnjutaSymbolManagerSearchCallback) on_calltip_search_complete,
	                                             assist,
	                                             NULL);
	
	/* Search system */
	notify = anjuta_async_notify_new();
	g_signal_connect (notify, "finished", G_CALLBACK(notify_system_calltips_finished), assist);
	assist->priv->async_calltip_system = TRUE;
	g_cancellable_reset (assist->priv->cancel_calltip_file);
	ianjuta_symbol_manager_search_system_async (assist->priv->isymbol_manager,
	                                            types,
	                                            TRUE, 
	                                            fields,
	                                            call_context, 
	                                            IANJUTA_SYMBOL_MANAGER_SEARCH_FS_PUBLIC, -1, -1,
	                                            assist->priv->cancel_calltip_system,
	                                            notify,
	                                            (IAnjutaSymbolManagerSearchCallback) on_calltip_search_complete,
	                                            assist,
	                                            NULL);
}

/**
 * cpp_java_assist_is_scope_context_character:
 * @ch: character to check
 *
 * Returns: if the current character seperates a scope
 */
static gboolean
cpp_java_assist_is_scope_context_character (gchar ch)
{
	if (g_ascii_isspace (ch))
		return FALSE;
	if (g_ascii_isalnum (ch))
		return TRUE;
	if (ch == '_' || ch == '.' || ch == ':' || ch == '>' || ch == '-')
		return TRUE;
	
	return FALSE;
}

#define SCOPE_BRACE_JUMP_LIMIT 50

/**
 * cpp_java_assist_get_scope_context
 * @editor: current editor
 * @scope_operator: The scope operator to check for
 * @iter: Current cursor position
 *
 * Find the scope context for calltips
 */
static gchar*
cpp_java_assist_get_scope_context (IAnjutaEditor* editor,
								   const gchar *scope_operator,
								   IAnjutaIterable *iter)
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
		if (cpp_java_assist_is_scope_context_character (ch))
		{
			scope_chars_found = TRUE;
		}
		else if (ch == ')')
		{
			if (!cpp_java_util_jump_to_matching_brace (iter, ch, SCOPE_BRACE_JUMP_LIMIT))
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
 * cpp_java_assist_create_calltip_context:
 * @assist: self
 * @iter: current cursor position
 *
 * Searches for a calltip context
 *
 * Returns: name of the method to show a calltip for or NULL
 */
static gchar*
cpp_java_assist_get_calltip_context (CppJavaAssist *assist,
                                     IAnjutaIterable *iter)
{
	gchar ch;
	gchar *context = NULL;	
	
	ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0, NULL);
	if (ch == ')')
	{
		if (!cpp_java_util_jump_to_matching_brace (iter, ')', -1))
			return NULL;
		if (!ianjuta_iterable_previous (iter, NULL))
			return NULL;
	}
	if (ch != '(')
	{
		if (!cpp_java_util_jump_to_matching_brace (iter, ')',
												   BRACE_SEARCH_LIMIT))
			return NULL;
	}
	
	/* Skip white spaces */
	while (ianjuta_iterable_previous (iter, NULL)
		&& g_ascii_isspace (ianjuta_editor_cell_get_char
								(IANJUTA_EDITOR_CELL (iter), 0, NULL)));

	context = cpp_java_assist_get_scope_context
		(IANJUTA_EDITOR (assist->priv->itip), "(", iter);

	/* Point iter to the first character of the scope to align calltip correctly */
	ianjuta_iterable_next (iter, NULL);
	
	return context;
}

/**
 * cpp_java_assist_create_calltip_context:
 * @assist: self
 * @call_context: The context (method/function name)
 * @position: iter where to show calltips
 *
 * Create the calltip context
 */
static void
cpp_java_assist_create_calltip_context (CppJavaAssist* assist,
                                        const gchar* call_context,
                                        IAnjutaIterable* position)
{
	assist->priv->calltip_context = g_strdup (call_context);
	assist->priv->calltip_iter = position;
}

/**
 * cpp_java_assist_clear_calltip_context:
 * @assist: self
 *
 * Clears the calltip context and brings it back into a save state
 */
static void
cpp_java_assist_clear_calltip_context (CppJavaAssist* assist)
{
	g_cancellable_cancel (assist->priv->cancel_calltip_file);
	g_cancellable_cancel (assist->priv->cancel_calltip_project);
	g_cancellable_cancel (assist->priv->cancel_calltip_system);
	
	g_free (assist->priv->calltip_context);
	assist->priv->calltip_context = NULL;
	
	g_list_foreach (assist->priv->tips, (GFunc) g_free, NULL);
	g_list_free (assist->priv->tips);
	assist->priv->tips = NULL;

	if (assist->priv->calltip_iter)
		g_object_unref (assist->priv->calltip_iter);
	assist->priv->calltip_iter = NULL;
}

/**
 * cpp_java_assist_calltip:
 * @assist: self
 * 
 * Creates a calltip if there is something to show a tip for
 * Calltips are queried async
 *
 * Returns: TRUE if a calltips was queried, FALSE otherwise
 */

static gboolean
cpp_java_assist_calltip (CppJavaAssist *assist)
{
	IAnjutaEditor *editor;
	IAnjutaIterable *iter;
	
	editor = IANJUTA_EDITOR (assist->priv->itip);
	
	iter = ianjuta_editor_get_position (editor, NULL);
	ianjuta_iterable_previous (iter, NULL);
	gchar *call_context =
		cpp_java_assist_get_calltip_context (assist, iter);
	if (call_context)
	{
		DEBUG_PRINT ("Searching calltip for: %s", call_context);
		if (assist->priv->calltip_context &&
		    g_str_equal (call_context, assist->priv->calltip_context))
		{
			/* Continue tip */
			if (assist->priv->tips)
			{
				if (!ianjuta_editor_tip_visible (IANJUTA_EDITOR_TIP (editor), NULL))
				{
					ianjuta_editor_tip_show (IANJUTA_EDITOR_TIP (editor),
					                         assist->priv->tips,
					                         assist->priv->calltip_iter, 0, NULL);
				}
			}
			g_free (call_context);
			return TRUE;
		}
		else /* New tip */
		{
			if (ianjuta_editor_tip_visible (IANJUTA_EDITOR_TIP (editor), NULL))
				ianjuta_editor_tip_cancel (IANJUTA_EDITOR_TIP (editor), NULL);
			
			cpp_java_assist_clear_calltip_context (assist);
			cpp_java_assist_create_calltip_context (assist, call_context, iter);
			cpp_java_assist_query_calltip (assist, call_context);
			g_free (call_context);
			return TRUE;
		}
	}
	else
	{
		if (ianjuta_editor_tip_visible (IANJUTA_EDITOR_TIP (editor), NULL))
			ianjuta_editor_tip_cancel (IANJUTA_EDITOR_TIP (editor), NULL);
		cpp_java_assist_clear_calltip_context (assist);
	}

	g_object_unref (iter);
	return FALSE;
}

/**
 * cpp_java_assist_populate:
 * @self: IAnjutaProvider object
 * @cursor: Iter at current cursor position (after current character)
 * @e: Error population
 */
static void
cpp_java_assist_populate (IAnjutaProvider* self, IAnjutaIterable* cursor, GError** e)
{
	CppJavaAssist* assist = CPP_JAVA_ASSIST (self);

	/* Check if this is a valid text region for completion */
	IAnjutaEditorAttribute attrib = ianjuta_editor_cell_get_attribute (IANJUTA_EDITOR_CELL(cursor),
	                                                                   NULL);
	if (attrib == IANJUTA_EDITOR_STRING ||
	    attrib == IANJUTA_EDITOR_COMMENT)
	{
		ianjuta_editor_assist_proposals (assist->priv->iassist,
		                                 IANJUTA_PROVIDER(self),
		                                 NULL, TRUE, NULL);
		return;
	}

	/* Check for calltip */
	if (assist->priv->itip && 
	    anjuta_preferences_get_bool_with_default (assist->priv->preferences,
	                                              PREF_CALLTIP_ENABLE,
	                                              TRUE))
	{	
		assist->priv->calltip_active = cpp_java_assist_calltip (assist);
			
	}
	
	/* Check if completion was in progress */
	if (assist->priv->member_completion || assist->priv->autocompletion)
	{
		IAnjutaIterable* start_iter = NULL;
		g_assert (assist->priv->completion_cache != NULL);
		gchar* pre_word = cpp_java_assist_get_pre_word (IANJUTA_EDITOR (assist->priv->iassist), cursor, &start_iter);
		if (pre_word && g_str_has_prefix (pre_word, assist->priv->pre_word))
		{
			/* Great, we just continue the current completion */
			g_object_unref (assist->priv->start_iter);
			assist->priv->start_iter = start_iter;

			cpp_java_assist_update_pre_word (assist, pre_word);
			cpp_java_assist_populate_real (assist, TRUE);
			g_free (pre_word);
			return;
		}
		else
			cpp_java_assist_clear_completion_cache (assist);
		g_free (pre_word);
	}
	/* Check for member completion */
	assist->priv->member_completion = FALSE;
	assist->priv->autocompletion = FALSE;
	if (cpp_java_assist_create_member_completion_cache (assist, cursor))
	{
		assist->priv->member_completion = TRUE;
		return;
	}
	else if (cpp_java_assist_create_autocompletion_cache (assist, cursor))
	{
		assist->priv->autocompletion = TRUE;
		return;
	}		
	/* Nothing to propose */
	if (assist->priv->start_iter)
	{
		g_object_unref (assist->priv->start_iter);
		assist->priv->start_iter = NULL;
	}
	ianjuta_editor_assist_proposals (assist->priv->iassist,
	                                 IANJUTA_PROVIDER(self),
	                                 NULL, TRUE, NULL);
} 

/**
 * cpp_java_assist_activate:
 * @self: IAnjutaProvider object
 * @iter: cursor position when proposal was activated
 * @data: Data assigned to the completion object
 * @e: Error population
 *
 * Called from the provider when the user activated a proposal
 */
static void
cpp_java_assist_activate (IAnjutaProvider* self, IAnjutaIterable* iter, gpointer data, GError** e)
{
	CppJavaAssist* assist = CPP_JAVA_ASSIST(self);
	ProposalData *prop_data = data;
	GString *assistance;
	IAnjutaEditor *te;
	gboolean add_space_after_func = FALSE;
	gboolean add_brace_after_func = FALSE;
				
	g_return_if_fail (prop_data != NULL);
	
	assistance = g_string_new (prop_data->name);
	
	if (prop_data->is_func)
	{
		add_space_after_func =
			anjuta_preferences_get_bool_with_default (assist->priv->preferences,
													 PREF_AUTOCOMPLETE_SPACE_AFTER_FUNC,
													 TRUE);
		add_brace_after_func =
			anjuta_preferences_get_bool_with_default (assist->priv->preferences,
													 PREF_AUTOCOMPLETE_BRACE_AFTER_FUNC,
													 TRUE);
		if (add_space_after_func)
			g_string_append (assistance, " ");
		
		if (add_brace_after_func)
			g_string_append (assistance, "(");
	}
	
	te = IANJUTA_EDITOR (assist->priv->iassist);
		
	ianjuta_document_begin_undo_action (IANJUTA_DOCUMENT (te), NULL);
	
	if (ianjuta_iterable_compare(iter, assist->priv->start_iter, NULL) != 0)
	{
		ianjuta_editor_selection_set (IANJUTA_EDITOR_SELECTION (te),
									  assist->priv->start_iter, iter, FALSE, NULL);
		ianjuta_editor_selection_replace (IANJUTA_EDITOR_SELECTION (te),
										  assistance->str, -1, NULL);
	}
	else
	{
		ianjuta_editor_insert (te, iter, assistance->str, -1, NULL);
	}
	ianjuta_document_end_undo_action (IANJUTA_DOCUMENT (te), NULL);

	/* Show calltip if we completed function */
	if (add_brace_after_func)
	{
		/* Check for calltip */
		if (assist->priv->itip && 
		    anjuta_preferences_get_bool_with_default (assist->priv->preferences,
		                                              PREF_CALLTIP_ENABLE,
		                                              TRUE))	
			assist->priv->calltip_active = cpp_java_assist_calltip (assist);

	}	
	g_string_free (assistance, TRUE);
}

/**
 * cpp_java_assist_get_start_iter:
 * @self: IAnjutaProvider object
 * @e: Error population
 *
 * Returns: the iter where the autocompletion starts
 */
static IAnjutaIterable*
cpp_java_assist_get_start_iter (IAnjutaProvider* provider, GError** e)
{
	CppJavaAssist* assist = CPP_JAVA_ASSIST (provider);
	return assist->priv->start_iter;
}

/**
 * cpp_java_assist_get_name:
 * @self: IAnjutaProvider object
 * @e: Error population
 *
 * Returns: the provider name
 */
static const gchar*
cpp_java_assist_get_name (IAnjutaProvider* provider, GError** e)
{
	return _("C/C++");
}

/**
 * cpp_java_assist_install:
 * @self: IAnjutaProvider object
 * @ieditor: Editor to install support for
 *
 * Returns: Registers provider for editor
 */
static void
cpp_java_assist_install (CppJavaAssist *assist, IAnjutaEditor *ieditor)
{
	g_return_if_fail (assist->priv->iassist == NULL);

	if (IANJUTA_IS_EDITOR_ASSIST (ieditor))
	{
		assist->priv->iassist = IANJUTA_EDITOR_ASSIST (ieditor);
		ianjuta_editor_assist_add (IANJUTA_EDITOR_ASSIST (ieditor), IANJUTA_PROVIDER(assist), NULL);
	}
	else
	{
		assist->priv->iassist = NULL;
	}

	if (IANJUTA_IS_EDITOR_TIP (ieditor))
	{
		assist->priv->itip = IANJUTA_EDITOR_TIP (ieditor);
	
	}
	else
	{
		assist->priv->itip = NULL;
	}
}

/**
 * cpp_java_assist_uninstall:
 * @self: IAnjutaProvider object
 *
 * Returns: Unregisters provider
 */
static void
cpp_java_assist_uninstall (CppJavaAssist *assist)
{
	g_return_if_fail (assist->priv->iassist != NULL);

	ianjuta_editor_assist_remove (assist->priv->iassist, IANJUTA_PROVIDER(assist), NULL);
	assist->priv->iassist = NULL;
}

static void
cpp_java_assist_init (CppJavaAssist *assist)
{
	assist->priv = g_new0 (CppJavaAssistPriv, 1);
	assist->priv->cancel_calltip_file = g_cancellable_new();
	assist->priv->cancel_calltip_project = g_cancellable_new();
	assist->priv->cancel_calltip_system = g_cancellable_new();
	assist->priv->cancel_file = g_cancellable_new();
	assist->priv->cancel_project = g_cancellable_new();
	assist->priv->cancel_system = g_cancellable_new();
}

static void
cpp_java_assist_finalize (GObject *object)
{
	CppJavaAssist *assist = CPP_JAVA_ASSIST (object);
	cpp_java_assist_uninstall (assist);
	cpp_java_assist_clear_completion_cache (assist);
#if 0
	if (assist->priv->calltip_context)
	{
		g_free (assist->priv->calltip_context);
		assist->priv->calltip_context = NULL;
	}
#endif
	g_object_unref (assist->priv->cancel_calltip_file);
	g_object_unref (assist->priv->cancel_calltip_project);
	g_object_unref (assist->priv->cancel_calltip_system);
	g_object_unref (assist->priv->cancel_file);
	g_object_unref (assist->priv->cancel_project);
	g_object_unref (assist->priv->cancel_system);		
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
cpp_java_assist_new (IAnjutaEditor *ieditor,
					 IAnjutaSymbolManager *isymbol_manager,
					 AnjutaPreferences *prefs)
{
	CppJavaAssist *assist;
	
	if (!IANJUTA_IS_EDITOR_ASSIST (ieditor) && !IANJUTA_IS_EDITOR_TIP (ieditor))
	{
		/* No assistance is available with the current editor */
		return NULL;
	}
	assist = g_object_new (TYPE_CPP_JAVA_ASSIST, NULL);
	assist->priv->isymbol_manager = isymbol_manager;
	assist->priv->preferences = prefs;
	cpp_java_assist_install (assist, ieditor);

	engine_parser_init (isymbol_manager);	
	
	return assist;
}

static void cpp_java_assist_iface_init(IAnjutaProviderIface* iface)
{
	iface->populate = cpp_java_assist_populate;
	iface->get_start_iter = cpp_java_assist_get_start_iter;
	iface->activate = cpp_java_assist_activate;
	iface->get_name = cpp_java_assist_get_name;
}
