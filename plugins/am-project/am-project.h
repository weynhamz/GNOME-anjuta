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

#ifndef _AM_PROJECT_H_
#define _AM_PROJECT_H_

#include <glib-object.h>

#include <libanjuta/anjuta-project.h>
#include <libanjuta/anjuta-token.h>
#include <libanjuta/anjuta-token-file.h>
#include <libanjuta/anjuta-token-list.h>

G_BEGIN_DECLS

#define AMP_TYPE_PROJECT		(amp_project_get_type ())
#define AMP_PROJECT(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), AMP_TYPE_PROJECT, AmpProject))
#define AMP_PROJECT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), AMP_TYPE_PROJECT, AmpProjectClass))
#define AMP_IS_PROJECT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), AMP_TYPE_PROJECT))
#define AMP_IS_PROJECT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((obj), AMP_TYPE_PROJECT))

typedef struct _AmpProject        AmpProject;
typedef struct _AmpProjectClass   AmpProjectClass;

struct _AmpProjectClass {
	GObjectClass parent_class;
};

typedef struct _AnjutaAmRootNode AnjutaAmRootNode;
typedef struct _AnjutaAmModuleNode AnjutaAmModuleNode;
typedef struct _AnjutaAmPackageNode AnjutaAmPackageNode;
typedef struct _AnjutaAmGroupNode AnjutaAmGroupNode;
typedef struct _AnjutaAmTargetNode AnjutaAmTargetNode;
typedef struct _AnjutaAmSourceNode AnjutaAmSourceNode;

typedef struct _AmpProperty AmpProperty;

typedef enum {
	AMP_PROPERTY_NAME = 0,
	AMP_PROPERTY_VERSION,
	AMP_PROPERTY_BUG_REPORT,
	AMP_PROPERTY_TARNAME,
	AMP_PROPERTY_URL
} AmpPropertyType;

typedef struct _AmpPropertyInfo   AmpPropertyInfo;

typedef struct _AmpTargetPropertyBuffer AmpTargetPropertyBuffer;

GType         amp_project_get_type (void);
AmpProject   *amp_project_new      (GFile *file, GError **error);


AmpTargetPropertyBuffer* amp_target_property_buffer_new (void);
void amp_target_property_buffer_free (AmpTargetPropertyBuffer *buffer);

void amp_target_property_buffer_add_source (AmpTargetPropertyBuffer *buffer, AnjutaAmSourceNode *source);
void amp_target_property_buffer_add_property (AmpTargetPropertyBuffer *buffer, AnjutaProjectProperty *prop);
GList *amp_target_property_buffer_steal_sources (AmpTargetPropertyBuffer *buffer);
GList *amp_target_property_buffer_steal_properties (AmpTargetPropertyBuffer *buffer);

gint amp_project_probe (GFile *directory, GError     **error);
gboolean amp_project_load (AmpProject *project, GFile *directory, GError **error);
AnjutaProjectNode *amp_project_load_node (AmpProject *project, AnjutaProjectNode *node, GError **error);
void amp_project_unload (AmpProject *project);

void amp_project_load_config (AmpProject *project, AnjutaToken *arg_list);
void amp_project_load_properties (AmpProject *project, AnjutaToken *macro, AnjutaToken *list);
void amp_project_load_module (AmpProject *project, AnjutaToken *module);

AnjutaAmRootNode *amp_project_get_root (AmpProject *project);
AnjutaAmGroupNode *amp_project_get_group (AmpProject *project, const gchar *id);
AnjutaAmTargetNode *amp_project_get_target (AmpProject *project, const gchar *id);
AnjutaAmSourceNode *amp_project_get_source (AmpProject *project, const gchar *id);
gboolean amp_project_get_token_location (AmpProject *project, AnjutaTokenFileLocation *location, AnjutaToken *token);

gboolean amp_project_move (AmpProject *project, const gchar *path);
gboolean amp_project_save (AmpProject *project, GError **error);

gboolean amp_project_dump (AmpProject *project, AnjutaProjectNode *node);

gchar * amp_project_get_uri (AmpProject *project);
GFile* amp_project_get_file (AmpProject *project);

AnjutaAmGroupNode* amp_project_add_group (AmpProject  *project, AnjutaAmGroupNode *parent, const gchar *name, GError **error);
AnjutaAmGroupNode* amp_project_add_sibling_group (AmpProject  *project, AnjutaAmGroupNode *parent, const gchar *name, gboolean after, AnjutaAmGroupNode *sibling, GError **error);
void amp_project_remove_group (AmpProject  *project, AnjutaAmGroupNode *group, GError **error);

AnjutaAmTargetNode* amp_project_add_target (AmpProject  *project, AnjutaAmGroupNode *parent, const gchar *name, AnjutaProjectNodeType type, GError **error);
AnjutaAmTargetNode* amp_project_add_sibling_target (AmpProject  *project, AnjutaAmGroupNode *parent, const gchar *name, AnjutaProjectNodeType type, gboolean after, AnjutaAmTargetNode *sibling, GError **error);
void amp_project_remove_target (AmpProject  *project, AnjutaAmTargetNode *target, GError **error);

AnjutaAmSourceNode* amp_project_add_source (AmpProject  *project, AnjutaAmTargetNode *parent, GFile *file, GError **error);
AnjutaAmSourceNode* amp_project_add_sibling_source (AmpProject  *project, AnjutaAmTargetNode *parent, GFile *file, gboolean after, AnjutaAmSourceNode *sibling, GError **error);
void amp_project_remove_source (AmpProject  *project, AnjutaAmSourceNode *source, GError **error);

AnjutaProjectNodeInfo *amp_project_get_type_info (AmpProject *project, AnjutaProjectNodeType type);
GList *amp_project_get_node_info (AmpProject *project, GError **error);

GList *amp_project_get_config_modules (AmpProject *project, GError **error);
GList *amp_project_get_config_packages  (AmpProject *project, const gchar* module, GError **error);

GList *amp_project_get_target_types (AmpProject *project, GError **error);

AnjutaProjectProperty *amp_project_get_property_list (AmpProject *project);
//gchar* amp_project_get_property (AmpProject *project, AmpPropertyType type);
//gboolean amp_project_set_property (AmpProject *project, AmpPropertyType type, const gchar* value);

gchar * amp_project_get_node_id (AmpProject *project, const gchar *path);

AnjutaProjectNode *amp_node_parent (AnjutaProjectNode *node);
AnjutaProjectNode *amp_node_first_child (AnjutaProjectNode *node);
AnjutaProjectNode *amp_node_last_child (AnjutaProjectNode *node);
AnjutaProjectNode *amp_node_next_sibling (AnjutaProjectNode *node);
AnjutaProjectNode *amp_node_prev_sibling (AnjutaProjectNode *node);
AnjutaProjectNodeType amp_node_get_type (AnjutaProjectNode *node);
//void amp_node_all_foreach (AnjutaProjectNode *node, AnjutaProjectNodeFunc func, gpointer data);

GFile *amp_group_get_directory (AnjutaAmGroupNode *group);
GFile *amp_group_get_makefile (AnjutaAmGroupNode *group);
gchar *amp_group_get_id (AnjutaAmGroupNode *group);
void amp_group_update_variable (AnjutaAmGroupNode *group, AnjutaToken *variable);
AnjutaToken* amp_group_get_variable_token (AnjutaAmGroupNode *group, AnjutaToken *variable);

const gchar *amp_target_get_name (AnjutaAmTargetNode *target);
gchar *amp_target_get_id (AnjutaAmTargetNode *target);

void amp_source_free (AnjutaAmSourceNode *node);
gchar *amp_source_get_id (AnjutaAmSourceNode *source);
GFile *amp_source_get_file (AnjutaAmSourceNode *source);

G_END_DECLS

#endif /* _AM_PROJECT_H_ */
