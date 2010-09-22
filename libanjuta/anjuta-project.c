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
#define NODE_DATA(node)  node
#define PROXY_DATA(node)  ((node) != NULL ? (AnjutaProjectProxyData *)((node)->data) : NULL)

/* Properties functions
 *---------------------------------------------------------------------------*/

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
	g_return_val_if_fail (node != NULL, NULL);
	
	return node->parent;
}

AnjutaProjectNode *
anjuta_project_node_first_child(AnjutaProjectNode *node)
{
	g_return_val_if_fail (node != NULL, NULL);
	
	return node->children;
}

AnjutaProjectNode *
anjuta_project_node_last_child(AnjutaProjectNode *node)
{
	g_return_val_if_fail (node != NULL, NULL);

	node = node->children;
	if (node)
		while (node->next)
			node = node->next;

  return node;
}

AnjutaProjectNode *
anjuta_project_node_next_sibling (AnjutaProjectNode *node)
{
	g_return_val_if_fail (node != NULL, NULL);
	
	return node->next;
}

AnjutaProjectNode *
anjuta_project_node_prev_sibling (AnjutaProjectNode *node)
{
	g_return_val_if_fail (node != NULL, NULL);
	
	return node->prev;
}

AnjutaProjectNode *anjuta_project_node_nth_child (AnjutaProjectNode *node, guint n)
{
	g_return_val_if_fail (node != NULL, NULL);

	node = node->children;
	if (node)
		while ((n-- > 0) && node)
			node = node->next;

	return node;
}

static AnjutaProjectNode *
anjuta_project_node_post_order_traverse (AnjutaProjectNode *node, AnjutaProjectNodeTraverseFunc func, gpointer data)
{
	AnjutaProjectNode *child;
	
	child = node->children;
	while (child != NULL)
	{
		AnjutaProjectNode *current;

		current = child;
		child = current->next;
		current = anjuta_project_node_post_order_traverse (current, func, data);
		if (current != NULL)
		{
			return current;
		}
	}
	
	return func (node, data) ? node : NULL;
}

static AnjutaProjectNode *
anjuta_project_node_pre_order_traverse (AnjutaProjectNode *node, AnjutaProjectNodeTraverseFunc func, gpointer data)
{
	AnjutaProjectNode *child;

	if (func (node, data))
	{
		return node;
	}
	
	child = node->children;
	while (child != NULL)
	{
		AnjutaProjectNode *current;

		current = child;
		child = current->next;
		current = anjuta_project_node_pre_order_traverse (current, func, data);
		if (current != NULL)
		{
			return current;
		}
	}

	return NULL;
}


AnjutaProjectNode *
anjuta_project_node_traverse (AnjutaProjectNode *node, GTraverseType order, AnjutaProjectNodeTraverseFunc func, gpointer data)
{
	g_return_val_if_fail (node != NULL, NULL);
	g_return_val_if_fail (func != NULL, NULL);
	g_return_val_if_fail ((order != G_PRE_ORDER) || (order != G_POST_ORDER), NULL);

	switch (order)
	{
	case G_PRE_ORDER:
		return anjuta_project_node_pre_order_traverse (node, func, data);
	case G_POST_ORDER:
		return anjuta_project_node_post_order_traverse (node, func, data);
	default:
		return NULL;
	}
}

AnjutaProjectNode *
anjuta_project_node_children_traverse (AnjutaProjectNode *node, AnjutaProjectNodeTraverseFunc func, gpointer data)
{
	AnjutaProjectNode *child;
	
	g_return_val_if_fail (node != NULL, NULL);

	child = node->children;
	while (child != NULL)
	{
		AnjutaProjectNode *current;

		current = child;
		child = current->next;
		if (func (current, data))
		{
			return current;
		}
	}

	return NULL;
}

static void
anjuta_project_node_post_order_foreach (AnjutaProjectNode *node, AnjutaProjectNodeForeachFunc func, gpointer data)
{
	AnjutaProjectNode *child;
	
	child = node->children;
	while (child != NULL)
	{
		AnjutaProjectNode *current;

		current = child;
		child = current->next;
		anjuta_project_node_post_order_foreach (current, func, data);
	}
	
	func (node, data);
}

static void
anjuta_project_node_pre_order_foreach (AnjutaProjectNode *node, AnjutaProjectNodeForeachFunc func, gpointer data)
{
	AnjutaProjectNode *child;

	func (node, data);
	
	child = node->children;
	while (child != NULL)
	{
		AnjutaProjectNode *current;

		current = child;
		child = current->next;
		anjuta_project_node_pre_order_foreach (current, func, data);
	}
}


void
anjuta_project_node_foreach (AnjutaProjectNode *node, GTraverseType order, AnjutaProjectNodeForeachFunc func, gpointer data)
{
	g_return_if_fail (node != NULL);
	g_return_if_fail (func != NULL);
	g_return_if_fail ((order != G_PRE_ORDER) || (order != G_POST_ORDER));

	switch (order)
	{
	case G_PRE_ORDER:
			anjuta_project_node_pre_order_foreach (node, func, data);
			break;
	case G_POST_ORDER:
			anjuta_project_node_post_order_foreach (node, func, data);
			break;
	default:
			break;
	}
}

void
anjuta_project_node_children_foreach (AnjutaProjectNode *node, AnjutaProjectNodeForeachFunc func, gpointer data)
{
	AnjutaProjectNode *child;
	
	g_return_if_fail (node != NULL);

	child = node->children;
	while (child != NULL)
	{
		AnjutaProjectNode *current;

		current = child;
		child = current->next;
		func (current, data);
	}
}

AnjutaProjectNode *
anjuta_project_node_append (AnjutaProjectNode *parent, AnjutaProjectNode *node)
{ 
	return anjuta_project_node_insert_before (parent, NULL, node);
}

AnjutaProjectNode *
anjuta_project_node_insert_before (AnjutaProjectNode *parent, AnjutaProjectNode *sibling, AnjutaProjectNode *node)
{
	g_return_val_if_fail (node != NULL, NULL);
	g_return_val_if_fail (parent != NULL, node);

	/* FIXME: Try to avoid filling parent member to allow these checks
	g_return_val_if_fail (node->parent == NULL)
	if (sibling)
		g_return_val_if_fail (sibling->parent == parent, node);*/

	g_object_ref_sink (node);
	
	node->parent = parent;
	if (sibling)
	{
		if (sibling->prev)
		{
			node->prev = sibling->prev;
			node->prev->next = node;
			node->next = sibling;
			sibling->prev = node;
		}
		else
		{
			node->parent->children = node;
			node->next = sibling;
			sibling->prev = node;
		}
	}
	else
	{
		if (parent->children)
		{
			sibling = parent->children;
			while (sibling->next)
				sibling = sibling->next;
			node->prev = sibling;
			sibling->next = node;
		}
		else
		{
			node->parent->children = node;
		}
	}

	return node;	
}

AnjutaProjectNode *
anjuta_project_node_insert_after (AnjutaProjectNode *parent, AnjutaProjectNode *sibling, AnjutaProjectNode *node)
{
	g_return_val_if_fail (node != NULL, NULL);
	g_return_val_if_fail (parent != NULL, node);

	/* FIXME: Try to avoid filling parent member to allow these checks
	g_return_val_if_fail (node->parent == NULL)
	if (sibling)
		g_return_val_if_fail (sibling->parent == parent, node);*/

	g_object_ref_sink (node);
	
	node->parent = parent;
	if (sibling)
    {
		if (sibling->next)
		{
			sibling->next->prev = node;
		}
		node->next = sibling->next;
		node->prev = sibling;
		sibling->next = node;
	}
	else
    {
		if (parent->children)
		{
			node->next = parent->children;
			parent->children->prev = node;
		}
		parent->children = node;
	}

	return node;	
}

AnjutaProjectNode *
anjuta_project_node_remove (AnjutaProjectNode *node)
{
	g_return_val_if_fail (node != NULL, NULL);
	
	if (node->prev)
		node->prev->next = node->next;
	else if (node->parent)
		node->parent->children = node->next;
	node->parent = NULL;
	if (node->next)
	{
		node->next->prev = node->prev;
		node->next = NULL;
	}
    node->prev = NULL;

	g_object_force_floating (node);
	
	return node;
}

AnjutaProjectNode *
anjuta_project_node_replace (AnjutaProjectNode *node, AnjutaProjectNode *replacement)
{
	AnjutaProjectNode *child;
	AnjutaProjectNode *sibling;
	AnjutaProjectNode *next;
	
	if (node->parent != NULL)
	{
		anjuta_project_node_insert_after (node->parent, node, replacement);
		anjuta_project_node_remove (node);
	}

	/* Move all children from node to replacement */
	sibling = NULL;
	for (child = anjuta_project_node_first_child (node); child != NULL; child = next)
	{
		next = anjuta_project_node_next_sibling (child);
		anjuta_project_node_remove (child);
		sibling = anjuta_project_node_insert_after (replacement, sibling, child);
		child = next;
	}
	
	/* Move all children from replacement to node */
	child = anjuta_project_node_next_sibling (sibling);
	sibling = NULL;
	for (; child != NULL; child = next)
	{
		next = anjuta_project_node_next_sibling (child);
		anjuta_project_node_remove (child);
		sibling = anjuta_project_node_insert_after (node, sibling, child);
		child = next;
	}
	
	return replacement;
}

AnjutaProjectNode *
anjuta_project_node_exchange (AnjutaProjectNode *node, AnjutaProjectNode *replacement)
{
	AnjutaProjectNode *marker = g_object_new (ANJUTA_TYPE_PROJECT_NODE, NULL);
	
	if (node->parent != NULL)
	{
		anjuta_project_node_insert_after (node->parent, node, marker);
		anjuta_project_node_remove (node);
	}
	if (replacement->parent != NULL)
	{
		anjuta_project_node_insert_after (replacement->parent, replacement, node);
		anjuta_project_node_remove (replacement);
	}
	if (marker->parent != NULL)
	{
		anjuta_project_node_insert_after (marker->parent, marker, replacement);
		anjuta_project_node_remove (marker);
	}
	g_object_unref (marker);

	return replacement;
}

AnjutaProjectNode *
anjuta_project_node_grab_children (AnjutaProjectNode *parent, AnjutaProjectNode *node)
{
	AnjutaProjectNode *child;
	AnjutaProjectNode *sibling;
	
	sibling = anjuta_project_node_last_child (parent);
	
	for (child = anjuta_project_node_first_child (node); child != NULL;)
	{
		AnjutaProjectNode *remove;

		remove = child;
		child = anjuta_project_node_next_sibling (child);
		anjuta_project_node_remove (remove);
		sibling = anjuta_project_node_insert_after (parent, sibling, remove);
	}
	
	return parent;
}


AnjutaProjectNode *
anjuta_project_node_prepend (AnjutaProjectNode *parent, AnjutaProjectNode *node)
{
	return anjuta_project_node_insert_before (parent, parent->children, node);	
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
anjuta_project_node_get_node_type (const AnjutaProjectNode *node)
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
	if ((NODE_DATA (node)->file == NULL) && (NODE_DATA (node)->name != NULL))
	{
		/* Try to create a file */
		AnjutaProjectNode *parent;

		parent = anjuta_project_node_parent (node);
		if ((parent != NULL) && (NODE_DATA (parent)->file != NULL))
		{
			NODE_DATA (node)->file = g_file_get_child (NODE_DATA (parent)->file, NODE_DATA (node)->name);
		}
	}

	return NODE_DATA (node)->file;
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
anjuta_project_group_compare (AnjutaProjectNode *node, gpointer data)
{
	GFile *file = (GFile *)data;

	if (((NODE_DATA (node)->type & ANJUTA_PROJECT_TYPE_MASK) == ANJUTA_PROJECT_GROUP) && g_file_equal (NODE_DATA(node)->file, file))
	{
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
	AnjutaProjectNode *node;

	node = anjuta_project_node_traverse (root, G_PRE_ORDER, anjuta_project_group_compare, directory);

	return node;
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
anjuta_project_target_compare (AnjutaProjectNode *node, gpointer data)
{
	const gchar *name = (gchar *)data;

	if (((NODE_DATA (node)->type & ANJUTA_PROJECT_TYPE_MASK) == ANJUTA_PROJECT_TARGET) && (strcmp (NODE_DATA(node)->name, name) == 0))
	{
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
	AnjutaProjectNode *node;

	node = anjuta_project_node_traverse (parent, G_PRE_ORDER, anjuta_project_target_compare, name);

	return node;
}

static gboolean
anjuta_project_source_compare (AnjutaProjectNode *node, gpointer data)
{
	GFile *file = (GFile *)data;

	if (((NODE_DATA (node)->type & ANJUTA_PROJECT_TYPE_MASK) == ANJUTA_PROJECT_SOURCE) && g_file_equal (NODE_DATA(node)->file, file))
	{
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
	AnjutaProjectNode *node;


	node = anjuta_project_node_traverse (parent, G_PRE_ORDER, anjuta_project_source_compare, file);

	return node;
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


static unsigned int anjuta_project_node_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (AnjutaProjectNode, anjuta_project_node, G_TYPE_INITIALLY_UNOWNED);

static void
anjuta_project_node_init (AnjutaProjectNode *node)
{
	node->next = NULL;
	node->prev = NULL;
	node->parent = NULL;
	node->children = NULL;
	
	node->type = 0;
	node->state = 0;
	node->properties = NULL;
	node->file = NULL;
	node->name = NULL;
}

static void
anjuta_project_node_dispose (GObject *object)
{
	AnjutaProjectNode *node = ANJUTA_PROJECT_NODE(object);

	//g_message ("anjuta_project_node_dispose node %p children %p parent %p prev %p name %s file %s", node, node->children, node->parent, node->prev, node->name, node->file != NULL ? g_file_get_path (node->file) : "no file");

	anjuta_project_node_remove (node);
	
	if (node->file != NULL) g_object_unref (node->file);
	node->file = NULL;

	while (node->children != NULL)
	{
		AnjutaProjectNode *child;
		
		child = anjuta_project_node_remove (node->children);
		g_object_unref (child);
	}
	
	G_OBJECT_CLASS (anjuta_project_node_parent_class)->dispose (object);
}

static void
anjuta_project_node_finalize (GObject *object)
{
	AnjutaProjectNode *node = ANJUTA_PROJECT_NODE(object);

	if (node->name != NULL) g_free (node->name);
	
	G_OBJECT_CLASS (anjuta_project_node_parent_class)->finalize (object);
}

static void 
anjuta_project_node_get_gobject_property (GObject    *object,
	      guint       prop_id,
	      GValue     *value,
	      GParamSpec *pspec)
{
	AnjutaProjectNode *node = ANJUTA_PROJECT_NODE(object);
        
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void 
anjuta_project_node_set_gobject_property (GObject      *object,
	      guint         prop_id,
	      const GValue *value,
	      GParamSpec   *pspec)
{
	AnjutaProjectNode *node = ANJUTA_PROJECT_NODE(object);
       G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
anjuta_project_node_class_init (AnjutaProjectNodeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	
	object_class->finalize = anjuta_project_node_finalize;
	object_class->dispose = anjuta_project_node_dispose;
	object_class->get_property = anjuta_project_node_get_gobject_property;
	object_class->set_property = anjuta_project_node_set_gobject_property;

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
	
	anjuta_project_node_signals[UPDATED] = g_signal_new ("updated",
	    G_OBJECT_CLASS_TYPE (object_class),
	    G_SIGNAL_RUN_LAST,
	    G_STRUCT_OFFSET (AnjutaProjectNodeClass, updated),
	    NULL, NULL,
        anjuta_cclosure_marshal_VOID__STRING_BOXED,
	    G_TYPE_NONE,
	    2,
	    G_TYPE_POINTER,
	    G_TYPE_ERROR);
	
	anjuta_project_node_signals[LOADED] = g_signal_new ("loaded",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (AnjutaProjectNodeClass, loaded),
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



/* Proxy node object
 *---------------------------------------------------------------------------*/

#define ANJUTA_TYPE_PROJECT_PROXY_NODE					(anjuta_project_proxy_node_get_type ())
#define ANJUTA_PROJECT_PROXY_NODE(obj)					(G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_PROJECT_PROXY_NODE, AnjutaProjectProxyNode))

typedef struct _AnjutaProjectProxyNode AnjutaProjectProxyNode;
GType anjuta_project_proxy_node_get_type (void) G_GNUC_CONST;

struct _AnjutaProjectProxyNode{
	AnjutaProjectNode base;
	AnjutaProjectNode *node;
	guint reference;
};


AnjutaProjectNode *
anjuta_project_proxy_new (AnjutaProjectNode *node)
{
#if 0	
	AnjutaProjectNode *proxy;
	GTypeQuery node_type_info;
	GTypeQuery base_type_info;
	guint extra_size;
	

	/* Clone node object */
	proxy = g_object_new (G_TYPE_FROM_INSTANCE (node), NULL);

	/* Swap specific data */
	g_type_query (G_TYPE_FROM_INSTANCE (node), &node_type_info);
	g_type_query (ANJUTA_TYPE_PROJECT_NODE, &base_type_info);

	extra_size = node_type_info.instance_size - base_type_info.instance_size;
	if (extra_size > 0)
	{
		gchar *data;

		data = g_new (gchar , extra_size);
		memcpy (data, ((gchar *)node) +  base_type_info.instance_size, extra_size);
		memcpy (((gchar *)node) +  base_type_info.instance_size, ((gchar *)proxy) +  base_type_info.instance_size, extra_size);
		memcpy (((gchar *)proxy) +  base_type_info.instance_size, data, extra_size);
		g_free (data);
	}

	/* Copy node data */
	proxy->type = node->type;
	proxy->file = node->file != NULL ? g_file_dup (node->file) : NULL;
	proxy->name = g_strdup (node->name);
	proxy->state = node->state;

	/* Shallow copy of all properties */
	if ((node->properties == NULL) || (((AnjutaProjectPropertyInfo *)node->properties->data)->override == NULL))
	{
		proxy->properties = node->properties;
	}
	else
	{
		GList *item;

		proxy->properties = node->properties;
		node->properties = g_list_copy (proxy->properties);
		for (item = g_list_first (node->properties); item != NULL; item = g_list_next (item))
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
#endif	
	AnjutaProjectProxyNode *proxy;
	
	/* If the node is already a proxy get original node */
	node = anjuta_project_proxy_get_node (node);
	
	/* Create proxy node */
	proxy = g_object_new (ANJUTA_TYPE_PROJECT_PROXY_NODE, NULL);
	proxy->reference = 1;
	proxy->node = node;
	proxy->base.type = node->type | ANJUTA_PROJECT_PROXY;
	proxy->base.file = node->file != NULL ? g_object_ref (node->file) : NULL;
	proxy->base.name = g_strdup (node->name);
		
	/* Shallow copy of all properties */
	if ((node->properties == NULL) || (((AnjutaProjectPropertyInfo *)node->properties->data)->override == NULL))
	{
		proxy->base.properties = node->properties;
	}
	else
	{
		GList *item;
		proxy->base.properties = g_list_copy (node->properties);
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
	
	return ANJUTA_PROJECT_NODE (proxy);
}

AnjutaProjectNode *
anjuta_project_proxy_unref (AnjutaProjectNode *node)
{
	g_object_unref (G_OBJECT (node));

	return node;
}

/* GObjet implementation
 *---------------------------------------------------------------------------*/

typedef struct _AnjutaProjectProxyNodeClass AnjutaProjectProxyNodeClass;

struct _AnjutaProjectProxyNodeClass {
	AnjutaProjectNodeClass parent_class;
};

G_DEFINE_TYPE (AnjutaProjectProxyNode, anjuta_project_proxy_node, ANJUTA_TYPE_PROJECT_NODE);

static void
anjuta_project_proxy_node_init (AnjutaProjectProxyNode *node)
{
}

static void
anjuta_project_proxy_node_finalize (GObject *object)
{
	AnjutaProjectProxyNode *proxy = ANJUTA_PROJECT_PROXY_NODE (object);
	
	G_OBJECT_CLASS (anjuta_project_proxy_node_parent_class)->finalize (object);
}


static void
anjuta_project_proxy_node_class_init (AnjutaProjectProxyNodeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	
	object_class->finalize = anjuta_project_proxy_node_finalize;
}


/* Proxy node functions
 *---------------------------------------------------------------------------*/

static void
reparent_children (AnjutaProjectNode *node, gpointer data)
{
	node->parent = ANJUTA_PROJECT_NODE (data);
}

static void
free_node_property (gpointer data, gpointer user_data)
{
	g_slice_free (AnjutaProjectPropertyInfo, data);
}

AnjutaProjectNode *
anjuta_project_proxy_exchange (AnjutaProjectNode *proxy, AnjutaProjectNode *node)
{
	GTypeQuery node_type_info;
	GTypeQuery base_type_info;
	guint extra_size;
	AnjutaProjectNode *other;
	

	/* Swap specific data */
	g_type_query (G_TYPE_FROM_INSTANCE (node), &node_type_info);
	g_type_query (ANJUTA_TYPE_PROJECT_NODE, &base_type_info);

	extra_size = node_type_info.instance_size - base_type_info.instance_size;
	if (extra_size > 0)
	{
		gchar *data;

		data = g_new (gchar , extra_size);
		memcpy (data, ((gchar *)node) +  base_type_info.instance_size, extra_size);
		memcpy (((gchar *)node) +  base_type_info.instance_size, ((gchar *)proxy) +  base_type_info.instance_size, extra_size);
		memcpy (((gchar *)proxy) +  base_type_info.instance_size, data, extra_size);
		g_free (data);
	}

	/* Exchange link */
	other = proxy->children;
	proxy->children = node->children;
	node->children = other;
	anjuta_project_node_children_foreach (proxy, reparent_children, proxy);
	anjuta_project_node_children_foreach (node, reparent_children, node);
	
	/* Delete node temporary properties */
	if ((node->properties != NULL) && (((AnjutaProjectPropertyInfo *)node->properties->data)->override != NULL))
	{
		g_list_foreach (node->properties, free_node_property, NULL);
	}
	node->properties = proxy->properties;
	proxy->properties = NULL;
	
#if 0	
	AnjutaProjectNodeData *data;
	
	data = proxy->data;
	proxy->data = node->data;
	node->data = data;
#endif	
	return proxy;
}

AnjutaProjectNode *
anjuta_project_proxy_get_node (AnjutaProjectNode *node)
{
	g_return_val_if_fail (node != NULL, FALSE);

	if (ANJUTA_PROJECT_NODE (node)->type & ANJUTA_PROJECT_PROXY)
	{
		return ANJUTA_PROJECT_PROXY_NODE (node)->node;
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

#if 0
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
#endif
