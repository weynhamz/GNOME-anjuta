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

struct _AnjutaDirRootNode {
	AnjutaProjectNode base;
};

AnjutaProjectNode*
dir_root_node_new (GFile *file)
{
	AnjutaDirRootNode *root = NULL;

	root = g_object_new (ANJUTA_TYPE_DIR_ROOT_NODE, NULL);
	root->base.type = ANJUTA_PROJECT_ROOT;
	root->base.custom_properties = NULL;
	root->base.native_properties = NULL;
	root->base.file = g_file_dup (file);
	root->base.name = NULL;

	return ANJUTA_PROJECT_NODE (root);
}

/* GObjet implementation
 *---------------------------------------------------------------------------*/

typedef struct _AnjutaDirRootNodeClass AnjutaDirRootNodeClass;

struct _AnjutaDirRootNodeClass {
	AnjutaProjectNodeClass parent_class;
};

G_DEFINE_TYPE (AnjutaDirRootNode, anjuta_dir_root_node, ANJUTA_TYPE_PROJECT_NODE);

static void
anjuta_dir_root_node_init (AnjutaDirRootNode *node)
{
}

static void
anjuta_dir_root_node_class_init (AnjutaDirRootNodeClass *klass)
{
}



/* Group objects
 *---------------------------------------------------------------------------*/

struct _AnjutaDirGroupNode {
	AnjutaProjectNode base;
	GFileMonitor *monitor;
	GObject *emitter;
};

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

AnjutaProjectNode*
dir_group_node_new (GFile *file, GObject *emitter)
{
	AnjutaDirGroupNode *group = NULL;

	group = g_object_new (ANJUTA_TYPE_DIR_GROUP_NODE, NULL);
	group->base.type = ANJUTA_PROJECT_GROUP;
	group->base.native_properties = NULL;
	group->base.custom_properties = NULL;
	group->base.file = g_object_ref (file);
	group->base.name = NULL;
	group->base.state = ANJUTA_PROJECT_CAN_ADD_GROUP |
						ANJUTA_PROJECT_CAN_ADD_SOURCE |
						ANJUTA_PROJECT_CAN_REMOVE |
						ANJUTA_PROJECT_REMOVE_FILE;

	group->emitter = emitter;

	/* Connect monitor if file exist */
	if (g_file_query_exists (file, NULL))
	{
		group->monitor = g_file_monitor_directory (file, G_FILE_MONITOR_NONE, NULL, NULL);

		g_signal_connect (G_OBJECT (group->monitor),
							"changed",
							G_CALLBACK (on_file_changed),
							group);
	}

	return ANJUTA_PROJECT_NODE (group);
}

/* GObjet implementation
 *---------------------------------------------------------------------------*/


typedef struct _AnjutaDirGroupNodeClass AnjutaDirGroupNodeClass;

struct _AnjutaDirGroupNodeClass {
	AnjutaProjectNodeClass parent_class;
};

G_DEFINE_TYPE (AnjutaDirGroupNode, anjuta_dir_group_node, ANJUTA_TYPE_PROJECT_NODE);

static void
anjuta_dir_group_node_init (AnjutaDirGroupNode *node)
{
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


/* Source object
 *---------------------------------------------------------------------------*/

struct _AnjutaDirSourceNode {
	AnjutaProjectNode base;
};


AnjutaProjectNode*
dir_source_node_new (GFile *file)
{
	AnjutaDirSourceNode *source = NULL;

	source = g_object_new (ANJUTA_TYPE_DIR_SOURCE_NODE, NULL);
	source->base.type = ANJUTA_PROJECT_SOURCE;
	source->base.native_properties = NULL;
	source->base.custom_properties = NULL;
	source->base.name = NULL;
	source->base.file = g_file_dup (file);
	source->base.state = ANJUTA_PROJECT_CAN_REMOVE |
						ANJUTA_PROJECT_REMOVE_FILE;

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
}

static void
anjuta_dir_source_node_class_init (AnjutaDirSourceNodeClass *klass)
{
}

