/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    property.h
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

#ifndef __PROPERTY_H__
#define __PROPERTY_H__

#include <config.h>

#include "values.h"

#include <glib.h>
#include <gtk/gtk.h>


// Project wizard property, used in middle page

typedef struct _NPWProperty NPWProperty;
typedef struct _NPWPage NPWPage;

// Property

typedef enum {
	NPW_HIDDEN_PROPERTY,
	NPW_BOOLEAN_PROPERTY,
	NPW_INTEGER_PROPERTY,
	NPW_STRING_PROPERTY,
	NPW_DIRECTORY_PROPERTY,
	NPW_FILE_PROPERTY
} NPWPropertyType;

typedef enum {
	NPW_MANDATORY_OPTION = 1 << 0,
	NPW_SUMMARY_OPTION = 1 << 1
} NPWPropertyOptions;

NPWProperty* npw_property_new(NPWPage* owner, NPWPropertyType type);
void npw_property_destroy(NPWProperty* this);

NPWPropertyType npw_property_get_type(const NPWProperty* this);

void npw_property_set_name(NPWProperty* this, const gchar* name);
const gchar* npw_property_get_name(const NPWProperty* this);

void npw_property_set_label(NPWProperty* this, const gchar* name);
const gchar* npw_property_get_label(const NPWProperty* this);

void npw_property_set_description(NPWProperty* this, const gchar* description);
const gchar* npw_property_get_description(const NPWProperty* this);

void npw_property_set_widget(NPWProperty* this, GtkWidget* widget);
GtkWidget* npw_property_get_widget(const NPWProperty* this);

void npw_property_set_default(NPWProperty* this, const gchar* value);

void npw_property_set_value(NPWProperty* this, const gchar* value, gint tag);
const char* npw_property_get_value(const NPWProperty* this);

void npw_property_set_mandatory_option(NPWProperty* this, gboolean value);
void npw_property_set_summary_option(NPWProperty* this, gboolean value);
NPWPropertyOptions npw_property_get_options(const NPWProperty* this);

// Page = list of properties

NPWPage* npw_page_new(NPWPropertyValues* value);

void npw_page_destroy(NPWPage* this);

typedef void (*NPWPropertyForeachFunc) (NPWProperty* head, gpointer data);

void npw_page_set_name(NPWPage* this, const gchar* name);
const gchar* npw_page_get_name(const NPWPage* this);

void npw_page_set_label(NPWPage* this, const gchar* name);
const gchar* npw_page_get_label(const NPWPage* this);

void npw_page_set_description(NPWPage* this, const gchar* name);
const gchar* npw_page_get_description(const NPWPage* this);

void npw_page_foreach_property(const NPWPage* this, NPWPropertyForeachFunc func, gpointer data);

#endif
