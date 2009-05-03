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

#include "git-commit-command.h"

struct _GitCommitCommandPriv
{
	GList *paths;
	gboolean amend;
	gboolean resolve_merge;
	gchar *log;
	gchar *author_name;
	gchar *author_email;
};

G_DEFINE_TYPE (GitCommitCommand, git_commit_command, GIT_TYPE_COMMAND);

static guint
git_commit_command_run (AnjutaCommand *command)
{
	GitCommitCommand *self;
	gchar *author;
	
	self = GIT_COMMIT_COMMAND (command);
	
	git_command_add_arg (GIT_COMMAND (self), "commit");

	if (self->priv->amend)
		git_command_add_arg (GIT_COMMAND (self), "--amend");

	if (self->priv->author_name && self->priv->author_email)
	{
		author = g_strdup_printf ("--author=%s <%s>", self->priv->author_name,
		                          self->priv->author_email);
		git_command_add_arg (GIT_COMMAND (self), author);

		g_free (author);
	}

	git_command_add_arg (GIT_COMMAND (self), "-m");
	git_command_add_arg (GIT_COMMAND (self), self->priv->log);
	
	if (self->priv->resolve_merge)
		git_command_add_arg (GIT_COMMAND (self), "-i");
	
	git_command_add_list_to_args (GIT_COMMAND (self), self->priv->paths);
	
	return 0;
}

static void
git_commit_command_init (GitCommitCommand *self)
{
	self->priv = g_new0 (GitCommitCommandPriv, 1);
}

static void
git_commit_command_finalize (GObject *object)
{
	GitCommitCommand *self;
	
	self = GIT_COMMIT_COMMAND (object);
	
	git_command_free_path_list (self->priv->paths);
	g_free (self->priv->log);
	g_free (self->priv->author_name);
	g_free (self->priv->author_email);
	g_free (self->priv);

	G_OBJECT_CLASS (git_commit_command_parent_class)->finalize (object);
}

static void
git_commit_command_class_init (GitCommitCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GitCommandClass* parent_class = GIT_COMMAND_CLASS (klass);
	AnjutaCommandClass* command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = git_commit_command_finalize;
	parent_class->output_handler = git_command_send_output_to_info;
	command_class->run = git_commit_command_run;
}


GitCommitCommand *
git_commit_command_new (const gchar *working_directory, gboolean amend, 
						gboolean resolve_merge, const gchar *log, 
                        const gchar *author_name, 
                        const gchar *author_email,
                        GList *paths)
{
	GitCommitCommand *self;
	
	self = g_object_new (GIT_TYPE_COMMIT_COMMAND, 
						 "working-directory", working_directory,
						 "single-line-output", TRUE,
						 NULL);
	
	self->priv->paths = git_command_copy_path_list (paths);
	self->priv->amend = amend;
	self->priv->resolve_merge = resolve_merge;
	self->priv->log = g_strdup (log);
	self->priv->author_name = g_strdup (author_name);
	self->priv->author_email = g_strdup (author_email);
	
	return self;
}
