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
#include "anjuta-marshal.h"

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
#define PROXY_DATA(node)  ((node) != NULL ? (AnjutaProjectProxyData *)((node)->data) : NULL)

/* Properties functions
 *---------------------------------------------------------------------------*/

typedef struct {
	AnjutaProjectNodeData base;
	AnjutaProjectNode *node;
	guint reference;
} AnjutaProjectProxyData;

/* Properties functions
 *---------------------------------------------------------------------------*/

AnjutaProjectProperty *
anjuta_project_property_next (AnjutaProjectProperty *list)
{
	return g_list_next (list);
}

AnjutaProjectPropertyInfo *
anjuta_project_property_get_info (AnjutaProjectProperty *property)
{
	return (AnjutaProjectPropertyInfo *)property->data;
}

AnjutaProjectPropertyInfo *
anjuta_project_property_lookup (AnjutaProjectProperty *list, AnjutaProjectProperty *prop)
{
	AnjutaProjectPropertyInfo *info;
	
	for (; list != NULL; list = g_list_next (list))
	{
		info = (AnjutaProjectPropertyInfo *)list->data;
		
		if (info->override == NULL)
		{
			info = NULL;
			break;
		}
		else if (info->override == prop)
		{
			break;
		}
		info = NULL;
	}

	return info;
}

AnjutaProjectProperty *
anjuta_project_property_override (AnjutaProjectProperty *list, AnjutaProjectProperty *prop)
{
	AnjutaProjectProperty *item;
	
	for (item = list; item != NULL; item = g_list_next (item))
	{
		AnjutaProjectPropertyInfo *info;
	
		info = (AnjutaProjectPropertyInfo *)item->data;
		
		if (info->override == NULL)
		{
			item = NULL;
			break;
		}
		else if (info->override == prop)
		{
			break;
		}
	}

	return item;
}

AnjutaProjectProperty *
anjuta_project_property_next_item (AnjutaProjectProperty *item)
{
	AnjutaProjectProperty *prop = ((AnjutaProjectPropertyInfo *)item->data)->override;

	for (item = g_list_next (item); item != NULL; item = g_list_next (item))
	{
		AnjutaProjectPropertyInfo *info;
	
		info = (AnjutaProjectPropertyInfo *)item->data;
		
		if (info->override == NULL)
		{
			item = NULL;
			break;
		}
		else if (info->override == prop)
		{
			break;
		}
	}

	return item;
}

AnjutaProjectProperty *
anjuta_project_property_insert (AnjutaProjectProperty *list, AnjutaProjectProperty *prop, AnjutaProjectPropertyInfo *info)
{
	GList *next;
	
	if (info->name == NULL) info->name = ((AnjutaProjectPropertyInfo *)prop->data)->name;
	info->type = ((AnjutaProjectPropertyInfo *)prop->data)->type;
	info->override = prop;

	next = ((AnjutaProjectPropertyInfo *)list->data)->override;
	if (next != NULL)
	{
		next = list;
	}
	list = g_list_prepend (next, info);
	
	return list;
}

AnjutaProjectProperty *
anjuta_project_property_remove (AnjutaProjectProperty *list, AnjutaProjectProperty *prop)
{
	AnjutaProjectPropertyInfo *info;
	GList *link;
	
	for (link = list; link != NULL; link = g_list_next (link))
	{
		info = (AnjutaProjectPropertyInfo *)link->data;
		if (info->override == NULL)
		{
			break;
		}
		else if ((info == prop->data) || (info->override == prop))
		{
			list = g_list_delete_link (list, link);
			if (list == NULL) list = info->override;
			break;
		}
	}
	
	return list;
}

void
anjuta_project_property_foreach (AnjutaProjectProperty *list, GFunc func, gpointer user_data)
{
	g_list_foreach (list, func, user_data);
}


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
	/* node->parent is filled by the new function */
	if (node->parent != NULL) node->parent = NULL;
	return g_node_insert_before (parent, sibling, node);
}

AnjutaProjectNode *
anjuta_project_node_insert_after (AnjutaProjectNode *parent, AnjutaProjectNode *sibling, AnjutaProjectNode *node)
{
	/* node->parent is filled by the new function */
	if (node->parent != NULL) node->parent = NULL;
	return g_node_insert_after (parent, sibling, node);
}

AnjutaProjectNode *
anjuta_project_node_remove (AnjutaProjectNode *node)
{
	if (node->parent != NULL)
	{
		g_node_unlink (node);
	}
	
	return node;
}

AnjutaProjectNode *
anjuta_project_node_replace (AnjutaProjectNode *node, AnjutaProjectNode *replacement)
{
	if (node->parent != NULL)
	{
		g_node_insert_after (node->parent, node, replacement);
		g_node_unlink (node);
	}
	
	return replacement;
}

AnjutaProjectNode *
anjuta_project_node_exchange (AnjutaProjectNode *node, AnjutaProjectNode *replacement)
{
	GNode *marker = g_node_new (NULL);
	GNode *child;
	GNode *sibling;
	GNode *next;
	
	if (node->parent != NULL)
	{
		g_node_insert_after (node->parent, node, marker);
		g_node_unlink (node);
	}
	if (replacement->parent != NULL)
	{
		g_node_insert_after (replacement->parent, replacement, node);
		g_node_unlink (replacement);
	}
	if (marker->parent != NULL)
	{
		g_node_insert_after (marker->parent, marker, replacement);
		g_node_unlink (marker);
	}
	g_node_destroy (marker);

	/* Move all children from node to replacement */
	sibling = NULL;
	for (child = g_node_first_child (node); child != NULL; child = next)
	{
		next = g_node_next_sibling (child);
		g_node_unlink (child);
		sibling = g_node_insert_after (replacement, sibling, child);
		child = next;
	}
	
	/* Move all children from replacement to node */
	child = g_node_next_sibling (sibling);
	sibling = NULL;
	for (; child != NULL; child = next)
	{
		next = g_node_next_sibling (child);
		g_node_unlink (child);
		sibling = g_node_insert_after (node, sibling, child);
		child = next;
	}

	return replacement;
}

AnjutaProjectNode *
anjuta_project_node_grab_children (AnjutaProjectNode *parent, AnjutaProjectNode *node)
{
	AnjutaProjectNode *child;
	AnjutaProjectNode *sibling;
	
	sibling = g_node_last_child (parent);
	
	for (child = g_node_first_child (node); child != NULL;)
	{
		AnjutaProjectNode *remove;

		remove = child;
		child = g_node_next_sibling (child);
		g_node_unlink (remove);
		sibling = g_node_insert_after (parent, sibling, remove);
	}
	
	return parent;
}


AnjutaProjectNode *
anjuta_project_node_prepend (AnjutaProjectNode *parent, AnjutaProjectNode *node)
{
	return g_node_prepend (parent, node);
}

gboolean
anjuta_project_node_set_state (AnjutaProjectNode *node, AnjutaProjectNodeState state)
{
	if (node == NULL) return FALSE;
	NODE_DATA (node)->state |= state;
	return TRUE;
}

gboolean
anjuta_project_node_clear_state (AnjutaProjectNode *node, AnjutaProjectNodeState state)
{
	if (node == NULL) return FALSE;
	NODE_DATA (node)->state &= ~state;
	return TRUE;
}

AnjutaProjectNodeState
anjuta_project_node_get_state (const AnjutaProjectNode *node)
{
	return node == NULL ? ANJUTA_PROJECT_OK : (NODE_DATA (node)->state);
}

AnjutaProjectNodeType
anjuta_project_node_get_type (const AnjutaProjectNode *node)
{
	return node == NULL ? ANJUTA_PROJECT_UNKNOWN : (NODE_DATA (node)->type & ANJUTA_PROJECT_TYPE_MASK);
}

AnjutaProjectNodeType
anjuta_project_node_get_full_type (const AnjutaProjectNode *node)
{
	return node == NULL ? ANJUTA_PROJECT_UNKNOWN : NODE_DATA (node)->type;
}

gchar *
anjuta_project_node_get_name (const AnjutaProjectNode *node)
{
	switch (NODE_DATA (node)->type & ANJUTA_PROJECT_TYPE_MASK)
	{
	case ANJUTA_PROJECT_GROUP:
		return g_file_get_basename (NODE_DATA (node)->file);
	case ANJUTA_PROJECT_SOURCE:
		return g_file_get_basename (NODE_DATA (node)->file);
	case ANJUTA_PROJECT_TARGET:
	case ANJUTA_PROJECT_MODULE:
	case ANJUTA_PROJECT_PACKAGE:
		return g_strdup (NODE_DATA (node)->name);
	default:
		return NULL;
	}
}

gchar*
anjuta_project_node_get_uri (AnjutaProjectNode *node)
{
	AnjutaProjectNode *parent;
	GFile *file;
	gchar *uri;
	
	switch (NODE_DATA (node)->type & ANJUTA_PROJECT_TYPE_MASK)
	{
	case ANJUTA_PROJECT_GROUP:
		uri = g_file_get_uri (NODE_DATA (node)->file);
		break;
	case ANJUTA_PROJECT_TARGET:
		parent = anjuta_project_node_parent (node);
		file = g_file_get_child (anjuta_project_group_get_directory (parent), anjuta_project_target_get_name (node));
		uri = g_file_get_uri (file);
		g_object_unref (file);
		break;
	case ANJUTA_PROJECT_SOURCE:
		uri = g_file_get_uri (NODE_DATA (node)->file);
		break;
	default:
		uri = NULL;
		break;
	}

	return uri;
}

GFile*
anjuta_project_node_get_file (AnjutaProjectNode *node)
{
	AnjutaProjectNodeData *data = NODE_DATA (node);

	if ((data->file == NULL) && (data->name != NULL))
	{
		/* Try to create a file */
		AnjutaProjectNode *parent;

		parent = anjuta_project_node_parent (node);
		if ((parent != NULL) && (NODE_DATA (parent)->file != NULL))
		{
			data->file = g_file_get_child (NODE_DATA (parent)->file, data->name);
		}
	}

	return data->file;
}

AnjutaProjectProperty *
anjuta_project_node_first_property (AnjutaProjectNode *node)
{
	GList *first;

	/* Get properties list */
	first = g_list_first (NODE_DATA (node)->properties);
	if (first != NULL)
	{
		AnjutaProjectPropertyInfo *info = (AnjutaProjectPropertyInfo *)first->data;

		if (info->override == NULL)
		{
			first = NULL;
		}
	}

	return first;
}

AnjutaProjectProperty *
anjuta_project_node_first_valid_property (AnjutaProjectNode *node)
{
	GList *first;

	/* Get properties list */
	first = g_list_first (NODE_DATA (node)->properties);
	if (first != NULL)
	{
		AnjutaProjectPropertyInfo *info = (AnjutaProjectPropertyInfo *)first->data;

		if (info->override != NULL)
		{
			first = g_list_first (info->override);
		}
	}

	return first;
}

AnjutaProjectProperty *
anjuta_project_node_get_property (AnjutaProjectNode *node, AnjutaProjectProperty *property)
{
	GList *item;
	AnjutaProjectPropertyInfo *info;
	
	/* Get main property */
	info = (AnjutaProjectPropertyInfo *)property->data;
	if (info->override != NULL) property = info->override;
	
	/* Get properties list */
	item = g_list_first (NODE_DATA (node)->properties);

	for (; item != NULL; item = g_list_next (item))
	{
		info = (AnjutaProjectPropertyInfo *)item->data;

		if (info->override == NULL)
		{
			item = NULL;
			break;
		}
		
		if (info->override == property)
		{
			break;
		}
	}

	return item;
}

AnjutaProjectProperty *
anjuta_project_node_insert_property (AnjutaProjectNode *node, AnjutaProjectProperty *frame, AnjutaProjectProperty *property)
{
	GList **list;
	GList *next;
	AnjutaProjectPropertyInfo *info;

	/* Fill missing information */
	info = (AnjutaProjectPropertyInfo *)property->data;
	if (info->name == NULL) info->name = ((AnjutaProjectPropertyInfo *)frame->data)->name;
	info->type = ((AnjutaProjectPropertyInfo *)frame->data)->type;
	info->override = frame;

	/* Get properties list */
	list = &(NODE_DATA (node)->properties);

	next = ((AnjutaProjectPropertyInfo *)(*list)->data)->override;
	if (next != NULL)
	{
		next = *list;
	}
	*list = g_list_concat (next, property);
	
	return property;
}

AnjutaProjectProperty *
anjuta_project_node_remove_property (AnjutaProjectNode *node, AnjutaProjectProperty *prop)
{
	AnjutaProjectPropertyInfo *info;

	info = (AnjutaProjectPropertyInfo *)prop->data;

	if (info->override != NULL)
	{
		GList *list;
		
		list = NODE_DATA (node)->properties;

		list = g_list_remove_link (list, prop);
		if (list == NULL)
		{
			list = g_list_first (info->override);
		}
		NODE_DATA (node)->properties = list;
	}
	else
	{
		prop = NULL;
	}
	
	return prop;
}


/*const gchar *
anjuta_project_node_get_property_value (AnjutaProjectNode *node, AnjutaProjectProperty prop)
{
	GList *item;

	for (item = g_list_first (NODE_DATA (node)->properties); item != NULL; item = g_list_next (item))
	{
		AnjutaProjectPropertyInfo *info = (AnjutaProjectPropertyInfo *)item->data;

		if ((info == prop) || ((info->override != NULL) && (info->override->data == prop)))
		{
			return info->value;
		}
	}
	
	return NULL;
}*/

/* Group access functions
 *---------------------------------------------------------------------------*/

GFile*
anjuta_project_group_get_directory (const AnjutaProjectNode *group)
{
	return NODE_DATA (group)->file;
}

static gboolean
anjuta_project_group_compare (GNode *node, gpointer data)
{
	GFile *file = *(GFile **)data;

	if (((NODE_DATA (node)->type & ANJUTA_PROJECT_TYPE_MASK) == ANJUTA_PROJECT_GROUP) && g_file_equal (NODE_DATA(node)->file, file))
	{
		*(AnjutaProjectNode **)data = node;

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

AnjutaProjectNode *
anjuta_project_group_get_node_from_file (const AnjutaProjectNode *root, GFile *directory)
{
	GFile *data;
	
	data = directory;
	g_node_traverse	((GNode *)root, G_PRE_ORDER, G_TRAVERSE_ALL, -1, anjuta_project_group_compare, &data);

	return (data == directory) ? NULL : (AnjutaProjectNode *)data;
}

AnjutaProjectNode *
anjuta_project_group_get_node_from_uri (const AnjutaProjectNode *root, const gchar *uri)
{
	GFile *file = g_file_new_for_uri (uri);
	AnjutaProjectNode *node;

	node = anjuta_project_group_get_node_from_file (root, file);
	g_object_unref (file);

	return node;
}

static gboolean
anjuta_project_target_compare (GNode *node, gpointer data)
{
	const gchar *name = *(gchar **)data;

	if (((NODE_DATA (node)->type & ANJUTA_PROJECT_TYPE_MASK) == ANJUTA_PROJECT_TARGET) && (strcmp (NODE_DATA(node)->name, name) == 0))
	{
		*(AnjutaProjectNode **)data = node;

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

AnjutaProjectNode *
anjuta_project_target_get_node_from_name (const AnjutaProjectNode *parent, const gchar *name)
{
	const gchar *data;
	
	data = name;
	g_node_traverse	((GNode *)parent, G_PRE_ORDER, G_TRAVERSE_ALL, 2, anjuta_project_target_compare, &data);

	return (data == name) ? NULL : (AnjutaProjectNode *)data;
}

static gboolean
anjuta_project_source_compare (GNode *node, gpointer data)
{
	GFile *file = *(GFile **)data;

	if (((NODE_DATA (node)->type & ANJUTA_PROJECT_TYPE_MASK) == ANJUTA_PROJECT_SOURCE) && g_file_equal (NODE_DATA(node)->file, file))
	{
		*(AnjutaProjectNode **)data = node;

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

AnjutaProjectNode *
anjuta_project_source_get_node_from_file (const AnjutaProjectNode *parent, GFile *file)
{
	GFile *data;
	
	data = file;
	g_node_traverse	((GNode *)parent, G_PRE_ORDER, G_TRAVERSE_ALL, 2, anjuta_project_source_compare, &data);

	return (data == file) ? NULL : (AnjutaProjectNode *)data;
}

AnjutaProjectNode *
anjuta_project_source_get_node_from_uri (const AnjutaProjectNode *parent, const gchar *uri)
{
	GFile *file = g_file_new_for_uri (uri);
	AnjutaProjectNode *node;

	node = anjuta_project_source_get_node_from_file (parent, file);
	g_object_unref (file);

	return node;
}

/* Target access functions
 *---------------------------------------------------------------------------*/

const gchar *
anjuta_project_target_get_name (const AnjutaProjectNode *target)
{
	return NODE_DATA (target)->name;
}

/* Source access functions
 *---------------------------------------------------------------------------*/

GFile*
anjuta_project_source_get_file (const AnjutaProjectNode *source)
{
	return NODE_DATA (source)->file;
}

/* Node information functions
 *---------------------------------------------------------------------------*/

const gchar *
anjuta_project_node_info_name (const AnjutaProjectNodeInfo *info)
{
	return info->name;
}

const gchar *   
anjuta_project_node_info_mime (const AnjutaProjectNodeInfo *info)
{
	return info->mime_type;
}

AnjutaProjectNodeType
anjuta_project_node_info_type (const AnjutaProjectNodeInfo *info)
{
	return info->type;
}

/* Proxy node functions
 *---------------------------------------------------------------------------*/

AnjutaProjectNode *
anjuta_project_proxy_new (AnjutaProjectNode *node)
{
	AnjutaProjectProxyData *proxy;
	AnjutaProjectNodeData *data;
	
	/* If the node is already a proxy get original node */
	node = anjuta_project_proxy_get_node (node);
	data = NODE_DATA (node);
	
	/* Create proxy node */
	proxy = g_slice_new0(AnjutaProjectProxyData);
	proxy->reference = 1;
	proxy->node = node;
	proxy->base.type = data->type | ANJUTA_PROJECT_PROXY;
	proxy->base.file = data->file != NULL ? g_object_ref (data->file) : NULL;
	proxy->base.name = g_strdup (data->name);
		
	/* Shallow copy of all properties */
	if ((data->properties == NULL) || (((AnjutaProjectPropertyInfo *)data->properties->data)->override == NULL))
	{
		proxy->base.properties = data->properties;
	}
	else
	{
		GList *item;
		proxy->base.properties = g_list_copy (data->properties);
		for (item = g_list_first (proxy->base.properties); item != NULL; item = g_list_next (item))
		{
			AnjutaProjectPropertyInfo *info = (AnjutaProjectPropertyInfo *)item->data;
			AnjutaProjectPropertyInfo *new_info;

			new_info = g_slice_new0(AnjutaProjectPropertyInfo);
			new_info->name = g_strdup (info->name);
			new_info->type = info->type;
			new_info->value = g_strdup (info->value);
			new_info->override = info->override;
			item->data = new_info;
		}
	}
		
	node = g_node_new (proxy);

	return node;
}

AnjutaProjectNode *
anjuta_project_proxy_unref (AnjutaProjectNode *node)
{
	if (NODE_DATA (node)->type & ANJUTA_PROJECT_PROXY)
	{
		PROXY_DATA (node)->reference--;

		if (PROXY_DATA (node)->reference == 0)
		{
			AnjutaProjectProxyData *proxy = PROXY_DATA (node);

			if (proxy->base.file) g_object_unref (proxy->base.file);
			g_free (proxy->base.name);
			if ((proxy->base.properties != NULL) && (((AnjutaProjectPropertyInfo *)proxy->base.properties->data)->override != NULL))
			{
				GList *item;
				
				for (item = g_list_first (proxy->base.properties); item != NULL; item = g_list_next (item))
				{
					AnjutaProjectPropertyInfo *info = (AnjutaProjectPropertyInfo *)item->data;

					g_free (info->name);
					g_free (info->value);
					g_slice_free (AnjutaProjectPropertyInfo, info);
				}
				g_list_free (proxy->base.properties);
			}
			g_slice_free (AnjutaProjectProxyData, proxy);
			g_node_destroy (node);
			node = NULL;
		}
	}

	return node;
}

AnjutaProjectNode *
anjuta_project_proxy_exchange_data (AnjutaProjectNode *proxy, AnjutaProjectNode *node)
{
	AnjutaProjectNodeData *data;
	
	data = proxy->data;
	proxy->data = node->data;
	node->data = data;
	
	return proxy;
}

AnjutaProjectNode *
anjuta_project_proxy_get_node (AnjutaProjectNode *node)
{
	g_return_val_if_fail (node != NULL, FALSE);

	if (NODE_DATA (node)->type & ANJUTA_PROJECT_PROXY)
	{
		return (PROXY_DATA (node)->node);
	}
	else
	{
		return node;
	}
}

gboolean
anjuta_project_node_is_proxy (AnjutaProjectNode *node)
{
	g_return_val_if_fail (node != NULL, FALSE);

	return NODE_DATA (node)->type & ANJUTA_PROJECT_PROXY ? TRUE : FALSE;
}

typedef struct {
	AnjutaProjectNodeData base;
	gpointer data;
	guint reference;
} AnjutaProjectIntrospectionData;


/**
 * anjuta_project_introspection_node_new:
 * 
 * @type: node type
 * @file: (allow-none): file
 * @name: file name
 * @user_data:
 * 
 * Returns: (transfer none): A list of #GFile corresponding to
 * each target of the requested type or %NULL if none exists. Free the returned list
 * with g_list_free() and the files with g_object_unref().
 */
AnjutaProjectNode *
anjuta_project_introspection_node_new (AnjutaProjectNodeType type, GFile *file, const gchar *name, gpointer user_data)
{
	AnjutaProjectIntrospectionData *data;
	AnjutaProjectNode *node;

	data = g_slice_new0(AnjutaProjectIntrospectionData); 
	data->base.type = type;
	data->base.properties = NULL;
	data->base.file = file != NULL ? g_object_ref (file) : NULL;
	data->base.name = name != NULL ? g_strdup (name) : NULL;
	data->data = user_data;

	node = g_node_new (data);
	//g_message ("New node %p data %p", node, data);

	return node;
}

/**
 * anjuta_project_introspection_node_new0:
 * 
 * Blah Blah
 * 
 * Returns: (transfer none): A list of #GFile corresponding to
 * each target of the requested type or %NULL if none exists. Free the returned list
 * with g_list_free() and the files with g_object_unref().
 */
AnjutaProjectNode *
anjuta_project_introspection_node_new0 (void)
{
	AnjutaProjectIntrospectionData *data;

	data = g_slice_new0(AnjutaProjectIntrospectionData); 
	data->base.type = 0;
	data->base.properties = NULL;
	data->base.file = NULL;
	data->base.state = 0;

    return g_node_new (data);
	
}

void
anjuta_project_introspection_node_free (AnjutaProjectNode *node)
{
    AnjutaProjectIntrospectionData *data = (AnjutaProjectIntrospectionData *)node->data;
	
	//g_message ("Free node %p data %p", node, data);
	if (data->base.file) g_object_unref (data->base.file);
	if (data->base.name) g_free (data->base.name);
	
    g_slice_free (AnjutaProjectIntrospectionData, data);
	
	g_node_destroy (node);
}

/**
 * anjuta_project_introspection_node_get_user_data
 * 
 * @node: node type
 * 
 * Returns: A list of #GFile corresponding to
 * each target of the requested type or %NULL if none exists. Free the returned list
 * with g_list_free() and the files with g_object_unref().
 */
gpointer
anjuta_project_introspection_node_get_user_data (AnjutaProjectNode *node)
{
    AnjutaProjectIntrospectionData *data = (AnjutaProjectIntrospectionData *)node->data;

	return data->data;
}

/* Implement GObject
 *---------------------------------------------------------------------------*/

enum
{
	UPDATED,
	LOADED,
	LAST_SIGNAL
};

enum {
	PROP_NONE,
	PROP_NAME,
	PROP_FILE,
	PROP_STATE,
	PROP_TYPE,
	PROP_DATA
};


static unsigned int signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (AnjutaProjectGObjectNode, anjuta_project_gobject_node, G_TYPE_OBJECT);

static void
anjuta_project_gobject_node_init (AnjutaProjectGObjectNode *node)
{
	node->next = NULL;
	node->prev = NULL;
	node->parent = NULL;
	node->children = NULL;
	node->base.type = 0;
	node->base.properties = NULL;
	node->base.file = NULL;
	node->base.name = NULL;
	node->data = NULL;	
}

static void
anjuta_project_gobject_node_dispose (GObject *object)
{
	AnjutaProjectGObjectNode *node = ANJUTA_PROJECT_GOBJECT_NODE(object);

	if (node->base.file != NULL) g_object_unref (node->base.file);
	node->base.file = NULL;
	
	G_OBJECT_CLASS (anjuta_project_gobject_node_parent_class)->dispose (object);
}

static void
anjuta_project_gobject_node_finalize (GObject *object)
{
	AnjutaProjectGObjectNode *node = ANJUTA_PROJECT_GOBJECT_NODE(object);

	if (node->base.name != NULL) g_free (node->base.name);

	G_OBJECT_CLASS (anjuta_project_gobject_node_parent_class)->finalize (object);
}

static void 
anjuta_project_gobject_node_get_property (GObject    *object,
	      guint       prop_id,
	      GValue     *value,
	      GParamSpec *pspec)
{
	AnjutaProjectGObjectNode *node = ANJUTA_PROJECT_GOBJECT_NODE(object);
        
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void 
anjuta_project_gobject_node_set_property (GObject      *object,
	      guint         prop_id,
	      const GValue *value,
	      GParamSpec   *pspec)
{
	AnjutaProjectGObjectNode *node = ANJUTA_PROJECT_GOBJECT_NODE(object);
       G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
anjuta_project_gobject_node_class_init (AnjutaProjectGObjectNodeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	
	object_class->finalize = anjuta_project_gobject_node_finalize;
	object_class->dispose = anjuta_project_gobject_node_dispose;
	object_class->get_property = anjuta_project_gobject_node_get_property;
	object_class->set_property = anjuta_project_gobject_node_set_property;

 	/*Change both signal to use marshal_VOID__POINTER_BOXED
	adding a AnjutaProjectNode pointer corresponding to the
	 loaded node => done
	 Such marshal doesn't exist as glib marshal, so look in the
	 symbol db plugin how to add new marshal => done
	 ToDo :
	 This new argument can be used in the plugin object in
	 order to add corresponding shortcut when the project
	 is loaded and a new node is loaded.
	 The plugin should probably get the GFile from the
	 AnjutaProjectNode object and then use a function
	 in project-view.c to create the corresponding shortcut*/
	
	signals[UPDATED] = g_signal_new ("updated",
	    G_OBJECT_CLASS_TYPE (object_class),
	    G_SIGNAL_RUN_LAST,
	    G_STRUCT_OFFSET (AnjutaProjectGObjectNodeClass, updated),
	    NULL, NULL,
        anjuta_cclosure_marshal_VOID__STRING_BOXED,
	    G_TYPE_NONE,
	    2,
	    G_TYPE_POINTER,
	    G_TYPE_ERROR);
	
	signals[LOADED] = g_signal_new ("loaded",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (AnjutaProjectGObjectNodeClass, loaded),
		NULL, NULL,
        anjuta_cclosure_marshal_VOID__STRING_BOXED,
		G_TYPE_NONE,
		2,
	    G_TYPE_POINTER,
		G_TYPE_ERROR);

	g_object_class_install_property 
                (G_OBJECT_CLASS (klass), PROP_TYPE,
                 g_param_spec_pointer ("type", 
                                       "Type",
                                       "Node type",
                                       G_PARAM_READWRITE));

	g_object_class_install_property 
                (G_OBJECT_CLASS (klass), PROP_STATE,
                 g_param_spec_pointer ("state", 
                                       "Stroject",
                                       "GbfProject Object",
                                       G_PARAM_READWRITE));

	g_object_class_install_property 
                (G_OBJECT_CLASS (klass), PROP_DATA,
                 g_param_spec_pointer ("project", 
                                       "Project",
                                       "GbfProject Object",
                                       G_PARAM_READWRITE));

	g_object_class_install_property 
                (G_OBJECT_CLASS (klass), PROP_NAME,
                 g_param_spec_string ("name", 
                                      "Name",
                                      "GbfProject Object",
				       "",
                                       G_PARAM_READWRITE));

	g_object_class_install_property 
                (G_OBJECT_CLASS (klass), PROP_FILE,
                 g_param_spec_object ("file", 
                                       "PDroject",
                                       "GbfProject Object",
				       G_TYPE_FILE,
                                       G_PARAM_READWRITE));

}

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

/**
 * anjuta_project_gobject_node_new:
 * 
 * @type: node type
 * @file: (allow-none): file
 * @name: file name
 * @user_data:
 * 
 * Returns: A list of #GFile corresponding to
 * each target of the requested type or %NULL if none exists. Free the returned list
 * with g_list_free() and the files with g_object_unref().
 */
AnjutaProjectGObjectNode*
anjuta_project_gobject_node_new (AnjutaProjectNodeType type, GFile *file, const gchar *name, gpointer user_data)
{
	AnjutaProjectGObjectNode* node;

	node = g_object_new (ANJUTA_TYPE_PROJECT_GOBJECT_NODE, NULL);
	node->base.type = type;
	if (file != NULL) node->base.file = g_object_ref (file);
	if (name != NULL) node->base.name = g_strdup (name);
	node->data = user_data;
	//g_message ("New gnode %p", node);

	return node;
}

void
anjuta_project_gobject_node_free (AnjutaProjectGObjectNode* node)
{
//	g_message ("Free gnode %p", node);
	g_object_unref (node);
}

// (type ANJUTA_TYPE_PROJECT_BOXED_NODE):

/**
 * anjuta_project_boxed_node_new:
 * 
 * @type: node type
 * @file: (allow-none): file
 * @name: file name
 * @user_data:
 * 
 * Returns: (type Anjuta.TYPE_PROJECT_BOXED_NODE):  A list of #GFile corresponding to
 * each target of the requested type or %NULL if none exists. Free the returned list
 * with g_list_free() and the files with g_object_unref().
 */
AnjutaProjectNode *
anjuta_project_boxed_node_new (AnjutaProjectNodeType type, GFile *file, const gchar *name, gpointer user_data)
{
	AnjutaProjectIntrospectionData *data;
	AnjutaProjectNode *node;

	data = g_slice_new0(AnjutaProjectIntrospectionData); 
	data->base.type = type;
	data->base.properties = NULL;
	data->base.file = file != NULL ? g_object_ref (file) : NULL;
	data->base.name = name != NULL ? g_strdup (name) : NULL;
	data->data = user_data;
	data->reference = 1;

	node = g_node_new (data);
	//g_message ("New node %p data %p", node, data);

	return node;
}

static gpointer
anjuta_project_boxed_node_copy (gpointer boxed)
{
	AnjutaProjectNode *node = (AnjutaProjectNode *)boxed;
	AnjutaProjectIntrospectionData *data = (AnjutaProjectIntrospectionData *)node->data;

	data->reference++;

	return boxed;
}

static void
anjuta_project_boxed_node_free (gpointer boxed)
{
	AnjutaProjectNode *node = (AnjutaProjectNode *)boxed;
	AnjutaProjectIntrospectionData *data = (AnjutaProjectIntrospectionData *)node->data;

	data->reference--;
	if (data->reference == 0)
	{
		anjuta_project_introspection_node_free (node);
	}
}

void
anjuta_project_boxed_node_register (void)
{
 	g_boxed_type_register_static ("ANJUTA_TYPE_PROJECT_BOXED_NODE", anjuta_project_boxed_node_copy, anjuta_project_boxed_node_free);
}

GType
anjuta_project_boxed_node_get_type (void)
{
  static GType type_id = 0;

  if (!type_id)
 	type_id = g_boxed_type_register_static ("ANJUTA_TYPE_PROJECT_BOXED_NODE", anjuta_project_boxed_node_copy, anjuta_project_boxed_node_free);
	
  return type_id;
}

