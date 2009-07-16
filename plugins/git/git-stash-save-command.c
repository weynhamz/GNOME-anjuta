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

#include "git-stash-save-command.h"

struct _GitStashSaveCommandPriv
{
	gboolean keep_index;
	gchar *message;
};

G_DEFINE_TYPE (GitStashSaveCommand, git_stash_save_command, GIT_TYPE_COMMAND);

static void
git_stash_save_command_init (GitStashSaveCommand *self)
{
	self->priv = g_new0 (GitStashSaveCommandPriv, 1);
}

static void
git_stash_save_command_finalize (GObject *object)
{
	GitStashSaveCommand *self;

	self = GIT_STASH_SAVE_COMMAND (object);

	g_free (self->priv->message);
	g_free (self->priv);

	G_OBJECT_CLASS (git_stash_save_command_parent_class)->finalize (object);
}

static guint
git_stash_save_command_run (AnjutaCommand *command)
{
	GitStashSaveCommand *self;

	self = GIT_STASH_SAVE_COMMAND (command);

	git_command_add_arg (GIT_COMMAND (command), "stash");
	git_command_add_arg (GIT_COMMAND (command), "save");

	if (self->priv->keep_index)
		git_command_add_arg (GIT_COMMAND (command), "--keep-index");

	if (self->priv->message)
		git_command_add_arg (GIT_COMMAND (command), self->priv->message);

	return 0;
}

static void
git_stash_save_command_class_init (GitStashSaveCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GitCommandClass* parent_class = GIT_COMMAND_CLASS (klass);
	AnjutaCommandClass *command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = git_stash_save_command_finalize;
	parent_class->output_handler = git_command_send_output_to_info;
	command_class->run = git_stash_save_command_run;
}


GitStashSaveCommand *
git_stash_save_command_new (const gchar *working_directory, 
							gboolean keep_index, const gchar *message)
{
	GitStashSaveCommand *self;

	self = g_object_new (GIT_TYPE_STASH_SAVE_COMMAND, 
						 "working-directory", working_directory,
						 "single-line-output", TRUE,
						 NULL);

	self->priv->keep_index = keep_index;
	self->priv->message = g_strdup (message);

	return self;
}
