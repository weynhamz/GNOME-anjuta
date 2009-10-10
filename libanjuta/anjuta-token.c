/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-token.c
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

#include "anjuta-token.h"

#include "anjuta-debug.h"

#include <glib-object.h>

#include <stdio.h>
#include <string.h>

typedef struct _AnjutaTokenData AnjutaTokenData;

struct _AnjutaTokenData
{
	AnjutaTokenType type;	
	gint flags;
	gchar *pos;
	gsize length;
};

struct _AnjutaToken
{
	AnjutaTokenData	*data;
	AnjutaToken	*next;
	AnjutaToken	*prev;
	AnjutaToken	*parent;
	AnjutaToken	*children;
};
		
#define ANJUTA_TOKEN_DATA(node)  ((node) != NULL ? (AnjutaTokenData *)((node)->data) : NULL)


/* Helpers functions
 *---------------------------------------------------------------------------*/

/* Return true and update end if the token is found.
 * If a close token is found, return FALSE but still update end */
gboolean
anjuta_token_match (AnjutaToken *token, gint flags, AnjutaToken *sequence, AnjutaToken **end)
{

	for (; sequence != NULL; /*sequence = flags & ANJUTA_SEARCH_BACKWARD ? anjuta_token_previous (sequence) : anjuta_token_next (sequence)*/)
	{
		AnjutaToken *toka;
		AnjutaToken *tokb = token;

		for (toka = sequence; toka != NULL; toka = anjuta_token_next_sibling (toka))
		{
			if (anjuta_token_compare (toka, tokb))
			{
				tokb = anjuta_token_next (tokb);
				if (tokb == NULL)
				{
					if (end) *end = sequence;
					return TRUE;
				}
			}
			else
			{
				break;
			}
		}

		if (flags & ANJUTA_SEARCH_BACKWARD)
		{
			sequence = flags & ANJUTA_SEARCH_INTO ? anjuta_token_previous (sequence) : anjuta_token_previous_sibling (sequence);
		}
		else
		{
			sequence = flags & ANJUTA_SEARCH_INTO ? anjuta_token_next (sequence) : anjuta_token_next_sibling (sequence);
		}
	}
	/*g_message ("matched %p %d", sequence, level);*/
	
	if (end) *end = sequence;
	
	return FALSE;
}


gboolean
anjuta_token_remove (AnjutaToken *token)
{
	ANJUTA_TOKEN_DATA (token)->flags |= ANJUTA_TOKEN_REMOVED;

	return TRUE;
}

gboolean
anjuta_token_compare (AnjutaToken *toka, AnjutaToken *tokb)
{
	AnjutaTokenData *data = ANJUTA_TOKEN_DATA (toka);
	AnjutaTokenData *datb = ANJUTA_TOKEN_DATA (tokb);

	if (datb->type)
	{
		if (datb->type != data->type) return FALSE;
	}
	
	if (datb != ANJUTA_TOKEN_NONE)
	{
		if (datb->length != 0)
		{
			if (data->length != datb->length) return FALSE;
		
			if ((data->flags & ANJUTA_TOKEN_CASE_INSENSITIVE)  && (datb->flags & ANJUTA_TOKEN_CASE_INSENSITIVE))
			{
				if (g_ascii_strncasecmp (data->pos, datb->pos, data->length) != 0) return FALSE;
			}
			else
			{
				if (strncmp (data->pos, datb->pos, data->length) != 0) return FALSE;
			}
		}
	}
		
	if (datb->flags & ANJUTA_TOKEN_PUBLIC_FLAGS)
	{
		if ((data->flags & datb->flags & ANJUTA_TOKEN_PUBLIC_FLAGS) == 0)
			return FALSE;
	}

	return TRUE;
}

AnjutaToken *
anjuta_token_next_after_children (AnjutaToken *token)
{
	if (token->next != NULL)
	{
		return token->next;
	}
	else if (token->parent != NULL)
	{
		return anjuta_token_next_after_children (token->parent);
	}
	else
	{
		return NULL;
	}
}

AnjutaToken *
anjuta_token_next (AnjutaToken *token)
{
	if (token->children != NULL)
	{
		return token->children;
	}
	else if (token->next != NULL)
	{
		return token->next;
	}
	else if (token->parent != NULL)
	{
		return anjuta_token_next_after_children (token->parent);
	}
	else
	{
		return NULL;
	}
}

AnjutaToken *
anjuta_token_previous (AnjutaToken *token)
{
	if (token->prev != NULL)
	{
		return token->prev;
	}
	else
	{
		return token->parent;
	}
}

AnjutaToken *
anjuta_token_next_sibling (AnjutaToken *token)
{
	return token ? token->next : NULL;
}

AnjutaToken *
anjuta_token_next_child (AnjutaToken *token)
{
	return token ? token->children : NULL;
}

AnjutaToken *
anjuta_token_previous_sibling (AnjutaToken *token)
{
	return token->prev;
}

AnjutaToken *
anjuta_token_last_child (AnjutaToken *token)
{
	AnjutaToken *last = NULL;

	for (; token != NULL; token = (AnjutaToken *)g_node_last_child ((GNode *)last))
	{
		last = token;
	}

	return last;
}

AnjutaToken *
anjuta_token_parent (AnjutaToken *token)
{
	return token->parent;
}

void
anjuta_token_set_type (AnjutaToken *token, gint type)
{
	ANJUTA_TOKEN_DATA (token)->type = type;
}

void
anjuta_token_set_flags (AnjutaToken *token, gint flags)
{
	ANJUTA_TOKEN_DATA (token)->flags |= flags;
}

void
anjuta_token_clear_flags (AnjutaToken *token, gint flags)
{
	ANJUTA_TOKEN_DATA (token)->flags &= ~flags;
}

gint
anjuta_token_get_type (AnjutaToken *token)
{
	return ANJUTA_TOKEN_DATA (token)->type;
}

gint
anjuta_token_get_flags (AnjutaToken *token)
{
	return ANJUTA_TOKEN_DATA (token)->flags;
}

gchar *
anjuta_token_get_value (AnjutaToken *token)
{
	AnjutaTokenData *data = ANJUTA_TOKEN_DATA (token);

	return data && (data->pos != NULL) ? g_strndup (data->pos, data->length) : NULL;
}

const gchar *
anjuta_token_get_string (AnjutaToken *token)
{
	return ANJUTA_TOKEN_DATA (token)->pos;
}

guint
anjuta_token_get_length (AnjutaToken *token)
{
	return ANJUTA_TOKEN_DATA (token)->length;
}

static void
anjuta_token_evaluate_token (AnjutaToken *token, GString *value, gboolean raw)
{
	if ((token != NULL) && (ANJUTA_TOKEN_DATA (token)->length != 0))
	{
		if (!raw)
		{
			gint type = anjuta_token_get_type (token);
			if ((type == ANJUTA_TOKEN_COMMENT) || (type == ANJUTA_TOKEN_OPEN_QUOTE) || (type == ANJUTA_TOKEN_CLOSE_QUOTE) || (type == ANJUTA_TOKEN_ESCAPE))
			{
				return;
			}
		}
		g_string_append_len (value, anjuta_token_get_string (token), anjuta_token_get_length (token));
	}
}	

gchar *
anjuta_token_evaluate_range (AnjutaToken *start, AnjutaToken *end)
{
	GString *value = g_string_new (NULL);

	for (;;)
	{
		if (start == NULL) break;
		anjuta_token_evaluate_token (start, value, FALSE);
		if (start == end) break;
		
		start = anjuta_token_next (start);
	}
	

	return g_string_free (value, FALSE);
}

static  void
anjuta_token_evaluate_child (AnjutaToken *token, GString *value, gboolean raw)
{
	anjuta_token_evaluate_token (token, value, raw);

	if (token->children) anjuta_token_evaluate_child (token->children, value, raw);

	if (token->next) anjuta_token_evaluate_child (token->next, value, raw);
}

gchar *
anjuta_token_evaluate (AnjutaToken *token)
{
	GString *value = g_string_new (NULL);
	gchar *str;

	if (token != NULL)
	{
		anjuta_token_evaluate_token (token, value, FALSE);
		if (token->children) anjuta_token_evaluate_child (token->children, value, FALSE);
	}

	str = g_string_free (value, FALSE);
	return *str == '\0' ? NULL : str; 	
}

gchar *
anjuta_token_value (AnjutaToken *token)
{
	GString *value = g_string_new (NULL);
	gchar *str;

	if (token != NULL)
	{
		anjuta_token_evaluate_token (token, value, TRUE);
		if (token->children) anjuta_token_evaluate_child (token->children, value, TRUE);
	}

	str = g_string_free (value, FALSE);
	return *str == '\0' ? NULL : str; 	
}

AnjutaToken *
anjuta_token_merge (AnjutaToken *first, AnjutaToken *end)
{
	AnjutaToken *child;
	AnjutaToken *tok;

	if (first == end) return first;

	child = (AnjutaToken *)g_node_last_child ((GNode *)first);
	do
	{
		tok = (AnjutaToken *)g_node_next_sibling ((GNode *)first);
		if (tok == NULL) break;
		
		g_node_unlink ((GNode *)tok);
		child = (AnjutaToken *)g_node_insert_after ((GNode *)first, (GNode *)child, (GNode *)tok);

	}
	while (tok != end);

	return first;
}

AnjutaToken *
anjuta_token_merge_previous (AnjutaToken *first, AnjutaToken *end)
{
	AnjutaToken *child;
	AnjutaToken *tok;

	if (first == end) return first;

	child = (AnjutaToken *)g_node_first_child ((GNode *)first);
	do
	{
		tok = (AnjutaToken *)g_node_prev_sibling ((GNode *)first);
		if (tok == NULL) break;
		
		g_node_unlink ((GNode *)tok);
		child = (AnjutaToken *)g_node_insert_before ((GNode *)first, (GNode *)child, (GNode *)tok);

	}
	while (tok != end);

	return first;
}

AnjutaToken *
anjuta_token_copy (const AnjutaToken *token)
{
	AnjutaToken *copy = NULL;

	if (token != NULL)
	{
		AnjutaTokenData *org = ANJUTA_TOKEN_DATA (token);
		AnjutaTokenData *data = NULL;
		AnjutaToken *child;
		AnjutaToken *last;

		data = g_slice_new0 (AnjutaTokenData);
		data->type =org->type;
		data->flags = org->type;
		if ((data->flags & ANJUTA_TOKEN_STATIC) || (org->pos == NULL))
		{
			data->pos = org->pos;
		}
		else
		{
			data->pos = g_strdup (ANJUTA_TOKEN_DATA (token)->pos);
		}
		data->length = org->length;

		copy = (AnjutaToken *)g_node_new (data);

		last = NULL;
		for (child = anjuta_token_next_child (token); child != NULL; child = anjuta_token_next_sibling (child))
		{
			AnjutaToken *new_child = anjuta_token_copy (child);
			last =  last == NULL ? anjuta_token_insert_child (copy, new_child) : anjuta_token_insert_after (last, new_child);
		}
	}

	return copy;
}

AnjutaToken *
anjuta_token_clear (AnjutaToken *token)
{
	AnjutaTokenData *data = ANJUTA_TOKEN_DATA (token);

	if (!(data->flags & ANJUTA_TOKEN_STATIC))
	{
		g_free (data->pos);
	}
	data->length = 0;
	data->pos = NULL;

	return token;
}

AnjutaToken *
anjuta_token_delete (AnjutaToken *token)
{
	GNode *last;
	GNode *child;
	AnjutaToken *next;
	
	for (child = g_node_first_child ((GNode *)token); child != NULL; child = g_node_first_child ((GNode *)token))
	{
		g_node_unlink (child);
		last = g_node_insert_after (((GNode *)token)->parent, last, child);
	}

	next = (AnjutaToken *)g_node_next_sibling (token);
	anjuta_token_clear (token);
	g_node_destroy ((GNode *)token);

	return next;
}

AnjutaToken *
anjuta_token_insert_child (AnjutaToken *parent, AnjutaToken *child)
{
	return (AnjutaToken *)g_node_insert_after ((GNode *)parent, (GNode *)NULL, (GNode *)child);
}

AnjutaToken *
anjuta_token_insert_after (AnjutaToken *sibling, AnjutaToken *token)
{
	return (AnjutaToken *)g_node_insert_after ((GNode *)sibling->parent, (GNode *)sibling, (GNode *)token);
}

AnjutaToken *
anjuta_token_insert_before (AnjutaToken *sibling, AnjutaToken *baby)
{
	return (AnjutaToken *)g_node_insert_before ((GNode *)sibling->parent, (GNode *)sibling, (GNode *)baby);
}	

AnjutaToken *anjuta_token_group (AnjutaToken *parent, AnjutaToken *last)
{
	AnjutaToken *child;
	AnjutaToken *tok;

	if (parent == last) return parent; 
	if (parent->children == last) return parent;

	child = (AnjutaToken *)g_node_last_child ((GNode *)parent);
	do
	{
		tok = (AnjutaToken *)g_node_next_sibling ((GNode *)parent);
		if (tok == NULL) break;
		
		g_node_unlink ((GNode *)tok);
		child = (AnjutaToken *)g_node_insert_after ((GNode *)parent, (GNode *)child, (GNode *)tok);

	}
	while (tok != last);

	return parent;
	
}

AnjutaToken *anjuta_token_group_new (AnjutaTokenType type, AnjutaToken* first)
{
	AnjutaToken *parent = anjuta_token_new_static (type, NULL);

	g_node_insert_before ((GNode *)first->parent, (GNode *)first, (GNode *)parent);
	return anjuta_token_group (parent, first);
}

AnjutaToken *anjuta_token_ungroup (AnjutaToken *token)
{
	GNode *last = (GNode *)token;
	GNode *child;
	
	for (child = g_node_first_child ((GNode *)token); child != NULL; child = g_node_first_child ((GNode *)token))
	{
		g_node_unlink (child);
		last = g_node_insert_after (((GNode *)token)->parent, last, child);
	}

	return token;
}

AnjutaToken *anjuta_token_split (AnjutaToken *token, guint size)
{
	if (ANJUTA_TOKEN_DATA (token)->length > size)
	{
		AnjutaToken *copy;

		copy = anjuta_token_copy (token);
		g_node_insert_before ((GNode *)token->parent, (GNode *)token, (GNode *)copy);

		ANJUTA_TOKEN_DATA (copy)->length = size;
		if (ANJUTA_TOKEN_DATA (token)->flags & ANJUTA_TOKEN_STATIC)
		{
			ANJUTA_TOKEN_DATA (token)->pos += size;
			ANJUTA_TOKEN_DATA (token)->length -= size;
		}
		else
		{
			memcpy(ANJUTA_TOKEN_DATA (token)->pos, ANJUTA_TOKEN_DATA (token)->pos + size, ANJUTA_TOKEN_DATA (token)->length - size);
		}

		return copy;
	}
	else
	{
		return token;
	}
}

AnjutaToken *anjuta_token_get_next_arg (AnjutaToken *arg, gchar ** value)
{
	for (;arg != NULL;)
	{
		switch (anjuta_token_get_type (arg))
		{
			case ANJUTA_TOKEN_START:
			case ANJUTA_TOKEN_NEXT:
			case ANJUTA_TOKEN_LAST:
				arg = anjuta_token_next_sibling (arg);
				continue;
			default:
				*value = anjuta_token_evaluate (arg);
				arg = anjuta_token_next_sibling (arg);
				break;
		}
		break;
	}
	
	return arg;
}

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

AnjutaToken *anjuta_token_new_string (AnjutaTokenType type, const char *value)
{
	if (value == NULL)
	{
		return anjuta_token_new_static (type, NULL);
	}
	else
	{
		AnjutaTokenData *data;

		data = g_slice_new0 (AnjutaTokenData);
		data->type = type  & ANJUTA_TOKEN_TYPE;
		data->flags = type & ANJUTA_TOKEN_FLAGS;
		data->pos = g_strdup (value);
		data->length = strlen (value);

		return (AnjutaToken *)g_node_new (data);
	}
}
	
AnjutaToken *
anjuta_token_new_fragment (gint type, const gchar *pos, gsize length)
{
	AnjutaTokenData *data;

	data = g_slice_new0 (AnjutaTokenData);
	data->type = type  & ANJUTA_TOKEN_TYPE;
	data->flags = (type & ANJUTA_TOKEN_FLAGS) | ANJUTA_TOKEN_STATIC;
	data->pos = (gchar *)pos;
	data->length = length;

	return (AnjutaToken *)g_node_new (data);
};

AnjutaToken *anjuta_token_new_static (AnjutaTokenType type, const char *value)
{
	return anjuta_token_new_fragment (type, value, value == NULL ? 0 : strlen (value));	
}


static void
free_token_data (GNode *node, gpointer data)
{
	g_slice_free (AnjutaTokenData, node->data);
}

void
anjuta_token_free (AnjutaToken *token)
{
	if (token == NULL) return;

	g_node_children_foreach ((GNode *)token, G_TRAVERSE_ALL, free_token_data, NULL);
	g_node_destroy ((GNode *)token);
}
