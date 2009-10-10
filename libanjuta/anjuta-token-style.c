/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-token-style.c
 * Copyright (C) Sébastien Granjoux 2009 <seb.sfo@free.fr>
 * 
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "anjuta-token-style.h"

#include "libanjuta/anjuta-debug.h"

#include <string.h>

/* Type definition
 *---------------------------------------------------------------------------*/

typedef struct _AnjutaTokenStyleSeparator AnjutaTokenStyleSeparator;

struct _AnjutaTokenStyleSeparator
{
	guint count;
	gchar *value;
	gboolean eol;
};

struct _AnjutaTokenStyle
{
	guint max_width;
	GHashTable *separator;
};

/* Private functions
 *---------------------------------------------------------------------------*/

void free_separator (AnjutaTokenStyleSeparator *sep, gpointer user_data)
{
	DEBUG_PRINT ("free sep %p count %d", sep, sep->count);
	g_free (sep->value);
	g_slice_free (AnjutaTokenStyleSeparator, sep);
}

void free_separator_list (guint key, GList *value, gpointer user_data)
{
	/* Free list elements */
	g_list_foreach (value, (GFunc)free_separator, NULL);
	g_list_free (value);
}

AnjutaTokenStyleSeparator*
anjuta_token_style_insert_separator (AnjutaTokenStyle *style, guint key, const gchar *value)
{
	GList *list;
	GList *last = NULL;
	GList *sibling = NULL;
	AnjutaTokenStyleSeparator *sep;


	/* Look the separator is already registered */
	list = (GList *)g_hash_table_lookup (style->separator, GINT_TO_POINTER (key));
	if (list != NULL)
	{
		for (sibling = list; sibling != NULL; sibling = g_list_next(sibling))
		{
			sep = (AnjutaTokenStyleSeparator *)sibling->data;

			/* Keep the first separator with count = 1, to insert the new one if
			 * not found */
			if ((last == NULL) && (sep->count == 1)) last = sibling;

			if (value == NULL)
			{
				if (sep->value == NULL)
				{
					sep->count++;
					break;
				}
			}
			else if ((sep->value != NULL) && (strcmp (sep->value, value) == 0))
			{
				sep->count++;
				break;
			}
		}
	}

	if (sibling != NULL)
	{
		/* Increment the separator count, Move it if needed */
		for (last = g_list_previous (sibling); last != NULL; last = g_list_previous (sibling))
		{
			if (((AnjutaTokenStyleSeparator *)sibling->data)->count >= ((AnjutaTokenStyleSeparator *)last->data)->count)
			{
				last->next = sibling->next;
				sibling->next = last;
				sibling->prev = last->prev;
				last->prev = sibling;
			}
			else
			{
				break;
			}
		}
		
		if (last == NULL)
		{
			/* Update the list head */
			list = sibling;
			g_hash_table_replace (style->separator, GINT_TO_POINTER (key), list);
		}

		return (AnjutaTokenStyleSeparator *)sibling->data;
	}
	else
	{
		/* Create a new separator */
		sep = g_slice_new0 (AnjutaTokenStyleSeparator);
		sep->count = 1;
		sep->value = g_strdup (value);
		sep->eol = value == NULL ? FALSE : strchr (value, '\n') != NULL;
		DEBUG_PRINT ("alloc sep %p count %d", sep, sep->count);
		list = g_list_insert_before (list, last, sep);
		g_hash_table_replace (style->separator, GINT_TO_POINTER (key), list);

		return sep;
	}
}

AnjutaTokenStyleSeparator*
anjuta_token_style_insert_separator_between (AnjutaTokenStyle *style, gint next, gint prev, const gchar *value)
{
	return anjuta_token_style_insert_separator (style, ((guint)prev & 0xFFFF) | (((guint)next & 0xFFFF) << 16), value);
}

static AnjutaToken*
anjuta_token_style_lookup (AnjutaTokenStyle *style)
{
	GList *list;
	
	list = g_hash_table_lookup (style->separator, GINT_TO_POINTER (ANJUTA_TOKEN_NEXT));

	return anjuta_token_new_string (ANJUTA_TOKEN_NEXT, ((AnjutaTokenStyleSeparator *)list->data)->value);
}

/* Public functions
 *---------------------------------------------------------------------------*/

void
anjuta_token_style_update (AnjutaTokenStyle *style, AnjutaToken *list)
{
	AnjutaToken *token;
	AnjutaToken *next_token;
	guint prev = 0;
	guint next = 0;
	guint line_width = 0;
	guint sep_count = 0;

	/* Initialize first line width */
	for (token = list; token != NULL; token = anjuta_token_previous (token))
	{
		gchar *value = anjuta_token_value (token);
		const gchar *eol = strrchr (value, '\n');
		gsize len = strlen (value);

		g_free (value);


		if (eol != NULL)
		{
			line_width = value + len - eol;
			break;
		}

		line_width += len;
	}
	
	for (token = anjuta_token_next_child (list); token != NULL; token = next_token)
	{
		gchar *value = NULL;
		const gchar *eol;
		gsize len;
		gint type;
		
		next_token = anjuta_token_next_sibling (token);
		type = anjuta_token_get_type (token);
		next = next_token == NULL ? 0 : anjuta_token_get_type (next_token);

		value = anjuta_token_value (token);
		if (value == NULL) continue;

		len = strlen (value);
		eol = strrchr (value, '\n');
		if (eol != NULL) len -= (eol - value);
		g_free (value);

		line_width += len;
		
		switch (type)
		{
			case ANJUTA_TOKEN_START:
			case ANJUTA_TOKEN_LAST:
			case ANJUTA_TOKEN_NEXT:
				break;
			default:
				if (eol != NULL)
				{
					line_width = len;
					sep_count = 0;
				}
				continue;
		}
		
		value = anjuta_token_evaluate (token);
		anjuta_token_style_insert_separator_between (style, 0, type, value);
		if (type == ANJUTA_TOKEN_NEXT)
		{
			anjuta_token_style_insert_separator_between (style, next, prev, value);
			anjuta_token_style_insert_separator_between (style, next, ANJUTA_TOKEN_ANY, value);
			anjuta_token_style_insert_separator_between (style, ANJUTA_TOKEN_ANY, prev, value);
		}
		g_free (value);

		if (eol == NULL)
		{
			sep_count++;
		}
		else
		{
			if ((sep_count > 1) && (line_width > style->max_width))
			{
				style->max_width = line_width;
			}
			sep_count = 0;
			line_width = len;
		}
	}
}	

static void
anjuta_token_style_format_line (AnjutaTokenStyle *style, AnjutaToken *bol, AnjutaToken *eol)
{
	
}

void
anjuta_token_style_format (AnjutaTokenStyle *style, AnjutaToken *list)
{
	AnjutaToken *arg;

	for (arg = anjuta_token_next_child (list); arg != NULL; arg = anjuta_token_next_sibling (arg))
	{
		if ((anjuta_token_get_type (arg) == ANJUTA_TOKEN_NEXT) && (anjuta_token_get_flags (arg) & (ANJUTA_TOKEN_ADDED)))
		{
			anjuta_token_insert_after (arg, anjuta_token_style_lookup (style));
			anjuta_token_free (arg);
		}
	}
#if 0	
	AnjutaToken *arg;

	if (style->sep == NULL)
	{
		for (arg = anjuta_token_next_child (list); arg != NULL; arg = anjuta_token_next_sibling (arg))
		{
			if ((anjuta_token_get_type (arg) == ANJUTA_TOKEN_SPACE) && (anjuta_token_get_flags (arg) & (ANJUTA_TOKEN_ADDED)))
			{
				anjuta_token_insert_after (arg, anjuta_token_copy (style->eol));
				anjuta_token_free (arg);
			}
		}
	}
	else
	{
		AnjutaToken *bol = anjuta_token_next_child (list);
		gboolean modified = FALSE;
		
		for (arg = bol; arg != NULL; arg = anjuta_token_next_sibling (arg))
		{
			gchar *value = anjuta_token_evaluate (arg);
			if (anjuta_token_get_flags (arg) & (ANJUTA_TOKEN_REMOVED | ANJUTA_TOKEN_ADDED)) modified = TRUE;
			if (strchr (value, '\n'))
			{
				if (modified) anjuta_token_style_format_line (style, list, arg);
				bol = arg;
				if (style->sep == NULL) modified = FALSE;
			}
			g_free (value);
		}
		if (modified) anjuta_token_style_format_line (style, bol, NULL);
	}
#endif
}

AnjutaToken *
anjuta_token_list_first (AnjutaToken *list)
{
	AnjutaToken *token;

	token = anjuta_token_next_child (list);
	if (token == NULL) return token;

	if (anjuta_token_get_type (token) == ANJUTA_TOKEN_START)
	{
		token = anjuta_token_next_sibling (token);
	}
	
	return token;
}

AnjutaToken *
anjuta_token_list_last (AnjutaToken *list)
{
	AnjutaToken *token;

	token = anjuta_token_list_first (list);
	for (;;)
	{
		AnjutaToken *next = anjuta_token_list_next (list);
		if (next == NULL) return token;
		token = next;
	}
}

AnjutaToken *
anjuta_token_list_next (AnjutaToken *sibling)
{
	AnjutaToken *token;

	token = anjuta_token_next_sibling (sibling);
	if (token == NULL) return token;

	if (anjuta_token_get_type (token) == ANJUTA_TOKEN_NEXT)
	{
		token = anjuta_token_next_sibling (token);
	}

	return token;
}

AnjutaToken *
anjuta_token_list_replace (AnjutaToken *sibling, AnjutaToken *baby)
{
	AnjutaToken *token;
	
	token = anjuta_token_insert_before (sibling, baby);
	if ((anjuta_token_get_type (sibling) != ANJUTA_TOKEN_NEXT) && (anjuta_token_get_type (sibling) != ANJUTA_TOKEN_LAST))
	{
		anjuta_token_remove (sibling);
	}

	return token;
}

AnjutaToken *
anjuta_token_list_replace_nth (AnjutaToken *list, guint n, AnjutaToken *baby)
{
	AnjutaToken *token;

	token = anjuta_token_list_first (list); 
	if (token == NULL)
	{
		token = anjuta_token_insert_child (list, anjuta_token_new_static (ANJUTA_TOKEN_START | ANJUTA_TOKEN_ADDED, NULL));
		token = anjuta_token_insert_after (token, anjuta_token_new_static (ANJUTA_TOKEN_LAST | ANJUTA_TOKEN_ADDED, NULL));
	}

	for (; n != 0; n--)
	{
		AnjutaToken *next;

		switch (anjuta_token_get_type (token))
		{
			case ANJUTA_TOKEN_LAST:
				anjuta_token_insert_before (token, anjuta_token_new_static (ANJUTA_TOKEN_NEXT | ANJUTA_TOKEN_ADDED, NULL));
				continue;
			case ANJUTA_TOKEN_NEXT:
				break;
			default:
				token = anjuta_token_next_sibling (token);
				if (token == NULL)
				{
					token = anjuta_token_insert_after (token, anjuta_token_new_static (ANJUTA_TOKEN_NEXT | ANJUTA_TOKEN_ADDED, NULL));
				}
				else if (anjuta_token_get_type (token) == ANJUTA_TOKEN_LAST)
				{
					token = anjuta_token_insert_before (token, anjuta_token_new_static (ANJUTA_TOKEN_NEXT | ANJUTA_TOKEN_ADDED, NULL));
				}
				break;
		}

		next = anjuta_token_next_sibling (token);
		if (next == NULL)
		{
			token = anjuta_token_insert_after (token, anjuta_token_new_static (ANJUTA_TOKEN_LAST | ANJUTA_TOKEN_ADDED, NULL));
		}
		else
		{
			token = next;
		}
	}

	return anjuta_token_list_replace (token, baby); 
}

AnjutaToken *
anjuta_token_list_insert_after (AnjutaToken *sibling, AnjutaToken *baby)
{
	AnjutaToken *token = sibling;
	AnjutaToken *separator;

	if (anjuta_token_get_type (token) == ANJUTA_TOKEN_LAST)
	{
		token = anjuta_token_previous_sibling (token);
	}
	else if ((anjuta_token_get_type (token) != ANJUTA_TOKEN_NEXT) && (anjuta_token_next_sibling (token) != NULL))
	{
		token = anjuta_token_next_sibling (token);
	}
	
	separator = anjuta_token_new_static (ANJUTA_TOKEN_NEXT | ANJUTA_TOKEN_ADDED, NULL);
	token = anjuta_token_insert_after (token, separator);
	token = anjuta_token_insert_after (token, baby);

	return token;
}

void
anjuta_token_list_remove (AnjutaToken *sibling)
{
	AnjutaToken *token;
	
	if ((anjuta_token_get_type (sibling) != ANJUTA_TOKEN_NEXT) && (anjuta_token_get_type (sibling) != ANJUTA_TOKEN_LAST))
	{
		anjuta_token_remove (sibling);
	}

	token = anjuta_token_next_sibling (sibling);
	if (anjuta_token_get_type (token) == ANJUTA_TOKEN_NEXT)
	{
		anjuta_token_remove (token);
		return;
	}

	token = anjuta_token_previous_sibling (sibling);
	if (anjuta_token_get_type (token) == ANJUTA_TOKEN_NEXT)
	{
		anjuta_token_remove (token);
		return;
	}

	return;
}


/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

AnjutaTokenStyle *
anjuta_token_style_new (const gchar *start, const gchar *next, const gchar *eol, const gchar *last, guint max_width)
{
	AnjutaTokenStyle *style;
	
	style = g_slice_new0 (AnjutaTokenStyle);
	style->max_width = max_width;
	
	style->separator = g_hash_table_new (g_direct_hash, NULL);
	anjuta_token_style_insert_separator (style, ANJUTA_TOKEN_START, start);
	anjuta_token_style_insert_separator (style, ANJUTA_TOKEN_NEXT, next);
	anjuta_token_style_insert_separator (style, ANJUTA_TOKEN_NEXT, eol);
	anjuta_token_style_insert_separator (style, ANJUTA_TOKEN_LAST, last);
	
	return style;	
}

void
anjuta_token_style_free (AnjutaTokenStyle *style)
{
	g_hash_table_foreach (style->separator, (GHFunc)free_separator_list, NULL);
	g_hash_table_destroy (style->separator);
	g_slice_free (AnjutaTokenStyle, style);
}
