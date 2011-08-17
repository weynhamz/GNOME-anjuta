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
#include "anjuta-enum-types.h"

#include <string.h>

/**
 * SECTION:anjuta-project
 * @title: Anjuta project
 * @short_description: Anjuta project
 * @see_also: 
 * @stability: Unstable
 * @include: libanjuta/anjuta-project.h
 * 
 * A project in Anjuta is represented by a tree. There are six kinds of node.
 *
 * The root node is the parent of all other nodes, it can implement
 * IAnjutaProject interface and represent the project itself but it is not
 * mandatory.
 * 
 * A module node represents a module in autotools project, it is a group of
 * packages.
 * 
 * A package node represents a package in autotools project, it is library.
 * 
 * A group node is used to group several target or source, it can represent
 * a directory by example.
 *
 * A target node represents an object file defined explicitely.
 * There are different kinds of target: program, library...
 * A target have as children all source needed to build it.
 *
 * A source node represents a source file. These are lead of the tree, a source
 * node cannot have children.
 *
 * All these nodes are base objects. They have derived in each project backend
 * to provide more specific information.
 */ 

/* Node properties
 *---------------------------------------------------------------------------*/

/* Implement Boxed type
 *---------------------------------------------------------------------------*/

/**
 * anjuta_project_property_new:
 * @name: (transfer none):
 * @value: (transfer none):
 * @native: (allow-none) (transfer none):
 *
 * Returns: (transfer full):
 */
AnjutaProjectProperty *
anjuta_project_property_new (const gchar *name, AnjutaProjectValueType type,
                             const gchar *value, AnjutaProjectProperty *native)
{
	AnjutaProjectProperty *prop = g_slice_new0(AnjutaProjectProperty);
	prop->name = g_strdup (name);
	prop->type = type;
	prop->value = g_strdup (value);

	if (native != NULL) {
		prop->native = native;
		prop->flags = native->flags;
		prop->detail = native->detail;
	}
	
	return prop;
}

AnjutaProjectProperty *
anjuta_project_property_copy (AnjutaProjectProperty *prop)
{
	return anjuta_project_property_new (prop->name, prop->type,
	                                    prop->value, prop->native);
}

void
anjuta_project_property_free (AnjutaProjectProperty *prop)
{
	g_free (prop->name);
	g_free (prop->value);
	g_slice_free (AnjutaProjectProperty, prop);
}

GType
anjuta_project_property_get_type (void)
{
	static GType type_id = 0;

	if (!type_id)
		type_id = g_boxed_type_register_static ("AnjutaProjectProperty",
		                                        (GBoxedCopyFunc) anjuta_project_property_copy,
		                                        (GBoxedFreeFunc) anjuta_project_property_free);

	return type_id;
}



/* Node
 *---------------------------------------------------------------------------*/


/* Moving in tree functions
 *---------------------------------------------------------------------------*/

/**
 * anjuta_project_node_parent:
 *
 * Returns: (transfer none):
 */
AnjutaProjectNode *
anjuta_project_node_parent(AnjutaProjectNode *node)
{
	g_return_val_if_fail (node != NULL, NULL);
	
	return node->parent;
}

/**
 * anjuta_project_node_root:
 *
 * Returns: (transfer none):
 */
AnjutaProjectNode *
anjuta_project_node_root (AnjutaProjectNode *node)
{
	g_return_val_if_fail (node != NULL, NULL);

	while (node->parent != NULL)
	{
		node = node->parent;
	}

	return node;
}

/**
 * anjuta_project_node_first_child:
 *
 * Returns: (transfer none):
 */

AnjutaProjectNode *
anjuta_project_node_first_child(AnjutaProjectNode *node)
{
	g_return_val_if_fail (node != NULL, NULL);
	
	return node->children;
}

/**
 * anjuta_project_node_last_child:
 *
 * Returns: (transfer none):
 */

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

/**
 * anjuta_project_node_next_sibling:
 *
 * Returns: (transfer none):
 */
AnjutaProjectNode *
anjuta_project_node_next_sibling (AnjutaProjectNode *node)
{
	g_return_val_if_fail (node != NULL, NULL);
	
	return node->next;
}

/**
 * anjuta_project_node_prev_sibling:
 *
 * Returns: (transfer none):
 */
AnjutaProjectNode *
anjuta_project_node_prev_sibling (AnjutaProjectNode *node)
{
	g_return_val_if_fail (node != NULL, NULL);
	
	return node->prev;
}

/**
 * anjuta_project_node_nth_child:
 *
 * Returns: (transfer none):
 */
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


/**
 * anjuta_project_node_traverse:
 * @func: (scope call):
 *
 * Returns: (transfer none):
 */
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

/**
 * anjuta_project_node_children_traverse:
 * @func: (scope call):
 *
 * Returns: (transfer none):
 */
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

/**
 * anjuta_project_node_foreach:
 * @func: (scope call):
 */
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

/**
 * anjuta_project_node_children_foreach:
 * @func: (scope call):
 */
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

/**
 * anjuta_project_node_parent_type:
 *
 * Returns: (transfer none):
 */
AnjutaProjectNode *
anjuta_project_node_parent_type(AnjutaProjectNode *node,  AnjutaProjectNodeType type)
{
	do
	{
		node = anjuta_project_node_parent (node);
		if (node == NULL) break;
	}
	while (anjuta_project_node_get_node_type (node) != type);

	return node;
}



/* Debugging functions
 *---------------------------------------------------------------------------*/

static gboolean check_node (AnjutaProjectNode *node, gpointer data)
{
	if (!ANJUTA_IS_PROJECT_NODE (node)) g_critical ("    Node %p of %p is not a AnjutaProjectNode", node, data);
	if (node->prev == NULL)
	{
		if ((node->parent != NULL) && (node->parent->children != node)) g_critical ("    Node %p of %p has the wrong parent", node, data);
	}
	else
	{
		if (node->prev->next != node) g_critical ("    Node %p of %p has the wrong predecessor", node, data);
		if (node->prev->parent != node->parent) g_critical ("    Node %p of %p has the wrong parent", node, data);
	}
	if (node->next != NULL)
	{
		if (node->next->prev != node) g_critical ("    Node %p of %p has the wrong successor", node, data);
	}
	if (node->children != NULL)
	{
		if (node->children->parent != node) g_critical ("    Node %p of %p has the wrong children", node, data);
	}

	return FALSE;
}

void
anjuta_project_node_check (AnjutaProjectNode *parent)
{
	AnjutaProjectNode *node;
	
	g_message ("Check node %p", parent);
	node = anjuta_project_node_traverse (parent, G_POST_ORDER, check_node, parent);
	if (node == NULL) g_message ("    Node %p is valid", parent);
}


static void
anjuta_project_node_show (AnjutaProjectNode *node, gint indent)
{
	g_message("%*s %p: %s", indent, "", node, node != NULL ? anjuta_project_node_get_name (node) : NULL);
}

static void
anjuta_project_node_dump_child (AnjutaProjectNode *parent, gint indent)
{
	AnjutaProjectNode *child;
	
	anjuta_project_node_show (parent, indent);
	indent += 4;
	
	for (child = anjuta_project_node_first_child (parent); child != NULL; child = anjuta_project_node_next_sibling (child))
	{
		anjuta_project_node_dump_child (child, indent);
	}
}

void
anjuta_project_node_dump (AnjutaProjectNode *parent)
{
		anjuta_project_node_dump_child (parent, 0);
}



/* Adding node functions
 *---------------------------------------------------------------------------*/

/**
 * anjuta_project_node_insert_before:
 * @parent:
 * @sibling: (allow-none) (transfer none):
 * @node: (transfer none):
 *
 * Returns: (transfer none):
 */
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

/**
 * anjuta_project_node_insert_after:
 * @parent:
 * @sibling: (allow-none) (transfer none):
 * @node: (transfer none):
 *
 * Returns: (transfer none):
 */
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

/**
 * anjuta_project_node_remove:
 * @node: (transfer none):
 *
 * Returns: (transfer full):
 */
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
	
	return node;
}

/**
 * anjuta_project_node_prepend:
 *
 * Returns: (transfer none):
 */
AnjutaProjectNode *
anjuta_project_node_prepend (AnjutaProjectNode *parent, AnjutaProjectNode *node)
{
	return anjuta_project_node_insert_before (parent, parent->children, node);	
}

/**
 * anjuta_project_node_append:
 *
 * Returns: (transfer none):
 */
AnjutaProjectNode *
anjuta_project_node_append (AnjutaProjectNode *parent, AnjutaProjectNode *node)
{ 
	return anjuta_project_node_insert_before (parent, NULL, node);
}

/* Access functions
 *---------------------------------------------------------------------------*/

AnjutaProjectNodeType
anjuta_project_node_get_node_type (const AnjutaProjectNode *node)
{
	return node == NULL ? ANJUTA_PROJECT_UNKNOWN : (node->type & ANJUTA_PROJECT_TYPE_MASK);
}

AnjutaProjectNodeType
anjuta_project_node_get_full_type (const AnjutaProjectNode *node)
{
	return node == NULL ? ANJUTA_PROJECT_UNKNOWN : node->type;
}


AnjutaProjectNodeState
anjuta_project_node_get_state (const AnjutaProjectNode *node)
{
	return node == NULL ? ANJUTA_PROJECT_OK : (node->state);
}

const gchar *
anjuta_project_node_get_name (const AnjutaProjectNode *node)
{
	if ((node->name == NULL) && (node->file != NULL))
	{
		((AnjutaProjectNode *)node)->name = g_file_get_basename (node->file);
	}

	return node->name;
}

/**
 * anjuta_project_node_get_file:
 *
 * Returns: (transfer none):
 */
GFile*
anjuta_project_node_get_file (const AnjutaProjectNode *node)
{
	switch (node->type & ANJUTA_PROJECT_TYPE_MASK)
	{
	case ANJUTA_PROJECT_OBJECT:	
	case ANJUTA_PROJECT_TARGET:
		if ((node->name) && (node->parent != NULL) && (node->parent->file != NULL))
		{
			GFile *file = g_file_get_child (node->parent->file, node->name);

			if ((node->file != NULL) && g_file_equal (node->file, file))
			{
				/* Keep the same file */
				g_object_unref (file);
			}
			else
			{
				/* Parent has been updated, update file */
				if (node->file != NULL) g_object_unref (node->file);
				((AnjutaProjectNode *)node)->file = file;
			}
		}
		break;
	default:
		break;
	}
			
	return node->file;		
}

/**
 * anjuta_project_node_get_custom_properties:
 *
 * Returns: (transfer none) (element-type Anjuta.ProjectProperty):
 */
GList *
anjuta_project_node_get_custom_properties (AnjutaProjectNode *node)
{
	return node->custom_properties;
}

/**
 * anjuta_project_node_get_native_properties:
 *
 * Returns: (transfer none) (element-type Anjuta.ProjectProperty):
 */
GList *
anjuta_project_node_get_native_properties (AnjutaProjectNode *node)
{
	return node->native_properties;
}

static gint
find_property (gconstpointer item, gconstpointer data)
{
	AnjutaProjectProperty *prop_a = (AnjutaProjectProperty *)item;
	AnjutaProjectProperty *prop_b = (AnjutaProjectProperty *)data;

	if (prop_a->native != NULL) prop_a = prop_a->native;
	if (prop_b->native != NULL) prop_b = prop_b->native;

	return prop_a != prop_b;
}

/**
 * anjuta_project_node_get_property:
 *
 * Returns: (transfer none):
 */
AnjutaProjectProperty *
anjuta_project_node_get_property (AnjutaProjectNode *node, AnjutaProjectProperty *property)
{
	GList *found;

	/* Search in custom properties */
	found = g_list_find_custom (node->custom_properties, property, find_property);

	if (found == NULL)
	{
		/* Search in native properties */
		found = g_list_find_custom (node->native_properties, property, find_property);
	}

	return found != NULL ? (AnjutaProjectProperty *)found->data : NULL;
}

/* If name is specified, look for a property with the same name, useful for
 * map properties */
AnjutaProjectProperty *
anjuta_project_node_get_map_property (AnjutaProjectNode *node, AnjutaProjectProperty *property, const gchar *name)
{
	GList *found = NULL;

	/* Check if the property is already the right one */
	if (property->native != NULL)
	{
		found = g_list_find (node->custom_properties, property);
	}
	
	/* Search in custom properties */
	if (found == NULL)
	{
		found = g_list_find_custom (node->custom_properties, property, find_property);
		if (name != NULL)
		{
			while ((found != NULL) && (strcmp (name, ((AnjutaProjectProperty *)found->data)->name) != 0))
			{
				found = g_list_find_custom (g_list_next (found), property, find_property);
			}
		}
	}

	if (found == NULL)
	{
		/* Search in native properties */
		found = g_list_find_custom (node->native_properties, property, find_property);
	}

	return found != NULL ? (AnjutaProjectProperty *)found->data : NULL;
}

/* Set functions
 *---------------------------------------------------------------------------*/

gboolean
anjuta_project_node_set_state (AnjutaProjectNode *node, AnjutaProjectNodeState state)
{
	if (node == NULL) return FALSE;
	node->state |= state;
	return TRUE;
}

gboolean
anjuta_project_node_clear_state (AnjutaProjectNode *node, AnjutaProjectNodeState state)
{
	if (node == NULL) return FALSE;
	node->state &= ~state;
	return TRUE;
}

/**
 * anjuta_project_node_insert_property:
 * @property: (transfer full):
 *
 * Returns: (transfer none):
 */

AnjutaProjectProperty *
anjuta_project_node_insert_property (AnjutaProjectNode *node, AnjutaProjectProperty *native, AnjutaProjectProperty *property)
{
	/* Make sure the property is native */
	if (native->native != NULL) native = native->native;
	
	/* Fill missing information */
	if (property->name == NULL) property->name = native->name;
	property->type = native->type;
	property->native = native;
	property->flags = native->flags;
	property->detail = native->detail;

	/* Get properties list */
	node->custom_properties = g_list_append (node->custom_properties, property);
	
	return property;
}

AnjutaProjectProperty *
anjuta_project_node_remove_property (AnjutaProjectNode *node, AnjutaProjectProperty *prop)
{
	GList *found;
	AnjutaProjectProperty *removed = NULL;

	/* Search the exact property, useful for list property */
	found = g_list_find (node->custom_properties, prop);
	if (found == NULL)
	{
		found = g_list_find_custom (node->custom_properties, prop, find_property);
	}

	if (found != NULL)
	{
		removed = (AnjutaProjectProperty *)found->data;
		node->custom_properties = g_list_delete_link (node->custom_properties, found);
		/* If name is not owned by the property, remove it as the
		 * property can be associated with another one, having a
		 * different name */
		if ((removed->native != NULL) && (removed->name == removed->native->name))
		{
			removed->name = NULL;
		}
	}


	return removed;
}


/* Get node from file functions
 *---------------------------------------------------------------------------*/

static gboolean
anjuta_project_group_compare (AnjutaProjectNode *node, gpointer data)
{
	GFile *file = (GFile *)data;

	if (((node->type & ANJUTA_PROJECT_TYPE_MASK) == ANJUTA_PROJECT_GROUP) && g_file_equal (node->file, file))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/**
 * anjuta_project_node_get_group_from_file:
 * @func: (scope call):
 *
 * Returns: (transfer none):
 */
AnjutaProjectNode *
anjuta_project_node_get_group_from_file (const AnjutaProjectNode *root, GFile *directory)
{
	AnjutaProjectNode *node;

	node = anjuta_project_node_traverse ((AnjutaProjectNode *)root, G_PRE_ORDER, anjuta_project_group_compare, directory);

	return node;
}

static gboolean
anjuta_project_target_compare (AnjutaProjectNode *node, gpointer data)
{
	const gchar *name = (gchar *)data;

	if (((node->type & ANJUTA_PROJECT_TYPE_MASK) == ANJUTA_PROJECT_TARGET) && (strcmp (node->name, name) == 0))
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

	node = anjuta_project_node_traverse ((AnjutaProjectNode *)parent, G_PRE_ORDER, anjuta_project_target_compare, (gpointer)name);

	return node;
}

static gboolean
anjuta_project_source_compare (AnjutaProjectNode *node, gpointer data)
{
	GFile *file = (GFile *)data;

	if (((node->type & ANJUTA_PROJECT_TYPE_MASK) == ANJUTA_PROJECT_SOURCE) && g_file_equal (node->file, file))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/**
 * anjuta_project_node_get_source_from_file:
 * @func: (scope call):
 *
 * Returns: (transfer none):
 */
AnjutaProjectNode *
anjuta_project_node_get_source_from_file (const AnjutaProjectNode *parent, GFile *file)
{
	AnjutaProjectNode *node;


	node = anjuta_project_node_traverse ((AnjutaProjectNode *)parent, G_PRE_ORDER, anjuta_project_source_compare, (gpointer)file);

	return node;
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
	PROP_NATIVE_PROPERTIES,
	PROP_CUSTOM_PROPERTIES
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
	node->native_properties = NULL;
	node->custom_properties = NULL;
	node->file = NULL;
	node->name = NULL;
}

static void
anjuta_project_node_dispose (GObject *object)
{
	AnjutaProjectNode *node = ANJUTA_PROJECT_NODE(object);

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
	switch (prop_id) {
		case PROP_NAME:
			g_value_set_string (value, anjuta_project_node_get_name (node));
			break;
		case PROP_FILE:
			g_value_set_object (value, node->file);
			break;
		case PROP_STATE:
			g_value_set_flags (value, anjuta_project_node_get_state (node));
			break;
		case PROP_TYPE:
			g_value_set_flags (value, anjuta_project_node_get_node_type (node));
			break;
		case PROP_NATIVE_PROPERTIES:
			g_value_set_pointer (value, node->native_properties);
			break;
		case PROP_CUSTOM_PROPERTIES:
			g_value_set_pointer (value, node->custom_properties);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void 
anjuta_project_node_set_gobject_property (GObject      *object,
                                          guint         prop_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
	AnjutaProjectNode *node = ANJUTA_PROJECT_NODE(object);
	switch (prop_id) {
		case PROP_NAME:
			if (node->name != NULL)
				g_free (node->name);
			node->name = g_value_dup_string (value);
			break;
		case PROP_FILE:
			if (node->file != NULL)
				g_object_unref (node->file);
			node->file = g_value_dup_object (value);
			break;
		case PROP_STATE:
			anjuta_project_node_set_state (node, g_value_get_flags (value));
			break;
		case PROP_TYPE:
			node->type = g_value_get_flags (value);
			break;
		case PROP_NATIVE_PROPERTIES:
			node->native_properties = g_value_get_pointer (value);
			break;
		/* XXX: We may need to copy this instead */
		case PROP_CUSTOM_PROPERTIES:
			node->custom_properties = g_value_get_pointer (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
anjuta_project_node_class_init (AnjutaProjectNodeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GParamSpec* pspec;
	
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

	pspec = g_param_spec_flags ("type",
	                            "Type",
	                            "Node type",
	                            ANJUTA_TYPE_PROJECT_NODE_TYPE,
	                            0,
	                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_TYPE,
	                                 pspec);

	pspec = g_param_spec_flags ("state",
	                            "State",
	                            "Node state",
	                            ANJUTA_TYPE_PROJECT_NODE_STATE,
	                            0,
	                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_STATE,
	                                 pspec);

	pspec = g_param_spec_string ("name",
	                             "Name",
	                             "Node name",
	                             "",
	                             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_NAME,
	                                 pspec);

	pspec = g_param_spec_object ("file",
	                             "File",
	                             "The GFile for the node",
	                             G_TYPE_FILE,
	                             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_FILE,
	                                 pspec);

/**
 * AnjutaProjectNode:native-properties:
 *
 * type: GLib.List<Anjuta.ProjectProperty>
 * Transfer: none
 */
	pspec = g_param_spec_pointer ("native-properties",
	                              "Native properties",
	                              "The list of all possible properties",
	                              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_NATIVE_PROPERTIES,
	                                 pspec);
/**
 * AnjutaProjectNode:custom-properties:
 *
 * Type: GLib.List<Anjuta.ProjectProperty>
 * Transfer: none
 */
	pspec = g_param_spec_pointer ("custom-properties",
	                              "Custom properties",
	                              "The list of overriden properties",
	                              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_CUSTOM_PROPERTIES,
	                                 pspec);

}



/* Node information
 *---------------------------------------------------------------------------*/

/* Public functions
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

/**
 * anjuta_project_node_info_new:
 * @name: (transfer none):
 * @mime_type: (transfer none):
 *
 * Returns: (transfer full):
 */
AnjutaProjectNodeInfo *
anjuta_project_node_info_new (AnjutaProjectNodeType type,
                              const gchar *name,
                              const gchar *mime_type)
{
	AnjutaProjectNodeInfo *info = g_slice_new0 (AnjutaProjectNodeInfo);
	info->type = type;
	info->name = g_strdup (name);
	info->mime_type = g_strdup (mime_type);

	return info;
}

AnjutaProjectNodeInfo *
anjuta_project_node_info_copy (AnjutaProjectNodeInfo *info)
{
	return anjuta_project_node_info_new (info->type, info->name, info->mime_type);
}

void anjuta_project_node_info_free (AnjutaProjectNodeInfo *info)
{
	g_slice_free (AnjutaProjectNodeInfo, info);
}

/* Implement Boxed type
 *---------------------------------------------------------------------------*/

GType
anjuta_project_node_info_get_type ()
{
	static GType type_id = 0;

	if (!type_id)
		type_id = g_boxed_type_register_static ("AnjutaProjectNodeInfo",
		                                        (GBoxedCopyFunc) anjuta_project_node_info_copy,
		                                        (GBoxedFreeFunc) anjuta_project_node_info_free);

	return type_id;
}
