/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-project.c
 * Copyright (C) Sébastien Granjoux 2009 <seb.sfo@free.fr>
 * 
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "anjuta-project.h"

#include "anjuta-debug.h"

#include <string.h>

/**
 * SECTION:anjuta-project
 * @title: Anjuta project
 * @short_description: Anjuta project
 * @see_also: 
 * @stability: Unstable
 * @include: libanjuta/anjuta-project.h
 * 
 * A project in Anjuta is represented by a tree. There are three kinds of node.
 * 
 * A source node represents a source file. These are lead of the tree, a source
 * node cannot have children.
 *
 * A target node represents an object file defined explicitely.
 * There are different kinds of target: program, library...
 * A target have as children all source needed to build it.
 *
 * A group node is used to group several target or source, it can represent
 * a directory by example. The root node of the project is a group node
 * representing the project directory.
 *
 * All these nodes are base objects. They have derived in each project backend
 * to provide more specific information.
 */ 

/* convenient shortcut macro the get the AnjutaProjectNode from a GNode */
#define NODE_DATA(node)  ((node) != NULL ? (AnjutaProjectNodeData *)((node)->data) : NULL)
#define GROUP_DATA(node)  ((node) != NULL ? (AnjutaProjectGroupData *)((node)->data) : NULL)
#define TARGET_DATA(node)  ((node) != NULL ? (AnjutaProjectTargetData *)((node)->data) : NULL)
#define SOURCE_DATA(node)  ((node) != NULL ? (AnjutaProjectSourceData *)((node)->data) : NULL)


/* Node access functions
 *---------------------------------------------------------------------------*/

AnjutaProjectNode *
anjuta_project_node_parent(AnjutaProjectNode *node)
{
	return node->parent;
}

AnjutaProjectNode *
anjuta_project_node_first_child(AnjutaProjectNode *node)
{
	return g_node_first_child (node);
}

AnjutaProjectNode *
anjuta_project_node_last_child(AnjutaProjectNode *node)
{
	return g_node_last_child (node);
}

AnjutaProjectNode *
anjuta_project_node_next_sibling (AnjutaProjectNode *node)
{
	return g_node_next_sibling (node);
}

AnjutaProjectNode *
anjuta_project_node_prev_sibling (AnjutaProjectNode *node)
{
	return g_node_prev_sibling (node);
}

AnjutaProjectNode *anjuta_project_node_nth_child (AnjutaProjectNode *node, guint n)
{
	return g_node_nth_child (node, n);
}

typedef struct
{
	AnjutaProjectNodeFunc func;
	gpointer data;
} AnjutaProjectNodePacket;

static gboolean
anjuta_project_node_traverse_func (GNode *node, gpointer data)
{
	AnjutaProjectNodePacket *pack = (AnjutaProjectNodePacket *)data;
	
	pack->func ((AnjutaProjectNode *)node, pack->data);

	return FALSE;
}

void
anjuta_project_node_all_foreach (AnjutaProjectNode *node, AnjutaProjectNodeFunc func, gpointer data)
{
    AnjutaProjectNodePacket pack = {func, data};
	
	/* POST_ORDER is important when deleting the node, children has to be
	 * deleted first */
	g_node_traverse (node, G_POST_ORDER, G_TRAVERSE_ALL, -1, anjuta_project_node_traverse_func, &pack);
}

void
anjuta_project_node_children_foreach (AnjutaProjectNode *node, AnjutaProjectNodeFunc func, gpointer data)
{
	g_node_children_foreach (node, G_TRAVERSE_ALL, func, data);
}

AnjutaProjectNode *
anjuta_project_node_append (AnjutaProjectNode *parent, AnjutaProjectNode *node)
{
	return g_node_append (parent, node);
}

AnjutaProjectNode *
anjuta_project_node_insert_before (AnjutaProjectNode *parent, AnjutaProjectNode *sibling, AnjutaProjectNode *node)
{
	return g_node_insert_before (parent, sibling, node);
}

AnjutaProjectNode *
anjuta_project_node_insert_after (AnjutaProjectNode *parent, AnjutaProjectNode *sibling, AnjutaProjectNode *node)
{
	return g_node_insert_after (parent, sibling, node);
}

AnjutaProjectNode *
anjuta_project_node_prepend (AnjutaProjectNode *parent, AnjutaProjectNode *node)
{
	return g_node_prepend (parent, node);
}


AnjutaProjectNodeType
anjuta_project_node_get_type (const AnjutaProjectNode *node)
{
	return node == NULL ? ANJUTA_PROJECT_UNKNOWN : NODE_DATA (node)->type;
}

gchar *
anjuta_project_node_get_name (const AnjutaProjectNode *node)
{
	switch (NODE_DATA (node)->type)
	{
	case ANJUTA_PROJECT_GROUP:
		return g_file_get_basename (GROUP_DATA (node)->directory);
	case ANJUTA_PROJECT_TARGET:
		return g_strdup (TARGET_DATA (node)->name);
	case ANJUTA_PROJECT_SOURCE:
		return g_file_get_basename (SOURCE_DATA (node)->file);
	default:
		return NULL;
	}
}

gchar*
anjuta_project_node_get_uri (AnjutaProjectNode *node)
{
	AnjutaProjectGroup *parent;
	GFile *file;
	gchar *uri;
	
	switch (NODE_DATA (node)->type)
	{
	case ANJUTA_PROJECT_GROUP:
		uri = g_file_get_uri (GROUP_DATA (node)->directory);
		break;
	case ANJUTA_PROJECT_TARGET:
		parent = anjuta_project_node_parent (node);
		file = g_file_get_child (anjuta_project_group_get_directory (parent), anjuta_project_target_get_name (node));
		uri = g_file_get_uri (file);
		g_object_unref (file);
		break;
	case ANJUTA_PROJECT_SOURCE:
		uri = g_file_get_uri (SOURCE_DATA (node)->file);
		break;
	default:
		uri = NULL;
		break;
	}

	return uri;
}

/* Group access functions
 *---------------------------------------------------------------------------*/

GFile*
anjuta_project_group_get_directory (const AnjutaProjectGroup *group)
{
	return GROUP_DATA (group)->directory;
}

static gboolean
anjuta_project_group_compare (GNode *node, gpointer data)
{
	GFile *file = *(GFile **)data;

	if ((NODE_DATA(node)->type == ANJUTA_PROJECT_GROUP) && g_file_equal (GROUP_DATA(node)->directory, file))
	{
		*(AnjutaProjectNode **)data = node;

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

AnjutaProjectGroup *
anjuta_project_group_get_node_from_file (const AnjutaProjectGroup *root, GFile *directory)
{
	GFile *data;
	
	data = directory;
	g_node_traverse	(root, G_PRE_ORDER, G_TRAVERSE_ALL, -1, anjuta_project_group_compare, &data);

	return (data == directory) ? NULL : (AnjutaProjectNode *)data;
}

AnjutaProjectGroup *
anjuta_project_group_get_node_from_uri (const AnjutaProjectNode *root, const gchar *uri)
{
	GFile *file = g_file_new_for_uri (uri);
	AnjutaProjectGroup *node;

	node = anjuta_project_group_get_node_from_file (root, file);
	g_object_unref (file);

	return node;
}

static gboolean
anjuta_project_target_compare (GNode *node, gpointer data)
{
	const gchar *name = *(gchar **)data;

	if ((NODE_DATA(node)->type == ANJUTA_PROJECT_TARGET) && (strcmp (TARGET_DATA(node)->name, name) == 0))
	{
		*(AnjutaProjectNode **)data = node;

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

AnjutaProjectTarget *
anjuta_project_target_get_node_from_name (const AnjutaProjectGroup *parent, const gchar *name)
{
	const gchar *data;
	
	data = name;
	g_node_traverse	(parent, G_PRE_ORDER, G_TRAVERSE_ALL, 2, anjuta_project_target_compare, &data);

	return (data == name) ? NULL : (AnjutaProjectTarget *)data;
}

static gboolean
anjuta_project_source_compare (GNode *node, gpointer data)
{
	GFile *file = *(GFile **)data;

	if ((NODE_DATA(node)->type == ANJUTA_PROJECT_SOURCE) && g_file_equal (SOURCE_DATA(node)->file, file))
	{
		*(AnjutaProjectNode **)data = node;

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

AnjutaProjectSource *
anjuta_project_source_get_node_from_file (const AnjutaProjectNode *parent, GFile *file)
{
	GFile *data;
	
	data = file;
	g_node_traverse	(parent, G_PRE_ORDER, G_TRAVERSE_ALL, 2, anjuta_project_source_compare, &data);

	return (data == file) ? NULL : (AnjutaProjectNode *)data;
}

AnjutaProjectSource *
anjuta_project_source_get_node_from_uri (const AnjutaProjectNode *parent, const gchar *uri)
{
	GFile *file = g_file_new_for_uri (uri);
	AnjutaProjectSource *node;

	node = anjuta_project_source_get_node_from_file (parent, file);
	g_object_unref (file);

	return node;
}

/* Target access functions
 *---------------------------------------------------------------------------*/

const gchar *
anjuta_project_target_get_name (const AnjutaProjectTarget *target)
{
	return TARGET_DATA (target)->name;
}

AnjutaProjectTargetType
anjuta_project_target_get_type (const AnjutaProjectTarget *target)
{
	return TARGET_DATA (target)->type;
}

/* Source access functions
 *---------------------------------------------------------------------------*/

GFile*
anjuta_project_source_get_file (const AnjutaProjectSource *source)
{
	return SOURCE_DATA (source)->file;
}

/* Target type functions
 *---------------------------------------------------------------------------*/

const gchar *
anjuta_project_target_type_name (const AnjutaProjectTargetType type)
{
	return type->name;
}

const gchar *   
anjuta_project_target_type_mime (const AnjutaProjectTargetType type)
{
	return type->mime_type;
}

AnjutaProjectTargetClass
anjuta_project_target_type_class (const AnjutaProjectTargetType type)
{
	return type->base;
}
