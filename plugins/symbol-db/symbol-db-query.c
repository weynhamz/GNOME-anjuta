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
#include <stdarg.h>
#include <libgda/gda-statement.h>
#include <libanjuta/interfaces/ianjuta-symbol-query.h>
#include "symbol-db-engine.h"
#include "symbol-db-query.h"
#include "symbol-db-query-result.h"

#define SYMBOL_DB_QUERY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
	SYMBOL_DB_TYPE_QUERY, SymbolDBQueryPriv))

enum
{
	PROP_0,
	PROP_QUERY_NAME,
	PROP_QUERY_MODE,
	PROP_FILTERS,
	PROP_FILE_SCOPE,
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
	IAnjutaSymbolQueryMode mode;
	IAnjutaSymbolField fields[IANJUTA_SYMBOL_FIELD_END];
	IAnjutaSymbolType filters;
	IAnjutaSymbolQueryFileScope file_scope;

	gboolean async;
	SymbolDBEngine *dbe_system;
	SymbolDBEngine *dbe_project;
	SymbolDBEngine *dbe_selected;
	
	/* Param holders */
	GdaSet *params;
	GdaHolder *param_pattern, *param_file_path, *param_limit, *param_offset;

	/* Aync results */
	gboolean is_canceled;
	IAnjutaIterable *async_result;
};

typedef enum
{
	SDB_QUERY_TABLE_SYMBOL,
	SDB_QUERY_TABLE_FILE,
	SDB_QUERY_TABLE_IMPLEMENTATION,
	SDB_QUERY_TABLE_ACCESS,
	SDB_QUERY_TABLE_TYPE,
	SDB_QUERY_TABLE_KIND,
	SDB_QUERY_TABLE_MAX,
}  SdbQueryTable;

/* This array must map to each table enumerated above */
static gchar *table_joins[] =
{
	NULL,
	"LEFT JOIN file ON symbol.file_defined_id = file.file_id",
	"LEFT JOIN sym_implementation ON symbol.implementation_kind_id = sym_implementation.sym_impl_id",
	"LEFT JOIN sym_access ON symbol.access_kind_id = sym_access.access_kind_id",
	"LEFT JOIN sym_type ON symbol.type_id = sym_type.type_id",
	"LEFT JOIN sym_kind ON symbol.kind_id = sym_kind.sym_kind_id",
};

typedef struct
{
	gchar *column;
	SdbQueryTable table;
} SdbQueryFieldSpec;

/* This table must map to each IAnjutaSymbolField value */
SdbQueryFieldSpec field_specs[] = {
	{"symbol.symbol_id ", SDB_QUERY_TABLE_SYMBOL},
	{"symbol.name ", SDB_QUERY_TABLE_SYMBOL},
	{"symbol.file_position ", SDB_QUERY_TABLE_SYMBOL},
	{"symbol.scope_definition_id ", SDB_QUERY_TABLE_SYMBOL},
	{"symbol.signature ", SDB_QUERY_TABLE_SYMBOL},
	{"symbol.returntype ", SDB_QUERY_TABLE_SYMBOL},
	{"symbol.is_file_scope ", SDB_QUERY_TABLE_SYMBOL},
	{"file.file_path", SDB_QUERY_TABLE_FILE},
	{"sym_implementation.implementation_name", SDB_QUERY_TABLE_IMPLEMENTATION},
	{"sym_access.access_name", SDB_QUERY_TABLE_ACCESS},
	{"sym_type.type_type ", SDB_QUERY_TABLE_TYPE},
	{"sym_type.type_name", SDB_QUERY_TABLE_TYPE},
	{"sym_kind.kind_name ", SDB_QUERY_TABLE_KIND},
	{"sym_kind.is_container", SDB_QUERY_TABLE_KIND}
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

G_DEFINE_TYPE_WITH_CODE (SymbolDBQuery, sdb_query, ANJUTA_TYPE_ASYNC_COMMAND,
                         G_IMPLEMENT_INTERFACE (IANJUTA_TYPE_SYMBOL_QUERY,
                                                ianjuta_symbol_query_iface_init));
/**
 * This creates SQL header like "SELECT ... FROM symbol LEFT JOIN ... WHERE "
 */
static void
sdb_query_build_sql_head (SymbolDBQuery *query, GString *sql)
{
	GString *sql_joins;
	gint i;
	gboolean tables_joined[SDB_QUERY_TABLE_MAX];
	SymbolDBQueryPriv *priv;
	IAnjutaSymbolField *field_ptr;

	g_return_if_fail (SYMBOL_DB_IS_QUERY (query));
	g_return_if_fail (sql != NULL);

	priv = SYMBOL_DB_QUERY (query)->priv;
	g_return_if_fail (priv->fields != 0);

	/* Ensure the lookup tables in order */
	g_assert (sizeof (table_joins) == SDB_QUERY_TABLE_MAX);
	g_assert (sizeof (field_specs) == IANJUTA_SYMBOL_FIELD_END);
	
	for (i = 0; i < SDB_QUERY_TABLE_MAX; i++)
		tables_joined[i] = FALSE;
	/* "symbol" table is in built, so skip it */
	tables_joined[SDB_QUERY_TABLE_SYMBOL] = TRUE;
	
	g_string_assign (sql, "SELECT ");
	sql_joins = g_string_new_len ("", 512);
	field_ptr = priv->fields;
	while (*field_ptr != IANJUTA_SYMBOL_FIELD_END)
	{
		g_string_append (sql, field_specs[*field_ptr].column);
		g_string_append (sql, " ");
		if (!tables_joined[field_specs[*field_ptr].table])
		{
			tables_joined[field_specs[*field_ptr].table] = TRUE;
			g_string_append (sql_joins,
			                 table_joins[field_specs[*field_ptr].table]);
			g_string_append (sql_joins, " ");
		}
		field_ptr++;
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

static IAnjutaIterable*
sdb_query_execute_real (SymbolDBQuery *query)
{
	SymbolDBQueryResult *iter;
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
	iter = symbol_db_query_result_new (data_model, 
	                                   priv->fields,
	                                   symbol_db_engine_get_type_conversion_hash (priv->dbe_selected),
	                                   symbol_db_engine_get_project_directory (priv->dbe_selected));
	return IANJUTA_ITERABLE (iter);
}

static IAnjutaIterable*
sdb_query_execute (SymbolDBQuery *query)
{
	switch (query->priv->mode)
	{
		case IANJUTA_SYMBOL_QUERY_MODE_SYNC:
			return sdb_query_execute_real (query);
		case IANJUTA_SYMBOL_QUERY_MODE_ASYNC:
			query->priv->is_canceled = FALSE;
			anjuta_command_start (ANJUTA_COMMAND (query));
			return NULL;
		case IANJUTA_SYMBOL_QUERY_MODE_QUEUED_SINGLE:
		case IANJUTA_SYMBOL_QUERY_MODE_QUEUED_MULTI:
			/* FIXME */
		default:
			g_warn_if_reached ();
	}
	return NULL;
}

static void
on_sdb_query_async_data_arrived (SymbolDBQuery *query, gpointer data)
{
	if (!query->priv->is_canceled)
		g_signal_emit_by_name (query, "async-result",
		                       query->priv->async_result);
	g_object_unref (query->priv->async_result);
	query->priv->async_result = NULL;
	query->priv->is_canceled = FALSE;
}

static guint
sdb_query_async_run (AnjutaCommand *command)
{
	SymbolDBQuery *query;

	g_return_val_if_fail (SYMBOL_DB_IS_QUERY (command), -1);
	query = SYMBOL_DB_QUERY (command);

	g_return_val_if_fail (query->priv->mode == IANJUTA_SYMBOL_QUERY_MODE_ASYNC, -1);
	
	query->priv->async_result = sdb_query_execute_real (query);
	return 0;
}

static void
sdb_query_async_cancel (AnjutaCommand *command)
{
	SymbolDBQuery *query;
	g_return_if_fail (SYMBOL_DB_IS_QUERY (command));
	query = SYMBOL_DB_QUERY (command);

	g_return_if_fail (query->priv->mode != IANJUTA_SYMBOL_QUERY_MODE_SYNC);
	query->priv->is_canceled = TRUE;
}

static void
sdb_query_init (SymbolDBQuery *query)
{
	SymbolDBQueryPriv *priv;
	GdaHolder *param;
	GSList *param_holders = NULL;
	
	priv = query->priv = SYMBOL_DB_QUERY_GET_PRIVATE(query);

	/* By default only ID and Name fields are enabled */
	priv->fields[0] = IANJUTA_SYMBOL_FIELD_ID;
	priv->fields[1] = IANJUTA_SYMBOL_FIELD_NAME;
	priv->fields[2] = IANJUTA_SYMBOL_FIELD_END;
	
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

	/* Prepare async signals */
	priv->is_canceled = FALSE;
	g_signal_connect (query, "notify-data-arrived",
	                  G_CALLBACK (on_sdb_query_async_data_arrived), query);
}

static void
sdb_query_finalize (GObject *object)
{
	G_OBJECT_CLASS (sdb_query_parent_class)->finalize (object);
}

static void
sdb_query_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	SymbolDBQuery *query;
	SymbolDBQueryPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_QUERY (object));
	query = SYMBOL_DB_QUERY (object);
	priv = query->priv;
	
	switch (prop_id)
	{
	case PROP_QUERY_NAME:
		priv->name = g_value_get_enum (value);
		sdb_query_update (query);
		break;
	case PROP_QUERY_MODE:
		priv->mode = g_value_get_enum (value);
		break;
	case PROP_FILTERS:
		priv->filters = g_value_get_enum (value);
		sdb_query_update (query);
		break;
	case PROP_FILE_SCOPE:
		priv->file_scope = g_value_get_enum (value);
		sdb_query_update (query);
		break;
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
	case PROP_QUERY_NAME:
		g_value_set_enum (value, priv->name);
		break;
	case PROP_QUERY_MODE:
		g_value_set_enum (value, priv->mode);
		break;
	case PROP_FILTERS:
		g_value_set_enum (value, priv->filters);
		break;
	case PROP_FILE_SCOPE:
		g_value_set_enum (value, priv->file_scope);
		break;
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
	AnjutaCommandClass *command_class = ANJUTA_COMMAND_CLASS (klass);
	
	g_type_class_add_private (klass, sizeof (SymbolDBQueryPriv));

	object_class->finalize = sdb_query_finalize;
	object_class->set_property = sdb_query_set_property;
	object_class->get_property = sdb_query_get_property;

	command_class->run = sdb_query_async_run;
	command_class->cancel = sdb_query_async_cancel;

	g_object_class_install_property (object_class,
	                                 PROP_QUERY_NAME,
	                                 g_param_spec_enum ("query-name",
	                                                    "Query name",
	                                                    "The query name",
	                                                    IANJUTA_TYPE_SYMBOL_QUERY_NAME,
	                                                    IANJUTA_SYMBOL_QUERY_SEARCH_PROJECT,
	                                                    G_PARAM_READABLE |
	                                                    G_PARAM_CONSTRUCT));
	g_object_class_install_property (object_class,
	                                 PROP_QUERY_MODE,
	                                 g_param_spec_enum ("query-mode",
	                                                    "Query Mode",
	                                                    "The query mode",
	                                                    IANJUTA_TYPE_SYMBOL_QUERY_MODE,
	                                                    IANJUTA_SYMBOL_QUERY_MODE_SYNC,
	                                                    G_PARAM_READABLE |
	                                                    G_PARAM_WRITABLE));
	g_object_class_install_property (object_class,
	                                 PROP_FILTERS,
	                                 g_param_spec_enum ("filters",
	                                                    "Symbol type filters",
	                                                    "The symbol type filters",
	                                                    IANJUTA_TYPE_SYMBOL_TYPE,
	                                                    -1,
	                                                    G_PARAM_READABLE |
	                                                    G_PARAM_WRITABLE));
	g_object_class_install_property (object_class,
	                                 PROP_FILE_SCOPE,
	                                 g_param_spec_enum ("file-scope",
	                                                    "file scope",
	                                                    "The file scope search",
	                                                    IANJUTA_TYPE_SYMBOL_QUERY_FILE_SCOPE,
	                                                    IANJUTA_SYMBOL_QUERY_SEARCH_FS_IGNORE,
	                                                    G_PARAM_READABLE |
	                                                    G_PARAM_WRITABLE));
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

	abs_file_path = g_file_get_path ((GFile*)file);
	rel_file_path = symbol_db_util_get_file_db_path (priv->dbe_selected, abs_file_path);
	g_value_take_string (&sv, rel_file_path);
	gda_holder_set_value (priv->param_file_path, &sv, NULL);
	g_free (abs_file_path);
	g_value_reset (&sv);
	
	return sdb_query_execute (SYMBOL_DB_QUERY (query));
}

static void
sdb_query_set_fields (IAnjutaSymbolQuery *query, IAnjutaSymbolField field0, ...)
{
	gint i = 0;
	IAnjutaSymbolField arg;
	va_list vl;
	SymbolDBQueryPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_QUERY (query));

	priv = SYMBOL_DB_QUERY (query)->priv;
	va_start (vl, field0);
	arg = field0;
	priv->fields[i] = arg;
	i++;
	while (arg != IANJUTA_SYMBOL_FIELD_END)
	{
		arg = va_arg (vl, IAnjutaSymbolField);
		priv->fields[i] = arg;
		i++;
	}
	va_end (vl);
}

static void
sdb_query_set_mode (IAnjutaSymbolQuery *query, IAnjutaSymbolQueryMode mode,
                    GError **err)
{
	g_object_set (query, "query-mode", mode, NULL);
}

static void
sdb_query_set_filters (IAnjutaSymbolQuery *query, IAnjutaSymbolType filters,
                       gboolean include_types, GError **err)
{
	g_object_set (query, "filters", filters, NULL);
	/* FIXME: include_types */
}

static void
sdb_query_set_limit (IAnjutaSymbolQuery *query, gint limit, GError **err)
{
	g_object_set (query, "limit", limit, NULL);
}

static void
sdb_query_set_offset (IAnjutaSymbolQuery *query, gint offset, GError **err)
{
	g_object_set (query, "offset", offset, NULL);
}

static void
sdb_query_set_file_scope (IAnjutaSymbolQuery *query,
                          IAnjutaSymbolQueryFileScope file_scope,
                          GError **err)
{
	g_object_set (query, "file-scope", file_scope, NULL);
}

static void
ianjuta_symbol_query_iface_init (IAnjutaSymbolQueryIface *iface)
{
	iface->search_system = sdb_query_search_system;
	iface->search_project = sdb_query_search_project;
	iface->search_file = sdb_query_search_file;
	iface->set_fields = sdb_query_set_fields;
	iface->set_mode = sdb_query_set_mode;
	iface->set_filters = sdb_query_set_filters;
	iface->set_file_scope = sdb_query_set_file_scope;
	iface->set_limit = sdb_query_set_limit;
	iface->set_offset = sdb_query_set_offset;
}

SymbolDBQuery *
symbol_db_query_new (SymbolDBEngine *system_db_engine,
                     SymbolDBEngine *project_db_engine,
                     IAnjutaSymbolQueryName name)
{
	return g_object_new (SYMBOL_DB_TYPE_QUERY,
	                     "db-engine-system", system_db_engine,
	                     "db-engine-project", project_db_engine,
	                     "query-name", name, NULL);
}
