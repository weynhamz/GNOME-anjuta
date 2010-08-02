/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * python-assist.c
 * Copyright (C) Ishan Chattopadhyaya 2009 <ichattopadhyaya@gmail.com>
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
#include <unistd.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-launcher.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-editor-cell.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-editor-tip.h>
#include <libanjuta/interfaces/ianjuta-document.h>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>
#include <libanjuta/interfaces/ianjuta-symbol.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h> 
#include "python-assist.h"
#include "python-utils.h"

#define PREF_AUTOCOMPLETE_ENABLE "language.python.code.completion.enable"
#define PREF_AUTOCOMPLETE_SPACE_AFTER_FUNC "language.python.code.completion.space.after.func"
#define PREF_AUTOCOMPLETE_BRACE_AFTER_FUNC "language.python.code.completion.brace.after.func"
#define PREF_CALLTIP_ENABLE "language.python.code.calltip.enable"
#define PREF_INTERPRETER_PATH "language.python.interpreter.path"
#define PREF_INTERPRETER_PYTHONPATH "language.python.interpreter.PYTHONPATH"
#define MAX_COMPLETIONS 30
#define BRACE_SEARCH_LIMIT 500
#define SCOPE_BRACE_JUMP_LIMIT 50

#define AUTOCOMPLETE_SCRIPT SCRIPTS_DIR"/anjuta-python-autocomplete.py"


static void python_assist_iface_init(IAnjutaProviderIface* iface);

//G_DEFINE_TYPE (PythonAssist, python_assist, G_TYPE_OBJECT);
G_DEFINE_TYPE_WITH_CODE (PythonAssist,
			 python_assist,
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (IANJUTA_TYPE_PROVIDER,
			                        python_assist_iface_init))

typedef struct
{
	gchar *name;
	gboolean is_func;
	IAnjutaSymbolType type;
} PythonAssistTag;

struct _PythonAssistPriv {
	AnjutaPreferences *preferences;
	IAnjutaSymbolManager* isymbol_manager;
	IAnjutaDocumentManager* idocument_manager;
	IAnjutaEditorAssist* iassist;
	IAnjutaEditorTip* itip;
	IAnjutaEditor* editor;
	AnjutaLauncher* launcher;

	const gchar* project_root;
	const gchar* editor_filename;
	
	/* Last used cache */
	gchar *search_cache;
	gchar *pre_word;
	gchar *calltip_context;
	gint calltip_context_position;
	
	GCompletion *completion_cache;
	gint cache_position;
	GString* rope_cache;

	IAnjutaIterable* start_iter;
};

static gchar*
completion_function (gpointer data)
{
	PythonAssistTag * tag = (PythonAssistTag*) data;
	return tag->name;
}

static gint 
completion_compare (gconstpointer a, gconstpointer b)
{
	PythonAssistTag * tag_a = (PythonAssistTag*) a;
	PythonAssistTag * tag_b = (PythonAssistTag*) b;
	gint cmp;
	
	cmp = strcmp (tag_a->name, tag_b->name);
	if (cmp == 0) cmp = tag_a->type - tag_b->type;
	
	return cmp;
}

static void 
python_assist_tag_destroy (PythonAssistTag *tag)
{
	g_free (tag->name);
	g_free (tag);
}

static gboolean 
is_scope_context_character (gchar ch)
{
	if (g_ascii_isspace (ch))
		return FALSE;
	if (g_ascii_isalnum (ch))
		return TRUE;
	if (ch == '.')
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

static gchar* 
python_assist_get_scope_context (IAnjutaEditor* editor,
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
			if (!python_util_jump_to_matching_brace (iter, ch, SCOPE_BRACE_JUMP_LIMIT))
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
python_assist_get_pre_word (IAnjutaEditor* editor, IAnjutaIterable *iter)
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
python_assist_destroy_completion_cache (PythonAssist *assist)
{
	if (assist->priv->launcher)
	{
		g_object_unref (assist->priv->launcher);
		assist->priv->launcher = NULL;
	}
	if (assist->priv->search_cache)
	{
		g_free (assist->priv->search_cache);
		assist->priv->search_cache = NULL;
	}
	if (assist->priv->completion_cache)
	{
		GList* items = assist->priv->completion_cache->items;
		if (items)
		{
			g_list_foreach (items, (GFunc) python_assist_tag_destroy, NULL);
			g_completion_clear_items (assist->priv->completion_cache);
		}
		g_completion_free (assist->priv->completion_cache);
		assist->priv->completion_cache = NULL;
	}
	if (assist->priv->rope_cache)
	{
		g_free (assist->priv->rope_cache);
		assist->priv->rope_cache = NULL;
	}
}

static gchar* 
get_tag_name (gchar* tag)
{
	gchar tagname[256];
	gint i;

	for (i=0; i<strlen(tag) && tag[i]!=' '; i++)
		tagname[i] = tag[i];
	tagname[i]='\0';
	return g_strdup(tagname);
}

static IAnjutaSymbolType 
get_tag_type (gchar* tag)
{
	int i;
	int startpos;
	int counter=0;
	gchar type[20];
	for (i=0; i<strlen(tag); i++)
		if (tag[i]==',')
		{
			startpos=i+2;
			break;
		}
	if (startpos<strlen(tag))
	{
		for (i=startpos; tag[i]!=')' && tag[i]!='\0'; i++)
		    type[counter++]=tag[i];
		type[counter]='\0';
	}
	if (g_str_equal(type, "function") || g_str_equal(type, "builtin") /*|| g_str_equal(type, "imported")*/)
		return IANJUTA_SYMBOL_TYPE_FUNCTION;
	else
		return IANJUTA_SYMBOL_TYPE_VARIABLE;

}


static gboolean 
is_cache_fresh (IAnjutaEditor *editor, gint editor_position, gint cache_position)
{
	gint i;

	if (editor_position < cache_position)
		return FALSE;
	IAnjutaIterable *begin = ianjuta_editor_get_position_from_offset (editor, cache_position, NULL);
	IAnjutaIterable *end = ianjuta_editor_get_position_from_offset (editor, editor_position, NULL);
	
	gchar* text = ianjuta_editor_get_text (editor, begin, end, NULL);

	if (!text)
		return TRUE;
	
	for (i=0; i<strlen(text); i++)
		if (text[i]==' ' || text[i]=='\n' || text[i]=='\r' || text[i]==':' || text[i]=='{' || text[i]=='(' || text[i]==';')
			return FALSE;

	return TRUE;
}



static void free_proposal (IAnjutaEditorAssistProposal* proposal)
{
	g_free (proposal->label);
	g_free(proposal);
}

void
python_assist_update_autocomplete (PythonAssist *assist)
{
	gint length;
	GList *completion_list;
	
	IAnjutaEditor *editor = (IAnjutaEditor*)assist->priv->editor;
	int editor_position = ianjuta_editor_get_offset(editor,NULL);

	// if cache is stale

	if (!is_cache_fresh(editor, editor_position, assist->priv->cache_position))
	{
		python_assist_destroy_completion_cache (assist);
		ianjuta_editor_assist_proposals (assist->priv->iassist, IANJUTA_PROVIDER(assist),
		                                 NULL, TRUE, NULL);
		return;
	}

	// If cache is empty, show nothing
	if (assist->priv->completion_cache == NULL)
	{
		ianjuta_editor_assist_proposals (assist->priv->iassist, IANJUTA_PROVIDER(assist),
		                                 NULL, TRUE, NULL);
		return;
	}

	if (assist->priv->pre_word && strlen(assist->priv->pre_word)>0)
	{
	    g_completion_complete (assist->priv->completion_cache, assist->priv->pre_word, NULL);

		completion_list = assist->priv->completion_cache->cache;
	}
	else
	{
		completion_list = assist->priv->completion_cache->items;
	}

	length = g_list_length (completion_list);

	DEBUG_PRINT ("Populating %d proposals", length);

	
	if (1) //length <= max_completions)
	{
		
		GList *node, *suggestions = NULL;
		
		for (node = completion_list; node != NULL; node = g_list_next (node))
		{
			PythonAssistTag *tag = node->data;
			IAnjutaEditorAssistProposal* proposal = g_new0(IAnjutaEditorAssistProposal, 1);
				
			if (tag->is_func)
				proposal->label = g_strdup_printf ("%s()", tag->name);
			else
				proposal->label = g_strdup(tag->name);
				
			proposal->data = tag;
			suggestions = g_list_prepend (suggestions, proposal);
		}
		suggestions = g_list_reverse (suggestions);
		ianjuta_editor_assist_proposals (assist->priv->iassist, IANJUTA_PROVIDER(assist),
		                                 suggestions, TRUE, NULL);
		g_list_foreach (suggestions, (GFunc) free_proposal, NULL);
		g_list_free (suggestions);
	}
	else
	{
		ianjuta_editor_assist_proposals (assist->priv->iassist, IANJUTA_PROVIDER(assist),
		                                 NULL, TRUE, NULL);
		return;
	}
}

/* Returns NULL if creation fails */
static gchar*
create_tmp_file (const gchar* source)
{
	gchar* tmp_file;
	gint tmp_fd;
	GError* err = NULL;

	tmp_fd = g_file_open_tmp (NULL, &tmp_file, &err);
	if (tmp_fd >= 0)
	{
		FILE *rope_file = fdopen (tmp_fd, "w");
		fprintf (rope_file, "%s", source);
		fclose (rope_file);
		close (tmp_fd);
		return tmp_file;
	}
	else
	{
		g_warning ("Creating tmp_file failed: %s", err->message);
		g_error_free (err);
		return NULL;
	}	
}

static void 
on_autocomplete_output (AnjutaLauncher *launcher,
                        AnjutaLauncherOutputType output_type,
                        const gchar *chars,
                        gpointer user_data)
{
	PythonAssist* assist = PYTHON_ASSIST (user_data);
	if (output_type == ANJUTA_LAUNCHER_OUTPUT_STDOUT)
	{
		if (assist->priv->rope_cache)
		{
			g_string_append (assist->priv->rope_cache, chars);
		}
		else
		{
			assist->priv->rope_cache = g_string_new (chars);
		}
	}
}

static void
on_autocomplete_finished (AnjutaLauncher* launcher,
                          int child_pid, int exit_status,
                          gulong time, gpointer user_data)
{
	PythonAssist* assist = PYTHON_ASSIST (user_data);
	DEBUG_PRINT ("Python-Complete took %lu seconds", time);
	if (exit_status != 0)
	{
		python_assist_destroy_completion_cache (assist);
		return;
	}
	else
	{
		g_message ("PythonAssist: %s", assist->priv->rope_cache->str);
		g_string_free (assist->priv->rope_cache, TRUE);
		assist->priv->rope_cache = NULL;
	}
}

/* This needs to be ported to AnjutaLauncher and made asynchronous. Extra
 * points for avoiding the intermediate script and using the Python/C API
 */
#define BUFFER_SIZE 1024
gboolean // VERIFY CONTENTS
python_assist_create_word_completion_cache (PythonAssist *assist)
{
	IAnjutaEditor *editor = IANJUTA_EDITOR (assist->priv->iassist);
	const gchar *cur_filename;
	gint offset = ianjuta_editor_get_offset(editor, NULL);
	const gchar *project = assist->priv->project_root;
	gchar *interpreter_path;
	gchar *ropecommand;

	GList *suggestions = NULL;
	gchar *source = ianjuta_editor_get_text_all (editor, NULL);
	gchar *tmp_file;

	cur_filename = assist->priv->editor_filename;
	if (!project)
		project = g_get_tmp_dir ();
	
	interpreter_path = anjuta_preferences_get (assist->priv->preferences,
	                                           PREF_INTERPRETER_PATH);

	tmp_file = create_tmp_file (source);
	g_free (source);

	if (!tmp_file)
		return FALSE;
	
	ropecommand = g_strdup_printf("%s %s -o autocomplete -p \"%s\" -r \"%s\" -s \"%s\" -f %d", 
	                              interpreter_path, AUTOCOMPLETE_SCRIPT, project, 
	                              cur_filename, tmp_file, offset);

	g_free (tmp_file);
	DEBUG_PRINT ("%s\n", ropecommand);

	assist->priv->launcher = anjuta_launcher_new ();
	g_signal_connect (assist->priv->launcher, "child-exited",
	                  G_CALLBACK(on_autocomplete_finished), assist);
	anjuta_launcher_execute (assist->priv->launcher, ropecommand,
	                         on_autocomplete_output,
	                         assist);
	g_free (ropecommand);
	
	assist->priv->completion_cache = g_completion_new (completion_function);
	g_completion_add_items (assist->priv->completion_cache, suggestions);
	assist->priv->cache_position = offset;
	assist->priv->search_cache = g_strdup (assist->priv->pre_word);

	return TRUE;
}




static gchar* //OK
python_assist_get_calltip_context (PythonAssist *assist,
                                     IAnjutaIterable *iter)
{
	gchar ch;
	gchar *context = NULL;	
	gint original_offset = ianjuta_editor_get_offset (IANJUTA_EDITOR (assist->priv->iassist), NULL);
	
	ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0, NULL);
	if (ch == ')')
	{
		if (!python_util_jump_to_matching_brace (iter, ')', -1))
			return NULL;
		if (!ianjuta_iterable_previous (iter, NULL))
			return NULL;
	}
	if (ch != '(')
	{
		if (!python_util_jump_to_matching_brace (iter, ')',
												   BRACE_SEARCH_LIMIT))
			return NULL;
	}
	
	/* Skip white spaces */
	while (ianjuta_iterable_previous (iter, NULL)
		&& g_ascii_isspace (ianjuta_editor_cell_get_char
								(IANJUTA_EDITOR_CELL (iter), 0, NULL)))
		original_offset--;

	context = python_assist_get_scope_context
		(IANJUTA_EDITOR (assist->priv->iassist), "(", iter);

	/* Point iter to the first character of the scope to align calltip correctly */
	ianjuta_iterable_next (iter, NULL);
	
	return context;
}

static gint
python_assist_get_calltip_context_position (PythonAssist *assist,
                                     IAnjutaIterable *iter)
{
	gchar ch;
	gint final_offset;
	IAnjutaEditor *editor = IANJUTA_EDITOR (assist->priv->iassist);
	IAnjutaIterable *current_iter = ianjuta_editor_get_position (editor, NULL);
	
	while (ianjuta_iterable_previous (current_iter, NULL))
	{
		ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (current_iter), 0, NULL);
		if (ch == '(')
		    break;
	}
	final_offset = ianjuta_iterable_get_position (current_iter, NULL);
	
	return final_offset-1;
}

/* This needs to be ported to AnjutaLauncher and made asynchronous. Extra
 * points for avoiding the intermediate script and using the Python/C API
 */
static GList*
python_assist_create_rope_calltips (PythonAssist *assist, IAnjutaIterable* iter)
{
	GList* tips = NULL;

	IAnjutaEditor *editor = IANJUTA_EDITOR (assist->priv->iassist);
	gint offset = assist->priv->calltip_context_position;
	gchar *interpreter_path;
	const gchar *cur_filename;
	gchar *source = ianjuta_editor_get_text_all (editor, NULL);
	const gchar *project = assist->priv->project_root;
	gchar *tmp_file;
	gchar *ropecommand;
	gchar calltip[BUFFER_SIZE];
	int counter = 0;
	FILE *rope;

	cur_filename = assist->priv->editor_filename;
	if (!project)
		project = g_get_tmp_dir ();
	
	interpreter_path = anjuta_preferences_get (assist->priv->preferences,
	                                           PREF_INTERPRETER_PATH);

	tmp_file = create_tmp_file (source);
	g_free (source);

	if (!tmp_file)
		return NULL;
	
	ropecommand = g_strdup_printf("%s %s -o calltip -p \"%s\" -r \"%s\" -s \"%s\" -f %d", 
								interpreter_path, AUTOCOMPLETE_SCRIPT, project, 
								cur_filename, tmp_file, offset);

	DEBUG_PRINT ("%s\n", ropecommand);

	g_free (tmp_file);

	rope = popen (ropecommand, "r");
	while (fgets(calltip, BUFFER_SIZE, rope) != NULL)
	{
		calltip[strlen(calltip)-1] = '\0';
		tips = g_list_append (tips, g_strdup(calltip));
		counter++;
		DEBUG_PRINT ("'Calltip: %s\n'", calltip);
	}
	pclose(rope);

	DEBUG_PRINT ("%s(): %d\n", "python_assist_create_rope_calltips", assist->priv->calltip_context_position);
	DEBUG_PRINT("Calltips returned %d suggestions..\n", counter);
	
	return tips;
}

static gboolean
python_assist_show_calltip (PythonAssist *assist, gchar *call_context, 
							  IAnjutaIterable *position_iter)
{	
	GList *tips = NULL;

	// Add calltips from rope
	tips = g_list_concat (tips, python_assist_create_rope_calltips (assist, position_iter));
	
	if (tips)
	{	
		ianjuta_editor_tip_show (IANJUTA_EDITOR_TIP(assist->priv->itip), tips,
										 position_iter, 
										 NULL);
		g_list_foreach (tips, (GFunc) g_free, NULL);
		g_list_free (tips);
		return TRUE;
	}
	return FALSE;
}

static void //NEWLY
python_assist_calltip (PythonAssist *assist)
{
	IAnjutaEditor *editor;
	IAnjutaIterable *iter;
	IAnjutaIterable *cur_pos;

	DEBUG_PRINT("calltip started\n");
	
	editor = IANJUTA_EDITOR (assist->priv->iassist);
	
	iter = ianjuta_editor_get_position (editor, NULL);
	cur_pos = ianjuta_iterable_clone (iter, NULL);
	ianjuta_iterable_previous (iter, NULL);

	if (1) //calltips)
	{
		gchar *call_context = python_assist_get_calltip_context (assist, iter);
		gint call_context_position = python_assist_get_calltip_context_position (assist, iter);

		DEBUG_PRINT ("CALLTIP CONTEXT: %s at position %d\n", call_context, call_context_position);

		if (call_context)
		{
			if(ianjuta_editor_tip_visible (IANJUTA_EDITOR_TIP (editor), NULL))
			{
				if (!g_str_equal (call_context, assist->priv->calltip_context))
				{
					assist->priv->calltip_context_position = call_context_position;
					
					python_assist_show_calltip (assist, call_context, 
					                              iter);
					g_free (assist->priv->calltip_context);
					assist->priv->calltip_context = g_strdup(call_context);
					
				}
			}
			else
			{
				assist->priv->calltip_context_position = call_context_position;
				python_assist_show_calltip (assist, call_context, 
				                              iter);
				g_free (assist->priv->calltip_context);
				assist->priv->calltip_context = g_strdup(call_context);
				
			}
		}
		else
		{
			//ianjuta_editor_assist_cancel_tips (assist->priv->iassist, NULL);
			ianjuta_editor_tip_cancel (IANJUTA_EDITOR_TIP(assist->priv->itip), NULL);
			g_free (assist->priv->calltip_context);
			assist->priv->calltip_context = NULL;
			assist->priv->calltip_context_position = -1;
		}
		g_free (call_context);
	}
	g_object_unref (iter);	
	DEBUG_PRINT("calltip ended\n");
}




static void
on_editor_char_added (IAnjutaEditor *editor, IAnjutaIterable *insert_pos,
					  gchar ch, PythonAssist *assist)
{	
	gboolean enable_calltips =
		anjuta_preferences_get_bool_with_default (assist->priv->preferences,
												 PREF_CALLTIP_ENABLE,
												 TRUE);
	if (enable_calltips)
		python_assist_calltip (assist);

}

static gboolean
is_word_or_operator(gchar c)
{
	if (is_word_character (c) || c == '.' || c == '-' || c == '>')
		return TRUE;
	return FALSE;
}


static IAnjutaIterable*
python_parse_expression (PythonAssist* assist, IAnjutaIterable* iter, IAnjutaIterable** start_iter)
{
	IAnjutaEditor* editor = editor = IANJUTA_EDITOR (assist->priv->iassist);
	IAnjutaIterable* cur_pos = ianjuta_iterable_clone (iter, NULL);
	gboolean op_start = FALSE;
	gboolean ref_start = FALSE;
	gchar* stmt = NULL;

	DEBUG_PRINT ("parse started\n");
	
	/* Search for a operator in the current line */
	do 
	{
		gchar ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL(cur_pos), 0, NULL);

		DEBUG_PRINT ("ch == '%c'", ch);
		
		if (!is_word_or_operator (ch))
			break;

		if (ch == '.' || (op_start && ch == '-') || (ref_start && ch == ':'))
		{
			/* Found an operator, get the statement and the pre_word */
			IAnjutaIterable* pre_word_start = ianjuta_iterable_clone (cur_pos, NULL);
			IAnjutaIterable* pre_word_end = ianjuta_iterable_clone (iter, NULL);
			IAnjutaIterable* stmt_end = ianjuta_iterable_clone (pre_word_start, NULL);

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
				if (!is_word_character (word_ch))
					break;
			}
			ianjuta_iterable_next (cur_pos, NULL);
			stmt = ianjuta_editor_get_text (editor,
			                                cur_pos, stmt_end, NULL);
			*start_iter = pre_word_start;
			g_object_unref (stmt_end);
			DEBUG_PRINT("parse ended\n");
			return NULL;
		}
	}
	while (ianjuta_iterable_previous (cur_pos, NULL));
	DEBUG_PRINT("parse ended\n");
	return NULL;
}

static gchar
get_previous_character(IAnjutaEditor* editor)
{
	IAnjutaIterable *cur_pos = ianjuta_editor_get_position (editor, NULL);
	
	gchar ch;

		ianjuta_iterable_previous (cur_pos, NULL);
	ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL(cur_pos), 0, NULL);

	DEBUG_PRINT("Previous char is: %c\n", ch);
	return ch; 
}

static void
python_assist_populate (IAnjutaProvider* self, IAnjutaIterable* iter, GError** e)
{
	PythonAssist* assist = PYTHON_ASSIST (self);
	IAnjutaEditor *editor;
	gboolean autocomplete = anjuta_preferences_get_bool_with_default (assist->priv->preferences,
	                                                                  PREF_AUTOCOMPLETE_ENABLE,
	                                                                  TRUE);	
	editor = IANJUTA_EDITOR (assist->priv->iassist);

	g_free (assist->priv->pre_word);
	assist->priv->pre_word = NULL;

	ianjuta_iterable_previous (iter, NULL);
	
	if (autocomplete)
	{
		/* Check for member completion */
		IAnjutaIterable* start_iter = NULL;

		python_parse_expression (assist, iter, &start_iter);

		if (start_iter && assist->priv->pre_word && assist->priv->search_cache &&
		    g_str_has_prefix (assist->priv->pre_word, assist->priv->search_cache))
		{
			if (assist->priv->start_iter)
				g_object_unref (assist->priv->start_iter);
			assist->priv->start_iter = start_iter;
			python_assist_update_autocomplete (assist);

			return;
		}

		/* Normal autocompletion */
		/* Moved iter to begin of word */		
		DEBUG_PRINT ("NORMAL\n");
		assist->priv->pre_word = python_assist_get_pre_word (editor, iter);
		DEBUG_PRINT ("Pre word: %s\n", assist->priv->pre_word);
		
		if (!assist->priv->search_cache || !assist->priv->pre_word ||
		    !g_str_has_prefix (assist->priv->pre_word, assist->priv->search_cache))
		{
			if (assist->priv->start_iter)
				g_object_unref (assist->priv->start_iter);
			assist->priv->start_iter = ianjuta_iterable_clone(iter, NULL);
			ianjuta_iterable_next (IANJUTA_ITERABLE (assist->priv->start_iter), NULL);
			python_assist_create_word_completion_cache(assist);

			// If previous character isn't a <dot>, clear cache
			if (!is_scope_context_character (get_previous_character(editor)))
				python_assist_destroy_completion_cache (assist);
				python_assist_update_autocomplete (assist);
			    
			return;
		}
		else
		{
			python_assist_update_autocomplete (assist);
			return;
		}
	}
	/* Keep completion system happy */
	ianjuta_editor_assist_proposals (assist->priv->iassist,
	                                 IANJUTA_PROVIDER(self),
	                                 NULL, TRUE, NULL);
} 



static void 
python_assist_activate (IAnjutaProvider* self, IAnjutaIterable* iter, gpointer data, GError** e)
{
	PythonAssist* assist = PYTHON_ASSIST(self);
	PythonAssistTag *tag;
	GString *assistance;
	IAnjutaEditor *te;
	gboolean add_space_after_func = FALSE;
	gboolean add_brace_after_func = FALSE;
	
	DEBUG_PRINT ("assist-chosen: \n");
	
	tag = data;	
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

	DEBUG_PRINT("Reached here safely");
	
	/* Show calltip if we completed function */
	if (add_brace_after_func)
		python_assist_calltip (assist);
	
	g_string_free (assistance, TRUE);
}

static IAnjutaIterable*
python_assist_get_start_iter (IAnjutaProvider* provider, GError** e)
{ 
	PythonAssist* assist = PYTHON_ASSIST (provider);
	return assist->priv->start_iter;
}

static const gchar*
python_assist_get_name (IAnjutaProvider* provider, GError** e)
{
	return _("Python");
}

static void 
python_assist_install (PythonAssist *assist, IAnjutaEditor *ieditor)
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
	
		g_signal_connect (ieditor, "char-added",
						  G_CALLBACK (on_editor_char_added), assist);
		/*g_signal_connect (assist, "backspace",
						  G_CALLBACK (on_editor_backspace), assist);*/

	}
	else
	{
		assist->priv->itip = NULL;
	}
}

static void
python_assist_uninstall (PythonAssist *assist)
{
	g_return_if_fail (assist->priv->iassist != NULL);

	DEBUG_PRINT ("Python uninstall called\n");
	
	g_signal_handlers_disconnect_by_func (assist->priv->iassist, G_CALLBACK (on_editor_char_added), assist);

	ianjuta_editor_assist_remove (assist->priv->iassist, IANJUTA_PROVIDER(assist), NULL);

	assist->priv->iassist = NULL;
}

static void
python_assist_init (PythonAssist *assist)
{
	assist->priv = g_new0 (PythonAssistPriv, 1);
}

static void
python_assist_finalize (GObject *object)
{
	PythonAssist *assist = PYTHON_ASSIST (object);
	python_assist_uninstall (assist);
	python_assist_destroy_completion_cache (assist);
	if (assist->priv->calltip_context)
	{
		g_free (assist->priv->calltip_context);
		assist->priv->calltip_context = NULL;
	}
	g_free (assist->priv);
	G_OBJECT_CLASS (python_assist_parent_class)->finalize (object);
}

static void
python_assist_class_init (PythonAssistClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = python_assist_finalize;
}

PythonAssist * 
python_assist_new (IAnjutaEditorAssist *iassist,
                   IAnjutaSymbolManager *isymbol_manager,
                   IAnjutaDocumentManager *idocument_manager,
                   AnjutaPreferences *prefs,
                   const gchar *editor_filename,
                   const gchar *project_root)
{
	PythonAssist *assist = g_object_new (TYPE_PYTHON_ASSIST, NULL);
	assist->priv->isymbol_manager = isymbol_manager;
	assist->priv->idocument_manager = idocument_manager;
	assist->priv->editor_filename = editor_filename;
	assist->priv->preferences = prefs;
	assist->priv->project_root = project_root;
	assist->priv->editor=(IAnjutaEditor*)iassist;
	python_assist_install (assist, IANJUTA_EDITOR (iassist));
	DEBUG_PRINT("New done\n");
	return assist;
}

static void python_assist_iface_init(IAnjutaProviderIface* iface)
{
	iface->populate = python_assist_populate;
	iface->get_start_iter = python_assist_get_start_iter;
	iface->activate = python_assist_activate;
	iface->get_name = python_assist_get_name;
}
