/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    property.c
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

#include "property.h"

#include <glib/gdir.h>

#define STRING_CHUNK_SIZE	256

// Property and Page

struct _NPWPage
{
	GNode* list;
	GStringChunk* string_pool;
	GMemChunk* data_pool;
	NPWPropertyValues* value;
	gchar* name;
	gchar* label;
	gchar* description;
};

struct _NPWProperty {
	NPWPropertyType type;
	NPWPropertyOptions options;
	gchar* label;
	gchar* description;
	gchar* defvalue;
	NPWPropertyKey key;
	GtkWidget* widget;
	NPWPage* owner;
};

// Property

NPWProperty*
npw_property_new(NPWPage* owner, NPWPropertyType type)
{
	NPWProperty* this;

	g_return_val_if_fail(owner, NULL);

	this = g_chunk_new0(NPWProperty, owner->data_pool);
	this->owner = owner;
	this->type = type;
	// value is set to NULL
	g_node_append_data(owner->list, this);

	return this;
}

void
npw_property_destroy(NPWProperty* this)
{
	GNode* node;

	node = g_node_find_child(this->owner->list, G_TRAVERSE_ALL, this);
	if (node != NULL)
	{
		g_node_destroy(node);
		// Memory allocated in string pool and data pool is not free
	}
}

NPWPropertyType
npw_property_get_type(const NPWProperty* this)
{
	return this->type;
}

void
npw_property_set_name(NPWProperty* this, const gchar* name)
{
	this->key = npw_property_values_add(this->owner->value, name);
}

const gchar*
npw_property_get_name(const NPWProperty* this)
{
	return npw_property_values_get_name(this->owner->value, this->key);
}

void
npw_property_set_label(NPWProperty* this, const gchar* label)
{
	this->label = g_string_chunk_insert(this->owner->string_pool, label);
}

const gchar*
npw_property_get_label(const NPWProperty* this)
{
	return this->label;
}

void
npw_property_set_description(NPWProperty* this, const gchar* description)
{
	this->description = g_string_chunk_insert(this->owner->string_pool, description);
}

const gchar*
npw_property_get_description(const NPWProperty* this)
{
	return this->description;
}

void
npw_property_set_widget(NPWProperty* this, GtkWidget* widget)
{
	this->widget = widget;
}

GtkWidget*
npw_property_get_widget(const NPWProperty* this)
{
	return this->widget;
}

void
npw_property_set_default(NPWProperty* this, const gchar* value)
{
	this->defvalue = g_string_chunk_insert(this->owner->string_pool, value);
}

void
npw_property_set_value(NPWProperty* this, const gchar* value, gint tag)
{
	npw_property_values_set(this->owner->value, this->key, value, tag);
}

const char*
npw_property_get_value(const NPWProperty* this)
{
	if (npw_property_values_get_tag(this->owner->value, this->key) == 0)
	{
		return this->defvalue;
	}
	else
	{
		return npw_property_values_get(this->owner->value, this->key);
	}	
}

void
npw_property_set_mandatory_option(NPWProperty* this, gboolean value)
{
	if (value)
	{
		this->options |= NPW_MANDATORY_OPTION;
	}
	else
	{
		this->options &= ~NPW_MANDATORY_OPTION;
	}
}

void
npw_property_set_summary_option(NPWProperty* this, gboolean value)
{
	if (value)
	{
		this->options |= NPW_SUMMARY_OPTION;
	}
	else
	{
		this->options &= ~NPW_SUMMARY_OPTION;
	}
}

NPWPropertyOptions
npw_property_get_options(const NPWProperty* this)
{
	return this->options;
}

// Page

NPWPage*
npw_page_new(NPWPropertyValues* value)
{
	NPWPage* this;

	this = g_new0(NPWPage, 1);
	this->string_pool = g_string_chunk_new(STRING_CHUNK_SIZE);
	this->data_pool = g_mem_chunk_new("property pool", sizeof(NPWProperty), STRING_CHUNK_SIZE * sizeof(NPWProperty) / 4, G_ALLOC_ONLY);
	this->list = g_node_new(NULL);
	this->value = value;

	return this;
}

void
npw_page_destroy(NPWPage* this)
{
	g_return_if_fail(this != NULL);

	g_string_chunk_free(this->string_pool);
	g_mem_chunk_destroy(this->data_pool);
	g_node_destroy(this->list);
	g_free(this);
}

void
npw_page_set_name(NPWPage* this, const gchar* name)
{
	this->name = g_string_chunk_insert(this->string_pool, name);
}

const gchar*
npw_page_get_name(const NPWPage* this)
{
	return this->name;
}

void
npw_page_set_label(NPWPage* this, const gchar* label)
{
	this->label = g_string_chunk_insert(this->string_pool, label);
}

const gchar*
npw_page_get_label(const NPWPage* this)
{
	return this->label;
}

void
npw_page_set_description(NPWPage* this, const gchar* description)
{
	this->description = g_string_chunk_insert(this->string_pool, description);
}

const gchar*
npw_page_get_description(const NPWPage* this)
{
	return this->description;
}

typedef struct _PageForeachPropertyData
{
	NPWPropertyForeachFunc func;
	gpointer data;
} PageForeachPropertyData;

static void
cb_page_foreach_property(GNode* node, gpointer data)
{
	PageForeachPropertyData* d = (PageForeachPropertyData *)data;

	(d->func)((NPWProperty*)node->data, d->data);
}

void
npw_page_foreach_property(const NPWPage* this, NPWPropertyForeachFunc func, gpointer data)
{
	PageForeachPropertyData d;

	d.func = func;
	d.data = data;
	g_node_children_foreach(this->list, G_TRAVERSE_LEAFS, cb_page_foreach_property, &d);
}
