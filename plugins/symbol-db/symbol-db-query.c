/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Naba Kumar 2010 <naba@gnome.org>
 * 
 * anjuta is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <limits.h>
#include <libgda/gda-statement.h>
#include "symbol-db-query.h"
#include "symbol-db-engine.h"

#define SYMBOL_DB_QUERY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
	SYMBOL_DB_TYPE_QUERY, SymbolDBQueryPriv))

enum
{
	PROP_0,
	PROP_QUERY_KIND,
	PROP_STATEMENT,
	PROP_LIMIT,
	PROP_OFFSET,
	PROP_DB_ENGINE
};

struct _SymbolDBQueryPriv {
	GdaStatement *stmt;
	gint limit;
	gint offset;
	SymbolDBEngine *dbe;
};

G_DEFINE_TYPE (SymbolDBQuery, sdb_query, G_TYPE_OBJECT);

static void
sdb_query_init (SymbolDBQuery *object)
{
	object->priv = SYMBOL_DB_QUERY_GET_PRIVATE(object);
}

static void
sdb_query_finalize (GObject *object)
{
	G_OBJECT_CLASS (sdb_query_parent_class)->finalize (object);
}

static void
sdb_query_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	SymbolDBQueryPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_QUERY (object));
	priv = SYMBOL_DB_QUERY (object)->priv;
	
	switch (prop_id)
	{
	case PROP_LIMIT:
		priv->limit = g_value_get_int (value);
		break;
	case PROP_OFFSET:
		priv->offset = g_value_get_int (value);
		break;
	case PROP_DB_ENGINE:
		priv->dbe = g_value_get_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
sdb_query_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	SymbolDBQueryPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_QUERY (object));
	priv = SYMBOL_DB_QUERY (object)->priv;
	
	switch (prop_id)
	{
	case PROP_STATEMENT:
		g_value_set_object (value, priv->stmt);
		break;
	case PROP_LIMIT:
		g_value_set_int (value, priv->limit);
		break;
	case PROP_OFFSET:
		g_value_set_int (value, priv->offset);
		break;
	case PROP_DB_ENGINE:
		g_value_set_object (value, priv->dbe);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
sdb_query_class_init (SymbolDBQueryClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	g_type_class_add_private (klass, sizeof (SymbolDBQueryPriv));

	object_class->finalize = sdb_query_finalize;
	object_class->set_property = sdb_query_set_property;
	object_class->get_property = sdb_query_get_property;

	g_object_class_install_property (object_class,
	                                 PROP_QUERY_KIND,
	                                 g_param_spec_enum ("query-kind",
	                                                    "Query kind",
	                                                    "The query kind",
	                                                    SymbolDBQueryType,
	                                                    G_PARAM_READABLE | G_PARAM_CONSTRUCT));
	g_object_class_install_property (object_class,
	                                 PROP_STATEMENT,
	                                 g_param_spec_object ("statement",
	                                                      "Sql Statement",
	                                                      "The compiled query statement",
	                                                      GDA_TYPE_STATEMENT,
	                                                      G_PARAM_READABLE));

	g_object_class_install_property (object_class,
	                                 PROP_LIMIT,
	                                 g_param_spec_int ("limit",
	                                                   "Query Limit",
	                                                   "Limit to resultset",
	                                                   0, INT_MAX, INT_MAX,
	                                                   G_PARAM_READABLE | G_PARAM_WRITABLE));
	
	g_object_class_install_property (object_class,
	                                 PROP_OFFSET,
	                                 g_param_spec_int ("offset",
	                                                   "Query offset",
	                                                   "Offset of begining of resultset",
	                                                   0, INT_MAX, 0,
	                                                   G_PARAM_READABLE | G_PARAM_WRITABLE));
	
	g_object_class_install_property (object_class,
	                                 PROP_DB_ENGINE,
	                                 g_param_spec_object ("db-engine",
	                                                      "DB Engine",
	                                                      "The SymbolDBEngine",
	                                                      SYMBOL_TYPE_DB_ENGINE,
	                                                      G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
}

IAnjutaIterable*
sdb_query_search (gchar *search_string)
{
	/* TODO: Add public function implementation here */
}

IAnjutaIterable*
sdb_query_search_prefix (gchar *search_string)
{
	/* TODO: Add public function implementation here */
}
