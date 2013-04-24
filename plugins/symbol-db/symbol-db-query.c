/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * symbol-db-query.c
 * Copyright (C) Naba Kumar		2010 <naba@gnome.org>
 * Copyright (C) Massimo Cora'  2011 <maxcvs@email.it>
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
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-symbol-query.h>
#include "symbol-db-engine.h"
#include "symbol-db-query.h"
#include "symbol-db-query-result.h"

#define SYMBOL_DB_QUERY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
	SYMBOL_DB_TYPE_QUERY, SymbolDBQueryPriv))

/* Class properties */
enum
{
	PROP_0,
	PROP_QUERY_NAME,
	PROP_QUERY_DB,
	PROP_QUERY_MODE,
	PROP_FILTERS,
	PROP_FILE_SCOPE,
	PROP_STATEMENT,
	PROP_LIMIT,
	PROP_OFFSET,
	PROP_ORDER_BY,
	PROP_GROUP_BY,
	PROP_DB_ENGINE_SYSTEM,
	PROP_DB_ENGINE_PROJECT,
	PROP_DB_ENGINE_SELECTED,
	PROP_SESSION_PACKAGES
};

struct _SymbolDBQueryPriv {
	gchar *sql_stmt;
	GdaStatement *stmt;

	IAnjutaSymbolQueryName name;
	IAnjutaSymbolQueryMode mode;
	IAnjutaSymbolField fields[IANJUTA_SYMBOL_FIELD_END];
	IAnjutaSymbolType filters;
	IAnjutaSymbolQueryFileScope file_scope;
	IAnjutaSymbolField group_by;
	IAnjutaSymbolField order_by;

	SymbolDBEngine *dbe_system;
	SymbolDBEngine *dbe_project;
	/* a reference to dbe_system or dbe_project */
	SymbolDBEngine *dbe_selected;
	GHashTable *session_packages;
	
	/* Param holders */
	GdaSet *params;
	GdaHolder *param_pattern, *param_file_path, *param_limit, *param_offset;
	GdaHolder *param_file_line, *param_id;

	/* Aync results */
	gboolean query_queued;
	GAsyncQueue *async_result_queue;
	guint async_poll_id;
	gint async_run_count;
	gint async_cancel_count;
	gint async_result_count;
};

/* Enumerated list of DB tables used in queries */
typedef enum
{
	SDB_QUERY_TABLE_SYMBOL,
	SDB_QUERY_TABLE_FILE,
	SDB_QUERY_TABLE_IMPLEMENTATION,
	SDB_QUERY_TABLE_ACCESS,
	SDB_QUERY_TABLE_KIND,
	SDB_QUERY_TABLE_PROJECT,
	SDB_QUERY_TABLE_MAX,
}  SdbQueryTable;

/* This array must map to each table enumerated above */
static gchar *table_joins[] =
{
	NULL,
	"LEFT JOIN file ON symbol.file_defined_id = file.file_id",
	"LEFT JOIN sym_implementation ON symbol.implementation_kind_id = sym_implementation.sym_impl_id",
	"LEFT JOIN sym_access ON symbol.access_kind_id = sym_access.access_kind_id",
	"LEFT JOIN sym_kind ON symbol.kind_id = sym_kind.sym_kind_id",
	"LEFT JOIN file ON symbol.file_defined_id = file.file_id \
	 LEFT JOIN project ON file.prj_id = project.project_id",
};

/* Spec to associate a coloum to its table */
typedef struct
{
	gchar *column;
	SdbQueryTable table;
} SdbQueryFieldSpec;

/* Association of all columns to its tables.
 * This table must map to each IAnjutaSymbolField value
 */
SdbQueryFieldSpec field_specs[] = {
	{"symbol.symbol_id ", SDB_QUERY_TABLE_SYMBOL},
	{"symbol.name ", SDB_QUERY_TABLE_SYMBOL},
	{"symbol.file_position ", SDB_QUERY_TABLE_SYMBOL},
	{"symbol.scope_definition_id ", SDB_QUERY_TABLE_SYMBOL},
	{"symbol.is_file_scope ", SDB_QUERY_TABLE_SYMBOL},
	{"symbol.signature ", SDB_QUERY_TABLE_SYMBOL},
	{"symbol.returntype ", SDB_QUERY_TABLE_SYMBOL},
	{"symbol.type_type ", SDB_QUERY_TABLE_SYMBOL},
	{"symbol.type_name ", SDB_QUERY_TABLE_SYMBOL},
	{"file.file_path ", SDB_QUERY_TABLE_FILE},
	{"project.project_name ", SDB_QUERY_TABLE_PROJECT},
	{"project.project_version ", SDB_QUERY_TABLE_PROJECT},
	{"sym_implementation.implementation_name ", SDB_QUERY_TABLE_IMPLEMENTATION},
	{"sym_access.access_name ", SDB_QUERY_TABLE_ACCESS},
	{"sym_kind.kind_name ", SDB_QUERY_TABLE_KIND},
	{"sym_kind.is_container ", SDB_QUERY_TABLE_KIND}
};

/* FIXME: This maps to the bit position of IAnjutaSymbolType enum. This can
 * easily get out of hand if IAnjutaSymbolType enum value and this is not
 * associated more concretely through DB. To do it properly, get this list
 * from database directly instead of hardcoding here.
 */
static gchar* kind_names[] =
{
	NULL,
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
 * sdb_query_build_sql_head:
 * @query: The query object
 * @sql: The string where sql statement will be constructed.
 * 
 * This creates SQL header like "SELECT ... FROM symbol LEFT JOIN ... WHERE ".
 * It goes through the list of fields specified for the query and constructs
 * the SELECT part. At the same time, also adds the necessary table joins
 * needed for the columns.
 */
static void
sdb_query_build_sql_head (SymbolDBQuery *query, GString *sql)
{
	GString *sql_joins;
	gint i;
	gboolean tables_joined[SDB_QUERY_TABLE_MAX];
	SymbolDBQueryPriv *priv;
	IAnjutaSymbolField *field_ptr;
	gboolean first_field = TRUE;

	g_return_if_fail (SYMBOL_DB_IS_QUERY (query));
	g_return_if_fail (sql != NULL);

	priv = SYMBOL_DB_QUERY (query)->priv;
	g_return_if_fail (priv->fields != 0);

	/* Ensure the lookup tables are in order */
	g_assert (G_N_ELEMENTS (table_joins) == SDB_QUERY_TABLE_MAX);
	g_assert (G_N_ELEMENTS (field_specs) == IANJUTA_SYMBOL_FIELD_END);

	/* Initialize table joins state to FALSE */
	for (i = 0; i < SDB_QUERY_TABLE_MAX; i++)
		tables_joined[i] = FALSE;
	
	/* "symbol" table is in-built, so skip it */
	tables_joined[SDB_QUERY_TABLE_SYMBOL] = TRUE;
	
	g_string_assign (sql, "SELECT ");
	sql_joins = g_string_sized_new (512);

	/* For each field, construct the coloumns list and the necessary joins */
	field_ptr = priv->fields;
	while (*field_ptr != IANJUTA_SYMBOL_FIELD_END)
	{
		if (first_field)
			first_field = FALSE;
		else
			g_string_append (sql, ", ");
		
		g_string_append (sql, field_specs[*field_ptr].column);
		if (tables_joined[field_specs[*field_ptr].table] == FALSE)
		{
			g_string_append (sql_joins,
			                 table_joins[field_specs[*field_ptr].table]);
			g_string_append (sql_joins, " ");
			tables_joined[field_specs[*field_ptr].table] = TRUE;
		}
		field_ptr++;
	}
	g_string_append (sql, " FROM symbol ");
	g_string_append (sql, sql_joins->str);
	g_string_append (sql, " WHERE ");
	g_string_free (sql_joins, TRUE);
}

/**
 * sdb_query_build_sql_kind_filter:
 * @query: The query.
 * @sql: The string where filter SQL code is appended.
 * 
 * Constructs the necessary sql conditional to filter the resultset to
 * only those set to filtered symbol types. It will add a subquery to
 * achieve this filter.
 */
static void
sdb_query_build_sql_kind_filter (SymbolDBQuery *query, GString *sql)
{
	gboolean first = TRUE;
	gint bit_count = 0;
	IAnjutaSymbolType filters;
	SymbolDBQueryPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_QUERY (query));
	g_return_if_fail (sql != NULL);

	priv = SYMBOL_DB_QUERY (query)->priv;

	/* For each filter, identified by bit shifting filters mask, create
	 * SQL subquery to select only those symbol types.
	 */
	filters = priv->filters;
	if (filters)
	{
		g_string_append (sql, "AND (symbol.kind_id IN (SELECT sym_kind_id FROM sym_kind WHERE kind_name IN (");
		while (filters)
		{
			bit_count++;
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
		g_string_append (sql, "))) ");
	}
}

/**
 * sdb_query_add_field:
 * @query: The query.
 * @field: The field to add.
 * 
 * Adds @field to current list of fields if it isn't already there.
 * It is used to add additional required fields for some queries which the
 * user may not have set himself.
 */
static void
sdb_query_add_field (SymbolDBQuery *query, IAnjutaSymbolField field)
{
	gint idx = 0;

	/* Iterate until the given field is found in the list, otherwise add it */
	while (query->priv->fields[idx] != IANJUTA_SYMBOL_FIELD_END)
	{
		if (query->priv->fields[idx] == field)
			return;
		idx++;
	}
	query->priv->fields[idx] = field;
	query->priv->fields[idx + 1] = IANJUTA_SYMBOL_FIELD_END;
}

static gboolean
sdb_query_is_field_set (SymbolDBQuery *query, IAnjutaSymbolField field)
{
	gint i;

	for (i = 0; i < IANJUTA_SYMBOL_FIELD_END; i++)
	{
		if (query->priv->fields[i] == field)
			return TRUE;
	}

	/* not found */
	return FALSE;
}

/**
 * sdb_query_reset:
 * @query: The query
 * 
 * Resets the query to unprepared status.
 */
static void
sdb_query_reset (SymbolDBQuery *query)
{
	if (query->priv->stmt)
		g_object_unref (query->priv->stmt);
	query->priv->stmt = NULL;
	g_free (query->priv->sql_stmt);
	query->priv->sql_stmt = NULL;
}

/**
 * sdb_query_update:
 * @query: The query
 * 
 * Updates the SQL query based on latest set paramenters. Usually called
 * after some parameter influencing the SQL contruct is changed. It will
 * also attempt to compile the resulting SQL statement if possible, otherwise
 * defer it for later when for database connection is established.
 */
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
		case IANJUTA_SYMBOL_QUERY_SEARCH:
			condition = " (symbol.name LIKE ## /* name:'pattern' type:gchararray */) ";
			break;
		case IANJUTA_SYMBOL_QUERY_SEARCH_ALL:
			condition = "1 = 1 ";
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
			sdb_query_add_field (query, IANJUTA_SYMBOL_FIELD_FILE_PATH);
			break;
		case IANJUTA_SYMBOL_QUERY_SEARCH_IN_SCOPE:
			condition = " \
				(symbol.name LIKE ## /* name:'pattern' type:gchararray */ \
				AND symbol.scope_id = \
					(\
						SELECT scope_definition_id \
						FROM symbol \
						WHERE symbol_id = ## /* name:'symbolid' type:gint */ \
					)) ";
			break;
		case IANJUTA_SYMBOL_QUERY_SEARCH_ID:
			condition = "(symbol.symbol_id = ## /* name:'symbolid' type:gint */)";
			break;
		case IANJUTA_SYMBOL_QUERY_SEARCH_MEMBERS:
			condition =
				"(symbol.scope_id IN \
				( \
					SELECT symbol.scope_definition_id \
					FROM symbol \
					WHERE symbol.symbol_id = ## /* name:'symbolid' type:gint */ \
				) \
				AND symbol.scope_id > 0) ORDER BY symbol.name ";
			break;
		case IANJUTA_SYMBOL_QUERY_SEARCH_CLASS_PARENTS:
			condition =
				"(symbol.symbol_id IN \
				( \
					SELECT heritage.symbol_id_base \
					FROM heritage \
					WHERE heritage.symbol_id_derived = ## /* name:'symbolid' type:gint */ \
				)) ";
			break;
		case IANJUTA_SYMBOL_QUERY_SEARCH_SCOPE:
			condition =
				"(file.file_path = ## /* name:'filepath' type:gchararray */ \
				 AND symbol.file_position <= ## /* name:'fileline' type:gint */)  \
				 ORDER BY symbol.file_position DESC ";
			sdb_query_add_field (query, IANJUTA_SYMBOL_FIELD_FILE_PATH);
			g_object_set (query, "limit", 1, NULL);
			break;
		case IANJUTA_SYMBOL_QUERY_SEARCH_PARENT_SCOPE:
			condition =
				"(symbol.scope_definition_id IN \
				( \
					SELECT symbol.scope_id \
					FROM symbol \
					WHERE symbol.symbol_id = ## /* name:'symbolid' type:gint */ \
				)) ";
			g_object_set (query, "limit", 1, NULL);
			break;
		case IANJUTA_SYMBOL_QUERY_SEARCH_PARENT_SCOPE_FILE:
			condition =
				"(symbol.scope_definition_id IN \
				( \
					SELECT symbol.scope_id \
					FROM symbol \
					WHERE symbol.symbol_id = ## /* name:'symbolid' type:gint */ \
				) AND file.file_path = ## /* name:'filepath' type:gchararray */) ";
			sdb_query_add_field (query, IANJUTA_SYMBOL_FIELD_FILE_PATH);
			g_object_set (query, "limit", 1, NULL);
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

	/* Add condition of the SQL statement */
	g_string_append (sql, condition);

	/* Add symbol type filters of the SQL statement */
	sdb_query_build_sql_kind_filter (query, sql);
		
	/* Add filter for file scope */
	switch (priv->file_scope)
	{
		case IANJUTA_SYMBOL_QUERY_SEARCH_FS_IGNORE:
			break;
		case IANJUTA_SYMBOL_QUERY_SEARCH_FS_PRIVATE:
			g_string_append (sql, "AND (symbol.is_file_scope = 1) ");
			break;
		case IANJUTA_SYMBOL_QUERY_SEARCH_FS_PUBLIC:
			g_string_append (sql, "AND (symbol.is_file_scope = 0) ");
			break;
		default:
			g_warn_if_reached ();
	}

	/* Group by clause */
	if (priv->group_by != IANJUTA_SYMBOL_FIELD_END)
		g_string_append_printf (sql, "GROUP BY %s ", field_specs[priv->group_by].column);

	/* Order by clause */
	if (priv->order_by != IANJUTA_SYMBOL_FIELD_END)
		g_string_append_printf (sql, "ORDER BY %s ", field_specs[priv->order_by].column);
	
	/* Add tail of the SQL statement */
	g_string_append (sql, "LIMIT ## /* name:'limit' type:gint */ ");
	g_string_append (sql, "OFFSET ## /* name:'offset' type:gint */ ");

	/* Prepare statement */
	g_free (priv->sql_stmt);
	priv->sql_stmt = sql->str;
	if (priv->stmt) 
		g_object_unref (priv->stmt);

	/* If database is not connected, defer the statement compilation for later,
	 * otherwise compile it now.
	 */
	if (symbol_db_engine_is_connected (priv->dbe_selected))
		priv->stmt = symbol_db_engine_get_statement (priv->dbe_selected, sql->str);
	else
		priv->stmt = NULL;
	g_string_free (sql, FALSE);
}

/**
 * sdb_query_execute_real:
 * @query: The query
 *
 * Executes the query for real. If for some reason, the SQL statement wasn't
 * compiled before, it will be compiled now. Subsequent invocation would not
 * require recompilation, unless some parameters involved in SQL contruct has
 * been changed.
 * 
 * Returns: Result set iterator.
 */
static SymbolDBQueryResult*
sdb_query_execute_real (SymbolDBQuery *query)
{
	GdaDataModel *data_model;
	SymbolDBQueryPriv *priv = query->priv;

	if (!symbol_db_engine_is_connected (priv->dbe_selected))
	{
		g_warning ("Attempt to make a query when database is not connected");
		return GINT_TO_POINTER (-1);
	}
	if (symbol_db_engine_is_scanning (priv->dbe_selected))
		return GINT_TO_POINTER (-1);
	
	if (!priv->sql_stmt)
		sdb_query_update (query);
	else if (!priv->stmt)
		priv->stmt = symbol_db_engine_get_statement (priv->dbe_selected,
		                                             priv->sql_stmt);
	data_model = symbol_db_engine_execute_select (priv->dbe_selected,
	                                              priv->stmt,
	                                              priv->params);
	
	if (!data_model) return GINT_TO_POINTER (-1);
	return symbol_db_query_result_new (data_model, 
	                                   priv->fields,
	                                   symbol_db_engine_get_type_conversion_hash (priv->dbe_selected),
	                                   symbol_db_engine_get_project_directory (priv->dbe_selected));
}

static void
sdb_query_handle_result (SymbolDBQuery *query, SymbolDBQueryResult *result)
{
	if (GPOINTER_TO_INT (result) == -1)
	{
		g_warning ("Error in executing query");
		g_signal_emit_by_name (query, "async-result", NULL);
	}
	else
	{
		if (symbol_db_query_result_is_empty (result))
		{
			g_signal_emit_by_name (query, "async-result", NULL);
		}
		else
		{			
			g_signal_emit_by_name (query, "async-result", result);			
		}
		if (result)
			g_object_unref (result);
	}
}

/*
 * The callback from async command. Reduces the currently pending async
 * command invocation counts. If there are no more pending canceled commands
 * anymore, then emits the "async-result" signal (because its not covered by
 * the cancelation), otherwise, it reduces cancelable commands count.
 */
static gboolean
on_sdb_query_async_poll (gpointer data)
{
	SymbolDBQueryResult *result;
	SymbolDBQuery *query = SYMBOL_DB_QUERY (data);
	
	while ((result = g_async_queue_try_pop (query->priv->async_result_queue)))
	{
		query->priv->async_result_count++;
		if (query->priv->async_result_count > query->priv->async_cancel_count)
		{
			sdb_query_handle_result (query, result);
		}
	}
	if (query->priv->async_result_count < query->priv->async_run_count)
		return TRUE; /* More results coming */
	query->priv->async_poll_id = 0;
	return FALSE; /* No more results coming */
}

/**
 * sdb_query_async_run:
 * @command: The command.
 * 
 * Implementation of anjuta_command_run(). Runs the async command, presumably
 * from a different thread, and emits the data-arrived signal on command object.
 */
static gpointer
sdb_query_async_run (gpointer data)
{
	GAsyncQueue *queue;
	SymbolDBQuery *query = SYMBOL_DB_QUERY (data);
	SymbolDBQueryPriv *priv = query->priv;

	g_async_queue_ref (priv->async_result_queue);
	queue = priv->async_result_queue;
	g_async_queue_push (queue, sdb_query_execute_real (query));
	g_async_queue_unref (queue);

	g_object_unref (query);
	return 0;
}

/**
 * sdb_query_cancel:
 * @command: The async command.
 * 
 * Implementation of anjuta_command_cancel().
 * Cancels any currently executing async commands. Puts the currently pending
 * async invocation count into pending cancel count, which will be eventually
 * counted down to 0 without emitting "async-result" signal. Also, clears
 * any pending query queue (for queued mode).
 */
static void
sdb_query_async_cancel (IAnjutaSymbolQuery *query, GError **err)
{
	SymbolDBQueryPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_QUERY (query));
	priv = SYMBOL_DB_QUERY (query)->priv;
	
	g_return_if_fail (priv->mode != IANJUTA_SYMBOL_QUERY_MODE_SYNC);
	priv->async_cancel_count = priv->async_run_count;
	priv->query_queued = FALSE;
}

/* When the DB is connected, any pending SQL statement will be compiled */
static void
on_sdb_query_dbe_connected (SymbolDBEngine *dbe, SymbolDBQuery *query)
{
	g_return_if_fail (SYMBOL_DB_IS_QUERY (query));

	if (!query->priv->stmt && query->priv->sql_stmt)
	{
		query->priv->stmt =
			symbol_db_engine_get_statement (query->priv->dbe_selected,
			                                query->priv->sql_stmt);
	}
}

static void
on_sdb_query_dbe_disconnected (SymbolDBEngine *dbe, SymbolDBQuery *query)
{
}

/* The callback for DB scan-end. Any queued query will be executed here */
static void
on_sdb_query_dbe_scan_end (SymbolDBEngine *dbe, gint something,
                           SymbolDBQuery *query)
{
	g_return_if_fail (SYMBOL_DB_IS_QUERY (query));
	
	if (query->priv->mode == IANJUTA_SYMBOL_QUERY_MODE_QUEUED &&
	    query->priv->query_queued &&
	    !symbol_db_engine_is_scanning (query->priv->dbe_selected))
	{
		sdb_query_handle_result (query, sdb_query_execute_real (query));
		query->priv->query_queued = FALSE;
	}
}

/**
 * sdb_query_execute:
 * @query: The query
 *
 * Executes the query. If the query is in sync mode, the query is executed
 * immediately. If the query is in async mode, an async command is started
 * and pending async count increased. If the query is in queued mode, the
 * query is executed immediately if the DB is not busy scanning, otherwise,
 * it is defered until the DB is done scanning (at which point "async-result"
 * will be emitted).
 *
 * Returns: The resultset iterator for sync or successful queued queries,
 * otherwise returns NULL for async or unsuccessful queued queries (their
 * results will be delivered via "async-result" signal).
 */
static IAnjutaIterable*
sdb_query_execute (SymbolDBQuery *query)
{
	SymbolDBQueryResult *result;
	switch (query->priv->mode)
	{
		case IANJUTA_SYMBOL_QUERY_MODE_SYNC:
			result = sdb_query_execute_real (query);

			if (GPOINTER_TO_INT (result) == -1) /* Error trap */
				return NULL;

			/* Empty resultset is useless for us. Return NULL instead */
			if (result == NULL || symbol_db_query_result_is_empty (result))
			{
				if (result != NULL)
					g_object_unref (result);
				return NULL;
			}
			return IANJUTA_ITERABLE (result);
		case IANJUTA_SYMBOL_QUERY_MODE_ASYNC:
			query->priv->async_run_count++;
			if (query->priv->async_poll_id == 0)
			{
				query->priv->async_poll_id =
					g_idle_add (on_sdb_query_async_poll, query);
			}
			g_thread_new ("SymbolDBQuery Thread", sdb_query_async_run,
			              g_object_ref (query));
			return NULL;
		case IANJUTA_SYMBOL_QUERY_MODE_QUEUED:
			query->priv->query_queued = TRUE;

			/* FIXME: shoudn't be called with g_idle_add ? */
			on_sdb_query_dbe_scan_end (NULL, 0, query);
			break;
		default:
			g_warn_if_reached ();
	}
	return NULL;
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

	priv->group_by = IANJUTA_SYMBOL_FIELD_END;
	priv->order_by = IANJUTA_SYMBOL_FIELD_END;
	
	/* Prepare sql parameter holders */
	param = priv->param_pattern = gda_holder_new_string ("pattern", "");
	param_holders = g_slist_prepend (param_holders, param);
	
	param = priv->param_file_path = gda_holder_new_string ("filepath", "");
	param_holders = g_slist_prepend (param_holders, param);

	param = priv->param_limit = gda_holder_new_int ("limit", INT_MAX);
	param_holders = g_slist_prepend (param_holders, param);

	param = priv->param_offset = gda_holder_new_int ("offset", 0);
	param_holders = g_slist_prepend (param_holders, param);

	param = priv->param_id = gda_holder_new_int ("symbolid", 0);
	param_holders = g_slist_prepend (param_holders, param);

	param = priv->param_file_line = gda_holder_new_int ("fileline", 0);
	param_holders = g_slist_prepend (param_holders, param);

	priv->params = gda_set_new (param_holders);
	g_slist_free (param_holders);

	/* Prepare async signals */
	priv->async_run_count = 0;
	priv->async_cancel_count = 0;
	priv->async_result_count = 0;
	priv->query_queued = FALSE;

	priv->async_result_queue = g_async_queue_new ();
}

static void
sdb_query_dispose (GObject *object)
{
	SymbolDBQueryPriv *priv;

	priv = SYMBOL_DB_QUERY (object)->priv;
	if (priv->dbe_selected)
	{
		g_signal_handlers_disconnect_by_func (priv->dbe_selected,
		                                      on_sdb_query_dbe_scan_end,
		                                      object);
		g_signal_handlers_disconnect_by_func (priv->dbe_selected,
		                                      on_sdb_query_dbe_connected,
		                                      object);
		g_signal_handlers_disconnect_by_func (priv->dbe_selected,
		                                      on_sdb_query_dbe_disconnected,
		                                      object);
		/* no need to unref dbe_selected */
		priv->dbe_selected = NULL;
	}
	if (priv->dbe_system)
	{
		g_object_unref (priv->dbe_system);
		priv->dbe_system = NULL;
	}
	if (priv->dbe_project)
	{
		g_object_unref (priv->dbe_project);
		priv->dbe_project = NULL;
	}
	if (priv->stmt)
	{
		g_object_unref (priv->stmt);
		priv->stmt = NULL;
	}
	if (priv->params)
	{
		g_object_unref (priv->params);
		priv->params = NULL;
	}
	if (priv->async_result_queue)
	{
		g_async_queue_unref (priv->async_result_queue);
		priv->async_result_queue = NULL;
	}
	if (priv->async_poll_id)
	{
		g_warning ("There are still running async threads!");
	    g_source_remove (priv->async_poll_id);
	}
	G_OBJECT_CLASS (sdb_query_parent_class)->dispose (object);
}

static void
sdb_query_finalize (GObject *object)
{
	SymbolDBQueryPriv *priv;

	priv = SYMBOL_DB_QUERY (object)->priv;
	g_free (priv->sql_stmt);
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
		sdb_query_reset (query);
		break;
	case PROP_QUERY_MODE:
		priv->mode = g_value_get_enum (value);
		break;
	case PROP_FILTERS:
		priv->filters = g_value_get_int (value);
		sdb_query_reset (query);
		break;
	case PROP_FILE_SCOPE:
		priv->file_scope = g_value_get_enum (value);
		sdb_query_reset (query);
		break;
	case PROP_LIMIT:
		gda_holder_set_value (priv->param_limit, value, NULL);
		break;
	case PROP_OFFSET:
		gda_holder_set_value (priv->param_offset, value, NULL);
		break;
	case PROP_GROUP_BY:
		priv->group_by = g_value_get_enum (value);
		sdb_query_reset (query);
		break;
	case PROP_ORDER_BY:
		priv->group_by = g_value_get_enum (value);
		sdb_query_reset (query);
		break;
	case PROP_DB_ENGINE_SYSTEM:
		g_assert (priv->dbe_system == NULL);
		priv->dbe_system = g_value_get_object (value);
		break;
	case PROP_DB_ENGINE_PROJECT:
		g_assert (priv->dbe_project == NULL);
		priv->dbe_project = g_value_get_object (value);
		break;
	case PROP_QUERY_DB:
		g_assert (priv->dbe_project != NULL);
		g_assert (priv->dbe_system != NULL);
		g_assert (priv->dbe_selected == NULL);
		switch (g_value_get_enum (value))
		{
			case IANJUTA_SYMBOL_QUERY_DB_PROJECT:
				priv->dbe_selected = priv->dbe_project;
				break;
			case IANJUTA_SYMBOL_QUERY_DB_SYSTEM:
				priv->dbe_selected = priv->dbe_system;
				break;
		}
		g_object_ref (priv->dbe_project);
		g_object_ref (priv->dbe_system);
			
		g_signal_connect (priv->dbe_selected, "scan-end",
		                  G_CALLBACK (on_sdb_query_dbe_scan_end), query);
		g_signal_connect (priv->dbe_selected, "db-connected",
		                  G_CALLBACK (on_sdb_query_dbe_connected), query);
		g_signal_connect (priv->dbe_selected, "db-disconnected",
		                  G_CALLBACK (on_sdb_query_dbe_disconnected), query);
		break;
	case PROP_SESSION_PACKAGES:
		priv->session_packages = g_value_get_pointer (value);
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
		g_value_set_int (value, priv->filters);
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
	case PROP_GROUP_BY:
		g_value_set_enum (value, priv->group_by);
		break;
	case PROP_ORDER_BY:
		g_value_set_enum (value, priv->order_by);
		break;
	case PROP_DB_ENGINE_SYSTEM:
		g_value_set_object (value, priv->dbe_system);
		break;
	case PROP_DB_ENGINE_PROJECT:
		g_value_set_object (value, priv->dbe_project);
		break;
	case PROP_DB_ENGINE_SELECTED:
		g_value_set_object (value, priv->dbe_selected);
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
	object_class->dispose = sdb_query_dispose;
	object_class->set_property = sdb_query_set_property;
	object_class->get_property = sdb_query_get_property;

	g_object_class_install_property (object_class,
	                                 PROP_DB_ENGINE_SYSTEM,
	                                 g_param_spec_object ("db-engine-system",
	                                                      "System DB Engine",
	                                                      "The System SymbolDBEngine",
	                                                      SYMBOL_TYPE_DB_ENGINE,
	                                                      G_PARAM_READABLE |
	                                                      G_PARAM_WRITABLE |
	                                                      G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
	                                 PROP_DB_ENGINE_PROJECT,
	                                 g_param_spec_object ("db-engine-project",
	                                                      "Project DB Engine",
	                                                      "The Project SymbolDBEngine",
	                                                      SYMBOL_TYPE_DB_ENGINE,
	                                                      G_PARAM_READABLE |
	                                                      G_PARAM_WRITABLE |
	                                                      G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (object_class,
	                                 PROP_QUERY_NAME,
	                                 g_param_spec_enum ("query-name",
	                                                    "Query name",
	                                                    "The query name",
	                                                    IANJUTA_TYPE_SYMBOL_QUERY_NAME,
	                                                    IANJUTA_SYMBOL_QUERY_SEARCH,
	                                                    G_PARAM_READABLE |
	                                                    G_PARAM_WRITABLE |
	                                                    G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
	                                 PROP_QUERY_DB,
	                                 g_param_spec_enum ("query-db",
	                                                    "Query DB",
	                                                    "The query database",
	                                                    IANJUTA_TYPE_SYMBOL_QUERY_DB,
	                                                    IANJUTA_SYMBOL_QUERY_DB_PROJECT,
	                                                    G_PARAM_WRITABLE |
	                                                    G_PARAM_CONSTRUCT_ONLY));
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
	                                 g_param_spec_int ("filters",
	                                                   "Symbol type filters",
	                                                   "The symbol type filters",
	                                                   IANJUTA_SYMBOL_TYPE_NONE,
	                                                   IANJUTA_SYMBOL_TYPE_MAX,
	                                                   IANJUTA_SYMBOL_TYPE_NONE,
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
	                                 PROP_GROUP_BY,
	                                 g_param_spec_enum ("group-by",
	                                                    "Query group by",
	                                                    "Group by given field",
	                                                    IANJUTA_TYPE_SYMBOL_FIELD,
	                                                    IANJUTA_SYMBOL_FIELD_END,
	                                                    G_PARAM_READABLE |
	                                                    G_PARAM_WRITABLE));
	g_object_class_install_property (object_class,
	                                 PROP_ORDER_BY,
	                                 g_param_spec_enum ("order-by",
	                                                    "Query order by",
	                                                    "Query order by given field",
	                                                    IANJUTA_TYPE_SYMBOL_FIELD,
	                                                    IANJUTA_SYMBOL_FIELD_END,
	                                                    G_PARAM_READABLE |
	                                                    G_PARAM_WRITABLE));	

	g_object_class_install_property (object_class,
	                                 PROP_DB_ENGINE_SELECTED,
	                                 g_param_spec_object ("db-engine-selected",
	                                                      "Selected DB Engine",
	                                                      "The selected SymbolDBEngine",
	                                                      SYMBOL_TYPE_DB_ENGINE,
	                                                      G_PARAM_READABLE));

	g_object_class_install_property (object_class,
	                                 PROP_SESSION_PACKAGES,
	                                 g_param_spec_pointer ("session-packages",
	                                                       "Session Packages",
	                                                       "The session packages",
	                                                       G_PARAM_READABLE |
	                                                       G_PARAM_WRITABLE));
	
	// FIXMEPROP_SESSION_PACKAGES	
}

/* IAnjutaSymbolQuery implementation */

static void
sdb_query_set_fields (IAnjutaSymbolQuery *query, gint n_fields,
                      IAnjutaSymbolField *fields, GError **err)
{
	gint i;
	SymbolDBQueryPriv *priv;

	g_return_if_fail (SYMBOL_DB_IS_QUERY (query));

	priv = SYMBOL_DB_QUERY (query)->priv;

	/* Transfer the given list of column fields to internal array */
	for (i = 0; i < n_fields; i++)
		priv->fields[i] = fields[i];
	priv->fields[i] = IANJUTA_SYMBOL_FIELD_END;
	sdb_query_reset (SYMBOL_DB_QUERY (query));
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
sdb_query_set_group_by (IAnjutaSymbolQuery *query, IAnjutaSymbolField field, GError **err)
{
	g_object_set (query, "group-by", field, NULL);
}

static void
sdb_query_set_order_by (IAnjutaSymbolQuery *query, IAnjutaSymbolField field, GError **err)
{
	g_object_set (query, "order-by", field, NULL);
}

static void
sdb_query_set_file_scope (IAnjutaSymbolQuery *query,
                          IAnjutaSymbolQueryFileScope file_scope,
                          GError **err)
{
	g_object_set (query, "file-scope", file_scope, NULL);
}

/* Search queries */


static IAnjutaIterable*
sdb_query_search (IAnjutaSymbolQuery *query, const gchar *search_string,
                  GError **error)
{
	SDB_QUERY_SEARCH_HEADER;
	g_return_val_if_fail (priv->name == IANJUTA_SYMBOL_QUERY_SEARCH, NULL);
	SDB_PARAM_SET_STATIC_STRING (priv->param_pattern, search_string);
	return sdb_query_execute (SYMBOL_DB_QUERY (query));
}

static IAnjutaIterable*
sdb_query_search_all (IAnjutaSymbolQuery *query, GError **error)
{
	SDB_QUERY_SEARCH_HEADER;
	g_return_val_if_fail (priv->name == IANJUTA_SYMBOL_QUERY_SEARCH_ALL, NULL);
	return sdb_query_execute (SYMBOL_DB_QUERY (query));
}

static IAnjutaIterable*
sdb_query_search_file (IAnjutaSymbolQuery *query, const gchar *search_string,
                       const GFile *file, GError **error)
{
	const gchar *rel_file_path;
	gchar *abs_file_path;
	SDB_QUERY_SEARCH_HEADER;
	g_return_val_if_fail (priv->name == IANJUTA_SYMBOL_QUERY_SEARCH_FILE, NULL);

	abs_file_path = g_file_get_path ((GFile*)file);
	rel_file_path = symbol_db_util_get_file_db_path (priv->dbe_selected, abs_file_path);

	SDB_PARAM_SET_STATIC_STRING (priv->param_pattern, search_string);
	SDB_PARAM_SET_STATIC_STRING (priv->param_file_path, rel_file_path);
	g_free (abs_file_path);
	return sdb_query_execute (SYMBOL_DB_QUERY (query));
}

static IAnjutaIterable*
sdb_query_search_in_scope (IAnjutaSymbolQuery *query, const gchar *search_string,
                           IAnjutaSymbol *scope, GError **error)
{
	SDB_QUERY_SEARCH_HEADER;
	g_return_val_if_fail (priv->name == IANJUTA_SYMBOL_QUERY_SEARCH_IN_SCOPE, NULL);
	SDB_PARAM_SET_STATIC_STRING (priv->param_pattern, search_string);
	SDB_PARAM_SET_INT (priv->param_id, ianjuta_symbol_get_int (scope, IANJUTA_SYMBOL_FIELD_ID, NULL));
	return sdb_query_execute (SYMBOL_DB_QUERY (query));
}

static IAnjutaIterable*
sdb_query_search_id (IAnjutaSymbolQuery *query, gint symbol_id,
                     GError **error)
{
	SDB_QUERY_SEARCH_HEADER;
	g_return_val_if_fail (symbol_id > 0, NULL);
	g_return_val_if_fail (priv->name == IANJUTA_SYMBOL_QUERY_SEARCH_ID, NULL);

	SDB_PARAM_SET_INT (priv->param_id, symbol_id);
	return sdb_query_execute (SYMBOL_DB_QUERY (query));
}

static IAnjutaIterable*
sdb_query_search_members (IAnjutaSymbolQuery *query, IAnjutaSymbol *symbol,
                          GError **error)
{
	SDB_QUERY_SEARCH_HEADER;
	g_return_val_if_fail (priv->name == IANJUTA_SYMBOL_QUERY_SEARCH_MEMBERS, NULL);

	SDB_PARAM_SET_INT (priv->param_id, ianjuta_symbol_get_int (symbol, IANJUTA_SYMBOL_FIELD_ID, NULL));
	return sdb_query_execute (SYMBOL_DB_QUERY (query));
}

static IAnjutaIterable*
sdb_query_search_class_parents (IAnjutaSymbolQuery *query, IAnjutaSymbol *symbol,
                                GError **error)
{
	SDB_QUERY_SEARCH_HEADER;
	g_return_val_if_fail (priv->name == IANJUTA_SYMBOL_QUERY_SEARCH_CLASS_PARENTS, NULL);

	SDB_PARAM_SET_INT (priv->param_id, ianjuta_symbol_get_int (symbol, IANJUTA_SYMBOL_FIELD_ID, NULL));
	return sdb_query_execute (SYMBOL_DB_QUERY (query));
}

static IAnjutaIterable*
sdb_query_search_scope (IAnjutaSymbolQuery *query, const gchar *file_path,
                        gint file_line, GError **error)
{
	const gchar *db_relative_path;
	SDB_QUERY_SEARCH_HEADER;
	g_return_val_if_fail (priv->name == IANJUTA_SYMBOL_QUERY_SEARCH_SCOPE, NULL);

	db_relative_path = symbol_db_util_get_file_db_path (priv->dbe_selected, file_path);
	if (db_relative_path == NULL)
		return NULL;

	SDB_PARAM_SET_INT (priv->param_file_line, file_line);
	SDB_PARAM_SET_STATIC_STRING (priv->param_file_path, db_relative_path);
	return sdb_query_execute (SYMBOL_DB_QUERY (query));
}

static IAnjutaIterable*
sdb_query_search_parent_scope (IAnjutaSymbolQuery *query, IAnjutaSymbol *symbol,
                               GError **error)
{
	SDB_QUERY_SEARCH_HEADER;
	g_return_val_if_fail (priv->name == IANJUTA_SYMBOL_QUERY_SEARCH_PARENT_SCOPE, NULL);

	SDB_PARAM_SET_INT (priv->param_id, ianjuta_symbol_get_int (symbol, IANJUTA_SYMBOL_FIELD_ID, NULL));
	return sdb_query_execute (SYMBOL_DB_QUERY (query));
}

static IAnjutaIterable*
sdb_query_search_parent_scope_file (IAnjutaSymbolQuery *query, IAnjutaSymbol *symbol,
                               const gchar *file_path, GError **error)
{
	const gchar *db_relative_path;
	SDB_QUERY_SEARCH_HEADER;
	g_return_val_if_fail (priv->name == IANJUTA_SYMBOL_QUERY_SEARCH_PARENT_SCOPE_FILE, NULL);

	db_relative_path = symbol_db_util_get_file_db_path (priv->dbe_selected, file_path);
	if (db_relative_path == NULL)
		return NULL;
	
	SDB_PARAM_SET_INT (priv->param_id, ianjuta_symbol_get_int (symbol, IANJUTA_SYMBOL_FIELD_ID, NULL));
	SDB_PARAM_SET_STATIC_STRING (priv->param_file_path, db_relative_path);
	return sdb_query_execute (SYMBOL_DB_QUERY (query));
}

static void
ianjuta_symbol_query_iface_init (IAnjutaSymbolQueryIface *iface)
{
	iface->set_fields = sdb_query_set_fields;
	iface->set_mode = sdb_query_set_mode;
	iface->set_filters = sdb_query_set_filters;
	iface->set_file_scope = sdb_query_set_file_scope;
	iface->set_limit = sdb_query_set_limit;
	iface->set_offset = sdb_query_set_offset;
	iface->set_group_by = sdb_query_set_group_by;
	iface->set_order_by = sdb_query_set_order_by;
	iface->cancel = sdb_query_async_cancel;
	iface->search = sdb_query_search;
	iface->search_all = sdb_query_search_all;
	iface->search_file = sdb_query_search_file;
	iface->search_in_scope = sdb_query_search_in_scope;
	iface->search_id = sdb_query_search_id;
	iface->search_members = sdb_query_search_members;
	iface->search_class_parents = sdb_query_search_class_parents;
	iface->search_scope = sdb_query_search_scope;
	iface->search_parent_scope = sdb_query_search_parent_scope;
	iface->search_parent_scope_file = sdb_query_search_parent_scope_file;
}

SymbolDBQuery *
symbol_db_query_new (SymbolDBEngine *system_db_engine,
                     SymbolDBEngine *project_db_engine,
                     IAnjutaSymbolQueryName name,
                     IAnjutaSymbolQueryDb db,
                 	 GHashTable *session_packages)
{
	return g_object_new (SYMBOL_DB_TYPE_QUERY,
	                     "db-engine-system", system_db_engine,
	                     "db-engine-project", project_db_engine,
	                     "query-db", db, 
	                     "query-name", name,
	                     "session-packages", session_packages,
	                      NULL);
}
