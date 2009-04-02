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


G_DEFINE_TYPE (GitCloneCommand, git_clone_command, GIT_TYPE_COMMAND);

typedef struct _GitCloneCommandPrivate GitCloneCommandPrivate;

struct _GitCloneCommandPrivate
{
	gchar *uri;
	gchar *repository_name;
};

#define GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), GIT_TYPE_CLONE_COMMAND, GitCloneCommandPrivate))

static guint
git_clone_command_run (AnjutaCommand *command)
{
	GitCloneCommandPrivate *priv;
	GitCommand *self;

	priv = GET_PRIVATE (command);
	self = GIT_COMMAND (command);
	
	git_command_add_arg (self, "clone");
	git_command_add_arg (self, priv->uri);
	git_command_add_arg (self, priv->repository_name);
	
	return 0;
}

static void
git_clone_command_init (GitCloneCommand *object)
{
	/* TODO: Add initialization code here */
}

static void
git_clone_command_finalize (GObject *object)
{
	GitCloneCommandPrivate *priv = GET_PRIVATE (object);

	g_free (priv->uri);
	g_free (priv->repository_name);

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

	g_type_class_add_private (klass, sizeof (GitCloneCommandPrivate));
}

GitCloneCommand *
git_clone_command_new (const gchar *uri, const gchar *working_directory,
                       const gchar *repository_name)
{
	GitCloneCommand *clone_command;
	GitCloneCommandPrivate *priv;

	g_assert (uri != NULL);
	g_assert (working_directory != NULL);
	g_assert (repository_name != NULL);
	
	clone_command = g_object_new (GIT_TYPE_CLONE_COMMAND,
	                              "working-directory", working_directory,
	                              "single-line-output", TRUE,
	                              NULL);
	
	priv = GET_PRIVATE (clone_command);
	priv->uri = g_strdup (uri);
	priv->repository_name = g_strdup (repository_name);
	
	return clone_command;
}