/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    header.c
    Copyright (C) 2004 Sebastien Granjoux

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
//	Project header data

#include <config.h>

#include "header.h"

#include <glib/gdir.h>

#define STRING_CHUNK_SIZE	256

/*---------------------------------------------------------------------------*/

struct _NPWHeaderList
{
	GNode* list;
	GStringChunk* string_pool;
	GMemChunk* data_pool;
};

struct _NPWHeader {
	gchar* name;
	gchar* description;
	gchar* iconfile;
	gchar* category;
	gchar* filename;
	NPWHeaderList* owner;
	GNode* node;
};

/* Header
 *---------------------------------------------------------------------------*/

NPWHeader*
npw_header_new (NPWHeaderList* owner)
{
	NPWHeader* this;

	g_return_val_if_fail (owner, NULL);

	this = g_chunk_new0(NPWHeader, owner->data_pool);
	this->owner = owner;
	this->node = g_node_append_data (owner->list, this);

	return this;
}

void
npw_header_destroy (NPWHeader* this)
{
	GNode* node;

	node = g_node_find_child (this->owner->list, G_TRAVERSE_ALL, this);
	if (node != NULL)
	{
		g_node_destroy (node);
		// Memory allocated in string pool and project pool is not free
	}
}

void
npw_header_set_name (NPWHeader* this, const gchar* name)
{
	this->name = g_string_chunk_insert (this->owner->string_pool, name);
}

const gchar*
npw_header_get_name (const NPWHeader* this)
{
	return this->name;
}

void
npw_header_set_filename (NPWHeader* this, const gchar* filename)
{
	this->filename = g_string_chunk_insert (this->owner->string_pool, filename);
}

const gchar*
npw_header_get_filename (const NPWHeader* this)
{
	return this->filename;
}

void
npw_header_set_category (NPWHeader* this, const gchar* category)
{
	npw_header_list_organize(this->owner, category, this);
	this->category = ((NPWHeader *)this->node->parent->data)->category;
}

const gchar*
npw_header_get_category (const NPWHeader* this)
{
	return this->category;
}

void
npw_header_set_description (NPWHeader* this, const gchar* description)
{
	this->description = g_string_chunk_insert (this->owner->string_pool, description);
}

const gchar*
npw_header_get_description (const NPWHeader* this)
{
	return this->description;
}

void
npw_header_set_iconfile (NPWHeader* this, const gchar* iconfile)
{
	this->iconfile = g_string_chunk_insert (this->owner->string_pool, iconfile);
}

const gchar*
npw_header_get_iconfile (const NPWHeader* this)
{
	return this->iconfile;
}

gboolean
npw_header_is_leaf(const NPWHeader* this)
{
	return G_NODE_IS_LEAF(this->node);
}


/* Header list
 *---------------------------------------------------------------------------*/

static GNode*
npw_header_list_find_parent(const NPWHeaderList* this, const gchar* category)
{
	GNode* node;

	for (node = g_node_first_child(this->list); node != NULL; node = g_node_next_sibling(node))
	{
		const char* s;

		s = ((NPWHeader *)node->data)->category;
		if ((s != NULL) && (strcmp(s, category) == 0))
		{
			break;
		}
	}

	return node;
}

typedef struct _HeaderListForeachProjectData
{
	NPWHeaderForeachFunc func;
	gpointer data;
} HeaderListForeachProjectData;

static void
cb_header_list_foreach_project (GNode* node, gpointer data)
{
	HeaderListForeachProjectData* d = (HeaderListForeachProjectData *)data;

	(d->func)((NPWHeader*)node->data, d->data);
}

static gboolean
npw_header_list_foreach_node (GNode* list, NPWHeaderForeachFunc func, gpointer data, GTraverseFlags flag)
{
	HeaderListForeachProjectData d;

	if (g_node_first_child (list) == NULL) return FALSE;

	d.func = func;
	d.data = data;
				     
	g_node_children_foreach (list, flag, cb_header_list_foreach_project, &d);

	return TRUE;
}
	
/*---------------------------------------------------------------------------*/

NPWHeaderList*
npw_header_list_new (void)
{
	NPWHeaderList* this;

	this = g_new (NPWHeaderList, 1);
	this->string_pool = g_string_chunk_new (STRING_CHUNK_SIZE);
	this->data_pool = g_mem_chunk_new ("project pool", sizeof (NPWHeader), STRING_CHUNK_SIZE * sizeof (NPWHeader) / 4, G_ALLOC_ONLY);
	this->list = g_node_new (NULL);

	return this;
}

void
npw_header_list_destroy (NPWHeaderList* this)
{
	g_return_if_fail (this != NULL);

	g_string_chunk_free (this->string_pool);
	g_mem_chunk_destroy (this->data_pool);
	g_node_destroy (this->list);
	g_free (this);
}

void
npw_header_list_organize(NPWHeaderList* this, const gchar* category, NPWHeader* header)
{
	GNode* node;

	/* node without a category stay on top */
	if ((category == NULL) || (*category == '\0')) return;
	
	/* Detach the node */
	g_node_unlink(header->node);

	/* Find parent */
	node = npw_header_list_find_parent(this, category);
	if (node == NULL)
	{
		NPWHeader* new_parent;

		/* Create a new parent */
		new_parent = npw_header_new(this);
		new_parent->category = g_string_chunk_insert (this->string_pool, category);
		node = new_parent->node;
	}
	
	/* Insert node as a child */
	g_node_insert(node, -1, header->node);
}

gboolean
npw_header_list_foreach_project (const NPWHeaderList* this, NPWHeaderForeachFunc func, gpointer data)
{
	return npw_header_list_foreach_node (this->list, func, data, G_TRAVERSE_LEAFS);
}
	
gboolean
npw_header_list_foreach_project_in (const NPWHeaderList* this, const gchar* category, NPWHeaderForeachFunc func, gpointer data)
{
	GNode* node;

	node = npw_header_list_find_parent(this, category);
	if (node == NULL) return FALSE;

	return npw_header_list_foreach_node (node, func, data, G_TRAVERSE_LEAFS);
}

gboolean
npw_header_list_foreach_category (const NPWHeaderList* this, NPWHeaderForeachFunc func, gpointer data)
{
	return npw_header_list_foreach_node (this->list, func, data, G_TRAVERSE_NON_LEAFS);
}
