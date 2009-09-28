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

#ifndef _STD_SYMBOL_H_
#define _STD_SYMBOL_H_

#include <glib-object.h>

#include "ijs-symbol.h"

G_BEGIN_DECLS

#define STD_TYPE_SYMBOL             (std_symbol_get_type ())
#define STD_SYMBOL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), STD_TYPE_SYMBOL, StdSymbol))
#define STD_SYMBOL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), STD_TYPE_SYMBOL, StdSymbolClass))
#define STD_IS_SYMBOL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), STD_TYPE_SYMBOL))
#define STD_IS_SYMBOL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), STD_TYPE_SYMBOL))
#define STD_SYMBOL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), STD_TYPE_SYMBOL, StdSymbolClass))

typedef struct _StdSymbolClass StdSymbolClass;
typedef struct _StdSymbol StdSymbol;

struct _StdSymbolClass
{
	GObjectClass parent_class;
};

struct _StdSymbol
{
	GObject parent_instance;
};

GType std_symbol_get_type (void) G_GNUC_CONST;
StdSymbol* std_symbol_new (void);

G_END_DECLS

#endif /* _STD_SYMBOL_H_ */
