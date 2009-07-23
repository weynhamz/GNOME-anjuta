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

#include "git-stash-list-command.h"

/* Splits the stash ID from the stash message */
#define STASH_REGEX "(stash@\\{(\\d+)\\})(?:\\:) (.*)"

struct _GitStashListCommandPriv
{
	GRegex *stash_regex;
	GQueue *output;
};

G_DEFINE_TYPE (GitStashListCommand, git_stash_list_command, GIT_TYPE_COMMAND);

static void
git_stash_list_command_init (GitStashListCommand *self)
{
	self->priv = g_new0 (GitStashListCommandPriv, 1);
	self->priv->stash_regex = g_regex_new (STASH_REGEX, 0, 0, NULL);
	
	self->priv->output = g_queue_new ();
}

static void
git_stash_list_command_finalize (GObject *object)
{
	GitStashListCommand *self;
	GList *current_stash;
	
	self = GIT_STASH_LIST_COMMAND (object);
	current_stash = self->priv->output->head;
	
	g_regex_unref (self->priv->stash_regex);
	
	while (current_stash)
	{
		g_object_unref (current_stash->data);
		current_stash = g_list_next (current_stash);
	}
	
	g_queue_free (self->priv->output);
	g_free (self->priv);

	G_OBJECT_CLASS (git_stash_list_command_parent_class)->finalize (object);
}

static guint
git_stash_list_command_run (AnjutaCommand *command)
{
	GitStashListCommand *self;
	
	self = GIT_STASH_LIST_COMMAND (command);
	
	git_command_add_arg (GIT_COMMAND (command), "stash");
	git_command_add_arg (GIT_COMMAND (command), "list");
	return 0;
}

static void
git_stash_list_command_handle_output (GitCommand *git_command, 
									  const gchar *output)
{
	GitStashListCommand *self;
	GMatchInfo *match_info;
	gchar *stash_id;
	gchar *stash_number;
	gchar *stash_message;
	GitStash *stash;

	self = GIT_STASH_LIST_COMMAND (git_command);
	
	match_info = NULL;
	stash_id = NULL;
	stash_message = NULL;
	stash = NULL;
	
	if (g_regex_match (self->priv->stash_regex, output, 0, &match_info))
	{
		stash_id = g_match_info_fetch (match_info, 1);
		stash_number = g_match_info_fetch (match_info, 2);
		stash_message = g_match_info_fetch (match_info, 3);

		stash = git_stash_new (stash_id, stash_message, atoi (stash_number));

		g_free (stash_id);
		g_free (stash_number);
		g_free (stash_message);

		g_queue_push_head (self->priv->output, stash);
		anjuta_command_notify_data_arrived (ANJUTA_COMMAND (git_command));
	}

	if (match_info)
		g_match_info_free (match_info);
}

static void
git_stash_list_command_class_init (GitStashListCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GitCommandClass* parent_class = GIT_COMMAND_CLASS (klass);
	AnjutaCommandClass *command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = git_stash_list_command_finalize;
	parent_class->output_handler = git_stash_list_command_handle_output;
	command_class->run = git_stash_list_command_run;
}


GitStashListCommand *
git_stash_list_command_new (const gchar *working_directory)
{
	GitStashListCommand *self;
	
	self = g_object_new (GIT_TYPE_STASH_LIST_COMMAND,
						 "working-directory", working_directory,
						 "single-line-output", TRUE,
						 NULL);
	
	return self;
}

GQueue *
git_stash_list_command_get_output (GitStashListCommand *self)
{
	return self->priv->output;
}
