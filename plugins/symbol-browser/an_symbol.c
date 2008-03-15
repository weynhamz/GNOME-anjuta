/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * iterable.c
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
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-symbol.h>
#include "an_symbol_info.h"
#include "an_symbol.h"

struct _AnjutaSymbolPriv
{
	const TMTag *tm_tag;
	gchar* uri;
};

static gpointer parent_class;

/* Anjuta symbol view class */

static void
anjuta_symbol_finalize (GObject * obj)
{
	AnjutaSymbol *s = ANJUTA_SYMBOL (obj);
	
	if (s->priv->uri)
		g_free (s->priv->uri);
	g_free (s->priv);
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
anjuta_symbol_instance_init (GObject * obj)
{
	AnjutaSymbol *s;
	
	s = ANJUTA_SYMBOL (obj);
	s->priv = g_new0 (AnjutaSymbolPriv, 1);
}

static void
anjuta_symbol_class_init (AnjutaSymbolClass * klass)
{
	AnjutaSymbolClass *sc;
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);
	sc = ANJUTA_SYMBOL_CLASS (klass);
	object_class->finalize = anjuta_symbol_finalize;
}

AnjutaSymbol*
anjuta_symbol_new (const TMTag *tm_tag)
{
	AnjutaSymbol *s;
	
	g_return_val_if_fail (tm_tag != NULL, NULL);
	g_return_val_if_fail (tm_tag->type < tm_tag_max_t, NULL);
	g_return_val_if_fail (!(tm_tag->type & (tm_tag_file_t|tm_tag_undef_t)),
						  NULL);
	
	s = g_object_new (ANJUTA_TYPE_SYMBOL, NULL);
	s->priv->tm_tag = tm_tag;
	s->priv->uri = NULL;
	return s;
}

void
anjuta_symbol_set_tag (AnjutaSymbol *symbol, const TMTag *tm_tag)
{
	g_return_if_fail (ANJUTA_IS_SYMBOL (symbol));
	symbol->priv->tm_tag = NULL;
	if (symbol->priv->uri)
	{
		g_free(symbol->priv->uri);
		symbol->priv->uri = NULL;
	}
	if (tm_tag != NULL)
	{
		g_return_if_fail (tm_tag->type < tm_tag_max_t);
		g_return_if_fail (!(tm_tag->type & (tm_tag_file_t|tm_tag_undef_t)));
		
		symbol->priv->tm_tag = tm_tag;
	}
}

/* IAnjutaSymbol implementation */

static IAnjutaSymbolType
isymbol_type (IAnjutaSymbol *isymbol, GError **err)
{
	AnjutaSymbol *s;
	
	g_return_val_if_fail (ANJUTA_IS_SYMBOL (isymbol), IANJUTA_SYMBOL_TYPE_UNDEF);
	s = ANJUTA_SYMBOL (isymbol);
	g_return_val_if_fail (s->priv->tm_tag != NULL, IANJUTA_SYMBOL_TYPE_UNDEF);
	return s->priv->tm_tag->type;
}

static const gchar*
isymbol_type_name (IAnjutaSymbol *isymbol, GError **err)
{
	AnjutaSymbol *s;

	g_return_val_if_fail (ANJUTA_IS_SYMBOL (isymbol), NULL);
	s = ANJUTA_SYMBOL (isymbol);
	g_return_val_if_fail (s->priv->tm_tag != NULL, NULL);
	return s->priv->tm_tag->atts.entry.type_ref[1];
}

static const gchar*
isymbol_type_str (IAnjutaSymbol *isymbol, GError **err)
{
	DEBUG_PRINT ("TODO: isymbol_type_str ()");
	return NULL;
}

static const gchar*
isymbol_name (IAnjutaSymbol *isymbol, GError **err)
{
	AnjutaSymbol *s;

	g_return_val_if_fail (ANJUTA_IS_SYMBOL (isymbol), NULL);
	s = ANJUTA_SYMBOL (isymbol);
	g_return_val_if_fail (s->priv->tm_tag != NULL, NULL);
	return s->priv->tm_tag->name;
}

static const gchar*
isymbol_args (IAnjutaSymbol *isymbol, GError **err)
{
	AnjutaSymbol *s;

	g_return_val_if_fail (ANJUTA_IS_SYMBOL (isymbol), NULL);
	s = ANJUTA_SYMBOL (isymbol);
	g_return_val_if_fail (s->priv->tm_tag != NULL, NULL);
	return s->priv->tm_tag->atts.entry.arglist;
}

static const gchar*
isymbol_scope (IAnjutaSymbol *isymbol, GError **err)
{
	AnjutaSymbol *s;

	g_return_val_if_fail (ANJUTA_IS_SYMBOL (isymbol), NULL);
	s = ANJUTA_SYMBOL (isymbol);
	g_return_val_if_fail (s->priv->tm_tag != NULL, NULL);
	return s->priv->tm_tag->atts.entry.scope;
}

static const gchar*
isymbol_inheritance (IAnjutaSymbol *isymbol, GError **err)
{
	AnjutaSymbol *s;

	g_return_val_if_fail (ANJUTA_IS_SYMBOL (isymbol), NULL);
	s = ANJUTA_SYMBOL (isymbol);
	g_return_val_if_fail (s->priv->tm_tag != NULL, NULL);
//	return s->priv->tm_tag->atts.entry.inheritance;
	DEBUG_PRINT ("isymbol_inheritance () FIXME");
	return NULL;
}

static const gchar*
isymbol_access (IAnjutaSymbol *isymbol, GError **err)
{
	AnjutaSymbol *s;

	g_return_val_if_fail (ANJUTA_IS_SYMBOL (isymbol), '\0');
	s = ANJUTA_SYMBOL (isymbol);
	g_return_val_if_fail (s->priv->tm_tag != NULL, '\0');
	DEBUG_PRINT ("isymbol_access () FIXME");
	return "fixme";
//	return s->priv->tm_tag->atts.entry.access;
}

static const gchar*
isymbol_impl (IAnjutaSymbol *isymbol, GError **err)
{
	AnjutaSymbol *s;

	g_return_val_if_fail (ANJUTA_IS_SYMBOL (isymbol), '\0');
	s = ANJUTA_SYMBOL (isymbol);
	g_return_val_if_fail (s->priv->tm_tag != NULL, '\0');
	DEBUG_PRINT ("isymbol_impl () FIXME");
	return "fixme";
	
//	return s->priv->tm_tag->atts.entry.impl;	
}

static const gchar*
isymbol_uri (IAnjutaSymbol *isymbol, GError **err)
{
	AnjutaSymbol *s;

	g_return_val_if_fail (ANJUTA_IS_SYMBOL (isymbol), NULL);
	s = ANJUTA_SYMBOL (isymbol);
	g_return_val_if_fail (s->priv->tm_tag != NULL, NULL);
	if (s->priv->tm_tag->atts.entry.file == NULL)
		return NULL;
	if (s->priv->uri == NULL)
	{
		const gchar *file_path;
		file_path = s->priv->tm_tag->atts.entry.file->work_object.file_name;
		s->priv->uri = gnome_vfs_get_uri_from_local_path (file_path);
	}
	return s->priv->uri;
}

static gulong
isymbol_line (IAnjutaSymbol *isymbol, GError **err)
{
	AnjutaSymbol *s;

	g_return_val_if_fail (ANJUTA_IS_SYMBOL (isymbol), 0);
	s = ANJUTA_SYMBOL (isymbol);
	g_return_val_if_fail (s->priv->tm_tag != NULL, 0);
	return s->priv->tm_tag->atts.entry.line;
}

static gboolean
isymbol_is_local (IAnjutaSymbol *isymbol, GError **err)
{
	AnjutaSymbol *s;

	g_return_val_if_fail (ANJUTA_IS_SYMBOL (isymbol), FALSE);
	s = ANJUTA_SYMBOL (isymbol);
	g_return_val_if_fail (s->priv->tm_tag != NULL, FALSE);
	return s->priv->tm_tag->atts.entry.local;
}

static const GdkPixbuf*
isymbol_icon (IAnjutaSymbol *isymbol, GError **err)
{
	AnjutaSymbol *s;
	SVNodeType node_type;
	
	g_return_val_if_fail (ANJUTA_IS_SYMBOL (isymbol), FALSE);
	s = ANJUTA_SYMBOL (isymbol);
	g_return_val_if_fail (s->priv->tm_tag != NULL, NULL);
	node_type = anjuta_symbol_info_get_node_type (NULL, s->priv->tm_tag);
	return anjuta_symbol_info_get_pixbuf (node_type);
}

static void
isymbol_iface_init (IAnjutaSymbolIface *iface)
{
	iface->type = isymbol_type;
	iface->type_str = isymbol_type_str;	
	iface->type_name = isymbol_type_name;
	iface->name = isymbol_name;
	iface->args = isymbol_args;
	iface->scope = isymbol_scope;
	iface->inheritance = isymbol_inheritance;	
	iface->access = isymbol_access;
	iface->impl = isymbol_impl;
	iface->uri = isymbol_uri;
	iface->line = isymbol_line;
	iface->is_local = isymbol_is_local;
	iface->icon = isymbol_icon;
}

ANJUTA_TYPE_BEGIN (AnjutaSymbol, anjuta_symbol, G_TYPE_OBJECT);
ANJUTA_TYPE_ADD_INTERFACE (isymbol, IANJUTA_TYPE_SYMBOL);
ANJUTA_TYPE_END;

/* IAnjutaIterable implementation */
