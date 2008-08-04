/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2008 <jrliggett@cox.net>
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

#include "git-command.h"

#define ERROR_REGEX "^(?:warning|fatal|error): (.*)"
#define PROGRESS_REGEX "(\\d{1,3}(?=%))"
#define STATUS_REGEX "(.*):"

enum
{
	PROP_0,
	
	PROP_WORKING_DIRECTORY,
	PROP_SINGLE_LINE_OUTPUT,
	PROP_STRIP_NEWLINES
};

struct _GitCommandPriv
{
	AnjutaLauncher *launcher;
	GList *args;
	size_t num_args;
	gchar *working_directory;
	GRegex *error_regex;
	GRegex *progress_regex;
	GRegex *status_regex;
	GString *error_string;
	GQueue *info_queue;
	gboolean single_line_output;
	gboolean strip_newlines;
};

G_DEFINE_TYPE (GitCommand, git_command, ANJUTA_TYPE_SYNC_COMMAND);

static void
git_command_multi_line_output_arrived (AnjutaLauncher *launcher, 
									   AnjutaLauncherOutputType output_type,
									   const gchar *chars, GitCommand *self)
{	
	GitCommandClass *klass;
	
	klass = GIT_COMMAND_GET_CLASS (self);
	
	switch (output_type)
	{
		case ANJUTA_LAUNCHER_OUTPUT_STDOUT:
			if (klass->output_handler)
				GIT_COMMAND_GET_CLASS (self)->output_handler (self, chars);
			break;
		case ANJUTA_LAUNCHER_OUTPUT_STDERR:
			GIT_COMMAND_GET_CLASS (self)->error_handler (self, chars);
			break;
		default:
			break;
	}
}

/* Split the string up line by line. Works almost like g_strsplit, execpt the
 * newlines can be preserved. */
static gchar **
split_lines (const gchar *string, gboolean strip_newlines)
{
	GList *string_list;
	gchar *string_pos;
	const gchar *remainder;
	gsize length;
	guint n;
	gchar **lines;
	GList *current_line;
	
	string_list = NULL;
	string_pos = strchr (string, '\n');
	remainder = string;
	n = 0;
	
	if (string_pos)
	{
		while (string_pos)
		{	
			if (strip_newlines)
				length = (string_pos - remainder);
			else /* Preserve newline */
				length = ((string_pos + 1) - remainder);
			
			string_list = g_list_prepend (string_list, g_strndup (remainder,
																  length));
			string_pos++; /* Move on to next newline */
			n++;
			
			remainder = string_pos;
			string_pos = strchr (remainder, '\n');
			
		}
	}
	else
	{
		/* If there are no newlines in the string, just return a vector with
		 * one line in it. */
		string_list = g_list_prepend (string_list, g_strdup (string));
		n++;
	}
	
	lines = g_new (gchar *, n + 1);
	lines[n--] = NULL;
	
	for (current_line = string_list; 
		 current_line; 
		 current_line = g_list_next (current_line))
	{
		lines[n--] = current_line->data;
	}
	
	g_list_free (string_list);
	
	return lines;
	
}

static void
git_command_single_line_output_arrived (AnjutaLauncher *launcher, 
										AnjutaLauncherOutputType output_type,
										const gchar *chars, GitCommand *self)
{
	void (*output_handler) (GitCommand *git_command, const gchar *output);
	gchar **lines;
	gchar **current_line;
		
	switch (output_type)
	{
		case ANJUTA_LAUNCHER_OUTPUT_STDOUT:
			output_handler = GIT_COMMAND_GET_CLASS (self)->output_handler;
			break;
		case ANJUTA_LAUNCHER_OUTPUT_STDERR:
			output_handler = GIT_COMMAND_GET_CLASS (self)->error_handler;
			break;
		default:
			output_handler = NULL;
			break;
	}
	
	if (output_handler)
	{
		lines = split_lines (chars, self->priv->strip_newlines);
		
		for (current_line = lines; *current_line; current_line++)
			output_handler (self, *current_line);
		
		g_strfreev (lines);
	}
}

static void
git_command_launch (GitCommand *self)
{
	gchar **args;
	GList *current_arg;
	gint i;
	AnjutaLauncherOutputCallback callback;
	
	args = g_new0 (gchar *, self->priv->num_args + 2);
	current_arg = self->priv->args;
	i = 1;
	
	args[0] = "git";
	
	while (current_arg)
	{
		args[i] = current_arg->data;
		current_arg = g_list_next (current_arg);
		i++;
	}
	
	if (self->priv->single_line_output)
		callback = (AnjutaLauncherOutputCallback) git_command_single_line_output_arrived;
	else
		callback = (AnjutaLauncherOutputCallback) git_command_multi_line_output_arrived;

	chdir (self->priv->working_directory);
	
	if (!anjuta_launcher_execute_v (self->priv->launcher, 
									args,
									NULL,
									callback,
									self))
	{
		git_command_append_error (self, "Command execution failed.");
		anjuta_command_notify_complete (ANJUTA_COMMAND (self), 1);
	}
	
	/* Strings aren't copied; don't free them, just the vector */
	g_free (args);
}

static void
git_command_start (AnjutaCommand *command)
{
	/* We consider the command to be complete when the launcher notifies us of
	 * the child git process's completion, instead of when ::run returns. In 
	 * this case, execute the command if ::run retruns 0. */ 
	if (ANJUTA_COMMAND_GET_CLASS (command)->run (command) == 0)
		git_command_launch (GIT_COMMAND (command));
}

static void
git_command_error_handler (GitCommand *self, const gchar *output)
{
	GMatchInfo *match_info;
	gchar *error;
	gchar *progress;
	gfloat progress_fraction;
	gchar *delimiter; /* Some delimiter that git puts in its output */
	gchar *clean_output; /* Ouput without this delimiter */
	gchar *status;
	
	if (g_regex_match (self->priv->error_regex, output, 0, &match_info))
	{
		error = g_match_info_fetch (match_info, 1);
		g_match_info_free (match_info);
		
		g_string_append (self->priv->error_string, error);
		g_free (error);
	}
	else if (g_regex_match (self->priv->progress_regex, output, 0, &match_info))
	{	
		progress_fraction = 0.0;
		
		/* Make sure not to report 100% progress twice */
		while (g_match_info_matches (match_info) &&
			   progress_fraction < 1.0)
		{
			progress = g_match_info_fetch (match_info, 1);
			progress_fraction = (g_ascii_strtod (progress, NULL) / 100);
			g_free (progress);
			
			anjuta_command_notify_progress (ANJUTA_COMMAND (self),
										    progress_fraction);
			
			g_match_info_next (match_info, NULL);		
		}
		
		g_match_info_free (match_info);
		
		/* Some git versions put the status on a different line; newer ones
		 * in the 1.5 series use the same line, so check for it here. */
		if (g_regex_match (self->priv->status_regex, output, 0, &match_info))
		{	
			status = g_match_info_fetch (match_info, 1);
			git_command_push_info (self, status);
			
			g_free (status);
			g_match_info_free (match_info);
		}
	}
	else
	{
		/* With some commands, like fetch, git will put some kind of 
		 * delimiter in its output, character 0x1b. If it exists, filter it
	     * out. */
		delimiter = strchr (output, 0x1b);
		
		if (delimiter)
		{
			clean_output = g_strndup (output, (delimiter - output));
			git_command_send_output_to_info (self, clean_output);
			g_free (clean_output);
		}
		else
			git_command_send_output_to_info (self, output);
	}
}

static void
git_command_child_exited (AnjutaLauncher *launcher, gint child_pid, gint status, 
						  gulong time, GitCommand *self)
{	
	if (strlen (self->priv->error_string->str) > 0)
	{
		anjuta_command_set_error_message (ANJUTA_COMMAND (self),
										  self->priv->error_string->str);
	}
	
	anjuta_command_notify_complete (ANJUTA_COMMAND (self), 
									(guint) WEXITSTATUS (status));
}

static void
git_command_init (GitCommand *self)
{
	self->priv = g_new0 (GitCommandPriv, 1);
	self->priv->launcher = anjuta_launcher_new ();
	anjuta_launcher_set_encoding (self->priv->launcher, NULL);
	
	g_signal_connect (G_OBJECT (self->priv->launcher), "child-exited",
					  G_CALLBACK (git_command_child_exited),
					  self);
	
	self->priv->error_regex = g_regex_new (ERROR_REGEX, 0, 0, NULL);
	self->priv->progress_regex = g_regex_new (PROGRESS_REGEX, 0, 0, NULL);
	self->priv->status_regex = g_regex_new (STATUS_REGEX, 0, 0, NULL);
	self->priv->error_string = g_string_new ("");
	self->priv->info_queue = g_queue_new ();
}

static void
git_command_finalize (GObject *object)
{
	GitCommand *self;
	GList *current_arg;
	GList *current_info;
	
	self = GIT_COMMAND (object);
	
	current_arg = self->priv->args;
	
	while (current_arg)
	{
		g_free (current_arg->data);
		current_arg = g_list_next (current_arg);
	}
	
	current_info = self->priv->info_queue->head;
	
	while (current_info)
	{
		g_free (current_info->data);
		current_info = g_list_next (current_info);
	}
	
	g_object_unref (self->priv->launcher);
	g_regex_unref (self->priv->error_regex);
	g_regex_unref (self->priv->progress_regex);
	g_regex_unref (self->priv->status_regex);
	g_string_free (self->priv->error_string, TRUE);
	g_queue_free (self->priv->info_queue);
	g_free (self->priv->working_directory);
	g_free (self->priv);
	
	G_OBJECT_CLASS (git_command_parent_class)->finalize (object);
}

static void
git_command_set_property (GObject *object, guint prop_id, const GValue *value,
						  GParamSpec *pspec)
{
	GitCommand *self;
	
	self = GIT_COMMAND (object);
	
	switch (prop_id)
	{
		case PROP_WORKING_DIRECTORY:
			g_free (self->priv->working_directory);
			self->priv->working_directory = g_value_dup_string (value);
			break;
		case PROP_SINGLE_LINE_OUTPUT:
			self->priv->single_line_output = g_value_get_boolean (value);
			break;
		case PROP_STRIP_NEWLINES:
			self->priv->strip_newlines = g_value_get_boolean (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  			break;
	}
}

static void
git_command_get_property (GObject *object, guint prop_id, GValue *value,
						  GParamSpec *pspec)
{
	GitCommand *self;
	
	self = GIT_COMMAND (object);
	
	switch (prop_id)
	{
		case PROP_WORKING_DIRECTORY:
			g_value_set_string (value, self->priv->working_directory);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  			break;
	}
}

static void
git_command_class_init (GitCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaCommandClass* command_class = ANJUTA_COMMAND_CLASS (klass);
	
	object_class->finalize = git_command_finalize;
	object_class->set_property = git_command_set_property;
	object_class->get_property = git_command_get_property;
	command_class->start = git_command_start;
	klass->output_handler = NULL;
	klass->error_handler = git_command_error_handler;
	
	g_object_class_install_property (object_class, PROP_WORKING_DIRECTORY,
									 g_param_spec_string ("working-directory",
														  "",
														  "Directory to run git in.",
														  "",
														  G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
	
	g_object_class_install_property (object_class, PROP_SINGLE_LINE_OUTPUT,
									 g_param_spec_boolean ("single-line-output",
														   "",
														   "If TRUE, output "
														   "handlers are given "
														   "output one line at "
														   "a time.", 
														   FALSE,
														   G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE));
	
	g_object_class_install_property (object_class, PROP_STRIP_NEWLINES,
									 g_param_spec_boolean ("strip-newlines",
														   "",
														   "If TRUE, newlines "
														   "are automatically "
														   "removed from "
														   "single line "
														   "output.", 
														   FALSE,
														   G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE));
}


void
git_command_add_arg (GitCommand *self, const gchar *arg)
{
	self->priv->args = g_list_append (self->priv->args, g_strdup (arg));
	self->priv->num_args++;
}

void 
git_command_add_list_to_args (GitCommand *self, GList *list)
{
	GList *current_arg;
	
	current_arg = list;
	
	while (current_arg)
	{
		self->priv->args = g_list_append (self->priv->args, 
										  g_strdup (current_arg->data));
		self->priv->num_args++;
		
		current_arg = g_list_next (current_arg);
	}
}

void
git_command_append_error (GitCommand *self, const gchar *error_line)
{
	if (strlen (self->priv->error_string->str) > 0)
		g_string_append_printf (self->priv->error_string, "\n%s", error_line);
	else
		g_string_append (self->priv->error_string, error_line);
}

void
git_command_push_info (GitCommand *self, const gchar *info)
{
	g_queue_push_tail (self->priv->info_queue, g_strdup (info));
	anjuta_command_notify_data_arrived (ANJUTA_COMMAND (self));
}

GQueue *
git_command_get_info_queue (GitCommand *self)
{
	return self->priv->info_queue;
}

void
git_command_send_output_to_info (GitCommand *git_command, const gchar *output)
{
	gchar *newline;
	gchar *info_string;

	/* Strip off the newline before sending it to the queue */
	newline = strrchr (output, '\n');
	
	if (newline)
		info_string = g_strndup (output, (newline - output));
	else
		info_string = g_strdup (output);
	
	git_command_push_info (git_command, info_string);
}

GList *
git_command_copy_path_list (GList *list)
{
	GList *current_path;
	GList *new_list;
	
	new_list = NULL;
	current_path = list;
	
	while (current_path)
	{
		new_list = g_list_append (new_list, g_strdup (current_path->data));
		current_path = g_list_next (current_path);
	}
	
	return new_list;
}

void
git_command_free_path_list (GList *list)
{
	GList *current_path;
	
	current_path = list;
	
	while (current_path)
	{
		g_free (current_path->data);
		current_path = g_list_next (current_path);
	}
	
	g_list_free (list);
}
