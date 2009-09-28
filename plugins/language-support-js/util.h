/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    Copyright (C) 2009 Maxim Ermilov   <zaspire@rambler.ru>

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

#ifndef _UTIL_H_
#define _UTIL_H_

#include <glib.h>

#include "ijs-symbol.h"

#undef DEBUG_PRINT
#define DEBUG_PRINT g_warning

enum BaseType
{
	BASE_CLASS,
	BASE_FUNC,
	BASE_ENUM,
	BASE_NAMESPACE,
};

struct _Argument
{
	gchar *name;
	GList *types;
};

typedef struct _Argument Argument;

GList * get_import_include_paths (void);
gpointer getPlugin (void);
void setPlugin (gpointer pl);
IJsSymbol* global_search (const gchar *name);
void highlight_lines (GList *lines);

gchar* get_gir_path (void);
gboolean code_is_in_comment_or_str (gchar *str, gboolean remove_not_code);

#endif
