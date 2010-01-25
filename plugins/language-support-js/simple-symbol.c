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

#include "simple-symbol.h"

static void simple_symbol_interface_init (IJsSymbolIface *iface);
static GList* simple_symbol_get_arg_list (IJsSymbol *obj);
static gint simple_symbol_get_base_type (IJsSymbol *obj);
static GList* simple_symbol_get_func_ret_type (IJsSymbol *obj);
static IJsSymbol* simple_symbol_get_member (IJsSymbol *obj, const gchar * name);
static const gchar * simple_symbol_get_name (IJsSymbol *obj);
static GList* simple_symbol_list_member (IJsSymbol *obj);

G_DEFINE_TYPE_WITH_CODE (SimpleSymbol, simple_symbol, G_TYPE_OBJECT,
			G_IMPLEMENT_INTERFACE (IJS_TYPE_SYMBOL,
						simple_symbol_interface_init));

static void
simple_symbol_init (SimpleSymbol *object)
{
	object->name = NULL;
	object->member = NULL;
	object->ret_type = NULL;
	object->args = NULL;
}

static void
simple_symbol_finalize (GObject *obj)
{
	SimpleSymbol *self = SIMPLE_SYMBOL (obj);
	g_free (self->name);

	g_list_foreach (self->member, (GFunc)g_object_unref, NULL);
	g_list_free (self->member);
	
	g_list_foreach (self->ret_type, (GFunc)g_free, NULL);
	g_list_free (self->ret_type);

	g_list_foreach (self->args, (GFunc)g_free, NULL);
	g_list_free (self->args);

	G_OBJECT_CLASS (simple_symbol_parent_class)->finalize (obj);
}

static void
simple_symbol_class_init (SimpleSymbolClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	/*GObjectClass* parent_class = G_OBJECT_CLASS (klass);*/

	object_class->finalize = simple_symbol_finalize;
}


SimpleSymbol*
simple_symbol_new (void)
{
	return g_object_new (SIMPLE_TYPE_SYMBOL, NULL);
}

static void
simple_symbol_interface_init (IJsSymbolIface *iface)
{
	iface->get_arg_list = simple_symbol_get_arg_list;
	iface->get_base_type = simple_symbol_get_base_type;
	iface->get_func_ret_type = simple_symbol_get_func_ret_type;
	iface->get_member = simple_symbol_get_member;
	iface->get_name = simple_symbol_get_name;
	iface->list_member = simple_symbol_list_member;
}

static GList*
simple_symbol_get_arg_list (IJsSymbol *obj)
{
	SimpleSymbol *symbol = SIMPLE_SYMBOL (obj);
	GList* ret = NULL;
	GList* i;
	for (i = symbol->args; i; i = g_list_next (i))
	{
		ret = g_list_append (ret, g_strdup ((gchar*)i->data));
	}
	return ret;
}

static gint
simple_symbol_get_base_type (IJsSymbol *obj)
{
	SimpleSymbol *symbol = SIMPLE_SYMBOL (obj);
	return symbol->type;
}

static GList*
simple_symbol_get_func_ret_type (IJsSymbol *obj)
{
	SimpleSymbol *symbol = SIMPLE_SYMBOL (obj);
	GList* ret = NULL;
	GList* i;
	for (i = symbol->ret_type; i; i = g_list_next (i))
	{
		ret = g_list_append (ret, g_strdup ((gchar*)i->data));
	}
	return ret;
}

static IJsSymbol*
simple_symbol_get_member (IJsSymbol *obj, const gchar * name)
{
	GList *i;
	SimpleSymbol *symbol = SIMPLE_SYMBOL (obj);
	for (i = symbol->member; i; i = g_list_next (i))
	{
		IJsSymbol* t = IJS_SYMBOL (i->data);
		if (g_strcmp0 (name, ijs_symbol_get_name (t)) == 0 )
		{
			g_object_ref (t);
			return t;
		}
	}
	return NULL;
}

static const gchar *
simple_symbol_get_name (IJsSymbol *obj)
{
	SimpleSymbol *symbol = SIMPLE_SYMBOL (obj);
	return symbol->name;
}

static GList*
simple_symbol_list_member (IJsSymbol *obj)
{
	GList *i;
	SimpleSymbol *symbol = SIMPLE_SYMBOL (obj);
	GList *ret = NULL;
	for (i = symbol->member; i; i = g_list_next (i))
	{
		IJsSymbol* t = IJS_SYMBOL (i->data);
		ret = g_list_append (ret, g_strdup (ijs_symbol_get_name (t)));
	}
	return ret;
}
