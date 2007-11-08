/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2007 <jrliggett@cox.net>
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

#include "svn-revert-command.h"

struct _SvnRevertCommandPriv
{
	GList *paths;
	gboolean recursive;
};

G_DEFINE_TYPE (SvnRevertCommand, svn_revert_command, SVN_TYPE_COMMAND);

static void
svn_revert_command_init (SvnRevertCommand *self)
{	
	self->priv = g_new0 (SvnRevertCommandPriv, 1);
}

static void
svn_revert_command_finalize (GObject *object)
{
	SvnRevertCommand *self;
	
	self = SVN_REVERT_COMMAND (object);
	
	svn_command_free_path_list (self->priv->paths);
	g_free (self->priv);

	G_OBJECT_CLASS (svn_revert_command_parent_class)->finalize (object);
}

static guint
svn_revert_command_run (AnjutaCommand *command)
{
	SvnRevertCommand *self;
	SvnCommand *svn_command;
	GList *current_path;
	apr_array_header_t *revert_paths;
	svn_error_t *error;
	
	self = SVN_REVERT_COMMAND (command);
	svn_command = SVN_COMMAND (command);
	current_path = self->priv->paths;
	revert_paths = apr_array_make (svn_command_get_pool (svn_command),
								   g_list_length (self->priv->paths), 
								   sizeof (char *));
	
	while (current_path)
	{
		/* I just copied this so don't blame me... */
		(*((const char **) apr_array_push (revert_paths))) = current_path->data;
		current_path = g_list_next (current_path);
	}
	
	error = svn_client_revert (revert_paths, 
							   self->priv->recursive,
							   svn_command_get_client_context (svn_command), 
							   svn_command_get_pool (svn_command));

	if (error)
	{
		svn_command_set_error (svn_command, error);
		return 1;
	}
	
	return 0;
}

static void
svn_revert_command_class_init (SvnRevertCommandClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	AnjutaCommandClass *command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = svn_revert_command_finalize;
	command_class->run = svn_revert_command_run;
}


SvnRevertCommand *
svn_revert_command_new (GList *paths, gboolean recursive)
{
	SvnRevertCommand *self;
	
	self = g_object_new (SVN_TYPE_REVERT_COMMAND, NULL);
	self->priv->paths = svn_command_copy_path_list (paths);
	self->priv->recursive = recursive;
	
	return self;
}

void
svn_revert_command_destroy (SvnRevertCommand *self)
{
	g_object_unref (self);
}
