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

#include <string.h>
#include <gio/gio.h>

#include "ijs-symbol.h"
#include "node-symbol.h"
#include "local-symbol.h"
#include "util.h"

static void local_symbol_interface_init (IJsSymbolIface *iface);
static GList* local_symbol_get_arg_list (IJsSymbol *obj);
static gint local_symbol_get_base_type (IJsSymbol *obj);
static GList* local_symbol_get_func_ret_type (IJsSymbol *obj);
static IJsSymbol* local_symbol_get_member (IJsSymbol *obj, const gchar * name);
static const gchar * local_symbol_get_name (IJsSymbol *obj);
static GList* local_symbol_list_member (IJsSymbol *obj);

static GList* get_var_list (gint lineno, JSContext *my_cx);

G_DEFINE_TYPE_WITH_CODE (LocalSymbol, local_symbol, G_TYPE_OBJECT,
			G_IMPLEMENT_INTERFACE (IJS_TYPE_SYMBOL,
						local_symbol_interface_init));

typedef struct _LocalSymbolPrivate LocalSymbolPrivate;
struct _LocalSymbolPrivate
{
	JSContext *my_cx;
	JSNode *node;
	GList *missed_semicolon;
	gchar *self_name;
	GList *calls;
};

#define LOCAL_SYMBOL_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), LOCAL_TYPE_SYMBOL, LocalSymbolPrivate))

static void
local_symbol_init (LocalSymbol *object)
{
	LocalSymbolPrivate *priv = LOCAL_SYMBOL_PRIVATE (object);
	priv->my_cx = NULL;
	priv->node = NULL;
	priv->self_name = NULL;
	priv->calls = NULL;
	priv->missed_semicolon = NULL;
}

static void
local_symbol_finalize (GObject *object)
{
	LocalSymbolPrivate *priv = LOCAL_SYMBOL_PRIVATE (object);

	g_list_free (priv->calls);
	g_list_free (priv->missed_semicolon);
	g_free (priv->self_name);
	if (priv->my_cx)
		g_object_unref (priv->my_cx);
	if (priv->node)
		g_object_unref (priv->node);

	G_OBJECT_CLASS (local_symbol_parent_class)->finalize (object);
}

static void
local_symbol_class_init (LocalSymbolClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	/*GObjectClass* parent_class = G_OBJECT_CLASS (klass);*/

	g_type_class_add_private (klass, sizeof (LocalSymbolPrivate));

	object_class->finalize = local_symbol_finalize;
}


LocalSymbol*
local_symbol_new (const gchar *filename)
{
	LocalSymbol* ret = LOCAL_SYMBOL (g_object_new (LOCAL_TYPE_SYMBOL, NULL));
	LocalSymbolPrivate *priv = LOCAL_SYMBOL_PRIVATE (ret);

	priv->node = js_node_new_from_file (filename);
	if (priv->node)
	{
		priv->missed_semicolon = js_node_get_lines_missed_semicolon (priv->node);
		priv->calls = NULL;
		priv->my_cx = js_context_new_from_node (priv->node, &priv->calls);

		GFile *file = g_file_new_for_path (filename);
		priv->self_name = g_file_get_basename (file);
		g_object_unref (file);
		if (strcmp (priv->self_name + strlen (priv->self_name) - 3, ".js") == 0)
			priv->self_name[strlen (priv->self_name) - 3] = '\0';
	}
	return ret;
}

GList*
local_symbol_get_missed_semicolons (LocalSymbol* object)
{
	g_assert (LOCAL_IS_SYMBOL (object));
	LocalSymbolPrivate *priv = LOCAL_SYMBOL_PRIVATE (object);
	return priv->missed_semicolon;
}

GList*
local_symbol_list_member_with_line (LocalSymbol* object, gint line)
{
	g_assert (LOCAL_IS_SYMBOL (object));
	LocalSymbolPrivate *priv = LOCAL_SYMBOL_PRIVATE (object);
	if (!priv->my_cx || !priv->node)
		return NULL;
	return get_var_list(line, priv->my_cx);
}

static void
local_symbol_interface_init (IJsSymbolIface *iface)
{
	iface->get_arg_list = local_symbol_get_arg_list;
	iface->get_base_type = local_symbol_get_base_type;
	iface->get_func_ret_type = local_symbol_get_func_ret_type;

	iface->get_member = local_symbol_get_member;
	iface->get_name = local_symbol_get_name;
	iface->list_member = local_symbol_list_member;
}

static GList*
local_symbol_get_arg_list (IJsSymbol *obj)
{
	g_assert_not_reached ();
	return NULL;
}

static gint
local_symbol_get_base_type (IJsSymbol *obj)
{
	return BASE_CLASS;
}

static GList*
local_symbol_get_func_ret_type (IJsSymbol *obj)
{
	g_assert_not_reached ();
	return NULL;
}

static IJsSymbol*
local_symbol_get_member (IJsSymbol *obj, const gchar * name)
{
	LocalSymbol* self = LOCAL_SYMBOL (obj);
	LocalSymbolPrivate *priv = LOCAL_SYMBOL_PRIVATE (self);

	if (!priv->my_cx || !priv->node)
		return NULL;

	JSNode *node = js_context_get_last_assignment (priv->my_cx, name);
	if (node)
		return IJS_SYMBOL (node_symbol_new (node, name, priv->my_cx));

	return NULL;
}

static const gchar *
local_symbol_get_name (IJsSymbol *obj)
{
	LocalSymbol* self = LOCAL_SYMBOL (obj);
	LocalSymbolPrivate *priv = LOCAL_SYMBOL_PRIVATE (self);

	return g_strdup (priv->self_name);
}

static GList*
local_symbol_list_member (IJsSymbol *obj)
{
	LocalSymbol* self = LOCAL_SYMBOL (obj);
	LocalSymbolPrivate *priv = LOCAL_SYMBOL_PRIVATE (self);

	if (!priv->my_cx || !priv->node)
		return NULL;

	return get_var_list(0, priv->my_cx);
}

static GList*
get_var_list (gint lineno, JSContext *my_cx)
{
	GList *i, *ret = NULL;
	for (i = my_cx->local_var; i; i = g_list_next (i))
	{
		Var *t = (Var *)i->data;
		if (!t->name)
			continue;
		ret = g_list_append (ret, g_strdup (t->name));
	}
	for (i = g_list_last (my_cx->childs); i; i = g_list_previous (i))
	{
		JSContext *t = JS_CONTEXT (i->data);
		if (lineno)
			if (t->bline > lineno || t->eline + 2 < lineno)
				continue;
		ret = g_list_concat (ret, get_var_list (lineno, t));
	}
	if (my_cx->func_name && lineno != 0)
	{
		for (i = my_cx->func_arg; i; i = g_list_next (i))
			ret = g_list_append (ret, g_strdup ((gchar*)i->data));
	}
	return ret;
}
