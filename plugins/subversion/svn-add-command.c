/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2007 <jrliggett@cox.net>
 *
 * Portions based on the original Subversion plugin 
 * Copyright (C) Johannes Schmid 2005 
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

#include "svn-add-command.h"

struct _SvnAddCommandPriv
{
	GList *paths;
	gboolean force;
	gboolean recursive;
};

G_DEFINE_TYPE (SvnAddCommand, svn_add_command, SVN_TYPE_COMMAND);

static void
svn_add_command_init (SvnAddCommand *self)
{
	self->priv = g_new0 (SvnAddCommandPriv, 1);
}

static void
svn_add_command_finalize (GObject *object)
{
	SvnAddCommand *self;
	
	self = SVN_ADD_COMMAND (object);
	
	svn_command_free_path_list (self->priv->paths);
	g_free (self->priv);

	G_OBJECT_CLASS (svn_add_command_parent_class)->finalize (object);
}

static guint
svn_add_command_run (AnjutaCommand *command)
{
	SvnAddCommand *self;
	SvnCommand *svn_command;
	GList *current_path;
	svn_error_t *error;
	
	self = SVN_ADD_COMMAND (command);
	svn_command = SVN_COMMAND (command);
	current_path = self->priv->paths;

	while (current_path)
	{
		error = svn_client_add2 (current_path->data,  
								 self->priv->force, 
								 self->priv->recursive, 
								 svn_command_get_client_context (svn_command), 
								 svn_command_get_pool (svn_command));
		
		if (error)
		{
			svn_command_set_error (svn_command, error);
			return 1;
		}
		
		current_path = g_list_next (current_path);
	}
	
	return 0;
}

static void
svn_add_command_class_init (SvnAddCommandClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	AnjutaCommandClass *command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = svn_add_command_finalize;
	command_class->run = svn_add_command_run;
}


SvnAddCommand *
svn_add_command_new_path (const gchar *path, gboolean force, gboolean recursive)
{
	SvnAddCommand *self;
	
	self = g_object_new (SVN_TYPE_ADD_COMMAND, NULL);
	self->priv->paths = g_list_append (self->priv->paths,  
									   svn_command_make_canonical_path (SVN_COMMAND (self),
																		path));
	self->priv->force = force;
	self->priv->recursive = recursive;
	
	return self;
}

SvnAddCommand *
svn_add_command_new_list (GList *paths, gboolean force, gboolean recursive)
{
	SvnAddCommand *self;
	GList *current_path;
	
	self = g_object_new (SVN_TYPE_ADD_COMMAND, NULL);
	current_path = paths;
	
	while (current_path)
	{
		self->priv->paths = g_list_append (self->priv->paths,  
										   svn_command_make_canonical_path (SVN_COMMAND (self),
																			current_path->data));
		current_path = g_list_next (current_path);
	}
	
	self->priv->force = force;
	self->priv->recursive = recursive;
	
	return self;	
}

void
svn_add_command_destroy (SvnAddCommand *self)
{
	g_object_unref (self);
}
