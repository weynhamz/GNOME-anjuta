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

// Properties of a project

typedef struct _NPWPropertyValues NPWPropertyValues;
typedef GSList* NPWPropertyKey;

//  Property list

NPWPropertyValues* npw_property_values_new (void);
void npw_property_values_destroy (NPWPropertyValues* this);

NPWPropertyKey npw_property_values_add (NPWPropertyValues* this, const gchar* name);
const gchar* npw_property_values_get_name (const NPWPropertyValues* this, NPWPropertyKey key);
void npw_property_values_set (NPWPropertyValues* this, NPWPropertyKey key, const gchar* value, gint tag);
const gchar* npw_property_values_get (const NPWPropertyValues* this, NPWPropertyKey key);
gint npw_property_values_get_tag (const NPWPropertyValues* this, NPWPropertyKey key);

typedef void (*NPWPropertyValuesForeachFunc) (const gchar* name, const gchar* value, gint tag, gpointer data);
void npw_property_values_foreach_property (const NPWPropertyValues* this, NPWPropertyValuesForeachFunc func, gpointer data);

#endif

