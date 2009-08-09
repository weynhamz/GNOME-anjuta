/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Carl-Anton Ingmarsson 2009 <ca.ingmarsson@gmail.com>
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

#include "git-clone-command.h"

struct _GitCloneCommandPriv
{
	gchar *url;
	gchar *repository_name;
};

G_DEFINE_TYPE (GitCloneCommand, git_clone_command, GIT_TYPE_COMMAND);

static guint
git_clone_command_run (AnjutaCommand *command)
{
	GitCloneCommand *self;

	self = GIT_CLONE_COMMAND (command);
	
	git_command_add_arg (GIT_COMMAND (self), "clone");
	git_command_add_arg (GIT_COMMAND (self), self->priv->url);
	git_command_add_arg (GIT_COMMAND (self), self->priv->repository_name);
	
	return 0;
}

static void
git_clone_command_init (GitCloneCommand *self)
{
	self->priv = g_new0 (GitCloneCommandPriv, 1);
}

static void
git_clone_command_finalize (GObject *object)
{
	GitCloneCommand *self;

	self = GIT_CLONE_COMMAND (object);

	g_free (self->priv->url);
	g_free (self->priv->repository_name);
	g_free (self->priv);

	G_OBJECT_CLASS (git_clone_command_parent_class)->finalize (object);
}

static void
git_clone_command_class_init (GitCloneCommandClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GitCommandClass *parent_class = GIT_COMMAND_CLASS (klass);
	AnjutaCommandClass *command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = git_clone_command_finalize;
	parent_class->output_handler = git_command_send_output_to_info;
	command_class->run = git_clone_command_run;
}

GitCloneCommand *
git_clone_command_new (const gchar *working_directory, const gchar *url, 
                       const gchar *repository_name)
{
	GitCloneCommand *self;

	self = g_object_new (GIT_TYPE_CLONE_COMMAND,
	                     "working-directory", working_directory,
	                     "single-line-output", TRUE,
	                     NULL);
	
	self->priv->url = g_strdup (url);
	self->priv->repository_name = g_strdup (repository_name);
	
	return self;
}