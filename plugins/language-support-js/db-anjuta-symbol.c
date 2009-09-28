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

#include <libanjuta/interfaces/ianjuta-symbol-manager.h>
#include <libanjuta/anjuta-plugin.h>

#include "db-anjuta-symbol.h"
#include "util.h"

static void db_anjuta_symbol_interface_init (IJsSymbolIface *iface);
static GList* db_anjuta_symbol_get_arg_list (IJsSymbol *obj);
static gint db_anjuta_symbol_get_base_type (IJsSymbol *obj);
static GList* db_anjuta_symbol_get_func_ret_type (IJsSymbol *obj);
static IJsSymbol* db_anjuta_symbol_get_member (IJsSymbol *obj, const gchar * name);
static const gchar * db_anjuta_symbol_get_name (IJsSymbol *obj);
static GList* db_anjuta_symbol_list_member (IJsSymbol *obj);

static DbAnjutaSymbol* db_anjuta_symbol_new_for_symbol (IAnjutaSymbolManager *obj, IAnjutaSymbol *self);


typedef struct _DbAnjutaSymbolPrivate DbAnjutaSymbolPrivate;
struct _DbAnjutaSymbolPrivate
{
	GFile *file;
	IAnjutaSymbolManager *obj;
	gchar *self_name;
	IAnjutaSymbol *self;
};

#define DB_ANJUTA_SYMBOL_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DB_TYPE_ANJUTA_SYMBOL, DbAnjutaSymbolPrivate))


G_DEFINE_TYPE_WITH_CODE (DbAnjutaSymbol, db_anjuta_symbol, G_TYPE_OBJECT,
			G_IMPLEMENT_INTERFACE (IJS_TYPE_SYMBOL,
						db_anjuta_symbol_interface_init));

static void
db_anjuta_symbol_init (DbAnjutaSymbol *object)
{
	DbAnjutaSymbolPrivate *priv = DB_ANJUTA_SYMBOL_PRIVATE (object);
	priv->file = NULL;
	priv->obj = NULL;
	priv->self_name = NULL;
	priv->self = NULL;
}

static void
db_anjuta_symbol_finalize (GObject *object)
{
	DbAnjutaSymbolPrivate *priv = DB_ANJUTA_SYMBOL_PRIVATE (object);

	g_object_unref (priv->self);
	g_free (priv->self_name);
	g_object_unref (priv->file);
	g_object_unref (priv->self);
	G_OBJECT_CLASS (db_anjuta_symbol_parent_class)->finalize (object);
}

static void
db_anjuta_symbol_class_init (DbAnjutaSymbolClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	/*GObjectClass* parent_class = G_OBJECT_CLASS (klass);*/

	g_type_class_add_private (klass, sizeof (DbAnjutaSymbolPrivate));

	object_class->finalize = db_anjuta_symbol_finalize;
}


static DbAnjutaSymbol*
db_anjuta_symbol_new_for_symbol (IAnjutaSymbolManager *obj, IAnjutaSymbol *self)
{
	DbAnjutaSymbol *object = DB_ANJUTA_SYMBOL (g_object_new (DB_TYPE_ANJUTA_SYMBOL, NULL));
	DbAnjutaSymbolPrivate *priv = DB_ANJUTA_SYMBOL_PRIVATE (object);

	priv->obj = obj;
	priv->self = self;

	return object;
}

DbAnjutaSymbol*
db_anjuta_symbol_new (const gchar *file_name)
{
	DbAnjutaSymbol *self = DB_ANJUTA_SYMBOL (g_object_new (DB_TYPE_ANJUTA_SYMBOL, NULL));
	DbAnjutaSymbolPrivate *priv = DB_ANJUTA_SYMBOL_PRIVATE (self);

	IAnjutaIterable* iter;
	AnjutaPlugin *plugin = ANJUTA_PLUGIN (getPlugin ());

	if (!plugin)
		return NULL;

	priv->obj = anjuta_shell_get_interface (plugin->shell, IAnjutaSymbolManager, NULL);

	priv->file = g_file_new_for_path (file_name);
	priv->self_name = g_file_get_basename (priv->file);
	if (strcmp (priv->self_name + strlen (priv->self_name) - 3, ".js") == 0)
		priv->self_name[strlen (priv->self_name) - 3] = '\0';

	iter = ianjuta_symbol_manager_search_file (priv->obj, IANJUTA_SYMBOL_TYPE_CLASS | IANJUTA_SYMBOL_TYPE_VARIABLE
												| IANJUTA_SYMBOL_TYPE_FILE | IANJUTA_SYMBOL_TYPE_OTHER,
												TRUE, (IAnjutaSymbolField)(IANJUTA_SYMBOL_FIELD_SIMPLE | IANJUTA_SYMBOL_FIELD_FILE_PATH),
												"%", priv->file, -1, -1, NULL);
	if (iter == NULL)
	{
		DEBUG_PRINT ("Not IN DB: %s", file_name);
		return NULL;
	}
	g_object_unref (iter);
	return self;
}

static void
db_anjuta_symbol_interface_init (IJsSymbolIface *iface)
{
	iface->get_arg_list = db_anjuta_symbol_get_arg_list;
	iface->get_base_type = db_anjuta_symbol_get_base_type;
	iface->get_func_ret_type = db_anjuta_symbol_get_func_ret_type;

	iface->get_member = db_anjuta_symbol_get_member;
	iface->get_name = db_anjuta_symbol_get_name;
	iface->list_member = db_anjuta_symbol_list_member;
}

static GList*
db_anjuta_symbol_get_arg_list (IJsSymbol *obj)
{
	g_assert_not_reached ();
	return NULL;
}

static gint
db_anjuta_symbol_get_base_type (IJsSymbol *obj)
{
	return BASE_CLASS;
}

static GList*
db_anjuta_symbol_get_func_ret_type (IJsSymbol *obj)
{
	g_assert_not_reached ();
	return NULL;
}

static IJsSymbol*
db_anjuta_symbol_get_member (IJsSymbol *obj, const gchar * name)
{
	DbAnjutaSymbol *self = DB_ANJUTA_SYMBOL (obj);
	DbAnjutaSymbolPrivate *priv = DB_ANJUTA_SYMBOL_PRIVATE (self);

	g_assert (priv->obj != NULL);

	if (priv->self)
	{
		DEBUG_PRINT ("TODO:%s %d", __FILE__, __LINE__);
	}
	else
	{
		g_assert (priv->file != NULL);

		IAnjutaIterable *iter = ianjuta_symbol_manager_search_file (priv->obj, IANJUTA_SYMBOL_TYPE_MAX, TRUE,  IANJUTA_SYMBOL_FIELD_SIMPLE, name, priv->file, -1, -1, NULL);
		if (iter)
		{
			IAnjutaSymbol *symbol = IANJUTA_SYMBOL (iter);
			return IJS_SYMBOL (db_anjuta_symbol_new_for_symbol (priv->obj, symbol));
		}
	}
	return NULL;
}

static const gchar *
db_anjuta_symbol_get_name (IJsSymbol *obj)
{
	DbAnjutaSymbol *self = DB_ANJUTA_SYMBOL (obj);
	DbAnjutaSymbolPrivate *priv = DB_ANJUTA_SYMBOL_PRIVATE (self);

	const gchar *ret = NULL;
	if (priv->self)
		ret = ianjuta_symbol_get_name (IANJUTA_SYMBOL (priv->self), NULL);
	else
		ret = priv->self_name;

	g_assert (ret != NULL);

	return ret;
}

static GList*
db_anjuta_symbol_list_member (IJsSymbol *obj)
{
	DbAnjutaSymbol *self = DB_ANJUTA_SYMBOL (obj);
	DbAnjutaSymbolPrivate *priv = DB_ANJUTA_SYMBOL_PRIVATE (self);

	GList *ret = NULL;
	IAnjutaIterable *iter;

	g_assert (priv->obj != NULL);

	if (priv->self)
	{
		iter = ianjuta_symbol_manager_get_members (priv->obj, priv->self, IANJUTA_SYMBOL_FIELD_SIMPLE, NULL);
	}else
	{
		g_assert (priv->file != NULL);

		iter = ianjuta_symbol_manager_search_file (priv->obj, IANJUTA_SYMBOL_TYPE_MAX, TRUE,  IANJUTA_SYMBOL_FIELD_SIMPLE, "%", priv->file, -1, -1, NULL);
	}
	if (!iter)
	{
		DEBUG_PRINT ("Can't get member");
		return NULL;
	}
	do {
		IAnjutaSymbol *symbol = IANJUTA_SYMBOL (iter);
		ret = g_list_append (ret, g_strdup (ianjuta_symbol_get_name (symbol, NULL)));
	}while (ianjuta_iterable_next (iter, NULL));
	g_object_unref (iter);
	return ret;
}
