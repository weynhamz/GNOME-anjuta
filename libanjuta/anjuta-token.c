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
#include <stdlib.h>

/**
 * SECTION:anjuta-token
 * @title: Anjuta token
 * @short_description: Anjuta token
 * @see_also:
 * @stability: Unstable
 * @include: libanjuta/anjuta-token.h
 *
 * A #AnjutaToken represents a token. It is a sequence of characters associated
 * with a type representing its meaning. By example, a token can represent
 * a keyword, a comment, a variable...
 *
 * The token can own the string or has only a pointer on some data allocated
 * somewhere else with a length.
 *
 * A token is linked with other tokens using three double linked lists.
 *
 * The first list using next and prev fields is used to keep the token in the
 * order where there are in the file. The first character of the first token is
 * the first character in the file.
 *
 * A second list is used to represent included
 * files. Such file is represented by a special token in the first list which
 * has a pointer, named children to a token list. Each token in this secondary
 * list has a pointer to its parent, it means the token representing the file
 * where is the token. It looks like a tree. In fact, every file is represented
 * by a special token, so the root node is normally a file token and has as
 * children all the token representing the file content. This parent/child list
 * is used for expanded variable too.
 *
 * A third list is used to group several tokens. A token can have a pointer to
 * another last token. It means that this token is a group starting from this
 * token to the one indicated by the last field. In addition each token in this
 * group has a pointer on the first token of the group. This grouping is
 * independent of the parent/child list. So a group can start in one file and
 * end in another included file. The grouping can be nested too. Typically
 * we can have a group representing a command, a sub group representing the
 * arguments and then one sub group for each argument.
 */

/*
 * A token with its pointer can be put in one of the following case:
 *
 * * Simple token:
 *		children = NULL && last = NULL
 *		This is the case of a simple token without any children.
 *
 * * Composite token:
 *		children = NULL && last != NULL
 *		This is the case of a token grouping several other tokens, representing
 *		by example an item in a list.
 *
 * * Parent token:
 *		children != NULL && last == NULL
 *		This is the case of a token having children, by example a variable where
 *		the children represent the content. Most of the time the value of the
 *		parent token is ignored.
 *
 * * Composite parent token:
 *		children != NULL && last != NULL && (last in list)
 *		This case represents a variable which is split into several tokens. The
 *		children represents the content of the variable. The token up to last
 *		corresponds to the variable name. After getting the variable content, the
 *		next token is after the last token of the variable.
 *
 * * Composite parent token:
 *		children != NULL && last != NULL && (last in children)
 *		This case would represent a variable which is split into several token
 *		with one inside the children of the variable. This is not possible as
 *		the complete name of the variable is needed to get the children.
 *
 * * Composite parent token:
 *		children != NULL && last != NULL && (last in parent)
 *		This case represents a variable split into several token where the
 *		last element is a sibling of one parent of the token. I think this case
 *		is possible.
 *
 */

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
	AnjutaToken	*next;
	AnjutaToken	*prev;
	AnjutaToken	*parent;
	AnjutaToken *last;
	AnjutaToken *group;
	AnjutaToken	*children;
	AnjutaTokenData data;
};

/* Helpers functions
 *---------------------------------------------------------------------------*/

/* Private functions
 *---------------------------------------------------------------------------*/

static AnjutaToken *
anjuta_token_next_child (AnjutaToken *child, AnjutaToken **last)
{
	if (child == NULL) return child;

	if (child->children != NULL)
	{
		child = child->children;
	}
	else
	{
		for (;;)
		{
			if ((*last == NULL) || (child == *last))
			{
				if (child->last == NULL)
				{
					child = NULL;
					break;
				}
				*last = child->last;
			}
			if (child->next != NULL)
			{
				child = child->next;
				break;
			}
			child = child->parent;
			if (child == NULL) break;
		}
	}

	return child;
}

AnjutaToken *
anjuta_token_next_after_children (AnjutaToken *token)
{
	while (token->last != NULL) token = token->last;
	while (token->next == NULL)
	{
		token = token->parent;
		if (token == NULL) return NULL;
		while (token->last != NULL) token = token->last;
	};

	return token->next;
}

static AnjutaToken *
anjuta_token_copy (AnjutaToken *token)
{
	AnjutaToken *copy = NULL;

	if (token != NULL)
	{
		copy = g_slice_new0 (AnjutaToken);
		copy->data.type = token->data.type;
		copy->data.flags = token->data.flags;
		if ((copy->data.flags & ANJUTA_TOKEN_STATIC) || (token->data.pos == NULL))
		{
			copy->data.pos = token->data.pos;
		}
		else
		{
			copy->data.pos = g_strdup (token->data.pos);
		}
		copy->data.length = token->data.length;
	}

	return copy;
}

/**
 * anjuta_token_unlink_token:
 * @token: a #AnjutaToken object.
 *
 * Unlink a single token, not the complete item, from the token tree.
 *
 * Return value: the removed token tree
 */
static AnjutaToken *
anjuta_token_unlink_token (AnjutaToken *token)
{

	if (token->prev != NULL)
	{
		token->prev->next = token->next;
	}
	else if ((token->parent != NULL) && (token->parent->children == token))
	{
		token->parent->children = token->next;
	}
	token->parent = NULL;

	if ((token->group != NULL) && (token->group->last == token))
	{
		AnjutaToken *prev;

		for (prev = token->prev; prev != NULL; prev = prev->prev)
		{
			if (prev->group == token->group)
			{
				/* Find previous token in the same group */
				token->group->last = prev;
				break;
			}
			else if (prev == token->group)
			{
				/* No more token in group */
				token->group->last = NULL;
				break;
			}
		}
	}

	if (token->next != NULL)
	{
		token->next->prev = token->prev;
		token->next = NULL;
	}
	token->prev = NULL;

	return token;
}

/**
 * anjuta_token_insert_token_before:
 * @sibling: a #AnjutaToken object.
 * @token: a #AnjutaToken object.
 *
 * Insert token before sibling.
 *
 * Return value: inserted token
 */
static AnjutaToken *
anjuta_token_insert_token_before (AnjutaToken *sibling, AnjutaToken *token)
{
	token->prev = sibling->prev;
	token->next = sibling;

	if (token->prev != NULL)
	{
		token->prev->next = token;
	}
	sibling->prev = token;

	if ((sibling->parent != NULL) && (sibling->parent->children == sibling))
	{
		sibling->parent->children = token;
	}
	token->parent = sibling->parent;

	return token;
}

static void
anjuta_token_evaluate_token (AnjutaToken *token, GString *value, gboolean raw)
{
	if ((token != NULL) && (token->data.length != 0))
	{
		if (!raw)
		{
			switch (anjuta_token_get_type (token))
			{
			case ANJUTA_TOKEN_COMMENT:
			case ANJUTA_TOKEN_OPEN_QUOTE:
			case ANJUTA_TOKEN_CLOSE_QUOTE:
			case ANJUTA_TOKEN_ESCAPE:
			case ANJUTA_TOKEN_MACRO:
			case ANJUTA_TOKEN_EOV:
				return;
			default:
				break;
			}
		}
		g_string_append_len (value, anjuta_token_get_string (token), anjuta_token_get_length (token));
	}
}

static void
anjuta_token_show (AnjutaToken *token, gint indent, gchar parent)
{
	static gchar type[] = "\0";
	const gchar *string;
	gsize length;

	type[0] = parent;
	fprintf (stderr, "%*s%s %p", indent, "", type, token);
	fprintf (stderr, ": %d ",
	    anjuta_token_get_type (token));
	string = anjuta_token_get_string (token);
	length = anjuta_token_get_length (token);
	if (string == NULL)
	{
		/* Value doesn't contain a newline */
		fprintf (stderr, "(%" G_GSIZE_FORMAT ")", length);
	}
	else
	{
		const gchar *newline;

		newline = g_strrstr_len (string, length, "\n");
		if (newline == NULL)
		{
			/* Value doesn't contain a newline */
			fprintf (stderr, "\"%.*s\"",
				(int)length,
		    	string);
		}
		else
		{
			/* Value contains a newline, take care of indentation */
			newline++;
			fprintf (stderr, "\"%.*s",
	    		(int)(newline - string),
		    	string);
			for (;;)
			{
				length -= newline - string;
				string = newline;

				newline = g_strrstr_len (string, length, "\n");
				if (newline == NULL) break;

				newline++;
				fprintf (stderr, "%*s  %.*s",
				    indent, "",
					(int)(newline - string),
	    			string);
			}
			fprintf (stderr, "%*s  %.*s\"",
				    indent, "",
					(int)length,
	    			string);
		}
	}
	fprintf (stderr, " %p/%p (%p/%p) %s\n",
		token->last, token->children,
		token->group, token->parent,
		anjuta_token_get_flags (token) & ANJUTA_TOKEN_REMOVED ? " (removed)" : "");
}

static AnjutaToken*
anjuta_token_dump_child (AnjutaToken *token, gint indent, gchar type)
{
	AnjutaToken *last;
	AnjutaToken *child;

	anjuta_token_show (token, indent, type);
	indent += 4;

	last = token;
	if (token->last != NULL)
	{
		do
		{
			child = last->next;
			if (child == NULL) break;
			last = anjuta_token_dump_child (child, indent, '+');
		}
		while (child != token->last);
	}

	if (token->children != NULL)
	{
		for (child = token->children; child != NULL; child = child->next)
		{
			child = anjuta_token_dump_child (child, indent, '*');
		}
	}

	return last;
}

static gboolean
anjuta_token_check_child (AnjutaToken *token, AnjutaToken *parent)
{
	if (token->parent != parent)
	{
		anjuta_token_show (token, 0, 0);
		fprintf(stderr, "Error: Children has %p as parent instead of %p\n", token->parent, parent);
		return FALSE;
	}

	return anjuta_token_check (token);
}

/* Get and set functions
 *---------------------------------------------------------------------------*/

void
anjuta_token_set_type (AnjutaToken *token, gint type)
{
	token->data.type = type;
}

gint
anjuta_token_get_type (AnjutaToken *token)
{
	return token->data.type;
}

void
anjuta_token_set_flags (AnjutaToken *token, gint flags)
{
	AnjutaToken *child;
	AnjutaToken *last = token->last;

	for (child = token; child != NULL; child = anjuta_token_next_child (child, &last))
	{
		child->data.flags |= flags;
	}
}

void
anjuta_token_clear_flags (AnjutaToken *token, gint flags)
{
	token->data.flags &= ~flags;
}

gint
anjuta_token_get_flags (AnjutaToken *token)
{
	return token->data.flags;
}

void
anjuta_token_set_string (AnjutaToken *token, const gchar *data, gsize length)
{
	if (!(token->data.flags & ANJUTA_TOKEN_STATIC))
	{
		g_free (token->data.pos);
		token->data.flags |= ANJUTA_TOKEN_STATIC;
	}
	token->data.pos = (gchar *)data;
	token->data.length = length;
}

const gchar *
anjuta_token_get_string (AnjutaToken *token)
{
	return token->data.pos;
}

void
anjuta_token_set_length (AnjutaToken *token, gsize length)
{
	token->data.length = length;
}

gsize
anjuta_token_get_length (AnjutaToken *token)
{
	return token->data.length;
}

/* Basic move functions
 *---------------------------------------------------------------------------*/

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
anjuta_token_last (AnjutaToken *token)
{
	AnjutaToken *last;

	for (last = token; last->last != NULL; last = last->last);
	if (last->children != NULL)
	{
		for (last = last->children; last->next != NULL; last = last->next);
	}

	return last;
}

AnjutaToken *
anjuta_token_parent (AnjutaToken *token)
{
	return token->parent;
}

AnjutaToken *
anjuta_token_list (AnjutaToken *token)
{
	return token->group;
}

/* Item move functions
 *---------------------------------------------------------------------------*/

AnjutaToken *
anjuta_token_last_item (AnjutaToken *list)
{
	return list->last;
}

AnjutaToken *
anjuta_token_first_item (AnjutaToken *list)
{
	AnjutaToken *first = NULL;

	if (list != NULL)
	{
		if (list->children != NULL)
		{
			first = list->children;
		}
		else if (list->last != NULL)
		{
			first = list->next;
		}
	}

	return first;
}

AnjutaToken *
anjuta_token_next_item (AnjutaToken *item)
{
	AnjutaToken *next;

	if (item != NULL)
	{
		do
		{
			next = NULL;
			if ((item->group == NULL) || (item->group->last != item))
			{
				AnjutaToken *last;
				for (last = item; last->last != NULL; last = last->last);
				next = anjuta_token_next (last);
				if ((next != NULL) && (next->group != item->group)) next = NULL;
			}
			item = next;
		}
		/* Loop if the current item has been deleted */
		while ((next != NULL) && (anjuta_token_get_flags (next) & ANJUTA_TOKEN_REMOVED));
	}

	return next;
}

AnjutaToken *
anjuta_token_previous_item (AnjutaToken *item)
{
	AnjutaToken *prev = NULL;


	if (item != NULL)
	{
		do
		{
			for (prev = item->prev; (prev != NULL) && (prev->group != item->group); prev = prev->group);
			item = prev;
		}
		/* Loop if the current item has been deleted */
		while ((prev != NULL) && (anjuta_token_get_flags (prev) & ANJUTA_TOKEN_REMOVED));
	}

	return prev;
}

/* Add/Insert/Remove tokens
 *---------------------------------------------------------------------------*/

/**
 * anjuta_token_append_child:
 * @parent: a #AnjutaToken object used as parent.
 * @children: a #AnjutaToken object.
 *
 * Insert all tokens in children as the last children of the given parent.
 *
 * Return value: The first token append.
 */
AnjutaToken *
anjuta_token_append_child (AnjutaToken *parent, AnjutaToken *children)
{
	AnjutaToken *token;
	AnjutaToken *last;
	AnjutaToken *old_group;
	AnjutaToken *old_parent;

	g_return_val_if_fail (parent != NULL, NULL);
	g_return_val_if_fail (children != NULL, NULL);

	old_group = children->group;
	old_parent = children->parent;

	if (parent->children == NULL)
	{
		parent->children = children;

		children->prev = NULL;
	}
	else
	{
		/* Find last children */
		for (last = parent->children; last->next != NULL;)
		{
			if ((last->last != NULL) && (last->last->parent == last->parent))
			{
				last = last->last;
			}
			else
			{
				last = last->next;
			}
		}

		last->next = children;
		children->prev = last;
	}

	/* Update each token */
	for (token = children;;)
	{
		if (token->parent == old_parent) token->parent = parent;
		if (token->group == old_group) token->group = parent->group;

		if (token->children != NULL)
		{
			token = token->children;
		}
		else if (token->next != NULL)
		{
			token = token->next;
		}
		else
		{
			while (token->parent != parent)
			{
				token = token->parent;
				if (token->next != NULL) break;
			}
			if (token->next == NULL) break;
			token = token->next;
		}
	}

	return children;
}

/**
 * anjuta_token_prepend_child:
 * @parent: a #AnjutaToken object used as parent.
 * @children: a #AnjutaToken object.
 *
 * Insert all tokens in children as the first children of the given parent.
 *
 * Return value: The first token append.
 */
AnjutaToken *
anjuta_token_prepend_child (AnjutaToken *parent, AnjutaToken *children)
{
	AnjutaToken *child;
	AnjutaToken *last = NULL;

	g_return_val_if_fail (parent != NULL, NULL);
	g_return_val_if_fail (children != NULL, NULL);

	/* Update each token */
	for (child = children;;)
	{
		AnjutaToken *next;

		if (child->parent == children->parent) child->parent = parent;
		if (child->group == children->group) child->group = parent->group;

		next = anjuta_token_next_child (child, &last);
		if (next == NULL) break;
		child = next;
	}

	child->next = parent->children;
	if (child->next) child->next->prev = child;
	parent->children = children;

	return children;
}

/**
 * anjuta_token_prepend_items:
 * @list: a #AnjutaToken object used as list.
 * @item: a #AnjutaToken object.
 *
 * Insert all tokens in item as item of the given list.
 *
 * Return value: The first token append.
 */
AnjutaToken *
anjuta_token_prepend_items (AnjutaToken *list, AnjutaToken *item)
{
	AnjutaToken *token;
	AnjutaToken *old_group;
	AnjutaToken *old_parent;

	g_return_val_if_fail (list != NULL, NULL);
	g_return_val_if_fail (item != NULL, NULL);

	old_group = item->group;
	old_parent = item->parent;

	/* Update each token */
	for (token = item;;)
	{
		if (token->parent == old_parent) token->parent = list->parent;
		if (token->group == old_group) token->group = list;

		if (token->children != NULL)
		{
			token = token->children;
		}
		else if (token->next != NULL)
		{
			token = token->next;
		}
		else
		{
			while (token->parent != list->parent)
			{
				token = token->parent;
				if (token->next != NULL) break;
			}
			if (token->next == NULL) break;
			token = token->next;
		}
	}

	token->next = list->next;
	if (token->next) token->next->prev = token;

	list->next = item;
	item->prev = list;

	if (list->last == NULL)
	{
		while (token->group != list) token = token->group;
		list->last = token;
	}

	return item;
}

/**
 * anjuta_token_insert_after:
 * @sibling: a #AnjutaToken object.
 * @list: a #AnjutaToken object.
 *
 * Insert all tokens after sibling.
 *
 * Return value: The first token inserted.
 */
AnjutaToken *
anjuta_token_insert_after (AnjutaToken *sibling, AnjutaToken *list)
{
	AnjutaToken *last;
	AnjutaToken *token;
	AnjutaToken *old_group;
	AnjutaToken *old_parent;

	g_return_val_if_fail (sibling != NULL, NULL);
	g_return_val_if_fail (list != NULL, NULL);

	old_group = list->group;
	old_parent = list->parent;

	/* Update each token */
	for (token = list;;)
	{
		if (token->parent == old_parent) token->parent = sibling->parent;
		if (token->group == old_group) token->group = sibling->group;

		if (token->children != NULL)
		{
			token = token->children;
		}
		else if (token->next != NULL)
		{
			token = token->next;
		}
		else
		{
			while (token->parent != sibling->parent)
			{
				token = token->parent;
				if (token->next != NULL) break;
			}
			if (token->next == NULL) break;
			token = token->next;
		}
	}

	for (last = sibling; last->last != NULL; last = last->last);

	token->next = last->next;
	if (token->next) token->next->prev = token;

	last->next = list;
	list->prev = last;

	if ((sibling->group != NULL) && (sibling->group->last == sibling))
	{
		while (token->group != sibling->group) token = token->group;
		sibling->group->last = token;
	}

	return list;
}

/**
 * anjuta_token_insert_before:
 * @sibling: a #AnjutaToken object.
 * @list: a #AnjutaToken object.
 *
 * Insert all tokens before sibling.
 *
 * Return value: The first token inserted.
 */
AnjutaToken *
anjuta_token_insert_before (AnjutaToken *sibling, AnjutaToken *list)
{
	AnjutaToken *last;
	AnjutaToken *token;
	AnjutaToken *old_group;
	AnjutaToken *old_parent;

	g_return_val_if_fail (sibling != NULL, NULL);
	g_return_val_if_fail (list != NULL, NULL);

	old_group = list->group;
	old_parent = list->parent;

	/* Update each token */
	for (token = list;;)
	{
		if (token->parent == old_parent) token->parent = sibling->parent;
		if (token->group == old_group) token->group = sibling->group;

		if (token->children != NULL)
		{
			token = token->children;
		}
		else if (token->next != NULL)
		{
			token = token->next;
		}
		else
		{
			while (token->parent != sibling->parent)
			{
				token = token->parent;
				if (token->next != NULL) break;
			}
			if (token->next == NULL) break;
			token = token->next;
		}
	}

	for (last = sibling; last->last != NULL; last = last->last);

	token->next = sibling;
	list->prev = sibling->prev;
	sibling->prev = token;

	if (list->prev) list->prev->next = list;

	if ((list->parent != NULL) && (list->parent->children == sibling)) list->parent->children = list;

	return list;
}

/**
 * anjuta_token_delete_parent:
 * @parent: a #AnjutaToken object used as parent.
 *
 * Delete only the parent token.
 *
 * Return value: the first children
 */
AnjutaToken *
anjuta_token_delete_parent (AnjutaToken *parent)
{
	AnjutaToken *token;

	g_return_val_if_fail (parent != NULL, NULL);

	if (parent->children == NULL) return NULL;

	/* Update each token */
	for (token = parent->children;;)
	{
		if (token->parent == parent) token->parent = parent->parent;

		if (token->children != NULL)
		{
			token = token->children;
		}
		else if (token->next != NULL)
		{
			token = token->next;
		}
		else
		{
			while (token->parent != parent->parent)
			{
				token = token->parent;
				if (token->next != NULL) break;
			}
			if (token->next == NULL) break;
			token = token->next;
		}
	}

	token->next = parent->next;
	if (token->next) token->next->prev = token;

	parent->next = parent->children;
	parent->children->prev = parent;
	parent->children = NULL;

	return anjuta_token_free (parent);
}

/* Merge function
 *---------------------------------------------------------------------------*/

/* anjuta_token_merge can be used with first or end being a floating token and
 * on already grouped tokens to change the group organisation */
AnjutaToken *
anjuta_token_merge (AnjutaToken *first, AnjutaToken *end)
{
	AnjutaToken *next;

	if ((first == end) || (end == NULL)) return first;

	/* Insert first or end in the same sequence if it is not already the case */
	for (next = first; next != end; next = anjuta_token_next (next))
	{
		if (next == NULL)
		{
			if (first->parent == NULL)
			{
				anjuta_token_insert_before (end, first);
			}
			else
			{
				anjuta_token_insert_after (first, end);
			}
			break;
		}
	}
	first->last = end;
	if ((end->group != NULL) && (end->group != first) && (end->group->last == end)) end->group->last = first;
	end->group = first;

	return first;
}

AnjutaToken *
anjuta_token_merge_own_children (AnjutaToken *group)
{
	AnjutaToken *token;
	AnjutaToken *next = NULL;

	if (group->last != NULL) return group;

	if (group->last->last != NULL) group->last = group->last->last;

	for (token = anjuta_token_next (group); (token != NULL) && (token != group->last); token = anjuta_token_next (token))
	{
		if (next == NULL)
		{
			if (token->last != NULL)
			{
				next = token->last;
				//token->last = NULL;
			}
			token->group = group;
		}
		else if (next == token)
		{
			next = NULL;
		}
	}

	return group;
}

AnjutaToken *
anjuta_token_merge_children (AnjutaToken *first, AnjutaToken *end)
{
	if (first == NULL)
	{
		return end;
	}
	if ((first == end) || (end == NULL))
	{
		return first;
	}

	if (first->parent == NULL)
	{
		first->parent = end->parent;
	}
	if (first->next == NULL)
	{
		anjuta_token_insert_before (end, first);
	}
	anjuta_token_unlink_token (end);
	if (end->last != NULL)
	{
		AnjutaToken *child;

		first->last = end->last;
		for (child = anjuta_token_next (first); child != first->last; child = anjuta_token_next (child))
		{
			if (child->group == end) child->group = first;
		}
		first->last->group = first;
	}
	end->group = first;
	anjuta_token_free (end);

	return first;
}

/**
 * anjuta_token_merge_previous:
 * @list: a #AnjutaToken object representing a list
 * @first: a #AnjutaToken object for the new beginning of the list
 *
 * If the list token is not already linked with first, it is inserted
 * just before first.
 * If the list token is already linked, it must be in the same list after
 * first token. It it possible to have several tokens beweent list and
 * first.
 *
 * Return value: the new list
 */
AnjutaToken *
anjuta_token_merge_previous (AnjutaToken *list, AnjutaToken *first)
{
	AnjutaToken *token;

	if ((first == NULL) || (list == first)) return list;

	/* Change group of all tokens from end to first
	 * if the list is already linked */
	if ((list->prev != NULL) || (list->parent != NULL))
	{
		for (token = first; token != NULL; token = anjuta_token_next_item (token))
		{
			token->group = list;
		}
	}

	token = anjuta_token_next (list);
	anjuta_token_unlink_token (list);
	anjuta_token_insert_token_before (first, list);

	return list;
}

AnjutaToken *anjuta_token_split (AnjutaToken *token, guint size)
{
	if (token->data.length > size)
	{
		AnjutaToken *copy;

		copy = anjuta_token_copy (token);
		anjuta_token_insert_before (token, copy);

		copy->data.length = size;
		if (token->data.flags & ANJUTA_TOKEN_STATIC)
		{
			token->data.pos += size;
			token->data.length -= size;
		}
		else
		{
			memcpy(token->data.pos, token->data.pos + size, token->data.length - size);
		}

		return copy;
	}
	else
	{
		return token;
	}
}

AnjutaToken *
anjuta_token_cut (AnjutaToken *token, guint pos, guint size)
{
	AnjutaToken *copy;

	copy = anjuta_token_copy (token);

	if (pos >= token->data.length)
	{
		if (!(copy->data.flags & ANJUTA_TOKEN_STATIC))
		{
			g_free (copy->data.pos);
		}
		copy->data.pos = NULL;
		copy->data.length = 0;
	}
	if ((pos + size) > token->data.length)
	{
		size = token->data.length - pos;
	}

	if (copy->data.flags & ANJUTA_TOKEN_STATIC)
	{
		copy->data.pos += pos;
	}
	else
	{
		memmove(copy->data.pos, copy->data.pos + pos, size);
	}
	copy->data.length = size;

	return copy;
}

static void
concat_token (AnjutaToken *token, gpointer user_data)
{
	AnjutaToken *first = (AnjutaToken *)user_data;

	if (anjuta_token_get_length (token) > 0)
	{
		if (anjuta_token_get_string (first) == NULL)
		{
			anjuta_token_set_string (first, anjuta_token_get_string (token), anjuta_token_get_length (token));
		}
		else if ((first != NULL) && ((anjuta_token_get_string(first) + anjuta_token_get_length (first)) == anjuta_token_get_string (token)))
		{
			anjuta_token_set_string (first, anjuta_token_get_string (first), anjuta_token_get_length (first) + anjuta_token_get_length (token));
		}
		else
		{
			AnjutaToken *new;

			new = anjuta_token_new_static_len (ANJUTA_TOKEN_CONTENT, anjuta_token_get_string (token), anjuta_token_get_length (token));
			anjuta_token_insert_after (first, new);
			anjuta_token_merge (first, new);
		}
	}
}


AnjutaToken *
anjuta_token_concat(AnjutaToken *token)
{
	AnjutaToken *new;

	new = anjuta_token_new_static (ANJUTA_TOKEN_CONTENT, NULL);
	anjuta_token_foreach_token (token, concat_token, new);

	anjuta_token_insert_token_before (token, new);
	anjuta_token_free (token);

	return new;
}

/* Token foreach
 *---------------------------------------------------------------------------*/

void
anjuta_token_foreach_token (AnjutaToken *token, AnjutaTokenForeachFunc func, gpointer user_data)
{
	if (token != NULL)
	{
		AnjutaToken *last_token;
		gint child = 0;

		last_token = token->last == NULL ? token : token->last;
		while (token != NULL)
		{
			if (child == 0) func (token, user_data);

			/* Check if we have found the last token */
			if (token == last_token)
			{
				/* Find last token */
				if (token->last == NULL)
				{
					break;
				}
				/* Last token still include additional tokens */
				last_token = token->last;
			}

			if (token->children != NULL)
			{
				/* Check children, only for last token */
				child++;
				token = token->children;
			}
			else if (token->next != NULL)
			{
				/* Get next sibling */
				token = token->next;
			}
			else
			{
				/* Get parent */
				for (;;)
				{
					child--;
					token = token->parent;
					if (token == NULL) break;
					if (token->next != NULL)
					{
						token = token->next;
						break;
					}
				}
			}
		}
	}

	return;
}

void
anjuta_token_foreach_content (AnjutaToken *token, AnjutaTokenForeachFunc func, gpointer user_data)
{
	if (token != NULL)
	{
		AnjutaToken *last_parent;	/* If not NULL, token belong to a parent and
		 							 * are not taken into account */
		AnjutaToken *last_token;
		gboolean expand = TRUE;

		last_parent = NULL;
		last_token = token->last == NULL ? token : token->last;
		for (;;)
		{
			if (expand && (token->children != NULL))
			{
				/* Children of the last token does not belong to the group */
				if (token == last_token)
				{
					/* Find last token */
					if (token->last == NULL)
					{
						break;
					}
					/* Last token still include additional tokens */
					last_token = token->last;
				}

				/* Enumerate children */
				token = token->children;
			}
			else
			{
				if (token->children == NULL)
				{
					/* Take into account only the content of token having no children */
					if (last_parent == NULL)
					{
						/* Take into account only the content of group having no children */
						func (token, user_data);
					}
				}

				/* Check if we have found the last token */
				if (token == last_token)
				{
					/* Find last token */
					if (token->last == NULL)
					{
						break;
					}
					/* Last token still include additional tokens */
					last_token = token->last;
				}

				if (token == last_parent)
				{
					/* Find last parent */
					if (token->last == NULL)
					{
						/* Found complete group having children */
						last_parent = NULL;
					}
					else
					{
						/* Parent group has additional token */
						last_parent = token->last;
					}
				}

				if (token->next != NULL)
				{
					/* Get next sibling */
					token = token->next;
					expand = TRUE;
				}
				else
				{
					/* Get parent */
					token = token->parent;
					expand = FALSE;
					if (token == NULL) break;
					last_parent = token->last;
				}
			}
		}
	}

	return;
}

static void
anjuta_token_foreach_container (AnjutaToken *token, AnjutaTokenForeachFunc func, gpointer user_data)
{
	if (token != NULL)
	{
		AnjutaToken *parent;	/* If not NULL, token belong to a parent and
		 							 * are not taken into account */
		AnjutaToken *last_token;
		gboolean expand = TRUE;

		parent = NULL;
		last_token = token->last == NULL ? token : token->last;
		for (;;)
		{
			/* Take into account only the content of parent token */
			if (expand && (parent == NULL))
			{
				func (token, user_data);
			}
			if (expand && (token->children != NULL))
			{
				if (parent == NULL) parent = token;
				/* Enumerate children */
				token = token->children;
			}
			else
			{
				/* Check if we have found the last token */
				if (token == last_token)
				{
					/* Find last token */
					if (token->last == NULL)
					{
						break;
					}
					/* Last token still include additional tokens */
					last_token = token->last;
				}

				if (token->next != NULL)
				{
					/* Get next sibling */
					token = token->next;
					expand = TRUE;
				}
				else
				{
					/* Get parent */
					token = token->parent;
					if (token == parent) parent = NULL;
					expand = FALSE;
				}
			}
		}
	}

	return;
}

AnjutaToken *
anjuta_token_foreach_post_order (AnjutaToken *token, AnjutaTokenForeachFunc func, gpointer user_data)
{
	if (token != NULL)
	{
		AnjutaToken *last_parent;	/* If not NULL, token belong to a parent */
		AnjutaToken *last_token;
		AnjutaToken buffer;			/* Temporary token allowing func to destroy
		 							 * the current token */
		gboolean expand = TRUE;

		last_parent = NULL;
		last_token = token->last == NULL ? token : token->last;
		while (token != NULL)
		{
			if (expand && (token->children != NULL))
			{
				/* Check if we have found the last token */
				if (token == last_token)
				{
					/* Find last token */
					if (token->last == NULL)
					{
						break;
					}
					/* Last token still include additional tokens */
					last_token = token->last;
				}

				/* Enumerate children */
				token = token->children;
			}
			else
			{
				/* Save token data in case it is destroyed */
				memcpy (&buffer, token, sizeof (buffer));
				/* Take into account all token */
				func (token, user_data);

				/* Check if we have found the last token */
				if (token == last_token)
				{
					/* Find last token */
					if (buffer.last == NULL)
					{
						token = &buffer;
						break;
					}
					/* Last token still include additional tokens */
					last_token = buffer.last;
				}

				if (token == last_parent)
				{
					/* Find last parent */
					if (buffer.last == NULL)
					{
						/* Found complete group having children */
						last_parent = NULL;
					}
					else
					{
						/* Parent group has additional token */
						last_parent = buffer.last;
					}
				}

				if (buffer.next != NULL)
				{
					/* Get next sibling */
					token = buffer.next;
					expand = TRUE;
				}
				else
				{
					/* Get parent */
					token = buffer.parent;
					if (token != NULL) last_parent = token->last;
					expand = FALSE;
				}
			}
		}

		while ((token != NULL) && (token->next == NULL))
		{
			token = token->parent;
		};

		if (token != NULL) token = token->next;
	}

	return token;
}

/* Token evaluation
 *---------------------------------------------------------------------------*/

static void
evaluate_token (AnjutaToken *token, gpointer user_data)
{
	GString *value = (GString *)user_data;

	anjuta_token_evaluate_token (token, value, FALSE);
}


gchar *
anjuta_token_evaluate (AnjutaToken *token)
{
	GString *value = g_string_new (NULL);

	anjuta_token_foreach_content (token, evaluate_token, value);

	/* Return NULL and free data for an empty string */
	return g_string_free (value, *(value->str) == '\0');
}

/* Does not evaluate content if token is a variable */
gchar *
anjuta_token_evaluate_name (AnjutaToken *token)
{
	GString *value = g_string_new (NULL);

	anjuta_token_foreach_container (token, evaluate_token, value);

	/* Return NULL and free data for an empty string */
	return g_string_free (value, *(value->str) == '\0');
}

gboolean
anjuta_token_is_empty (AnjutaToken *token)
{
	return (token == NULL) || ((token->data.length == 0) && (token->last == NULL) && (token->children == NULL));
}


/* Other functions
 *---------------------------------------------------------------------------*/

gboolean
anjuta_token_compare (AnjutaToken *toka, AnjutaToken *tokb)
{
	if (tokb->data.type)
	{
		if (tokb->data.type != toka->data.type) return FALSE;
	}

	if (tokb->data.type != ANJUTA_TOKEN_NONE)
	{
		if (tokb->data.length != 0)
		{
			if (toka->data.length != tokb->data.length) return FALSE;

			if ((toka->data.flags & ANJUTA_TOKEN_CASE_INSENSITIVE)  && (tokb->data.flags & ANJUTA_TOKEN_CASE_INSENSITIVE))
			{
				if (g_ascii_strncasecmp (toka->data.pos, tokb->data.pos, toka->data.length) != 0) return FALSE;
			}
			else
			{
				if (strncmp (toka->data.pos, tokb->data.pos, toka->data.length) != 0) return FALSE;
			}
		}
	}

	if (tokb->data.flags & ANJUTA_TOKEN_PUBLIC_FLAGS)
	{
		if ((toka->data.flags & tokb->data.flags & ANJUTA_TOKEN_PUBLIC_FLAGS) == 0)
			return FALSE;
	}

	return TRUE;
}

void
anjuta_token_dump (AnjutaToken *token)
{
	if (token == NULL) return;

	anjuta_token_dump_child (token, 0, 0);
}

void
anjuta_token_dump_link (AnjutaToken *token)
{
	AnjutaToken *last = token;

	while (last->last != NULL) last = last->last;

	for (; token != last; token = anjuta_token_next (token))
	{
		anjuta_token_show (token, 0, 0);
	}
}

gboolean
anjuta_token_check (AnjutaToken *token)
{
	if ((token->children != NULL) && (token->last != NULL))
	{
		anjuta_token_show (token, 0, 0);
		fprintf(stderr, "Error: Previous token has both non NULL children and last\n");

		return FALSE;
	}

	if (token->children != NULL)
	{
		AnjutaToken *child;

		for (child = token->children; child != NULL; child = child->next)
		{
			if (!anjuta_token_check_child (child, token)) return FALSE;
		}
	}

	if (token->last != NULL)
	{
		AnjutaToken *child;

		for (child = anjuta_token_next (token); child != NULL; child = anjuta_token_next (child))
		{
			if (!anjuta_token_check (child)) return FALSE;
			if (child == token->last) break;
		}
	}

	return TRUE;
}

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

AnjutaToken *anjuta_token_new_string (AnjutaTokenType type, const gchar *value)
{
	AnjutaToken *token;

	if (value == NULL)
	{
		token = anjuta_token_new_static (type, NULL);
	}
	else
	{
		token = g_slice_new0 (AnjutaToken);
		token->data.type = type  & ANJUTA_TOKEN_TYPE;
		token->data.flags = type & ANJUTA_TOKEN_FLAGS;
		token->data.pos = g_strdup (value);
		token->data.length = strlen (value);
	}

	return token;
}

AnjutaToken*
anjuta_token_new_string_len (AnjutaTokenType type, gchar *value, gsize length)
{
	AnjutaToken *token;

	if (value == NULL)
	{
		token = anjuta_token_new_static (type, NULL);
	}
	else
	{
		token = g_slice_new0 (AnjutaToken);
		token->data.type = type  & ANJUTA_TOKEN_TYPE;
		token->data.flags = type & ANJUTA_TOKEN_FLAGS;
		token->data.pos = value;
		token->data.length = length;
	}

	return token;
}

AnjutaToken *
anjuta_token_new_static_len (gint type, const gchar *pos, gsize length)
{
	AnjutaToken *token;

	token = g_slice_new0 (AnjutaToken);
	token->data.type = type  & ANJUTA_TOKEN_TYPE;
	token->data.flags = (type & ANJUTA_TOKEN_FLAGS) | ANJUTA_TOKEN_STATIC;
	token->data.pos = (gchar *)pos;
	token->data.length = length;

	return token;
};

AnjutaToken *anjuta_token_new_static (AnjutaTokenType type, const char *value)
{
	return anjuta_token_new_static_len (type, value, value == NULL ? 0 : strlen (value));
}

AnjutaToken*
anjuta_token_free_children (AnjutaToken *token)
{
	AnjutaToken *child;
	AnjutaToken *last;

	if (token == NULL) return NULL;

	for (child = token->children; child != NULL; child = token->children)
	{
		anjuta_token_free (child);
	}
	token->children = NULL;

	if (token->last != NULL)
	{
		last = token->last;
		for (child = anjuta_token_next (token); child != NULL; child = anjuta_token_next (token))
		{
			anjuta_token_free (child);
			if (child == last) break;
		}
	}
	token->last = NULL;

	return token;
}

static void
free_token (AnjutaToken *token, gpointer user_data)
{
	anjuta_token_unlink_token (token);
	if ((token->data.pos != NULL) && !(token->data.flags & ANJUTA_TOKEN_STATIC))
	{
		g_free (token->data.pos);
	}
	g_slice_free (AnjutaToken, token);
}


AnjutaToken*
anjuta_token_free (AnjutaToken *token)
{
	AnjutaToken *next;

	if (token == NULL) return NULL;

	next = anjuta_token_foreach_post_order (token, free_token, NULL);

	return next;
}
