/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/**
 * SECTION:anjuta-children
 * @title: Children management
 * @short_description: Children management in Anjuta
 * @see_also:
 * @stability: Unstable
 * @include: libanjuta/anjuta-children.h
 * 
 * All child processes in anjuta are registred using API here. SIGCHLD should
 * never be used directly. Use anjuta_children_register() or
 * anjuta_children_unregister() to register or unregister a child process.
 * Notifications are sent if callback function is given in
 * anjuta_children_register(), which can be used in lieu of hooking SIGCHLD
 * signal.
 * 
 * plugin developers should not bother about anjuta_children_init() and
 * anjuta_children_finalize() functions. They are only for the shell
 * implementor.
 */

#include <signal.h>
#include <libanjuta/anjuta-children.h>
#include <libanjuta/anjuta-debug.h>
#define EXIT_POLL_TIMEOUT 100 /* 100 ms */

static GList *registered_child_processes = NULL;
static GList *registered_child_processes_cb = NULL;
static GList *registered_child_processes_cb_data = NULL;
static GList *pending_exit_notifies = NULL;
static gint exit_poll_timeout = 0;

typedef struct {
	int status;
	pid_t pid;
} ExitNotifyData;

static gboolean
on_notify_child_exit (gpointer user_data)
{
	gint idx;
	int (*callback) (int st, gpointer d);
	gpointer cb_data;
	ExitNotifyData *idle_data; /* = (ExitNotifyData*) user_data; */
	pid_t pid;
	int status;
	
	/* If no child is currently registered, remove timer */
	if (registered_child_processes == NULL)
	{
		DEBUG_PRINT ("No child process to monitor, destroying poll timer");
		exit_poll_timeout = 0;
		return FALSE;
	}
	
	/* If nothing pending, wait for next poll */
	if (pending_exit_notifies == NULL)
		return TRUE;

	idle_data = pending_exit_notifies->data;
	pending_exit_notifies = g_list_remove_link (pending_exit_notifies,
												pending_exit_notifies);
	pid = idle_data->pid;
	status = idle_data->status;
	g_free (idle_data);
	
	idx = g_list_index (registered_child_processes, (int *) pid);
	if (idx < 0)
		return TRUE; /* Continue polling */

	callback =
		g_list_nth_data (registered_child_processes_cb, idx);
	g_return_val_if_fail (callback != NULL, TRUE);
	
	cb_data = g_list_nth_data (registered_child_processes_cb_data, idx);
	if (callback)
		(*callback) (status, cb_data);
	anjuta_children_unregister (pid);

	/* If no child is currently registered, remove timer */
	if (registered_child_processes == NULL)
	{
		DEBUG_PRINT ("No child process to monitor, destroying poll timer");
		exit_poll_timeout = 0;
		return FALSE;
	}
	return TRUE; /* Otherwise continue polling */
}

static void
anjuta_child_terminated (int t)
{
	gint idx;
	int status;
	pid_t pid;
	sigset_t set, oldset;
	
	/* block other incoming SIGCHLD signals */
	sigemptyset(&set);
	sigaddset(&set, SIGCHLD);
	sigprocmask(SIG_BLOCK, &set, &oldset);

	while (1)
	{
		ExitNotifyData *idle_data;
		
		pid = waitpid (-1, &status, WNOHANG);
		
		if (pid < 1)
			return;
		idx = g_list_index (registered_child_processes, (int *) pid);
		if (idx < 0)
			continue;
		
		idle_data = g_new0 (ExitNotifyData, 1);
		idle_data->pid = pid;
		idle_data->status = status;
		
		/* Queue the exit notify so that it is handled in proper main loop
		 * context.
		 * FIXME: This is not thread safe. We should instead created a GSource
		 * class for this 'event'.
		 */
		pending_exit_notifies = g_list_append (pending_exit_notifies,
											   idle_data);
	}
	/* re-install the signal handler (some systems need this) */
	anjuta_children_recover ();

	/* and unblock it */
	sigemptyset(&set);
	sigaddset(&set, SIGCHLD);
	sigprocmask(SIG_UNBLOCK, &set, &oldset);	
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
	
	/* Start exit poll if not yet running */

	/* Callback is notified in idle to ensure it is process in
	 * main loop. Otherwise a deadlock could occur if the SIGCHLD was
	 * received after a lock in mainloop and the callback tries to
	 * use mainloop again (e.g. adding/removing a idle/timeout function.
	 */
	if (exit_poll_timeout == 0)
	{
		DEBUG_PRINT ("Setting up child process monitor poll timer");
		exit_poll_timeout = g_timeout_add (EXIT_POLL_TIMEOUT,
										   on_notify_child_exit, NULL);
	}
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
	if (exit_poll_timeout)
		g_source_remove (exit_poll_timeout);
	exit_poll_timeout = 0;
}
