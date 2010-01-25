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

#ifndef _IMPORT_SYMBOL_H_
#define _IMPORT_SYMBOL_H_

#include <glib-object.h>

#include "ijs-symbol.h"

G_BEGIN_DECLS

#define IMPORT_TYPE_SYMBOL             (import_symbol_get_type ())
#define IMPORT_SYMBOL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), IMPORT_TYPE_SYMBOL, ImportSymbol))
#define IMPORT_SYMBOL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), IMPORT_TYPE_SYMBOL, ImportSymbolClass))
#define IMPORT_IS_SYMBOL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IMPORT_TYPE_SYMBOL))
#define IMPORT_IS_SYMBOL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), IMPORT_TYPE_SYMBOL))
#define IMPORT_SYMBOL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), IMPORT_TYPE_SYMBOL, ImportSymbolClass))

typedef struct _ImportSymbolClass ImportSymbolClass;
typedef struct _ImportSymbol ImportSymbol;

struct _ImportSymbolClass
{
	GObjectClass parent_class;
};

struct _ImportSymbol
{
	GObject parent_instance;
};

GType import_symbol_get_type (void) G_GNUC_CONST;
ImportSymbol* import_symbol_new (void);

G_END_DECLS

#endif /* _IMPORT_SYMBOL_H_ */
