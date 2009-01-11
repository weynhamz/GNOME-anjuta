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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef __PROPERTY_H__
#define __PROPERTY_H__

#include <config.h>

#include "values.h"

#include <glib.h>
#include <gtk/gtk.h>


typedef struct _NPWProperty NPWProperty;
typedef struct _NPWPage NPWPage;
typedef struct _NPWItem NPWItem;

/* You should update the NPWPropertyTypeString array in the .c file,
 * after changing the NPWPropertyType enum */
typedef enum {
	NPW_UNKNOWN_PROPERTY = 0,
	NPW_HIDDEN_PROPERTY,
	NPW_BOOLEAN_PROPERTY,
	NPW_INTEGER_PROPERTY,
	NPW_STRING_PROPERTY,
	NPW_LIST_PROPERTY,
	NPW_DIRECTORY_PROPERTY,
	NPW_FILE_PROPERTY,
	NPW_ICON_PROPERTY,
	NPW_LAST_PROPERTY
} NPWPropertyType;

/* You should update the NPWPropertyRestrictionString array in the .c file,
 * after changing the NPWPropertyRestriction enum */
typedef enum {
	NPW_NO_RESTRICTION = 0,
	NPW_FILENAME_RESTRICTION,
	NPW_LAST_RESTRICTION
} NPWPropertyRestriction;

typedef enum {
	NPW_MANDATORY_OPTION = 1 << 0,
	NPW_SUMMARY_OPTION = 1 << 1,
	NPW_EDITABLE_OPTION = 1 << 2,
	NPW_EXIST_OPTION = 1 << 3,
	NPW_EXIST_SET_OPTION = 1 << 4
} NPWPropertyOptions;

typedef enum {
	NPW_DEFAULT = -1,
	NPW_FALSE = 0,
	NPW_TRUE = 1
} NPWPropertyBooleanValue;

NPWProperty* npw_property_new (void);
void npw_property_free (NPWProperty* prop);

void npw_property_set_type (NPWProperty* prop, NPWPropertyType type);
void npw_property_set_string_type (NPWProperty* prop, const gchar* type);
NPWPropertyType npw_property_get_type (const NPWProperty* prop);

void npw_property_set_restriction (NPWProperty* prop, NPWPropertyRestriction restriction);
void npw_property_set_string_restriction (NPWProperty* prop, const gchar* restriction);
NPWPropertyRestriction npw_property_get_restriction (const NPWProperty* prop);
gboolean npw_property_is_valid_restriction (const NPWProperty* prop);

void npw_property_set_name (NPWProperty* prop, const gchar* name, NPWPage *page);
const gchar* npw_property_get_name (const NPWProperty* prop);

void npw_property_set_label (NPWProperty* prop, const gchar* name);
const gchar* npw_property_get_label (const NPWProperty* prop);

void npw_property_set_description (NPWProperty* prop, const gchar* description);
const gchar* npw_property_get_description (const NPWProperty* prop);

GtkWidget* npw_property_create_widget (NPWProperty* prop);
void npw_property_set_widget (NPWProperty* prop, GtkWidget* widget);
GtkWidget* npw_property_get_widget (const NPWProperty* prop);

void npw_property_set_default (NPWProperty* prop, const gchar* value);

gboolean npw_property_update_value_from_widget (NPWProperty* prop);
gboolean npw_property_save_value_from_widget (NPWProperty* prop);
gboolean npw_property_remove_value (NPWProperty* prop);
const char* npw_property_get_value (const NPWProperty* prop);

gboolean npw_property_add_list_item (NPWProperty* prop, const char* name, const gchar* label);

void npw_property_set_mandatory_option (NPWProperty* prop, gboolean value);
void npw_property_set_summary_option (NPWProperty* prop, gboolean value);
void npw_property_set_editable_option (NPWProperty* prop, gboolean value);
NPWPropertyOptions npw_property_get_options (const NPWProperty* prop);

void npw_property_set_exist_option (NPWProperty* prop, NPWPropertyBooleanValue value);
NPWPropertyBooleanValue npw_property_get_exist_option (const NPWProperty* prop);


NPWPage* npw_page_new (GHashTable* value);
void npw_page_free (NPWPage* page);

void npw_page_set_name (NPWPage* page, const gchar* name);
const gchar* npw_page_get_name (const NPWPage* page);

void npw_page_set_label (NPWPage* page, const gchar* name);
const gchar* npw_page_get_label (const NPWPage* page);

void npw_page_set_description (NPWPage* page, const gchar* name);
const gchar* npw_page_get_description (const NPWPage* page);

void npw_page_set_widget (NPWPage* page, GtkWidget *widget);
GtkWidget *npw_page_get_widget (const NPWPage *page);

void npw_page_foreach_property (const NPWPage* page, GFunc func, gpointer data);
void npw_page_add_property (NPWPage* page, NPWProperty* prop);

#endif
