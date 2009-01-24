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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef __HEADER_H__
#define __HEADER_H__

#include <glib.h>

typedef struct _NPWHeader NPWHeader;
typedef struct _NPWHeaderList NPWHeaderList;

NPWHeader* npw_header_new (void);
void npw_header_free (NPWHeader* self);

void npw_header_set_name (NPWHeader* this, const gchar* name);
const gchar* npw_header_get_name (const NPWHeader* self);

void npw_header_set_filename (NPWHeader* this, const gchar* filename);
const gchar* npw_header_get_filename (const NPWHeader* self);

void npw_header_set_category (NPWHeader* this, const gchar* category);
const gchar* npw_header_get_category(const NPWHeader* self);

void npw_header_set_description (NPWHeader* this, const gchar* description);
const gchar* npw_header_get_description (const NPWHeader* self);

void npw_header_set_iconfile (NPWHeader* this, const gchar* confile);
const gchar* npw_header_get_iconfile (const NPWHeader* self);

void npw_header_add_required_program (NPWHeader* self, const gchar* program);

/* Returns list of missing programs, or NULL if none is missing
 * Only the glist should be freed, not the strings within
 */
GList* npw_header_check_required_programs (NPWHeader* self);

void npw_header_add_required_package (NPWHeader* self, const gchar* package);

/* Returns list of missing packages, or NULL if none is missing
 * Only the glist should be freed, not the strings within
 */
GList* npw_header_check_required_packages (NPWHeader* self);


GList* npw_header_list_new (void);
void npw_header_list_free (GList* list);
GList * npw_header_list_insert_header (GList *list, NPWHeader *header);
NPWHeader* npw_header_list_find_header (GList *list, NPWHeader *header);

#endif
