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

#include "git-file-command.h"

enum
{
	PROP_0,

	PROP_WORKING_DIRECTORY
};

struct _GitFileCommandPriv
{
	gchar *working_directory;
};

G_DEFINE_TYPE (GitFileCommand, git_file_command, ANJUTA_TYPE_SYNC_COMMAND);

static void
git_file_command_init (GitFileCommand *self)
{
	self->priv = g_new0 (GitFileCommandPriv, 1);
}

static void
git_file_command_finalize (GObject *object)
{
	GitFileCommand *self;
	
	self = GIT_FILE_COMMAND (object);
	
	g_free (self->priv->working_directory);
	g_free (self->priv);

	G_OBJECT_CLASS (git_file_command_parent_class)->finalize (object);
}

static void
git_file_command_set_property (GObject *object, guint prop_id, 
							   const GValue *value, GParamSpec *pspec)
{
	GitFileCommand *self;
	
	g_return_if_fail (GIT_IS_FILE_COMMAND (object));
	
	self = GIT_FILE_COMMAND (object);
	
	switch (prop_id)
	{
		case PROP_WORKING_DIRECTORY:
			g_free (self->priv->working_directory);
			self->priv->working_directory = g_value_dup_string (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
git_file_command_get_property (GObject *object, guint prop_id, GValue *value, 
							   GParamSpec *pspec)
{
	GitFileCommand *self;
	
	g_return_if_fail (GIT_IS_FILE_COMMAND (object));
	
	self = GIT_FILE_COMMAND (object);

	switch (prop_id)
	{
		case PROP_WORKING_DIRECTORY:
			g_value_set_string (value, self->priv->working_directory);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
git_file_command_class_init (GitFileCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = git_file_command_finalize;
	object_class->set_property = git_file_command_set_property;
	object_class->get_property = git_file_command_get_property;

	g_object_class_install_property (object_class,
	                                 PROP_WORKING_DIRECTORY,
	                                 g_param_spec_string ("working-directory",
	                                                      "",
	                                                      "Working directory of the command",
	                                                      "",
	                                                      G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

