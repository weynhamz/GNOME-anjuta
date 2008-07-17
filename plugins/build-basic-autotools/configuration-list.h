/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    configuration-list.h
    Copyright (C) 2008 Sébastien Granjoux

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
 
#ifndef CONFIGURATION_LIST_H
#define CONFIGURATION_LIST_H

#include <glib.h>

typedef struct _BuildConfiguration BuildConfiguration;
typedef struct _BuildConfigurationList BuildConfigurationList;

BuildConfigurationList* build_configuration_list_new (void);
void build_configuration_list_free (BuildConfigurationList *list);

void build_configuration_list_from_string_list (BuildConfigurationList *list, GList *str_list);
GList *build_configuration_list_to_string_list (BuildConfigurationList *list);

BuildConfiguration *build_configuration_list_get_first (BuildConfigurationList *list);
BuildConfiguration *build_configuration_list_get_selected (BuildConfigurationList *list);
gint build_configuration_list_get_position (BuildConfigurationList *list, BuildConfiguration *cfg);
BuildConfiguration *build_configuration_next (BuildConfiguration *cfg);
BuildConfiguration *build_configuration_list_get (BuildConfigurationList *list, const gchar *name);
BuildConfiguration *build_configuration_list_select (BuildConfigurationList *list, const gchar *name);

void build_configuration_list_set_project_uri (BuildConfigurationList *list, const gchar *uri);
const gchar *build_configuration_get_translated_name (BuildConfiguration *cfg);
const gchar *build_configuration_get_name (BuildConfiguration *cfg);
gboolean build_configuration_list_set_build_uri (BuildConfigurationList *list, BuildConfiguration *cfg, const gchar *build_uri);
gchar *build_configuration_list_get_build_uri (BuildConfigurationList *list, BuildConfiguration *cfg);
const gchar *build_configuration_get_relative_build_uri (BuildConfiguration *cfg);
void build_configuration_set_args (BuildConfiguration *cfg, const gchar *args);
gchar **build_configuration_get_args (BuildConfiguration *cfg);

#endif /* CONFIGURATION_LIST_H */
