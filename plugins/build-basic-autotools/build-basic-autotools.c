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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <config.h>
#include <ctype.h>
#include <pcre.h>

#include <libgnomevfs/gnome-vfs-utils.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-stock.h>
#include <libanjuta/anjuta-launcher.h>
#include <libanjuta/pixmaps.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-buildable.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>

#include "build-basic-autotools.h"

#define ICON_FILE "anjuta-build-basic-autotools-plugin.png"
#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-build-basic-autotools-plugin.ui"
#define MAKE_COMMAND "make -s"

gpointer parent_class;

typedef struct
{
	gchar *pattern;
	int options;
	gchar *replace;
	pcre *regex;
} BuildPattern;

/* Command processing */
typedef struct
{
	gchar *command;
	IAnjutaMessageView *message_view;
	AnjutaLauncher *launcher;
} BuildContext;

static GList *patterns_list = NULL;

static void
build_regex_load ()
{
	FILE *fp;
	
	if (patterns_list)
		return;
	
	fp = fopen (PACKAGE_DATA_DIR"/build/automake-c.filters", "r");
	if (fp == NULL)
	{
		g_warning ("Failed to load filters: %s",
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
			g_warning ("Cannot parse regex: %s", buffer);
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
			g_warning ("PCRE compilarion failed: %s: regex \"%s\" at char %d",
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
			bp->regex,             /* result of pcre_compile() */
			NULL,           /* we didn’t study the pattern */
			details,        /* the subject string */
			strlen (details),/* the length of the subject string */
			0,              /* start at offset 0 in the subject */
			bp->options,              /* default options */
			ovector,        /* vector for substring information */
			30);            /* number of elements in the vector */
	
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
			// g_message ("dodo: %d, %d, %d", idx, ovector[2*idx], ovector[2*idx+1]);
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
on_build_mesg_format (IAnjutaMessageView *view, const gchar *line,
					  AnjutaPlugin *plugin)
{
	gchar *dummy_fn, *summary;
	gint dummy_int;
	IAnjutaMessageViewType type;
	GList *node;
	
	g_return_if_fail (line != NULL);
	
	type = IANJUTA_MESSAGE_VIEW_TYPE_NORMAL;
	if (parse_error_line(line, &dummy_fn, &dummy_int))
	{
		if (strstr (line, _("warning:")) != NULL)
			type = IANJUTA_MESSAGE_VIEW_TYPE_WARNING;
		else
			type = IANJUTA_MESSAGE_VIEW_TYPE_ERROR;
		
		g_free(dummy_fn);
	}
	else if (strstr (line, ":") != NULL)
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
}

static void
on_build_mesg_parse (IAnjutaMessageView *view, const gchar *line,
					 AnjutaPlugin *plugin)
{
	gchar *filename;
	gint lineno;
	if (parse_error_line (line, &filename, &lineno))
	{
		/* Go to file and line number */
		g_message ("Go to %s: %d", filename, lineno);
	}
}

static void
on_build_terminated (AnjutaLauncher *launcher,
					 gint child_pid, gint status, gulong time_taken,
					 BuildContext *context)
{
	gchar *buff1;

	g_signal_handlers_disconnect_by_func (context->launcher,
										  G_CALLBACK (on_build_terminated),
										  context);
	buff1 = g_strdup_printf (_("Total time taken: %lu secs\n"), time_taken);
	if (status)
	{
		ianjuta_message_view_buffer_append (context->message_view,
									 _("Completed ... unsuccessful\n"), NULL);
	}
	else
	{
		ianjuta_message_view_buffer_append (context->message_view,
								   _("Completed ... successful\n"), NULL);
	}
	ianjuta_message_view_buffer_append (context->message_view, buff1, NULL);
	/*
	if (anjuta_preferences_get_int (ANJUTA_PREFERENCES (app->preferences),
									BEEP_ON_BUILD_COMPLETE))
		gdk_beep ();
	*/
	g_free (buff1);
	/* anjuta_update_app_status (TRUE, NULL); */
	
	/* Goto the first error if it exists */
	/* if (anjuta_preferences_get_int (ANJUTA_PREFERENCES (app->preferences),
									"build.option.gotofirst"))
		an_message_manager_next(app->messages);
	*/
}

static void
build_execute_command (BasicAutotoolsPlugin* plugin, const gchar *dir,
					   const gchar *command)
{
	IAnjutaMessageManager *mesg_manager;
	BuildContext *context;
	gchar mname[128];
	static gint message_pane_count = 0;
	
	g_return_if_fail (command != NULL);
	
	build_regex_init();
	
	context = g_new0 (BuildContext, 1);
	
	mesg_manager = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
											   IAnjutaMessageManager, NULL);
	snprintf (mname, 128, _("Build %d"), ++message_pane_count);
	context->message_view =
		ianjuta_message_manager_add_view (mesg_manager, mname,
										  ICON_FILE, NULL);
	g_signal_connect (G_OBJECT (context->message_view), "buffer_flushed",
					  G_CALLBACK (on_build_mesg_format), plugin);
	g_signal_connect (G_OBJECT (context->message_view), "message_clicked",
					  G_CALLBACK (on_build_mesg_parse), plugin);
	
	context->launcher = anjuta_launcher_new ();
	
	g_signal_connect (G_OBJECT (context->launcher), "child-exited",
					  G_CALLBACK (on_build_terminated), context);
	ianjuta_message_view_buffer_append (context->message_view, command, NULL);
	ianjuta_message_view_buffer_append (context->message_view, "\n", NULL);
	chdir (dir);
	anjuta_launcher_execute (context->launcher, command,
							 on_build_mesg_arrived, context);
}

/* UI actions */
static void
build_build_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	build_execute_command (plugin, plugin->project_root_dir, MAKE_COMMAND);
}

static void
build_install_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	build_execute_command (plugin, plugin->project_root_dir,
						   MAKE_COMMAND" install");
}

static void
build_clean_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	build_execute_command (plugin, plugin->project_root_dir,
						   MAKE_COMMAND" clean");
}

static void
build_configure_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	gint response;
	GtkWindow *parent;
	gchar *input = NULL;
	
	parent = GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell);
	response = anjuta_util_dialog_input (parent, _("Configure Parameters:"),
										 &input);
	if (response)
	{
		gchar *cmd;
		if (input)
		{
			cmd = g_strconcat ("./configure ", input, NULL);
			g_free (input);
		}
		else
		{
			cmd = g_strdup ("./configure");
		}
		build_execute_command (plugin, plugin->project_root_dir, cmd);
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
										 &input);
	if (response)
	{
		gchar *cmd;
		if (input)
		{
			cmd = g_strconcat ("./autogen.sh ", input, NULL);
			g_free (input);
		}
		else
		{
			cmd = g_strdup ("./autogen.sh");
		}
		build_execute_command (plugin, plugin->project_root_dir, cmd);
		g_free (cmd);
	}
}

static void
build_distribution_project (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	build_execute_command (plugin, plugin->project_root_dir,
						   MAKE_COMMAND" dist");
}

static void
build_build_module (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	gchar *dirname = g_dirname (plugin->current_editor_filename);
	build_execute_command (plugin, dirname, MAKE_COMMAND);
	g_free (dirname);
}

static void
build_install_module (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	gchar *dirname = g_dirname (plugin->current_editor_filename);
	build_execute_command (plugin, dirname, MAKE_COMMAND" install");
	g_free (dirname);
}

static void
build_clean_module (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	gchar *dirname = g_dirname (plugin->current_editor_filename);
	build_execute_command (plugin, dirname, MAKE_COMMAND" clean");
	g_free (dirname);
}

static void
build_compile_file (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	g_message ("Unimplemented");
}

/* File manager context menu */
static void
fm_compile (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	g_message ("Unimplemented");
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
	build_execute_command (plugin, dir, MAKE_COMMAND);
}

static void
fm_install (GtkAction *action, BasicAutotoolsPlugin *plugin)
{
	gchar *dir;
	
	g_return_if_fail (plugin->fm_current_filename != NULL);
	
	if (g_file_test (plugin->fm_current_filename, G_FILE_TEST_IS_DIR))
		dir = g_strdup (plugin->fm_current_filename);
	else
		dir = g_path_get_dirname (plugin->fm_current_filename);
	build_execute_command (plugin, dir, MAKE_COMMAND" install");
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
	build_execute_command (plugin, dir, MAKE_COMMAND" clean");
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
		N_("Run C_onfigure ..."), NULL,
		N_("Configure project"),
		G_CALLBACK (build_configure_project)
	},
	{
		"ActionBuildAutogen", NULL,
		N_("Run _Autogenerate ..."), NULL,
		N_("Autogenrate project files"),
		G_CALLBACK (build_autogen_project)
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
		"ActionPopupBuild", NULL,
		N_("_Build"), NULL, NULL, NULL
	},
	{
		"ActionPopupBuildCompile", GTK_STOCK_CONVERT,
		N_("_Compile"), NULL,
		N_("Complie file"),
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
	if (!g_file_test (filename, G_FILE_TEST_EXISTS))
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
	
	g_message ("Updateing project UI");
	
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
	if (directory_has_file (bb_plugin->project_root_dir, "autogen.sh"))
	{
		action = anjuta_ui_get_action (ui, "ActionGroupBuild",
									   "ActionBuildAutogen");
		g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
	}
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
	
	g_message ("Updateing module UI");
	
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

	BasicAutotoolsPlugin *ba_plugin = (BasicAutotoolsPlugin*)plugin;
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
	
	action = anjuta_ui_get_action (ui, "ActionGroupBuild", "ActionPopupBuild");
	g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupBuild",
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
	BasicAutotoolsPlugin *ba_plugin = (BasicAutotoolsPlugin*)plugin;
	
	if (ba_plugin->fm_current_filename)
		g_free (ba_plugin->fm_current_filename);
	ba_plugin->fm_current_filename = NULL;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupBuild", "ActionPopupBuild");
	g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
}

static void
value_added_project_root_uri (AnjutaPlugin *plugin, const gchar *name,
							  const GValue *value, gpointer user_data)
{
	BasicAutotoolsPlugin *bb_plugin;
	const gchar *root_uri;

	bb_plugin = (BasicAutotoolsPlugin *) plugin;
	
	g_message ("Project added");
	
	if (bb_plugin->project_root_dir)
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

	bb_plugin = (BasicAutotoolsPlugin *) plugin;
	if (bb_plugin->project_root_dir)
		g_free (bb_plugin->project_root_dir);
	bb_plugin->project_root_dir = NULL;
	update_project_ui (bb_plugin);
}

static void
value_added_current_editor (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	AnjutaUI *ui;
	gchar *uri;
	GObject *editor;
	
	editor = g_value_get_object (value);
	
	BasicAutotoolsPlugin *ba_plugin = (BasicAutotoolsPlugin*)plugin;
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	if (ba_plugin->current_editor_filename)
		g_free (ba_plugin->current_editor_filename);
	ba_plugin->current_editor_filename = NULL;
	
	uri = ianjuta_file_get_uri (IANJUTA_FILE (editor), NULL);
	if (uri)
	{
		gchar *filename;
		
		filename = gnome_vfs_get_local_path_from_uri (uri);
		g_return_if_fail (filename != NULL);
		ba_plugin->current_editor_filename = filename;
		g_free (uri);
		update_module_ui (ba_plugin);
	}
}

static void
value_removed_current_editor (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	BasicAutotoolsPlugin *ba_plugin = (BasicAutotoolsPlugin*)plugin;
	
	if (ba_plugin->current_editor_filename)
		g_free (ba_plugin->current_editor_filename);
	ba_plugin->current_editor_filename = NULL;
	
	update_module_ui (ba_plugin);
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	BasicAutotoolsPlugin *ba_plugin = (BasicAutotoolsPlugin*)plugin;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	/* Add action group */
	ba_plugin->build_action_group = 
		anjuta_ui_add_action_group_entries (ui,
											"ActionGroupBuild",
											_("Build commands"),
											build_actions,
											sizeof(build_actions)/sizeof(GtkActionEntry),
											plugin);
	/* Add UI */
	ba_plugin->build_merge_id = anjuta_ui_merge (ui, UI_FILE);

	update_project_ui (ba_plugin);
	update_module_ui (ba_plugin);
	
	/* Add watches */
	ba_plugin->fm_watch_id = 
		anjuta_plugin_add_watch (plugin, "file_manager_current_uri",
								 value_added_fm_current_uri,
								 value_removed_fm_current_uri, NULL);
	ba_plugin->project_watch_id = 
		anjuta_plugin_add_watch (plugin, "project_root_uri",
								 value_added_project_root_uri,
								 value_removed_project_root_uri, NULL);
	ba_plugin->editor_watch_id = 
		anjuta_plugin_add_watch (plugin, "document_manager_current_editor",
								 value_added_current_editor,
								 value_removed_current_editor, NULL);
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	BasicAutotoolsPlugin *ba_plugin = (BasicAutotoolsPlugin*)plugin;
	
	/* Remove watches */
	anjuta_plugin_remove_watch (plugin, ba_plugin->fm_watch_id, TRUE);
	anjuta_plugin_remove_watch (plugin, ba_plugin->project_watch_id, TRUE);
	anjuta_plugin_remove_watch (plugin, ba_plugin->editor_watch_id, TRUE);
	
	/* Remove UI */
	anjuta_ui_unmerge (ui, ba_plugin->build_merge_id);
	
	/* Remove action group */
	anjuta_ui_remove_action_group (ui, ba_plugin->build_action_group);
	return TRUE;
}

static void
dispose (GObject *obj)
{
}

static void
basic_autotools_plugin_instance_init (GObject *obj)
{
	BasicAutotoolsPlugin *ba_plugin = (BasicAutotoolsPlugin*) obj;
	ba_plugin->fm_current_filename = NULL;
	ba_plugin->project_root_dir = NULL;
	ba_plugin->current_editor_filename = NULL;
}

static void
basic_autotools_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
}

static void
ibuildable_compile (IAnjutaBuildable *manager, const gchar * filename,
					GError **err)
{
}

static void
ibuildable_build (IAnjutaBuildable *manager, GError **err)
{
}

static void
ibuildable_clean (IAnjutaBuildable *manager, GError **err)
{
}

static void
ibuildable_install (IAnjutaBuildable *manager, GError **err)
{
}

static void
ibuildable_iface_init (IAnjutaBuildableIface *iface)
{
	iface->compile = ibuildable_compile;
	iface->build = ibuildable_build;
	iface->clean = ibuildable_clean;
	iface->install = ibuildable_install;
}

ANJUTA_PLUGIN_BEGIN (BasicAutotoolsPlugin, basic_autotools_plugin);
ANJUTA_INTERFACE (ibuildable, IANJUTA_TYPE_BUILDABLE);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (BasicAutotoolsPlugin, basic_autotools_plugin);
