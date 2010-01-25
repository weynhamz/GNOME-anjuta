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

#ifndef _IJS_SYMBOL_H_
#define _IJS_SYMBOL_H_

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define IJS_TYPE_SYMBOL (ijs_symbol_get_type ())
#define IJS_SYMBOL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), IJS_TYPE_SYMBOL, IJsSymbol))
#define IJS_IS_SYMBOL(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IJS_TYPE_SYMBOL))
#define IJS_SYMBOL_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), IJS_TYPE_SYMBOL, IJsSymbolIface))

#define IJS_SYMBOL_ERROR ijs_symbol_error_quark()

typedef struct _IJsSymbol IJsSymbol;
typedef struct _IJsSymbolIface IJsSymbolIface;


struct _IJsSymbolIface {
	GTypeInterface g_iface;
	

	GList* (*get_arg_list) (IJsSymbol *obj);
	gint (*get_base_type) (IJsSymbol *obj);
	GList* (*get_func_ret_type) (IJsSymbol *obj);
	IJsSymbol* (*get_member) (IJsSymbol *obj, const gchar * name);
	const gchar * (*get_name) (IJsSymbol *obj);
	GList* (*list_member) (IJsSymbol *obj);
};


GQuark ijs_symbol_error_quark     (void);
GType  ijs_symbol_get_type        (void);

GList* ijs_symbol_get_arg_list (IJsSymbol *obj);

gint ijs_symbol_get_base_type (IJsSymbol *obj);

GList* ijs_symbol_get_func_ret_type (IJsSymbol *obj);

IJsSymbol* ijs_symbol_get_member (IJsSymbol *obj, const gchar * name);

const gchar * ijs_symbol_get_name (IJsSymbol *obj);

GList* ijs_symbol_list_member (IJsSymbol *obj);


G_END_DECLS

#endif
