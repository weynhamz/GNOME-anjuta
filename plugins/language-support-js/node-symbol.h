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

#ifndef _NODE_SYMBOL_H_
#define _NODE_SYMBOL_H_

#include <glib-object.h>

#include "jsparse.h"

G_BEGIN_DECLS

#define NODE_TYPE_SYMBOL             (node_symbol_get_type ())
#define NODE_SYMBOL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), NODE_TYPE_SYMBOL, NodeSymbol))
#define NODE_SYMBOL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), NODE_TYPE_SYMBOL, NodeSymbolClass))
#define NODE_IS_SYMBOL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NODE_TYPE_SYMBOL))
#define NODE_IS_SYMBOL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), NODE_TYPE_SYMBOL))
#define NODE_SYMBOL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), NODE_TYPE_SYMBOL, NodeSymbolClass))

typedef struct _NodeSymbolClass NodeSymbolClass;
typedef struct _NodeSymbol NodeSymbol;

struct _NodeSymbolClass
{
	GObjectClass parent_class;
};

struct _NodeSymbol
{
	GObject parent_instance;
};

GType node_symbol_get_type (void) G_GNUC_CONST;
NodeSymbol* node_symbol_new (JSNode *node, const gchar *name, JSContext *my_cx);

G_END_DECLS

#endif /* _NODE_SYMBOL_H_ */
