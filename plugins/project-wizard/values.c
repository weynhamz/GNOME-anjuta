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
//	Project property data

#include <config.h>
#include <string.h>

#include "values.h"

#define STRING_CHUNK_SIZE	256

struct _NPWPropertyValues
{
	GHashTable* hash;
	GSList* list;
	GStringChunk* string_pool;
};

typedef struct _NPWValue
{
	gint tag;
	gboolean empty;
	const gchar* name;
	gchar value[0];
} NPWValue;



NPWPropertyValues*
npw_property_values_new (void)
{
	NPWPropertyValues* this;

	this = g_new0(NPWPropertyValues, 1);
	this->string_pool = g_string_chunk_new (STRING_CHUNK_SIZE);
	this->hash = g_hash_table_new (g_str_hash, g_str_equal);
	this->list = NULL;

	return this;
}

void
npw_property_values_destroy (NPWPropertyValues* this)
{
	GSList* node;
	g_return_if_fail (this != NULL);

	g_string_chunk_free (this->string_pool);
	g_hash_table_destroy (this->hash);
	for (node = this->list; node != NULL; node = g_slist_next (node))
	{
		g_free (node->data);
	}
	
	g_slist_free (this->list);
	g_free (this);
}

NPWPropertyKey
npw_property_values_add (NPWPropertyValues* this, const gchar* name)
{
	gchar* key;
	gpointer value;
	NPWValue* new_value;

	if (!g_hash_table_lookup_extended (this->hash, name, (gpointer)&key, &value))
	{
		key = g_string_chunk_insert (this->string_pool, name);
		new_value = (NPWValue *)g_new (char, sizeof (NPWValue));
		new_value->name = key;
		new_value->tag = 0;
		this->list = g_slist_prepend (this->list, new_value);
		value = (gpointer)this->list;
		g_hash_table_insert (this->hash, key, value);
	}

	return value;
}

const gchar*
npw_property_values_get_name (const NPWPropertyValues* this, NPWPropertyKey key)
{
	NPWValue* new_value;

	new_value = (NPWValue *)key->data;

	return new_value->name;
}

void
npw_property_values_set (NPWPropertyValues* this, NPWPropertyKey key, const gchar* value, gint tag)
{
	NPWValue* new_value;
	const gchar* name;

	new_value = (NPWValue *)key->data;
	name = new_value->name;
	g_free (new_value);

	new_value = (NPWValue *)g_new (char, sizeof (NPWValue) + (value == NULL ? 0: strlen (value)) + 1);
	new_value->name = name;
	new_value->tag = tag;
	if (value == NULL)
	{
		new_value->value[0] = '\0';
	}
	else
	{
		strcpy (new_value->value, value);
	}
	key->data = new_value;
}

const gchar*
npw_property_values_get (const NPWPropertyValues* this, NPWPropertyKey key)
{
	NPWValue* value;

	value = (NPWValue*)key->data;

	return value == NULL ? NULL : value->value;
}

gint
npw_property_values_get_tag (const NPWPropertyValues* this, NPWPropertyKey key)
{
	NPWValue* value;

	value = (NPWValue*)key->data;

	return value == NULL ? 0 : value->tag;
}

typedef struct _NPWPropertyValueForeachPropertyData
{
	NPWPropertyValuesForeachFunc func;
	gpointer data;
} NPWPropertyValuesForeachPropertyData;

static void
cb_property_values_foreach_property (gpointer value, gpointer data)
{
	NPWPropertyValuesForeachPropertyData* d = (NPWPropertyValuesForeachPropertyData *)data;
	NPWValue* v = (NPWValue*)value;

	(d->func)(v->name, v->value, v->tag, d->data);
}

void
npw_property_values_foreach_property (const NPWPropertyValues* this, NPWPropertyValuesForeachFunc func, gpointer data)
{
	NPWPropertyValuesForeachPropertyData d;

	d.func = func;
	d.data = data;
	g_slist_foreach (this->list, cb_property_values_foreach_property, &d);
}

