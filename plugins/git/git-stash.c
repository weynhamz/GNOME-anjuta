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

#include "git-stash.h"

struct _GitStashPriv
{
	gchar *id;
	gchar *message;
	guint number;
};

G_DEFINE_TYPE (GitStash, git_stash, G_TYPE_OBJECT);

static void
git_stash_init (GitStash *self)
{
	self->priv = g_new0 (GitStashPriv, 1);
}

static void
git_stash_finalize (GObject *object)
{
	GitStash *self;
	
	self = GIT_STASH (object);
	
	g_free (self->priv->id);
	g_free (self->priv->message);
	g_free (self->priv);

	G_OBJECT_CLASS (git_stash_parent_class)->finalize (object);
}

static void
git_stash_class_init (GitStashClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = git_stash_finalize;
}

GitStash *
git_stash_new (const gchar *id, const gchar *message, guint number)
{
	GitStash *self;
	
	self = g_object_new (GIT_TYPE_STASH, NULL);
	
	self->priv->id = g_strdup (id);
	self->priv->message = g_strdup (message);
	self->priv->number = number;
	
	return self;
}

gchar *
git_stash_get_id (GitStash *self)
{
	return g_strdup (self->priv->id);
}

gchar *
git_stash_get_message (GitStash *self)
{
	return g_strdup (self->priv->message);
}

guint
git_stash_get_number (GitStash *self)
{
	return self->priv->number;
}