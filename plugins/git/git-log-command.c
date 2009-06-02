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

#define COMMIT_REGEX "^commit ([[:xdigit:]]{40})"
#define PARENT_REGEX "^parents (.*)"
#define AUTHOR_REGEX "^author (.*)"
#define TIME_REGEX "^time (\\d*)"
#define SHORT_LOG_REGEX "^(?:short log) (.*)"

#include "git-log-command.h"

struct _GitLogCommandPriv
{
	GQueue *output_queue;
	GHashTable *revisions;
	GitRevision *current_revision;
	GRegex *commit_regex;
	GRegex *parent_regex;
	GRegex *author_regex;
	GRegex *time_regex;
	GRegex *short_log_regex;
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
git_log_command_init (GitLogCommand *self)
{
	self->priv = g_new0 (GitLogCommandPriv, 1);
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
git_log_command_finalize (GObject *object)
{
	GitLogCommand *self;
	GList *current_output;
	
	self = GIT_LOG_COMMAND (object);
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
	g_free (self->priv->path);
	
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
	else
		git_command_add_arg (GIT_COMMAND (command), "HEAD");
	
	if (self->priv->path)
	{
		git_command_add_arg (GIT_COMMAND (command), "--");
		git_command_add_arg (GIT_COMMAND (command), self->priv->path);
	}
	
	return 0;
}

static void
git_log_command_handle_output (GitCommand *git_command, const gchar *output)
{
	GitLogCommand *self;
	GMatchInfo *match_info;
	gchar *commit_sha;
	gchar *parents;
	gchar **parent_shas;
	gint i;
	GitRevision *parent_revision;
	gchar *author;
	gchar *time;
	gchar *short_log;
	
	self = GIT_LOG_COMMAND (git_command);
	
	/* Entries are delimited by the hex value 0x0c */
	if (*output == 0x0c && self->priv->current_revision)
	{	
		g_queue_push_tail (self->priv->output_queue, 
						   self->priv->current_revision);
		anjuta_command_notify_data_arrived (ANJUTA_COMMAND (git_command));
	}
	
	if (g_regex_match (self->priv->commit_regex, output, 0, &match_info))
	{
		commit_sha = g_match_info_fetch (match_info, 1);
		
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
	else if (g_regex_match (self->priv->parent_regex, output, 0, &match_info))
	{	
		parents = g_match_info_fetch (match_info, 1);
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
	else if (g_regex_match (self->priv->author_regex, output, 0, &match_info))
	{
		author = g_match_info_fetch (match_info, 1);
		git_revision_set_author (self->priv->current_revision, author);
		
		g_free (author);
	}
	else if (g_regex_match (self->priv->time_regex, output, 0, &match_info))
	{
		time = g_match_info_fetch (match_info, 1);
		git_revision_set_date (self->priv->current_revision, atol (time));
		
		g_free (time);
	}
	else if (g_regex_match (self->priv->short_log_regex, output, 0, 
							&match_info))
	{
		short_log = g_match_info_fetch (match_info, 1);
		git_revision_set_short_log (self->priv->current_revision, short_log);
		
		g_free (short_log);
	}
	
	if (match_info)
		g_match_info_free (match_info);
				 
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
}


GitLogCommand *
git_log_command_new (const gchar *working_directory,
					 const gchar *path,
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
	return self->priv->output_queue;
}
