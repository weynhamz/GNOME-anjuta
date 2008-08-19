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

#include "git-rebase-continue-command.h"

struct _GitRebaseContinueCommandPriv
{
	GitRebaseContinueAction action;
};

G_DEFINE_TYPE (GitRebaseContinueCommand, git_rebase_continue_command, 
			   GIT_TYPE_COMMAND);

static void
git_rebase_continue_command_init (GitRebaseContinueCommand *self)
{
	self->priv = g_new0 (GitRebaseContinueCommandPriv, 1);
}

static void
git_rebase_continue_command_finalize (GObject *object)
{
	GitRebaseContinueCommand *self;
	
	self = GIT_REBASE_CONTINUE_COMMAND (object);
	
	g_free (self->priv);

	G_OBJECT_CLASS (git_rebase_continue_command_parent_class)->finalize (object);
}

static guint
git_rebase_continue_command_run (AnjutaCommand *command)
{
	GitRebaseContinueCommand *self;
	
	self = GIT_REBASE_CONTINUE_COMMAND (command);
	
	git_command_add_arg (GIT_COMMAND (command), "rebase");
	
	switch (self->priv->action)
	{
		case GIT_REBASE_CONTINUE_ACTION_CONTINUE:
			git_command_add_arg (GIT_COMMAND (command), "--continue");
			break;
		case GIT_REBASE_CONTINUE_ACTION_SKIP:
			git_command_add_arg (GIT_COMMAND (command), "--skip");
			break;
		case GIT_REBASE_CONTINUE_ACTION_ABORT:
			git_command_add_arg (GIT_COMMAND (command), "--abort");
			break;
		default:
			break;
	}
	
	return 0;
}

static void
git_rebase_continue_command_class_init (GitRebaseContinueCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GitCommandClass* parent_class = GIT_COMMAND_CLASS (klass);
	AnjutaCommandClass* command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = git_rebase_continue_command_finalize;
	parent_class->output_handler = git_command_send_output_to_info;
	command_class->run = git_rebase_continue_command_run;
}

GitRebaseContinueCommand *
git_rebase_continue_command_new (const gchar *working_directory, 
								 GitRebaseContinueAction action)
{
	GitRebaseContinueCommand *self;
	
	self = g_object_new (GIT_TYPE_REBASE_CONTINUE_COMMAND,
						 "working-directory", working_directory,
						 "single-line-output", TRUE,
						 NULL);
	
	self->priv->action = action;
	
	return self;
}
