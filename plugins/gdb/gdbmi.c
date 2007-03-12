/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gdbmi.c
 * Copyright (C) Naba Kumar 2005 <naba@gnome.org>
 * 
 * gdbmi.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2, or (at your option) any later version.
 * 
 * gdbmi.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with main.c.  See the file "COPYING".  If not,
 * write to:  The Free Software Foundation, Inc.,
 *            59 Temple Place - Suite 330,
 *            Boston,  MA  02111-1307, USA.
 */

/* GDB MI parser */
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "gdbmi.h"

#define GDBMI_DUMP_INDENT_SIZE 4

struct _GDBMIValue
{
	GDBMIDataType type;
	gchar *name;
	union {
		GHashTable *hash;
		GQueue *list;
		GString *literal;
	} data;
};

struct _GDBMIForeachHashData
{
	GFunc user_callback;
	gpointer user_data;
};

static guint GDBMI_deleted_hash_value = 0;

void
gdbmi_value_free (GDBMIValue *val)
{
	g_return_if_fail (val != NULL);
	
	if (val->type == GDBMI_DATA_LITERAL)
	{
		g_string_free (val->data.literal, TRUE);
	}
	else if (val->type == GDBMI_DATA_LIST)
	{
		gdbmi_value_foreach (val, (GFunc)gdbmi_value_free, NULL);
		g_queue_free (val->data.list);
	}
	else
	{
		g_hash_table_destroy (val->data.hash);
	}
	g_free (val->name);
	g_free (val);
}

GDBMIValue *
gdbmi_value_new (GDBMIDataType data_type, const gchar *name)
{
	GDBMIValue *val = g_new0 (GDBMIValue, 1);
	
	val->type = data_type;
	if (name)
		val->name = g_strdup (name);
	
	switch (data_type)
	{
		case GDBMI_DATA_HASH:
			val->data.hash =
				g_hash_table_new_full (g_str_hash, g_str_equal,
									   (GDestroyNotify)g_free,
									   (GDestroyNotify)gdbmi_value_free);
			break;
		case GDBMI_DATA_LIST:
			val->data.list = g_queue_new ();
			break;
		case GDBMI_DATA_LITERAL:
			val->data.literal = g_string_new (NULL);
			break;
		default:
			g_warning ("Unknow MI data type. Should not reach here");
			return NULL;
	}
	return val;
}

GDBMIValue*
gdbmi_value_literal_new (const gchar *name, const gchar *data)
{
	GDBMIValue* val;
	val = gdbmi_value_new (GDBMI_DATA_LITERAL, name);
	gdbmi_value_literal_set (val, data);
	return val;
}

const gchar*
gdbmi_value_get_name (const GDBMIValue *val)
{
	g_return_val_if_fail (val != NULL, NULL);
	return val->name;
}

GDBMIDataType
gdbmi_value_get_type (const GDBMIValue *val)
{
	return val->type;
}

void
gdbmi_value_set_name (GDBMIValue *val, const gchar *name)
{
	g_return_if_fail (val != NULL);
	g_return_if_fail (name != NULL);
	g_free (val->name);
	val->name = g_strdup (name);
}

gint
gdbmi_value_get_size (const GDBMIValue* val)
{
	g_return_val_if_fail (val != NULL, 0);
	
	if (val->type == GDBMI_DATA_LITERAL)
	{
		if (val->data.literal->str)
			return 1;
		else
			return 0;
	}
	else if (val->type == GDBMI_DATA_LIST)
		return g_queue_get_length (val->data.list);
	else if (val->type == GDBMI_DATA_HASH)
		return g_hash_table_size (val->data.hash);
	else
		return 0;
}

static void
gdbmi_value_hash_foreach (const gchar *key, GDBMIValue* val,
						  struct _GDBMIForeachHashData *hash_data)
{
	hash_data->user_callback (val, hash_data->user_data);
}

void
gdbmi_value_foreach (const GDBMIValue* val, GFunc func, gpointer user_data)
{
	g_return_if_fail (val != NULL);
	g_return_if_fail (func != NULL);
	
	if (val->type == GDBMI_DATA_LIST)
	{
		g_queue_foreach (val->data.list, func, user_data);
	}
	else if (val->type == GDBMI_DATA_HASH)
	{
		struct _GDBMIForeachHashData hash_data = {NULL, NULL};
		
		hash_data.user_callback = func;
		hash_data.user_data = user_data;
		g_hash_table_foreach (val->data.hash, (GHFunc)gdbmi_value_hash_foreach,
							  &hash_data);
	}
	else
	{
		g_warning ("Can not do foreach for GDBMIValue this type");
	}
}

/* Literal operations */
void
gdbmi_value_literal_set (GDBMIValue* val, const gchar *data)
{
	g_return_if_fail (val != NULL);
	g_return_if_fail (val->type == GDBMI_DATA_LITERAL);
	g_string_assign (val->data.literal, data);
}

const gchar*
gdbmi_value_literal_get (const GDBMIValue* val)
{
	g_return_val_if_fail (val != NULL, NULL);
	g_return_val_if_fail (val->type == GDBMI_DATA_LITERAL, NULL);
	return val->data.literal->str;
}

/* Hash operations */
void
gdbmi_value_hash_insert (GDBMIValue* val, const gchar *key, GDBMIValue *value)
{
	gpointer orig_key;
	gpointer orig_value;

	g_return_if_fail (val != NULL);
	g_return_if_fail (key != NULL);
	g_return_if_fail (value != NULL);
	g_return_if_fail (val->type == GDBMI_DATA_HASH);

	/* GDBMI hash table could contains several data with the same
	 * key (output of -thread-list-ids)
	 * Keep old value under a random name, we get them using
	 * foreach function */
	if (g_hash_table_lookup_extended (val->data.hash, key, &orig_key, &orig_value))
	{
		/* Key already exist, remove it and insert value with
		 * another name */
		g_hash_table_steal (val->data.hash, key);
		g_free (orig_key);
		gchar *new_key = g_strdup_printf("[%d]", GDBMI_deleted_hash_value++);
		g_hash_table_insert (val->data.hash, new_key, orig_value);

	}
	g_hash_table_insert (val->data.hash, g_strdup (key), value);
}

const GDBMIValue*
gdbmi_value_hash_lookup (const GDBMIValue* val, const gchar *key)
{
	g_return_val_if_fail (val != NULL, NULL);
	g_return_val_if_fail (key != NULL, NULL);
	g_return_val_if_fail (val->type == GDBMI_DATA_HASH, NULL);
	
	return g_hash_table_lookup (val->data.hash, key);
}

/* List operations */
void
gdbmi_value_list_append (GDBMIValue* val, GDBMIValue *value)
{
	g_return_if_fail (val != NULL);
	g_return_if_fail (value != NULL);
	g_return_if_fail (val->type == GDBMI_DATA_LIST);
	
	g_queue_push_tail (val->data.list, value);
}

const GDBMIValue*
gdbmi_value_list_get_nth (const GDBMIValue* val, gint idx)
{
	g_return_val_if_fail (val != NULL, NULL);
	g_return_val_if_fail (val->type == GDBMI_DATA_LIST, NULL);
	
	if (idx >= 0)
		return g_queue_peek_nth (val->data.list, idx);
	else
		return g_queue_peek_tail (val->data.list);
}

static void
gdbmi_value_dump_foreach (const GDBMIValue *val, gpointer indent_level)
{
	gdbmi_value_dump (val, GPOINTER_TO_INT (indent_level));
}

void
gdbmi_value_dump (const GDBMIValue *val, gint indent_level)
{
	gint i, next_indent;
	
	g_return_if_fail (val != NULL);
	
	for (i = 0; i < indent_level; i++)
		printf (" ");
	
	next_indent = indent_level + GDBMI_DUMP_INDENT_SIZE;
	if (val->type == GDBMI_DATA_LITERAL)
	{
		gchar *v;
		
		v = g_strescape (val->data.literal->str, NULL);
		if (val->name)
			printf ("%s = \"%s\",\n", val->name, v);
		else
			printf ("\"%s\",\n", v);
		g_free (v);
	}
	else if (val->type == GDBMI_DATA_LIST)
	{
		if (val->name)
			printf ("%s = [\n", val->name);
		else
			printf ("[\n");
		gdbmi_value_foreach (val, (GFunc)gdbmi_value_dump_foreach,
							 GINT_TO_POINTER (next_indent));
		for (i = 0; i < indent_level; i++)
			printf (" ");
		printf ("],\n");
	}
	else if (val->type == GDBMI_DATA_HASH)
	{
		if (val->name)
			printf ("%s = {\n", val->name);
		else
			printf ("{\n");
		gdbmi_value_foreach (val, (GFunc)gdbmi_value_dump_foreach,
							 GINT_TO_POINTER (next_indent));
		for (i = 0; i < indent_level; i++)
			printf (" ");
		printf ("},\n");
	}
}

static GDBMIValue*
gdbmi_value_parse_real (gchar **ptr)
{
	GDBMIValue *val = NULL;
	
	if (**ptr == '\0')
	{
		/* End of stream */
		g_warning ("Parse error: Reached end of stream");
	}
	else if (**ptr == '"')
	{
		/* Value is literal */
		gboolean escaped;
		GString *buff;
		gchar *p;
		gchar *value, *compressed_value;
		gint i;
		
		*ptr = g_utf8_next_char (*ptr);
		escaped = FALSE;
		buff = g_string_new ("");
		while (escaped || **ptr != '"')
		{
			if (**ptr == '\0')
			{
				g_warning ("Parse error: Invalid literal value");
				return NULL;
			}
			if (escaped)
			{
				escaped = FALSE;
			}
			else if (**ptr == '\\')
			{
				escaped = TRUE;
			}
			p = g_utf8_next_char (*ptr);
			for (i = 0; i < (p - *ptr); i++)
				g_string_append_c (buff, *(*ptr + i));
			*ptr = p;
		}
		/* Get pass the closing quote */
		*ptr = g_utf8_next_char (*ptr);
		
		value = g_string_free (buff, FALSE);
		compressed_value = g_strcompress (value);
		val = gdbmi_value_literal_new (NULL, compressed_value);
		g_free (value);
		g_free (compressed_value);
	}
	else if (isalpha (**ptr))
	{
		/* Value is assignment */
		gchar *name;
		gchar *p;
		
		/* Get assignment name */
		p = *ptr;
		while (**ptr != '=')
		{
			if (**ptr == '\0')
			{
				g_warning ("Parse error: Invalid assignment name");
				return NULL;
			}
			*ptr = g_utf8_next_char (*ptr);
		}
		name = g_strndup (p, *ptr - p);
		
		/* Skip pass assignment operator */
		*ptr = g_utf8_next_char (*ptr);
		
		/* Retrieve assignment value */
		val = gdbmi_value_parse_real (ptr);
		if (val)
		{
			gdbmi_value_set_name (val, name);
		}
		else
		{
			g_warning ("Parse error: From parent");
		}
		g_free (name);
	}
	else if (**ptr == '{')
	{
		/* Value is hash */
		gboolean error = FALSE;
		
		*ptr = g_utf8_next_char (*ptr);
		val = gdbmi_value_new (GDBMI_DATA_HASH, NULL);
		while (**ptr != '}')
		{
			GDBMIValue *element;
			element = gdbmi_value_parse_real (ptr);
			if (element == NULL)
			{
				g_warning ("Parse error: From parent");
				error = TRUE;
				break;
			}
			if (gdbmi_value_get_name == NULL)
			{
				g_warning ("Parse error: Hash element has no name => '%s'",
						   *ptr);
				error = TRUE;
				gdbmi_value_free (element);
				break;
			}
			if (**ptr != ',' && **ptr != '}')
			{
				g_warning ("Parse error: Invalid element separator => '%s'",
						   *ptr);
				error = TRUE;
				gdbmi_value_free (element);
				break;
			}
			gdbmi_value_hash_insert (val, gdbmi_value_get_name (element),
									 element);
			
			/* Get pass the comma separator */
			if (**ptr == ',')
				*ptr = g_utf8_next_char (*ptr);
		}
		if (error)
		{
			gdbmi_value_free (val);
			val = NULL;
		}
		/* Get pass the closing hash */
		*ptr = g_utf8_next_char (*ptr);
	}
	else if (**ptr == '[')
	{
		/* Value is list */
		gboolean error = FALSE;
		
		*ptr = g_utf8_next_char (*ptr);
		val = gdbmi_value_new (GDBMI_DATA_LIST, NULL);
		while (**ptr != ']')
		{
			GDBMIValue *element;
			element = gdbmi_value_parse_real (ptr);
			if (element == NULL)
			{
				g_warning ("Parse error: From parent");
				error = TRUE;
				break;
			}
			if (**ptr != ',' && **ptr != ']')
			{
				g_warning ("Parse error: Invalid element separator => '%s'",
						   *ptr);
				error = TRUE;
				gdbmi_value_free (element);
				break;
			}
			gdbmi_value_list_append (val, element);
			
			/* Get pass the comma separator */
			if (**ptr == ',')
				*ptr = g_utf8_next_char (*ptr);
		}
		if (error)
		{
			gdbmi_value_free (val);
			val = NULL;
		}
		/* Get pass the closing list */
		*ptr = g_utf8_next_char (*ptr);
	}
	else
	{
		/* Should not be here -- Error */
		g_warning ("Parse error: Should not be here => '%s'", *ptr);
	}
	return val;
}

GDBMIValue*
gdbmi_value_parse (const gchar *message)
{
	GDBMIValue *val;
	gchar *msg, *ptr;
	
	g_return_val_if_fail (message != NULL, NULL);
	
	if (strcasecmp(message, "^error") == 0)
	{
		g_warning ("GDB reported error without any error message");
		return NULL; /* No message */
	}
	
	val = NULL;
	if (strchr (message, ','))
	{
		msg = g_strconcat ("{", strchr (message, ',') + 1, "}", NULL);
		ptr = msg;
		val = gdbmi_value_parse_real (&ptr);
		g_free (msg);
	}
	return val;
}
