/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) 2008  Ignacio Casal Quinteiro <nacho.resa@gmail.com>
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

#include "svn-checkout-command.h"

struct _SvnCheckoutCommandPriv
{
	gchar *url;
	gchar *path;
};

G_DEFINE_TYPE (SvnCheckoutCommand, svn_checkout_command, SVN_TYPE_COMMAND);

static void
svn_checkout_command_init (SvnCheckoutCommand *self)
{	
	self->priv = g_new0 (SvnCheckoutCommandPriv, 1);
}

static void
svn_checkout_command_finalize (GObject *object)
{
	SvnCheckoutCommand *self;
	
	self = SVN_CHECKOUT_COMMAND (object);
	
	g_free (self->priv->url);
	g_free (self->priv->path);
	g_free (self->priv);

	G_OBJECT_CLASS (svn_checkout_command_parent_class)->finalize (object);
}

static guint
svn_checkout_command_run (AnjutaCommand *command)
{
	SvnCheckoutCommand *self;
	SvnCommand *svn_command;
	svn_opt_revision_t revision;
	svn_opt_revision_t peg_revision;
	svn_error_t *error;
	svn_revnum_t revision_number;
	gchar *revision_message;
	
	self = SVN_CHECKOUT_COMMAND (command);
	svn_command = SVN_COMMAND (command);
	
	revision.kind = svn_opt_revision_head;
	peg_revision.kind = svn_opt_revision_unspecified;
	
	error = svn_client_checkout3 (&revision_number,
				  				  self->priv->url,
				  				  self->priv->path,
				  				  &peg_revision,
				  				  &revision,
				  				  svn_depth_unknown,
				  				  TRUE,
				  				  FALSE,
				  				  svn_command_get_client_context (svn_command),
				  				  svn_command_get_pool (svn_command));
	
	if (error)
	{
		svn_command_set_error (svn_command, error);
		return 1;
	}
	
	revision_message = g_strdup_printf ("Checked out revision %ld.", 
										(glong) revision_number);
	svn_command_push_info (SVN_COMMAND (command), revision_message);
	g_free (revision_message);
	
	return 0;
}

static void
svn_checkout_command_class_init (SvnCheckoutCommandClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	AnjutaCommandClass *command_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = svn_checkout_command_finalize;
	command_class->run = svn_checkout_command_run;
}


SvnCheckoutCommand *
svn_checkout_command_new (const gchar *url, const gchar *path)
{
	SvnCheckoutCommand *self;
	
	self = g_object_new (SVN_TYPE_CHECKOUT_COMMAND, NULL);
	
	self->priv->url = svn_command_make_canonical_path (SVN_COMMAND (self),
													   (gchar*) url);
	self->priv->path = svn_command_make_canonical_path (SVN_COMMAND (self),
														(gchar*) path);
	
	return self;
}

void
svn_checkout_command_destroy (SvnCheckoutCommand *self)
{
	g_object_unref (self);
}
