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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __VALUES_H__
#define __VALUES_H__

#include <glib.h>

typedef struct _NPWValueHeap NPWValueHeap;
typedef struct _NPWValue NPWValue;

typedef enum {
	NPW_EMPTY_VALUE = 0, 		/* value should be NULL */
			  	    	/* The two following states are exclusive */
	NPW_VALID_VALUE = 1 << 0,	/* non empty, active value */ 
	NPW_OLD_VALUE = 1 << 1,		/* non empty, removed value */

	NPW_DEFAULT_VALUE = 1 << 2  	/* default value = could be overwritten */
} NPWValueTag;

NPWValueHeap* npw_value_heap_new (void);
void npw_value_heap_free (NPWValueHeap* this);

NPWValue* npw_value_heap_find_value (NPWValueHeap* this, const gchar* name);

void npw_value_heap_set_value (NPWValueHeap* this, NPWValue* node, const gchar* value, NPWValueTag tag);
const gchar* npw_value_heap_get_value (const NPWValueHeap* this, const NPWValue* node);

const gchar* npw_value_heap_get_name (const NPWValueHeap* this, const NPWValue* node);
NPWValueTag npw_value_heap_get_tag (const NPWValueHeap* this, const NPWValue* node);

typedef void (*NPWValueHeapForeachFunc) (const gchar* name, const gchar* value, NPWValueTag tag, gpointer user_data);
void npw_value_heap_foreach_value (const NPWValueHeap* this, NPWValueHeapForeachFunc func, gpointer user_data);

#endif

