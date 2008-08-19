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

#ifndef _GIT_REF_H_
#define _GIT_REF_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define GIT_TYPE_REF             (git_ref_get_type ())
#define GIT_REF(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_REF, GitRef))
#define GIT_REF_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_REF, GitRefClass))
#define GIT_IS_REF(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_REF))
#define GIT_IS_REF_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_REF))
#define GIT_REF_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_REF, GitRefClass))

typedef struct _GitRefClass GitRefClass;
typedef struct _GitRef GitRef;
typedef struct _GitRefPriv GitRefPriv;

typedef enum
{
	GIT_REF_TYPE_BRANCH,
	GIT_REF_TYPE_TAG,
	GIT_REF_TYPE_REMOTE
} GitRefType;

struct _GitRefClass
{
	GObjectClass parent_class;
};

struct _GitRef
{
	GObject parent_instance;
	
	GitRefPriv *priv;
};

GType git_ref_get_type (void) G_GNUC_CONST;
GitRef *git_ref_new (const gchar *name, GitRefType type);
gchar *git_ref_get_name (GitRef *self);
GitRefType git_ref_get_ref_type (GitRef *self);

G_END_DECLS

#endif /* _GIT_REF_H_ */
