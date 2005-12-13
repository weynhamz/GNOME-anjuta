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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
	AnjutaSymbol *symbol;
	const GPtrArray *tm_tags_array;
};

static gpointer parent_class;

/* Anjuta symbol iter class */

static void
anjuta_symbol_iter_finalize (GObject * obj)
{
	AnjutaSymbolIter *si = ANJUTA_SYMBOL_ITER (obj);
	
	g_free (si->priv);
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
anjuta_symbol_iter_dispose (GObject * obj)
{
	AnjutaSymbolIter *si = ANJUTA_SYMBOL_ITER (obj);
	
	if (si->priv->symbol)
	{
		g_object_unref (si->priv->symbol);
		si->priv->symbol = NULL;
	}
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
anjuta_symbol_iter_instance_init (GObject * obj)
{
	AnjutaSymbolIter *si;
	
	si = ANJUTA_SYMBOL_ITER (obj);
	si->priv = g_new0 (AnjutaSymbolIterPriv, 1);
	si->priv->current_pos = 0;
	si->priv->symbol = NULL;
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
	return si;
}

/* IAnjutaIterable implementation */

static gboolean
isymbol_iter_first (IAnjutaIterable *iterable, GError **err)
{
	AnjutaSymbolIter *si = ANJUTA_SYMBOL_ITER (iterable);
	
	si->priv->current_pos = 0;
	if (si->priv->tm_tags_array->len <= 0)
		return FALSE;
	return TRUE;
}

static gboolean
isymbol_iter_next (IAnjutaIterable *iterable, GError **err)
{
	AnjutaSymbolIter *si = ANJUTA_SYMBOL_ITER (iterable);
	
	if (si->priv->current_pos >= (si->priv->tm_tags_array->len - 1))
		return FALSE;
	si->priv->current_pos++;
	return TRUE;
}

static gboolean
isymbol_iter_previous (IAnjutaIterable *iterable, GError **err)
{
	AnjutaSymbolIter *si = ANJUTA_SYMBOL_ITER (iterable);
	
	if (si->priv->current_pos <= 0)
		return FALSE;
	si->priv->current_pos--;
	return TRUE;
}

static gboolean
isymbol_iter_last (IAnjutaIterable *iterable, GError **err)
{
	AnjutaSymbolIter *si = ANJUTA_SYMBOL_ITER (iterable);
	
	si->priv->current_pos = 0;
	if (si->priv->tm_tags_array->len <= 0)
		return FALSE;
	si->priv->current_pos = si->priv->tm_tags_array->len - 1;
	return TRUE;
}

static void
isymbol_iter_foreach (IAnjutaIterable *iterable, GFunc callback,
					  gpointer user_data, GError **err)
{
	AnjutaSymbolIter *si = ANJUTA_SYMBOL_ITER (iterable);
	g_ptr_array_foreach ((GPtrArray*)si->priv->tm_tags_array,
	callback, user_data);
}

static gpointer
isymbol_iter_get (IAnjutaIterable *iterable, GType data_type, GError **err)
{
	AnjutaSymbolIter *si = ANJUTA_SYMBOL_ITER (iterable);
	g_return_val_if_fail (data_type != ANJUTA_TYPE_SYMBOL, NULL);
	g_return_val_if_fail (si->priv->tm_tags_array->len > 0, NULL);
	
	if (si->priv->symbol == NULL)
		si->priv->symbol = anjuta_symbol_new (si->priv->tm_tags_array->pdata[si->priv->current_pos]);
	else
		anjuta_symbol_set_tag (si->priv->symbol,
							   si->priv->tm_tags_array->pdata[si->priv->current_pos]);
	return si->priv->symbol;
}

static gpointer
isymbol_iter_get_nth (IAnjutaIterable *iterable, GType data_type,
					  gint position, GError **err)
{
	AnjutaSymbolIter *si = ANJUTA_SYMBOL_ITER (iterable);
	g_return_val_if_fail (data_type != ANJUTA_TYPE_SYMBOL, NULL);
	
	if (si->priv->symbol == NULL)
		si->priv->symbol = anjuta_symbol_new (si->priv->tm_tags_array->pdata[position]);
	else
		anjuta_symbol_set_tag (si->priv->symbol,
							   si->priv->tm_tags_array->pdata[position]);
	return si->priv->symbol;
}

static void
isymbol_iter_set (IAnjutaIterable *iterable, GType data_type,
				  gpointer data, GError **err)
{
	DEBUG_PRINT ("set() is not valid for AnjutaSymbolIter implementation");
}

static void
isymbol_iter_set_nth (IAnjutaIterable *iterable, GType data_type,
					  gpointer data, gint position, GError **err)
{
	g_warning ("set() is not valid for AnjutaSymbolIter implementation");
}

static gint
isymbol_iter_get_position (IAnjutaIterable *iterable, GError **err)
{
	AnjutaSymbolIter *si = ANJUTA_SYMBOL_ITER (iterable);
	return si->priv->current_pos;
}

static gint
isymbol_iter_get_length (IAnjutaIterable *iterable, GError **err)
{
	AnjutaSymbolIter *si = ANJUTA_SYMBOL_ITER (iterable);
	return si->priv->tm_tags_array->len;
}

static gboolean
isymbol_iter_get_settable (IAnjutaIterable *iterable, GError **err)
{
	return FALSE;
}

static void
isymbol_iter_iface_init (IAnjutaIterableIface *iface, GError **err)
{
	iface->first = isymbol_iter_first;
	iface->next = isymbol_iter_next;
	iface->previous = isymbol_iter_previous;
	iface->last = isymbol_iter_last;
	iface->foreach = isymbol_iter_foreach;
	iface->get = isymbol_iter_get;
	iface->get_nth = isymbol_iter_get_nth;
	iface->get_position = isymbol_iter_get_position;
	iface->get_length = isymbol_iter_get_length;
	iface->get_settable = isymbol_iter_get_settable;
	iface->set = isymbol_iter_set;
	iface->set_nth = isymbol_iter_set_nth;
}

ANJUTA_TYPE_BEGIN (AnjutaSymbolIter, anjuta_symbol_iter, G_TYPE_OBJECT);
ANJUTA_TYPE_ADD_INTERFACE (isymbol_iter, IANJUTA_TYPE_ITERABLE);
ANJUTA_TYPE_END;
