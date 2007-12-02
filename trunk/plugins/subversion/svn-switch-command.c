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

#include "svn-switch-command.h"

struct _SvnSwitchCommandPriv
{
	gchar *working_copy_path;
	gchar *branch_url;
	glong revision;
	gboolean recursive;
};

G_DEFINE_TYPE (SvnSwitchCommand, svn_switch_command, SVN_TYPE_COMMAND);

static void
svn_switch_command_init (SvnSwitchCommand *self)
{	
	self->priv = g_new0 (SvnSwitchCommandPriv, 1);
}

static void
svn_switch_command_finalize (GObject *object)
{
	SvnSwitchCommand *self;
	
	self = SVN_SWITCH_COMMAND (object);
	
	g_free (self->priv->working_copy_path);
	g_free (self->priv->branch_url);
	g_free (self->priv);

	G_OBJECT_CLASS (svn_switch_command_parent_class)->finalize (object);
}

static guint
svn_switch_command_run (AnjutaCommand *command)
{
	SvnSwitchCommand *self;
	SvnCommand *svn_command;
	svn_opt_revision_t revision;
	svn_revnum_t switched_revision;
	gchar *revision_message;
	svn_error_t *error;
	
	self = SVN_SWITCH_COMMAND (command);
	svn_command = SVN_COMMAND (command);
	
	if (self->priv->revision == SVN_SWITCH_REVISION_HEAD)
		revision.kind = svn_opt_revision_head;
	else
	{
		revision.kind = svn_opt_revision_number;
		revision.value.number = self->priv->revision;
	}
	
	error = svn_client_switch (&switched_revision,
							   self->priv->working_copy_path,
							   self->priv->branch_url,
							   &revision,
							   self->priv->recursive,
							   svn_command_get_client_context (svn_command),
							   svn_command_get_pool (svn_command));
							   
	if (error)
	{
		svn_command_set_error (svn_command, error);
		return 1;
	}
	
	revision_message = g_strdup_printf ("Switched to revision %ld.", 
										switched_revision);
	
	svn_command_push_info (svn_command, revision_message);
	g_free (revision_message);
	
	return 0;
}

static void
svn_switch_command_class_init (SvnSwitchCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaCommandClass* command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = svn_switch_command_finalize;
	command_class->run = svn_switch_command_run;
}

SvnSwitchCommand *
svn_switch_command_new (gchar *working_copy_path, gchar *branch_url, 
						glong revision, gboolean recursive)
{
	SvnSwitchCommand *self;
	
	self = g_object_new (SVN_TYPE_SWITCH_COMMAND, NULL);
	self->priv->working_copy_path = g_strdup (working_copy_path);
	self->priv->branch_url = g_strdup (branch_url);
	self->priv->revision = revision;
	self->priv->recursive = recursive;
	
	return self;
}

void
svn_switch_command_destroy (SvnSwitchCommand *self)
{
	g_object_unref (self);
}
