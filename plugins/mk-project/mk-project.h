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
#define MKP_TYPE_SOURCE					   (mkp_source_get_type ())
#define MKP_SOURCE(obj)							(G_TYPE_CHECK_INSTANCE_CAST ((obj), MKP_TYPE_SOURCE, MkpSource))


GType mkp_source_get_type (void) G_GNUC_CONST;
GType mkp_target_get_type (void) G_GNUC_CONST;
GType mkp_group_get_type (void) G_GNUC_CONST;
GType mkp_project_get_type (void) G_GNUC_CONST;

typedef struct _MkpProject        MkpProject;
typedef struct _MkpProjectClass   MkpProjectClass;

typedef struct _MkpGroup MkpGroup;
typedef struct _MkpTarget MkpTarget;
typedef struct _MkpSource MkpSource;
typedef struct _MkpProperty MkpProperty;
typedef struct _MkpVariable MkpVariable;
typedef struct _MkpRule MkpRule;

typedef struct _MkpClass MkpSourceClass;
typedef struct _MkpClass MkpTargetClass;
typedef struct _MkpClass MkpGroupClass;

struct _MkpVariable {
	gchar *name;
	AnjutaTokenType assign;
	AnjutaToken *value;
};

typedef enum {
	AM_GROUP_TOKEN_CONFIGURE,
	AM_GROUP_TOKEN_SUBDIRS,
	AM_GROUP_TOKEN_DIST_SUBDIRS,
	AM_GROUP_TARGET,
	AM_GROUP_TOKEN_LAST
} MkpGroupTokenCategory;

struct _MkpGroup {
	AnjutaProjectNode base;		/* Common node data */
	gboolean dist_only;			/* TRUE if the group is distributed but not built */
	GFile *makefile;				/* GFile corresponding to group makefile */
	AnjutaTokenFile *tfile;		/* Corresponding Makefile */
	GList *tokens[AM_GROUP_TOKEN_LAST];					/* List of token used by this group */
};

typedef enum _MkpTargetFlag
{
	AM_TARGET_CHECK = 1 << 0,
	AM_TARGET_NOINST = 1 << 1,
	AM_TARGET_DIST = 1 << 2,
	AM_TARGET_NODIST = 1 << 3,
	AM_TARGET_NOBASE = 1 << 4,
	AM_TARGET_NOTRANS = 1 << 5,
	AM_TARGET_MAN = 1 << 6,
	AM_TARGET_MAN_SECTION = 31 << 7
} MkpTargetFlag;

struct _MkpTarget {
	AnjutaProjectNode base;
	gchar *install;
	gint flags;
	GList* tokens;
};

struct _MkpSource {
	AnjutaProjectNode base;
	AnjutaToken* token;
};

typedef struct _MkpNodeInfo MkpNodeInfo;

struct _MkpNodeInfo {
	AnjutaProjectNodeInfo base;
	AnjutaTokenType token;
	const gchar *prefix;
	const gchar *install;
};

struct _MkpClass {
	AnjutaProjectNodeClass parent_class;
};

struct _MkpProjectClass {
	AnjutaProjectNodeClass parent_class;
};


typedef enum {
	MKP_PROPERTY_NAME = 0,
	MKP_PROPERTY_VERSION,
	MKP_PROPERTY_BUG_REPORT,
	MKP_PROPERTY_TARNAME,
	MKP_PROPERTY_URL
} MkpPropertyType;


GType         mkp_project_get_type (void);
MkpProject   *mkp_project_new (GFile *file, GError  **error);


gint mkp_project_probe (GFile *directory, GError     **error);
gboolean mkp_project_load (MkpProject *project, GFile *directory, GError **error);
AnjutaProjectNode* mkp_project_load_node (MkpProject *project, AnjutaProjectNode *node, GError **error);
gboolean mkp_project_reload (MkpProject *project, GError **error);
void mkp_project_unload (MkpProject *project);

MkpProject *mkp_project_get_root (MkpProject *project);
MkpVariable *mkp_project_get_variable (MkpProject *project, const gchar *name);
GList *mkp_project_list_variable (MkpProject *project);
AnjutaToken* mkp_project_get_variable_token (MkpProject *project, AnjutaToken *variable);

void mkp_project_update_variable (MkpProject *project, AnjutaToken *variable);
void mkp_project_add_rule (MkpProject *project, AnjutaToken *rule);

MkpGroup *mkp_project_get_group (MkpProject *project, const gchar *id);
MkpTarget *mkp_project_get_target (MkpProject *project, const gchar *id);
MkpSource *mkp_project_get_source (MkpProject *project, const gchar *id);

gboolean mkp_project_move (MkpProject *project, const gchar *path);
gboolean mkp_project_save (MkpProject *project, GError **error);

gchar * mkp_project_get_uri (MkpProject *project);
GFile* mkp_project_get_file (MkpProject *project);
gboolean mkp_project_get_token_location (MkpProject *project, AnjutaTokenFileLocation *location, AnjutaToken *token);

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
gchar *mkp_target_get_id (MkpTarget *target);

gchar *mkp_source_get_id (MkpSource *source);
GFile *mkp_source_get_file (MkpSource *source);

gchar *mkp_variable_evaluate (MkpVariable *variable, MkpProject *project);
const gchar* mkp_variable_get_name (MkpVariable *variable);


G_END_DECLS

#endif /* _MK_PROJECT_H_ */
