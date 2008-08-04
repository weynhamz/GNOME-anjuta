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

#include "git-checkout-files-command.h"

struct _GitCheckoutFilesCommandPriv
{
	GList *paths;
};

G_DEFINE_TYPE (GitCheckoutFilesCommand, git_checkout_files_command, GIT_TYPE_COMMAND);

static guint
git_checkout_files_command_run (AnjutaCommand *command)
{
	GitCheckoutFilesCommand *self;
	
	self = GIT_CHECKOUT_FILES_COMMAND (command);
	
	git_command_add_arg (GIT_COMMAND (self), "checkout");
	git_command_add_list_to_args (GIT_COMMAND (self), self->priv->paths);
	
	return 0;
}

static void
git_checkout_files_command_init (GitCheckoutFilesCommand *self)
{
	self->priv = g_new0 (GitCheckoutFilesCommandPriv, 1);
}

static void
git_checkout_files_command_finalize (GObject *object)
{
	GitCheckoutFilesCommand *self;
	
	self = GIT_CHECKOUT_FILES_COMMAND (object);
	
	git_command_free_path_list (self->priv->paths);
	g_free (self->priv);

	G_OBJECT_CLASS (git_checkout_files_command_parent_class)->finalize (object);
}

static void
git_checkout_files_command_class_init (GitCheckoutFilesCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GitCommandClass* parent_class = GIT_COMMAND_CLASS (klass);
	AnjutaCommandClass* command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = git_checkout_files_command_finalize;
	parent_class->output_handler = git_command_send_output_to_info;
	command_class->run = git_checkout_files_command_run;
}


GitCheckoutFilesCommand *
git_checkout_files_command_new (const gchar *working_directory, GList *paths)
{
	GitCheckoutFilesCommand *self;
	
	self = g_object_new (GIT_TYPE_CHECKOUT_FILES_COMMAND, 
						 "working-directory", working_directory,
						 NULL);
	
	self->priv->paths = git_command_copy_path_list (paths);
	
	return self;
}
