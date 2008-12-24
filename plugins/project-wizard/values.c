/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    values.c
    Copyright (C) 2004 Sebastien Granjoux

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

/*
 * Store all project property values (= just the name and the value
 * without other high level information like the associated widget)
 *
 * The property values are stored independently because we want to keep
 * all values entered by the user even if it goes back in the wizard and
 * regenerate all properties.
 * Moreover it removes dependencies between the installer part and the
 * properties. The installer part could work with the values only.
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "values.h"

#include <string.h>

/*---------------------------------------------------------------------------*/

/* One property value, so just a name and a value plus a tag !
 * The tag is defined in the header file, but is not really used in this code */

struct _NPWValue
{
	NPWValueTag tag;
	const gchar* name;
	gchar* value;
};

/* Creation and Destruction
 *---------------------------------------------------------------------------*/

static NPWValue*
npw_value_new (const gchar *name)
{
	NPWValue *value;
	
	value = g_slice_new (NPWValue);
	value->tag = NPW_EMPTY_VALUE;
	value->name = name;
	value->value = NULL;
	
	return value;
}

static void
npw_value_free (NPWValue *value)
{
	g_free (value->value);
	g_slice_free (NPWValue, value);
}
	
GHashTable*
npw_value_heap_new (void)
{
	GHashTable* hash;

	hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify)npw_value_free);
	
	return hash;
}

void
npw_value_heap_free (GHashTable* hash)
{
	/* GSList* node; */
	g_return_if_fail (hash != NULL);

	g_hash_table_destroy (hash);
}

/* Find a value or list all values
 *---------------------------------------------------------------------------*/

/* Return key corresponding to name, create key if it doesn't exist */

NPWValue*
npw_value_heap_find_value (GHashTable* hash, const gchar* name)
{
	NPWValue* node;

	if (!g_hash_table_lookup_extended (hash, name, NULL, (gpointer)&node))
	{
		gchar* new_name;

		new_name = g_strdup (name);
		node = npw_value_new (new_name);
		g_hash_table_insert (hash, new_name, node);
	}

	return node;
}

void
npw_value_heap_foreach_value (GHashTable* hash, GHFunc func, gpointer user_data)
{
	g_hash_table_foreach (hash, func, user_data);
}

/* Access value attributes
 *---------------------------------------------------------------------------*/

const gchar*
npw_value_get_name ( const NPWValue* node)
{
	g_return_val_if_fail (node != NULL, NULL);

	return node->name;
}

/* set new value, return FALSE if value has not changed */

gboolean
npw_value_set_value (NPWValue* node, const gchar* value, NPWValueTag tag)
{
	gboolean change = FALSE;

	g_return_val_if_fail (node != NULL, FALSE);

	if (tag == NPW_EMPTY_VALUE)
	{
		if (node->tag != NPW_EMPTY_VALUE)
		{
			node->tag = NPW_EMPTY_VALUE;
			change = TRUE;
		}
	}
	else
	{
		/* Set value */
		if (value == NULL)
		{
			if (node->value != NULL)
			{
				g_free (node->value);
				node->value = NULL;
				change = TRUE;
			}
		}
		else
		{
			if ((node->value == NULL) || (strcmp (node->value, value) != 0))
			{
				g_free (node->value);
				node->value = g_strdup (value);
				change = TRUE;
			}
		}
		/* Set tag */
		if (change == TRUE)
		{
			/* remove valid tag if value change */
			node->tag &= ~NPW_VALID_VALUE;
		}
		else if ((tag & NPW_VALID_VALUE) != (node->tag & NPW_VALID_VALUE))
		{
			/* Changing valid tag count as a change */
			change = TRUE;
		}
		node->tag &= NPW_VALID_VALUE;	
		node->tag |= tag;
	}

	return change;
}

const gchar*
npw_value_get_value (const NPWValue* node)
{
	g_return_val_if_fail (node != NULL, NULL);
	
	return node->tag == NPW_EMPTY_VALUE ? NULL : node->value;
}

NPWValueTag
npw_value_get_tag (const NPWValue* node)
{
	g_return_val_if_fail (node != NULL, NPW_EMPTY_VALUE);

	return node->tag;
}


