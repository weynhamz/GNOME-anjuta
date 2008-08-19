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

#include "git-format-patch-command.h"

struct _GitFormatPatchCommandPriv
{
	gchar *output_directory;
	gchar *branch;
	gboolean add_signoff;
};

G_DEFINE_TYPE (GitFormatPatchCommand, git_format_patch_command, 
			   GIT_TYPE_COMMAND);

static void
git_format_patch_command_init (GitFormatPatchCommand *self)
{
	self->priv = g_new0 (GitFormatPatchCommandPriv, 1);
}

static void
git_format_patch_command_finalize (GObject *object)
{
	GitFormatPatchCommand *self;
	
	self = GIT_FORMAT_PATCH_COMMAND (object);
	
	g_free (self->priv->output_directory);
	g_free (self->priv->branch);
	g_free (self->priv);

	G_OBJECT_CLASS (git_format_patch_command_parent_class)->finalize (object);
}

static guint
git_format_patch_command_run (AnjutaCommand *command)
{
	GitFormatPatchCommand *self;
	
	self = GIT_FORMAT_PATCH_COMMAND (command);
	
	git_command_add_arg (GIT_COMMAND (command), "format-patch");
	
	if (self->priv->output_directory)
	{
		git_command_add_arg (GIT_COMMAND (command), "-o");
		git_command_add_arg (GIT_COMMAND (command), 
							 self->priv->output_directory);
	}
	
	if (self->priv->add_signoff)
		git_command_add_arg (GIT_COMMAND (command), "-s");
	
	git_command_add_arg (GIT_COMMAND (command), self->priv->branch);
	
	return 0;
}

static void
git_format_patch_command_class_init (GitFormatPatchCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GitCommandClass* parent_class = GIT_COMMAND_CLASS (klass);
	AnjutaCommandClass* command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = git_format_patch_command_finalize;
	parent_class->output_handler = git_command_send_output_to_info;
	command_class->run = git_format_patch_command_run;
}


GitFormatPatchCommand *
git_format_patch_command_new (const gchar *working_directory, 
							  const gchar *output_directory,
							  const gchar *branch,
							  gboolean add_signoff)
{
	GitFormatPatchCommand *self;
	
	self = g_object_new (GIT_TYPE_FORMAT_PATCH_COMMAND, 
						 "working-directory", working_directory,
						 "single-line-output", TRUE,
						 NULL);
	
	self->priv->output_directory = g_strdup (output_directory);
	self->priv->branch = g_strdup (branch);
	self->priv->add_signoff = add_signoff;
	
	return self;
}
