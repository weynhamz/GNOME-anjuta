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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __FILE_H__
#define __FILE_H__

#include <glib.h>

// file and list of files 

typedef struct _NPWFile NPWFile;
typedef struct _NPWFileList NPWFileList;

typedef enum {
	NPW_FILE,
	NPW_SCRIPT,
	NPW_DIRECTORY
} NPWFileType;

NPWFile* npw_file_new(NPWFileList* owner);
void npw_file_destroy(NPWFile* this);

void npw_file_set_type(NPWFile* this, NPWFileType type);
NPWFileType npw_file_get_type(const NPWFile* this);

void npw_file_set_destination(NPWFile* this, const gchar* destination);
const gchar* npw_file_get_destination(const NPWFile* this);

void npw_file_set_source(NPWFile* this, const gchar* destination);
const gchar* npw_file_get_source(const NPWFile* this);

const NPWFile* npw_file_next(const NPWFile* this);

NPWFileList* npw_file_list_new(void);

void npw_file_list_destroy(NPWFileList* this);

typedef void (*NPWFileForeachFunc) (NPWFile* file);

void npw_file_list_foreach_file(const NPWFileList* this, NPWFileForeachFunc func);
const NPWFile* npw_file_list_first(const NPWFileList* this);

#endif
