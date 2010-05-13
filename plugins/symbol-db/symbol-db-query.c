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

#include "symbol-db-query.h"

enum
{
	PROP_0,

	PROP_SQL_STATEMENT,
	PROP_SQL_LIMIT,
	PROP_SQL_OFFSET,
	PROP_DB_ENGINE
};



G_DEFINE_TYPE (SymbolDBQuery, sdb_query, G_TYPE_OBJECT);

static void
sdb_query_init (SymbolDBQuery *object)
{
	/* TODO: Add initialization code here */
}

static void
sdb_query_finalize (GObject *object)
{
	/* TODO: Add deinitalization code here */

	G_OBJECT_CLASS (sdb_query_parent_class)->finalize (object);
}

static void
sdb_query_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	g_return_if_fail (SYMBOL_IS_DB_QUERY (object));

	switch (prop_id)
	{
	case PROP_SQL_STATEMENT:
		/* TODO: Add setter for "sql-statement" property here */
		break;
	case PROP_SQL_LIMIT:
		/* TODO: Add setter for "sql-limit" property here */
		break;
	case PROP_SQL_OFFSET:
		/* TODO: Add setter for "sql-offset" property here */
		break;
	case PROP_DB_ENGINE:
		/* TODO: Add setter for "db-engine" property here */
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
sdb_query_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	g_return_if_fail (SYMBOL_IS_DB_QUERY (object));

	switch (prop_id)
	{
	case PROP_SQL_STATEMENT:
		/* TODO: Add getter for "sql-statement" property here */
		break;
	case PROP_SQL_LIMIT:
		/* TODO: Add getter for "sql-limit" property here */
		break;
	case PROP_SQL_OFFSET:
		/* TODO: Add getter for "sql-offset" property here */
		break;
	case PROP_DB_ENGINE:
		/* TODO: Add getter for "db-engine" property here */
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
	GObjectClass* parent_class = G_OBJECT_CLASS (klass);

	object_class->finalize = sdb_query_finalize;
	object_class->set_property = sdb_query_set_property;
	object_class->get_property = sdb_query_get_property;

	g_object_class_install_property (object_class,
	                                 PROP_SQL_STATEMENT,
	                                 g_param_spec_object ("sql-statement",
	                                                      "Sql Statement",
	                                                      "The compiled sql statement",
	                                                      GdaStatement,
	                                                      G_PARAM_READABLE));

	g_object_class_install_property (object_class,
	                                 PROP_SQL_LIMIT,
	                                 g_param_spec_object ("sql-limit",
	                                                      "Sql Limit",
	                                                      "Limit to resultset",
	                                                      gint,
	                                                      G_PARAM_READABLE | G_PARAM_WRITABLE));

	g_object_class_install_property (object_class,
	                                 PROP_SQL_OFFSET,
	                                 g_param_spec_object ("sql-offset",
	                                                      "Sql offset",
	                                                      "Offset of begining of resultset",
	                                                      gint,
	                                                      G_PARAM_READABLE | G_PARAM_WRITABLE));

	g_object_class_install_property (object_class,
	                                 PROP_DB_ENGINE,
	                                 g_param_spec_object ("db-engine",
	                                                      "DB Engine",
	                                                      "The SymbolDBEngine",
	                                                      SymbolDBEngine*,
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
