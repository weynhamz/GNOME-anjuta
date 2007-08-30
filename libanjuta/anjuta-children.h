/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 *   anjuta-children.h
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

#ifndef _ANJUTA_CHILDREN_H_
#define _ANJUTA_CHILDREN_H_

#include <sys/types.h>
#include <sys/wait.h>
#include <glib.h>

G_BEGIN_DECLS

/**
 * AnjutaChildTerminatedCallback:
 * @exit_staus: Exit status of the child.
 * @user_data: User data
 *
 * Callback function definition for child termination.
 */
typedef  void (*AnjutaChildTerminatedCallback) (int exit_staus,
												gpointer user_data);

void anjuta_children_register (pid_t pid,
							   AnjutaChildTerminatedCallback ch_terminated,
							   gpointer data);
void anjuta_children_unregister (pid_t pid);
void anjuta_children_foreach (GFunc cb, gpointer data);
void anjuta_children_recover (void);
void anjuta_children_finalize(void);

G_END_DECLS

#endif
