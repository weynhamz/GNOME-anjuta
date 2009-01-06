/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Massimo Cora' 2007-2008 <maxcvs@email.it>
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

#ifndef _SYMBOL_DB_ENGINE_PRIV_H_
#define _SYMBOL_DB_ENGINE_PRIV_H_

#include <glib-object.h>
#include <glib.h>
#include <libanjuta/anjuta-launcher.h>
#include <libgda/libgda.h>
#include <sql-parser/gda-sql-parser.h>


/* file should be specified without the ".db" extension. */
#define ANJUTA_DB_FILE	".anjuta_sym_db"

/* if tables.sql changes or general db structure changes modify also the value here */
#define SYMBOL_DB_VERSION	"1.0"

#define TABLES_SQL	PACKAGE_DATA_DIR"/tables.sql"

#define CTAGS_MARKER	"#_#\n"

#define SHARED_MEMORY_PREFIX		"/dev/shm"

#define THREADS_MAX_CONCURRENT			2
#define TRIGGER_SIGNALS_DELAY			100

#define MEMORY_POOL_STRING_SIZE			100
#define MEMORY_POOL_INT_SIZE			100


#define USE_ASYNC_QUEUE
#undef USE_ASYNC_QUEUE


#ifdef USE_ASYNC_QUEUE
#define MP_LEND_OBJ_STR(sdb_priv, OUT_gvalue) \
		OUT_gvalue = (GValue*)g_async_queue_pop(sdb_priv->mem_pool_string); 
/*		DEBUG_PRINT ("lend str %p, qlength %d [-]", OUT_gvalue, g_async_queue_length (sdb_priv->mem_pool_string));*/

#define MP_RETURN_OBJ_STR(sdb_priv, gvalue) \
	g_async_queue_push(sdb_priv->mem_pool_string, gvalue); 
/*	DEBUG_PRINT ("return str %p, qlength %d [+]", gvalue, g_async_queue_length (sdb_priv->mem_pool_string));*/

#define MP_LEND_OBJ_INT(sdb_priv, OUT_gvalue) \
		OUT_gvalue = (GValue*)g_async_queue_pop(sdb_priv->mem_pool_int); 
/*		DEBUG_PRINT ("lend int, qlength %d [-]", g_async_queue_length (sdb_priv->mem_pool_int));*/

#define MP_RETURN_OBJ_INT(sdb_priv, gvalue) \
	g_async_queue_push(sdb_priv->mem_pool_int, gvalue); 
/*	DEBUG_PRINT ("return int, qlength %d [+]", g_async_queue_length (sdb_priv->mem_pool_int));*/
#else
#define MP_LEND_OBJ_STR(sdb_priv, OUT_gvalue) \
		OUT_gvalue = (GValue*)g_queue_pop_head(sdb_priv->mem_pool_string); 

#define MP_RETURN_OBJ_STR(sdb_priv, gvalue) \
	g_queue_push_head(sdb_priv->mem_pool_string, gvalue); 

#define MP_LEND_OBJ_INT(sdb_priv, OUT_gvalue) \
		OUT_gvalue = (GValue*)g_queue_pop_head(sdb_priv->mem_pool_int); 

#define MP_RETURN_OBJ_INT(sdb_priv, gvalue) \
	g_queue_push_head(sdb_priv->mem_pool_int, gvalue);
#endif

#define MP_SET_HOLDER_BATCH_STR(priv, param, string_, ret_bool, ret_value) { \
	GValue *value_str; \
	MP_LEND_OBJ_STR(priv, value_str); \
	g_value_set_static_string (value_str, string_); \
	ret_value = gda_holder_take_static_value (param, value_str, &ret_bool, NULL); \
	if (ret_value != NULL && G_VALUE_HOLDS_STRING (ret_value) == TRUE) \
	{ \
		MP_RETURN_OBJ_STR(priv, ret_value); \
	} \
}

#define MP_SET_HOLDER_BATCH_INT(priv, param, int_, ret_bool, ret_value) { \
	GValue *value_int; \
	MP_LEND_OBJ_INT(priv, value_int); \
	g_value_set_int (value_int, int_); \
	ret_value = gda_holder_take_static_value (param, value_int, &ret_bool, NULL); \
	if (ret_value != NULL && G_VALUE_HOLDS_INT (ret_value) == TRUE) \
	{ \
		MP_RETURN_OBJ_INT(priv, ret_value); \
	} \
}


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
	PREP_QUERY_UPDATE_SYMBOL_SCOPE_ID_MIXED,
	PREP_QUERY_GET_SYMBOL_ID_BY_UNIQUE_INDEX_KEY,
	PREP_QUERY_GET_SYMBOL_ID_BY_UNIQUE_INDEX_KEY_EXT,
	PREP_QUERY_GET_SYMBOL_ID_BY_UNIQUE_INDEX_KEY_EXT2,
	PREP_QUERY_UPDATE_SYMBOL_ALL,
	PREP_QUERY_REMOVE_NON_UPDATED_SYMBOLS,
	PREP_QUERY_RESET_UPDATE_FLAG_SYMBOLS,
	PREP_QUERY_GET_REMOVED_IDS,
	PREP_QUERY_TMP_REMOVED_DELETE_ALL,
	PREP_QUERY_REMOVE_FILE_BY_PROJECT_NAME,
	PREP_QUERY_COUNT
		
} static_query_type;

typedef struct _static_query_node
{
	static_query_type query_id;
	const gchar *query_str;
	GdaStatement *stmt;
	GdaSet *plist;

} static_query_node;

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
	DYN_PREP_QUERY_GET_FILES_FOR_PROJECT,
	DYN_PREP_QUERY_COUNT
		
} dyn_query_type;

/**
 * dyn_query_node's possible structures
 *
 *           sym_extra_info_gtree          with has_gtree_child = FALSE
 *                   |
 *       ... +-------+-------+ ...
 *           |       |       |    <========  keys = sym_info
 *         CDQN    CDQN    CDQN              values = ChildDynQueryNode
 *
 *
 *
 *           sym_extra_info_gtree          with has_gtree_child = TRUE
 *                   |
 *       ... +-------+-------+ ...
 *           |       |       |    <========  keys = sym_info, values = GTree
 *         GTree    GTree   GTree            
 *         / | \     |       |    <========  keys = other_parameters, values = ChildDynQueryNode
 *        /  |  \   ...     ...   
 *     CDQN CDQN CDQN
 *
 *
 */
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


/* WARNING: these must match the ones on libanjuta.idl [AnjutaSymbol::Field] */
typedef enum {
	SYMINFO_SIMPLE = 1,
	SYMINFO_FILE_PATH = 2,
	SYMINFO_IMPLEMENTATION = 4,
	SYMINFO_ACCESS = 8,
	SYMINFO_KIND = 16,
	SYMINFO_TYPE = 32,
	SYMINFO_TYPE_NAME = 64,
	SYMINFO_LANGUAGE = 128,
	SYMINFO_FILE_IGNORE = 256,
	SYMINFO_FILE_INCLUDE = 512,
	SYMINFO_PROJECT_NAME = 1024,
	SYMINFO_WORKSPACE_NAME = 2048
	
} SymExtraInfo;


/* the SymbolDBEngine Private structure */
struct _SymbolDBEnginePriv
{
	gchar *ctags_path;
	
	GdaConnection *db_connection;
	GdaSqlParser *sql_parser;
	gchar *db_directory;
	gchar *project_directory;
	gchar *cnc_string;

	gint scan_process_id;
	GAsyncQueue *scan_process_id_queue;
	
	GAsyncQueue *scan_queue;	
	GAsyncQueue *updated_symbols_id;
	GAsyncQueue *updated_scope_symbols_id;
	GAsyncQueue *inserted_symbols_id;
	
	gchar *shared_mem_str;
	FILE *shared_mem_file;
	gint shared_mem_fd;
	AnjutaLauncher *ctags_launcher;
	GList *removed_launchers;
	gboolean shutting_down;
	
	GMutex* mutex;
	GAsyncQueue* signals_queue;
	
	GThreadPool *thread_pool;
	
	gint timeout_trigger_handler;
	
	gint trigger_closure_retries;
	gint thread_closure_retries;
	
	GHashTable *sym_type_conversion_hash;
	GHashTable *garbage_shared_mem_files;
	
	/* Caches */
	GHashTable *kind_cache;
	GHashTable *access_cache;
	GHashTable *implementation_cache;	
	
	GTree *file_symbols_cache;
	
	static_query_node *static_query_list[PREP_QUERY_COUNT]; 
	dyn_query_node *dyn_query_list[DYN_PREP_QUERY_COUNT];
	
#ifdef USE_ASYNC_QUEUE	
	GAsyncQueue *mem_pool_string;
	GAsyncQueue *mem_pool_int;
#else
	GQueue *mem_pool_string;
	GQueue *mem_pool_int;
#endif
};

#endif
