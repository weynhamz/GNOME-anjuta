/* 
 * parser.c Copyright (C) 2002
 *        Etay Meiri            <etay-m@bezeqint.net>
 *        Jean-Noel Guiheneuf   <jnoel@saudionline.com.sa>
 *
 * Adapted from kdevelop - gdbparser.cpp  Copyright (C) 1999
 *        by John Birch         <jb.nz@writeme.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "parser.h"

#include <stdlib.h>
#include <libgnome/gnome-i18n.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

static gchar * skip_string (gchar *buf);

static gchar *
skip_quotes (gchar *buf, gchar quotes)
{
	if (buf && *buf == quotes)
	{
		buf++;

		while (*buf)
		{
			if (*buf == '\\')
				buf++; // skips \" or \' problems
			else if (*buf == quotes)
				return buf + 1;

			buf++;
		}
	}
	return buf;
}

static gchar *
skip_delim (gchar * buf, gchar open, gchar close)
{
	if (buf && *buf == open)
	{
		buf++;

		while (*buf)
		{
			if (*buf == open)
				buf = skip_delim (buf, open, close);
			else if (*buf == close)
				return buf + 1;
			else if (*buf == '\"')
				buf = skip_string (buf);
			else if (*buf == '\'')
				buf = skip_quotes (buf, *buf);
			else if (*buf)
				buf++;
		}
	}
	return buf;
}

static gchar *
skip_string (gchar *buf)
{
	if (buf && *buf == '\"')
	{
		buf = skip_quotes (buf, *buf);
		while (*buf)
		{
			if ((strncmp (buf, ", \"", 3) == 0) ||
				(strncmp (buf, ", '", 3) == 0))
				buf = skip_quotes (buf + 2, *(buf + 2));

			else if (strncmp (buf, " <", 2) == 0)	// take care of
													// <repeats
				buf = skip_delim (buf + 1, '<', '>');
			else
				break;
		}

		// If the string is long then it's chopped and has ... after it.
		while (*buf && *buf == '.')
			buf++;
	}
	return buf;
}

static gchar *
skip_token_end (gchar * buf)
{
	if (buf)
	{
		switch (*buf)
		{
		case '"':
			return skip_string (buf);
		case '\'':
			return skip_quotes (buf, *buf);
		case '{':
			return skip_delim (buf, '{', '}');
		case '<':
			return skip_delim (buf, '<', '>');
		case '(':
			return skip_delim (buf, '(', ')');
		}

		while (*buf && !isspace (*buf) && *buf != ',' && *buf != '}' &&
			   *buf != '=')
			buf++;
	}
	return buf;
}

static gchar *
skip_token_value (gchar * buf)
{
	gchar *end;

	if (!buf)
		return NULL;
	while (TRUE)
	{
		buf = skip_token_end (buf);

		end = buf;
		while (*end && isspace (*end) && *end != '\n')
			end++;

		if (*end == 0 || *end == ',' || *end == '\n' || *end == '=' ||
			*end == '}')
			break;

		if (buf == end)
			break;

		buf = end;
	}
	return buf;
}

static gchar *
skip_next_token_start (gchar * buf)
{
	if (!buf)
		return NULL;

	while (*buf &&
		   (isspace (*buf) || *buf == ',' || *buf == '}' || *buf == '='))
		buf++;

	return buf;
}

static gchar *
skip_next_token (gchar * buf)
{
	if (!buf)
		return NULL;

	while (*buf &&
		   (isspace (*buf) || *buf == ',' || *buf == '='))
		buf++;

	return buf;
}

static IAnjutaDebuggerDataType
get_type (gchar **buf)
{
	gchar *pos;
	
	if (!*buf || !*(*buf = skip_next_token_start (*buf)))
		return IANJUTA_DEBUGGER_UNKNOWN_TYPE;

	// A reference, probably from a parameter value.
	if (**buf == '@')
		return IANJUTA_DEBUGGER_REFERENCE_TYPE;

	// Structures and arrays - (but which one is which?)
	// {void (void)} 0x804a944 <__builtin_new+41> - this is a fn pointer
	// (void (*)(void)) 0x804a944 <f(E *, char)> - so is this - ugly!!!
	if (**buf == '{')
	{
		(*buf)++;
		if (**buf ==  '{')
			return IANJUTA_DEBUGGER_ARRAY_TYPE;

		if (strncmp (*buf, "<No data fields>}", 17) == 0)
		{
			(*buf) += 17;
			return IANJUTA_DEBUGGER_VALUE_TYPE;
		}

		pos = *buf;
		while (*pos)
		{
			switch (*pos)
			{
			case '=':
				return IANJUTA_DEBUGGER_STRUCT_TYPE;
			case '"':
				pos = skip_string (pos);
				break;
			case '\'':
				pos = skip_quotes (pos, '\'');
				break;
			case ',':
				if (*(pos - 1) == '}')
				{
					g_warning ("??????\n");
				}
				return IANJUTA_DEBUGGER_ARRAY_TYPE;
			case '}':
				if (*(pos + 1) == ',' || *(pos + 1) == '\n' || !*(pos + 1))
					return IANJUTA_DEBUGGER_ARRAY_TYPE;			 // Hmm a single element
												 // array??
				if (strncmp (pos + 1, " 0x", 3) == 0)
					return IANJUTA_DEBUGGER_POINTER_TYPE;		 // What about references?
				return IANJUTA_DEBUGGER_UNKNOWN_TYPE;			 // very odd?
			case '(':
				pos = skip_delim (pos, '(', ')');
				break;
			case '<':
				pos = skip_delim (pos, '<', '>');
				break;
			default:
				pos++;
				break;
			}
		}
		return IANJUTA_DEBUGGER_UNKNOWN_TYPE;
	}

	// some sort of address. We need to sort out if we have
	// a 0x888888 "this is a char*" type which we'll term a value
	// or whether we just have an address
	if (strncmp (*buf, "0x", 2) == 0)
	{
		pos = *buf;
		while (*pos)
		{
			if (!isspace (*pos))
				pos++;
			else if (*(pos + 1) == '\"')
				return IANJUTA_DEBUGGER_VALUE_TYPE;
			else
				break;
		}
		return IANJUTA_DEBUGGER_POINTER_TYPE;
	}

	// Pointers and references - references are a bit odd
	// and cause GDB to fail to produce all the local data
	// if they haven't been initialised. but that's not our problem!!
	// (void (*)(void)) 0x804a944 <f(E *, char)> - this is a fn pointer
	if (**buf == '(')
	{
		pos = *buf;
		pos = skip_delim (pos, '(', ')');
		pos -= 2;
		switch (*pos)
		{
		case ')':
		case '*':
			return IANJUTA_DEBUGGER_POINTER_TYPE;
		case '&':
			return IANJUTA_DEBUGGER_REFERENCE_TYPE;
		default:
			/* fix (char * const) - case */
			while(*pos && (isalpha(*pos) || *pos == ' ')) --pos;
			switch(*pos)
			{
			  case '*':	return IANJUTA_DEBUGGER_POINTER_TYPE;
			  case '&':	return IANJUTA_DEBUGGER_REFERENCE_TYPE;
			  default:	return IANJUTA_DEBUGGER_UNKNOWN_TYPE;
			}
		}
	}

	pos = skip_token_value (*buf);
	if ((strncmp (pos, " = ", 3) == 0) || (*pos == '='))
		return IANJUTA_DEBUGGER_NAME_TYPE;

	return IANJUTA_DEBUGGER_VALUE_TYPE;
}

static gchar *
get_name (gchar **buf)
{
	gchar *start;

	start = skip_next_token_start (*buf);
	if (*start)
	{
		gchar *t;

		*buf = skip_token_value (start);
		t = *buf;
		if (*t == '=')
			t--;
		return g_strstrip (g_strndup (start, t - start + 1));
	}
	else
		*buf = start;

	return NULL;
}

static gchar *
get_value (gchar **buf)
{
	gchar *start;
	gchar *value;

	/* g_print("get_value: %s\n",*buf); */

	start = skip_next_token_start (*buf);
	*buf = skip_token_value (start);

	if (*start == '{')
		return g_strstrip (g_strndup (start + 1, *buf - start - 1));
	if (*buf == start)
		return NULL;
	value = g_strstrip (g_strndup (start, *buf - start));

	return value;
}

static void
parse_parent (IAnjutaDebuggerWatch* parent, gchar **buf)
{
	IAnjutaDebuggerDataType type;
	IAnjutaDebuggerWatch* child;
	guint idx;
	gchar *pos;
	guint i;

	if (*buf == NULL)
		return;

	type = get_type (buf);
	if (type == IANJUTA_DEBUGGER_NAME_TYPE)
	{
		parent->name = get_name (buf);
		type = get_type (buf);
	}
	
	parent->type = type;
		
	switch (parent->type)
	{
    case IANJUTA_DEBUGGER_ROOT_TYPE:
    case IANJUTA_DEBUGGER_UNKNOWN_TYPE:
    case IANJUTA_DEBUGGER_NAME_TYPE:
		break;
    case IANJUTA_DEBUGGER_POINTER_TYPE:
    case IANJUTA_DEBUGGER_VALUE_TYPE:
    case IANJUTA_DEBUGGER_REFERENCE_TYPE:
		parent->value = get_value (buf);
	    break;
    case IANJUTA_DEBUGGER_ARRAY_TYPE:
	    child = NULL;
	    idx = 0;

	    while (**buf)
		{
			*buf = skip_next_token (*buf);
			
			if (**buf == '\0')
				return;
			if (**buf == '}')
			{
				*buf = *buf + 1;
				return;
			}

			if (child == NULL)
			{
				parent->children = g_new0 (IAnjutaDebuggerWatch, 1);
				child = parent->children;
			}
			else
			{
				child->sibling = g_new0 (IAnjutaDebuggerWatch, 1);
				child = child->sibling;
			}
			child->index = idx;
			parse_parent (child, buf);
		
			if (child->value)
			{	
				pos = strstr (child->value, " <repeats");
				if (pos)
				{
					if ((i = atoi (pos + 10)))
						idx += (i - 1);
				}
			}
			idx++;
		}
			
	   break;
    case IANJUTA_DEBUGGER_STRUCT_TYPE:
	    child = NULL;

	    while (**buf)
		{
			*buf = skip_next_token (*buf);
			
			if (**buf == '\0')
				return;
			if (**buf == '}')
			{
				*buf = *buf + 1;
				return;
			}

			if (child == NULL)
			{
				parent->children = g_new0 (IAnjutaDebuggerWatch, 1);
				child = parent->children;
			}
			else
			{
				child->sibling = g_new0 (IAnjutaDebuggerWatch, 1);
				child = child->sibling;
			}
			
			parse_parent (child, buf);
		}
		return;
	}
	
}

IAnjutaDebuggerWatch*
gdb_watch_parse (const GDBMIValue *mi_results)
{
	IAnjutaDebuggerWatch *watch;
	const GDBMIValue *value = NULL;
	
	if (mi_results) value = gdbmi_value_hash_lookup (mi_results, "value");
	if ((mi_results == NULL) || (value == NULL))
	{
		watch = g_new0 (IAnjutaDebuggerWatch, 1);
		watch->value = "?";
	}
	else
	{
		gchar *pos;
		gchar *full_output;
		
		/* Concat the answers of gdb */
		full_output = (char *)gdbmi_value_literal_get (value);
		
		pos = full_output;
		
		watch = g_new0 (IAnjutaDebuggerWatch, 1);
		
		parse_parent (watch, &pos);
	}

	return watch;
}

void gdb_watch_free (IAnjutaDebuggerWatch* this)
{
	if (this->name != NULL) g_free ((char *)this->name);
	if (this->value != NULL) g_free ((char *)this->value);
		
	if (this->children != NULL) gdb_watch_free (this->children);
	if (this->sibling != NULL) gdb_watch_free (this->sibling);
		
	g_free (this);
}
