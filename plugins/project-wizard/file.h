/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    file.h
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

#ifndef __FILE_H__
#define __FILE_H__

#include <glib.h>

typedef struct _NPWFile NPWFile;
typedef struct _NPWFileList NPWFileList;

typedef enum {
	NPW_FILE,
	NPW_DIRECTORY
} NPWFileType;

typedef enum {
	NPW_FILE_DEFAULT = -1,
	NPW_FILE_FALSE = 0,
	NPW_FILE_TRUE = 1
} NPWFileBooleanValue;

NPWFile* npw_file_new_file (const gchar *destination, const gchar *source);
void npw_file_free (NPWFile* file);

NPWFileType npw_file_get_type (const NPWFile* file);
const gchar* npw_file_get_destination (const NPWFile* file);
const gchar* npw_file_get_source (const NPWFile* file);

void npw_file_set_execute (NPWFile* file, gboolean value);
gboolean npw_file_get_execute (const NPWFile* file);

void npw_file_set_project (NPWFile* file, gboolean value);
gboolean npw_file_get_project (const NPWFile* file);

void npw_file_set_autogen (NPWFile* file, NPWFileBooleanValue value);
NPWFileBooleanValue npw_file_get_autogen (const NPWFile* file);

#endif
