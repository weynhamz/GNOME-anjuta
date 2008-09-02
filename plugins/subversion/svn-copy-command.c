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

#include "svn-copy-command.h"

struct _SvnCopyCommandPriv
{
	gchar *source_path;
	glong source_revision;
	gchar *dest_path;
	gchar *log_message;
};

G_DEFINE_TYPE (SvnCopyCommand, svn_copy_command, SVN_TYPE_COMMAND);

static svn_error_t*
on_log_callback (const char **log_msg,
				 const char **tmp_file,
				 apr_array_header_t *commit_items,
				 void *baton,
				 apr_pool_t *pool)
{
	
	SvnCopyCommand *self;
	
	self = SVN_COPY_COMMAND (baton);
	
	*log_msg = self->priv->log_message;
	*tmp_file = NULL;
	
	return SVN_NO_ERROR;
}

static void
svn_copy_command_init (SvnCopyCommand *self)
{
	svn_client_ctx_t *client_context;
	
	self->priv = g_new0 (SvnCopyCommandPriv, 1);
	client_context = svn_command_get_client_context (SVN_COMMAND (self));
	
	client_context->log_msg_func = on_log_callback;
	client_context->log_msg_baton = self;
}	

static void
svn_copy_command_finalize (GObject *object)
{
	SvnCopyCommand *self;
	
	self = SVN_COPY_COMMAND (object);
	
	g_free (self->priv->source_path);
	g_free (self->priv->dest_path);
	g_free (self->priv->log_message);
	g_free (self->priv);

	G_OBJECT_CLASS (svn_copy_command_parent_class)->finalize (object);
}

static guint
svn_copy_command_run (AnjutaCommand *command)
{
	SvnCopyCommand *self;
	SvnCommand *svn_command;
	svn_opt_revision_t revision;
	svn_commit_info_t *commit_info;
	gchar *revision_message;
	svn_error_t *error;
	
	self = SVN_COPY_COMMAND (command);
	svn_command = SVN_COMMAND (command);
	
	switch (self->priv->source_revision)
	{
		case SVN_COPY_REVISION_WORKING:
			revision.kind = svn_opt_revision_working;
			break;
		case SVN_COPY_REVISION_HEAD:
			revision.kind = svn_opt_revision_head;
			break;
		default:
			revision.kind = svn_opt_revision_number;
			revision.value.number = self->priv->source_revision;
			break;
	}
	
	error = svn_client_copy3 (&commit_info,
							  self->priv->source_path,
							  &revision,
							  self->priv->dest_path,
							  svn_command_get_client_context (svn_command),
							  svn_command_get_pool (svn_command));
	
	if (error)
	{
		svn_command_set_error (svn_command, error);
		return 1;
	}
	
	if (commit_info &&
		svn_path_is_url (self->priv->dest_path))
	{
		revision_message = g_strdup_printf ("Committed revision %ld.", 
											commit_info->revision);
		svn_command_push_info (SVN_COMMAND (command), revision_message);
		g_free (revision_message);
	}
	
	return 0;
}

static void
svn_copy_command_class_init (SvnCopyCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaCommandClass* command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = svn_copy_command_finalize;
	command_class->run = svn_copy_command_run;
}

SvnCopyCommand *
svn_copy_command_new (const gchar *source_path, glong source_revision, 
					  const gchar *dest_path, const gchar *log_message)
{
	SvnCopyCommand *self;
	
	self = g_object_new (SVN_TYPE_COPY_COMMAND, NULL);
	
	self->priv->source_path = svn_command_make_canonical_path (SVN_COMMAND (self),
															   source_path);
	self->priv->source_revision = source_revision;
	self->priv->dest_path = svn_command_make_canonical_path (SVN_COMMAND (self),
															 dest_path);
	self->priv->log_message = g_strdup (log_message);
	
	return self;
}

void
svn_copy_command_destroy (SvnCopyCommand *self)
{
	g_object_unref (self);
}
