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

#include "git-branch-checkout-command.h"

struct _GitBranchCheckoutCommandPriv
{
	gchar *branch_name;
};

G_DEFINE_TYPE (GitBranchCheckoutCommand, git_branch_checkout_command, GIT_TYPE_COMMAND);

static void
git_branch_checkout_command_init (GitBranchCheckoutCommand *self)
{
	self->priv = g_new0 (GitBranchCheckoutCommandPriv, 1);
}

static void
git_branch_checkout_command_finalize (GObject *object)
{
	GitBranchCheckoutCommand *self;
	
	self = GIT_BRANCH_CHECKOUT_COMMAND (object);
	
	g_free (self->priv->branch_name);
	g_free (self->priv);

	G_OBJECT_CLASS (git_branch_checkout_command_parent_class)->finalize (object);
}

static guint
git_branch_checkout_command_run (AnjutaCommand *command)
{
	GitBranchCheckoutCommand *self;
	
	self = GIT_BRANCH_CHECKOUT_COMMAND (command);
	
	git_command_add_arg (GIT_COMMAND (command), "checkout");
	git_command_add_arg (GIT_COMMAND (command), self->priv->branch_name);
	
	return 0;
}

static void
git_branch_checkout_command_class_init (GitBranchCheckoutCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GitCommandClass* parent_class = GIT_COMMAND_CLASS (klass);
	AnjutaCommandClass* command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = git_branch_checkout_command_finalize;
	parent_class->output_handler = git_command_send_output_to_info;
	command_class->run = git_branch_checkout_command_run;
}


GitBranchCheckoutCommand *
git_branch_checkout_command_new (const gchar *working_directory, 
								 const gchar *branch_name)
{
	GitBranchCheckoutCommand *self;
	
	self = g_object_new (GIT_TYPE_BRANCH_CHECKOUT_COMMAND,
						 "working-directory", working_directory,
						 "single-line-output", TRUE,
						 NULL);
	
	self->priv->branch_name = g_strdup (branch_name);
	
	return self;
}
