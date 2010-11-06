/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/* command-queue.c
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

#include "command-queue.h"
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-project-backend.h>

#include <string.h>

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

struct _PmCommandQueue
{
	GQueue *job_queue;
	GAsyncQueue *work_queue;
	GAsyncQueue *done_queue;
	GThread *worker;
	guint idle_func;
	gboolean stopping;
	guint busy;
};


/* Signal
 *---------------------------------------------------------------------------*/

/* Command functions
 *---------------------------------------------------------------------------*/

static gboolean
pm_command_exit_work (PmJob *job)
{
	PmCommandQueue *queue;
	
	g_return_val_if_fail (job != NULL, FALSE);
	
	queue = (PmCommandQueue *)job->user_data;

	/* Push job in complete queue as g_thread_exit will stop the thread
	 * immediatly */
	g_async_queue_push (queue->done_queue, job);
	g_thread_exit (0);
	
	return TRUE;
}

static PmCommandWork PmExitCommand = {NULL, pm_command_exit_work, NULL};

/* Forward declarations
 *---------------------------------------------------------------------------*/

static gboolean pm_command_queue_idle (PmCommandQueue *queue);

/* Worker thread functions
 *---------------------------------------------------------------------------*/

/* Run work function in worker thread */
static gpointer
pm_command_queue_thread_main_loop (PmCommandQueue *queue)
{
	for (;;)
	{
		PmJob *job;
		PmCommandFunc func;
		
		/* Get new job */
		job = (PmJob *)g_async_queue_pop (queue->work_queue);

		/* Get work function and call it if possible */
		func = job->work->worker;
		if (func != NULL)
		{
			func (job);
		}
		
		/* Push completed job in queue */
		g_async_queue_push (queue->done_queue, job);
	}

	return NULL;
}

/* Main thread functions
 *---------------------------------------------------------------------------*/

static gboolean
pm_command_queue_start_thread (PmCommandQueue *queue, GError **error)
{
	queue->done_queue = g_async_queue_new ();
	queue->work_queue = g_async_queue_new ();
	queue->job_queue = g_queue_new ();

	queue->worker = g_thread_create ((GThreadFunc) pm_command_queue_thread_main_loop, queue, TRUE, error);

	if (queue->worker == NULL) {
		g_async_queue_unref (queue->work_queue);
		queue->work_queue = NULL;
		g_async_queue_unref (queue->done_queue);
		queue->done_queue = NULL;
		g_queue_free (queue->job_queue);
		queue->job_queue = NULL;

		return FALSE;
	}
	else
	{
		queue->idle_func = g_idle_add ((GSourceFunc) pm_command_queue_idle, queue);
		queue->stopping = FALSE;

		return TRUE;
	}
}

static gboolean
pm_command_queue_stop_thread (PmCommandQueue *queue)
{
	if (queue->job_queue)
	{
		PmJob *job;
		
		queue->stopping = TRUE;

		// Remove idle function
		g_idle_remove_by_data (queue);
		queue->idle_func = 0;

		// Request to terminate thread
		job = pm_job_new (&PmExitCommand, NULL, NULL, NULL, 0, NULL, NULL, queue);
		g_async_queue_push (queue->work_queue, job);
		g_thread_join (queue->worker);
		queue->worker = NULL;

		// Free queue
		g_async_queue_unref (queue->work_queue);
		queue->work_queue = NULL;
		g_queue_foreach (queue->job_queue, (GFunc)pm_job_free, NULL);
		g_queue_free (queue->job_queue);
		queue->job_queue = NULL;
		for (;;)
		{
			job = g_async_queue_try_pop (queue->done_queue);
			if (job == NULL) break;
			pm_job_free (job);
		}
		queue->done_queue = NULL;
	}

	return TRUE;
}

/* Run a command in job queue */
static gboolean
pm_command_queue_run_command (PmCommandQueue *queue)
{
	gboolean running = TRUE;
	
	if (queue->busy == 0)
	{
		/* Worker thread is waiting for new command, check job queue */
		PmJob *job;
		
		do
		{
			PmCommandFunc func;
			
			/* Get next command */
			job = g_queue_pop_head (queue->job_queue);
			running = job != NULL;
			if (!running) break;
			
			/* Get setup function and call it if possible */
			func = job->work->setup;
			if (func != NULL)
			{
				running = func (job);
			}
				
			if (running)
			{
				/* Execute work function in the worker thread */
				queue->busy = 1;
				
				if (queue->idle_func == 0)
				{
					queue->idle_func = g_idle_add ((GSourceFunc) pm_command_queue_idle, queue);
				}
				g_async_queue_push (queue->work_queue, job);
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
pm_command_queue_idle (PmCommandQueue *queue)
{
	gboolean running;

	for (;;)
	{
		PmCommandFunc func;
		PmJob *job;
		
		/* Get completed command */
		job = (PmJob *)g_async_queue_try_pop (queue->done_queue);
		if (job == NULL) break;

		/* Get complete function and call it if possible */
		func = job->work->complete;
		if (func != NULL)
		{
			running = func (job);
		}
		pm_job_free (job);
		queue->busy--;
	}
	
	running = pm_command_queue_run_command (queue);
	if (!running) queue->idle_func = 0;
	
	return running;
}

/* Job functions
 *---------------------------------------------------------------------------*/

PmJob *
pm_job_new (PmCommandWork* work, AnjutaProjectNode *node, AnjutaProjectNode *parent, AnjutaProjectNode *sibling, AnjutaProjectNodeType type, GFile *file, const gchar *name, gpointer user_data)
{
	PmJob *job;

	job = g_new0 (PmJob, 1);
	job->work = work;
	job->node = node;
	job->parent = parent;
	job->sibling = sibling;
	job->type = type;
	if (file != NULL) job->file = g_object_ref (file);
	if (name != NULL) job->name = g_strdup (name);
	job->user_data = user_data;

	return job;
}

void
pm_job_free (PmJob *job)
{
	if (job->error != NULL) g_error_free (job->error);
	if (job->map != NULL) g_hash_table_destroy (job->map);
	if (job->file != NULL) g_object_unref (job->file);
	if (job->name != NULL) g_free (job->name);
}

/* Public functions
 *---------------------------------------------------------------------------*/

void
pm_command_queue_push (PmCommandQueue *queue, PmJob *job)
{
	g_queue_push_tail (queue->job_queue, job);
	
	pm_command_queue_run_command (queue);
}

gboolean
pm_command_queue_is_busy (PmCommandQueue *queue)
{
	//g_message ("pm_command_queue_is_empty %d %d %d busy %d", g_queue_get_length (queue->job_queue), g_async_queue_length(queue->work_queue), g_async_queue_length(queue->done_queue), queue->busy);
	return queue->busy;
}

PmCommandQueue*
pm_command_queue_new (void)
{
	PmCommandQueue *queue;

	queue = g_new0 (PmCommandQueue, 1);
	
	queue->job_queue = NULL;
	queue->work_queue = NULL;
	queue->done_queue = NULL;
	queue->worker = NULL;
	queue->idle_func = 0;
	queue->stopping = FALSE;
	queue->busy = 0;

	pm_command_queue_start_thread (queue, NULL);
	
	return queue;
}

void
pm_command_queue_free (PmCommandQueue *queue)
{
	pm_command_queue_stop_thread (queue);

	g_free (queue);
}

