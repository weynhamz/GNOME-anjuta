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

#ifndef _GIT_STATUS_H_
#define _GIT_STATUS_H_

#include <glib-object.h>
#include <libanjuta/anjuta-vcs-status-tree-view.h>

G_BEGIN_DECLS

#define GIT_TYPE_STATUS             (git_status_get_type ())
#define GIT_STATUS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_STATUS, GitStatus))
#define GIT_STATUS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_STATUS, GitStatusClass))
#define GIT_IS_STATUS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_STATUS))
#define GIT_IS_STATUS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_STATUS))
#define GIT_STATUS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_STATUS, GitStatusClass))

typedef struct _GitStatusClass GitStatusClass;
typedef struct _GitStatus GitStatus;
typedef struct _GitStatusPriv GitStatusPriv;

struct _GitStatusClass
{
	GObjectClass parent_class;
};

struct _GitStatus
{
	GObject parent_instance;
	
	GitStatusPriv *priv;
};

GType git_status_get_type (void) G_GNUC_CONST;
GitStatus *git_status_new (const gchar *path, const gchar *status);
gchar *git_status_get_path (GitStatus *self);
AnjutaVcsStatus git_status_get_vcs_status (GitStatus *self);

G_END_DECLS

#endif /* _GIT_STATUS_H_ */
