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

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h>

#include "gir-symbol.h"
#include "ijs-symbol.h"
#include "simple-symbol.h"
#include "util.h"

static void gir_symbol_interface_init (IJsSymbolIface *iface);
static GList* gir_symbol_get_arg_list (IJsSymbol *obj);
static gint gir_symbol_get_base_type (IJsSymbol *obj);
static GList* gir_symbol_get_func_ret_type (IJsSymbol *obj);
static IJsSymbol* gir_symbol_get_member (IJsSymbol *obj, const gchar * name);
static const gchar * gir_symbol_get_name (IJsSymbol *obj);
static GList* gir_symbol_list_member (IJsSymbol *obj);

G_DEFINE_TYPE_WITH_CODE (GirSymbol, gir_symbol, G_TYPE_OBJECT,
					G_IMPLEMENT_INTERFACE (IJS_TYPE_SYMBOL,
						gir_symbol_interface_init));

typedef struct _GirSymbolPrivate GirSymbolPrivate;
struct _GirSymbolPrivate
{
	GList *member;
	gchar *name;
};

static IJsSymbol* parse_node (xmlNode *node);

#define GIR_SYMBOL_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), GIR_TYPE_SYMBOL, GirSymbolPrivate))

static void
gir_symbol_init (GirSymbol *object)
{
	GirSymbolPrivate *priv = GIR_SYMBOL_PRIVATE (object);
	priv->name = NULL;
	priv->member = NULL;
}

static void
gir_symbol_finalize (GObject *object)
{
	GirSymbolPrivate *priv = GIR_SYMBOL_PRIVATE (object);

	g_free (priv->name);
	g_list_foreach (priv->member, (GFunc)g_object_unref, NULL);
	g_list_free (priv->member);
	G_OBJECT_CLASS (gir_symbol_parent_class)->finalize (object);
}

static void
gir_symbol_class_init (GirSymbolClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	/*GObjectClass* parent_class = G_OBJECT_CLASS (klass);*/

	g_type_class_add_private (klass, sizeof (GirSymbolPrivate));

	object_class->finalize = gir_symbol_finalize;
}

static gchar *cur_gir = NULL;

IJsSymbol*
gir_symbol_new (const gchar *filename, const gchar *lib_name)
{
	GirSymbol* symbol = g_object_new (GIR_TYPE_SYMBOL, NULL);
	GirSymbolPrivate *priv = GIR_SYMBOL_PRIVATE (symbol);

	g_assert (lib_name != NULL);

	if (!lib_name)
	{
		g_object_unref (symbol);
		return NULL;
	}
	priv->member = NULL;
	priv->name = g_strdup (lib_name);

	cur_gir = g_strdup_printf ("imports.gi.%s.", priv->name);
	if (!g_file_test (filename, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
	{
		g_object_unref (symbol);
		return NULL;
	}
	xmlDocPtr doc = xmlParseFile(filename);
	xmlNode *root;
	xmlNode *i;

	if (doc == NULL) {
		g_warning ("could not parse file");
		g_object_unref (symbol);
		return NULL;
	}
	root = xmlDocGetRootElement (doc);
	for (i = root->children; i; i = i->next)
	{
		xmlNode *j;
		if (!i->name)
			continue;
		if (g_strcmp0 ((const char*)i->name, "namespace") !=0)
			continue;
		for (j = i->children; j; j = j->next)
		{
			IJsSymbol *n = parse_node (j);
			if (!n)
				continue;
			priv->member = g_list_append (priv->member, n);
		}
	}
	xmlFreeDoc (doc);
	return IJS_SYMBOL (symbol);
}

static void
gir_symbol_interface_init (IJsSymbolIface *iface)
{
	iface->get_arg_list = gir_symbol_get_arg_list;
	iface->get_base_type = gir_symbol_get_base_type;
	iface->get_func_ret_type = gir_symbol_get_func_ret_type;
	iface->get_member = gir_symbol_get_member;
	iface->get_name = gir_symbol_get_name;
	iface->list_member = gir_symbol_list_member;
}

static GList*
gir_symbol_get_arg_list (IJsSymbol *obj)
{
	g_assert_not_reached ();
	return NULL;
}

static gint
gir_symbol_get_base_type (IJsSymbol *obj)
{
	return BASE_CLASS;
}

static GList*
gir_symbol_get_func_ret_type (IJsSymbol *obj)
{
	g_assert_not_reached ();
	return NULL;
}

static IJsSymbol*
gir_symbol_get_member (IJsSymbol *obj, const gchar * name)
{
	GList *i;
	GirSymbolPrivate *priv = GIR_SYMBOL_PRIVATE (obj);

	for (i = priv->member; i; i = g_list_next (i))
	{
		IJsSymbol* t = IJS_SYMBOL (i->data);
		if (g_strcmp0 (name, ijs_symbol_get_name (t)) == 0)
		{
			g_object_ref (t);
			return t;
		}
	}
	return NULL;
}

static const gchar *
gir_symbol_get_name (IJsSymbol *obj)
{
	GirSymbol* symbol = GIR_SYMBOL (obj);
	GirSymbolPrivate *priv = GIR_SYMBOL_PRIVATE (symbol);
	return priv->name;
}

static GList*
gir_symbol_list_member (IJsSymbol *obj)
{
	GList *i;
	GList *ret = NULL;
	GirSymbolPrivate *priv = GIR_SYMBOL_PRIVATE (obj);

	for (i = priv->member; i; i = g_list_next (i))
	{
		IJsSymbol* t = IJS_SYMBOL (i->data);
		ret = g_list_append (ret, g_strdup (ijs_symbol_get_name (t)));
	}
	return ret;
}

static IJsSymbol*
parse_class (xmlNode *node)
{
	SimpleSymbol *ret = NULL;
	xmlNode *i;
	gchar *name;

	name = (gchar*)xmlGetProp (node, (const xmlChar*)"name");
	if (!name)
		return NULL;

	ret = simple_symbol_new ();
	ret->name = name;
//puts (name);
	for (i = node->children; i; i = i->next)
	{
		IJsSymbol *child = parse_node (i);
		if (!child)
			continue;
		ret->member = g_list_append (ret->member, child);
	}
	return IJS_SYMBOL (ret);
}

static IJsSymbol*
parse_enumeration (xmlNode *node)
{
	SimpleSymbol *ret = NULL;
	xmlNode *i;
	gchar *name;

	name = (gchar*)xmlGetProp (node, (const xmlChar*)"name");
	if (!name)
		return NULL;
	ret = simple_symbol_new ();
	ret->name = name;
	ret->type = BASE_ENUM;

	for (i = node->children; i; i = i->next)
	{
		gchar *name = (gchar*)xmlGetProp (i, (const xmlChar*)"name");
		if (!name)
			continue;
		SimpleSymbol *t = simple_symbol_new ();
		t->name = name;
		ret->member = g_list_append (ret->member, t);
	}
	return IJS_SYMBOL (ret);
}

static IJsSymbol*
parse_function (xmlNode *node)
{
	SimpleSymbol *ret = NULL;
	xmlNode *i, *k;
	gchar *name;

	name = (gchar*)xmlGetProp (node, (xmlChar*)"name");
	if (!name)
		return NULL;

	ret = simple_symbol_new ();
	ret->name = name;
	ret->type = BASE_FUNC;

	for (i = node->children; i; i = i->next)
	{
		if (!i->name)
			continue;
		if (strcmp ((const gchar*)i->name, "return-value") == 0)
		{
			for (k = i->children; k; k = k->next)
			{
				xmlChar *tmp;
				if (!k->name)
					continue;
				tmp = xmlGetProp (k, (const xmlChar*)"name");
				if (!tmp)
					continue;
				ret->ret_type = g_list_append (ret->ret_type, g_strdup_printf ("%s%s", cur_gir, (gchar*)tmp));/*TODO:Fix type*/
				xmlFree (tmp);
			}
		}
		if (strcmp ((const gchar*)i->name, "parameters") == 0)
		{
			for (k = i->children; k; k = k->next)
			{
				gchar *name;
				Argument *tmp;
				if (!k->name)
					continue;
				name = (gchar*)xmlGetProp (node, (const xmlChar*)"name");
				if (!name)
					continue;
				tmp = g_new (Argument, 1);
				tmp->name = name;
				tmp->types = NULL;
				ret->args = g_list_append (ret->args, tmp);
			}
		}
	}
	return IJS_SYMBOL (ret);
}

static IJsSymbol*
parse_node (xmlNode *node)
{
	if (!node || !node->name)
		return NULL;
	if (strcmp ((const gchar*)node->name, "text") == 0
			|| strcmp ((const gchar*)node->name, "implements") == 0)
		return NULL;
	if (strcmp ((const gchar*)node->name, "namespace") == 0 || strcmp ((const gchar*)node->name, "class") == 0
			|| strcmp ((const gchar*)node->name, "record") == 0
			|| strcmp ((const gchar*)node->name, "bitfield") == 0
			|| strcmp ((const gchar*)node->name, "interface") == 0)
	{
		return parse_class (node);
	}
	if (strcmp ((const gchar*)node->name, "union") == 0)
	{
		return parse_class (node);
	}
	if (strcmp ((const gchar*)node->name, "function") == 0 || strcmp ((const gchar*)node->name, "method") == 0
			|| strcmp ((const gchar*)node->name, "callback") == 0
			|| strcmp ((const gchar*)node->name, "constructor") == 0)
	{
		return parse_function (node);
	}
	if (strcmp ((const gchar*)node->name, "alias") == 0)
	{
		SimpleSymbol *self = NULL;
		gchar *name = (gchar*)xmlGetProp (node, (const xmlChar*)"name");
		if (!name)
			return NULL;
		self = simple_symbol_new ();
		self->name = name;
		return IJS_SYMBOL (self);
	}
	if (strcmp ((const gchar*)node->name, "constant") == 0 ||
			strcmp ((const gchar*)node->name, "signal") == 0 ||
			strcmp ((const gchar*)node->name, "field") == 0 ||
			strcmp ((const gchar*)node->name, "property") == 0 ||
			strcmp ((const gchar*)node->name, "member") == 0)
	{
		SimpleSymbol *ret = NULL;
		gchar *name = (gchar*)xmlGetProp (node, (const xmlChar*)"name");
		if (!name)
			return NULL;
		ret = simple_symbol_new ();
		ret->name = name;
		return IJS_SYMBOL (ret);
	}
	if (strcmp ((const gchar*)node->name, "enumeration") == 0)
	{
		return parse_enumeration (node);
	}
	puts ((const gchar*)node->name);
//	g_assert_not_reached ();
	return NULL;
}
