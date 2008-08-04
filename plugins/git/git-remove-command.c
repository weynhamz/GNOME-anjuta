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

#include "git-remove-command.h"

struct _GitRemoveCommandPriv
{
	GList *paths;
	gboolean force;
};

G_DEFINE_TYPE (GitRemoveCommand, git_remove_command, GIT_TYPE_COMMAND);

static void
git_remove_command_init (GitRemoveCommand *self)
{
	self->priv = g_new0 (GitRemoveCommandPriv, 1);
}

static void
git_remove_command_finalize (GObject *object)
{
	GitRemoveCommand *self;
	
	self = GIT_REMOVE_COMMAND (object);
	
	git_command_free_path_list (self->priv->paths);
	g_free (self->priv);
	
	G_OBJECT_CLASS (git_remove_command_parent_class)->finalize (object);
}

static guint
git_remove_command_run (AnjutaCommand *command)
{
	GitRemoveCommand *self;
	
	self = GIT_REMOVE_COMMAND (command);
	
	git_command_add_arg (GIT_COMMAND (command), "rm");
	
	if (self->priv->force)
		git_command_add_arg (GIT_COMMAND (command), "-f");
	
	git_command_add_list_to_args (GIT_COMMAND (command), self->priv->paths);
	
	return 0;
}

static void
git_remove_command_class_init (GitRemoveCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaCommandClass* command_class = ANJUTA_COMMAND_CLASS (klass);
	
	object_class->finalize = git_remove_command_finalize;
	command_class->run = git_remove_command_run;
}


GitRemoveCommand *
git_remove_command_new_path (const gchar *working_directory, const gchar *path,
							 gboolean force)
{
	GitRemoveCommand *self;
	
	self = g_object_new (GIT_TYPE_REMOVE_COMMAND,
						 "working-directory", working_directory,
						 NULL);
	
	self->priv->paths = g_list_append (self->priv->paths, g_strdup (path));
	self->priv->force = force;
	
	return self;
}


GitRemoveCommand *
git_remove_command_new_list (const gchar *working_directory, GList *path_list,
							 gboolean force)
{
	GitRemoveCommand *self;
	
	self = g_object_new (GIT_TYPE_REMOVE_COMMAND,
						 "working-directory", working_directory,
						 NULL);
	
	self->priv->paths = git_command_copy_path_list (path_list);
	self->priv->force = force;
	
	return self;
}
