/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * an_symbol_iter.h
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
#ifndef __ANJUTA_SYMBOL_ITER__
#define __ANJUTA_SYMBOL_ITER__

#include <glib.h>
#include <glib-object.h>
#include "an_symbol.h"

typedef struct _AnjutaSymbolIter      AnjutaSymbolIter;
typedef struct _AnjutaSymbolIterClass AnjutaSymbolIterClass;
typedef struct _AnjutaSymbolIterPriv  AnjutaSymbolIterPriv;

#define ANJUTA_TYPE_SYMBOL_ITER            (anjuta_symbol_iter_get_type ())
#define ANJUTA_SYMBOL_ITER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_SYMBOL_ITER, AnjutaSymbolIter))
#define ANJUTA_SYMBOL_ITER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_SYMBOL_ITER, AnjutaSymbolIterClass))
#define ANJUTA_IS_SYMBOL_ITER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_SYMBOL_ITER))
#define ANJUTA_IS_SYMBOL_ITER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_SYMBOL_ITER))

struct _AnjutaSymbolIter
{
	AnjutaSymbol parent;
	AnjutaSymbolIterPriv *priv;
};

struct _AnjutaSymbolIterClass
{
	AnjutaSymbolClass parent_class;
};

GType anjuta_symbol_iter_get_type (void);
AnjutaSymbolIter* anjuta_symbol_iter_new (const GPtrArray *tm_tags_array);

G_END_DECLS
#endif
