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

#include <libanjuta/interfaces/ianjuta-symbol-manager.h>
#include <libanjuta/interfaces/ianjuta-symbol.h>

/* file should be specified without the ".db" extension. */
#define ANJUTA_DB_FILE	".anjuta_sym_db"

/* if tables.sql changes or general db structure changes modify also the value here */
#define SYMBOL_DB_VERSION	"300.6"

#define TABLES_SQL			PACKAGE_DATA_DIR"/tables.sql"

#define CTAGS_MARKER	"#_#\n"

#define SHARED_MEMORY_PREFIX			SYMBOL_DB_SHM

#define THREADS_MAX_CONCURRENT			2
#define TRIGGER_SIGNALS_DELAY			100

#define MEMORY_POOL_STRING_SIZE			400
#define MEMORY_POOL_INT_SIZE			400

#define DUMMY_VOID_STRING				""
#define MP_VOID_STRING					"-"

#define BATCH_SYMBOL_NUMBER				15000

#define MP_RESET_OBJ_STR(gvalue) \
		g_value_set_static_string (gvalue, DUMMY_VOID_STRING);

/* given a GdaSet plist this macro resets all the GValues associated within its
 * GdaHolders.
 */
#define MP_RESET_PLIST(plist) { \
		if (plist != NULL) \
		{ \
			GSList* holders; \
			for (holders = plist->holders; holders; holders = holders->next) { \
				GValue *gvalue = (GValue*)gda_holder_get_value (holders->data); \
				if (G_VALUE_HOLDS_STRING(gvalue)) \
					MP_RESET_OBJ_STR(gvalue); \
			} \
		} \
}

#define MP_LEND_OBJ_STR(sdb_priv, OUT_gvalue) \
		OUT_gvalue = (GValue*)g_queue_pop_head(sdb_priv->mem_pool_string); \
		MP_RESET_OBJ_STR(OUT_gvalue);

#define MP_RETURN_OBJ_STR(sdb_priv, gvalue) \
		g_value_set_static_string (gvalue, MP_VOID_STRING); \
		g_queue_push_head(sdb_priv->mem_pool_string, gvalue); 

#define MP_LEND_OBJ_INT(sdb_priv, OUT_gvalue) \
		OUT_gvalue = (GValue*)g_queue_pop_head(sdb_priv->mem_pool_int); 

#define MP_RETURN_OBJ_INT(sdb_priv, gvalue) \
		g_queue_push_head(sdb_priv->mem_pool_int, gvalue);

/* ret_value, even if not used outside, permits variable reusing without 
 * forcing the compiler to redeclare it everytime
 */
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

#define SDB_LOCK(priv) if (priv->mutex) g_mutex_lock (priv->mutex);
#define SDB_UNLOCK(priv) if (priv->mutex) g_mutex_unlock (priv->mutex);

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
	PREP_QUERY_UPDATE_FILE_ANALYSE_TIME,
	PREP_QUERY_GET_ALL_FROM_FILE_WHERE_NOT_IN_SYMBOLS,
	PREP_QUERY_LANGUAGE_NEW,
	PREP_QUERY_GET_LANGUAGE_ID_BY_UNIQUE_NAME,
	PREP_QUERY_SYM_KIND_NEW,
	PREP_QUERY_GET_SYM_KIND_BY_UNIQUE_NAME,
	PREP_QUERY_SYM_ACCESS_NEW,
	PREP_QUERY_GET_SYM_ACCESS_BY_UNIQUE_NAME,
	PREP_QUERY_SYM_IMPLEMENTATION_NEW,
	PREP_QUERY_GET_SYM_IMPLEMENTATION_BY_UNIQUE_NAME,
	PREP_QUERY_HERITAGE_NEW,
	PREP_QUERY_SCOPE_NEW,
	PREP_QUERY_GET_SCOPE_ID,	
	PREP_QUERY_SYMBOL_NEW,
	PREP_QUERY_GET_SYMBOL_ID_BY_CLASS_NAME,
	PREP_QUERY_GET_SYMBOL_ID_BY_CLASS_NAME_AND_NAMESPACE,
	PREP_QUERY_UPDATE_SYMBOL_SCOPE_ID,
	PREP_QUERY_GET_SYMBOL_ID_BY_UNIQUE_INDEX_KEY_EXT,
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

typedef IAnjutaSymbolType SymType;

/* the SymbolDBEngine Private structure */
struct _SymbolDBEnginePriv
{
	gchar *anjuta_db_file;
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
	gint scanning;
	
	gchar *shared_mem_str;
	FILE *shared_mem_file;
	gint shared_mem_fd;
	AnjutaLauncher *ctags_launcher;
	GList *removed_launchers;
	gboolean shutting_down;
	gboolean is_first_population;
	gsize symbols_scanned_count;
	
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
	GHashTable *language_cache;

	/* Table maps */
	GQueue *tmp_heritage_tablemap;
	
	static_query_node *static_query_list[PREP_QUERY_COUNT]; 

	/* Memory pool */
	GQueue *mem_pool_string;
	GQueue *mem_pool_int;

#ifdef DEBUG
	GTimer *first_scan_timer_DEBUG;
#endif	
};

#endif
