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

#include "git-push-command.h"

struct _GitPushCommandPriv
{
	gchar *url; 
	gboolean push_all;
	gboolean push_tags;
};

G_DEFINE_TYPE (GitPushCommand, git_push_command, GIT_TYPE_COMMAND);

static void
git_push_command_init (GitPushCommand *self)
{
	self->priv = g_new0 (GitPushCommandPriv, 1);
}

static void
git_push_command_finalize (GObject *object)
{
	GitPushCommand *self;
	
	self = GIT_PUSH_COMMAND (object);
	
	g_free (self->priv->url);
	g_free (self->priv);

	G_OBJECT_CLASS (git_push_command_parent_class)->finalize (object);
}

static guint
git_push_command_run (AnjutaCommand *command)
{
	GitPushCommand *self;
	
	self = GIT_PUSH_COMMAND (command);
	
	git_command_add_arg (GIT_COMMAND (command), "push");
	
	if (self->priv->push_all)
		git_command_add_arg (GIT_COMMAND (command), "--all");
	
	if (self->priv->push_tags)
		git_command_add_arg (GIT_COMMAND (command), "--tags");
	
	git_command_add_arg (GIT_COMMAND (command), self->priv->url);
	
	return 0;
}

static void
git_push_command_class_init (GitPushCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GitCommandClass* parent_class = GIT_COMMAND_CLASS (klass);
	AnjutaCommandClass* command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = git_push_command_finalize;
	parent_class->output_handler = git_command_send_output_to_info;
	command_class->run = git_push_command_run;
}


GitPushCommand *
git_push_command_new (const gchar *working_directory,
					  const gchar *url, 
					  gboolean push_all, 
                      gboolean push_tags)
{
	GitPushCommand *self;
	
	self =  g_object_new (GIT_TYPE_PUSH_COMMAND,
						  "working-directory", working_directory,
						  "single-line-output", TRUE,
						  NULL);
	
	self->priv->url = g_strdup (url);
	self->priv->push_all = push_all;
	self->priv->push_tags = push_tags;
	
	return self;
}
