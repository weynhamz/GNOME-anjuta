/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/* project.c
 *
 * Copyright (C) 2010  Sébastien Granjoux
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "project.h"
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-project-backend.h>
#include "gbf-project-model.h"
#include "gbf-project-view.h"

/* Public functions
 *---------------------------------------------------------------------------*/

struct _ProjectManagerProject{
	AnjutaPlugin *plugin;
	
	IAnjutaProject *project;
	GbfProjectModel *model;
};

/* Public functions
 *---------------------------------------------------------------------------*/

gboolean 
pm_project_load (ProjectManagerProject *project, GFile *file, GError **error)
{
	AnjutaPluginManager *plugin_manager;
	GList *desc;
	IAnjutaProjectBackend *backend;
	gint found = 0;
	gboolean ok;
	
	g_return_val_if_fail (file != NULL, FALSE);
	
	if (project->project != NULL)
	{
		g_object_unref (project->project);
		project->project = NULL;
	}
	
	DEBUG_PRINT ("loading gbf backend…\n");
	plugin_manager = anjuta_shell_get_plugin_manager (project->plugin->shell, NULL);

	if (!anjuta_plugin_manager_is_active_plugin (plugin_manager, "IAnjutaProjectBackend"))
	{
		GList *descs = NULL;
		
		descs = anjuta_plugin_manager_query (plugin_manager,
											 "Anjuta Plugin",
											 "Interfaces",
											 "IAnjutaProjectBackend",
											 NULL);
		backend = NULL;
		for (desc = g_list_first (descs); desc != NULL; desc = g_list_next (desc)) {
			AnjutaPluginDescription *backend_desc;
			gchar *location = NULL;
			IAnjutaProjectBackend *plugin;
			gint backend_val;
				
			backend_desc = (AnjutaPluginDescription *)desc->data;
			anjuta_plugin_description_get_string (backend_desc, "Anjuta Plugin", "Location", &location);
			plugin = (IAnjutaProjectBackend *)anjuta_plugin_manager_get_plugin_by_id (plugin_manager, location);
			g_free (location);

			backend_val = ianjuta_project_backend_probe (plugin, file, NULL);
			if (backend_val > found)
			{
				/* Backend found */;
				backend = plugin;
				found = backend_val;
			}
		}
		g_list_free (descs);
	}
	else
	{
		/* A backend is already loaded, use it */
		backend = IANJUTA_PROJECT_BACKEND (anjuta_shell_get_object (project->plugin->shell,
                                        "IAnjutaProjectBackend", NULL));

		g_object_ref (backend);
	}
	
	if (!backend)
	{
		/* FIXME: Set err */
		g_warning ("no backend available for this project\n");
		
		return FALSE;
	}
	
	DEBUG_PRINT ("%s", "Creating new gbf project\n");
	project->project = ianjuta_project_backend_new_project (backend, NULL);
	if (!project->project)
	{
		/* FIXME: Set err */
		g_warning ("project creation failed\n");
		
		return FALSE;
	}
	
	/* FIXME: use the error parameter to determine if the project
	 * was loaded successfully */
	ok = ianjuta_project_load (project->project, file, error);
	g_message ("load project %s %p ok %d", g_file_get_path (file), project->project, ok);
	if (!ok)
	{
		g_warning ("unable to load project");
		/* Unable to load project, destroy project object */
		g_object_unref (project->project);
		project->project = NULL;
	}
	else
	{
		g_object_set (G_OBJECT (project->model), "project", project, NULL);
	}

	return ok;
}

gboolean 
pm_project_unload (ProjectManagerProject *project, GError **error)
{
	g_object_set (G_OBJECT (project->model), "project", NULL, NULL);
	g_object_unref (project->project);
	project->project = NULL;

	return TRUE;
}

gboolean
pm_project_refresh (ProjectManagerProject *project, GError **error)
{
	ianjuta_project_refresh (project->project, error);

	return TRUE;
}

GtkWidget *
pm_project_configure (ProjectManagerProject *project, AnjutaProjectNode *node)
{
	GtkWidget *properties;
	
	if (node == NULL)
	{
		properties = ianjuta_project_configure (project->project, NULL);
	}
	else
	{
		properties = ianjuta_project_configure_node (project->project, node, NULL);
	}

	return properties;
}

IAnjutaProjectCapabilities
pm_project_get_capabilities (ProjectManagerProject *project)
{
	IAnjutaProjectCapabilities caps = IANJUTA_PROJECT_CAN_ADD_NONE;

	if (project->project != NULL)
		caps = ianjuta_project_get_capabilities (project->project, NULL);

	return caps;
}

GList *
pm_project_get_target_types (ProjectManagerProject *project)
{
	g_return_val_if_fail (project->project != NULL, NULL);
	
	return ianjuta_project_get_target_types (project->project, NULL);
}

AnjutaProjectNode *
pm_project_get_root (ProjectManagerProject *project)
{
	if (project->project == NULL) return NULL;

	return ianjuta_project_get_root (project->project, NULL);
}

GList *
pm_project_get_packages (ProjectManagerProject *project)
{
	if (project->project == NULL) return NULL;

	return ianjuta_project_get_packages (project->project, NULL);
}

AnjutaProjectNode *
pm_project_add_group (ProjectManagerProject *project, AnjutaProjectNode *group, const gchar *name, GError **error)
{
	g_return_val_if_fail (project->project != NULL, NULL);
	
	return ianjuta_project_add_group (project->project, group, name, error);
}

AnjutaProjectNode *
pm_project_add_target (ProjectManagerProject *project, AnjutaProjectNode *group, const gchar *name, AnjutaProjectTargetType type, GError **error)
{
	g_return_val_if_fail (project->project != NULL, NULL);
	
	return ianjuta_project_add_target (project->project, group, name, type, error);
}

AnjutaProjectNode *
pm_project_add_source (ProjectManagerProject *project, AnjutaProjectNode *target, GFile *file, GError **error)
{
	AnjutaProjectNode *source;

	g_return_val_if_fail (project->project != NULL, NULL);
	
	source = ianjuta_project_add_source (project->project, target, file, error);

	return source;
}

gboolean
pm_project_remove (ProjectManagerProject *project, AnjutaProjectNode *node, GError **error)
{
	ianjuta_project_remove_node (project->project, node, error);

	return TRUE;
}

gboolean
pm_project_remove_data (ProjectManagerProject *project, GbfTreeData *data, GError **error)
{
	GtkTreeIter iter;
	
	if (gbf_project_model_find_tree_data (project->model, &iter, data))
	{
		gbf_project_model_remove (project->model, &iter);

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


gboolean
pm_project_is_open (ProjectManagerProject *project)
{
	return project->project != NULL;
}

IAnjutaProject *
pm_project_get_project (ProjectManagerProject *project)
{
	return project->project;
}

GbfProjectModel *
pm_project_get_model (ProjectManagerProject *project)
{
	return project->model;
}

AnjutaProjectNode *
pm_project_get_node (ProjectManagerProject *project, GbfTreeData *data)
{
	AnjutaProjectNode *node = NULL;
	
	if (data != NULL)
	{
		AnjutaProjectNode *root = NULL;
		AnjutaProjectNode *group = NULL;
		AnjutaProjectNode *target = NULL;

		root = pm_project_get_root (project);
		if ((root != NULL) && (data->group != NULL))
		{
			group = anjuta_project_group_get_node_from_file (root, data->group);
			node = group;
		}

		if ((group != NULL) && (data->target != NULL))
		{
			target = anjuta_project_target_get_node_from_name (group, data->target);
			node = target;
		}

		if (((group != NULL) || (target != NULL)) && (data->source != NULL))
		{
			node = anjuta_project_source_get_node_from_file (target != NULL ? target : group, data->source);
		}
	}

	return node;
}


/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

ProjectManagerProject*
pm_project_new (AnjutaPlugin *plugin)
{
	ProjectManagerProject *project;

	project = g_new0 (ProjectManagerProject, 1);
	project->plugin = plugin;
	project->project = NULL;
	project->model = gbf_project_model_new (NULL);

	return project;
}

void
pm_project_free (ProjectManagerProject* project)
{
	g_object_unref (G_OBJECT (project->model));
	g_free (project);
}
