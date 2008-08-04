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

#include "git-branch-create-command.h"

struct _GitBranchCreateCommandPriv
{
	gchar *name;
	gchar *revision;
	gboolean checkout;
};

G_DEFINE_TYPE (GitBranchCreateCommand, git_branch_create_command, GIT_TYPE_COMMAND);

static void
git_branch_create_command_init (GitBranchCreateCommand *self)
{
	self->priv = g_new0 (GitBranchCreateCommandPriv, 1);
}

static void
git_branch_create_command_finalize (GObject *object)
{
	GitBranchCreateCommand *self;
	
	self = GIT_BRANCH_CREATE_COMMAND (object);
	
	g_free (self->priv->name);
	g_free (self->priv->revision);
	g_free (self->priv);

	G_OBJECT_CLASS (git_branch_create_command_parent_class)->finalize (object);
}

static guint
git_branch_create_command_run (AnjutaCommand *command)
{
	GitBranchCreateCommand *self;
	
	self = GIT_BRANCH_CREATE_COMMAND (command);
	
	if (self->priv->checkout)
	{
		git_command_add_arg (GIT_COMMAND (command), "checkout");
		git_command_add_arg (GIT_COMMAND (command), "-b");
	}
	else
		git_command_add_arg (GIT_COMMAND (command), "branch");
	
	git_command_add_arg (GIT_COMMAND (command), self->priv->name);
	
	if (self->priv->revision)
		git_command_add_arg (GIT_COMMAND (command), self->priv->revision);
	
	return 0;
}

static void
git_branch_create_command_class_init (GitBranchCreateCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GitCommandClass* parent_class = GIT_COMMAND_CLASS (klass);
	AnjutaCommandClass* command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = git_branch_create_command_finalize;
	parent_class->output_handler = git_command_send_output_to_info;
	command_class->run = git_branch_create_command_run;
}


GitBranchCreateCommand *
git_branch_create_command_new (const gchar *working_directory, 
							   const gchar *name, const gchar *revision,
							   gboolean checkout)
{
	GitBranchCreateCommand *self;
	
	self = g_object_new (GIT_TYPE_BRANCH_CREATE_COMMAND,
						 "working-directory", working_directory,
						 "single-line-output", TRUE,
						 NULL);
	
	self->priv->name = g_strdup (name);
	self->priv->revision = g_strdup (revision);
	self->priv->checkout = checkout;
	
	return self;
}

gchar *
git_branch_create_command_get_branch_name (GitBranchCreateCommand *self)
{
	return g_strdup (self->priv->name);
}
