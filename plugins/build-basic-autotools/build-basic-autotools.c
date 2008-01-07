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
#include <pcre.h>

#include <libgnomevfs/gnome-vfs-utils.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-launcher.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>
#include <libanjuta/interfaces/ianjuta-buildable.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <libanjuta/interfaces/ianjuta-indicable.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>

#include "build-basic-autotools.h"
#include "executer.h"

#define ICON_FILE "anjuta-build-basic-autotools-plugin-48.png"
#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-build-basic-autotools-plugin.ui"
#define MAX_BUILD_PANES 3
#define PREF_INDICATORS_AUTOMATIC "indicators.automatic"
#define PREF_INSTALL_ROOT "build.install.root"
#define PREF_INSTALL_ROOT_COMMAND "build.install.root.command"
#define PREF_USE_SB "build.use_scratchbox"
#define PREF_SB_PATH "build.scratchbox.path"

#define CHECK_BUTTON "preferences_toggle:bool:0:0:build.install.root"
#define ENTRY "preferences_entry:text:sudo:0:build.install.root.command"
#define SB_CHECK "preferences_toggle:bool:0:0:build.use_scratchbox"
#define SB_ENTRY "preferences_entry:text:/scratchbox:0:build.scratchbox.path"

#define DEFAULT_COMMAND_COMPILE "make"
#define DEFAULT_COMMAND_BUILD "make"
#define DEFAULT_COMMAND_BUILD_TARBALL "make tarball"
#define DEFAULT_COMMAND_INSTALL "make install"
#define DEFAULT_COMMAND_CONFIGURE "./configure"
#define DEFAULT_COMMAND_GENERATE "./autogen.sh"
#define DEFAULT_COMMAND_CLEAN "make clean"

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
	pcre *regex;
} BuildPattern;

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
	gchar *command;
	IAnjutaMessageView *message_view;
	AnjutaLauncher *launcher;
	GHashTable *build_dir_stack;
	
	/* Indicator locations */
	GSList *locations;
	
	/* Editors in which indicators have been updated */
	GHashTable *indicators_updated_editors;
} BuildContext;

/* Declarations */
static void update_project_ui (BasicAutotoolsPlugin *bb_plugin);
static gboolean directory_has_file (const gchar *dirname, const gchar *filename);

static GList *patterns_list = NULL;

/* Allow installation as root (#321455) */
static void on_root_check_toggled(GtkWidget* toggle_button, GtkWidget* entry)
{
		gtk_widget_set_sensitive(entry, 
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle_button)));
}

static void on_sb_check_toggled(GtkWidget* toggle_button, GtkWidget* entry)
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
	gint line_start, line_end;
	
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

static void
build_context_destroy (BuildContext *context)
{
	if (context->message_view)
		gtk_widget_destroy (GTK_WIDGET (context->message_view));
	if (context->launcher)
	{
		/*
		if (anjuta_launcher_is_busy (context->launcher))
			anjuta_launcher_reset (context->launcher);
		*/
		g_object_unref (context->launcher);
	}
	if (context->build_dir_stack)
		g_hash_table_destroy (context->build_dir_stack);
	if (context->indicators_updated_editors)
		g_hash_table_destroy (context->indicators_updated_editors);
	g_free (context->command);
	
	g_slist_foreach (context->locations, (GFunc) build_indicator_location_free,
					 NULL);
	g_slist_free (context->locations);
	
	g_free (context);
}

static void
build_context_reset (BuildContext *context)
{
	/* Reset context */
	g_free (context->command);
	context->command = NULL;
	
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
build_regex_init ()
{
	GList *node;
	const char *error;
	int erroffset;

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
			pcre_compile(
			   pattern->pattern,
			   pattern->options,
			   &error,           /* for error message */
			   &erroffset,       /* for error offset */
			   NULL);            /* use default character tables */
		if (pattern->regex == NULL) {
			DEBUG_PRINT ("PCRE compilation failed: %s: regex \"%s\" at char %d",
						pattern->pattern, error, erroffset);
		}
		node = g_list_next (node);
	}
}

/* Regex processing */
static gchar*
build_get_summary (const gchar *details, BuildPattern* bp)
{
	int rc;
	int ovector[30];
	const gchar *iter;
	GString *ret;
	gchar *final;

	if (!bp || !bp->regex)
		return NULL;
	
	rc = pcre_exec(
			bp->regex,       /* result of pcre_compile() */
			NULL,            /* we didn’t study the pattern */
			details,         /* the subject string */
			strlen (details),/* the length of the subject string */
			0,               /* start at offset 0 in the subject */
			bp->options,     /* default options */
			ovector,         /* vector for substring information */
			30);             /* number of elements in the vector */
	
	if (rc < 0)
		return NULL;
	
	ret = g_string_new ("");
	iter = bp->replace;
	while (*iter != '\0')
	{
		if (*iter == '\\' && isdigit(*(iter + 1)))
		{
			char temp[2] = {0, 0};
			
			temp[0] = *(iter + 1);
			int idx = atoi (temp);
			ret = g_string_append_len (ret, details + ovector[2*idx],
									   ovector[2*idx+1] - ovector[2*idx]);
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

static gchar* get_real_directory(BuildContext* context, gchar* dir)
{
	AnjutaPreferences* prefs = anjuta_shell_get_preferences (context->plugin->shell, NULL);
	gchar* real_dir = dir;
	if (anjuta_preferences_get_int (prefs, PREF_USE_SB))
	{
			gchar* username = getenv("USERNAME");
			if (!username || strlen(username) == 0)
				username = getenv("USER");
			if (!username || strlen(username) == 0)
				return real_dir;
		
		const gchar* sb_dir = anjuta_preferences_get(prefs, PREF_SB_PATH);
		real_dir = g_strdup_printf("%s/users/%s%s", sb_dir, username, dir);
		g_free(dir);
		DEBUG_PRINT("sb_dir = %s", real_dir);
	}
	return real_dir;
}

static void
on_build_mesg_format (IAnjutaMessageView *view, const gchar *one_line,
					  BuildContext *context)
{
	gchar *dummy_fn, *line;
	gint dummy_int;
	IAnjutaMessageViewType type;
	GList *node;
	gchar* dir = g_new0(gchar, 2048);
	gchar *summary = NULL;
	gchar *freeptr = NULL;
	BasicAutotoolsPlugin *p = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (context->plugin);
	
	g_return_if_fail (one_line != NULL);
	
	/* The translations should match that of 'make' program */
	if ((sscanf (one_line, _("make[%d]: Entering directory '%s'"), &dummy_int, dir) == 2) ||
		(sscanf (one_line, _("make: Entering directory '%s'"), dir) == 1) ||
		(sscanf (one_line, _("make[%d]: Entering directory `%s'"), &dummy_int, dir) == 2) ||
		(sscanf (one_line, _("make: Entering directory `%s'"), dir) == 1))
	{
		gchar* summary;
		/* FIXME: Hack to remove the last ' */
		gchar *idx = strchr (dir, '\'');
		if (idx != NULL)
		{
			*idx = '\0';
		}		
		dir = get_real_directory(context, dir);
		build_context_push_dir (context, "default", dir);
		summary = g_strdup_printf(_("Entering: %s"), dir);
		ianjuta_message_view_append (view, IANJUTA_MESSAGE_VIEW_TYPE_INFO, 
									 summary, one_line, NULL);
		g_free(summary);

		return;
	}
	/* Translation for the following should match that of 'make' program */
	if ((sscanf (one_line, _("make[%d]: Leaving directory '%s'"), &dummy_int, dir) == 2) ||
		(sscanf (one_line, _("make: Leaving directory '%s'"), dir) == 1) ||
		(sscanf (one_line, _("make[%d]: Leaving directory `%s'"), &dummy_int, dir) == 2) ||
		(sscanf (one_line, _("make: Leaving directory `%s'"), dir) == 1))
	{
		gchar* summary;
		/* FIXME: Hack to remove the last ' */
		gchar *idx = strchr (dir, '\'');
		if (idx != NULL)
		{
			*idx = '\0';
		}
		dir = get_real_directory(context, dir);
		build_context_pop_dir (context, "default", dir);
		summary = g_strdup_printf(_("Leaving: %s"), dir);
		ianjuta_message_view_append (view, IANJUTA_MESSAGE_VIEW_TYPE_INFO, 
									 summary, one_line, NULL);
		g_free(summary);
		return;
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
		
		/* Go to file and line number */
		docman = anjuta_shell_get_interface (context->plugin->shell,
											 IAnjutaDocumentManager,
											 NULL);
		
		/* Full path is detected from parse_error_line() */
		ianjuta_document_manager_goto_file_line_mark(docman, filename, lineno, TRUE, NULL);
		g_free(filename);
	}
}

static void
on_build_terminated (AnjutaLauncher *launcher,
					 gint child_pid, gint status, gulong time_taken,
					 BuildContext *context)
{
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
	if (context->launcher)
		g_object_unref (context->launcher);
	context->launcher = NULL;
	update_project_ui (ANJUTA_PLUGIN_BASIC_AUTOTOOLS (context->plugin));
}

static void
build_release_context (BasicAutotoolsPlugin *plugin, BuildContext *context)
{
	plugin->contexts_pool = g_list_remove (plugin->contexts_pool, context);
	build_context_destroy (context);
}

static void
on_message_view_destroyed (BuildContext *context, GtkWidget *view)
{
	DEBUG_PRINT ("Destroying build context");
	context->message_view = NULL;
	build_release_context (ANJUTA_PLUGIN_BASIC_AUTOTOOLS (context->plugin), context);
}

static gboolean
g_hashtable_foreach_true (gpointer key, gpointer value, gpointer user_data)
{
	return TRUE;
}

static BuildContext*
build_get_context (BasicAutotoolsPlugin *plugin, const gchar *dir,
				   const gchar *command)
{
	IAnjutaMessageManager *mesg_manager;
	gchar mname[128];
	gchar *subdir;
	BuildContext *context = NULL;
	static gint message_pane_count = 0;
	
	/* Initialise regex rules */
	build_regex_init();

	subdir = g_path_get_basename (dir);
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
	
	context->command = g_strdup (command);
	context->launcher = anjuta_launcher_new ();
	g_signal_connect (G_OBJECT (context->launcher), "child-exited",
					  G_CALLBACK (on_build_terminated), context);
	build_context_push_dir (context, "default", dir);
	chdir (dir);
	
	plugin->contexts_pool = g_list_append (plugin->contexts_pool, context);
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

static void
build_execute_command (BasicAutotoolsPlugin* bplugin, const gchar *dir,
					   const gchar *command,
					   gboolean save_file)
{
	AnjutaPlugin* plugin = ANJUTA_PLUGIN(bplugin);
	AnjutaPreferences* prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	BuildContext *context;
	gchar* real_command;
	
	g_return_if_fail (command != NULL);

	if (save_file) 
		save_all_files (ANJUTA_PLUGIN (plugin));
	
	context = build_get_context (bplugin, dir, command);
	
	if (anjuta_preferences_get_int (prefs , PREF_USE_SB))
	{
		const gchar* sb_path = anjuta_preferences_get(prefs, PREF_SB_PATH);
		/* we need to skip the /scratchbox/users part, maybe could be done more clever */
		const gchar* real_dir = strstr(dir, "/home");
		real_command = g_strdup_printf("%s/login -d %s \"%s\"", sb_path,
									  real_dir, command);
	}
	else
	{
		real_command = g_strdup(command);
	}
	
	ianjuta_message_view_buffer_append (context->message_view,
										"Building in directory: ", NULL);
	ianjuta_message_view_buffer_append (context->message_view, dir, NULL);
	ianjuta_message_view_buffer_append (context->message_view, "\n", NULL);
	ianjuta_message_view_buffer_append (context->message_view, command, NULL);
	ianjuta_message_view_buffer_append (context->message_view, "\n", NULL);
	
	anjuta_launcher_execute (context->launcher, real_command,
							 on_build_mesg_arrived, context);
	g_free(real_command);
}

static gboolean
build_compile_file_real (BasicAutotoolsPlugin *plugin, const gchar *file)
{
	gchar *file_basename;
	gchar *file_dirname;
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
	
	file_basename = g_path_get_basename (file);
	file_dirname = g_path_get_dirname (file);
	ext_ptr = strrchr (file_basename, '.');
	if (ext_ptr)
	{
		const gchar *new_ext;
		new_ext = g_hash_table_lookup (target_ext, ext_ptr);
		if (new_ext)
		{
			gchar *command;
			
			*ext_ptr = '\0';
			command = g_strconcat (CHOOSE_COMMAND (plugin, COMPILE), " ",
								   file_basename, new_ext, NULL);
			build_execute_command (plugin, file_dirname, command, TRUE);
			g_free (command);
			ret = TRUE;
		}
	} else {
		/* If file has no extension, take it as target itself */
		gchar *command;
		command = g_strconcat (CHOOSE_COMMAND(plugin, COMPILE), " ",
							   file_basename, NULL);
		build_execute_command (plugin, file_dirname, command, TRUE);
		g_free (command);
		ret = TRUE;
	}
	g_free (file_basename);
	g_free (file_dirname);
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
	build_execute_command (plugin, plugin->project_root_dir,
						   CHOOSE_COMMAND (plugin, BUILD), TRUE);
}

static void
build_install_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	gchar* root = get_root_install_command(plugin);
	gchar* command = g_strdup_printf("%s %s", root,
									 CHOOSE_COMMAND (plugin, INSTALL));
	g_free(root);
	build_execute_command (plugin, plugin->project_root_dir,
						   command, TRUE);
	g_free(command);
}

static void
build_clean_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	build_execute_command (plugin, plugin->project_root_dir,
						   CHOOSE_COMMAND (plugin, CLEAN), FALSE);
}

static void
build_configure_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	gint response;
	GtkWindow *parent;
	gchar *input = NULL;
	
	parent = GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell);
	response = anjuta_util_dialog_input (parent, _("Configure Parameters:"),
										 plugin->configure_args, &input);
	if (response)
	{
		gchar *cmd;
		if (input)
		{
			cmd = g_strdup_printf ("%s %s", CHOOSE_COMMAND (plugin, CONFIGURE),
								   input);
			g_free (plugin->configure_args);
			plugin->configure_args = input;
		}
		else
		{
			cmd = g_strdup (CHOOSE_COMMAND (plugin, CONFIGURE));
		}
		build_execute_command (plugin, plugin->project_root_dir, cmd, TRUE);
		g_free (cmd);
	}
}

static void
build_autogen_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	gint response;
	GtkWindow *parent;
	gchar *input = NULL;
	
	parent = GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell);
	response = anjuta_util_dialog_input (parent, _("Configure Parameters:"),
										 plugin->configure_args, &input);
	if (response)
	{
		gboolean has_autogen = directory_has_file (plugin->project_root_dir,
												   "autogen.sh");
		gchar *cmd;
		if (input)
		{
			if (has_autogen)
				cmd = g_strdup_printf ("%s %s",
									   CHOOSE_COMMAND (plugin, GENERATE),
									   input);
			else /* FIXME: Get override command for this too */
				cmd = g_strconcat ("autoreconf -i --force ", input, NULL);
			g_free (plugin->configure_args);
			plugin->configure_args = input;
		}
		else
		{
			if (has_autogen)
				cmd = g_strdup (CHOOSE_COMMAND (plugin, GENERATE));
			else /* FIXME: Get override command for this too */
				cmd = g_strdup ("autoreconf -i --force");
		}
		build_execute_command (plugin, plugin->project_root_dir, cmd, TRUE);
		g_free (cmd);
	}
}

static void
build_distribution_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	build_execute_command (plugin, plugin->project_root_dir,
						   CHOOSE_COMMAND (plugin, BUILD_TARBALL),
						   FALSE);
}

static void
build_execute_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	execute_program (plugin, NULL);
}

static void
build_build_module (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	gchar *dirname = g_dirname (plugin->current_editor_filename);
	build_execute_command (plugin, dirname, CHOOSE_COMMAND (plugin, BUILD),
						   TRUE);
	g_free (dirname);
}

static void
build_install_module (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	gchar *dirname = g_dirname (plugin->current_editor_filename);
	gchar* root = get_root_install_command(plugin);
	gchar* command = g_strdup_printf ("%s %s", root,
									  CHOOSE_COMMAND (plugin, INSTALL));
	g_free(root);
	build_execute_command (plugin, dirname,
						   command, TRUE);
	g_free(command);
	g_free (dirname);
}

static void
build_clean_module (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	gchar *dirname = g_dirname (plugin->current_editor_filename);
	build_execute_command (plugin, dirname, CHOOSE_COMMAND (plugin, CLEAN),
						   FALSE);
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
	
	g_return_if_fail (plugin->fm_current_filename != NULL);
	
	if (g_file_test (plugin->fm_current_filename, G_FILE_TEST_IS_DIR))
		dir = g_strdup (plugin->fm_current_filename);
	else
		dir = g_path_get_dirname (plugin->fm_current_filename);
	build_execute_command (plugin, dir, CHOOSE_COMMAND (plugin, BUILD), TRUE);
}

static void
fm_install (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	gchar *dir;
	gchar* root; 
	gchar* command; 
	
	g_return_if_fail (plugin->fm_current_filename != NULL);
	
	
	if (g_file_test (plugin->fm_current_filename, G_FILE_TEST_IS_DIR))
		dir = g_strdup (plugin->fm_current_filename);
	else
		dir = g_path_get_dirname (plugin->fm_current_filename);
	root = get_root_install_command(plugin);
	command = g_strdup_printf ("%s %s", root,
							   CHOOSE_COMMAND (plugin, INSTALL));
	g_free(root);
	build_execute_command (plugin, dir, command, TRUE);
}

static void
fm_clean (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	gchar *dir;
	
	g_return_if_fail (plugin->fm_current_filename != NULL);
	
	if (g_file_test (plugin->fm_current_filename, G_FILE_TEST_IS_DIR))
		dir = g_strdup (plugin->fm_current_filename);
	else
		dir = g_path_get_dirname (plugin->fm_current_filename);
	build_execute_command (plugin, dir, CHOOSE_COMMAND (plugin, CLEAN), FALSE);
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
	
	g_return_if_fail (plugin->pm_current_filename != NULL);
	
	if (g_file_test (plugin->pm_current_filename, G_FILE_TEST_IS_DIR))
		dir = g_strdup (plugin->pm_current_filename);
	else
		dir = g_path_get_dirname (plugin->pm_current_filename);
	build_execute_command (plugin, dir, CHOOSE_COMMAND (plugin, BUILD), TRUE);
}

static void
pm_install (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	gchar *dir;
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
	build_execute_command (plugin, dir, command, TRUE);
	g_free(command);
}

static void
pm_clean (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	gchar *dir;
	
	g_return_if_fail (plugin->pm_current_filename != NULL);
	
	if (g_file_test (plugin->pm_current_filename, G_FILE_TEST_IS_DIR))
		dir = g_strdup (plugin->pm_current_filename);
	else
		dir = g_path_get_dirname (plugin->pm_current_filename);
	build_execute_command (plugin, dir, CHOOSE_COMMAND (plugin, CLEAN), FALSE);
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
		N_("Run C_onfigure..."), NULL,
		N_("Configure project"),
		G_CALLBACK (build_configure_project)
	},
	{
		"ActionBuildAutogen", NULL,
		N_("Run _Autogenerate..."), NULL,
		N_("Autogenerate project files"),
		G_CALLBACK (build_autogen_project)
	},
	{
		"ActionBuildDistribution", NULL,
		N_("Build _Tarball"), NULL,
		N_("Build project tarball distribution"),
		G_CALLBACK (build_distribution_project)
	},
	{
		"ActionBuildExecute", NULL,
		N_("_Execute Program..."), "F3",
		N_("Execute program"),
		G_CALLBACK (build_execute_project)
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

static void
update_project_ui (BasicAutotoolsPlugin *bb_plugin)
{
	AnjutaUI *ui;
	GtkAction *action;
	
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (bb_plugin)->shell, NULL);
	
	DEBUG_PRINT ("Updateing project UI");
	
	if (!bb_plugin->project_root_dir)
	{
		action = anjuta_ui_get_action (ui, "ActionGroupBuild",
									   "ActionBuildBuildProject");
		g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
		action = anjuta_ui_get_action (ui, "ActionGroupBuild",
									   "ActionBuildInstallProject");
		g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
		action = anjuta_ui_get_action (ui, "ActionGroupBuild",
									   "ActionBuildCleanProject");
		g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
		action = anjuta_ui_get_action (ui, "ActionGroupBuild",
									   "ActionBuildDistribution");
		g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
		action = anjuta_ui_get_action (ui, "ActionGroupBuild",
									   "ActionBuildConfigure");
		g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
		action = anjuta_ui_get_action (ui, "ActionGroupBuild",
									   "ActionBuildAutogen");
		g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
		return;
	}
	if (directory_has_makefile (bb_plugin->project_root_dir))
	{
		action = anjuta_ui_get_action (ui, "ActionGroupBuild",
									   "ActionBuildBuildProject");
		g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
		action = anjuta_ui_get_action (ui, "ActionGroupBuild",
									   "ActionBuildInstallProject");
		g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
		action = anjuta_ui_get_action (ui, "ActionGroupBuild",
									   "ActionBuildCleanProject");
		g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
		action = anjuta_ui_get_action (ui, "ActionGroupBuild",
									   "ActionBuildDistribution");
		g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
	}			
	if (directory_has_file (bb_plugin->project_root_dir, "configure"))
	{
		action = anjuta_ui_get_action (ui, "ActionGroupBuild",
									   "ActionBuildConfigure");
		g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
	}
	action = anjuta_ui_get_action (ui, "ActionGroupBuild",
								   "ActionBuildAutogen");
	g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
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
	gchar *dirname;
	gchar *module;
	gchar *filename;
	gchar *label;

	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (bb_plugin)->shell, NULL);
	
	DEBUG_PRINT ("Updateing module UI");
	
	if (!bb_plugin->current_editor_filename)
	{
		action = anjuta_ui_get_action (ui, "ActionGroupBuild",
									   "ActionBuildBuildModule");
		g_object_set (G_OBJECT (action), "sensitive", FALSE,
					  "label", _("_Build"), NULL);
		action = anjuta_ui_get_action (ui, "ActionGroupBuild",
									   "ActionBuildInstallModule");
		g_object_set (G_OBJECT (action), "sensitive", FALSE,
					  "label", _("_Install"), NULL);
		action = anjuta_ui_get_action (ui, "ActionGroupBuild",
									   "ActionBuildCleanModule");
		g_object_set (G_OBJECT (action), "sensitive", FALSE,
					  "label", _("_Clean"), NULL);
		action = anjuta_ui_get_action (ui, "ActionGroupBuild",
									   "ActionBuildCompileFile");
		g_object_set (G_OBJECT (action), "sensitive", FALSE,
					  "label", _("Co_mpile"), NULL);
		return;
	}
	
	dirname = g_dirname (bb_plugin->current_editor_filename);
	module = escape_label (g_basename (dirname));
	filename = escape_label (g_basename (bb_plugin->current_editor_filename));
	if (directory_has_makefile (dirname))
	{
		label = g_strdup_printf (_("_Build (%s)"), module);
		action = anjuta_ui_get_action (ui, "ActionGroupBuild",
									   "ActionBuildBuildModule");
		g_object_set (G_OBJECT (action), "sensitive", TRUE,
					  /*"label", label, */NULL);
		g_free (label);
		
		label = g_strdup_printf (_("_Install (%s)"), module);
		action = anjuta_ui_get_action (ui, "ActionGroupBuild",
									   "ActionBuildInstallModule");
		g_object_set (G_OBJECT (action), "sensitive", TRUE,
					  /* "label", label, */NULL);
		g_free (label);
		
		label = g_strdup_printf (_("_Clean (%s)"), module);
		action = anjuta_ui_get_action (ui, "ActionGroupBuild",
									   "ActionBuildCleanModule");
		g_object_set (G_OBJECT (action), "sensitive", TRUE,
					  /* "label", label,*/ NULL);
		g_free (label);
	}
	label = g_strdup_printf (_("Co_mpile (%s)"), filename);
	action = anjuta_ui_get_action (ui, "ActionGroupBuild",
								   "ActionBuildCompileFile");
	g_object_set (G_OBJECT (action), "sensitive", TRUE,
				  /* "label", label, */ NULL);
	g_free (label);
	g_free (module);
	g_free (filename);
	g_free (dirname);
}

static void
on_session_save (AnjutaShell *shell, AnjutaSessionPhase phase,
				 AnjutaSession *session, BasicAutotoolsPlugin *plugin)
{
	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;
	if (plugin->program_args)
		anjuta_session_set_string (session, "Execution",
								   "Program arguments", plugin->program_args);
	anjuta_session_set_int (session, "Execution", "Run in terminal",
							plugin->run_in_terminal + 1);
	if (plugin->last_exec_uri)
	{
		anjuta_session_set_string (session, "Execution",
								   "Last selected uri", plugin->last_exec_uri);
	}		
	
	if (plugin->configure_args)
		anjuta_session_set_string (session, "Build",
								   "Configure parameters",
								   plugin->configure_args);
}

static void
on_session_load (AnjutaShell *shell, AnjutaSessionPhase phase,
				 AnjutaSession *session, BasicAutotoolsPlugin *plugin)
{
	gchar *program_args, *configure_args, *last_uri;
	gint run_in_terminal;
				
	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;
	
	program_args = anjuta_session_get_string (session, "Execution",
											  "Program arguments");
	if (program_args)
	{
		g_free (plugin->program_args);
		plugin->program_args = program_args;
	}
	
	configure_args = anjuta_session_get_string (session, "Build",
											  "Configure parameters");
	if (configure_args)
	{
		g_free (plugin->configure_args);
		plugin->configure_args = configure_args;
	}
	
	last_uri = anjuta_session_get_string (session, "Execution",
								   "Last selected uri");
	
	if (last_uri)
	{
		plugin->last_exec_uri = last_uri;
	}
	
	/* The flag is store as 1 == FALSE, 2 == TRUE */
	run_in_terminal =
		anjuta_session_get_int (session, "Execution", "Run in terminal");
	run_in_terminal--;
	
	if (run_in_terminal >= 0)
		plugin->run_in_terminal = run_in_terminal;
}

static void
value_added_fm_current_uri (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	AnjutaUI *ui;
	GtkAction *action;
	const gchar *uri;
	gchar *dirname, *filename;
	gboolean makefile_exists, is_dir;
	
	uri = g_value_get_string (value);
	filename = gnome_vfs_get_local_path_from_uri (uri);
	g_return_if_fail (filename != NULL);

	BasicAutotoolsPlugin *ba_plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (plugin);
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	if (ba_plugin->fm_current_filename)
		g_free (ba_plugin->fm_current_filename);
	ba_plugin->fm_current_filename = filename;
	
	is_dir = g_file_test (filename, G_FILE_TEST_IS_DIR);
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
value_removed_fm_current_uri (AnjutaPlugin *plugin,
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
	
	uri = g_value_get_string (value);
	filename = gnome_vfs_get_local_path_from_uri (uri);
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
	
	DEBUG_PRINT ("Project added");
	
	g_free (bb_plugin->project_root_dir);
	bb_plugin->project_root_dir = NULL;
	
	root_uri = g_value_get_string (value);
	if (root_uri)
	{
		bb_plugin->project_root_dir =
			gnome_vfs_get_local_path_from_uri (root_uri);
		if (bb_plugin->project_root_dir)
		{
			update_project_ui (bb_plugin);
		}
	}
}

static void
value_removed_project_root_uri (AnjutaPlugin *plugin, const gchar *name,
								gpointer user_data)
{
	BasicAutotoolsPlugin *bb_plugin;

	bb_plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (plugin);
	
	g_free (bb_plugin->project_root_dir);
	g_free (bb_plugin->program_args);
	g_free (bb_plugin->configure_args);
	
	bb_plugin->run_in_terminal = TRUE;
	bb_plugin->program_args = NULL;
	bb_plugin->configure_args = NULL;
	bb_plugin->project_root_dir = NULL;
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
on_editor_changed (IAnjutaEditor *editor, gint position, gboolean added,
				   gint length, gint lines, const gchar *text,
				   BasicAutotoolsPlugin *ba_plugin)
{
	gint line, begin_pos, end_pos;
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
	DEBUG_PRINT ("Editor changed: position = %d, added = %d,"
				 " length = %d, lines = %d, text = \'%s\'",
				 position, added, length, lines, text);
}

static void
value_added_current_editor (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	AnjutaUI *ui;
	gchar *uri;
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
	
	uri = ianjuta_file_get_uri (IANJUTA_FILE (editor), NULL);
	if (uri)
	{
		gchar *filename;
		
		filename = gnome_vfs_get_local_path_from_uri (uri);
		g_free (uri);
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

	update_project_ui (ba_plugin);
	update_module_ui (ba_plugin);
	
	/* Add watches */
	ba_plugin->fm_watch_id = 
		anjuta_plugin_add_watch (plugin, "file_manager_current_uri",
								 value_added_fm_current_uri,
								 value_removed_fm_current_uri, NULL);
	ba_plugin->pm_watch_id = 
		anjuta_plugin_add_watch (plugin, "project_manager_current_uri",
								 value_added_pm_current_uri,
								 value_removed_pm_current_uri, NULL);
	ba_plugin->project_watch_id = 
		anjuta_plugin_add_watch (plugin, "project_root_uri",
								 value_added_project_root_uri,
								 value_removed_project_root_uri, NULL);
	ba_plugin->editor_watch_id = 
		anjuta_plugin_add_watch (plugin, "document_manager_current_editor",
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
	anjuta_plugin_remove_watch (plugin, ba_plugin->project_watch_id, TRUE);
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
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (G_OBJECT(obj)));
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
	g_free (ba_plugin->current_editor_filename);
	g_free (ba_plugin->program_args);
	g_free (ba_plugin->configure_args);
	
	ba_plugin->fm_current_filename = NULL;
	ba_plugin->pm_current_filename = NULL;
	ba_plugin->project_root_dir = NULL;
	ba_plugin->current_editor_filename = NULL;
	ba_plugin->program_args = NULL;
	ba_plugin->configure_args = NULL;
	
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (G_OBJECT(obj)));
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
	ba_plugin->current_editor = NULL;
	ba_plugin->current_editor_filename = NULL;
	ba_plugin->contexts_pool = NULL;
	ba_plugin->configure_args = NULL;
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
	build_execute_command (plugin, directory, CHOOSE_COMMAND (plugin, BUILD),
						   TRUE);
}

static void
ibuildable_clean (IAnjutaBuildable *manager, const gchar *directory,
				  GError **err)
{
	BasicAutotoolsPlugin *plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (manager);
	build_execute_command (plugin, directory, CHOOSE_COMMAND (plugin, CLEAN),
						   FALSE);
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
	build_execute_command (plugin, directory, command, TRUE);
	g_free(command);
}

static void
ibuildable_configure (IAnjutaBuildable *manager, const gchar *directory,
					  GError **err)
{
	BasicAutotoolsPlugin *plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (manager);
	build_execute_command (plugin, directory,
						   CHOOSE_COMMAND (plugin, CONFIGURE),
						   TRUE);
}

static void
ibuildable_generate (IAnjutaBuildable *manager, const gchar *directory,
					 GError **err)
{
	BasicAutotoolsPlugin *plugin = ANJUTA_PLUGIN_BASIC_AUTOTOOLS (manager);
	if (directory_has_file (plugin->project_root_dir, "autogen.sh"))
	{
		build_execute_command (plugin, directory,
							   CHOOSE_COMMAND (plugin, GENERATE), FALSE);
	}
	else
	{
		/* FIXME: get override command for this too */
		build_execute_command (plugin, directory,
							   "autoreconf -i --force", FALSE);
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
ifile_open (IAnjutaFile *manager, const gchar *uri,
			GError **err)
{
	ianjuta_buildable_execute (IANJUTA_BUILDABLE (manager), uri, NULL);
}

static gchar*
ifile_get_uri (IAnjutaFile *manager, GError **err)
{
	DEBUG_PRINT ("Unsupported operation");
	return NULL;
}

static void
ifile_iface_init (IAnjutaFileIface *iface)
{
	iface->open = ifile_open;
	iface->get_uri = ifile_get_uri;
}

static GladeXML *gxml;

static void
ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	GtkWidget *root_check;
	GtkWidget *sb_check;
	GtkWidget *entry;
	GtkWidget *sb_entry;
		
	/* Create the preferences page */
	gxml = glade_xml_new (GLADE_FILE, "preferences_dialog_build", NULL);
	root_check = glade_xml_get_widget(gxml, CHECK_BUTTON);
	sb_check = glade_xml_get_widget(gxml, SB_CHECK);
	entry = glade_xml_get_widget(gxml, ENTRY);
	sb_entry = glade_xml_get_widget(gxml, SB_ENTRY);
	
	g_signal_connect(G_OBJECT(root_check), "toggled", G_CALLBACK(on_root_check_toggled), entry);
	g_signal_connect(G_OBJECT(sb_check), "toggled", G_CALLBACK(on_sb_check_toggled), sb_entry);
	
	
	anjuta_preferences_add_page (prefs, gxml, "Build", _("Build Autotools"),  ICON_FILE);
}

static void
ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	GtkWidget *root_check;
	GtkWidget *sb_check;
	GtkWidget *entry;
	GtkWidget *sb_entry;

	root_check = glade_xml_get_widget(gxml, CHECK_BUTTON);
	sb_check = glade_xml_get_widget(gxml, SB_CHECK);
	entry = glade_xml_get_widget(gxml, ENTRY);
	sb_entry = glade_xml_get_widget(gxml, SB_ENTRY);
	g_signal_handlers_disconnect_by_func(G_OBJECT(root_check),
		G_CALLBACK(on_root_check_toggled), entry);
	g_signal_handlers_disconnect_by_func(G_OBJECT(root_check),
		G_CALLBACK(on_sb_check_toggled), sb_entry);
		
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
ANJUTA_PLUGIN_ADD_INTERFACE (ibuildable, IANJUTA_TYPE_BUILDABLE);
ANJUTA_PLUGIN_ADD_INTERFACE (ifile, IANJUTA_TYPE_FILE);
ANJUTA_PLUGIN_ADD_INTERFACE (ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (BasicAutotoolsPlugin, basic_autotools_plugin);
