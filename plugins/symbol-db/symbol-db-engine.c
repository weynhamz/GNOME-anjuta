/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Massimo Cora' 2007 <maxcvs@email.it>
 * 
 * anjuta is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

/*

interesting queries:

------------------------
* get all namespaces.
select symbol.name from symbol join sym_kind on symbol.kind_id = 
	sym_kind.sym_kind_id where sym_kind.kind_name = "namespace";

------------------------
* get all symbols_id which scope is under all namespaces' ones
select * from symbol where scope_id in (select symbol.scope_definition_id 
	from symbol join sym_kind on symbol.kind_id = sym_kind.sym_kind_id where 
	sym_kind.kind_name = "namespace");

------------------------
* get all symbols which have a scope_id of symbol X. X is a symbol of kind namespace,
class, struct etc. Symbol X can be retrieved by something like
select * from symbol join sym_type on symbol.type_id = sym_type.type_id where 
symbol.name = "First" and sym_type.type_type = "namespace";
our query is:
select * from symbol where scope_id = ;
at the end we have:

select * from symbol where scope_id = (select scope_definition_id from symbol join 
	sym_type on symbol.type_id = sym_type.type_id where symbol.name = 
	"First" and sym_type.type_type = "namespace");


------------------------
* get a symbol by its name and type. In this case we want to search for the
  class Fourth_2_class
select * from symbol join sym_type on symbol.type_id = sym_type.type_id where 
	symbol.name = "Fourth_2_class" and sym_type.type = "class";

sqlite> select * from symbol join sym_kind on symbol.kind_id = sym_kind.sym_kind_id  
			join scope on scope.scope_id = symbol.scope_id 
			join sym_type on sym_type.type_id = scope.type_id 
		where symbol.name = "Fourth_2_class" 
			and sym_kind.kind_name = "class" 
			and scope = "Fourth" 
			and sym_type.type = "namespace";

183|13|Fourth_2_class|52|0||140|137|175|8|-1|-1|0|8|class|137|Fourth|172|172|namespace|Fourth

//// alternativa ////
= get the *derived symbol*
select * from symbol 
	join sym_kind on symbol.kind_id = sym_kind.sym_kind_id 
	where symbol.name = "Fourth_2_class" 
		and sym_kind.kind_name = "class" 
		and symbol.scope_id in (select scope.scope_id from scope 
									join sym_type on scope.type_id = sym_type.type_id 
									where sym_type.type = 'namespace' 
										and sym_type.type_name = 'Fourth');

query that get the symbol's parent classes

select symbol_id_base, symbol.name from heritage 
	join symbol on heritage.symbol_id_base = symbol.symbol_id 
	where symbol_id_derived = (
		select symbol_id from symbol 
			join sym_kind on symbol.kind_id = sym_kind.sym_kind_id 
			where symbol.name = "Fourth_2_class" 
				and sym_kind.kind_name = "class" 
				and symbol.scope_id in (
					select scope.scope_id from scope 
						join sym_type on scope.type_id = sym_type.type_id 
						where sym_type.type = 'namespace' 
							and sym_type.type_name = 'Fourth'
					)
		);


182|Fourth_1_class

*/


#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <signal.h>
#include <fcntl.h>           /* For O_* constants */
#include <string.h>

#include <libgnomevfs/gnome-vfs.h>
#include <libanjuta/interfaces/ianjuta-symbol.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-launcher.h>
#include <libanjuta/anjuta-utils.h>
#include <libgda/libgda.h>
#include <sql-parser/gda-sql-parser.h>
#include "readtags.h"
#include "symbol-db-engine.h"
#include "symbol-db-engine-iterator.h"


/* file should be specified without the ".db" extension. */
#define ANJUTA_DB_FILE	".anjuta_sym_db"

#define TABLES_SQL	PACKAGE_DATA_DIR"/tables.sql"

#define CTAGS_MARKER	"#_#\n"

#define SHARED_MEMORY_PREFIX		"/dev/shm"

// FIXME: detect it by prefs
#define CTAGS_PATH		"/usr/bin/ctags"

#define THREADS_MONITOR_TIMEOUT			10
#define THREADS_MAX_CONCURRENT			15
#define TRIGGER_SIGNALS_DELAY			100
#define	TRIGGER_MAX_CLOSURE_RETRIES		50
#define	THREAD_MAX_CLOSURE_RETRIES		20

enum {
	DO_UPDATE_SYMS = 1,
	DO_UPDATE_SYMS_AND_EXIT,
	DONT_UPDATE_SYMS,
	DONT_UPDATE_SYMS_AND_EXIT,
	DONT_FAKE_UPDATE_SYMS,
	END_UPDATE_GROUP_SYMS
};

typedef enum
{
	PREP_QUERY_WORKSPACE_NEW = 0,
	PREP_QUERY_GET_WORKSPACE_ID_BY_UNIQUE_NAME,
	PREP_QUERY_PROJECT_NEW,
	PREP_QUERY_GET_PROJECT_ID_BY_UNIQUE_NAME,
	PREP_QUERY_UPDATE_PROJECT_ANALYSE_TIME,
	PREP_QUERY_FILE_NEW,
	PREP_QUERY_GET_FILE_ID_BY_UNIQUE_NAME,
	PREP_QUERY_GET_ALL_FROM_FILE_BY_PROJECT_NAME,
	PREP_QUERY_GET_ALL_FROM_FILE_BY_PROJECT_ID,
	PREP_QUERY_UPDATE_FILE_ANALYSE_TIME,
	PREP_QUERY_GET_ALL_FROM_FILE_WHERE_NOT_IN_SYMBOLS,
	PREP_QUERY_LANGUAGE_NEW,
	PREP_QUERY_GET_LANGUAGE_ID_BY_UNIQUE_NAME,
	PREP_QUERY_SYM_TYPE_NEW,
	PREP_QUERY_GET_SYM_TYPE_ID,
	PREP_QUERY_SYM_KIND_NEW,
	PREP_QUERY_GET_SYM_KIND_BY_UNIQUE_NAME,
	PREP_QUERY_SYM_ACCESS_NEW,
	PREP_QUERY_GET_SYM_ACCESS_BY_UNIQUE_NAME,
	PREP_QUERY_SYM_IMPLEMENTATION_NEW,
	PREP_QUERY_GET_SYM_IMPLEMENTATION_BY_UNIQUE_NAME,
	PREP_QUERY_HERITAGE_NEW,
	PREP_QUERY_SCOPE_NEW,
	PREP_QUERY_GET_SCOPE_ID,
	PREP_QUERY_GET_PARENT_SCOPE_ID_BY_SYMBOL_ID_NO_FILE,
	PREP_QUERY_GET_PARENT_SCOPE_ID_BY_SYMBOL_ID,
	PREP_QUERY_GET_PARENT_SCOPE_ID_BY_SYMBOL_ID_BY_SYMBOL_ID,
	PREP_QUERY_GET_SCOPE_DEFINITION_ID_BY_WALK_DOWN_SCOPE_PATH,
	PREP_QUERY_TMP_HERITAGE_NEW,
	PREP_QUERY_GET_ALL_FROM_TMP_HERITAGE,
	PREP_QUERY_GET_ALL_FROM_TMP_HERITAGE_WITH_INHERITS,
	PREP_QUERY_TMP_HERITAGE_DELETE_ALL,
	PREP_QUERY_SYMBOL_NEW,
	PREP_QUERY_GET_SYMBOL_SCOPE_DEFINITION_ID,
	PREP_QUERY_GET_SYMBOL_ID_BY_CLASS_NAME,
	PREP_QUERY_GET_SYMBOL_ID_BY_CLASS_NAME_AND_NAMESPACE,
	PREP_QUERY_UPDATE_SYMBOL_SCOPE_ID,
	PREP_QUERY_GET_SYMBOL_ID_BY_UNIQUE_INDEX_KEY,
	PREP_QUERY_UPDATE_SYMBOL_ALL,
	PREP_QUERY_REMOVE_NON_UPDATED_SYMBOLS,
	PREP_QUERY_RESET_UPDATE_FLAG_SYMBOLS,
	PREP_QUERY_GET_REMOVED_IDS,
	PREP_QUERY_TMP_REMOVED_DELETE_ALL,
	PREP_QUERY_COUNT
		
} static_query_type;

typedef struct _static_query_node
{
	static_query_type query_id;
	const gchar *query_str;
	GdaStatement *stmt;
	GdaSet *plist;

} static_query_node;

#define STATIC_QUERY_POPULATE_INIT_NODE(query_list_ptr, query_type, gda_stmt) { \
	static_query_node *q = g_new0 (static_query_node, 1); \
	q->query_id = query_type; \
	q->query_str = gda_stmt; \
	q->stmt = NULL; \
	q->plist = NULL; \
	query_list_ptr [query_type] = q; \
}


typedef enum {
	DYN_PREP_QUERY_GET_CLASS_PARENTS = 0,
	DYN_PREP_QUERY_GET_CLASS_PARENTS_BY_SYMBOL_ID,
	DYN_PREP_QUERY_GET_GLOBAL_MEMBERS_FILTERED,
	DYN_PREP_QUERY_GET_SCOPE_MEMBERS,
	DYN_PREP_QUERY_GET_CURRENT_SCOPE,
	DYN_PREP_QUERY_GET_FILE_SYMBOLS,
	DYN_PREP_QUERY_GET_SYMBOL_INFO_BY_ID,
	DYN_PREP_QUERY_FIND_SYMBOL_NAME_BY_PATTERN,
	DYN_PREP_QUERY_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED,
	DYN_PREP_QUERY_GET_SCOPE_MEMBERS_BY_SYMBOL_ID,
	DYN_PREP_QUERY_GET_SCOPE_MEMBERS_BY_SYMBOL_ID_FILTERED,
	DYN_PREP_QUERY_COUNT
		
} dyn_query_type;


typedef struct _dyn_query_node {
	dyn_query_type dyn_query_id;
	GTree * sym_extra_info_gtree;
	gboolean has_gtree_child;
	
} dyn_query_node;

typedef struct _ChildDynQueryNode {
	gchar *query_str;
	GdaStatement *stmt;
	GdaSet *plist;	
	
} DynChildQueryNode;

#define DYN_QUERY_POPULATE_INIT_NODE(query_list_ptr, query_type, gtree_child) { \
	dyn_query_node *q = g_new0 (dyn_query_node, 1); \
	q->dyn_query_id = query_type; \
	q->sym_extra_info_gtree = NULL; \
	q->has_gtree_child = gtree_child; \
	query_list_ptr [query_type] = q; \
}


typedef void (SymbolDBEngineCallback) (SymbolDBEngine * dbe,
									   gpointer user_data);

enum
{
	SINGLE_FILE_SCAN_END,
	SCAN_END,
	SYMBOL_INSERTED,
	SYMBOL_UPDATED,
	SYMBOL_SCOPE_UPDATED,
	SYMBOL_REMOVED,
	LAST_SIGNAL
};

static unsigned int signals[LAST_SIGNAL] = { 0 };

struct _SymbolDBEnginePriv
{
	GdaConnection *db_connection;
	GdaSqlParser *sql_parser;
	gchar *db_directory;
	gchar *project_directory;

	GAsyncQueue *scan_queue;	
	GAsyncQueue *updated_symbols_id;
	GAsyncQueue *inserted_symbols_id;
	
	gchar *shared_mem_str;
	FILE *shared_mem_file;
	gint shared_mem_fd;
	AnjutaLauncher *ctags_launcher;
	gboolean scanning_status;
	gboolean force_sym_update;
	
	GMutex* mutex;
	GAsyncQueue* signals_queue;
	GQueue* thread_list;
	
	gboolean thread_status;
	gint concurrent_threads;
	
	gint thread_monitor_handler;	
	gint timeout_trigger_handler;
	
	gint trigger_closure_retries;
	gint thread_closure_retries;
	
	GHashTable *sym_type_conversion_hash;
	GHashTable *garbage_shared_mem_files;
	
	/* Caches */
	GHashTable *kind_cache;
	GHashTable *access_cache;
	GHashTable *implementation_cache;
	
	static_query_node *static_query_list[PREP_QUERY_COUNT]; 
	dyn_query_node *dyn_query_list[DYN_PREP_QUERY_COUNT];
};

typedef struct _ThreadDataOutput {
	gchar * chars;
	gpointer user_data;
	
} ThreadDataOutput;

typedef struct _UpdateFileSymbolsData {	
	gchar *project;
	gboolean update_prj_analyse_time;
	GPtrArray * files_path;
	
} UpdateFileSymbolsData;


static GObjectClass *parent_class = NULL;

/* some forward declarations */
static void 
sdb_engine_second_pass_do (SymbolDBEngine * dbe);
static gint
sdb_engine_add_new_symbol (SymbolDBEngine * dbe, const tagEntry * tag_entry,
						   int file_defined_id,
						   gboolean sym_update);


static inline gint
sdb_engine_cache_lookup (GHashTable* hash_table, const gchar* lookup)
{
	gpointer orig_key = NULL;
	gpointer value = NULL;
	
	/* avoid lazy initialization may gain some cpu cycles. Just lookup here. */	
	if (g_hash_table_lookup_extended (hash_table, 
									  lookup,
									  &orig_key,
									  &value))
	{
		gint table_id = GPOINTER_TO_INT (value);
		return table_id;
	}
	return -1;
}

static inline void
sdb_engine_insert_cache (GHashTable* hash_table, const gchar* key,
						 gint value)
{
	g_hash_table_insert (hash_table, g_strdup (key), 
						 GINT_TO_POINTER (value));
}

static void
sdb_engine_clear_caches (SymbolDBEngine* dbe)
{
	SymbolDBEnginePriv *priv = dbe->priv;
	if (priv->kind_cache)
		g_hash_table_destroy (priv->kind_cache);
	if (priv->access_cache)
		g_hash_table_destroy (priv->access_cache);	
	if (priv->implementation_cache)
		g_hash_table_destroy (priv->implementation_cache);
	priv->kind_cache = NULL;
	priv->access_cache = NULL;
	priv->implementation_cache = NULL;
}

static void
sdb_engine_init_caches (SymbolDBEngine* dbe)
{
	SymbolDBEnginePriv *priv = dbe->priv;
	priv->kind_cache = g_hash_table_new_full (g_str_hash,
											g_str_equal,
											g_free,
											NULL);
	
	priv->access_cache = g_hash_table_new_full (g_str_hash,
											g_str_equal,
											g_free,
											NULL);
	
	priv->implementation_cache = g_hash_table_new_full (g_str_hash,
											g_str_equal,
											g_free,
											NULL);
}


static gboolean 
sdb_engine_execute_unknown_sql (SymbolDBEngine *dbe, const gchar *sql)
{
	GdaStatement *stmt;
	GObject *res;
	SymbolDBEnginePriv *priv;
	
	priv = dbe->priv;	
	
	stmt = gda_sql_parser_parse_string (priv->sql_parser, sql, NULL, NULL);	

	if (stmt == NULL)
		return FALSE;
	
    if ((res = gda_connection_statement_execute (priv->db_connection, 
												   (GdaStatement*)stmt, 
												 	NULL,
													GDA_STATEMENT_MODEL_RANDOM_ACCESS, 
													NULL, NULL)) == NULL)
	{
		g_object_unref (stmt);
		return FALSE;
	}
	else 
	{
		g_object_unref (res);
		g_object_unref (stmt);
		return TRUE;		
	}
}

static GdaDataModel *
sdb_engine_execute_select_sql (SymbolDBEngine * dbe, const gchar *sql)
{
	GdaStatement *stmt;
	GdaDataModel *res;
	SymbolDBEnginePriv *priv;
	const gchar *remain;
	
	priv = dbe->priv;	
	
	stmt = gda_sql_parser_parse_string (priv->sql_parser, sql, &remain, NULL);	

	if (stmt == NULL)
		return NULL;
	
    res = gda_connection_statement_execute_select (priv->db_connection, 
												   (GdaStatement*)stmt, NULL, NULL);
	if (!res) 
		DEBUG_PRINT ("Could not execute query: %s\n", sql);
	
	if (remain != NULL)
	{
		/* this shouldn't never happen */		
		sdb_engine_execute_select_sql (dbe, remain);
	}	
	
    g_object_unref (stmt);
	
	return res;
}

static gint
sdb_engine_execute_non_select_sql (SymbolDBEngine * dbe, const gchar *sql)
{
	GdaStatement *stmt;
    gint nrows;
	SymbolDBEnginePriv *priv;
	const gchar *remain;	
	
	priv = dbe->priv;
	stmt = gda_sql_parser_parse_string (priv->sql_parser, 
										sql, &remain, NULL);

	if (stmt == NULL)
		return -1;
	
	nrows = gda_connection_statement_execute_non_select (priv->db_connection, stmt, 
														 NULL, NULL, NULL);
	if (remain != NULL) {
		/* may happen for example when sql is a file-content */
		sdb_engine_execute_non_select_sql (dbe, remain);
	}
	
	g_object_unref (stmt);
	return nrows;
}

/**
 * Use a proxy to return an already present or a fresh new prepared query 
 * from static 'query_list'. We should perform actions in the fastest way, because
 * these queries are time-critical.
 * A GdaSet will also be populated once, avoiding so to create again later on.
 */
static inline const GdaStatement *
sdb_engine_get_statement_by_query_id (SymbolDBEngine * dbe, static_query_type query_id)
{
	static_query_node *node;
	SymbolDBEnginePriv *priv;

	priv = dbe->priv;

	/* no way: if connection is NULL we will break here. There must be
	 * a connection established to db before using this function */
	g_return_val_if_fail (priv->db_connection != NULL, NULL);
	
	node = priv->static_query_list[query_id];

	if (node->stmt == NULL)
	{
		DEBUG_PRINT ("generating new statement.. %d", query_id);
		/* create a new GdaStatement */
		node->stmt =
			gda_sql_parser_parse_string (priv->sql_parser, node->query_str, NULL, 
										 NULL);

		if (gda_statement_get_parameters ((GdaStatement*)node->stmt, 
										  &node->plist, NULL) == FALSE)
		{
			g_warning ("Error on getting parameters for %d", query_id);
		}
	}

	return node->stmt;
}


static gint
gtree_compare_func (gconstpointer a, gconstpointer b, gpointer user_data)
{
	return (gint)a - (gint)b;
}

/**
 *
 */
static inline const DynChildQueryNode *
sdb_engine_get_dyn_query_node_by_id (SymbolDBEngine *dbe, dyn_query_type query_id,
									 SymExtraInfo sym_info, gint other_parameters)
{
	dyn_query_node *node;
	SymbolDBEnginePriv *priv;

	priv = dbe->priv;

	node = priv->dyn_query_list[query_id];

	if (node->sym_extra_info_gtree == NULL) 
	{
		/* we didn't find any extra info symbol, nor it has been added before */
		return NULL;
	}
	
	if (node->has_gtree_child == FALSE) 
	{
		/* use only sym_info as key, ignore other_parameters */
		return g_tree_lookup (node->sym_extra_info_gtree, (gpointer)sym_info);
	}
	else {
		GTree *child_gtree;

		child_gtree = g_tree_lookup (node->sym_extra_info_gtree, (gpointer)sym_info);
		if (child_gtree == NULL) {			
			DEBUG_PRINT ("child_gtree is null");
			return NULL;
		}
		
		DynChildQueryNode *result;
		result = g_tree_lookup (child_gtree, (gpointer)other_parameters);
		
		return result;
	}
}


static void
sdb_engine_dyn_child_query_node_destroy (gpointer data)
{
	DynChildQueryNode *node_to_destroy;
	
	node_to_destroy = (DynChildQueryNode *)data;

	g_free (node_to_destroy->query_str);
	g_object_unref (node_to_destroy->stmt);
	g_object_unref (node_to_destroy->plist);
	g_free (node_to_destroy);
}


/**
 * WARNING: We suppose that before calling this function we already checked with
 * sdb_engine_get_dyn_query_node_by_id () that the node we're going to add 
 * isn't yet inserted.
 */
static inline const DynChildQueryNode *
sdb_engine_insert_dyn_query_node_by_id (SymbolDBEngine *dbe, dyn_query_type query_id,
									 	SymExtraInfo sym_info, gint other_parameters,
										const gchar *sql)
{
	dyn_query_node *node;
	SymbolDBEnginePriv *priv;
	priv = dbe->priv;
	
	/* no way: if connection is NULL we will break here. There must be
	 * a connection established to db before using this function */
	g_return_val_if_fail (priv->db_connection != NULL, NULL);

	node = priv->dyn_query_list[query_id];
	
	if (node->sym_extra_info_gtree == NULL) 
	{
		/* lazy initialization */
		if (node->has_gtree_child == FALSE)
		{
			node->sym_extra_info_gtree = 
					g_tree_new_full ((GCompareDataFunc)&gtree_compare_func, 
										 NULL,
										 NULL,
										 sdb_engine_dyn_child_query_node_destroy);
		}
		else
		{
			node->sym_extra_info_gtree = g_tree_new_full ((GCompareDataFunc)&gtree_compare_func, 
										 NULL,
										 NULL,
										 (GDestroyNotify)g_tree_destroy);
		}
	}

	if (node->has_gtree_child == FALSE) 
	{
		/* main GTree has direct DynChildQueryNode* as its leaves, other parameters
		 * will be ignored
		 */
		DynChildQueryNode *dyn_node;
				
		dyn_node = g_new0 (DynChildQueryNode, 1);
		
		/* create a new GdaStatement */
		dyn_node->stmt =
			gda_sql_parser_parse_string (priv->sql_parser, sql, NULL, 
										 NULL);

		if (gda_statement_get_parameters ((GdaStatement*)dyn_node->stmt, 
										  &dyn_node->plist, NULL) == FALSE)
		{
			g_warning ("Error on getting parameters for dyn %d", query_id);
		}
		
		dyn_node->query_str = g_strdup (sql);
		
		DEBUG_PRINT ("inserting direct child into main_gtree");
		/* insert it into gtree, thanks */
		g_tree_insert (node->sym_extra_info_gtree, (gpointer)sym_info, dyn_node);
		
		/* return it */
		return dyn_node;
	}
	else {
		/* ok, this is a slightly more complex case */
		GTree *child_gtree;
		DynChildQueryNode *dyn_node;

		child_gtree = g_tree_new_full ((GCompareDataFunc)&gtree_compare_func,
									 NULL,
									 NULL, 
									 sdb_engine_dyn_child_query_node_destroy); 
				
		dyn_node = g_new0 (DynChildQueryNode, 1);
		
		/* create a new GdaStatement */
		dyn_node->stmt =
			gda_sql_parser_parse_string (priv->sql_parser, sql, NULL, 
										 NULL);

		if (gda_statement_get_parameters ((GdaStatement*)dyn_node->stmt, 
										  &dyn_node->plist, NULL) == FALSE)
		{
			g_warning ("Error on getting parameters for dyn %d", query_id);
		}
		
		dyn_node->query_str = g_strdup (sql);
		
		DEBUG_PRINT ("inserting child into main_gtree's gtree child, "
					 "other_parameters %d, sym_info %d, dyn_node %p", 
					 other_parameters, sym_info, dyn_node);
		
		/* insert the dyn_node into child_gtree, then child_gtree into main_gtree */
		g_tree_insert (child_gtree, (gpointer)other_parameters, dyn_node);
		
		g_tree_insert (node->sym_extra_info_gtree, (gpointer)sym_info, child_gtree);
		
		/* return it */
		return dyn_node;		
	}
	
}

/**
 * Return a GdaSet of parameters calculated from the statement. It does not check
 * if it's null. You *must* be sure to have called sdb_engine_get_statement_by_query_id () first.
 */
static inline const GdaSet *
sdb_engine_get_query_parameters_list (SymbolDBEngine *dbe, static_query_type query_id)
{
	SymbolDBEnginePriv *priv;
	
	priv = dbe->priv;
	
	static_query_node *node;
	node = priv->static_query_list[query_id];
	return node->plist;
}

/**
 * Clear the static cached queries data. You should call this function when closing/
 * destroying SymbolDBEngine object.
 */
static void
sdb_engine_free_cached_queries (SymbolDBEngine *dbe)
{
	SymbolDBEnginePriv *priv;
	gint i;
	static_query_node *node;
	
	priv = dbe->priv;

	for (i = 0; i < PREP_QUERY_COUNT; i++)
	{
		node = priv->static_query_list[i];

		if (node != NULL && node->stmt != NULL)
		{
			g_object_unref (node->stmt);
			node->stmt = NULL;
		}
		
		if (node != NULL && node->plist != NULL)
		{
			g_object_unref (node->plist);
			node->plist = NULL;
		}
	}
}

static void
sdb_engine_free_cached_dynamic_queries (SymbolDBEngine *dbe)
{
	SymbolDBEnginePriv *priv;
	gint i;
	dyn_query_node *node;

	priv = dbe->priv;
	
	for (i = 0; i < DYN_PREP_QUERY_COUNT; i++)
	{
		node = priv->dyn_query_list[i];

		
		if (node != NULL && node->sym_extra_info_gtree != NULL)
		{
			g_object_unref (node->sym_extra_info_gtree );
			node->sym_extra_info_gtree = NULL;
		}		
	}
}

static gboolean
sdb_engine_disconnect_from_db (SymbolDBEngine * dbe)
{
	SymbolDBEnginePriv *priv;

	g_return_val_if_fail (dbe != NULL, FALSE);
	priv = dbe->priv;

	if (priv->db_connection != NULL)
		gda_connection_close (priv->db_connection);
	priv->db_connection = NULL;

	if (priv->sql_parser != NULL)
		g_object_unref (priv->sql_parser);
	
	g_free (priv->db_directory);
	priv->db_directory = NULL;
	
	g_free (priv->project_directory);
	priv->project_directory = NULL;
	
	return TRUE;
}

/**
 * @return -1 on error. Otherwise the id of tuple.
 */
static gint
sdb_engine_get_tuple_id_by_unique_name (SymbolDBEngine * dbe, static_query_type qtype,
										gchar * param_key,
										const GValue * param_value)
{
	const GdaSet *plist;
	const GdaStatement *stmt;
	GdaHolder *param;
	GdaDataModel *data_model;
	const GValue *num;
	gint table_id;
	SymbolDBEnginePriv *priv;
	
	priv = dbe->priv;

	/* get prepared query */
	if ((stmt = sdb_engine_get_statement_by_query_id (dbe, qtype)) == NULL)
	{
		g_warning ("Query is null");
		return -1;
	}

	plist = sdb_engine_get_query_parameters_list (dbe, qtype);
	
	if ((param = gda_set_get_holder ((GdaSet*)plist, param_key)) == NULL)
	{
		g_warning ("sdb_engine_get_tuple_id_by_unique_name: param is NULL "
				   "from pquery!\n");
		return -1;
	}
	gda_holder_set_value (param, param_value);
	
	/* execute the query with parametes just set */
	data_model = gda_connection_statement_execute_select (priv->db_connection, 
														  (GdaStatement*)stmt, 
														  (GdaSet*)plist, NULL);
		
	if (!GDA_IS_DATA_MODEL (data_model) ||
		gda_data_model_get_n_rows (GDA_DATA_MODEL (data_model)) <= 0)
	{
		if (data_model != NULL)
			g_object_unref (data_model);
		return -1;
	}

	/* get and parse the results. */
	num = gda_data_model_get_value_at (GDA_DATA_MODEL (data_model), 0, 0);

	table_id = g_value_get_int (num);
	g_object_unref (data_model);
	return table_id;
}

/**
 * This is the same as sdb_engine_get_tuple_id_by_unique_name () but for two
 * unique parameters. This should be the quickest way. Surely quicker than
 * use g_strdup_printf () with a va_list for example.
 * @return -1 on error. Otherwise the id of table
 *
 */
static gint
sdb_engine_get_tuple_id_by_unique_name2 (SymbolDBEngine * dbe, 
										 static_query_type qtype,
										 gchar * param_key1,
										 const GValue * value1,
										 gchar * param_key2,
										 const GValue * value2)
{
	const GdaSet *plist;
	const GdaStatement *stmt;
	GdaHolder *param;
	GdaDataModel *data_model;
	const GValue *num;
	gint table_id;
	SymbolDBEnginePriv *priv;
	
	priv = dbe->priv;

	/* get prepared query */
	if ((stmt = sdb_engine_get_statement_by_query_id (dbe, qtype)) == NULL)
	{
		g_warning ("Query is null");
		return -1;
	}

	plist = sdb_engine_get_query_parameters_list (dbe, qtype);
	
	/* look for and set the first parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, param_key1)) == NULL)
	{
		g_warning ("sdb_engine_get_tuple_id_by_unique_name2: "
				   "param is NULL from pquery! [par1: %s] [par2: %s]\n",
				   param_key1, param_key2);
		return -1;
	}
	gda_holder_set_value (param, value1);
	
	/* ...and the second one */
	if ((param = gda_set_get_holder ((GdaSet*)plist, param_key2)) == NULL)
	{
		g_warning ("sdb_engine_get_tuple_id_by_unique_name2: "
				   "param is NULL from pquery! [par1: %s] [par2: %s]\n",
				   param_key1, param_key2);
		return -1;
	}
	gda_holder_set_value (param, value2);

	/* execute the query with parametes just set */
	data_model = gda_connection_statement_execute_select (priv->db_connection, 
														  (GdaStatement*)stmt, 
														  (GdaSet*)plist, NULL);
		
	if (!GDA_IS_DATA_MODEL (data_model) ||
		gda_data_model_get_n_rows (GDA_DATA_MODEL (data_model)) <= 0)
	{
		if (data_model != NULL)
			g_object_unref (data_model);
		return -1;
	}

	/* get and parse the results. */
	num = gda_data_model_get_value_at (GDA_DATA_MODEL (data_model), 0, 0);

	table_id = g_value_get_int (num);
	g_object_unref (data_model);
	
	return table_id;
}

static gint
sdb_engine_get_tuple_id_by_unique_name3 (SymbolDBEngine * dbe, 
										 static_query_type qtype,
										 gchar * param_key1,
										 const GValue * value1,
										 gchar * param_key2,
										 const GValue * value2,
										 gchar * param_key3,
										 const GValue * value3)
{
	const GdaSet *plist;
	const GdaStatement *stmt;
	GdaHolder *param;
	GdaDataModel *data_model;
	const GValue *num;
	gint table_id;
	SymbolDBEnginePriv *priv;
	
	priv = dbe->priv;

	/* get prepared query */
	if ((stmt = sdb_engine_get_statement_by_query_id (dbe, qtype)) == NULL)
	{
		g_warning ("Query is null");
		return -1;
	}

	plist = sdb_engine_get_query_parameters_list (dbe, qtype);
	
	if ((param = gda_set_get_holder ((GdaSet*)plist, param_key1)) == NULL)
	{
		g_warning ("sdb_engine_get_tuple_id_by_unique_name: param is NULL "
				   "from pquery!\n");
		return -1;
	}
	gda_holder_set_value (param, value1);
	

	/* ...and the second one */
	if ((param = gda_set_get_holder ((GdaSet*)plist, param_key2)) == NULL)
	{
		g_warning ("sdb_engine_get_tuple_id_by_unique_name2: "
				   "param is NULL from pquery! [par1: %s] [par2: %s]\n",
				   param_key1, param_key2);
		return -1;
	}
	gda_holder_set_value (param, value2);

	/* ...and the third one */
	if ((param = gda_set_get_holder ((GdaSet*)plist, param_key3)) == NULL)
	{
		g_warning ("sdb_engine_get_tuple_id_by_unique_name2: "
				   "param is NULL from pquery! [par1: %s] [par2: %s]\n",
				   param_key1, param_key3);
		return -1;
	}
	gda_holder_set_value (param, value3);
		
	/* execute the query with parametes just set */
	data_model = gda_connection_statement_execute_select (priv->db_connection, 
														  (GdaStatement*)stmt, 
														  (GdaSet*)plist, NULL);
		
	if (!GDA_IS_DATA_MODEL (data_model) ||
		gda_data_model_get_n_rows (GDA_DATA_MODEL (data_model)) <= 0)
	{
		if (data_model != NULL)
			g_object_unref (data_model);
		return -1;
	}

	/* get and parse the results. */
	num = gda_data_model_get_value_at (GDA_DATA_MODEL (data_model), 0, 0);

	table_id = g_value_get_int (num);
	g_object_unref (data_model);
	return table_id;
}

static int
sdb_engine_get_file_defined_id (SymbolDBEngine* dbe,
								const gchar* base_prj_path,
								const gchar* fake_file_on_db,
								tagEntry* tag_entry)
{
	GValue* value = gda_value_new (G_TYPE_STRING);
	gint file_defined_id = 0;
	if (base_prj_path != NULL)
	{
		/* in this case fake_file will be ignored. */
		
		/* we expect here an absolute path */
		g_value_set_string (value,
							tag_entry->file + strlen (base_prj_path) );
	}
	else
	{
		/* check whether the fake_file can substitute the tag_entry->file one */
		if (fake_file_on_db == NULL)
			g_value_set_string (value, tag_entry->file);
		else
			g_value_set_string (value, fake_file_on_db);
	}
	
	if ((file_defined_id = sdb_engine_get_tuple_id_by_unique_name (dbe,
													PREP_QUERY_GET_FILE_ID_BY_UNIQUE_NAME,
													"filepath",
													value)) < 0)
	{	
		/* if we arrive here there should be some sync problems between the filenames
		 * in database and the ones in the ctags files. We trust in db's ones,
		 * so we'll just return here.
		 */
		g_warning ("sync problems between db and ctags filenames entries. "
				   "File was %s (base_path: %s, fake_file: %s, tag_file: %s)", 
				   g_value_get_string (value), base_prj_path, fake_file_on_db, 
				   tag_entry->file);
		gda_value_free (value);
		return -1;
	}
	gda_value_free (value);
	return file_defined_id;
}

static GTimer *sym_timer_DEBUG  = NULL;

/**
 * If fake_file is != NULL we claim and assert that tags contents which are
 * scanned belong to the fake_file in the project.
 * More: the fake_file refers to just one single file and cannot be used
 * for multiple fake_files.
 */
static void
sdb_engine_populate_db_by_tags (SymbolDBEngine * dbe, FILE* fd,
								gchar * fake_file_on_db,
								gboolean force_sym_update)
{
	tagFile *tag_file;
	tagFileInfo tag_file_info;
	tagEntry tag_entry;
	gint file_defined_id_cache = 0;
	gchar* tag_entry_file_cache = NULL;
	
	SymbolDBEnginePriv *priv = dbe->priv;

	gchar* base_prj_path = fake_file_on_db == NULL ?
		priv->project_directory : NULL;
	
	g_return_if_fail (dbe != NULL);

	g_return_if_fail (priv->db_connection != NULL);
	g_return_if_fail (fd != NULL);

	if (priv->updated_symbols_id == NULL)
		priv->updated_symbols_id = g_async_queue_new ();
	
	if (priv->inserted_symbols_id == NULL)
		priv->inserted_symbols_id = g_async_queue_new ();
	
	DEBUG_PRINT ("sdb_engine_populate_db_by_tags ()");
	if ((tag_file = tagsOpen_1 (fd, &tag_file_info)) == NULL)
	{
		g_warning ("error in opening ctags file");
	}

	if (sym_timer_DEBUG == NULL)
		sym_timer_DEBUG = g_timer_new ();
	else
		g_timer_reset (sym_timer_DEBUG);
	gint tags_total_DEBUG = 0;
	while (tagsNext (tag_file, &tag_entry) != TagFailure)
	{
		gint file_defined_id = 0;
		if (file_defined_id_cache > 0)
		{
			if (g_str_equal (tag_entry.file, tag_entry_file_cache))
			{
				file_defined_id = file_defined_id_cache;
			}
		}
		if (file_defined_id == 0)
		{
			file_defined_id = sdb_engine_get_file_defined_id (dbe,
															  base_prj_path,
															  fake_file_on_db,
															  &tag_entry);
			file_defined_id_cache = file_defined_id;
			g_free (tag_entry_file_cache);
			tag_entry_file_cache = g_strdup(tag_entry.file);
		}
		sdb_engine_add_new_symbol (dbe, &tag_entry, file_defined_id,
								   force_sym_update);
		
		tags_total_DEBUG ++;
	}
	g_free (tag_entry_file_cache);
	
	gdouble elapsed_DEBUG = g_timer_elapsed (sym_timer_DEBUG, NULL);
	DEBUG_PRINT ("elapsed: %f for (%d) [%f per symbol]", elapsed_DEBUG,
				 tags_total_DEBUG, elapsed_DEBUG / tags_total_DEBUG);

	/* notify listeners that another file has been scanned */
	g_async_queue_push (priv->signals_queue, (gpointer)(SINGLE_FILE_SCAN_END +1));
}

static gpointer
sdb_engine_ctags_output_thread (gpointer data)
{
	ThreadDataOutput *output;
	gint len_chars;
	gchar *chars, *chars_ptr;
	gint remaining_chars;
	gint len_marker;
	SymbolDBEnginePriv *priv;
	SymbolDBEngine *dbe;
	
	output = (ThreadDataOutput *)data;
	dbe = output->user_data;
	chars = chars_ptr = output->chars;	
	
	g_return_val_if_fail (dbe != NULL, (gpointer)-1);	
	g_return_val_if_fail (chars_ptr != NULL, (gpointer)-1);

	priv = dbe->priv;

	/* lock */
	if (priv->mutex)
		g_mutex_lock (priv->mutex);
	priv->thread_status = TRUE;
	
	remaining_chars = len_chars = strlen (chars_ptr);
	len_marker = strlen (CTAGS_MARKER);	

	if ( len_chars >= len_marker ) 
	{
		gchar *marker_ptr = NULL;
		gint tmp_str_length = 0;

		/* is it an end file marker? */
		marker_ptr = strstr (chars_ptr, CTAGS_MARKER);

		do  {
			if (marker_ptr != NULL) 
			{
				gint scan_flag;
				gchar *real_file;
		
				/* set the length of the string parsed */
				tmp_str_length = marker_ptr - chars_ptr;
	
				/*DEBUG_PRINT ("program output [new version]: ==>%s<==", chars_ptr);*/
				/* write to shm_file all the chars_ptr received without the marker ones */
				fwrite (chars_ptr, sizeof(gchar), tmp_str_length, priv->shared_mem_file);

				chars_ptr = marker_ptr + len_marker;
				remaining_chars -= (tmp_str_length + len_marker);
				fflush (priv->shared_mem_file);
				
				/* get the scan flag from the queue. We need it to know whether
				 * an update of symbols must be done or not */
				scan_flag = (int)g_async_queue_try_pop (priv->scan_queue);
				real_file = g_async_queue_try_pop (priv->scan_queue);
				
				/* and now call the populating function */
				if (scan_flag == DO_UPDATE_SYMS ||
					scan_flag == DO_UPDATE_SYMS_AND_EXIT)
				{
					sdb_engine_populate_db_by_tags (dbe, priv->shared_mem_file,
								(int)real_file == DONT_FAKE_UPDATE_SYMS ? NULL : real_file, 
								TRUE);
				}
				else 
				{
					sdb_engine_populate_db_by_tags (dbe, priv->shared_mem_file,
								(int)real_file == DONT_FAKE_UPDATE_SYMS ? NULL : real_file, 
								FALSE);					
				}
				
				/* don't forget to free the real_file, if it's a char */
				if ((int)real_file != DONT_FAKE_UPDATE_SYMS)
					g_free (real_file);
				
				/* check also if, together with an end file marker, we have an 
				 * end group-of-files end marker.
				 */
				if (scan_flag == DO_UPDATE_SYMS_AND_EXIT || 
					 scan_flag == DONT_UPDATE_SYMS_AND_EXIT )
				{
					gint tmp_inserted;
					gint tmp_updated;

					/* proceed with second passes */
					DEBUG_PRINT ("FOUND end-of-group-files marker."
								 "go on with sdb_engine_second_pass_do ()");
					
					chars_ptr += len_marker;
					remaining_chars -= len_marker;
					
					/* will emit symbol_scope_updated */
					sdb_engine_second_pass_do (dbe);					
					
					/* Here we are. It's the right time to notify the listeners
					 * about out fresh new inserted/updated symbols...
					 * Go on by emitting them.
					 */
					while ((tmp_inserted = (int)
							g_async_queue_try_pop (priv->inserted_symbols_id)) > 0)
					{
						/* we must be sure to insert both signals at once */
						g_async_queue_lock (priv->signals_queue);
						
						/* the +1 is because asyn_queue doesn't want NULL values */
						g_async_queue_push_unlocked (priv->signals_queue, 
													 (gpointer)(SYMBOL_INSERTED + 1));
						g_async_queue_push_unlocked (priv->signals_queue, 
													 (gpointer) tmp_inserted);
						g_async_queue_unlock (priv->signals_queue);
					}
					
					while ((tmp_updated = (int)
							g_async_queue_try_pop (priv->updated_symbols_id)) > 0)
					{
						g_async_queue_lock (priv->signals_queue);
						g_async_queue_push_unlocked (priv->signals_queue, (gpointer)
													 (SYMBOL_UPDATED + 1));
						g_async_queue_push_unlocked (priv->signals_queue, 
													 (gpointer) tmp_updated);
						g_async_queue_unlock (priv->signals_queue);
					}

					
					/* emit signal. The end of files-group can be cannot be
					 * determined by the caller. This is the only way.
					 */
					DEBUG_PRINT ("EMITTING scan-end");
					g_async_queue_push (priv->signals_queue, (gpointer)(SCAN_END + 1));
				}
				
				/* truncate the file to 0 length */
				ftruncate (priv->shared_mem_fd, 0);				
			}
			else
			{ 
				/* marker_ptr is NULL here. We should then exit the loop. */
				/* write to shm_file all the chars received */
				fwrite (chars_ptr, sizeof(gchar), remaining_chars, 
						priv->shared_mem_file);

				fflush (priv->shared_mem_file);
				break;
			}

			/* found out a new marker */ 
			marker_ptr = strstr (marker_ptr + len_marker, CTAGS_MARKER);
		} while (remaining_chars + len_marker < len_chars || marker_ptr != NULL);
	}
	else 
	{
		DEBUG_PRINT ("no len_chars > len_marker");
	}

	priv->thread_status = FALSE;
	priv->concurrent_threads--;
	/* unlock */
	if (priv->mutex)
		g_mutex_unlock (priv->mutex);
	
	g_free (chars);
	g_free (output);
	
	return 0;
}


static gboolean
sdb_engine_timeout_trigger_signals (gpointer user_data)
{
	SymbolDBEngine *dbe = (SymbolDBEngine *) user_data;
	SymbolDBEnginePriv *priv;

	g_return_val_if_fail (user_data != NULL, FALSE);
	
	priv = dbe->priv;
		
/*	DEBUG_PRINT ("signals trigger");*/
	if (g_async_queue_length (priv->signals_queue) > 0)
	{
		gpointer tmp;
		gpointer sign = NULL;
		gint real_signal;
	
		while ((sign = g_async_queue_try_pop (priv->signals_queue)) != NULL)  {

			if (sign== NULL) {
				return g_async_queue_length (priv->signals_queue) > 0 ? TRUE : FALSE;
			}
	
			real_signal = (gint)sign- 1;
	
			switch (real_signal) {
				case SINGLE_FILE_SCAN_END:
					g_signal_emit (dbe, signals[SINGLE_FILE_SCAN_END], 0);
					break;
		
				case SCAN_END:
					g_signal_emit (dbe, signals[SCAN_END], 0);
					break;
	
				case SYMBOL_INSERTED:
					tmp = g_async_queue_try_pop (priv->signals_queue);
					g_signal_emit (dbe, signals[SYMBOL_INSERTED], 0, tmp);
					break;
	
				case SYMBOL_UPDATED:
					tmp = g_async_queue_try_pop (priv->signals_queue);
					g_signal_emit (dbe, signals[SYMBOL_UPDATED], 0, tmp);
					break;
	
				case SYMBOL_SCOPE_UPDATED:
					tmp = g_async_queue_try_pop (priv->signals_queue);
					g_signal_emit (dbe, signals[SYMBOL_SCOPE_UPDATED], 0, tmp);
					break;
	
				case SYMBOL_REMOVED:
					tmp = g_async_queue_try_pop (priv->signals_queue);
					g_signal_emit (dbe, signals[SYMBOL_REMOVED], 0, tmp);
					break;
			}
		}		
		/* reset to 0 the retries */
		priv->trigger_closure_retries = 0;
	}
	else {
		priv->trigger_closure_retries++;
	}
	
	if (priv->thread_status == FALSE &&
		priv->trigger_closure_retries > TRIGGER_MAX_CLOSURE_RETRIES &&
		g_queue_get_length (priv->thread_list) <= 0 &&
		priv->thread_monitor_handler <= 0)
	{
		DEBUG_PRINT ("removing signals trigger");
		/* remove the trigger coz we don't need it anymore... */
		g_source_remove (priv->timeout_trigger_handler);
		priv->timeout_trigger_handler = 0;
		return FALSE;
	}
	
	return TRUE;
}

static gboolean
sdb_engine_thread_monitor (gpointer data)
{
	gpointer output;
	SymbolDBEngine *dbe = (SymbolDBEngine *) data;
	SymbolDBEnginePriv *priv;

	g_return_val_if_fail (data != NULL, FALSE);
	
	priv = dbe->priv;
		
	if (priv->concurrent_threads > THREADS_MAX_CONCURRENT) {
		/* monitor acted here. There are plenty threads already working. */
		return TRUE;
	}
	
	output = g_queue_pop_head (priv->thread_list);
 
	if (output != NULL)
	{
		priv->concurrent_threads ++;
		g_thread_create ((GThreadFunc)sdb_engine_ctags_output_thread, output, 
					 FALSE, NULL);
		priv->thread_closure_retries = 0;
	}
	else 
	{
		priv->thread_closure_retries++;
	}

	if (priv->thread_closure_retries > THREAD_MAX_CLOSURE_RETRIES &&
		g_queue_get_length (priv->thread_list) <= 0)
	{
		DEBUG_PRINT ("removing thread monitor");
		/* remove the thread monitor */
		g_source_remove (priv->thread_monitor_handler);
		priv->thread_monitor_handler = 0;
		return FALSE;
	}
	
	/* recall this monitor */
	return TRUE;	
}

static void
sdb_engine_ctags_output_callback_1 (AnjutaLauncher * launcher,
								  AnjutaLauncherOutputType output_type,
								  const gchar * chars, gpointer user_data)
{ 
	ThreadDataOutput *output;
	SymbolDBEngine *dbe = (SymbolDBEngine *) user_data;
	SymbolDBEnginePriv *priv;

	g_return_if_fail (user_data != NULL);
	
	priv = dbe->priv;	
	
	output  = g_new0 (ThreadDataOutput, 1);
	output->chars = g_strdup (chars);
	output->user_data = user_data;

	if (priv->thread_list == NULL)
		priv->thread_list = g_queue_new ();
	
	g_queue_push_tail (priv->thread_list, output);

	/* thread monitor */
	if (priv->thread_monitor_handler <= 0)
	{
		priv->thread_monitor_handler = 
			g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE,
					THREADS_MONITOR_TIMEOUT, 
					sdb_engine_thread_monitor, 
					user_data,
					NULL);
		priv->thread_closure_retries = 0;
	}
	
	/* signals monitor */
	if (priv->timeout_trigger_handler <= 0)
	{
		priv->timeout_trigger_handler = 
			g_timeout_add_full (G_PRIORITY_LOW, TRIGGER_SIGNALS_DELAY, 
						   sdb_engine_timeout_trigger_signals, user_data, NULL);
		priv->trigger_closure_retries = 0;
	}
}

static void
on_scan_files_end_1 (AnjutaLauncher * launcher, int child_pid,
				   int exit_status, gulong time_taken_in_seconds,
				   gpointer data)
{
	DEBUG_PRINT ("ctags ended");
}

/* Scans with ctags and produce an output 'tags' file [shared memory file]
 * containing language symbols. This function will call ctags 
 * executale and then sdb_engine_populate_db_by_tags () when it'll detect some
 * output.
 * Please note the files_list/real_files_list parameter:
 * this version of sdb_engine_scan_files_1 () let you scan for text buffer(s) that
 * will be claimed as buffers for the real files.
 * 1. simple mode: files_list represents the real files on disk and so we don't 
 * need real_files_list, which will be NULL.
 * 2. advanced mode: files_list represents temporary flushing of buffers on disk, i.e.
 * /tmp/anjuta_XYZ.cxx. real_files_list is the representation of those files on 
 * database. On the above example we can have anjuta_XYZ.cxx mapped as /src/main.c 
 * on db. In this mode files_list and real_files_list must have the same size.
 *
 */
/* server mode version */
static gboolean
sdb_engine_scan_files_1 (SymbolDBEngine * dbe, const GPtrArray * files_list,
						 const GPtrArray *real_files_list, gboolean symbols_update)
{
	SymbolDBEnginePriv *priv;
	gint i;
	
	g_return_val_if_fail (dbe != NULL, FALSE);
	g_return_val_if_fail (files_list != NULL, FALSE);
	
	if (files_list->len == 0)
		return FALSE;
	
	/* Check if ctags is really installed */
	if (!anjuta_util_prog_is_installed (CTAGS_PATH, TRUE))
		return FALSE;
	
	/* start process in server mode */
	priv = dbe->priv;

	if (real_files_list != NULL && (files_list->len != real_files_list->len)) 
	{
		g_warning ("no matched size between real_files_list and files_list");
		return FALSE;
	}
	
	/* if ctags_launcher isn't initialized, then do it now. */
	if (priv->ctags_launcher == NULL) 
	{
		gchar *exe_string;
		
		DEBUG_PRINT ("creating anjuta_launcher");
		priv->ctags_launcher = anjuta_launcher_new ();

		anjuta_launcher_set_check_passwd_prompt (priv->ctags_launcher, FALSE);
		anjuta_launcher_set_encoding (priv->ctags_launcher, NULL);
		
		g_signal_connect (G_OBJECT (priv->ctags_launcher), "child-exited",
						  G_CALLBACK (on_scan_files_end_1), NULL);

		exe_string = g_strdup_printf ("%s --fields=afmiKlnsStz --c++-kinds=+p "
								  "--filter=yes --filter-terminator='"CTAGS_MARKER"'",
								  CTAGS_PATH);
		
		anjuta_launcher_execute (priv->ctags_launcher,
								 exe_string, sdb_engine_ctags_output_callback_1, 
								 dbe);
		g_free (exe_string);
	}
	
	/* what about the scan_queue? is it initialized? It will contain mainly 
	 * ints that refers to the force_update status.
	 */
	if (priv->scan_queue == NULL)
	{
		priv->scan_queue = g_async_queue_new ();		
	}
	
	/* create the shared memory file */
	if (priv->shared_mem_file == 0)
	{
		gchar *temp_file;
		while (TRUE)
		{
			temp_file = g_strdup_printf ("/anjuta-%d_%ld.tags", getpid (),
								 time (NULL));
			gchar *test;
			test = g_strconcat (SHARED_MEMORY_PREFIX, temp_file, NULL);
			if (g_file_test (test, G_FILE_TEST_EXISTS) == TRUE)
			{
				DEBUG_PRINT ("file %s already exists... retrying", test);
				g_free (test);
				g_free (temp_file);
				continue;
			}
			else
			{
				g_free (test);
				break;
			}
		}

		priv->shared_mem_str = temp_file;
		
		if ((priv->shared_mem_fd = 
			 shm_open (temp_file, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR)) < 0)
		{
			g_warning ("Error while trying to open a shared memory file. Be"
					   "sure to have "SHARED_MEMORY_PREFIX" mounted with tmpfs");
		}
	
		priv->shared_mem_file = fdopen (priv->shared_mem_fd, "a+b");
		DEBUG_PRINT ("temp_file %s", temp_file);

		/* no need to free temp_file (alias shared_mem_str). It will be freed on plugin finalize */
	}
	
	priv->scanning_status = TRUE;	

	for (i = 0; i < files_list->len; i++)
	{
		gchar *node = (gchar *) g_ptr_array_index (files_list, i);
		
		if (g_file_test (node, G_FILE_TEST_EXISTS) == FALSE)
		{
			g_warning ("File %s not scanned because it does not exist", node);
			continue;
		}
			
		DEBUG_PRINT ("sent to stdin [%d] %s", i, node);
		anjuta_launcher_send_stdin (priv->ctags_launcher, node);
		anjuta_launcher_send_stdin (priv->ctags_launcher, "\n");

		if (symbols_update == TRUE) 
		{
			/* will this be the last file in the list? */
			if (i + 1 >= files_list->len) 
			{
				/* yes */
				g_async_queue_push (priv->scan_queue, (gpointer) DO_UPDATE_SYMS_AND_EXIT);
			}
			else {
				/* no */
				g_async_queue_push (priv->scan_queue, (gpointer) DO_UPDATE_SYMS);
			}
		}
		else 
		{
			if (i + 1 >= files_list->len) 
			{
				/* yes */
				g_async_queue_push (priv->scan_queue, (gpointer) DONT_UPDATE_SYMS_AND_EXIT);
			}
			else {
				/* no */
				g_async_queue_push (priv->scan_queue, (gpointer) DONT_UPDATE_SYMS);
			}
		}

		/* don't forget to add the real_files if the caller provided a list for
		 * them! */
		if (real_files_list != NULL)
		{
			g_async_queue_push (priv->scan_queue, 
								(gpointer)g_strdup (
								g_ptr_array_index (real_files_list, i)));
		}
		else 
		{
			/* else add a DONT_FAKE_UPDATE_SYMS marker, just to notify that this 
			 * is not a fake file scan 
			 */
			g_async_queue_push (priv->scan_queue, (gpointer) DONT_FAKE_UPDATE_SYMS);
		}
	}

	priv->scanning_status = FALSE;

	return TRUE;
}

static void
sdb_engine_init (SymbolDBEngine * object)
{
	SymbolDBEngine *sdbe;
	GHashTable *h;

	sdbe = SYMBOL_DB_ENGINE (object);
	sdbe->priv = g_new0 (SymbolDBEnginePriv, 1);

	/* initialize an hash table to be used and shared with Iterators */
	sdbe->priv->sym_type_conversion_hash =
		g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);	
	h = sdbe->priv->sym_type_conversion_hash;

	/* please if you change some value below here remember to change also on */
	g_hash_table_insert (h, g_strdup("class"), 
				(gpointer)IANJUTA_SYMBOL_TYPE_CLASS);

	g_hash_table_insert (h, g_strdup("enum"), 
				(gpointer)IANJUTA_SYMBOL_TYPE_ENUM);

	g_hash_table_insert (h, g_strdup("enumerator"), 
				(gpointer)IANJUTA_SYMBOL_TYPE_ENUMERATOR);

	g_hash_table_insert (h, g_strdup("field"), 
				(gpointer)IANJUTA_SYMBOL_TYPE_FIELD);

	g_hash_table_insert (h, g_strdup("function"), 
				(gpointer)IANJUTA_SYMBOL_TYPE_FUNCTION);

	g_hash_table_insert (h, g_strdup("interface"), 
				(gpointer)IANJUTA_SYMBOL_TYPE_INTERFACE);

	g_hash_table_insert (h, g_strdup("member"), 
				(gpointer)IANJUTA_SYMBOL_TYPE_MEMBER);

	g_hash_table_insert (h, g_strdup("method"), 
				(gpointer)IANJUTA_SYMBOL_TYPE_METHOD);

	g_hash_table_insert (h, g_strdup("namespace"), 
				(gpointer)IANJUTA_SYMBOL_TYPE_NAMESPACE);

	g_hash_table_insert (h, g_strdup("package"), 
				(gpointer)IANJUTA_SYMBOL_TYPE_PACKAGE);

	g_hash_table_insert (h, g_strdup("prototype"), 
				(gpointer)IANJUTA_SYMBOL_TYPE_PROTOTYPE);
				
	g_hash_table_insert (h, g_strdup("struct"), 
				(gpointer)IANJUTA_SYMBOL_TYPE_STRUCT);
				
	g_hash_table_insert (h, g_strdup("typedef"), 
				(gpointer)IANJUTA_SYMBOL_TYPE_TYPEDEF);
				
	g_hash_table_insert (h, g_strdup("union"), 
				(gpointer)IANJUTA_SYMBOL_TYPE_UNION);
				
	g_hash_table_insert (h, g_strdup("variable"), 
				(gpointer)IANJUTA_SYMBOL_TYPE_VARIABLE);

	g_hash_table_insert (h, g_strdup("externvar"), 
				(gpointer)IANJUTA_SYMBOL_TYPE_EXTERNVAR);

	g_hash_table_insert (h, g_strdup("macro"), 
				(gpointer)IANJUTA_SYMBOL_TYPE_MACRO);

	g_hash_table_insert (h, g_strdup("macro_with_arg"), 
				(gpointer)IANJUTA_SYMBOL_TYPE_MACRO_WITH_ARG);

	g_hash_table_insert (h, g_strdup("file"), 
				(gpointer)IANJUTA_SYMBOL_TYPE_FILE);

	g_hash_table_insert (h, g_strdup("other"), 
				(gpointer)IANJUTA_SYMBOL_TYPE_OTHER);	
	
	
	/* create the hash table that will store shared memory files strings used for 
	 * buffer scanning. Un Engine destroy will unlink () them.
	 */
	sdbe->priv->garbage_shared_mem_files = g_hash_table_new_full (g_str_hash, g_str_equal, 
													  g_free, NULL);	
	
	/* create Anjuta Launcher instance. It will be used for tags parsing. */
	sdbe->priv->ctags_launcher = NULL;

	/*
	 * STATIC QUERY STRUCTURE INITIALIZE
	 */
	
	/* -- workspace -- */
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
									PREP_QUERY_WORKSPACE_NEW, 
		"INSERT INTO workspace (workspace_name, analyse_time) "
	 	"VALUES (## /* name:'wsname' type:gchararray */,"
	 	"datetime ('now', 'localtime'))");
	
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
									PREP_QUERY_GET_WORKSPACE_ID_BY_UNIQUE_NAME, 
	 	"SELECT workspace_id FROM workspace WHERE workspace_name = ## /* name:'wsname' "
	 	"type:gchararray */ LIMIT 1");

	/* -- project -- */
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
									PREP_QUERY_PROJECT_NEW, 
	 	"INSERT INTO project (project_name, wrkspace_id, analyse_time) "
	 	"VALUES (## /* name:'prjname' type:gchararray */,"
	 	"## /* name:'wsid' type:gint */, datetime ('now', 'localtime'))");

	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
									PREP_QUERY_GET_PROJECT_ID_BY_UNIQUE_NAME, 
	 	"SELECT project_id FROM project WHERE project_name = ## /* name:'prjname' "
	 	"type:gchararray */ LIMIT 1");

	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
									PREP_QUERY_UPDATE_PROJECT_ANALYSE_TIME, 
	 	"UPDATE project SET analyse_time = datetime('now', 'localtime', '+10 seconds') WHERE "
	 	"project_name = ## /* name:'prjname' type:gchararray */");
	
	/* -- file -- */
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
									PREP_QUERY_FILE_NEW, 
	 	"INSERT INTO file (file_path, prj_id, lang_id, analyse_time) VALUES ("
	 	"## /* name:'filepath' type:gchararray */, ## /* name:'prjid' "
	 	"type:gint */, ## /* name:'langid' type:gint */, "
	 	"datetime ('now', 'localtime'))");
	
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
									PREP_QUERY_GET_FILE_ID_BY_UNIQUE_NAME, 
		"SELECT file_id FROM file WHERE file_path = ## /* name:'filepath' "
	 	"type:gchararray */ LIMIT 1");
	
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
									PREP_QUERY_GET_ALL_FROM_FILE_BY_PROJECT_NAME,
		"SELECT * FROM file WHERE prj_id = (SELECT project_id FROM project "
	 	"WHERE project_name = ## /* name:'prjname' type:gchararray */)");

	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
									PREP_QUERY_GET_ALL_FROM_FILE_BY_PROJECT_ID,
		"SELECT file_id, file_path, prj_id, lang_id, analyse_time FROM file "
	 	"WHERE prj_id = ## /* name:'prjid' type:gint */");
/*
		"SELECT * FROM file JOIN project on project_id = prj_id WHERE "\
		"project.name = ## / * name:'prjname' type:gchararray * /",
*/
									

	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
									PREP_QUERY_UPDATE_FILE_ANALYSE_TIME,
		"UPDATE file SET analyse_time = datetime('now', 'localtime') WHERE "
	 	"file_path = ## /* name:'filepath' type:gchararray */");

	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
									PREP_QUERY_GET_ALL_FROM_FILE_WHERE_NOT_IN_SYMBOLS,
		"SELECT file_id, file_path FROM file WHERE file_id NOT IN "
		"(SELECT file_defined_id FROM symbol)");
	
	/* -- language -- */
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
									PREP_QUERY_LANGUAGE_NEW,
		"INSERT INTO language (language_name) VALUES (## /* name:'langname' "
	 	"type:gchararray */)");

	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
									PREP_QUERY_GET_LANGUAGE_ID_BY_UNIQUE_NAME,
	 	"SELECT language_id FROM language WHERE language_name = ## /* name:'langname' "
	 	"type:gchararray */ LIMIT 1");
	
	/* -- sym type -- */
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
									PREP_QUERY_SYM_TYPE_NEW,
		"INSERT INTO sym_type (type_type, type_name) VALUES (## /* name:'type' "
	 	"type:gchararray */, ## /* name:'typename' type:gchararray */)");

	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
									PREP_QUERY_GET_SYM_TYPE_ID,
	 	"SELECT type_id FROM sym_type WHERE type_type = ## /* name:'type' "
	 	"type:gchararray */ AND type_name = ## /* name:'typename' "
	 	"type:gchararray */ LIMIT 1");

	/* -- sym kind -- */
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
									PREP_QUERY_SYM_KIND_NEW,
	 	"INSERT INTO sym_kind (kind_name) VALUES(## /* name:'kindname' "
	 	"type:gchararray */)");

	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
									PREP_QUERY_GET_SYM_KIND_BY_UNIQUE_NAME,
		"SELECT sym_kind_id FROM sym_kind WHERE kind_name = ## /* "
	 	"name:'kindname' type:gchararray */");
	
	/* -- sym access -- */
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
									PREP_QUERY_SYM_ACCESS_NEW,
		"INSERT INTO sym_access (access_name) VALUES(## /* name:'accesskind' "
	 	"type:gchararray */)");

	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
									PREP_QUERY_GET_SYM_ACCESS_BY_UNIQUE_NAME,
		"SELECT access_kind_id FROM sym_access WHERE access_name = ## /* "
	 	"name:'accesskind' type:gchararray */ LIMIT 1");
	
	/* -- sym implementation -- */
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
		 							PREP_QUERY_SYM_IMPLEMENTATION_NEW,
	 	"INSERT INTO sym_implementation (implementation_name) VALUES(## /* name:'implekind' "
	 	"type:gchararray */)");
	
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_GET_SYM_IMPLEMENTATION_BY_UNIQUE_NAME,
	 	"SELECT sym_impl_id FROM sym_implementation WHERE kind = ## /* "
	 	"name:'implekind' type:gchararray */ LIMIT 1");
	
	/* -- heritage -- */
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_HERITAGE_NEW,
	 	"INSERT INTO heritage (symbol_id_base, symbol_id_derived) VALUES(## /* "
	 	"name:'symbase' type:gint */, ## /* name:'symderived' type:gint */)");	
	
	/* -- scope -- */
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_SCOPE_NEW,
	 	"INSERT INTO scope (scope_name, type_id) VALUES(## /* name:'scope' "
	 	"type:gchararray */, ## /* name:'typeid' type:gint */)");
	
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_GET_SCOPE_ID,
	 	"SELECT scope_id FROM scope WHERE scope_name = ## /* name:'scope' "
	 	"type:gchararray */ AND type_id = ## /* name:'typeid' type:gint */ LIMIT 1");
	
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_GET_PARENT_SCOPE_ID_BY_SYMBOL_ID_NO_FILE,
	 	"SELECT symbol.symbol_id, symbol.file_defined_id, "
	 	"symbol.file_position, symbol.scope_definition_id, symbol.scope_id "
	 	"FROM symbol WHERE symbol.scope_definition_id = ( "
	 	"SELECT symbol.scope_id FROM symbol WHERE symbol.symbol_id = "
	 	"## /* name:'symid' type:gint */) "
	 	"AND symbol.scope_definition_id > 0");
	
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_GET_PARENT_SCOPE_ID_BY_SYMBOL_ID,
	 	"SELECT symbol.symbol_id, symbol.file_defined_id, "
	 	"symbol.file_position, symbol.scope_definition_id, symbol.scope_id "
	 	"FROM symbol JOIN file "
	 	"ON symbol.file_defined_id = file.file_id "
	 	"WHERE symbol.scope_definition_id = ( "
	 	"SELECT symbol.scope_id FROM symbol WHERE symbol.symbol_id = "
	 	"## /* name:'symid' type:gint */) "
	 	"AND symbol.scope_definition_id > 0 "
	 	"AND file.file_path = ## /* name:'dbfile' type:gchararray */");
	
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_GET_PARENT_SCOPE_ID_BY_SYMBOL_ID_BY_SYMBOL_ID,
	 	"SELECT symbol.scope_definition_id FROM symbol WHERE "
	 	"file_defined_id = (SELECT file_defined_id FROM symbol WHERE symbol_id = "
	 	"/* name:'scopedsymid' type:gint */) "
	 	"AND file_position < (SELECT file_position FROM symbol WHERE symbol_id = "
	 	"/* name:'scopedsymid' type:gint */) "
	 	"ORDER BY file_position DESC");
	
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_GET_SCOPE_DEFINITION_ID_BY_WALK_DOWN_SCOPE_PATH,
	 	"SELECT scope_definition_id FROM symbol "
	 	"WHERE scope_id = /* name:'scopeid' type:gint */ AND scope_definition_id = ("
		 	"SELECT scope.scope_id FROM scope "
			"INNER JOIN sym_type ON scope.type_id = sym_type.type_id "
			"WHERE sym_type.type_type = /* name:'symtype' type:gchararray */ "
				"AND scope.scope_name = /* name:'scopename' type:gchararray */) LIMIT 1");
	
	/* -- tmp heritage -- */
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_TMP_HERITAGE_NEW,
	 	"INSERT INTO __tmp_heritage_scope (symbol_referer_id, field_inherits, "
	 	"field_struct, field_typeref, field_enum, field_union, "
	 	"field_class, field_namespace) VALUES (## /* name:'symreferid' "
	 	"type:gint */, ## /* name:'finherits' type:gchararray */, ## /* "
	 	"name:'fstruct' type:gchararray */, ## /* name:'ftyperef' "
	 	"type:gchararray */, ## /* name:'fenum' type:gchararray */, ## /* "
	 	"name:'funion' type:gchararray */, ## /* name:'fclass' type:gchararray "
	 	"*/, ## /* name:'fnamespace' type:gchararray */)");
	
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_GET_ALL_FROM_TMP_HERITAGE,
	 	"SELECT * FROM __tmp_heritage_scope");
	
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_GET_ALL_FROM_TMP_HERITAGE_WITH_INHERITS,
	 	"SELECT * FROM __tmp_heritage_scope WHERE field_inherits != ''");
	
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_TMP_HERITAGE_DELETE_ALL,
	 	"DELETE FROM __tmp_heritage_scope");
	
	/* -- symbol -- */
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_SYMBOL_NEW,
	 	"INSERT INTO symbol (file_defined_id, name, file_position, "
	 	"is_file_scope, signature, scope_definition_id, scope_id, type_id, "
	 	"kind_id, access_kind_id, implementation_kind_id, update_flag) VALUES("
	 	"## /* name:'filedefid' type:gint */, ## /* name:'name' "
	 	"type:gchararray */, ## /* name:'fileposition' type:gint */, ## /* "
	 	"name:'isfilescope' type:gint */, ## /* name:'signature' "
	 	"type:gchararray */,## /* name:'scopedefinitionid' type:gint */, ## "
	 	"/* name:'scopeid' type:gint */,## /* name:'typeid' type:gint */, ## "
	 	"/* name:'kindid' type:gint */,## /* name:'accesskindid' type:gint */, "
	 	"## /* name:'implementationkindid' type:gint */, ## /* "
	 	"name:'updateflag' type:gint */)");
	
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_GET_SYMBOL_SCOPE_DEFINITION_ID,
	 	"SELECT scope_definition_id FROM symbol JOIN sym_type ON symbol.type_id "
	 	"= sym_type.type_id WHERE sym_type.type_type = ## /* name:'tokenname' "
	 	"type:gchararray */ AND sym_type.type_name = ## /* name:'objectname' "
	 	"type:gchararray */ LIMIT 1");
	
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_GET_SYMBOL_ID_BY_CLASS_NAME,
	 	"SELECT symbol_id FROM symbol JOIN sym_type ON symbol.type_id = "
	 	"sym_type.type_id AND symbol.scope_id = 0 WHERE AND sym_type.type_type='class' AND "
	 	"name = ## /* name:'klassname' type:gchararray */ LIMIT 1");
	
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_GET_SYMBOL_ID_BY_CLASS_NAME_AND_NAMESPACE,
	 	"SELECT symbol_id FROM symbol JOIN scope ON symbol.scope_id = "
	 	"scope.scope_id JOIN sym_type ON scope.type_id = sym_type.type_id "
	 	"WHERE symbol.name = ## /* name:'klassname' type:gchararray */ AND "
	 	"scope.scope_name = ## /* name:'namespacename' type:gchararray */ AND "
	 	"sym_type.type_type = 'namespace' LIMIT 1");
	
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_UPDATE_SYMBOL_SCOPE_ID,
	 	"UPDATE symbol SET scope_id = ## /* name:'scopeid' type:gint */ "
	 	"WHERE symbol_id = ## /* name:'symbolid' type:gint */");
	
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_GET_SYMBOL_ID_BY_UNIQUE_INDEX_KEY,
	 	"SELECT symbol_id FROM symbol WHERE name = ## /* name:'symname' "
	 	"type:gchararray */ AND file_defined_id =  ## /* name:'filedefid' "
	 	"type:gint */ AND type_id = ## /* name:'typeid' type:gint */ LIMIT 1");
	
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_UPDATE_SYMBOL_ALL,
	 	"UPDATE symbol SET is_file_scope = ## /* name:'isfilescope' type:gint "
	 	"*/, file_position = ## /* name:'fileposition' type:gint */, "
	 	"signature = ## /* name:'signature' type:gchararray */, "
	 	"scope_definition_id = ## /* name:'scopedefinitionid' type:gint */, "
	 	"scope_id = ## /* name:'scopeid' type:gint */, kind_id = "
	 	"## /* name:'kindid' type:gint */, access_kind_id = ## /* name:"
	 	"'accesskindid' type:gint */, implementation_kind_id = ## /* name:"
	 	"'implementationkindid' type:gint */, update_flag = ## /* name:"
	 	"'updateflag' type:gint */ WHERE symbol_id = ## /* name:'symbolid' type:"
	 	"gint */");
	
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_REMOVE_NON_UPDATED_SYMBOLS,
	 	"DELETE FROM symbol WHERE file_defined_id = (SELECT file_id FROM file "
	 	"WHERE file_path = ## /* name:'filepath' type:gchararray */) "
	 	"AND update_flag = 0");
	
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_RESET_UPDATE_FLAG_SYMBOLS,
	 	"UPDATE symbol SET update_flag = 0 "
	 	"WHERE file_defined_id = (SELECT file_id FROM file WHERE "
	 	"file_path = ## /* name:'filepath' type:gchararray */)");
	
	/* -- tmp_removed -- */
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_GET_REMOVED_IDS,
	 	"SELECT symbol_removed_id FROM __tmp_removed");
	
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_TMP_REMOVED_DELETE_ALL,
	 	"DELETE FROM __tmp_removed");

	
	/*
	 * DYNAMIC QUERY STRUCTURE INITIALIZE
	 */
	DYN_QUERY_POPULATE_INIT_NODE(sdbe->priv->dyn_query_list,
									DYN_PREP_QUERY_GET_CLASS_PARENTS,
									TRUE);
	
	DYN_QUERY_POPULATE_INIT_NODE(sdbe->priv->dyn_query_list,
									DYN_PREP_QUERY_GET_CLASS_PARENTS_BY_SYMBOL_ID,
									FALSE);
	
	DYN_QUERY_POPULATE_INIT_NODE(sdbe->priv->dyn_query_list,
									DYN_PREP_QUERY_GET_GLOBAL_MEMBERS_FILTERED,
									TRUE);
	
	DYN_QUERY_POPULATE_INIT_NODE(sdbe->priv->dyn_query_list,
									DYN_PREP_QUERY_GET_SCOPE_MEMBERS,
									FALSE);
	
	DYN_QUERY_POPULATE_INIT_NODE(sdbe->priv->dyn_query_list,
									DYN_PREP_QUERY_GET_CURRENT_SCOPE,				
									FALSE);
	
	DYN_QUERY_POPULATE_INIT_NODE(sdbe->priv->dyn_query_list,
									DYN_PREP_QUERY_GET_FILE_SYMBOLS,
									FALSE);

	DYN_QUERY_POPULATE_INIT_NODE(sdbe->priv->dyn_query_list,
								 	DYN_PREP_QUERY_GET_SYMBOL_INFO_BY_ID,
									FALSE);

	DYN_QUERY_POPULATE_INIT_NODE(sdbe->priv->dyn_query_list,
								 	DYN_PREP_QUERY_FIND_SYMBOL_NAME_BY_PATTERN,
									FALSE);

	DYN_QUERY_POPULATE_INIT_NODE(sdbe->priv->dyn_query_list,
									DYN_PREP_QUERY_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED,			
									TRUE);

	DYN_QUERY_POPULATE_INIT_NODE(sdbe->priv->dyn_query_list,
								 	DYN_PREP_QUERY_GET_SCOPE_MEMBERS_BY_SYMBOL_ID,
									TRUE);
	
	/* init cache hashtables */
	sdb_engine_init_caches (sdbe);
}

static void
sdb_engine_unlink_shared_files (gpointer key, gpointer value, gpointer user_data)
{
	shm_unlink (key);
}

static void
sdb_engine_finalize (GObject * object)
{
	SymbolDBEngine *dbe;
	SymbolDBEnginePriv *priv;
	
	dbe = SYMBOL_DB_ENGINE (object);
	priv = dbe->priv;
	
	if (priv->timeout_trigger_handler > 0)
		g_source_remove (priv->timeout_trigger_handler);
	
	if (priv->thread_monitor_handler > 0)
		g_source_remove (priv->thread_monitor_handler);
		
	sdb_engine_disconnect_from_db (dbe);
	sdb_engine_free_cached_queries (dbe);
	sdb_engine_free_cached_dynamic_queries (dbe);
	
	if (priv->scan_queue)
	{
		g_async_queue_unref (priv->scan_queue);
		priv->scan_queue = NULL;
	}
	
	if (priv->updated_symbols_id)
	{
		g_async_queue_unref (priv->updated_symbols_id);
		priv->updated_symbols_id = NULL;
	}	

	if (priv->inserted_symbols_id)
	{
		g_async_queue_unref (priv->inserted_symbols_id);
		priv->inserted_symbols_id = NULL;
	}	
	
	if (priv->shared_mem_file) 
	{
		fclose (priv->shared_mem_file);
		priv->shared_mem_file = NULL; 
	}	
	
	if (priv->shared_mem_str)
	{
		shm_unlink (priv->shared_mem_str);
		g_free (priv->shared_mem_str);
		priv->shared_mem_str = NULL;
	}
	
	if (priv->garbage_shared_mem_files)
	{
		g_hash_table_foreach (priv->garbage_shared_mem_files, 
							  sdb_engine_unlink_shared_files,
							  NULL);
		/* destroy the hash table */
		g_hash_table_destroy (priv->garbage_shared_mem_files);
	}
	
	if (priv->ctags_launcher)
	{
		anjuta_launcher_signal (priv->ctags_launcher, SIGINT);
		g_object_unref (priv->ctags_launcher);
	}	
	
	if (priv->mutex)
	{
		g_mutex_free (priv->mutex);
		priv->mutex = NULL;
	}
	
	if (priv->thread_list != NULL)
		g_queue_free  (priv->thread_list);

	if (priv->sym_type_conversion_hash)
		g_hash_table_destroy (priv->sym_type_conversion_hash);
	
	if (priv->signals_queue != NULL)
		g_async_queue_unref (priv->signals_queue);
	
	sdb_engine_clear_caches (dbe);
	
	g_free (priv);
	
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
sdb_engine_class_init (SymbolDBEngineClass * klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));

	object_class->finalize = sdb_engine_finalize;

	signals[SINGLE_FILE_SCAN_END]
		= g_signal_new ("single-file-scan-end",
						G_OBJECT_CLASS_TYPE (object_class),
						G_SIGNAL_RUN_FIRST,
						G_STRUCT_OFFSET (SymbolDBEngineClass, single_file_scan_end),
						NULL, NULL,
						g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	
	signals[SCAN_END]
		= g_signal_new ("scan-end",
						G_OBJECT_CLASS_TYPE (object_class),
						G_SIGNAL_RUN_FIRST,
						G_STRUCT_OFFSET (SymbolDBEngineClass, scan_end),
						NULL, NULL,
						g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	signals[SYMBOL_INSERTED]
		= g_signal_new ("symbol-inserted",
						G_OBJECT_CLASS_TYPE (object_class),
						G_SIGNAL_RUN_FIRST,
						G_STRUCT_OFFSET (SymbolDBEngineClass, symbol_inserted),
						NULL, NULL,
						g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 
						1,
						G_TYPE_INT);
	
	signals[SYMBOL_UPDATED]
		= g_signal_new ("symbol-updated",
						G_OBJECT_CLASS_TYPE (object_class),
						G_SIGNAL_RUN_FIRST,
						G_STRUCT_OFFSET (SymbolDBEngineClass, symbol_updated),
						NULL, NULL,
						g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 
						1,
						G_TYPE_INT);

	signals[SYMBOL_SCOPE_UPDATED]
		= g_signal_new ("symbol-scope-updated",
						G_OBJECT_CLASS_TYPE (object_class),
						G_SIGNAL_RUN_FIRST,
						G_STRUCT_OFFSET (SymbolDBEngineClass, symbol_scope_updated),
						NULL, NULL,
						g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 
						1,
						G_TYPE_INT);
	
	signals[SYMBOL_REMOVED]
		= g_signal_new ("symbol-removed",
						G_OBJECT_CLASS_TYPE (object_class),
						G_SIGNAL_RUN_FIRST,
						G_STRUCT_OFFSET (SymbolDBEngineClass, symbol_removed),
						NULL, NULL,
						g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 
						1,
						G_TYPE_INT);
	
}

GType
sdb_engine_get_type (void)
{
	static GType our_type = 0;

	if (our_type == 0)
	{
		static const GTypeInfo our_info = {
			sizeof (SymbolDBEngineClass),	/* class_size */
			(GBaseInitFunc) NULL,	/* base_init */
			(GBaseFinalizeFunc) NULL,	/* base_finalize */
			(GClassInitFunc) sdb_engine_class_init,	/* class_init */
			(GClassFinalizeFunc) NULL,	/* class_finalize */
			NULL /* class_data */ ,
			sizeof (SymbolDBEngine),	/* instance_size */
			0,					/* n_preallocs */
			(GInstanceInitFunc) sdb_engine_init,	/* instance_init */
			NULL				/* value_table */
		};

		our_type = g_type_register_static (G_TYPE_OBJECT, "SymbolDBEngine",
										   &our_info, 0);
	}

	return our_type;
}

SymbolDBEngine *
symbol_db_engine_new (void)
{
	SymbolDBEngine *sdbe;
	SymbolDBEnginePriv *priv;

	sdbe = g_object_new (SYMBOL_TYPE_DB_ENGINE, NULL);
	
	priv = sdbe->priv;
	priv->mutex = g_mutex_new ();
	return sdbe;
}

/**
 * Set some default parameters to use with the current database.
 */
static void
sdb_engine_set_defaults_db_parameters (SymbolDBEngine * dbe)
{	
	sdb_engine_execute_unknown_sql (dbe, "PRAGMA page_size = 32768");
	sdb_engine_execute_unknown_sql (dbe, "PRAGMA cache_size = 12288");
	sdb_engine_execute_unknown_sql (dbe, "PRAGMA synchronous = OFF");
	sdb_engine_execute_unknown_sql (dbe, "PRAGMA temp_store = MEMORY");
	sdb_engine_execute_unknown_sql (dbe, "PRAGMA case_sensitive_like = 1");
}

/* Will create priv->db_connection.
 * Connect to database identified by db_directory.
 * Usually db_directory is defined also into priv. We let it here as parameter 
 * because it is required and cannot be null.
 */
static gboolean
sdb_engine_connect_to_db (SymbolDBEngine * dbe, const gchar *cnc_string)
{
	SymbolDBEnginePriv *priv;

	g_return_val_if_fail (dbe != NULL, FALSE);
	priv = dbe->priv;

	priv->mutex = g_mutex_new ();
	priv->signals_queue = g_async_queue_new ();
	
	if (priv->db_connection != NULL)
	{
		/* if it's the case that the connection isn't NULL, we 
		 * should notify the user
		 * and return FALSE. It's his task to disconnect and retry to connect */
		g_warning ("connection is already established. Please disconnect "
				   "and then try to reconnect.");
		return FALSE;
	}

	/* establish a connection. If the sqlite file does not exist it will 
	 * be created 
	 */
	priv->db_connection
		= gda_connection_open_from_string ("SQLite", cnc_string, NULL, 
										   GDA_CONNECTION_OPTIONS_NONE, NULL);	
	
	if (!GDA_IS_CONNECTION (priv->db_connection))
	{
		g_warning ("Could not open connection to %s\n", cnc_string);
		return FALSE;
	}

	priv->sql_parser = gda_connection_create_parser (priv->db_connection);
	
	if (!GDA_IS_SQL_PARSER (priv->sql_parser)) 
	{
		g_warning ("Could not create sql parser. Check your libgda installation");
		return FALSE;
	}
	
	DEBUG_PRINT ("connected to database %s", cnc_string);
	return TRUE;
}


/**
 * Creates required tables for the database to work.
 * @param tables_sql_file File containing sql code.
 */
static gboolean
sdb_engine_create_db_tables (SymbolDBEngine * dbe, const gchar * tables_sql_file)
{
	GError *err;
	SymbolDBEnginePriv *priv;
	gchar *contents;
	gsize sizez;

	g_return_val_if_fail (tables_sql_file != NULL, FALSE);

	priv = dbe->priv;

	g_return_val_if_fail (priv->db_connection != NULL, FALSE);

	/* read the contents of the file */
	if (g_file_get_contents (tables_sql_file, &contents, &sizez, &err) == FALSE)
	{
		g_warning ("Something went wrong while trying to read %s",
				   tables_sql_file);

		if (err != NULL)
			g_message ("%s", err->message);
		return FALSE;
	}

	sdb_engine_execute_non_select_sql (dbe, contents);
	
	g_free (contents);
	return TRUE;
}

/**
 * Check if the database already exists into the prj_directory
 */
gboolean
symbol_db_engine_db_exists (SymbolDBEngine * dbe, const gchar * prj_directory)
{
	SymbolDBEnginePriv *priv;

	g_return_val_if_fail (prj_directory != NULL, FALSE);

	priv = dbe->priv;

	/* check whether the db filename already exists.*/
	gchar *tmp_file = g_strdup_printf ("%s/%s.db", prj_directory,
									   ANJUTA_DB_FILE);
	
	if (g_file_test (tmp_file, G_FILE_TEST_EXISTS) == FALSE)
	{
		DEBUG_PRINT ("db %s does not exist", tmp_file);
		g_free (tmp_file);
		return FALSE;
	}

	g_free (tmp_file);
	return TRUE;
}

/**
 * Check if a file is already present [and scanned] in db.
 */
gboolean
symbol_db_engine_file_exists (SymbolDBEngine * dbe, const gchar * abs_file_path)
{
	SymbolDBEnginePriv *priv;
	gchar *relative;
	gint file_defined_id;
	GValue *value;

	g_return_val_if_fail (dbe != NULL, FALSE);
	g_return_val_if_fail (abs_file_path != NULL, FALSE);
	
	priv = dbe->priv;

	if (priv->mutex)
	{
		g_mutex_lock (priv->mutex);
	}
	
	relative = symbol_db_engine_get_file_db_path (dbe, abs_file_path);
	if (relative == NULL)
	{
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);
		return FALSE;
	}	
	value = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value, relative);	

	if ((file_defined_id = sdb_engine_get_tuple_id_by_unique_name (dbe,
													PREP_QUERY_GET_FILE_ID_BY_UNIQUE_NAME,
													"filepath",
													value)) < 0)
	{	
		g_free (relative);
		gda_value_free (value);
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);
		return FALSE;	
	}
	
	g_free (relative);
	gda_value_free (value);
	if (priv->mutex)
		g_mutex_unlock (priv->mutex);
	return TRUE;	
}

gboolean 
symbol_db_engine_close_db (SymbolDBEngine *dbe)
{
	SymbolDBEnginePriv *priv;

	g_return_val_if_fail (dbe != NULL, FALSE);
	
	priv = dbe->priv;
	return sdb_engine_disconnect_from_db (dbe);
}

/**
 * Open or create a new database at given directory.
 */
gboolean
symbol_db_engine_open_db (SymbolDBEngine * dbe, const gchar * base_db_path,
						  const gchar * prj_directory)
{
	SymbolDBEnginePriv *priv;
	gboolean needs_tables_creation = FALSE;
	gchar *cnc_string;

	g_return_val_if_fail (dbe != NULL, FALSE);
	g_return_val_if_fail (base_db_path != NULL, FALSE);

	priv = dbe->priv;

	/* check whether the db filename already exists. If it's not the case
	 * create the tables for the database. */
	gchar *tmp_file = g_strdup_printf ("%s/%s.db", base_db_path,
									   ANJUTA_DB_FILE);

	if (g_file_test (tmp_file, G_FILE_TEST_EXISTS) == FALSE)
	{
		needs_tables_creation = TRUE;
	}
	g_free (tmp_file);


	priv->db_directory = g_strdup (base_db_path);
	
	/* save the project_directory */
	priv->project_directory = g_strdup (prj_directory);

	cnc_string = g_strdup_printf ("DB_DIR=%s;DB_NAME=%s", base_db_path,
								ANJUTA_DB_FILE);

/*	
	FIXME: do we really need to save the datasource? 
	if (gda_config_save_data_source (base_db_path, "SQLite",
									 cnc_string, "Anjuta Project",
									 "", "", FALSE) == FALSE)
	{
		return FALSE;
	}
*/
	DEBUG_PRINT ("symbol_db_engine_open_db (): opening/connecting to database...");
	sdb_engine_connect_to_db (dbe, cnc_string);

	if (needs_tables_creation == TRUE)
	{
		DEBUG_PRINT ("symbol_db_engine_open_db (): creating tables: it needs tables...");
		sdb_engine_create_db_tables (dbe, TABLES_SQL);
	}

	sdb_engine_set_defaults_db_parameters (dbe);
	
	return TRUE;
}

gboolean
symbol_db_engine_add_new_workspace (SymbolDBEngine * dbe,
									const gchar * workspace_name)
{
/*
CREATE TABLE workspace (workspace_id integer PRIMARY KEY AUTOINCREMENT,
                        workspace_name varchar (50) not null unique,
                        analyse_time DATE
                        );
*/
	const GdaSet *plist;
	const GdaStatement *stmt;
	GdaHolder *param;
	SymbolDBEnginePriv *priv;

	g_return_val_if_fail (dbe != NULL, FALSE);	
	priv = dbe->priv;

	g_return_val_if_fail (priv->db_connection != NULL, FALSE);

	if ((stmt =
		 sdb_engine_get_statement_by_query_id (dbe, PREP_QUERY_WORKSPACE_NEW)) == NULL)
	{
		g_warning ("query is null");
		return FALSE;
	}

	plist = sdb_engine_get_query_parameters_list (dbe, PREP_QUERY_WORKSPACE_NEW);

	if ((param = gda_set_get_holder ((GdaSet*)plist, "wsname")) == NULL)
	{
		g_warning ("param is NULL from pquery!\n");
		return FALSE;
	}
	gda_holder_set_value_str (param, NULL, workspace_name);
	
	/* execute the query with parametes just set */
	if (gda_connection_statement_execute_non_select (priv->db_connection, 
														  (GdaStatement*)stmt, 
														  (GdaSet*)plist, NULL, NULL) == -1)
	{		
		return FALSE;
	}

	return TRUE;
}

/**
 * Test it project_name is created in the opened database
 */
gboolean
symbol_db_engine_project_exists (SymbolDBEngine * dbe,	/*gchar* workspace, */
							   	const gchar * project_name)
{
	GValue *value;
	SymbolDBEnginePriv *priv;
	gint prj_id;

	priv = dbe->priv;

	g_return_val_if_fail (priv->db_connection != NULL, FALSE);
	value = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value, project_name);

	/* test the existence of the project in db */
	if ((prj_id = sdb_engine_get_tuple_id_by_unique_name (dbe,
				PREP_QUERY_GET_PROJECT_ID_BY_UNIQUE_NAME,
				"prjname",
				 value)) <= 0)
	{
		DEBUG_PRINT ("symbol_db_engine_project_exists (): no project named %s found",
					 project_name);
		gda_value_free (value);
		return FALSE;
	}

	gda_value_free (value);

	/* we found it */
	return TRUE;
}


/**
 * @param workspace Can be NULL. In that case a default workspace will be created, 
 * 					and project will depend on that.
 * @param project Project name. Must NOT be NULL.
 */
gboolean
symbol_db_engine_add_new_project (SymbolDBEngine * dbe, const gchar * workspace,
								  const gchar * project)
{
/*
CREATE TABLE project (project_id integer PRIMARY KEY AUTOINCREMENT,
                      project_name varchar (50) not null unique,
                      wrkspace_id integer REFERENCES workspace (workspace_id),
                      analyse_time DATE
                      );
*/
	const GdaSet *plist;
	const GdaStatement *stmt;
	GdaHolder *param;
	GValue *value;
	const gchar *workspace_name;
	gint wks_id;
	SymbolDBEnginePriv *priv;

	g_return_val_if_fail (dbe != NULL, FALSE);	
	priv = dbe->priv;

	if (workspace == NULL)
	{
		workspace_name = "anjuta_workspace_default";	
		
		DEBUG_PRINT ("adding default workspace... '%s'", workspace_name);
		value = gda_value_new (G_TYPE_STRING);
		g_value_set_string (value, workspace_name);
		
		if ((wks_id = sdb_engine_get_tuple_id_by_unique_name (dbe,
				 PREP_QUERY_GET_WORKSPACE_ID_BY_UNIQUE_NAME,
				 "wsname",
				 value)) <= 0)
		{ 
			if (symbol_db_engine_add_new_workspace (dbe, workspace_name) == FALSE)
			{
				gda_value_free (value);
				DEBUG_PRINT ("Project cannot be added because a default workspace "
							 "cannot be created");
				return FALSE;
			}
		}
		gda_value_free (value);
	}
	else
	{
		workspace_name = workspace;
	}

	value = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value, workspace_name);

	/* get workspace id */
	if ((wks_id = sdb_engine_get_tuple_id_by_unique_name (dbe,
				 PREP_QUERY_GET_WORKSPACE_ID_BY_UNIQUE_NAME,
				 "wsname",
				 value)) <= 0)
	{
		DEBUG_PRINT ("symbol_db_engine_add_new_project (): no workspace id");
		gda_value_free (value);
		return FALSE;
	}
	gda_value_free (value);
	
	/* insert new project */
	if ((stmt =
		 sdb_engine_get_statement_by_query_id (dbe, PREP_QUERY_PROJECT_NEW)) == NULL)
	{
		g_warning ("query is null");
		return FALSE;
	}

	plist = sdb_engine_get_query_parameters_list (dbe, PREP_QUERY_PROJECT_NEW);	
	
	/* lookup parameters */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "prjname")) == NULL)
	{
		g_warning ("param prjname is NULL from pquery!");
		return FALSE;
	}
	gda_holder_set_value_str (param, NULL, project);
		
	if ((param = gda_set_get_holder ((GdaSet*)plist, "wsid")) == NULL)
	{
		g_warning ("param prjname is NULL from pquery!");
		return FALSE;
	}
	value = gda_value_new (G_TYPE_INT);
	g_value_set_int (value, wks_id);	
	gda_holder_set_value (param, value);
		
	/* execute the query with parametes just set */
	if (gda_connection_statement_execute_non_select (priv->db_connection, 
														  (GdaStatement*)stmt, 
														  (GdaSet*)plist, NULL, NULL) == -1)
	{		
		gda_value_free (value);
		return FALSE;
	}
	gda_value_free (value);	
	return TRUE;
}


static gint
sdb_engine_add_new_language (SymbolDBEngine * dbe, const gchar *language)
{
/*
CREATE TABLE language (language_id integer PRIMARY KEY AUTOINCREMENT,
                       language_name varchar (50) not null unique);
*/
	gint table_id;
	GValue *value;
	SymbolDBEnginePriv *priv;
		
	g_return_val_if_fail (language != NULL, -1);

	priv = dbe->priv;
	
	value = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value, language);

	/* check for an already existing table with language "name". */
	if ((table_id = sdb_engine_get_tuple_id_by_unique_name (dbe,
						PREP_QUERY_GET_LANGUAGE_ID_BY_UNIQUE_NAME,
						"langname",
						value)) < 0)
	{
		/* insert a new entry on db */
		const GdaSet *plist;
		const GdaStatement *stmt;
		GdaHolder *param;
		GdaSet *last_inserted;

		if ((stmt = sdb_engine_get_statement_by_query_id (dbe, PREP_QUERY_LANGUAGE_NEW))
			== NULL)
		{
			g_warning ("query is null");
			return FALSE;
		}

		plist = sdb_engine_get_query_parameters_list (dbe, PREP_QUERY_LANGUAGE_NEW);

		if ((param = gda_set_get_holder ((GdaSet*)plist, "langname")) == NULL)
		{
			g_warning ("param langname is NULL from pquery!");
			return FALSE;
		}
		gda_holder_set_value_str (param, NULL, language);
		
		GError *err = NULL;
		/* execute the query with parametes just set */
		if (gda_connection_statement_execute_non_select (priv->db_connection, 
														 (GdaStatement*)stmt, 
														 (GdaSet*)plist, &last_inserted,
														 &err) == -1)
		{		
			DEBUG_PRINT ("Error: %s", err->message);
			table_id = -1;
		}
		else {
			const GValue *value = gda_set_get_holder_value (last_inserted, "+0");
			table_id = g_value_get_int (value);
		}		
	}
	gda_value_free (value);	
	
	return table_id;
}


/* Add a file to project. 
 * This function requires an opened db, i.e. calling before
 * symbol_db_engine_open_db ()
 * filepath: referes to a full file path.
 * project: 
 * WARNING: we suppose that project_directory is already set.
 * WARNING2: we suppose that the given local_filepath include the project_directory path.
 * + correct example: local_filepath: /home/user/projects/foo_project/src/main.c
 *                    project_directory: /home/user/projects/foo_project
 * - wrong one: local_filepath: /tmp/foo.c
 *                    project_directory: /home/user/projects/foo_project
 */
static gboolean
sdb_engine_add_new_file (SymbolDBEngine * dbe, const gchar * project_name,
						 const gchar * local_filepath, const gchar * language)
{
/*
CREATE TABLE file (file_id integer PRIMARY KEY AUTOINCREMENT,
                   file_path TEXT not null unique,
                   prj_id integer REFERENCES project (projec_id),
                   lang_id integer REFERENCES language (language_id),
                   analyse_time DATE
                   );
*/
	SymbolDBEnginePriv *priv;
	gint project_id;
	gint language_id;
	gint file_id;
	GValue *value;
	
	priv = dbe->priv;

	/* check if the file is a correct one compared to the local_filepath */
	if (strstr (local_filepath, priv->project_directory) == NULL)
		return FALSE;
	
/*	DEBUG_PRINT ("sdb_engine_add_new_file project_name %s local_filepath %s language %s",
				 project_name, local_filepath, language);*/
	
	value = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value, project_name);

	/* check for an already existing table with project "project". */
	if ((project_id = sdb_engine_get_tuple_id_by_unique_name (dbe,
								  PREP_QUERY_GET_PROJECT_ID_BY_UNIQUE_NAME,
								  "prjname",
								  value)) < 0)
	{
		g_warning ("no project with that name exists");
		gda_value_free (value);
		return FALSE;
	}

	gda_value_free (value);
	value = gda_value_new (G_TYPE_STRING);
	/* we're gonna set the file relative to the project folder, not the full one.
	 * e.g.: we have a file on disk: "/tmp/foo/src/file.c" and a db_directory located on
	 * "/tmp/foo/". The entry on db will be "src/file.c" 
	 */
	gchar *relative_path = symbol_db_engine_get_file_db_path (dbe, local_filepath);
	if (relative_path == NULL)
	{
		return FALSE;
	}	
	g_value_set_string (value, relative_path);	

	if ((file_id = sdb_engine_get_tuple_id_by_unique_name (dbe,
								   PREP_QUERY_GET_FILE_ID_BY_UNIQUE_NAME,
								   "filepath",
								   value)) < 0)
	{
		/* insert a new entry on db */
		const GdaSet *plist;
		const GdaStatement *stmt;
		GdaHolder *param;
		GValue *value;

		language_id = sdb_engine_add_new_language (dbe, language);

		if ((stmt = sdb_engine_get_statement_by_query_id (dbe, PREP_QUERY_FILE_NEW))
			== NULL)
		{
			g_warning ("query is null");
			g_free (relative_path);
			return FALSE;
		}

		plist = sdb_engine_get_query_parameters_list (dbe, PREP_QUERY_FILE_NEW);
		
		/* filepath parameter */
		if ((param = gda_set_get_holder ((GdaSet*)plist, "filepath")) == NULL)
		{
			g_warning ("param langname is NULL from pquery!");
			g_free (relative_path);			
			return FALSE;
		}
		
		gda_holder_set_value_str (param, NULL, relative_path);

		/* project id parameter */
		if ((param = gda_set_get_holder ((GdaSet*)plist, "prjid")) == NULL)
		{
			g_warning ("param prjid is NULL from pquery!");
			g_free (relative_path);
			return FALSE;
		}
		value = gda_value_new (G_TYPE_INT);
		g_value_set_int (value, project_id);
		gda_holder_set_value (param, value);

		/* language id parameter */
		if ((param = gda_set_get_holder ((GdaSet*)plist, "langid")) == NULL)
		{
			g_warning ("param langid is NULL from pquery!");
			g_free (relative_path);
			return FALSE;
		}

		gda_value_reset_with_type (value, G_TYPE_INT);
		g_value_set_int (value, language_id);
		gda_holder_set_value (param, value);
		gda_value_free (value);

		/* execute the query with parametes just set */
		if (gda_connection_statement_execute_non_select (priv->db_connection, 
														 (GdaStatement*)stmt, 
														 (GdaSet*)plist, NULL,
														 NULL) == -1)
		{		
			g_free (relative_path);
			return FALSE;
		}	
	}
	gda_value_free (value);
	g_free (relative_path);
	
	return TRUE;
} 

gboolean
symbol_db_engine_add_new_files (SymbolDBEngine * dbe, 
								const gchar * project_name,
								const GPtrArray * files_path, 
								const GPtrArray * languages,
								gboolean force_scan)
{
	gint i;
	SymbolDBEnginePriv *priv;
	GPtrArray * filtered_files_path;
	GPtrArray * filtered_languages;
	gboolean ret_code;
	g_return_val_if_fail (dbe != NULL, FALSE);
	priv = dbe->priv;

	g_return_val_if_fail (priv->db_connection != NULL, FALSE);
	g_return_val_if_fail (project_name != NULL, FALSE);
	g_return_val_if_fail (files_path->len > 0, FALSE);
	g_return_val_if_fail (languages->len > 0, FALSE);

	filtered_files_path = g_ptr_array_new ();
	filtered_languages = g_ptr_array_new ();
	
	for (i = 0; i < files_path->len; i++)
	{
		gchar *node_file = (gchar *) g_ptr_array_index (files_path, i);
		gchar *node_lang = (gchar *) g_ptr_array_index (languages, i);
		
		/* test the existance of node file */
		if (g_file_test (node_file, G_FILE_TEST_EXISTS) == FALSE)
		{
			g_warning ("File %s does NOT exist", node_file);
			continue;
		}
		
		if (force_scan == FALSE)
		{
			if (symbol_db_engine_file_exists (dbe, node_file) == TRUE)
				/* we don't want to touch the already present file... within 
				 * its symbols
				 */
				continue;
		}
		
		if (sdb_engine_add_new_file (dbe, project_name, node_file, 
									 node_lang) == FALSE)
		{
			g_warning ("Error processing file %s, db_directory %s, project_name %s, "
					   "project_directory %s", node_file, 
					   priv->db_directory, project_name, priv->project_directory);
			return FALSE;
		}
		
		/* note: we don't use g_strdup () here because we'll free the filtered_files_path
		 * before returning from this function.
		 */
		g_ptr_array_add (filtered_files_path, node_file);
	}

	/* perform the scan of files. It will spawn a fork() process with 
	 * AnjutaLauncher and ctags in server mode. After the ctags cmd has been 
	 * executed, the populating process'll take place.
	 */
	ret_code = sdb_engine_scan_files_1 (dbe, filtered_files_path, NULL, FALSE);
	g_ptr_array_free (filtered_files_path, TRUE);
	return ret_code;
}


static gint
sdb_engine_add_new_sym_type (SymbolDBEngine * dbe, const tagEntry * tag_entry)
{
/*
	CREATE TABLE sym_type (type_id integer PRIMARY KEY AUTOINCREMENT,
                   type_type varchar (256) not null ,
                   type_name varchar (256) not null ,
                   unique (type_type, type_name)
                   );
*/
	const gchar *type;
	const gchar *type_name;
	gint table_id;	
	const GdaSet *plist;
	const GdaStatement *stmt;
	GdaHolder *param;
	GdaSet *last_inserted;
	SymbolDBEnginePriv *priv;
	
	priv = dbe->priv;
	
	/* we assume that tag_entry is != NULL */
	type = tag_entry->kind;
	type_name = tag_entry->name;
	
	/* it does not exist. Create a new tuple. */
	if ((stmt = sdb_engine_get_statement_by_query_id (dbe, PREP_QUERY_SYM_TYPE_NEW))
		== NULL)
	{
		g_warning ("query is null");
		return -1;
	}

	plist = sdb_engine_get_query_parameters_list (dbe, PREP_QUERY_SYM_TYPE_NEW);

	/* type parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "type")) == NULL)
	{
		g_warning ("param type is NULL from pquery!");
		return -1;
	}
	gda_holder_set_value_str (param, NULL, type);

	/* type_name parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "typename")) == NULL)
	{
		g_warning ("param typename is NULL from pquery!");
		return -1;
	}
	gda_holder_set_value_str (param, NULL, type_name);
	
	/* execute the query with parametes just set */
	if (gda_connection_statement_execute_non_select (priv->db_connection, 
													 (GdaStatement*)stmt, 
													 (GdaSet*)plist, &last_inserted,
													 NULL) == -1)
	{
		GValue *value1, *value2;

		value1 = gda_value_new (G_TYPE_STRING);
		g_value_set_string (value1, type);

		value2 = gda_value_new (G_TYPE_STRING);
		g_value_set_string (value2, type_name);

		if ((table_id = sdb_engine_get_tuple_id_by_unique_name2 (dbe,
													 PREP_QUERY_GET_SYM_TYPE_ID,
													 "type", value1,
													 "typename", value2)) < 0)
		{
			table_id = -1;
		}

		gda_value_free (value1);
		gda_value_free (value2);
		return table_id;
	}	
	else 
	{
		const GValue *value = gda_set_get_holder_value (last_inserted, "+0");
		table_id = g_value_get_int (value);
	}		
	
	return table_id;
}

static gint
sdb_engine_add_new_sym_kind (SymbolDBEngine * dbe, const tagEntry * tag_entry)
{
/* 
	CREATE TABLE sym_kind (sym_kind_id integer PRIMARY KEY AUTOINCREMENT,
                       kind_name varchar (50) not null unique
                       );
*/
	const gchar *kind_name;
	gint table_id;
	GValue *value;
	SymbolDBEnginePriv *priv;
	
	priv = dbe->priv;

	/* we assume that tag_entry is != NULL */
	kind_name = tag_entry->kind;

	/* no kind associated with current tag */
	if (kind_name == NULL)
		return -1;

	table_id = sdb_engine_cache_lookup (priv->kind_cache, kind_name);
	if (table_id != -1)
	{
		return table_id;
	}

	value = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value, kind_name);
		
	
	if ((table_id = sdb_engine_get_tuple_id_by_unique_name (dbe,
										PREP_QUERY_GET_SYM_KIND_BY_UNIQUE_NAME,
										"kindname",
										value)) < 0)
	{
		const GdaSet *plist;
		const GdaStatement *stmt;
		GdaHolder *param;
		GdaSet *last_inserted;

		/* not found. Go on with inserting  */
		if ((stmt = sdb_engine_get_statement_by_query_id (dbe, PREP_QUERY_SYM_KIND_NEW))
			== NULL)
		{
			g_warning ("query is null");
			return -1;
		}

		plist = sdb_engine_get_query_parameters_list (dbe, PREP_QUERY_SYM_KIND_NEW);
		
		/* kindname parameter */
		if ((param = gda_set_get_holder ((GdaSet*)plist, "kindname")) == NULL)
		{
			g_warning ("param kindname is NULL from pquery!");
			return FALSE;
		}
		gda_holder_set_value_str (param, NULL, kind_name);

	
		/* execute the query with parametes just set */
		GError *err = NULL;
		if (gda_connection_statement_execute_non_select (priv->db_connection, 
														 (GdaStatement*)stmt, 
														 (GdaSet*)plist, &last_inserted,
														 &err) == -1)
		{		
			table_id = -1;		
		}			
		else
		{
			const GValue *value = gda_set_get_holder_value (last_inserted, "+0");
			table_id = g_value_get_int (value);
			/* we should cache only tables which are != -1 */
			sdb_engine_insert_cache (priv->kind_cache, kind_name, table_id);
		}
	}
	gda_value_free (value);	
	
	return table_id;
}

static gint
sdb_engine_add_new_sym_access (SymbolDBEngine * dbe, const tagEntry * tag_entry)
{
/* 
	CREATE TABLE sym_access (access_kind_id integer PRIMARY KEY AUTOINCREMENT,
                         access_name varchar (50) not null unique
                         );
*/
	const gchar *access;
	gint table_id;
	GValue *value;
	SymbolDBEnginePriv *priv;

	priv = dbe->priv;
	
	/* we assume that tag_entry is != NULL */	
	if ((access = tagsField (tag_entry, "access")) == NULL)
	{
		/* no access associated with current tag */
		return -1;
	}
	
	table_id = sdb_engine_cache_lookup (priv->access_cache, access);
	if (table_id != -1)
	{
		return table_id;
	}

	
	value = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value, access);

	if ((table_id = sdb_engine_get_tuple_id_by_unique_name (dbe,
									PREP_QUERY_GET_SYM_ACCESS_BY_UNIQUE_NAME,
									"accesskind",
									value)) < 0)
	{
		const GdaSet *plist;
		const GdaStatement *stmt;
		GdaHolder *param;
		GdaSet *last_inserted;

		/* not found. Go on with inserting  */
		if ((stmt =
			 sdb_engine_get_statement_by_query_id (dbe,
										 PREP_QUERY_SYM_ACCESS_NEW)) == NULL)
		{
 			g_warning ("query is null");
			return -1;
		}
		
		plist = sdb_engine_get_query_parameters_list (dbe, PREP_QUERY_SYM_ACCESS_NEW);
		
		/* accesskind parameter */
		if ((param = gda_set_get_holder ((GdaSet*)plist, "accesskind")) == NULL)
		{			
			g_warning ("param accesskind is NULL from pquery!");
			return -1;
		}
		gda_holder_set_value_str (param, NULL, access);
		
		/* execute the query with parametes just set */
		GError *err = NULL;
		if (gda_connection_statement_execute_non_select (priv->db_connection, 
														 (GdaStatement*)stmt, 
														 (GdaSet*)plist, &last_inserted,
														 &err) == -1)
		{		
			table_id = -1;		
		}			
		else
		{
			const GValue *value = gda_set_get_holder_value (last_inserted, "+0");
			table_id = g_value_get_int (value);
			/* we should cache only tables which are != -1 */
			sdb_engine_insert_cache (priv->access_cache, access, table_id);
		}		
	}
	gda_value_free (value);	
	
	
	return table_id;
}


static gint
sdb_engine_add_new_sym_implementation (SymbolDBEngine * dbe,
									   const tagEntry * tag_entry)
{
/*	
	CREATE TABLE sym_implementation (sym_impl_id integer PRIMARY KEY AUTOINCREMENT,
                                 implementation_name varchar (50) not null unique
                                 );
*/
	const gchar *implementation;
	gint table_id;
	GValue *value;
	SymbolDBEnginePriv *priv;

	priv = dbe->priv;
	
	/* we assume that tag_entry is != NULL */	
	if ((implementation = tagsField (tag_entry, "implementation")) == NULL)
	{
		/* no implementation associated with current tag */
		return -1;
	}
	table_id = sdb_engine_cache_lookup (priv->implementation_cache, implementation);
	if (table_id != -1)
	{
		return table_id;
	}
	
	value = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value, implementation);

	if ((table_id = sdb_engine_get_tuple_id_by_unique_name (dbe,
							PREP_QUERY_GET_SYM_IMPLEMENTATION_BY_UNIQUE_NAME,
							"implekind",
							value)) < 0)
	{
		const GdaSet *plist;
		const GdaStatement *stmt;
		GdaHolder *param;
		GdaSet *last_inserted;

		/* not found. Go on with inserting  */
		if ((stmt = sdb_engine_get_statement_by_query_id (dbe,
										 PREP_QUERY_SYM_IMPLEMENTATION_NEW)) ==
			NULL)
		{
			g_warning ("query is null");
			return -1;
		}

		plist = sdb_engine_get_query_parameters_list (dbe, PREP_QUERY_SYM_IMPLEMENTATION_NEW);
		
		/* implekind parameter */
		if ((param = gda_set_get_holder ((GdaSet*)plist, "implekind")) == NULL)
		{			
			g_warning ("param accesskind is NULL from pquery!");
			return -1;
		}
		gda_holder_set_value_str (param, NULL, implementation);

		/* execute the query with parametes just set */
		GError *err = NULL;
		if (gda_connection_statement_execute_non_select (priv->db_connection, 
														 (GdaStatement*)stmt, 
														 (GdaSet*)plist, &last_inserted,
														 &err) == -1)
		{		
			table_id = -1;		
		}			
		else
		{
			const GValue *value = gda_set_get_holder_value (last_inserted, "+0");
			table_id = g_value_get_int (value);			
			/* we should cache only tables which are != -1 */
			sdb_engine_insert_cache (priv->implementation_cache, implementation, 
									 table_id);	
		}
	}
	gda_value_free (value);	
	
	return table_id;
}


static void
sdb_engine_add_new_heritage (SymbolDBEngine * dbe, gint base_symbol_id,
							 gint derived_symbol_id)
{
/*
	CREATE TABLE heritage (symbol_id_base integer REFERENCES symbol (symbol_id),
                       symbol_id_derived integer REFERENCES symbol (symbol_id),
                       PRIMARY KEY (symbol_id_base, symbol_id_derived)
                       );
*/
	const GdaSet *plist;
	const GdaStatement *stmt;
	GdaHolder *param;
	GValue *value;
	SymbolDBEnginePriv *priv;

	g_return_if_fail (base_symbol_id > 0);
	g_return_if_fail (derived_symbol_id > 0);

	priv = dbe->priv;
	
	if ((stmt = sdb_engine_get_statement_by_query_id (dbe, PREP_QUERY_HERITAGE_NEW))
		== NULL)
	{
		g_warning ("query is null");
		return;
	}

	plist = sdb_engine_get_query_parameters_list (dbe, PREP_QUERY_HERITAGE_NEW);
		
	/* symbase parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "symbase")) == NULL)
	{			
		g_warning ("param accesskind is NULL from pquery!");
		return;
	}
	value = gda_value_new (G_TYPE_INT);
	g_value_set_int (value, base_symbol_id);
	gda_holder_set_value (param, value);

	/* symderived id parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "symderived")) == NULL)
	{
		g_warning ("param symderived is NULL from pquery!");
		return;
	}
	gda_value_reset_with_type (value, G_TYPE_INT);
	g_value_set_int (value, derived_symbol_id);
	gda_holder_set_value (param, value);
	gda_value_free (value);

	/* execute the query with parametes just set */
	if (gda_connection_statement_execute_non_select (priv->db_connection, 
													 (GdaStatement*)stmt, 
													 (GdaSet*)plist, NULL,
													 NULL) == -1)
	{		
		g_warning ("Error adding heritage");
	}	
}


static gint
sdb_engine_add_new_scope_definition (SymbolDBEngine * dbe, const tagEntry * tag_entry,
									 gint type_table_id)
{
/*
	CREATE TABLE scope (scope_id integer PRIMARY KEY AUTOINCREMENT,
                    scope_name varchar(256) not null,
                    type_id integer REFERENCES sym_type (type_id),
					unique (scope_name, type_id)
                    );
*/
	const gchar *scope;
	gint table_id;
	const GdaSet *plist;
	const GdaStatement *stmt;
	GdaHolder *param;
	GdaSet *last_inserted;
	GValue *value;
	SymbolDBEnginePriv *priv;

	g_return_val_if_fail (tag_entry->kind != NULL, -1);

	priv = dbe->priv;
	
	/* This symbol will define a scope which name is tag_entry->name
	 * For example if we get a tag MyFoo with kind "namespace", it will define 
	 * the "MyFoo" scope, which type is "namespace MyFoo"
	 */
	scope = tag_entry->name;

	/* filter out 'variable' and 'member' kinds. They define no scope. */
	if (strcmp (tag_entry->kind, "variable") == 0 ||
		strcmp (tag_entry->kind, "member") == 0)
	{
		return -1;
	}

	if ((stmt = sdb_engine_get_statement_by_query_id (dbe, PREP_QUERY_SCOPE_NEW))
		== NULL)
	{
		g_warning ("query is null");
		return -1;
	}

	plist = sdb_engine_get_query_parameters_list (dbe, PREP_QUERY_SCOPE_NEW);
		
	/* scope parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "scope")) == NULL)
	{			
		g_warning ("param scope is NULL from pquery!");
		return -1;
	}
	gda_holder_set_value_str (param, NULL, scope);

	/* typeid parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "typeid")) == NULL)
	{
		g_warning ("param typeid is NULL from pquery!");
		return -1;
	}
	value = gda_value_new (G_TYPE_INT);
	g_value_set_int (value, type_table_id);
	gda_holder_set_value (param, value);
	gda_value_free (value);

	/* execute the query with parameters just set */
	if (gda_connection_statement_execute_non_select (priv->db_connection, 
													 (GdaStatement*)stmt, 
													 (GdaSet*)plist, &last_inserted,
													 NULL) == -1)
	{
		GValue *value1, *value2;
/*		DEBUG_PRINT ("sdb_engine_add_new_scope_definition (): BAD INSERTION "
					 "[%s %s]", tag_entry->kind, scope);
*/		
		/* let's check for an already present scope table with scope and type_id infos. */
		value1 = gda_value_new (G_TYPE_STRING);
		g_value_set_string (value1, scope);

		value2 = gda_value_new (G_TYPE_INT);
		g_value_set_int (value2, type_table_id);
	
		if ((table_id = sdb_engine_get_tuple_id_by_unique_name2 (dbe,
												 PREP_QUERY_GET_SCOPE_ID,
												 "scope", 
												 value1,
												 "typeid",
												 value2)) < 0)
		{
			table_id = -1;
		}

		gda_value_free (value1);
		gda_value_free (value2);
	}
	else  {
		const GValue *value = gda_set_get_holder_value (last_inserted, "+0");
		table_id = g_value_get_int (value);
	}	

	return table_id;
}

/**
 * Saves the tagEntry info for a second pass parsing.
 * Usually we don't know all the symbol at the first scan of the tags. We need
 * a second one. These tuples are created for that purpose.
 *
 * @return the table_id of the inserted tuple. -1 on error.
 */
static gint
sdb_engine_add_new_tmp_heritage_scope (SymbolDBEngine * dbe,
									   const tagEntry * tag_entry,
									   gint symbol_referer_id)
{
/*
	CREATE TABLE __tmp_heritage_scope (tmp_heritage_scope_id integer PRIMARY KEY 
							AUTOINCREMENT,
							symbol_referer_id integer not null,
							field_inherits varchar(256) not null,
							field_struct varchar(256),
							field_typeref varchar(256),
							field_enum varchar(256),
							field_union varchar(256),
							field_class varchar(256),
							field_namespace varchar(256)
							);
*/
	const GdaSet *plist;
	const GdaStatement *stmt;
	GdaHolder *param;
	GdaSet *last_inserted;
	gint table_id;
	SymbolDBEnginePriv *priv;
	GValue *value;
	const gchar *field_inherits, *field_struct, *field_typeref,
		*field_enum, *field_union, *field_class, *field_namespace;
	gboolean good_tag;

	/* we assume that tag_entry is != NULL */
	/* init the flag */
	good_tag = FALSE;

	priv = dbe->priv;
	
	if ((field_inherits = tagsField (tag_entry, "inherits")) == NULL)
	{
		field_inherits = "";
	}
	else
		good_tag = TRUE;

	if ((field_struct = tagsField (tag_entry, "struct")) == NULL)
	{
		field_struct = "";
	}
	else
		good_tag = TRUE;

	if ((field_typeref = tagsField (tag_entry, "typeref")) == NULL)
	{
		field_typeref = "";
	}
	else
		good_tag = TRUE;

	if ((field_enum = tagsField (tag_entry, "enum")) == NULL)
	{
		field_enum = "";
	}
	else
		good_tag = TRUE;

	if ((field_union = tagsField (tag_entry, "union")) == NULL)
	{
		field_union = "";
	}
	else
		good_tag = TRUE;

	if ((field_class = tagsField (tag_entry, "class")) == NULL)
	{
		field_class = "";
	}
	else
		good_tag = TRUE;

	if ((field_namespace = tagsField (tag_entry, "namespace")) == NULL)
	{
		field_namespace = "";
	}
	else
		good_tag = TRUE;

	if (!good_tag)
		return -1;


	if ((stmt = sdb_engine_get_statement_by_query_id (dbe, PREP_QUERY_TMP_HERITAGE_NEW))
		== NULL)
	{
		g_warning ("query is null");
		return -1;
	}

	plist = sdb_engine_get_query_parameters_list (dbe,PREP_QUERY_TMP_HERITAGE_NEW);
		
	/* symreferid parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "symreferid")) == NULL)
	{
		g_warning ("param symreferid is NULL from pquery!");
		return -1;
	}

	value = gda_value_new (G_TYPE_INT);
	g_value_set_int (value, symbol_referer_id);
	gda_holder_set_value (param, value);
	gda_value_free (value);

	/* finherits parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "finherits")) == NULL)
	{
		g_warning ("param finherits is NULL from pquery!");
		return -1;
	}
	gda_holder_set_value_str (param, NULL, field_inherits);

	/* fstruct parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "fstruct")) == NULL)
	{
		g_warning ("param fstruct is NULL from pquery!");
		return -1;
	}
	gda_holder_set_value_str (param, NULL, field_struct);

	/* ftyperef parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "ftyperef")) == NULL)
	{
		g_warning ("param ftyperef is NULL from pquery!");
		return -1;
	}
	gda_holder_set_value_str (param, NULL, field_typeref);

	/* fenum parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "fenum")) == NULL)
	{
		g_warning ("param fenum is NULL from pquery!");
		return -1;
	}
	gda_holder_set_value_str (param, NULL, field_enum);
	
	/* funion parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "funion")) == NULL)
	{
		g_warning ("param funion is NULL from pquery!");
		return -1;
	}
	gda_holder_set_value_str (param, NULL, field_union);
	
	/* fclass parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "fclass")) == NULL)
	{
		g_warning ("param fclass is NULL from pquery!");
		return -1;
	}
	gda_holder_set_value_str (param, NULL, field_class);

	/* fnamespace parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "fnamespace")) == NULL)
	{
		g_warning ("param fnamespace is NULL from pquery!");
		return -1;
	}
	gda_holder_set_value_str (param, NULL, field_namespace);

	/* execute the query with parametes just set */
	if (gda_connection_statement_execute_non_select (priv->db_connection, 
													 (GdaStatement*)stmt, 
													 (GdaSet*)plist, &last_inserted,
													 NULL) == -1)
	{
		table_id = -1;
	}
	else 
	{
		const GValue *value = gda_set_get_holder_value (last_inserted, 
														"+0");
		table_id = g_value_get_int (value);
	}
	
	return table_id;
}

/** Return the symbol_id of the changed symbol */
static gint
sdb_engine_second_pass_update_scope_1 (SymbolDBEngine * dbe,
									   GdaDataModel * data, gint data_row,
									   gchar * token_name,
									   const GValue * token_value)
{
	gint scope_id;
	GValue *value1, *value2, *value;
	const GValue *value_id2;
	gint symbol_referer_id;
	const gchar *tmp_str;
	gchar **tmp_str_splitted;
	gint tmp_str_splitted_length;
	gchar *object_name = NULL;
	gboolean free_token_name = FALSE;
	const GdaSet *plist;
	const GdaStatement *stmt;
	GdaHolder *param;
	SymbolDBEnginePriv *priv;

	g_return_val_if_fail (G_VALUE_HOLDS_STRING (token_value), FALSE);
	
	priv = dbe->priv;
	tmp_str = g_value_get_string (token_value);

	/* we don't need empty strings */
	if (strcmp (tmp_str, "") == 0)
	{
		return -1;
	}

	/* we could have something like "First::Second::Third::Fourth" as tmp_str, so 
	 * take only the lastscope, in this case 'Fourth'.
	 */
	tmp_str_splitted = g_strsplit (tmp_str, ":", 0);
	tmp_str_splitted_length = g_strv_length (tmp_str_splitted);

	if (tmp_str_splitted_length > 0)
	{
		/* handle special typedef case. Usually we have something like struct:my_foo.
		 * splitting we have [0]-> struct [1]-> my_foo
		 */
		if (strcmp (token_name, "typedef") == 0)
		{
			free_token_name = TRUE;
			token_name = g_strdup (tmp_str_splitted[0]);
		}

		object_name = g_strdup (tmp_str_splitted[tmp_str_splitted_length - 1]);
	}
	else
	{
		g_strfreev (tmp_str_splitted);
		return -1;
	}

	g_strfreev (tmp_str_splitted);

	value1 = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value1, token_name);

	value2 = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value2, object_name);

	/* we're gonna access db. Lets lock here */
	if (priv->mutex)
		g_mutex_lock (priv->mutex);					
	
	if ((scope_id = sdb_engine_get_tuple_id_by_unique_name2 (dbe,
									 PREP_QUERY_GET_SYMBOL_SCOPE_DEFINITION_ID,
									 "tokenname",
									 value1,
									 "objectname",
									 value2)) < 0)
	{
		if (free_token_name)
			g_free (token_name);

		gda_value_free (value1);
		gda_value_free (value2);
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);					
		
		return -1;
	}
	gda_value_free (value1);
	gda_value_free (value2);
	
	if (free_token_name)
		g_free (token_name);

	
	/* if we reach this point we should have a good scope_id.
	 * Go on with symbol updating.
	 */
	value_id2 = gda_data_model_get_value_at_col_name (data, "symbol_referer_id",
													  data_row);
	symbol_referer_id = g_value_get_int (value_id2);
	
	if ((stmt = sdb_engine_get_statement_by_query_id (dbe,
											 PREP_QUERY_UPDATE_SYMBOL_SCOPE_ID))
		== NULL)
	{
		g_warning ("query is null");
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);					
		return -1;
	}

	plist = sdb_engine_get_query_parameters_list (dbe, PREP_QUERY_UPDATE_SYMBOL_SCOPE_ID);

	/* scopeid parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "scopeid")) == NULL)
	{
		g_warning ("param scopeid is NULL from pquery!");
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);					
		return -1;
	}

	value = gda_value_new (G_TYPE_INT);
	g_value_set_int (value, scope_id);
	gda_holder_set_value (param, value);

	/* symbolid parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "symbolid")) == NULL)
	{
		g_warning ("param symbolid is NULL from pquery!");
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);					
		return -1;
	}

	gda_value_reset_with_type (value, G_TYPE_INT);
	g_value_set_int (value, symbol_referer_id);
	gda_holder_set_value (param, value);
	gda_value_free (value);

	/* execute the query with parametes just set */
	gda_connection_statement_execute_non_select (priv->db_connection, 
													 (GdaStatement*)stmt, 
													 (GdaSet*)plist, NULL,
													 NULL);

	if (priv->mutex)
		g_mutex_unlock (priv->mutex);					
	return symbol_referer_id;
}


/**
 * @param data Must be filled with some values. It must have num_rows > 0
 * @note *CALL THIS BEFORE second_pass_update_heritage ()*
 * @note *DO NOT FREE data* inside this function.
 */
static void
sdb_engine_second_pass_update_scope (SymbolDBEngine * dbe, GdaDataModel * data)
{
	SymbolDBEnginePriv *priv;
	/*
	 * Fill up the scope. 
	 * The case: "my_foo_func_1" is the name of the current tag parsed. 
	 * Suppose we have a namespace MyFooNamespace, under which is declared
	 * a class MyFooClass. Under that class there are some funcs like 
	 * my_foo_func_1 () etc. ctags will present us this info about 
	 * my_foo_func_1 ():
	 * "class : MyFooNamespace::MyFooClass"
	 * but hey! We don't need to know the namespace here, we just want to 
	 * know that my_foo_func_1 is in the scope of MyFooClass. That one will 
	 * then be mapped inside MyFooNamespace, but that's another thing.
	 * Go on with the parsing then.
	 */
	gint i;
	
	priv = dbe->priv;

	/* temporary unlock. This function may take a while to be completed
	 * so let other db-task to be executed, so that main thread
	 * isn't locked up. 
	 * sdb_engine_second_pass_update_scope_1 () which is called later on will
	 * access db and then will lock again.
	 */
	if (priv->mutex)
		g_mutex_unlock (priv->mutex);					
	
	for (i = 0; i < gda_data_model_get_n_rows (data); i++)
	{
		GValue *value;
		
		if ((value =
			 (GValue *) gda_data_model_get_value_at_col_name (data,
															  "field_class",
															  i)) != NULL)
		{
			sdb_engine_second_pass_update_scope_1 (dbe, data, i, "class",
												   value);
		}

		if ((value =
			 (GValue *) gda_data_model_get_value_at_col_name (data,
															  "field_struct",
															  i)) != NULL)
		{
			sdb_engine_second_pass_update_scope_1 (dbe, data, i, "struct",
												   value);
		}

		if ((value =
			 (GValue *) gda_data_model_get_value_at_col_name (data,
															  "field_typeref",
															  i)) != NULL)
		{
			/* this is a "typedef", not a "typeref". */
			sdb_engine_second_pass_update_scope_1 (dbe, data, i, "typedef",
												   value);
		}

		if ((value =
			 (GValue *) gda_data_model_get_value_at_col_name (data,
															  "field_enum",
															  i)) != NULL)
		{
			sdb_engine_second_pass_update_scope_1 (dbe, data, i, "enum", value);
		}

		if ((value =
			 (GValue *) gda_data_model_get_value_at_col_name (data,
															  "field_union",
															  i)) != NULL)
		{
			sdb_engine_second_pass_update_scope_1 (dbe, data, i, "union",
												   value);
		}

		if ((value =
			 (GValue *) gda_data_model_get_value_at_col_name (data,
															  "field_namespace",
															  i)) != NULL)
		{
			sdb_engine_second_pass_update_scope_1 (dbe, data, i, "namespace",
												   value);
		}
	}

	/* relock */
	if (priv->mutex)
		g_mutex_lock (priv->mutex);					
	
}


/**
 * @param data Must be filled with some values. It must have num_rows > 0
 * @note *CALL THIS AFTER second_pass_update_scope ()*
 */
static void
sdb_engine_second_pass_update_heritage (SymbolDBEngine * dbe,
										GdaDataModel * data)
{
	gint i;
	SymbolDBEnginePriv *priv;
	
	g_return_if_fail (dbe != NULL);
	
	priv = dbe->priv;

	/* unlock */
	if (priv->mutex)
		g_mutex_unlock (dbe->priv->mutex);
	
	for (i = 0; i < gda_data_model_get_n_rows (data); i++)
	{
		const GValue *value;
		const gchar *inherits;
		gchar *item;
		gchar **inherits_list;
		gint j;
		
		value = gda_data_model_get_value_at_col_name (data,
													  "field_inherits", i);
		inherits = g_value_get_string (value);

		/* there can be multiple inheritance. Check that. */
		inherits_list = g_strsplit (inherits, ",", 0);

		if (inherits_list != NULL)
			DEBUG_PRINT ("inherits %s\n", inherits);

		/* retrieve as much info as we can from the items */
		for (j = 0; j < g_strv_length (inherits_list); j++)
		{
			gchar **namespaces;
			gchar *klass_name;
			gchar *namespace_name;
			gint namespaces_length;
			gint base_klass_id;
			gint derived_klass_id;
			const GValue *value;

			item = inherits_list[j];
			DEBUG_PRINT ("heritage %s\n", item);

			/* A item may have this string form:
			 * MyFooNamespace1::MyFooNamespace2::MyFooClass
			 * We should find the field 'MyFooNamespace2' because it's the one 
			 * that is reachable by the scope_id value of the symbol.
			 */

			namespaces = g_strsplit (item, "::", 0);
			namespaces_length = g_strv_length (namespaces);

			if (namespaces_length > 1)
			{
				/* this is the case in which we have the case with 
				 * namespace + class
				 */
				namespace_name = g_strdup (namespaces[namespaces_length - 2]);
				klass_name = g_strdup (namespaces[namespaces_length - 1]);
			}
			else
			{
				/* have a last check before setting namespace_name to null.
				 * check whether the field_namespace is void or not.
				 */
				const GValue *namespace_value;
				const gchar *tmp_namespace;
				gchar **tmp_namespace_array = NULL;
				gint tmp_namespace_length;

				namespace_value =
					gda_data_model_get_value_at_col_name (data,
														  "field_namespace", i);
				tmp_namespace = g_value_get_string (namespace_value);
				if (tmp_namespace != NULL)
				{
					tmp_namespace_array = g_strsplit (tmp_namespace, "::", 0);
					tmp_namespace_length = g_strv_length (tmp_namespace_array);

					if (tmp_namespace_length > 0)
						namespace_name =
							g_strdup (tmp_namespace_array
									  [tmp_namespace_length - 1]);
					else
						namespace_name = NULL;
				}
				else
				{
					namespace_name = NULL;
				}

				klass_name = g_strdup (namespaces[namespaces_length - 1]);

				g_strfreev (tmp_namespace_array);
			}

			g_strfreev (namespaces);

			/* get the derived_klass_id. It should be the 
			 * symbol_referer_id field into __tmp_heritage_scope table
			 */
			if ((value = (GValue *) gda_data_model_get_value_at (data,
																 1, i)) != NULL)
			{
				derived_klass_id = g_value_get_int (value);
			}
			else
			{
				derived_klass_id = 0;
			}

			/* we're on the query side of the function. It needs some locking... */
			if (priv->mutex)
				g_mutex_lock (dbe->priv->mutex);
			
			/* ok, search for the symbol_id of the base class */
			if (namespace_name == NULL)
			{
				GValue *value1;

				value1 = gda_value_new (G_TYPE_STRING);
				g_value_set_string (value1, klass_name);
				
				if ((base_klass_id =
					 sdb_engine_get_tuple_id_by_unique_name (dbe,
										 PREP_QUERY_GET_SYMBOL_ID_BY_CLASS_NAME,
										 "klassname",
										 value1)) < 0)
				{
					gda_value_free (value1);

					if (priv->mutex)
						g_mutex_unlock (dbe->priv->mutex);

					continue;
				}
				gda_value_free (value1);				
			}
			else
			{
				GValue *value1;
				GValue *value2;

				value1 = gda_value_new (G_TYPE_STRING);
				g_value_set_string (value1, klass_name);

				value2 = gda_value_new (G_TYPE_STRING);
				g_value_set_string (value2, namespace_name);

				DEBUG_PRINT ("value1 : %s value2 : %s", klass_name, namespace_name);
				if ((base_klass_id =
					 sdb_engine_get_tuple_id_by_unique_name2 (dbe,
						  PREP_QUERY_GET_SYMBOL_ID_BY_CLASS_NAME_AND_NAMESPACE,
						  "klassname",
						  value1,
						  "namespacename",
						  value2)) < 0)
				{
					gda_value_free (value1);
					gda_value_free (value2);

					if (priv->mutex)
						g_mutex_unlock (dbe->priv->mutex);

					continue;
				}
				gda_value_free (value1);
				gda_value_free (value2);
			}

			g_free (namespace_name);
			g_free (klass_name);
			
			DEBUG_PRINT ("gonna sdb_engine_add_new_heritage with "
						 "base_klass_id %d, derived_klass_id %d", base_klass_id, 
						 derived_klass_id);
			sdb_engine_add_new_heritage (dbe, base_klass_id, derived_klass_id);
			if (priv->mutex)
				g_mutex_unlock (dbe->priv->mutex);
			
		}

		g_strfreev (inherits_list);			
	}
	
	/* relock before leaving... */
	if (priv->mutex)
		g_mutex_lock (dbe->priv->mutex);
	
}

/**
 * Process the temporary table to update the symbols on scope and inheritance 
 * fields.
 * *CALL THIS FUNCTION ONLY AFTER HAVING PARSED ALL THE TAGS ONCE*
 *
 */
static void
sdb_engine_second_pass_do (SymbolDBEngine * dbe)
{
	const GdaStatement *stmt1, *stmt2, *stmt3;
	GdaDataModel *data_model;
	SymbolDBEnginePriv *priv;

	priv = dbe->priv;
	
	/* prepare for scope second scan */
	if ((stmt1 =
		 sdb_engine_get_statement_by_query_id (dbe,
									 PREP_QUERY_GET_ALL_FROM_TMP_HERITAGE))
		== NULL)
	{
		g_warning ("query is null");
		return;
	}

	/* execute the query */
	data_model = gda_connection_statement_execute_select (priv->db_connection, 
														  (GdaStatement*)stmt1, 
														  NULL, NULL);
	
	if (!GDA_IS_DATA_MODEL (data_model) ||
		gda_data_model_get_n_rows (GDA_DATA_MODEL (data_model)) <= 0)
	{
		if (data_model != NULL)
			g_object_unref (data_model);
		data_model = NULL;
	}
	else
	{
		sdb_engine_second_pass_update_scope (dbe, data_model);
	}

	if (data_model != NULL)
		g_object_unref (data_model);

	/* prepare for heritage second scan */
	if ((stmt2 =
		 sdb_engine_get_statement_by_query_id (dbe,
						 PREP_QUERY_GET_ALL_FROM_TMP_HERITAGE_WITH_INHERITS))
		== NULL)
	{
		g_warning ("query is null");
		return;
	}

	/* execute the query */
	data_model = gda_connection_statement_execute_select (priv->db_connection, 
														  (GdaStatement*)stmt2, 
														  NULL, NULL);

	if (!GDA_IS_DATA_MODEL (data_model) ||
		gda_data_model_get_n_rows (GDA_DATA_MODEL (data_model)) <= 0)
	{
		if (data_model != NULL)
			g_object_unref (data_model);
		data_model = NULL;
	}
	else
	{
		sdb_engine_second_pass_update_heritage (dbe, data_model);
	}

	if (data_model != NULL)
		g_object_unref (data_model);

	/* clean tmp heritage table */
	if ((stmt3 =
		 sdb_engine_get_statement_by_query_id (dbe,
									 PREP_QUERY_TMP_HERITAGE_DELETE_ALL))
		== NULL)
	{
		g_warning ("query is null");
		return;
	}

	/* execute the query */
	gda_connection_statement_execute_non_select (priv->db_connection, 
														  (GdaStatement*)stmt3, 
														  NULL, NULL, NULL);	
}

/* base_prj_path can be NULL. In that case path info tag_entry will be taken
 * as an absolute path.
 * fake_file can be used when a buffer updating is being executed. In that 
 * particular case both base_prj_path and tag_entry->file will be ignored. 
 * fake_file is real_path of file on disk
 */
static gint
sdb_engine_add_new_symbol (SymbolDBEngine * dbe, const tagEntry * tag_entry,
						   gint file_defined_id,
						   gboolean sym_update)
{
/*
	CREATE TABLE symbol (symbol_id integer PRIMARY KEY AUTOINCREMENT,
                     file_defined_id integer not null REFERENCES file (file_id),
                     name varchar (256) not null,
                     file_position integer,
                     is_file_scope integer,
                     signature varchar (256),
                     scope_definition_id integer,
                     scope_id integer,
                     type_id integer REFERENCES sym_type (type_id),
                     kind_id integer REFERENCES sym_kind (sym_kind_id),
                     access_kind_id integer REFERENCES sym_access (sym_access_id),
                     implementation_kind_id integer REFERENCES sym_implementation 
								(sym_impl_id)
                     );
*/
	SymbolDBEnginePriv *priv;
	const GdaSet *plist;
	const GdaStatement *stmt;
	GdaHolder *param;
	GdaSet *last_inserted;
	const gchar *tmp_str;
	gint table_id, symbol_id;
	char* name;
	gint file_position = 0;
	gint is_file_scope = 0;
	gchar signature[256];
	gint scope_definition_id = 0;
	gint scope_id = 0;
	gint type_id = 0;
	gint kind_id = 0;
	gint access_kind_id = 0;
	gint implementation_kind_id = 0;
	GValue *value, *value1, *value2, *value3;
	gboolean sym_was_updated = FALSE;
	gint update_flag;

	g_return_val_if_fail (dbe != NULL, -1);
	priv = dbe->priv;

	/* keep it at 0 if sym_update == false */
	if (sym_update == FALSE)
		update_flag = 0;
	else
		update_flag = 1;

	g_return_val_if_fail (tag_entry != NULL, -1);

	/* parse the entry name */
	if (strlen (tag_entry->name) > 256)
	{
		g_warning ("tag_entry->name too big for database");
		return -1;
	}
	name = g_strdup (tag_entry->name);
	file_position = tag_entry->address.lineNumber;
	is_file_scope = tag_entry->fileScope;

	memset (signature, 0, sizeof (signature));
	if ((tmp_str = tagsField (tag_entry, "signature")) != NULL)
	{
		if (strlen (tmp_str) > sizeof (signature))
		{
			memcpy (signature, tmp_str, sizeof (signature));
		}
		else
		{
			memcpy (signature, tmp_str, strlen (tmp_str));
		}
	}
	
	type_id = sdb_engine_add_new_sym_type (dbe, tag_entry);

	/* scope_definition_id tells what scope this symbol defines
	 * this call *MUST BE DONE AFTER* sym_type table population.
	 */
	scope_definition_id = sdb_engine_add_new_scope_definition (dbe, tag_entry,
															   type_id);

	/* the container scopes can be: union, struct, typeref, class, namespace etc.
	 * this field will be parse in the second pass.
	 */
	scope_id = 0;

	kind_id = sdb_engine_add_new_sym_kind (dbe, tag_entry);
	
	access_kind_id = sdb_engine_add_new_sym_access (dbe, tag_entry);
	implementation_kind_id =
		sdb_engine_add_new_sym_implementation (dbe, tag_entry);
	
	/* ok: was the symbol updated [at least on it's type_id/name]? 
	 * There are 3 cases:
	 * #1. The symbol remain the same [at least on unique index key]. We will 
	 *     perform only a simple update.
	 * #2. The symbol has changed: at least on name/type/file. We will insert a 
	 *     new symbol on table 'symbol'. Deletion of old one will take place 
	 *     at a second stage, when a delete of all symbols with 
	 *     'tmp_flag = 0' will be done.
	 * #3. The symbol has been deleted. As above it will be deleted at 
	 *     a second stage because of the 'tmp_flag = 0'. Triggers will remove 
	 *     also scope_ids and other things.
	 */

	value1 = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value1, name);

	value2 = gda_value_new (G_TYPE_INT);
	g_value_set_int (value2, file_defined_id);

	value3 = gda_value_new (G_TYPE_INT);
	g_value_set_int (value3, type_id);

	if ((symbol_id = sdb_engine_get_tuple_id_by_unique_name3 (dbe,
								  PREP_QUERY_GET_SYMBOL_ID_BY_UNIQUE_INDEX_KEY,
								  "symname", value1,
								  "filedefid",value2,
								  "typeid", value3)) <= 0)
	{
		/* case 2 and 3 */
		sym_was_updated = FALSE;
		
		gda_value_free (value1);
		gda_value_free (value2);
		gda_value_free (value3);

		/* create specific query for a fresh new symbol */
		if ((stmt = sdb_engine_get_statement_by_query_id (dbe, PREP_QUERY_SYMBOL_NEW))
			== NULL)
		{
			g_warning ("query is null");
			return -1;
		}

		plist = sdb_engine_get_query_parameters_list (dbe, PREP_QUERY_SYMBOL_NEW);
		
		/* filedefid parameter */
		if ((param = gda_set_get_holder ((GdaSet*)plist, "filedefid")) == NULL)
		{
			g_warning ("param filedefid is NULL from pquery!");
			return -1;
		}
		value = gda_value_new (G_TYPE_INT);
		g_value_set_int (value, file_defined_id);
		gda_holder_set_value (param, value);

		/* name parameter */
		if ((param = gda_set_get_holder ((GdaSet*)plist, "name")) == NULL)
		{
			g_warning ("param name is NULL from pquery!");
			gda_value_free (value);
			return -1;
		}
		gda_holder_set_value_str (param, NULL, name);

		/* typeid parameter */
		if ((param = gda_set_get_holder ((GdaSet*)plist, "typeid")) == NULL)
		{
			g_warning ("param typeid is NULL from pquery!");
			gda_value_free (value);
			return -1;			
		}
		gda_value_reset_with_type (value, G_TYPE_INT);
		g_value_set_int (value, type_id);
		gda_holder_set_value (param, value);
	}
	else
	{
		/* case 1 */
		sym_was_updated = TRUE;
		
		gda_value_free (value1);
		gda_value_free (value2);
		gda_value_free (value3);

		/* create specific query for a fresh new symbol */
		if ((stmt = sdb_engine_get_statement_by_query_id (dbe,
												 PREP_QUERY_UPDATE_SYMBOL_ALL))
			== NULL)
		{
			g_warning ("query is null");
			return -1;
		}

		plist = sdb_engine_get_query_parameters_list (dbe, PREP_QUERY_UPDATE_SYMBOL_ALL);
		
		/* symbolid parameter */
		if ((param = gda_set_get_holder ((GdaSet*)plist, "symbolid")) == NULL)
		{
			g_warning ("param isfilescope is NULL from pquery!");
			return -1;
		}

		value = gda_value_new (G_TYPE_INT);
		g_value_set_int (value, symbol_id);
		gda_holder_set_value (param, value);
	}
	
	/* common params */

	/* fileposition parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "fileposition")) == NULL)
	{
		g_warning ("param fileposition is NULL from pquery!");
		gda_value_free (value);
		return -1;
	}
	gda_value_reset_with_type (value, G_TYPE_INT);
	g_value_set_int (value, file_position);
	gda_holder_set_value (param, value);

	
	/* isfilescope parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "isfilescope")) == NULL)	
	{
		g_warning ("param isfilescope is NULL from pquery!");
		gda_value_free (value);
		return -1;
	}
	gda_value_reset_with_type (value, G_TYPE_INT);
	g_value_set_int (value, is_file_scope);
	gda_holder_set_value (param, value);

	/* signature parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "signature")) == NULL)	
	{
		g_warning ("param signature is NULL from pquery!");
		gda_value_free (value);
		return -1;
	}
	gda_holder_set_value_str (param, NULL, signature);

	/* scopedefinitionid parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "scopedefinitionid")) == NULL)	
	{
		g_warning ("param scopedefinitionid is NULL from pquery!");
		gda_value_free (value);
		return -1;
	}
	gda_value_reset_with_type (value, G_TYPE_INT);
	g_value_set_int (value, scope_definition_id);
	gda_holder_set_value (param, value);

	/* scopeid parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "scopeid")) == NULL)	
	{
		g_warning ("param scopeid is NULL from pquery!");
		gda_value_free (value);
		return -1;
	}
	gda_value_reset_with_type (value, G_TYPE_INT);
	g_value_set_int (value, scope_id);
	gda_holder_set_value (param, value);

	/* kindid parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "kindid")) == NULL)	
	{
		g_warning ("param kindid is NULL from pquery!");
		gda_value_free (value);
		return -1;
	}
	gda_value_reset_with_type (value, G_TYPE_INT);
	g_value_set_int (value, kind_id);
	gda_holder_set_value (param, value);

	/* accesskindid parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "accesskindid")) == NULL)	
	{
		g_warning ("param accesskindid is NULL from pquery!");
		gda_value_free (value);
		return -1;
	}
	gda_value_reset_with_type (value, G_TYPE_INT);
	g_value_set_int (value, access_kind_id);
	gda_holder_set_value (param, value);

	/* implementationkindid parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "implementationkindid")) == NULL)	
	{
		g_warning ("param implementationkindid is NULL from pquery!");
		gda_value_free (value);
		return -1;
	}
	gda_value_reset_with_type (value, G_TYPE_INT);
	g_value_set_int (value, implementation_kind_id);
	gda_holder_set_value (param, value);

	/* updateflag parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "updateflag")) == NULL)
	{
		g_warning ("param updateflag is NULL from pquery!");
		gda_value_free (value);
		return -1;
	}
	gda_value_reset_with_type (value, G_TYPE_INT);
	g_value_set_int (value, update_flag);
	gda_holder_set_value (param, value);
	gda_value_free (value);
	
	/* execute the query with parametes just set */
	gint nrows;
	nrows = gda_connection_statement_execute_non_select (priv->db_connection, 
													 (GdaStatement*)stmt, 
													 (GdaSet*)plist, &last_inserted,
													 NULL);
	
	if (sym_was_updated == FALSE)
	{
		if (nrows > 0)
		{
			const GValue *value = gda_set_get_holder_value (last_inserted, "+0");
			table_id = g_value_get_int (value);			
		
			/* This is a wrong place to emit the symbol-updated signal. Infact
		 	 * db is in a inconsistent state, e.g. inheritance references are still
		 	 * *not* calculated.
		 	 * So add the symbol id into a queue that will be parsed once and emitted.
		 	 */		
			g_async_queue_push (priv->inserted_symbols_id, (gpointer) table_id);			
		}
		else
		{
			table_id = -1;
		}
	}
	else
	{
		if (nrows > 0)
		{
			table_id = symbol_id;
						
			/* This is a wrong place to emit the symbol-updated signal. Infact
		  	 * db is in a inconsistent state, e.g. inheritance references are still
		  	 * *not* calculated.
			 * So add the symbol id into a queue that will be parsed once and emitted.
			 */
			g_async_queue_push (priv->updated_symbols_id, (gpointer) table_id);
		}
		else 
		{
			table_id = -1;
		}
	}	

	/* before returning the table_id we have to fill some infoz on temporary tables
	 * so that in a second pass we can parse also the heritage and scope fields.
	 */
	if (table_id > 0)
		sdb_engine_add_new_tmp_heritage_scope (dbe, tag_entry, table_id);
	
	return table_id;
}


/**
 * Select * from __tmp_removed and emits removed signals.
 */
static void
sdb_engine_detects_removed_ids (SymbolDBEngine *dbe)
{
	const GdaStatement *stmt1, *stmt2;
	GdaDataModel *data_model;
	SymbolDBEnginePriv *priv;
	gint i, num_rows;	
		
	priv = dbe->priv;
	
	/* ok, now we should read from __tmp_removed all the symbol ids which have
	 * been removed, and emit a signal 
	 */
	if ((stmt1 = sdb_engine_get_statement_by_query_id (dbe,
											 PREP_QUERY_GET_REMOVED_IDS))
		== NULL)
	{
		g_warning ("query is null");
		return;
	}

	
	data_model = gda_connection_statement_execute_select (priv->db_connection, 
												   (GdaStatement*)stmt1, 
													NULL, NULL);

	
	if (GDA_IS_DATA_MODEL (data_model)) 
	{
		if ((num_rows = gda_data_model_get_n_rows (data_model)) <= 0)
		{
			DEBUG_PRINT ("sdb_engine_detects_removed_ids (): nothing to remove");
			g_object_unref (data_model);
			return;
		}
	}
	else
	{
		if (data_model != NULL)
			g_object_unref (data_model);
		return;
	}

	/* get and parse the results. */
	for (i = 0; i < num_rows; i++) 
	{
		const GValue *val;
		gint tmp;
		val = gda_data_model_get_value_at (data_model, 0, i);
		tmp = g_value_get_int (val);
	
		DEBUG_PRINT ("EMITTING symbol-removed");
		g_async_queue_push (priv->signals_queue, (gpointer)(SYMBOL_REMOVED + 1));
		g_async_queue_push (priv->signals_queue, (gpointer)tmp);
	}

	g_object_unref (data_model);
	
	/* let's clean the tmp_table */
	if ((stmt2 = sdb_engine_get_statement_by_query_id (dbe,
											 PREP_QUERY_TMP_REMOVED_DELETE_ALL))
		== NULL)
	{
		g_warning ("query is null");
		return;
	}

	/* bye bye */
	gda_connection_statement_execute_non_select (priv->db_connection, 
													 (GdaStatement*)stmt2, 
													 NULL, NULL,
													 NULL);
}

/**
 * WARNING: do not use this function thinking that it would do a scan of symbols
 * too. Use symbol_db_engine_update_files_symbols () instead. This one will set
 * up some things on db, like removing the 'old' symbols which have not been 
 * updated.
 */
static gboolean
sdb_engine_update_file (SymbolDBEngine * dbe, const gchar * file_on_db)
{
	const GdaSet *plist1, *plist2, *plist3;
	const GdaStatement *stmt1, *stmt2, *stmt3;
	GdaHolder *param;
	SymbolDBEnginePriv *priv;

	priv = dbe->priv;

	/* if we're updating symbols we must do some other operations on db 
	 * symbols, like remove the ones which don't have an update_flag = 1 
	 * per updated file.
	 */

	/* good. Go on with removing of old symbols, marked by a 
	 * update_flag = 0. 
	 */

	/* Triggers will take care of updating/deleting connected symbols
	 * tuples, like sym_kind, sym_type etc */
	if ((stmt1 = sdb_engine_get_statement_by_query_id (dbe, 
									PREP_QUERY_REMOVE_NON_UPDATED_SYMBOLS)) == NULL)
	{
		g_warning ("query is null");
		return FALSE;
	}

	plist1 = sdb_engine_get_query_parameters_list (dbe, PREP_QUERY_REMOVE_NON_UPDATED_SYMBOLS);
	
	if ((param = gda_set_get_holder ((GdaSet*)plist1, "filepath")) == NULL)
	{
		g_warning ("param filepath is NULL from pquery!");
		return FALSE;
	}
	gda_holder_set_value_str (param, NULL, file_on_db);
	
	gda_connection_statement_execute_non_select (priv->db_connection, (GdaStatement*)stmt1, 
														 (GdaSet*)plist1, NULL, NULL);	

	/* emits removed symbols signals */
	sdb_engine_detects_removed_ids (dbe);

	/* reset the update_flag to 0 */	
	if ((stmt2 = sdb_engine_get_statement_by_query_id (dbe, 
									PREP_QUERY_RESET_UPDATE_FLAG_SYMBOLS)) == NULL)
	{
		g_warning ("query is null");
		return FALSE;
	}
	
	plist2 = sdb_engine_get_query_parameters_list (dbe, PREP_QUERY_RESET_UPDATE_FLAG_SYMBOLS);
	
	if ((param = gda_set_get_holder ((GdaSet*)plist2, "filepath")) == NULL)
	{
		g_warning ("param filepath is NULL from pquery!");
		return FALSE;
	}
	gda_holder_set_value_str (param, NULL, file_on_db);

	
	gda_connection_statement_execute_non_select (priv->db_connection, (GdaStatement*)stmt2, 
														 (GdaSet*)plist2, NULL, NULL);	


	/* last but not least, update the file analyse_time */
	if ((stmt3 = sdb_engine_get_statement_by_query_id (dbe,
											 PREP_QUERY_UPDATE_FILE_ANALYSE_TIME))
		== NULL)
	{
		g_warning ("query is null");
		return FALSE;
	}

	plist3 = sdb_engine_get_query_parameters_list (dbe, PREP_QUERY_UPDATE_FILE_ANALYSE_TIME);
	
	/* filepath parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist3, "filepath")) == NULL)
	{
		g_warning ("param filepath is NULL from pquery!");
		return FALSE;
	}
	gda_holder_set_value_str (param, NULL, file_on_db);

	gda_connection_statement_execute_non_select (priv->db_connection, (GdaStatement*)stmt3, 
														 (GdaSet*)plist3, NULL, NULL);	
	return TRUE;
}

/**
 * @param data is a GPtrArray *files_to_scan
 * It will be freed when this callback will be called.
 */
static void
on_scan_update_files_symbols_end (SymbolDBEngine * dbe, 
								  UpdateFileSymbolsData* update_data)
{
	SymbolDBEnginePriv *priv;
	GPtrArray *files_to_scan;
	gint i;

	DEBUG_PRINT ("on_scan_update_files_symbols_end  ();");

	g_return_if_fail (dbe != NULL);
	g_return_if_fail (update_data != NULL);
	
	priv = dbe->priv;
	files_to_scan = update_data->files_path;
	
	sdb_engine_clear_caches (dbe);
	
	/* we need a reinitialization */
	sdb_engine_init_caches (dbe);
	
	for (i = 0; i < files_to_scan->len; i++)
	{
		gchar *node = (gchar *) g_ptr_array_index (files_to_scan, i);
		
		if (strstr (node, priv->project_directory) == NULL) 
		{
			g_warning ("on_scan_update_files_symbols_end  node %s is shorter than "
					   "prj_directory %s",
					   node, priv->project_directory);
			continue;
		}
		
		/* clean the db from old un-updated with the last update step () */
		if (sdb_engine_update_file (dbe, node + 
									strlen (priv->project_directory)) == FALSE)
		{
			g_warning ("Error processing file %s", node + 
					   strlen (priv->project_directory) );
			return;
		}
		g_free (node);
	}
		
	g_signal_handlers_disconnect_by_func (dbe, on_scan_update_files_symbols_end,
										  update_data);

	/* if true, we'll update the project scanning time too. 
	 * warning: project time scanning won't could be set before files one.
	 * This why we'll fork the process calling sdb_engine_scan_files ()
	 */
	if (update_data->update_prj_analyse_time == TRUE)
	{
		const GdaSet *plist;
		const GdaStatement *stmt;
		GdaHolder *param;

		/* and the project analyse_time */
		if ((stmt = sdb_engine_get_statement_by_query_id (dbe,
									PREP_QUERY_UPDATE_PROJECT_ANALYSE_TIME))
			== NULL)
		{
			g_warning ("query is null");
			return;
		}

		plist = sdb_engine_get_query_parameters_list (dbe, PREP_QUERY_UPDATE_PROJECT_ANALYSE_TIME);
			
		/* prjname parameter */
		if ((param = gda_set_get_holder ((GdaSet*)plist, "prjname")) == NULL)
		{
			g_warning ("param prjname is NULL from pquery!");
			return;
		}
		gda_holder_set_value_str (param, NULL, update_data->project);
	
		gda_connection_statement_execute_non_select (priv->db_connection, 
													 (GdaStatement*)stmt, 
													 (GdaSet*)plist, NULL, NULL);
	}	
	
	/* free the GPtrArray. */
	g_ptr_array_free (files_to_scan, TRUE);

	g_free (update_data->project);
	g_free (update_data);
}

const GHashTable*
symbol_db_engine_get_sym_type_conversion_hash (SymbolDBEngine *dbe)
{
	SymbolDBEnginePriv *priv;
	g_return_val_if_fail (dbe != NULL, NULL);
	
	priv = dbe->priv;
		
	return priv->sym_type_conversion_hash;
}

GPtrArray *
symbol_db_engine_fill_type_array (IAnjutaSymbolType match_types)
{
	GPtrArray *filter_array;
	filter_array = g_ptr_array_new ();

	if (match_types & IANJUTA_SYMBOL_TYPE_CLASS)
	{
		g_ptr_array_add (filter_array, g_strdup ("class"));
	}

	if (match_types & IANJUTA_SYMBOL_TYPE_ENUM)
	{
		g_ptr_array_add (filter_array, g_strdup ("enum"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_ENUMERATOR)
	{
		g_ptr_array_add (filter_array, g_strdup ("enumerator"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_FIELD)
	{
		g_ptr_array_add (filter_array, g_strdup ("field"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_FUNCTION)
	{
		g_ptr_array_add (filter_array, g_strdup ("function"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_INTERFACE)
	{
		g_ptr_array_add (filter_array, g_strdup ("interface"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_MEMBER)
	{
		g_ptr_array_add (filter_array, g_strdup ("member"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_METHOD)
	{
		g_ptr_array_add (filter_array, g_strdup ("method"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_NAMESPACE)
	{
		g_ptr_array_add (filter_array, g_strdup ("namespace"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_PACKAGE)
	{
		g_ptr_array_add (filter_array, g_strdup ("package"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_PROTOTYPE)
	{
		g_ptr_array_add (filter_array, g_strdup ("prototype"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_STRUCT)
	{
		g_ptr_array_add (filter_array, g_strdup ("struct"));
	}

	if (match_types & IANJUTA_SYMBOL_TYPE_TYPEDEF)
	{
		g_ptr_array_add (filter_array, g_strdup ("typedef"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_STRUCT)
	{
		g_ptr_array_add (filter_array, g_strdup ("struct"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_UNION)
	{
		g_ptr_array_add (filter_array, g_strdup ("union"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_VARIABLE)
	{
		g_ptr_array_add (filter_array, g_strdup ("variable"));
	}
				
	if (match_types & IANJUTA_SYMBOL_TYPE_EXTERNVAR)
	{
		g_ptr_array_add (filter_array, g_strdup ("externvar"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_MACRO)
	{
		g_ptr_array_add (filter_array, g_strdup ("macro"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_MACRO_WITH_ARG)
	{
		g_ptr_array_add (filter_array, g_strdup ("macro_with_arg"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_FILE)
	{
		g_ptr_array_add (filter_array, g_strdup ("file"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_VARIABLE)
	{
		g_ptr_array_add (filter_array, g_strdup ("variable"));
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_OTHER)
	{
		g_ptr_array_add (filter_array, g_strdup ("other"));
	}

	return filter_array;
}

/**
 * Update symbols of saved files. 
 * @note WARNING: files_path and it's contents will be freed on 
 * on_scan_update_files_symbols_end () callback.
 */
gboolean
symbol_db_engine_update_files_symbols (SymbolDBEngine * dbe, const gchar * project, 
									   GPtrArray * files_path,
									   gboolean update_prj_analyse_time)
{
	SymbolDBEnginePriv *priv;
	UpdateFileSymbolsData *update_data;
	
	priv = dbe->priv;

	DEBUG_PRINT ("symbol_db_engine_update_files_symbols ()");
	g_return_val_if_fail (priv->db_connection != NULL, FALSE);
	g_return_val_if_fail (project != NULL, FALSE);

	update_data = g_new0 (UpdateFileSymbolsData, 1);
	
	update_data->update_prj_analyse_time = update_prj_analyse_time;
	update_data->files_path = files_path;
	update_data->project = g_strdup (project);
	
	/* data will be freed when callback will be called. The signal will be
	 * disconnected too, don't worry about disconneting it by hand.
	 */
	g_signal_connect (G_OBJECT (dbe), "scan-end",
					  G_CALLBACK (on_scan_update_files_symbols_end), update_data);
	
	sdb_engine_scan_files_1 (dbe, files_path, NULL, TRUE);

	return TRUE;
}

/* Update symbols of the whole project. It scans all file symbols etc. 
 * FIXME: libgda does not support nested prepared queries like 
 * PREP_QUERY_GET_ALL_FROM_FILE_BY_PROJECT_NAME. When it will do please
 * remember to update this function.
 */
gboolean
symbol_db_engine_update_project_symbols (SymbolDBEngine *dbe, const gchar *project)
{
	const GdaSet *plist;
	const GdaStatement *stmt;
	GdaHolder *param;
	GValue *value;
	GdaDataModel *data_model;
	gint project_id;
	gint num_rows = 0;
	gint i;
	GPtrArray *files_to_scan;
	SymbolDBEnginePriv *priv;
	
	g_return_val_if_fail (dbe != NULL, FALSE);
	g_return_val_if_fail (project != NULL, FALSE);
	priv = dbe->priv;

	value = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value, project);

	/* get project id */
	if ((project_id = sdb_engine_get_tuple_id_by_unique_name (dbe,
									 PREP_QUERY_GET_PROJECT_ID_BY_UNIQUE_NAME,
									 "prjname",
									 value)) <= 0)
	{
		gda_value_free (value);
		return FALSE;
	}

	if ((stmt = sdb_engine_get_statement_by_query_id (dbe,
								 PREP_QUERY_GET_ALL_FROM_FILE_BY_PROJECT_ID))
		== NULL)
	{
		g_warning ("query is null");
		gda_value_free (value);
		return FALSE;
	}

	plist = sdb_engine_get_query_parameters_list (dbe, 
								PREP_QUERY_GET_ALL_FROM_FILE_BY_PROJECT_ID);

	/* prjid parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "prjid")) == NULL)
	{
		g_warning ("param prjid is NULL from pquery!");
		return FALSE;
	}
	gda_value_reset_with_type (value, G_TYPE_INT);
	g_value_set_int (value, project_id);
	gda_holder_set_value (param, value);

	/* execute the query with parametes just set */
	data_model = gda_connection_statement_execute_select (priv->db_connection, 
												   (GdaStatement*)stmt, NULL, NULL);

	if (!GDA_IS_DATA_MODEL (data_model) ||
		(num_rows = gda_data_model_get_n_rows (GDA_DATA_MODEL (data_model))) <= 0)
	{
		g_message ("no rows");
		if (data_model != NULL)
			g_object_unref (data_model);
		data_model = NULL;
	}

	/* we don't need it anymore */
	gda_value_free (value);

	/* initialize the array */
	files_to_scan = g_ptr_array_new ();

	/* we can now scan each filename entry to check the last modification time. */
	for (i = 0; i < num_rows; i++)
	{
		const GValue *value, *value1;
		const gchar *file_name;
		gchar *file_abs_path;
		struct tm filetm;
		time_t db_file_time;
		gchar *date_string;
		gchar *abs_vfs_path;
		GnomeVFSHandle *handle;

		if ((value =
			 gda_data_model_get_value_at_col_name (data_model,
												   "file_path", i)) == NULL)
		{
			continue;
		}

		/* build abs path. */
		file_name = g_value_get_string (value);
		if (priv->project_directory != NULL)
		{
			/* FIXME */
			abs_vfs_path = g_strdup_printf ("file://%s%s", priv->project_directory,
										file_name);
			file_abs_path = g_strdup_printf ("%s%s", priv->project_directory,
										file_name);
		}
		else
		{
			/* FIXME */
			abs_vfs_path = g_strdup_printf ("file://%s", file_name);
			file_abs_path = g_strdup (file_name);
		}

		GnomeVFSURI *uri = gnome_vfs_uri_new (abs_vfs_path);
		GnomeVFSFileInfo *file_info = gnome_vfs_file_info_new ();

		/* retrieve data/time info */
		if (gnome_vfs_open_uri (&handle, uri,
								GNOME_VFS_OPEN_READ | GNOME_VFS_OPEN_RANDOM) !=
			GNOME_VFS_OK)
		{
			g_message ("could not open URI %s", abs_vfs_path);
			gnome_vfs_uri_unref (uri);
			gnome_vfs_file_info_unref (file_info);
			g_free (abs_vfs_path);
			g_free (file_abs_path);
			continue;
		}

		if (gnome_vfs_get_file_info_from_handle (handle, file_info,
												 GNOME_VFS_FILE_INFO_DEFAULT) !=
			GNOME_VFS_OK)
		{
			g_message ("cannot get file info from handle");
			gnome_vfs_close (handle);
			gnome_vfs_uri_unref (uri);
			gnome_vfs_file_info_unref (file_info);
			g_free (file_abs_path);
			continue;
		}

		if ((value1 =
			 gda_data_model_get_value_at_col_name (data_model,
												   "analyse_time", i)) == NULL)
		{
			continue;
		}

		/* weirdly we have a strange libgda behaviour here too. 
		 * as from ChangeLog GDA_TYPE_TIMESTAMP as SQLite does not impose a 
		 * known format for dates (there is no date datatype).
		 * We have then to do some hackery to retrieve the date.        
		 */
		date_string = (gchar *) g_value_get_string (value1);	
	
		/* fill a struct tm with the date retrieved by the string. */
		/* string is something like '2007-04-18 23:51:39' */
		memset (&filetm, 0, sizeof (struct tm));
		filetm.tm_year = atoi (date_string) - 1900;
		date_string += 5;
		filetm.tm_mon = atoi (date_string) - 1;
		date_string += 3;
		filetm.tm_mday = atoi (date_string);
		date_string += 3;
		filetm.tm_hour = atoi (date_string);
		date_string += 3;
		filetm.tm_min = atoi (date_string);
		date_string += 3;
		filetm.tm_sec = atoi (date_string);
		
		/* subtract one hour to the db_file_time. */
		db_file_time = mktime (&filetm) /*- 3600*/;

		if (difftime (db_file_time, file_info->mtime) < 0)
		{
/*			g_message ("FILES TO BE UPDATED ===> : %s [diff time %f] date string is %s", 
					   file_name, difftime (db_file_time, file_info->mtime),
					   date_string);
*/			
			g_ptr_array_add (files_to_scan, file_abs_path);
		}
		
		gnome_vfs_close (handle);
		gnome_vfs_uri_unref (uri);
		gnome_vfs_file_info_unref (file_info);
		g_free (abs_vfs_path);
		/* no need to free file_abs_path, it's been added to files_to_scan */
	}
	
	if (data_model)
		g_object_unref (data_model);
	
	if (files_to_scan->len > 0)
	{
		/* at the end let the scanning function do its job */
		return symbol_db_engine_update_files_symbols (dbe, project,
											   files_to_scan, TRUE);
	}
	return TRUE;
}


/* Remove a file, together with its symbols, from a project. */
gboolean
symbol_db_engine_remove_file (SymbolDBEngine * dbe, const gchar * project,
							  const gchar * file)
{
	gchar *query_str;
	SymbolDBEnginePriv *priv;	
	
	g_return_val_if_fail (dbe != NULL, FALSE);
	priv = dbe->priv;
	
	if (strlen (file) < strlen (priv->project_directory)) 
	{
		g_warning ("wrong file to delete.");
		return FALSE;
	}
	
	/* Triggers will take care of updating/deleting connected symbols
	 * tuples, like sym_kind, sym_type etc */
	query_str = g_strdup_printf ("DELETE FROM file WHERE prj_id "
				 "= (SELECT project_id FROM project WHERE project_name = '%s') AND "
				 "file_path = '%s'", project, file + strlen (priv->project_directory));

	sdb_engine_execute_non_select_sql (dbe, query_str);
	g_free (query_str);
	
	/* emits removed symbols signals */
	sdb_engine_detects_removed_ids (dbe);
	return TRUE;
}


static void
on_scan_update_buffer_end (SymbolDBEngine * dbe, gpointer data)
{
	SymbolDBEnginePriv *priv;
	GPtrArray *files_to_scan;
	gint i;

	g_return_if_fail (dbe != NULL);
	g_return_if_fail (data != NULL);

	priv = dbe->priv;
	files_to_scan = (GPtrArray *) data;

	DEBUG_PRINT ("files_to_scan->len %d", files_to_scan->len);

	for (i = 0; i < files_to_scan->len; i++)
	{
		gchar *node = (gchar *) g_ptr_array_index (files_to_scan, i);

		 /* FIXME remove DEBUG_PRINT */
		DEBUG_PRINT ("processing updating for file [on disk] %s, "
					 "passed to on_scan_update_buffer_end (): %s", 
					 node, node + strlen (priv->db_directory));

		gchar *relative_path = symbol_db_engine_get_file_db_path (dbe, node);
		if (relative_path != NULL)
		{
			/* will be emitted removed signals */
			if (sdb_engine_update_file (dbe, relative_path) == FALSE)
			{
				g_warning ("Error processing file %s", node);
				g_free (relative_path);
				return;
			}
			g_free (relative_path);
		}
		g_free (node);
	}
		
	g_signal_handlers_disconnect_by_func (dbe, on_scan_update_buffer_end,
										  files_to_scan);

	/* free the GPtrArray. */
	g_ptr_array_free (files_to_scan, TRUE);
	data = files_to_scan = NULL;
}

/* Update symbols of a file by a memory-buffer to perform a real-time updating 
 * of symbols. 
 * real_files_list: full path on disk to 'real file' to update. e.g.
 * /home/foouser/fooproject/src/main.c
 */
gboolean
symbol_db_engine_update_buffer_symbols (SymbolDBEngine * dbe, const gchar *project,
										GPtrArray * real_files_list,
										const GPtrArray * text_buffers,
										const GPtrArray * buffer_sizes)
{
	SymbolDBEnginePriv *priv;
	gint i;
	/* array that'll represent the /dev/shm/anjuta-XYZ files */
	GPtrArray *temp_files;
	GPtrArray *real_files_on_db;
	
	g_return_val_if_fail (dbe != NULL, FALSE);
	priv = dbe->priv;
	
	g_return_val_if_fail (priv->db_connection != NULL, FALSE);
	g_return_val_if_fail (project != NULL, FALSE);
	g_return_val_if_fail (real_files_list != NULL, FALSE);
	g_return_val_if_fail (text_buffers != NULL, FALSE);
	g_return_val_if_fail (buffer_sizes != NULL, FALSE);
	
	DEBUG_PRINT ("symbol_db_engine_update_buffer_symbols ()");

	temp_files = g_ptr_array_new();	
	real_files_on_db = g_ptr_array_new();
	
	/* obtain a GPtrArray with real_files on database */
	for (i=0; i < real_files_list->len; i++) 
	{
		gchar *relative_path = symbol_db_engine_get_file_db_path (dbe, 
									g_ptr_array_index (real_files_list, i));
		if (relative_path == NULL)
		{
			g_warning ("symbol_db_engine_update_buffer_symbols  (): "
					   "relative_path is NULL");
			return FALSE;
		}
		g_ptr_array_add (real_files_on_db, relative_path);
	}	
	
	/* create a temporary file for each buffer */
	for (i=0; i < real_files_list->len; i++) 
	{
		FILE *buffer_mem_file;
		const gchar *temp_buffer;
		gint buffer_mem_fd;
		gint temp_size;
		gchar *shared_temp_file;
		gchar *base_filename;
		const gchar *curr_real_file;
				
		curr_real_file = g_ptr_array_index (real_files_list, i);
		
		/* it's ok to have just the base filename to create the
		 * target buffer one */
		base_filename = g_filename_display_basename (curr_real_file);
		
		shared_temp_file = g_strdup_printf ("/anjuta-%d-%ld-%s", getpid (),
						 time (NULL), base_filename);
		g_free (base_filename);
		
		if ((buffer_mem_fd = 
			 shm_open (shared_temp_file, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR)) < 0)
		{
			g_warning ("Error while trying to open a shared memory file. Be"
					   "sure to have "SHARED_MEMORY_PREFIX" mounted with tmpfs");
			return FALSE;
		}
	
		buffer_mem_file = fdopen (buffer_mem_fd, "w+b");
		
		temp_buffer = g_ptr_array_index (text_buffers, i);
		temp_size = (gint)g_ptr_array_index (buffer_sizes, i);
		
		fwrite (temp_buffer, sizeof(gchar), temp_size, buffer_mem_file);
		fflush (buffer_mem_file);
		fclose (buffer_mem_file);
		
		/* add the temp file to the array. */
		g_ptr_array_add (temp_files, g_strdup_printf (SHARED_MEMORY_PREFIX"%s", 
													  shared_temp_file));
		
		/* check if we already have an entry stored in the hash table, else
		 * insert it 
		 */		
		if (g_hash_table_lookup (priv->garbage_shared_mem_files, shared_temp_file) 
			== NULL)
		{
			DEBUG_PRINT ("inserting into garbage hash table %s", shared_temp_file);
			g_hash_table_insert (priv->garbage_shared_mem_files, shared_temp_file, 
								 NULL);
		}
		else 
		{
			/* the item is already stored. Just free it here. */
			g_free (shared_temp_file);
		}
	}
	
	/* data will be freed when callback will be called. The signal will be
	 * disconnected too, don't worry about disconnecting it by hand.
	 */
	g_signal_connect (G_OBJECT (dbe), "scan-end",
					  G_CALLBACK (on_scan_update_buffer_end), real_files_list);
	
	sdb_engine_scan_files_1 (dbe, temp_files, real_files_on_db, TRUE);

	/* let's free the temp_files array */
	for (i=0; i < temp_files->len; i++)
		g_free (g_ptr_array_index (temp_files, i));
	
	/* FIXME TODO remove temp files! look at task for details */
	
	
	g_ptr_array_free (temp_files, TRUE);
	
	/* and the real_files_on_db too */
	for (i=0; i < real_files_on_db->len; i++)
		g_free (g_ptr_array_index (real_files_on_db, i));
	
	g_ptr_array_free (real_files_on_db, TRUE);
	return TRUE;
}

gboolean
symbol_db_engine_is_locked (SymbolDBEngine * dbe)
{
	SymbolDBEnginePriv *priv;

	g_return_val_if_fail (dbe != NULL, FALSE);
	
	priv = dbe->priv;
	return priv->scanning_status;
}

/* user must free the returned value */
gchar*
symbol_db_engine_get_full_local_path (SymbolDBEngine *dbe, const gchar* file)
{
	SymbolDBEnginePriv *priv;
	gchar *full_path;
	
	g_return_val_if_fail (dbe != NULL, NULL);
	
	priv = dbe->priv;
	full_path = g_strdup_printf ("%s%s", priv->project_directory, file);
	return full_path;	
}

gchar*
symbol_db_engine_get_file_db_path (SymbolDBEngine *dbe, const gchar* full_local_file_path)
{
	SymbolDBEnginePriv *priv;
	gchar *relative_path;
	const gchar *tmp;
	g_return_val_if_fail (dbe != NULL, NULL);
		
	priv = dbe->priv;
	
	if (priv->db_directory == NULL)
		return NULL;

	if (strlen (priv->project_directory) >= strlen (full_local_file_path)) 
	{
		return NULL;
	}

	tmp = full_local_file_path + strlen (priv->project_directory);
	relative_path = strdup (tmp);

	return relative_path;
}


static inline gint
sdb_engine_walk_down_scope_path (SymbolDBEngine *dbe, const GPtrArray* scope_path) 
{
	SymbolDBEnginePriv *priv;
	gint final_definition_id;
	gint scope_path_len;
	gint i;
	GdaDataModel *data;
	const GdaSet *plist;
	const GdaStatement *stmt;
	GdaHolder *param;
	
		
	g_return_val_if_fail (dbe != NULL, FALSE);
	priv = dbe->priv;
	
	scope_path_len = scope_path->len;
	
	/* we'll return if the length is even or minor than 3 */
	if (scope_path_len < 3 || scope_path_len % 2 == 0)
	{
		g_warning ("bad scope_path.");
		return -1;
	}
	
	if ((stmt =
		 sdb_engine_get_statement_by_query_id (dbe, 
			PREP_QUERY_GET_SCOPE_DEFINITION_ID_BY_WALK_DOWN_SCOPE_PATH)) == NULL)
	{
		g_warning ("query is null");
		return -1;
	}

	plist = sdb_engine_get_query_parameters_list (dbe, 
			PREP_QUERY_GET_SCOPE_DEFINITION_ID_BY_WALK_DOWN_SCOPE_PATH);	
	final_definition_id = 0;
	for (i=0; i < scope_path_len -1; i = i + 2)
	{
		const GValue *value;
		GValue *value_scope;

		if ((param = gda_set_get_holder ((GdaSet*)plist, "symtype")) == NULL)
		{
			return -1;
		}
		gda_holder_set_value_str (param, NULL, 
								  (gchar*)g_ptr_array_index (scope_path, i));
		
		if ((param = gda_set_get_holder ((GdaSet*)plist, "scopename")) == NULL)
		{
			return -1;
		}
		gda_holder_set_value_str (param, NULL, 
								  (gchar*)g_ptr_array_index (scope_path, i + 1));

		
		if ((param = gda_set_get_holder ((GdaSet*)plist, "scopeid")) == NULL)
		{
			return -1;
		}
		value_scope = gda_value_new (G_TYPE_INT);
		g_value_set_int (value_scope, final_definition_id);
		gda_holder_set_value (param, value_scope);
		gda_value_free (value_scope);

		data = gda_connection_statement_execute_select (priv->db_connection, 
														  (GdaStatement*)stmt, 
														  (GdaSet*)plist, NULL);
	
		if (!GDA_IS_DATA_MODEL (data) ||
			gda_data_model_get_n_rows (GDA_DATA_MODEL (data)) <= 0)
		{
			if (data != NULL)
				g_object_unref (data);
			return -1;
		}
		
		value = gda_data_model_get_value_at (data, 0, 0);
		if (G_VALUE_HOLDS (value, G_TYPE_INT))
		{
			final_definition_id = g_value_get_int (value);
			g_object_unref (data);
		}
		else
		{
			/* something went wrong. Our symbol cannot be retrieved coz of a
			 * bad scope path.
			 */
			final_definition_id = -1;
			break;
		}
	}	
	
	return final_definition_id;
}


/**
 * tries to get all the files with zero symbols: these should be the ones
 * excluded by an abort on population process.
 * @return A GPtrArray with paths on disk of the files. Must be freed by caller.
 * @return NULL if no files are found.
 */
GPtrArray *
symbol_db_engine_get_files_with_zero_symbols (SymbolDBEngine *dbe)
{
	/*select * from file where file_id not in (select file_defined_id from symbol);*/
	SymbolDBEnginePriv *priv;
	GdaDataModel *data_model;
	GPtrArray *files_to_scan;
	const GdaStatement *stmt;
	gint i, num_rows = 0;

	g_return_val_if_fail (dbe != NULL, NULL);
	priv = dbe->priv;
	
	if (priv->mutex)
		g_mutex_lock (priv->mutex);	
	
	if ((stmt = sdb_engine_get_statement_by_query_id (dbe,
								 PREP_QUERY_GET_ALL_FROM_FILE_WHERE_NOT_IN_SYMBOLS))
		== NULL)
	{
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);
		return NULL;
	}

	data_model = gda_connection_statement_execute_select (priv->db_connection, 
														  (GdaStatement*)stmt, 
														  NULL, NULL);
	
	if (!GDA_IS_DATA_MODEL (data_model) ||
		gda_data_model_get_n_rows (GDA_DATA_MODEL (data_model)) <= 0)
	{
		if (data_model != NULL)
			g_object_unref (data_model);
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);
		return NULL;
	}	
		
	/* initialize the array */
	files_to_scan = g_ptr_array_new ();

	/* we can now scan each filename entry to check the last modification time. */
	for (i = 0; i < num_rows; i++)
	{
		const GValue *value;
		const gchar *file_name;
		gchar *file_abs_path = NULL;

		if ((value =
			 gda_data_model_get_value_at_col_name (data_model,
												   "file_path", i)) == NULL)
		{
			continue;
		}

		/* build abs path. */
		file_name = g_value_get_string (value);
		if (priv->db_directory != NULL)
		{
			file_abs_path = g_strdup_printf ("%s%s", priv->project_directory,
										file_name);
		}
		g_ptr_array_add (files_to_scan, file_abs_path);
	}

	g_object_unref (data_model);
	
	if (priv->mutex)
		g_mutex_unlock (priv->mutex);
	
	return files_to_scan;
}

static inline void
sdb_engine_prepare_symbol_info_sql (SymbolDBEngine *dbe, GString *info_data,
									GString *join_data, SymExtraInfo sym_info) 
{
	if (sym_info & SYMINFO_FILE_PATH 	|| 
		sym_info & SYMINFO_LANGUAGE  	||
		sym_info & SYMINFO_PROJECT_NAME ||
		sym_info & SYMINFO_FILE_IGNORE  ||
		sym_info & SYMINFO_FILE_INCLUDE) 
	{
		info_data = g_string_append (info_data, ",file.file_path AS file_path ");
		join_data = g_string_append (join_data, "LEFT JOIN file ON "
				"symbol.file_defined_id = file.file_id ");
	}

	if (sym_info & SYMINFO_LANGUAGE)
	{
		info_data = g_string_append (info_data, ",language.language_name "
									 "AS language_name ");
		join_data = g_string_append (join_data, "LEFT JOIN language ON "
				"file.lang_id = language.language_id ");
	}
	
	if (sym_info & SYMINFO_IMPLEMENTATION)
	{
		info_data = g_string_append (info_data, ",sym_implementation.implementation_name "
									 "AS implementation_name " );
		join_data = g_string_append (join_data, "LEFT JOIN sym_implementation ON "
				"symbol.implementation_kind_id = sym_implementation.sym_impl_id ");
	}
	
	if (sym_info & SYMINFO_ACCESS)
	{
		info_data = g_string_append (info_data, ",sym_access.access_name AS access_name ");
		join_data = g_string_append (join_data, "LEFT JOIN sym_access ON "
				"symbol.access_kind_id = sym_access.access_kind_id ");
	}
	
	if (sym_info & SYMINFO_KIND)
	{
		info_data = g_string_append (info_data, ",sym_kind.kind_name AS kind_name");
		join_data = g_string_append (join_data, "LEFT JOIN sym_kind ON "
				"symbol.kind_id = sym_kind.sym_kind_id ");
	}
	
	if (sym_info & SYMINFO_TYPE || sym_info & SYMINFO_TYPE_NAME)
	{
		info_data = g_string_append (info_data, ",sym_type.type_type AS type_type, "
									 "sym_type.type_name AS type_name");
		join_data = g_string_append (join_data, "LEFT JOIN sym_type ON "
				"symbol.type_id = sym_type.type_id ");
	}

	if (sym_info & SYMINFO_PROJECT_NAME ||
		sym_info & SYMINFO_FILE_IGNORE  ||
		sym_info & SYMINFO_FILE_INCLUDE)
	{
		info_data = g_string_append (info_data, ",project.project_name AS project_name ");
		join_data = g_string_append (join_data, "LEFT JOIN project ON "
				"file.prj_id = project.project_id ");
	}	

	if (sym_info & SYMINFO_FILE_IGNORE)
	{
		info_data = g_string_append (info_data, ",file_ignore.file_ignore_type "
									 "AS ignore_type ");
		join_data = g_string_append (join_data, "LEFT JOIN ext_ignore ON "
				"ext_ignore.prj_id = project.project_id "
				"LEFT JOIN file_ignore ON "
				"ext_ignore.file_ign_id = file_ignore.file_ignore_id ");
	}

	if (sym_info & SYMINFO_FILE_INCLUDE)
	{
		info_data = g_string_append (info_data, ",file_include.file_include_type "
									 "AS file_include_type ");
		join_data = g_string_append (join_data, "LEFT JOIN ext_include ON "
				"ext_include.prj_id = project.project_id "
				"LEFT JOIN file_include ON "
				"ext_include.file_incl_id = file_include.file_include_id ");
	}
	
/* TODO, or better.. TAKE A DECISION
	if (sym_info & SYMINFO_WORKSPACE_NAME)
	{
		info_data = g_string_append (info_data, ",sym_access.access_name ");
		join_data = g_string_append (info_data, "LEFT JOIN sym_kind ON "
				"symbol.kind_id = sym_kind.sym_kind_id ");
	}
*/
}


/**
 * Same behaviour as symbol_db_engine_get_class_parents () but this is quicker because
 * of the child_klass_symbol_id, aka the derived class symbol_id.
 * Return an iterator (eventually) containing the base classes or NULL if error occurred.
 */
SymbolDBEngineIterator *
symbol_db_engine_get_class_parents_by_symbol_id (SymbolDBEngine *dbe, 
												 gint child_klass_symbol_id,
												 SymExtraInfo sym_info)
{
	SymbolDBEnginePriv *priv;
	gchar *query_str;
	GdaDataModel *data;
	GdaHolder *param;
	GString *info_data;
	GString *join_data;
	GValue *value;
	const DynChildQueryNode *dyn_node;
	
	g_return_val_if_fail (dbe != NULL, FALSE);
	priv = dbe->priv;

	if (priv->mutex)
	{
		g_mutex_lock (priv->mutex);
	}

	if ((dyn_node = sdb_engine_get_dyn_query_node_by_id (dbe, 
					DYN_PREP_QUERY_GET_CLASS_PARENTS_BY_SYMBOL_ID, sym_info, 0)) == NULL)
	{
	
		/* info_data contains the stuff after SELECT and befor FROM */
		info_data = g_string_new ("");
	
		/* join_data contains the optionals joins to do to retrieve new data on other
	 	 * tables.
	 	 */
		join_data = g_string_new ("");

		/* fill info_data and join data with optional sql */
		sdb_engine_prepare_symbol_info_sql (dbe, info_data, join_data, sym_info);
	
		query_str = g_strdup_printf("SELECT symbol.symbol_id,AS symbol_id "
				"symbol.name AS name, symbol.file_position AS file_position, "
				"symbol.is_file_scope AS is_file_scope, symbol.signature AS signature "
				"%s FROM heritage "
				"JOIN symbol ON heritage.symbol_id_base = symbol.symbol_id %s "
				"WHERE heritage.symbol_id_derived = ## /* name:'childklassid' type:gint */", 
						info_data->str, join_data->str);
	
		DEBUG_PRINT ("symbol_db_engine_get_class_parents_by_symbol_id query: %s", 
					 query_str);
		
		dyn_node = sdb_engine_insert_dyn_query_node_by_id (dbe, 
						DYN_PREP_QUERY_GET_CLASS_PARENTS_BY_SYMBOL_ID,
						sym_info, 0,
						query_str);
		
		g_free (query_str);
		g_string_free (info_data, TRUE);
		g_string_free (join_data, TRUE);
	}

	if (dyn_node == NULL) 
	{		
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);		
		return NULL;
	}
	
	if ((param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "childklassid")) == NULL)
	{
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);		
		return NULL;
	}
		
	value = gda_value_new (G_TYPE_INT);
	g_value_set_int (value, child_klass_symbol_id);
		
	gda_holder_set_value (param, value);
	gda_value_free (value);
	

	/* execute the query with parametes just set */
	data = gda_connection_statement_execute_select (priv->db_connection, 
												  (GdaStatement*)dyn_node->stmt, 
												  (GdaSet*)dyn_node->plist, NULL);
		
	if (!GDA_IS_DATA_MODEL (data) ||
		gda_data_model_get_n_rows (data) <= 0)
	{
		if (data != NULL)
			g_object_unref (data);
		
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);
		return NULL;
	}
	
	if (priv->mutex)
		g_mutex_unlock (priv->mutex);
	return (SymbolDBEngineIterator *)symbol_db_engine_iterator_new (data, 
												priv->sym_type_conversion_hash);	
}


/** 
 * Returns an iterator to the data retrieved from database. 
 * The iterator, if not null, will contain a list of parent classes for the given 
 * symbol name.
 * scope_path can be NULL.
 */
#define DYN_GET_CLASS_PARENTS_EXTRA_PAR_FINAL_DEF_ZERO		1
#define DYN_GET_CLASS_PARENTS_EXTRA_PAR_FINAL_DEF_POSITIVE	2

SymbolDBEngineIterator *
symbol_db_engine_get_class_parents (SymbolDBEngine *dbe, const gchar *klass_name, 
									 const GPtrArray *scope_path, SymExtraInfo sym_info)
{
	SymbolDBEnginePriv *priv;
	gchar *query_str;
	GdaDataModel *data;
	GdaHolder *param;
	GString *info_data;
	GString *join_data;
	gint final_definition_id;
	const DynChildQueryNode *dyn_node;
	
	g_return_val_if_fail (dbe != NULL, FALSE);
	priv = dbe->priv;
	
	if (priv->mutex)
	{
		g_mutex_lock (priv->mutex);
	}
	
	final_definition_id = -1;
	if (scope_path != NULL)	
		final_definition_id = sdb_engine_walk_down_scope_path (dbe, scope_path);

	if ((dyn_node = sdb_engine_get_dyn_query_node_by_id (dbe, 
					DYN_PREP_QUERY_GET_CLASS_PARENTS, sym_info, 
					final_definition_id > 0 ? 
					DYN_GET_CLASS_PARENTS_EXTRA_PAR_FINAL_DEF_POSITIVE :
					DYN_GET_CLASS_PARENTS_EXTRA_PAR_FINAL_DEF_ZERO)) == NULL)
	{
		
		/* info_data contains the stuff after SELECT and before FROM */
		info_data = g_string_new ("");
	
		/* join_data contains the optionals joins to do to retrieve new data on other
	 	 * tables.
	 	 */
		join_data = g_string_new ("");

		/* fill info_data and join data with optional sql */
		sdb_engine_prepare_symbol_info_sql (dbe, info_data, join_data, sym_info);
	
		if (final_definition_id > 0)
		{		
			query_str = g_strdup_printf("SELECT symbol.symbol_id AS symbol_id, "
				"symbol.name AS name, symbol.file_position AS file_position, "
				"symbol.is_file_scope AS is_file_scope, symbol.signature AS signature "
				"%s FROM heritage "
				"JOIN symbol ON heritage.symbol_id_base = symbol.symbol_id %s "
				"WHERE symbol_id_derived = ("
					"SELECT symbol_id FROM symbol "
						"JOIN sym_kind ON symbol.kind_id = sym_kind.sym_kind_id "
						"WHERE symbol.name = ## /* name:'klassname' type:gchararray */ "
							"AND sym_kind.kind_name = 'class' "
							"AND symbol.scope_id = ## /* name:'defid' type:gint */"
					")", info_data->str, join_data->str);
			
			dyn_node = sdb_engine_insert_dyn_query_node_by_id (dbe, 
							DYN_PREP_QUERY_GET_CLASS_PARENTS,
							sym_info, DYN_GET_CLASS_PARENTS_EXTRA_PAR_FINAL_DEF_POSITIVE,
							query_str);
		}
		else 
		{
			query_str = g_strdup_printf("SELECT symbol.symbol_id AS symbol_id, "
				"symbol.name AS name, symbol.file_position AS file_position, "
				"symbol.is_file_scope AS is_file_scope, "
				"symbol.signature AS signature %s FROM heritage "
				"JOIN symbol ON heritage.symbol_id_base = symbol.symbol_id %s "
				"WHERE symbol_id_derived = ("
					"SELECT symbol_id FROM symbol "
						"JOIN sym_kind ON symbol.kind_id = sym_kind.sym_kind_id "
						"WHERE symbol.name = ## /* name:'klassname' type:gchararray */ "
							"AND sym_kind.kind_name = 'class' "
					")", info_data->str, join_data->str);
			
			dyn_node = sdb_engine_insert_dyn_query_node_by_id (dbe, 
							DYN_PREP_QUERY_GET_CLASS_PARENTS,
							sym_info, DYN_GET_CLASS_PARENTS_EXTRA_PAR_FINAL_DEF_ZERO,
							query_str);
		}	
		
		g_free (query_str);
		g_string_free (info_data, TRUE);
		g_string_free (join_data, TRUE);
	}

	
	if (dyn_node == NULL) 
	{		
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);		
		return NULL;
	}
	
	if ((param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "klassname")) == NULL)
	{
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);
		return NULL;
	}
	gda_holder_set_value_str (param, NULL, klass_name);
	
	if (final_definition_id > 0)
	{
		GValue *value;
		if ((param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "defid")) == NULL)
		{
			if (priv->mutex)
				g_mutex_unlock (priv->mutex);
			return NULL;
		}
		
		value = gda_value_new (G_TYPE_INT);
		g_value_set_int (value, final_definition_id);	
		
		gda_holder_set_value (param, value);
		gda_value_free (value);
	}	
			
	/* execute the query with parametes just set */
	data = gda_connection_statement_execute_select (priv->db_connection, 
												  (GdaStatement*)dyn_node->stmt, 
												  (GdaSet*)dyn_node->plist, NULL);
		
	if (!GDA_IS_DATA_MODEL (data) ||
		gda_data_model_get_n_rows (GDA_DATA_MODEL (data)) <= 0)
	{
		if (data != NULL)
			g_object_unref (data);
		
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);		
		return NULL;
	}

	if (priv->mutex)
		g_mutex_unlock (priv->mutex);
	return (SymbolDBEngineIterator *)symbol_db_engine_iterator_new (data, 
												priv->sym_type_conversion_hash);
}


/**
 * A maximum of 5 filter_kinds are admitted.
 */
#define DYN_GET_GLOBAL_MEMBERS_FILTERED_EXTRA_PAR_LIMIT					1
#define DYN_GET_GLOBAL_MEMBERS_FILTERED_EXTRA_PAR_OFFSET				2
#define DYN_GET_GLOBAL_MEMBERS_FILTERED_EXTRA_PAR_GROUP_YES				4
#define DYN_GET_GLOBAL_MEMBERS_FILTERED_EXTRA_PAR_GROUP_NO				8
#define DYN_GET_GLOBAL_MEMBERS_FILTERED_EXTRA_PAR_INCLUDE_KINDS_YES 	16
#define DYN_GET_GLOBAL_MEMBERS_FILTERED_EXTRA_PAR_INCLUDE_KINDS_NO	 	32

SymbolDBEngineIterator *
symbol_db_engine_get_global_members_filtered (SymbolDBEngine *dbe, 
									const GPtrArray *filter_kinds,
									gboolean include_kinds, gboolean group_them,
									gint results_limit, gint results_offset,
								 	SymExtraInfo sym_info)
{
	SymbolDBEnginePriv *priv;
	GdaDataModel *data;
	GString *info_data;
	GString *join_data;
	GString *filter_str;
	gchar *query_str;
	const gchar *group_by_option;
	gchar *limit = "";
	gboolean limit_free = FALSE;
	gchar *offset = "";
	gboolean offset_free = FALSE;
	const DynChildQueryNode *dyn_node = NULL;
	GdaHolder *param;

	/* use to merge multiple extra_parameters flags */
	gint other_parameters = 0;	
	
	g_return_val_if_fail (dbe != NULL, NULL);
	priv = dbe->priv;

	if (priv->mutex)
	{
		g_mutex_lock (priv->mutex);
	}
	
	/* check for an already flagged sym_info with KIND. SYMINFO_KIND on sym_info
	 * is already contained into the default query infos.
	 */
	sym_info = sym_info & ~SYMINFO_KIND;

	if (group_them == TRUE)
	{
		other_parameters |= DYN_GET_GLOBAL_MEMBERS_FILTERED_EXTRA_PAR_GROUP_YES;
		group_by_option = "GROUP BY symbol.name";
	}
	else 
	{
		other_parameters |= DYN_GET_GLOBAL_MEMBERS_FILTERED_EXTRA_PAR_GROUP_NO;
		group_by_option = "";
	}

	if (results_limit > 0)
	{
		other_parameters |= DYN_GET_GLOBAL_MEMBERS_FILTERED_EXTRA_PAR_LIMIT;
		limit_free = TRUE;
		limit = g_strdup_printf ("LIMIT ## /* name:'limit' type:gint */");
	}
	
	if (results_offset > 0)
	{
		other_parameters |= DYN_GET_GLOBAL_MEMBERS_FILTERED_EXTRA_PAR_OFFSET;
		offset = g_strdup_printf ("OFFSET ## /* name:'offset' type:gint */");
		offset_free = TRUE;
	}
	
	if (filter_kinds == NULL) 
	{
		if ((dyn_node = sdb_engine_get_dyn_query_node_by_id (dbe, 
					DYN_PREP_QUERY_GET_CLASS_PARENTS, sym_info, 
					other_parameters)) == NULL)
		{
			/* info_data contains the stuff after SELECT and befor FROM */
			info_data = g_string_new ("");
	
			/* join_data contains the optionals joins to do to retrieve new data on other
	 	 	 * tables.
	 	 	 */
			join_data = g_string_new ("");

			/* fill info_data and join data with optional sql */
			sdb_engine_prepare_symbol_info_sql (dbe, info_data, join_data, sym_info);
			
			query_str = g_strdup_printf ("SELECT symbol.symbol_id AS symbol_id, "
				"symbol.name AS name, symbol.file_position AS file_position, "
				"symbol.is_file_scope AS is_file_scope, "
				"symbol.signature AS signature, sym_kind.kind_name AS kind_name %s FROM symbol "
					"JOIN sym_kind ON symbol.kind_id = sym_kind.sym_kind_id %s "
					"WHERE symbol.scope_id <= 0 AND symbol.is_file_scope = 0 "
							"%s %s %s", info_data->str, join_data->str,
						 	group_by_option, limit, offset);
			
			dyn_node = sdb_engine_insert_dyn_query_node_by_id (dbe, 
							DYN_PREP_QUERY_GET_GLOBAL_MEMBERS_FILTERED,
							sym_info, other_parameters,
							query_str);			
			
			g_free (query_str);
			g_string_free (join_data, TRUE);
			g_string_free (info_data, TRUE);
		}
	}
	else
	{
		filter_str = g_string_new ("");
		/* build filter string */
		gint i;
		if (include_kinds == TRUE)
		{
			other_parameters |= 
				DYN_GET_GLOBAL_MEMBERS_FILTERED_EXTRA_PAR_INCLUDE_KINDS_YES;
			filter_str = g_string_append (filter_str , 
				"AND sym_kind.kind_name IN (## /* name:'filter0' type:gchararray */");
		}
		else
		{
			other_parameters |= 
				DYN_GET_GLOBAL_MEMBERS_FILTERED_EXTRA_PAR_INCLUDE_KINDS_NO;
			filter_str = g_string_append (filter_str , 
				"AND sym_kind.kind_name NOT IN (## /* name:'filter0' type:gchararray */");
		}
			
		if ((dyn_node = sdb_engine_get_dyn_query_node_by_id (dbe, 
				DYN_PREP_QUERY_GET_CLASS_PARENTS, sym_info, 
				other_parameters)) == NULL)
		{			
			/* info_data contains the stuff after SELECT and befor FROM */
			info_data = g_string_new ("");
	
			/* join_data contains the optionals joins to do to retrieve new data on other
 	 	 	 * tables.
 	 	 	 */
			join_data = g_string_new ("");

			/* fill info_data and join data with optional sql */
			sdb_engine_prepare_symbol_info_sql (dbe, info_data, join_data, sym_info);				

			for (i = 1; i < 5; i++)
			{				
				g_string_append_printf (filter_str , 
						",## /* name:'filter%d' type:gchararray */", i);
			}
			filter_str = g_string_append (filter_str , ")");
		
			query_str = g_strdup_printf ("SELECT symbol.symbol_id AS symbol_id, "
				"symbol.name AS name, symbol.file_position AS file_position, "
				"symbol.is_file_scope AS is_file_scope, symbol.signature AS signature, "
					"sym_kind.kind_name AS kind_name %s FROM symbol "
					"%s JOIN sym_kind ON symbol.kind_id = sym_kind.sym_kind_id "
					"WHERE symbol.scope_id <= 0 AND symbol.is_file_scope = 0 "
					"%s %s %s %s", info_data->str, join_data->str, 
							 filter_str->str, group_by_option, limit, offset);
		
			dyn_node = sdb_engine_insert_dyn_query_node_by_id (dbe, 
						DYN_PREP_QUERY_GET_GLOBAL_MEMBERS_FILTERED,
						sym_info, other_parameters,
						query_str);
				
			g_free (query_str);				
			g_string_free (join_data, TRUE);
			g_string_free (info_data, TRUE);				
		}			
		g_string_free (filter_str, TRUE);
	}
	
	
	if (limit_free)
		g_free (limit);
	
	if (offset_free)
		g_free (offset);
	
	if (dyn_node == NULL)
	{
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);
		return NULL;
	}
	
	if (other_parameters & DYN_GET_GLOBAL_MEMBERS_FILTERED_EXTRA_PAR_LIMIT)
	{	
		GValue *value;
		if ((param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "limit")) == NULL)
		{
			if (priv->mutex)
				g_mutex_unlock (priv->mutex);
			return NULL;
		}

		value = gda_value_new (G_TYPE_INT);
		g_value_set_int (value, results_limit);	
		gda_holder_set_value (param, value);
		gda_value_free (value);
	}

	if (other_parameters & DYN_GET_GLOBAL_MEMBERS_FILTERED_EXTRA_PAR_OFFSET)
	{	
		GValue *value;
		if ((param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "offset")) == NULL)
		{
			if (priv->mutex)
				g_mutex_unlock (priv->mutex);
			return NULL;
		}

		value = gda_value_new (G_TYPE_INT);
		g_value_set_int (value, results_offset);	
		gda_holder_set_value (param, value);
		gda_value_free (value);
	}
	
	
	if (other_parameters & DYN_GET_GLOBAL_MEMBERS_FILTERED_EXTRA_PAR_INCLUDE_KINDS_YES ||
		other_parameters & DYN_GET_GLOBAL_MEMBERS_FILTERED_EXTRA_PAR_INCLUDE_KINDS_NO)
	{	
		/* bad hardcode but it saves strings alloc/dealloc. */
		param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "filter0");
		if (filter_kinds->len > 0)
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 0));

		param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "filter1");
		if (filter_kinds->len > 1)
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 1));
		else
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 0));

		param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "filter2");
		if (filter_kinds->len > 2)
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 2));
		else
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 0));

		param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "filter3");
		if (filter_kinds->len > 3)
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 3));
		else
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 0));
		
		param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "filter4");
		if (filter_kinds->len > 4)
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 4));
		else
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 0));		
	}
	

	/* execute the query with parametes just set */
	data = gda_connection_statement_execute_select (priv->db_connection, 
												  (GdaStatement*)dyn_node->stmt, 
												  (GdaSet*)dyn_node->plist, NULL);
		
	if (!GDA_IS_DATA_MODEL (data) ||
		gda_data_model_get_n_rows (GDA_DATA_MODEL (data)) <= 0)
	{
		if (data != NULL)
			g_object_unref (data);
		
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);		
		return NULL;
	}

	if (priv->mutex)
		g_mutex_unlock (priv->mutex);
	return (SymbolDBEngineIterator *)symbol_db_engine_iterator_new (data, 
												priv->sym_type_conversion_hash);
}


/**
 * A filtered version of the symbol_db_engine_get_scope_members_by_symbol_id ().
 * You can specify which kind of symbols to retrieve, and if include them or exclude.
 * Kinds are 'namespace', 'class' etc.
 * @param filter_kinds cannot be NULL.
 * @param results_limit Limit results to an upper bound. -1 If you don't want to use this par.
 * @param results_offset Skip results_offset results. -1 If you don't want to use this par. 
 */
#define DYN_GET_SCOPE_MEMBERS_BY_SYMBOL_ID_FILTERED_EXTRA_PAR_LIMIT					1
#define DYN_GET_SCOPE_MEMBERS_BY_SYMBOL_ID_FILTERED_EXTRA_PAR_OFFSET				2
#define DYN_GET_SCOPE_MEMBERS_BY_SYMBOL_ID_FILTERED_EXTRA_PAR_INCLUDE_KINDS_YES		4
#define DYN_GET_SCOPE_MEMBERS_BY_SYMBOL_ID_FILTERED_EXTRA_PAR_INCLUDE_KINDS_NO		8

SymbolDBEngineIterator *
symbol_db_engine_get_scope_members_by_symbol_id_filtered (SymbolDBEngine *dbe, 
									gint scope_parent_symbol_id, 
									const GPtrArray *filter_kinds,
									gboolean include_kinds,
									gint results_limit,
									gint results_offset,
									SymExtraInfo sym_info)
{
	SymbolDBEnginePriv *priv;
	gchar *query_str;	
	GdaDataModel *data;
	GString *info_data;
	GString *join_data;
	GString *filter_str;
	gchar *limit = "";
	gboolean limit_free = FALSE;
	gchar *offset = "";
	gboolean offset_free = FALSE;
	gint other_parameters;
	const DynChildQueryNode *dyn_node = NULL;
	GdaHolder *param;
	
	g_return_val_if_fail (dbe != NULL, NULL);
	priv = dbe->priv;

	if (priv->mutex)
	{
		g_mutex_lock (priv->mutex);
	}
	
	if (scope_parent_symbol_id <= 0)
	{
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);
		return NULL;
	}
	
	/* syminfo kind is already included in results */
	sym_info = sym_info & ~SYMINFO_KIND;

	/* init parameters */
	other_parameters = 0;	

	if (results_limit > 0)
	{
		limit = g_strdup_printf ("LIMIT ## /* name:'limit' type:gint */");
		limit_free = TRUE;
		other_parameters |= 
			DYN_GET_SCOPE_MEMBERS_BY_SYMBOL_ID_FILTERED_EXTRA_PAR_LIMIT;
	}
	
	if (results_offset > 0)
	{
		offset = g_strdup_printf ("OFFSET ## /* name:'offset' type:gint */");
		offset_free = TRUE;
		other_parameters |=
			DYN_GET_SCOPE_MEMBERS_BY_SYMBOL_ID_FILTERED_EXTRA_PAR_OFFSET;
	}
	
	filter_str = g_string_new ("");
	/* build filter string */
	gint i;
	if (include_kinds == TRUE)
	{
		other_parameters |= 
			DYN_GET_SCOPE_MEMBERS_BY_SYMBOL_ID_FILTERED_EXTRA_PAR_INCLUDE_KINDS_YES;
		filter_str = g_string_append (filter_str , 
			"AND sym_kind.kind_name IN (## /* name:'filter0' type:gchararray */");		
	}
	else
	{
		other_parameters |= 
			DYN_GET_SCOPE_MEMBERS_BY_SYMBOL_ID_FILTERED_EXTRA_PAR_INCLUDE_KINDS_NO;
		filter_str = g_string_append (filter_str , 
			"AND sym_kind.kind_name NOT IN (## /* name:'filter0' type:gchararray */");
	}

	
	if ((dyn_node = sdb_engine_get_dyn_query_node_by_id (dbe, 
				DYN_PREP_QUERY_GET_SCOPE_MEMBERS_BY_SYMBOL_ID_FILTERED, sym_info, 
				other_parameters)) == NULL)
	{
		/* info_data contains the stuff after SELECT and befor FROM */
		info_data = g_string_new ("");
	
		/* join_data contains the optionals joins to do to retrieve new data on other
	 	 * tables.
	 	 */
		join_data = g_string_new ("");
			
		/* fill info_data and join data with optional sql */
		sdb_engine_prepare_symbol_info_sql (dbe, info_data, join_data, sym_info);
		
		for (i = 1; i < 5; i++)
		{				
			g_string_append_printf (filter_str , 
					",## /* name:'filter%d' type:gchararray */", i);
		}
		filter_str = g_string_append (filter_str , ")");
	
		/* ok, beware that we use an 'alias hack' to accomplish compatibility with 
	 	 * sdb_engine_prepare_symbol_info_sql () function. In particular we called
	 	 * the first joining table 'a', the second one 'symbol', where there is the info we
	 	 * want 
	 	 */		
		query_str = g_strdup_printf ("SELECT symbol.symbol_id AS symbol_id, "
			"symbol.name AS name, "
			"symbol.file_position AS file_position, "
			"symbol.is_file_scope AS is_file_scope, symbol.signature AS signature, "
			"sym_kind.kind_name AS kind_name %s "
			"FROM symbol a, symbol symbol "
			"%s JOIN sym_kind ON symbol.kind_id = sym_kind.sym_kind_id "
			"WHERE a.symbol_id = ## /* name:'scopeparentsymid' type:gint */ "
			"AND symbol.scope_id = a.scope_definition_id "
			"AND symbol.scope_id > 0 %s %s %s", info_data->str, join_data->str,
									 filter_str->str, limit, offset);		
									 
		dyn_node = sdb_engine_insert_dyn_query_node_by_id (dbe, 
							DYN_PREP_QUERY_GET_SCOPE_MEMBERS_BY_SYMBOL_ID_FILTERED,
							sym_info, other_parameters,
							query_str);
			
		g_free (query_str);
		g_string_free (join_data, TRUE);
		g_string_free (info_data, TRUE);
	}
	
	g_string_free (filter_str, TRUE);	
	
	if (limit_free)
		g_free (limit);
	
	if (offset_free)
		g_free (offset);

	if (dyn_node == NULL)
	{
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);
		return NULL;
	}	
	
	if (other_parameters & DYN_GET_SCOPE_MEMBERS_BY_SYMBOL_ID_FILTERED_EXTRA_PAR_LIMIT)
	{	
		GValue *value;
		if ((param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "limit")) == NULL)
		{
			if (priv->mutex)
				g_mutex_unlock (priv->mutex);
			return NULL;
		}

		value = gda_value_new (G_TYPE_INT);
		g_value_set_int (value, results_limit);	
		gda_holder_set_value (param, value);
		gda_value_free (value);
	}

	if (other_parameters & DYN_GET_SCOPE_MEMBERS_BY_SYMBOL_ID_FILTERED_EXTRA_PAR_OFFSET)
	{	
		GValue *value;
		if ((param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "offset")) == NULL)
		{
			if (priv->mutex)
				g_mutex_unlock (priv->mutex);
			return NULL;
		}

		value = gda_value_new (G_TYPE_INT);
		g_value_set_int (value, results_offset);	
		gda_holder_set_value (param, value);
		gda_value_free (value);
	}	
	
	if (other_parameters & 
			DYN_GET_SCOPE_MEMBERS_BY_SYMBOL_ID_FILTERED_EXTRA_PAR_INCLUDE_KINDS_YES ||
		other_parameters & 
			DYN_GET_SCOPE_MEMBERS_BY_SYMBOL_ID_FILTERED_EXTRA_PAR_INCLUDE_KINDS_NO)
	{	
		/* bad hardcode but it saves strings alloc/dealloc. */
		param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "filter0");
		if (filter_kinds->len > 0)
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 0));

		param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "filter1");
		if (filter_kinds->len > 1)
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 1));
		else
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 0));

		param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "filter2");
		if (filter_kinds->len > 2)
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 2));
		else
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 0));

		param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "filter3");
		if (filter_kinds->len > 3)
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 3));
		else
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 0));
		
		param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "filter4");
		if (filter_kinds->len > 4)
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 4));
		else
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 0));		
	}	


	GValue *value;
	if ((param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "scopeparentsymid")) == NULL)
	{
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);
		return NULL;
	}

	value = gda_value_new (G_TYPE_INT);
	g_value_set_int (value, scope_parent_symbol_id);
	gda_holder_set_value (param, value);
	gda_value_free (value);
	
	/* execute the query with parametes just set */
	data = gda_connection_statement_execute_select (priv->db_connection, 
												  (GdaStatement*)dyn_node->stmt, 
												  (GdaSet*)dyn_node->plist, NULL);
		
	if (!GDA_IS_DATA_MODEL (data) ||
		gda_data_model_get_n_rows (GDA_DATA_MODEL (data)) <= 0)
	{
		if (data != NULL)
			g_object_unref (data);
		
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);		
		return NULL;
	}

	if (priv->mutex)
		g_mutex_unlock (priv->mutex);
	return (SymbolDBEngineIterator *)symbol_db_engine_iterator_new (data, 
												priv->sym_type_conversion_hash);	
}

/**
 * Sometimes it's useful going to query just with ids [and so integers] to have
 * a little speed improvement.
 */
#define DYN_GET_SCOPE_MEMBERS_BY_SYMBOL_ID_EXTRA_PAR_LIMIT		1
#define DYN_GET_SCOPE_MEMBERS_BY_SYMBOL_ID_EXTRA_PAR_OFFSET		2

SymbolDBEngineIterator *
symbol_db_engine_get_scope_members_by_symbol_id (SymbolDBEngine *dbe, 
									gint scope_parent_symbol_id, 
									gint results_limit,
									gint results_offset,
									SymExtraInfo sym_info)
{
/*
select b.* from symbol a, symbol b where a.symbol_id = 348 and 
			b.scope_id = a.scope_definition_id;
*/
	SymbolDBEnginePriv *priv;
	gchar *query_str;	
	GdaDataModel *data;
	GString *info_data;
	GString *join_data;
	gchar *limit = "";
	gboolean limit_free = FALSE;
	gchar *offset = "";
	gboolean offset_free = FALSE;
	gint other_parameters;
	const DynChildQueryNode *dyn_node = NULL;
	GdaHolder *param;
	
	g_return_val_if_fail (dbe != NULL, NULL);
	priv = dbe->priv;

	if (priv->mutex)
	{
		g_mutex_lock (priv->mutex);
	}
	
	if (scope_parent_symbol_id <= 0)
	{
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);
		return NULL;
	}

	other_parameters = 0;
	
	if (results_limit > 0)
	{
		limit = g_strdup_printf ("LIMIT ## /* name:'limit' type:gint */");
		limit_free = TRUE;
		other_parameters |= DYN_GET_SCOPE_MEMBERS_BY_SYMBOL_ID_EXTRA_PAR_LIMIT;
	}
	
	if (results_offset > 0)
	{
		offset = g_strdup_printf ("OFFSET ## /* name:'offset' type:gint */");
		offset_free = TRUE;
		other_parameters |= DYN_GET_SCOPE_MEMBERS_BY_SYMBOL_ID_EXTRA_PAR_OFFSET;
	}
	
	if ((dyn_node = sdb_engine_get_dyn_query_node_by_id (dbe, 
				DYN_PREP_QUERY_GET_SCOPE_MEMBERS_BY_SYMBOL_ID, sym_info, 
				other_parameters)) == NULL)
	{
		/* info_data contains the stuff after SELECT and befor FROM */
		info_data = g_string_new ("");
		
		/* join_data contains the optionals joins to do to retrieve new data on other
	 	 * tables.
	 	 */
		join_data = g_string_new ("");

		/* fill info_data and join data with optional sql */
		sdb_engine_prepare_symbol_info_sql (dbe, info_data, join_data, sym_info);		
	
	
		/* ok, beware that we use an 'alias hack' to accomplish compatibility with 
	 	 * sdb_engine_prepare_symbol_info_sql () function. In particular we called
	 	 * the first joining table 'a', the second one 'symbol', where is the info we
	 	 * want 
	 	 */
		query_str = g_strdup_printf ("SELECT symbol.symbol_id AS symbol_id, "
			"symbol.name AS name, symbol.file_position AS file_position, "
			"symbol.is_file_scope AS is_file_scope, symbol.signature AS signature "
			"%s FROM symbol a, symbol symbol "
			"%s WHERE a.symbol_id = ## /* name:'scopeparentsymid' type:gint */ "
			"AND symbol.scope_id = a.scope_definition_id "
			"AND symbol.scope_id > 0 %s %s", info_data->str, join_data->str,
								 limit, offset);	
		
		dyn_node = sdb_engine_insert_dyn_query_node_by_id (dbe, 
							DYN_PREP_QUERY_GET_SCOPE_MEMBERS_BY_SYMBOL_ID,
							sym_info, other_parameters,
							query_str);
		
		g_free (query_str);
		g_string_free (info_data, TRUE);
		g_string_free (join_data, TRUE);
	}
	
	if (limit_free)
		g_free (limit);
	
	if (offset_free)
		g_free (offset);

	if (dyn_node == NULL) 
	{		
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);		
		return NULL;
	}

	
	if (other_parameters & DYN_GET_SCOPE_MEMBERS_BY_SYMBOL_ID_EXTRA_PAR_LIMIT)
	{	
		GValue *value;
		if ((param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "limit")) == NULL)
		{
			if (priv->mutex)
				g_mutex_unlock (priv->mutex);
			return NULL;
		}

		value = gda_value_new (G_TYPE_INT);
		g_value_set_int (value, results_limit);	
		gda_holder_set_value (param, value);
		gda_value_free (value);
	}

	if (other_parameters & DYN_GET_SCOPE_MEMBERS_BY_SYMBOL_ID_EXTRA_PAR_OFFSET)
	{	
		GValue *value;
		if ((param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "offset")) == NULL)
		{
			if (priv->mutex)
				g_mutex_unlock (priv->mutex);
			return NULL;
		}

		value = gda_value_new (G_TYPE_INT);
		g_value_set_int (value, results_offset);	
		gda_holder_set_value (param, value);
		gda_value_free (value);
	}
	
	GValue *value;
	if ((param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "scopeparentsymid")) == NULL)
	{
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);
		return NULL;
	}

	value = gda_value_new (G_TYPE_INT);
	g_value_set_int (value, scope_parent_symbol_id);
	gda_holder_set_value (param, value);
	gda_value_free (value);	
	
	/* execute the query with parametes just set */
	data = gda_connection_statement_execute_select (priv->db_connection, 
												  (GdaStatement*)dyn_node->stmt, 
												  (GdaSet*)dyn_node->plist, NULL);
		
	if (!GDA_IS_DATA_MODEL (data) ||
		gda_data_model_get_n_rows (GDA_DATA_MODEL (data)) <= 0)
	{
		if (data != NULL)
			g_object_unref (data);
		
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);		
		return NULL;
	}

	if (priv->mutex)
		g_mutex_unlock (priv->mutex);
	return (SymbolDBEngineIterator *)symbol_db_engine_iterator_new (data, 
												priv->sym_type_conversion_hash);
}

/** 
 * scope_path cannot be NULL.
 * scope_path will be something like "scope1_kind", "scope1_name", "scope2_kind", 
 * "scope2_name", NULL 
 */
SymbolDBEngineIterator *
symbol_db_engine_get_scope_members (SymbolDBEngine *dbe, 
									const GPtrArray* scope_path, 
									SymExtraInfo sym_info)
{
/*
simple scope 
	
select * from symbol where scope_id = (
	select scope.scope_id from scope
		inner join sym_type on scope.type_id = sym_type.type_id 
		where sym_type.type = 'class' 
			and scope.scope_name = 'MyClass'
	);
	
select * from symbol where scope_id = (
	select scope.scope_id from scope 
		inner join sym_type on scope.type_id = sym_type.type_id 
		where sym_type.type = 'struct' 
			and scope.scope_name = '_faa_1');
	
	
es. scope_path = First, namespace, Second, namespace, NULL, 
	symbol_name = Second_1_class	
*/
	SymbolDBEnginePriv *priv;
	gchar *query_str;	
	GdaDataModel *data;
	gint final_definition_id;
	GString *info_data;
	GString *join_data;
	GdaHolder *param;
	GValue *value;
	const DynChildQueryNode *dyn_node;
	
	g_return_val_if_fail (dbe != NULL, NULL);
	priv = dbe->priv;

	if (priv->mutex)
	{
		g_mutex_lock (priv->mutex);	
	}
	
	final_definition_id = sdb_engine_walk_down_scope_path (dbe, scope_path);
	
	if (final_definition_id <= 0) 
	{
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);
		return NULL;
	}

	if ((dyn_node = sdb_engine_get_dyn_query_node_by_id (dbe, 
		DYN_PREP_QUERY_GET_SCOPE_MEMBERS, sym_info, 0)) == NULL)
	{
		/* info_data contains the stuff after SELECT and befor FROM */
		info_data = g_string_new ("");
	
		/* join_data contains the optionals joins to do to retrieve new data on other
	 	 * tables.
	 	 */
		join_data = g_string_new ("");

		/* fill info_data and join data with optional sql */
		sdb_engine_prepare_symbol_info_sql (dbe, info_data, join_data, sym_info);
	
		query_str = g_strdup_printf ("SELECT symbol.symbol_id AS symbol_id, "
			"symbol.name AS name, symbol.file_position AS file_position, "
			"symbol.is_file_scope AS is_file_scope, symbol.signature AS signature "
			"%s FROM symbol "
			"%s WHERE scope_id = ## /* name:'defid' type:gint */", 
									 info_data->str, join_data->str);
		
		dyn_node = sdb_engine_insert_dyn_query_node_by_id (dbe, 
						DYN_PREP_QUERY_GET_SCOPE_MEMBERS,
						sym_info, 0,
						query_str);
		
		g_free (query_str);
		g_string_free (info_data, TRUE);
		g_string_free (join_data, TRUE);		
	}

	if (dyn_node == NULL) 
	{		
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);		
		return NULL;
	}	
	
	if ((param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "defid")) == NULL)
	{
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);		
		return NULL;
	}
		
	value = gda_value_new (G_TYPE_INT);
	g_value_set_int (value, final_definition_id);
		
	gda_holder_set_value (param, value);
	gda_value_free (value);
	

	/* execute the query with parametes just set */
	data = gda_connection_statement_execute_select (priv->db_connection, 
												  (GdaStatement*)dyn_node->stmt, 
												  (GdaSet*)dyn_node->plist, NULL);
	
	if (!GDA_IS_DATA_MODEL (data) ||
		gda_data_model_get_n_rows (data) <= 0)
	{
		if (data != NULL)
			g_object_unref (data);
		
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);
		return NULL;
	}

	
	if (priv->mutex)
		g_mutex_unlock (priv->mutex);
	return (SymbolDBEngineIterator *)symbol_db_engine_iterator_new (data, 
												priv->sym_type_conversion_hash);
}

/**
 * Returns an iterator to the data retrieved from database. 
 * It will be possible to get the scope specified by the line of the file. 
 */
SymbolDBEngineIterator *
symbol_db_engine_get_current_scope (SymbolDBEngine *dbe, const gchar* filename, 
									gulong line, SymExtraInfo sym_info)
{
	SymbolDBEnginePriv *priv;
	gchar *query_str;
	GdaDataModel *data;
	GString *info_data;
	GString *join_data;
	GdaHolder *param;
	GValue *value;
	const DynChildQueryNode *dyn_node;
	
	g_return_val_if_fail (dbe != NULL, NULL);
	priv = dbe->priv;
	
	if (priv->mutex)
	{
		g_mutex_lock (priv->mutex);
	}

	if ((dyn_node = sdb_engine_get_dyn_query_node_by_id (dbe, 
		DYN_PREP_QUERY_GET_CURRENT_SCOPE, sym_info, 0)) == NULL)
	{
		/* info_data contains the stuff after SELECT and befor FROM */
		info_data = g_string_new ("");
	
		/* join_data contains the optionals joins to do to retrieve new data on other
	 	 * tables.
	 	 */
		join_data = g_string_new ("");

		/* fill info_data and join data with optional sql */
		sdb_engine_prepare_symbol_info_sql (dbe, info_data, join_data, sym_info);
	
		/* WARNING: probably there can be some problems with escaping file names here.
	 	 * They should come already escaped as from project db.
	 	 */
		query_str = g_strdup_printf ("SELECT symbol.symbol_id AS symbol_id, "
			"symbol.name AS name, symbol.file_position AS file_position, "
			"symbol.is_file_scope AS is_file_scope, symbol.signature AS signature, "
			"MIN(## /* name:'linenum' type:gint */ - symbol.file_position) "
			"FROM symbol "
				"JOIN file ON file_defined_id = file_id "
				"WHERE file.file_path = ## /* name:'filepath' type:gchararray */ "
					"AND ## /* name:'linenum' type:gint */ - "
					"symbol.file_position >= 0");

		dyn_node = sdb_engine_insert_dyn_query_node_by_id (dbe, 
						DYN_PREP_QUERY_GET_CURRENT_SCOPE,
						sym_info, 0,
						query_str);
		
		g_string_free (info_data, TRUE);
		g_string_free (join_data, TRUE);		
		g_free (query_str);
	}
	
	if (dyn_node == NULL) 
	{		
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);		
		return NULL;
	}
	
	if ((param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "linenum")) == NULL)
	{
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);		
		return NULL;
	}
		
	value = gda_value_new (G_TYPE_INT);
	g_value_set_int (value, line);		
	gda_holder_set_value (param, value);
	gda_value_free (value);
	
	
	if ((param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "filepath")) == NULL)
	{
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);		
		return NULL;
	}
		
	gda_holder_set_value_str (param, NULL, filename);	
	
	/* execute the query with parametes just set */
	data = gda_connection_statement_execute_select (priv->db_connection, 
												  (GdaStatement*)dyn_node->stmt, 
												  (GdaSet*)dyn_node->plist, NULL);
	
	if (!GDA_IS_DATA_MODEL (data) ||
		gda_data_model_get_n_rows (data) <= 0)
	{
		if (data != NULL)
			g_object_unref (data);
		
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);
		return NULL;
	}
	
	if (priv->mutex)
		g_mutex_unlock (priv->mutex);
	return (SymbolDBEngineIterator *)symbol_db_engine_iterator_new (data, 
												priv->sym_type_conversion_hash);
}


/**
 * @param file_path Full local file path, e.g. /home/user/foo/file.c
 */
SymbolDBEngineIterator *
symbol_db_engine_get_file_symbols (SymbolDBEngine *dbe, 
									const gchar *file_path, SymExtraInfo sym_info)
{
	SymbolDBEnginePriv *priv;
	gchar *query_str;	
	GdaDataModel *data;
	GString *info_data;
	GString *join_data;
	GdaHolder *param;
	const DynChildQueryNode *dyn_node;
	
	g_return_val_if_fail (dbe != NULL, NULL);
	g_return_val_if_fail (file_path != NULL, NULL);
	priv = dbe->priv;	
	g_return_val_if_fail (priv->db_directory != NULL, NULL);

	if (priv->mutex)
	{
		g_mutex_lock (priv->mutex);
	}
	/* check for an already flagged sym_info with FILE_PATH. SYMINFO_FILE_PATH on 
	 * sym_info is already contained into the default query infos.
	 */
	sym_info = sym_info & ~SYMINFO_FILE_PATH;
	

	if ((dyn_node = sdb_engine_get_dyn_query_node_by_id (dbe, 
		DYN_PREP_QUERY_GET_FILE_SYMBOLS, sym_info, 0)) == NULL)
	{
		/* info_data contains the stuff after SELECT and befor FROM */
		info_data = g_string_new ("");

		/* join_data contains the optionals joins to do to retrieve new data on other
	 	 * tables.
	 	 */
		join_data = g_string_new ("");

		/* fill info_data and join data with optional sql */
		sdb_engine_prepare_symbol_info_sql (dbe, info_data, join_data, sym_info);

		/* rember to do a file_path + strlen(priv->db_directory): a project relative 
	 	 * file path 
	 	 */
		query_str = g_strdup_printf ("SELECT symbol.symbol_id AS symbol_id, "
			"symbol.name AS name, symbol.file_position AS file_position, "
			"symbol.is_file_scope AS is_file_scope, symbol.signature AS signature "
			"%s FROM symbol "
				"JOIN file ON symbol.file_defined_id = file.file_id "
			"%s WHERE file.file_path = ## /* name:'filepath' type:gchararray */", 
						info_data->str, join_data->str);
	
		dyn_node = sdb_engine_insert_dyn_query_node_by_id (dbe, 
						DYN_PREP_QUERY_GET_FILE_SYMBOLS,
						sym_info, 0,
						query_str);
		
		g_free (query_str);
		g_string_free (info_data, TRUE);
		g_string_free (join_data, TRUE);
	}

	if (dyn_node == NULL) 
	{		
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);		
		return NULL;
	}
	
/*	DEBUG_PRINT ("query for symbol_db_engine_get_file_symbols is %s [filepath: %s]",
				 dyn_node->query_str, file_path);*/
	
	if ((param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "filepath")) == NULL)
	{
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);		
		return NULL;
	}
		
	gchar *relative_path = symbol_db_engine_get_file_db_path (dbe, file_path);
	if (relative_path == NULL)
	{
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);		
		return NULL;
	}
	
	gda_holder_set_value_str (param, NULL, relative_path);
	g_free (relative_path);
	
	/* execute the query with parametes just set */
	data = gda_connection_statement_execute_select (priv->db_connection, 
												  (GdaStatement*)dyn_node->stmt, 
												  (GdaSet*)dyn_node->plist, NULL);
	
	if (!GDA_IS_DATA_MODEL (data) ||
		gda_data_model_get_n_rows (data) <= 0)
	{
		if (data != NULL)
			g_object_unref (data);
		
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);
		return NULL;
	}
	
	if (priv->mutex)
		g_mutex_unlock (priv->mutex);
	return (SymbolDBEngineIterator *)symbol_db_engine_iterator_new (data, 
												priv->sym_type_conversion_hash);
}


SymbolDBEngineIterator *
symbol_db_engine_get_symbol_info_by_id (SymbolDBEngine *dbe, 
									gint sym_id, SymExtraInfo sym_info)
{
	SymbolDBEnginePriv *priv;
	gchar *query_str;	
	GdaDataModel *data;
	GString *info_data;
	GString *join_data;
	GdaHolder *param;
	GValue *value;
	const DynChildQueryNode *dyn_node;
	
	g_return_val_if_fail (dbe != NULL, NULL);
	priv = dbe->priv;

	if (priv->mutex)
	{
		g_mutex_lock (priv->mutex);
	}

	if ((dyn_node = sdb_engine_get_dyn_query_node_by_id (dbe, 
		DYN_PREP_QUERY_GET_SYMBOL_INFO_BY_ID, sym_info, 0)) == NULL)
	{
		/* info_data contains the stuff after SELECT and befor FROM */
		info_data = g_string_new ("");
	
		/* join_data contains the optionals joins to do to retrieve new data on other
	 	 * tables.
	 	 */
		join_data = g_string_new ("");

		/* fill info_data and join data with optional sql */
		sdb_engine_prepare_symbol_info_sql (dbe, info_data, join_data, sym_info);
	
		query_str = g_strdup_printf ("SELECT symbol.symbol_id AS symbol_id, "
			"symbol.name AS name, symbol.file_position AS file_position, "
			"symbol.is_file_scope AS is_file_scope, symbol.signature AS signature "
			"%s FROM symbol "
			"%s WHERE symbol.symbol_id = ## /* name:'symid' type:gint */", 
									info_data->str, join_data->str);
		
		dyn_node = sdb_engine_insert_dyn_query_node_by_id (dbe, 
						DYN_PREP_QUERY_GET_SYMBOL_INFO_BY_ID,
						sym_info, 0,
						query_str);
		
		g_free (query_str);
		g_string_free (info_data, TRUE);
		g_string_free (join_data, TRUE);	
	}
	
	if ((param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "symid")) == NULL)
	{
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);		
		return NULL;
	}
		
	value = gda_value_new (G_TYPE_INT);
	g_value_set_int (value, sym_id);		
	gda_holder_set_value (param, value);
	gda_value_free (value);
	
	/* execute the query with parametes just set */
	data = gda_connection_statement_execute_select (priv->db_connection, 
												  (GdaStatement*)dyn_node->stmt, 
												  (GdaSet*)dyn_node->plist, NULL);
	
	if (!GDA_IS_DATA_MODEL (data) ||
		gda_data_model_get_n_rows (data) <= 0)
	{
		if (data != NULL)
			g_object_unref (data);
		
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);
		return NULL;
	}
	
	if (priv->mutex)
		g_mutex_unlock (priv->mutex);
	return (SymbolDBEngineIterator *)symbol_db_engine_iterator_new (data, 
												priv->sym_type_conversion_hash);
}



/**
 * Use this function to find symbols names by patterns like '%foo_func%'
 * that will return a family of my_foo_func_1, your_foo_func_2 etc
 * @name must not be NULL.
 */
SymbolDBEngineIterator *
symbol_db_engine_find_symbol_by_name_pattern (SymbolDBEngine *dbe, 
									const gchar *pattern, SymExtraInfo sym_info)
{
	SymbolDBEnginePriv *priv;
	gchar *query_str;	
	GdaDataModel *data;
	GString *info_data;
	GString *join_data;
	GdaHolder *param;
	const DynChildQueryNode *dyn_node;
	
	g_return_val_if_fail (dbe != NULL, NULL);
	priv = dbe->priv;

	if (priv->mutex)
	{
		g_mutex_lock (priv->mutex);
	}

	if ((dyn_node = sdb_engine_get_dyn_query_node_by_id (dbe, 
		DYN_PREP_QUERY_FIND_SYMBOL_NAME_BY_PATTERN, sym_info, 0)) == NULL)
	{
		/* info_data contains the stuff after SELECT and befor FROM */
		info_data = g_string_new ("");
	
		/* join_data contains the optionals joins to do to retrieve new data on other
		 * tables.
		 */
		join_data = g_string_new ("");

		/* fill info_data and join data with optional sql */
		sdb_engine_prepare_symbol_info_sql (dbe, info_data, join_data, sym_info);
	
		query_str = g_strdup_printf ("SELECT symbol.symbol_id AS symbol_id, "
			"symbol.name AS name, symbol.file_position AS file_position, "
			"symbol.is_file_scope AS is_file_scope, symbol.signature AS signature "
			"%s FROM symbol "
			"%s WHERE symbol.name LIKE ## /* name:'pattern' type:gchararray */", 
									 info_data->str, join_data->str);
		
		dyn_node = sdb_engine_insert_dyn_query_node_by_id (dbe, 
						DYN_PREP_QUERY_FIND_SYMBOL_NAME_BY_PATTERN,
						sym_info, 0,
						query_str);
		
		g_free (query_str);
		g_string_free (info_data, TRUE);
		g_string_free (join_data, TRUE);
	}

	if (dyn_node == NULL) 
	{		
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);		
		return NULL;
	}

	if ((param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "pattern")) == NULL)
	{
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);		
		return NULL;
	}

	gda_holder_set_value_str (param, NULL, pattern);	
	
	/* execute the query with parametes just set */
	data = gda_connection_statement_execute_select (priv->db_connection, 
												  (GdaStatement*)dyn_node->stmt, 
												  (GdaSet*)dyn_node->plist, NULL);
	
	if (!GDA_IS_DATA_MODEL (data) ||
		gda_data_model_get_n_rows (data) <= 0)
	{
		if (data != NULL)
			g_object_unref (data);
		
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);
		return NULL;
	}
	
	if (priv->mutex)
		g_mutex_unlock (priv->mutex);
	return (SymbolDBEngineIterator *)symbol_db_engine_iterator_new (data, 
												priv->sym_type_conversion_hash);
}

/** 
 * No iterator for now. We need the quickest query possible. 
 * @param db_file Can be NULL. In that case there won't be any check on file.
 */
gint
symbol_db_engine_get_parent_scope_id_by_symbol_id (SymbolDBEngine *dbe,
									gint scoped_symbol_id,
									const gchar * db_file)
{
/*
select * from symbol where scope_definition_id = (
	select scope_id from symbol where symbol_id = 26
	);
*/
	SymbolDBEnginePriv *priv;
	GdaDataModel *data;
	const GValue* value;
	GValue *value_scoped;
	GdaHolder *param;
	gint res;
	gint num_rows;
	const GdaSet *plist;
	const GdaStatement *stmt, *stmt2;	
	
	g_return_val_if_fail (dbe != NULL, -1);
	priv = dbe->priv;

	if (priv->mutex)
	{
		g_mutex_lock (priv->mutex);
	}

	if (db_file == NULL)
	{
		if ((stmt = sdb_engine_get_statement_by_query_id (dbe, 
						PREP_QUERY_GET_PARENT_SCOPE_ID_BY_SYMBOL_ID_NO_FILE))
			== NULL)
		{
			if (priv->mutex)
				g_mutex_unlock (priv->mutex);
			return -1;
		}

		plist = sdb_engine_get_query_parameters_list (dbe, 
						PREP_QUERY_GET_PARENT_SCOPE_ID_BY_SYMBOL_ID_NO_FILE);		
	}
	else 
	{
		if ((stmt = sdb_engine_get_statement_by_query_id (dbe, 
						PREP_QUERY_GET_PARENT_SCOPE_ID_BY_SYMBOL_ID))
			== NULL)
		{
			if (priv->mutex)
				g_mutex_unlock (priv->mutex);
			return -1;
		}

		plist = sdb_engine_get_query_parameters_list (dbe, 
						PREP_QUERY_GET_PARENT_SCOPE_ID_BY_SYMBOL_ID);
		
		/* dbfile parameter */
		if ((param = gda_set_get_holder ((GdaSet*)plist, "dbfile")) == NULL)
		{			
			g_warning ("param dbfile is NULL from pquery!");
			if (priv->mutex)
				g_mutex_unlock (priv->mutex);
			return -1;
		}
		gda_holder_set_value_str (param, NULL, db_file);
	}

	/* scoped symbol id */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "symid")) == NULL)
	{			
		g_warning ("param symid is NULL from pquery!");
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);
		return -1;
	}	
	value_scoped = gda_value_new (G_TYPE_INT);
	g_value_set_int (value_scoped, scoped_symbol_id);
	gda_holder_set_value (param, value_scoped);
	gda_value_free (value_scoped);	
	
	/* execute the query with parametes just set */
	data = gda_connection_statement_execute_select (priv->db_connection, 
													  (GdaStatement*)stmt, 
													  (GdaSet*)plist, NULL);
		
	if (!GDA_IS_DATA_MODEL (data) ||
		(num_rows = gda_data_model_get_n_rows (GDA_DATA_MODEL (data))) <= 0)
	{
		if (data != NULL)
			g_object_unref (data);
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);		
		return -1;
	}

	/* Hey, we can go in trouble here. Imagine this situation:
	 * We have two namespaces, 'Editor' and 'Entwickler'. Inside
	 * them there are two classes with the same name, 'Preferences', hence 
	 * the same scope. We still aren't able to retrieve the real parent_symbol_id
	 * of a method into these classes.
	 *
	 * This is how the above query will result:
	 * 
	 * 324|31|Preferences|28|0||192|92|276|3|-1|-1|0
	 * 365|36|Preferences|33|0||192|2|276|3|-1|-1|0
	 *
	 * Say that 92 is 'Editor' scope, and '2' is 'Entwickler' one. We're looking 
	 * for the latter, and so we know that the right answer is symbol_id 365.
	 * 1. We'll store the symbold_id(s) and the scope_id(s) of both 324 and 365.
	 * 2. We'll query the db to retrieve all the symbols written before our 
	 *    scoped_symbol_id in the file where scoped_symbol_id is.
	 * 3. Compare every symbol's scope_definition_id retrieved on step 2 against
	 *    all the ones saved at point 1. The first match is our parent_scope symbol_id.
	 */
	if (num_rows > 1) 
	{
		gint i;
		
		GList *candidate_parents = NULL;
		typedef struct _candidate_node {
			gint symbol_id;
			gint scope_id;
		} candidate_node;
			
		GdaDataModel *detailed_data;
		
		/* step 1 */
		for (i=0; i < num_rows; i++)
		{
			candidate_node *node;
			const GValue *tmp_value, *tmp_value1;
			
			node = g_new0 (candidate_node, 1);
			
			/* get the symbol_id from the former 'data' result set */
			tmp_value = gda_data_model_get_value_at (data, 0, i);
			node->symbol_id = tmp_value != NULL && G_VALUE_HOLDS_INT (tmp_value)
				? g_value_get_int (tmp_value) : -1;
			
			/* get scope_id [or the definition] */
			/* handles this case:
			 * 
			 * namespace_foo [scope_id 0 or > 0, scope_def X]
			 * |
			 * +--- class_foo 
			 *      |
			 *      +---- scoped_symbol_id
  		     *
			 */
			tmp_value1 = gda_data_model_get_value_at (data, 4, i);
			node->scope_id = tmp_value1 != NULL && G_VALUE_HOLDS_INT (tmp_value1)
				? g_value_get_int (tmp_value1) : -1;

			/* handles this case:
			 * 
			 * namespace_foo [scope_id 0, scope_def X]
			 * |
			 * +--- scoped_symbol_id
			 *
			 */
			if (node->scope_id <= 0)
			{
				tmp_value1 = gda_data_model_get_value_at (data, 3, i);
				node->scope_id = tmp_value1 != NULL && G_VALUE_HOLDS_INT (tmp_value1)
					? g_value_get_int (tmp_value1) : -1;
			}
						
			candidate_parents = g_list_prepend (candidate_parents, node);
		}

		
		/* step 2 */
		if ((stmt2 = sdb_engine_get_statement_by_query_id (dbe, 
					PREP_QUERY_GET_PARENT_SCOPE_ID_BY_SYMBOL_ID_NO_FILE)) == NULL)
		{
			if (priv->mutex)
				g_mutex_unlock (priv->mutex);
			return -1;
		}

		plist = sdb_engine_get_query_parameters_list (dbe, 
						PREP_QUERY_GET_PARENT_SCOPE_ID_BY_SYMBOL_ID_NO_FILE);		

		/* scoped symbol id */
		if ((param = gda_set_get_holder ((GdaSet*)plist, "scopedsymid")) == NULL)
		{			
			g_warning ("param scopedsymid is NULL from pquery!");
			if (priv->mutex)
				g_mutex_unlock (priv->mutex);
			return -1;
		}	
		value_scoped = gda_value_new (G_TYPE_INT);
		g_value_set_int (value_scoped, scoped_symbol_id);
		gda_holder_set_value (param, value_scoped);
		gda_value_free (value_scoped);	
	
		/* we should receive just ONE row. If it's > 1 than we have surely an
		 * error on code.
		 */
		detailed_data = gda_connection_statement_execute_select (priv->db_connection, 
													  (GdaStatement*)stmt2, 
													  (GdaSet*)plist, NULL);
				
		if (!GDA_IS_DATA_MODEL (detailed_data) ||
			(num_rows = gda_data_model_get_n_rows (GDA_DATA_MODEL (detailed_data))) <= 0)
		{
			if (detailed_data != NULL)
				g_object_unref (detailed_data);
			if (priv->mutex)
				g_mutex_unlock (priv->mutex);		
			res = -1;
		}	
		else		/* ok we have a good result here */
		{
			gint j;
			gboolean found = FALSE;
			gint parent_scope_definition_id;
			gint found_symbol_id = -1;
			candidate_node *node = NULL;
			
			for (j=0; j < gda_data_model_get_n_rows (detailed_data); j++)
			{
				const GValue *tmp_value;				
				gint k;

				tmp_value = gda_data_model_get_value_at (detailed_data, 0, j);
				parent_scope_definition_id = tmp_value != NULL && G_VALUE_HOLDS_INT (tmp_value)
					? g_value_get_int (tmp_value) : -1;
				
				for (k = 0; k < g_list_length (candidate_parents); k++) 
				{
					node = g_list_nth_data (candidate_parents, k); 
					/*DEBUG_PRINT ("node->symbol_id %d node->scope_id %d", 
								 node->symbol_id, node->scope_id);*/
					if (node->scope_id == parent_scope_definition_id) 
					{
						found = TRUE;
						found_symbol_id = node->symbol_id;
						break;
					}						
				}
				
				if (found)
					break;
			}
			res = found_symbol_id;
		}
		
		if (detailed_data)
			g_object_unref (detailed_data);

		for (i=0; i < g_list_length (candidate_parents); i++) 
		{
			g_free (g_list_nth_data (candidate_parents, i));
		}
		g_list_free (candidate_parents);
	}
	else
	{
		value = gda_data_model_get_value_at (data, 0, 0);
		res = value != NULL && G_VALUE_HOLDS_INT (value)
			? g_value_get_int (value) : -1;		
	}
	g_object_unref (data);
	
	if (priv->mutex)
		g_mutex_unlock (priv->mutex);
	return res;
}

/**
 * @param pattern Pattern you want to search for. If NULL it will use '%' and LIKE for query.
 *        Please provide a pattern with '%' if you also specify a exact_match = FALSE
 * @param exact_match Should the pattern be searched for an exact match?
 * @param filter_kinds Can be NULL. In that case these filters will be taken into consideration.
 * @param include_kinds Should the filter_kinds (if not null) be applied as inluded or excluded?
 * @param global_symbols_search If TRUE only global public function will be searched. If false
 *		  even private or static (for C language) will be searched.
 * @param results_limit Limit results to an upper bound. -1 If you don't want to use this par.
 * @param results_offset Skip results_offset results. -1 If you don't want to use this par.	 
 * @param sym_info Infos about symbols you want to know.
 */
#define DYN_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED_EXTRA_PAR_EXACT_MATCH_YES			1
#define DYN_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED_EXTRA_PAR_EXACT_MATCH_NO			2
#define DYN_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED_EXTRA_PAR_INCLUDE_KINDS_YES		4
#define DYN_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED_EXTRA_PAR_INCLUDE_KINDS_NO			8
#define DYN_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED_EXTRA_PAR_GLOBAL_SEARCH_YES		16
#define DYN_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED_EXTRA_PAR_GLOBAL_SEARCH_NO			32
#define DYN_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED_EXTRA_PAR_LIMIT					64
#define DYN_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED_EXTRA_PAR_OFFSET					128

SymbolDBEngineIterator *
symbol_db_engine_find_symbol_by_name_pattern_filtered (SymbolDBEngine *dbe, 
									const gchar *pattern, 
									gboolean exact_match,
									const GPtrArray *filter_kinds,
									gboolean include_kinds,
									gboolean global_symbols_search,
									gint results_limit, 
									gint results_offset,
									SymExtraInfo sym_info)
{
	SymbolDBEnginePriv *priv;
	GdaDataModel *data;
	GString *info_data;
	GString *join_data;
	GString *filter_str;
	gchar *query_str;
	const gchar *match_str;
	GdaHolder *param;
	const DynChildQueryNode *dyn_node;
	GValue *value;
	gint other_parameters;
	gchar *limit = "";
	gboolean limit_free = FALSE;
	gchar *offset = "";
	gboolean offset_free = FALSE;

	g_return_val_if_fail (dbe != NULL, NULL);
	priv = dbe->priv;

	if (priv->mutex)
	{
		g_mutex_lock (priv->mutex);
	}
	
	sym_info = sym_info & ~SYMINFO_KIND;
	
	/* initialize dynamic stuff */
	other_parameters = 0;
	dyn_node = NULL;

	
	/* check for a null pattern. If NULL we'll set a patter like '%' 
	 * and exact_match = FALSE . In this way we will match everything.
	 */
	if (pattern == NULL)
	{
		pattern = "%";
		exact_match = FALSE;
		other_parameters |= DYN_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED_EXTRA_PAR_EXACT_MATCH_NO;
	}
	
	/* check for match */
	if (exact_match == TRUE)
	{
		other_parameters |= 
			DYN_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED_EXTRA_PAR_EXACT_MATCH_YES;
		match_str = " = ## /* name:'pattern' type:gchararray */";
	}
	else
	{
		other_parameters |= 
			DYN_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED_EXTRA_PAR_EXACT_MATCH_NO;
		match_str = " LIKE ## /* name:'pattern' type:gchararray */";
	}
	
	if (global_symbols_search == TRUE)
	{
		other_parameters |= 
			DYN_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED_EXTRA_PAR_GLOBAL_SEARCH_YES;
	}
	else
	{
		other_parameters |= 
			DYN_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED_EXTRA_PAR_GLOBAL_SEARCH_NO;
	}
	
	if (results_limit > 0)
	{
		other_parameters |= DYN_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED_EXTRA_PAR_LIMIT;
		limit_free = TRUE;
		limit = g_strdup_printf ("LIMIT ## /* name:'limit' type:gint */");
	}
	
	if (results_offset > 0)
	{
		other_parameters |= DYN_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED_EXTRA_PAR_OFFSET;
		offset = g_strdup_printf ("OFFSET ## /* name:'offset' type:gint */");
		offset_free = TRUE;
	}
	
	if (filter_kinds == NULL) 
	{
		if ((dyn_node = sdb_engine_get_dyn_query_node_by_id (dbe, 
			DYN_PREP_QUERY_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED, sym_info, 
														 other_parameters)) == NULL)
		{
			/* info_data contains the stuff after SELECT and befor FROM */
			info_data = g_string_new ("");
	
			/* join_data contains the optionals joins to do to retrieve new data on other
			 * tables.
		 	 */
			join_data = g_string_new ("");

		
			/* fill info_data and join data with optional sql */
			sdb_engine_prepare_symbol_info_sql (dbe, info_data, join_data, sym_info);

			query_str = g_strdup_printf ("SELECT symbol.symbol_id AS symbol_id, "
					"symbol.name AS name, symbol.file_position AS file_position, "
					"symbol.is_file_scope AS is_file_scope, symbol.signature AS signature, "
					"sym_kind.kind_name AS kind_name "
					"%s FROM symbol %s JOIN sym_kind ON symbol.kind_id = sym_kind.sym_kind_id "
					"WHERE symbol.name %s AND symbol.is_file_scope = "
					"## /* name:'globalsearch' type:gint */ %s %s", 
				info_data->str, join_data->str, match_str, limit, offset);			

			dyn_node = sdb_engine_insert_dyn_query_node_by_id (dbe, 
						DYN_PREP_QUERY_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED,
						sym_info, other_parameters,
						query_str);

			g_free (query_str);
			g_string_free (info_data, TRUE);
			g_string_free (join_data, TRUE);
		}
	}
	else
	{
		filter_str = g_string_new ("");
		/* build filter string */
		if (filter_kinds->len > 0)
		{
			if (include_kinds == TRUE)
			{
				other_parameters |= 
					DYN_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED_EXTRA_PAR_INCLUDE_KINDS_YES;
				
				filter_str = g_string_append (filter_str, 
					"AND sym_kind.kind_name IN (## /* name:'filter0' type:gchararray */");
			}
			else
			{
				other_parameters |= 
					DYN_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED_EXTRA_PAR_INCLUDE_KINDS_NO;
				
				filter_str = g_string_append (filter_str, 
					"AND sym_kind.kind_name NOT IN (## /* name:'filter0' type:gchararray */");
			}

			if ((dyn_node = sdb_engine_get_dyn_query_node_by_id (dbe, 
					DYN_PREP_QUERY_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED, sym_info, 
					other_parameters)) == NULL)
			{			
				gint i;
				/* info_data contains the stuff after SELECT and befor FROM */
				info_data = g_string_new ("");
	
				/* join_data contains the optionals joins to do to retrieve new data on other
				 * tables.
			 	 */
				join_data = g_string_new ("");
		
				/* fill info_data and join data with optional sql */
				sdb_engine_prepare_symbol_info_sql (dbe, info_data, join_data, sym_info);
				
				for (i = 1; i < 5; i++)
				{				
					g_string_append_printf (filter_str , 
							",## /* name:'filter%d' type:gchararray */", i);
				}
				filter_str = g_string_append (filter_str , ")");

				query_str = g_strdup_printf ("SELECT symbol.symbol_id AS symbol_id, symbol.name AS name, "
					"symbol.file_position AS file_position, "
					"symbol.is_file_scope AS is_file_scope, symbol.signature AS signature, "
					"sym_kind.kind_name AS kind_name "
						"%s FROM symbol %s JOIN sym_kind ON symbol.kind_id = sym_kind.sym_kind_id "
						"WHERE symbol.name %s AND symbol.is_file_scope = "
						"## /* name:'globalsearch' type:gint */ %s GROUP BY symbol.name %s %s", 
				 		info_data->str, join_data->str, match_str, 
				 		filter_str->str, limit, offset);

				dyn_node = sdb_engine_insert_dyn_query_node_by_id (dbe, 
							DYN_PREP_QUERY_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED,
							sym_info, other_parameters,
							query_str);

				g_free (query_str);
				g_string_free (info_data, TRUE);
				g_string_free (join_data, TRUE);				
			}
		}

		g_string_free (filter_str, TRUE);
	}	
	
	if (limit_free)
		g_free (limit);
	
	if (offset_free)
		g_free (offset);

	if (dyn_node == NULL)
	{
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);
		return NULL;
	}
	
	if (other_parameters & DYN_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED_EXTRA_PAR_LIMIT)
	{	
		GValue *value;
		if ((param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "limit")) == NULL)
		{
			if (priv->mutex)
				g_mutex_unlock (priv->mutex);
			return NULL;
		}

		value = gda_value_new (G_TYPE_INT);
		g_value_set_int (value, results_limit);	
		gda_holder_set_value (param, value);
		gda_value_free (value);
	}

	if (other_parameters & DYN_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED_EXTRA_PAR_OFFSET)
	{	
		GValue *value;
		if ((param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "offset")) == NULL)
		{
			if (priv->mutex)
				g_mutex_unlock (priv->mutex);
			return NULL;
		}

		value = gda_value_new (G_TYPE_INT);
		g_value_set_int (value, results_offset);
		gda_holder_set_value (param, value);
		gda_value_free (value);
	}
	
	if (other_parameters & DYN_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED_EXTRA_PAR_INCLUDE_KINDS_YES ||
		other_parameters & DYN_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED_EXTRA_PAR_INCLUDE_KINDS_NO)
	{	
		/* bad hardcode but it saves strings alloc/dealloc. */
		param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "filter0");
		if (filter_kinds->len > 0)
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 0));

		param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "filter1");
		if (filter_kinds->len > 1)
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 1));
		else
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 0));

		param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "filter2");
		if (filter_kinds->len > 2)
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 2));
		else
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 0));

		param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "filter3");
		if (filter_kinds->len > 3)
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 3));
		else
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 0));
		
		param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "filter4");
		if (filter_kinds->len > 4)
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 4));
		else
			gda_holder_set_value_str (param, NULL, g_ptr_array_index (filter_kinds, 0));		
	}

	if ((param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "globalsearch")) == NULL)
	{
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);
		return NULL;
	}

	value = gda_value_new (G_TYPE_INT);
	g_value_set_int (value, !global_symbols_search);	
	gda_holder_set_value (param, value);
	gda_value_free (value);

	
	if ((param = gda_set_get_holder ((GdaSet*)dyn_node->plist, "pattern")) == NULL)
	{
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);
		return NULL;
	}
	
	gda_holder_set_value_str (param, NULL, pattern);
	
/*	DEBUG_PRINT ("symbol_db_engine_find_symbol_by_name_pattern_filtered query: %s",
				 dyn_node->query_str);*/
		
	/* execute the query with parametes just set */
	data = gda_connection_statement_execute_select (priv->db_connection, 
												  (GdaStatement*)dyn_node->stmt, 
												  (GdaSet*)dyn_node->plist, NULL);
		
	if (!GDA_IS_DATA_MODEL (data) ||
		gda_data_model_get_n_rows (GDA_DATA_MODEL (data)) <= 0)
	{
		if (data != NULL)
			g_object_unref (data);
		
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);		
		return NULL;
	}

	if (priv->mutex)
		g_mutex_unlock (priv->mutex);
	return (SymbolDBEngineIterator *)symbol_db_engine_iterator_new (data, 
												priv->sym_type_conversion_hash);
}

