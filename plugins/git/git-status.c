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

#include "git-status.h"

struct _GitStatusPriv
{
	gchar *path;
	AnjutaVcsStatus status;
};

G_DEFINE_TYPE (GitStatus, git_status, G_TYPE_OBJECT);

static void
git_status_init (GitStatus *self)
{
	self->priv = g_new0 (GitStatusPriv, 1);
}

static void
git_status_finalize (GObject *object)
{
	GitStatus *self;
	
	self = GIT_STATUS (object);
	
	g_free (self->priv->path);
	g_free (self->priv);

	G_OBJECT_CLASS (git_status_parent_class)->finalize (object);
}

static void
git_status_class_init (GitStatusClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = git_status_finalize;
}

GitStatus *
git_status_new (const gchar *path, AnjutaVcsStatus status)
{
	GitStatus *self;
	
	self = g_object_new (GIT_TYPE_STATUS, NULL);
	
	self->priv->path = g_strdup (path);
	self->priv->status = status;
	
	return self;
}

gchar *
git_status_get_path (GitStatus *self)
{
	return g_strdup (self->priv->path);
}

AnjutaVcsStatus
git_status_get_vcs_status (GitStatus *self)
{
	return self->priv->status;
}

