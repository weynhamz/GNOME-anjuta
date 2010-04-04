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
#include <libanjuta/anjuta-marshal.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-error.h>
#include <libanjuta/interfaces/ianjuta-project-backend.h>
#include "gbf-project-model.h"
#include "gbf-project-view.h"

/* Types
 *---------------------------------------------------------------------------*/

typedef enum
{
	LOAD,
	UNLOAD,
	EXIT
} PmCommand;

typedef struct _PmJob PmJob;

typedef void (*PmJobCallback) (AnjutaPmProject *project, PmJob *job);

struct _PmJob
{
	PmCommand command;
	GFile *file;
	gchar *name;
	AnjutaProjectNode *node;
	PmJobCallback callback;
	GError *error;
	AnjutaProjectNode *new_node;
};

/* Signal
 *---------------------------------------------------------------------------*/

enum
{
	UPDATED,
	LAST_SIGNAL
};

static unsigned int signals[LAST_SIGNAL] = { 0 };

/* Job functions
 *---------------------------------------------------------------------------*/

static PmJob *
pm_job_new (PmCommand command, GFile *file, const gchar *name, AnjutaProjectNode *node, PmJobCallback callback)
{
	PmJob *job;

	job = g_new0 (PmJob, 1);
	job->command = command;
	job->file = file != NULL ? g_object_ref (file) : NULL;
	job->name = name != NULL ? g_strdup (name) : NULL;
	job->node = node;
	job->callback = callback;

	return job;
}

static void
pm_job_free (PmJob *job)
{
	if (job->file != NULL) g_object_unref (job->file);
	if (job->name != NULL) g_free (job->name);
	if (job->error != NULL) g_error_free (job->error);
}

static void
pm_project_push_command (AnjutaPmProject *project, PmCommand command, GFile *file, const gchar *name, AnjutaProjectNode *node, PmJobCallback callback)
{
	PmJob *job;

	job = pm_job_new (command, file, name, node, callback);

	g_async_queue_push (project->job_queue, job);
}

/* Thread functions
 *---------------------------------------------------------------------------*/

static gpointer
pm_project_thread_main_loop (AnjutaPmProject *project)
{
	PmJob *job;
	
	for (;;)
	{
		AnjutaProjectNode *root;
		AnjutaProjectNode *node;
		
		job = (PmJob *)g_async_queue_pop (project->job_queue);

		switch (job->command)
		{
		case LOAD:
			root = ianjuta_project_new_root (project->project, job->file, NULL);
			node = ianjuta_project_load_node (project->project, root, &(job->error));
			g_message ("load get root %p node %p, error %p", root, node, job->error);
			if (job->error == NULL)
			{
				job->node = node;
			}
			else
			{
				ianjuta_project_free_node (project->project, root, NULL);
			}
			break;
		case EXIT:
			do
			{
				pm_job_free (job);
				job = (PmJob *)g_async_queue_try_pop (project->done_queue);
			}
			while (job != NULL);
			return NULL;
		default:
			break;
		}
		g_async_queue_push (project->done_queue, job);
	}
}

static gboolean
pm_project_idle_func (AnjutaPmProject *project)
{
	PmJob *job;

	for (;;)
	{
		job = (PmJob *)g_async_queue_try_pop (project->done_queue);

		if (job == NULL) return TRUE;

		if (job->callback != NULL)
		{
			job->callback (project, job);
		}
		pm_job_free (job);
	}
}

static gboolean
pm_project_start_thread (AnjutaPmProject *project, GError **error)
{
	project->done_queue = g_async_queue_new ();
	project->job_queue = g_async_queue_new ();

	project->worker = g_thread_create ((GThreadFunc) pm_project_thread_main_loop, project, TRUE, error);

	if (project->worker == NULL) {
		g_async_queue_unref (project->job_queue);
		project->job_queue = NULL;
		g_async_queue_unref (project->done_queue);
		project->done_queue = NULL;

		return FALSE;
	}
	else
	{
		project->idle_func = g_idle_add ((GSourceFunc) pm_project_idle_func, project);
		project->stopping = FALSE;

		return TRUE;
	}
}

static gboolean
pm_project_stop_thread (AnjutaPmProject *project)
{
	if (project->job_queue)
	{
		PmJob *job;
		
		project->stopping = TRUE;

		// Remove idle function
		g_idle_remove_by_data (project);
		project->idle_func = 0;

		// Request to terminate thread
		job = pm_job_new (EXIT, NULL, NULL, NULL, NULL);
		g_async_queue_push (project->job_queue, job);
		g_thread_join (project->worker);
		project->worker = NULL;

		// Free queue
		g_async_queue_unref (project->job_queue);
		project->job_queue = NULL;
		for (;;)
		{
			job = g_async_queue_try_pop (project->done_queue);
			if (job == NULL) break;
			pm_job_free (job);
		}
		project->done_queue = NULL;
	}

	return TRUE;
}

/* Public functions
 *---------------------------------------------------------------------------*/

static void
on_pm_project_loaded (AnjutaPmProject *project, PmJob *job)
{
	if (job->error != NULL)
	{
		g_warning ("unable to load project");
		/* Unable to load project, destroy project object */
		pm_project_stop_thread (project);
		g_object_unref (project->project);
		project->project = NULL;
	}
	else
	{
		g_message ("project loaded project root %p node %p", anjuta_pm_project_get_root (project), job->node);
		g_message ("root child %d", g_node_n_children (job->node));
		g_message ("root all nodes %d", g_node_n_nodes (job->node, G_TRAVERSE_ALL));
		project->root = job->node;
		g_object_set (G_OBJECT (project->model), "project", project, NULL);
	}
	g_signal_emit (G_OBJECT (project), signals[UPDATED], 0, job->error);
}

gboolean 
anjuta_pm_project_load (AnjutaPmProject *project, GFile *file, GError **error)
{
	AnjutaPluginManager *plugin_manager;
	GList *desc;
	IAnjutaProjectBackend *backend;
	gint found = 0;
	GError *err = NULL;
	
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

	if (!pm_project_start_thread(project, &err))
	{
		return FALSE;
	}

	pm_project_push_command (project, LOAD, file, NULL, NULL, on_pm_project_loaded);

	return TRUE;
}

gboolean 
anjuta_pm_project_unload (AnjutaPmProject *project, GError **error)
{
	pm_project_stop_thread (project);
	
	g_object_set (G_OBJECT (project->model), "project", NULL, NULL);
	g_object_unref (project->project);
	project->project = NULL;

	return TRUE;
}

gboolean
anjuta_pm_project_refresh (AnjutaPmProject *project, GError **error)
{
	ianjuta_project_refresh (project->project, error);

	return TRUE;
}

GtkWidget *
anjuta_pm_project_configure (AnjutaPmProject *project, AnjutaProjectNode *node)
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
anjuta_pm_project_get_capabilities (AnjutaPmProject *project)
{
	IAnjutaProjectCapabilities caps = IANJUTA_PROJECT_CAN_ADD_NONE;

	if (project->project != NULL)
		caps = ianjuta_project_get_capabilities (project->project, NULL);

	return caps;
}

GList *
anjuta_pm_project_get_target_types (AnjutaPmProject *project)
{
	g_return_val_if_fail (project->project != NULL, NULL);
	
	return ianjuta_project_get_target_types (project->project, NULL);
}

AnjutaProjectNode *
anjuta_pm_project_get_root (AnjutaPmProject *project)
{
	return project->root;
}

GList *
anjuta_pm_project_get_packages (AnjutaPmProject *project)
{
	if (project->project == NULL) return NULL;

	return ianjuta_project_get_packages (project->project, NULL);
}

AnjutaProjectNode *
anjuta_pm_project_add_group (AnjutaPmProject *project, AnjutaProjectNode *group, const gchar *name, GError **error)
{
	g_return_val_if_fail (project->project != NULL, NULL);
	
	return ianjuta_project_add_group (project->project, group, name, error);
}

AnjutaProjectNode *
anjuta_pm_project_add_target (AnjutaPmProject *project, AnjutaProjectNode *group, const gchar *name, AnjutaProjectTargetType type, GError **error)
{
	g_return_val_if_fail (project->project != NULL, NULL);
	
	return ianjuta_project_add_target (project->project, group, name, type, error);
}

AnjutaProjectNode *
anjuta_pm_project_add_source (AnjutaPmProject *project, AnjutaProjectNode *target, GFile *file, GError **error)
{
	AnjutaProjectNode *source;

	g_return_val_if_fail (project->project != NULL, NULL);
	
	source = ianjuta_project_add_source (project->project, target, file, error);

	return source;
}

gboolean
anjuta_pm_project_remove (AnjutaPmProject *project, AnjutaProjectNode *node, GError **error)
{
	ianjuta_project_remove_node (project->project, node, error);

	return TRUE;
}

gboolean
anjuta_pm_project_remove_data (AnjutaPmProject *project, GbfTreeData *data, GError **error)
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
anjuta_pm_project_is_open (AnjutaPmProject *project)
{
	return project->project != NULL;
}

IAnjutaProject *
anjuta_pm_project_get_project (AnjutaPmProject *project)
{
	return project->project;
}

GbfProjectModel *
anjuta_pm_project_get_model (AnjutaPmProject *project)
{
	return project->model;
}

AnjutaProjectNode *
anjuta_pm_project_get_node (AnjutaPmProject *project, GbfTreeData *data)
{
	AnjutaProjectNode *node = NULL;
	
	if (data != NULL)
	{
		AnjutaProjectNode *root = NULL;
		AnjutaProjectNode *group = NULL;
		AnjutaProjectNode *target = NULL;

		root = anjuta_pm_project_get_root (project);
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

/* Implement GObject
 *---------------------------------------------------------------------------*/

G_DEFINE_TYPE (AnjutaPmProject, anjuta_pm_project, G_TYPE_OBJECT);

static void
anjuta_pm_project_init (AnjutaPmProject *project)
{
	project->model = gbf_project_model_new (NULL);
	project->plugin = NULL;
	
	project->job_queue = NULL;
	project->done_queue = NULL;
	project->worker = NULL;
	project->idle_func = 0;
	project->stopping = FALSE;
}

static void
anjuta_pm_project_finalize (GObject *object)
{
	AnjutaPmProject *project = ANJUTA_PM_PROJECT(object);
	
	g_object_unref (G_OBJECT (project->model));
	
	G_OBJECT_CLASS (anjuta_pm_project_parent_class)->finalize (object);
}

static void
anjuta_pm_project_class_init (AnjutaPmProjectClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	
	object_class->finalize = anjuta_pm_project_finalize;

	signals[UPDATED] = g_signal_new ("updated",
	    G_OBJECT_CLASS_TYPE (object_class),
	    G_SIGNAL_RUN_LAST,
	    G_STRUCT_OFFSET (AnjutaPmProjectClass, updated),
	    NULL, NULL,
	    g_cclosure_marshal_VOID__BOXED,
	    G_TYPE_NONE,
	    1,
	    G_TYPE_ERROR);
	
}

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

AnjutaPmProject*
anjuta_pm_project_new (AnjutaPlugin *plugin)
{
	AnjutaPmProject *project;

	project = g_object_new (ANJUTA_TYPE_PM_PROJECT, NULL);
	project->plugin = plugin;

	return project;
}

void
anjuta_pm_project_free (AnjutaPmProject* project)
{
	g_object_unref (project);
}
