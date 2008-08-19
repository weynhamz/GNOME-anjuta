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

#include "git-remote-add-command.h"

struct _GitRemoteAddCommandPriv
{
	gchar *name;
	gchar *url;
	gboolean fetch;
};

G_DEFINE_TYPE (GitRemoteAddCommand, git_remote_add_command, GIT_TYPE_COMMAND);

static void
git_remote_add_command_init (GitRemoteAddCommand *self)
{
	self->priv = g_new0 (GitRemoteAddCommandPriv, 1);
}

static void
git_remote_add_command_finalize (GObject *object)
{
	GitRemoteAddCommand *self;
	
	self = GIT_REMOTE_ADD_COMMAND (object);
	
	g_free (self->priv->name);
	g_free (self->priv->url);
	g_free (self->priv);

	G_OBJECT_CLASS (git_remote_add_command_parent_class)->finalize (object);
}

static guint
git_remote_add_command_run (AnjutaCommand *command)
{
	GitRemoteAddCommand *self;
	
	self = GIT_REMOTE_ADD_COMMAND (command);
	
	git_command_add_arg (GIT_COMMAND (command), "remote");
	git_command_add_arg (GIT_COMMAND (command), "add");
	
	if (self->priv->fetch)
		git_command_add_arg (GIT_COMMAND (command), "-f");
	
	git_command_add_arg (GIT_COMMAND (command), self->priv->name);
	git_command_add_arg (GIT_COMMAND (command), self->priv->url);
	
	return 0;
}

static void
git_remote_add_command_class_init (GitRemoteAddCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GitCommandClass* parent_class = GIT_COMMAND_CLASS (klass);
	AnjutaCommandClass* command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = git_remote_add_command_finalize;
	parent_class->output_handler = git_command_send_output_to_info;
	command_class->run = git_remote_add_command_run;
}


GitRemoteAddCommand *
git_remote_add_command_new (const gchar *working_directory, 
							const gchar *name, const gchar *url,
							gboolean fetch)
{
	GitRemoteAddCommand *self;
	
	self = g_object_new (GIT_TYPE_REMOTE_ADD_COMMAND,
						 "working-directory", working_directory,
						 NULL);
	
	self->priv->name = g_strdup (name);
	self->priv->url = g_strdup (url);
	self->priv->fetch = fetch;
	
	return self;
}

gchar *
git_remote_add_command_get_branch_name (GitRemoteAddCommand *self)
{
	return g_strdup (self->priv->name);
}
