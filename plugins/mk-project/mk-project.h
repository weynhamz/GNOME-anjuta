/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* mk-project.h
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

#ifndef _MK_PROJECT_H_
#define _MK_PROJECT_H_

#include <glib-object.h>

#include <libanjuta/anjuta-project.h>
#include <libanjuta/anjuta-token.h>
#include <libanjuta/anjuta-token-file.h>
#include <libanjuta/anjuta-token-style.h>

G_BEGIN_DECLS

#define YYSTYPE AnjutaToken*

#define MKP_TYPE_PROJECT		(mkp_project_get_type ())
#define MKP_PROJECT(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), MKP_TYPE_PROJECT, MkpProject))
#define MKP_PROJECT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), MKP_TYPE_PROJECT, MkpProjectClass))
#define MKP_IS_PROJECT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MKP_TYPE_PROJECT))
#define MKP_IS_PROJECT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((obj), MKP_TYPE_PROJECT))

#define MKP_GROUP(obj)		((MkpGroup *)obj)
#define MKP_TARGET(obj)		((MkpTarget *)obj)
#define MKP_SOURCE(obj)		((MkpSource *)obj)

typedef struct _MkpProject        MkpProject;
typedef struct _MkpProjectClass   MkpProjectClass;

struct _MkpProjectClass {
	GObjectClass parent_class;
};

typedef AnjutaProjectGroup MkpGroup;
typedef AnjutaProjectTarget MkpTarget;
typedef AnjutaProjectSource MkpSource;
typedef struct _MkpProperty MkpProperty;
typedef struct _MkpVariable MkpVariable;
typedef struct _MkpRule MkpRule;

typedef enum {
	MKP_PROPERTY_NAME = 0,
	MKP_PROPERTY_VERSION,
	MKP_PROPERTY_BUG_REPORT,
	MKP_PROPERTY_TARNAME,
	MKP_PROPERTY_URL
} MkpPropertyType;


GType         mkp_project_get_type (void);
MkpProject   *mkp_project_new      (void);


gint mkp_project_probe (GFile *directory, GError     **error);
gboolean mkp_project_load (MkpProject *project, GFile *directory, GError **error);
gboolean mkp_project_reload (MkpProject *project, GError **error);
void mkp_project_unload (MkpProject *project);

MkpGroup *mkp_project_get_root (MkpProject *project);
MkpVariable *mkp_project_get_variable (MkpProject *project, const gchar *name);
GList *mkp_project_list_variable (MkpProject *project);

MkpGroup *mkp_project_get_group (MkpProject *project, const gchar *id);
MkpTarget *mkp_project_get_target (MkpProject *project, const gchar *id);
MkpSource *mkp_project_get_source (MkpProject *project, const gchar *id);

gboolean mkp_project_move (MkpProject *project, const gchar *path);
gboolean mkp_project_save (MkpProject *project, GError **error);

gchar * mkp_project_get_uri (MkpProject *project);
GFile* mkp_project_get_file (MkpProject *project);

MkpGroup* mkp_project_add_group (MkpProject  *project, MkpGroup *parent,	const gchar *name, GError **error);
void mkp_project_remove_group (MkpProject  *project, MkpGroup *group, GError **error);

MkpTarget* mkp_project_add_target (MkpProject  *project, MkpGroup *parent, const gchar *name, const gchar *type, GError **error);
void mkp_project_remove_target (MkpProject  *project, MkpTarget *target, GError **error);

MkpSource* mkp_project_add_source (MkpProject  *project, MkpTarget *target, const gchar *uri, GError **error);
void mkp_project_remove_source (MkpProject  *project, MkpSource *source, GError **error);

gchar * mkp_project_get_node_id (MkpProject *project, const gchar *path);

GFile *mkp_group_get_directory (MkpGroup *group);
GFile *mkp_group_get_makefile (MkpGroup *group);
gchar *mkp_group_get_id (MkpGroup *group);

const gchar *mkp_target_get_name (MkpTarget *target);
AnjutaProjectTargetType amp_target_get_type (MkpTarget *target);
gchar *mkp_target_get_id (MkpTarget *target);

gchar *mkp_source_get_id (MkpSource *source);
GFile *mkp_source_get_file (MkpSource *source);

gchar *mkp_variable_evaluate (MkpVariable *variable, AnjutaProjectNode *context);
const gchar* mkp_variable_get_name (MkpVariable *variable);

G_END_DECLS

#endif /* _MK_PROJECT_H_ */
