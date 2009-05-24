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

#include "git-cherry-pick-command.h"

struct _GitCherryPickCommandPriv
{
	gchar *revision;
	gboolean no_commit;
	gboolean show_source;
	gboolean add_signoff;
};

G_DEFINE_TYPE (GitCherryPickCommand, git_cherry_pick_command, GIT_TYPE_COMMAND);

static void
git_cherry_pick_command_init (GitCherryPickCommand *self)
{
	self->priv = g_new0 (GitCherryPickCommandPriv, 1);
}

static void
git_cherry_pick_command_finalize (GObject *object)
{
	GitCherryPickCommand *self;
	
	self = GIT_CHERRY_PICK_COMMAND (object);
	
	g_free (self->priv->revision);
	g_free (self->priv);

	G_OBJECT_CLASS (git_cherry_pick_command_parent_class)->finalize (object);
}

static guint
git_cherry_pick_command_run (AnjutaCommand *command)
{
	GitCherryPickCommand *self;
	
	self = GIT_CHERRY_PICK_COMMAND (command);
	
	git_command_add_arg (GIT_COMMAND (command), "cherry-pick");
	
	if (self->priv->no_commit)
		git_command_add_arg (GIT_COMMAND (command), "-n");

	if (self->priv->show_source)
		git_command_add_arg (GIT_COMMAND (command), "-x");

	if (self->priv->add_signoff)
		git_command_add_arg (GIT_COMMAND (command), "-s");
	
	git_command_add_arg (GIT_COMMAND (command), self->priv->revision);
	
	return 0;
}

static void
git_cherry_pick_command_class_init (GitCherryPickCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GitCommandClass* parent_class = GIT_COMMAND_CLASS (klass);
	AnjutaCommandClass* command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = git_cherry_pick_command_finalize;
	parent_class->output_handler = git_command_send_output_to_info;
	command_class->run = git_cherry_pick_command_run;
}


GitCherryPickCommand *
git_cherry_pick_command_new (const gchar *working_directory, 
                             const gchar *revision, gboolean no_commit,
                             gboolean show_source, gboolean add_signoff)
{
	GitCherryPickCommand *self;
	
	self = g_object_new (GIT_TYPE_CHERRY_PICK_COMMAND,
						 "working-directory", working_directory,
						 "single-line-output", TRUE,
						 NULL);
	
	self->priv->revision = g_strdup (revision);
	self->priv->no_commit = no_commit;
	self->priv->show_source = show_source;
	self->priv->add_signoff = add_signoff;
	
	return self;
}
