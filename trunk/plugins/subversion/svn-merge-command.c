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

#include "svn-merge-command.h"

struct _SvnMergeCommandPriv
{
	gchar *path1;
	gchar *path2;
	glong start_revision;
	glong end_revision;
	gchar *target_path;
	gboolean recursive;
	gboolean ignore_ancestry;
	gboolean force;
	gboolean dry_run;
};

G_DEFINE_TYPE (SvnMergeCommand, svn_merge_command, SVN_TYPE_COMMAND);

static void
svn_merge_command_init (SvnMergeCommand *self)
{	
	self->priv = g_new0 (SvnMergeCommandPriv, 1);
}

static void
svn_merge_command_finalize (GObject *object)
{
	SvnMergeCommand *self;
	
	self = SVN_MERGE_COMMAND (object);
	
	g_free (self->priv->path1);
	g_free (self->priv->path2);
	g_free (self->priv->target_path);
	g_free (self->priv);

	G_OBJECT_CLASS (svn_merge_command_parent_class)->finalize (object);
}

static guint
svn_merge_command_run (AnjutaCommand *command)
{
	SvnMergeCommand *self;
	SvnCommand *svn_command;
	svn_opt_revision_t start_revision;
	svn_opt_revision_t end_revision;
	svn_error_t *error;
	
	self = SVN_MERGE_COMMAND (command);
	svn_command = SVN_COMMAND (command);
	
	if (self->priv->start_revision == SVN_MERGE_REVISION_HEAD)
		start_revision.kind = svn_opt_revision_head;
	else
	{
		start_revision.kind = svn_opt_revision_number;
		start_revision.value.number = self->priv->start_revision;
	}
	
	if (self->priv->end_revision == SVN_MERGE_REVISION_HEAD)
		end_revision.kind = svn_opt_revision_head;
	else
	{
		end_revision.kind = svn_opt_revision_number;
		end_revision.value.number = self->priv->end_revision;
	}
	
	error = svn_client_merge2 (self->priv->path1,
							   &start_revision,
							   self->priv->path2,
							   &end_revision,
							   self->priv->target_path,
							   self->priv->recursive,
							   self->priv->ignore_ancestry,
							   self->priv->force,
							   self->priv->dry_run,
							   NULL,
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
svn_merge_command_class_init (SvnMergeCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaCommandClass* command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = svn_merge_command_finalize;
	command_class->run = svn_merge_command_run;
}

SvnMergeCommand *
svn_merge_command_new (gchar *path1, 
					   gchar *path2, 
					   glong start_revision, 
					   glong end_revision, 
					   gchar *target_path, 
					   gboolean recursive, 
					   gboolean ignore_ancestry, 
					   gboolean force, 
					   gboolean dry_run)
{
	SvnMergeCommand *self;
	
	self = g_object_new (SVN_TYPE_MERGE_COMMAND, NULL);
	self->priv->path1 = svn_command_make_canonical_path (SVN_COMMAND (self),
														path1);
	self->priv->path2 = svn_command_make_canonical_path (SVN_COMMAND (self),
														 path2);
	self->priv->start_revision = start_revision;
	self->priv->end_revision = end_revision;
	self->priv->target_path = svn_command_make_canonical_path (SVN_COMMAND (self),
															   target_path);
	self->priv->recursive = recursive;
	self->priv->ignore_ancestry = ignore_ancestry;
	self->priv->force = force;
	self->priv->dry_run = dry_run;
	
	return self;
}

void
svn_merge_command_destroy (SvnMergeCommand *self)
{
	g_object_unref (self);
}
