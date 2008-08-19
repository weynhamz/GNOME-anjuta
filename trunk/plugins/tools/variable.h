/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    variable.h
    Copyright (C) 2005 Sebastien Granjoux

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

#ifndef __VARIABLE_H__
#define __VARIABLE_H__

#include <libanjuta/anjuta-shell.h>

#include <glib.h>

typedef struct _ATPVariable
{
	AnjutaShell* shell;
} ATPVariable;

typedef enum {
	ATP_DEFAULT_VARIABLE = 0,
	ATP_DIRECTORY_VARIABLE = 1 << 1,	/* Variable is a directory */
	ATP_FILE_VARIABLE = 1 << 2,		/* Variable is a filename */
	ATP_INTERACTIVE_VARIABLE = 1 << 3,	/* Variable display a dialog */
	ATP_NO_VARIABLE = 1 << 15
} ATPFlags;
	
ATPVariable* atp_variable_construct (ATPVariable* this, AnjutaShell* shell);
void atp_variable_destroy (ATPVariable* this);

guint atp_variable_get_count (const ATPVariable* this);
const gchar* atp_variable_get_name (const ATPVariable* this, guint id);
const gchar* atp_variable_get_help (const ATPVariable* this, guint id);
ATPFlags atp_variable_get_flag (const ATPVariable* this, guint id);

gchar* atp_variable_get_value_from_id (const ATPVariable* this, guint id);
gchar* atp_variable_get_value (const ATPVariable* this, const gchar* name);
gchar* atp_variable_get_value_from_name_part (const ATPVariable* this, const gchar* name, gsize length);

#endif
