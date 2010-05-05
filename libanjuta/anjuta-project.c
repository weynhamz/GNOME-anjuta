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
	return g_node_insert_before (parent, sibling, node);
}

AnjutaProjectNode *
anjuta_project_node_insert_after (AnjutaProjectNode *parent, AnjutaProjectNode *sibling, AnjutaProjectNode *node)
{
	return g_node_insert_after (parent, sibling, node);
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

	/* Get properties list */
	item = g_list_first (NODE_DATA (node)->properties);

	for (; item != NULL; item = g_list_next (item))
	{
		AnjutaProjectPropertyInfo *info = (AnjutaProjectPropertyInfo *)item->data;

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
	if (NODE_DATA (node)->type & ANJUTA_PROJECT_PROXY)
	{
		PROXY_DATA (node)->reference++;
	}
	else
	{
		AnjutaProjectProxyData *proxy;
		AnjutaProjectNodeData *data = NODE_DATA (node);

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
	}

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

gboolean
anjuta_project_node_is_proxy (AnjutaProjectNode *node)
{
	g_return_val_if_fail (node != NULL, FALSE);

	return NODE_DATA (node)->type & ANJUTA_PROJECT_PROXY ? TRUE : FALSE;
}
