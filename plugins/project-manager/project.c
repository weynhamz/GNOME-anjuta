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
	LOAD = 0,
	RELOAD,
	ADD,
	REMOVE,
	EXIT,
	LAST_COMMAND
} PmCommand;

typedef struct _PmJob PmJob;

typedef gboolean (*PmCommandFunc) (AnjutaPmProject *project, PmJob *job);

typedef struct _PmCommandWork
{
	PmCommandFunc setup;
	PmCommandFunc worker;
	PmCommandFunc complete;
} PmCommandWork;

struct _PmJob
{
	PmCommand command;
	GFile *file;
	gchar *name;
	AnjutaProjectNode *node;
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

/* Command functions
 *---------------------------------------------------------------------------*/

static PmCommandWork PmCommands[LAST_COMMAND];


/* Forward declarations
 *---------------------------------------------------------------------------*/

static gboolean pm_project_idle_func (AnjutaPmProject *project);
static gboolean pm_project_run_command (AnjutaPmProject *project);

/* Job functions
 *---------------------------------------------------------------------------*/

static PmJob *
pm_job_new (PmCommand command, AnjutaProjectNode *node)
{
	PmJob *job;

	job = g_new0 (PmJob, 1);
	job->command = command;
	job->node = node;

	return job;
}

static void
pm_job_free (PmJob *job)
{
	if (job->error != NULL) g_error_free (job->error);
	if (job->map != NULL) g_hash_table_destroy (job->map);
}

/* Worker thread functions
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

	if (new_node != NULL)
	{
		for (new_node = anjuta_project_node_first_child (new_node); new_node != NULL; new_node = anjuta_project_node_next_sibling (new_node))
		{
			children = g_list_prepend (children, new_node);
		}
		children = g_list_reverse (children);
	}

	for (old_node = anjuta_project_node_first_child (old_node); old_node != NULL; old_node = anjuta_project_node_next_sibling (old_node))
	{
		GList *same;

		same = g_list_find_custom (children, old_node, (GCompareFunc)pm_project_compare_node);
		
		if (same != NULL)
		{
			g_hash_table_insert (job->map, old_node, (AnjutaProjectNode *)same->data);
			
			pm_project_map_children ((PmJob *)job, old_node, (AnjutaProjectNode *)same->data);
			children = g_list_delete_link (children, same);
		}
		else
		{
			/* Mark deleted node */
			g_hash_table_insert (job->map, old_node, NULL);
			pm_project_map_children ((PmJob *)job, old_node, NULL);
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
		
		//g_hash_table_insert (job->map, old_node, new_node);
			
		pm_project_map_children (job, old_node, new_node);
	}
}

static gboolean
pm_command_load_work (AnjutaPmProject *project, PmJob *job)
{
	AnjutaProjectNode *node;
	

	node = ianjuta_project_load_node (project->project, job->proxy, &(job->error));
	if (job->error == NULL)
	{
		pm_project_map_node (job);
	}
	
	return TRUE;
}

static gboolean
pm_command_save_work (AnjutaPmProject *project, PmJob *job)
{
	AnjutaProjectNode *node;
	
	node = ianjuta_project_save_node (project->project, job->node, &(job->error));
	
	return TRUE;
}

static gboolean
pm_command_exit_work (AnjutaPmProject *project, PmJob *job)
{
	g_return_val_if_fail (job != NULL, FALSE);

	/* Push job in complete queue as g_thread_exit will stop the thread
	 * immediatly */
	g_async_queue_push (project->done_queue, job);
	g_thread_exit (0);
	
	return TRUE;
}

/* Run work function in worker thread */
static gpointer
pm_project_thread_main_loop (AnjutaPmProject *project)
{
	for (;;)
	{
		PmJob *job;
		PmCommandFunc func;
		
		/* Get new job */
		job = (PmJob *)g_async_queue_pop (project->work_queue);

		/* Get work function and call it if possible */
		func = PmCommands[job->command].worker;
		if (func != NULL)
		{
			func (project, job);
		}
		
		/* Push completed job in queue */
		g_async_queue_push (project->done_queue, job);
	}
}

/* Main thread functions
 *---------------------------------------------------------------------------*/

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
		job = pm_job_new (EXIT, NULL);
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
			pm_job_free (job);
		}
		project->done_queue = NULL;
	}

	return TRUE;
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
		ianjuta_project_free_node (project, node, NULL);
	}
}

static void
pm_project_push_command (AnjutaPmProject *project, PmCommand command, AnjutaProjectNode *node)
{
	PmJob *job;

	job = pm_job_new (command, node);
	g_queue_push_tail (project->job_queue, job);
	
	pm_project_run_command (project);
}

static void
on_pm_project_load_incomplete (AnjutaProjectNode *node, AnjutaPmProject *project)
{
	gint state = anjuta_project_node_get_state (node);
	
	if ((state & ANJUTA_PROJECT_INCOMPLETE) && !(state & ANJUTA_PROJECT_LOADING))
	{
		project->incomplete_node++;
		anjuta_project_node_set_state (node, ANJUTA_PROJECT_LOADING);
		//g_message ("load incomplete %p", node);
		pm_project_push_command (project, RELOAD, node);
	}
}

static gboolean
pm_command_load_setup (AnjutaPmProject *project, PmJob *job)
{
	g_return_val_if_fail (job != NULL, FALSE);
	g_return_val_if_fail (job->node != NULL, FALSE);
	
	job->proxy = anjuta_project_proxy_new (job->node);
	anjuta_project_node_replace (job->node, job->proxy);
	anjuta_project_proxy_exchange_data (job->node, job->proxy);
	
	return TRUE;
}

static void
check_queue (GQueue *queue, GHashTable *map)
{
	guint i;
	
	//g_message ("check queue size %d", g_queue_get_length (queue));
	for (i = 0;; i++)
	{
		PmJob *job = (PmJob *)g_queue_peek_nth (queue, i);
		
		if (job == NULL) break;
		if (job->node != NULL)
		{
			AnjutaProjectNode *replace;
			AnjutaProjectNode *node;
			
			/* Get original node if we have a proxy here */
			node = anjuta_project_proxy_get_node (job->node);
			
			if (g_hash_table_lookup_extended (map, node, NULL, (gpointer *)&replace))
			{
				//g_message ("*** delete node %p used ***", job->node);
				if (replace != NULL)
				{
					/* Replaced node */
					job->node = replace;
				}
				else
				{
					/* node has been deleted */
					g_warning ("Node %s has been delete before executing command %d", anjuta_project_node_get_name (job->node), job->command);
					job = g_queue_pop_nth (queue, i);
					pm_job_free (job);
					i--;
				}
			}
		}
	}
}

static gboolean
pm_command_load_complete (AnjutaPmProject *project, PmJob *job)
{
	anjuta_project_proxy_exchange_data (job->proxy, job->node);
	anjuta_project_node_exchange (job->proxy, job->node);

	if (job->error != NULL)
	{
		g_warning ("unable to load node");
		g_signal_emit (G_OBJECT (project), signals[job->command == LOAD ? LOADED : UPDATED], 0, job->error);
	}
	else
	{
		if (job->command == LOAD)
		{
			g_object_set (G_OBJECT (project->model), "project", project, NULL);
			project->incomplete_node = 0;
		}
		
		if (project->root == job->node)
		{
			gbf_project_model_update_tree (project->model, job->node, NULL, job->map);
		}
		else
		{
			GtkTreeIter iter;
			gboolean found;
			
			found = gbf_project_model_find_node (project->model, &iter, NULL, job->node);
			//g_message ("reload node %p found %d", job->node, found);
			gbf_project_model_update_tree (project->model, job->node, &iter, job->map);
		}
		
		
		// Check for incompletely loaded object and load them
		if (anjuta_project_node_get_state (job->node) & ANJUTA_PROJECT_INCOMPLETE)
		{
			project->incomplete_node--;
			//g_message ("remaining node %d", project->incomplete_node);
		}
		anjuta_project_node_clear_state (job->node, ANJUTA_PROJECT_LOADING | ANJUTA_PROJECT_INCOMPLETE);
		anjuta_project_node_all_foreach (job->node, (AnjutaProjectNodeFunc)on_pm_project_load_incomplete, project);
		
		if (project->incomplete_node == 0)
		{
			g_signal_emit (G_OBJECT (project), signals[job->command == LOAD ? LOADED : UPDATED], 0, NULL);
		}
		check_queue (project->job_queue, job->map);
	}
	
	anjuta_project_node_all_foreach (job->proxy, (AnjutaProjectNodeFunc)pm_free_node, project->project);
	
	return TRUE;
}

static gboolean
pm_command_add_setup (AnjutaPmProject *project, PmJob *job)
{
	AnjutaProjectNode *parent;
	AnjutaProjectNode *sibling;
	
	g_return_val_if_fail (job != NULL, FALSE);
	g_return_val_if_fail (job->node != NULL, FALSE);

	/* Add new node in project tree.
	 * It is safe to do it here because the worker thread is waiting */
	parent = anjuta_project_node_parent (job->node);
	sibling = job->node->prev;
	job->node->parent = NULL;
	job->node->prev = NULL;
	anjuta_project_node_insert_before (parent, sibling, job->node);
	
	return TRUE;
}

static gboolean
pm_command_remove_setup (AnjutaPmProject *project, PmJob *job)
{
	GtkTreeIter iter;
	
	g_return_val_if_fail (job != NULL, FALSE);
	g_return_val_if_fail (job->node != NULL, FALSE);

	/* Remove node from project tree */
	if (gbf_project_model_find_node (project->model, &iter, NULL, job->node))
	{
		gbf_project_model_remove (project->model, &iter);
		anjuta_project_node_set_state (job->node, ANJUTA_PROJECT_REMOVED);
		
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

static gboolean
pm_command_remove_complete (AnjutaPmProject *project, PmJob *job)
{
	g_return_val_if_fail (job != NULL, FALSE);
	g_return_val_if_fail (job->node != NULL, FALSE);

	/* Remove node from node tree */
	anjuta_project_node_remove (job->node);
	anjuta_project_node_all_foreach (job->node, (AnjutaProjectNodeFunc)pm_free_node, project->project);
	
	return TRUE;
}
 
static PmCommandWork PmCommands[LAST_COMMAND] = {
			{pm_command_load_setup,
			pm_command_load_work,
			pm_command_load_complete},
			{pm_command_load_setup,
			pm_command_load_work,
			pm_command_load_complete},
			{pm_command_add_setup,
			pm_command_save_work,
			NULL},
			{pm_command_remove_setup,
			pm_command_save_work,
			pm_command_remove_complete},
			{NULL,
			pm_command_exit_work,
			NULL}};

/* Run a command in job queue */
static gboolean
pm_project_run_command (AnjutaPmProject *project)
{
	gboolean running = TRUE;
	
	if (project->busy == 0)
	{
		/* Worker thread is waiting for new command, check job queue */
		PmJob *job;
		
		do
		{
			PmCommandFunc func;
			
			/* Get next command */
			job = g_queue_pop_head (project->job_queue);
			running = job != NULL;
			if (!running) break;
			
			/* Get setup function and call it if possible */
			func = PmCommands[job->command].setup;
			if (func != NULL)
			{
				running = func (project, job);
			}
				
			if (running)
			{
				/* Execute work function in the worker thread */
				project->busy = 1;
				
				if (project->idle_func == 0)
				{
					project->idle_func = g_idle_add ((GSourceFunc) pm_project_idle_func, project);
				}
				g_async_queue_push (project->work_queue, job);
			}
			else
			{
				/* Discard command */
				pm_job_free (job);
			}
		} while (!running);
	}
	
	return running;
}

static gboolean
pm_project_idle_func (AnjutaPmProject *project)
{
	gboolean running;

	for (;;)
	{
		PmCommandFunc func;
		PmJob *job;
		
		/* Get completed command */
		job = (PmJob *)g_async_queue_try_pop (project->done_queue);
		if (job == NULL) break;

		/* Get complete function and call it if possible */
		func = PmCommands[job->command].complete;
		if (func != NULL)
		{
			running = func (project, job);
		}
		pm_job_free (job);
		project->busy--;
	}
	
	running = pm_project_run_command (project);
	if (!running) project->idle_func = 0;
	
	return running;
}

/* Public functions
 *---------------------------------------------------------------------------*/

static void
on_node_updated (IAnjutaProject *sender, AnjutaProjectNode *node, AnjutaPmProject *project)
{
	g_message ("on node updated %p", node);
	pm_project_push_command (project, RELOAD, node);
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
	g_signal_connect (G_OBJECT (project->project),
						"node-updated",
						G_CALLBACK (on_node_updated),
						project);
	
	project->root = ianjuta_project_new_node (project->project, NULL, ANJUTA_PROJECT_ROOT, file, NULL, NULL);
	pm_project_push_command (project, LOAD, project->root);
	
	return TRUE;
}

static gboolean
anjuta_pm_project_reload_node (AnjutaPmProject *project, AnjutaProjectNode *node, GError **error)
{
	pm_project_push_command (project, RELOAD, node);

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
	//g_message ("reload project %p", project->root);
	
	pm_project_push_command (project, RELOAD, project->root);

	return TRUE;
}

gint
anjuta_pm_project_get_capabilities (AnjutaPmProject *project)
{
	gint caps = 0;

	if (project->project != NULL)
	{
		GList *item;

		for (item = anjuta_pm_project_get_node_info (project); item != NULL; item = g_list_next (item))
		{
			AnjutaProjectNodeInfo *info = (AnjutaProjectNodeInfo *)item->data;

			switch (info->type & ANJUTA_PROJECT_TYPE_MASK)
			{
			case ANJUTA_PROJECT_GROUP:
				caps |= ANJUTA_PROJECT_CAN_ADD_GROUP;
				break;
			case ANJUTA_PROJECT_TARGET:
				caps |= ANJUTA_PROJECT_CAN_ADD_TARGET;
				break;
			case ANJUTA_PROJECT_SOURCE:
				caps |= ANJUTA_PROJECT_CAN_ADD_SOURCE;
				break;
			case ANJUTA_PROJECT_MODULE:
				caps |= ANJUTA_PROJECT_CAN_ADD_MODULE;
				break;
			case ANJUTA_PROJECT_PACKAGE:
				caps |= ANJUTA_PROJECT_CAN_ADD_PACKAGE;
				break;
			default:
				break;
			}
		}
	}

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
anjuta_pm_project_add_group (AnjutaPmProject *project, AnjutaProjectNode *parent, AnjutaProjectNode *sibling, const gchar *name, GError **error)
{
	AnjutaProjectNode *node;
	
	g_return_val_if_fail (project->project != NULL, NULL);
	
	node = ianjuta_project_new_node (project->project, parent, ANJUTA_PROJECT_GROUP, NULL, name, error);
	node->parent = parent;
	node->prev = sibling;
	pm_project_push_command (project, ADD, node);

	return node;
}

AnjutaProjectNode *
anjuta_pm_project_add_target (AnjutaPmProject *project, AnjutaProjectNode *parent, AnjutaProjectNode *sibling, const gchar *name, AnjutaProjectNodeType type, GError **error)
{
	g_return_val_if_fail (project->project != NULL, NULL);
	
	return ianjuta_project_add_target (project->project, parent, name, type, error);
}

AnjutaProjectNode *
anjuta_pm_project_add_source (AnjutaPmProject *project, AnjutaProjectNode *parent, AnjutaProjectNode *sibling, const gchar *name, GError **error)
{
	AnjutaProjectNode *node;

	g_return_val_if_fail (project->project != NULL, NULL);
	
	node = ianjuta_project_new_node (project->project, parent, ANJUTA_PROJECT_SOURCE, NULL, name, error);
	node->parent = parent;
	node->prev = sibling;
	pm_project_push_command (project, ADD, node);

	return node;
}

gboolean
anjuta_pm_project_remove (AnjutaPmProject *project, AnjutaProjectNode *node, GError **error)
{
	pm_project_push_command (project, REMOVE, node);

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
