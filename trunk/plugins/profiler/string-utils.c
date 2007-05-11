/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * string-utils.c
 * Copyright (C) James Liggett 2006 <jrliggett@cox.net>
 * 
 * string-utils.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2, or (at your option) any later version.
 * 
 * plugin.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.c.  See the file "COPYING".  If not,
 * write to:  The Free Software Foundation, Inc.,
 *            59 Temple Place - Suite 330,
 *            Boston,  MA  02111-1307, USA.
 */

#include <string.h>
#include <stdlib.h>
#include "string-utils.h"

#define NONE -1

gchar *read_to_whitespace(gchar *buffer, int *end_pos, int start_pos)
{
	size_t buffer_length;
	gint begin;
	gint i;
	gchar *string;
	
	begin = NONE;
	*end_pos = 0;
	buffer_length = strlen(buffer);
	
	for (i = 0; i < buffer_length; i++)
	{
		/* If we haven't found a string yet, ignore any leading whitespace */
		if (begin == NONE)
		{
			if (!g_ascii_isspace(buffer[i]))
				begin = i;
		}
		else
		{
			/* Found the end of the string or we're at the end of the buffer */
			if (g_ascii_isspace(buffer[i]) || i == (buffer_length - 1))
			{
				*end_pos = i + start_pos;
				string = g_strndup(&buffer[begin], i - begin);
				return string;
			}
				
		}
		
	}
	
	return NULL;
}

gchar *strip_whitespace(gchar* buffer)
{
	size_t buffer_length;
	gint i;
	gchar *string;
	
	buffer_length = strlen(buffer);
	string = NULL;
	
	for (i = 0; i < buffer_length; i++)
	{
		if (!g_ascii_isspace(buffer[i]))
			break;
	}
	
	if (i < buffer_length)
		string = g_strdup(&buffer[i]);
	
	return string;
}

gchar *read_to_delimiter(gchar *buffer, gchar *delimiter)
{
	gint i;
	size_t buffer_length;
	gchar *end;  /* end of returned string */
	gchar *string;
	
	string = NULL;
	
	/* Ignore any leading whitespace */
	buffer_length = strlen(buffer);
	
	for (i = 0; i < buffer_length; i++)
	{
		if (!g_ascii_isspace(buffer[i]))
			break;
	}
	
	end = strstr(&buffer[i], delimiter);
	
	if (end)
		string = g_strndup(&buffer[i], (end - &buffer[i]));
	
	return string;	
}
