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

#ifndef _GI_SYMBOL_H_
#define _GI_SYMBOL_H_

#include <glib-object.h>
#include "ijs-symbol.h"

G_BEGIN_DECLS

#define GI_TYPE_SYMBOL             (gi_symbol_get_type ())
#define GI_SYMBOL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GI_TYPE_SYMBOL, GiSymbol))
#define GI_SYMBOL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GI_TYPE_SYMBOL, GiSymbolClass))
#define GI_IS_SYMBOL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GI_TYPE_SYMBOL))
#define GI_IS_SYMBOL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GI_TYPE_SYMBOL))
#define GI_SYMBOL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GI_TYPE_SYMBOL, GiSymbolClass))

typedef struct _GiSymbolClass GiSymbolClass;
typedef struct _GiSymbol GiSymbol;

struct _GiSymbolClass
{
	GObjectClass parent_class;
};

struct _GiSymbol
{
	GObject parent_instance;
};

GType gi_symbol_get_type (void) G_GNUC_CONST;
IJsSymbol* gi_symbol_new (void);

G_END_DECLS

#endif /* _GI_SYMBOL_H_ */
