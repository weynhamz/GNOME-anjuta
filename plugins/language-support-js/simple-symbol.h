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

#ifndef _SIMPLE_SYMBOL_H_
#define _SIMPLE_SYMBOL_H_

#include <glib-object.h>
#include "ijs-symbol.h"
#include "util.h"

G_BEGIN_DECLS

#define SIMPLE_TYPE_SYMBOL             (simple_symbol_get_type ())
#define SIMPLE_SYMBOL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SIMPLE_TYPE_SYMBOL, SimpleSymbol))
#define SIMPLE_SYMBOL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SIMPLE_TYPE_SYMBOL, SimpleSymbolClass))
#define SIMPLE_IS_SYMBOL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SIMPLE_TYPE_SYMBOL))
#define SIMPLE_IS_SYMBOL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SIMPLE_TYPE_SYMBOL))
#define SIMPLE_SYMBOL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SIMPLE_TYPE_SYMBOL, SimpleSymbolClass))

typedef struct _SimpleSymbolClass SimpleSymbolClass;
typedef struct _SimpleSymbol SimpleSymbol;

struct _SimpleSymbolClass
{
	GObjectClass parent_class;
};

struct _SimpleSymbol
{
	GObject parent_instance;

	gchar * name;
	gint type;
	GList* member; /*List of IJsSymbol**/
	GList* ret_type;
	GList* args;
};

GType simple_symbol_get_type (void) G_GNUC_CONST;
SimpleSymbol* simple_symbol_new (void);

G_END_DECLS

#endif /* _SIMPLE_SYMBOL_H_ */
