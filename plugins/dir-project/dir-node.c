/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8 -*- */
/* dir-node.c
 *
 * Copyright (C) 2009  Sébastien Granjoux
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "dir-node.h"

#include <libanjuta/interfaces/ianjuta-project.h>
#include <libanjuta/anjuta-debug.h>


/* Root objects
 *---------------------------------------------------------------------------*/

/* GObjet implementation
 *---------------------------------------------------------------------------*/

G_DEFINE_TYPE (AnjutaDirRootNode, anjuta_dir_root_node, ANJUTA_TYPE_DIR_GROUP_NODE);

static void
anjuta_dir_root_node_init (AnjutaDirRootNode *node)
{
	node->base.type = ANJUTA_PROJECT_ROOT;
}

static void
anjuta_dir_root_node_class_init (AnjutaDirRootNodeClass *klass)
{
}



/* Group node
 *---------------------------------------------------------------------------*/

static void
on_file_changed (GFileMonitor *monitor,
			GFile *file,
			GFile *other_file,
			GFileMonitorEvent event_type,
			gpointer data)
{
	switch (event_type) {
		case G_FILE_MONITOR_EVENT_CHANGED:
		case G_FILE_MONITOR_EVENT_DELETED:
		case G_FILE_MONITOR_EVENT_CREATED:
			g_signal_emit_by_name (ANJUTA_DIR_GROUP_NODE (data)->emitter, "file-changed", data);
			break;
		default:
			break;
	}
}

gboolean
dir_group_node_set_file (AnjutaDirGroupNode *group, GFile *new_file, GObject *emitter)
{
	if (group->base.file != NULL)
	{
		g_object_unref (group->base.file);
		group->base.file = NULL;
	}
	if (group->monitor != NULL)
	{
		g_file_monitor_cancel (group->monitor);
		group->monitor = NULL;
	}

	if (new_file)
	{
		group->base.file = g_object_ref (new_file);

		/* Connect monitor if file exist */
		group->emitter = emitter;
		if (g_file_query_exists (new_file, NULL))
		{
			group->monitor = g_file_monitor_directory (new_file, G_FILE_MONITOR_NONE, NULL, NULL);

			g_signal_connect (G_OBJECT (group->monitor),
			                  "changed",
			                  G_CALLBACK (on_file_changed),
			                  group);
		}
	}

	return TRUE;
}


AnjutaProjectNode*
dir_group_node_new (GFile *file, GObject *emitter)
{
	AnjutaDirGroupNode *group = NULL;

	group = g_object_new (ANJUTA_TYPE_DIR_GROUP_NODE, NULL);
	dir_group_node_set_file (group, file, emitter);

	return ANJUTA_PROJECT_NODE (group);
}

/* GObjet implementation
 *---------------------------------------------------------------------------*/


G_DEFINE_TYPE (AnjutaDirGroupNode, anjuta_dir_group_node, ANJUTA_TYPE_PROJECT_NODE);

static void
anjuta_dir_group_node_init (AnjutaDirGroupNode *node)
{
	node->base.type = ANJUTA_PROJECT_GROUP;
	node->base.properties = NULL;
	node->base.properties_info = NULL;
	node->base.name = NULL;
	node->base.state = ANJUTA_PROJECT_CAN_ADD_GROUP |
		ANJUTA_PROJECT_CAN_ADD_SOURCE |
		ANJUTA_PROJECT_CAN_REMOVE |
		ANJUTA_PROJECT_REMOVE_FILE;
	node->monitor = NULL;
	node->emitter = NULL;
}

static void
anjuta_dir_group_node_finalize (GObject *object)
{
	AnjutaDirGroupNode *node = ANJUTA_DIR_GROUP_NODE (object);

	if (node->monitor != NULL) g_file_monitor_cancel (node->monitor);

	G_OBJECT_CLASS (anjuta_dir_group_node_parent_class)->finalize (object);
}

static void
anjuta_dir_group_node_class_init (AnjutaDirGroupNodeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = anjuta_dir_group_node_finalize;
}


/* Object node
 *---------------------------------------------------------------------------*/

struct _AnjutaDirObjectNode {
	AnjutaProjectNode base;
};


AnjutaProjectNode*
dir_object_node_new (GFile *file)
{
	AnjutaDirObjectNode *node = NULL;

	node = g_object_new (ANJUTA_TYPE_DIR_OBJECT_NODE, NULL);
	node->base.file = g_file_dup (file);

	return ANJUTA_PROJECT_NODE (node);
}

/* GObjet implementation
 *---------------------------------------------------------------------------*/

typedef struct _AnjutaDirObjectNodeClass AnjutaDirObjectNodeClass;

struct _AnjutaDirObjectNodeClass {
	AnjutaProjectNodeClass parent_class;
};

G_DEFINE_TYPE (AnjutaDirObjectNode, anjuta_dir_object_node, ANJUTA_TYPE_PROJECT_NODE);

static void
anjuta_dir_object_node_init (AnjutaDirObjectNode *node)
{
	node->base.type = ANJUTA_PROJECT_OBJECT;
	node->base.properties = NULL;
	node->base.properties_info = NULL;
	node->base.name = NULL;
	node->base.state = ANJUTA_PROJECT_CAN_REMOVE |
						ANJUTA_PROJECT_REMOVE_FILE;
}

static void
anjuta_dir_object_node_class_init (AnjutaDirObjectNodeClass *klass)
{
}



/* Source node
 *---------------------------------------------------------------------------*/

struct _AnjutaDirSourceNode {
	AnjutaProjectNode base;
};


AnjutaProjectNode*
dir_source_node_new (GFile *file)
{
	AnjutaDirSourceNode *source = NULL;

	source = g_object_new (ANJUTA_TYPE_DIR_SOURCE_NODE, NULL);
	source->base.file = g_file_dup (file);

	return ANJUTA_PROJECT_NODE (source);
}

/* GObjet implementation
 *---------------------------------------------------------------------------*/

typedef struct _AnjutaDirSourceNodeClass AnjutaDirSourceNodeClass;

struct _AnjutaDirSourceNodeClass {
	AnjutaProjectNodeClass parent_class;
};

G_DEFINE_TYPE (AnjutaDirSourceNode, anjuta_dir_source_node, ANJUTA_TYPE_PROJECT_NODE);

static void
anjuta_dir_source_node_init (AnjutaDirSourceNode *node)
{
	node->base.type = ANJUTA_PROJECT_SOURCE;
	node->base.properties = NULL;
	node->base.properties_info = NULL;
	node->base.name = NULL;
	node->base.state = ANJUTA_PROJECT_CAN_REMOVE |
		ANJUTA_PROJECT_REMOVE_FILE;
}

static void
anjuta_dir_source_node_class_init (AnjutaDirSourceNodeClass *klass)
{
}

