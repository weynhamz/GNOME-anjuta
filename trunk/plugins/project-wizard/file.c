/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    file.c
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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
 * File data read in .wiz file
 *
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "file.h"

#include <glib/gdir.h>

/*---------------------------------------------------------------------------*/

#define STRING_CHUNK_SIZE	256

/*---------------------------------------------------------------------------*/

struct _NPWFileList
{
	GNode* list;
	GStringChunk* string_pool;
	GMemChunk* data_pool;
};

struct _NPWFile {
NPWFileType type;
	gchar* source;
	gchar* destination;
	gint attribute;
	NPWFileList* owner;
	GNode* node;
};

typedef enum {
	NPW_EXECUTE_FILE = 1 << 0,
	NPW_PROJECT_FILE = 1 << 1,
	NPW_AUTOGEN_SET = 1 << 2,
	NPW_AUTOGEN_FILE = 1 << 3
} NPWFileAttributes;

/* File object
 *---------------------------------------------------------------------------*/

NPWFile*
npw_file_new (NPWFileList* owner)
{
	NPWFile* this;

	g_return_val_if_fail (owner, NULL);

	this = g_chunk_new0(NPWFile, owner->data_pool);
	this->owner = owner;
	this->node = g_node_append_data (owner->list, this);
	this->attribute = 0;
	
	return this;
}

void
npw_file_free (NPWFile* this)
{
	GNode* node;

	node = g_node_find_child (this->owner->list, G_TRAVERSE_ALL, this);
	if (node != NULL)
	{
		g_node_destroy (node);
		/* Memory allocated in string pool and project pool is not free */
	}
}

void
npw_file_set_type (NPWFile* this, NPWFileType type)
{
	this->type = type;
}

NPWFileType
npw_file_get_type (const NPWFile* this)
{
	return this->type;
}

void
npw_file_set_destination (NPWFile* this, const gchar* destination)
{
	this->destination = g_string_chunk_insert (this->owner->string_pool, destination);
}

const gchar*
npw_file_get_destination (const NPWFile* this)
{
	return this->destination;
}

void
npw_file_set_source (NPWFile* this, const gchar* source)
{
	this->source = g_string_chunk_insert (this->owner->string_pool, source);
}

const gchar*
npw_file_get_source (const NPWFile* this)
{
	return this->source;
}

void
npw_file_set_execute (NPWFile* this, gboolean value)
{
	if (value)
	{
		this->attribute |= NPW_EXECUTE_FILE;
	}
	else
	{
		this->attribute &= ~NPW_EXECUTE_FILE;
	}
}

gboolean
npw_file_get_execute (const NPWFile* this)
{
	return this->attribute & NPW_EXECUTE_FILE;
}

void
npw_file_set_project (NPWFile* this, gboolean value)
{
	if (value)
	{
		this->attribute |= NPW_PROJECT_FILE;
	}
	else
	{
		this->attribute &= ~NPW_PROJECT_FILE;
	}
}

gboolean
npw_file_get_project (const NPWFile* this)
{
	return this->attribute & NPW_PROJECT_FILE;
}

void
npw_file_set_autogen (NPWFile* this, NPWFileBooleanValue value)
{
	switch (value)
	{
	case NPW_FILE_TRUE:
		this->attribute |= NPW_AUTOGEN_FILE | NPW_AUTOGEN_SET;
		break;
	case NPW_FILE_FALSE:
		this->attribute |= NPW_AUTOGEN_SET;
		this->attribute &= ~NPW_AUTOGEN_FILE;
		break;
	case NPW_FILE_DEFAULT:
		this->attribute &= ~(NPW_AUTOGEN_SET | NPW_AUTOGEN_FILE);
		break;
	}
}

NPWFileBooleanValue
npw_file_get_autogen (const NPWFile* this)
{
	return this->attribute & NPW_AUTOGEN_SET ? (this->attribute & NPW_AUTOGEN_FILE ? NPW_FILE_TRUE : NPW_FILE_FALSE) : NPW_FILE_DEFAULT;
}

const NPWFile*
npw_file_next (const NPWFile* this)
{
	GNode* node = this->node->next;

	return node == NULL ? NULL : (NPWFile *)node->data;
}

/* File list object
 *---------------------------------------------------------------------------*/

NPWFileList*
npw_file_list_new (void)
{
	NPWFileList* this;

	this = g_new (NPWFileList, 1);
	this->string_pool = g_string_chunk_new (STRING_CHUNK_SIZE);
	this->data_pool = g_mem_chunk_new ("file pool", sizeof (NPWFile), STRING_CHUNK_SIZE * sizeof (NPWFile) / 4, G_ALLOC_ONLY);
	this->list = g_node_new (NULL);

	return this;
}

void
npw_file_list_free (NPWFileList* this)
{
	g_return_if_fail (this != NULL);

	g_string_chunk_free (this->string_pool);
	g_mem_chunk_destroy (this->data_pool);
	g_node_destroy (this->list);
	g_free (this);
}

static void
cb_file_list_foreach_file (GNode* node, gpointer data)
{
	((NPWFileForeachFunc)data)((NPWFile*)node->data);
}

void
npw_file_list_foreach_file (const NPWFileList* this, NPWFileForeachFunc func)
{
	g_node_children_foreach (this->list, G_TRAVERSE_LEAFS, cb_file_list_foreach_file, (gpointer)func);
}

const NPWFile*
npw_file_list_first (const NPWFileList* this)
{
	/* Should work even if first child is NULL (empty list) */
	GNode* node = g_node_first_child (this->list);

	return node == NULL ? NULL : (NPWFile *)node->data;
}
