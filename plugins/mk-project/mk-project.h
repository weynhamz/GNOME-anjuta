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
#include <libanjuta/anjuta-token-list.h>

G_BEGIN_DECLS

//#define YYSTYPE AnjutaToken*

#define MKP_TYPE_PROJECT		(mkp_project_get_type ())
#define MKP_PROJECT(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), MKP_TYPE_PROJECT, MkpProject))
#define MKP_PROJECT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), MKP_TYPE_PROJECT, MkpProjectClass))
#define MKP_IS_PROJECT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MKP_TYPE_PROJECT))
#define MKP_IS_PROJECT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((obj), MKP_TYPE_PROJECT))

#define MKP_TYPE_GROUP					   (mkp_group_get_type ())
#define MKP_GROUP(obj)							(G_TYPE_CHECK_INSTANCE_CAST ((obj), MKP_TYPE_GROUP, MkpGroup))
#define MKP_TYPE_TARGET					   (mkp_target_get_type ())
#define MKP_TARGET(obj)							(G_TYPE_CHECK_INSTANCE_CAST ((obj), MKP_TYPE_TARGET, MkpTarget))
#define MKP_TYPE_OBJECT					   (mkp_object_get_type ())
#define MKP_OBJECT(obj)							(G_TYPE_CHECK_INSTANCE_CAST ((obj), MKP_TYPE_OBJECT, MkpObject))
#define MKP_TYPE_SOURCE					   (mkp_source_get_type ())
#define MKP_SOURCE(obj)							(G_TYPE_CHECK_INSTANCE_CAST ((obj), MKP_TYPE_SOURCE, MkpSource))


GType mkp_source_get_type (void) G_GNUC_CONST;
GType mkp_object_get_type (void) G_GNUC_CONST;
GType mkp_target_get_type (void) G_GNUC_CONST;
GType mkp_group_get_type (void) G_GNUC_CONST;
GType mkp_project_get_type (void) G_GNUC_CONST;

typedef struct _MkpProject        MkpProject;
typedef struct _MkpProjectClass   MkpProjectClass;

typedef struct _MkpGroup MkpGroup;
typedef struct _MkpTarget MkpTarget;
typedef struct _MkpObject MkpObject;
typedef struct _MkpSource MkpSource;
typedef struct _MkpProperty MkpProperty;
typedef struct _MkpVariable MkpVariable;
typedef struct _MkpRule MkpRule;

typedef struct _MkpNodeClass MkpSourceClass;
typedef struct _MkpNodeClass MkpObjectClass;
typedef struct _MkpNodeClass MkpTargetClass;
typedef struct _MkpNodeClass MkpGroupClass;

struct _MkpVariable {
	gchar *name;
	AnjutaTokenType assign;
	AnjutaToken *value;
};

struct _MkpGroup {
	AnjutaProjectNode base;
};

struct _MkpTarget {
	AnjutaProjectNode base;
	GList* tokens;
};

struct _MkpObject {
	AnjutaProjectNode base;
};

struct _MkpSource {
	AnjutaProjectNode base;
	AnjutaToken* token;
};

typedef struct _MkpNodeInfo MkpNodeInfo;

struct _MkpNodeInfo {
	AnjutaProjectNodeInfo base;
};

struct _MkpNodeClass {
	AnjutaProjectNodeClass parent_class;
};

struct _MkpProjectClass {
	AnjutaProjectNodeClass parent_class;
};


GType         mkp_project_get_type (void);
MkpProject   *mkp_project_new (GFile *file, GError  **error);


gint mkp_project_probe (GFile *directory, GError     **error);
gboolean mkp_project_load (MkpProject *project, GFile *directory, GError **error);
AnjutaProjectNode* mkp_project_load_node (MkpProject *project, AnjutaProjectNode *node, GError **error);
gboolean mkp_project_reload (MkpProject *project, GError **error);
void mkp_project_unload (MkpProject *project);

MkpProject *mkp_project_get_root (MkpProject *project);
AnjutaToken* mkp_project_get_variable_token (MkpProject *project, AnjutaToken *variable);

void mkp_project_update_variable (MkpProject *project, AnjutaToken *variable);
void mkp_project_add_rule (MkpProject *project, AnjutaToken *rule);

gboolean mkp_project_move (MkpProject *project, const gchar *path);
gboolean mkp_project_save (MkpProject *project, GError **error);

gboolean mkp_project_get_token_location (MkpProject *project, AnjutaTokenFileLocation *location, AnjutaToken *token);

gchar *mkp_variable_evaluate (MkpVariable *variable, MkpProject *project);
const gchar* mkp_variable_get_name (MkpVariable *variable);


G_END_DECLS

#endif /* _MK_PROJECT_H_ */
