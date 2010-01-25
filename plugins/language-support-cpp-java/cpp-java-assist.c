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
#define PREF_AUTOCOMPLETE_CHOICES "language.cpp.code.completion.choices"
#define PREF_AUTOCOMPLETE_SPACE_AFTER_FUNC "language.cpp.code.completion.space.after.func"
#define PREF_AUTOCOMPLETE_BRACE_AFTER_FUNC "language.cpp.code.completion.brace.after.func"
#define PREF_CALLTIP_ENABLE "language.cpp.code.calltip.enable"
#define MAX_COMPLETIONS 10
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
	
	/* Last used cache */
	gchar *search_cache;
	gchar *scope_context_cache;
	gchar *pre_word;
	gchar *calltip_context;
	
	GCompletion *completion_cache;
	gboolean editor_only;
	IAnjutaIterable* start_iter;
	
	gboolean async_file : 1;
	gboolean async_system : 1;
	gboolean async_project : 1;
};

static gchar*
completion_function (gpointer data)
{
	CppJavaAssistTag * tag = (CppJavaAssistTag*) data;
	return tag->name;
}

static gint
completion_compare (gconstpointer a, gconstpointer b)
{
	CppJavaAssistTag * tag_a = (CppJavaAssistTag*) a;
	CppJavaAssistTag * tag_b = (CppJavaAssistTag*) b;
	gint cmp;
	
	cmp = strcmp (tag_a->name, tag_b->name);
	if (cmp == 0) cmp = tag_a->type - tag_b->type;
	
	return cmp;
}

static void
cpp_java_assist_tag_destroy (CppJavaAssistTag *tag)
{
	g_free (tag->name);
	if (tag->icon)
		gdk_pixbuf_unref (tag->icon);
	g_free (tag);
}

static gboolean
is_scope_context_character (gchar ch)
{
	if (g_ascii_isspace (ch))
		return FALSE;
	if (g_ascii_isalnum (ch))
		return TRUE;
	if (ch == '_' || ch == '.' || ch == ':' || ch == '>' || ch == '-')
		return TRUE;
	
	return FALSE;
}	

static gboolean
is_word_character (gchar ch)
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
 * If mergeable is NULL than no merge will be made with iter elements, elsewhere
 * mergeable will be returned with iter elements.
 */
static GCompletion*
create_completion (CppJavaAssist* assist, IAnjutaIterable* iter,
				   GCompletion* mergeable)
{	
	GCompletion *completion;	
	
	if (mergeable == NULL)
		completion = g_completion_new (completion_function);
	else
		completion = mergeable;
	
	GList* suggestions = NULL;
	do
	{
		const gchar* name = ianjuta_symbol_get_name (IANJUTA_SYMBOL(iter), NULL);
		if (name != NULL)
		{
			CppJavaAssistTag *tag = g_new0 (CppJavaAssistTag, 1);
			const GdkPixbuf* sym_icon;
			tag->name = g_strdup (name);
			DEBUG_PRINT ("Created tag: %s", tag->name);
			tag->type = ianjuta_symbol_get_sym_type (IANJUTA_SYMBOL (iter),
													 NULL);
			sym_icon = ianjuta_symbol_get_icon (IANJUTA_SYMBOL(iter), NULL);
			tag->icon = sym_icon ? gdk_pixbuf_copy (sym_icon) : NULL;
			tag->is_func = (tag->type == IANJUTA_SYMBOL_TYPE_PROTOTYPE ||
			                tag->type == IANJUTA_SYMBOL_TYPE_FUNCTION ||
							tag->type == IANJUTA_SYMBOL_TYPE_METHOD ||
							tag->type == IANJUTA_SYMBOL_TYPE_MACRO_WITH_ARG);
			if (!g_list_find_custom (suggestions, tag, completion_compare))
				suggestions = g_list_prepend (suggestions, tag);
			else
				g_free (tag);
		}
		else
			break;
	}
	while (ianjuta_iterable_next (iter, NULL));
	
	suggestions = g_list_sort (suggestions, completion_compare);
	g_completion_add_items (completion, suggestions);
	return completion;
}

static void cpp_java_assist_update_autocomplete(CppJavaAssist* assist);

static void
on_query_data (gint search_id, IAnjutaIterable* iter, CppJavaAssist* assist)
{
	assist->priv->completion_cache = create_completion (assist,
	                                                    iter,
	                                                    assist->priv->completion_cache);
	cpp_java_assist_update_autocomplete(assist);
}

static void
system_finished (AnjutaAsyncNotify* notify, CppJavaAssist* assist)
{
	assist->priv->async_system = FALSE;
}

static void
file_finished (AnjutaAsyncNotify* notify, CppJavaAssist* assist)
{
	assist->priv->async_file = FALSE;
}

static void
project_finished (AnjutaAsyncNotify* notify, CppJavaAssist* assist)
{
	assist->priv->async_project = FALSE;
}

#define SCOPE_BRACE_JUMP_LIMIT 50

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
		if (is_scope_context_character (ch))
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

static gchar*
cpp_java_assist_get_pre_word (IAnjutaEditor* editor, IAnjutaIterable *iter)
{
	IAnjutaIterable *end;
	gchar ch, *preword_chars = NULL;
	gboolean out_of_range = FALSE;
	gboolean preword_found = FALSE;
	
	end = ianjuta_iterable_clone (iter, NULL);
	ianjuta_iterable_next (end, NULL);

	ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0, NULL);
	
	while (ch && is_word_character (ch))
	{
		preword_found = TRUE;
		if (!ianjuta_iterable_previous (iter, NULL))
		{
			out_of_range = TRUE;
			break;
		}
		ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0, NULL);
	}
	
	if (preword_found)
	{
		IAnjutaIterable *begin = ianjuta_iterable_clone (iter, NULL);
		if (!out_of_range)
			ianjuta_iterable_next (begin, NULL);
		preword_chars = ianjuta_editor_get_text (editor, begin, end, NULL);
		g_object_unref (begin);
	}
	g_object_unref (end);
	return preword_chars;
}

static void
cpp_java_assist_destroy_completion_cache (CppJavaAssist *assist)
{
	if (assist->priv->search_cache)
	{
		g_free (assist->priv->search_cache);
		assist->priv->search_cache = NULL;
	}
	if (assist->priv->scope_context_cache)
	{
		g_free (assist->priv->scope_context_cache);
		assist->priv->scope_context_cache = NULL;
	} 
	if (assist->priv->completion_cache)
	{
		GList* items = assist->priv->completion_cache->items;
		if (items)
		{
			g_list_foreach (items, (GFunc) cpp_java_assist_tag_destroy, NULL);
			g_completion_clear_items (assist->priv->completion_cache);
		}
		g_completion_free (assist->priv->completion_cache);
		assist->priv->completion_cache = NULL;
	}
}

static void free_proposal (IAnjutaEditorAssistProposal* proposal)
{
	g_free (proposal->label);
	g_free(proposal);
}

static void
cpp_java_assist_update_autocomplete (CppJavaAssist *assist)
{
	gint max_completions, length;
	GList *completion_list;

	gboolean queries_active = (assist->priv->async_file || assist->priv->async_project || assist->priv->async_system);

	// DEBUG_PRINT ("Queries active: %d", queries_active);
	
	if (assist->priv->completion_cache == NULL)
	{
		ianjuta_editor_assist_proposals (assist->priv->iassist, IANJUTA_PROVIDER(assist),
		                                 NULL, !queries_active, NULL);
		return;
	}

	if (assist->priv->pre_word && strlen(assist->priv->pre_word))
	{
	    g_completion_complete (assist->priv->completion_cache, assist->priv->pre_word, NULL);

		completion_list = assist->priv->completion_cache->cache;
	}
	else
	{
		completion_list = assist->priv->completion_cache->items;
	}
		
	max_completions =
		anjuta_preferences_get_int_with_default (assist->priv->preferences,
												 PREF_AUTOCOMPLETE_CHOICES,
												 MAX_COMPLETIONS);

	length = g_list_length (completion_list);

	DEBUG_PRINT ("Populating %d proposals", length);
	
	if (length <= max_completions)
	{
		GList *node, *suggestions = NULL;
			
		for (node = completion_list; node != NULL; node = g_list_next (node))
		{
			CppJavaAssistTag *tag = node->data;
			IAnjutaEditorAssistProposal* proposal = g_new0(IAnjutaEditorAssistProposal, 1);
				
			if (tag->is_func)
				proposal->label = g_strdup_printf ("%s()", tag->name);
			else
				proposal->label = g_strdup(tag->name);
				
			proposal->data = tag;
			proposal->icon = tag->icon;
			suggestions = g_list_prepend (suggestions, proposal);
		}
		suggestions = g_list_reverse (suggestions);
		ianjuta_editor_assist_proposals (assist->priv->iassist, IANJUTA_PROVIDER(assist),
		                                 suggestions, !queries_active, NULL);
		g_list_foreach (suggestions, (GFunc) free_proposal, NULL);
		g_list_free (suggestions);
	}
	else
	{
		ianjuta_editor_assist_proposals (assist->priv->iassist, IANJUTA_PROVIDER(assist),
		                                 NULL, !queries_active, NULL);
		return;
	}
}

static void
cpp_java_assist_create_word_completion_cache (CppJavaAssist *assist)
{
	gint max_completions;

	max_completions =
		anjuta_preferences_get_int_with_default (assist->priv->preferences,
												 PREF_AUTOCOMPLETE_CHOICES,
												 MAX_COMPLETIONS);
	
	cpp_java_assist_destroy_completion_cache (assist);
	if (assist->priv->async_file)
	{
		assist->priv->async_file = FALSE;
	}
	if (assist->priv->async_system)
	{
		assist->priv->async_system = FALSE;
	}
	if (assist->priv->async_project)
	{
		assist->priv->async_project = FALSE;
	}
	if (!assist->priv->pre_word || strlen(assist->priv->pre_word) < 3)
		return;

	gchar *pattern = g_strconcat (assist->priv->pre_word, "%", NULL);
		
	if (IANJUTA_IS_FILE (assist->priv->iassist))
	{
		GFile *file = ianjuta_file_get_file (IANJUTA_FILE (assist->priv->iassist), NULL);
		if (file != NULL)
		{
			AnjutaAsyncNotify* notify = anjuta_async_notify_new();
			g_signal_connect (notify, "finished", G_CALLBACK(file_finished), assist);
			assist->priv->async_file = TRUE;
			ianjuta_symbol_manager_search_file_async (assist->priv->isymbol_manager,
																				 IANJUTA_SYMBOL_TYPE_UNDEF,
																				 TRUE,
																				 IANJUTA_SYMBOL_FIELD_SIMPLE|IANJUTA_SYMBOL_FIELD_TYPE,
																				 pattern, file, -1, -1, NULL,
																				 notify, (IAnjutaSymbolManagerSearchCallback) on_query_data, assist,
																				 NULL);
			g_object_unref (file);
		}
	}
	{
		/* This will avoid duplicates of FUNCTION and PROTOTYPE */
		IAnjutaSymbolType types = IANJUTA_SYMBOL_TYPE_MAX & ~IANJUTA_SYMBOL_TYPE_FUNCTION;
		AnjutaAsyncNotify* notify = anjuta_async_notify_new();
		g_signal_connect (notify, "finished", G_CALLBACK(project_finished), assist);
		assist->priv->async_project = TRUE;
		ianjuta_symbol_manager_search_project_async (assist->priv->isymbol_manager,
											 types,
											 TRUE,
											 IANJUTA_SYMBOL_FIELD_SIMPLE|IANJUTA_SYMBOL_FIELD_TYPE,
											 pattern, IANJUTA_SYMBOL_MANAGER_SEARCH_FS_PUBLIC, -1, -1, 
											 NULL,
											 notify, (IAnjutaSymbolManagerSearchCallback) on_query_data, assist,
											 NULL);
		
	}
	{
		IAnjutaSymbolType types = IANJUTA_SYMBOL_TYPE_MAX & ~IANJUTA_SYMBOL_TYPE_FUNCTION;
		AnjutaAsyncNotify* notify = anjuta_async_notify_new();
		g_signal_connect (notify, "finished", G_CALLBACK(system_finished), assist);
		assist->priv->async_system = TRUE;
		ianjuta_symbol_manager_search_system_async (assist->priv->isymbol_manager,
											 types,
											 TRUE,
											 IANJUTA_SYMBOL_FIELD_SIMPLE|IANJUTA_SYMBOL_FIELD_TYPE,
											 pattern, IANJUTA_SYMBOL_MANAGER_SEARCH_FS_PUBLIC, -1, -1,
											 NULL,
											 notify, (IAnjutaSymbolManagerSearchCallback) on_query_data, assist,
		                                            NULL);
	}
	g_free (pattern);
	assist->priv->search_cache = g_strdup (assist->priv->pre_word);
	DEBUG_PRINT ("Started async search for: %s", assist->priv->pre_word);
}

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

static GList*
cpp_java_assist_create_calltips (IAnjutaIterable* iter)
{
	GList* tips = NULL;
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

static gboolean
cpp_java_assist_show_calltip (CppJavaAssist *assist, gchar *call_context,
							  IAnjutaIterable *position_iter)
{	
	GList *tips = NULL;
	gint max_completions;
	
	max_completions =
		anjuta_preferences_get_int_with_default (assist->priv->preferences,
												 PREF_AUTOCOMPLETE_CHOICES,
												 MAX_COMPLETIONS);

	/* Search file */
	if (IANJUTA_IS_FILE (assist->priv->itip))
	{
		GFile *file = ianjuta_file_get_file (IANJUTA_FILE (assist->priv->itip), NULL);

		if (file != NULL)
		{
			IAnjutaIterable* iter_file = ianjuta_symbol_manager_search_file (assist->priv->isymbol_manager,
																			 IANJUTA_SYMBOL_TYPE_PROTOTYPE|
																			 IANJUTA_SYMBOL_TYPE_FUNCTION|
																			 IANJUTA_SYMBOL_TYPE_METHOD|
																			 IANJUTA_SYMBOL_TYPE_MACRO_WITH_ARG,
																			 TRUE, IANJUTA_SYMBOL_FIELD_SIMPLE,
																			 call_context, file, max_completions, -1, NULL);
												 
			if (iter_file) 
			{
				tips = cpp_java_assist_create_calltips (iter_file);
				g_object_unref (iter_file);
			}
			g_object_unref (file);
		}
	}

	/* Search Project */
	IAnjutaIterable* iter_project = 
		ianjuta_symbol_manager_search_project (assist->priv->isymbol_manager,
									   IANJUTA_SYMBOL_TYPE_PROTOTYPE|
									   IANJUTA_SYMBOL_TYPE_FUNCTION|
									   IANJUTA_SYMBOL_TYPE_METHOD|
									   IANJUTA_SYMBOL_TYPE_MACRO_WITH_ARG,
									   TRUE, IANJUTA_SYMBOL_FIELD_SIMPLE,
									   call_context, IANJUTA_SYMBOL_MANAGER_SEARCH_FS_PUBLIC,
									   max_completions, -1, NULL);
	if (iter_project)
	{
		tips = g_list_concat (tips, cpp_java_assist_create_calltips (iter_project));
		g_object_unref (iter_project);
	}
	
	/* Search global */
	IAnjutaIterable* iter_global = 
		ianjuta_symbol_manager_search_system (assist->priv->isymbol_manager,
									   IANJUTA_SYMBOL_TYPE_PROTOTYPE|
									   IANJUTA_SYMBOL_TYPE_FUNCTION|
									   IANJUTA_SYMBOL_TYPE_METHOD|
									   IANJUTA_SYMBOL_TYPE_MACRO_WITH_ARG,
									   TRUE, IANJUTA_SYMBOL_FIELD_SIMPLE,
									   call_context, IANJUTA_SYMBOL_MANAGER_SEARCH_FS_PUBLIC,
									   max_completions, -1, NULL);
	if (iter_global)
	{
		tips = g_list_concat (tips, cpp_java_assist_create_calltips (iter_global));
		g_object_unref (iter_global);
	}
	
	if (tips)
	{	
		ianjuta_editor_tip_show (IANJUTA_EDITOR_TIP(assist->priv->itip), tips,
										 position_iter, 0,
										 NULL);
		g_list_foreach (tips, (GFunc) g_free, NULL);
		g_list_free (tips);
		return TRUE;
	}
	return FALSE;
}

static void
cpp_java_assist_calltip (CppJavaAssist *assist,
					      			   gboolean calltips, gboolean backspace)
{
	IAnjutaEditor *editor;
	IAnjutaIterable *iter;
	
	if (!calltips)
		return; /* Nothing to do */
	
	editor = IANJUTA_EDITOR (assist->priv->itip);
	
	iter = ianjuta_editor_get_position (editor, NULL);
	ianjuta_iterable_previous (iter, NULL);
	if (calltips)
	{
		gchar *call_context =
			cpp_java_assist_get_calltip_context (assist, iter);
		if (call_context)
		{
			if (ianjuta_editor_tip_visible (IANJUTA_EDITOR_TIP (editor), NULL))
			{
				if (assist->priv->calltip_context &&
				    !g_str_equal (call_context, assist->priv->calltip_context))
				{
					cpp_java_assist_show_calltip (assist, call_context,
					                              iter);
					g_free (assist->priv->calltip_context);
					assist->priv->calltip_context = g_strdup(call_context);
				}
			}
			else
			{
				cpp_java_assist_show_calltip (assist, call_context,
				                              iter);
				g_free (assist->priv->calltip_context);
				assist->priv->calltip_context = g_strdup(call_context);	
			}
		}
		else
		{
			ianjuta_editor_tip_cancel (IANJUTA_EDITOR_TIP(assist->priv->itip), NULL);
			g_free (assist->priv->calltip_context);
			assist->priv->calltip_context = NULL;
		}
		g_free (call_context);
	}
	g_object_unref (iter);	
}

static void
on_editor_char_added (IAnjutaEditor *editor, IAnjutaIterable *insert_pos,
					  gchar ch, CppJavaAssist *assist)
{
	gboolean enable_calltips =
		anjuta_preferences_get_bool_with_default (assist->priv->preferences,
												 PREF_CALLTIP_ENABLE,
												 TRUE);
	cpp_java_assist_calltip(assist, enable_calltips, (ch == '\b'));
}

/* FIXME: find a better tester */
static gboolean
is_expression_separator (gchar c)
{
	if (c == ';' || c == '\n' || c == '\r' || c == '\t' || /*c == '(' || c == ')' || */
	    c == '{' || c == '}' || c == '=' || c == '<' /*|| c == '>'*/ || c == '\v' || c == '!')
	{
		return TRUE;
	}

	return FALSE;
}

static IAnjutaIterable*
cpp_java_parse_expression (CppJavaAssist* assist, IAnjutaIterable* iter, IAnjutaIterable** start_iter)
{
	IAnjutaEditor* editor = editor = IANJUTA_EDITOR (assist->priv->iassist);
	IAnjutaIterable* res = NULL;
	IAnjutaIterable* cur_pos = ianjuta_iterable_clone (iter, NULL);
	gboolean op_start = FALSE;
	gboolean ref_start = FALSE;
	gchar* stmt = NULL;
	
	/* Search for a operator in the current line */
	do 
	{
		gchar ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL(cur_pos), 0, NULL);

		DEBUG_PRINT ("ch == '%c'", ch);
		
		if (is_expression_separator(ch)) {
			DEBUG_PRINT ("found char '%c' which is an expression_separator", ch);
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
			/* Move the end character to be behind the current typed character */
			ianjuta_iterable_next (pre_word_end, NULL);
			
			assist->priv->pre_word = ianjuta_editor_get_text (editor,
			                                                  pre_word_start, pre_word_end, NULL);

			/* Try to get the name of the variable
			 * FIXME: What about get_widget()-> for example */
			while (ianjuta_iterable_previous (cur_pos, NULL))
			{
				gchar word_ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL(cur_pos), 0, NULL);
				
				if (is_expression_separator(word_ch)) 
					break;				
			}
			ianjuta_iterable_next (cur_pos, NULL);
			stmt = ianjuta_editor_get_text (editor,
			                                cur_pos, stmt_end, NULL);
			*start_iter = pre_word_start;
			g_object_unref (stmt_end);
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

		DEBUG_PRINT ("Pre word: %s Statement: %s", assist->priv->pre_word, stmt);

		if (IANJUTA_IS_FILE (assist->priv->iassist))
		{
			GFile *file = ianjuta_file_get_file (IANJUTA_FILE (assist->priv->iassist), NULL);
			if (file != NULL)
			{
				filename = g_file_get_path (file);
			}
			g_object_unref (file);
		}
		start = ianjuta_editor_get_start_position (editor, NULL);
		above_text = ianjuta_editor_get_text (editor, start, iter, NULL);
		g_object_unref (start);
		
		lineno = ianjuta_editor_get_lineno (editor, NULL);

		/* the parser works even for the "Gtk::" like expressions, so it shouldn't be 
		 * created a specific case to handle this.
		 */
		DEBUG_PRINT ("calling engine_parser_process_expression stmt: %s ", stmt);
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

static gboolean
cpp_java_assist_valid_iter (CppJavaAssist* assist, IAnjutaIterable* iter)
{
	IAnjutaEditorCell* cell = IANJUTA_EDITOR_CELL (iter);
	IAnjutaEditorAttribute attribute = ianjuta_editor_cell_get_attribute (cell, NULL);
	return (attribute != IANJUTA_EDITOR_STRING && attribute != IANJUTA_EDITOR_COMMENT);
}

static void
cpp_java_assist_populate (IAnjutaProvider* self, IAnjutaIterable* iter, GError** e)
{
	CppJavaAssist* assist = CPP_JAVA_ASSIST (self);
	IAnjutaEditor *editor;
	gboolean autocomplete = anjuta_preferences_get_bool_with_default (assist->priv->preferences,
	                                                                  PREF_AUTOCOMPLETE_ENABLE,
	                                                                  TRUE);	
	editor = IANJUTA_EDITOR (assist->priv->iassist);

	g_free (assist->priv->pre_word);
	assist->priv->pre_word = NULL;

	ianjuta_iterable_previous (iter, NULL);

	if (autocomplete && cpp_java_assist_valid_iter (assist, iter))
	{
		/* Check for member completion */
		IAnjutaIterable* start_iter = NULL;
		IAnjutaIterable* res = 
			cpp_java_parse_expression (assist, iter, &start_iter);
		if (start_iter && assist->priv->pre_word && assist->priv->search_cache && res != NULL &&
		    g_str_has_prefix (assist->priv->pre_word, assist->priv->search_cache))
		{
			if (assist->priv->start_iter)
				g_object_unref (assist->priv->start_iter);
			assist->priv->start_iter = start_iter;
			cpp_java_assist_update_autocomplete (assist);
			g_object_unref (res);
			return;
		}
		/* we should have a res with just one item */
		if (res != NULL) 
		{
			do
			{
				IAnjutaSymbol* symbol = IANJUTA_SYMBOL(res);
				const gchar* name = ianjuta_symbol_get_name(symbol, NULL);
				if (name != NULL)
				{
					DEBUG_PRINT ("PARENT Completion Symbol: %s", name);

					/* we have just printed the parent's name */
					/* let's get it's members and print them */
					IAnjutaIterable *children = 
						ianjuta_symbol_manager_get_members (assist->priv->isymbol_manager,
						                                    symbol,
						                                    IANJUTA_SYMBOL_FIELD_SIMPLE,
						                                    NULL);
					if (children != NULL) {
						cpp_java_assist_destroy_completion_cache (assist);
						assist->priv->completion_cache = create_completion (assist,
						                                                    children,
						                                                    NULL);
						if (assist->priv->start_iter)
							g_object_unref (assist->priv->start_iter);
						assist->priv->start_iter = start_iter;
						cpp_java_assist_update_autocomplete(assist);
					}
				}
				else
					break;
			}
			while (ianjuta_iterable_next (res, NULL));
			g_object_unref (res);
			return;
		}
		
		/* Normal autocompletion */
		/* Moved iter to begin of word */		
		assist->priv->pre_word = cpp_java_assist_get_pre_word (editor, iter);
		DEBUG_PRINT ("Pre word: %s", assist->priv->pre_word);
		
		if (assist->priv->pre_word && strlen (assist->priv->pre_word) > 3)
		{	
			if (!assist->priv->search_cache ||
			    !g_str_has_prefix (assist->priv->pre_word, assist->priv->search_cache))
			{
				if (assist->priv->start_iter)
					g_object_unref (assist->priv->start_iter);
				assist->priv->start_iter = ianjuta_iterable_clone(iter, NULL);
				ianjuta_iterable_next (IANJUTA_ITERABLE (assist->priv->start_iter), NULL);
				cpp_java_assist_create_word_completion_cache(assist);
				return;
			}
			else
			{
				cpp_java_assist_update_autocomplete (assist);
				return;
			}
		}
	}
	/* Keep completion system happy */
	ianjuta_editor_assist_proposals (assist->priv->iassist,
	                                 IANJUTA_PROVIDER(self),
	                                 NULL, TRUE, NULL);
} 

static void
cpp_java_assist_activate (IAnjutaProvider* self, IAnjutaIterable* iter, gpointer data, GError** e)
{
	CppJavaAssist* assist = CPP_JAVA_ASSIST(self);
	CppJavaAssistTag *tag;
	GString *assistance;
	IAnjutaEditor *te;
	gboolean add_space_after_func = FALSE;
	gboolean add_brace_after_func = FALSE;
	
	//DEBUG_PRINT ("assist-chosen: %d", selection);
		
	tag = data;	
	
	g_return_if_fail (tag != NULL);
	
	assistance = g_string_new (tag->name);
	
	if (tag->is_func)
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
		cpp_java_assist_calltip (assist, TRUE, FALSE);
	
	g_string_free (assistance, TRUE);
}

static IAnjutaIterable*
cpp_java_assist_get_start_iter (IAnjutaProvider* provider, GError** e)
{
	CppJavaAssist* assist = CPP_JAVA_ASSIST (provider);
	return assist->priv->start_iter;
}

static const gchar*
cpp_java_assist_get_name (IAnjutaProvider* provider, GError** e)
{
	return _("C/C++");
}

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
	
		g_signal_connect (IANJUTA_EDITOR_TIP (ieditor), "char-added",
						  G_CALLBACK (on_editor_char_added), assist);
	}
	else
	{
		assist->priv->itip = NULL;
	}
}

static void
cpp_java_assist_uninstall (CppJavaAssist *assist)
{
	g_return_if_fail (assist->priv->iassist != NULL);

	DEBUG_PRINT ("uninstall called");
	
	g_signal_handlers_disconnect_by_func (assist->priv->iassist, G_CALLBACK (on_editor_char_added), assist);

	ianjuta_editor_assist_remove (assist->priv->iassist, IANJUTA_PROVIDER(assist), NULL);

	assist->priv->iassist = NULL;
}

static void
cpp_java_assist_init (CppJavaAssist *assist)
{
	assist->priv = g_new0 (CppJavaAssistPriv, 1);
}

static void
cpp_java_assist_finalize (GObject *object)
{
	CppJavaAssist *assist = CPP_JAVA_ASSIST (object);
	cpp_java_assist_uninstall (assist);
	cpp_java_assist_destroy_completion_cache (assist);
	if (assist->priv->calltip_context)
	{
		g_free (assist->priv->calltip_context);
		assist->priv->calltip_context = NULL;
	}
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
