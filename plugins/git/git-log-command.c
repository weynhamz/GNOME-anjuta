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

#include "git-log-command.h"

struct _GitLogCommandPriv
{
	GitLogDataCommand *data_command;
	guint return_code;

	gchar *branch;
	gchar *path;
	
	/* Filters */
	gchar *author;
	gchar *grep;
	gchar *since_date;
	gchar *until_date;
	gchar *since_commit;
	gchar *until_commit;
};

G_DEFINE_TYPE (GitLogCommand, git_log_command, GIT_TYPE_COMMAND);

static void
on_data_command_data_arrived (AnjutaCommand *command, GitLogCommand *self)
{
	anjuta_command_notify_data_arrived (command);
}

static void
on_data_command_finished (AnjutaCommand *command, guint return_code,
                          GitLogCommand *self)
{
	ANJUTA_COMMAND_CLASS (git_log_command_parent_class)->notify_complete (ANJUTA_COMMAND (self),
	                                                                      self->priv->return_code);
}

static void
git_log_command_init (GitLogCommand *self)
{
	self->priv = g_new0 (GitLogCommandPriv, 1);

	self->priv->data_command = git_log_data_command_new ();

	g_signal_connect_object (G_OBJECT (self->priv->data_command), "data-arrived",
	                         G_CALLBACK (on_data_command_data_arrived),
	                         self, 0);

	g_signal_connect_object (G_OBJECT (self->priv->data_command), "command-finished",
	                         G_CALLBACK (on_data_command_finished),
	                         self, 0);
}

static void
git_log_command_finalize (GObject *object)
{
	GitLogCommand *self;
	
	self = GIT_LOG_COMMAND (object);
	
	g_object_unref (self->priv->data_command);
	g_free (self->priv->author);
	g_free (self->priv->grep);
	g_free (self->priv->since_date);
	g_free (self->priv->until_date);
	g_free (self->priv->since_commit);
	g_free (self->priv->until_commit);
	g_free (self->priv);

	G_OBJECT_CLASS (git_log_command_parent_class)->finalize (object);
}

static guint
git_log_command_run (AnjutaCommand *command)
{
	GitLogCommand *self;
	gchar *filter_arg;
	GString *commit_range;
	
	self = GIT_LOG_COMMAND (command);
	
	git_command_add_arg (GIT_COMMAND (command), "rev-list");
	git_command_add_arg (GIT_COMMAND (command), "--topo-order");
	git_command_add_arg (GIT_COMMAND (command), "--pretty=format:parents %P%n"
												"author %an%n"
												"time %at%n"
												"short log %s%n"
												"\x0c");
	
	if (self->priv->author)
	{
		filter_arg = g_strdup_printf ("--author=%s", self->priv->author);
		git_command_add_arg (GIT_COMMAND (command), filter_arg);
		g_free (filter_arg);
	}
	
	if (self->priv->grep)
	{
		filter_arg = g_strdup_printf ("--grep=%s", self->priv->grep);
		git_command_add_arg (GIT_COMMAND (command), filter_arg);
		g_free (filter_arg);
	}
	
	if (self->priv->since_date)
	{
		filter_arg = g_strdup_printf ("--since=%s", self->priv->since_date);
		git_command_add_arg (GIT_COMMAND (command), filter_arg);
		g_free (filter_arg);
	}
	
	if (self->priv->until_date)
	{
		filter_arg = g_strdup_printf ("--until=%s", self->priv->until_date);
		git_command_add_arg (GIT_COMMAND (command), filter_arg);
		g_free (filter_arg);
	}
	
	if (self->priv->since_commit || self->priv->until_commit)
	{
		commit_range = g_string_new ("");
		
		/* Not the most elegant way of doing it... */
		if (self->priv->since_commit)
			g_string_append (commit_range, self->priv->since_commit);
		
		g_string_append (commit_range, "..");
		
		if (self->priv->until_commit)
			g_string_append (commit_range, self->priv->until_commit);
		
		git_command_add_arg (GIT_COMMAND (command), commit_range->str);
		
		g_string_free (commit_range, TRUE);
	}

	if (self->priv->branch)
		git_command_add_arg (GIT_COMMAND (command), self->priv->branch);
	else
		git_command_add_arg (GIT_COMMAND (command), "HEAD");
	
	if (self->priv->path)
	{
		git_command_add_arg (GIT_COMMAND (command), "--");
		git_command_add_arg (GIT_COMMAND (command), self->priv->path);
	}

	/* Start the data processing task */
	anjuta_command_start (ANJUTA_COMMAND (self->priv->data_command));
	
	return 0;
}

static void
git_log_command_notify_complete (AnjutaCommand *command, guint return_code)
{
	GitLogCommand *self;

	self = GIT_LOG_COMMAND (command);

	/* Send an empty string to the data processing command so that it knows
	 * to stop when it's done processing data. The command will finish when 
	 * the processing thread finishes, and not when git stops executing */
	git_log_data_command_push_line (self->priv->data_command, "");

	/* Use the git return code */
	self->priv->return_code = return_code;
}

static void
git_log_command_handle_output (GitCommand *git_command, const gchar *output)
{
	GitLogCommand *self;
	
	self = GIT_LOG_COMMAND (git_command);

	git_log_data_command_push_line (self->priv->data_command, output);
}

static void
git_log_command_class_init (GitLogCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GitCommandClass* parent_class = GIT_COMMAND_CLASS (klass);
	AnjutaCommandClass *command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = git_log_command_finalize;
	parent_class->output_handler = git_log_command_handle_output;
	command_class->run = git_log_command_run;
	command_class->notify_complete = git_log_command_notify_complete;
}


GitLogCommand *
git_log_command_new (const gchar *working_directory,
					 const gchar *branch, const gchar *path,
					 const gchar *author, const gchar *grep,
					 const gchar *since_date, const gchar *until_date,
					 const gchar *since_commit,
					 const gchar *until_commit)
{
	GitLogCommand *self;
	
	self = g_object_new (GIT_TYPE_LOG_COMMAND, 
						 "working-directory", working_directory,
						 "single-line-output", TRUE,
						 NULL);
	
	self->priv->author = g_strdup (author);
	self->priv->path = g_strdup (path);
	self->priv->branch = g_strdup (branch);
	self->priv->grep = g_strdup (grep);
	self->priv->since_date = g_strdup (since_date);
	self->priv->until_date = g_strdup (until_date);
	self->priv->since_commit = g_strdup (since_commit);
	self->priv->until_commit = g_strdup (until_commit);
	
	return self;
}

GQueue *
git_log_command_get_output_queue (GitLogCommand *self)
{
	return git_log_data_command_get_output (self->priv->data_command);
}
