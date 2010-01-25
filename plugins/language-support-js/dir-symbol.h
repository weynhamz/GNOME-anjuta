/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    Copyright (C) 2009 Maxim Ermilov   <zaspire@rambler.ru>

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

#ifndef _DIR_SYMBOL_H_
#define _DIR_SYMBOL_H_

#include <glib-object.h>

#include "ijs-symbol.h"

G_BEGIN_DECLS

#define DIR_TYPE_SYMBOL             (dir_symbol_get_type ())
#define DIR_SYMBOL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIR_TYPE_SYMBOL, DirSymbol))
#define DIR_SYMBOL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DIR_TYPE_SYMBOL, DirSymbolClass))
#define DIR_IS_SYMBOL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIR_TYPE_SYMBOL))
#define DIR_IS_SYMBOL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DIR_TYPE_SYMBOL))
#define DIR_SYMBOL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DIR_TYPE_SYMBOL, DirSymbolClass))

typedef struct _DirSymbolClass DirSymbolClass;
typedef struct _DirSymbol DirSymbol;

struct _DirSymbolClass
{
	GObjectClass parent_class;
};

struct _DirSymbol
{
	GObject parent_instance;
};

GType dir_symbol_get_type (void) G_GNUC_CONST;
DirSymbol* dir_symbol_new (const gchar* dirname);
gchar* dir_symbol_get_path (DirSymbol* self);

G_END_DECLS

#endif /* _DIR_SYMBOL_H_ */
