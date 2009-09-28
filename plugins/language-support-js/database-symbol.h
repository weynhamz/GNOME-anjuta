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

#ifndef _DATABASE_SYMBOL_H_
#define _DATABASE_SYMBOL_H_

#include <glib-object.h>

#include "ijs-symbol.h"

G_BEGIN_DECLS

#define DATABASE_TYPE_SYMBOL             (database_symbol_get_type ())
#define DATABASE_SYMBOL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), DATABASE_TYPE_SYMBOL, DatabaseSymbol))
#define DATABASE_SYMBOL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DATABASE_TYPE_SYMBOL, DatabaseSymbolClass))
#define DATABASE_IS_SYMBOL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DATABASE_TYPE_SYMBOL))
#define DATABASE_IS_SYMBOL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DATABASE_TYPE_SYMBOL))
#define DATABASE_SYMBOL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DATABASE_TYPE_SYMBOL, DatabaseSymbolClass))

typedef struct _DatabaseSymbolClass DatabaseSymbolClass;
typedef struct _DatabaseSymbol DatabaseSymbol;

struct _DatabaseSymbolClass
{
	GObjectClass parent_class;
};

struct _DatabaseSymbol
{
	GObject parent_instance;
};

GType database_symbol_get_type (void) G_GNUC_CONST;
DatabaseSymbol* database_symbol_new (void);
void database_symbol_set_file (DatabaseSymbol *object, const gchar* filename);
GList* database_symbol_list_local_member (DatabaseSymbol *object, gint line);
GList* database_symbol_list_member_with_line (DatabaseSymbol *object, gint line);

G_END_DECLS

#endif /* _DATABASE_SYMBOL_H_ */
