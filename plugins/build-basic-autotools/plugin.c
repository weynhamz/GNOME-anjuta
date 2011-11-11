/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2000 Naba Kumar

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <config.h>
#include <ctype.h>
#include <stdlib.h>

#include <glib/gstdio.h>
#include <gio/gio.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-launcher.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-plugin-manager.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-manager.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>
#include <libanjuta/interfaces/ianjuta-buildable.h>
#include <libanjuta/interfaces/ianjuta-builder.h>
#include <libanjuta/interfaces/ianjuta-environment.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-language.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <libanjuta/interfaces/ianjuta-indicable.h>
#include <libanjuta/interfaces/ianjuta-markable.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>

#include "plugin.h"
#include "build-options.h"
#include "executer.h"
#include "program.h"
#include "build.h"

#include <sys/wait.h>
#if defined(__FreeBSD__)
#include <sys/signal.h>
#endif

#define ICON_FILE "anjuta-build-basic-autotools-plugin-48.png"
#define ANJUTA_PIXMAP_BUILD            "anjuta-build"
#define ANJUTA_STOCK_BUILD             "anjuta-build"

#define UI_FILE PACKAGE_DATA_DIR "/ui/anjuta-build-basic-autotools-plugin.xml"
#define MAX_BUILD_PANES 3
#define PREF_SCHEMA "org.gnome.anjuta.plugins.build"
#define PREF_INDICATORS_AUTOMATIC "indicators-automatic"
#define PREF_PARALLEL_MAKE "parallel-make"
#define PREF_PARALLEL_MAKE_JOB "parallel-make-job"
#define PREF_TRANSLATE_MESSAGE "translate-message"
#define PREF_CONTINUE_ON_ERROR "continue-error"

#define BUILD_PREFS_DIALOG "preferences-dialog-build"
#define BUILD_PREFS_ROOT "preferences-build-container"
#define INSTALL_ROOT_CHECK "preferences:install-root"
#define INSTALL_ROOT_ENTRY "preferences:install-root-command"
#define PARALLEL_MAKE_CHECK "preferences:parallel-make"
#define PARALLEL_MAKE_SPIN "preferences:parallel-make-job"

static gpointer parent_class;

typedef struct
{
	gchar *pattern;
	int options;
	gchar *replace;
	GRegex *regex;
} BuildPattern;

typedef struct
{
	gchar *pattern;
	GRegex *regex;
	GRegex *local_regex;
} MessagePattern;

typedef struct
{
	GFile *file;
	gchar *tooltip;
	gint line;
	IAnjutaIndicableIndicator indicator;
} BuildIndicatorLocation;

/* Command processing */
struct _BuildContext
{
	AnjutaPlugin *plugin;

	AnjutaLauncher *launcher;
	gboolean used;

	BuildProgram *program;

	IAnjutaMessageView *message_view;
	GHashTable *build_dir_stack;

	/* Indicator locations */
	GSList *locations;

	/* Editors in which indicators have been updated */
	GHashTable *indicators_updated_editors;

	/* Environment */
	IAnjutaEnvironment *environment;

	/* Saved files */
	gint file_saved;
};

/* Declarations */
static void update_project_ui (BasicAutotoolsPlugin *bb_plugin);

static GList *patterns_list = NULL;

/* The translations should match that of 'make' program. Both strings uses
 * pearl regular expression
 * 2 similar strings are used in order to parse the output of 2 different
 * version of make if necessary. If you update one string, move the first
 * string into the second slot and then replace the first string only. */
static MessagePattern patterns_make_entering[] = {{N_("make(\\[\\d+\\])?:\\s+Entering\\s+directory\\s+`(.+)'"), NULL, NULL},
												{N_("make(\\[\\d+\\])?:\\s+Entering\\s+directory\\s+'(.+)'"), NULL, NULL},
												{NULL, NULL, NULL}};

/* The translations should match that of 'make' program. Both strings uses
 * pearl regular expression
 * 2 similar strings are used in order to parse the output of 2 different
 * version of make if necessary. If you update one string, move the first
 * string into the second slot and then replace the first string only. */
static MessagePattern patterns_make_leaving[] = {{N_("make(\\[\\d+\\])?:\\s+Leaving\\s+directory\\s+`(.+)'"), NULL, NULL},
												{N_("make(\\[\\d+\\])?:\\s+Leaving\\s+directory\\s+'(.+)'"), NULL, NULL},
												{NULL, NULL, NULL}};

/* Helper functions
 *---------------------------------------------------------------------------*/

/* Allow installation as root (#321455) */
static void on_root_check_toggled(GtkWidget* toggle_button, GtkWidget* entry)
{
		gtk_widget_set_sensitive(entry,
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle_button)));
}

static gchar*
escape_label (const gchar *str)
{
	GString *ret;
	const gchar *iter;

	ret = g_string_new ("");
	iter = str;
	while (*iter != '\0')
	{
		if (*iter == '_')
		{
			ret = g_string_append (ret, "__");
			iter++;
		}
		else
		{
			const gchar *start;
			const gchar *end;

			start = iter;
			iter = g_utf8_next_char (iter);
			end = iter;

			ret = g_string_append_len (ret, start, end - start);
		}
	}
	return g_string_free (ret, FALSE);
}

/* Indicator locations reported by the build */

static BuildIndicatorLocation*
build_indicator_location_new (const gchar *filename, gint line,
							  IAnjutaIndicableIndicator indicator,
                              const gchar* tooltip)
{
	BuildIndicatorLocation *loc = g_new0 (BuildIndicatorLocation, 1);
	loc->file = g_file_new_for_path (filename);
	loc->line = line;
	loc->indicator = indicator;
	loc->tooltip = g_strdup (tooltip);
	return loc;
}

static void
build_indicator_location_set (BuildIndicatorLocation *loc,
							  IAnjutaEditor *editor,
							  GFile *editor_file)
{
	IAnjutaIterable *line_start, *line_end;

	if (editor && editor_file &&
		IANJUTA_IS_INDICABLE (editor) &&
		IANJUTA_IS_EDITOR (editor) &&
		g_file_equal (editor_file, loc->file))
	{
		DEBUG_PRINT ("loc line: %d", loc->line);

		line_start = ianjuta_editor_get_line_begin_position (editor,
															 loc->line, NULL);

		line_end = ianjuta_editor_get_line_end_position (editor,
														 loc->line, NULL);
		ianjuta_indicable_set (IANJUTA_INDICABLE (editor),
							   line_start, line_end, loc->indicator,
							   NULL);

		g_object_unref (line_start);
		g_object_unref (line_end);
	}
	if (editor && editor_file &&
	    IANJUTA_IS_MARKABLE (editor) &&
		g_file_equal (editor_file, loc->file))
	{
		ianjuta_markable_mark (IANJUTA_MARKABLE (editor),
		                       loc->line, IANJUTA_MARKABLE_MESSAGE,
		                       loc->tooltip, NULL);
	}
}

static void
build_indicator_location_free (BuildIndicatorLocation *loc)
{
	g_object_unref (loc->file);
	g_free (loc->tooltip);
	g_free (loc);
}

/* Build context */

static void
build_context_stack_destroy (gpointer value)
{
	GSList *slist = (GSList *)value;
	if (slist)
	{
		g_slist_foreach (slist, (GFunc)g_free, NULL);
		g_slist_free (slist);
	}
}

static void
build_context_push_dir (BuildContext *context, const gchar *key,
						const gchar *dir)
{
	GSList *dir_stack;

	if (context->build_dir_stack == NULL)
	{
		context->build_dir_stack =
			g_hash_table_new_full (g_str_hash, g_str_equal, NULL,
								   build_context_stack_destroy);
	}
	dir_stack = g_hash_table_lookup (context->build_dir_stack, key);
	if (dir_stack)
		g_hash_table_steal (context->build_dir_stack, key);

	dir_stack =	g_slist_prepend (dir_stack, g_strdup (dir));
	g_hash_table_insert (context->build_dir_stack, (gpointer)key, dir_stack);
}

static void
build_context_pop_dir (BuildContext *context, const gchar *key,
						const gchar *dir)
{
	GSList *dir_stack;
	gchar *top_dir;

	if (context->build_dir_stack == NULL)
		return;
	dir_stack = g_hash_table_lookup (context->build_dir_stack, key);
	if (dir_stack == NULL)
		return;

	g_hash_table_steal (context->build_dir_stack, key);
	top_dir = dir_stack->data;
	dir_stack =	g_slist_remove (dir_stack, top_dir);

	if (strcmp (top_dir, dir) != 0)
	{
		DEBUG_PRINT("%s", "Directory stack misaligned!!");
	}
	g_free (top_dir);
	if (dir_stack)
		g_hash_table_insert (context->build_dir_stack, (gpointer)key, dir_stack);
}

static const gchar *
build_context_get_dir (BuildContext *context, const gchar *key)
{
	GSList *dir_stack;

	if (context->build_dir_stack == NULL)
		return NULL;
	dir_stack = g_hash_table_lookup (context->build_dir_stack, key);
	if (dir_stack == NULL)
		return NULL;

	return dir_stack->data;
}

const gchar *
build_context_get_work_dir (BuildContext* context)
{
	return context->program->work_dir;
}

AnjutaPlugin *
build_context_get_plugin (BuildContext* context)
{
	return context->plugin;
}

static void
build_context_cancel (BuildContext *context)
{
	if (context->launcher != NULL)
	{
		anjuta_launcher_signal (context->launcher, SIGTERM);
	}
}

static gboolean
build_context_destroy_command (BuildContext *context)
{
	if (context->used) return FALSE;
	if (context->program)
	{
		build_program_free (context->program);
		context->program = NULL;
	}

	if (context->launcher)
	{
		g_object_unref (context->launcher);
		context->launcher = NULL;
	}

	if (context->environment)
	{
		g_object_unref (context->environment);
		context->environment = NULL;
	}

	/* Empty context, remove from pool */
	if (context->message_view == NULL)
	{
		ANJUTA_PLUGIN_BASIC_AUTOTOOLS (context->plugin)->contexts_pool =
			g_list_remove (ANJUTA_PLUGIN_BASIC_AUTOTOOLS (context->plugin)->contexts_pool,
							context);
		g_free (context);

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

static gboolean
build_context_destroy_view (BuildContext *context)
{
	BasicAutotoolsPlugin* plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (context->plugin);
	if (context->message_view)
	{
		gtk_widget_destroy (GTK_WIDGET (context->message_view));
		context->message_view = NULL;
	}

	if (context->build_dir_stack)
	{
		g_hash_table_destroy (context->build_dir_stack);
		context->build_dir_stack = NULL;
	}
	if (context->indicators_updated_editors)
	{
		g_hash_table_destroy (context->indicators_updated_editors);
		context->indicators_updated_editors = NULL;
	}

	g_slist_foreach (context->locations, (GFunc) build_indicator_location_free,
					 NULL);
	g_slist_free (context->locations);
	context->locations = NULL;

	/* Empty context, remove from pool */
	if (context->launcher == NULL)
	{
		plugin->contexts_pool =
			g_list_remove (plugin->contexts_pool,
			               context);
		g_free (context);
	}
	else
	{
		/* Kill process */
		anjuta_launcher_signal (context->launcher, SIGKILL);
	}

	return TRUE;
}

void
build_context_destroy (BuildContext *context)
{
	if (build_context_destroy_command (context))
	{
		build_context_destroy_view (context);
	}
}

static void
build_context_reset (BuildContext *context)
{
	/* Reset context */

	ianjuta_message_view_clear (context->message_view, NULL);

	if (context->build_dir_stack)
		g_hash_table_destroy (context->build_dir_stack);
	context->build_dir_stack = NULL;

	g_slist_foreach (context->locations,
					 (GFunc) build_indicator_location_free, NULL);
	g_slist_free (context->locations);
	context->locations = NULL;
}

static void
build_regex_load ()
{
	FILE *fp;

	if (patterns_list)
		return;

	fp = fopen (PACKAGE_DATA_DIR "/build/automake-c.filters", "r");
	if (fp == NULL)
	{
		DEBUG_PRINT ("Failed to load filters: %s",
				   PACKAGE_DATA_DIR "/build/automake-c.filters");
		return;
	}
	while (!feof (fp) && !ferror (fp))
	{
		char buffer[1024];
		gchar **tokens;
		BuildPattern *pattern;

		if (!fgets (buffer, 1024, fp))
			break;
		tokens = g_strsplit (buffer, "|||", 3);

		if (!tokens[0] || !tokens[1])
		{
			DEBUG_PRINT ("Cannot parse regex: %s", buffer);
			g_strfreev (tokens);
			continue;
		}
		pattern = g_new0 (BuildPattern, 1);
		pattern->pattern = g_strdup (tokens[0]);
		pattern->replace = g_strdup (tokens[1]);
		if (tokens[2])
			pattern->options = atoi (tokens[2]);
		g_strfreev (tokens);

		patterns_list = g_list_prepend (patterns_list, pattern);
	}
	fclose (fp);
	patterns_list = g_list_reverse (patterns_list);
}

static void
build_regex_init_message (MessagePattern *patterns)
{
	g_return_if_fail (patterns != NULL);

	if (patterns->regex != NULL)
		return;		/* Already done */

	for (;patterns->pattern != NULL; patterns++)
	{
		/* Untranslated string */
		patterns->regex = g_regex_new(
			   patterns->pattern,
			   0,
			   0,
			   NULL);

		/* Translated string */
		patterns->local_regex = g_regex_new(
			   _(patterns->pattern),
			   0,
			   0,
			   NULL);
	}
}

static void
build_regex_init ()
{
	GList *node;
	GError *error = NULL;

	build_regex_init_message (patterns_make_entering);

	build_regex_init_message (patterns_make_leaving);

	build_regex_load ();
	if (!patterns_list)
		return;

	if (((BuildPattern*)(patterns_list->data))->regex)
		return;

	node = patterns_list;
	while (node)
	{
		BuildPattern *pattern;

		pattern = node->data;
		pattern->regex =
			g_regex_new(
			   pattern->pattern,
			   pattern->options,
			   0,
			   &error);           /* for error message */
		if (error != NULL) {
			DEBUG_PRINT ("GRegex compilation failed: pattern \"%s\": error %s",
						pattern->pattern, error->message);
			g_error_free (error);
		}
		node = g_list_next (node);
	}
}

/* Regex processing */
static gchar*
build_get_summary (const gchar *details, BuildPattern* bp)
{
	gboolean matched;
	GMatchInfo *match_info;
	const gchar *iter;
	GString *ret;
	gchar *final = NULL;

	if (!bp || !bp->regex)
		return NULL;

	matched = g_regex_match(
			  bp->regex,       /* result of g_regex_new() */
			  details,         /* the subject string */
			  0,
			  &match_info);

	if (matched)
	{
		ret = g_string_new ("");
		iter = bp->replace;
		while (*iter != '\0')
		{
			if (*iter == '\\' && isdigit(*(iter + 1)))
			{
				char temp[2] = {0, 0};
				gint start_pos, end_pos;

				temp[0] = *(iter + 1);
				int idx = atoi (temp);

				g_match_info_fetch_pos (match_info, idx, &start_pos, &end_pos);

				ret = g_string_append_len (ret, details + start_pos,
									   end_pos - start_pos);
				iter += 2;
			}
			else
			{
				const gchar *start;
				const gchar *end;

				start = iter;
				iter = g_utf8_next_char (iter);
				end = iter;

				ret = g_string_append_len (ret, start, end - start);
			}
		}

		final = g_string_free (ret, FALSE);
		if (strlen (final) <= 0) {
			g_free (final);
			final = NULL;
		}
	}
	g_match_info_free (match_info);

	return final;
}

static void
on_build_mesg_arrived (AnjutaLauncher *launcher,
					   AnjutaLauncherOutputType output_type,
					   const gchar * mesg, gpointer user_data)
{
	BuildContext *context = (BuildContext*)user_data;
	/* Message view could have been destroyed */
	if (context->message_view)
		ianjuta_message_view_buffer_append (context->message_view, mesg, NULL);
}

static gboolean
parse_error_line (const gchar * line, gchar ** filename, int *lineno)
{
	gint i = 0;
	gint j = 0;
	gint k = 0;
	gchar *dummy;

	while (line[i++] != ':')
	{
		if (i >= strlen (line) || i >= 512 || line[i - 1] == ' ')
		{
			goto down;
		}
	}
	if (isdigit (line[i]))
	{
		j = i;
		while (isdigit (line[i++])) ;
		dummy = g_strndup (&line[j], i - j - 1);
		*lineno = atoi (dummy);
		if (dummy)
			g_free (dummy);
		dummy = g_strndup (line, j - 1);
		*filename = g_strdup (g_strstrip (dummy));
		if (dummy)
			g_free (dummy);
		return TRUE;
	}

      down:
	i = strlen (line);
	do
	{
		i--;
		if (i < 0)
		{
			*filename = NULL;
			*lineno = 0;
			return FALSE;
		}
	} while (isspace (line[i]) == FALSE);
	k = i++;
	while (line[i++] != ':')
	{
		if (i >= strlen (line) || i >= 512 || line[i - 1] == ' ')
		{
			*filename = NULL;
			*lineno = 0;
			return FALSE;
		}
	}
	if (isdigit (line[i]))
	{
		j = i;
		while (isdigit (line[i++])) ;
		dummy = g_strndup (&line[j], i - j - 1);
		*lineno = atoi (dummy);
		if (dummy)
			g_free (dummy);
		dummy = g_strndup (&line[k], j - k - 1);
		*filename = g_strdup (g_strstrip (dummy));
		if (dummy)
			g_free (dummy);
		return TRUE;
	}
	*lineno = 0;
	*filename = NULL;
	return FALSE;
}

static void
on_build_mesg_format (IAnjutaMessageView *view, const gchar *one_line,
					  BuildContext *context)
{
	gchar *dummy_fn, *line;
	gint dummy_int;
	IAnjutaMessageViewType type;
	GList *node;
	gchar *summary = NULL;
	gchar *freeptr = NULL;
	BasicAutotoolsPlugin *p = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (context->plugin);
	gboolean matched;
	GMatchInfo *match_info;
	MessagePattern *pat;

	g_return_if_fail (one_line != NULL);

	/* Check if make enter a new directory */
	matched = FALSE;
	for (pat = patterns_make_entering; pat->pattern != NULL; pat++)
	{
		matched = g_regex_match(
						pat->regex,
				  		one_line,
			  			0,
			  			&match_info);
		if (matched) break;
		g_match_info_free (match_info);
		matched = g_regex_match(
						pat->local_regex,
				  		one_line,
			  			0,
			  			&match_info);
		if (matched) break;
		g_match_info_free (match_info);
	}
	if (matched)
	{
		gchar *dir;
		gchar *summary;

		dir = g_match_info_fetch (match_info, 2);
		dir = context->environment ? ianjuta_environment_get_real_directory(context->environment, dir, NULL)
								: dir;
		build_context_push_dir (context, "default", dir);
		summary = g_strdup_printf(_("Entering: %s"), dir);
		ianjuta_message_view_append (view, IANJUTA_MESSAGE_VIEW_TYPE_NORMAL,
									 summary, one_line, NULL);
		g_free (dir);
		g_free(summary);
		g_match_info_free (match_info);
	}

	/* Check if make leave a directory */
	matched = FALSE;
	for (pat = patterns_make_leaving; pat->pattern != NULL; pat++)
	{
		matched = g_regex_match(
						pat->regex,
				  		one_line,
			  			0,
			  			&match_info);
		if (matched) break;
		g_match_info_free (match_info);
		matched = g_regex_match(
						pat->local_regex,
				  		one_line,
			  			0,
			  			&match_info);
		if (matched) break;
		g_match_info_free (match_info);
	}
	if (matched)
	{
		gchar *dir;
		gchar *summary;

		dir = g_match_info_fetch (match_info, 2);
		dir = context->environment ? ianjuta_environment_get_real_directory(context->environment, dir, NULL)
								: dir;
		build_context_pop_dir (context, "default", dir);
		summary = g_strdup_printf(_("Leaving: %s"), dir);
		ianjuta_message_view_append (view, IANJUTA_MESSAGE_VIEW_TYPE_NORMAL,
									 summary, one_line, NULL);
		g_free (dir);
		g_free(summary);
		g_match_info_free (match_info);
	}

	/* Save freeptr so that we can free the copied string */
	line = freeptr = g_strdup (one_line);

	g_strchug(line); /* Remove leading whitespace */
	if (g_str_has_prefix(line, "if ") == TRUE)
	{
		char *end;
		line = line + 3;

		/* Find the first occurence of ';' (ignoring nesting in quotations) */
		end = strchr(line, ';');
		if (end)
		{
			*end = '\0';
		}
	}

	type = IANJUTA_MESSAGE_VIEW_TYPE_NORMAL;
	if (parse_error_line(line, &dummy_fn, &dummy_int))
	{
		gchar *start_str, *end_str, *mid_str;
		BuildIndicatorLocation *loc;
		IAnjutaIndicableIndicator indicator;

		if ((strstr (line, "warning:") != NULL) ||
		/* The translations should match that of 'gcc' program.
		 * The second string with -old should be used for an older
		 * version of 'gcc' if necessary. If you update one string,
		 * move the first one to translate the -old string and then
		 * replace the first string only. */
			(strstr (line, _("warning:")) != NULL) ||
			(strstr (line, _("warning:-old")) != NULL))
		{
			type = IANJUTA_MESSAGE_VIEW_TYPE_WARNING;
			indicator = IANJUTA_INDICABLE_WARNING;
		}
		else if ((strstr (line, "error:") != NULL) ||
		/* The translations should match that of 'gcc' program.
		 * The second string with -old should be used for an older
		 * version of 'gcc' if necessary. If you update one string,
		 * move the first one to translate the -old string and then
		 * replace the first string only. */
		          (strstr (line, _("error:")) != NULL) ||
			  (strstr (line, _("error:-old")) != NULL))
		{
			type = IANJUTA_MESSAGE_VIEW_TYPE_ERROR;
			indicator = IANJUTA_INDICABLE_CRITICAL;
		}
		else
		{
			type = IANJUTA_MESSAGE_VIEW_TYPE_NORMAL;
			indicator = IANJUTA_INDICABLE_IMPORTANT;
		}

		mid_str = strstr (line, dummy_fn);
		DEBUG_PRINT ("mid_str = %s, line = %s", mid_str, line);
		start_str = g_strndup (line, mid_str - line);
		end_str = line + strlen (start_str) + strlen (dummy_fn);
		DEBUG_PRINT("dummy_fn: %s", dummy_fn);
		if (g_path_is_absolute(dummy_fn))
		{
			mid_str = g_strdup(dummy_fn);
		}
		else
		{
			mid_str = g_build_filename (build_context_get_dir (context, "default"),
										dummy_fn, NULL);
		}
		DEBUG_PRINT ("mid_str: %s", mid_str);

		if (mid_str)
		{
			line = g_strconcat (start_str, mid_str, end_str, NULL);

			/* We sucessfully build an absolute path of the file (mid_str),
			 * so we create an indicator location for it and save it.
			 * Additionally, check of current editor holds this file and if
			 * so, set the indicator.
			 */
			DEBUG_PRINT ("dummy int: %d", dummy_int);

			loc = build_indicator_location_new (mid_str, dummy_int,
												indicator, end_str);
			context->locations = g_slist_prepend (context->locations, loc);

			/* If current editor file is same as indicator file, set indicator */
			if (g_settings_get_boolean (p->settings, PREF_INDICATORS_AUTOMATIC))
			{
				build_indicator_location_set (loc, p->current_editor,
											  p->current_editor_file);
			}
		}
		else
		{
			line = g_strconcat (start_str, dummy_fn, end_str, NULL);
		}
		g_free (start_str);
		g_free (mid_str);
		g_free (dummy_fn);
	}

	node = patterns_list;
	while (node)
	{
		BuildPattern *pattern = node->data;
		summary = build_get_summary (line, pattern);
		if (summary)
			break;
		node = g_list_next (node);
	}

	if (summary)
	{
		ianjuta_message_view_append (view, type, summary, line, NULL);
		g_free (summary);
	}
	else
		ianjuta_message_view_append (view, type, line, "", NULL);
	g_free(freeptr);
}

static void
on_build_mesg_parse (IAnjutaMessageView *view, const gchar *line,
					 BuildContext *context)
{
	gchar *filename;
	gint lineno;
	if (parse_error_line (line, &filename, &lineno))
	{
		IAnjutaDocumentManager *docman;
		GFile* file;

		/* Go to file and line number */
		docman = anjuta_shell_get_interface (context->plugin->shell,
											 IAnjutaDocumentManager,
											 NULL);

		/* Full path is detected from parse_error_line() */
		file = g_file_new_for_path(filename);
		ianjuta_document_manager_goto_file_line_mark(docman, file, lineno, TRUE, NULL);
		g_object_unref (file);
	}
}

static void
on_build_terminated (AnjutaLauncher *launcher,
					 gint child_pid, gint status, gulong time_taken,
					 BuildContext *context)
{
	context->used = FALSE;
	if (context->program->callback != NULL)
	{
		GError *err = NULL;

		if (WIFEXITED (status))
		{
			if (WEXITSTATUS (status) != 0)
			{
				err = g_error_new (ianjuta_builder_error_quark (),
								   WEXITSTATUS (status),
								   _("Command exited with status %d"), WEXITSTATUS (status));
			}
		}
		else if (WIFSIGNALED (status))
		{
			switch (WTERMSIG (status))
			{
				case SIGTERM:
					err = g_error_new (ianjuta_builder_error_quark (),
									 IANJUTA_BUILDER_CANCELED,
									 _("Command canceled by user"));
					break;
				case SIGKILL:
					err = g_error_new (ianjuta_builder_error_quark (),
									 IANJUTA_BUILDER_ABORTED,
									 _("Command aborted by user"));
					break;
				default:
					err = g_error_new (ianjuta_builder_error_quark (),
									   IANJUTA_BUILDER_INTERRUPTED,
									   _("Command terminated with signal %d"), WTERMSIG(status));
					break;
			}
		}
		else
		{
			err = g_error_new_literal (ianjuta_builder_error_quark (),
							   IANJUTA_BUILDER_TERMINATED,
							   _("Command terminated for an unknown reason"));
		}
		build_program_callback (context->program, G_OBJECT (context->plugin), context, err);
		if (err != NULL) g_error_free (err);
	}
	if (context->used)
		return;	/* Another command is run */

	g_signal_handlers_disconnect_by_func (context->launcher,
										  G_CALLBACK (on_build_terminated),
										  context);

	/* Message view could have been destroyed before */
	if (context->message_view)
	{
		IAnjutaMessageManager *mesg_manager;
		gchar *buff1;

		buff1 = g_strdup_printf (_("Total time taken: %lu secs\n"),
								 time_taken);
		mesg_manager = anjuta_shell_get_interface (ANJUTA_PLUGIN (context->plugin)->shell,
									IAnjutaMessageManager, NULL);
		if (status)
		{
			ianjuta_message_view_buffer_append (context->message_view,
									_("Completed unsuccessfully\n"), NULL);
			ianjuta_message_manager_set_view_icon_from_stock (mesg_manager,
									context->message_view,
									GTK_STOCK_STOP, NULL);
		}
		else
		{
			ianjuta_message_view_buffer_append (context->message_view,
									   _("Completed successfully\n"), NULL);
			ianjuta_message_manager_set_view_icon_from_stock (mesg_manager,
									context->message_view,
									GTK_STOCK_APPLY, NULL);
		}
		ianjuta_message_view_buffer_append (context->message_view, buff1, NULL);
		g_free (buff1);

		/* Goto the first error if it exists */
		/* if (anjuta_preferences_get_int (ANJUTA_PREFERENCES (app->preferences),
										"build.option.gotofirst"))
			an_message_manager_next(app->messages);
		*/
	}
	update_project_ui (ANJUTA_PLUGIN_BASIC_AUTOTOOLS (context->plugin));

	build_context_destroy_command (context);
}

static void
on_message_view_destroyed (BuildContext *context, GtkWidget *view)
{
	DEBUG_PRINT ("%s", "Destroying build context");
	context->message_view = NULL;

	build_context_destroy_view (context);
}

static void
build_set_animation (IAnjutaMessageManager* mesg_manager, BuildContext* context)
{
	GtkIconTheme *icon_theme = gtk_icon_theme_get_default ();
	GtkIconInfo *icon_info;
	const gchar *name;
	icon_info = gtk_icon_theme_lookup_icon (icon_theme, "process-working", 16, 0);
	name = gtk_icon_info_get_filename (icon_info);
	if (name != NULL)
	{
		int size = gtk_icon_info_get_base_size (icon_info);
		GdkPixbufSimpleAnim* anim = gdk_pixbuf_simple_anim_new (size, size, 5);
		GdkPixbuf *image = gdk_pixbuf_new_from_file (name, NULL);
		if (image)
		{
			int grid_width = gdk_pixbuf_get_width (image);
			int grid_height = gdk_pixbuf_get_height (image);
			int x,y;
			for (y = 0; y < grid_height; y += size)
			{
				for (x = 0; x < grid_width ; x += size)
				{
					GdkPixbuf *pixbuf = gdk_pixbuf_new_subpixbuf (image, x, y, size,size);
					if (pixbuf)
					{
						gdk_pixbuf_simple_anim_add_frame (anim, pixbuf);
					}
				}
			}
			ianjuta_message_manager_set_view_icon (mesg_manager,
			                                       context->message_view,
			                                       GDK_PIXBUF_ANIMATION (anim), NULL);
			g_object_unref (image);
		}
	}
	gtk_icon_info_free (icon_info);
}


static BuildContext*
build_get_context_with_message(BasicAutotoolsPlugin *plugin, const gchar *dir)
{
	IAnjutaMessageManager *mesg_manager;
	gchar mname[128];
	gchar *subdir;
	BuildContext *context = NULL;
	static gint message_pane_count = 0;

	/* Initialise regex rules */
	build_regex_init();

	subdir = g_path_get_basename (dir);
	/* Translators: the first number is the number of the build attemp,
	   the string is the directory where the build takes place */
	snprintf (mname, 128, _("Build %d: %s"), ++message_pane_count, subdir);
	g_free (subdir);

	/* If we already have MAX_BUILD_PANES build panes, find a free context */
	if (g_list_length (plugin->contexts_pool) >= MAX_BUILD_PANES)
	{
		GList *node;
		node = plugin->contexts_pool;
		while (node)
		{
			BuildContext *c;
			c = node->data;
			if (c->launcher == NULL)
			{
				context = c;
				break;
			}
			node = g_list_next (node);
		}
	}

	mesg_manager = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
											   IAnjutaMessageManager, NULL);
	if (context)
	{
		build_context_reset (context);

		/* It will be re-inserted in right order */
		plugin->contexts_pool = g_list_remove (plugin->contexts_pool, context);
		ianjuta_message_manager_set_view_title (mesg_manager,
												context->message_view,
												mname, NULL);
	}
	else
	{

		/* If no free context found, create one */
		context = g_new0 (BuildContext, 1);
		context->plugin = ANJUTA_PLUGIN(plugin);
		context->indicators_updated_editors =
			g_hash_table_new (g_direct_hash, g_direct_equal);

		context->message_view =
			ianjuta_message_manager_add_view (mesg_manager, mname,
											  ICON_FILE, NULL);

		g_signal_connect (G_OBJECT (context->message_view), "buffer_flushed",
						  G_CALLBACK (on_build_mesg_format), context);
		g_signal_connect (G_OBJECT (context->message_view), "message_clicked",
						  G_CALLBACK (on_build_mesg_parse), context);
		g_object_weak_ref (G_OBJECT (context->message_view),
						   (GWeakNotify)on_message_view_destroyed, context);
	}
	build_set_animation (mesg_manager, context);

	ianjuta_message_manager_set_current_view (mesg_manager,
											  context->message_view, NULL);

	/* Reset indicators in editors */
	if (IANJUTA_IS_INDICABLE (plugin->current_editor))
		ianjuta_indicable_clear (IANJUTA_INDICABLE (plugin->current_editor),
								 NULL);
	if (IANJUTA_IS_MARKABLE (plugin->current_editor))
		ianjuta_markable_delete_all_markers (IANJUTA_MARKABLE (plugin->current_editor),
		                                     IANJUTA_MARKABLE_MESSAGE, NULL);
	g_hash_table_remove_all (context->indicators_updated_editors);


	return context;
}

BuildContext*
build_get_context (BasicAutotoolsPlugin *plugin, const gchar *dir,
		   gboolean with_view)
{
	BuildContext *context = NULL;
	AnjutaPluginManager *plugin_manager;
	gchar *buf;

	if (with_view)
	{
		context = build_get_context_with_message (plugin, dir);
	}
	else
	{
		/* Context without a message view */
		context = g_new0 (BuildContext, 1);
		DEBUG_PRINT ("new context %p", context);
		context->plugin = ANJUTA_PLUGIN(plugin);
	}

	plugin_manager = anjuta_shell_get_plugin_manager (ANJUTA_PLUGIN (plugin)->shell, NULL);

	if (context->environment != NULL)
	{
		g_object_unref (context->environment);
	}
	if (anjuta_plugin_manager_is_active_plugin (plugin_manager, "IAnjutaEnvironment"))
	{
		IAnjutaEnvironment *env = IANJUTA_ENVIRONMENT (anjuta_shell_get_object (ANJUTA_PLUGIN (plugin)->shell,
					"IAnjutaEnvironment", NULL));

		g_object_ref (env);
		context->environment = env;
	}
	else
	{
		context->environment = NULL;
	}

	context->launcher = anjuta_launcher_new ();
	g_signal_connect (G_OBJECT (context->launcher), "child-exited",
					  G_CALLBACK (on_build_terminated), context);
	build_context_push_dir (context, "default", dir);
	buf = g_strconcat (dir, "/", NULL);
	g_chdir (buf);
	g_free (buf);

	plugin->contexts_pool = g_list_append (plugin->contexts_pool, context);

	return context;
}

void
build_set_command_in_context (BuildContext* context, BuildProgram *prog)
{
	context->program = prog;
	context->used = TRUE;
}

gboolean
build_execute_command_in_context (BuildContext* context, GError **err)
{
	GSettings* settings = ANJUTA_PLUGIN_BASIC_AUTOTOOLS(context->plugin)->settings;

	/* Send options to make */
	if (strcmp (build_program_get_basename (context->program), "make") == 0)
	{
		if (g_settings_get_boolean (settings, PREF_PARALLEL_MAKE))
		{
			gchar *arg = g_strdup_printf ("-j%d", g_settings_get_int (settings , PREF_PARALLEL_MAKE_JOB));
			build_program_insert_arg (context->program, 1, arg);
			g_free (arg);
		}
		if (g_settings_get_boolean (settings, PREF_CONTINUE_ON_ERROR))
		{
			build_program_insert_arg (context->program, 1, "-k");
		}
	}

	/* Set a current working directory which can contains symbolic links */
	build_program_add_env (context->program, "PWD", context->program->work_dir);

	if (!g_settings_get_boolean (settings, PREF_TRANSLATE_MESSAGE))
	{
		build_program_add_env (context->program, "LANGUAGE", "C");
	}

	if (!build_program_override (context->program, context->environment))
	{
		build_context_destroy_command (context);
		return FALSE;
	}

	if (context->message_view)
	{
		gchar *command;

		command = g_strjoinv (" ", context->program->argv);
		ianjuta_message_view_buffer_append (context->message_view,
										"Building in directory: ", NULL);
		ianjuta_message_view_buffer_append (context->message_view, context->program->work_dir, NULL);
		ianjuta_message_view_buffer_append (context->message_view, "\n", NULL);
		ianjuta_message_view_buffer_append (context->message_view, command, NULL);
		ianjuta_message_view_buffer_append (context->message_view, "\n", NULL);
		g_free (command);

		anjuta_launcher_execute_v (context->launcher,
		    context->program->work_dir,
		    context->program->argv,
		    context->program->envp,
		    on_build_mesg_arrived, context);
	}
	else
	{
		anjuta_launcher_execute_v (context->launcher,
		    context->program->work_dir,
		    context->program->argv,
		    context->program->envp,
		    NULL, NULL);
	}

	return TRUE;
}

static void
build_delayed_execute_command (IAnjutaFileSavable *savable, GFile *file, gpointer user_data)
{
	BuildContext *context = (BuildContext *)user_data;

	if (savable != NULL)
	{
		g_signal_handlers_disconnect_by_func (savable, G_CALLBACK (build_delayed_execute_command), user_data);
		context->file_saved--;
	}

	if (context->file_saved == 0)
	{
		build_execute_command_in_context (context, NULL);
	}
}

gboolean
build_save_and_execute_command_in_context (BuildContext* context, GError **err)
{
	IAnjutaDocumentManager *docman;

	context->file_saved = 0;

	docman = anjuta_shell_get_interface (context->plugin->shell, IAnjutaDocumentManager, NULL);
	/* No document manager, so no file to save */
	if (docman != NULL)
	{
		GList *doc_list = ianjuta_document_manager_get_doc_widgets (docman, NULL);
		GList *node;

		for (node = g_list_first (doc_list); node != NULL; node = g_list_next (node))
		{
			if (IANJUTA_IS_FILE_SAVABLE (node->data))
			{
				IAnjutaFileSavable* save = IANJUTA_FILE_SAVABLE (node->data);
				if (ianjuta_file_savable_is_dirty (save, NULL))
				{
					context->file_saved++;
					g_signal_connect (G_OBJECT (save), "saved", G_CALLBACK (build_delayed_execute_command), context);
					ianjuta_file_savable_save (save, NULL);
				}
			}
		}
		g_list_free (doc_list);
	}

	build_delayed_execute_command (NULL, NULL, context);

	return TRUE;
}

static void
build_cancel_command (BasicAutotoolsPlugin* bplugin, BuildContext *context,
					  GError **err)
{
	GList *node;

	if (context == NULL) return;

	for (node = g_list_first (bplugin->contexts_pool); node != NULL; node = g_list_next (node))
	{
		if (node->data == context)
		{
			build_context_cancel (context);
			return;
		}
	}

	/* Invalid handle passed */
	g_return_if_reached ();
}


/* User actions
 *---------------------------------------------------------------------------*/

static GFile*
build_module_from_file (BasicAutotoolsPlugin *plugin, GFile *file, gchar **target)
{
	if (plugin->project_root_dir == NULL)
	{
		/* No project, use file without extension */
		gchar *basename;
		GFile *module = NULL;
		GFile *parent;
		gchar *ptr;

		basename = g_file_get_basename (file);
		ptr = strrchr (basename, '.');
		if ((ptr != NULL) && (ptr != basename))
		{

			*ptr = '\0';
		}
		parent = g_file_get_parent (file);
		if (parent != NULL)
		{
			module = g_file_get_child (parent, basename);
			g_object_unref (parent);
		}
		if (target != NULL)
		{
			if (ptr != NULL) *ptr = '.';
			*target = basename;
		}
		else
		{
			g_free (basename);
		}

		return module;
	}
	else
	{
		/* Get corresponding build directory */
		return build_file_from_file (plugin, file, target);
	}
}

/* Main menu */
static void
on_build_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	if (plugin->project_root_dir)
	{
		build_configure_and_build (plugin, build_build_file_or_dir, plugin->project_root_dir, NULL, NULL, NULL);
	}
}

static void
on_install_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	if (plugin->project_root_dir)
	{
		build_configure_and_build (plugin, build_install_dir,  plugin->project_root_dir, NULL, NULL, NULL);
	}
}

static void
on_clean_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	if (plugin->project_root_dir)
	{
		build_clean_dir (plugin, plugin->project_root_dir, NULL);
	}
}

static void
on_configure_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	build_configure_dialog (plugin, NULL, NULL, NULL, NULL, NULL);
}

static void
on_build_tarball (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	build_configure_and_build (plugin, (BuildFunc) build_tarball, NULL, NULL, NULL, NULL);
}

static void
on_build_module (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	GFile *module;

	g_return_if_fail (plugin->current_editor_file != NULL);

	module = build_module_from_file (plugin, plugin->current_editor_file, NULL);
	if (module != NULL)
	{
		build_configure_and_build (plugin, build_build_file_or_dir, module, NULL, NULL, NULL);
		g_object_unref (module);
	}
}

static void
on_install_module (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	g_return_if_fail (plugin->current_editor_file != NULL);

	build_configure_and_build (plugin, build_install_dir,  plugin->current_editor_file, NULL, NULL, NULL);
}

static void
on_clean_module (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	g_return_if_fail (plugin->current_editor_file != NULL);

	build_clean_dir (plugin, plugin->current_editor_file, NULL);
}

static void
on_compile_file (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	g_return_if_fail (plugin->current_editor_file != NULL);

	build_configure_and_build (plugin, (BuildFunc) build_compile_file, plugin->current_editor_file, NULL, NULL, NULL);
}

static void
on_distclean_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	build_distclean (plugin);
}

static void
on_select_configuration (GtkRadioMenuItem *item, gpointer user_data)
{
	if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (item)))
	{
		BasicAutotoolsPlugin *plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (user_data);
		gchar *name;
		GValue *value;
		gchar *uri;

		name = g_object_get_data (G_OBJECT (item), "untranslated_name");

		build_configuration_list_select (plugin->configurations, name);

		value = g_new0 (GValue, 1);
		g_value_init (value, G_TYPE_STRING);

		uri = build_configuration_list_get_build_uri (plugin->configurations, build_configuration_list_get_selected (plugin->configurations));
		g_value_set_string (value, uri);
		g_free (uri);

		anjuta_shell_add_value (ANJUTA_PLUGIN (plugin)->shell, IANJUTA_BUILDER_ROOT_URI, value, NULL);
	}
}

/* File manager context menu */
static void
fm_compile (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	g_return_if_fail (plugin->fm_current_file != NULL);

	build_configure_and_build (plugin, (BuildFunc) build_compile_file, plugin->fm_current_file, NULL, NULL, NULL);
}

static void
fm_build (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	GFile *module;
	g_return_if_fail (plugin->fm_current_file != NULL);

	module = build_module_from_file (plugin, plugin->fm_current_file, NULL);
	if (module != NULL)
	{
		build_configure_and_build (plugin, build_build_file_or_dir, module, NULL, NULL, NULL);
		g_object_unref (module);
	}
}

static void
fm_install (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	g_return_if_fail (plugin->fm_current_file != NULL);

	build_configure_and_build (plugin, build_install_dir,  plugin->fm_current_file, NULL, NULL, NULL);
}

static void
fm_clean (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	g_return_if_fail (plugin->fm_current_file != NULL);

	build_clean_dir (plugin, plugin->fm_current_file, NULL);
}

/* Project manager context menu */
static void
pm_compile (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	g_return_if_fail (plugin->pm_current_file != NULL);

	build_configure_and_build (plugin, (BuildFunc) build_compile_file, plugin->pm_current_file, NULL, NULL, NULL);
}

static void
pm_build (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	GFile *module;

	g_return_if_fail (plugin->pm_current_file != NULL);

	module = build_module_from_file (plugin, plugin->pm_current_file, NULL);
	if (module != NULL)
	{
		build_configure_and_build (plugin, build_build_file_or_dir, module, NULL, NULL, NULL);
		g_object_unref (module);
	}
}

static void
pm_install (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	g_return_if_fail (plugin->pm_current_file != NULL);

	build_configure_and_build (plugin, build_install_dir,  plugin->pm_current_file, NULL, NULL, NULL);
}

static void
pm_clean (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	g_return_if_fail (plugin->pm_current_file != NULL);

	build_clean_dir (plugin, plugin->pm_current_file, NULL);
}

/* Message view context menu */
static void
mv_cancel (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	IAnjutaMessageManager *msgman;

	msgman = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
										 IAnjutaMessageManager,
										 NULL);

	if (msgman != NULL)
	{
		IAnjutaMessageView *view;

		view = ianjuta_message_manager_get_current_view (msgman, NULL);
		if (view != NULL)
		{
			GList *node;

			for (node = g_list_first (plugin->contexts_pool); node != NULL; node = g_list_next (node))
			{
				BuildContext *context;

				context = (BuildContext *)node->data;
				if (context->message_view == view)
				{
					build_context_cancel (context);
					return;
				}
			}
		}
	}
}

static GtkActionEntry build_actions[] =
{
	{
		"ActionMenuBuild", NULL,
		N_("_Build"), NULL, NULL, NULL
	},
	{
		"ActionBuildBuildProject", GTK_STOCK_CONVERT,
		N_("_Build Project"), "<shift>F7",
		N_("Build whole project"),
		G_CALLBACK (on_build_project)
	},
	{
		"ActionBuildInstallProject", NULL,
		N_("_Install Project"), NULL,
		N_("Install whole project"),
		G_CALLBACK (on_install_project)
	},
	{
		"ActionBuildCleanProject", NULL,
		N_("_Clean Project"), NULL,
		N_("Clean whole project"),
		G_CALLBACK (on_clean_project)
	},
	{
		"ActionBuildConfigure", NULL,
		N_("C_onfigure Project…"), NULL,
		N_("Configure project"),
		G_CALLBACK (on_configure_project)
	},
	{
		"ActionBuildDistribution", NULL,
		N_("Build _Tarball"), NULL,
		N_("Build project tarball distribution"),
		G_CALLBACK (on_build_tarball)
	},
	{
		"ActionBuildBuildModule", ANJUTA_STOCK_BUILD,
		N_("_Build Module"), "F7",
		N_("Build module associated with current file"),
		G_CALLBACK (on_build_module)
	},
	{
		"ActionBuildInstallModule", NULL,
		N_("_Install Module"), NULL,
		N_("Install module associated with current file"),
		G_CALLBACK (on_install_module)
	},
	{
		"ActionBuildCleanModule", NULL,
		N_("_Clean Module"), NULL,
		N_("Clean module associated with current file"),
		G_CALLBACK (on_clean_module)
	},
	{
		"ActionBuildCompileFile", NULL,
		N_("Co_mpile File"), "F9",
		N_("Compile current editor file"),
		G_CALLBACK (on_compile_file)
	},
	{
		"ActionBuildSelectConfiguration", NULL,
		N_("Select Configuration"), NULL,
		N_("Select current configuration"),
		NULL
	},
	{
		"ActionBuildRemoveConfiguration", NULL,
		N_("Remove Configuration"), NULL,
		N_("Clean project (distclean) and remove configuration directory if possible"),
		G_CALLBACK (on_distclean_project)
	}
};

static GtkActionEntry build_popup_actions[] =
{
	{
		"ActionPopupBuild", NULL,
		N_("_Build"), NULL, NULL, NULL
	},
	{
		"ActionPopupBuildCompile", GTK_STOCK_CONVERT,
		N_("_Compile"), NULL,
		N_("Compile file"),
		G_CALLBACK (fm_compile)
	},
	{
		"ActionPopupBuildBuild", GTK_STOCK_EXECUTE,
		N_("_Build"), NULL,
		N_("Build module"),
		G_CALLBACK (fm_build)
	},
	{
		"ActionPopupBuildInstall", NULL,
		N_("_Install"), NULL,
		N_("Install module"),
		G_CALLBACK (fm_install)
	},
	{
		"ActionPopupBuildClean", NULL,
		N_("_Clean"), NULL,
		N_("Clean module"),
		G_CALLBACK (fm_clean)
	},
	{
		"ActionPopupPMBuild", NULL,
		N_("_Build"), NULL, NULL, NULL
	},
	{
		"ActionPopupPMBuildCompile", GTK_STOCK_CONVERT,
		N_("_Compile"), NULL,
		N_("Compile file"),
		G_CALLBACK (pm_compile)
	},
	{
		"ActionPopupPMBuildBuild", GTK_STOCK_EXECUTE,
		N_("_Build"), NULL,
		N_("Build module"),
		G_CALLBACK (pm_build)
	},
	{
		"ActionPopupPMBuildInstall", NULL,
		N_("_Install"), NULL,
		N_("Install module"),
		G_CALLBACK (pm_install)
	},
	{
		"ActionPopupPMBuildClean", NULL,
		N_("_Clean"), NULL,
		N_("Clean module"),
		G_CALLBACK (pm_clean)
	},
	{
		"ActionPopupMVBuildCancel", NULL,
		N_("_Cancel command"), NULL,
		N_("Cancel build command"),
		G_CALLBACK (mv_cancel)
	}
};

static void
update_module_ui (BasicAutotoolsPlugin *bb_plugin)
{
	AnjutaUI *ui;
	GtkAction *action;
	gchar *filename= NULL;
	gchar *module = NULL;
	gchar *label;
	gboolean has_file = FALSE;
	gboolean has_makefile= FALSE;
	gboolean has_project = TRUE;
	gboolean has_object = FALSE;

	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (bb_plugin)->shell, NULL);

	DEBUG_PRINT ("%s", "Updating module UI");

	has_file = bb_plugin->current_editor_file != NULL;
	has_project = bb_plugin->project_root_dir != NULL;
	if (has_file)
	{
		GFile *mod;
		gchar *target;
		gchar *module_name;


		mod = build_module_from_file (bb_plugin, bb_plugin->current_editor_file, &target);

		if (has_project && !g_file_equal (mod, bb_plugin->project_root_dir) && !g_file_equal (mod, bb_plugin->project_build_dir))
		{
			module_name = g_file_get_basename (mod);
			module = escape_label (module_name);
			g_free (module_name);
		}
		if (target != NULL)
		{
			filename = escape_label (target);
			g_free (target);
		}
		has_makefile = directory_has_makefile (mod) || directory_has_makefile_am (bb_plugin, mod);
		g_object_unref (mod);

		mod = build_object_from_file (bb_plugin, bb_plugin->current_editor_file);
		if (mod != NULL)
		{
			has_object = TRUE;
			g_object_unref (mod);
		}
	}

	action = anjuta_ui_get_action (ui, "ActionGroupBuild",
								   "ActionBuildBuildModule");
	label = g_strdup_printf (module ? _("_Build (%s)") : _("_Build"), module);
	g_object_set (G_OBJECT (action), "sensitive", has_file && (has_makefile || !has_project),
					  "label", label, NULL);
	g_free (label);

	action = anjuta_ui_get_action (ui, "ActionGroupBuild",
								   "ActionBuildInstallModule");
	label = g_strdup_printf (module ? _("_Install (%s)") : _("_Install"), module);
	g_object_set (G_OBJECT (action), "sensitive", has_makefile,
	              "visible", has_project, "label", label, NULL);
	g_free (label);

	action = anjuta_ui_get_action (ui, "ActionGroupBuild",
								   "ActionBuildCleanModule");
	label = g_strdup_printf (module ? _("_Clean (%s)") : _("_Clean"), module);
	g_object_set (G_OBJECT (action), "sensitive", has_makefile,
	              "visible", has_project, "label", label, NULL);
	g_free (label);


	action = anjuta_ui_get_action (ui, "ActionGroupBuild",
								   "ActionBuildCompileFile");
	label = g_strdup_printf (filename ? _("Co_mpile (%s)") : _("Co_mpile"), filename);
	g_object_set (G_OBJECT (action), "sensitive", has_object,
					  "label", label, NULL);
	g_free (label);

	g_free (module);
	g_free (filename);
}

static void
update_fm_module_ui (BasicAutotoolsPlugin *bb_plugin)
{
	AnjutaUI *ui;
	GtkAction *action;
	gboolean has_file = FALSE;
	gboolean has_makefile= FALSE;
	gboolean has_project = TRUE;
	gboolean has_object = FALSE;
	gboolean is_directory = FALSE;

	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (bb_plugin)->shell, NULL);

	has_file = bb_plugin->fm_current_file != NULL;
	if (has_file)
	{
		GFile *mod;

		mod = build_module_from_file (bb_plugin, bb_plugin->fm_current_file, NULL);
		if (mod != NULL)
		{
			has_makefile = directory_has_makefile (mod) || directory_has_makefile_am (bb_plugin, mod);
			g_object_unref (mod);
		}

		is_directory = g_file_query_file_type (bb_plugin->fm_current_file, 0, NULL) == G_FILE_TYPE_DIRECTORY;
		if (!is_directory)
		{
			mod = build_object_from_file (bb_plugin, bb_plugin->fm_current_file);
			if (mod != NULL)
			{
				has_object = TRUE;
				g_object_unref (mod);
			}
		}
	}
	has_project = bb_plugin->project_root_dir != NULL;

	action = anjuta_ui_get_action (ui, "ActionGroupPopupBuild",
									   "ActionPopupBuild");
	g_object_set (G_OBJECT (action), "visible", has_file && (has_makefile || (!is_directory && !has_project)), NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupPopupBuild",
								   "ActionPopupBuildCompile");
	g_object_set (G_OBJECT (action), "sensitive", has_object, "visible", !is_directory, NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupPopupBuild",
								   "ActionPopupBuildBuild");
	g_object_set (G_OBJECT (action), "sensitive", has_file && (has_makefile || (!is_directory && !has_project)), NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupPopupBuild",
								   "ActionPopupBuildInstall");
	g_object_set (G_OBJECT (action), "sensitive", has_makefile, "visible", has_project, NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupPopupBuild",
								   "ActionPopupBuildClean");
	g_object_set (G_OBJECT (action), "sensitive", has_makefile, "visible", has_project, NULL);
}

static void
update_pm_module_ui (BasicAutotoolsPlugin *bb_plugin)
{
	AnjutaUI *ui;
	GtkAction *action;
	gboolean has_file = FALSE;
	gboolean has_makefile= FALSE;
	gboolean has_project = TRUE;
	gboolean has_object = FALSE;
	gboolean is_directory = FALSE;

	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (bb_plugin)->shell, NULL);

	has_file = bb_plugin->pm_current_file != NULL;
	if (has_file)
	{
		GFile *mod;

		mod = build_module_from_file (bb_plugin, bb_plugin->pm_current_file, NULL);
		if (mod != NULL)
		{
			has_makefile = directory_has_makefile (mod) || directory_has_makefile_am (bb_plugin, mod);
			g_object_unref (mod);
		}

		is_directory = g_file_query_file_type (bb_plugin->pm_current_file, 0, NULL) == G_FILE_TYPE_DIRECTORY;
		if (!is_directory)
		{
			mod = build_object_from_file (bb_plugin, bb_plugin->pm_current_file);
			if (mod != NULL)
			{
				has_object = TRUE;
				g_object_unref (mod);
			}
		}
	}
	has_project = bb_plugin->project_root_dir != NULL;

	action = anjuta_ui_get_action (ui, "ActionGroupPopupBuild",
									   "ActionPopupPMBuild");
	g_object_set (G_OBJECT (action), "visible", has_file && (has_makefile || !has_project), NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupPopupBuild",
								   "ActionPopupPMBuildCompile");
	g_object_set (G_OBJECT (action), "sensitive", has_object, "visible", !is_directory, NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupPopupBuild",
								   "ActionPopupPMBuildBuild");
	g_object_set (G_OBJECT (action), "sensitive", has_file && (has_makefile || !has_project), NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupPopupBuild",
								   "ActionPopupPMBuildInstall");
	g_object_set (G_OBJECT (action), "sensitive", has_makefile, "visible", has_project, NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupPopupBuild",
								   "ActionPopupPMBuildClean");
	g_object_set (G_OBJECT (action), "sensitive", has_makefile, "visible", has_project, NULL);
}


static void
update_project_ui (BasicAutotoolsPlugin *bb_plugin)
{
	AnjutaUI *ui;
	GtkAction *action;
	gboolean has_makefile;
	gboolean has_project;

	DEBUG_PRINT ("%s", "Updating project UI");

	has_project = bb_plugin->project_root_dir != NULL;
	has_makefile = has_project && (directory_has_makefile (bb_plugin->project_build_dir) || directory_has_makefile_am (bb_plugin, bb_plugin->project_build_dir));

	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (bb_plugin)->shell, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupBuild",
								   "ActionBuildBuildProject");
	g_object_set (G_OBJECT (action), "sensitive", has_project, "visible", has_project, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupBuild",
								   "ActionBuildInstallProject");
	g_object_set (G_OBJECT (action), "sensitive", has_project, "visible", has_project, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupBuild",
								   "ActionBuildCleanProject");
	g_object_set (G_OBJECT (action), "sensitive", has_makefile, "visible", has_project, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupBuild",
								   "ActionBuildDistribution");
	g_object_set (G_OBJECT (action), "sensitive", has_project, "visible", has_project, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupBuild",
								   "ActionBuildConfigure");
	g_object_set (G_OBJECT (action), "sensitive", has_project, "visible", has_project, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupBuild",
								   "ActionBuildSelectConfiguration");
	g_object_set (G_OBJECT (action), "sensitive", has_project, "visible", has_project, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupBuild",
								   "ActionBuildRemoveConfiguration");
	g_object_set (G_OBJECT (action), "sensitive", has_makefile, "visible", has_project, NULL);

	update_module_ui (bb_plugin);
}

void
build_update_configuration_menu (BasicAutotoolsPlugin *plugin)
{
	GtkWidget *submenu = NULL;
	BuildConfiguration *cfg;
	BuildConfiguration *selected;
	GSList *group = NULL;

	submenu = gtk_menu_new ();
	selected = build_configuration_list_get_selected (plugin->configurations);
	for (cfg = build_configuration_list_get_first (plugin->configurations); cfg != NULL; cfg = build_configuration_next (cfg))
	{
		GtkWidget *item;

		item = gtk_radio_menu_item_new_with_mnemonic (group, build_configuration_get_translated_name (cfg));
		group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
		if (cfg == selected)
		{
			gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
		}
		g_object_set_data_full (G_OBJECT (item), "untranslated_name", g_strdup (build_configuration_get_name (cfg)), g_free);
		g_signal_connect (G_OBJECT (item), "toggled", G_CALLBACK (on_select_configuration), plugin);
		gtk_menu_shell_append (GTK_MENU_SHELL (submenu), item);
	}
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (plugin->configuration_menu), submenu);
	gtk_widget_show_all (submenu);
}

static void
on_session_save (AnjutaShell *shell, AnjutaSessionPhase phase,
				 AnjutaSession *session, BasicAutotoolsPlugin *plugin)
{
	GList *configurations;
	BuildConfiguration *cfg;
	const gchar *name;

	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;

	configurations = build_configuration_list_to_string_list (plugin->configurations);
	anjuta_session_set_string_list (session, "Build",
									"Configuration list",
									configurations);
	g_list_foreach (configurations, (GFunc)g_free, NULL);
	g_list_free (configurations);

	cfg = build_configuration_list_get_selected (plugin->configurations);
	if (cfg != NULL)
	{
		name = build_configuration_get_name (cfg);
		anjuta_session_set_string (session, "Build", "Selected Configuration", name);
	}
	for (cfg = build_configuration_list_get_first (plugin->configurations); cfg != NULL; cfg = build_configuration_next (cfg))
	{
		gchar *key;
		GList *list;

		key = g_strconcat("BuildArgs/", build_configuration_get_name (cfg), NULL);
		anjuta_session_set_string (session, "Build",
			       key,
			       build_configuration_get_args(cfg));
		g_free (key);

		list = build_configuration_get_variables (cfg);
		if (list != NULL)
		{
			key = g_strconcat("BuildEnv/", build_configuration_get_name (cfg), NULL);
			anjuta_session_set_string_list (session, "Build",
				       key,
			    	   list);
			g_free (key);
		}
	}
}

static void
on_session_load (AnjutaShell *shell, AnjutaSessionPhase phase,
				 AnjutaSession *session, BasicAutotoolsPlugin *plugin)
{
	GList *configurations;
	gchar *selected;
	BuildConfiguration *cfg;

	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;

	configurations = anjuta_session_get_string_list (session, "Build",
											  "Configuration list");

	build_configuration_list_from_string_list (plugin->configurations, configurations);
	g_list_foreach (configurations, (GFunc)g_free, NULL);
	g_list_free (configurations);

	selected = anjuta_session_get_string (session, "Build", "Selected Configuration");
	build_configuration_list_select (plugin->configurations, selected);
	g_free (selected);

	for (cfg = build_configuration_list_get_first (plugin->configurations); cfg != NULL; cfg = build_configuration_next (cfg))
	{
		gchar *key;
		gchar *args;
		GList *env;

		key = g_strconcat("BuildArgs/", build_configuration_get_name (cfg), NULL);
		args = anjuta_session_get_string (session, "Build", key);
		g_free (key);
		if (args != NULL)
		{
			build_configuration_set_args (cfg, args);
			g_free (args);
		}

		key = g_strconcat("BuildEnv/", build_configuration_get_name (cfg), NULL);
		env = anjuta_session_get_string_list (session, "Build",	key);
		g_free (key);
		if (env != NULL)
		{
			GList *item;

			/* New variables are added at the beginning of the list */
			for (item = env; item != NULL; item = g_list_next (item))
			{
				build_configuration_set_variable (cfg, (gchar *)item->data);
				g_free (item->data);
			}
			g_list_free (env);
		}

	}

	build_project_configured (G_OBJECT (plugin), NULL, NULL, NULL);
}

static void
value_added_fm_current_file (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	BasicAutotoolsPlugin *ba_plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (plugin);

	if (ba_plugin->fm_current_file)
		g_object_unref (ba_plugin->fm_current_file);
	ba_plugin->fm_current_file = g_value_dup_object (value);

	update_fm_module_ui (ba_plugin);
}

static void
value_removed_fm_current_file (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	BasicAutotoolsPlugin *ba_plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (plugin);

	if (ba_plugin->fm_current_file)
		g_object_unref (ba_plugin->fm_current_file);
	ba_plugin->fm_current_file = NULL;

	update_fm_module_ui (ba_plugin);
}

static void
value_added_pm_current_uri (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	BasicAutotoolsPlugin *ba_plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (plugin);
	const gchar *uri;

	uri = g_value_get_string (value);
	if (ba_plugin->pm_current_file)
		g_object_unref (ba_plugin->pm_current_file);
	ba_plugin->pm_current_file = g_file_new_for_uri (uri);

	update_pm_module_ui (ba_plugin);
}

static void
value_removed_pm_current_uri (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	BasicAutotoolsPlugin *ba_plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (plugin);

	if (ba_plugin->pm_current_file)
		g_object_unref (ba_plugin->pm_current_file);
	ba_plugin->pm_current_file = NULL;

	update_pm_module_ui (ba_plugin);
}

static void
value_added_project_root_uri (AnjutaPlugin *plugin, const gchar *name,
							  const GValue *value, gpointer user_data)
{
	BasicAutotoolsPlugin *bb_plugin;
	const gchar *root_uri;

	bb_plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (plugin);

	g_free (bb_plugin->project_root_dir);
	bb_plugin->project_root_dir = NULL;

	root_uri = g_value_get_string (value);
	if (root_uri)
	{
		bb_plugin->project_root_dir = g_file_new_for_uri (root_uri);
	}

	build_configuration_list_set_project_uri (bb_plugin->configurations, root_uri);

	/* Export project build uri */
	anjuta_shell_add_value (ANJUTA_PLUGIN(plugin)->shell,
							IANJUTA_BUILDER_ROOT_URI,
							value, NULL);

	update_project_ui (bb_plugin);
}

static void
value_removed_project_root_uri (AnjutaPlugin *plugin, const gchar *name,
								gpointer user_data)
{
	BasicAutotoolsPlugin *bb_plugin;

	bb_plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (plugin);

	if (bb_plugin->project_root_dir != NULL) g_object_unref (bb_plugin->project_root_dir);
	if (bb_plugin->project_build_dir != NULL) g_object_unref (bb_plugin->project_build_dir);
	g_free (bb_plugin->program_args);

	bb_plugin->run_in_terminal = TRUE;
	bb_plugin->program_args = NULL;
	bb_plugin->project_build_dir = NULL;
	bb_plugin->project_root_dir = NULL;

	build_configuration_list_set_project_uri (bb_plugin->configurations, NULL);

	/* Export project build uri */
	anjuta_shell_remove_value (ANJUTA_PLUGIN (plugin)->shell,
							   IANJUTA_BUILDER_ROOT_URI, NULL);

	update_project_ui (bb_plugin);
}

static void
value_added_project_build_uri (AnjutaPlugin *plugin, const gchar *name,
							  const GValue *value, gpointer user_data)
{
	BasicAutotoolsPlugin *bb_plugin;
	const gchar *build_uri;

	bb_plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (plugin);

	if (bb_plugin->project_build_dir != NULL) g_object_unref (bb_plugin->project_build_dir);
	bb_plugin->project_build_dir = NULL;

	build_uri = g_value_get_string (value);
	if (build_uri)
	{
		bb_plugin->project_build_dir = g_file_new_for_uri (build_uri);
	}
	update_project_ui (bb_plugin);
}

static gint
on_update_indicators_idle (gpointer data)
{
	BasicAutotoolsPlugin *ba_plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (data);
	IAnjutaEditor *editor = ba_plugin->current_editor;

	/* If indicators are not yet updated in the editor, do it */
	if (ba_plugin->current_editor_file &&
		IANJUTA_IS_INDICABLE (editor) &&
		g_settings_get_boolean (ba_plugin->settings,
		                        PREF_INDICATORS_AUTOMATIC))
	{
		GList *node;
		node = ba_plugin->contexts_pool;
		while (node)
		{
			BuildContext *context = node->data;
			if (g_hash_table_lookup (context->indicators_updated_editors,
									 editor) == NULL)
			{
				GSList *loc_node;
				ianjuta_indicable_clear (IANJUTA_INDICABLE (editor), NULL);


				loc_node = context->locations;
				while (loc_node)
				{
					build_indicator_location_set ((BuildIndicatorLocation*) loc_node->data,
					IANJUTA_EDITOR (editor), ba_plugin->current_editor_file);
					loc_node = g_slist_next (loc_node);
				}
				g_hash_table_insert (context->indicators_updated_editors,
									 editor, editor);
			}
			node = g_list_next (node);
		}
	}
	return FALSE;
}

static void
on_editor_destroy (IAnjutaEditor *editor, BasicAutotoolsPlugin *ba_plugin)
{
	g_hash_table_remove (ba_plugin->editors_created, editor);
}

static void
on_editor_changed (IAnjutaEditor *editor, IAnjutaIterable *position,
				   gboolean added, gint length, gint lines, const gchar *text,
				   BasicAutotoolsPlugin *ba_plugin)
{
	gint line;
	IAnjutaIterable *begin_pos, *end_pos;
	if (g_hash_table_lookup (ba_plugin->editors_created,
							 editor) == NULL)
		return;
	line  = ianjuta_editor_get_line_from_position (editor, position, NULL);
	begin_pos = ianjuta_editor_get_line_begin_position (editor, line, NULL);
	end_pos = ianjuta_editor_get_line_end_position (editor, line, NULL);
	if (IANJUTA_IS_INDICABLE (editor))
	{
		DEBUG_PRINT ("Clearing indicator on line %d", line);
		ianjuta_indicable_set (IANJUTA_INDICABLE (editor), begin_pos,
							   end_pos, IANJUTA_INDICABLE_NONE, NULL);
	}
	DEBUG_PRINT ("Editor changed: line number = %d, added = %d,"
				 " text length = %d, number of lines = %d",
				 line, added, length, lines);
	g_object_unref (begin_pos);
	g_object_unref (end_pos);
}

static void
value_added_current_editor (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	AnjutaUI *ui;
	GObject *editor;

	editor = g_value_get_object (value);

	if (!IANJUTA_IS_EDITOR(editor))
		return;

	BasicAutotoolsPlugin *ba_plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (plugin);
	ui = anjuta_shell_get_ui (plugin->shell, NULL);

	ba_plugin->current_editor = IANJUTA_EDITOR (editor);

	if (g_hash_table_lookup (ba_plugin->editors_created,
							 ba_plugin->current_editor) == NULL)
	{
		g_hash_table_insert (ba_plugin->editors_created,
							 ba_plugin->current_editor,
							 ba_plugin->current_editor);
		g_signal_connect (ba_plugin->current_editor, "destroy",
						  G_CALLBACK (on_editor_destroy), plugin);
		g_signal_connect (ba_plugin->current_editor, "changed",
						  G_CALLBACK (on_editor_changed), plugin);
	}

	if (ba_plugin->current_editor_file != NULL) g_object_unref (ba_plugin->current_editor_file);
	ba_plugin->current_editor_file = ianjuta_file_get_file (IANJUTA_FILE (editor), NULL);
	update_module_ui (ba_plugin);

	g_idle_add (on_update_indicators_idle, plugin);
}

static void
value_removed_current_editor (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	BasicAutotoolsPlugin *ba_plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (plugin);
#if 0
	if (ba_plugin->indicators_updated_editors &&
		g_hash_table_lookup (ba_plugin->indicators_updated_editors,
							 ba_plugin->current_editor))
	{
		g_hash_table_remove (ba_plugin->indicators_updated_editors,
							 ba_plugin->current_editor);
	}
#endif
	if (ba_plugin->current_editor_file)
		g_object_unref (ba_plugin->current_editor_file);
	ba_plugin->current_editor_file = NULL;
	ba_plugin->current_editor = NULL;

	update_module_ui (ba_plugin);
}

static void
register_stock_icons (AnjutaPlugin *plugin)
{
	static gboolean registered = FALSE;

	if (registered)
		return;
	registered = TRUE;

	BEGIN_REGISTER_ICON (plugin);
	REGISTER_ICON_FULL (ANJUTA_PIXMAP_BUILD, ANJUTA_STOCK_BUILD);
	END_REGISTER_ICON;
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	static gboolean initialized = FALSE;
	BasicAutotoolsPlugin *ba_plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (plugin);

	if (!initialized)
	{
		register_stock_icons (plugin);
	}
	ui = anjuta_shell_get_ui (plugin->shell, NULL);

	g_signal_connect (plugin->shell, "save-session",
					  G_CALLBACK (on_session_save),
					  plugin);
	g_signal_connect (plugin->shell, "load-session",
					  G_CALLBACK (on_session_load),
					  plugin);

	/* Add action group */
	ba_plugin->build_action_group =
		anjuta_ui_add_action_group_entries (ui,
											"ActionGroupBuild",
											_("Build commands"),
											build_actions,
											sizeof(build_actions)/sizeof(GtkActionEntry),
											GETTEXT_PACKAGE, TRUE, plugin);
	ba_plugin->build_popup_action_group =
		anjuta_ui_add_action_group_entries (ui,
			       	"ActionGroupPopupBuild",
				/* Translators: This is a group of build
				 * commands which appears in pop up menus */
			       	_("Build popup commands"),
			       	build_popup_actions,
											sizeof(build_popup_actions)/sizeof(GtkActionEntry),
											GETTEXT_PACKAGE, FALSE, plugin);
	/* Add UI */
	ba_plugin->build_merge_id = anjuta_ui_merge (ui, UI_FILE);

	ba_plugin->configuration_menu = gtk_ui_manager_get_widget (GTK_UI_MANAGER(ui),
                                        "/MenuMain/PlaceHolderBuildMenus/MenuBuild/SelectConfiguration");
	update_project_ui (ba_plugin);

	/* Add watches */
	ba_plugin->fm_watch_id =
		anjuta_plugin_add_watch (plugin, IANJUTA_FILE_MANAGER_SELECTED_FILE,
								 value_added_fm_current_file,
								 value_removed_fm_current_file, NULL);
	ba_plugin->pm_watch_id =
		anjuta_plugin_add_watch (plugin, IANJUTA_PROJECT_MANAGER_CURRENT_URI,
								 value_added_pm_current_uri,
								 value_removed_pm_current_uri, NULL);
	ba_plugin->project_root_watch_id =
		anjuta_plugin_add_watch (plugin, IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI,
								 value_added_project_root_uri,
								 value_removed_project_root_uri, NULL);
	ba_plugin->project_build_watch_id =
		anjuta_plugin_add_watch (plugin, IANJUTA_BUILDER_ROOT_URI,
								 value_added_project_build_uri,
								 NULL, NULL);
	ba_plugin->editor_watch_id =
		anjuta_plugin_add_watch (plugin,  IANJUTA_DOCUMENT_MANAGER_CURRENT_DOCUMENT,
								 value_added_current_editor,
								 value_removed_current_editor, NULL);

	initialized = TRUE;
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	BasicAutotoolsPlugin *ba_plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (plugin);

	ui = anjuta_shell_get_ui (plugin->shell, NULL);

	g_signal_handlers_disconnect_by_func (plugin->shell,
										  G_CALLBACK (on_session_save),
										  plugin);
	g_signal_handlers_disconnect_by_func (plugin->shell,
										  G_CALLBACK (on_session_load),
										  plugin);

	/* Remove watches */
	anjuta_plugin_remove_watch (plugin, ba_plugin->fm_watch_id, TRUE);
	anjuta_plugin_remove_watch (plugin, ba_plugin->pm_watch_id, TRUE);
	anjuta_plugin_remove_watch (plugin, ba_plugin->project_root_watch_id, TRUE);
	anjuta_plugin_remove_watch (plugin, ba_plugin->project_build_watch_id, TRUE);
	anjuta_plugin_remove_watch (plugin, ba_plugin->editor_watch_id, TRUE);

	/* Remove UI */
	anjuta_ui_unmerge (ui, ba_plugin->build_merge_id);

	/* Remove action group */
	anjuta_ui_remove_action_group (ui, ba_plugin->build_action_group);
	anjuta_ui_remove_action_group (ui, ba_plugin->build_popup_action_group);
	return TRUE;
}

static void
dispose (GObject *obj)
{
	BasicAutotoolsPlugin *ba_plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (obj);

	g_object_unref (ba_plugin->settings);

	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
finalize (GObject *obj)
{
	gint i;
	BasicAutotoolsPlugin *ba_plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (obj);

	for (i = 0; i < IANJUTA_BUILDABLE_N_COMMANDS; i++)
	{
		g_free (ba_plugin->commands[i]);
		ba_plugin->commands[i] = NULL;
	}

	if (ba_plugin->fm_current_file != NULL) g_object_unref (ba_plugin->fm_current_file);
	if (ba_plugin->pm_current_file != NULL) g_object_unref (ba_plugin->pm_current_file);
	if (ba_plugin->current_editor_file != NULL) g_object_unref (ba_plugin->current_editor_file);
	if (ba_plugin->project_root_dir != NULL) g_object_unref (ba_plugin->project_root_dir);
	if (ba_plugin->project_build_dir != NULL) g_object_unref (ba_plugin->project_build_dir);
	g_free (ba_plugin->program_args);
	build_configuration_list_free (ba_plugin->configurations);

	ba_plugin->fm_current_file = NULL;
	ba_plugin->pm_current_file = NULL;
	ba_plugin->current_editor_file = NULL;
	ba_plugin->project_root_dir = NULL;
	ba_plugin->project_build_dir = NULL;
	ba_plugin->program_args = NULL;
	ba_plugin->configurations = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
basic_autotools_plugin_instance_init (GObject *obj)
{
	gint i;
	BasicAutotoolsPlugin *ba_plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (obj);

	for (i = 0; i < IANJUTA_BUILDABLE_N_COMMANDS; i++)
		ba_plugin->commands[i] = NULL;

	ba_plugin->fm_current_file = NULL;
	ba_plugin->pm_current_file = NULL;
	ba_plugin->current_editor_file = NULL;
	ba_plugin->project_root_dir = NULL;
	ba_plugin->project_build_dir = NULL;
	ba_plugin->current_editor = NULL;
	ba_plugin->contexts_pool = NULL;
	ba_plugin->configurations = build_configuration_list_new ();
	ba_plugin->program_args = NULL;
	ba_plugin->run_in_terminal = TRUE;
	ba_plugin->last_exec_uri = NULL;
	ba_plugin->editors_created = g_hash_table_new (g_direct_hash,
												   g_direct_equal);
	ba_plugin->settings = g_settings_new (PREF_SCHEMA);
}

static void
basic_autotools_plugin_class_init (GObjectClass *klass)
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
	klass->finalize = finalize;
}


/* IAnjutaBuildable implementation
 *---------------------------------------------------------------------------*/

static void
ibuildable_set_command (IAnjutaBuildable *manager,
						IAnjutaBuildableCommand command_id,
						const gchar *command_override, GError **err)
{
	BasicAutotoolsPlugin *plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (manager);
	if (plugin->commands[command_id])
		g_free (plugin->commands[command_id]);
	plugin->commands[command_id] = g_strdup (command_override);
}

static void
ibuildable_reset_commands (IAnjutaBuildable *manager, GError **err)
{
	gint i;
	BasicAutotoolsPlugin *ba_plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (manager);

	for (i = 0; i < IANJUTA_BUILDABLE_N_COMMANDS; i++)
	{
		g_free (ba_plugin->commands[i]);
		ba_plugin->commands[i] = NULL;
	}
}

static const gchar *
ibuildable_get_command (IAnjutaBuildable *manager,
						IAnjutaBuildableCommand command_id,
						GError **err)
{
	BasicAutotoolsPlugin *plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (manager);
	return plugin->commands[command_id];
}

static void
ibuildable_build (IAnjutaBuildable *manager, const gchar *directory,
				  GError **err)
{
	BasicAutotoolsPlugin *plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (manager);
	GFile *file;

	file = g_file_new_for_path (directory);
	if (file == NULL) return;

	build_build_file_or_dir (plugin, file, NULL, NULL, err);

	g_object_unref (file);
}

static void
ibuildable_clean (IAnjutaBuildable *manager, const gchar *directory,
				  GError **err)
{
	BasicAutotoolsPlugin *plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (manager);
	GFile *file;

	file = g_file_new_for_path (directory);
	if (file == NULL) return;

	build_clean_dir (plugin, file, err);

	g_object_unref (file);
}

static void
ibuildable_install (IAnjutaBuildable *manager, const gchar *directory,
					GError **err)
{
	BasicAutotoolsPlugin *plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (manager);
	GFile *file;

	file = g_file_new_for_path (directory);
	if (file == NULL) return;

	build_install_dir (plugin, file, NULL, NULL, err);

	g_object_unref (file);
}

static void
ibuildable_configure (IAnjutaBuildable *manager, const gchar *directory,
					  GError **err)
{
	BasicAutotoolsPlugin *plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (manager);
	GFile *file;

	file = g_file_new_for_path (directory);
	if (file == NULL) return;

	build_configure_dir (plugin, file, NULL, NULL, NULL, NULL, NULL, NULL);

	g_object_unref (file);
}

static void
ibuildable_generate (IAnjutaBuildable *manager, const gchar *directory,
					 GError **err)
{
	BasicAutotoolsPlugin *plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (manager);
	GFile *file;

	file = g_file_new_for_path (directory);
	if (file == NULL) return;

	build_generate_dir (plugin, file, NULL, NULL, NULL, NULL, NULL, NULL);

	g_object_unref (file);
}

static void
ibuildable_execute (IAnjutaBuildable *manager, const gchar *uri,
					GError **err)
{
	BasicAutotoolsPlugin *plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (manager);
	if (uri && strlen (uri) > 0)
		execute_program (plugin, uri);
	else
		execute_program (plugin, NULL);
}

static void
ibuildable_iface_init (IAnjutaBuildableIface *iface)
{
	/* iface->compile = ibuildable_compile; */
	iface->set_command = ibuildable_set_command;
	iface->get_command = ibuildable_get_command;
	iface->reset_commands = ibuildable_reset_commands;
	iface->build = ibuildable_build;
	iface->clean = ibuildable_clean;
	iface->install = ibuildable_install;
	iface->configure = ibuildable_configure;
	iface->generate = ibuildable_generate;
	iface->execute = ibuildable_execute;
}


/* IAnjutaFile implementation
 *---------------------------------------------------------------------------*/

static void
ifile_open (IAnjutaFile *manager, GFile* file,
			GError **err)
{
	gchar* uri = g_file_get_uri (file);
	ianjuta_buildable_execute (IANJUTA_BUILDABLE (manager), uri, NULL);
	g_free(uri);
}

static GFile*
ifile_get_file (IAnjutaFile *manager, GError **err)
{
	DEBUG_PRINT ("%s", "Unsupported operation");
	return NULL;
}

static void
ifile_iface_init (IAnjutaFileIface *iface)
{
	iface->open = ifile_open;
	iface->get_file = ifile_get_file;
}


/* IAnjutaBuilder implementation
 *---------------------------------------------------------------------------*/

static IAnjutaBuilderHandle
ibuilder_is_built (IAnjutaBuilder *builder, const gchar *uri,
	       IAnjutaBuilderCallback callback, gpointer user_data,
       	       GError **err)
{
	BasicAutotoolsPlugin *plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (builder);
	BuildContext *context;
	GFile *file;

	file = g_file_new_for_uri (uri);
	if (file == NULL) return NULL;

	context = build_is_file_built (plugin, file, callback, user_data, err);

	g_object_unref (file);

	return (IAnjutaBuilderHandle)context;
}

static IAnjutaBuilderHandle
ibuilder_build (IAnjutaBuilder *builder, const gchar *uri,
	       	IAnjutaBuilderCallback callback, gpointer user_data,
		GError **err)
{
	BasicAutotoolsPlugin *plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (builder);
	BuildContext *context;
	GFile *file;

	file = g_file_new_for_uri (uri);
	if (file == NULL) return NULL;

	context = build_configure_and_build (plugin, build_build_file_or_dir, plugin->project_root_dir, callback, user_data, NULL);

	g_object_unref (file);

	return (IAnjutaBuilderHandle)context;
}

static void
ibuilder_cancel (IAnjutaBuilder *builder, IAnjutaBuilderHandle handle, GError **err)
{
	BasicAutotoolsPlugin *plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (builder);

	build_cancel_command (plugin, (BuildContext *)handle, err);
}

static GList*
ibuilder_list_configuration (IAnjutaBuilder *builder, GError **err)
{
	BasicAutotoolsPlugin *plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (builder);

	return build_list_configuration (plugin);
}

static const gchar*
ibuilder_get_uri_configuration (IAnjutaBuilder *builder, const gchar *uri, GError **err)
{
	BasicAutotoolsPlugin *plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (builder);

	return build_get_uri_configuration (plugin, uri);
}

static void
ibuilder_iface_init (IAnjutaBuilderIface *iface)
{
	iface->is_built = ibuilder_is_built;
	iface->build = ibuilder_build;
	iface->cancel = ibuilder_cancel;
	iface->list_configuration = ibuilder_list_configuration;
	iface->get_uri_configuration = ibuilder_get_uri_configuration;
}

/* IAnjutaPreferences implementation
 *---------------------------------------------------------------------------*/

static void
ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	GtkWidget *root_check;
	GtkWidget *make_check;
	GtkWidget *root_entry;
	GtkWidget *make_entry;
	GtkBuilder *bxml;
	BasicAutotoolsPlugin *plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (ipref);

	/* Create the preferences page */
	bxml =  anjuta_util_builder_new (BUILDER_FILE, NULL);
	if (!bxml) return;

	anjuta_util_builder_get_objects (bxml,
	    INSTALL_ROOT_CHECK, &root_check,
	    INSTALL_ROOT_ENTRY, &root_entry,
	    PARALLEL_MAKE_CHECK, &make_check,
	    PARALLEL_MAKE_SPIN, &make_entry,
	    NULL);

	g_signal_connect(G_OBJECT(root_check), "toggled", G_CALLBACK(on_root_check_toggled), root_entry);
	on_root_check_toggled (root_check, root_entry);

	g_signal_connect(G_OBJECT(make_check), "toggled", G_CALLBACK(on_root_check_toggled), make_entry);
	on_root_check_toggled (make_check, make_entry);

	anjuta_preferences_add_from_builder (prefs, bxml, plugin->settings,
	                                     BUILD_PREFS_ROOT, _("Build Autotools"),  ICON_FILE);

	g_object_unref (bxml);
}

static void
ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	anjuta_preferences_remove_page(prefs, _("Build Autotools"));
}

static void
ipreferences_iface_init(IAnjutaPreferencesIface* iface)
{
	iface->merge = ipreferences_merge;
	iface->unmerge = ipreferences_unmerge;
}

ANJUTA_PLUGIN_BEGIN (BasicAutotoolsPlugin, basic_autotools_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE (ibuilder, IANJUTA_TYPE_BUILDER);
ANJUTA_PLUGIN_ADD_INTERFACE (ibuildable, IANJUTA_TYPE_BUILDABLE);
ANJUTA_PLUGIN_ADD_INTERFACE (ifile, IANJUTA_TYPE_FILE);
ANJUTA_PLUGIN_ADD_INTERFACE (ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (BasicAutotoolsPlugin, basic_autotools_plugin);
