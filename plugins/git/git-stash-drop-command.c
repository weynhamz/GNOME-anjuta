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

#include "git-stash-drop-command.h"

struct _GitStashDropCommandPriv
{
	gchar *stash;
};

G_DEFINE_TYPE (GitStashDropCommand, git_stash_drop_command, 
			   GIT_TYPE_COMMAND);

static void
git_stash_drop_command_init (GitStashDropCommand *self)
{
	self->priv = g_new0 (GitStashDropCommandPriv, 1);
}

static void
git_stash_drop_command_finalize (GObject *object)
{
	GitStashDropCommand *self;
	
	self = GIT_STASH_DROP_COMMAND (object);
	
	g_free (self->priv->stash);
	g_free (self->priv);

	G_OBJECT_CLASS (git_stash_drop_command_parent_class)->finalize (object);
}

static guint
git_stash_drop_command_run (AnjutaCommand *command)
{	
	GitStashDropCommand *self;
	
	self = GIT_STASH_DROP_COMMAND (command);
	
	git_command_add_arg (GIT_COMMAND (command), "stash");
	git_command_add_arg (GIT_COMMAND (command), "drop");
	git_command_add_arg (GIT_COMMAND (command), self->priv->stash);
	
	return 0;
}

static void
git_stash_drop_command_class_init (GitStashDropCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GitCommandClass *parent_class = GIT_COMMAND_CLASS (klass);
	AnjutaCommandClass *command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = git_stash_drop_command_finalize;
	parent_class->output_handler = git_command_send_output_to_info;
	command_class->run = git_stash_drop_command_run;
}


GitStashDropCommand *
git_stash_drop_command_new (const gchar *working_directory, const gchar *stash)
{
	GitStashDropCommand *self;
	
	self =  g_object_new (GIT_TYPE_STASH_DROP_COMMAND, 
						  "working-directory", working_directory,
						  NULL);
	
	self->priv->stash = g_strdup (stash);
	
	return self;
}

