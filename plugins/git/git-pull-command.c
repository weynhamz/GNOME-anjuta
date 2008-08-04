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

#include "git-pull-command.h"

struct _GitPullCommandPriv
{
	gchar *url; 
	gboolean no_commit; 
	gboolean squash; 
	gboolean commit_fast_forward;
	gboolean append_fetch_data;
	gboolean force; 
	gboolean no_follow_tags;
};

G_DEFINE_TYPE (GitPullCommand, git_pull_command, GIT_TYPE_COMMAND);

static void
git_pull_command_init (GitPullCommand *self)
{
	self->priv = g_new0 (GitPullCommandPriv, 1);
}

static void
git_pull_command_finalize (GObject *object)
{
	GitPullCommand *self;
	
	self = GIT_PULL_COMMAND (object);
	
	g_free (self->priv->url);
	g_free (self->priv);

	G_OBJECT_CLASS (git_pull_command_parent_class)->finalize (object);
}

static guint
git_pull_command_run (AnjutaCommand *command)
{
	GitPullCommand *self;
	
	self = GIT_PULL_COMMAND (command);
	
	git_command_add_arg (GIT_COMMAND (command), "pull");
	
	if (self->priv->no_commit)
		git_command_add_arg (GIT_COMMAND (command), "--no-commit");
	
	if (self->priv->squash)
		git_command_add_arg (GIT_COMMAND (command), "--squash");
	
	if (self->priv->commit_fast_forward)
		git_command_add_arg (GIT_COMMAND (command), "--no-ff");
	
	if (self->priv->append_fetch_data)
		git_command_add_arg (GIT_COMMAND (command), "-a");
	
	if (self->priv->force)
		git_command_add_arg (GIT_COMMAND (command), "-f");
	
	if (self->priv->no_follow_tags)
		git_command_add_arg (GIT_COMMAND (command), "--no-tags");
	
	git_command_add_arg (GIT_COMMAND (command), self->priv->url);
	
	return 0;
}

static void
git_pull_command_class_init (GitPullCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GitCommandClass* parent_class = GIT_COMMAND_CLASS (klass);
	AnjutaCommandClass* command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = git_pull_command_finalize;
	parent_class->output_handler = git_command_send_output_to_info;
	command_class->run = git_pull_command_run;
}


GitPullCommand *
git_pull_command_new (const gchar *working_directory,
					  const gchar *url, 
					  gboolean no_commit, gboolean squash, 
					  gboolean commit_fast_forward,
					  gboolean append_fetch_data, 
					  gboolean force, gboolean no_follow_tags)
{
	GitPullCommand *self;
	
	self =  g_object_new (GIT_TYPE_PULL_COMMAND,
						  "working-directory", working_directory,
						  "single-line-output", TRUE,
						  NULL);
	
	self->priv->url = g_strdup (url);
	self->priv->no_commit = no_commit;
	self->priv->squash = squash;
	self->priv->commit_fast_forward = commit_fast_forward;
	self->priv->append_fetch_data = append_fetch_data;
	self->priv->force = force;
	self->priv->no_follow_tags = no_follow_tags;
	
	return self;
}
