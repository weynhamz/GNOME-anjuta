/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*  transform.h
 *  Copyright (C) 2006 Armin Burgmeier
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __CLASSGEN_TRANSFORM_H__
#define __CLASSGEN_TRANSFORM_H__

#include "element-editor.h"

#include <glib/ghash.h>

G_BEGIN_DECLS

/* This file contains several functions that transform the user input from
 * the main window to the actual output that is given to autogen.
 *
 * Most of these functions are very specialized and only for one special
 * use case, but it should not be that hard to make them operate more
 * general if you need them to. */

gboolean
cg_transform_default_c_type_to_g_type (const gchar *c_type,
                                       const gchar **g_type_prefix,
                                       const gchar **g_type_name);

void
cg_transform_custom_c_type_to_g_type (const gchar *c_type,
                                      gchar **g_type_prefix,
                                      gchar **g_type_name,
                                      gchar **g_func_prefix);

void
cg_transform_c_type_to_g_type (const gchar *c_type,
                               gchar **g_type_prefix,
                               gchar **g_type_name);

void
cg_transform_string (GHashTable *table,
                     const gchar *index);

void
cg_transform_flags (GHashTable *table,
                    const gchar *index,
                    const CgElementEditorFlags *flags);

void
cg_transform_guess_paramspec (GHashTable *table,
                              const gchar *param_index,
                              const gchar *type_index,
                              const gchar *guess_entry);

void
cg_transform_arguments (GHashTable *table,
                        const gchar *index,
                        gboolean make_void);

void
cg_transform_string_to_identifier (GHashTable *table,
                                   const gchar *string_index,
                                   const gchar *identifier_index);

void
cg_transform_first_argument (GHashTable *table,
                             const gchar *index,
                             const gchar *type);

guint
cg_transform_arguments_to_gtypes (GHashTable *table,
                                  const gchar *arguments_index,
                                  const gchar *gtypes_index);

G_END_DECLS

#endif /* __CLASSGEN_TRANSFORM_H__ */
