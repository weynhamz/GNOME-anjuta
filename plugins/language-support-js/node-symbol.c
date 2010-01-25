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

#include "node-symbol.h"
#include "ijs-symbol.h"
#include "util.h"

static void node_symbol_interface_init (IJsSymbolIface *iface);
static GList* node_symbol_get_arg_list (IJsSymbol *obj);
static gint node_symbol_get_base_type (IJsSymbol *obj);
static GList* node_symbol_get_func_ret_type (IJsSymbol *obj);
static IJsSymbol* node_symbol_get_member (IJsSymbol *obj, const gchar * name);
static const gchar * node_symbol_get_name (IJsSymbol *obj);
static GList* node_symbol_list_member (IJsSymbol *obj);

static gchar* get_complex_node_type (JSNode *node, JSContext *my_cx);

typedef struct _NodeSymbolPrivate NodeSymbolPrivate;
struct _NodeSymbolPrivate
{
	gchar * name;
	JSNode *node;
	JSContext *my_cx;
};

#define NODE_SYMBOL_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), NODE_TYPE_SYMBOL, NodeSymbolPrivate))

G_DEFINE_TYPE_WITH_CODE (NodeSymbol, node_symbol, G_TYPE_OBJECT,
			G_IMPLEMENT_INTERFACE (IJS_TYPE_SYMBOL,
						node_symbol_interface_init));

static void
node_symbol_init (NodeSymbol *object)
{
	NodeSymbolPrivate *priv = NODE_SYMBOL_PRIVATE(object);
	priv->name = NULL;
	priv->node = NULL;
	priv->my_cx = NULL;
}

static void
node_symbol_finalize (GObject *object)
{
	NodeSymbolPrivate *priv = NODE_SYMBOL_PRIVATE(object);

	g_free (priv->name);
	if (priv->node)
		g_object_unref (priv->node);
	if (priv->my_cx)
		g_object_unref (priv->my_cx);
	G_OBJECT_CLASS (node_symbol_parent_class)->finalize (object);
}

static void
node_symbol_class_init (NodeSymbolClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	/*GObjectClass* parent_class = G_OBJECT_CLASS (klass);*/

	g_type_class_add_private (klass, sizeof (NodeSymbolPrivate));

	object_class->finalize = node_symbol_finalize;
}


NodeSymbol*
node_symbol_new (JSNode *node, const gchar *name, JSContext *my_cx)
{
	NodeSymbol* ret = NODE_SYMBOL (g_object_new (NODE_TYPE_SYMBOL, NULL));
	NodeSymbolPrivate *priv = NODE_SYMBOL_PRIVATE(ret);

	g_return_val_if_fail (node != NULL && name != NULL && my_cx != NULL, NULL);

	priv->node = node;
	priv->name = g_strdup (name);
	priv->my_cx = my_cx;
	g_object_ref (node);
	g_object_ref (my_cx);
	return ret;
}

static void
node_symbol_interface_init (IJsSymbolIface *iface)
{
	iface->get_arg_list = node_symbol_get_arg_list;
	iface->get_base_type = node_symbol_get_base_type;
	iface->get_func_ret_type = node_symbol_get_func_ret_type;

	iface->get_member = node_symbol_get_member;
	iface->get_name = node_symbol_get_name;
	iface->list_member = node_symbol_list_member;
}

static GList*
node_symbol_get_arg_list (IJsSymbol *obj)
{
	NodeSymbol* self = NODE_SYMBOL (obj);
	NodeSymbolPrivate *priv = NODE_SYMBOL_PRIVATE(self);

	if ((JSNodeArity)priv->node->pn_arity != PN_FUNC)
	{
		g_assert_not_reached ();
		return NULL;
	}

	GList *ret = NULL;

	if (priv->node->pn_u.func.args)
	{
		JSNode *i;
		JSNode *args = priv->node->pn_u.func.args;

		g_assert (args->pn_arity == PN_LIST);

		for (i = args->pn_u.list.head; i; i = i->pn_next)
		{
			ret = g_list_append (ret, g_strdup (js_node_get_name (i)));
		}
	}

	return ret;
}

static gint
node_symbol_get_base_type (IJsSymbol *obj)
{
	NodeSymbol* self = NODE_SYMBOL (obj);
	NodeSymbolPrivate *priv = NODE_SYMBOL_PRIVATE(self);

	if ((JSNodeArity)priv->node->pn_arity == PN_FUNC)
		return BASE_FUNC;
	return BASE_CLASS;
}

static GList*
node_symbol_get_func_ret_type (IJsSymbol *obj)
{
	NodeSymbol* self = NODE_SYMBOL (obj);
	NodeSymbolPrivate *priv = NODE_SYMBOL_PRIVATE(self);

	if (priv->node->pn_arity != PN_FUNC)
		return NULL;
	return js_context_get_func_ret_type (priv->my_cx, priv->name);
}

static IJsSymbol*
node_symbol_get_member (IJsSymbol *obj, const gchar * name)
{
	NodeSymbol* self = NODE_SYMBOL (obj);
	NodeSymbolPrivate *priv = NODE_SYMBOL_PRIVATE(self);

	gchar *tname = get_complex_node_type (priv->node, priv->my_cx);
	if (!tname)
		return NULL;

	if (js_context_get_member_list (priv->my_cx, tname)) //TODO:Fix mem leak
	{
			return IJS_SYMBOL (
				node_symbol_new (js_context_get_member (priv->my_cx, tname, name),
									name, priv->my_cx));
	}
	IJsSymbol *t = global_search (tname);
	if (t)
		return ijs_symbol_get_member (t, name);

	return NULL;
}

static const gchar *
node_symbol_get_name (IJsSymbol *obj)
{
	NodeSymbol* self = NODE_SYMBOL (obj);
	NodeSymbolPrivate *priv = NODE_SYMBOL_PRIVATE(self);

	gchar *tname = get_complex_node_type (priv->node, priv->my_cx);
	if (tname)
		return tname;

	return g_strdup (priv->name);
}

static GList*
node_symbol_list_member (IJsSymbol *obj)
{
	NodeSymbol* self = NODE_SYMBOL (obj);
	NodeSymbolPrivate *priv = NODE_SYMBOL_PRIVATE(self);

	gchar *name = get_complex_node_type (priv->node, priv->my_cx);

	if (!name)
		return NULL;
	GList *t = js_context_get_member_list (priv->my_cx, name);
	if (t)
		return t;

	IJsSymbol *sym = global_search (name);
	if (sym)
		return ijs_symbol_list_member (sym);
	return NULL;
}

static gchar*
get_complex_node_type (JSNode *node, JSContext *my_cx)
{
	Type *name = js_context_get_node_type (my_cx, node);
	if (!name)
		return NULL;

	if (!name->isFuncCall)
		return name->name;

	IJsSymbol* sym = global_search (name->name);
	if (sym && ijs_symbol_get_base_type (sym) == BASE_FUNC)
	{
		GList *ret = ijs_symbol_get_func_ret_type (sym);
		if (ret)
		{
			g_assert (ret->data != NULL);
			return (gchar*)ret->data;
		}
	}
	return NULL;
}
