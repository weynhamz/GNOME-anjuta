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

#ifndef _GIR_SYMBOL_H_
#define _GIR_SYMBOL_H_

#include <glib-object.h>
#include "ijs-symbol.h"

G_BEGIN_DECLS

#define GIR_TYPE_SYMBOL             (gir_symbol_get_type ())
#define GIR_SYMBOL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIR_TYPE_SYMBOL, GirSymbol))
#define GIR_SYMBOL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIR_TYPE_SYMBOL, GirSymbolClass))
#define GIR_IS_SYMBOL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIR_TYPE_SYMBOL))
#define GIR_IS_SYMBOL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIR_TYPE_SYMBOL))
#define GIR_SYMBOL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIR_TYPE_SYMBOL, GirSymbolClass))

typedef struct _GirSymbolClass GirSymbolClass;
typedef struct _GirSymbol GirSymbol;

struct _GirSymbolClass
{
	GObjectClass parent_class;
};

struct _GirSymbol
{
	GObject parent_instance;
};

GType gir_symbol_get_type (void) G_GNUC_CONST;
IJsSymbol* gir_symbol_new (const gchar *filename, const gchar *lib_name);

G_END_DECLS

#endif /* _GIR_SYMBOL_H_ */
