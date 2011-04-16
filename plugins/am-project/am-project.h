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

#include "amp-root.h"

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


typedef struct _AmpProjectClass   AmpProjectClass;

typedef struct _AmpModuleNode AmpModuleNode;
typedef struct _AmpPackageNode AmpPackageNode;
typedef struct _AmpGroupNode AmpGroupNode;
typedef struct _AmpTargetNode AmpTargetNode;
typedef struct _AmpSourceNode AmpSourceNode;

struct _AmpProjectClass {
	AmpRootNodeClass parent_class;
};

typedef struct _AmpProperty AmpProperty;

GType         amp_project_get_type (void);
AmpProject   *amp_project_new      (GFile *file, GError **error);

void amp_project_register (GTypeModule *module);


gint amp_project_probe (GFile *directory, GError     **error);
void amp_project_unload (AmpProject *project);
gboolean amp_project_is_loaded (AmpProject *project);


void amp_project_load_config (AmpProject *project, AnjutaToken *arg_list);
void amp_project_load_properties (AmpProject *project, AnjutaToken *macro, AnjutaToken *list);
void amp_project_load_module (AmpProject *project, AnjutaToken *module);

AnjutaToken *amp_project_get_config_token (AmpProject *project, GFile *file);

AnjutaTokenFile* amp_project_set_configure (AmpProject *project, GFile *configure);
gboolean amp_project_update_configure (AmpProject *project, AnjutaToken *token);
AnjutaToken* amp_project_get_configure_token (AmpProject *project);
void amp_project_update_root (AmpProject *project, AmpProject *new_project);

AmpProject *amp_project_get_root (AmpProject *project);
AmpGroupNode *amp_project_get_group (AmpProject *project, const gchar *id);
AmpTargetNode *amp_project_get_target (AmpProject *project, const gchar *id);
AmpSourceNode *amp_project_get_source (AmpProject *project, const gchar *id);
gboolean amp_project_get_token_location (AmpProject *project, AnjutaTokenFileLocation *location, AnjutaToken *token);

gboolean amp_project_move (AmpProject *project, const gchar *path);

gboolean amp_project_dump (AmpProject *project, AnjutaProjectNode *node);

gchar * amp_project_get_uri (AmpProject *project);
GFile* amp_project_get_file (AmpProject *project);

gboolean amp_project_is_busy (AmpProject *project);

void amp_project_add_file (AmpProject *project, GFile *file, AnjutaTokenFile* token);

AmpGroupNode* amp_project_add_group (AmpProject  *project, AmpGroupNode *parent, const gchar *name, GError **error);
AmpGroupNode* amp_project_add_sibling_group (AmpProject  *project, AmpGroupNode *parent, const gchar *name, gboolean after, AmpGroupNode *sibling, GError **error);
void amp_project_remove_group (AmpProject  *project, AmpGroupNode *group, GError **error);

AmpTargetNode* amp_project_add_target (AmpProject  *project, AmpGroupNode *parent, const gchar *name, AnjutaProjectNodeType type, GError **error);
AmpTargetNode* amp_project_add_sibling_target (AmpProject  *project, AmpGroupNode *parent, const gchar *name, AnjutaProjectNodeType type, gboolean after, AmpTargetNode *sibling, GError **error);
void amp_project_remove_target (AmpProject  *project, AmpTargetNode *target, GError **error);

AmpSourceNode* amp_project_add_source (AmpProject  *project, AmpTargetNode *parent, GFile *file, GError **error);
AmpSourceNode* amp_project_add_sibling_source (AmpProject  *project, AmpTargetNode *parent, GFile *file, gboolean after, AmpSourceNode *sibling, GError **error);
void amp_project_remove_source (AmpProject  *project, AmpSourceNode *source, GError **error);

AnjutaProjectNodeInfo *amp_project_get_type_info (AmpProject *project, AnjutaProjectNodeType type);
const GList *amp_project_get_node_info (AmpProject *project, GError **error);

GList *amp_project_get_config_modules (AmpProject *project, GError **error);
GList *amp_project_get_config_packages  (AmpProject *project, const gchar* module, GError **error);

GList *amp_project_get_target_types (AmpProject *project, GError **error);

AnjutaProjectNode *amp_node_parent (AnjutaProjectNode *node);
AnjutaProjectNode *amp_node_first_child (AnjutaProjectNode *node);
AnjutaProjectNode *amp_node_last_child (AnjutaProjectNode *node);
AnjutaProjectNode *amp_node_next_sibling (AnjutaProjectNode *node);
AnjutaProjectNode *amp_node_prev_sibling (AnjutaProjectNode *node);
//void amp_node_all_foreach (AnjutaProjectNode *node, AnjutaProjectNodeFunc func, gpointer data);

GFile *amp_group_node_get_directory (AmpGroupNode *group);
gchar *amp_group_node_get_id (AmpGroupNode *group);
void amp_group_node_update_variable (AmpGroupNode *group, AnjutaToken *variable);
AnjutaToken* amp_group_node_get_variable_token (AmpGroupNode *group, AnjutaToken *variable);

const gchar *amp_target_node_get_name (AmpTargetNode *target);
gchar *amp_target_node_get_id (AmpTargetNode *target);

void amp_source_node_free (AmpSourceNode *node);
gchar *amp_source_node_get_id (AmpSourceNode *source);
GFile *amp_source_node_get_file (AmpSourceNode *source);

gchar* canonicalize_automake_variable (const gchar *name);
gboolean split_automake_variable (gchar *name, gint *flags, gchar **module, gchar **primary);
gchar* get_relative_path (GFile *parent, GFile *file);
GFileType file_type (GFile *file, const gchar *filename);




G_END_DECLS

#endif /* _AM_PROJECT_H_ */
