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

#include "git-bisect-start-command.h"

struct _GitBisectStartCommandPriv
{
	gchar *bad_revision;
	gchar *good_revision;
};

G_DEFINE_TYPE (GitBisectStartCommand, git_bisect_start_command, 
			   GIT_TYPE_COMMAND);

static void
git_bisect_start_command_init (GitBisectStartCommand *self)
{
	self->priv = g_new0 (GitBisectStartCommandPriv, 1);
}

static void
git_bisect_start_command_finalize (GObject *object)
{
	GitBisectStartCommand *self;
	
	self = GIT_BISECT_START_COMMAND (object);
	
	g_free (self->priv->bad_revision);
	g_free (self->priv->good_revision);
	g_free (self->priv);

	G_OBJECT_CLASS (git_bisect_start_command_parent_class)->finalize (object);
}

static guint
git_bisect_start_command_run (AnjutaCommand *command)
{
	GitBisectStartCommand *self;
	
	self = GIT_BISECT_START_COMMAND (command);
	
	git_command_add_arg (GIT_COMMAND (command), "bisect");
	git_command_add_arg (GIT_COMMAND (command), "start");
	
	if (self->priv->bad_revision)
		git_command_add_arg (GIT_COMMAND (command), self->priv->bad_revision);
	
	/* If a good revision was given with no bad revision, put head in for the 
	 * bad revision, because git expects both revisions in this case, so don't
	 * confuse it. */
	if (self->priv->good_revision)
	{
		if (!self->priv->bad_revision)
			git_command_add_arg (GIT_COMMAND (command), "HEAD");
		
		git_command_add_arg (GIT_COMMAND (command), self->priv->good_revision);
	}
	
	return 0;
}

static void
git_bisect_start_command_class_init (GitBisectStartCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GitCommandClass* parent_class = GIT_COMMAND_CLASS (klass);
	AnjutaCommandClass* command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = git_bisect_start_command_finalize;
	parent_class->output_handler = git_command_send_output_to_info;
	command_class->run = git_bisect_start_command_run;
}


GitBisectStartCommand *
git_bisect_start_command_new (const gchar *working_directory, 
							  const gchar *bad_revision,
							  const gchar *good_revision)
{
	GitBisectStartCommand *self;
	
	self = g_object_new (GIT_TYPE_BISECT_START_COMMAND,
						 "working-directory", working_directory,
						 "single-line-output", TRUE,
						 NULL);
	
	self->priv->bad_revision = g_strdup (bad_revision);
	self->priv->good_revision = g_strdup (good_revision);
	
	return self;
}
