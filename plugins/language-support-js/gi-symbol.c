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

#include <gio/gio.h>

#include "gi-symbol.h"
#include "util.h"
#include "gir-symbol.h"
#include "string.h"

static void gi_symbol_interface_init (IJsSymbolIface *iface);
static GList* gi_symbol_get_arg_list (IJsSymbol *obj);
static gint gi_symbol_get_base_type (IJsSymbol *obj);
static GList* gi_symbol_get_func_ret_type (IJsSymbol *obj);
static IJsSymbol* gi_symbol_get_member (IJsSymbol *obj, const gchar * name);
static const gchar * gi_symbol_get_name (IJsSymbol *obj);
static GList* gi_symbol_list_member (IJsSymbol *obj);

G_DEFINE_TYPE_WITH_CODE (GiSymbol, gi_symbol, G_TYPE_OBJECT,
			G_IMPLEMENT_INTERFACE (IJS_TYPE_SYMBOL,
						gi_symbol_interface_init));

typedef struct _GiSymbolPrivate GiSymbolPrivate;
struct _GiSymbolPrivate
{
	GList *member;
};

#define GI_SYMBOL_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), GI_TYPE_SYMBOL, GiSymbolPrivate))

static void
gi_symbol_init (GiSymbol *object)
{
	GiSymbolPrivate *priv = GI_SYMBOL_PRIVATE(object);
	priv->member = NULL;
}

static void
gi_symbol_finalize (GObject *object)
{
	GiSymbolPrivate *priv = GI_SYMBOL_PRIVATE(object);

	g_list_foreach (priv->member, (GFunc)g_object_unref, NULL);
	g_list_free (priv->member);
	G_OBJECT_CLASS (gi_symbol_parent_class)->finalize (object);
}

static void
gi_symbol_class_init (GiSymbolClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	/*GObjectClass* parent_class = G_OBJECT_CLASS (klass);*/

	g_type_class_add_private (klass, sizeof (GiSymbolPrivate));

	object_class->finalize = gi_symbol_finalize;
}


IJsSymbol*
gi_symbol_new ()
{
	return g_object_new (GI_TYPE_SYMBOL, NULL);
}

static void
gi_symbol_interface_init (IJsSymbolIface *iface)
{
	iface->get_arg_list = gi_symbol_get_arg_list;
	iface->get_base_type = gi_symbol_get_base_type;
	iface->get_func_ret_type = gi_symbol_get_func_ret_type;

	iface->get_member = gi_symbol_get_member;
	iface->get_name = gi_symbol_get_name;
	iface->list_member = gi_symbol_list_member;
}

static GList*
gi_symbol_get_arg_list (IJsSymbol *obj)
{
	g_assert_not_reached ();
	return NULL;
}

static gint
gi_symbol_get_base_type (IJsSymbol *obj)
{
	return BASE_CLASS;
}

static GList*
gi_symbol_get_func_ret_type (IJsSymbol *obj)
{
	g_assert_not_reached ();
	return NULL;
}

static IJsSymbol*
gi_symbol_get_member (IJsSymbol *obj, const gchar * name)
{
	GList *i;

	GiSymbol *object = GI_SYMBOL (obj);
	GiSymbolPrivate *priv = GI_SYMBOL_PRIVATE (object);

	g_assert (object != NULL);
	g_assert (priv != NULL);

	g_assert (name != NULL);

	if (!name)
		return NULL;
	for (i = priv->member; i; i = g_list_next (i))
	{
		IJsSymbol *lib = IJS_SYMBOL (i->data);
		if (g_strcmp0 (name, ijs_symbol_get_name (lib)) == 0)
		{
			g_object_ref (lib);
			return lib;
		}
	}

	GFileInfo *info;
	const gchar *lib_name = name;
	gchar *gir_path = get_gir_path ();
	g_assert (gir_path);
	GFile *dir = g_file_new_for_path (gir_path);
	GFileEnumerator *enumerator = g_file_enumerate_children (dir, G_FILE_ATTRIBUTE_STANDARD_NAME, G_FILE_QUERY_INFO_NONE, NULL, NULL);
	g_free (gir_path);
	if (enumerator)
	{
		for (info = g_file_enumerator_next_file(enumerator, NULL, NULL); info; info = g_file_enumerator_next_file(enumerator, NULL, NULL))
		{
			const gchar *name = g_file_info_get_name (info);
			if (!name)
			{
				g_object_unref (info);		
				continue;
			}
			if (strncmp (name, lib_name, strlen (lib_name)) == 0)
			{
				GFile *file = g_file_get_child (dir, name);
				gchar *path = g_file_get_path (file);
				IJsSymbol *n = NULL;
				if (g_file_test (path, G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS))
					n = gir_symbol_new (path, lib_name);
				g_free (path);
				if (n)
				{
					priv->member = g_list_append (priv->member, n);
					g_object_ref (n);
				}
				g_object_unref (enumerator);
				return n;
			}
			g_object_unref (info);
		}
		g_object_unref (enumerator);
	}
	return NULL;
}

static const gchar *
gi_symbol_get_name (IJsSymbol *obj)
{
	return "gi";
}

static GList*
gi_symbol_list_member (IJsSymbol *obj)
{
	GList *ret = NULL;

	GFileInfo *info;
	gchar *gir_path = get_gir_path ();
	GFile *dir = g_file_new_for_path (gir_path);
	GFileEnumerator *enumerator = g_file_enumerate_children (dir, G_FILE_ATTRIBUTE_STANDARD_NAME, G_FILE_QUERY_INFO_NONE, NULL, NULL);

	g_free (gir_path);
	if (enumerator)
	{
		for (info = g_file_enumerator_next_file(enumerator, NULL, NULL); info; info = g_file_enumerator_next_file (enumerator, NULL, NULL))
		{
			int i;
			const gchar *name = g_file_info_get_name (info);
			if (!name)
			{
				g_object_unref (info);
				continue;
			}
			for (i = 0; i < strlen (name); i++)
			{
				if (name[i] != '-' && name[i] != '.')
					continue;
				if (i == 0)
					break;
				gchar *lib_name = g_strndup (name, i);
				ret = g_list_append (ret, lib_name);
				break;
			}
			g_object_unref (info);
		}
		g_object_unref (enumerator);
	}
	return ret;
}
