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

#ifndef _DB_ANJUTA_SYMBOL_H_
#define _DB_ANJUTA_SYMBOL_H_

#include <glib-object.h>

#include "ijs-symbol.h"

G_BEGIN_DECLS

#define DB_TYPE_ANJUTA_SYMBOL             (db_anjuta_symbol_get_type ())
#define DB_ANJUTA_SYMBOL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), DB_TYPE_ANJUTA_SYMBOL, DbAnjutaSymbol))
#define DB_ANJUTA_SYMBOL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DB_TYPE_ANJUTA_SYMBOL, DbAnjutaSymbolClass))
#define DB_IS_ANJUTA_SYMBOL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DB_TYPE_ANJUTA_SYMBOL))
#define DB_IS_ANJUTA_SYMBOL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DB_TYPE_ANJUTA_SYMBOL))
#define DB_ANJUTA_SYMBOL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DB_TYPE_ANJUTA_SYMBOL, DbAnjutaSymbolClass))

typedef struct _DbAnjutaSymbolClass DbAnjutaSymbolClass;
typedef struct _DbAnjutaSymbol DbAnjutaSymbol;

struct _DbAnjutaSymbolClass
{
	GObjectClass parent_class;
};

struct _DbAnjutaSymbol
{
	GObject parent_instance;
};

GType db_anjuta_symbol_get_type (void) G_GNUC_CONST;
DbAnjutaSymbol* db_anjuta_symbol_new (const gchar *file_name);

G_END_DECLS

#endif /* _DB_ANJUTA_SYMBOL_H_ */
