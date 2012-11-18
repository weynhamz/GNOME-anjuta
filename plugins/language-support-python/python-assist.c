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
#include <glib/gi18n.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-language-provider.h>
#include <libanjuta/anjuta-launcher.h>
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-cell.h>
#include <libanjuta/interfaces/ianjuta-editor-tip.h>
#include <libanjuta/interfaces/ianjuta-language-provider.h>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>
#include <libanjuta/interfaces/ianjuta-symbol.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include "python-assist.h"

#define PREF_INTERPRETER_PATH "interpreter-path"
#define MAX_COMPLETIONS 30
#define BRACE_SEARCH_LIMIT 500
#define SCOPE_BRACE_JUMP_LIMIT 50

#define AUTOCOMPLETE_SCRIPT SCRIPTS_DIR"/anjuta-python-autocomplete.py"

#define AUTOCOMPLETE_REGEX_IN_GET_OBJECT "get_object\\s*\\(\\s*['\"]\\w*$"
#define FILE_LIST_DELIMITER "|"
#define SCOPE_CONTEXT_CHARACTERS ".0"
#define WORD_CHARACTER "_0"

static void iprovider_iface_init(IAnjutaProviderIface* iface);
static void ilanguage_provider_iface_init(IAnjutaLanguageProviderIface* iface);

G_DEFINE_TYPE_WITH_CODE (PythonAssist,
                         python_assist,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (IANJUTA_TYPE_PROVIDER,
			                                    iprovider_iface_init)
                         G_IMPLEMENT_INTERFACE (IANJUTA_TYPE_LANGUAGE_PROVIDER,
			                                    ilanguage_provider_iface_init))

struct _PythonAssistPriv {
	GSettings* settings;
	IAnjutaEditorAssist* iassist;
	IAnjutaEditorTip* itip;
	AnjutaLanguageProvider* lang_prov;
	AnjutaLauncher* launcher;
	AnjutaLauncher* calltip_launcher;	
	AnjutaPlugin* plugin;

	const gchar* project_root;
	const gchar* editor_filename;
	
	/* Autocompletion */
	GCompletion *completion_cache;
	gchar *pre_word;
	
	gint cache_position;
	GString* rope_cache;

	/* Calltips */
	gchar* calltip_context;
	IAnjutaIterable* calltip_iter;
	GList* tips;
	GString* calltip_cache;
};

static gchar*
completion_function (gpointer data)
{
	AnjutaLanguageProposalData * tag = ANJUTA_LANGUAGE_PROPOSAL_DATA(data);
	return tag->name;
}

static gint 
completion_compare (gconstpointer a, gconstpointer b)
{
	AnjutaLanguageProposalData * tag_a = ANJUTA_LANGUAGE_PROPOSAL_DATA(a);
	AnjutaLanguageProposalData * tag_b = ANJUTA_LANGUAGE_PROPOSAL_DATA(b);
	gint cmp;
	
	cmp = strcmp (tag_a->name, tag_b->name);
	if (cmp == 0) cmp = tag_a->type - tag_b->type;
	
	return cmp;
}

static void 
python_assist_tag_destroy (AnjutaLanguageProposalData *tag)
{
	anjuta_language_proposal_data_free (tag);
}

static void
python_assist_cancel_queries (PythonAssist* assist)
{
	if (assist->priv->launcher)
	{
		g_object_unref (assist->priv->launcher);
		assist->priv->launcher = NULL;
	}
}

static void 
python_assist_destroy_completion_cache (PythonAssist *assist)
{
	python_assist_cancel_queries (assist);
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
		g_string_free (assist->priv->rope_cache, TRUE);
		assist->priv->rope_cache = NULL;
	}
}

static void free_proposal (IAnjutaEditorAssistProposal* proposal)
{
	g_free (proposal->label);
	g_free(proposal);
}

static void
python_assist_cancelled (IAnjutaEditorAssist* editor,
                         PythonAssist* assist)
{
	python_assist_cancel_queries (assist);
}

static void
python_assist_update_autocomplete (PythonAssist *assist)
{
	GList *node, *suggestions = NULL;
	GList *completion_list = g_completion_complete (assist->priv->completion_cache, assist->priv->pre_word, NULL);
	
	for (node = completion_list; node != NULL; node = g_list_next (node))
	{
		AnjutaLanguageProposalData *tag = ANJUTA_LANGUAGE_PROPOSAL_DATA(node->data);
		IAnjutaEditorAssistProposal* proposal = g_new0(IAnjutaEditorAssistProposal, 1);

		if (tag->is_func)
			proposal->label = g_strdup_printf ("%s()", tag->name);
		else
			proposal->label = g_strdup(tag->name);
		
		if (tag->info)
			proposal->info = g_strdup(tag->info);
		proposal->data = tag;
		suggestions = g_list_prepend (suggestions, proposal);
	}
	suggestions = g_list_reverse (suggestions);
	
	ianjuta_editor_assist_proposals (IANJUTA_EDITOR_ASSIST (assist->priv->iassist),
	                                 IANJUTA_PROVIDER(assist), suggestions,
	                                 assist->priv->pre_word, TRUE, NULL);
	
	g_list_foreach (suggestions, (GFunc) free_proposal, NULL);
	g_list_free (suggestions);
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
		if (rope_file)
		{
			fprintf (rope_file, "%s", source);
			fclose (rope_file);
			close (tmp_fd);
			return tmp_file;
		}
	}

	/* Error */
	g_warning ("Creating tmp_file failed: %s", err->message);
	g_error_free (err);
	return NULL;
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
		DEBUG_PRINT ("chars from script: %s", chars);
		if (assist->priv->rope_cache)
		{
			g_string_append (assist->priv->rope_cache, chars);
		}
		else
		{
			assist->priv->rope_cache = g_string_new (chars);
		}
	}
	if (output_type == ANJUTA_LAUNCHER_OUTPUT_STDERR)
	{
		g_warning ("Problem in python script: %s", chars);
	}
}

static void
on_autocomplete_finished (AnjutaLauncher* launcher,
                          int child_pid, int exit_status,
                          gulong time, gpointer user_data)
{
	PythonAssist* assist = PYTHON_ASSIST (user_data);
	DEBUG_PRINT ("Python-Complete took %lu seconds and returned %d", time, exit_status);

	g_object_unref (launcher);
	assist->priv->launcher = NULL;
	
	if (assist->priv->rope_cache)
	{
		GStrv completions = g_strsplit (assist->priv->rope_cache->str, "\n", -1);
		GStrv cur_comp;
		GList* suggestions = NULL;
		GError *err = NULL;
		GRegex* regex = g_regex_new ("\\|(.+)\\|(.+)\\|(.+)\\|(.+)\\|(.+)\\|",
		                             0, 0, &err);
		if (err)
		{
			g_warning ("Error creating regex: %s", err->message);
			g_error_free (err);
			return;
		}

		/* Parse output and create completion list */
		for (cur_comp = completions; *cur_comp != NULL; cur_comp++)
		{
			AnjutaLanguageProposalData* tag;
			GMatchInfo* match_info;
			
			g_regex_match (regex, *cur_comp, 0, &match_info);
			if (g_match_info_matches (match_info) && 
			    g_match_info_get_match_count (match_info) == 6)
			{
				gchar* name = g_match_info_fetch (match_info, 1);
				gchar* type = g_match_info_fetch (match_info, 3); 
				gchar* location = g_match_info_fetch (match_info, 4); 
				gchar* info = g_match_info_fetch (match_info, 5); 
				tag = anjuta_language_proposal_data_new (name);

				/* info will be set to "_" if there is no relevant info */
				tag->info = NULL; 
				if (!g_str_equal(info, "_"))
					tag->info = g_strdup(info);
				

				if (g_str_equal(type, "function") || g_str_equal (type, "builtin"))
				{
					tag->type = IANJUTA_SYMBOL_TYPE_FUNCTION;
					tag->is_func = TRUE;
					/* TODO: not implemented yet */
					tag->has_para = TRUE;
				}
				else if (g_str_equal(type, "builder_object"))
				{
					tag->type = IANJUTA_SYMBOL_TYPE_EXTERNVAR;
					if (!g_str_equal(location, "_"))
						tag->info = g_strdup(location);
				}
				else
					tag->type = IANJUTA_SYMBOL_TYPE_VARIABLE;
				g_free (type);
				g_free (info);
				g_free (location);
				
				if (!g_list_find_custom (suggestions, tag, completion_compare))
				{
					suggestions = g_list_prepend (suggestions, tag);
				}
				else
					anjuta_language_proposal_data_free (tag);
			}
			g_match_info_free (match_info);
		}

		g_regex_unref (regex);
		g_strfreev (completions);

		g_string_free (assist->priv->rope_cache, TRUE);
		assist->priv->rope_cache = NULL;

		assist->priv->completion_cache = g_completion_new (completion_function);
		g_completion_add_items (assist->priv->completion_cache, suggestions);
		g_list_free (suggestions);
		
		/* Show autocompletion */
		python_assist_update_autocomplete (assist);
	}
}

static gboolean
python_assist_create_word_completion_cache (PythonAssist *assist, IAnjutaIterable* cursor)
{
	IAnjutaEditor *editor = IANJUTA_EDITOR (assist->priv->iassist);
	const gchar *cur_filename;
	gint offset = ianjuta_iterable_get_position (cursor, NULL);
	const gchar *project = assist->priv->project_root;
	gchar *interpreter_path;
	gchar *ropecommand;
	GString *builder_file_paths = g_string_new("");
	GList *project_files_list, *node;

	gchar *source = ianjuta_editor_get_text_all (editor, NULL);
	gchar *tmp_file;

	cur_filename = assist->priv->editor_filename;
	if (!project)
		project = g_get_tmp_dir ();
	
	/* Create rope command and temporary source file */
	interpreter_path = g_settings_get_string (assist->priv->settings,
	                                          PREF_INTERPRETER_PATH);

	tmp_file = create_tmp_file (source);
	g_free (source);

	if (!tmp_file)
		return FALSE;

	/* Get a list of all the builder files in the project */
	IAnjutaProjectManager *manager = anjuta_shell_get_interface (ANJUTA_PLUGIN (assist->priv->plugin)->shell,
								     IAnjutaProjectManager,
								     NULL);
	project_files_list = ianjuta_project_manager_get_elements (IANJUTA_PROJECT_MANAGER (manager), 
								   ANJUTA_PROJECT_SOURCE, 
								   NULL);
	for (node = project_files_list; node != NULL; node = g_list_next (node))
	{
		gchar *file_path = g_file_get_path (node->data);
		builder_file_paths = g_string_append (builder_file_paths, FILE_LIST_DELIMITER);
		builder_file_paths = g_string_append (builder_file_paths, file_path);
		g_free (file_path);
		g_object_unref (node->data);
	}
	g_list_free (project_files_list);
	
	ropecommand = g_strdup_printf("%s %s -o autocomplete -p \"%s\" -r \"%s\" -s \"%s\" -f %d -b \"%s\"", 
	                              interpreter_path, AUTOCOMPLETE_SCRIPT, project, 
	                              cur_filename, tmp_file, offset, builder_file_paths->str);
	g_string_free (builder_file_paths, TRUE);
	g_free (tmp_file);

	DEBUG_PRINT("%s", ropecommand);

	/* Exec command and wait for results */
	assist->priv->launcher = anjuta_launcher_new ();
	g_signal_connect (assist->priv->launcher, "child-exited",
	                  G_CALLBACK(on_autocomplete_finished), assist);
	anjuta_launcher_execute (assist->priv->launcher, ropecommand,
	                         on_autocomplete_output,
	                         assist);
	g_free (ropecommand);

	assist->priv->cache_position = offset;
	
	ianjuta_editor_assist_proposals (IANJUTA_EDITOR_ASSIST (assist->priv->iassist),
	                                 IANJUTA_PROVIDER (assist), NULL,
	                                 NULL, FALSE, NULL);
	
	return TRUE;
}

static void 
on_calltip_output (AnjutaLauncher *launcher,
                   AnjutaLauncherOutputType output_type,
                   const gchar *chars,
                   gpointer user_data)
{
	PythonAssist* assist = PYTHON_ASSIST (user_data);
	if (output_type == ANJUTA_LAUNCHER_OUTPUT_STDOUT)
	{
		if (assist->priv->calltip_cache)
		{
			g_string_append (assist->priv->calltip_cache, chars);
		}
		else
		{
			assist->priv->calltip_cache = g_string_new (chars);
		}
	}
}

static void
on_calltip_finished (AnjutaLauncher* launcher,
                     int child_pid, int exit_status,
                     gulong time, gpointer user_data)
{
	PythonAssist* assist = PYTHON_ASSIST (user_data);
	DEBUG_PRINT ("Python-Complete took %lu seconds and returned %d", time, exit_status);

	g_object_unref (launcher);
	assist->priv->calltip_launcher = NULL;

	if (assist->priv->calltip_cache)
	{
		GString* calltip_text = g_string_new (assist->priv->calltip_cache->str);
		assist->priv->tips = g_list_prepend (NULL, calltip_text->str);
		if (g_strncasecmp ("None", assist->priv->tips->data, 4))
		{
			ianjuta_editor_tip_show (IANJUTA_EDITOR_TIP(assist->priv->itip),
				                     assist->priv->tips,
				                     assist->priv->calltip_iter,
				                     NULL);
		}
		g_string_free (assist->priv->calltip_cache, TRUE);
		assist->priv->calltip_cache = NULL;
	}
}

static gint
python_assist_get_calltip_context_position (PythonAssist *assist)
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

static void
python_assist_query_calltip (PythonAssist* assist, const gchar *call_context)
{
	IAnjutaEditor *editor = IANJUTA_EDITOR (assist->priv->iassist);
	
	gint offset = python_assist_get_calltip_context_position (assist);
	
	gchar *interpreter_path;
	const gchar *cur_filename;
	gchar *source = ianjuta_editor_get_text_all (editor, NULL);
	const gchar *project = assist->priv->project_root;
	gchar *tmp_file;
	gchar *ropecommand;

	cur_filename = assist->priv->editor_filename;
	if (!project)
		project = g_get_tmp_dir ();
	
	interpreter_path = g_settings_get_string (assist->priv->settings,
	                                          PREF_INTERPRETER_PATH);

	tmp_file = create_tmp_file (source);
	g_free (source);

	if (!tmp_file)
		return;
	
	ropecommand = g_strdup_printf("%s %s -o calltip -p \"%s\" -r \"%s\" -s \"%s\" -f %d", 
								interpreter_path, AUTOCOMPLETE_SCRIPT, project, 
								cur_filename, tmp_file, offset);

	DEBUG_PRINT ("%s", ropecommand);

	g_free (tmp_file);

	/* Exec command and wait for results */
	assist->priv->calltip_launcher = anjuta_launcher_new ();
	g_signal_connect (assist->priv->calltip_launcher, "child-exited",
	                  G_CALLBACK(on_calltip_finished), assist);
	anjuta_launcher_execute (assist->priv->calltip_launcher, ropecommand,
	                         on_calltip_output,
	                         assist);
	g_free (ropecommand);	
}

static void
python_assist_create_calltip_context (PythonAssist* assist,
                                      const gchar* call_context,
                                      IAnjutaIterable* position)
{
	assist->priv->calltip_context = g_strdup (call_context);
	assist->priv->calltip_iter = position;
}

static void
python_assist_clear_calltip_context (PythonAssist* assist)
{
	if (assist->priv->calltip_launcher)
	{
		g_object_unref (assist->priv->calltip_launcher);
	}	
	assist->priv->calltip_launcher = NULL;
	
	g_list_foreach (assist->priv->tips, (GFunc) g_free, NULL);
	g_list_free (assist->priv->tips);
	assist->priv->tips = NULL;
	
	g_free (assist->priv->calltip_context);
	assist->priv->calltip_context = NULL;
	
	if (assist->priv->calltip_iter)
		g_object_unref (assist->priv->calltip_iter);
	assist->priv->calltip_iter = NULL;
}

static gchar*
python_assist_get_calltip_context (IAnjutaLanguageProvider *self,
                                   IAnjutaIterable *iter,
                                   GError** e)
{
	PythonAssist* assist = PYTHON_ASSIST (self);
	gchar* calltip_context;
	calltip_context = anjuta_language_provider_get_calltip_context (
	                     assist->priv->lang_prov, assist->priv->itip, iter,
	                     SCOPE_CONTEXT_CHARACTERS);
	return calltip_context;
}

static void
python_assist_update_pre_word (PythonAssist* assist, const gchar* pre_word)
{
	g_free (assist->priv->pre_word);
	if (pre_word == NULL) 
		pre_word = "";
	assist->priv->pre_word = g_strdup (pre_word);
}

/* returns TRUE if a '.', "'", or '"' preceeds the cursor position */
static gint
python_assist_completion_trigger_char (IAnjutaEditor* editor,
                   IAnjutaIterable* cursor)
{
	IAnjutaIterable* iter = ianjuta_iterable_clone (cursor, NULL);
	gboolean retval = FALSE;
	/* Go backward first as we are behind the current character */
	if (ianjuta_iterable_previous (iter, NULL))
	{
		gchar c = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter),
		                                        0, NULL);
		retval = ((c == '.') || (c == '\'') || (c == '"'));
	}
	g_object_unref (iter);
	return retval;
}

static GList*
python_assist_get_calltip_cache (IAnjutaLanguageProvider* self,
                                 gchar* call_context,
                                 GError** e)
{
	PythonAssist* assist = PYTHON_ASSIST (self);
	if (!g_strcmp0 (call_context, assist->priv->calltip_context))
	{
		DEBUG_PRINT ("Calltip was found in the cache.");
		return assist->priv->tips;
	}
	else
	{
		DEBUG_PRINT ("Calltip is not available in the cache!");
		return NULL;
	}
}

static void
python_assist_new_calltip (IAnjutaLanguageProvider* self,
                           gchar* call_context,
                           IAnjutaIterable* cursor,
                           GError** e)
{
	PythonAssist* assist = PYTHON_ASSIST (self);
	python_assist_clear_calltip_context (assist);
	python_assist_create_calltip_context (assist, call_context, cursor);
	python_assist_query_calltip (assist, call_context);
}

static IAnjutaIterable*
python_assist_populate_completions (IAnjutaLanguageProvider* self,
                                    IAnjutaIterable* cursor,
                                    GError** e)
{
	PythonAssist* assist = PYTHON_ASSIST (self);
	IAnjutaIterable* start_iter = NULL;
	gchar* pre_word;
	gboolean completion_trigger_char;
	
	pre_word = anjuta_language_provider_get_pre_word (
	                                   assist->priv->lang_prov, 
	                                   IANJUTA_EDITOR (assist->priv->iassist),
	                                   cursor, &start_iter, WORD_CHARACTER);

	DEBUG_PRINT ("Preword: %s", pre_word);
	
	/* Check if completion was in progress */
	if (assist->priv->completion_cache)
	{
		if (pre_word && g_str_has_prefix (pre_word, assist->priv->pre_word))
		{
			DEBUG_PRINT ("Continue autocomplete for %s", pre_word);
			
			/* Great, we just continue the current completion */
			python_assist_update_pre_word (assist, pre_word);
			python_assist_update_autocomplete (assist);
			g_free (pre_word);
			return start_iter;
		}
	}
	else
	{
		DEBUG_PRINT ("Cancelling autocomplete");
		python_assist_destroy_completion_cache (assist);
	}

	/* Autocompletion should not be triggered if we haven't started typing a word unless
	 * we just typed . or ' or "
	 */
	completion_trigger_char = python_assist_completion_trigger_char (
	                                  IANJUTA_EDITOR (assist->priv->iassist),
	                                  cursor);
	if (( (pre_word && strlen (pre_word) >= 3) || completion_trigger_char )
		&& python_assist_create_word_completion_cache (assist, cursor))
	{
		DEBUG_PRINT ("New autocomplete for %s", pre_word);
		if (!start_iter)
			start_iter = ianjuta_iterable_clone (cursor, NULL);
		python_assist_update_pre_word (assist, pre_word ? pre_word : "");
		g_free (pre_word);
		return start_iter;
	}
	g_free (pre_word);
	
	return NULL;
}

static void 
python_assist_install (PythonAssist *assist,
                       IAnjutaEditor *ieditor)
{
	g_return_if_fail (assist->priv->iassist == NULL);

	if (IANJUTA_IS_EDITOR_ASSIST (ieditor))
	{
		assist->priv->iassist = IANJUTA_EDITOR_ASSIST (ieditor);
		ianjuta_editor_assist_add (IANJUTA_EDITOR_ASSIST (ieditor), IANJUTA_PROVIDER(assist), NULL);
		g_signal_connect (ieditor, "cancelled", G_CALLBACK (python_assist_cancelled), assist);
	}
	else
		assist->priv->iassist = NULL;

	if (IANJUTA_IS_EDITOR_TIP (ieditor))
		assist->priv->itip = IANJUTA_EDITOR_TIP (ieditor);
	else
		assist->priv->itip = NULL;
		
	if (IANJUTA_IS_FILE (assist->priv->iassist))
	{
		GFile *file = ianjuta_file_get_file (IANJUTA_FILE (assist->priv->iassist), NULL);
		if (file != NULL)
		{
			assist->priv->editor_filename = g_file_get_path (file);
			g_object_unref (file);
		}
	}
}

static void
python_assist_uninstall (PythonAssist *assist)
{
	g_return_if_fail (assist->priv->iassist != NULL);

	if (IANJUTA_EDITOR_ASSIST (assist->priv->iassist))
	{
		g_signal_handlers_disconnect_by_func (assist->priv->iassist, python_assist_cancelled, assist);
		ianjuta_editor_assist_remove (assist->priv->iassist, IANJUTA_PROVIDER(assist), NULL);
	}
	
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
	python_assist_clear_calltip_context (assist);
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
python_assist_new (IAnjutaEditor *ieditor,
                   IAnjutaSymbolManager *isymbol_manager,
                   GSettings* settings,
                   AnjutaPlugin *plugin,
                   const gchar *project_root)
{
	PythonAssist *assist = g_object_new (TYPE_PYTHON_ASSIST, NULL);
	assist->priv->lang_prov = g_object_new (ANJUTA_TYPE_LANGUAGE_PROVIDER, NULL);
	assist->priv->settings = settings;
	assist->priv->plugin = plugin;
	assist->priv->project_root = project_root;
		
	/* Install support */
	python_assist_install (assist, ieditor);
	anjuta_language_provider_install (assist->priv->lang_prov, ieditor, settings);
	return assist;
}

static void
python_assist_activate (IAnjutaProvider* self,
                        IAnjutaIterable* iter,
                        gpointer data,
                        GError** e)
{
	PythonAssist* assist = PYTHON_ASSIST (self);
	anjuta_language_provider_activate (assist->priv->lang_prov, self, iter,
	                                   data);
}

static void
python_assist_populate (IAnjutaProvider* self,
                        IAnjutaIterable* cursor,
                        GError** e)
{
	PythonAssist* assist = PYTHON_ASSIST (self);
	anjuta_language_provider_populate (assist->priv->lang_prov, self, cursor);
}

static const gchar*
python_assist_get_name (IAnjutaProvider* self,
                        GError** e)
{
	return _("Python");
}

static IAnjutaIterable*
python_assist_get_start_iter (IAnjutaProvider* self,
                              GError** e)
{
	PythonAssist* assist = PYTHON_ASSIST (self);
	return anjuta_language_provider_get_start_iter (assist->priv->lang_prov);
}

static void
iprovider_iface_init (IAnjutaProviderIface* iface)
{
	iface->activate            = python_assist_activate;
	iface->populate            = python_assist_populate;
	iface->get_name            = python_assist_get_name;
	iface->get_start_iter      = python_assist_get_start_iter;
}

static void
ilanguage_provider_iface_init (IAnjutaLanguageProviderIface* iface)
{
	iface->get_calltip_cache   = python_assist_get_calltip_cache;
	iface->get_calltip_context = python_assist_get_calltip_context;
	iface->new_calltip         = python_assist_new_calltip;
	iface->populate_completions   = python_assist_populate_completions;
}
