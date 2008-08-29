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

#ifndef _GIT_BRANCH_H_
#define _GIT_BRANCH_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define GIT_TYPE_BRANCH             (git_branch_get_type ())
#define GIT_BRANCH(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_BRANCH, GitBranch))
#define GIT_BRANCH_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_BRANCH, GitBranchClass))
#define GIT_IS_BRANCH(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_BRANCH))
#define GIT_IS_BRANCH_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_BRANCH))
#define GIT_BRANCH_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_BRANCH, GitBranchClass))

typedef struct _GitBranchClass GitBranchClass;
typedef struct _GitBranch GitBranch;
typedef struct _GitBranchPriv GitBranchPriv;

struct _GitBranchClass
{
	GObjectClass parent_class;
};

struct _GitBranch
{
	GObject parent_instance;
	
	GitBranchPriv *priv;
};

GType git_branch_get_type (void) G_GNUC_CONST;
GitBranch *git_branch_new (const gchar *name, gboolean active);
gchar *git_branch_get_name (GitBranch *self);
gboolean git_branch_is_active (GitBranch *self);

G_END_DECLS

#endif /* _GIT_BRANCH_H_ */
