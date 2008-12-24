/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    file.c
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

/*
 * File data read in .wiz file
 *
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "file.h"

/*---------------------------------------------------------------------------*/

struct _NPWFile {
NPWFileType type;
	gchar* source;
	gchar* destination;
	gint attribute;
};

typedef enum {
	NPW_EXECUTE_FILE = 1 << 0,
	NPW_PROJECT_FILE = 1 << 1,
	NPW_AUTOGEN_SET = 1 << 2,
	NPW_AUTOGEN_FILE = 1 << 3
} NPWFileAttributes;

/* File object
 *---------------------------------------------------------------------------*/

NPWFile*
npw_file_new_file (const gchar *destination, const gchar *source)
{
	NPWFile *file;

	g_return_val_if_fail (destination && source, NULL);

	file = g_slice_new(NPWFile);
	file->type = NPW_FILE;
	file->destination = g_strdup (destination);
	file->source = g_strdup (source);
	file->attribute = 0;
	
	return file;
}

void
npw_file_free (NPWFile* file)
{
	g_free (file->destination);
	g_free (file->source);
	g_slice_free (NPWFile, file);
}

NPWFileType
npw_file_get_type (const NPWFile* file)
{
	return file->type;
}

const gchar*
npw_file_get_destination (const NPWFile* file)
{
	return file->destination;
}

const gchar*
npw_file_get_source (const NPWFile* file)
{
	return file->source;
}

void
npw_file_set_execute (NPWFile* file, gboolean value)
{
	if (value)
	{
		file->attribute |= NPW_EXECUTE_FILE;
	}
	else
	{
		file->attribute &= ~NPW_EXECUTE_FILE;
	}
}

gboolean
npw_file_get_execute (const NPWFile* file)
{
	return file->attribute & NPW_EXECUTE_FILE;
}

void
npw_file_set_project (NPWFile* file, gboolean value)
{
	if (value)
	{
		file->attribute |= NPW_PROJECT_FILE;
	}
	else
	{
		file->attribute &= ~NPW_PROJECT_FILE;
	}
}

gboolean
npw_file_get_project (const NPWFile* file)
{
	return file->attribute & NPW_PROJECT_FILE;
}

void
npw_file_set_autogen (NPWFile* file, NPWFileBooleanValue value)
{
	switch (value)
	{
	case NPW_FILE_TRUE:
		file->attribute |= NPW_AUTOGEN_FILE | NPW_AUTOGEN_SET;
		break;
	case NPW_FILE_FALSE:
		file->attribute |= NPW_AUTOGEN_SET;
		file->attribute &= ~NPW_AUTOGEN_FILE;
		break;
	case NPW_FILE_DEFAULT:
		file->attribute &= ~(NPW_AUTOGEN_SET | NPW_AUTOGEN_FILE);
		break;
	}
}

NPWFileBooleanValue
npw_file_get_autogen (const NPWFile* file)
{
	return file->attribute & NPW_AUTOGEN_SET ? (file->attribute & NPW_AUTOGEN_FILE ? NPW_FILE_TRUE : NPW_FILE_FALSE) : NPW_FILE_DEFAULT;
}
