/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    build-basic-autotools.c
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
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <libanjuta/interfaces/ianjuta-indicable.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>

#include "build-basic-autotools.h"
#include "build-options.h"
#include "executer.h"

#include <sys/wait.h>

#define ICON_FILE "anjuta-build-basic-autotools-plugin-48.png"
#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-build-basic-autotools-plugin.ui"
#define MAX_BUILD_PANES 3
#define PREF_INDICATORS_AUTOMATIC "indicators.automatic"
#define PREF_INSTALL_ROOT "build.install.root"
#define PREF_INSTALL_ROOT_COMMAND "build.install.root.command"
#define PREF_PARALLEL_MAKE "build.parallel.make"
#define PREF_PARALLEL_MAKE_JOB "build.parallel.make.job"
#define PREF_TRANSLATE_MESSAGE "build.translate.message"
#define PREF_CONTINUE_ON_ERROR "build.continue.error"

#define BUILD_PREFS_DIALOG "preferences_dialog_build"
#define BUILD_PREFS_ROOT "preferences_build_container"
#define INSTALL_ROOT_CHECK "preferences_toggle:bool:0:0:build.install.root"
#define INSTALL_ROOT_ENTRY "preferences_entry:text:sudo:0:build.install.root.command"
#define PARALLEL_MAKE_CHECK "preferences_toggle:bool:0:0:build.parallel.make"
#define PARALLEL_MAKE_SPIN "preferences_spin:int:1:0:build.parallel.make.job"

#define DEFAULT_COMMAND_COMPILE "make"
#define DEFAULT_COMMAND_BUILD "make"
#define DEFAULT_COMMAND_IS_BUILT "make -q"
#define DEFAULT_COMMAND_BUILD_TARBALL "make dist"
#define DEFAULT_COMMAND_INSTALL "make install"
#define DEFAULT_COMMAND_CONFIGURE "configure"
#define DEFAULT_COMMAND_GENERATE "autogen.sh"
#define DEFAULT_COMMAND_CLEAN "make clean"
#define DEFAULT_COMMAND_DISTCLEAN "make distclean"
#define DEFAULT_COMMAND_AUTORECONF "autoreconf -i --force"

#define CHOOSE_COMMAND(plugin,command) \
	((plugin->commands[(IANJUTA_BUILDABLE_COMMAND_##command)]) ? \
			(plugin->commands[(IANJUTA_BUILDABLE_COMMAND_##command)]) \
			: \
			(DEFAULT_COMMAND_##command))

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
	gchar *project_dir;
	gchar *build_dir;
	gchar **argv;
	GtkWindow *parent;
} BuildLinkCommand;

typedef struct
{
	gchar *pattern;
	GRegex *regex;
	GRegex *local_regex;
} MessagePattern;

typedef struct
{
	gchar *filename;
	gint line;
	IAnjutaIndicableIndicator indicator;
} BuildIndicatorLocation;

/* Command processing */
typedef struct
{
	AnjutaPlugin *plugin;
	
	AnjutaLauncher *launcher;
	gboolean used;
	
	IAnjutaBuilderCallback callback;
	gpointer user_data;

	IAnjutaMessageView *message_view;
	GHashTable *build_dir_stack;
	
	/* Indicator locations */
	GSList *locations;
	
	/* Editors in which indicators have been updated */
	GHashTable *indicators_updated_editors;
	
	/* Environment */
	IAnjutaEnvironment *environment;
} BuildContext;

/* Declarations */
static void update_project_ui (BasicAutotoolsPlugin *bb_plugin);
static gboolean directory_has_file (const gchar *dirname, const gchar *filename);

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

static gchar*
build_shell_expand (const gchar *input)
{
	GString* expand;
	
	if (input == NULL) return NULL;
	
	expand = g_string_sized_new (strlen (input));
	
	for (; *input != '\0'; input++)
	{
		switch (*input)
		{
			case '$':
			{
				/* Variable expansion */
				const gchar *end;
				gint var_name_len;

				end = input + 1;
				while (isalnum (*end) || (*end == '_')) end++;
				var_name_len = end - input - 1;
				if (var_name_len > 0)
				{
					const gchar *value;
					
					g_string_append_len (expand, input + 1, var_name_len);
					value = g_getenv (expand->str + expand->len - var_name_len);
					g_string_truncate (expand, expand->len - var_name_len);
					g_string_append (expand, value);
					input = end - 1;
					continue;
				}
				break;
			}
			case '~':
			{
				/* User home directory expansion */
				if (isspace(input[1]) || (input[1] == G_DIR_SEPARATOR) || (input[1] == '\0'))
				{
					g_string_append (expand, g_get_home_dir());
					continue;
				}
				break;
			}
			default:
				break;
		}
		g_string_append_c (expand, *input);
	}
	
	return g_string_free (expand, FALSE);
}

static gchar **
build_argv_command (const gchar *cmd_dir, const gchar *cmd, gchar **argv)
{
	gchar **new_cmd;
	gchar **split_cmd;
	guint split_len;
	
	split_cmd = g_strsplit (cmd, " ", 2);
	split_len = g_strv_length (split_cmd);
	
	if (argv == NULL)
	{
		new_cmd = split_cmd;
	}
	else
	{
		guint argv_len;
		
		argv_len = g_strv_length (argv);
		new_cmd = g_new (gchar *, split_len + argv_len + 1);
		memcpy (new_cmd, split_cmd, split_len * sizeof (gchar *));
		g_free (split_cmd);
		memcpy (new_cmd + split_len, argv, (argv_len + 1) * sizeof (gchar *));
		g_free (argv);
	}
	
	if (cmd_dir != NULL)
	{
		gchar *cmd = new_cmd[0];
		
		new_cmd[0] = g_strconcat (cmd_dir, G_DIR_SEPARATOR_S, cmd, NULL);
		g_free (cmd);
	}
	
	return new_cmd;
}

static gchar **
build_add_env_variable (gchar **envp, const gchar *name, const gchar *value)
{
	gchar **var;
	guint len = 0;

	if (envp != NULL)
	{
		/* Look for an already existing variable */
		for (var = envp; *var != NULL; var++)
		{
			if (strcmp (*var, name) == 0)
			{
				/* Just update variable */
				if (value == NULL)
				{
					*var[strlen(name)] = '\0';
				}
				else
				{
					g_free (*var);
					*var = g_strconcat (name, "=", value, NULL);
				}
				
				return envp;
			}
		}
		len = var - envp;
	}	
	
	/* Need to create a new variable */
	var = g_new (gchar *, len + 2);
	*var = g_strconcat (name, value == NULL ? NULL : "=", value, NULL);
	if (envp != NULL)
	{
		memcpy (&var[1], envp, len * sizeof (gchar *));
		g_free (envp);
	}
	var[len + 1] = NULL;
	
	return var;
}

static gchar **
build_add_arg (gchar **argv, const gchar *argument)
{
	gchar **new_argv;
	guint len;

	len = g_strv_length (argv);
	
	/* Need to create a new variable */
	new_argv = g_new (gchar *, len + 2);
	new_argv[0] = argv[0];
	new_argv[1] = g_strdup (argument);
	memcpy (&new_argv[2], &argv[1], len * sizeof (gchar *));
	g_free (argv);
	
	return new_argv;
}	
	
/* Allow installation as root (#321455) */
static void on_root_check_toggled(GtkWidget* toggle_button, GtkWidget* entry)
{
		gtk_widget_set_sensitive(entry, 
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle_button)));
}

static gchar*
get_root_install_command(BasicAutotoolsPlugin *bplugin)
{
	AnjutaPlugin* plugin = ANJUTA_PLUGIN(bplugin);
	AnjutaPreferences* prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	if (anjuta_preferences_get_int (prefs , PREF_INSTALL_ROOT))
	{
		gchar* command = anjuta_preferences_get(prefs, PREF_INSTALL_ROOT_COMMAND);
		if (command != NULL)
			return command;
		else
			return g_strdup("");
	}
	else
		return g_strdup("");
}

/* Indicator locations reported by the build */

static BuildIndicatorLocation*
build_indicator_location_new (const gchar *filename, gint line,
							  IAnjutaIndicableIndicator indicator)
{
	BuildIndicatorLocation *loc = g_new0 (BuildIndicatorLocation, 1);
	loc->filename = g_strdup (filename);
	loc->line = line;
	loc->indicator = indicator;
	return loc;
}

static void
build_indicator_location_set (BuildIndicatorLocation *loc,
							  IAnjutaEditor *editor,
							  const gchar *editor_filename)
{
	IAnjutaIterable *line_start, *line_end;
	
	if (editor && editor_filename &&
		IANJUTA_IS_INDICABLE (editor) &&
		IANJUTA_IS_EDITOR (editor) &&
		strcmp (editor_filename, loc->filename) == 0)
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
}

static void
build_indicator_location_free (BuildIndicatorLocation *loc)
{
	g_free (loc->filename);
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
		DEBUG_PRINT("Directory stack misaligned!!");
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

static gboolean
build_context_destroy_command (BuildContext *context)
{
	if (context->used) return FALSE;
	if (context->callback)
	{
		GError *err;
			
		/* Emit command-finished signal abort */
		err = g_error_new_literal (ianjuta_builder_error_quark (),
								   IANJUTA_BUILDER_ABORTED,
								   _("Command aborted"));
		context->callback (G_OBJECT (context->plugin), context, err, context->user_data);
		g_error_free (err);
		context->callback = NULL;
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
	
static void
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
	
	fp = fopen (PACKAGE_DATA_DIR"/build/automake-c.filters", "r");
	if (fp == NULL)
	{
		DEBUG_PRINT ("Failed to load filters: %s",
				   PACKAGE_DATA_DIR"/build/automake-c.filters");
		return;
	}
	while (!feof (fp) && !ferror (fp))
	{
		char buffer[1024];
		gchar **tokens;
		BuildPattern *pattern;
		
		fgets (buffer, 1024, fp);
		if (ferror (fp))
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
	i = strlen (line) - 1;
	while (isspace (line[i]) == FALSE)
	{
		i--;
		if (i < 0)
		{
			*filename = NULL;
			*lineno = 0;
			return FALSE;
		}
	}
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
			(strstr (line, _("warning:")) != NULL))
		{
			type = IANJUTA_MESSAGE_VIEW_TYPE_WARNING;
			indicator = IANJUTA_INDICABLE_WARNING;
		}
		else
		{
			type = IANJUTA_MESSAGE_VIEW_TYPE_ERROR;
			indicator = IANJUTA_INDICABLE_CRITICAL;
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
												indicator);
			context->locations = g_slist_prepend (context->locations, loc);
			
			/* If current editor file is same as indicator file, set indicator */
			if (anjuta_preferences_get_int (anjuta_shell_get_preferences (context->plugin->shell, NULL), PREF_INDICATORS_AUTOMATIC))
			{
				build_indicator_location_set (loc, p->current_editor,
											  p->current_editor_filename);
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
	else if (strstr (line, ": ") != NULL)
	{
		type = IANJUTA_MESSAGE_VIEW_TYPE_INFO;
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
	if (context->callback != NULL)
	{
		GError *err = NULL;
		IAnjutaBuilderCallback callback;
		
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
		callback = context->callback;	
		context->callback = NULL;
		callback (G_OBJECT (context->plugin), context, err, context->user_data);	
	}
	if (context->used)
		return;	/* Another command is run */
	
	g_signal_handlers_disconnect_by_func (context->launcher,
										  G_CALLBACK (on_build_terminated),
										  context);
	
	/* Message view could have been destroyed before */
	if (context->message_view)
	{
		gchar *buff1;
	
		buff1 = g_strdup_printf (_("Total time taken: %lu secs\n"),
								 time_taken);
		if (status)
		{
			ianjuta_message_view_buffer_append (context->message_view,
									_("Completed unsuccessful\n"), NULL);
		}
		else
		{
			ianjuta_message_view_buffer_append (context->message_view,
									   _("Completed successful\n"), NULL);
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
	DEBUG_PRINT ("Destroying build context");
	context->message_view = NULL;
	
	build_context_destroy_view (context);
}

static gboolean
g_hashtable_foreach_true (gpointer key, gpointer value, gpointer user_data)
{
	return TRUE;
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

	ianjuta_message_manager_set_current_view (mesg_manager,
											  context->message_view, NULL);
	
	/* Reset indicators in editors */
	if (IANJUTA_IS_INDICABLE (plugin->current_editor))
		ianjuta_indicable_clear (IANJUTA_INDICABLE (plugin->current_editor),
								 NULL);
	/* This is only since glib 2.12.
	g_hash_table_remove_all (context->indicators_updated_editors);
	*/
	g_hash_table_foreach_remove (context->indicators_updated_editors,
								 g_hashtable_foreach_true, NULL);
	
	return context;
}
	
static BuildContext*
build_get_context (BasicAutotoolsPlugin *plugin, const gchar *dir,
		   gboolean with_view)
{
	BuildContext *context = NULL;
	AnjutaPluginManager *plugin_manager;

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
	
	context->callback = NULL;
	context->launcher = anjuta_launcher_new ();
	g_signal_connect (G_OBJECT (context->launcher), "child-exited",
					  G_CALLBACK (on_build_terminated), context);
	build_context_push_dir (context, "default", dir);
	dir = g_strconcat (dir, "/", NULL);
	g_chdir (dir);
	
	plugin->contexts_pool = g_list_append (plugin->contexts_pool, context);
	
	return context;
}

/* Save all current anjuta files */
static void
save_all_files (AnjutaPlugin *plugin)
{
	IAnjutaDocumentManager *docman;
	IAnjutaFileSavable* save;

	docman = anjuta_shell_get_interface (plugin->shell, IAnjutaDocumentManager, NULL);
	/* No document manager, so no file to save */
	if (docman != NULL)
	{
		save = IANJUTA_FILE_SAVABLE (docman);
		if (save) ianjuta_file_savable_save (save, NULL);
	}
}

static gboolean
build_execute_command_in_context (BuildContext* context, const gchar *dir,
					gchar *argv[],
								  gchar *envp[],
								  IAnjutaBuilderCallback callback, gpointer user_data,
								  GError **err)
{
	gchar *real_dir;
	
	g_return_val_if_fail (argv != NULL, FALSE);

	real_dir = g_strdup (dir);
	
	context->callback = callback;
	context->user_data = user_data;
	context->used = TRUE;
	
	/* Add current directory */
	envp = build_add_env_variable (envp, "PWD", dir);
	
  	if (context->environment)
	{
		if (!ianjuta_environment_override (context->environment, &real_dir, &argv, &envp, NULL))
		{
			g_object_unref (context->environment);
			context->environment = NULL;
		}
	}
	
	if (context->message_view)
	{
		gchar *command;
		
		command = g_strjoinv (" ", argv);
		ianjuta_message_view_buffer_append (context->message_view,
										"Building in directory: ", NULL);
		ianjuta_message_view_buffer_append (context->message_view, dir, NULL);
		ianjuta_message_view_buffer_append (context->message_view, "\n", NULL);
		ianjuta_message_view_buffer_append (context->message_view, command, NULL);
		ianjuta_message_view_buffer_append (context->message_view, "\n", NULL);
		g_free (command);
	
		anjuta_launcher_execute_v (context->launcher, argv, envp,
								 on_build_mesg_arrived, context);
	}
	else
	{
		anjuta_launcher_execute_v (context->launcher, argv, envp,
								 NULL, NULL);
	}		
	
	return TRUE;
}	

static gchar *
target_from_source (BasicAutotoolsPlugin *plugin, const gchar *target)
{
	if ((plugin->project_root_dir == NULL) || 
		(plugin->project_build_dir == NULL) || 
		(strcmp (plugin->project_root_dir, plugin->project_build_dir) == 0))
	{
		return g_strdup (target);
	}
	else
	{
		return g_strconcat (plugin->project_build_dir, target + strlen (plugin->project_root_dir), NULL);
	}
}

static BuildContext*
build_execute_command_full (BasicAutotoolsPlugin* bplugin, const gchar *dir,
			gchar *argv[],
			gboolean save_file, gboolean with_view, gchar *envp[],
			IAnjutaBuilderCallback callback, gpointer user_data,
			GError **err)
{
	AnjutaPlugin* plugin = ANJUTA_PLUGIN(bplugin);
	BuildContext *context;
	AnjutaPreferences* prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	gboolean ok;
	gchar **arg;
	
	if (save_file) 
		save_all_files (ANJUTA_PLUGIN (plugin));

	context = build_get_context (bplugin, dir, with_view);

	/* Send options to make */
	if (strcmp (argv[0], "make") == 0)
	{
		if (!anjuta_preferences_get_int (prefs , PREF_TRANSLATE_MESSAGE))
		{
			envp = build_add_env_variable (envp, "LANGUAGE", "C");
		}
		if (anjuta_preferences_get_int (prefs , PREF_PARALLEL_MAKE))
		{
			gchar *arg = g_strdup_printf ("-j%d", anjuta_preferences_get_int (prefs , PREF_PARALLEL_MAKE_JOB));
			argv = build_add_arg (argv, arg);
			g_free (arg);
		}
		if (anjuta_preferences_get_int (prefs , PREF_CONTINUE_ON_ERROR))
		{
			argv = build_add_arg (argv, "-k");
		}
	}
	
	/* Expand shell variable */
	for (arg = argv; *arg != NULL; arg++)
	{
		gchar *old_arg;
		
		old_arg = *arg;
		*arg = build_shell_expand (old_arg);
		g_free (old_arg);
	}
	
	ok = build_execute_command_in_context (context, dir, argv, envp, callback, user_data, NULL);
	g_strfreev (argv);
	
	if (ok)
	{
		return context;
	}
	else
	{
		build_context_destroy (context);
		
		return NULL;
	}
}


static gboolean
build_execute_command (BasicAutotoolsPlugin* bplugin, const gchar *dir,
		   const gchar *command, gboolean save_file, GError **err)
{
	gchar **argv;
	gboolean ok;

	/* Store args and environment variables as string array */
	if (!g_shell_parse_argv (command, NULL, &argv, NULL))
		return FALSE;
	
	ok = build_execute_command_full (bplugin, dir, argv, save_file, TRUE, NULL, NULL, NULL, NULL) != NULL;

	return ok;
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
			if (context->launcher != NULL)
			{
				anjuta_launcher_signal (context->launcher, SIGTERM);
				return;
			}
		}	
	}	

	/* Invalid handle passed */
	g_return_if_reached ();	
}

static gboolean
build_compile_file_real (BasicAutotoolsPlugin *plugin, const gchar *file)
{
	gchar *target;
	gchar *target_basename;
	gchar *target_dirname;
	gchar *ext_ptr;
	gboolean ret;
	
	/* FIXME: This should be configuration somewhere, eg. preferences */
	static GHashTable *target_ext = NULL;
	if (!target_ext)
	{
		target_ext = g_hash_table_new (g_str_hash, g_str_equal);
		g_hash_table_insert (target_ext, ".c", ".o");
		g_hash_table_insert (target_ext, ".cpp", ".o");
		g_hash_table_insert (target_ext, ".cxx", ".o");
		g_hash_table_insert (target_ext, ".c++", ".o");
		g_hash_table_insert (target_ext, ".cc", ".o");
		g_hash_table_insert (target_ext, ".in", "");
		g_hash_table_insert (target_ext, ".in.in", ".in");
		g_hash_table_insert (target_ext, ".la", ".la");
		g_hash_table_insert (target_ext, ".a", ".a");
		g_hash_table_insert (target_ext, ".so", ".so");
		g_hash_table_insert (target_ext, ".java", ".class");
	}
	
	g_return_val_if_fail (file != NULL, FALSE);
	ret = FALSE;

	target = target_from_source (plugin, file);
	target_basename = g_path_get_basename (target);
	target_dirname = g_path_get_dirname (target);
	ext_ptr = strrchr (target_basename, '.');
	if (ext_ptr)
	{
		const gchar *new_ext;
		new_ext = g_hash_table_lookup (target_ext, ext_ptr);
		if (new_ext)
		{
			gchar *command;
			
			*ext_ptr = '\0';
			command = g_strconcat (CHOOSE_COMMAND (plugin, COMPILE), " ",
								   target_basename, new_ext, NULL);
			build_execute_command (plugin, target_dirname, command, TRUE, NULL);
			g_free (command);
			ret = TRUE;
		}
	} else {
		/* If file has no extension, take it as target itself */
		gchar *command;
		command = g_strconcat (CHOOSE_COMMAND(plugin, COMPILE), " ",
							   target_basename, NULL);
		build_execute_command (plugin, target_dirname, command, TRUE, NULL);
		g_free (command);
		ret = TRUE;
	}
	g_free (target_basename);
	g_free (target_dirname);
	g_free (target);
	if (ret == FALSE)
	{
		/* FIXME: Prompt the user to create a Makefile with a wizard
		   (if there is no Makefile in the directory) or to add a target
		   rule in the above hash table, eg. editing the preferences, if
		   there is target extension defined for that file extension.
		*/
		GtkWindow *window;
		window = GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell);
		anjuta_util_dialog_error (window, _("Can not compile \"%s\": No compile rule defined for this file type."), file);
	}
	return ret;
}

/* UI actions */
static void
build_build_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	build_execute_command (plugin, plugin->project_build_dir,
						   CHOOSE_COMMAND (plugin, BUILD), TRUE, NULL);
}

static void
build_install_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	gchar* root = get_root_install_command(plugin);
	gchar* command = g_strdup_printf("%s %s", root,
									 CHOOSE_COMMAND (plugin, INSTALL));
	g_free(root);
	build_execute_command (plugin, plugin->project_build_dir,
						   command, TRUE, NULL);
	g_free(command);
}

static void
build_clean_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	build_execute_command (plugin, plugin->project_build_dir,
						   CHOOSE_COMMAND (plugin, CLEAN), FALSE, NULL);
}

static void
build_remove_build_dir (GObject *sender,
						IAnjutaBuilderHandle context,
						GError *error,
						gpointer user_data)
{
	
}

static void
build_distclean_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	gchar **argv;

	if (!g_shell_parse_argv (CHOOSE_COMMAND (plugin, DISTCLEAN), NULL, &argv, NULL))
		return;
	
	build_execute_command_full (plugin, plugin->project_build_dir, argv, TRUE, TRUE, NULL, build_remove_build_dir, plugin, NULL);
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

static void
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
build_project_configured (GObject *sender,
							IAnjutaBuilderHandle context,
							GError *error,
							gpointer user_data)
{
	BasicAutotoolsPlugin *plugin = (BasicAutotoolsPlugin *)sender;
	GValue *value;
	gchar *uri;
	
	value = g_new0 (GValue, 1);
	g_value_init (value, G_TYPE_STRING);
	
	uri = build_configuration_list_get_build_uri (plugin->configurations, build_configuration_list_get_selected (plugin->configurations));
	g_value_set_string (value, uri);
	g_free (uri);
	
	anjuta_shell_add_value (ANJUTA_PLUGIN (plugin)->shell, IANJUTA_BUILDER_ROOT_URI, value, NULL);
	
	build_update_configuration_menu (plugin);
}

static void
build_configure_project_after_autogen (GObject *sender,
									   IAnjutaBuilderHandle context,
									   GError *error,
									   gpointer user_data)
{
	BuildLinkCommand *link = (BuildLinkCommand *)user_data;
	BasicAutotoolsPlugin *plugin = (BasicAutotoolsPlugin *)sender;
	
	if (error == NULL)
	{
		struct stat conf_stat, log_stat;
		gchar *filename;
		
		filename = g_build_filename (link->project_dir, "configure", NULL);
		if (stat (filename, &conf_stat) != 0)
		{
			anjuta_util_dialog_error (link->parent, _("Can not configure project: Missing configure script in %s."), link->project_dir);
		}
		else
		{
			g_free (filename);
			filename = g_build_filename (link->build_dir, "config.log", NULL);
			if ((stat (filename, &log_stat) != 0) ||
				(log_stat.st_mtime < conf_stat.st_mtime))
			{
				link->argv = build_argv_command (link->project_dir,
												 CHOOSE_COMMAND (plugin, CONFIGURE),
												 link->argv);
				build_execute_command_in_context ((BuildContext *)context, link->build_dir, link->argv, 
												  NULL, build_project_configured, NULL, NULL);
			}
		}
		g_free (filename);
	}
	
	g_free (link->project_dir);
	g_free (link->build_dir);
	g_strfreev (link->argv);
	g_free (link);
}

static void
build_configure_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	GtkWindow *parent;
	gboolean run_autogen = FALSE;
	const gchar *project_root;
	GValue value = {0,};

	run_autogen = !directory_has_file (plugin->project_root_dir, "configure");
	
	anjuta_shell_get_value (ANJUTA_PLUGIN (plugin)->shell, IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI, &value, NULL);
	project_root = g_value_get_string (&value);
	parent = GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell);
	if (build_dialog_configure (parent, project_root, plugin->configurations, &run_autogen))
	{
		BuildConfiguration *config;
		gchar **argv;
		GFile *build_file;
		gchar *build_dir;
		gchar *uri;

		config = build_configuration_list_get_selected (plugin->configurations);
		uri = build_configuration_list_get_build_uri (plugin->configurations, config);
		build_file = g_file_new_for_uri (uri);
		g_free (uri);
		build_dir = g_file_get_path (build_file);
		g_object_unref (build_file);
		
		IAnjutaBuilderCallback callback = NULL;
		BuildLinkCommand* user_data = NULL;

		if (run_autogen)
		{
			gboolean has_autogen = directory_has_file (plugin->project_root_dir,
													   "autogen.sh");
			
			gchar **args = g_strdupv (build_configuration_get_args (config));
			argv = build_argv_command (plugin->project_root_dir,
									   has_autogen ? CHOOSE_COMMAND (plugin, GENERATE) : CHOOSE_COMMAND (plugin, AUTORECONF),
									   args);
									   
			
			/* Plan to run configure afterward */
			user_data = g_new (BuildLinkCommand, 1);
			user_data->project_dir = g_strdup (plugin->project_root_dir);
			user_data->build_dir = g_strdup (build_dir);
		    /* Remove command name from argument array */
			user_data->argv = g_strdupv (&argv[1]); 
			user_data->parent = parent;
		
			callback = build_configure_project_after_autogen;
		}
		else
		{
			/* Run configure only */
			gchar **args = g_strdupv (build_configuration_get_args (config));
			
			argv = build_argv_command (plugin->project_root_dir,
									   CHOOSE_COMMAND (plugin, CONFIGURE),
									   args);
			callback = build_project_configured;
		}
		build_execute_command_full (plugin, build_dir, 
					   argv, TRUE, TRUE, NULL,
					   callback, user_data, NULL);
		g_free (build_dir);
	}
}

static void
build_distribution_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	build_execute_command (plugin, plugin->project_build_dir,
			   CHOOSE_COMMAND (plugin, BUILD_TARBALL),
			   FALSE, NULL);
}

static void
build_build_module (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	gchar *dirname = g_dirname (plugin->current_editor_filename);
	gchar *target = target_from_source (plugin, dirname);

	build_execute_command (plugin, target,
			   CHOOSE_COMMAND (plugin, BUILD),
			   TRUE, NULL);
	g_free (target);
	g_free (dirname);
}

static void
build_install_module (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	gchar *dirname = g_dirname (plugin->current_editor_filename);
	gchar* root = get_root_install_command(plugin);
	gchar *target = target_from_source (plugin, dirname);
	gchar* command = g_strdup_printf ("%s %s", root,
									  CHOOSE_COMMAND (plugin, INSTALL));
	g_free(root);
	build_execute_command (plugin, target, command, TRUE, NULL);
	g_free(command);
	g_free (target);
	g_free (dirname);
}

static void
build_clean_module (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	gchar *dirname = g_dirname (plugin->current_editor_filename);
	gchar *target = target_from_source (plugin, dirname);
	build_execute_command (plugin, target,
			   CHOOSE_COMMAND (plugin, CLEAN),
			   FALSE, NULL);
	g_free (target);
	g_free (dirname);
}

static void
build_compile_file (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	if (plugin->current_editor_filename)
	{
		build_compile_file_real (plugin, plugin->current_editor_filename);
	}
}

/* File manager context menu */
static void
fm_compile (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	if (plugin->fm_current_filename)
	{
		build_compile_file_real (plugin, plugin->fm_current_filename);
	}
}

static void
fm_build (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	gchar *dir;
	gchar *target;
	
	g_return_if_fail (plugin->fm_current_filename != NULL);
	
	if (g_file_test (plugin->fm_current_filename, G_FILE_TEST_IS_DIR))
		dir = g_strdup (plugin->fm_current_filename);
	else
		dir = g_path_get_dirname (plugin->fm_current_filename);
	target = target_from_source (plugin, dir);
	build_execute_command (plugin, target,
			   CHOOSE_COMMAND (plugin, BUILD),
			   TRUE, NULL);
	g_free (target);
	g_free (dir);
}

static void
fm_install (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	gchar *dir;
	gchar* root; 
	gchar* command; 
	gchar *target;
	
	g_return_if_fail (plugin->fm_current_filename != NULL);
	
	
	if (g_file_test (plugin->fm_current_filename, G_FILE_TEST_IS_DIR))
		dir = g_strdup (plugin->fm_current_filename);
	else
		dir = g_path_get_dirname (plugin->fm_current_filename);
	root = get_root_install_command(plugin);
	command = g_strdup_printf ("%s %s", root,
							   CHOOSE_COMMAND (plugin, INSTALL));
	g_free(root);
	target = target_from_source (plugin, dir);
	build_execute_command (plugin, dir, command, TRUE, NULL);
	g_free (target);
}

static void
fm_clean (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	gchar *dir;
	gchar *target;
	
	g_return_if_fail (plugin->fm_current_filename != NULL);
	
	if (g_file_test (plugin->fm_current_filename, G_FILE_TEST_IS_DIR))
		dir = g_strdup (plugin->fm_current_filename);
	else
		dir = g_path_get_dirname (plugin->fm_current_filename);
	
	target = target_from_source (plugin, dir);
	build_execute_command (plugin, target,
			   CHOOSE_COMMAND (plugin, CLEAN),
			   FALSE, NULL);
	g_free (target);
}

/* Project manager context menu */
static void
pm_compile (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	if (plugin->pm_current_filename)
	{
		build_compile_file_real (plugin, plugin->pm_current_filename);
	}
}

static void
pm_build (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	gchar *dir;
	gchar *target;
	
	g_return_if_fail (plugin->pm_current_filename != NULL);
	
	if (g_file_test (plugin->pm_current_filename, G_FILE_TEST_IS_DIR))
		dir = g_strdup (plugin->pm_current_filename);
	else
		dir = g_path_get_dirname (plugin->pm_current_filename);
	target = target_from_source (plugin, dir);
	build_execute_command (plugin, target,
			   CHOOSE_COMMAND (plugin, BUILD),
			   TRUE, NULL);
	g_free (target);
	g_free (dir);
}

static void
pm_install (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	gchar *dir;
	gchar *target;
	gchar* root; 
	gchar* command; 
	
	g_return_if_fail (plugin->pm_current_filename != NULL);
	
	root = get_root_install_command(plugin);
	command = g_strdup_printf ("%s %s", root,
							   CHOOSE_COMMAND (plugin, INSTALL));
	g_free(root);
	
	if (g_file_test (plugin->pm_current_filename, G_FILE_TEST_IS_DIR))
		dir = g_strdup (plugin->pm_current_filename);
	else
		dir = g_path_get_dirname (plugin->pm_current_filename);
	target = target_from_source (plugin, dir);
	build_execute_command (plugin, target, command, TRUE, NULL);
	g_free(command);
	g_free (target);
	g_free (dir);
}

static void
pm_clean (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	gchar *dir;
	gchar *target;
	
	g_return_if_fail (plugin->pm_current_filename != NULL);
	
	if (g_file_test (plugin->pm_current_filename, G_FILE_TEST_IS_DIR))
		dir = g_strdup (plugin->pm_current_filename);
	else
		dir = g_path_get_dirname (plugin->pm_current_filename);
	target = target_from_source (plugin, dir);
	build_execute_command (plugin, target,
			   CHOOSE_COMMAND (plugin, CLEAN),
			   FALSE, NULL);
	g_free (dir);
	g_free (target);
}

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
					if (context->launcher != NULL)
					{
						anjuta_launcher_signal (context->launcher, SIGTERM);
					}
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
		"ActionBuildBuildProject", NULL,
		N_("_Build Project"), "<shift>F11",
		N_("Build whole project"),
		G_CALLBACK (build_build_project)
	},
	{
		"ActionBuildInstallProject", NULL,
		N_("_Install Project"), NULL,
		N_("Install whole project"),
		G_CALLBACK (build_install_project)
	},
	{
		"ActionBuildCleanProject", NULL,
		N_("_Clean Project"), NULL,
		N_("Clean whole project"),
		G_CALLBACK (build_clean_project)
	},
	{
		"ActionBuildConfigure", NULL,
		N_("C_onfigure Project..."), NULL,
		N_("Configure project"),
		G_CALLBACK (build_configure_project)
	},
	{
		"ActionBuildDistribution", NULL,
		N_("Build _Tarball"), NULL,
		N_("Build project tarball distribution"),
		G_CALLBACK (build_distribution_project)
	},
	{
		"ActionBuildBuildModule", GTK_STOCK_EXECUTE,
		N_("_Build Module"), "F11",
		N_("Build module associated with current file"),
		G_CALLBACK (build_build_module)
	},
	{
		"ActionBuildInstallModule", NULL,
		N_("_Install Module"), NULL,
		N_("Install module associated with current file"),
		G_CALLBACK (build_install_module)
	},
	{
		"ActionBuildCleanModule", NULL,
		N_("_Clean Module"), NULL,
		N_("Clean module associated with current file"),
		G_CALLBACK (build_clean_module)
	},
	{
		"ActionBuildCompileFile", GTK_STOCK_CONVERT,
		N_("Co_mpile File"), "F9",
		N_("Compile current editor file"),
		G_CALLBACK (build_compile_file)
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
		G_CALLBACK (build_distclean_project)
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

static gboolean
directory_has_makefile (const gchar *dirname)
{
	gchar *makefile;
	gboolean makefile_exists;
	
	makefile_exists = TRUE;	
	makefile = g_build_filename (dirname, "Makefile", NULL);
	if (!g_file_test (makefile, G_FILE_TEST_EXISTS))
	{
		g_free (makefile);
		makefile = g_build_filename (dirname, "makefile", NULL);
		if (!g_file_test (makefile, G_FILE_TEST_EXISTS))
		{
			g_free (makefile);
			makefile = g_build_filename (dirname, "MAKEFILE", NULL);
			if (!g_file_test (makefile, G_FILE_TEST_EXISTS))
			{
				makefile_exists = FALSE;
			}
		}
	}
	g_free (makefile);
	return makefile_exists;
}

static gboolean
directory_has_file (const gchar *dirname, const gchar *filename)
{
	gchar *filepath;
	gboolean exists;
	
	exists = TRUE;	
	filepath = g_build_filename (dirname, filename, NULL);
	if (!g_file_test (filepath, G_FILE_TEST_EXISTS))
		exists = FALSE;
	
	g_free (filepath);
	return exists;
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

static void
update_module_ui (BasicAutotoolsPlugin *bb_plugin)
{
	AnjutaUI *ui;
	GtkAction *action;
	gchar *filename= NULL;
	gchar *module = NULL;
	gchar *label;
	gboolean has_makefile= FALSE;

	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (bb_plugin)->shell, NULL);
	
	DEBUG_PRINT ("Updating module UI");
	
	if (bb_plugin->current_editor_filename != NULL)
	{
		gchar *dirname;
		gchar *build_dirname;

		dirname = g_dirname (bb_plugin->current_editor_filename);
		build_dirname = target_from_source (bb_plugin, dirname);
		
		module = escape_label (g_basename (dirname));
		filename = escape_label (g_basename (bb_plugin->current_editor_filename));
		has_makefile = directory_has_makefile (build_dirname);
		g_free (build_dirname);
		g_free (dirname);
	}

	action = anjuta_ui_get_action (ui, "ActionGroupBuild",
								   "ActionBuildBuildModule");
	label = g_strdup_printf (module ? _("_Build (%s)") : _("_Build"), module);
	g_object_set (G_OBJECT (action), "sensitive", has_makefile,
					  "label", label, NULL);
	g_free (label);

	action = anjuta_ui_get_action (ui, "ActionGroupBuild",
								   "ActionBuildInstallModule");
	label = g_strdup_printf (module ? _("_Install (%s)") : _("_Install"), module);
	g_object_set (G_OBJECT (action), "sensitive", has_makefile,
					  "label", label, NULL);
	g_free (label);

	action = anjuta_ui_get_action (ui, "ActionGroupBuild",
								   "ActionBuildCleanModule");
	label = g_strdup_printf (module ? _("_Clean (%s)") : _("_Clean"), module);
	g_object_set (G_OBJECT (action), "sensitive", has_makefile,
					  "label", label, NULL);
	g_free (label);
	
	
	action = anjuta_ui_get_action (ui, "ActionGroupBuild",
								   "ActionBuildCompileFile");
	label = g_strdup_printf (filename ? _("Co_mpile (%s)") : _("Co_mpile"), filename);
	g_object_set (G_OBJECT (action), "sensitive", has_makefile,
					  "label", label, NULL);
	g_free (label);
	
	g_free (module);
	g_free (filename);
}

static void
update_project_ui (BasicAutotoolsPlugin *bb_plugin)
{
	AnjutaUI *ui;
	GtkAction *action;
	gboolean has_makefile;
	gboolean has_project;
	
	DEBUG_PRINT ("Updating project UI");
	
	has_project = bb_plugin->project_root_dir != NULL;
	has_makefile = has_project && directory_has_makefile (bb_plugin->project_build_dir);

	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (bb_plugin)->shell, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupBuild",
								   "ActionBuildBuildProject");
	g_object_set (G_OBJECT (action), "sensitive", has_makefile, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupBuild",
								   "ActionBuildInstallProject");
	g_object_set (G_OBJECT (action), "sensitive", has_makefile, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupBuild",
								   "ActionBuildCleanProject");
	g_object_set (G_OBJECT (action), "sensitive", has_makefile, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupBuild",
								   "ActionBuildDistribution");
	g_object_set (G_OBJECT (action), "sensitive", has_makefile, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupBuild",
								   "ActionBuildConfigure");
	g_object_set (G_OBJECT (action), "sensitive", has_project, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupBuild",
								   "ActionBuildSelectConfiguration");
	g_object_set (G_OBJECT (action), "sensitive", has_project, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupBuild",
								   "ActionBuildRemoveConfiguration");
	g_object_set (G_OBJECT (action), "sensitive", has_makefile, NULL);
	
	update_module_ui (bb_plugin);
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
}

static void
on_session_load (AnjutaShell *shell, AnjutaSessionPhase phase,
				 AnjutaSession *session, BasicAutotoolsPlugin *plugin)
{
	GList *configurations;
	gchar *selected;
				
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

	build_project_configured (G_OBJECT (plugin), NULL, NULL, NULL);
}

static void
value_added_fm_current_file (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	AnjutaUI *ui;
	GtkAction *action;
	GFile* file;
	GFileInfo* file_info;
	gchar* filename;
	gchar* dirname;
	gboolean makefile_exists, is_dir;
	
	file = g_value_get_object (value);
	filename = g_file_get_path (file);
	g_return_if_fail (filename != NULL);

	BasicAutotoolsPlugin *ba_plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (plugin);
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	if (ba_plugin->fm_current_filename)
		g_free (ba_plugin->fm_current_filename);
	ba_plugin->fm_current_filename = filename;
	
	file_info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_TYPE, 
								   G_FILE_QUERY_INFO_NONE, NULL, NULL);
	is_dir = (g_file_info_get_file_type(file_info) == G_FILE_TYPE_DIRECTORY);
	if (is_dir)
		dirname = g_strdup (filename);
	else
		dirname = g_path_get_dirname (filename);
	makefile_exists = directory_has_makefile (dirname);
	g_free (dirname);
	
	if (!makefile_exists)
		return;
	
	action = anjuta_ui_get_action (ui, "ActionGroupPopupBuild", "ActionPopupBuild");
	g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupPopupBuild",
										"ActionPopupBuildCompile");
	if (is_dir)
		g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
	else
		g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
}

static void
value_removed_fm_current_file (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	AnjutaUI *ui;
	GtkAction *action;
	BasicAutotoolsPlugin *ba_plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (plugin);
	
	if (ba_plugin->fm_current_filename)
		g_free (ba_plugin->fm_current_filename);
	ba_plugin->fm_current_filename = NULL;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupPopupBuild", "ActionPopupBuild");
	g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
}

static void
value_added_pm_current_uri (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	AnjutaUI *ui;
	GtkAction *action;
	const gchar *uri;
	gchar *dirname, *filename;
	gboolean makefile_exists, is_dir;
	GFile* file;
	
	uri = g_value_get_string (value);
	file = g_file_new_for_uri (uri);
	filename = g_file_get_path (file);
	g_object_unref (file);
	g_return_if_fail (filename != NULL);
	
	/*
	if (g_file_test (filename, G_FILE_TEST_EXISTS) == FALSE)
		return;
	*/
	
	BasicAutotoolsPlugin *ba_plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (plugin);
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	if (ba_plugin->pm_current_filename)
		g_free (ba_plugin->pm_current_filename);
	ba_plugin->pm_current_filename = filename;
	
	is_dir = g_file_test (filename, G_FILE_TEST_IS_DIR);
	if (is_dir)
		dirname = g_strdup (filename);
	else
		dirname = g_path_get_dirname (filename);
	makefile_exists = directory_has_makefile (dirname);
	g_free (dirname);
	
	if (!makefile_exists)
		return;
	
	action = anjuta_ui_get_action (ui, "ActionGroupPopupBuild", "ActionPopupPMBuild");
	g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupPopupBuild",
										"ActionPopupPMBuildCompile");
	if (is_dir)
		g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
	else
		g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
}

static void
value_removed_pm_current_uri (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	AnjutaUI *ui;
	GtkAction *action;
	BasicAutotoolsPlugin *ba_plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (plugin);
	
	if (ba_plugin->pm_current_filename)
		g_free (ba_plugin->pm_current_filename);
	ba_plugin->pm_current_filename = NULL;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupPopupBuild", "ActionPopupPMBuild");
	g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
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
		GFile* file = g_file_new_for_uri (root_uri);
		bb_plugin->project_root_dir =
			g_file_get_path(file);
		g_object_unref (file);
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
	
	g_free (bb_plugin->project_root_dir);
	g_free (bb_plugin->project_build_dir);
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
	
	g_free (bb_plugin->project_build_dir);
	bb_plugin->project_build_dir = NULL;
	
	build_uri = g_value_get_string (value);
	if (build_uri)
	{
		GFile* file = g_file_new_for_uri (build_uri);
		bb_plugin->project_build_dir =
			g_file_get_path(file);
		g_object_unref (file);
	}
	update_project_ui (bb_plugin);
}

static gint
on_update_indicators_idle (gpointer data)
{
	AnjutaPlugin *plugin = ANJUTA_PLUGIN (data);
	BasicAutotoolsPlugin *ba_plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (data);
	IAnjutaEditor *editor = ba_plugin->current_editor;
	
	/* If indicators are not yet updated in the editor, do it */
	if (ba_plugin->current_editor_filename &&
		IANJUTA_IS_INDICABLE (editor) &&
		anjuta_preferences_get_int (anjuta_shell_get_preferences (plugin->shell,
																  NULL),
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
					IANJUTA_EDITOR (editor), ba_plugin->current_editor_filename);
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
	GFile* file;
	GObject *editor;
	
	editor = g_value_get_object (value);
	
	if (!IANJUTA_IS_EDITOR(editor))
		return;
	
	BasicAutotoolsPlugin *ba_plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (plugin);
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	g_free (ba_plugin->current_editor_filename);
	ba_plugin->current_editor_filename = NULL;
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
	
	file = ianjuta_file_get_file (IANJUTA_FILE (editor), NULL);
	if (file)
	{
		gchar *filename;
		
		filename = g_file_get_path(file);
		g_object_unref (file);
		g_return_if_fail (filename != NULL);
		ba_plugin->current_editor_filename = filename;
		update_module_ui (ba_plugin);
	}
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
	if (ba_plugin->current_editor_filename)
		g_free (ba_plugin->current_editor_filename);
	ba_plugin->current_editor_filename = NULL;
	ba_plugin->current_editor = NULL;
	
	update_module_ui (ba_plugin);
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	static gboolean initialized = FALSE;
	BasicAutotoolsPlugin *ba_plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (plugin);
	
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
	
	g_free (ba_plugin->fm_current_filename);
	g_free (ba_plugin->pm_current_filename);
	g_free (ba_plugin->project_root_dir);
	g_free (ba_plugin->project_build_dir);
	g_free (ba_plugin->current_editor_filename);
	g_free (ba_plugin->program_args);
	build_configuration_list_free (ba_plugin->configurations);
	
	ba_plugin->fm_current_filename = NULL;
	ba_plugin->pm_current_filename = NULL;
	ba_plugin->project_root_dir = NULL;
	ba_plugin->project_build_dir = NULL;
	ba_plugin->current_editor_filename = NULL;
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
	
	ba_plugin->fm_current_filename = NULL;
	ba_plugin->pm_current_filename = NULL;
	ba_plugin->project_root_dir = NULL;
	ba_plugin->project_build_dir = NULL;
	ba_plugin->current_editor = NULL;
	ba_plugin->current_editor_filename = NULL;
	ba_plugin->contexts_pool = NULL;
	ba_plugin->configurations = build_configuration_list_new ();
	ba_plugin->program_args = NULL;
	ba_plugin->run_in_terminal = TRUE;
	ba_plugin->last_exec_uri = NULL;
	ba_plugin->editors_created = g_hash_table_new (g_direct_hash,
												   g_direct_equal);
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

#if 0
static void
ibuildable_compile (IAnjutaBuildable *manager, const gchar * filename,
					GError **err)
{
	BasicAutotoolsPlugin *plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (manager);
	build_compile_file_real (plugin, filename);
}
#endif

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
	build_execute_command (plugin, directory,
			   CHOOSE_COMMAND (plugin, BUILD),
			   TRUE, NULL);
}

static void
ibuildable_clean (IAnjutaBuildable *manager, const gchar *directory,
				  GError **err)
{
	BasicAutotoolsPlugin *plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (manager);
	build_execute_command (plugin, directory,
			   CHOOSE_COMMAND (plugin, CLEAN),
			   FALSE, NULL);
}

static void
ibuildable_install (IAnjutaBuildable *manager, const gchar *directory,
					GError **err)
{
	BasicAutotoolsPlugin *plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (manager);
	gchar* root = get_root_install_command(plugin);
	gchar* command = g_strdup_printf ("%s %s", root,
									  CHOOSE_COMMAND (plugin, INSTALL));
	g_free(root);
	build_execute_command (plugin, directory, command, TRUE, NULL);
	g_free(command);
}

static void
ibuildable_configure (IAnjutaBuildable *manager, const gchar *directory,
					  GError **err)
{
	BasicAutotoolsPlugin *plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (manager);
	build_execute_command (plugin, directory,
			   CHOOSE_COMMAND (plugin, CONFIGURE),
			   TRUE, NULL);
}

static void
ibuildable_generate (IAnjutaBuildable *manager, const gchar *directory,
					 GError **err)
{
	BasicAutotoolsPlugin *plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (manager);
	if (directory_has_file (plugin->project_root_dir, "autogen.sh"))
	{
		build_execute_command (plugin, directory,
				   CHOOSE_COMMAND (plugin, GENERATE),
				   FALSE, NULL);
	}
	else
	{
		/* FIXME: get override command for this too */
		build_execute_command (plugin, directory,
				   "autoreconf -i --force", 
				   FALSE, NULL);
	}
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

static void
ifile_open (IAnjutaFile *manager, GFile* file,
			GError **err)
{
	gchar* uri = g_file_get_uri (file);
	ianjuta_buildable_execute (IANJUTA_BUILDABLE (manager), uri, NULL);
	g_free(file);
}

static GFile*
ifile_get_file (IAnjutaFile *manager, GError **err)
{
	DEBUG_PRINT ("Unsupported operation");
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
	gchar *filename;
	gchar *dirname;
	gchar **argv;
	gchar **args = g_new0 (gchar *, 2);
	GFile* file = g_file_new_for_uri (uri);
	
	filename = g_file_get_path (file);
	g_object_unref (file);
	if (filename == NULL) return NULL;
	args[0] = g_path_get_basename (filename);
	dirname = g_path_get_dirname (filename);
	g_free (filename);	
	argv = build_argv_command (NULL, CHOOSE_COMMAND (plugin, IS_BUILT), args);
	
	context = build_execute_command_full (plugin, dirname, argv,
			       			TRUE, FALSE, NULL,
			       			callback, user_data, err);
	g_free (dirname);
	
	return (IAnjutaBuilderHandle)context;
}

static IAnjutaBuilderHandle
ibuilder_build (IAnjutaBuilder *builder, const gchar *uri,
	       	IAnjutaBuilderCallback callback, gpointer user_data,
		GError **err)
{
	BasicAutotoolsPlugin *plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (builder);
	BuildContext *context;
	gchar *filename;
	gchar *dirname;
	gchar **argv;
	gchar **args = g_new0 (gchar *, 2);
	GFile* file = g_file_new_for_uri (uri);

	filename = g_file_get_path (file);
	g_object_unref (file);
	if (filename == NULL) return NULL;
	args[0] = g_path_get_basename (filename);
	dirname = g_path_get_dirname (filename);
	g_free (filename);	
	argv = build_argv_command (NULL, CHOOSE_COMMAND (plugin, BUILD), args);

	context = build_execute_command_full (plugin, dirname, argv,
		       				TRUE, TRUE, NULL,
						callback, user_data, err);
	g_free (dirname);
	
	return (IAnjutaBuilderHandle)context;
}

static void
ibuilder_cancel (IAnjutaBuilder *builder, IAnjutaBuilderHandle handle, GError **err)
{
	BasicAutotoolsPlugin *plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (builder);

	build_cancel_command (plugin, (BuildContext *)handle, err);
}

static void
ibuilder_iface_init (IAnjutaBuilderIface *iface)
{
	iface->is_built = ibuilder_is_built;
	iface->build = ibuilder_build;
	iface->cancel = ibuilder_cancel;
}

/* IAnjutaPreferences implementation
 *---------------------------------------------------------------------------*/

static GladeXML *gxml;

static void
ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	GtkWidget *root_check;
	GtkWidget *entry;
		
	/* Create the preferences page */
	gxml = glade_xml_new (GLADE_FILE, BUILD_PREFS_DIALOG, NULL);
	root_check = glade_xml_get_widget(gxml, INSTALL_ROOT_CHECK);
	entry = glade_xml_get_widget(gxml, INSTALL_ROOT_ENTRY);
	g_signal_connect(G_OBJECT(root_check), "toggled", G_CALLBACK(on_root_check_toggled), entry);
	on_root_check_toggled (root_check, entry);
	root_check = glade_xml_get_widget(gxml, PARALLEL_MAKE_CHECK);
	entry = glade_xml_get_widget(gxml, PARALLEL_MAKE_SPIN);
	g_signal_connect(G_OBJECT(root_check), "toggled", G_CALLBACK(on_root_check_toggled), entry);
	on_root_check_toggled (root_check, entry);
	
	anjuta_preferences_add_page (prefs, gxml, BUILD_PREFS_ROOT, _("Build Autotools"),  ICON_FILE);
}

static void
ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	GtkWidget *root_check;
	GtkWidget *entry;

	root_check = glade_xml_get_widget(gxml, INSTALL_ROOT_CHECK);
	entry = glade_xml_get_widget(gxml, INSTALL_ROOT_ENTRY);
	g_signal_handlers_disconnect_by_func(G_OBJECT(root_check),
		G_CALLBACK(on_root_check_toggled), entry);
	root_check = glade_xml_get_widget(gxml, PARALLEL_MAKE_CHECK);
	entry = glade_xml_get_widget(gxml, PARALLEL_MAKE_SPIN);
	g_signal_handlers_disconnect_by_func(G_OBJECT(root_check),
		G_CALLBACK(on_root_check_toggled), entry);
		
	anjuta_preferences_remove_page(prefs, _("Build Autotools"));
	
	g_object_unref (gxml);
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
