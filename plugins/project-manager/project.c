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
#include "project-model.h"
#include "project-view.h"
#include "dialogs.h"


/*

2 Threads: GUI and Work

Add new source.

1. Work add new node
 + Obvious, because it has the parent node
 - Not possible, if the GUI has to get all child of the same node

2. Work add new node, GUI use tree data

3. Work create new node only but doesn't add it

4. GUI create a new node and call the save function later


The GUI cannot change links, else the Work cannot follow links when getting a
node. The Work can even need parent links.
=> GUI cannot read links except in particular case

The GUI can read and write common part
=> We need to copy the properties anyway when changing them

The GUI can only read common part
=> 

Have a proxy node for setting properties, this proxy will copy add common data
and keep a link with the original node and reference count. The GUI can still
read all data in the proxy without disturbing the Work which can change the
underlines node. The proxy address is returned immediatly when the
set_properties function is used. If another set_properties is done on the same
node the proxy is reused, incrementing reference count.

There is no need to proxy after add or remove functions, because the links are
already copied in the module.
 
After changing a property should we reload automatically ?


Reloading a node can change its property, so we need a proxy for the load too.

Proxy has to be created in GUI, because we need to update tree data.

Instead of using a Proxy, we can copy all data everytime, this will allow
automatic reload.
 

 
 Work has always a full access to link
 GUI read a special GNode tree created by thread

 Work can always read common data, and can write them before sending them to GUI
 or in when modification are requested by the GUI (the GUI get a proxy)
 GUI can only read common data

 Work has always a full access to specific data.
 GUI has no access to specific data
 
 
*/
 
/* Types
 *---------------------------------------------------------------------------*/

typedef enum
{
	LOAD,
	UNLOAD,
	RELOAD,
	RELOAD_NODE,
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
	AnjutaProjectNode *proxy;
	GHashTable *map;
};

/* Signal
 *---------------------------------------------------------------------------*/

enum
{
	UPDATED,
	LOADED,
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
	if (node != NULL)
	{
		job->node = node;
		job->proxy = anjuta_project_proxy_new (node);
		g_message ("node %p proxy %p", job->node, job->proxy);
	}
	job->callback = callback;

	return job;
}

static void
pm_free_node (AnjutaProjectNode *node, IAnjutaProject *project)
{
	gint type = anjuta_project_node_get_full_type (node);
	
	if (type & ANJUTA_PROJECT_PROXY)
	{
		g_message ("free proxy %p", node);
		anjuta_project_proxy_unref (node);
	}
	else
	{
		ianjuta_project_remove_node (project, node, NULL);
	}
}

static void
pm_job_free (PmJob *job, IAnjutaProject *project)
{
	if (job->file != NULL) g_object_unref (job->file);
	if (job->name != NULL) g_free (job->name);
	if (job->error != NULL) g_error_free (job->error);
	if ((job->proxy != NULL) && (job->node != NULL))
	{
		g_message ("free proxy %p", job->node);
		anjuta_project_node_all_foreach (job->node, (AnjutaProjectNodeFunc)pm_free_node, project);
	}
	if (job->map != NULL) g_hash_table_destroy (job->map);
}

static void
pm_project_push_command (AnjutaPmProject *project, PmCommand command, GFile *file, const gchar *name, AnjutaProjectNode *node, PmJobCallback callback)
{
	PmJob *job;

	job = pm_job_new (command, file, name, node, callback);

	if (project->busy == 0)
	{
		project->busy++;
		g_async_queue_push (project->work_queue, job);
	}
	else
	{
		g_queue_push_tail (project->job_queue, job);
	}
}

/* Thread functions
 *---------------------------------------------------------------------------*/

static gint
pm_project_compare_node (AnjutaProjectNode *old_node, AnjutaProjectNode *new_node)
{
	gchar *name1;
	gchar *name2;
	GFile *file1;
	GFile *file2;

	name1 = anjuta_project_node_get_name (old_node);
	name2 = anjuta_project_node_get_name (new_node);
	file1 = anjuta_project_node_get_file (old_node);
	file2 = anjuta_project_node_get_file (new_node);

	return (anjuta_project_node_get_type (old_node) == anjuta_project_node_get_type (new_node))
		&& ((name1 == NULL) || (name2 == NULL) || (strcmp (name1, name2) == 0))
		&& ((file1 == NULL) || (file2 == NULL) || g_file_equal (file1, file2)) ? 0 : 1;
}

static void
pm_project_map_children (PmJob *job, AnjutaProjectNode *old_node, AnjutaProjectNode *new_node)
{
	GList *children = NULL;

	for (new_node = anjuta_project_node_first_child (new_node); new_node != NULL; new_node = anjuta_project_node_next_sibling (new_node))
	{
		children = g_list_prepend (children, new_node);
		g_message ("prepend %p", new_node);
	}
	children = g_list_reverse (children);

	for (old_node = anjuta_project_node_first_child (old_node); old_node != NULL; old_node = anjuta_project_node_next_sibling (old_node))
	{
		GList *same;

		same = g_list_find_custom (children, old_node, (GCompareFunc)pm_project_compare_node);
		g_message ("find %p get %p", old_node, same->data);
		
		if (same != NULL)
		{
			g_hash_table_insert (job->map, old_node, (AnjutaProjectNode *)same->data);
			g_message ("map %p->%p", old_node, same->data);
			
			pm_project_map_children ((PmJob *)job, old_node, (AnjutaProjectNode *)same->data);
			children = g_list_delete_link (children, same);
		}
	}
	
	g_list_free (children);
}
 
static void
pm_project_map_node (PmJob *job)
{
	if ((job->proxy != NULL) && (job->node != NULL))
	{
		AnjutaProjectNode *old_node;
		AnjutaProjectNode *new_node;
		
		job->map = g_hash_table_new (g_direct_hash, NULL);
		old_node = job->node;
		new_node = job->proxy;
		
		if (pm_project_compare_node (old_node, new_node) == 0)
		{
			g_hash_table_insert (job->map, old_node, new_node);
			g_message ("map %p->%p", old_node, new_node);
			
			pm_project_map_children (job, old_node, new_node);
		}
	}
}
 
static gpointer
pm_project_thread_main_loop (AnjutaPmProject *project)
{
	PmJob *job;
	
	for (;;)
	{
		AnjutaProjectNode *root;
		AnjutaProjectNode *node;
		
		job = (PmJob *)g_async_queue_pop (project->work_queue);

		switch (job->command)
		{
		case LOAD:
			root = ianjuta_project_new_root_node (project->project, job->file, NULL);
			node = ianjuta_project_load_node (project->project, root, &(job->error));
			g_message ("load get root %p node %p, error %p", root, node, job->error);
			if (job->error == NULL)
			{
				job->node = node;
			}
			else
			{
				ianjuta_project_remove_node (project->project, root, NULL);
			}
			break;
		case RELOAD:
			node = ianjuta_project_load_node (project->project, job->node, &(job->error));
			/*if (job->error == NULL)
			{
				job->node = node;
			}*/
			break;						
		case RELOAD_NODE:
			g_message ("thread load node %p", job->proxy);
			node = ianjuta_project_load_node (project->project, job->proxy, &(job->error));
			g_message ("thread load node complete");
			if (job->error == NULL)
			{
				pm_project_map_node (job);
			}
			/*if (job->error == NULL)
			{
				job->node = node;
			}*/
			break;
		case EXIT:
			do
			{
				pm_job_free (job, project->project);
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
		pm_job_free (job, project->project);
		project->busy--;
		if (project->busy == 0)
		{
			job = g_queue_pop_head (project->job_queue);
			if (job != NULL)
		    {
				project->busy = 1;
				g_async_queue_push (project->work_queue, job);
			}
		}
	}
}

static gboolean
pm_project_start_thread (AnjutaPmProject *project, GError **error)
{
	project->done_queue = g_async_queue_new ();
	project->work_queue = g_async_queue_new ();
	project->job_queue = g_queue_new ();

	project->worker = g_thread_create ((GThreadFunc) pm_project_thread_main_loop, project, TRUE, error);

	if (project->worker == NULL) {
		g_async_queue_unref (project->work_queue);
		project->work_queue = NULL;
		g_async_queue_unref (project->done_queue);
		project->done_queue = NULL;
		g_queue_free (project->job_queue);
		project->job_queue = NULL;

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
		g_async_queue_push (project->work_queue, job);
		g_thread_join (project->worker);
		project->worker = NULL;

		// Free queue
		g_async_queue_unref (project->work_queue);
		project->work_queue = NULL;
		g_queue_foreach (project->job_queue, (GFunc)pm_job_free, project->project);
		g_queue_free (project->job_queue);
		project->job_queue = NULL;
		for (;;)
		{
			job = g_async_queue_try_pop (project->done_queue);
			if (job == NULL) break;
			pm_job_free (job, project->project);
		}
		project->done_queue = NULL;
	}

	return TRUE;
}

/* Public functions
 *---------------------------------------------------------------------------*/

static void on_pm_project_load_incomplete (AnjutaProjectNode *node, AnjutaPmProject *project);

static void
on_pm_project_reloaded_node (AnjutaPmProject *project, PmJob *job)
{
	if (job->error != NULL)
	{
		g_warning ("unable to load node");
		pm_project_stop_thread (project);
		g_object_unref (project->project);
		project->project = NULL;
		g_signal_emit (G_OBJECT (project), signals[LOADED], 0, job->error);
	}
	else
	{
		g_message ("reloaded node");
		//g_object_set (G_OBJECT (project->model), "project", project, NULL);
		// Check for incompletely loaded object and load them
		if (anjuta_project_node_get_state (job->proxy) & ANJUTA_PROJECT_INCOMPLETE)
		{
			project->incomplete_node--;
			g_message ("remaining node %d", project->incomplete_node);
		}
		anjuta_project_node_clear_state (job->proxy, ANJUTA_PROJECT_LOADING | ANJUTA_PROJECT_INCOMPLETE);
		anjuta_project_node_all_foreach (job->proxy, (AnjutaProjectNodeFunc)on_pm_project_load_incomplete, project);
		
		if (project->root == job->node)
		{
			project->root = job->proxy;
			g_message ("reload root");
			gbf_project_model_update_tree (project->model, job->proxy, NULL, job->map);
		}
		else
		{
			GtkTreeIter iter;
			gboolean found;
			
			found = gbf_project_model_find_node (project->model, &iter, NULL, job->node);
			g_message ("reload node %d", found);
			gbf_project_model_update_tree (project->model, job->proxy, &iter, job->map);
		}
		
		
		if (project->incomplete_node == 0)
		{
			g_signal_emit (G_OBJECT (project), signals[LOADED], 0, NULL);
		}
	}
}

static void
on_pm_project_reloaded (AnjutaPmProject *project, PmJob *job)
{
	if (job->error != NULL)
	{
		g_warning ("unable to load node");
		pm_project_stop_thread (project);
		g_object_unref (project->project);
		project->project = NULL;
		g_signal_emit (G_OBJECT (project), signals[LOADED], 0, job->error);
	}
	else
	{
#if 0
		//g_object_set (G_OBJECT (project->model), "project", project, NULL);
		// Check for incompletely loaded object and load them
		anjuta_project_node_clear_state (job->node, ANJUTA_PROJECT_LOADING | ANJUTA_PROJECT_INCOMPLETE);
		g_message ("remaining node %d", project->incomplete_node);
		project->incomplete_node--;
		anjuta_project_node_all_foreach (job->node, (AnjutaProjectNodeFunc)on_pm_project_load_incomplete, project);
		//g_signal_emit (G_OBJECT (project), signals[UPDATED], 0, job->error);
		gbf_project_model_update_tree (project->model, NULL, NULL, NULL);
		
		if (project->incomplete_node == 0)
		{
			g_signal_emit (G_OBJECT (project), signals[LOADED], 0, NULL);
		}
#endif
	}
}

static void
on_pm_project_load_incomplete (AnjutaProjectNode *node, AnjutaPmProject *project)
{
	gint state = anjuta_project_node_get_state (node);
	
	if ((state & ANJUTA_PROJECT_INCOMPLETE) && !(state & ANJUTA_PROJECT_LOADING))
	{
		project->incomplete_node++;
		anjuta_project_node_set_state (node, ANJUTA_PROJECT_LOADING);
		pm_project_push_command (project, RELOAD_NODE, NULL, NULL, node, on_pm_project_reloaded_node);
	}
}

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
		g_signal_emit (G_OBJECT (project), signals[LOADED], 0, job->error);
	}
	else
	{
		g_message ("project loaded project root %p node %p", anjuta_pm_project_get_root (project), job->node);
		g_message ("root child %d", g_node_n_children (job->node));
		g_message ("root all nodes %d", g_node_n_nodes (job->node, G_TRAVERSE_ALL));
		project->root = job->node;
		g_object_set (G_OBJECT (project->model), "project", project, NULL);
		gbf_project_model_update_tree (project->model, project->root, NULL, NULL);
		
		// Check for incompletely loaded object and load them
		project->incomplete_node = 0;
		anjuta_project_node_all_foreach (job->node, (AnjutaProjectNodeFunc)on_pm_project_load_incomplete, project);
		if (project->incomplete_node == 0)
		{
			g_signal_emit (G_OBJECT (project), signals[LOADED], 0, NULL);
		}
	}
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
anjuta_pm_project_reload_node (AnjutaPmProject *project, AnjutaProjectNode *node, GError **error)
{
	pm_project_push_command (project, RELOAD, NULL, NULL, node, on_pm_project_reloaded);

	return TRUE;
}

gboolean 
anjuta_pm_project_unload (AnjutaPmProject *project, GError **error)
{
	pm_project_stop_thread (project);
	
	g_object_set (G_OBJECT (project->model), "project", NULL, NULL);
	g_object_unref (project->project);
	project->project = NULL;

	/* Remove project properties dialogs */
	if (project->properties_dialog != NULL) gtk_widget_destroy (project->properties_dialog);
	project->properties_dialog = NULL;
	
	return TRUE;
}

gboolean
anjuta_pm_project_refresh (AnjutaPmProject *project, GError **error)
{
	g_message ("reload project");
	pm_project_push_command (project, RELOAD_NODE, NULL, NULL, project->root, on_pm_project_reloaded_node);

	return TRUE;
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
anjuta_pm_project_get_node_info (AnjutaPmProject *project)
{
	g_return_val_if_fail (project->project != NULL, NULL);
	
	return ianjuta_project_get_node_info (project->project, NULL);
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
	
	return ianjuta_project_add_name_node (project->project, group, NULL, ANJUTA_PROJECT_GROUP, name, error);
}

AnjutaProjectNode *
anjuta_pm_project_add_target (AnjutaPmProject *project, AnjutaProjectNode *group, const gchar *name, AnjutaProjectNodeType type, GError **error)
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

AnjutaProjectNode *
anjuta_pm_project_get_node_from_file (AnjutaPmProject *project, AnjutaProjectNodeType type, GFile *file)
{
	GtkTreeIter iter;
	AnjutaProjectNode *node = NULL;
	
	if (gbf_project_model_find_file (project->model, &iter, NULL, type, file))
	{
		
		node = gbf_project_model_get_node (project->model, &iter);
	}

	return NULL;
}

/* Display properties dialog. These dialogs are not modal, so a pointer on each
 * dialog is kept with in node data to be able to destroy them if the node is
 * removed. It is useful to put the dialog at the top if the same target is
 * selected while the corresponding dialog already exist instead of creating
 * two times the same dialog.
 * The project properties dialog is display if the node iterator is NULL. */

gboolean
anjuta_pm_project_show_properties_dialog (AnjutaPmProject *project, GbfTreeData *data)
{
	GtkWidget **dialog;
	AnjutaProjectNode *node;
	
	if (data == NULL)
	{
		/* Show project properties dialog */
		dialog = &project->properties_dialog;
		node = project->root;
	}
	else
	{
		/* Show node properties dialog */
		dialog = &data->properties_dialog;
		node = anjuta_pm_project_get_node (project, data);
	}
	
	if (*dialog != NULL)
	{
		/* Show already existing dialog */
		gtk_window_present (GTK_WINDOW (*dialog));
	}
	else
	{
		pm_project_create_properties_dialog (
			project->project,
			dialog,
			GTK_WINDOW (project->plugin->shell),
			node);
	}

	return TRUE;
}

/* Implement GObject
 *---------------------------------------------------------------------------*/

G_DEFINE_TYPE (AnjutaPmProject, anjuta_pm_project, G_TYPE_OBJECT);

static void
anjuta_pm_project_init (AnjutaPmProject *project)
{
	project->model = gbf_project_model_new (NULL);
	project->plugin = NULL;

	project->properties_dialog = NULL;
	
	project->job_queue = NULL;
	project->work_queue = NULL;
	project->done_queue = NULL;
	project->worker = NULL;
	project->idle_func = 0;
	project->stopping = FALSE;
	project->busy = 0;
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
	
	signals[LOADED] = g_signal_new ("loaded",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (AnjutaPmProjectClass, loaded),
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
