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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

/* Goto to corresponding location in editor, opening file if necessary
 *---------------------------------------------------------------------------*/
void goto_location_in_editor (AnjutaPlugin *plugin, const gchar* uri, guint line);

#endif
