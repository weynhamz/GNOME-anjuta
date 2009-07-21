/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2009 <jrliggett@cox.net>
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

#include "git-stash-apply-command.h"

struct _GitStashApplyCommandPriv
{
	gboolean restore_index;
	gchar *stash;
};

G_DEFINE_TYPE (GitStashApplyCommand, git_stash_apply_command, GIT_TYPE_COMMAND);

static void
git_stash_apply_command_init (GitStashApplyCommand *self)
{
	self->priv = g_new0 (GitStashApplyCommandPriv, 1);
}

static void
git_stash_apply_command_finalize (GObject *object)
{
	GitStashApplyCommand *self;

	self = GIT_STASH_APPLY_COMMAND (object);

	g_free (self->priv->stash);
	g_free (self->priv);

	G_OBJECT_CLASS (git_stash_apply_command_parent_class)->finalize (object);
}

static guint
git_stash_apply_command_run (AnjutaCommand *command)
{
	GitStashApplyCommand *self;

	self = GIT_STASH_APPLY_COMMAND (command);

	git_command_add_arg (GIT_COMMAND (command), "stash");
	git_command_add_arg (GIT_COMMAND (command), "apply");

	if (self->priv->restore_index)
		git_command_add_arg (GIT_COMMAND (command), "--index");

	git_command_add_arg (GIT_COMMAND (command), self->priv->stash);

	return 0;
}

static void
git_stash_apply_command_class_init (GitStashApplyCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GitCommandClass* parent_class = GIT_COMMAND_CLASS (klass);
	AnjutaCommandClass *command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = git_stash_apply_command_finalize;
	parent_class->output_handler = git_command_send_output_to_info;
	command_class->run = git_stash_apply_command_run;
}


GitStashApplyCommand *
git_stash_apply_command_new (const gchar *working_directory, 
							gboolean restore_index, const gchar *stash)
{
	GitStashApplyCommand *self;

	self = g_object_new (GIT_TYPE_STASH_APPLY_COMMAND, 
						 "working-directory", working_directory,
						 "single-line-output", TRUE,
						 NULL);

	self->priv->restore_index = restore_index;
	self->priv->stash = g_strdup (stash);

	return self;
}
