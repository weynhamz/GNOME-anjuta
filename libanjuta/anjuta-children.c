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

static GList *registered_child_processes;
static GList *registered_child_processes_cb;
static GList *registered_child_processes_cb_data;

static void
anjuta_child_terminated (int t)
{
	int status;
	gint idx;
	pid_t pid;
	int (*callback) (int st, gpointer d);
	gpointer cb_data;
	
	pid = waitpid (-1, &status, WNOHANG);
	
	if (pid < 1)
		return;
	idx = g_list_index (registered_child_processes, (int *) pid);
	if (idx < 0)
		return;
	callback =
		g_list_nth_data (registered_child_processes_cb, idx);
	g_return_if_fail (callback != NULL);
	
	cb_data = g_list_nth_data (registered_child_processes_cb_data, idx);
	if (callback)
		(*callback) (status, cb_data);
	anjuta_children_unregister (pid);
	signal(SIGCHLD, anjuta_child_terminated);
}

void
anjuta_children_init()
{
	registered_child_processes = NULL;
	registered_child_processes_cb = NULL;
	registered_child_processes_cb_data = NULL;
	signal(SIGCHLD, anjuta_child_terminated);
}

void
anjuta_children_register (pid_t pid,
						  AnjutaChildTerminatedCallback ch_terminated,
						  gpointer data)
{
	if (pid < 1)
		return;
	registered_child_processes =
		g_list_append (registered_child_processes, (int *) pid);
	registered_child_processes_cb =
		g_list_append (registered_child_processes_cb, ch_terminated);
	registered_child_processes_cb_data =
		g_list_append (registered_child_processes_cb_data, data);
}

void
anjuta_children_unregister (pid_t pid)
{
	gint idx;
	GList *ptr;
	
	idx = g_list_index (registered_child_processes, (int *) pid);
	registered_child_processes =
		g_list_remove (registered_child_processes, (int *) pid);
	
	ptr = g_list_nth (registered_child_processes_cb, idx);
	// g_assert (ptr != NULL);
	registered_child_processes_cb =
		g_list_delete_link (registered_child_processes_cb, ptr);
	
	ptr = g_list_nth (registered_child_processes_cb_data, idx);
	// g_assert (data != NULL);
	registered_child_processes_cb_data =
		g_list_delete_link (registered_child_processes_cb_data, ptr);
}

void
anjuta_children_foreach (GFunc cb, gpointer data)
{
	g_list_foreach (registered_child_processes, cb, data);
}

void
anjuta_children_finalize()
{
	signal(SIGCHLD, SIG_DFL);
}
