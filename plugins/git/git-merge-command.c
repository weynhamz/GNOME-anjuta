/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-git
 * Copyright (C) James Liggett 2008 <jrliggett@cox.net>
 * 
 * anjuta-git is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta-git is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta-git.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include "git-merge-command.h"

struct _GitMergeCommandPriv
{
	gchar *branch;
	gchar *log;
	gboolean no_commit;
	gboolean squash;
};

G_DEFINE_TYPE (GitMergeCommand, git_merge_command, GIT_TYPE_COMMAND);

static void
git_merge_command_init (GitMergeCommand *self)
{
	self->priv = g_new0 (GitMergeCommandPriv, 1);
}

static void
git_merge_command_finalize (GObject *object)
{
	GitMergeCommand *self;
	
	self = GIT_MERGE_COMMAND (object);
	
	g_free (self->priv->branch);
	g_free (self->priv->log);
	g_free (self->priv);

	G_OBJECT_CLASS (git_merge_command_parent_class)->finalize (object);
}

static guint
git_merge_command_run (AnjutaCommand *command)
{
	GitMergeCommand *self;
	
	self = GIT_MERGE_COMMAND (command);
	
	git_command_add_arg (GIT_COMMAND (command), "merge");
	
	if (self->priv->no_commit)
		git_command_add_arg (GIT_COMMAND (command), "--no-commit");
	
	if (self->priv->squash)
		git_command_add_arg (GIT_COMMAND (command), "--squash");
	
	if (self->priv->log)
	{
		git_command_add_arg (GIT_COMMAND (command), "-m");
		git_command_add_arg (GIT_COMMAND (command), self->priv->log);
	}
	
	git_command_add_arg (GIT_COMMAND (command), self->priv->branch);
	
	return 0;
}

static void
git_merge_command_class_init (GitMergeCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GitCommandClass* parent_class = GIT_COMMAND_CLASS (klass);
	AnjutaCommandClass* command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = git_merge_command_finalize;
	parent_class->output_handler = git_command_send_output_to_info;
	command_class->run = git_merge_command_run;
}


GitMergeCommand *
git_merge_command_new (const gchar *working_directory, const gchar *branch, 
					   const gchar *log, gboolean no_commit, gboolean squash)
{
	GitMergeCommand *self;
	
	self =  g_object_new (GIT_TYPE_MERGE_COMMAND,
						  "working-directory", working_directory,
						  "single-line-output", TRUE,
						  NULL);
	
	self->priv->branch = g_strdup (branch);
	self->priv->log = g_strdup (log);
	self->priv->no_commit = no_commit;
	self->priv->squash = squash;
	
	return self;
}
