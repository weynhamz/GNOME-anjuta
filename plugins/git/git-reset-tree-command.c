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

#include "git-reset-tree-command.h"

struct _GitResetTreeCommandPriv
{
	gchar *revision;
	GitResetTreeMode mode;
};

G_DEFINE_TYPE (GitResetTreeCommand, git_reset_tree_command, GIT_TYPE_COMMAND);

static guint
git_reset_tree_command_run (AnjutaCommand *command)
{
	GitResetTreeCommand *self;
	
	self = GIT_RESET_TREE_COMMAND (command);
	
	git_command_add_arg (GIT_COMMAND (command), "reset");
	
	switch (self->priv->mode)
	{
		case GIT_RESET_TREE_MODE_MIXED:
			git_command_add_arg (GIT_COMMAND (command), "--mixed");
			break;
		case GIT_RESET_TREE_MODE_SOFT:
			git_command_add_arg (GIT_COMMAND (command), "--soft");
			break;
		case GIT_RESET_TREE_MODE_HARD:
			git_command_add_arg (GIT_COMMAND (command), "--hard");
			break;
		default:
			break;
	}
	
	git_command_add_arg (GIT_COMMAND (command), self->priv->revision);
	
	return 0;
}

static void
git_reset_tree_command_init (GitResetTreeCommand *self)
{
	self->priv = g_new0 (GitResetTreeCommandPriv, 1);
}

static void
git_reset_tree_command_finalize (GObject *object)
{
	GitResetTreeCommand *self;
	
	self = GIT_RESET_TREE_COMMAND (object);
	
	g_free (self->priv->revision);
	g_free (self->priv);

	G_OBJECT_CLASS (git_reset_tree_command_parent_class)->finalize (object);
}

static void
git_reset_tree_command_class_init (GitResetTreeCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GitCommandClass* parent_class = GIT_COMMAND_CLASS (klass);
	AnjutaCommandClass* command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = git_reset_tree_command_finalize;
	parent_class->output_handler = git_command_send_output_to_info;
	command_class->run = git_reset_tree_command_run;
}


GitResetTreeCommand *
git_reset_tree_command_new (const gchar *working_directory, 
							const gchar *revision, GitResetTreeMode mode)
{
	GitResetTreeCommand *self;
	
	self = g_object_new (GIT_TYPE_RESET_TREE_COMMAND, 
						 "working-directory", working_directory,
						 "single-line-output", TRUE,
						 NULL);
	
	self->priv->revision = g_strdup (revision);
	self->priv->mode = mode;
	
	return self;
}
