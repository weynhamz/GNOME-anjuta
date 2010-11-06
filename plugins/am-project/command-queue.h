/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/* command-queue.h
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

#ifndef _COMMAND_QUEUE_H_
#define _COMMAND_QUEUE_H_

#include <glib.h>
#include <gio/gio.h>
#include <glib-object.h>

#include <libanjuta/anjuta-project.h>

G_BEGIN_DECLS

typedef struct _PmJob PmJob;

typedef struct _PmCommandQueue PmCommandQueue;

typedef gboolean (*PmCommandFunc) (PmJob *job);

typedef struct _PmCommandWork
{
	PmCommandFunc setup;
	PmCommandFunc worker;
	PmCommandFunc complete;
} PmCommandWork;

struct _PmJob
{
	PmCommandWork *work;
	AnjutaProjectNodeType type;
	GFile *file;
	gchar *name;
	AnjutaProjectNode *node;
	AnjutaProjectNode *sibling;
	AnjutaProjectNode *parent;
	GError *error;
	AnjutaProjectNode *proxy;
	AnjutaProjectProperty *property;
	GHashTable *map;
	gpointer user_data;
};

PmJob * pm_job_new (PmCommandWork *work, AnjutaProjectNode *node, AnjutaProjectNode *parent, AnjutaProjectNode *sibling, AnjutaProjectNodeType type, GFile *file, const gchar *name, gpointer user_data);
void pm_job_free (PmJob *job);

PmCommandQueue*	pm_command_queue_new (void);
void pm_command_queue_free (PmCommandQueue *queue);

void pm_command_queue_push (PmCommandQueue *queue, PmJob *job);
gboolean pm_command_queue_is_busy (PmCommandQueue *queue);


G_END_DECLS

#endif /* _COMMAND_QUEUE_H_ */
