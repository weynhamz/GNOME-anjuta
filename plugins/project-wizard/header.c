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

// Header and Header List

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
};

// Header

NPWHeader*
npw_header_new(NPWHeaderList* owner)
{
	NPWHeader* this;

	g_return_val_if_fail(owner, NULL);

	this = g_chunk_new0(NPWHeader, owner->data_pool);
	this->owner = owner;
	g_node_append_data(owner->list, this);

	return this;
}

void
npw_header_destroy(NPWHeader* this)
{
	GNode* node;

	node = g_node_find_child(this->owner->list, G_TRAVERSE_ALL, this);
	if (node != NULL)
	{
		g_node_destroy(node);
		// Memory allocated in string pool and project pool is not free
	}
}

void
npw_header_set_name(NPWHeader* this, const gchar* name)
{
	this->name = g_string_chunk_insert(this->owner->string_pool, name);
}

const gchar*
npw_header_get_name(const NPWHeader* this)
{
	return this->name;
}

void
npw_header_set_filename(NPWHeader* this, const gchar* filename)
{
	this->filename = g_string_chunk_insert(this->owner->string_pool, filename);
}

const gchar*
npw_header_get_filename(const NPWHeader* this)
{
	return this->filename;
}

void
npw_header_set_category(NPWHeader* this, const gchar* category)
{
	this->category = g_string_chunk_insert(this->owner->string_pool, category);
}

void
npw_header_set_description(NPWHeader* this, const gchar* description)
{
	this->description = g_string_chunk_insert(this->owner->string_pool, description);
}

void
npw_header_set_iconfile(NPWHeader* this, const gchar* iconfile)
{
	this->iconfile = g_string_chunk_insert(this->owner->string_pool, iconfile);
}

const gchar*
npw_header_get_iconfile(const NPWHeader* this)
{
	return this->iconfile;
}



NPWHeaderList*
npw_header_list_new(void)
{
	NPWHeaderList* this;

	this = g_new(NPWHeaderList, 1);
	this->string_pool = g_string_chunk_new(STRING_CHUNK_SIZE);
	this->data_pool = g_mem_chunk_new("project pool", sizeof(NPWHeader), STRING_CHUNK_SIZE * sizeof(NPWHeader) / 4, G_ALLOC_ONLY);
	this->list = g_node_new(NULL);

	return this;
}

void
npw_header_list_destroy(NPWHeaderList* this)
{
	g_return_if_fail(this != NULL);

	g_string_chunk_free(this->string_pool);
	g_mem_chunk_destroy(this->data_pool);
	g_node_destroy(this->list);
	g_free(this);
}

typedef struct _HeaderListForeachProjectData
{
	NPWHeaderForeachFunc func;
	gpointer data;
} HeaderListForeachProjectData;

static void
cb_header_list_foreach_project(GNode* node, gpointer data)
{
	HeaderListForeachProjectData* d = (HeaderListForeachProjectData *)data;

	(d->func)((NPWHeader*)node->data, d->data);
}

void
npw_header_list_foreach_project(const NPWHeaderList* this, NPWHeaderForeachFunc func, gpointer data)
{
	HeaderListForeachProjectData d;

	d.func = func;
	d.data = data;
				     
	g_node_children_foreach(this->list, G_TRAVERSE_LEAFS, cb_header_list_foreach_project, &d);
}
