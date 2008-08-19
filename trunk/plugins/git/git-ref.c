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

#include "git-ref.h"

struct _GitRefPriv
{
	gchar *name;
	GitRefType type;
};

G_DEFINE_TYPE (GitRef, git_ref, G_TYPE_OBJECT);

static void
git_ref_init (GitRef *self)
{
	self->priv = g_new0 (GitRefPriv, 1);
}

static void
git_ref_finalize (GObject *object)
{
	GitRef *self;
	
	self = GIT_REF (object);
	
	g_free (self->priv->name);
	g_free (self->priv);

	G_OBJECT_CLASS (git_ref_parent_class)->finalize (object);
}

static void
git_ref_class_init (GitRefClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = git_ref_finalize;
}


GitRef *
git_ref_new (const gchar *name, GitRefType type)
{
	GitRef *self;
	
	self = g_object_new (GIT_TYPE_REF, NULL);
	
	self->priv->name = g_strdup (name);
	self->priv->type = type;
	
	return self;
}

gchar *
git_ref_get_name (GitRef *self)
{
	return g_strdup (self->priv->name);
}

GitRefType
git_ref_get_ref_type (GitRef *self)
{
	return self->priv->type;
}
