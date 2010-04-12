/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* am-project.h
 *
 * Copyright (C) 2009  Sébastien Granjoux
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _DIR_PROJECT_H_
#define _DIR_PROJECT_H_

#include <glib-object.h>

#include <libanjuta/anjuta-project.h>

G_BEGIN_DECLS

#define DIR_TYPE_PROJECT			(dir_project_get_type ())
#define DIR_PROJECT(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), DIR_TYPE_PROJECT, DirProject))
#define DIR_PROJECT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), DIR_TYPE_PROJECT, DirProjectClass))
#define DIR_IS_PROJECT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIR_TYPE_PROJECT))
#define DIR_IS_PROJECT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((obj), DIR_TYPE_PROJECT))

#define DIR_GROUP(obj)		((DirGroup *)obj)
#define DIR_TARGET(obj)		((DirTarget *)obj)
#define DIR_SOURCE(obj)		((DirSource *)obj)


typedef struct _DirProject        DirProject;
typedef struct _DirProjectClass   DirProjectClass;

struct _DirProjectClass {
	GObjectClass parent_class;
};

typedef AnjutaProjectNode DirGroup;
typedef AnjutaProjectNode DirTarget;
typedef AnjutaProjectNode DirSource;


GType         dir_project_get_type (void);
DirProject   *dir_project_new      (void);

gint dir_project_probe (GFile *directory, GError     **error);

gboolean dir_project_load (DirProject *project, GFile *directory, GError **error);
AnjutaProjectNode *dir_project_load_node (DirProject *project, AnjutaProjectNode *node, GError **error);
gboolean dir_project_reload (DirProject *project, GError **error);
void dir_project_unload (DirProject *project);

G_END_DECLS

#endif /* _DIR_PROJECT_H_ */
