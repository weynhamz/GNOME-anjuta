/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * svn-backend.h (c) 2005 Johannes Schmid
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef SVN_BACKEND_H
#define SVN_BACKEND_H

#include <glib-object.h>
#include <gnome.h>
#include <libanjuta/anjuta-plugin.h>
#include <svn_client.h>

typedef struct _SVNBackend SVNBackend;
typedef struct _SVNBackendClass SVNBackendClass;

typedef struct _SVN SVN;

#include "plugin.h"	

#define SVN_BACKEND_TYPE            (svn_backend_get_type ())
#define SVN_BACKEND(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SVN_BACKEND_TYPE, SVNBackend))
#define SVN_BACKEND_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SVN_BACKEND_TYPE, SVNBackend))
#define IS_SVN_BACKEND(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SVN_BACKEND_TYPE))
#define IS_SVN_BACKEND_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SVN_BACKEND_TYPE))


struct _SVNBackend
{
	GObject object;
	
	SVN* svn;
	Subversion* plugin;
};

struct _SVNBackendClass
{
	GObjectClass klass;

};

GType svn_backend_get_type (void);
SVNBackend* svn_backend_new (Subversion* plugin);

gboolean svn_backend_busy(SVNBackend* backend);

void svn_backend_add(SVNBackend* backend, const gchar* filename,
					 gboolean force, gboolean recurse);
void svn_backend_remove(SVNBackend* backend, const gchar* filename,
						gboolean force);
void svn_backend_commit(SVNBackend* backend, const gchar* path, 
						const gchar* log, gboolean recurse);
void svn_backend_update(SVNBackend* backend, const gchar* path,
						const gchar* revision, gboolean recurse);
void svn_backend_diff(SVNBackend* backend, const gchar* path,
						const gchar* revision, gboolean recurse);
#endif

