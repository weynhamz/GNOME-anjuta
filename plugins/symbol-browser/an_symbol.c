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


const gchar* 
anjuta_symbol_get_name (AnjutaSymbol* symbol)
{
	g_return_val_if_fail (symbol != NULL, NULL);
	
	return symbol->priv->tm_tag->name;
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
static const gchar*
isymbol_get_name (IAnjutaSymbol *isymbol, GError **err)
{
	AnjutaSymbol *s;

	g_return_val_if_fail (ANJUTA_IS_SYMBOL (isymbol), NULL);
	s = ANJUTA_SYMBOL (isymbol);
	g_return_val_if_fail (s->priv->tm_tag != NULL, NULL);
	return s->priv->tm_tag->name;
}

static IAnjutaSymbolType
isymbol_get_sym_type (IAnjutaSymbol *isymbol, GError **err)
{
	AnjutaSymbol *s;
	
	g_return_val_if_fail (ANJUTA_IS_SYMBOL (isymbol), IANJUTA_SYMBOL_TYPE_UNDEF);
	s = ANJUTA_SYMBOL (isymbol);
	g_return_val_if_fail (s->priv->tm_tag != NULL, IANJUTA_SYMBOL_TYPE_UNDEF);
	return s->priv->tm_tag->type;
}

static const gchar*
isymbol_get_args (IAnjutaSymbol *isymbol, GError **err)
{
	AnjutaSymbol *s;

	g_return_val_if_fail (ANJUTA_IS_SYMBOL (isymbol), NULL);
	s = ANJUTA_SYMBOL (isymbol);
	g_return_val_if_fail (s->priv->tm_tag != NULL, NULL);
	return s->priv->tm_tag->atts.entry.arglist;
}

static const gchar*
isymbol_get_extra_info_string (IAnjutaSymbol *isymbol, IAnjutaSymbolField sym_info,
							   GError **err)
{
	AnjutaSymbol *s;

	g_return_val_if_fail (ANJUTA_IS_SYMBOL (isymbol), NULL);
	s = ANJUTA_SYMBOL (isymbol);
	g_return_val_if_fail (s->priv->tm_tag != NULL, NULL);
	
	switch (sym_info) {
		case IANJUTA_SYMBOL_FIELD_FILE_PATH:
			if (s->priv->tm_tag->atts.entry.file == NULL)
				return NULL;
			return s->priv->tm_tag->atts.entry.file->work_object.file_name;
			
		case IANJUTA_SYMBOL_FIELD_IMPLEMENTATION:
			g_message ("TODO IANJUTA_SYMBOL_FIELD_IMPLEMENTATION");
			return NULL;
			/*return s->priv->tm_tag->atts.entry.impl;*/
		
		case IANJUTA_SYMBOL_FIELD_ACCESS:
			g_message ("TODO IANJUTA_SYMBOL_FIELD_ACCESS");
			return NULL;
			/*return s->priv->tm_tag->atts.entry.access;*/
		
		case IANJUTA_SYMBOL_FIELD_KIND:
			g_message ("No symbol kind to string translation available");
			return NULL;
			
		case IANJUTA_SYMBOL_FIELD_TYPE:
			g_message ("No symbol type to string translation available");
			return NULL;		
		
		case IANJUTA_SYMBOL_FIELD_TYPE_NAME:
			return s->priv->tm_tag->atts.entry.type_ref[1];
		
		case IANJUTA_SYMBOL_FIELD_LANGUAGE:
			g_message ("No file language available");
			return NULL;
		
		case IANJUTA_SYMBOL_FIELD_FILE_IGNORE:
			return NULL;
		
		case IANJUTA_SYMBOL_FIELD_FILE_INCLUDE:
			return NULL;
		
		case IANJUTA_SYMBOL_FIELD_PROJECT_NAME:
			return NULL;
		
		case IANJUTA_SYMBOL_FIELD_WORKSPACE_NAME:
			return NULL;
		
		default:
			return NULL;
	}
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
isymbol_get_line (IAnjutaSymbol *isymbol, GError **err)
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
isymbol_get_icon (IAnjutaSymbol *isymbol, GError **err)
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
	iface->get_uri = isymbol_uri;
	iface->get_name = isymbol_get_name;	
	iface->get_line = isymbol_get_line;
	iface->is_local = isymbol_is_local;
	iface->get_sym_type = isymbol_get_sym_type;
	iface->get_args = isymbol_get_args;
	iface->get_extra_info_string = isymbol_get_extra_info_string;
	iface->get_icon = isymbol_get_icon;
}


ANJUTA_TYPE_BEGIN (AnjutaSymbol, anjuta_symbol, G_TYPE_OBJECT);
ANJUTA_TYPE_ADD_INTERFACE (isymbol, IANJUTA_TYPE_SYMBOL);
ANJUTA_TYPE_END;


