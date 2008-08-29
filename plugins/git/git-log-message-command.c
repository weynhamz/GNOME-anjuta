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

#define COMMITTER_REGEX "^committer"
#define COMMIT_REGEX "^commit"

#include "git-log-message-command.h"

struct _GitLogMessageCommandPriv
{
	gchar *sha;
	GRegex *committer_regex;
	GRegex *commit_regex;
	GString *log_message;
	gboolean found_committer_line;
	gboolean found_message;
};

G_DEFINE_TYPE (GitLogMessageCommand, git_log_message_command, GIT_TYPE_COMMAND);

static void
git_log_message_command_init (GitLogMessageCommand *self)
{
	self->priv = g_new0 (GitLogMessageCommandPriv, 1);
	
	self->priv->committer_regex = g_regex_new (COMMITTER_REGEX, 0, 0, NULL);
	self->priv->commit_regex = g_regex_new (COMMIT_REGEX, 0, 0, NULL);
	self->priv->log_message = g_string_new ("");
}

static void
git_log_message_command_finalize (GObject *object)
{
	GitLogMessageCommand *self;
	
	self = GIT_LOG_MESSAGE_COMMAND (object);
	
	g_regex_unref (self->priv->committer_regex);
	g_regex_unref (self->priv->commit_regex);
	g_string_free (self->priv->log_message, TRUE);

	G_OBJECT_CLASS (git_log_message_command_parent_class)->finalize (object);
}

static guint
git_log_message_command_run (AnjutaCommand *command)
{
	GitLogMessageCommand *self;
	gchar *revision;

	self = GIT_LOG_MESSAGE_COMMAND (command);
	
	revision = g_strdup_printf ("%s^..%s", self->priv->sha, self->priv->sha);
	
	git_command_add_arg (GIT_COMMAND (command), "rev-list");
	git_command_add_arg (GIT_COMMAND (command), "--pretty=raw");
	git_command_add_arg (GIT_COMMAND (command), revision);
	
	g_free (revision);
	
	return 0;
}

static void
git_log_message_command_handle_output (GitCommand *git_command, 
									   const gchar *output)
{
	GitLogMessageCommand *self;
	
	self = GIT_LOG_MESSAGE_COMMAND (git_command);
	
	/* It is possible that we could encounter multiple objects, usually with
	 * merges. */
	if (g_regex_match (self->priv->commit_regex, output, 0, NULL))
	{
		self->priv->found_message = FALSE;
		self->priv->found_committer_line = FALSE;
	}
	
	if (self->priv->found_message)
		g_string_append (self->priv->log_message, output);
	
	if (self->priv->found_committer_line)
		self->priv->found_message = TRUE;
	else if (g_regex_match (self->priv->committer_regex, output, 0, NULL))
		self->priv->found_committer_line = TRUE; /* Skip next line */
}

static void
git_log_message_command_class_init (GitLogMessageCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GitCommandClass* parent_class = GIT_COMMAND_CLASS (klass);
	AnjutaCommandClass *command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = git_log_message_command_finalize;
	parent_class->output_handler = git_log_message_command_handle_output;
	command_class->run = git_log_message_command_run;
}


GitLogMessageCommand *
git_log_message_command_new (const gchar *working_directory, const gchar *sha)
{
	GitLogMessageCommand *self;
	
	self = g_object_new (GIT_TYPE_LOG_MESSAGE_COMMAND, 
						 "working-directory", working_directory,
						 "single-line-output", TRUE,
						 NULL);
	
	self->priv->sha = g_strdup (sha);
	
	return self;
}

gchar *
git_log_message_command_get_message (GitLogMessageCommand *self)
{
	return g_strdup (g_strchomp (self->priv->log_message->str));
}

