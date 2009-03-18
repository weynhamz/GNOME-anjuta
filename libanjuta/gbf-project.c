/* gbf-project.c
 *
 * Copyright (C) 2002 Jeroen Zwartepoorte
 *
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgnome/gnome-macros.h>
#include "gbf-project.h"

GNOME_CLASS_BOILERPLATE (GbfProject, gbf_project, GObject, G_TYPE_OBJECT);

void 
gbf_project_load (GbfProject  *project,
		  const gchar *path,
		  GError     **error)
{
	g_return_if_fail (project != NULL);
	g_return_if_fail (GBF_IS_PROJECT (project));
	g_return_if_fail (path != NULL);
	g_return_if_fail (error == NULL || *error == NULL);

	GBF_PROJECT_GET_CLASS (project)->load (project, path, error);
}

gboolean 
gbf_project_probe (GbfProject  *project,
		   const gchar *path,
		   GError     **error)
{
	g_return_val_if_fail (project != NULL, FALSE);
	g_return_val_if_fail (GBF_IS_PROJECT (project), FALSE);
	g_return_val_if_fail (path != NULL, FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	return GBF_PROJECT_GET_CLASS (project)->probe (project, path, error);
}

void
gbf_project_refresh (GbfProject *project,
		     GError    **error)
{
	g_return_if_fail (project != NULL);
	g_return_if_fail (GBF_IS_PROJECT (project));
	g_return_if_fail (error == NULL || *error == NULL);

	GBF_PROJECT_GET_CLASS (project)->refresh (project, error);
}

GbfProjectCapabilities
gbf_project_get_capabilities (GbfProject *project,
		     GError    **error)
{
	g_return_val_if_fail (project != NULL, GBF_PROJECT_CAN_ADD_NONE);
	g_return_val_if_fail (GBF_IS_PROJECT (project), GBF_PROJECT_CAN_ADD_NONE);
	g_return_val_if_fail (error == NULL || *error == NULL, GBF_PROJECT_CAN_ADD_NONE);

	return GBF_PROJECT_GET_CLASS (project)->get_capabilities (project, error);
}

/* Groups. */
gchar * 
gbf_project_add_group (GbfProject  *project,
		       const gchar *parent_id,
		       const gchar *name,
		       GError     **error)
{
	g_return_val_if_fail (project != NULL, NULL);
	g_return_val_if_fail (GBF_IS_PROJECT (project), NULL);
	g_return_val_if_fail (parent_id != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	return GBF_PROJECT_GET_CLASS (project)->add_group (project, 
							   parent_id, 
							   name, 
							   error);
}

void 
gbf_project_remove_group (GbfProject  *project,
			  const gchar *id,
			  GError     **error)
{
	g_return_if_fail (project != NULL);
	g_return_if_fail (GBF_IS_PROJECT (project));
	g_return_if_fail (id != NULL);
	g_return_if_fail (error == NULL || *error == NULL);

	GBF_PROJECT_GET_CLASS (project)->remove_group (project, id, error);
}

GbfProjectGroup * 
gbf_project_get_group (GbfProject  *project,
		       const gchar *id,
		       GError     **error)
{
	g_return_val_if_fail (project != NULL, NULL);
	g_return_val_if_fail (GBF_IS_PROJECT (project), NULL);
	g_return_val_if_fail (id != NULL, NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	return GBF_PROJECT_GET_CLASS (project)->get_group (project, id, error);
}

GtkWidget * 
gbf_project_configure_group (GbfProject  *project,
			     const gchar *id,
			     GError     **error)
{
	g_return_val_if_fail (project != NULL, NULL);
	g_return_val_if_fail (GBF_IS_PROJECT (project), NULL);
	g_return_val_if_fail (id != NULL, NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	return GBF_PROJECT_GET_CLASS (project)->configure_group (project, 
								 id, 
								 error);
}

GtkWidget * 
gbf_project_configure_new_group (GbfProject *project,
				 GError    **error)
{
	g_return_val_if_fail (project != NULL, NULL);
	g_return_val_if_fail (GBF_IS_PROJECT (project), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	return GBF_PROJECT_GET_CLASS (project)->configure_new_group (project, 
								     error);
}

/* Targets. */
gchar * 
gbf_project_add_target (GbfProject  *project,
			const gchar *group_id,
			const gchar *name,
			const gchar *type,
			GError     **error)
{
	g_return_val_if_fail (project != NULL, NULL);
	g_return_val_if_fail (GBF_IS_PROJECT (project), NULL);
	g_return_val_if_fail (group_id != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (type != NULL, NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	return GBF_PROJECT_GET_CLASS (project)->add_target (project, 
							    group_id,
							    name,
							    type,
							    error);
}

void 
gbf_project_remove_target (GbfProject  *project,
			   const gchar *id,
			   GError     **error)
{
	g_return_if_fail (project != NULL);
	g_return_if_fail (GBF_IS_PROJECT (project));
	g_return_if_fail (id != NULL);	
	g_return_if_fail (error == NULL || *error == NULL);

	GBF_PROJECT_GET_CLASS (project)->remove_target (project, id, error);
}

GbfProjectTarget * 
gbf_project_get_target (GbfProject  *project,
			const gchar *id,
			GError     **error)
{
	g_return_val_if_fail (project != NULL, NULL);
	g_return_val_if_fail (GBF_IS_PROJECT (project), NULL);
	g_return_val_if_fail (id != NULL, NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	return GBF_PROJECT_GET_CLASS (project)->get_target (project, 
							    id, 
							    error);
}

GList * 
gbf_project_get_all_targets (GbfProject *project,
			     GError    **error)
{
	g_return_val_if_fail (project != NULL, NULL);
	g_return_val_if_fail (GBF_IS_PROJECT (project), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	return GBF_PROJECT_GET_CLASS (project)->get_all_targets (project, 
								 error);
}

GList * 
gbf_project_get_all_groups (GbfProject *project,
			    GError    **error)
{
	g_return_val_if_fail (project != NULL, NULL);
	g_return_val_if_fail (GBF_IS_PROJECT (project), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	return GBF_PROJECT_GET_CLASS (project)->get_all_groups (project, 
								error);
}

GtkWidget * 
gbf_project_configure_target (GbfProject  *project,
			      const gchar *id,
			      GError     **error)
{
	g_return_val_if_fail (project != NULL, NULL);
	g_return_val_if_fail (GBF_IS_PROJECT (project), NULL);
	g_return_val_if_fail (id != NULL, NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	return GBF_PROJECT_GET_CLASS (project)->configure_target (project, 
								  id, 
								  error);
}
GtkWidget * 
gbf_project_configure_new_target (GbfProject *project,
				  GError    **error)
{
	g_return_val_if_fail (project != NULL, NULL);
	g_return_val_if_fail (GBF_IS_PROJECT (project), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	return GBF_PROJECT_GET_CLASS (project)->configure_new_target (project,
								      error);
}

/* Sources. */
gchar * 
gbf_project_add_source (GbfProject  *project,
			const gchar *target_id,
			const gchar *uri,
			GError     **error)
{
	g_return_val_if_fail (project != NULL, NULL);
	g_return_val_if_fail (GBF_IS_PROJECT (project), NULL);
	g_return_val_if_fail (target_id != NULL, NULL);
	g_return_val_if_fail (uri != NULL, NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	return GBF_PROJECT_GET_CLASS (project)->add_source (project, 
							    target_id,
							    uri,
							    error);
}

void 
gbf_project_remove_source (GbfProject  *project,
			   const gchar *id,
			   GError     **error)
{
	g_return_if_fail (project != NULL);
	g_return_if_fail (GBF_IS_PROJECT (project));
	g_return_if_fail (id != NULL);
	g_return_if_fail (error == NULL || *error == NULL);
	
	GBF_PROJECT_GET_CLASS (project)->remove_source (project, id, error);
}

GbfProjectTargetSource * 
gbf_project_get_source (GbfProject  *project,
			const gchar *id,
			GError     **error)
{
	g_return_val_if_fail (project != NULL, NULL);
	g_return_val_if_fail (GBF_IS_PROJECT (project), NULL);
	g_return_val_if_fail (id != NULL, NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	return GBF_PROJECT_GET_CLASS (project)->get_source (project, 
							    id, 
							    error);
}

GList * 
gbf_project_get_all_sources (GbfProject *project,
			     GError    **error)
{
	g_return_val_if_fail (project != NULL, NULL);
	g_return_val_if_fail (GBF_IS_PROJECT (project), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	return GBF_PROJECT_GET_CLASS (project)->get_all_sources (project, 
								 error);
}

GtkWidget * 
gbf_project_configure_source (GbfProject  *project,
			      const gchar *id,
			      GError     **error)
{
	g_return_val_if_fail (project != NULL, NULL);
	g_return_val_if_fail (GBF_IS_PROJECT (project), NULL);
	g_return_val_if_fail (id != NULL, NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	return GBF_PROJECT_GET_CLASS (project)->configure_source (project, 
								  id, 
								  error);
}

GtkWidget * 
gbf_project_configure_new_source (GbfProject *project,
				  GError    **error)
{
	g_return_val_if_fail (project != NULL, NULL);
	g_return_val_if_fail (GBF_IS_PROJECT (project), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	return GBF_PROJECT_GET_CLASS (project)->configure_new_source (project, 
								      error);
}

/* Project */

GtkWidget *
gbf_project_configure (GbfProject *project, GError **error)
{
	g_return_val_if_fail (project != NULL, NULL);
	g_return_val_if_fail (GBF_IS_PROJECT (project), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	return GBF_PROJECT_GET_CLASS (project)->configure (project, error);
}

GList *
gbf_project_get_config_modules   (GbfProject *project, GError **error)
{
	g_return_val_if_fail (project != NULL, NULL);
	g_return_val_if_fail (GBF_IS_PROJECT (project), NULL);
	
	return GBF_PROJECT_GET_CLASS (project)->get_config_modules (project, error);
}

GList *
gbf_project_get_config_packages  (GbfProject *project,
																	const gchar* module,
																	GError **error)
{
	g_return_val_if_fail (project != NULL, NULL);
	g_return_val_if_fail (module != NULL, NULL);
	g_return_val_if_fail (GBF_IS_PROJECT (project), NULL);
	
	return GBF_PROJECT_GET_CLASS (project)->get_config_packages (project, module,
																															 error);
}

/* Types. */
const gchar * 
gbf_project_name_for_type (GbfProject  *project,
			   const gchar *type)
{
	g_return_val_if_fail (project != NULL, NULL);
	g_return_val_if_fail (GBF_IS_PROJECT (project), NULL);
	g_return_val_if_fail (type != NULL, NULL);
	
	return GBF_PROJECT_GET_CLASS (project)->name_for_type (project, type);
}

const gchar * 
gbf_project_mimetype_for_type (GbfProject  *project,
			       const gchar *type)
{
	g_return_val_if_fail (project != NULL, NULL);
	g_return_val_if_fail (GBF_IS_PROJECT (project), NULL);
	g_return_val_if_fail (type != NULL, NULL);

	return GBF_PROJECT_GET_CLASS (project)->mimetype_for_type (project, type);
}

gchar **
gbf_project_get_types (GbfProject *project) 
{
	g_return_val_if_fail (project != NULL, NULL);
	g_return_val_if_fail (GBF_IS_PROJECT (project), NULL);
	g_return_val_if_fail (
		GBF_PROJECT_GET_CLASS (project)->get_types != NULL, NULL);
	
	return GBF_PROJECT_GET_CLASS (project)->get_types (project);
}

static void
gbf_project_class_init (GbfProjectClass *klass) 
{
	parent_class = g_type_class_peek_parent (klass);

	g_signal_new ("project-updated",
		      G_TYPE_FROM_CLASS (klass),
		      G_SIGNAL_RUN_LAST,
		      G_STRUCT_OFFSET (GbfProjectClass, project_updated),
		      NULL, NULL,
		      g_cclosure_marshal_VOID__VOID,
		      G_TYPE_NONE, 0);
}

static void
gbf_project_instance_init (GbfProject *project)
{
}

GQuark
gbf_project_error_quark (void)
{
	static GQuark quark = 0;

	if (quark == 0)
		quark = g_quark_from_string ("gbf-project-quark");

	return quark;
}

GbfProjectGroup *
gbf_project_group_copy (GbfProjectGroup *group)
{
	GbfProjectGroup *new_group;
	GList *l;
	
	new_group = g_new0 (GbfProjectGroup, 1);
	new_group->id = g_strdup (group->id);
	new_group->parent_id = g_strdup (group->parent_id);
	new_group->name = g_strdup (group->name);
	
	for (l = group->groups; l; l = l->next)
		new_group->groups = g_list_prepend (new_group->groups, g_strdup (l->data));
	new_group->groups = g_list_reverse (new_group->groups);
	
	for (l = group->targets; l; l = l->next)
		new_group->targets = g_list_prepend (new_group->targets, g_strdup (l->data));
	new_group->targets = g_list_reverse (new_group->targets);

	return new_group;
}

void
gbf_project_group_free (GbfProjectGroup *group)
{
	g_free (group->id);
	g_free (group->parent_id);
	g_free (group->name);

	while (group->groups) {
		g_free (group->groups->data);
		group->groups = g_list_delete_link (group->groups, group->groups);
	}
	
	while (group->targets) {
		g_free (group->targets->data);
		group->targets = g_list_delete_link (group->targets, group->targets);
	}
	g_free (group);
}

GbfProjectTarget *
gbf_project_target_copy (GbfProjectTarget *target)
{
	GbfProjectTarget *new_target;
	GList *l;
	
	new_target = g_new0 (GbfProjectTarget, 1);
	new_target->id = g_strdup (target->id);
	new_target->group_id = g_strdup (target->group_id);
	new_target->name = g_strdup (target->name);
	new_target->type = g_strdup (target->type);
	
	for (l = target->sources; l; l = l->next)
		new_target->sources = g_list_prepend (new_target->sources, g_strdup (l->data));
	new_target->sources = g_list_reverse (new_target->sources);

	return new_target;
}

void
gbf_project_target_free (GbfProjectTarget *target)
{
	g_free (target->id);
	g_free (target->group_id);
	g_free (target->name);
	g_free (target->type);

	while (target->sources) {
		g_free (target->sources->data);
		target->sources = g_list_delete_link (target->sources, target->sources);
	}

	g_free (target);
}

GbfProjectTargetSource *
gbf_project_target_source_copy (GbfProjectTargetSource *source)
{
	GbfProjectTargetSource *new_source;
	
	new_source = g_new0 (GbfProjectTargetSource, 1);
	new_source->id = g_strdup (source->id);
	new_source->target_id = g_strdup (source->target_id);
	new_source->source_uri = g_strdup (source->source_uri);
	
	return new_source;
}

void
gbf_project_target_source_free (GbfProjectTargetSource *source)
{
	g_free (source->id);
	g_free (source->target_id);
	g_free (source->source_uri);
	g_free (source);
}

GType 
gbf_project_group_get_type (void)
{
	static GType our_type = 0;
  
	if (our_type == 0)
		our_type = g_boxed_type_register_static ("GbfProjectGroup",
							 (GBoxedCopyFunc) gbf_project_group_copy,
							 (GBoxedFreeFunc) gbf_project_group_free);
	
	return our_type;
}

GType 
gbf_project_target_get_type (void)
{
	static GType our_type = 0;
  
	if (our_type == 0)
		our_type = g_boxed_type_register_static ("GbfProjectTarget",
							 (GBoxedCopyFunc) gbf_project_target_copy,
							 (GBoxedFreeFunc) gbf_project_target_free);
	
	return our_type;
}

GType 
gbf_project_target_source_get_type (void)
{
	static GType our_type = 0;
  
	if (our_type == 0)
		our_type = g_boxed_type_register_static ("GbfProjectTargetSource",
							 (GBoxedCopyFunc) gbf_project_target_source_copy,
							 (GBoxedFreeFunc) gbf_project_target_source_free);
	
	return our_type;
}
