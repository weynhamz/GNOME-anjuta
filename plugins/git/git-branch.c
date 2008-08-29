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

#include "git-branch.h"

struct _GitBranchPriv
{
	gchar *name;
	gboolean active;
};

G_DEFINE_TYPE (GitBranch, git_branch, G_TYPE_OBJECT);

static void
git_branch_init (GitBranch *self)
{
	self->priv = g_new0 (GitBranchPriv, 1);
}

static void
git_branch_finalize (GObject *object)
{
	GitBranch *self;
	
	self = GIT_BRANCH (object);
	
	g_free (self->priv->name);
	g_free (self->priv);

	G_OBJECT_CLASS (git_branch_parent_class)->finalize (object);
}

static void
git_branch_class_init (GitBranchClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = git_branch_finalize;
}


GitBranch *
git_branch_new (const gchar *name, gboolean active)
{
	GitBranch *self;
	
	self = g_object_new (GIT_TYPE_BRANCH, NULL);
	
	self->priv->name = g_strdup (name);
	self->priv->active = active;
	
	return self;
}

gchar *
git_branch_get_name (GitBranch *self)
{
	return g_strdup (self->priv->name);
}

gboolean
git_branch_is_active (GitBranch *self)
{
	return self->priv->active;
}
