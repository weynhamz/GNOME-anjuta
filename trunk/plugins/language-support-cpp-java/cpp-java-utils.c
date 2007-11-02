/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    cpp-java-utils.c
    Copyright (C) 2000 Naba Kumar  <naba@gnome.org>

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

#include <libanjuta/interfaces/ianjuta-editor-cell.h>

#include "cpp-java-utils.h"

#define LEFT_BRACE(ch) (ch == ')'? '(' : (ch == '}'? '{' : (ch == ']'? '[' : ch)))  

gboolean
cpp_java_util_jump_to_matching_brace (IAnjutaIterable *iter, gchar brace, gint limit)
{
	gchar point_ch = brace;
	gint cur_iteration = 0;
	gboolean use_limit = (limit > 0);
	GString *braces_stack = g_string_new ("");
	
	g_return_val_if_fail (point_ch == ')' || point_ch == ']' ||
						  point_ch == '}', FALSE);
	
	/* DEBUG_PRINT ("Matching brace being"); */
	/* Push brace */
	g_string_prepend_c (braces_stack, point_ch);
	
	while (ianjuta_iterable_previous (iter, NULL))
	{
		/* Check limit */
		cur_iteration++;
		if (use_limit && cur_iteration > limit)
			break;
		
		/* Skip comments and strings */
		IAnjutaEditorAttribute attrib =
			ianjuta_editor_cell_get_attribute (IANJUTA_EDITOR_CELL (iter), NULL);
		if (attrib == IANJUTA_EDITOR_COMMENT || attrib == IANJUTA_EDITOR_STRING)
			continue;
		
		/* DEBUG_PRINT ("point ch = %c", point_ch); */
		point_ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0,
												 NULL);
		if (point_ch == ')' || point_ch == ']' || point_ch == '}')
		{	
			/* Push brace */
			g_string_prepend_c (braces_stack, point_ch);
			continue;
		}
		if (point_ch == LEFT_BRACE (braces_stack->str[0]))
		{
			/* Pop brace */
			g_string_erase (braces_stack, 0, 1);
		}
		/* Bail out if there is no more in stack */
		if (braces_stack->str[0] == '\0')
		{
			/* DEBUG_PRINT ("Matching brace end -- found"); */
			return TRUE;
		}
	}
	/* DEBUG_PRINT ("Matching brace end -- not found"); */
	return FALSE;
}
