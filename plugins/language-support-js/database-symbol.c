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

#include "local-symbol.h"
#include "std-symbol.h"
#include "import-symbol.h"
#include "database-symbol.h"
#include "util.h"

static void database_symbol_interface_init (IJsSymbolIface *iface);
static GList* database_symbol_get_arg_list (IJsSymbol *obj);
static gint database_symbol_get_base_type (IJsSymbol *obj);
static GList* database_symbol_get_func_ret_type (IJsSymbol *obj);
static IJsSymbol* database_symbol_get_member (IJsSymbol *obj, const gchar * name);
static const gchar * database_symbol_get_name (IJsSymbol *obj);
static GList* database_symbol_list_member (IJsSymbol *obj);

static IJsSymbol* find (const gchar* name, IJsSymbol *sym);

typedef struct _DatabaseSymbolPrivate DatabaseSymbolPrivate;
struct _DatabaseSymbolPrivate
{
	GList *symbols;
	LocalSymbol *local;
	StdSymbol *global;
};

#define DATABASE_SYMBOL_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DATABASE_TYPE_SYMBOL, DatabaseSymbolPrivate))

G_DEFINE_TYPE_WITH_CODE (DatabaseSymbol, database_symbol, G_TYPE_OBJECT,
			G_IMPLEMENT_INTERFACE (IJS_TYPE_SYMBOL,
						database_symbol_interface_init));

static void
database_symbol_init (DatabaseSymbol *object)
{
	DatabaseSymbolPrivate *priv = DATABASE_SYMBOL_PRIVATE(object);
	priv->symbols = NULL;
	priv->local = NULL;
	priv->global = NULL;
}

static void
database_symbol_finalize (GObject *object)
{
	DatabaseSymbolPrivate *priv = DATABASE_SYMBOL_PRIVATE(object);

	g_object_unref (priv->local);
	g_object_unref (priv->global);
	g_list_foreach (priv->symbols, (GFunc)g_object_unref, NULL);
	g_list_free (priv->symbols);
	G_OBJECT_CLASS (database_symbol_parent_class)->finalize (object);
}

static void
database_symbol_class_init (DatabaseSymbolClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
/*	GObjectClass* parent_class = G_OBJECT_CLASS (klass);*/

	g_type_class_add_private (klass, sizeof (DatabaseSymbolPrivate));

	object_class->finalize = database_symbol_finalize;
}

void
database_symbol_set_file (DatabaseSymbol *object, const gchar* filename)
{
	GList *missed;
	g_assert (DATABASE_IS_SYMBOL (object));
	DatabaseSymbolPrivate *priv = DATABASE_SYMBOL_PRIVATE (object);

	if (priv->local)
	{
		g_object_unref (priv->local);
	}

	priv->local = local_symbol_new (filename);
	missed = local_symbol_get_missed_semicolons (priv->local);
	highlight_lines (missed);
}

DatabaseSymbol*
database_symbol_new ()
{
	DatabaseSymbol* self = DATABASE_SYMBOL (g_object_new (DATABASE_TYPE_SYMBOL, NULL));
	DatabaseSymbolPrivate *priv = DATABASE_SYMBOL_PRIVATE (self);

	priv->global = std_symbol_new ();
	priv->local = NULL;
	priv->symbols = g_list_append (NULL, import_symbol_new ());

	return self;
}

static void
database_symbol_interface_init (IJsSymbolIface *iface)
{
	iface->get_arg_list = database_symbol_get_arg_list;
	iface->get_base_type = database_symbol_get_base_type;
	iface->get_func_ret_type = database_symbol_get_func_ret_type;

	iface->get_member = database_symbol_get_member;
	iface->get_name = database_symbol_get_name;
	iface->list_member = database_symbol_list_member;
}

static GList*
database_symbol_get_arg_list (IJsSymbol *obj)
{
	g_assert_not_reached ();
	return NULL;
}

static gint
database_symbol_get_base_type (IJsSymbol *obj)
{
	return BASE_CLASS;
}

static GList*
database_symbol_get_func_ret_type (IJsSymbol *obj)
{
	g_assert_not_reached ();
	return NULL;
}

static IJsSymbol*
database_symbol_get_member (IJsSymbol *obj, const gchar * name)
{
	DatabaseSymbol* self = DATABASE_SYMBOL (obj);
	DatabaseSymbolPrivate *priv = DATABASE_SYMBOL_PRIVATE (self);

	if (!name || strlen (name) == 0)
	{
		g_object_ref (priv->local);
		return IJS_SYMBOL (priv->local);
	}

	GList *i;
	for (i = priv->symbols; i; i = g_list_next (i))
	{
		IJsSymbol *t = IJS_SYMBOL (i->data);
		if (strncmp (name, ijs_symbol_get_name (t), strlen (ijs_symbol_get_name (t))) != 0 )
			continue;
		if (strlen (name + strlen (ijs_symbol_get_name (t))) == 0)
		{
			g_object_ref (t);
			return t;
		}
		return find (name + strlen (ijs_symbol_get_name (t)) + 1, t);
	}

	IJsSymbol *ret = find (name, IJS_SYMBOL (priv->local));
	if (!ret)
		ret = find (name, IJS_SYMBOL (priv->global));

	return ret;
}

static const gchar *
database_symbol_get_name (IJsSymbol *obj)
{
	g_assert_not_reached ();
	return NULL;
}

static GList*
database_symbol_list_member (IJsSymbol *obj)
{
	DatabaseSymbol* self = DATABASE_SYMBOL (obj);
	DatabaseSymbolPrivate *priv = DATABASE_SYMBOL_PRIVATE (self);

	GList *ret = NULL;
	ret = ijs_symbol_list_member (IJS_SYMBOL (priv->global));
	if (priv->local)
		ret = g_list_concat (ret, ijs_symbol_list_member (IJS_SYMBOL (priv->local)));
	ret = g_list_append (ret, g_strdup ("imports"));
	return ret;
}

GList*
database_symbol_list_local_member (DatabaseSymbol *object, gint line)
{
	DatabaseSymbolPrivate *priv = DATABASE_SYMBOL_PRIVATE (object);
	return local_symbol_list_member_with_line (priv->local, line);
}

GList*
database_symbol_list_member_with_line (DatabaseSymbol *object, gint line)
{
	g_assert (DATABASE_IS_SYMBOL (object));
	DatabaseSymbolPrivate *priv = DATABASE_SYMBOL_PRIVATE (object);

	GList *ret = NULL;
	ret = ijs_symbol_list_member (IJS_SYMBOL (priv->global));
	if (priv->local)
		ret = g_list_concat (ret, local_symbol_list_member_with_line (priv->local, line));
	ret = g_list_append (ret, g_strdup ("imports"));
	return ret;
}

static IJsSymbol*
find (const gchar* name, IJsSymbol *sym)
{
	gchar *vname = NULL, *left = NULL;
	if (!sym)
		return NULL;
	if (!name)
		return NULL;
	int i;
	for (i = 0; i < strlen (name); i++)
	{
		if (name[i] != '.')
			continue;
		vname = g_strndup (name, i);
		left = g_strdup (name + i + 1);
		break;
	}
	if (!vname)
		vname = g_strdup (name);
	if (strlen (vname) == 0)
	{
		g_free (vname);
		g_free (left);
		return NULL;
	}
	gboolean is_func_call = *((vname + strlen (vname)) - 1) == ')';
	if (is_func_call)
		vname [strlen (vname) - 2] = '\0';
	GList *j;////TODO
	for (j = ijs_symbol_list_member (sym); j; j = g_list_next (j))
	{
		gchar *t = (gchar*)j->data;
//puts (t);
		if (strcmp (vname, t) != 0 )
			continue;
		if (!is_func_call)
		{
			if (!left)
				return ijs_symbol_get_member (sym, t);
			IJsSymbol *tjs = ijs_symbol_get_member (sym, t);
			IJsSymbol *ret = find (left, tjs);
			g_object_unref (tjs);
			return ret;
		}
		else
		{
			IJsSymbol *s = ijs_symbol_get_member (sym, t);
			if (!s)
				return NULL;
			if (ijs_symbol_get_base_type (s) != BASE_FUNC)
			{
				g_object_unref (s);
				return NULL;
			}
			GList* rets = ijs_symbol_get_func_ret_type (s);
			g_object_unref (s);
			if (rets == NULL)
			{
				return NULL;
			}
			//TODO: Fix
			IJsSymbol *ret = global_search (rets->data);
			if (!ret)
				return NULL;
			if (!left)
				return ret;
			s = find (left, ret);
			g_object_unref (ret);
			return s;
		}
	}
	return NULL;
}
