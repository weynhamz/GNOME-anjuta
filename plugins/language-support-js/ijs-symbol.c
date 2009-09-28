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


#include "ijs-symbol.h"

GQuark 
ijs_symbol_error_quark (void)
{
	static GQuark quark = 0;
	
	if (quark == 0) {
		quark = g_quark_from_static_string ("ijs-symbol-quark");
	}
	
	return quark;
}

/**
 * ijs_symbol_get_arg_list:
 * @obj: A #IJsSymbol object
 *
 * If @obj has BASE_TYPE BASE_FUNC than return GList* of name of arg (gchar*)
 *
 * Return value: NULL or GList* of gchar *.
 */
GList*
ijs_symbol_get_arg_list (IJsSymbol *obj)
{
	g_return_val_if_fail (IJS_IS_SYMBOL(obj), NULL);
	return IJS_SYMBOL_GET_IFACE (obj)->get_arg_list (obj);
}

/* Default implementation */
static GList*
ijs_symbol_get_arg_list_default (IJsSymbol *obj)
{
	g_return_val_if_reached (NULL);
}

/**
 * ijs_symbol_get_base_type:
 * @obj: A #IJsSymbol object
 *
 * Get @obj BASE_TYPE.
 *
 * Return value: @obj BASE_TYPE.
 */
gint
ijs_symbol_get_base_type (IJsSymbol *obj)
{
	g_return_val_if_fail (IJS_IS_SYMBOL(obj), 0);
	return IJS_SYMBOL_GET_IFACE (obj)->get_base_type (obj);
}

/* Default implementation */
static gint
ijs_symbol_get_base_type_default (IJsSymbol *obj)
{
	g_return_val_if_reached (0);
}

/**
 * ijs_symbol_get_func_ret_type:
 * @obj: A #IJsSymbol object
 *
 * If @obj has BASE_TYPE BASE_FUNC than return GList* of name of possible return type (gchar*)
 *
 * Return value: NULL or GList* of gchar *.
 */
GList*
ijs_symbol_get_func_ret_type (IJsSymbol *obj)
{
	g_return_val_if_fail (IJS_IS_SYMBOL(obj), NULL);
	return IJS_SYMBOL_GET_IFACE (obj)->get_func_ret_type (obj);
}

/* Default implementation */
static GList*
ijs_symbol_get_func_ret_type_default (IJsSymbol *obj)
{
	g_return_val_if_reached (NULL);
}

/**
 * ijs_symbol_get_member:
 * @obj: A #IJsSymbol object
 * @name: Name of member
 *
 * Get member of @obj with @name
 *
 * Return value: member of @obj with @name or NULL.
 */
IJsSymbol*
ijs_symbol_get_member (IJsSymbol *obj, const gchar * name)
{
	g_return_val_if_fail (IJS_IS_SYMBOL(obj), NULL);
	return IJS_SYMBOL_GET_IFACE (obj)->get_member (obj, name);
}

/* Default implementation */
static IJsSymbol*
ijs_symbol_get_member_default (IJsSymbol *obj, const gchar * name)
{
	g_return_val_if_reached (NULL);
}

/**
 * ijs_symbol_get_name:
 * @obj: A #IJsSymbol object
 *
 * Get self name
 *
 * Return value: self name or NULL.
 */
const gchar *
ijs_symbol_get_name (IJsSymbol *obj)
{
	g_return_val_if_fail (IJS_IS_SYMBOL(obj), NULL);
	return IJS_SYMBOL_GET_IFACE (obj)->get_name (obj);
}

/* Default implementation */
static const gchar *
ijs_symbol_get_name_default (IJsSymbol *obj)
{
	g_return_val_if_reached (NULL);
}

/**
 * ijs_symbol_list_member:
 * @obj: A #IJsSymbol object
 *
 * Get list of member's names. 
 *
 * Return value: self name or NULL.
 */
GList*
ijs_symbol_list_member (IJsSymbol *obj)
{
	g_return_val_if_fail (IJS_IS_SYMBOL(obj), NULL);
	return IJS_SYMBOL_GET_IFACE (obj)->list_member (obj);
}

/* Default implementation */
static GList*
ijs_symbol_list_member_default (IJsSymbol *obj)
{
	g_return_val_if_reached (NULL);
}

static void
ijs_symbol_base_init (IJsSymbolIface* klass)
{
	static gboolean initialized = FALSE;

	klass->get_arg_list = ijs_symbol_get_arg_list_default;
	klass->get_base_type = ijs_symbol_get_base_type_default;
	klass->get_func_ret_type = ijs_symbol_get_func_ret_type_default;
	klass->get_member = ijs_symbol_get_member_default;
	klass->get_name = ijs_symbol_get_name_default;
	klass->list_member = ijs_symbol_list_member_default;
	
	if (!initialized) {

		initialized = TRUE;
	}
}

GType
ijs_symbol_get_type (void)
{
	static GType type = 0;
	if (!type) {
		static const GTypeInfo info = {
			sizeof (IJsSymbolIface),
			(GBaseInitFunc) ijs_symbol_base_init,
			NULL, 
			NULL,
			NULL,
			NULL,
			0,
			0,
			NULL
		};
		type = g_type_register_static (G_TYPE_INTERFACE, "IJsSymbol", &info, 0);
		g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	}
	return type;			
}
