/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Jeffrey Stedfast <fejj@ximian.com>
 *
 *  Copyright 2003 Ximian, Inc. (www.ximian.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "vgstrpool.h"


#ifdef ENABLE_STRPOOL

struct _VgStrpoolNode {
	unsigned int ref_count;
	char *str;
};

static GHashTable *strpool = NULL;


void
vg_strpool_init (void)
{
	if (strpool != NULL)
		return;
	
	strpool = g_hash_table_new (g_str_hash, g_str_equal);
}


static void
strpool_foreach (gpointer key, gpointer val, gpointer user_data)
{
	struct _VgStrpoolNode *node = val;
	
	g_free (node->str);
	g_free (node);
}


void
vg_strpool_shutdown (void)
{
	if (strpool == NULL)
		return;
	
	g_hash_table_foreach (strpool, strpool_foreach, NULL);
	g_hash_table_destroy (strpool);
	strpool = NULL;
}


char *
vg_strpool_add (char *string, int own)
{
	struct _VgStrpoolNode *node;
	
	g_return_val_if_fail (strpool != NULL, string);
	g_return_val_if_fail (string != NULL, NULL);
	
	if (!(node = g_hash_table_lookup (strpool, string))) {
		node = g_new (struct _VgStrpoolNode, 1);
		node->ref_count = 0;
		node->str = own ? string : g_strdup (string);
		
		g_hash_table_insert (strpool, node->str, node);
	} else if (own) {
		g_free (string);
	}
	
	node->ref_count++;
	
	return node->str;
}


char *
vg_strdup (const char *string)
{
	if (string == NULL)
		return NULL;
	
	return vg_strpool_add ((char *) string, FALSE);
}


char *
vg_strndup (const char *string, size_t n)
{
	char *str;
	
	if (string == NULL)
		return NULL;
	
	str = g_strndup (string, n);
	
	return vg_strpool_add (str, TRUE);
}


void
vg_strfree (char *string)
{
	struct _VgStrpoolNode *node;
	
	g_return_if_fail (strpool != NULL);
	
	if (string == NULL)
		return;
	
	if (!(node = g_hash_table_lookup (strpool, string))) {
		g_warning ("tring to free a string (%p) not created with vg_str[n]dup", string);
		g_free (string);
		return;
	}
	
	if (node->ref_count == 1) {
		g_hash_table_remove (strpool, string);
		g_free (node->str);
		g_free (node);
	} else {
		node->ref_count--;
	}
}

#endif /* ENABLE_STRPOOL */
