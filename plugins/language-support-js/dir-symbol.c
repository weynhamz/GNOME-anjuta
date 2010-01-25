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
#include <string.h>

#include "local-symbol.h"
#include "db-anjuta-symbol.h"
#include "dir-symbol.h"
#include "util.h"

typedef struct _DirSymbolPrivate DirSymbolPrivate;
struct _DirSymbolPrivate
{
	GFile *self_dir;
};

#define DIR_SYMBOL_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DIR_TYPE_SYMBOL, DirSymbolPrivate))

static void dir_symbol_interface_init (IJsSymbolIface *iface);
static GList* dir_symbol_get_arg_list (IJsSymbol *obj);
static gint dir_symbol_get_base_type (IJsSymbol *obj);
static GList* dir_symbol_get_func_ret_type (IJsSymbol *obj);
static IJsSymbol* dir_symbol_get_member (IJsSymbol *obj, const gchar * name);
static const gchar * dir_symbol_get_name (IJsSymbol *obj);
static GList* dir_symbol_list_member (IJsSymbol *obj);

G_DEFINE_TYPE_WITH_CODE (DirSymbol, dir_symbol, G_TYPE_OBJECT,
			G_IMPLEMENT_INTERFACE (IJS_TYPE_SYMBOL,
						dir_symbol_interface_init));

static void
dir_symbol_init (DirSymbol *object)
{
	DirSymbolPrivate* priv = DIR_SYMBOL_PRIVATE (object);
	priv->self_dir = NULL;
}

static void
dir_symbol_finalize (GObject *object)
{
	DirSymbolPrivate* priv = DIR_SYMBOL_PRIVATE (object);

	g_object_unref (priv->self_dir);

	G_OBJECT_CLASS (dir_symbol_parent_class)->finalize (object);
}

static void
dir_symbol_class_init (DirSymbolClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	/*GObjectClass* parent_class = G_OBJECT_CLASS (klass);*/

	g_type_class_add_private (klass, sizeof (DirSymbolPrivate));

	object_class->finalize = dir_symbol_finalize;
}

gchar*
dir_symbol_get_path (DirSymbol* self)
{
	g_assert (DIR_IS_SYMBOL (self));
	DirSymbolPrivate* priv = DIR_SYMBOL_PRIVATE (self);
	g_assert (priv->self_dir != NULL);
	return g_file_get_path (priv->self_dir);
}

DirSymbol*
dir_symbol_new (const gchar* dirname)
{
	DirSymbol* self = DIR_SYMBOL (g_object_new (DIR_TYPE_SYMBOL, NULL));
	DirSymbolPrivate* priv = DIR_SYMBOL_PRIVATE (self);

	g_assert (dirname != NULL);

	if (!g_file_test (dirname, G_FILE_TEST_IS_DIR))
	{
		g_object_unref (self);
		return NULL;
	}

	priv->self_dir = g_file_new_for_path (dirname);
	gchar *tmp = g_file_get_basename (priv->self_dir);

	if (!tmp || tmp[0] == '.')
	{
		g_free (tmp);
		g_object_unref (self);
		return NULL;
	}
	g_free (tmp);

	GFileInfo *info;
	GFileEnumerator *enumerator = g_file_enumerate_children (priv->self_dir, G_FILE_ATTRIBUTE_STANDARD_NAME, G_FILE_QUERY_INFO_NONE, NULL, NULL);

	if (enumerator == NULL)
	{
		g_object_unref (self);
		return NULL;
	}
	gboolean has_child = FALSE;
	for (info = g_file_enumerator_next_file (enumerator, NULL, NULL); info; info = g_file_enumerator_next_file (enumerator, NULL, NULL))
	{
		const gchar *name = g_file_info_get_name (info);
		if (!name)
		{
			g_object_unref (info);
			continue;
		}
		GFile *file = g_file_get_child (priv->self_dir, name);
		gchar *file_path = g_file_get_path (file);
		g_object_unref (file);
		if (g_file_test (file_path, G_FILE_TEST_IS_DIR))
		{
			DirSymbol *t = dir_symbol_new (file_path);
			g_free (file_path);
			g_object_unref (info);
			if (t)
			{
				g_object_unref (t);
				has_child = TRUE;
				break;
			}
			continue;
		}
		g_free (file_path);
		size_t len = strlen (name);
		if (len <= 3 || strcmp (name + len - 3, ".js") != 0)
		{
			g_object_unref (info);
			continue;
		}
		g_object_unref (info);
		has_child = TRUE;
	}
	g_object_unref (enumerator);
	if (!has_child)
	{
		g_object_unref (self);
		return NULL;
	}

	return self;
}

static void
dir_symbol_interface_init (IJsSymbolIface *iface)
{
	iface->get_arg_list = dir_symbol_get_arg_list;
	iface->get_base_type = dir_symbol_get_base_type;
	iface->get_func_ret_type = dir_symbol_get_func_ret_type;

	iface->get_member = dir_symbol_get_member;
	iface->get_name = dir_symbol_get_name;
	iface->list_member = dir_symbol_list_member;
}

static GList*
dir_symbol_get_arg_list (IJsSymbol *obj)
{
	g_assert_not_reached ();
	return NULL;
}

static gint
dir_symbol_get_base_type (IJsSymbol *obj)
{
	return BASE_CLASS;
}

static GList*
dir_symbol_get_func_ret_type (IJsSymbol *obj)
{
	g_assert_not_reached ();
	return NULL;
}

static IJsSymbol*
dir_symbol_get_member (IJsSymbol *obj, const gchar * name)
{
	DirSymbol* self = DIR_SYMBOL (obj);
	DirSymbolPrivate* priv = DIR_SYMBOL_PRIVATE (self);

	g_assert (name != NULL);
	
	GFile *file = g_file_get_child (priv->self_dir, name);
	gchar *path = g_file_get_path (file);
	g_object_unref (file);

	if (g_file_test (path, G_FILE_TEST_IS_DIR))
	{
		IJsSymbol *ret = IJS_SYMBOL (dir_symbol_new (path));
		g_free (path);
		return ret;
	}
	g_free (path);

	gchar *namejs = g_strconcat (name, ".js", NULL);
	file = g_file_get_child (priv->self_dir, namejs);
	g_free (namejs);
	path = g_file_get_path (file);
	g_object_unref (file);

	if (g_file_test (path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
	{
		IJsSymbol *ret = IJS_SYMBOL (db_anjuta_symbol_new (path));
		if (!ret)
			ret = IJS_SYMBOL (local_symbol_new (path));
		g_free (path);
		return ret;
	}
	g_free (path);
	return NULL;
}

static const gchar *
dir_symbol_get_name (IJsSymbol *obj)
{
	DirSymbol* self = DIR_SYMBOL (obj);
	DirSymbolPrivate* priv = DIR_SYMBOL_PRIVATE (self);

	return g_file_get_basename (priv->self_dir);
}

static GList*
dir_symbol_list_member (IJsSymbol *obj)
{
	DirSymbol* self = DIR_SYMBOL (obj);
	DirSymbolPrivate* priv = DIR_SYMBOL_PRIVATE (self);
	GList *ret = NULL;

	GFileInfo *info;

	GFileEnumerator *enumerator = g_file_enumerate_children (priv->self_dir, G_FILE_ATTRIBUTE_STANDARD_NAME, G_FILE_QUERY_INFO_NONE, NULL, NULL);

	if (enumerator == NULL)
		return NULL;

	for (info = g_file_enumerator_next_file (enumerator, NULL, NULL); info; info = g_file_enumerator_next_file (enumerator, NULL, NULL))
	{
		const gchar *name = g_file_info_get_name (info);
		if (!name)
		{
			g_object_unref (info);
			continue;
		}

		GFile *file = g_file_get_child (priv->self_dir, name);
		gchar *path = g_file_get_path (file);
		g_object_unref (file);

		if (g_file_test (path, G_FILE_TEST_IS_DIR))
		{
			DirSymbol *t = dir_symbol_new (path);
			g_free (path);
			if (t)
			{
				g_object_unref (t);
				ret = g_list_append (ret, g_strdup (name));
			}
			g_object_unref (info);
			continue;
		}
		size_t len = strlen (name);
		if (len <= 3 || strcmp (name + len - 3, ".js") != 0)
		{
			g_object_unref (info);
			continue;
		}
		gchar *t = g_strdup (name);
		g_object_unref (info);
		*(t + len - 3) = '\0';
		ret = g_list_append (ret, t);
	}

	return ret;
}
