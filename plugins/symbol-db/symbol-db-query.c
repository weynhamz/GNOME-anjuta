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
#include <libanjuta/interfaces/ianjuta-symbol-query.h>
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
	PROP_DB_ENGINE_SYSTEM,
	PROP_DB_ENGINE_PROJECT,
	PROP_DB_ENGINE_SELECTED
};

struct _SymbolDBQueryPriv {
	GdaStatement *stmt;
	gboolean prepared;
	
	IAnjutaSymbolQueryName name;
	IAnjutaSymbolField fields;
	IAnjutaSymbolType filters;
	IAnjutaSymbolQueryFileScope scope;

	gboolean async;
	SymbolDBEngine *dbe_system;
	SymbolDBEngine *dbe_project;
	SymbolDBEngine *dbe_selected;
	
	/* Param holders */
	GdaSet *params;
	GdaHolder *param_pattern, *param_file_path, *param_limit, *param_offset;
};

typedef struct
{
	IAnjutaSymbolType fields;
	gchar *columns;
	gchar *join;
} SymbolDBQueryFieldSpec;

SymbolDBQueryFieldSpec field_specs[] = {
	{
		IANJUTA_SYMBOL_FIELD_SIMPLE,
		"symbol.symbol_id "
		"symbol.name "
		"symbol.file_position "
		"symbol.scope_definition_id "
		"symbol.signature "
		"symbol.returntype "
		"symbol.is_file_scope ",
		NULL},
	{
		IANJUTA_SYMBOL_FIELD_FILE_PATH,
		"file.file_path",
		"LEFT JOIN file ON symbol.file_defined_id = file.file_id"
	},
	{
		IANJUTA_SYMBOL_FIELD_IMPLEMENTATION,
		"sym_implementation.implementation_name",
		"LEFT JOIN sym_implementation ON symbol.implementation_kind_id = sym_implementation.sym_impl_id"
	},
	{
		IANJUTA_SYMBOL_FIELD_ACCESS,
		"sym_access.access_name",
		"LEFT JOIN sym_access ON symbol.access_kind_id = sym_access.access_kind_id"
	},
	{
		IANJUTA_SYMBOL_FIELD_TYPE | IANJUTA_SYMBOL_FIELD_TYPE_NAME,
		"sym_type.type_type "
		"sym_type.type_name",
		"LEFT JOIN sym_type ON symbol.type_id = sym_type.type_id"
	},
	{
		IANJUTA_SYMBOL_FIELD_KIND,
		"sym_kind.kind_name "
		"sym_kind.is_container",
		"LEFT JOIN sym_kind ON symbol.kind_id = sym_kind.sym_kind_id"
	}
};

/* FIXME: This maps to the bit position of IAnjutaSymbolType enum. This can
 * easily get out of hand if IAnjutaSymbolType enum value and this is not
 * associated more concretely through DB.
 */
static gchar* kind_names[] =
{
	"undef",
	"class",
	"enum",
	"enumerator",
	"field",
	"function",
	"interface",
	"member",
	"method",
	"namespace",
	"package",
	"prototype",
	"struct",
	"typedef",
	"union",
	"variable",
	"externvar",
	"macro",
	"macro_with_arg",
	"file",
	"other"
};

static void ianjuta_symbol_query_iface_init (IAnjutaSymbolQueryIface *iface);

G_DEFINE_TYPE_WITH_CODE (SymbolDBQuery, sdb_query, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (IANJUTA_TYPE_SYMBOL_QUERY,
                                                ianjuta_symbol_query_iface_init));
/**
 * This creates SQL header like "SELECT ... FROM symbol LEFT JOIN ... WHERE "
 */
static void
sdb_query_build_sql_head (SymbolDBQuery *query, GString *sql)
{
	GString *sql_joins;
	gint specs_len, i;
	SymbolDBQueryPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_QUERY (query));
	g_return_if_fail (sql != NULL);

	priv = SYMBOL_DB_QUERY (query)->priv;
	g_return_if_fail (priv->fields != 0);

	g_string_assign (sql, "SELECT ");
	sql_joins = g_string_new_len ("", 512);
	specs_len = sizeof (field_specs) / sizeof (SymbolDBQueryFieldSpec);
	for (i = 0; i < specs_len; i++)
	{
		if (field_specs[i].fields & priv->fields)
		{
			g_string_append (sql, field_specs[i].columns);
			g_string_append (sql, " ");
			g_string_append (sql_joins, field_specs[i].join);
			g_string_append (sql_joins, " ");
		}
	}
	g_string_append (sql, "FROM symbol ");
	g_string_append (sql, sql_joins->str);
	g_string_append (sql, "WHERE ");
	g_string_free (sql_joins, TRUE);
}

static gboolean
sdb_query_build_sql_kind_filter (SymbolDBQuery *query, GString *sql)
{
	gboolean first = TRUE;
	gint bit_count = 0;
	IAnjutaSymbolType filters;
	SymbolDBQueryPriv *priv;

	g_return_val_if_fail (SYMBOL_DB_IS_QUERY (query), TRUE);
	g_return_val_if_fail (sql != NULL, TRUE);

	priv = SYMBOL_DB_QUERY (query)->priv;

	filters = priv->filters;
	if (filters)
	{
		g_string_append (sql, "(symbol.kind_id IN (SELECT kind_id FROM sym_kind WHERE kind_name IN (");
		while (filters)
		{
			if (filters & 1)
			{
				if (first) first = FALSE;
				else g_string_append (sql, ", ");
				g_string_append (sql, "'");
				g_string_append (sql, kind_names[bit_count]);
				g_string_append (sql, "'");
			}
			filters >>= 1;
		}
		g_string_append (sql, ")) ");
		return TRUE;
	}
	return FALSE;
}

static void
sdb_query_update (SymbolDBQuery *query)
{
	const gchar *condition;
	GString *sql;
	SymbolDBQueryPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_QUERY (query));
	priv = SYMBOL_DB_QUERY (query)->priv;

	/* Prepare select conditions */
	switch (priv->name)
	{
		case IANJUTA_SYMBOL_QUERY_SEARCH_PROJECT:
			condition = " (symbol.name LIKE ## /* name:'pattern' type:gchararray */) ";
			priv->dbe_selected = priv->dbe_project;
			break;
		case IANJUTA_SYMBOL_QUERY_SEARCH_SYSTEM:
			condition = " (symbol.name LIKE ## /* name:'pattern' type:gchararray */) ";
			priv->dbe_selected = priv->dbe_system;
			break;
		case IANJUTA_SYMBOL_QUERY_SEARCH_FILE:
			condition = " \
				(symbol.name LIKE ## /* name:'pattern' type:gchararray */) AND \
				(symbol.file_defined_id IN \
					( \
						SELECT file_id \
						FROM file \
						WHERE file_path = ## /* name:'filepath' type:gchararray */ \
					) \
				) ";
			priv->filters &= IANJUTA_SYMBOL_FIELD_FILE_PATH;
			break;
		default:
			g_warning ("Invalid query kind");
			g_warn_if_reached ();
			return;
	}

	/* Build SQL statement */
	sql = g_string_new_len ("", 1024);

	/* Add head of the SQL statement */
	sdb_query_build_sql_head (query, sql);

	/* Add symbol type filters of the SQL statement */
	if (sdb_query_build_sql_kind_filter (query, sql))
		g_string_append (sql, "AND ");

	/* Add condition of the SQL statement */
	g_string_append (sql, condition);
	
	/* Add tail of the SQL statement */
	g_string_append (sql, "LIMIT ## /* name:'limit' type:gint */ ");
	g_string_append (sql, "OFFSET ## /* name:'offset' type:gint */ ");

	/* Prepare statement */
	priv->stmt = symbol_db_engine_get_statement (priv->dbe_selected, sql->str);
	g_string_free (sql, TRUE);
}

static void
sdb_query_init (SymbolDBQuery *query)
{
	SymbolDBQueryPriv *priv;
	GdaHolder *param;
	GSList *param_holders = NULL;
	
	priv = query->priv = SYMBOL_DB_QUERY_GET_PRIVATE(query);

	/* Prepare sql parameter holders */
	param = priv->param_pattern = gda_holder_new_string ("pattern", "");
	param_holders = g_slist_prepend (param_holders, param);
	
	param = priv->param_file_path = gda_holder_new_string ("filepath", "");
	param_holders = g_slist_prepend (param_holders, param);

	param = priv->param_limit = gda_holder_new_int ("limit", INT_MAX);
	param_holders = g_slist_prepend (param_holders, param);

	param = priv->param_limit = gda_holder_new_int ("offset", 0);
	param_holders = g_slist_prepend (param_holders, param);

	priv->params = gda_set_new (param_holders);
	g_slist_free (param_holders);
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
		gda_holder_set_value (priv->param_limit, value, NULL);
		break;
	case PROP_OFFSET:
		gda_holder_set_value (priv->param_offset, value, NULL);
		break;
	case PROP_DB_ENGINE_SYSTEM:
		priv->dbe_system = g_value_get_object (value);
		break;
	case PROP_DB_ENGINE_PROJECT:
		priv->dbe_project = g_value_get_object (value);
		break;
	case PROP_DB_ENGINE_SELECTED:
		priv->dbe_selected = g_value_get_object (value);
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
		g_value_copy (gda_holder_get_value (priv->param_limit), value);
		break;
	case PROP_OFFSET:
		g_value_copy (gda_holder_get_value (priv->param_offset), value);
		break;
	case PROP_DB_ENGINE_SYSTEM:
		g_value_set_object (value, priv->dbe_system);
		break;
	case PROP_DB_ENGINE_PROJECT:
		g_value_set_object (value, priv->dbe_project);
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
	                                                    IANJUTA_TYPE_SYMBOL_QUERY_NAME,
	                                                    IANJUTA_SYMBOL_QUERY_SEARCH_PROJECT,
	                                                    G_PARAM_READABLE |
	                                                    G_PARAM_CONSTRUCT));
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
	                                                   G_PARAM_READABLE |
	                                                   G_PARAM_WRITABLE));
	
	g_object_class_install_property (object_class,
	                                 PROP_OFFSET,
	                                 g_param_spec_int ("offset",
	                                                   "Query offset",
	                                                   "Offset of begining of resultset",
	                                                   0, INT_MAX, 0,
	                                                   G_PARAM_READABLE |
	                                                   G_PARAM_WRITABLE));
	
	g_object_class_install_property (object_class,
	                                 PROP_DB_ENGINE_SYSTEM,
	                                 g_param_spec_object ("db-engine-system",
	                                                      "System DB Engine",
	                                                      "The System SymbolDBEngine",
	                                                      SYMBOL_TYPE_DB_ENGINE,
	                                                      G_PARAM_READABLE |
	                                                      G_PARAM_WRITABLE |
	                                                      G_PARAM_CONSTRUCT));
	g_object_class_install_property (object_class,
	                                 PROP_DB_ENGINE_PROJECT,
	                                 g_param_spec_object ("db-engine-project",
	                                                      "Project DB Engine",
	                                                      "The Project SymbolDBEngine",
	                                                      SYMBOL_TYPE_DB_ENGINE,
	                                                      G_PARAM_READABLE |
	                                                      G_PARAM_WRITABLE |
	                                                      G_PARAM_CONSTRUCT));
	g_object_class_install_property (object_class,
	                                 PROP_DB_ENGINE_SELECTED,
	                                 g_param_spec_object ("db-engine-selected",
	                                                      "Selected DB Engine",
	                                                      "The selected SymbolDBEngine",
	                                                      SYMBOL_TYPE_DB_ENGINE,
	                                                      G_PARAM_READABLE));
}

static IAnjutaIterable*
sdb_query_execute (SymbolDBQuery *query)
{
	SymbolDBEngineIterator *iter;
	GdaDataModel *data_model;
	SymbolDBQueryPriv *priv = query->priv;
	
	if (!priv->prepared)
	{
		sdb_query_update (query);
		priv->prepared = TRUE;
	}
	data_model = symbol_db_engine_execute_select (priv->dbe_selected,
	                                              priv->stmt,
	                                              priv->params);
	iter = symbol_db_engine_iterator_new (data_model, 
	                                      symbol_db_engine_get_type_conversion_hash (priv->dbe_selected),
	                                      symbol_db_engine_get_project_directory (priv->dbe_selected));
	return IANJUTA_ITERABLE (iter);
}

static IAnjutaIterable*
sdb_query_search_system (IAnjutaSymbolQuery *query,
                         const gchar *search_string, GError **error)
{
	GValue sv = {0};
	SymbolDBQueryPriv *priv;

	g_return_val_if_fail (SYMBOL_DB_IS_QUERY (query), NULL);

	priv = SYMBOL_DB_QUERY (query)->priv;
	g_return_val_if_fail (priv->name == IANJUTA_SYMBOL_QUERY_SEARCH_SYSTEM, NULL);

	g_value_init (&sv, G_TYPE_STRING);
	g_value_set_static_string (&sv, search_string);
	gda_holder_set_value (priv->param_pattern, &sv, NULL);
	return sdb_query_execute (SYMBOL_DB_QUERY (query));
}

static IAnjutaIterable*
sdb_query_search_project (IAnjutaSymbolQuery *query,
                          const gchar *search_string, GError **error)
{
	GValue sv = {0};
	SymbolDBQueryPriv *priv;

	g_return_val_if_fail (SYMBOL_DB_IS_QUERY (query), NULL);

	priv = SYMBOL_DB_QUERY (query)->priv;
	g_return_val_if_fail (priv->name == IANJUTA_SYMBOL_QUERY_SEARCH_PROJECT, NULL);

	g_value_init (&sv, G_TYPE_STRING);
	g_value_set_static_string (&sv, search_string);
	gda_holder_set_value (priv->param_pattern, &sv, NULL);
	return sdb_query_execute (SYMBOL_DB_QUERY (query));
}

static IAnjutaIterable*
sdb_query_search_file (IAnjutaSymbolQuery *query, const gchar *search_string,
                       const GFile *file, GError **error)
{
	gchar *rel_file_path, *abs_file_path;
	GValue sv = {0};
	SymbolDBQueryPriv *priv;

	g_return_val_if_fail (SYMBOL_DB_IS_QUERY (query), NULL);

	priv = SYMBOL_DB_QUERY (query)->priv;
	g_return_val_if_fail (priv->name == IANJUTA_SYMBOL_QUERY_SEARCH_SYSTEM, NULL);

	g_value_init (&sv, G_TYPE_STRING);
	g_value_set_static_string (&sv, search_string);
	gda_holder_set_value (priv->param_pattern, &sv, NULL);

	abs_file_path = g_file_get_path (file);
	rel_file_path = symbol_db_util_get_file_db_path (priv->dbe_selected, abs_file_path);
	g_value_take_string (&sv, rel_file_path);
	gda_holder_set_value (priv->param_file_path, &sv, NULL);
	g_free (abs_file_path);
	g_value_reset (&sv);
	
	return sdb_query_execute (SYMBOL_DB_QUERY (query));
}

static void
ianjuta_symbol_query_iface_init (IAnjutaSymbolQueryIface *iface)
{
	iface->search_system = sdb_query_search_system;
	iface->search_project = sdb_query_search_project;
	iface->search_file = sdb_query_search_file;
}
