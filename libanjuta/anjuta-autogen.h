/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    anjuta-autogen.h
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

#ifndef __ANJUTA_AUTOGEN_H__
#define __ANJUTA_AUTOGEN_H__

#include <glib.h>


typedef struct _AnjutaAutogen AnjutaAutogen;
typedef void (*AnjutaAutogenFunc) (AnjutaAutogen* autogen, gpointer data);
typedef void (*AnjutaAutogenOutputFunc) (const gchar* output, gpointer data);

/**
 * AnjutaAutogenError:
 * @ANJUTA_AUTOGEN_ERROR_BUSY: autogen is already running
 * @ANJUTA_AUTOGEN_ERROR_WRITE_FAIL: unable to write file
 *
 * Possibly errors that can occur during charset conversion
 */
typedef enum
{
	ANJUTA_AUTOGEN_ERROR_BUSY,
	ANJUTA_AUTOGEN_ERROR_WRITE_FAIL
} AnjutaAutogenError;

#define ANJUTA_AUTOGEN_ERROR anjuta_autogen_error_quark()

GQuark anjuta_autogen_error_quark (void);


AnjutaAutogen* anjuta_autogen_new (void);
void anjuta_autogen_free (AnjutaAutogen* this);

gboolean anjuta_autogen_write_definition_file (AnjutaAutogen* this, GHashTable* values, GError **error);

void anjuta_autogen_set_library_path (AnjutaAutogen* this, const gchar *directory);
void anjuta_autogen_clear_library_path (AnjutaAutogen* this);
GList *anjuta_autogen_get_library_paths (AnjutaAutogen* this);
gboolean anjuta_autogen_set_input_file (AnjutaAutogen* this, const gchar* filename, const gchar* start_marker, const gchar* end_marker);
gboolean anjuta_autogen_set_output_file (AnjutaAutogen* this, const gchar* filename);
gboolean anjuta_autogen_set_output_callback (AnjutaAutogen* this, AnjutaAutogenOutputFunc func, gpointer user_data);

gboolean anjuta_autogen_execute (AnjutaAutogen* this, AnjutaAutogenFunc func, gpointer data, GError** error);

gboolean anjuta_check_autogen(void);

#endif
