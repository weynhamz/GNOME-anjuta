/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/* 
    utilities.h
    Copyright (C) 2000  Kh. Naba Kumar Singh

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
#ifndef _UTILITIES_H_
#define _UTILITIES_H_

#include <libanjuta/anjuta-plugin.h>

#include <glib.h>

/* Functions that dynamic allocate memory. Return value(s) should be g_freed
 * Removes while spaces in the text
 *---------------------------------------------------------------------------*/
gchar* gdb_util_remove_white_spaces(const gchar* text);

/* In this case only GList must be freed and not the data
 * Because output data are the input data. Only GList is allocated
 *---------------------------------------------------------------------------*/
GList* gdb_util_remove_blank_lines(const GList* lines);

#endif
