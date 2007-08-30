/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gdbmi.h
 * Copyright (C) Naba Kumar 2005 <naba@gnome.org>
 * 
 * gdbmi.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2, or (at your option) any later version.
 * 
 * gdbmi.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with main.c.  See the file "COPYING".  If not,
 * write to:  The Free Software Foundation, Inc.,
 *            51 Franklin Street, Fifth Floor,
 *            Boston,  MA  02110-1301, USA.
 */

/* GDB MI parser */

#ifndef __GDBMI_H__
#define __GDBMI_H__

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
	GDBMI_DATA_HASH,
	GDBMI_DATA_LIST,
	GDBMI_DATA_LITERAL
} GDBMIDataType;

typedef struct _GDBMIValue GDBMIValue;

GDBMIValue *gdbmi_value_new (GDBMIDataType data_type, const gchar *name);
GDBMIValue* gdbmi_value_literal_new (const gchar *name, const gchar *data);
void gdbmi_value_free (GDBMIValue *val);

const gchar* gdbmi_value_get_name (const GDBMIValue *val);
void gdbmi_value_set_name (GDBMIValue *val, const gchar *name);
GDBMIDataType gdbmi_value_get_type (const GDBMIValue* val);
gint gdbmi_value_get_size (const GDBMIValue* val);
void gdbmi_value_foreach (const GDBMIValue* val, GFunc func, gpointer user_data);

/* Literal operations */
void gdbmi_value_literal_set (GDBMIValue* val, const gchar *data);
const gchar* gdbmi_value_literal_get (const GDBMIValue* val);

/* Hash operations */
void gdbmi_value_hash_insert (GDBMIValue* val, const gchar *key,
							  GDBMIValue *value);
const GDBMIValue* gdbmi_value_hash_lookup (const GDBMIValue* val, const gchar *key);

/* List operations */
void gdbmi_value_list_append (GDBMIValue* val, GDBMIValue *value);
const GDBMIValue* gdbmi_value_list_get_nth (const GDBMIValue* val, gint idx);

/* Parser and dumper */
GDBMIValue* gdbmi_value_parse (const gchar *message);
void gdbmi_value_dump (const GDBMIValue *val, gint indent_level);

G_END_DECLS

#endif
