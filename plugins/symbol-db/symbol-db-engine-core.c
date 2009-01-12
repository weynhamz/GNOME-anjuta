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

* OR * 
		
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

#include <gio/gio.h>
#include <libanjuta/interfaces/ianjuta-symbol.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-launcher.h>
#include <libanjuta/anjuta-utils.h>
#include <libgda/libgda.h>
#include <sql-parser/gda-sql-parser.h>
#include "readtags.h"
#include "symbol-db-engine-priv.h"
#include "symbol-db-engine-core.h"
#include "symbol-db-engine-iterator.h"
#include "symbol-db-engine-utils.h"


/*
 * utility macros
 */
#define STATIC_QUERY_POPULATE_INIT_NODE(query_list_ptr, query_type, gda_stmt) { \
	static_query_node *q = g_new0 (static_query_node, 1); \
	q->query_id = query_type; \
	q->query_str = gda_stmt; \
	q->stmt = NULL; \
	q->plist = NULL; \
	query_list_ptr [query_type] = q; \
}

#define DYN_QUERY_POPULATE_INIT_NODE(dquery_list_ptr, dquery_type, gtree_child) { \
	dyn_query_node *dq = g_new0 (dyn_query_node, 1); \
	dq->dyn_query_id = dquery_type; \
	dq->sym_extra_info_gtree = NULL; \
	dq->has_gtree_child = gtree_child; \
	dquery_list_ptr [dquery_type] = dq; \
}


typedef void (SymbolDBEngineCallback) (SymbolDBEngine * dbe,
									   gpointer user_data);

/* 
 * signals 
 */
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

/*
 * enums
 */
enum {
	DO_UPDATE_SYMS = 1,
	DO_UPDATE_SYMS_AND_EXIT,
	DONT_UPDATE_SYMS,
	DONT_UPDATE_SYMS_AND_EXIT,
	DONT_FAKE_UPDATE_SYMS,
	END_UPDATE_GROUP_SYMS
};

/*
 * structs used for callback data.
 */
typedef struct _UpdateFileSymbolsData {	
	gchar *project;
	gboolean update_prj_analyse_time;
	GPtrArray * files_path;
	
} UpdateFileSymbolsData;

typedef struct _ScanFiles1Data {
	SymbolDBEngine *dbe;
	
	gchar *real_file;	/* may be NULL. If not NULL must be freed */
	gint partial_count;
	gint files_list_len;
	gint symbols_update;
	
} ScanFiles1Data;

/*
 * global file variables
 */ 
static GObjectClass *parent_class = NULL;

/*
 * forward declarations 
 */
static void 
sdb_engine_second_pass_do (SymbolDBEngine * dbe);

static gint
sdb_engine_add_new_symbol (SymbolDBEngine * dbe, const tagEntry * tag_entry,
						   int file_defined_id, gboolean sym_update);

inline const GdaStatement *
sdb_engine_get_statement_by_query_id (SymbolDBEngine * dbe, static_query_type query_id);

inline const GdaSet *
sdb_engine_get_query_parameters_list (SymbolDBEngine *dbe, static_query_type query_id);

inline const DynChildQueryNode *
sdb_engine_get_dyn_query_node_by_id (SymbolDBEngine *dbe, dyn_query_type query_id,
									 SymExtraInfo sym_info, gsize other_parameters);

inline const DynChildQueryNode *
sdb_engine_insert_dyn_query_node_by_id (SymbolDBEngine *dbe, dyn_query_type query_id,
									 	SymExtraInfo sym_info, gsize other_parameters,
										const gchar *sql);

inline gint
sdb_engine_get_tuple_id_by_unique_name (SymbolDBEngine * dbe, static_query_type qtype,
										gchar * param_key,
										GValue * param_value);

/*
 * implementation starts here 
 */
static inline gint
sdb_engine_cache_lookup (GHashTable* hash_table, const gchar* lookup)
{
	gpointer orig_key = NULL;
	gpointer value = NULL;
	
	/* avoiding lazy initialization may gain some cpu cycles. Just lookup here. */	
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

#if 0
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
#endif

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
inline const GdaStatement *
sdb_engine_get_statement_by_query_id (SymbolDBEngine * dbe, static_query_type query_id)
{
	static_query_node *node;
	SymbolDBEnginePriv *priv;

	priv = dbe->priv;

	/* no way: if connection is NULL we will break here. There must be
	 * a connection established to db before using this function */
	/*g_return_val_if_fail (priv->db_connection != NULL, NULL);*/
	
	if ((node = priv->static_query_list[query_id]) == NULL)
		return NULL;

	if (node->stmt == NULL)
	{
		/*DEBUG_PRINT ("generating new statement.. %d", query_id);*/
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


/**
 *
 */
inline const DynChildQueryNode *
sdb_engine_get_dyn_query_node_by_id (SymbolDBEngine *dbe, dyn_query_type query_id,
									 SymExtraInfo sym_info, gsize other_parameters)
{
	dyn_query_node *node;
	SymbolDBEnginePriv *priv;

	priv = dbe->priv;

	node = priv->dyn_query_list[query_id];

	if (node == NULL || node->sym_extra_info_gtree == NULL) 
	{
		/* we didn't find any extra info symbol, nor it has been added before */
		return NULL;
	}
	
	if (node->has_gtree_child == FALSE) 
	{
		/* use only sym_info as key, ignore other_parameters */
		return g_tree_lookup (node->sym_extra_info_gtree, GINT_TO_POINTER (sym_info));
	}
	else {
		GTree *child_gtree;
		DynChildQueryNode *result;
		
		child_gtree = g_tree_lookup (node->sym_extra_info_gtree, GINT_TO_POINTER (sym_info));
		if (child_gtree == NULL) 
		{			
			return NULL;
		}
		
		result = g_tree_lookup (child_gtree, GINT_TO_POINTER (other_parameters));
		return result;
	}
}

static void
sdb_engine_dyn_child_query_node_destroy (gpointer data)
{
	DynChildQueryNode *node_to_destroy;

	node_to_destroy = (DynChildQueryNode *)data;

	g_free (node_to_destroy->query_str);
	if (node_to_destroy->stmt)
		g_object_unref (node_to_destroy->stmt);
	if (node_to_destroy->plist)
		g_object_unref (node_to_destroy->plist);
	g_free (node_to_destroy);
}

/**
 * WARNING: We suppose that before calling this function we already checked with
 * sdb_engine_get_dyn_query_node_by_id () that the node we're going to add 
 * isn't yet inserted.
 */
inline const DynChildQueryNode *
sdb_engine_insert_dyn_query_node_by_id (SymbolDBEngine *dbe, dyn_query_type query_id,
									 	SymExtraInfo sym_info, gsize other_parameters,
										const gchar *sql)
{
	dyn_query_node *node;
	SymbolDBEnginePriv *priv;
	priv = dbe->priv;
	
	/* no way: if connection is NULL we will break here. There must be
	 * a connection established to db before using this function */
	g_return_val_if_fail (priv->db_connection != NULL, NULL);

	node = priv->dyn_query_list[query_id];
	
	g_return_val_if_fail (node != NULL, NULL);
	
	if (node->sym_extra_info_gtree == NULL) 
	{
		/* lazy initialization */
		if (node->has_gtree_child == FALSE)
		{
			node->sym_extra_info_gtree = 
					g_tree_new_full ((GCompareDataFunc)&symbol_db_gtree_compare_func, 
										 NULL,
										 NULL,
										 sdb_engine_dyn_child_query_node_destroy);
		}
		else
		{
			node->sym_extra_info_gtree = 
					g_tree_new_full ((GCompareDataFunc)&symbol_db_gtree_compare_func, 
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
		dyn_node->plist = NULL;
		dyn_node->stmt =
			gda_sql_parser_parse_string (priv->sql_parser, sql, NULL, 
										 NULL);

		if (gda_statement_get_parameters ((GdaStatement*)dyn_node->stmt, 
										  &dyn_node->plist, NULL) == FALSE)
		{
			g_warning ("Error on getting parameters for dyn %d", query_id);
		}
		
		dyn_node->query_str = g_strdup (sql);
				
		/* insert it into gtree, thanks */
		g_tree_insert (node->sym_extra_info_gtree, GINT_TO_POINTER (sym_info), dyn_node);
		
		/* return it */
		return dyn_node;
	}
	else
	{
		/* ok, this is a slightly more complex case */
		GTree *child_gtree;
		DynChildQueryNode *dyn_node;

		child_gtree = g_tree_new_full ((GCompareDataFunc)&symbol_db_gtree_compare_func,
									 NULL,
									 NULL, 
									 sdb_engine_dyn_child_query_node_destroy); 
				
		dyn_node = g_new0 (DynChildQueryNode, 1);
		
		/* create a new GdaStatement */
		dyn_node->plist = NULL;
		dyn_node->stmt =
			gda_sql_parser_parse_string (priv->sql_parser, sql, NULL, 
										 NULL);

		if (gda_statement_get_parameters ((GdaStatement*)dyn_node->stmt, 
										  &dyn_node->plist, NULL) == FALSE)
		{
			g_warning ("Error on getting parameters for dyn %d", query_id);
		}
		
		dyn_node->query_str = g_strdup (sql);
		
/*		
		DEBUG_PRINT ("inserting child into main_gtree's gtree child, "
					 "other_parameters %d, sym_info %d, dyn_node %p", 
					 other_parameters, sym_info, dyn_node);
*/		
		/* insert the dyn_node into child_gtree, then child_gtree into main_gtree */
		g_tree_insert (child_gtree, GINT_TO_POINTER (other_parameters), dyn_node);
		
		g_tree_insert (node->sym_extra_info_gtree, GINT_TO_POINTER (sym_info), child_gtree);
		
		/* return it */
		return dyn_node;		
	}	
}

/**
 * Return a GdaSet of parameters calculated from the statement. It does not check
 * if it's null. You *must* be sure to have called sdb_engine_get_statement_by_query_id () first.
 */
inline const GdaSet *
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
			/*DEBUG_PRINT ("sdb_engine_free_cached_queries stmt %d", i);*/
			g_object_unref (node->stmt);
			node->stmt = NULL;
		}
		
		if (node != NULL && node->plist != NULL)
		{
			/*DEBUG_PRINT ("sdb_engine_free_cached_queries plist %d", i);*/
			g_object_unref (node->plist);
			node->plist = NULL;
		}
		
		/* last but not the least free the node itself */
		g_free (node);
		priv->static_query_list[i] = NULL;
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
			g_tree_destroy (node->sym_extra_info_gtree);			
			node->sym_extra_info_gtree = NULL;
		}		
		
		/* last but not the least free the node itself */
		g_free (node);
		priv->dyn_query_list[i] = NULL;		
	}
}

static gboolean
sdb_engine_disconnect_from_db (SymbolDBEngine * dbe)
{
	SymbolDBEnginePriv *priv;

	g_return_val_if_fail (dbe != NULL, FALSE);
	priv = dbe->priv;

	DEBUG_PRINT ("Disconnecting from %s", priv->cnc_string);
	g_free (priv->cnc_string);
	priv->cnc_string = NULL;

	if (priv->db_connection != NULL)
		gda_connection_close (priv->db_connection);
	priv->db_connection = NULL;

	if (priv->sql_parser != NULL)
		g_object_unref (priv->sql_parser);
	priv->sql_parser = NULL;
	
	g_free (priv->db_directory);
	priv->db_directory = NULL;
	
	g_free (priv->project_directory);
	priv->project_directory = NULL;
	
	return TRUE;
}

/**
 * @return -1 on error. Otherwise the id of tuple.
 */
inline gint
sdb_engine_get_tuple_id_by_unique_name (SymbolDBEngine * dbe, static_query_type qtype,
										gchar * param_key,
										GValue * param_value)
{
	const GdaSet *plist;
	const GdaStatement *stmt;
	GdaHolder *param;
	GdaDataModel *data_model;
	const GValue *num;
	gint table_id;
	SymbolDBEnginePriv *priv;
	GValue *ret_value;	
	gboolean ret_bool;
	
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

	ret_value = gda_holder_take_static_value (param, param_value, &ret_bool, NULL);	
	if (G_VALUE_HOLDS_STRING (param_value) == TRUE)
	{
		if (ret_value != NULL)
			MP_RETURN_OBJ_STR(priv, ret_value);		
	}
	else
	{
		if (ret_value != NULL)
			MP_RETURN_OBJ_INT(priv, ret_value);		
	}	
	
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
	num = gda_data_model_get_value_at (GDA_DATA_MODEL (data_model), 0, 0, NULL);

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
static inline gint
sdb_engine_get_tuple_id_by_unique_name2 (SymbolDBEngine * dbe, 
										 static_query_type qtype,
										 gchar * param_key1,
										 GValue * value1,
										 gchar * param_key2,
										 GValue * value2)
{
	const GdaSet *plist;
	const GdaStatement *stmt;
	GdaHolder *param;
	GdaDataModel *data_model;
	const GValue *num;
	gint table_id;
	SymbolDBEnginePriv *priv;
	gboolean ret_bool;
	GValue *ret_value;
	
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
	
	ret_value = gda_holder_take_static_value (param, value1, &ret_bool, NULL);	
	if (G_VALUE_HOLDS_STRING (value1) == TRUE)
	{
		if (ret_value != NULL)
			MP_RETURN_OBJ_STR(priv, ret_value);
	}
	else
	{
		if (ret_value != NULL)
			MP_RETURN_OBJ_INT(priv, ret_value);		
	}	
		
	
	/* ...and the second one */
	if ((param = gda_set_get_holder ((GdaSet*)plist, param_key2)) == NULL)
	{
		g_warning ("sdb_engine_get_tuple_id_by_unique_name2: "
				   "param is NULL from pquery! [par1: %s] [par2: %s]\n",
				   param_key1, param_key2);
		return -1;
	}
	
	ret_value = gda_holder_take_static_value (param, value2, &ret_bool, NULL);	
	if (G_VALUE_HOLDS_STRING (value2) == TRUE)
	{
		if (ret_value != NULL)
			MP_RETURN_OBJ_STR(priv, ret_value);
	}
	else
	{
		if (ret_value != NULL)
			MP_RETURN_OBJ_INT(priv, ret_value);		
	}	

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
	num = gda_data_model_get_value_at (GDA_DATA_MODEL (data_model), 0, 0, NULL);

	table_id = g_value_get_int (num);
	g_object_unref (data_model);
	
	return table_id;
}

static inline gint
sdb_engine_get_tuple_id_by_unique_name3 (SymbolDBEngine * dbe, 
										 static_query_type qtype,
										 gchar * param_key1,
										 GValue * value1,
										 gchar * param_key2,
										 GValue * value2,
										 gchar * param_key3,
										 GValue * value3)
{
	const GdaSet *plist;
	const GdaStatement *stmt;
	GdaHolder *param;
	GdaDataModel *data_model;
	const GValue *num;
	gint table_id;
	SymbolDBEnginePriv *priv;
	gboolean ret_bool;
	GValue *ret_value;
	
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
	
	ret_value = gda_holder_take_static_value (param, value1, &ret_bool, NULL);	
	if (G_VALUE_HOLDS_STRING (value1) == TRUE)
	{
		if (ret_value != NULL)
			MP_RETURN_OBJ_STR(priv, ret_value);
	}
	else
	{
		if (ret_value != NULL)
			MP_RETURN_OBJ_INT(priv, ret_value);		
	}		

	
	/* ...and the second one */
	if ((param = gda_set_get_holder ((GdaSet*)plist, param_key2)) == NULL)
	{
		g_warning ("sdb_engine_get_tuple_id_by_unique_name2: "
				   "param is NULL from pquery! [par1: %s] [par2: %s]\n",
				   param_key1, param_key2);
		return -1;
	}
	
	ret_value = gda_holder_take_static_value (param, value2, &ret_bool, NULL);	
	if (G_VALUE_HOLDS_STRING (value2) == TRUE)
	{
		if (ret_value != NULL)
			MP_RETURN_OBJ_STR(priv, ret_value);
	}
	else
	{
		if (ret_value != NULL)
			MP_RETURN_OBJ_INT(priv, ret_value);		
	}	


	/* ...and the third one */
	if ((param = gda_set_get_holder ((GdaSet*)plist, param_key3)) == NULL)
	{
		g_warning ("sdb_engine_get_tuple_id_by_unique_name2: "
				   "param is NULL from pquery! [par1: %s] [par2: %s]\n",
				   param_key1, param_key3);
		return -1;
	}
	
	ret_value = gda_holder_take_static_value (param, value3, &ret_bool, NULL);	
	if (G_VALUE_HOLDS_STRING (value3) == TRUE)
	{
		if (ret_value != NULL)
			MP_RETURN_OBJ_STR(priv, ret_value);
	}
	else
	{
		if (ret_value != NULL)
			MP_RETURN_OBJ_INT(priv, ret_value);		
	}	

		
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
	num = gda_data_model_get_value_at (GDA_DATA_MODEL (data_model), 0, 0, NULL);

	table_id = g_value_get_int (num);
	g_object_unref (data_model);
	return table_id;
}

static inline gint
sdb_engine_get_tuple_id_by_unique_name4 (SymbolDBEngine * dbe, 
										 static_query_type qtype,
										 gchar * param_key1,
										 GValue * value1,
										 gchar * param_key2,
										 GValue * value2,
										 gchar * param_key3,
										 GValue * value3,
										 gchar * param_key4,
										 GValue * value4)
{
	const GdaSet *plist;
	const GdaStatement *stmt;
	GdaHolder *param;
	GdaDataModel *data_model;
	const GValue *num;
	gint table_id;
	SymbolDBEnginePriv *priv;
	gboolean ret_bool;
	GValue *ret_value;
	
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
	
	ret_value = gda_holder_take_static_value (param, value1, &ret_bool, NULL);	
	if (G_VALUE_HOLDS_STRING (value1) == TRUE)
	{
		if (ret_value != NULL)
			MP_RETURN_OBJ_STR(priv, ret_value);
	}
	else
	{
		if (ret_value != NULL)
			MP_RETURN_OBJ_INT(priv, ret_value);		
	}		

	

	/* ...and the second one */
	if ((param = gda_set_get_holder ((GdaSet*)plist, param_key2)) == NULL)
	{
		g_warning ("sdb_engine_get_tuple_id_by_unique_name2: "
				   "param is NULL from pquery!");
		return -1;
	}
	
	ret_value = gda_holder_take_static_value (param, value2, &ret_bool, NULL);	
	if (G_VALUE_HOLDS_STRING (value2) == TRUE)
	{
		if (ret_value != NULL)
			MP_RETURN_OBJ_STR(priv, ret_value);
	}
	else
	{
		if (ret_value != NULL)
			MP_RETURN_OBJ_INT(priv, ret_value);		
	}		


	/* ...and the third one */
	if ((param = gda_set_get_holder ((GdaSet*)plist, param_key3)) == NULL)
	{
		g_warning ("sdb_engine_get_tuple_id_by_unique_name2: "
				   "param is NULL from pquery!");
		return -1;
	}
	
	ret_value = gda_holder_take_static_value (param, value3, &ret_bool, NULL);	
	if (G_VALUE_HOLDS_STRING (value3) == TRUE)
	{
		if (ret_value != NULL)
			MP_RETURN_OBJ_STR(priv, ret_value);
	}
	else
	{
		if (ret_value != NULL)
			MP_RETURN_OBJ_INT(priv, ret_value);		
	}		

		
	/* ...and the fourth one */
	if ((param = gda_set_get_holder ((GdaSet*)plist, param_key4)) == NULL)
	{
		g_warning ("sdb_engine_get_tuple_id_by_unique_name2: "
				   "param is NULL from pquery!");
		return -1;
	}

	ret_value = gda_holder_take_static_value (param, value4, &ret_bool, NULL);	
	if (G_VALUE_HOLDS_STRING (value4) == TRUE)
	{
		if (ret_value != NULL)
			MP_RETURN_OBJ_STR(priv, ret_value);
	}
	else
	{
		if (ret_value != NULL)
			MP_RETURN_OBJ_INT(priv, ret_value);		
	}		
	
			
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
	num = gda_data_model_get_value_at (GDA_DATA_MODEL (data_model), 0, 0, NULL);

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
	SymbolDBEnginePriv *priv;
	GValue* value;
	
	priv = dbe->priv;	

	MP_LEND_OBJ_STR(priv, value);
	
	gint file_defined_id = 0;
	if (base_prj_path != NULL && g_str_has_prefix (tag_entry->file, base_prj_path))
	{
		/* in this case fake_file will be ignored. */
		
		/* we expect here an absolute path */
		g_value_set_static_string (value,
							tag_entry->file + strlen (base_prj_path) );
	}
	else
	{
		/* check whether the fake_file can substitute the tag_entry->file one */
		if (fake_file_on_db == NULL)
			g_value_set_static_string (value, tag_entry->file);
		else
			g_value_set_static_string (value, fake_file_on_db);
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
		return -1;
	}

	return file_defined_id;
}

static
gboolean sdb_engine_udpated_scope_gtree_populate (gpointer key,
                                                  gpointer value,
                                                  gpointer data)
{
	SymbolDBEnginePriv *priv;
	SymbolDBEngine * dbe = data;
	priv = dbe->priv;

	GList *list;
	list = value;
	if (g_list_length  (list) > 1)
	{
		GList *item = list;
		while (item != NULL)
		{
			g_async_queue_push (priv->updated_scope_symbols_id, 
								(gpointer) item->data);
			item = item->next;
		}
	}
	
	/* proceed with a freeing of the values */
	g_list_free (list);
	return FALSE;
}

static GTimer *sym_timer_DEBUG  = NULL;
static gint tags_total_DEBUG = 0;
static gdouble elapsed_total_DEBUG = 0;

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
	
	if ((tag_file = tagsOpen_1 (fd, &tag_file_info)) == NULL)
	{
		g_warning ("error in opening ctags file");
	}

	if (sym_timer_DEBUG == NULL)
		sym_timer_DEBUG = g_timer_new ();
	else
		g_timer_reset (sym_timer_DEBUG);
	gint tags_num_DEBUG = 0;
	tag_entry.file = NULL;

	while (tagsNext (tag_file, &tag_entry) != TagFailure)
	{
		gint file_defined_id = 0;
		if (tag_entry.file == NULL)
		{
			continue;
		}
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
			tag_entry_file_cache = g_strdup (tag_entry.file);
		}
		/* insert or update a symbol */
		sdb_engine_add_new_symbol (dbe, &tag_entry, file_defined_id,
								   force_sym_update);
		tags_num_DEBUG++;
		tag_entry.file = NULL;
	}
	g_free (tag_entry_file_cache);

	if (force_sym_update == TRUE)
	{
	
		/* any scope_updated symbols to the releative queue */
		g_tree_foreach (priv->file_symbols_cache, 
					sdb_engine_udpated_scope_gtree_populate, dbe);
		/* destroying the tree and recreating it should be faster than removing each
		 * balanced item inside it */
		g_tree_destroy (priv->file_symbols_cache);
		priv->file_symbols_cache = g_tree_new_full ((GCompareDataFunc)&symbol_db_gtree_compare_func, 
										 NULL,
										 NULL,
										 NULL);
	}

	gdouble elapsed_DEBUG = g_timer_elapsed (sym_timer_DEBUG, NULL);
	tags_total_DEBUG += tags_num_DEBUG;
	elapsed_total_DEBUG += elapsed_DEBUG;
	DEBUG_PRINT ("elapsed: %f for (%d) [%f sec/symbol] [av %f sec/symbol]", elapsed_DEBUG,
				 tags_num_DEBUG, elapsed_DEBUG / tags_num_DEBUG, 
				 elapsed_total_DEBUG / tags_total_DEBUG);

	/* notify listeners that another file has been scanned */
	g_async_queue_push (priv->signals_queue, GINT_TO_POINTER (SINGLE_FILE_SCAN_END +1));
	
	/* we've done with tag_file but we don't need to tagsClose (tag_file); */
}

static void
sdb_engine_ctags_output_thread (gpointer data, gpointer user_data)
{
	gint len_chars;
	gchar *chars, *chars_ptr;
	gint remaining_chars;
	gint len_marker;
	SymbolDBEnginePriv *priv;
	SymbolDBEngine *dbe;
	
	chars = chars_ptr = (gchar *)data;
	dbe = SYMBOL_DB_ENGINE (user_data);
	
	g_return_if_fail (dbe != NULL);	
	g_return_if_fail (chars_ptr != NULL);

	priv = dbe->priv;

	/* lock */
	if (priv->mutex)
		g_mutex_lock (priv->mutex);
	
	remaining_chars = len_chars = strlen (chars_ptr);
	len_marker = strlen (CTAGS_MARKER);	

	if (len_chars >= len_marker) 
	{
		gchar *marker_ptr = NULL;
		gint tmp_str_length = 0;

		/* is it an end file marker? */
		marker_ptr = strstr (chars_ptr, CTAGS_MARKER);

		do  
		{
			if (marker_ptr != NULL) 
			{
				int scan_flag;
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
				scan_flag = GPOINTER_TO_INT(g_async_queue_try_pop (priv->scan_queue));
				real_file = g_async_queue_try_pop (priv->scan_queue);
				
				/* and now call the populating function */
				if (scan_flag == DO_UPDATE_SYMS ||
					scan_flag == DO_UPDATE_SYMS_AND_EXIT)
				{
					sdb_engine_populate_db_by_tags (dbe, priv->shared_mem_file,
								(gsize)real_file == DONT_FAKE_UPDATE_SYMS ? NULL : real_file, 
								TRUE);
				}
				else 
				{
					sdb_engine_populate_db_by_tags (dbe, priv->shared_mem_file,
								(gsize)real_file == DONT_FAKE_UPDATE_SYMS ? NULL : real_file, 
								FALSE);					
				}
				
				/* don't forget to free the real_file, if it's a char */
				if ((gsize)real_file != DONT_FAKE_UPDATE_SYMS)
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
					DEBUG_PRINT ("%s", "FOUND end-of-group-files marker."
								 "go on with sdb_engine_second_pass_do ()");
					
					chars_ptr += len_marker;
					remaining_chars -= len_marker;
					
					/* will emit symbol_scope_updated */
					sdb_engine_second_pass_do (dbe);					
					
					/* Here we are. It's the right time to notify the listeners
					 * about out fresh new inserted/updated symbols...
					 * Go on by emitting them.
					 */
					while ((tmp_inserted = GPOINTER_TO_INT(
							g_async_queue_try_pop (priv->inserted_symbols_id))) > 0)
					{
						/* we must be sure to insert both signals at once */
						g_async_queue_lock (priv->signals_queue);
						
						/* the +1 is because asyn_queue doesn't want NULL values */
						g_async_queue_push_unlocked (priv->signals_queue, 
													 GINT_TO_POINTER(SYMBOL_INSERTED + 1));
						g_async_queue_push_unlocked (priv->signals_queue, 
													 GINT_TO_POINTER(tmp_inserted));
						g_async_queue_unlock (priv->signals_queue);
					}
					
					while ((tmp_updated = GPOINTER_TO_INT(
							g_async_queue_try_pop (priv->updated_symbols_id))) > 0)
					{
						g_async_queue_lock (priv->signals_queue);
						g_async_queue_push_unlocked (priv->signals_queue, GINT_TO_POINTER
													 (SYMBOL_UPDATED + 1));
						g_async_queue_push_unlocked (priv->signals_queue, 
													 GINT_TO_POINTER(tmp_updated));
						g_async_queue_unlock (priv->signals_queue);
					}

					while ((tmp_updated = GPOINTER_TO_INT(
							g_async_queue_try_pop (priv->updated_scope_symbols_id))) > 0)
					{
						g_async_queue_lock (priv->signals_queue);
						g_async_queue_push_unlocked (priv->signals_queue, GINT_TO_POINTER (
													 SYMBOL_SCOPE_UPDATED + 1));
						g_async_queue_push_unlocked (priv->signals_queue, 
													GINT_TO_POINTER(tmp_updated));
						g_async_queue_unlock (priv->signals_queue);
					}
					
					
					/* emit signal. The end of files-group can be cannot be
					 * determined by the caller. This is the only way.
					 */
					DEBUG_PRINT ("%s", "EMITTING scan-end");
					g_async_queue_push (priv->signals_queue, GINT_TO_POINTER(SCAN_END + 1));
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
		DEBUG_PRINT ("%s", "no len_chars > len_marker");
	}
	
	/* unlock */
	if (priv->mutex)
		g_mutex_unlock (priv->mutex);
	
	g_free (chars);
}


/**
 * This function runs on the main glib thread, so that it can safely spread signals 
 */
static gboolean
sdb_engine_timeout_trigger_signals (gpointer user_data)
{
	SymbolDBEngine *dbe = (SymbolDBEngine *) user_data;
	SymbolDBEnginePriv *priv;

	g_return_val_if_fail (user_data != NULL, FALSE);	
	priv = dbe->priv;		

	if (g_async_queue_length (priv->signals_queue) > 0)
	{
		gpointer tmp;
		gpointer sign = NULL;
		gsize real_signal;
	
		while ((sign = g_async_queue_try_pop (priv->signals_queue)) != NULL)  
		{
			if (sign == NULL) 
			{
				return g_async_queue_length (priv->signals_queue) > 0 ? TRUE : FALSE;
			}
	
			real_signal = (gsize)sign -1;
	
			switch (real_signal) 
			{
				case SINGLE_FILE_SCAN_END:
					g_signal_emit (dbe, signals[SINGLE_FILE_SCAN_END], 0);
					break;
		
				case SCAN_END:
				{
					/* get the process id from the queue */
					gint int_tmp = GPOINTER_TO_INT(g_async_queue_pop (priv->scan_process_id_queue));
					g_signal_emit (dbe, signals[SCAN_END], 0, int_tmp);
				}
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
	
	if (g_thread_pool_unprocessed (priv->thread_pool) == 0 &&
		g_thread_pool_get_num_threads (priv->thread_pool) == 0)
	{
		/*DEBUG_PRINT ("%s", "removing signals trigger");*/
		/* remove the trigger coz we don't need it anymore... */
		g_source_remove (priv->timeout_trigger_handler);
		priv->timeout_trigger_handler = 0;
		return FALSE;
	}
	
	return TRUE;
}

static void
sdb_engine_ctags_output_callback_1 (AnjutaLauncher * launcher,
								  AnjutaLauncherOutputType output_type,
								  const gchar * chars, gpointer user_data)
{
	SymbolDBEngine *dbe = (SymbolDBEngine *) user_data;
	SymbolDBEnginePriv *priv;

	g_return_if_fail (user_data != NULL);
	
	priv = dbe->priv;	
	
	if (priv->shutting_down == TRUE)
		return;

	g_thread_pool_push (priv->thread_pool, g_strdup (chars), NULL);
	
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
				   gpointer user_data)
{
	SymbolDBEngine *dbe = (SymbolDBEngine *) user_data;
	SymbolDBEnginePriv *priv;

	g_return_if_fail (user_data != NULL);
	
	priv = dbe->priv;	
	
	DEBUG_PRINT ("***** ctags ended *****");
	
	if (priv->shutting_down == TRUE)
		return;

	priv->ctags_path = NULL;
}

static void
sdb_engine_ctags_launcher_create (SymbolDBEngine * dbe)
{
	SymbolDBEnginePriv *priv;
	gchar *exe_string;
		
	priv = dbe->priv;
	
	DEBUG_PRINT ("%s", "creating anjuta_launcher");

	priv->ctags_launcher = anjuta_launcher_new ();

	anjuta_launcher_set_check_passwd_prompt (priv->ctags_launcher, FALSE);
	anjuta_launcher_set_encoding (priv->ctags_launcher, NULL);
		
	g_signal_connect (G_OBJECT (priv->ctags_launcher), "child-exited",
						  G_CALLBACK (on_scan_files_end_1), NULL);

	exe_string = g_strdup_printf ("%s --sort=no --fields=afmiKlnsStz --c++-kinds=+p "
								  "--filter=yes --filter-terminator='"CTAGS_MARKER"'",
								  priv->ctags_path);
		
	anjuta_launcher_execute (priv->ctags_launcher,
								 exe_string, sdb_engine_ctags_output_callback_1, 
								 dbe);
	g_free (exe_string);
}

/**
 * A GAsyncReadyCallback function. This function is the async continuation for
 * sdb_engine_scan_files_1 ().
 */
static void  
sdb_engine_scan_files_2 (GFile *gfile,
                         GAsyncResult *res,
                         gpointer user_data)
{
	ScanFiles1Data *sf_data = (ScanFiles1Data*)user_data;
	SymbolDBEngine *dbe;
	SymbolDBEnginePriv *priv;
	GFileInfo *ginfo;
	gchar *local_path;
	gchar *real_file;
	gboolean symbols_update;
	gint partial_count;
	gint files_list_len;

	dbe = sf_data->dbe;
	symbols_update = sf_data->symbols_update;
	real_file = sf_data->real_file;
	files_list_len = sf_data->files_list_len;
	partial_count = sf_data->partial_count;

	priv = dbe->priv;
	
	ginfo = g_file_query_info_finish (gfile, res, NULL);

	local_path = g_file_get_path (gfile);
	
	if (ginfo == NULL || 
		g_file_info_get_attribute_boolean (ginfo, 
								   G_FILE_ATTRIBUTE_ACCESS_CAN_READ) == FALSE)
	{
		g_warning ("File does not exist or is unreadable by user (%s)", local_path);

		g_free (local_path);
		g_free (real_file);
		g_free (sf_data);

		if (ginfo)
			g_object_unref (ginfo);
		if (gfile)
			g_object_unref (gfile);
		return;
	}
	
	/*DEBUG_PRINT ("sent to stdin %s", local_path);*/
	anjuta_launcher_send_stdin (priv->ctags_launcher, local_path);
	anjuta_launcher_send_stdin (priv->ctags_launcher, "\n");
	
	if (symbols_update == TRUE) 
	{
		/* will this be the last file in the list? */
		if (partial_count + 1 >= files_list_len) 
		{
			/* yes */
			g_async_queue_push (priv->scan_queue, GINT_TO_POINTER (DO_UPDATE_SYMS_AND_EXIT));
		}
		else 
		{
			/* no */
			g_async_queue_push (priv->scan_queue, GINT_TO_POINTER (DO_UPDATE_SYMS));
		}
	}
	else 
	{
		if (partial_count + 1 >= files_list_len) 
		{
			/* yes */
			g_async_queue_push (priv->scan_queue, GINT_TO_POINTER (DONT_UPDATE_SYMS_AND_EXIT));
		}
		else {
			/* no */
			g_async_queue_push (priv->scan_queue, GINT_TO_POINTER (DONT_UPDATE_SYMS));
		}
	}

	/* don't forget to add the real_files if the caller provided a list for
	 * them! */
	if (real_file != NULL)
	{
		g_async_queue_push (priv->scan_queue, 
							real_file);
	}
	else 
	{
		/* else add a DONT_FAKE_UPDATE_SYMS marker, just to notify that this 
		 * is not a fake file scan 
		 */
		g_async_queue_push (priv->scan_queue, GINT_TO_POINTER (DONT_FAKE_UPDATE_SYMS));
	}	
	
	/* we don't need ginfo object anymore, bye */
	g_object_unref (ginfo);
	g_object_unref (gfile);
	g_free (local_path);
	g_free (sf_data);
	/* no need to free real_file. For two reasons: 1. it's null. 2. it has been
	 * pushed in the async queue and will be freed later
	 */
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

	g_return_val_if_fail (files_list != NULL, FALSE);
	
	if (files_list->len == 0)
		return FALSE;	
	
	/* start process in server mode */
	priv = dbe->priv;

	if (real_files_list != NULL && (files_list->len != real_files_list->len)) 
	{
		g_warning ("no matched size between real_files_list and files_list");		
		return FALSE;
	}
	
	/* if ctags_launcher isn't initialized, then do it now. */
	/* lazy initialization */
	if (priv->ctags_launcher == NULL) 
	{
		sdb_engine_ctags_launcher_create (dbe);
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
		/*DEBUG_PRINT ("temp_file %s", temp_file);*/

		/* no need to free temp_file (alias shared_mem_str). It will be freed on plugin finalize */
	}
	
	for (i = 0; i < files_list->len; i++)
	{
		GFile *gfile;
		ScanFiles1Data *sf_data;
		gchar *node = (gchar *) g_ptr_array_index (files_list, i);
		gfile = g_file_new_for_path (node);
	
		/* prepare an ojbect where to store some data for the async call */
		sf_data = g_new0 (ScanFiles1Data, 1);
		sf_data->dbe = dbe;
		sf_data->files_list_len = files_list->len;
		sf_data->partial_count = i;
		sf_data->symbols_update = symbols_update;
		
		if (real_files_list != NULL)
		{
			sf_data->real_file = g_strdup (g_ptr_array_index (real_files_list, i));
		}
		else
		{
			sf_data->real_file = NULL;
		}

		/* call it */
		g_file_query_info_async (gfile, 
								 G_FILE_ATTRIBUTE_ACCESS_CAN_READ, 
								 G_FILE_QUERY_INFO_NONE, 
								 G_PRIORITY_LOW,
								 NULL,
								 (GAsyncReadyCallback)sdb_engine_scan_files_2,
								 sf_data);
	}

	return TRUE;
}

static void
sdb_engine_init (SymbolDBEngine * object)
{
	SymbolDBEngine *sdbe;
	GHashTable *h;
	gint i;
	
	sdbe = SYMBOL_DB_ENGINE (object);
	sdbe->priv = g_new0 (SymbolDBEnginePriv, 1);

	sdbe->priv->db_connection = NULL;
	sdbe->priv->sql_parser = NULL;
	sdbe->priv->db_directory = NULL;
	sdbe->priv->project_directory = NULL;
	sdbe->priv->cnc_string = NULL;	
	
	/* initialize an hash table to be used and shared with Iterators */
	sdbe->priv->sym_type_conversion_hash =
		g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);	
	h = sdbe->priv->sym_type_conversion_hash;

	/* please if you change some value below here remember to change also on */
	g_hash_table_insert (h, g_strdup("class"), 
				GINT_TO_POINTER (IANJUTA_SYMBOL_TYPE_CLASS));

	g_hash_table_insert (h, g_strdup("enum"), 
				GINT_TO_POINTER (IANJUTA_SYMBOL_TYPE_ENUM));

	g_hash_table_insert (h, g_strdup("enumerator"), 
				GINT_TO_POINTER (IANJUTA_SYMBOL_TYPE_ENUMERATOR));

	g_hash_table_insert (h, g_strdup("field"), 
				GINT_TO_POINTER (IANJUTA_SYMBOL_TYPE_FIELD));

	g_hash_table_insert (h, g_strdup("function"), 
				GINT_TO_POINTER (IANJUTA_SYMBOL_TYPE_FUNCTION));

	g_hash_table_insert (h, g_strdup("interface"), 
				GINT_TO_POINTER (IANJUTA_SYMBOL_TYPE_INTERFACE));

	g_hash_table_insert (h, g_strdup("member"), 
				GINT_TO_POINTER (IANJUTA_SYMBOL_TYPE_MEMBER));

	g_hash_table_insert (h, g_strdup("method"), 
				GINT_TO_POINTER (IANJUTA_SYMBOL_TYPE_METHOD));

	g_hash_table_insert (h, g_strdup("namespace"), 
				GINT_TO_POINTER (IANJUTA_SYMBOL_TYPE_NAMESPACE));

	g_hash_table_insert (h, g_strdup("package"), 
				GINT_TO_POINTER (IANJUTA_SYMBOL_TYPE_PACKAGE));

	g_hash_table_insert (h, g_strdup("prototype"), 
				GINT_TO_POINTER (IANJUTA_SYMBOL_TYPE_PROTOTYPE));
				
	g_hash_table_insert (h, g_strdup("struct"), 
				GINT_TO_POINTER (IANJUTA_SYMBOL_TYPE_STRUCT));
				
	g_hash_table_insert (h, g_strdup("typedef"), 
				GINT_TO_POINTER (IANJUTA_SYMBOL_TYPE_TYPEDEF));
				
	g_hash_table_insert (h, g_strdup("union"), 
				GINT_TO_POINTER (IANJUTA_SYMBOL_TYPE_UNION));
				
	g_hash_table_insert (h, g_strdup("variable"), 
				GINT_TO_POINTER (IANJUTA_SYMBOL_TYPE_VARIABLE));

	g_hash_table_insert (h, g_strdup("externvar"), 
				GINT_TO_POINTER (IANJUTA_SYMBOL_TYPE_EXTERNVAR));

	g_hash_table_insert (h, g_strdup("macro"), 
				GINT_TO_POINTER (IANJUTA_SYMBOL_TYPE_MACRO));

	g_hash_table_insert (h, g_strdup("macro_with_arg"), 
				GINT_TO_POINTER (IANJUTA_SYMBOL_TYPE_MACRO_WITH_ARG));

	g_hash_table_insert (h, g_strdup("file"), 
				GINT_TO_POINTER (IANJUTA_SYMBOL_TYPE_FILE));

	g_hash_table_insert (h, g_strdup("other"), 
				GINT_TO_POINTER (IANJUTA_SYMBOL_TYPE_OTHER));
	
	
	/* create the hash table that will store shared memory files strings used for 
	 * buffer scanning. Un Engine destroy will unlink () them.
	 */
	sdbe->priv->garbage_shared_mem_files = g_hash_table_new_full (g_str_hash, g_str_equal, 
													  g_free, NULL);	
	
	sdbe->priv->ctags_launcher = NULL;
	sdbe->priv->removed_launchers = NULL;
	sdbe->priv->shutting_down = FALSE;

	/* set the ctags executable path to NULL */
	sdbe->priv->ctags_path = NULL;

	/* identify the scan process with an id. There can be multiple files associated
	 * within a process. A call to scan_files () will put inside the queue an id
	 * returned and emitted by scan-end.
	 */
	sdbe->priv->scan_process_id_queue = g_async_queue_new ();
	sdbe->priv->scan_process_id = 1;
	
	/* the scan_queue? It will contain mainly 
	 * ints that refer to the force_update status.
	 */
	sdbe->priv->scan_queue = g_async_queue_new ();

	/* the thread pool for tags scannning */
	sdbe->priv->thread_pool = g_thread_pool_new (sdb_engine_ctags_output_thread,
												 sdbe, THREADS_MAX_CONCURRENT,
												 FALSE, NULL);
	
	/* some signals queues */
	sdbe->priv->signals_queue = g_async_queue_new ();
	sdbe->priv->updated_symbols_id = g_async_queue_new ();
	sdbe->priv->updated_scope_symbols_id = g_async_queue_new ();
	sdbe->priv->inserted_symbols_id = g_async_queue_new ();
	
	
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
		"SELECT file_id, file_path AS db_file_path, prj_id, lang_id, analyse_time FROM file "
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
		"SELECT file_id, file_path AS db_file_path FROM file WHERE file_id NOT IN "
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

	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
									PREP_QUERY_GET_LANGUAGE_COUNT,
	 	"SELECT COUNT(*) FROM language");	
	
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
	 	"## /* name:'scopedsymid' type:gint */) "
	 	"AND file_position < (SELECT file_position FROM symbol WHERE symbol_id = "
	 	"## /* name:'scopedsymid' type:gint */) "
	 	"ORDER BY file_position DESC");
	
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_GET_SCOPE_DEFINITION_ID_BY_WALK_DOWN_SCOPE_PATH,
	 	"SELECT scope_definition_id FROM symbol "
	 	"WHERE scope_id = ## /* name:'scopeid' type:gint */ AND scope_definition_id = ("
		 	"SELECT scope.scope_id FROM scope "
			"INNER JOIN sym_type ON scope.type_id = sym_type.type_id "
			"WHERE sym_type.type_type = ## /* name:'symtype' type:gchararray */ "
				"AND scope.scope_name = ## /* name:'scopename' type:gchararray */) LIMIT 1");
	
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
	 	"sym_type.type_id AND symbol.scope_id = 0 WHERE sym_type.type_type='class' AND "
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
	 	"UPDATE symbol SET scope_id =  ## /* name:'scopeid' type:gint */ "
	 	"WHERE symbol_id = ## /* name:'symbolid' type:gint */");
	
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_UPDATE_SYMBOL_SCOPE_ID_MIXED,
		"UPDATE symbol SET scope_id = (SELECT scope_definition_id FROM symbol "
		"JOIN sym_type ON symbol.type_id "
	 	"= sym_type.type_id WHERE sym_type.type_type = ## /* name:'tokenname' "
	 	"type:gchararray */ AND sym_type.type_name = ## /* name:'objectname' "
	 	"type:gchararray */ LIMIT 1) "
	 	"WHERE symbol_id = ## /* name:'symbolid' type:gint */");
	
	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_GET_SYMBOL_ID_BY_UNIQUE_INDEX_KEY,
	 	"SELECT symbol_id FROM symbol WHERE name = ## /* name:'symname' "
	 	"type:gchararray */ AND file_defined_id =  ## /* name:'filedefid' " 
	 	"type:gint */ AND type_id = ## /* name:'typeid' type:gint */ LIMIT 1");	

	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_GET_SYMBOL_ID_BY_UNIQUE_INDEX_KEY_EXT,
	 	"SELECT symbol_id FROM symbol WHERE name = ## /* name:'symname' "
	 	"type:gchararray */ AND file_defined_id =  ## /* name:'filedefid' "
	 	"type:gint */ AND file_position = ## /* name:'filepos' type:gint */ "
		"AND type_id = ## /* name:'typeid' type:gint */ AND "
		"update_flag = 0 LIMIT 1");

	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_GET_SYMBOL_ID_BY_UNIQUE_INDEX_KEY_EXT2,
	 	"SELECT symbol_id FROM symbol WHERE name = ## /* name:'symname' "
	 	"type:gchararray */ AND file_defined_id =  ## /* name:'filedefid' "
	 	"type:gint */ AND type_id = ## /* name:'typeid' type:gint */ AND "
		"update_flag = 0 LIMIT 1");
	
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
	 	"WHERE file.file_path = ## /* name:'filepath' type:gchararray */) "
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

	STATIC_QUERY_POPULATE_INIT_NODE(sdbe->priv->static_query_list, 
	 								PREP_QUERY_REMOVE_FILE_BY_PROJECT_NAME,
	 	"DELETE FROM file WHERE prj_id = "
		"(SELECT project_id FROM project WHERE project_name = "
		"## /* name:'prjname' type:gchararray */) AND "
		"file_path = ## /* name:'filepath' type:gchararray */");
	
	
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
									TRUE);

	DYN_QUERY_POPULATE_INIT_NODE(sdbe->priv->dyn_query_list,
									DYN_PREP_QUERY_FIND_SYMBOL_BY_NAME_PATTERN_FILTERED,			
									TRUE);

	DYN_QUERY_POPULATE_INIT_NODE(sdbe->priv->dyn_query_list,
								 	DYN_PREP_QUERY_GET_SCOPE_MEMBERS_BY_SYMBOL_ID,
									TRUE);

	DYN_QUERY_POPULATE_INIT_NODE(sdbe->priv->dyn_query_list,
								 	DYN_PREP_QUERY_GET_SCOPE_MEMBERS_BY_SYMBOL_ID_FILTERED,
									TRUE);
	
	DYN_QUERY_POPULATE_INIT_NODE(sdbe->priv->dyn_query_list,
									DYN_PREP_QUERY_GET_FILES_FOR_PROJECT,
								 	TRUE);
	
	/* init cache hashtables */
	sdb_engine_init_caches (sdbe);
	
	sdbe->priv->file_symbols_cache = g_tree_new_full ((GCompareDataFunc)&symbol_db_gtree_compare_func, 
										 NULL,
										 NULL,
										 NULL);
	
	/* init memory pool object for GValue strings */
#ifdef USE_ASYNC_QUEUE	
	sdbe->priv->mem_pool_string = g_async_queue_new_full (g_free);
	sdbe->priv->mem_pool_int = g_async_queue_new_full (g_free);
#else
	sdbe->priv->mem_pool_string = g_queue_new ();
	sdbe->priv->mem_pool_int = g_queue_new ();	
#endif
	
	for (i = 0; i < MEMORY_POOL_STRING_SIZE; i++) 
	{
		GValue *value = gda_value_new (G_TYPE_STRING);
#ifdef USE_ASYNC_QUEUE	
		g_async_queue_push (sdbe->priv->mem_pool_string, value);
#else
		g_queue_push_head (sdbe->priv->mem_pool_string, value);
#endif		
	}

	for (i = 0; i < MEMORY_POOL_INT_SIZE; i++) 
	{
		GValue *value = gda_value_new (G_TYPE_INT);
#ifdef USE_ASYNC_QUEUE			
		g_async_queue_push (sdbe->priv->mem_pool_int, value);		
#else
		g_queue_push_head (sdbe->priv->mem_pool_int, value);		
#endif		
	}	
}

static void
sdb_engine_unlink_shared_files (gpointer key, gpointer value, gpointer user_data)
{
	shm_unlink (key);
}

static void 
sdb_engine_unref_removed_launchers (gpointer data, gpointer user_data)
{
	g_object_unref (data);
}

static void
sdb_engine_finalize (GObject * object)
{
	SymbolDBEngine *dbe;
	SymbolDBEnginePriv *priv;
	
	dbe = SYMBOL_DB_ENGINE (object);
	priv = dbe->priv;
	
	if (priv->thread_pool)
	{
		g_thread_pool_free (priv->thread_pool, TRUE, TRUE);
		priv->thread_pool = NULL;
	}
	
	if (priv->ctags_launcher)
	{
		g_object_unref (priv->ctags_launcher);
		priv->ctags_launcher = NULL;
	}		
	
	if (priv->removed_launchers)
	{
		g_list_foreach (priv->removed_launchers, 
						sdb_engine_unref_removed_launchers, NULL);
		g_list_free (priv->removed_launchers);
		priv->removed_launchers = NULL;
	}
	
	if (priv->mutex)
	{
		g_mutex_free (priv->mutex);
		priv->mutex = NULL;
	}
	
	if (priv->timeout_trigger_handler > 0)
		g_source_remove (priv->timeout_trigger_handler);

	sdb_engine_disconnect_from_db (dbe);	
	sdb_engine_free_cached_queries (dbe);
	sdb_engine_free_cached_dynamic_queries (dbe);
	
	if (priv->scan_process_id_queue)
	{
		g_async_queue_unref (priv->scan_process_id_queue);
		priv->scan_process_id_queue = NULL;
	}
	
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

	if (priv->updated_scope_symbols_id)
	{
		g_async_queue_unref (priv->updated_scope_symbols_id);
		priv->updated_scope_symbols_id = NULL;
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
	

	if (priv->sym_type_conversion_hash)
		g_hash_table_destroy (priv->sym_type_conversion_hash);
	
	if (priv->signals_queue)
		g_async_queue_unref (priv->signals_queue);
	
	sdb_engine_clear_caches (dbe);

	g_tree_destroy (priv->file_symbols_cache);

#ifdef USE_ASYNC_QUEUE	
	g_async_queue_unref (priv->mem_pool_string);
	g_async_queue_unref (priv->mem_pool_int);
	priv->mem_pool_string = NULL;
	priv->mem_pool_int = NULL;	
#else
	g_queue_foreach (priv->mem_pool_string, (GFunc)gda_value_free, NULL);
	g_queue_free (priv->mem_pool_string);
	
	g_queue_foreach (priv->mem_pool_int, (GFunc)gda_value_free, NULL);
	g_queue_free (priv->mem_pool_int);	
	
	priv->mem_pool_string = NULL;
	priv->mem_pool_int = NULL;	
#endif
	
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
						g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 
						1,
						G_TYPE_INT);

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

gboolean
symbol_db_engine_set_ctags_path (SymbolDBEngine * dbe, const gchar * ctags_path)
{
	SymbolDBEnginePriv *priv;

	g_return_val_if_fail (dbe != NULL, FALSE);
	g_return_val_if_fail (ctags_path != NULL, FALSE);
	
	priv = dbe->priv;
	
	/* Check if ctags is really installed */
	if (!anjuta_util_prog_is_installed (ctags_path, TRUE))
	{
		g_warning ("symbol_db_engine_set_ctags_path (): Wrong path for ctags. Keeping "
				   "the old value %s", priv->ctags_path);
		return priv->ctags_path != NULL;
	}	
	
	/* have we already got it? */
	if (priv->ctags_path != NULL && 
		g_strcmp0 (priv->ctags_path, ctags_path) == 0)
		return TRUE;

	/* free the old value */
	g_free (priv->ctags_path);
	
	/* is anjutalauncher already created? */
	if (priv->ctags_launcher != NULL)
	{
		AnjutaLauncher *tmp;
		tmp = priv->ctags_launcher;

		/* recreate it on the fly */
		sdb_engine_ctags_launcher_create (dbe);

		/* keep the launcher alive to avoid crashes */
		priv->removed_launchers = g_list_prepend (priv->removed_launchers, tmp);
	}	
	
	/* set the new one */
	priv->ctags_path = g_strdup (ctags_path);	
	return TRUE;
}

SymbolDBEngine *
symbol_db_engine_new (const gchar * ctags_path)
{
	SymbolDBEngine *sdbe;
	SymbolDBEnginePriv *priv;
	
	g_return_val_if_fail (ctags_path != NULL, NULL);
	sdbe = g_object_new (SYMBOL_TYPE_DB_ENGINE, NULL);
	
	priv = sdbe->priv;
	priv->mutex = g_mutex_new ();

	/* set the mandatory ctags_path */
	if (!symbol_db_engine_set_ctags_path (sdbe, ctags_path))
	{
		return NULL;
	}
		
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
	sdb_engine_execute_unknown_sql (dbe, "PRAGMA journal_mode = OFF");
	sdb_engine_execute_unknown_sql (dbe, "PRAGMA read_uncommitted = 1");
}

/**
 * Delete all entries that don't have a reference on symbol table.
 */
static void
sdb_engine_normalize_sym_type (SymbolDBEngine * dbe)
{	
	sdb_engine_execute_unknown_sql (dbe, "DELETE FROM sym_type WHERE type_id NOT IN "
		"(SELECT type_id FROM symbol)");
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

	priv->cnc_string = g_strdup (cnc_string);
	priv->sql_parser = gda_connection_create_parser (priv->db_connection);
	
	if (!GDA_IS_SQL_PARSER (priv->sql_parser)) 
	{
		g_warning ("Could not create sql parser. Check your libgda installation");
		return FALSE;
	}
	
	DEBUG_PRINT ("Connected to database %s", cnc_string);
	return TRUE;
}

gboolean
symbol_db_engine_is_connected (SymbolDBEngine * dbe)
{
	SymbolDBEnginePriv *priv;

	g_return_val_if_fail (dbe != NULL, FALSE);
	priv = dbe->priv;	
	
	return priv->db_connection && priv->cnc_string && priv->sql_parser && 
		gda_connection_is_opened (priv->db_connection );
}

/**
 * Creates required tables for the database to work.
 * @param tables_sql_file File containing sql code.
 */
static gboolean
sdb_engine_create_db_tables (SymbolDBEngine * dbe, const gchar * tables_sql_file)
{	
	SymbolDBEnginePriv *priv;
	gchar *contents;
	gchar *query;
	gsize sizez;

	g_return_val_if_fail (tables_sql_file != NULL, FALSE);

	priv = dbe->priv;

	g_return_val_if_fail (priv->db_connection != NULL, FALSE);

	/* read the contents of the file */
	if (g_file_get_contents (tables_sql_file, &contents, &sizez, NULL) == FALSE)
	{
		g_warning ("Something went wrong while trying to read %s",
				   tables_sql_file);

		return FALSE;
	}

	sdb_engine_execute_non_select_sql (dbe, contents);	
	g_free (contents);
	
	/* set the current symbol db database version. This may help if new tables/fields
	 * are added/removed in future versions.
	 */
	query = "INSERT INTO version VALUES ("SYMBOL_DB_VERSION")";
	sdb_engine_execute_non_select_sql (dbe, query);	
	
	return TRUE;
}

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
	MP_LEND_OBJ_STR(priv, value);	
	g_value_set_static_string (value, relative);	

	if ((file_defined_id = sdb_engine_get_tuple_id_by_unique_name (dbe,
													PREP_QUERY_GET_FILE_ID_BY_UNIQUE_NAME,
													"filepath",
													value)) < 0)
	{	
		g_free (relative);
		if (priv->mutex)
			g_mutex_unlock (priv->mutex);
		return FALSE;	
	}
	
	g_free (relative);
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
	
	/* terminate threads, if ever they're running... */
	g_thread_pool_free (priv->thread_pool, TRUE, TRUE);
	priv->thread_pool = g_thread_pool_new (sdb_engine_ctags_output_thread,
										   dbe, THREADS_MAX_CONCURRENT,
										   FALSE, NULL);
	
	return sdb_engine_disconnect_from_db (dbe);
}

gboolean
symbol_db_engine_open_db (SymbolDBEngine * dbe, const gchar * base_db_path,
						  const gchar * prj_directory)
{
	SymbolDBEnginePriv *priv;
	gboolean needs_tables_creation = FALSE;
	gchar *cnc_string;

	DEBUG_PRINT ("SymbolDBEngine: opening project %s with base dir %s", 
				 prj_directory, base_db_path);
	
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

	DEBUG_PRINT ("symbol_db_engine_open_db (): opening/connecting to "
				 "database with %s...", cnc_string);
	sdb_engine_connect_to_db (dbe, cnc_string);

	if (needs_tables_creation == TRUE)
	{
		DEBUG_PRINT ("%s", "symbol_db_engine_open_db (): creating tables: it needs tables...");
		sdb_engine_create_db_tables (dbe, TABLES_SQL);
	}

	sdb_engine_set_defaults_db_parameters (dbe);

	/* normalize some tables */
	sdb_engine_normalize_sym_type (dbe);
		
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
	GValue *ret_value;
	gboolean ret_bool;

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
	MP_SET_HOLDER_BATCH_STR(priv, param, workspace_name, ret_bool, ret_value);
	
	/* execute the query with parametes just set */
	if (gda_connection_statement_execute_non_select (priv->db_connection, 
														  (GdaStatement*)stmt, 
														  (GdaSet*)plist, NULL, NULL) == -1)
	{		
		return FALSE;
	}

	return TRUE;
}

gboolean
symbol_db_engine_project_exists (SymbolDBEngine * dbe,	/*gchar* workspace, */
							   	const gchar * project_name)
{
	GValue *value;
	SymbolDBEnginePriv *priv;
	gint prj_id;

	priv = dbe->priv;

	g_return_val_if_fail (priv->db_connection != NULL, FALSE);
	MP_LEND_OBJ_STR(priv,value);	
	g_value_set_static_string (value, project_name);

	/* test the existence of the project in db */
	if ((prj_id = sdb_engine_get_tuple_id_by_unique_name (dbe,
				PREP_QUERY_GET_PROJECT_ID_BY_UNIQUE_NAME,
				"prjname",
				 value)) <= 0)
	{
		return FALSE;
	}

	/* we found it */
	return TRUE;
}

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
	const gchar *workspace_name;
	gint wks_id;
	SymbolDBEnginePriv *priv;
	GValue *ret_value;
	GValue *value;
	gboolean ret_bool;

	g_return_val_if_fail (dbe != NULL, FALSE);	
	priv = dbe->priv;

	if (workspace == NULL)
	{
		GValue *value;
		workspace_name = "anjuta_workspace_default";	
		
		DEBUG_PRINT ("adding default workspace... '%s'", workspace_name);
		MP_LEND_OBJ_STR(priv, value);		
		g_value_set_static_string (value, workspace_name);
		
		if ((wks_id = sdb_engine_get_tuple_id_by_unique_name (dbe,
				 PREP_QUERY_GET_WORKSPACE_ID_BY_UNIQUE_NAME,
				 "wsname",
				 value)) <= 0)
		{ 
			if (symbol_db_engine_add_new_workspace (dbe, workspace_name) == FALSE)
			{	
				DEBUG_PRINT ("%s", "Project cannot be added because a default workspace "
							 "cannot be created");
				return FALSE;
			}
		}
	}
	else
	{
		workspace_name = workspace;
	}

	MP_LEND_OBJ_STR (priv, value);
	g_value_set_static_string (value, workspace_name);

	/* get workspace id */
	if ((wks_id = sdb_engine_get_tuple_id_by_unique_name (dbe,
				 PREP_QUERY_GET_WORKSPACE_ID_BY_UNIQUE_NAME,
				 "wsname",
				 value)) <= 0)
	{
		DEBUG_PRINT ("No workspace id");
		return FALSE;
	}	
	
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
	
	MP_SET_HOLDER_BATCH_STR(priv, param, project, ret_bool, ret_value);	
		
	if ((param = gda_set_get_holder ((GdaSet*)plist, "wsid")) == NULL)
	{
		g_warning ("param prjname is NULL from pquery!");
		return FALSE;
	}
	
	MP_SET_HOLDER_BATCH_INT(priv, param, wks_id, ret_bool, ret_value);	
		
	/* execute the query with parametes just set */
	if (gda_connection_statement_execute_non_select (priv->db_connection, 
														  (GdaStatement*)stmt, 
														  (GdaSet*)plist, NULL, NULL) == -1)
	{		
		return FALSE;
	}

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

	MP_LEND_OBJ_STR(priv, value);
	g_value_set_static_string (value, language);

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
		GValue *ret_value;
		gboolean ret_bool;

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
		
		MP_SET_HOLDER_BATCH_STR(priv, param, language, ret_bool, ret_value);		
				
		/* execute the query with parametes just set */
		if (gda_connection_statement_execute_non_select (priv->db_connection, 
														 (GdaStatement*)stmt, 
														 (GdaSet*)plist, &last_inserted,
														 NULL) == -1)
		{		
			table_id = -1;
		}
		else {
			const GValue *value = gda_set_get_holder_value (last_inserted, "+0");
			table_id = g_value_get_int (value);
		}		
		
		if (last_inserted)
			g_object_unref (last_inserted);			
	}

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
sdb_engine_add_new_db_file (SymbolDBEngine * dbe, const gchar * project_name,
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
	GValue *value, *value2;
	
	priv = dbe->priv;

	/* check if the file is a correct one compared to the local_filepath */
	if (strstr (local_filepath, priv->project_directory) == NULL)
		return FALSE;

	MP_LEND_OBJ_STR(priv, value);	
	g_value_set_static_string (value, project_name);

	/* check for an already existing table with project "project". */
	if ((project_id = sdb_engine_get_tuple_id_by_unique_name (dbe,
								  PREP_QUERY_GET_PROJECT_ID_BY_UNIQUE_NAME,
								  "prjname",
								  value)) < 0)
	{
		g_warning ("no project with that name exists");
		return FALSE;
	}
	

	/* we're gonna set the file relative to the project folder, not the full one.
	 * e.g.: we have a file on disk: "/tmp/foo/src/file.c" and a db_directory located on
	 * "/tmp/foo/". The entry on db will be "src/file.c" 
	 */
	gchar *relative_path = symbol_db_engine_get_file_db_path (dbe, local_filepath);
	if (relative_path == NULL)
	{
		DEBUG_PRINT ("%s", "relative_path == NULL");
		return FALSE;
	}	
	
	MP_LEND_OBJ_STR(priv, value2);
	g_value_set_static_string (value2, relative_path);	

	if ((file_id = sdb_engine_get_tuple_id_by_unique_name (dbe,
								   PREP_QUERY_GET_FILE_ID_BY_UNIQUE_NAME,
								   "filepath",
								   value2)) < 0)
	{
		/* insert a new entry on db */
		const GdaSet *plist;
		const GdaStatement *stmt;
		GdaHolder *param;
		GValue *ret_value;
		gboolean ret_bool;

		
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
		
		MP_SET_HOLDER_BATCH_STR(priv, param, relative_path, ret_bool, ret_value);
								
		/* project id parameter */
		if ((param = gda_set_get_holder ((GdaSet*)plist, "prjid")) == NULL)
		{
			g_warning ("param prjid is NULL from pquery!");
			g_free (relative_path);
			return FALSE;
		}

		MP_SET_HOLDER_BATCH_INT(priv, param, project_id, ret_bool, ret_value);
		
		/* language id parameter */
		if ((param = gda_set_get_holder ((GdaSet*)plist, "langid")) == NULL)
		{
			g_warning ("param langid is NULL from pquery!");
			g_free (relative_path);
			return FALSE;
		}		

		MP_SET_HOLDER_BATCH_INT(priv, param, language_id, ret_bool, ret_value);
		
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

	g_free (relative_path);
	
	return TRUE;
} 

static gint 
sdb_engine_get_unique_scan_id (SymbolDBEngine * dbe)
{
	SymbolDBEnginePriv *priv;
	gint ret_id;
	
	priv = dbe->priv;
	
	if (priv->mutex)
		g_mutex_lock (priv->mutex);
	
	priv->scan_process_id++;	
	ret_id = priv->scan_process_id;
	
	/* add the current scan_process id into a queue */
	g_async_queue_push (priv->scan_process_id_queue, 
						GINT_TO_POINTER(priv->scan_process_id));

	if (priv->mutex)
		g_mutex_unlock (priv->mutex);
	return ret_id;
}
	
gint
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
	gint ret_id;
	
	g_return_val_if_fail (dbe != NULL, FALSE);
	g_return_val_if_fail (files_path != NULL, FALSE);
	g_return_val_if_fail (languages != NULL, FALSE);
	priv = dbe->priv;
	
	g_return_val_if_fail (priv->db_connection != NULL, FALSE);
	g_return_val_if_fail (files_path->len > 0, FALSE);
	g_return_val_if_fail (languages->len > 0, FALSE);

	filtered_files_path = g_ptr_array_new ();
	filtered_languages = g_ptr_array_new ();
	
	for (i = 0; i < files_path->len; i++)
	{
		const gchar *node_file = (const gchar *) g_ptr_array_index (files_path, i);
		const gchar *node_lang = (const gchar *) g_ptr_array_index (languages, i);
		
		if (force_scan == FALSE)
		{
			/* test the existence of the file in db */
			if (symbol_db_engine_file_exists (dbe, node_file) == TRUE)
			{
				/* we don't want to touch the already present file... within 
				 * its symbols
				 */
				continue;
			}
		}
		
		if (project_name != NULL && 
			sdb_engine_add_new_db_file (dbe, project_name, node_file, 
									 node_lang) == FALSE)
		{
			g_warning ("symbol_db_engine_add_new_files (): "
					   "Error processing file %s, db_directory %s, project_name %s, "
					   "project_directory %s", node_file, 
					   priv->db_directory, project_name, priv->project_directory);
			return FALSE;
		}
		
		/* note: we don't use g_strdup () here because we'll free the filtered_files_path
		 * before returning from this function.
		 */
		g_ptr_array_add (filtered_files_path, (gpointer)node_file);
	}

	/* perform the scan of files. It will spawn a fork() process with 
	 * AnjutaLauncher and ctags in server mode. After the ctags cmd has been 
	 * executed, the populating process'll take place.
	 */
	ret_code = sdb_engine_scan_files_1 (dbe, filtered_files_path, NULL, FALSE);
	
	if (ret_code == TRUE)
		ret_id = sdb_engine_get_unique_scan_id (dbe);
	else
		ret_id = -1;
	
	g_ptr_array_free (filtered_files_path, TRUE);
	return ret_id;
}

static inline gint
sdb_engine_add_new_sym_type (SymbolDBEngine * dbe, const tagEntry * tag_entry)
{
/*
	CREATE TABLE sym_type (type_id integer PRIMARY KEY AUTOINCREMENT,
                   type_type varchar (256) not null ,
                   type_name varchar (256) not null
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
	GValue *ret_value;
	gboolean ret_bool;
	
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

	MP_SET_HOLDER_BATCH_STR(priv, param, type, ret_bool, ret_value);

	/* type_name parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "typename")) == NULL)
	{
		g_warning ("param typename is NULL from pquery!");
		return -1;
	}
	
	MP_SET_HOLDER_BATCH_STR(priv, param, type_name, ret_bool, ret_value);
	
	/* execute the query with parametes just set */
	if (gda_connection_statement_execute_non_select (priv->db_connection, 
													 (GdaStatement*)stmt, 
													 (GdaSet*)plist, &last_inserted,
													 NULL) == -1)
	{
		GValue *value1, *value2;

		MP_LEND_OBJ_STR (priv, value1);
		g_value_set_static_string (value1, type);

		MP_LEND_OBJ_STR (priv, value2);
		g_value_set_static_string (value2, type_name);		

		if ((table_id = sdb_engine_get_tuple_id_by_unique_name2 (dbe,
													 PREP_QUERY_GET_SYM_TYPE_ID,
													 "type", value1,
													 "typename", value2)) < 0)
		{
			table_id = -1;
		}
		
		return table_id;
	}	
	else 
	{
		const GValue *value = gda_set_get_holder_value (last_inserted, "+0");
		table_id = g_value_get_int (value);
	}		
	
	if (last_inserted)
		g_object_unref (last_inserted);	
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

	MP_LEND_OBJ_STR(priv, value);	
	g_value_set_static_string (value, kind_name);
	
	if ((table_id = sdb_engine_get_tuple_id_by_unique_name (dbe,
										PREP_QUERY_GET_SYM_KIND_BY_UNIQUE_NAME,
										"kindname",
										value)) < 0)
	{
		const GdaSet *plist;
		const GdaStatement *stmt;
		GdaHolder *param;
		GdaSet *last_inserted;
		GValue *ret_value;
		gboolean ret_bool;

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

		MP_SET_HOLDER_BATCH_STR(priv, param, kind_name, ret_bool, ret_value);
	
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
			const GValue *value = gda_set_get_holder_value (last_inserted, "+0");
			table_id = g_value_get_int (value);
			/* we should cache only tables which are != -1 */
			sdb_engine_insert_cache (priv->kind_cache, kind_name, table_id);
		}
		if (last_inserted)
			g_object_unref (last_inserted);			
	}

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

	MP_LEND_OBJ_STR(priv, value);	
	g_value_set_static_string (value, access);

	if ((table_id = sdb_engine_get_tuple_id_by_unique_name (dbe,
									PREP_QUERY_GET_SYM_ACCESS_BY_UNIQUE_NAME,
									"accesskind",
									value)) < 0)
	{
		const GdaSet *plist;
		const GdaStatement *stmt;
		GdaHolder *param;
		GdaSet *last_inserted;
		GValue *ret_value;
		gboolean ret_bool;		

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
		
		MP_SET_HOLDER_BATCH_STR(priv, param, access, ret_bool, ret_value);
		
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
			const GValue *value = gda_set_get_holder_value (last_inserted, "+0");
			table_id = g_value_get_int (value);
			/* we should cache only tables which are != -1 */
			sdb_engine_insert_cache (priv->access_cache, access, table_id);
		}		
		
		if (last_inserted)
			g_object_unref (last_inserted);			
	}	


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

	MP_LEND_OBJ_STR(priv, value);	
	g_value_set_static_string (value, implementation);

	if ((table_id = sdb_engine_get_tuple_id_by_unique_name (dbe,
							PREP_QUERY_GET_SYM_IMPLEMENTATION_BY_UNIQUE_NAME,
							"implekind",
							value)) < 0)
	{
		const GdaSet *plist;
		const GdaStatement *stmt;
		GdaHolder *param;
		GdaSet *last_inserted;
		GValue *ret_value;
		gboolean ret_bool;

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
		
		MP_SET_HOLDER_BATCH_STR(priv, param, implementation, ret_bool, ret_value);		

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
			const GValue *value = gda_set_get_holder_value (last_inserted, "+0");
			table_id = g_value_get_int (value);			
			/* we should cache only tables which are != -1 */
			sdb_engine_insert_cache (priv->implementation_cache, implementation, 
									 table_id);	
		}
		if (last_inserted)
			g_object_unref (last_inserted);			
	}
	
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
	GValue *ret_value;
	gboolean ret_bool;
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
	
	MP_SET_HOLDER_BATCH_INT(priv, param, base_symbol_id, ret_bool, ret_value);		

	/* symderived id parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "symderived")) == NULL)
	{
		g_warning ("param symderived is NULL from pquery!");
		return;
	}
	
	MP_SET_HOLDER_BATCH_INT(priv, param, derived_symbol_id, ret_bool, ret_value);		
	
	/* execute the query with parametes just set */
	if (gda_connection_statement_execute_non_select (priv->db_connection, 
													 (GdaStatement*)stmt, 
													 (GdaSet*)plist, NULL,
													 NULL) == -1)
	{		
		g_warning ("Error adding heritage");
	}	
}

static inline gint
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
	GValue *ret_value;
	gboolean ret_bool;
	
	SymbolDBEnginePriv *priv;

	g_return_val_if_fail (tag_entry->kind != NULL, -1);

	priv = dbe->priv;
	
	
	/* This symbol will define a scope which name is tag_entry->name
	 * For example if we get a tag MyFoo with kind "namespace", it will define 
	 * the "MyFoo" scope, which type is "namespace MyFoo"
	 */
	scope = tag_entry->name;

	/* filter out 'variable' and 'member' kinds. They define no scope. */
	if (g_strcmp0 (tag_entry->kind, "variable") == 0 ||
		g_strcmp0 (tag_entry->kind, "member") == 0)
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
	
	MP_SET_HOLDER_BATCH_STR(priv, param, scope, ret_bool, ret_value);	

	/* typeid parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "typeid")) == NULL)
	{
		g_warning ("param typeid is NULL from pquery!");
		return -1;
	}
	
	MP_SET_HOLDER_BATCH_INT(priv, param, type_table_id, ret_bool, ret_value);

	/* execute the query with parameters just set */
	if (gda_connection_statement_execute_non_select (priv->db_connection, 
													 (GdaStatement*)stmt, 
													 (GdaSet*)plist, &last_inserted,
													 NULL) == -1)
	{
		GValue *value1, *value2;
		/* let's check for an already present scope table with scope and type_id infos. */
		MP_LEND_OBJ_STR (priv, value1);
		g_value_set_static_string (value1, scope);

		MP_LEND_OBJ_INT (priv, value2);		
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
	}
	else  {
		const GValue *value = gda_set_get_holder_value (last_inserted, "+0");
		table_id = g_value_get_int (value);
	}	

	if (last_inserted)
		g_object_unref (last_inserted);	
	
	return table_id;
}

/**
 * Saves the tagEntry info for a second pass parsing.
 * Usually we don't know all the symbol at the first scan of the tags. We need
 * a second one. These tuples are created for that purpose.
 *
 * @return the table_id of the inserted tuple. -1 on error.
 */
static inline gint
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
	const gchar *field_inherits, *field_struct, *field_typeref,
		*field_enum, *field_union, *field_class, *field_namespace;
	gboolean good_tag;
	GValue *ret_value;
	gboolean ret_bool;

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
	
	MP_SET_HOLDER_BATCH_INT(priv, param, symbol_referer_id, ret_bool, ret_value);	
	
	/* finherits parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "finherits")) == NULL)
	{
		g_warning ("param finherits is NULL from pquery!");
		return -1;
	}
	
	MP_SET_HOLDER_BATCH_STR(priv, param, field_inherits, ret_bool, ret_value);	
	

	/* fstruct parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "fstruct")) == NULL)
	{
		g_warning ("param fstruct is NULL from pquery!");
		return -1;
	}
	
	MP_SET_HOLDER_BATCH_STR(priv, param, field_struct, ret_bool, ret_value);	
	
	/* ftyperef parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "ftyperef")) == NULL)
	{
		g_warning ("param ftyperef is NULL from pquery!");
		return -1;
	}
	
	MP_SET_HOLDER_BATCH_STR(priv, param, field_typeref, ret_bool, ret_value);	
	
	/* fenum parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "fenum")) == NULL)
	{
		g_warning ("param fenum is NULL from pquery!");
		return -1;
	}
	
	MP_SET_HOLDER_BATCH_STR(priv, param, field_enum, ret_bool, ret_value);		
	
	/* funion parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "funion")) == NULL)
	{
		g_warning ("param funion is NULL from pquery!");
		return -1;
	}
	
	MP_SET_HOLDER_BATCH_STR(priv, param, field_union, ret_bool, ret_value);
		
	/* fclass parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "fclass")) == NULL)
	{
		g_warning ("param fclass is NULL from pquery!");
		return -1;
	}
	
	MP_SET_HOLDER_BATCH_STR(priv, param, field_class, ret_bool, ret_value);	

	/* fnamespace parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "fnamespace")) == NULL)
	{
		g_warning ("param fnamespace is NULL from pquery!");
		return -1;
	}
	
	MP_SET_HOLDER_BATCH_STR(priv, param, field_namespace, ret_bool, ret_value);		

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
		const GValue *value = gda_set_get_holder_value (last_inserted, "+0");
		table_id = g_value_get_int (value);
	}

	if (last_inserted)
		g_object_unref (last_inserted);	
	
	return table_id;
}

/** Return the symbol_id of the changed symbol */
static inline gint
sdb_engine_second_pass_update_scope_1 (SymbolDBEngine * dbe,
									   GdaDataModel * data, gint data_row,
									   gchar * token_name,
									   const GValue * token_value)
{
	gint scope_id;
	GValue *value1, *value2;
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
	GValue *ret_value;
	gboolean ret_bool;

	g_return_val_if_fail (G_VALUE_HOLDS_STRING (token_value), FALSE);
		
	priv = dbe->priv;
	tmp_str = g_value_get_string (token_value);

	/* we don't need empty strings */
	if (strlen (tmp_str) <= 0)
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
		if (g_strcmp0 (token_name, "typedef") == 0)
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

	MP_LEND_OBJ_STR (priv, value1);	
	g_value_set_static_string (value1, token_name);

	MP_LEND_OBJ_STR (priv, value2);
	g_value_set_static_string (value2, object_name);

	if ((scope_id = sdb_engine_get_tuple_id_by_unique_name2 (dbe,
									 PREP_QUERY_GET_SYMBOL_SCOPE_DEFINITION_ID,
									 "tokenname",
									 value1,
									 "objectname",
									 value2)) < 0)
	{
		if (free_token_name)
			g_free (token_name);

		return -1;
	}
	
	if (free_token_name)
		g_free (token_name);
	
	/* if we reach this point we should have a good scope_id.
	 * Go on with symbol updating.
	 */
	value_id2 = gda_data_model_get_value_at (data, 
						gda_data_model_get_column_index(data, "symbol_referer_id"), 
											 data_row, NULL);
	symbol_referer_id = g_value_get_int (value_id2);
	
	if ((stmt = sdb_engine_get_statement_by_query_id (dbe,
											 PREP_QUERY_UPDATE_SYMBOL_SCOPE_ID))
		== NULL)
	{
		g_warning ("query is null");
		return -1;
	}

	plist = sdb_engine_get_query_parameters_list (dbe, PREP_QUERY_UPDATE_SYMBOL_SCOPE_ID);

	/* scopeid parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "scopeid")) == NULL)
	{
		g_warning ("param scopeid is NULL from pquery!");
		return -1;
	}

	MP_SET_HOLDER_BATCH_INT(priv, param, scope_id, ret_bool, ret_value);		

	/* symbolid parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "symbolid")) == NULL)
	{
		g_warning ("param symbolid is NULL from pquery!");
		return -1;
	}

	MP_SET_HOLDER_BATCH_INT(priv, param, symbol_referer_id, ret_bool, ret_value);

	/* execute the query with parametes just set */
	gda_connection_statement_execute_non_select (priv->db_connection, 
													 (GdaStatement*)stmt, 
													 (GdaSet*)plist, NULL,
													 NULL);

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
	
	DEBUG_PRINT ("Processing %d rows", gda_data_model_get_n_rows (data));
	
	for (i = 0; i < gda_data_model_get_n_rows (data); i++)
	{
		GValue *value;		
		
		if ((value =
			 (GValue *) gda_data_model_get_value_at (data, 
						gda_data_model_get_column_index(data, "field_class"),
															  i, NULL)) != NULL)
		{
			sdb_engine_second_pass_update_scope_1 (dbe, data, i, "class",
												   value);
		}

		if ((value =
			 (GValue *) gda_data_model_get_value_at (data, 
						gda_data_model_get_column_index(data,"field_struct"),
															  i, NULL)) != NULL)
		{
			sdb_engine_second_pass_update_scope_1 (dbe, data, i, "struct",
												   value);
		}

		if ((value =
			 (GValue *) gda_data_model_get_value_at (data, 
						gda_data_model_get_column_index(data, "field_typeref"),
															  i, NULL)) != NULL)
		{
			/* this is a "typedef", not a "typeref". */
			sdb_engine_second_pass_update_scope_1 (dbe, data, i, "typedef",
												   value);
		}

		if ((value =
			 (GValue *) gda_data_model_get_value_at (data, 
						gda_data_model_get_column_index(data, "field_enum"),
															  i, NULL)) != NULL)
		{
			sdb_engine_second_pass_update_scope_1 (dbe, data, i, "enum", value);
		}

		if ((value =
			 (GValue *) gda_data_model_get_value_at (data, 
						gda_data_model_get_column_index(data, "field_union"),
															  i, NULL)) != NULL)
		{
			sdb_engine_second_pass_update_scope_1 (dbe, data, i, "union",
												   value);
		}

		if ((value =
			 (GValue *) gda_data_model_get_value_at (data, 
						gda_data_model_get_column_index(data, "field_namespace"),
															  i, NULL)) != NULL)
		{
			sdb_engine_second_pass_update_scope_1 (dbe, data, i, "namespace",
												   value);
		}
	}
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
	
	DEBUG_PRINT ("%s", "sdb_engine_second_pass_update_heritage ()");
	
	for (i = 0; i < gda_data_model_get_n_rows (data); i++)
	{
		const GValue *value;
		const gchar *inherits;
		gchar *item;
		gchar **inherits_list;
		gint j;
		
		value = gda_data_model_get_value_at (data, 
						gda_data_model_get_column_index(data, "field_inherits"), i, 
											 NULL);
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
					gda_data_model_get_value_at (data, 
						gda_data_model_get_column_index(data, "field_namespace"),
												 i, NULL);
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
																 1, i, NULL)) != NULL)
			{
				derived_klass_id = g_value_get_int (value);
			}
			else
			{
				derived_klass_id = 0;
			}

			/* ok, search for the symbol_id of the base class */
			if (namespace_name == NULL)
			{
				GValue *value1;

				MP_LEND_OBJ_STR (priv, value1);
				g_value_set_static_string (value1, klass_name);
				
				if ((base_klass_id =
					 sdb_engine_get_tuple_id_by_unique_name (dbe,
										 PREP_QUERY_GET_SYMBOL_ID_BY_CLASS_NAME,
										 "klassname",
										 value1)) < 0)
				{
					continue;
				}
			}
			else
			{
				GValue *value1;
				GValue *value2;

				MP_LEND_OBJ_STR (priv, value1);
				g_value_set_static_string (value1, klass_name);

				MP_LEND_OBJ_STR (priv, value2);
				g_value_set_static_string (value2, namespace_name);

				if ((base_klass_id =
					 sdb_engine_get_tuple_id_by_unique_name2 (dbe,
						  PREP_QUERY_GET_SYMBOL_ID_BY_CLASS_NAME_AND_NAMESPACE,
						  "klassname",
						  value1,
						  "namespacename",
						  value2)) < 0)
				{
					continue;
				}
			}

			g_free (namespace_name);
			g_free (klass_name);
			
			DEBUG_PRINT ("gonna sdb_engine_add_new_heritage with "
						 "base_klass_id %d, derived_klass_id %d", base_klass_id, 
						 derived_klass_id);
			sdb_engine_add_new_heritage (dbe, base_klass_id, derived_klass_id);
		}

		g_strfreev (inherits_list);			
	}	
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
	
	DEBUG_PRINT ("%s", "sdb_engine_second_pass_do()");	

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
	const gchar* name;
	gint file_position = 0;
	gint is_file_scope = 0;
	const gchar *signature;
	gint scope_definition_id = 0;
	gint scope_id = 0;
	gint type_id = 0;
	gint kind_id = 0;
	gint access_kind_id = 0;
	gint implementation_kind_id = 0;
	GValue *value1, *value2, *value3, *value4, *value5, *value6, *value7;
	gboolean sym_was_updated = FALSE;
	gint update_flag;
	GValue *ret_value;
	gboolean ret_bool;
	GTimer* timer = g_timer_new();
		
	g_return_val_if_fail (dbe != NULL, -1);
	priv = dbe->priv;

	/* keep it at 0 if sym_update == false */
	if (sym_update == FALSE)
		update_flag = 0;
	else
		update_flag = 1;

	g_return_val_if_fail (tag_entry != NULL, -1);

	/* parse the entry name */
	name = tag_entry->name;
	file_position = tag_entry->address.lineNumber;
	is_file_scope = tag_entry->fileScope;

	if ((tmp_str = tagsField (tag_entry, "signature")) != NULL)
	{
		signature = tmp_str;
	}
	else
	{
		signature = NULL;
	}

	type_id = sdb_engine_add_new_sym_type (dbe, tag_entry);

	/*DEBUG_PRINT ("add_symbol type_id: %f", g_timer_elapsed (timer, NULL));*/
	/* scope_definition_id tells what scope this symbol defines
	 * this call *MUST BE DONE AFTER* sym_type table population.
	 */
	scope_definition_id = sdb_engine_add_new_scope_definition (dbe, tag_entry,
															   type_id);

	/*DEBUG_PRINT ("add_symbol scope_definition_id: %f", g_timer_elapsed (timer, NULL));*/
	/* the container scopes can be: union, struct, typeref, class, namespace etc.
	 * this field will be parse in the second pass.
	 */
	scope_id = 0;

	kind_id = sdb_engine_add_new_sym_kind (dbe, tag_entry);
	/*DEBUG_PRINT ("add_symbol kind_id: %f", g_timer_elapsed (timer, NULL));*/
	
	access_kind_id = sdb_engine_add_new_sym_access (dbe, tag_entry);
	/*DEBUG_PRINT ("add_symbol access_kind_id: %f", g_timer_elapsed (timer, NULL));*/
	implementation_kind_id =
		sdb_engine_add_new_sym_implementation (dbe, tag_entry);
	/*DEBUG_PRINT ("add_symbol implementation_kind_id: %f", g_timer_elapsed (timer, NULL));*/
	/*DEBUG_PRINT ("add_symbol ids: %f", g_timer_elapsed (timer, NULL));*/
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
	
	if (update_flag == FALSE)
	{
		/* FIXME: are we sure to force this to -1? */
		symbol_id = -1;
	}
	else 
	{
		GList *sym_list;		
		MP_LEND_OBJ_STR (priv, value1);	
		g_value_set_static_string (value1, name);

		MP_LEND_OBJ_INT (priv, value2);
		g_value_set_int (value2, file_defined_id);

		MP_LEND_OBJ_INT (priv, value3);		
		g_value_set_int (value3, type_id);
		
		MP_LEND_OBJ_INT (priv, value4);		
		g_value_set_int (value4, file_position);		

		sym_list = g_tree_lookup (priv->file_symbols_cache, GINT_TO_POINTER(type_id));
		
		symbol_id = sdb_engine_get_tuple_id_by_unique_name4 (dbe,
								  PREP_QUERY_GET_SYMBOL_ID_BY_UNIQUE_INDEX_KEY_EXT,
								  "symname", value1,
								  "filedefid",value2,
								  "typeid", value3,
								  "filepos", value4);
		
		/* no luck, retry widely */
		if (symbol_id <= 0)
		{
			/* we should use more value and set them with the same values because
			 * sdb_engine_get_tuple_id_by_unique_name () will manage them
		 	 */
			MP_LEND_OBJ_STR (priv, value5);	
			g_value_set_static_string (value5, name);

			MP_LEND_OBJ_INT (priv, value6);
			g_value_set_int (value6, file_defined_id);

			MP_LEND_OBJ_INT (priv, value7);		
			g_value_set_int (value7, type_id);		

			symbol_id = sdb_engine_get_tuple_id_by_unique_name3 (dbe,
								  PREP_QUERY_GET_SYMBOL_ID_BY_UNIQUE_INDEX_KEY_EXT2,
								  "symname", value5,
								  "filedefid",value6,
								  "typeid", value7);			
		}

		if (symbol_id > 0) {
			/* attach to list the value of symbol: when this will have parsed all its
			 * symbols we'll be ready to check the lists with more than an element:
			 * those for sure will have an high probability for an updated scope.
			 */
			sym_list = g_list_prepend (sym_list, GINT_TO_POINTER(symbol_id));
			g_tree_insert (priv->file_symbols_cache, GINT_TO_POINTER(type_id), 
							   sym_list);
		}
	}
	/* ok then, parse the symbol id value */
	if (symbol_id <= 0)
	{
		/* case 2 and 3 */
		sym_was_updated = FALSE;

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
		
		MP_SET_HOLDER_BATCH_INT(priv, param, file_defined_id, ret_bool, ret_value);

		/* name parameter */
		if ((param = gda_set_get_holder ((GdaSet*)plist, "name")) == NULL)
		{
			g_warning ("param name is NULL from pquery!");			
			return -1;
		}
		
		MP_SET_HOLDER_BATCH_STR(priv, param, name, ret_bool, ret_value);		

		/* typeid parameter */
		if ((param = gda_set_get_holder ((GdaSet*)plist, "typeid")) == NULL)
		{
			g_warning ("param typeid is NULL from pquery!");
			return -1;			
		}
		
		MP_SET_HOLDER_BATCH_INT(priv, param, type_id, ret_bool, ret_value);		
	}
	else
	{
		/* case 1 */
		sym_was_updated = TRUE;

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

		MP_SET_HOLDER_BATCH_INT(priv, param, symbol_id, ret_bool, ret_value);
	}
	/*DEBUG_PRINT ("add_symbol init: %f", g_timer_elapsed (timer, NULL));*/
	/* common params */

	/* fileposition parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "fileposition")) == NULL)
	{
		g_warning ("param fileposition is NULL from pquery!");
		return -1;
	}
	
	MP_SET_HOLDER_BATCH_INT(priv, param, file_position, ret_bool, ret_value);
	
	/* isfilescope parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "isfilescope")) == NULL)	
	{
		g_warning ("param isfilescope is NULL from pquery!");
		return -1;
	}
	
	MP_SET_HOLDER_BATCH_INT(priv, param, is_file_scope, ret_bool, ret_value);
	
	/* signature parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "signature")) == NULL)	
	{
		g_warning ("param signature is NULL from pquery!");
		return -1;
	}
	
	MP_SET_HOLDER_BATCH_STR(priv, param, signature, ret_bool, ret_value);
	
	/* scopedefinitionid parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "scopedefinitionid")) == NULL)	
	{
		g_warning ("param scopedefinitionid is NULL from pquery!");
		return -1;
	}
	
	MP_SET_HOLDER_BATCH_INT(priv, param, scope_definition_id, ret_bool, ret_value);
	
	/* scopeid parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "scopeid")) == NULL)	
	{
		g_warning ("param scopeid is NULL from pquery!");
		return -1;
	}
	
	MP_SET_HOLDER_BATCH_INT(priv, param, scope_id, ret_bool, ret_value);
	
	/* kindid parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "kindid")) == NULL)	
	{
		g_warning ("param kindid is NULL from pquery!");
		return -1;
	}
	
	MP_SET_HOLDER_BATCH_INT(priv, param, kind_id, ret_bool, ret_value);
	
	/* accesskindid parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "accesskindid")) == NULL)	
	{
		g_warning ("param accesskindid is NULL from pquery!");
		return -1;
	}
	
	MP_SET_HOLDER_BATCH_INT(priv, param, access_kind_id, ret_bool, ret_value);

	/* implementationkindid parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "implementationkindid")) == NULL)	
	{
		g_warning ("param implementationkindid is NULL from pquery!");
		return -1;
	}
	
	MP_SET_HOLDER_BATCH_INT(priv, param, implementation_kind_id, ret_bool, ret_value);
	
	/* updateflag parameter */
	if ((param = gda_set_get_holder ((GdaSet*)plist, "updateflag")) == NULL)
	{
		g_warning ("param updateflag is NULL from pquery!");
		return -1;
	}
	
	MP_SET_HOLDER_BATCH_INT(priv, param, update_flag, ret_bool, ret_value);
	
	/*DEBUG_PRINT ("add_symbol batch: %f", g_timer_elapsed (timer, NULL));*/
	
	/* execute the query with parametes just set */
	gint nrows;
	nrows = gda_connection_statement_execute_non_select (priv->db_connection, 
													 (GdaStatement*)stmt, 
													 (GdaSet*)plist, &last_inserted,
													 NULL);
	/*DEBUG_PRINT ("add_symbol query: %f", g_timer_elapsed (timer, NULL));*/

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
			g_async_queue_push (priv->inserted_symbols_id, GINT_TO_POINTER(table_id));			
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
						
			g_async_queue_push (priv->updated_symbols_id, GINT_TO_POINTER(table_id));
		}
		else 
		{
			table_id = -1;
		}
	}
	
	if (last_inserted)
		g_object_unref (last_inserted);	
	
	/* before returning the table_id we have to fill some infoz on temporary tables
	 * so that in a second pass we can parse also the heritage and scope fields.
	 */
	if (table_id > 0)
		sdb_engine_add_new_tmp_heritage_scope (dbe, tag_entry, table_id);

	/*DEBUG_PRINT ("add_symbol end: %f", g_timer_elapsed (timer, NULL));*/
	g_timer_destroy (timer);
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
			DEBUG_PRINT ("nothing to remove");
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
		val = gda_data_model_get_value_at (data_model, 0, i, NULL);
		tmp = g_value_get_int (val);
	
		/*DEBUG_PRINT ("%s", "EMITTING symbol-removed");*/
		g_async_queue_push (priv->signals_queue, GINT_TO_POINTER(SYMBOL_REMOVED + 1));
		g_async_queue_push (priv->signals_queue, GINT_TO_POINTER(tmp));
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
	GValue *ret_value;
	gboolean ret_bool;

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
	
	MP_SET_HOLDER_BATCH_STR(priv, param, file_on_db, ret_bool, ret_value);
	
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
	MP_SET_HOLDER_BATCH_STR(priv, param, file_on_db, ret_bool, ret_value);
	
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
	
	MP_SET_HOLDER_BATCH_STR(priv, param, file_on_db, ret_bool, ret_value);

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
								  gint process_id,
								  UpdateFileSymbolsData* update_data)
{
	SymbolDBEnginePriv *priv;
	GPtrArray *files_to_scan;
	gint i;
	GValue *ret_value;
	gboolean ret_bool;

	DEBUG_PRINT ("%s", "");

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
			g_warning ("node %s is shorter than "
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
		
		MP_SET_HOLDER_BATCH_STR(priv, param, update_data->project, ret_bool, ret_value);
	
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

gint
symbol_db_engine_update_files_symbols (SymbolDBEngine * dbe, const gchar * project, 
									   GPtrArray * files_path,
									   gboolean update_prj_analyse_time)
{
	SymbolDBEnginePriv *priv;
	UpdateFileSymbolsData *update_data;
	gboolean ret_code;
	gint ret_id;
	gint i;
	GPtrArray * ready_files;
	
	priv = dbe->priv;

	g_return_val_if_fail (priv->db_connection != NULL, FALSE);
	g_return_val_if_fail (project != NULL, FALSE);

	ready_files = g_ptr_array_new ();
	
	/* check if the files exist in db before passing them to the scan procedure */
	for (i = 0; i < files_path->len; i++) 
	{
		gchar *curr_abs_file;
		
		curr_abs_file = g_ptr_array_index (files_path, i);
		/* check if the file exists in db. We will not scan buffers for files
		 * which aren't already in db
		 */
		if (symbol_db_engine_file_exists (dbe, curr_abs_file) == FALSE)
		{
			DEBUG_PRINT ("will not update file symbols claiming to be %s because not in db",
						 curr_abs_file);
			
			g_free (curr_abs_file);
			continue;
		}
		
		/* ok the file exists in db. Add it to ready_files */
		g_ptr_array_add (ready_files, curr_abs_file);
	}
	
	/* free just the array but not its values */
	g_ptr_array_free (files_path, FALSE);
	
	/* if no file has been added to the array then bail out here */
	if (ready_files->len <= 0)
	{
		g_ptr_array_free (ready_files, TRUE);
		DEBUG_PRINT ("not enough files to update");
		return -1;
	}
	
	update_data = g_new0 (UpdateFileSymbolsData, 1);
	
	update_data->update_prj_analyse_time = update_prj_analyse_time;
	update_data->files_path = ready_files;
	update_data->project = g_strdup (project);

	
	/* data will be freed when callback will be called. The signal will be
	 * disconnected too, don't worry about disconneting it by hand.
	 */
	g_signal_connect (G_OBJECT (dbe), "scan-end",
					  G_CALLBACK (on_scan_update_files_symbols_end), update_data);
	
	ret_code = sdb_engine_scan_files_1 (dbe, ready_files, NULL, TRUE);
	if (ret_code == TRUE)
		ret_id = sdb_engine_get_unique_scan_id (dbe);
	else
		ret_id = -1;
	
	return ret_id;
}

/* Update symbols of the whole project. It scans all file symbols etc. 
 * FIXME: libgda does not support nested prepared queries like 
 * PREP_QUERY_GET_ALL_FROM_FILE_BY_PROJECT_NAME. When it will do please
 * remember to update this function.
 */
gint
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
	GValue *ret_value;
	gboolean ret_bool;
	
	g_return_val_if_fail (dbe != NULL, FALSE);
	
	priv = dbe->priv;
	
	g_return_val_if_fail (project != NULL, FALSE);
	g_return_val_if_fail (priv->project_directory != NULL, FALSE);
	

	MP_LEND_OBJ_STR(priv, value);	
	g_value_set_static_string (value, project);

	/* get project id */
	if ((project_id = sdb_engine_get_tuple_id_by_unique_name (dbe,
									 PREP_QUERY_GET_PROJECT_ID_BY_UNIQUE_NAME,
									 "prjname",
									 value)) <= 0)
	{
		return FALSE;
	}

	if ((stmt = sdb_engine_get_statement_by_query_id (dbe,
								 PREP_QUERY_GET_ALL_FROM_FILE_BY_PROJECT_ID))
		== NULL)
	{
		g_warning ("query is null");
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
	
	MP_SET_HOLDER_BATCH_INT(priv, param, project_id, ret_bool, ret_value);	

	/* execute the query with parametes just set */
	data_model = gda_connection_statement_execute_select (priv->db_connection, 
												   (GdaStatement*)stmt, NULL, NULL);

	if (!GDA_IS_DATA_MODEL (data_model) ||
		(num_rows = gda_data_model_get_n_rows (GDA_DATA_MODEL (data_model))) <= 0)
	{
		if (data_model != NULL)
			g_object_unref (data_model);
		data_model = NULL;
	}

	/* initialize the array */
	files_to_scan = g_ptr_array_new ();

	/* we can now scan each filename entry to check the last modification time. */
	for (i = 0; i < num_rows; i++)
	{
		const GValue *value, *value1;
		const gchar *file_name;
		gchar *file_abs_path = NULL;
		struct tm filetm;
		time_t db_file_time;
		gchar *date_string;
		GFile *gfile;
		GFileInfo* gfile_info;
		GFileInputStream* gfile_is;

		if ((value =
			 gda_data_model_get_value_at (data_model, 
						gda_data_model_get_column_index(data_model,
												   "db_file_path"), 
										  i, NULL)) == NULL)
		{
			continue;
		}

		/* build abs path. */
		file_name = g_value_get_string (value);
		if (priv->project_directory != NULL)
		{
			file_abs_path = g_strdup_printf ("%s%s", priv->project_directory,
										file_name);
		}

		gfile = g_file_new_for_path (file_abs_path);
		if (gfile == NULL)
			continue;
		
		gfile_is = g_file_read (gfile, NULL, NULL);
		/* retrieve data/time info */
		if (gfile_is == NULL)
		{
			g_message ("could not open path %s", file_abs_path);
			g_free (file_abs_path);
			g_object_unref (gfile);
			continue;
		}
		g_object_unref (gfile_is);
				
		gfile_info = g_file_query_info (gfile, "*", G_FILE_QUERY_INFO_NONE,
										NULL, NULL);

		if (gfile_info == NULL)
		{
			g_message ("cannot get file info from handle");
			g_free (file_abs_path);
			g_object_unref (gfile);
			continue;
		}

		if ((value1 = gda_data_model_get_value_at (data_model, 
						gda_data_model_get_column_index(data_model,
												   "analyse_time"), i, NULL)) == NULL)
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

		
		if (difftime (db_file_time, g_file_info_get_attribute_uint32 (gfile_info, 
										  G_FILE_ATTRIBUTE_TIME_MODIFIED)) < 0)
		{
			g_ptr_array_add (files_to_scan, file_abs_path);
		}
		
		g_object_unref (gfile_info);
		g_object_unref (gfile);
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
	return -1;
}

gboolean
symbol_db_engine_remove_file (SymbolDBEngine * dbe, const gchar * project,
							  const gchar * abs_file)
{
	SymbolDBEnginePriv *priv;	
	const GdaSet *plist;
	const GdaStatement *stmt;
	GdaHolder *param;
	GValue *ret_value;
	gboolean ret_bool;

	
	g_return_val_if_fail (dbe != NULL, FALSE);
	g_return_val_if_fail (project != NULL, FALSE);
	g_return_val_if_fail (abs_file != NULL, FALSE);
	priv = dbe->priv;
	
	if (strlen (abs_file) < strlen (priv->project_directory)) 
	{
		g_warning ("wrong file to delete.");
		return FALSE;
	}
	
	DEBUG_PRINT ("deleting from db %s", abs_file);
	
	if ((stmt = sdb_engine_get_statement_by_query_id (dbe, 
									PREP_QUERY_REMOVE_FILE_BY_PROJECT_NAME)) == NULL)
	{
		g_warning ("query is null");
		return FALSE;
	}

	plist = sdb_engine_get_query_parameters_list (dbe, PREP_QUERY_REMOVE_FILE_BY_PROJECT_NAME);

	if ((param = gda_set_get_holder ((GdaSet*)plist, "prjname")) == NULL)
	{
		g_warning ("param prjname is NULL from pquery!");
		return FALSE;
	}
	
	MP_SET_HOLDER_BATCH_STR(priv, param, project, ret_bool, ret_value);
	
	if ((param = gda_set_get_holder ((GdaSet*)plist, "filepath")) == NULL)
	{
		g_warning ("param filepath is NULL from pquery!");
		return FALSE;
	}
	
	gchar * file_on_db = symbol_db_engine_get_file_db_path (dbe, abs_file);
	MP_SET_HOLDER_BATCH_STR(priv, param, file_on_db, ret_bool, ret_value);	

	/* Triggers will take care of updating/deleting connected symbols
	 * tuples, like sym_kind, sym_type etc */	
	gda_connection_statement_execute_non_select (priv->db_connection, (GdaStatement*)stmt, 
														 (GdaSet*)plist, NULL, NULL);	
	
	
	/* emits removed symbols signals */
	sdb_engine_detects_removed_ids (dbe);
	
	g_free (file_on_db);
	return TRUE;
}

void
symbol_db_engine_remove_files (SymbolDBEngine * dbe, const gchar * project,
							  const GPtrArray * files)
{
	SymbolDBEnginePriv *priv;	
	gint i;
	
	g_return_if_fail (dbe != NULL);
	g_return_if_fail (project != NULL);
	g_return_if_fail (files != NULL);
	priv = dbe->priv;

	for (i = 0; i < files->len; i++)
	{
		symbol_db_engine_remove_file (dbe, project, g_ptr_array_index (files, i));
	}	
}

static void
on_scan_update_buffer_end (SymbolDBEngine * dbe, gint process_id, gpointer data)
{
	SymbolDBEnginePriv *priv;
	GPtrArray *files_to_scan;
	gint i;

	g_return_if_fail (dbe != NULL);
	g_return_if_fail (data != NULL);

	priv = dbe->priv;
	files_to_scan = (GPtrArray *) data;

	for (i = 0; i < files_to_scan->len; i++)
	{
		gchar *node = (gchar *) g_ptr_array_index (files_to_scan, i);
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

gint
symbol_db_engine_update_buffer_symbols (SymbolDBEngine * dbe, const gchar *project,
										GPtrArray * real_files_list,
										const GPtrArray * text_buffers,
										const GPtrArray * buffer_sizes)
{
	SymbolDBEnginePriv *priv;
	gint i;
	gint ret_id;
	gboolean ret_code;
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
	
	DEBUG_PRINT ("%s", "");

	temp_files = g_ptr_array_new();	
	real_files_on_db = g_ptr_array_new();
	
	/* obtain a GPtrArray with real_files on database */
	for (i=0; i < real_files_list->len; i++) 
	{
		gchar *relative_path;
		const gchar *curr_abs_file;
		FILE *buffer_mem_file;
		const gchar *temp_buffer;
		gint buffer_mem_fd;
		gint temp_size;
		gchar *shared_temp_file;
		gchar *base_filename;
		
		curr_abs_file = g_ptr_array_index (real_files_list, i);
		/* check if the file exists in db. We will not scan buffers for files
		 * which aren't already in db
		 */
		if (symbol_db_engine_file_exists (dbe, curr_abs_file) == FALSE)
		{
			DEBUG_PRINT ("will not scan buffer claiming to be %s because not in db",
						 curr_abs_file);
			continue;
		}		
		
		relative_path = symbol_db_engine_get_file_db_path (dbe, curr_abs_file);
		if (relative_path == NULL)
		{
			g_warning ("symbol_db_engine_update_buffer_symbols  (): "
					   "relative_path is NULL");
			continue;
		}
		g_ptr_array_add (real_files_on_db, relative_path);

		/* it's ok to have just the base filename to create the
		 * target buffer one */
		base_filename = g_filename_display_basename (relative_path);
		
		shared_temp_file = g_strdup_printf ("/anjuta-%d-%ld-%s", getpid (),
						 time (NULL), base_filename);
		g_free (base_filename);
		
		if ((buffer_mem_fd = 
			 shm_open (shared_temp_file, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR)) < 0)
		{
			g_warning ("Error while trying to open a shared memory file. Be"
					   "sure to have "SHARED_MEMORY_PREFIX" mounted with tmpfs");
			return -1;
		}
	
		buffer_mem_file = fdopen (buffer_mem_fd, "w+b");
		
		temp_buffer = g_ptr_array_index (text_buffers, i);
		temp_size = GPOINTER_TO_INT(g_ptr_array_index (buffer_sizes, i));
		
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

	/* in case we didn't have any good buffer to scan...*/
	ret_id = -1;
	
	/* it may happens that no buffer is correctly set up */
	if (real_files_on_db->len > 0)
	{
		/* data will be freed when callback will be called. The signal will be
	 	* disconnected too, don't worry about disconnecting it by hand.
	 	*/
		g_signal_connect (G_OBJECT (dbe), "scan-end",
						  G_CALLBACK (on_scan_update_buffer_end), real_files_list);
	
		ret_code = sdb_engine_scan_files_1 (dbe, temp_files, real_files_on_db, TRUE);
		if (ret_code == TRUE)
			ret_id = sdb_engine_get_unique_scan_id (dbe);
		else
			ret_id = -1;
	}
	
	/* let's free the temp_files array */
	for (i=0; i < temp_files->len; i++)
		g_free (g_ptr_array_index (temp_files, i));
	
	g_ptr_array_free (temp_files, TRUE);
	
	/* and the real_files_on_db too */
	for (i=0; i < real_files_on_db->len; i++)
		g_free (g_ptr_array_index (real_files_on_db, i));
	
	g_ptr_array_free (real_files_on_db, TRUE);
	return ret_id;
}

