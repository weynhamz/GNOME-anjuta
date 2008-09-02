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

#include "svn-remove-command.h"

struct _SvnRemoveCommandPriv
{
	gchar *path;
	gchar *log_message;
	gboolean force;
};

G_DEFINE_TYPE (SvnRemoveCommand, svn_remove_command, SVN_TYPE_COMMAND);

static svn_error_t*
on_log_callback (const char **log_msg,
				 const char **tmp_file,
				 apr_array_header_t *commit_items,
				 void *baton,
				 apr_pool_t *pool)
{
	
	SvnRemoveCommand *self;
	
	self = SVN_REMOVE_COMMAND (baton);
	
	*log_msg = self->priv->log_message;
	*tmp_file = NULL;
	
	return SVN_NO_ERROR;
}

static void
svn_remove_command_init (SvnRemoveCommand *self)
{
	svn_client_ctx_t *client_context;
	
	self->priv = g_new0 (SvnRemoveCommandPriv, 1);
	client_context = svn_command_get_client_context (SVN_COMMAND (self));
	
	client_context->log_msg_func = on_log_callback;
	client_context->log_msg_baton = self;
}

static void
svn_remove_command_finalize (GObject *object)
{
	SvnRemoveCommand *self;
	
	self = SVN_REMOVE_COMMAND (object);
	
	g_free (self->priv->path);
	g_free (self->priv->log_message);
	g_free (self->priv);

	G_OBJECT_CLASS (svn_remove_command_parent_class)->finalize (object);
}

static guint
svn_remove_command_run (AnjutaCommand *command)
{
	SvnRemoveCommand *self;
	SvnCommand *svn_command;
	apr_array_header_t *path_array;
	svn_error_t *error;
	svn_client_commit_info_t* commit_info;
	gchar *revision_message;
	
	self = SVN_REMOVE_COMMAND (command);
	svn_command = SVN_COMMAND (command);
	path_array = apr_array_make (svn_command_get_pool (SVN_COMMAND (command)), 
													   1, sizeof (char *));
								 
	/* I just copied this so don't blame me... */
	(*((const char **) apr_array_push (path_array))) = self->priv->path;
	
	error = svn_client_delete (&commit_info, 
							   path_array, 
							   self->priv->force,
							   svn_command_get_client_context (svn_command), 
							   svn_command_get_pool (svn_command));
	
	if (error)
	{
		svn_command_set_error (svn_command, error);
		return 1;
	}
	
	if (commit_info)
	{
		revision_message = g_strdup_printf ("Committed revision %ld.", 
											commit_info->revision);
		svn_command_push_info (SVN_COMMAND (command), revision_message);
		g_free (revision_message);
	}
	
	
	return 0;
	
}

static void
svn_remove_command_class_init (SvnRemoveCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaCommandClass *command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = svn_remove_command_finalize;
	command_class->run = svn_remove_command_run;
}

SvnRemoveCommand *
svn_remove_command_new (const gchar *path, const gchar *log_message, gboolean force)
{
	SvnRemoveCommand *self;
	
	self = g_object_new (SVN_TYPE_REMOVE_COMMAND, NULL);
	self->priv->path = svn_command_make_canonical_path (SVN_COMMAND (self),
														path);
	self->priv->log_message = g_strdup (log_message);
	self->priv->force = force;
	
	return self;
}

void
svn_remove_command_destroy (SvnRemoveCommand *self)
{
	g_object_unref (self);
}

gchar *
svn_remove_command_get_path (SvnRemoveCommand *self)
{
	return g_strdup (self->priv->path);
}
