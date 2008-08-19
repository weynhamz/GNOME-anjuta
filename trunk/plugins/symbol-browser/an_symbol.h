/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta_symbol.c
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
#ifndef __ANJUTA_SYMBOL__
#define __ANJUTA_SYMBOL__

#include <glib-object.h>
#include <tm_tagmanager.h>

typedef struct _AnjutaSymbol      AnjutaSymbol;
typedef struct _AnjutaSymbolClass AnjutaSymbolClass;
typedef struct _AnjutaSymbolPriv  AnjutaSymbolPriv;

#define ANJUTA_TYPE_SYMBOL            (anjuta_symbol_get_type ())
#define ANJUTA_SYMBOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_SYMBOL, AnjutaSymbol))
#define ANJUTA_SYMBOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_SYMBOL, AnjutaSymbolClass))
#define ANJUTA_IS_SYMBOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_SYMBOL))
#define ANJUTA_IS_SYMBOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_SYMBOL))

struct _AnjutaSymbol
{
	GObject parent;
	AnjutaSymbolPriv *priv;
};

struct _AnjutaSymbolClass
{
	GObjectClass parent_class;
};

GType anjuta_symbol_get_type (void);
AnjutaSymbol* anjuta_symbol_new (const TMTag *tm_tag);
void anjuta_symbol_set_tag (AnjutaSymbol* symbol, const TMTag *tm_tag);
const gchar* anjuta_symbol_get_name (AnjutaSymbol* symbol);

G_END_DECLS
#endif
