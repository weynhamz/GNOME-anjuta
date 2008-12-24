/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    values.h
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

#ifndef __VALUES_H__
#define __VALUES_H__

#include <glib.h>

typedef struct _NPWValue NPWValue;

typedef enum {
	NPW_EMPTY_VALUE = 0, 		/* value should be NULL */
	NPW_VALID_VALUE = 1 << 0,	/* non empty, valid value */ 
	NPW_OLD_VALUE = 1 << 1,		/* non empty, removed value */
	NPW_DEFAULT_VALUE = 1 << 2  	/* default value = could be overwritten */
} NPWValueTag;

GHashTable* npw_value_heap_new (void);
void npw_value_heap_free (GHashTable* hash);

NPWValue* npw_value_heap_find_value (GHashTable* hash, const gchar* name);
void npw_value_heap_foreach_value (GHashTable* hash, GHFunc func, gpointer user_data);

gboolean npw_value_set_value (NPWValue* node, const gchar* value, NPWValueTag tag);
const gchar* npw_value_get_value (const NPWValue* node);
const gchar* npw_value_get_name (const NPWValue* node);
NPWValueTag npw_value_get_tag (const NPWValue* node);

#endif

