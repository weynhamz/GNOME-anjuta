/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    header.h
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

#ifndef __HEADER_H__
#define __HEADER_H__

#include <glib.h>

typedef struct _NPWHeader NPWHeader;
typedef struct _NPWHeaderList NPWHeaderList;

NPWHeader* npw_header_new (NPWHeaderList* owner);
void npw_header_free (NPWHeader* this);

void npw_header_set_name (NPWHeader* this, const gchar* name);
const gchar* npw_header_get_name (const NPWHeader* this);

void npw_header_set_filename (NPWHeader* this, const gchar* filename);
const gchar* npw_header_get_filename (const NPWHeader* this);

void npw_header_set_category (NPWHeader* this, const gchar* category);
const gchar* npw_header_get_category(const NPWHeader* this);

void npw_header_set_description (NPWHeader* this, const gchar* description);
const gchar* npw_header_get_description (const NPWHeader* this);

void npw_header_set_iconfile (NPWHeader* this, const gchar* confile);
const gchar* npw_header_get_iconfile (const NPWHeader* this);

gboolean npw_header_is_leaf(const NPWHeader* this);

NPWHeaderList* npw_header_list_new (void);

NPWHeaderList* npw_header_list_new (void);
void npw_header_list_free (NPWHeaderList* this);

void npw_header_list_organize(NPWHeaderList* this, const gchar* category, NPWHeader* header);

typedef void (*NPWHeaderForeachFunc) (NPWHeader* head, gpointer data);

gboolean npw_header_list_foreach_project (const NPWHeaderList* this, NPWHeaderForeachFunc func, gpointer data);
gboolean npw_header_list_foreach_project_in (const NPWHeaderList* this, const gchar* category, NPWHeaderForeachFunc func, gpointer data);
gboolean npw_header_list_foreach_category (const NPWHeaderList* this, NPWHeaderForeachFunc func, gpointer data);

#endif
