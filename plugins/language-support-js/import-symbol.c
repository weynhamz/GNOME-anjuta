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
#include "import-symbol.h"
#include "util.h"
#include "gi-symbol.h"
#include "dir-symbol.h"

static void import_symbol_interface_init (IJsSymbolIface *iface);
static GList* import_symbol_get_arg_list (IJsSymbol *obj);
static gint import_symbol_get_base_type (IJsSymbol *obj);
static GList* import_symbol_get_func_ret_type (IJsSymbol *obj);
static IJsSymbol* import_symbol_get_member (IJsSymbol *obj, const gchar * name);
static const gchar * import_symbol_get_name (IJsSymbol *obj);
static GList* import_symbol_list_member (IJsSymbol *obj);

typedef struct _ImportSymbolPrivate ImportSymbolPrivate;
struct _ImportSymbolPrivate
{
	GList *member;
	GList *dirs;
};

#define IMPORT_SYMBOL_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), IMPORT_TYPE_SYMBOL, ImportSymbolPrivate))

G_DEFINE_TYPE_WITH_CODE (ImportSymbol, import_symbol, G_TYPE_OBJECT,
			G_IMPLEMENT_INTERFACE (IJS_TYPE_SYMBOL,
						import_symbol_interface_init));

static void
import_symbol_init (ImportSymbol *object)
{
	ImportSymbolPrivate *priv = IMPORT_SYMBOL_PRIVATE(object);
	priv->member = NULL;
	priv->dirs = NULL;
}

static void
import_symbol_finalize (GObject *object)
{
	ImportSymbolPrivate *priv = IMPORT_SYMBOL_PRIVATE(object);

	g_list_foreach (priv->member, (GFunc)g_object_unref, NULL);
	g_list_foreach (priv->dirs, (GFunc)g_object_unref, NULL);
	g_list_free (priv->member);
	g_list_free (priv->dirs);
	G_OBJECT_CLASS (import_symbol_parent_class)->finalize (object);
}

static void
import_symbol_class_init (ImportSymbolClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	/*GObjectClass* parent_class = G_OBJECT_CLASS (klass);*/

	g_type_class_add_private (klass, sizeof (ImportSymbolPrivate));

	object_class->finalize = import_symbol_finalize;
}


ImportSymbol*
import_symbol_new ()
{
	ImportSymbol* ret = IMPORT_SYMBOL (g_object_new (IMPORT_TYPE_SYMBOL, NULL));
	ImportSymbolPrivate *priv = IMPORT_SYMBOL_PRIVATE(ret);

	priv->member = g_list_append (NULL, gi_symbol_new ());
	priv->dirs = NULL;

	return ret;
}

static void
import_symbol_interface_init (IJsSymbolIface *iface)
{
	iface->get_arg_list = import_symbol_get_arg_list;
	iface->get_base_type = import_symbol_get_base_type;
	iface->get_func_ret_type = import_symbol_get_func_ret_type;

	iface->get_member = import_symbol_get_member;
	iface->get_name = import_symbol_get_name;
	iface->list_member = import_symbol_list_member;
}

static void
post_init (ImportSymbol *symbol)
{
	ImportSymbolPrivate *priv = IMPORT_SYMBOL_PRIVATE (symbol);
	GList *paths = get_import_include_paths ();

	GList *i;
	for (i = priv->dirs; i; )
	{
		GList *k;
		gchar *path = dir_symbol_get_path (DIR_SYMBOL (i->data));
		gboolean has_path = FALSE;
		g_assert (path != NULL);
		for (k = paths; k; k = g_list_next (k))
		{
			if (g_strcmp0 (path, (gchar*)k->data) != 0)
				continue;
			has_path = TRUE;
			paths = g_list_delete_link (paths, k);
			break;
		}
		if (!has_path)
		{
			k = g_list_next (i);
			g_object_unref (i->data);
			priv->dirs = g_list_remove_link (priv->dirs, i);
			i = k;
		}
		else
			i = g_list_next (i);
		g_free (path);
	}
	for (i = paths; i; i = g_list_next (i))
	{
		gchar *path = (gchar*)i->data;
		g_assert (path != NULL);
		DirSymbol *sym = dir_symbol_new (path);
		if (sym)
			priv->dirs = g_list_append (priv->dirs, sym);
	}
	g_list_foreach (paths, (GFunc)g_free, NULL);
	g_list_free (paths);
}

static GList*
import_symbol_get_arg_list (IJsSymbol *obj)
{
	g_assert_not_reached ();
	return NULL;
}

static gint
import_symbol_get_base_type (IJsSymbol *obj)
{
	return BASE_CLASS;
}

static GList*
import_symbol_get_func_ret_type (IJsSymbol *obj)
{
	g_assert_not_reached ();
	return NULL;
}

static IJsSymbol*
import_symbol_get_member (IJsSymbol *obj, const gchar * name)
{
	ImportSymbol* self = IMPORT_SYMBOL (obj);
	ImportSymbolPrivate *priv = IMPORT_SYMBOL_PRIVATE(self);

	GList *i;

	post_init (self);

	for (i = priv->member; i; i = g_list_next (i))
	{
		IJsSymbol *t = IJS_SYMBOL (i->data);
		if (g_strcmp0 (name, ijs_symbol_get_name (t)) == 0 )
		{
			g_object_ref (t);
			return t;
		}
	}
	for (i = priv->dirs; i; i = g_list_next (i))
	{
		IJsSymbol *t = IJS_SYMBOL (i->data);
		IJsSymbol* ret = ijs_symbol_get_member (t, name);
		if (!ret)
			continue;
		g_object_ref (ret);
		return ret;
	}
	return NULL;
}

static const gchar *
import_symbol_get_name (IJsSymbol *obj)
{
	return "imports";
}

static GList*
import_symbol_list_member (IJsSymbol *obj)
{
	ImportSymbol* self = IMPORT_SYMBOL (obj);
	ImportSymbolPrivate *priv = IMPORT_SYMBOL_PRIVATE(self);

	GList *i;
	GList *ret = NULL;

	post_init (self);

	for (i = priv->member; i; i = g_list_next (i))
	{
		IJsSymbol *t = IJS_SYMBOL (i->data);
		ret = g_list_append (ret, g_strdup (ijs_symbol_get_name (t)));
	}
	for (i = priv->dirs; i; i = g_list_next (i))
	{
		IJsSymbol *t = IJS_SYMBOL (i->data);
		ret = g_list_concat (ret, ijs_symbol_list_member (t));
	}
	return ret;
}
