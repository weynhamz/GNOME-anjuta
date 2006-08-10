/*
 *   anjuta-children.c
 *   Copyright (C) 2003 Naba Kumar  <naba@gnome.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <libanjuta/anjuta-children.h>
#include <signal.h>

static GList *registered_child_processes = NULL;
static GList *registered_child_processes_cb = NULL;
static GList *registered_child_processes_cb_data = NULL;

static void
anjuta_child_terminated (int t)
{
	int status;
	gint idx;
	pid_t pid;
	int (*callback) (int st, gpointer d);
	gpointer cb_data;
	
	while (1)
	{
		pid = waitpid (-1, &status, WNOHANG);
		
		if (pid < 1)
			return;
		idx = g_list_index (registered_child_processes, (int *) pid);
		if (idx < 0)
			continue;
		callback =
			g_list_nth_data (registered_child_processes_cb, idx);
		g_return_if_fail (callback != NULL);
		
		cb_data = g_list_nth_data (registered_child_processes_cb_data, idx);
		if (callback)
			(*callback) (status, cb_data);
		anjuta_children_unregister (pid);
		anjuta_children_recover ();
	}
}

/**
 * anjuta_children_recover:
 * 
 * Recovers child management signaling.
 */
void
anjuta_children_recover ()
{
	signal(SIGCHLD, anjuta_child_terminated);
}

/**
 * anjuta_children_register:
 * @pid: Process ID of the child (usually the value retured by fork or similar
 * system calls.
 * @ch_terminated: Callback function which will be called when this child
 * exits. The callback should be defined as the
 * type #AnjutaChildTerminatedCallback.
 * @data: User data.
 *
 * Registers a child process with the manager. ch_terminated will be called
 * when the child terminates. DO NOT use SIGCHLD directly, otherwise whole
 * children management will fail.
 */
void
anjuta_children_register (pid_t pid,
						  AnjutaChildTerminatedCallback ch_terminated,
						  gpointer data)
{
	/* Reinforce possible loss in signal callback */
	anjuta_children_recover ();
	
	if (pid < 1)
		return;
	registered_child_processes =
		g_list_append (registered_child_processes, (int *) pid);
	registered_child_processes_cb =
		g_list_append (registered_child_processes_cb, ch_terminated);
	registered_child_processes_cb_data =
		g_list_append (registered_child_processes_cb_data, data);
}

/**
 * anjuta_children_unregister:
 * @pid: Process ID of the child.
 * 
 * Unregisters the child process (It should have been registred before with
 * #anjuta_children_register() call). No child terminated callback will be
 * executed for this child.
 */
void
anjuta_children_unregister (pid_t pid)
{
	gint idx;
	GList *ptr;
	
	idx = g_list_index (registered_child_processes, (int *) pid);
	registered_child_processes =
		g_list_remove (registered_child_processes, (int *) pid);
	
	ptr = g_list_nth (registered_child_processes_cb, idx);

	registered_child_processes_cb =
		g_list_delete_link (registered_child_processes_cb, ptr);
	
	ptr = g_list_nth (registered_child_processes_cb_data, idx);

	registered_child_processes_cb_data =
		g_list_delete_link (registered_child_processes_cb_data, ptr);
}

/**
 * anjuta_children_foreach:
 * @cb: Callback function.
 * @data: User data.
 * 
 * Calls the given callback function with the data for each child
 * registered, that have not yet been terminated.
 */
void
anjuta_children_foreach (GFunc cb, gpointer data)
{
	g_list_foreach (registered_child_processes, cb, data);
}

/**
 * anjuta_children_finalize:
 * 
 * Shuts down the children management. Usually not required to call, if you
 * you are anyway exiting the program.
 */
void
anjuta_children_finalize()
{
	signal(SIGCHLD, SIG_DFL);
}
