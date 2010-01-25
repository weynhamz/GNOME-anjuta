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

#include "std-symbol.h"
#include "simple-symbol.h"
#include "util.h"

static void std_symbol_interface_init (IJsSymbolIface *iface);
static GList* std_symbol_get_arg_list (IJsSymbol *obj);
static gint std_symbol_get_base_type (IJsSymbol *obj);
static GList* std_symbol_get_func_ret_type (IJsSymbol *obj);
static IJsSymbol* std_symbol_get_member (IJsSymbol *obj, const gchar * name);
static const gchar * std_symbol_get_name (IJsSymbol *obj);
static GList* std_symbol_list_member (IJsSymbol *obj);

typedef struct
{
	const gchar *name;
	const gchar **member;
}StandartObject;

static const gchar *ErrorMember[] = { "message", "name", NULL };
static const gchar *MathMember[] = {"E", "LN10", "LN2", "LOG10E", "LOG2E", "PI", "SQRT1_2", "SQRT2",
		"abs", "acos", "asin", "atan", "atan2", "ceil", "cos", "exp", "floor", "log", "max",
		"min", "pow", "random", "round", "sin", "sqrt", "tan", NULL
};
static const gchar *RegExpMember[] = { "compile", "exec", "test", "toString", "valueOf",
		"global", "ignoreCase", "input", "lastIndex", "lastMatch", "lastParen",
		"leftContext", "multiline", "prototype", "rightContext", "source",
};
static const gchar *DateMember[] = {
 "getTime", "getTimezoneOffset", "getYear",
 "getFullYear", "getUTCFullYear", "getMonth", "getUTCMonth", "getDate", "getUTCDate", "getDay", "getUTCDay", "getHours",
 "getUTCHours", "getMinutes", "getUTCMinutes", "getSeconds", "getUTCSeconds", "getMilliseconds",
 "getUTCMilliseconds", "setTime", "setYear", "setFullYear", "setUTCFullYear",
 "setMonth", "setUTCMonth", "setDate", "setUTCDate", "setHours", "setUTCHours",
 "setMinutes", "setUTCMinutes", "setSeconds", "setUTCSeconds",
 "setMilliseconds", "setUTCMilliseconds", "toUTCString", "toLocaleDateString", "toLocaleTimeString",
 "toLocaleFormat", "toDateString", "toTimeString", NULL
};
static const gchar *ArrayMember[] = {
		"concat", "join", "pop", "push", "reverse", "shift", "slice", "sort",
		"splice", "toLocaleString", "toString", "unshift", "valueOf", "length", NULL
};
static const gchar *StringMember[] = {
		"charAt", "charCodeAt", "concat", "fromCharCode", "indexOf", "lastIndexOf",
		"localeCompare", "match", "replace", "search",
		"slice", "split", "substr", "substring", "toLocaleLowerCase", "toLocaleUpperCase",
		"toLowerCase", "toString", "toUpperCase", "valueOf", "length", NULL
};
static const gchar *ObjectMember[] = {
		"hasOwnProperty", "isPrototypeOf", "propertyIsEnumerable", "toLocaleString",
		"toString", "valueOf", NULL
};
static const gchar *FunctionMember[] = {
		"arguments", "caller", "constructor", "length", "apply", "call", "toString", "valueOf", NULL
};
static const gchar *NumberMember[] = {
		"MAX_VALUE", "MIN_VALUE",
		"NaN", "NEGATIVE_INFINITY", "POSITIVE_INFINITY", "toExponential",
		"toFixed", "toLocaleString", "toPrecision", "toString", "valueOf", NULL
};
static const StandartObject stdSym[]= {
		{"undefined", NULL},
		{"Function", FunctionMember},
		{"Object", ObjectMember},
		{"Array", ArrayMember},
		{"Boolean", NULL},
		{"Date", DateMember},
		{"Math", MathMember},
		{"Number", NumberMember},
		{"NaN", NULL},
		{"Infinity", NULL},
		{"isNaN", NULL},
		{"isFinite", NULL},
		{"parseFloat", NULL},
		{"parseInt", NULL},
		{"String", StringMember},
		{"escape", NULL},
		{"unescape", NULL},
		{"decodeURI", NULL},
		{"encodeURI", NULL},
		{"decodeURIComponent", NULL},
		{"encodeURIComponent", NULL},
		{"uneval", NULL},
		{"Error", ErrorMember},
		{"InternalError", NULL},
		{"EvalError", NULL},
		{"RangeError", NULL},
		{"ReferenceError", NULL},
		{"SyntaxError", NULL},
		{"TypeError", NULL},
		{"URIError", NULL},
		{"RegExp", RegExpMember},
		{"Script", NULL},
		{"XML", NULL},
		{"XMLList", NULL},
		{"isXMLName", NULL},
		{"Namespace", NULL},
		{"QName", NULL},
		{"StopIteration", NULL},
		{"Iterator", NULL},
		{"Generator", NULL},
		{NULL, NULL},
};

typedef struct _StdSymbolPrivate StdSymbolPrivate;
struct _StdSymbolPrivate
{
	gpointer hz;
};

#define STD_SYMBOL_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), STD_TYPE_SYMBOL, StdSymbolPrivate))


G_DEFINE_TYPE_WITH_CODE (StdSymbol, std_symbol, G_TYPE_OBJECT,
				G_IMPLEMENT_INTERFACE (IJS_TYPE_SYMBOL,
						std_symbol_interface_init));

static void
std_symbol_init (StdSymbol *object)
{
	/* TODO: Add initialization code here */
}

static void
std_symbol_finalize (GObject *object)
{
	/* TODO: Add deinitalization code here */

	G_OBJECT_CLASS (std_symbol_parent_class)->finalize (object);
}

static void
std_symbol_class_init (StdSymbolClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	/*GObjectClass* parent_class = G_OBJECT_CLASS (klass);*/

	g_type_class_add_private (klass, sizeof (StdSymbolPrivate));

	object_class->finalize = std_symbol_finalize;
}


StdSymbol*
std_symbol_new (void)
{
	return g_object_new (STD_TYPE_SYMBOL, NULL);
}

static void
std_symbol_interface_init (IJsSymbolIface *iface)
{
	iface->get_arg_list = std_symbol_get_arg_list;
	iface->get_base_type = std_symbol_get_base_type;
	iface->get_func_ret_type = std_symbol_get_func_ret_type;

	iface->get_member = std_symbol_get_member;
	iface->get_name = std_symbol_get_name;
	iface->list_member = std_symbol_list_member;
}

static GList*
std_symbol_get_arg_list (IJsSymbol *obj)
{
	g_assert_not_reached ();
	return NULL;
}

static gint
std_symbol_get_base_type (IJsSymbol *obj)
{
	return BASE_CLASS;
}

static GList*
std_symbol_get_func_ret_type (IJsSymbol *obj)
{
	g_assert_not_reached ();
	return NULL;
}

static IJsSymbol*
std_symbol_get_member (IJsSymbol *obj, const gchar * name)
{
	int i;

	for (i = 0; stdSym[i].name; i++)
	{
		if (g_strcmp0 (name, stdSym[i].name) != 0)
			continue;
		if (!stdSym[i].member)
			break;

		SimpleSymbol* symbol = simple_symbol_new ();
		symbol->name = g_strdup (name);
		GList *members = NULL;
		int k;
		for (k = 0; stdSym[i].member[k]; k++)
		{
			SimpleSymbol* symbol = simple_symbol_new ();
			symbol->name = g_strdup (stdSym[i].member[k]);
			members = g_list_append (members, symbol);
		}
		symbol->member = members;
		return IJS_SYMBOL (symbol);
	}
	return NULL;
}

static const gchar *
std_symbol_get_name (IJsSymbol *obj)
{
	g_assert_not_reached ();
	return "";
}

static GList*
std_symbol_list_member (IJsSymbol *obj)
{
	int i;
	GList *ret = NULL;
	for (i = 0; stdSym[i].name; i++)
		ret = g_list_append (ret, g_strdup (stdSym[i].name));
	return ret;
}
