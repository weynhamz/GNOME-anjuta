/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    values.c
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

/*
 * Store all project property values (= just the name and the value
 * without other high level information like the associated widget)
 *
 * The property values are stored independently because we want to keep
 * all values entered by the user even if it goes back in the wizard and
 * regenerate all properties.
 * Moreover it removes dependencies between the installer part and the
 * properties. The installer part could work with the values only.
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "values.h"

#include <string.h>

/*---------------------------------------------------------------------------*/

#define STRING_CHUNK_SIZE	256

/*---------------------------------------------------------------------------*/

/* This stores all the values. It's basically an hash table with a string
 * and a mem chunk to allocate all memories inside the object.
 * After adding a new property, there is no way to remove it, but you can
 * use the tag to mark it as obsolete */

struct _NPWValueHeap
{
	GHashTable* hash;
	GStringChunk* string_pool;
	GMemChunk* value_pool;	
};

/* One property value, so just a name and a value plus a tag !
 * The tag is defined in the header file, but is not really used in this code */

struct _NPWValue
{
	NPWValueTag tag;
	const gchar* name;
	const gchar* value;
};

/* Creation and Destruction
 *---------------------------------------------------------------------------*/

static NPWValueHeap*
npw_value_heap_initialize (NPWValueHeap* this)
{
	this->string_pool = g_string_chunk_new (STRING_CHUNK_SIZE);
	this->value_pool = g_mem_chunk_new ("value pool", sizeof(NPWValue), (STRING_CHUNK_SIZE / 4) * sizeof(NPWValue), G_ALLOC_ONLY);
	this->hash = g_hash_table_new (g_str_hash, g_str_equal);

	return this;
}

static void
npw_value_heap_deinitialize (NPWValueHeap* this)
{
	g_string_chunk_free (this->string_pool);
	g_mem_chunk_destroy (this->value_pool);
	g_hash_table_destroy (this->hash);
}


NPWValueHeap*
npw_value_heap_new (void)
{
	NPWValueHeap* this;

	this = g_new0 (NPWValueHeap, 1);
	return npw_value_heap_initialize (this);
}

void
npw_value_heap_free (NPWValueHeap* this)
{
	/* GSList* node; */
	g_return_if_fail (this != NULL);

	npw_value_heap_deinitialize (this);
	g_free (this);
}

/* Find a value or list all values
 *---------------------------------------------------------------------------*/

/* Return key corresponding to name, create key if it doesn't exist */

NPWValue*
npw_value_heap_find_value (NPWValueHeap* this, const gchar* name)
{
	NPWValue* node;

	if (!g_hash_table_lookup_extended (this->hash, name, NULL, (gpointer)&node))
	{
		gchar* new_name;

		node = g_chunk_new (NPWValue, this->value_pool);
		new_name = g_string_chunk_insert (this->string_pool, name);
		node->name = new_name;
		node->tag = NPW_EMPTY_VALUE;
		node->value = NULL;
		g_hash_table_insert (this->hash, new_name, node);
	}

	return node;
}

typedef struct _NPWValueHeapForeachValueData
{
	NPWValueHeapForeachFunc func;
	gpointer data;
} NPWValueHeapForeachValueData;

static void
cb_value_heap_foreach_value (gpointer key, gpointer value, gpointer user_data)
{
	NPWValueHeapForeachValueData* data = (NPWValueHeapForeachValueData *)user_data;
	NPWValue* node = (NPWValue*)value;

	(data->func)(node->name, node->value, node->tag, data->data);
}

void
npw_value_heap_foreach_value (const NPWValueHeap* this, NPWValueHeapForeachFunc func, gpointer user_data)
{
	NPWValueHeapForeachValueData data;

	data.func = func;
	data.data = user_data;
	g_hash_table_foreach (this->hash, cb_value_heap_foreach_value, &data);
}

/* Access value attributes
 *---------------------------------------------------------------------------*/

const gchar*
npw_value_heap_get_name (const NPWValueHeap* this, const NPWValue* node)
{
	g_return_val_if_fail (node != NULL, NULL);

	return node->name;
}

/* set new value, return FALSE if value has not changed */

gboolean
npw_value_heap_set_value (NPWValueHeap* this, NPWValue* node, const gchar* value, NPWValueTag tag)
{
	gboolean change = FALSE;

	g_return_val_if_fail (node != NULL, FALSE);

	if (tag == NPW_EMPTY_VALUE)
	{
		if (node->tag != NPW_EMPTY_VALUE)
		{
			node->tag = NPW_EMPTY_VALUE;
			change = TRUE;
		}
	}
	else
	{
		/* Set value */
		if (value == NULL)
		{
			if (node->value != NULL)
			{
				node->value = NULL;
				change = TRUE;
			}
		}
		else
		{
			if ((node->value == NULL) || (strcmp (node->value, value) != 0))
			{
				node->value = g_string_chunk_insert (this->string_pool, value);
				change = TRUE;
			}
		}
		/* Set tag */
		if (change == TRUE)
		{
			/* remove valid tag if value change */
			node->tag &= ~NPW_VALID_VALUE;
		}
		else if ((tag & NPW_VALID_VALUE) != (node->tag & NPW_VALID_VALUE))
		{
			/* Changing valid tag count as a change */
			change = TRUE;
		}
		node->tag &= NPW_VALID_VALUE;	
		node->tag |= tag;
	}

	return change;
}

const gchar*
npw_value_heap_get_value (const NPWValueHeap* this, const NPWValue* node)
{
	g_return_val_if_fail (node != NULL, NULL);
	
	return node->tag == NPW_EMPTY_VALUE ? NULL : node->value;
}

NPWValueTag
npw_value_heap_get_tag (const NPWValueHeap* this, const NPWValue* node)
{
	g_return_val_if_fail (node != NULL, NPW_EMPTY_VALUE);

	return node->tag;
}


