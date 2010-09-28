/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-token-list.c
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

#include "anjuta-token-list.h"

#include "libanjuta/anjuta-debug.h"

#include <string.h>
#include <stdio.h>

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
	AnjutaTokenStyle *base;
};

/* Private functions
 *---------------------------------------------------------------------------*/

void free_separator (AnjutaTokenStyleSeparator *sep, gpointer user_data)
{
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
anjuta_token_style_lookup (AnjutaTokenStyle *style, AnjutaTokenType type, gboolean eol)
{
	GList *list;
	
	list = g_hash_table_lookup (style->separator, GINT_TO_POINTER (type));
	if ((list == NULL) && (style->base != NULL))
	{
		return anjuta_token_style_lookup (style->base, type, eol);
	}
	else
	{
		return anjuta_token_new_string (ANJUTA_TOKEN_NAME, ((AnjutaTokenStyleSeparator *)list->data)->value);
	}
}

/* Public style functions
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
		gchar *value = anjuta_token_evaluate (token);
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
	
	for (token = anjuta_token_first_item (list); token != NULL; token = next_token)
	{
		gchar *value = NULL;
		const gchar *eol;
		gsize len;
		gint type;
		
		next_token = anjuta_token_next_item (token);
		type = anjuta_token_get_type (token);
		next = next_token == NULL ? 0 : anjuta_token_get_type (next_token);

		value = anjuta_token_evaluate (token);
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

void
anjuta_token_style_format (AnjutaTokenStyle *style, AnjutaToken *list)
{
	AnjutaToken *item;
	AnjutaToken *last;
	AnjutaToken *text;
	AnjutaToken *prev;

	/* Find following tokens */
	for (last = list; last != NULL; last = anjuta_token_next (last))
	{
		/* Get all tokens in group */
		last = anjuta_token_last (last);

		gint flags = anjuta_token_get_flags (last);
		if (!(flags & (ANJUTA_TOKEN_ADDED | ANJUTA_TOKEN_REMOVED))) break;
	}
	
	/* Find previous token */
	for (prev = list; prev != NULL; prev = anjuta_token_previous (prev))
	{
		gint flags = anjuta_token_get_flags (prev);
		if ((anjuta_token_get_length (prev) != 0) && !(flags & (ANJUTA_TOKEN_ADDED | ANJUTA_TOKEN_REMOVED))) break;
		list = prev;    
	}

	for (item = list; (item != NULL) && (item != last); item = anjuta_token_next (item))
	{
		if (anjuta_token_get_flags (item) & ANJUTA_TOKEN_ADDED)
		{
			switch (anjuta_token_get_type (item))
			{
			case ANJUTA_TOKEN_START:
				text = anjuta_token_style_lookup (style, ANJUTA_TOKEN_START, FALSE);
				anjuta_token_set_flags (text, ANJUTA_TOKEN_ADDED);
				anjuta_token_insert_after (item, text);
				anjuta_token_merge (item, text);
				item = text;
				break;
			case ANJUTA_TOKEN_NEXT:
				text = anjuta_token_style_lookup (style, ANJUTA_TOKEN_NEXT, FALSE);
				anjuta_token_set_flags (text, ANJUTA_TOKEN_ADDED);
				anjuta_token_insert_after (item, text);
				anjuta_token_merge (item, text);
				item = text;
				break;
			case ANJUTA_TOKEN_LAST:
				text = anjuta_token_style_lookup (style, ANJUTA_TOKEN_LAST, FALSE);
				anjuta_token_set_flags (text, ANJUTA_TOKEN_ADDED);
				anjuta_token_insert_after (item, text);
				anjuta_token_merge (item, text);
				item = text;
				break;
			default:
				break;
			}
		}
	}
}

/* Word functions
 *---------------------------------------------------------------------------*/

/**
 * anjuta_token_first_word:
 * @list: a #AnjutaToken object being a list
 *
 * Get the first word of the list. A word is an item in the list which is not
 * a space or a separator.
 *
 * Return value: A #AnjutaToken representing the first word or NULL.
 */
AnjutaToken *
anjuta_token_first_word (AnjutaToken *list)
{
	AnjutaToken *item;

	for (item = anjuta_token_first_item (list); item != NULL; item = anjuta_token_next_item (item))
	{
		if (anjuta_token_list (item) != list)
		{
			item = NULL;
			break;
		}
		switch (anjuta_token_get_type (item))
		{
		case ANJUTA_TOKEN_START:
		case ANJUTA_TOKEN_NEXT:
			continue;
		case ANJUTA_TOKEN_LAST:
			item = NULL;
			break;
		default:
			if (anjuta_token_is_empty (item)) continue;
			break;
		}
		break;
	}

	return item;
}

AnjutaToken *
anjuta_token_next_word (AnjutaToken *item)
{
	AnjutaToken *next;
	
	for (next = anjuta_token_next_item (item); next != NULL; next = anjuta_token_next_item (next))
	{
		if (anjuta_token_list (item) != anjuta_token_list (next))
		{
			next = NULL;
			break;
		}
		switch (anjuta_token_get_type (next))
		{
		case ANJUTA_TOKEN_START:
		case ANJUTA_TOKEN_NEXT:
			continue;
		case ANJUTA_TOKEN_LAST:
			next = NULL;
			break;
		default:
			if (anjuta_token_is_empty (next)) continue;
			break;
		}
		break;
	}
	
	return next;
}

AnjutaToken *
anjuta_token_nth_word (AnjutaToken *list, guint n)
{
	AnjutaToken *item;
	gboolean no_item = TRUE;

	for (item = anjuta_token_first_item (list); item != NULL; item = anjuta_token_next_item (item))
	{
		if (anjuta_token_list (item) != list)
		{
			item = NULL;
			break;
		}
		switch (anjuta_token_get_type (item))
		{
		case ANJUTA_TOKEN_START:
			break;
		case ANJUTA_TOKEN_NEXT:
			if (no_item)	
			{
				if (n == 0) return NULL;
				n--;
			}
			no_item = TRUE;
			break;
		case ANJUTA_TOKEN_LAST:
			return NULL;
		default:
			if (n == 0) return item;
			n--;
			no_item = FALSE;
			break;
		}
	}

	return NULL;
}

AnjutaToken *
anjuta_token_replace_nth_word (AnjutaToken *list, guint n, AnjutaToken *item)
{
	AnjutaToken *token;
	gboolean no_item = TRUE;

	token = anjuta_token_first_item (list); 
	if (token == NULL)
	{
		token = anjuta_token_insert_after (token, anjuta_token_new_static (ANJUTA_TOKEN_LAST | ANJUTA_TOKEN_ADDED, NULL));
		anjuta_token_merge (list, token);
	}

	for (n++;;)
	{
		AnjutaToken *next;

		switch (anjuta_token_get_type (token))
		{
		case ANJUTA_TOKEN_LAST:
			if (no_item)
			{
				n--;
				if (n == 0)
				{
					token = anjuta_token_insert_before (token, item);
					return token;
				}
			}
			token = anjuta_token_insert_before (token, anjuta_token_new_static (ANJUTA_TOKEN_NEXT | ANJUTA_TOKEN_ADDED, NULL));
			no_item = TRUE;
			break;
		case ANJUTA_TOKEN_NEXT:
			if (no_item)	
			{
				n--;
				if (n == 0)
				{
					token = anjuta_token_insert_before (token, item);
					return token;
				}
			}
			no_item = TRUE;
			break;
		case ANJUTA_TOKEN_ITEM:
			n--;
			if (n == 0)
			{
				anjuta_token_set_flags (token, ANJUTA_TOKEN_REMOVED);
				token = anjuta_token_insert_before (token, item);
				return token;
			}
			no_item = FALSE;
			break;
		default:
			break;
		}

		next = anjuta_token_next_item (token);
		if (next == NULL)
		{
			token = anjuta_token_insert_after (token, anjuta_token_new_static (ANJUTA_TOKEN_LAST | ANJUTA_TOKEN_ADDED, NULL));
			anjuta_token_merge (list, token);
		}
		else
		{
			token = next;
		}
	}
}

AnjutaToken *
anjuta_token_insert_word_before (AnjutaToken *list, AnjutaToken *sibling, AnjutaToken *item)
{
	AnjutaToken *token;

	if (list == NULL) list = anjuta_token_list (sibling);

	for (token = anjuta_token_first_item (list); token != NULL;)
	{
		AnjutaToken *next;

		switch (anjuta_token_get_type (token))
		{
		case ANJUTA_TOKEN_LAST:
			anjuta_token_insert_before (token, anjuta_token_new_static (ANJUTA_TOKEN_NEXT | ANJUTA_TOKEN_ADDED, NULL));
			anjuta_token_insert_before (token, item);
			return item;
		case ANJUTA_TOKEN_START:		
			if (token == sibling)
			{
				anjuta_token_insert_after (token, anjuta_token_new_static (ANJUTA_TOKEN_NEXT | ANJUTA_TOKEN_ADDED, NULL));
				anjuta_token_insert_after (token, item);
				return item;
			}
			break;	
		case ANJUTA_TOKEN_NEXT:
			if (token == sibling)
			{
				token = anjuta_token_insert_before (token, item);
				anjuta_token_insert_before (token, anjuta_token_new_static (ANJUTA_TOKEN_NEXT | ANJUTA_TOKEN_ADDED, NULL));
				return item;
			}
			break;
		default:
			if (token == sibling)
			{
				anjuta_token_insert_before (token, item);
				anjuta_token_insert_before (token, anjuta_token_new_static (ANJUTA_TOKEN_NEXT | ANJUTA_TOKEN_ADDED, NULL));
				return item;
			}
			break;
		}

		next = anjuta_token_next_item (token);
		if (next == NULL)
		{
			token = anjuta_token_insert_after (token, anjuta_token_new_static (ANJUTA_TOKEN_NEXT | ANJUTA_TOKEN_ADDED, NULL));
			anjuta_token_insert_after (token, item);
			return item;
		}
		token = next;
	}
	
	anjuta_token_prepend_items (list, item);

	return item;
}

AnjutaToken *
anjuta_token_insert_word_after (AnjutaToken *list, AnjutaToken *sibling, AnjutaToken *item)
{
	AnjutaToken *token;

	if (list == NULL) list = anjuta_token_list (sibling);

	for (token = anjuta_token_first_item (list); token != NULL;)
	{
		AnjutaToken *next;

		switch (anjuta_token_get_type (token))
		{
		case ANJUTA_TOKEN_LAST:
			anjuta_token_insert_before (token, anjuta_token_new_static (ANJUTA_TOKEN_NEXT | ANJUTA_TOKEN_ADDED, NULL));
			anjuta_token_insert_before (token, item);
			return item;
		case ANJUTA_TOKEN_START:		
			if (token == sibling)
			{
				anjuta_token_insert_after (token, anjuta_token_new_static (ANJUTA_TOKEN_NEXT | ANJUTA_TOKEN_ADDED, NULL));
				anjuta_token_insert_after (token, item);
				return item;
			}
			break;	
		case ANJUTA_TOKEN_NEXT:
			if (token == sibling)
			{
				token = anjuta_token_insert_after (token, item);
				anjuta_token_insert_after (token, anjuta_token_new_static (ANJUTA_TOKEN_NEXT | ANJUTA_TOKEN_ADDED, NULL));
				return item;
			}
			break;
		default:
			if (token == sibling)
			{
				token = anjuta_token_insert_after (token, anjuta_token_new_static (ANJUTA_TOKEN_NEXT | ANJUTA_TOKEN_ADDED, NULL));
				anjuta_token_insert_after (token, item);
				return item;
			}
			break;
		}

		next = anjuta_token_next_item (token);
		if (next == NULL)
		{
			token = anjuta_token_insert_after (token, anjuta_token_new_static (ANJUTA_TOKEN_NEXT | ANJUTA_TOKEN_ADDED, NULL));
			anjuta_token_insert_after (token, item);
			return item;
		}
		token = next;
	}
	
	anjuta_token_prepend_items (list, item);

	return item;
}

AnjutaToken*
anjuta_token_remove_word (AnjutaToken *token)
{
	AnjutaToken *next;

	anjuta_token_set_flags (token, ANJUTA_TOKEN_REMOVED);
	next = anjuta_token_next_item (token);
	if ((next != NULL) && (anjuta_token_list (token) == anjuta_token_list (next)) && (anjuta_token_get_type (next) == ANJUTA_TOKEN_NEXT))
	{
		/* Remove following separator */
		anjuta_token_set_flags (next, ANJUTA_TOKEN_REMOVED);
	}
	else
	{
		next = anjuta_token_previous_item (token);
		if ((next != NULL) && (anjuta_token_list (token) == anjuta_token_list (next)) && (anjuta_token_get_type (next) == ANJUTA_TOKEN_NEXT))
		{
			/* Remove previous separator */
			anjuta_token_set_flags (next, ANJUTA_TOKEN_REMOVED);
		}
		else
		{
			next = NULL;
		}
	}
	
	return next;
}

AnjutaToken *
anjuta_token_insert_token_list (gboolean after, AnjutaToken *pos,...)
{
	AnjutaToken *first = NULL;
	GList *group = NULL;
	va_list args;
	gint type;

	va_start (args, pos);

	for (type = va_arg (args, gint); type != 0; type = va_arg (args, gint))
	{
		gchar *string = va_arg (args, gchar *);
		AnjutaToken *token;

		if (after)
		{
			pos = token = anjuta_token_insert_after (pos, anjuta_token_new_string (type | ANJUTA_TOKEN_ADDED, string));
		}
		else
		{
			token = anjuta_token_insert_before (pos, anjuta_token_new_string (type | ANJUTA_TOKEN_ADDED, string));
		}
		if (first == NULL) first = token;

		if (group != NULL)
		{
			anjuta_token_merge ((AnjutaToken *)group->data, token);
		}

		if (string == NULL)
		{
			switch (type)
			{
			case ANJUTA_TOKEN_LIST:
				break;
			default:
				group = g_list_delete_link (group, group);
				break;
			}
			group = g_list_prepend (group, token);
		}
	}
	g_list_free (group);
	
	va_end (args);
	
	return first;
}

AnjutaToken *
anjuta_token_find_type (AnjutaToken *list, gint flags, AnjutaTokenType* types)
{
	AnjutaToken *tok;
	AnjutaToken *last = NULL;
	
	for (tok = list; tok != NULL; tok = anjuta_token_next (tok))
	{
		AnjutaTokenType *type;
		for (type = types; *type != 0; type++)
		{
			if (anjuta_token_get_type (tok) == *type)
			{
				last = tok;
				if (flags & ANJUTA_TOKEN_SEARCH_NOT) break;
				if (!(flags & ANJUTA_TOKEN_SEARCH_LAST)) break;
			}
		}
		if ((flags & ANJUTA_TOKEN_SEARCH_NOT) && (*type == 0)) break;
	}

	return last;
}

AnjutaToken *
anjuta_token_skip_comment (AnjutaToken *token)
{
	if (token == NULL) return NULL;
	
	for (;;)
	{
		for (;;)
		{
			AnjutaToken *next = anjuta_token_next (token);

			if (next == NULL) return token;
			
			switch (anjuta_token_get_type (token))
			{
			case ANJUTA_TOKEN_FILE:
			case ANJUTA_TOKEN_SPACE:
				token = next;
				continue;
			case ANJUTA_TOKEN_COMMENT:
				token = next;
				break;
			default:
				return token;
			}
			break;
		}
		
		for (;;)
		{
			AnjutaToken *next = anjuta_token_next (token);

			if (next == NULL) return token;
			token = next;
			if (anjuta_token_get_type (token) == ANJUTA_TOKEN_EOL) break;
		}
	}
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

AnjutaTokenStyle *
anjuta_token_style_new_from_base (AnjutaTokenStyle *base)
{
	AnjutaTokenStyle *style;
	
	style = g_slice_new0 (AnjutaTokenStyle);
	style->max_width = base->max_width;
	style->base = base;
	
	style->separator = g_hash_table_new (g_direct_hash, NULL);

	return style;
}

void
anjuta_token_style_free (AnjutaTokenStyle *style)
{
	g_hash_table_foreach (style->separator, (GHFunc)free_separator_list, NULL);
	g_hash_table_destroy (style->separator);
	g_slice_free (AnjutaTokenStyle, style);
}
