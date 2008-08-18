/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * an_symbol_iter.c
 * Copyright (C) Naba Kumar  <naba@gnome.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <libgnome/gnome-macros.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-symbol.h>
#include <libanjuta/interfaces/ianjuta-iterable.h>
#include "an_symbol_iter.h"
#include "an_symbol.h"

struct _AnjutaSymbolIterPriv
{
	gint current_pos;
	const GPtrArray *tm_tags_array;
};

static gpointer parent_class;

static gboolean isymbol_iter_set_position (IAnjutaIterable *iterable,
										   gint position, GError **err);

/* Anjuta symbol iter class */

static void
anjuta_symbol_iter_finalize (GObject * obj)
{
	AnjutaSymbolIter *si = ANJUTA_SYMBOL_ITER (obj);
	
	g_free (si->priv);
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
anjuta_symbol_iter_dispose (GObject * obj)
{
	/* AnjutaSymbolIter *si = ANJUTA_SYMBOL_ITER (obj); */
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
anjuta_symbol_iter_instance_init (GObject * obj)
{
	AnjutaSymbolIter *si;
	
	si = ANJUTA_SYMBOL_ITER (obj);
	si->priv = g_new0 (AnjutaSymbolIterPriv, 1);
	si->priv->current_pos = 0;
}

static void
anjuta_symbol_iter_class_init (AnjutaSymbolIterClass * klass)
{
	AnjutaSymbolIterClass *sic;
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);
	sic = ANJUTA_SYMBOL_ITER_CLASS (klass);
	object_class->finalize = anjuta_symbol_iter_finalize;
	object_class->dispose = anjuta_symbol_iter_dispose;
}

AnjutaSymbolIter*
anjuta_symbol_iter_new (const GPtrArray *tm_tags_array)
{
	AnjutaSymbolIter *si;
	
	g_return_val_if_fail (tm_tags_array != NULL, NULL);
	
	si = g_object_new (ANJUTA_TYPE_SYMBOL_ITER, NULL);
	si->priv->tm_tags_array = tm_tags_array;
	ianjuta_iterable_first (IANJUTA_ITERABLE (si), NULL);
	return si;
}

/* IAnjutaIterable implementation */

static gboolean
isymbol_iter_first (IAnjutaIterable *iterable, GError **err)
{
	AnjutaSymbolIter *si = ANJUTA_SYMBOL_ITER (iterable);
	g_return_val_if_fail (iterable != NULL, FALSE);
	
	if (si->priv->tm_tags_array->len <= 0)
		return FALSE;
	
	si->priv->current_pos = 0;
	if (si->priv->tm_tags_array->len <= 0)
	{
		anjuta_symbol_set_tag (ANJUTA_SYMBOL (iterable),
							   NULL);
		return FALSE;
	}
	/* g_assert (si->priv->tm_tags_array->pdata[si->priv->current_pos]); */
	anjuta_symbol_set_tag (ANJUTA_SYMBOL (iterable),
						   si->priv->tm_tags_array->pdata[0]);
	return TRUE;
}

static gboolean
isymbol_iter_next (IAnjutaIterable *iterable, GError **err)
{
	AnjutaSymbolIter *si = ANJUTA_SYMBOL_ITER (iterable);
	g_return_val_if_fail (iterable != NULL, FALSE);
	
	if (si->priv->tm_tags_array->len <= 0)
		return FALSE;
	
	DEBUG_PRINT ("si->priv->tm_tags_array->len %d", si->priv->tm_tags_array->len);
	DEBUG_PRINT ("si->priv->current_pos %d", si->priv->current_pos);
	
	if (si->priv->current_pos >= (si->priv->tm_tags_array->len - 1))
	{
		anjuta_symbol_set_tag (ANJUTA_SYMBOL (iterable),
							si->priv->tm_tags_array->pdata[si->priv->tm_tags_array->len - 1]);
		return FALSE;
	}
	si->priv->current_pos++;
	/*g_assert (si->priv->tm_tags_array->pdata[si->priv->current_pos]); */
	anjuta_symbol_set_tag (ANJUTA_SYMBOL (iterable),
						si->priv->tm_tags_array->pdata[si->priv->current_pos]);
	return TRUE;
}

static gboolean
isymbol_iter_previous (IAnjutaIterable *iterable, GError **err)
{
	AnjutaSymbolIter *si = ANJUTA_SYMBOL_ITER (iterable);
	g_return_val_if_fail (iterable != NULL, FALSE);
	
	if (si->priv->tm_tags_array->len <= 0)
		return FALSE;
	
	if (si->priv->current_pos <= 0)
	{
		anjuta_symbol_set_tag (ANJUTA_SYMBOL (iterable),
							si->priv->tm_tags_array->pdata[0]);
		return FALSE;
	}
	si->priv->current_pos--;
	/* g_assert (si->priv->tm_tags_array->pdata[si->priv->current_pos]); */
	anjuta_symbol_set_tag (ANJUTA_SYMBOL (iterable),
						   si->priv->tm_tags_array->pdata[si->priv->current_pos]);
	return TRUE;
}

static gboolean
isymbol_iter_last (IAnjutaIterable *iterable, GError **err)
{
	AnjutaSymbolIter *si = ANJUTA_SYMBOL_ITER (iterable);
	
	si->priv->current_pos = 0;
	if (si->priv->tm_tags_array->len <= 0)
	{
		anjuta_symbol_set_tag (ANJUTA_SYMBOL (iterable),
							si->priv->tm_tags_array->pdata[0]);
		return FALSE;
	}
	si->priv->current_pos = si->priv->tm_tags_array->len - 1;
	/* g_assert (si->priv->tm_tags_array->pdata[si->priv->current_pos]); */
	anjuta_symbol_set_tag (ANJUTA_SYMBOL (iterable),
						   si->priv->tm_tags_array->pdata[si->priv->current_pos]);
	return TRUE;
}

static void
isymbol_iter_foreach (IAnjutaIterable *iterable, GFunc callback,
					  gpointer user_data, GError **err)
{
	AnjutaSymbolIter *si = ANJUTA_SYMBOL_ITER (iterable);
	g_return_if_fail (iterable != NULL);
	
	if (si->priv->tm_tags_array->len <= 0)
		return;
	
	gint saved_pos = si->priv->current_pos;
	
	isymbol_iter_first (iterable, NULL);
	while (!isymbol_iter_next (iterable, NULL))
		callback (iterable, user_data);
	isymbol_iter_set_position (iterable, saved_pos, NULL);
}

static gboolean
isymbol_iter_set_position (IAnjutaIterable *iterable,
						   gint position, GError **err)
{
	AnjutaSymbolIter *si = ANJUTA_SYMBOL_ITER (iterable);
	g_return_val_if_fail (iterable != NULL, FALSE);
	
	if (si->priv->tm_tags_array->len <= 0)
		return FALSE;
	
	if (position < 0)
		return FALSE;
	if (position > (si->priv->tm_tags_array->len - 1))
		return FALSE;
	si->priv->current_pos = position;
	/* g_assert (si->priv->tm_tags_array->pdata[si->priv->current_pos]); */
	anjuta_symbol_set_tag (ANJUTA_SYMBOL (iterable),
						   si->priv->tm_tags_array->pdata[si->priv->current_pos]);
	return TRUE;
}

static gint
isymbol_iter_get_position (IAnjutaIterable *iterable, GError **err)
{
	AnjutaSymbolIter *si = ANJUTA_SYMBOL_ITER (iterable);
	g_return_val_if_fail (iterable != NULL, FALSE);
	
	if (si->priv->tm_tags_array->len <= 0)
		return -1;
	
	return si->priv->current_pos;
}

static gint
isymbol_iter_get_length (IAnjutaIterable *iterable, GError **err)
{
	AnjutaSymbolIter *si = ANJUTA_SYMBOL_ITER (iterable);
	g_return_val_if_fail (iterable != NULL, FALSE);
	
	return si->priv->tm_tags_array->len;
}

static void
isymbol_iter_iface_init (IAnjutaIterableIface *iface, GError **err)
{
	iface->first = isymbol_iter_first;
	iface->next = isymbol_iter_next;
	iface->previous = isymbol_iter_previous;
	iface->last = isymbol_iter_last;
	iface->foreach = isymbol_iter_foreach;
	iface->set_position = isymbol_iter_set_position;
	iface->get_position = isymbol_iter_get_position;
	iface->get_length = isymbol_iter_get_length;
}

ANJUTA_TYPE_BEGIN (AnjutaSymbolIter, anjuta_symbol_iter, ANJUTA_TYPE_SYMBOL);
ANJUTA_TYPE_ADD_INTERFACE (isymbol_iter, IANJUTA_TYPE_ITERABLE);
ANJUTA_TYPE_END;
