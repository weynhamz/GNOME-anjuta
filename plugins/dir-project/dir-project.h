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

typedef AnjutaProjectGroup DirGroup;
typedef AnjutaProjectTarget DirTarget;
typedef AnjutaProjectSource DirSource;


GType         dir_project_get_type (void);
DirProject   *dir_project_new      (void);

gint dir_project_probe (GFile *directory, GError     **error);

gboolean dir_project_load (DirProject *project, GFile *directory, GError **error);
gboolean dir_project_reload (DirProject *project, GError **error);
void dir_project_unload (DirProject *project);

#if 0
AmpGroup *amp_project_get_root (AmpProject *project);
AmpGroup *amp_project_get_group (AmpProject *project, const gchar *id);
AmpTarget *amp_project_get_target (AmpProject *project, const gchar *id);
AmpSource *amp_project_get_source (AmpProject *project, const gchar *id);
gboolean amp_project_get_token_location (AmpProject *project, AnjutaTokenFileLocation *location, AnjutaToken *token);

gboolean amp_project_move (AmpProject *project, const gchar *path);
gboolean amp_project_save (AmpProject *project, GError **error);

gchar * amp_project_get_uri (AmpProject *project);
GFile* amp_project_get_file (AmpProject *project);

AmpGroup* amp_project_add_group (AmpProject  *project, AmpGroup *parent, const gchar *name, GError **error);
AmpGroup* amp_project_add_sibling_group (AmpProject  *project, AmpGroup *parent, const gchar *name, gboolean after, AmpGroup *sibling, GError **error);
void amp_project_remove_group (AmpProject  *project, AmpGroup *group, GError **error);

AmpTarget* amp_project_add_target (AmpProject  *project, AmpGroup *parent, const gchar *name, AnjutaProjectTargetType type, GError **error);
AmpTarget* amp_project_add_sibling_target (AmpProject  *project, AmpGroup *parent, const gchar *name, AnjutaProjectTargetType type, gboolean after, AmpTarget *sibling, GError **error);
void amp_project_remove_target (AmpProject  *project, AmpTarget *target, GError **error);

AmpSource* amp_project_add_source (AmpProject  *project, AmpTarget *parent, GFile *file, GError **error);
AmpSource* amp_project_add_sibling_source (AmpProject  *project, AmpTarget *parent, GFile *file, gboolean after, AmpSource *sibling, GError **error);
void amp_project_remove_source (AmpProject  *project, AmpSource *source, GError **error);


GList *amp_project_get_config_modules (AmpProject *project, GError **error);
GList *amp_project_get_config_packages  (AmpProject *project, const gchar* module, GError **error);

GList *amp_project_get_target_types (AmpProject *project, GError **error);

gchar* amp_project_get_property (AmpProject *project, AmpPropertyType type);
gboolean amp_project_set_property (AmpProject *project, AmpPropertyType type, const gchar* value);

gchar * amp_project_get_node_id (AmpProject *project, const gchar *path);

AnjutaProjectNode *amp_node_parent (AnjutaProjectNode *node);
AnjutaProjectNode *amp_node_first_child (AnjutaProjectNode *node);
AnjutaProjectNode *amp_node_last_child (AnjutaProjectNode *node);
AnjutaProjectNode *amp_node_next_sibling (AnjutaProjectNode *node);
AnjutaProjectNode *amp_node_prev_sibling (AnjutaProjectNode *node);
AnjutaProjectNodeType amp_node_get_type (AnjutaProjectNode *node);
void amp_node_all_foreach (AnjutaProjectNode *node, AnjutaProjectNodeFunc func, gpointer data);

GFile *amp_group_get_directory (AmpGroup *group);
GFile *amp_group_get_makefile (AmpGroup *group);
gchar *amp_group_get_id (AmpGroup *group);

const gchar *amp_target_get_name (AmpTarget *target);
AnjutaProjectTargetType amp_target_get_type (AmpTarget *target);
gchar *amp_target_get_id (AmpTarget *target);

void amp_source_free (AmpSource *node);
gchar *amp_source_get_id (AmpSource *source);
GFile *amp_source_get_file (AmpSource *source);
#endif

G_END_DECLS

#endif /* _DIR_PROJECT_H_ */
