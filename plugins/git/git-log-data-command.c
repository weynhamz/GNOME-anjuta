/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2010 <jrliggett@cox.net>
 * 
 * anjuta is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "git-log-data-command.h"

#define COMMIT_REGEX "^commit ([[:xdigit:]]{40})"
#define PARENT_REGEX "^parents (.*)"
#define AUTHOR_REGEX "^author (.*)"
#define TIME_REGEX "^time (\\d*)"
#define SHORT_LOG_REGEX "^(?:short log) (.*)"

struct _GitLogDataCommandPriv
{
	GAsyncQueue *input_queue;
	GQueue *output_queue;
	GHashTable *revisions;
	GitRevision *current_revision;
	GRegex *commit_regex;
	GRegex *parent_regex;
	GRegex *author_regex;
	GRegex *time_regex;
	GRegex *short_log_regex;
};

G_DEFINE_TYPE (GitLogDataCommand, git_log_data_command, 
               ANJUTA_TYPE_ASYNC_COMMAND);

static void
git_log_data_command_init (GitLogDataCommand *self)
{
	self->priv = g_new0 (GitLogDataCommandPriv, 1);
	self->priv->input_queue = g_async_queue_new_full (g_free);
	self->priv->output_queue = g_queue_new ();
	self->priv->revisions = g_hash_table_new_full (g_str_hash, g_str_equal,
												   g_free, g_object_unref);
	self->priv->commit_regex = g_regex_new (COMMIT_REGEX, 0, 0, NULL);
	self->priv->parent_regex = g_regex_new (PARENT_REGEX, 0, 0, NULL);
	self->priv->author_regex = g_regex_new (AUTHOR_REGEX, 0, 0, NULL);
	self->priv->time_regex = g_regex_new (TIME_REGEX, 0, 0, NULL);
	self->priv->short_log_regex = g_regex_new (SHORT_LOG_REGEX, 0, 0, NULL);
}

static void
git_log_data_command_finalize (GObject *object)
{
	GitLogDataCommand *self;
	GList *current_output;
	
	self = GIT_LOG_DATA_COMMAND (object);

	g_async_queue_unref (self->priv->input_queue);
	current_output = self->priv->output_queue->head;
	
	while (current_output)
	{
		g_object_unref (current_output->data);
		current_output = g_list_next (current_output);
	}
	
	g_queue_free (self->priv->output_queue);
	g_hash_table_destroy (self->priv->revisions);
	g_regex_unref (self->priv->commit_regex);
	g_regex_unref (self->priv->parent_regex);
	g_regex_unref (self->priv->author_regex);
	g_regex_unref (self->priv->time_regex);
	g_regex_unref (self->priv->short_log_regex);

	G_OBJECT_CLASS (git_log_data_command_parent_class)->finalize (object);
}

static guint
git_log_data_command_run (AnjutaCommand *command)
{
	GitLogDataCommand *self;
	gchar *line;
	GMatchInfo *commit_match_info;
	GMatchInfo *parent_match_info;
	GMatchInfo *author_match_info;
	GMatchInfo *time_match_info;
	GMatchInfo *short_log_match_info;
	gchar *commit_sha;
	gchar *parents;
	gchar **parent_shas;
	gint i;
	GitRevision *parent_revision;
	gchar *author;
	gchar *time;
	gchar *short_log;

	self = GIT_LOG_DATA_COMMAND (command);

	while ((line = g_async_queue_pop (self->priv->input_queue)))
	{
		/* An empty string means there's nothing left to process */
		if (g_utf8_strlen (line, -1) == 0)
		{
			g_free (line);
			break;
		}

		commit_match_info = NULL;
		parent_match_info = NULL;
		author_match_info = NULL;
		time_match_info = NULL;
		short_log_match_info = NULL;

		/* Entries are delimited by the hex value 0x0c */
		if (*line == 0x0c && self->priv->current_revision)
		{	
			anjuta_async_command_lock (ANJUTA_ASYNC_COMMAND (command));
			g_queue_push_tail (self->priv->output_queue, 
			                   self->priv->current_revision);
			anjuta_async_command_unlock (ANJUTA_ASYNC_COMMAND (command));
			
			anjuta_command_notify_data_arrived (command);
		}

		if (g_regex_match (self->priv->commit_regex, line, 0, &commit_match_info))
		{
			commit_sha = g_match_info_fetch (commit_match_info, 1);

			self->priv->current_revision = g_hash_table_lookup (self->priv->revisions, 
			                                                    commit_sha);

			if (!self->priv->current_revision)
			{
				self->priv->current_revision = git_revision_new ();
				git_revision_set_sha (self->priv->current_revision, commit_sha);
				g_hash_table_insert (self->priv->revisions, g_strdup (commit_sha),
				                     g_object_ref (self->priv->current_revision));
			}

			g_free (commit_sha);
		}
		else if (g_regex_match (self->priv->parent_regex, line, 0, 
		                        &parent_match_info))
		{	
			parents = g_match_info_fetch (parent_match_info, 1);
			parent_shas = g_strsplit (parents, " ", -1);

			for (i = 0; parent_shas[i]; i++)
			{
				parent_revision = g_hash_table_lookup (self->priv->revisions,
				                                       parent_shas[i]);

				if (!parent_revision)
				{
					parent_revision = git_revision_new ();
					git_revision_set_sha (parent_revision, parent_shas[i]);
					g_hash_table_insert (self->priv->revisions,
					                     g_strdup (parent_shas[i]),
					                     g_object_ref (parent_revision));
				}

				git_revision_add_child (parent_revision,
				                        self->priv->current_revision);
			}

			g_free (parents);
			g_strfreev (parent_shas);
		}
		else if (g_regex_match (self->priv->author_regex, line, 0, 
		                        &author_match_info))
		{
			author = g_match_info_fetch (author_match_info, 1);
			git_revision_set_author (self->priv->current_revision, author);

			g_free (author);
		}
		else if (g_regex_match (self->priv->time_regex, line, 0, 
		                        &time_match_info))
		{
			time = g_match_info_fetch (time_match_info, 1);
			git_revision_set_date (self->priv->current_revision, atol (time));

			g_free (time);
		}
		else if (g_regex_match (self->priv->short_log_regex, line, 0, 
		                        &short_log_match_info))
		{
			short_log = g_match_info_fetch (short_log_match_info, 1);
			git_revision_set_short_log (self->priv->current_revision, short_log);

			g_free (short_log);
		}

		if (commit_match_info)
			g_match_info_free (commit_match_info);

		if (parent_match_info)
			g_match_info_free (parent_match_info);

		if (author_match_info)
			g_match_info_free (author_match_info);

		if (time_match_info)
			g_match_info_free (time_match_info);

		if (short_log_match_info)
			g_match_info_free (short_log_match_info);

		g_free (line);
	}

	return 0;
}

static void
git_log_data_command_class_init (GitLogDataCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaCommandClass *command_class = ANJUTA_COMMAND_CLASS (klass);
	
	object_class->finalize = git_log_data_command_finalize;
	command_class->run = git_log_data_command_run;
}

GitLogDataCommand *
git_log_data_command_new (void)
{
	return g_object_new (GIT_TYPE_LOG_DATA_COMMAND, NULL);
}

GQueue *
git_log_data_command_get_output (GitLogDataCommand *self)
{
	return self->priv->output_queue;
}

void
git_log_data_command_push_line (GitLogDataCommand *self, const gchar *line)
{
	g_async_queue_push (self->priv->input_queue, g_strdup (line));
}
