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
symbol.name = "First" and sym_type.type = "namespace";
our query is:
select * from symbol where scope_id = ;
at the end we have:

select * from symbol where scope_id = (select scope_definition_id from symbol join 
	sym_type on symbol.type_id = sym_type.type_id where symbol.name = 
	"First" and sym_type.type = "namespace");


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
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-launcher.h>
#include <libgda/libgda.h>
#include "readtags.h"
#include "symbol-db-engine.h"
#include "symbol-db-engine-iterator.h"

/* file should be specified without the ".db" extension. */
#define ANJUTA_DB_FILE	".anjuta_sym_db"

#define TABLES_SQL	ANJUTA_DATA_DIR"/tables.sql"

#define CTAGS_MARKER	"#_#\n"

// FIXME: detect it by prefs
#define CTAGS_PATH		"/usr/bin/ctags"


enum {
	DO_UPDATE_SYMS = 1,
	DONT_UPDATE_SYMS,
	DONT_FAKE_UPDATE_SYMS,
	END_UPDATE_GROUP_SYMS
};

typedef enum
{
	PREP_QUERY_WORKSPACE_NEW = 0,
	PREP_QUERY_GET_WORKSPACE_ID_BY_UNIQUE_NAME,
	PREP_QUERY_PROJECT_NEW,
	PREP_QUERY_GET_PROJECT_ID_BY_UNIQUE_NAME,
	PREP_QUERY_UPDATE_PROJECT_ANALIZE_TIME,
	PREP_QUERY_FILE_NEW,
	PREP_QUERY_GET_FILE_ID_BY_UNIQUE_NAME,
	PREP_QUERY_GET_ALL_FROM_FILE_BY_PROJECT_NAME,
	PREP_QUERY_GET_ALL_FROM_FILE_BY_PROJECT_ID,
	PREP_QUERY_UPDATE_FILE_ANALIZE_TIME,
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
	PREP_QUERY_GET_REMOVED_IDS,
	PREP_QUERY_TMP_REMOVED_DELETE_ALL,
	PREP_QUERY_COUNT
} query_type;


typedef struct _query_node
{
	query_type query_id;
	gchar *query_str;
	GdaQuery *query;

} query_node;


/* *MUST* respect query_type enum order. */
/* FIXME: Libgda does not yet support 'LIMIT' keyword. This can be used to
 * speed up some select(s). Please upgrade them as soon as the library support
 * it.
 */
static query_node query_list[PREP_QUERY_COUNT] = {
	/* -- workspace -- */
	{
	 PREP_QUERY_WORKSPACE_NEW,
	 "INSERT INTO workspace (workspace_name, analize_time) "
	 "VALUES (## /* name:'wsname' type:gchararray */,"
	 "datetime ('now', 'localtime'))",
	 NULL
	},
	{
	 PREP_QUERY_GET_WORKSPACE_ID_BY_UNIQUE_NAME,
	 "SELECT workspace_id FROM workspace WHERE workspace_name = ## /* name:'wsname' "
	 "type:gchararray */",
	 NULL
	},
	/* -- project -- */
	{
	 PREP_QUERY_PROJECT_NEW,
	 "INSERT INTO project (project_name, wrkspace_id, analize_time) "
	 "VALUES (## /* name:'prjname' type:gchararray */,"
	 "## /* name:'wsid' type:gint */, datetime ('now', 'localtime'))",
	 NULL
	},
	{
	 PREP_QUERY_GET_PROJECT_ID_BY_UNIQUE_NAME,
	 "SELECT project_id FROM project WHERE project_name = ## /* name:'prjname' "
	 "type:gchararray */",
	 NULL
	},
	{
	 PREP_QUERY_UPDATE_PROJECT_ANALIZE_TIME,
	 "UPDATE project SET analize_time = datetime('now', 'localtime') WHERE "
	 "project_name = ## /* name:'prjname' type:gchararray */",
	 NULL
	},
	/* -- file -- */
	{
	 PREP_QUERY_FILE_NEW,
	 "INSERT INTO file (file_path, prj_id, lang_id, analize_time) VALUES ("
	 "## /* name:'filepath' type:gchararray */, ## /* name:'prjid' "
	 "type:gint */, ## /* name:'langid' type:gint */, "
	 "datetime ('now', 'localtime'))",
	 NULL
	},
	{
	 PREP_QUERY_GET_FILE_ID_BY_UNIQUE_NAME,
	 "SELECT file_id FROM file WHERE file_path = ## /* name:'filepath' "
	 "type:gchararray */",
	 NULL
	},
	{
	 PREP_QUERY_GET_ALL_FROM_FILE_BY_PROJECT_NAME,
	 "SELECT * FROM file WHERE prj_id = (SELECT project_id FROM project "
	 "WHERE project_name = ## /* name:'prjname' type:gchararray */)",
	 NULL
	},
	{
	 PREP_QUERY_GET_ALL_FROM_FILE_BY_PROJECT_ID,
	 "SELECT * FROM file WHERE prj_id = ## /* name:'prjid' type:gint */",
/*		
		"SELECT * FROM file JOIN project on project_id = prj_id WHERE "\
		"project.name = ## /* name:'prjname' type:gchararray * /",
*/
	 NULL
	},
	{
	 PREP_QUERY_UPDATE_FILE_ANALIZE_TIME,
	 "UPDATE file SET analize_time = datetime('now', 'localtime') WHERE "
	 "file_path = ## /* name:'filepath' type:gchararray */",
	 NULL},
	/* -- language -- */
	{
	 PREP_QUERY_LANGUAGE_NEW,
	 "INSERT INTO language (language_name) VALUES (## /* name:'langname' "
	 "type:gchararray */)",
	 NULL
	},
	{
	 PREP_QUERY_GET_LANGUAGE_ID_BY_UNIQUE_NAME,
	 "SELECT language_id FROM language WHERE language_name = ## /* name:'langname' "
	 "type:gchararray */",
	 NULL
	},
	/* -- sym type -- */
	{
	 PREP_QUERY_SYM_TYPE_NEW,
	 "INSERT INTO sym_type (type, type_name) VALUES (## /* name:'type' "
	 "type:gchararray */, ## /* name:'typename' type:gchararray */)",
	 NULL
	},
	{
	 PREP_QUERY_GET_SYM_TYPE_ID,
	 "SELECT type_id FROM sym_type WHERE type = ## /* name:'type' "
	 "type:gchararray */ AND type_name = ## /* name:'typename' "
	 "type:gchararray */",
	 NULL
	},
	/* -- sym kind -- */
	{
	 PREP_QUERY_SYM_KIND_NEW,
	 "INSERT INTO sym_kind (kind_name) VALUES(## /* name:'kindname' "
	 "type:gchararray */)",
	 NULL
	},
	{
	 PREP_QUERY_GET_SYM_KIND_BY_UNIQUE_NAME,
	 "SELECT sym_kind_id FROM sym_kind WHERE kind_name = ## /* "
	 "name:'kindname' type:gchararray */",
	 NULL
	},
	/* -- sym access -- */
	{
	 PREP_QUERY_SYM_ACCESS_NEW,
	 "INSERT INTO sym_access (access_name) VALUES(## /* name:'accesskind' "
	 "type:gchararray */)",
	 NULL
	},
	{
	 PREP_QUERY_GET_SYM_ACCESS_BY_UNIQUE_NAME,
	 "SELECT access_kind_id FROM sym_access WHERE access_name = ## /* "
	 "name:'accesskind' type:gchararray */",
	 NULL
	},
	/* -- sym implementation -- */
	{
	 PREP_QUERY_SYM_IMPLEMENTATION_NEW,
	 "INSERT INTO sym_implementation (implementation_name) VALUES(## /* name:'implekind' "
	 "type:gchararray */)",
	 NULL
	},
	{
	 PREP_QUERY_GET_SYM_IMPLEMENTATION_BY_UNIQUE_NAME,
	 "SELECT sym_impl_id FROM sym_implementation WHERE kind = ## /* "
	 "name:'implekind' type:gchararray */",
	 NULL
	},
	/* -- heritage -- */
	{
	 PREP_QUERY_HERITAGE_NEW,
	 "INSERT INTO heritage (symbol_id_base, symbol_id_derived) VALUES(## /* "
	 "name:'symbase' type:gint */, ## /* name:'symderived' type:gint */)",
	 NULL
	},
	/* -- scope -- */
	{
	 PREP_QUERY_SCOPE_NEW,
	 "INSERT INTO scope (scope, type_id) VALUES(## /* name:'scope' "
	 "type:gchararray */, ## /* name:'typeid' type:gint */)",
	 NULL
	},
	{
	 PREP_QUERY_GET_SCOPE_ID,
	 "SELECT scope_id FROM scope WHERE scope = ## /* name:'scope' "
	 "type:gchararray */ AND type_id = ## /* name:'typeid' type:gint */",
	 NULL
	},
	/* -- tmp heritage -- */
	{
	 PREP_QUERY_TMP_HERITAGE_NEW,
	 "INSERT INTO __tmp_heritage_scope (symbol_referer_id, field_inherits, "
	 "field_struct, field_typeref, field_enum, field_union, "
	 "field_class, field_namespace) VALUES (## /* name:'symreferid' "
	 "type:gint */, ## /* name:'finherits' type:gchararray */, ## /* "
	 "name:'fstruct' type:gchararray */, ## /* name:'ftyperef' "
	 "type:gchararray */, ## /* name:'fenum' type:gchararray */, ## /* "
	 "name:'funion' type:gchararray */, ## /* name:'fclass' type:gchararray "
	 "*/, ## /* name:'fnamespace' type:gchararray */)",
	 NULL
	},
	{
	 PREP_QUERY_GET_ALL_FROM_TMP_HERITAGE,
	 "SELECT * FROM __tmp_heritage_scope",
	 NULL
	},
	{
	 PREP_QUERY_GET_ALL_FROM_TMP_HERITAGE_WITH_INHERITS,
	 "SELECT * FROM __tmp_heritage_scope WHERE field_inherits != ''",
	 NULL
	},
	{
	 PREP_QUERY_TMP_HERITAGE_DELETE_ALL,
	 "DELETE FROM __tmp_heritage_scope",
	 NULL
	},
	/* -- symbol -- */
	{
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
	 "name:'updateflag' type:gint */)",
	 NULL
	},
	{
	 PREP_QUERY_GET_SYMBOL_SCOPE_DEFINITION_ID,
	 "SELECT scope_definition_id FROM symbol JOIN sym_type ON symbol.type_id"
	 "= sym_type.type_id WHERE sym_type.type = ## /* name:'tokenname' "
	 "type:gchararray */ AND sym_type.type_name = ## /* name:'objectname' "
	 "type:gchararray */",
	 NULL
	},
	{
	 PREP_QUERY_GET_SYMBOL_ID_BY_CLASS_NAME,
	 "SELECT symbol_id FROM symbol JOIN sym_type ON symbol.type_id = "
	 "sym_type.type_id WHERE scope_id=0 AND sym_type.type='class' AND "
	 "name = ## /* name:'klassname' type:gchararray */",
	 NULL
	},
	{
	 PREP_QUERY_GET_SYMBOL_ID_BY_CLASS_NAME_AND_NAMESPACE,
	 "SELECT symbol_id FROM symbol JOIN scope ON symbol.scope_id = "
	 "scope.scope_id JOIN sym_type ON scope.type_id = sym_type.type_id "
	 "WHERE symbol.name = /* name:'klassname' type:gchararray */ AND "
	 "scope.scope = /* name:'namespacename' type:gchararray */ AND "
	 "sym_type.type='namespace'",
	 NULL
	},
	{
	 PREP_QUERY_UPDATE_SYMBOL_SCOPE_ID,
	 "UPDATE symbol SET scope_id = ## /* name:'scopeid' type:gint */ "
	 "WHERE symbol_id = ## /* name:'symbolid' type:gint */",
	 NULL
	},
	{
	 PREP_QUERY_GET_SYMBOL_ID_BY_UNIQUE_INDEX_KEY,
	 "SELECT symbol_id FROM symbol WHERE name = ## /* name:'symname' "
	 "type:gchararray */ AND file_defined_id =  ## /* name:'filedefid' "
	 "type:gint */ AND type_id = ## /* name:'typeid' type:gint */",
	 NULL
	},
	{
	 PREP_QUERY_UPDATE_SYMBOL_ALL,
	 "UPDATE symbol SET is_file_scope = ## /* name:'isfilescope' type:gint "
	 "*/, signature = ## /* name:'signature' type:gchararray */, "
	 "scope_definition_id = ## /* name:'scopedefinitionid' type:gint */, "
	 "scope_id = ## /* name:'scopeid' type:gint */, kind_id = "
	 "## /* name:'kindid' type:gint */, access_kind_id = ## /* name:"
	 "'accesskindid' type:gint */, implementation_kind_id = ## /* name:"
	 "'implementationkindid' type:gint */, update_flag = ## /* name:"
	 "'updateflag' type:gint */ WHERE symbol_id = ## /* name:'symbolid' type:"
	 "gint */",
	 NULL
	},
	/* -- tmp_removed -- */
	{
	 PREP_QUERY_GET_REMOVED_IDS,
	 "SELECT symbol_removed_id FROM __tmp_removed",
	 NULL
	},
	{
	 PREP_QUERY_TMP_REMOVED_DELETE_ALL,
	 "DELETE FROM __tmp_removed",
	 NULL
	}	  
};


/* this is a special case which is not doable with a prepared query. */
static GdaCommand *last_insert_id_cmd = NULL;

typedef void (SymbolDBEngineCallback) (SymbolDBEngine * dbe,
									   gpointer user_data);

enum
{
	SCAN_END,
	SYMBOL_INSERTED,
	SYMBOL_UPDATED,
	SYMBOL_REMOVED,
	LAST_SIGNAL
};

static unsigned int signals[LAST_SIGNAL] = { 0 };


struct _SymbolDBEnginePriv
{
	GdaConnection *db_connection;
	GdaClient *gda_client;
	GdaDict *dict;
	gchar *dsn_name;
	gchar *project_name;
	gchar *data_source;

	GAsyncQueue *scan_queue;
	
	GAsyncQueue *updated_symbols_id;
	GAsyncQueue *inserted_symbols_id;
	
	gchar *shared_mem_str;
	FILE *shared_mem_file;
	gint shared_mem_fd;
	AnjutaLauncher *ctags_launcher;
	gboolean scanning_status;
	gboolean force_sym_update;
};

static GObjectClass *parent_class = NULL;


static void sdb_engine_second_pass_do (SymbolDBEngine * dbe);
static gint
sdb_engine_add_new_symbol (SymbolDBEngine * dbe, tagEntry * tag_entry,
						   gchar * base_prj_path, gchar * fake_file,
						   gboolean sym_update);


/**
 * Use this as little as you can. Prepared statements are quicker.
 */
static void inline
sdb_engine_execute_non_select_sql (SymbolDBEngine * dbe, const gchar * buffer)
{
	SymbolDBEnginePriv *priv;
	GdaCommand *command;

	priv = dbe->priv;

	command = gda_command_new (buffer, GDA_COMMAND_TYPE_SQL,
							   GDA_COMMAND_OPTION_STOP_ON_ERRORS);
	gda_connection_execute_non_select_command (priv->db_connection,
											   command, NULL, NULL);
	gda_command_free (command);
}

/**
 * Will test the opened project within the dbe plugin and the passed one.
 */
gboolean inline
symbol_db_engine_is_project_opened (SymbolDBEngine *dbe, const gchar* project_name)
{
	SymbolDBEnginePriv *priv;

	g_return_val_if_fail (dbe != NULL, FALSE);
	priv = dbe->priv;
	
	if (priv->project_name == NULL)
		return FALSE;
	
	return strcmp (project_name, priv->project_name) == 0 ? TRUE : FALSE;
}

/**
 * Use a proxy to return an already present or a fresh new prepared query 
 * from static 'query_list'. We should perform actions in the fastest way, because
 * these queries are time-critical.
 */

static inline const GdaQuery *
sdb_engine_get_query_by_id (SymbolDBEngine * dbe, query_type query_id)
{
	query_node *node;
	SymbolDBEnginePriv *priv;

	priv = dbe->priv;

	/* no way: if connection is NULL we will break here. There must be
	 * a connection established to db before using this function */
	g_return_val_if_fail (priv->db_connection != NULL, NULL);

	node = &query_list[query_id];

	/* check for a null dict into priv struct */
	if (priv->dict == NULL)
	{
		priv->dict = gda_dict_new ();
		gda_dict_set_connection (priv->dict, priv->db_connection);
	}

	if (node->query == NULL)
	{
		DEBUG_PRINT ("generating new query.... %d", query_id);
		/* create a new GdaQuery */
		node->query =
			gda_query_new_from_sql (priv->dict, node->query_str, NULL);
	}

	return node->query;
}

static inline gint
sdb_engine_get_last_insert_id (SymbolDBEngine * dbe)
{
	GdaDataModel *data;
	const GValue *num;
	gint table_id;
	SymbolDBEnginePriv *priv;

	priv = dbe->priv;

	/* check whether we have already initialized it or not. */
	if (last_insert_id_cmd == NULL)
	{
		DEBUG_PRINT ("creating new command...");
		last_insert_id_cmd = gda_command_new ("SELECT last_insert_rowid()",
											  GDA_COMMAND_TYPE_SQL,
											  GDA_COMMAND_OPTION_STOP_ON_ERRORS);
	}

	if ((data = gda_connection_execute_select_command (priv->db_connection,
													   last_insert_id_cmd, NULL,
													   NULL)) == NULL
		|| gda_data_model_get_n_rows (data) <= 0)
	{
		return -1;
	}

	num = gda_data_model_get_value_at (data, 0, 0);
	if (G_VALUE_HOLDS (num, G_TYPE_INT))
		table_id = g_value_get_int (num);
	else
		return -1;

	g_object_unref (data);
	return table_id;
}

/**
 * Clear the static cached queries data. You should call this function when closing/
 * destroying SymbolDBEngine object.
 * priv->dict is also destroyed.
 */
static void
sdb_engine_free_cached_queries (SymbolDBEngine * dbe)
{
	gint i;
	query_node *node;
	SymbolDBEnginePriv *priv;

	priv = dbe->priv;

	for (i = 0; i < PREP_QUERY_COUNT; i++)
	{
		node = &query_list[i];

		if (node->query != NULL)
		{
			g_object_unref ((gpointer) node->query);
			node->query = NULL;
		}
	}

	/* destroys also dict */
	g_object_unref (priv->dict);
}

static gboolean
sdb_engine_disconnect_from_db (SymbolDBEngine * dbe)
{
	SymbolDBEnginePriv *priv;

	g_return_val_if_fail (dbe != NULL, FALSE);
	priv = dbe->priv;

	if (priv->gda_client)
	{
		gda_client_close_all_connections (priv->gda_client);
		g_object_unref (priv->gda_client);
	}
	priv->gda_client = NULL;	
	priv->db_connection = NULL;

	g_free (priv->data_source);
	priv->data_source = NULL;

	g_free (priv->dsn_name);
	priv->dsn_name = NULL;

	if (priv->dict)
	{
		g_object_unref (priv->dict);
		priv->dict = NULL;
	}


	return TRUE;
}

static GTimer *sym_timer_DEBUG  = NULL;

/**
 * If base_prj_path != NULL then fake_file will not be parsed. Else
 * if fake_file is != NULL we claim and assert that tags contents which are
 * scanned belong to the fake_file in the project.
 * More: the fake_file refers to just one single file and cannot be used
 * for multiple fake_files.
 */
static void
sdb_engine_populate_db_by_tags (SymbolDBEngine * dbe, FILE* fd,
								gchar * base_prj_path, gchar * fake_file_on_db,
								gboolean force_sym_update)
{
	tagFile *tag_file;
	tagFileInfo tag_file_info;
	tagEntry tag_entry;

	SymbolDBEnginePriv *priv;

	g_return_if_fail (dbe != NULL);
	
	priv = dbe->priv;

	g_return_if_fail (priv->db_connection != NULL);
	g_return_if_fail (fd != NULL);

	if (priv->updated_symbols_id == NULL)
		priv->updated_symbols_id = g_async_queue_new ();
	
	if (priv->inserted_symbols_id == NULL)
		priv->inserted_symbols_id = g_async_queue_new ();
	
	DEBUG_PRINT ("sdb_engine_populate_db_by_tags ()");
	if ((tag_file = tagsOpen_1 (fd, &tag_file_info)) == NULL)
		g_warning ("error in opening ctags file");

	gda_connection_begin_transaction (priv->db_connection, "fixme", 0, NULL);

	if (sym_timer_DEBUG == NULL)
		sym_timer_DEBUG = g_timer_new ();
	else
		g_timer_reset (sym_timer_DEBUG);
	gint tags_total_DEBUG = 0;
	
	while (tagsNext (tag_file, &tag_entry) != TagFailure)
	{
		sdb_engine_add_new_symbol (dbe, &tag_entry, fake_file_on_db == NULL ?
								   base_prj_path : NULL, fake_file_on_db,
								   force_sym_update);
		
		tags_total_DEBUG ++;
	}
	
	gdouble elapsed_DEBUG = g_timer_elapsed (sym_timer_DEBUG, NULL);
	DEBUG_PRINT ("elapsed: %f for (%d) [%f per symbol]", elapsed_DEBUG,
				 tags_total_DEBUG, elapsed_DEBUG / tags_total_DEBUG);
	
	gda_connection_commit_transaction (priv->db_connection, "fixme", NULL);
}

static void
sdb_engine_ctags_output_callback_1 (AnjutaLauncher * launcher,
								  AnjutaLauncherOutputType output_type,
								  const gchar * chars, gpointer user_data)
{
	gint len_chars;
	gint remaining_chars;
	gint len_marker;
	gchar *chars_ptr;
	SymbolDBEnginePriv *priv;
	SymbolDBEngine *dbe;
		
	g_return_if_fail (user_data != NULL);	
	g_return_if_fail (chars != NULL);	
	
	dbe = (SymbolDBEngine*)user_data;
	priv = dbe->priv;
	
	chars_ptr = (gchar*)chars;
	
	remaining_chars = len_chars = strlen (chars);
	len_marker = strlen (CTAGS_MARKER);	

/*	DEBUG_PRINT ("gotta %s", chars_ptr);*/
	
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
				DEBUG_PRINT ("found marker!");
		
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
				sdb_engine_populate_db_by_tags (dbe, priv->shared_mem_file, 
							priv->data_source, 
							(int)real_file == DONT_FAKE_UPDATE_SYMS ? NULL : real_file, 
							scan_flag == DO_UPDATE_SYMS ? TRUE : FALSE);
				
				/* don't forget to free the real_life, if it's a char */
				if ((int)real_file != DONT_FAKE_UPDATE_SYMS)
					g_free (real_file);
				
				/* check also if, together with an end file marker, we have an 
				 * end group-of-files end marker.
				 */
				if ((strcmp (marker_ptr + len_marker, CTAGS_MARKER) == 0) ||
					ftell (priv->shared_mem_file) <= 0)
				{
					gint tmp_inserted;
					gint tmp_updated;
					
					/* proceed with second passes */
					DEBUG_PRINT ("FOUND end-of-group-files marker.\n"
								 "go on with sdb_engine_second_pass_do ()");
					sdb_engine_second_pass_do (dbe);				
					
					chars_ptr += len_marker;
					remaining_chars -= len_marker;


					/* Here we are. It's the right time to notify the listeners
					 * about out fresh new inserted/updated symbols...
					 * Go on by emitting them.
					 */
					while ((tmp_inserted = (int)
							g_async_queue_try_pop (priv->inserted_symbols_id)) > 0)
					{
						g_signal_emit (dbe, signals[SYMBOL_INSERTED], 0, tmp_inserted);
					}
					
					while ((tmp_updated = (int)
							g_async_queue_try_pop (priv->updated_symbols_id)) > 0)
					{
						g_signal_emit (dbe, signals[SYMBOL_UPDATED], 0, tmp_updated);
					}
					
					/* emit signal. The end of files-group can be cannot be
					 * determined by the caller. This is the only way.
					 */
					DEBUG_PRINT ("EMITTING scan-end");
					g_signal_emit (dbe, signals[SCAN_END], 0);
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
		} while (remaining_chars + len_marker < len_chars);
	}
	else 
	{
		DEBUG_PRINT ("no len_chars > len_marker");
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

		g_signal_connect (G_OBJECT (priv->ctags_launcher), "child-exited",
						  G_CALLBACK (on_scan_files_end_1), NULL);

		exe_string = g_strdup_printf ("%s --fields=afmiKlnsStz "
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
		temp_file = g_strdup_printf ("/anjuta-%d_%ld.tags", getpid (),
							 time (NULL));

		priv->shared_mem_str = temp_file;
		
		if ((priv->shared_mem_fd = 
			 shm_open (temp_file, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR)) < 0)
		{
			g_warning ("Error while trying to open a shared memory file. Be"
					   "sure to have /dev/shm mounted with tmpfs");
		}
	
		priv->shared_mem_file = fdopen (priv->shared_mem_fd, "a+b");
		DEBUG_PRINT ("temp_file %s", temp_file);

		/* no need to free temp_file. It will be freed on plugin finalize */
	}
	
	priv->scanning_status = TRUE;	

	DEBUG_PRINT ("files_list->len %d", files_list->len);
	
	for (i = 0; i < files_list->len; i++)
	{
		gchar *node = (gchar *) g_ptr_array_index (files_list, i);
		
		if (g_file_test (node, G_FILE_TEST_EXISTS) == FALSE)
		{
			g_warning ("File %s not scanned because it does not exist", node);
			continue;
		}
		DEBUG_PRINT ("anjuta_launcher_send_stdin %s", node);
		anjuta_launcher_send_stdin (priv->ctags_launcher, node);
		anjuta_launcher_send_stdin (priv->ctags_launcher, "\n");

		if (symbols_update == TRUE)
			g_async_queue_push (priv->scan_queue, (gpointer) DO_UPDATE_SYMS);
		else
			g_async_queue_push (priv->scan_queue, (gpointer) DONT_UPDATE_SYMS);

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
			/* else add a DONT_FAKE_UPDATE_SYMS marker, just to noty that this is
			 * not a fake file scan 
			 */
			g_async_queue_push (priv->scan_queue, (gpointer) DONT_FAKE_UPDATE_SYMS);
		}
	}

	/* hack to let ctags output a marker. We will then process it into the
	 * output callback function */
	anjuta_launcher_send_stdin (priv->ctags_launcher, "/dev/null\n");
	
	priv->scanning_status = FALSE;

	return TRUE;
}



static void
sdb_engine_init (SymbolDBEngine * object)
{
	SymbolDBEngine *sdbe;

	sdbe = SYMBOL_DB_ENGINE (object);
	sdbe->priv = g_new0 (SymbolDBEnginePriv, 1);

	/* initialize some priv data */
	sdbe->priv->gda_client = NULL;
	sdbe->priv->db_connection = NULL;
	sdbe->priv->dsn_name = NULL;
	sdbe->priv->project_name = NULL;
	sdbe->priv->data_source = NULL;

	sdbe->priv->scan_queue = NULL;	
	sdbe->priv->updated_symbols_id = NULL;
	sdbe->priv->inserted_symbols_id = NULL;
	sdbe->priv->shared_mem_file = NULL;
	sdbe->priv->shared_mem_fd = 0;
	sdbe->priv->shared_mem_str = NULL;
	sdbe->priv->scanning_status = FALSE;
	sdbe->priv->force_sym_update = FALSE;

	/* Initialize gda library. */
	gda_init ("AnjutaGda", NULL, 0, NULL);

	/* create Anjuta Launcher instance. It will be used for tags parsing. */
	sdbe->priv->ctags_launcher = NULL;
}

static void
sdb_engine_finalize (GObject * object)
{
	SymbolDBEngine *dbe;
	SymbolDBEnginePriv *priv;
	
	dbe = SYMBOL_DB_ENGINE (object);	
	priv = dbe->priv;

	sdb_engine_disconnect_from_db (dbe);
	sdb_engine_free_cached_queries (dbe);
	
	g_free (priv->project_name);

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
	
	if (priv->ctags_launcher)
	{
		anjuta_launcher_signal (priv->ctags_launcher, SIGINT);
		g_object_unref (priv->ctags_launcher);
	}	
	
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
sdb_engine_class_init (SymbolDBEngineClass * klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));

	object_class->finalize = sdb_engine_finalize;

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

	sdbe = g_object_new (SYMBOL_TYPE_DB_ENGINE, NULL);
	return sdbe;
}

/* Will create priv->db_connection, priv->gda_client.
 * Connect to database identified by data_source.
 * Usually data_source is defined also into priv. We let it here as parameter 
 * because it is required and cannot be null.
 */
static gboolean
sdb_engine_connect_to_db (SymbolDBEngine * dbe, const gchar * data_source)
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

	/* create new client */
	priv->gda_client = gda_client_new ();

	/* establish a connection. If the sqlite file does not exist it will 
	 * be created 
	 */
	priv->db_connection
		= gda_client_open_connection (priv->gda_client, data_source,
									  "", "",
									  GDA_CONNECTION_OPTIONS_READ_ONLY, NULL);

	if (!GDA_IS_CONNECTION (priv->db_connection))
	{
		g_warning ("could not open connection to %s\n", data_source);
		return FALSE;
	}

	g_message ("connected to database %s", data_source);
	return TRUE;
}


/**
 * Creates required tables for the database to work.
 * @param tables_sql_file File containing sql code.
 */
static gboolean
sdb_engine_create_db_tables (SymbolDBEngine * dbe, gchar * tables_sql_file)
{
	GError *err;
	GdaCommand *command;
	SymbolDBEnginePriv *priv;
	gchar *contents;
	gsize size;

	g_return_val_if_fail (tables_sql_file != NULL, FALSE);

	priv = dbe->priv;

	g_return_val_if_fail (priv->db_connection != NULL, FALSE);

	/* read the contents of the file */
	if (g_file_get_contents (tables_sql_file, &contents, &size, &err) == FALSE)
	{
		g_warning ("Something went wrong while trying to read %s",
				   tables_sql_file);

		if (err != NULL)
			g_message ("%s", err->message);
		return FALSE;
	}

	command = gda_command_new (contents, GDA_COMMAND_TYPE_SQL,
							   GDA_COMMAND_OPTION_STOP_ON_ERRORS);
	gda_connection_execute_non_select_command (priv->db_connection, command,
											   NULL, NULL);
	gda_command_free (command);

	g_free (contents);
	return TRUE;
}

/**
 * Check if the database already exists into the prj_directory
 */
gboolean
symbol_db_engine_db_exists (SymbolDBEngine * dbe, gchar * prj_directory)
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

	DEBUG_PRINT ("db %s does exist", tmp_file);
	return TRUE;
}

/**
 * Open or create a new database at given directory.
 */
gboolean
symbol_db_engine_open_db (SymbolDBEngine * dbe, gchar * prj_directory)
{
	SymbolDBEnginePriv *priv;
	/* Connection data */
	gchar *dsn_name;
	gboolean needs_tables_creation = FALSE;

	g_return_val_if_fail (prj_directory != NULL, FALSE);

	priv = dbe->priv;

	/* check whether the db filename already exists. If it's not the case
	 * create the tables for the database. */
	gchar *tmp_file = g_strdup_printf ("%s/%s.db", prj_directory,
									   ANJUTA_DB_FILE);

	if (g_file_test (tmp_file, G_FILE_TEST_EXISTS) == FALSE)
	{
		needs_tables_creation = TRUE;
	}
	g_free (tmp_file);


	priv->data_source = g_strdup (prj_directory);

	dsn_name = g_strdup_printf ("DB_DIR=%s;DB_NAME=%s", prj_directory,
								ANJUTA_DB_FILE);

	if (gda_config_save_data_source (priv->data_source, "SQLite",
									 dsn_name, "Anjuta Project",
									 "", "", FALSE) == FALSE)
	{
		return FALSE;
	}

	/* store the dsn name into Priv data. We can avoid to free it here coz it will
	 * used later again. */
	priv->dsn_name = dsn_name;

	DEBUG_PRINT ("opening/connecting to database...");
	sdb_engine_connect_to_db (dbe, priv->data_source);

	if (needs_tables_creation == TRUE)
	{
		DEBUG_PRINT ("creating tables: it needs tables...");
		sdb_engine_create_db_tables (dbe, TABLES_SQL);
	}

	return TRUE;
}

/**
 * @return -1 on error. Otherwise the id of table
 */
static gint
sdb_engine_get_table_id_by_unique_name (SymbolDBEngine * dbe, query_type qtype,
										gchar * param_key,
										const GValue * param_value)
{
	const GdaQuery *query;
	GdaParameterList *par_list;
	GdaParameter *param;
	const GValue *num;
	gint table_id;

	/* get prepared query */
	if ((query = sdb_engine_get_query_by_id (dbe, qtype)) == NULL)
	{
		g_warning ("query is null");
		return -1;
	}

	if (GDA_QUERY_TYPE_NON_PARSED_SQL
		== gda_query_get_query_type ((GdaQuery *) query))
	{
		g_warning ("sdb_engine_get_table_id_by_unique_name: non parsed "
				   "sql error");
		return -1;
	}

	if ((par_list = gda_query_get_parameter_list ((GdaQuery *) query)) == NULL)
	{
		g_warning ("par_list is NULL!\n");
		return -1;
	}

	if ((param = gda_parameter_list_find_param (par_list, param_key)) == NULL)
	{
		g_warning ("sdb_engine_get_table_id_by_unique_name: param is NULL "
				   "from pquery!\n");
		return -1;
	}

	gda_parameter_set_value (param, param_value);

	/* execute the query with parametes just set */
	GObject *obj =
		G_OBJECT (gda_query_execute ((GdaQuery *) query, par_list, FALSE,
									 NULL));

	if (!GDA_IS_DATA_MODEL (obj) ||
		gda_data_model_get_n_rows (GDA_DATA_MODEL (obj)) <= 0)
	{
/*		DEBUG_PRINT ("could not retrieve table id by unique name [par %s]", 
						param_key);*/

		if (obj != NULL)
			g_object_unref (obj);
		return -1;
	}

	/* get and parse the results. */
	num = gda_data_model_get_value_at (GDA_DATA_MODEL (obj), 0, 0);

	table_id = g_value_get_int (num);
	g_object_unref (obj);
	return table_id;
}

/**
 * This is the same as sdb_engine_get_table_id_by_unique_name () but for two
 * unique parameters. This should be the quickest way. Surely quicker than
 * use g_strdup_printf () with a va_list for example.
 * @return -1 on error. Otherwise the id of table
 *
 */
static gint
sdb_engine_get_table_id_by_unique_name2 (SymbolDBEngine * dbe, query_type qtype,
										 gchar * param_key1,
										 const GValue * value1,
										 gchar * param_key2,
										 const GValue * value2)
{
	const GdaQuery *query;
	GdaParameterList *par_list;
	GdaParameter *param;
	const GValue *num;
	gint table_id;

	/* get prepared query */
	if ((query = sdb_engine_get_query_by_id (dbe, qtype)) == NULL)
	{
		g_warning ("query is null");
		return -1;
	}

	if (GDA_QUERY_TYPE_NON_PARSED_SQL
		== gda_query_get_query_type ((GdaQuery *) query))
	{
		g_warning
			("sdb_engine_get_table_id_by_unique_name2: non parsed sql error");
		return -1;
	}

	if ((par_list = gda_query_get_parameter_list ((GdaQuery *) query)) == NULL)
	{
		g_warning ("par_list is NULL!\n");
		return -1;
	}

	/* look for and set the first parameter */
	if ((param = gda_parameter_list_find_param (par_list, param_key1)) == NULL)
	{
		g_warning ("sdb_engine_get_table_id_by_unique_name2: "
				   "param is NULL from pquery! [par1: %s] [par2: %s]\n",
				   param_key1, param_key2);
		return -1;
	}

	gda_parameter_set_value (param, value1);

	/* ...and the second one */
	if ((param = gda_parameter_list_find_param (par_list, param_key2)) == NULL)
	{
		g_warning ("param is NULL from pquery!\n");
		return -1;
	}

	gda_parameter_set_value (param, value2);

	/* execute the query with parametes just set */
	GObject *obj =
		G_OBJECT (gda_query_execute ((GdaQuery *) query, par_list, FALSE,
									 NULL));

	if (!GDA_IS_DATA_MODEL (obj) ||
		gda_data_model_get_n_rows (GDA_DATA_MODEL (obj)) <= 0)
	{

		if (obj != NULL)
			g_object_unref (obj);
		return -1;
	}

	/* get and parse the results. */
	num = gda_data_model_get_value_at (GDA_DATA_MODEL (obj), 0, 0);

	table_id = g_value_get_int (num);
	g_object_unref (obj);
	return table_id;
}

static gint
sdb_engine_get_table_id_by_unique_name3 (SymbolDBEngine * dbe, query_type qtype,
										 gchar * param_key1,
										 const GValue * value1,
										 gchar * param_key2,
										 const GValue * value2,
										 gchar * param_key3,
										 const GValue * value3)
{
	const GdaQuery *query;
	GdaParameterList *par_list;
	GdaParameter *param;
	const GValue *num;
	gint table_id;

	/* get prepared query */
	if ((query = sdb_engine_get_query_by_id (dbe, qtype)) == NULL)
	{
		g_warning ("query is null");
		return -1;
	}

	if (GDA_QUERY_TYPE_NON_PARSED_SQL
		== gda_query_get_query_type ((GdaQuery *) query))
	{
		g_warning
			("sdb_engine_get_table_id_by_unique_name2: non parsed sql error");
		return -1;
	}

	if ((par_list = gda_query_get_parameter_list ((GdaQuery *) query)) == NULL)
	{
		g_warning ("par_list is NULL!\n");
		return -1;
	}

	/* look for and set the first parameter */
	if ((param = gda_parameter_list_find_param (par_list, param_key1)) == NULL)
	{
		g_warning ("sdb_engine_get_table_id_by_unique_name2: "
				   "param is NULL from pquery! [par1: %s] [par2: %s]\n",
				   param_key1, param_key2);
		return -1;
	}

	gda_parameter_set_value (param, value1);

	/* ...and the second one */
	if ((param = gda_parameter_list_find_param (par_list, param_key2)) == NULL)
	{
		g_warning ("param is NULL from pquery!\n");
		return -1;
	}

	gda_parameter_set_value (param, value2);

	/* ...and the third one */
	if ((param = gda_parameter_list_find_param (par_list, param_key3)) == NULL)
	{
		g_warning ("param is NULL from pquery!\n");
		return -1;
	}

	gda_parameter_set_value (param, value3);


	/* execute the query with parametes just set */
	GObject *obj =
		G_OBJECT (gda_query_execute ((GdaQuery *) query, par_list, FALSE,
									 NULL));

	if (!GDA_IS_DATA_MODEL (obj) ||
		gda_data_model_get_n_rows (GDA_DATA_MODEL (obj)) <= 0)
	{

		if (obj != NULL)
			g_object_unref (obj);
		return -1;
	}

	/* get and parse the results. */
	num = gda_data_model_get_value_at (GDA_DATA_MODEL (obj), 0, 0);

	table_id = g_value_get_int (num);
	g_object_unref (obj);
	return table_id;
}


gboolean
symbol_db_engine_add_new_workspace (SymbolDBEngine * dbe,
									gchar * workspace_name)
{
/*
CREATE TABLE workspace (workspace_id integer PRIMARY KEY AUTOINCREMENT,
                        workspace_name varchar (50) not null unique,
                        analize_time DATE
                        );
*/
	const GdaQuery *query;
	GdaParameterList *par_list;
	GdaParameter *param;
	GValue *value;
	SymbolDBEnginePriv *priv;

	priv = dbe->priv;

	g_return_val_if_fail (priv->db_connection != NULL, FALSE);

	if ((query =
		 sdb_engine_get_query_by_id (dbe, PREP_QUERY_WORKSPACE_NEW)) == NULL)
	{
		g_warning ("query is null");
		return FALSE;
	}

	if (GDA_QUERY_TYPE_NON_PARSED_SQL ==
		gda_query_get_query_type ((GdaQuery *) query))
	{
		g_warning ("non parsed sql error");
		return FALSE;
	}

	if ((par_list = gda_query_get_parameter_list ((GdaQuery *) query)) == NULL)
	{
		g_warning ("par_list is NULL!\n");
		return FALSE;
	}

	if ((param = gda_parameter_list_find_param (par_list, "wsname")) == NULL)
	{
		g_warning ("param is NULL from pquery!\n");
		return FALSE;
	}

	value = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value, workspace_name);

	gda_parameter_set_value (param, value);

	/* execute the query with parametes just set */
	gda_query_execute ((GdaQuery *) query, par_list, FALSE, NULL);
	gda_value_free (value);

	return TRUE;
}

/**
 * Return the name of the opened project.
 * NULL on error. Returned string must be freed by caller.
 */
gchar*
symbol_db_engine_get_opened_project_name (SymbolDBEngine * dbe)
{
	SymbolDBEnginePriv *priv;
	g_return_val_if_fail (dbe != NULL, NULL);
	priv = dbe->priv;
	
	g_return_val_if_fail (priv->db_connection != NULL, NULL);
	
	return g_strdup (priv->project_name);
}

/**
 * Open a new project.
 * It will test if project was correctly created. 
 */
gboolean
symbol_db_engine_open_project (SymbolDBEngine * dbe,	/*gchar* workspace, */
							   const gchar * project_name)
{
	GValue *value;
	SymbolDBEnginePriv *priv;
	gint prj_id;

	priv = dbe->priv;

	g_return_val_if_fail (priv->db_connection != NULL, FALSE);
	if (symbol_db_engine_is_project_opened (dbe, project_name) == TRUE) {
		g_warning ("project already opened, %s (priv %s)", project_name, 
				   priv->project_name);
		return FALSE;
	}

	value = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value, project_name);

	/* test the existence of the project in db */
	if ((prj_id = sdb_engine_get_table_id_by_unique_name (dbe,
				PREP_QUERY_GET_PROJECT_ID_BY_UNIQUE_NAME,
				"prjname",
				 value)) <= 0)
	{
		gda_value_free (value);
		return FALSE;
	}

	gda_value_free (value);

	/* open the project... */
	priv->project_name = g_strdup (project_name);

	return TRUE;
}


gboolean 
symbol_db_engine_close_project (SymbolDBEngine *dbe, gchar* project_name)
{
	SymbolDBEnginePriv *priv;

	priv = dbe->priv;

	g_free (priv->project_name);
	priv->project_name = NULL;				

	return sdb_engine_disconnect_from_db (dbe);
}


/**
 * @param workspace Can be NULL. In that case a default workspace will be created, 
 * 					and project will depend on that.
 * @param project Project name. Must NOT be NULL.
 */
// FIXME: file_include file_ignore
gboolean
symbol_db_engine_add_new_project (SymbolDBEngine * dbe, gchar * workspace,
								  gchar * project)
{
/*
CREATE TABLE project (project_id integer PRIMARY KEY AUTOINCREMENT,
                      project_name varchar (50) not null unique,
                      wrkspace_id integer REFERENCES workspace (workspace_id),
                      analize_time DATE
                      );
*/
	const GdaQuery *query;
	GdaParameterList *par_list;
	GdaParameter *param;
	GValue *value;
	gchar *workspace_name;
	gint wks_id;
	SymbolDBEnginePriv *priv;

	g_return_val_if_fail (dbe != NULL, FALSE);
	
	priv = dbe->priv;

	if (symbol_db_engine_is_project_opened (dbe, project) == TRUE) 
	{
		g_warning ("You have an already opened project. Cannot continue.");
		return FALSE;
	}
	
	if (workspace == NULL)
	{
		workspace_name = "anjuta_workspace_default";

		DEBUG_PRINT ("adding default workspace...");
		if (symbol_db_engine_add_new_workspace (dbe, workspace_name) == FALSE)
		{
			DEBUG_PRINT ("Project cannot be added because a default workspace "
						 "cannot be created");
			return FALSE;
		}
	}
	else
	{
		workspace_name = workspace;
	}

	value = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value, workspace_name);

	/* get workspace id */
	if ((wks_id = sdb_engine_get_table_id_by_unique_name (dbe,
				 PREP_QUERY_GET_WORKSPACE_ID_BY_UNIQUE_NAME,
				 "wsname",
				 value)) <= 0)
	{
		DEBUG_PRINT ("no workspace id");
		gda_value_free (value);
		return FALSE;
	}

	/* insert new project */
	if ((query =
		 sdb_engine_get_query_by_id (dbe, PREP_QUERY_PROJECT_NEW)) == NULL)
	{
		g_warning ("query is null");
		gda_value_free (value);
		return FALSE;
	}

	if (GDA_QUERY_TYPE_NON_PARSED_SQL ==
		gda_query_get_query_type ((GdaQuery *) query))
	{
		g_warning ("non parsed sql error");
		gda_value_free (value);
		return FALSE;
	}

	if ((par_list = gda_query_get_parameter_list ((GdaQuery *) query)) == NULL)
	{
		g_warning ("par_list is NULL!\n");
		gda_value_free (value);
		return FALSE;
	}

	if ((param = gda_parameter_list_find_param (par_list, "prjname")) == NULL)
	{
		g_warning ("param prjname is NULL from pquery!");
		gda_value_free (value);
		return FALSE;
	}

	gda_value_reset_with_type (value, G_TYPE_STRING);
	g_value_set_string (value, project);
	gda_parameter_set_value (param, value);

	if ((param = gda_parameter_list_find_param (par_list, "wsid")) == NULL)
	{
		g_warning ("param is NULL from pquery!");
		return FALSE;
	}

	gda_value_reset_with_type (value, G_TYPE_INT);
	g_value_set_int (value, wks_id);

	gda_parameter_set_value (param, value);

	/* execute the query with parametes just set */
	gda_query_execute ((GdaQuery *) query, par_list, FALSE, NULL);
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

	g_return_val_if_fail (language != NULL, -1);

	value = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value, language);

	/* check for an already existing table with language "name". */
	if ((table_id = sdb_engine_get_table_id_by_unique_name (dbe,
						PREP_QUERY_GET_LANGUAGE_ID_BY_UNIQUE_NAME,
						"langname",
						value)) < 0)
	{

		/* insert a new entry on db */
		const GdaQuery *query;
		GdaParameterList *par_list;
		GdaParameter *param;
		GValue *value;

		if ((query = sdb_engine_get_query_by_id (dbe, PREP_QUERY_LANGUAGE_NEW))
			== NULL)
		{
			g_warning ("query is null");
			return FALSE;
		}

		if (GDA_QUERY_TYPE_NON_PARSED_SQL ==
			gda_query_get_query_type ((GdaQuery *) query))
		{
			g_warning ("non parsed sql error");
			return FALSE;
		}

		if ((par_list =
			 gda_query_get_parameter_list ((GdaQuery *) query)) == NULL)
		{
			g_warning ("par_list is NULL!\n");
			return FALSE;
		}

		if ((param =
			 gda_parameter_list_find_param (par_list, "langname")) == NULL)
		{
			g_warning ("param langname is NULL from pquery!");
			return FALSE;
		}

		value = gda_value_new (G_TYPE_STRING);
		g_value_set_string (value, language);

		gda_parameter_set_value (param, value);
		gda_value_free (value);
		/* execute the query with parametes just set */
		gda_query_execute ((GdaQuery *) query, par_list, FALSE, NULL);

		table_id = sdb_engine_get_last_insert_id (dbe);
	}
	gda_value_free (value);
	return table_id;
}


/* Add a file to project. 
 * This function requires an opened db, i.e. calling before
 * symbol_db_engine_open_db ()
 * filepath: referes to a full file path.
 * WARNING: we suppose that project is already opened.
 */
static gboolean
sdb_engine_add_new_file (SymbolDBEngine * dbe, const gchar * project,
						 const gchar * filepath, const gchar * language)
{
/*
CREATE TABLE file (file_id integer PRIMARY KEY AUTOINCREMENT,
                   file_path TEXT not null unique,
                   prj_id integer REFERENCES project (projec_id),
                   lang_id integer REFERENCES language (language_id),
                   analize_time DATE
                   );
*/
	SymbolDBEnginePriv *priv;
	gint project_id;
	gint language_id;
	gint file_id;
	GValue *value;
	
	priv = dbe->priv;
	
	value = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value, project);

	/* check for an already existing table with project "project". */
	if ((project_id = sdb_engine_get_table_id_by_unique_name (dbe,
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
	 * e.g.: we have a file on disk: "/tmp/foo/src/file.c" and a datasource located on
	 * "/tmp/foo". The entry on db will be "src/file.c" 
	 */
	g_value_set_string (value, filepath + strlen(priv->data_source));

	if ((file_id = sdb_engine_get_table_id_by_unique_name (dbe,
								   PREP_QUERY_GET_FILE_ID_BY_UNIQUE_NAME,
								   "filepath",
								   value)) < 0)
	{
		/* insert a new entry on db */
		const GdaQuery *query;
		GdaParameterList *par_list;
		GdaParameter *param;
		GValue *value;

		language_id = sdb_engine_add_new_language (dbe, language);

		if ((query = sdb_engine_get_query_by_id (dbe, PREP_QUERY_FILE_NEW))
			== NULL)
		{
			g_warning ("query is null");
			return FALSE;
		}

		if (GDA_QUERY_TYPE_NON_PARSED_SQL ==
			gda_query_get_query_type ((GdaQuery *) query))
		{
			g_warning ("non parsed sql error");
			return FALSE;
		}

		if ((par_list =
			 gda_query_get_parameter_list ((GdaQuery *) query)) == NULL)
		{
			g_warning ("par_list is NULL!\n");
			return FALSE;
		}

		/* filepath parameter */
		if ((param =
			 gda_parameter_list_find_param (par_list, "filepath")) == NULL)
		{
			g_warning ("param filepath is NULL from pquery!");
			return FALSE;
		}

		value = gda_value_new (G_TYPE_STRING);
		/* relative filepath */
		g_value_set_string (value, filepath + strlen(priv->data_source));
		gda_parameter_set_value (param, value);

		/* project id parameter */
		if ((param = gda_parameter_list_find_param (par_list, "prjid")) == NULL)
		{
			g_warning ("param prjid is NULL from pquery!");
			return FALSE;
		}

		gda_value_reset_with_type (value, G_TYPE_INT);
		g_value_set_int (value, project_id);
		gda_parameter_set_value (param, value);

		/* language id parameter */
		if ((param =
			 gda_parameter_list_find_param (par_list, "langid")) == NULL)
		{
			g_warning ("param langid is NULL from pquery!");
			return FALSE;
		}

		gda_value_reset_with_type (value, G_TYPE_INT);
		g_value_set_int (value, language_id);
		gda_parameter_set_value (param, value);
		gda_value_free (value);

		/* execute the query with parametes just set */
		gda_query_execute ((GdaQuery *) query, par_list, FALSE, NULL);
	}
	gda_value_free (value);

	return TRUE;
}


static void
sdb_prepare_executing_commands (SymbolDBEngine *dbe)
{
	GdaCommand *command;
	SymbolDBEnginePriv *priv;
	priv = dbe->priv;

	command = gda_command_new ("PRAGMA page_size = 4096", GDA_COMMAND_TYPE_SQL,
							   GDA_COMMAND_OPTION_STOP_ON_ERRORS);
	gda_connection_execute_non_select_command (priv->db_connection,
											   command, NULL, NULL);
	gda_command_free (command);
	
	command = gda_command_new ("PRAGMA cache_size = 10000", GDA_COMMAND_TYPE_SQL,
							   GDA_COMMAND_OPTION_STOP_ON_ERRORS);
	gda_connection_execute_non_select_command (priv->db_connection,
											   command, NULL, NULL);
	gda_command_free (command);

	command = gda_command_new ("PRAGMA synchronous = OFF", GDA_COMMAND_TYPE_SQL,
							   GDA_COMMAND_OPTION_STOP_ON_ERRORS);
	gda_connection_execute_non_select_command (priv->db_connection,
											   command, NULL, NULL);
	gda_command_free (command);
	
	command = gda_command_new ("PRAGMA temp_store = MEMORY", GDA_COMMAND_TYPE_SQL,
							   GDA_COMMAND_OPTION_STOP_ON_ERRORS);
	gda_connection_execute_non_select_command (priv->db_connection,
											   command, NULL, NULL);
	gda_command_free (command);
	
	
}

gboolean
symbol_db_engine_add_new_files (SymbolDBEngine * dbe, const gchar * project,
								const GPtrArray * files_path, const gchar * language,
								gboolean scan_symbols)
{
	gint i;
	SymbolDBEnginePriv *priv;

	g_return_val_if_fail (dbe != NULL, FALSE);
	priv = dbe->priv;

	g_return_val_if_fail (priv->db_connection != NULL, FALSE);
	g_return_val_if_fail (project != NULL, FALSE);

	/* FIXME try this... */
	//sdb_prepare_executing_commands (dbe);
	
	
	if (symbol_db_engine_is_project_opened (dbe, project) == FALSE) 
	{
		g_warning ("your project isn't opened, %s (priv %s)", project, 
				   priv->project_name);
		return FALSE;
	}	
	
	for (i = 0; i < files_path->len; i++)
	{
		gchar *node = (gchar *) g_ptr_array_index (files_path, i);
		
		/* test the existance of node file */
		if (g_file_test (node, G_FILE_TEST_EXISTS) == FALSE)
		{
			g_warning ("File %s doesn't exist", node);
			continue;
		}
			
/*		DEBUG_PRINT ("gonna adding symbols for %s", node);*/
		
		if (sdb_engine_add_new_file (dbe, project, node, language) == FALSE)
		{
			g_warning ("Error processing file %s", node);
			return FALSE;
		}
	}


	/* perform the scan of files. It will spawn a fork() process with 
	 * AnjutaLauncher and ctags in server mode. After the ctags cmd has been 
	 * executed, the populating process'll take place.
	 */
	if (scan_symbols)
		return sdb_engine_scan_files_1 (dbe, files_path, NULL, FALSE);
	
	return TRUE;
}


static gint
sdb_engine_add_new_sym_type (SymbolDBEngine * dbe, tagEntry * tag_entry)
{
/*
	CREATE TABLE sym_type (type_id integer PRIMARY KEY AUTOINCREMENT,
                   type varchar (256) not null ,
                   type_name varchar (256) not null ,
                   unique (type, type_name)
                   );
*/
	const gchar *type;
	const gchar *type_name;
	gint table_id;
	GValue *value1, *value2;

	g_return_val_if_fail (tag_entry != NULL, -1);

	type = tag_entry->kind;
	type_name = tag_entry->name;

	value1 = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value1, type);

	value2 = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value2, type_name);

	if ((table_id = sdb_engine_get_table_id_by_unique_name2 (dbe,
													 PREP_QUERY_GET_SYM_TYPE_ID,
													 "type", value1,
													 "typename",
													 value2)) < 0)
	{
		const GdaQuery *query;
		GdaParameterList *par_list;
		GdaParameter *param;
		GValue *value;

		/* it does not exist. Create a new tuple. */
		if ((query = sdb_engine_get_query_by_id (dbe, PREP_QUERY_SYM_TYPE_NEW))
			== NULL)
		{
			g_warning ("query is null");
			return -1;
		}

		if (GDA_QUERY_TYPE_NON_PARSED_SQL ==
			gda_query_get_query_type ((GdaQuery *) query))
		{
			g_warning ("non parsed sql error");
			return -1;
		}

		if ((par_list =
			 gda_query_get_parameter_list ((GdaQuery *) query)) == NULL)
		{
			g_warning ("par_list is NULL!\n");
			return -1;
		}

		/* type parameter */
		if ((param = gda_parameter_list_find_param (par_list, "type")) == NULL)
		{
			g_warning ("param type is NULL from pquery!");
			return -1;
		}

		value = gda_value_new (G_TYPE_STRING);
		g_value_set_string (value, type);
		gda_parameter_set_value (param, value);

		/* type_name parameter */
		if ((param =
			 gda_parameter_list_find_param (par_list, "typename")) == NULL)
		{
			g_warning ("param typename is NULL from pquery!");
			return -1;
		}

		gda_value_reset_with_type (value, G_TYPE_STRING);
		g_value_set_string (value, type_name);
		gda_parameter_set_value (param, value);
		gda_value_free (value);

		/* execute the query with parametes just set */
		gda_query_execute ((GdaQuery *) query, par_list, FALSE, NULL);

		table_id = sdb_engine_get_last_insert_id (dbe);
	}

	gda_value_free (value1);
	gda_value_free (value2);

	return table_id;
}

static gint
sdb_engine_add_new_sym_kind (SymbolDBEngine * dbe, tagEntry * tag_entry)
{
/* 
	CREATE TABLE sym_kind (sym_kind_id integer PRIMARY KEY AUTOINCREMENT,
                       kind_name varchar (50) not null unique
                       );
*/
	const gchar *kind_name;
	gint table_id;
	GValue *value;

	g_return_val_if_fail (tag_entry != NULL, -1);

	kind_name = tag_entry->kind;

	/* no kind associated with current tag */
	if (kind_name == NULL)
		return -1;

	value = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value, kind_name);

	if ((table_id = sdb_engine_get_table_id_by_unique_name (dbe,
										PREP_QUERY_GET_SYM_KIND_BY_UNIQUE_NAME,
										"kindname",
										value)) < 0)
	{
		const GdaQuery *query;
		GdaParameterList *par_list;
		GdaParameter *param;
		GValue *value;

		/* not found. Go on with inserting  */
		if ((query = sdb_engine_get_query_by_id (dbe, PREP_QUERY_SYM_KIND_NEW))
			== NULL)
		{
			g_warning ("query is null");
			return -1;
		}

		if (GDA_QUERY_TYPE_NON_PARSED_SQL ==
			gda_query_get_query_type ((GdaQuery *) query))
		{
			g_warning ("non parsed sql error");
			return -1;
		}

		if ((par_list =
			 gda_query_get_parameter_list ((GdaQuery *) query)) == NULL)
		{
			g_warning ("par_list is NULL!\n");
			return -1;
		}

		/* kindname parameter */
		if ((param =
			 gda_parameter_list_find_param (par_list, "kindname")) == NULL)
		{
			g_warning ("param kindname is NULL from pquery!");
			return -1;
		}

		value = gda_value_new (G_TYPE_STRING);
		g_value_set_string (value, kind_name);
		gda_parameter_set_value (param, value);
		gda_value_free (value);

		/* execute the query with parametes just set */
		gda_query_execute ((GdaQuery *) query, par_list, FALSE, NULL);

		table_id = sdb_engine_get_last_insert_id (dbe);
	}
	gda_value_free (value);
	return table_id;
}

static gint
sdb_engine_add_new_sym_access (SymbolDBEngine * dbe, tagEntry * tag_entry)
{
/* 
	CREATE TABLE sym_access (access_kind_id integer PRIMARY KEY AUTOINCREMENT,
                         access_name varchar (50) not null unique
                         );
*/
	const gchar *access;
	gint table_id;
	GValue *value;

	g_return_val_if_fail (tag_entry != NULL, -1);

	if ((access = tagsField (tag_entry, "access")) == NULL)
	{
		/* no access associated with current tag */
		return -1;
	}

	value = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value, access);

	if ((table_id = sdb_engine_get_table_id_by_unique_name (dbe,
									PREP_QUERY_GET_SYM_ACCESS_BY_UNIQUE_NAME,
									"accesskind",
									value)) < 0)
	{
		const GdaQuery *query;
		GdaParameterList *par_list;
		GdaParameter *param;
		GValue *value;

		/* not found. Go on with inserting  */
		if ((query =
			 sdb_engine_get_query_by_id (dbe,
										 PREP_QUERY_SYM_ACCESS_NEW)) == NULL)
		{
			g_warning ("query is null");
			return -1;
		}

		if (GDA_QUERY_TYPE_NON_PARSED_SQL ==
			gda_query_get_query_type ((GdaQuery *) query))
		{
			g_warning ("non parsed sql error");
			return -1;
		}

		if ((par_list =
			 gda_query_get_parameter_list ((GdaQuery *) query)) == NULL)
		{
			g_warning ("par_list is NULL!\n");
			return -1;
		}

		/* accesskind parameter */
		if ((param =
			 gda_parameter_list_find_param (par_list, "accesskind")) == NULL)
		{
			g_warning ("param accesskind is NULL from pquery!");
			return -1;
		}

		value = gda_value_new (G_TYPE_STRING);
		g_value_set_string (value, access);
		gda_parameter_set_value (param, value);
		gda_value_free (value);

		/* execute the query with parametes just set */
		gda_query_execute ((GdaQuery *) query, par_list, FALSE, NULL);

		table_id = sdb_engine_get_last_insert_id (dbe);
	}
	gda_value_free (value);
	return table_id;
}

static gint
sdb_engine_add_new_sym_implementation (SymbolDBEngine * dbe,
									   tagEntry * tag_entry)
{
/*	
	CREATE TABLE sym_implementation (sym_impl_id integer PRIMARY KEY AUTOINCREMENT,
                                 implementation_name varchar (50) not null unique
                                 );
*/
	const gchar *implementation;
	gint table_id;
	GValue *value;

	g_return_val_if_fail (tag_entry != NULL, -1);

	if ((implementation = tagsField (tag_entry, "implementation")) == NULL)
	{
		/* no implementation associated with current tag */
		return -1;
	}

	value = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value, implementation);

	if ((table_id = sdb_engine_get_table_id_by_unique_name (dbe,
							PREP_QUERY_GET_SYM_IMPLEMENTATION_BY_UNIQUE_NAME,
							"implekind",
							value)) < 0)
	{
		const GdaQuery *query;
		GdaParameterList *par_list;
		GdaParameter *param;
		GValue *value;

		/* not found. Go on with inserting  */
		if ((query =
			 sdb_engine_get_query_by_id (dbe,
										 PREP_QUERY_SYM_IMPLEMENTATION_NEW)) ==
			NULL)
		{
			g_warning ("query is null");
			return -1;
		}

		if (GDA_QUERY_TYPE_NON_PARSED_SQL ==
			gda_query_get_query_type ((GdaQuery *) query))
		{
			g_warning ("non parsed sql error");
			return -1;
		}

		if ((par_list =
			 gda_query_get_parameter_list ((GdaQuery *) query)) == NULL)
		{
			g_warning ("par_list is NULL!\n");
			return -1;
		}

		/* implekind parameter */
		if ((param =
			 gda_parameter_list_find_param (par_list, "implekind")) == NULL)
		{
			g_warning ("param implekind is NULL from pquery!");
			return -1;
		}

		value = gda_value_new (G_TYPE_STRING);
		g_value_set_string (value, implementation);
		gda_parameter_set_value (param, value);
		gda_value_free (value);

		/* execute the query with parametes just set */
		gda_query_execute ((GdaQuery *) query, par_list, FALSE, NULL);

		table_id = sdb_engine_get_last_insert_id (dbe);
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
	const GdaQuery *query;
	GdaParameterList *par_list;
	GdaParameter *param;
	GValue *value;

	g_return_if_fail (base_symbol_id > 0);
	g_return_if_fail (derived_symbol_id > 0);

	if ((query = sdb_engine_get_query_by_id (dbe, PREP_QUERY_HERITAGE_NEW))
		== NULL)
	{
		g_warning ("query is null");
		return;
	}

	if (GDA_QUERY_TYPE_NON_PARSED_SQL ==
		gda_query_get_query_type ((GdaQuery *) query))
	{
		g_warning ("non parsed sql error");
		return;
	}

	if ((par_list = gda_query_get_parameter_list ((GdaQuery *) query)) == NULL)
	{
		g_warning ("par_list is NULL!\n");
		return;
	}

	/* symbase parameter */
	if ((param = gda_parameter_list_find_param (par_list, "symbase")) == NULL)
	{
		g_warning ("param symbase is NULL from pquery!");
		return;
	}

	value = gda_value_new (G_TYPE_INT);
	g_value_set_int (value, base_symbol_id);
	gda_parameter_set_value (param, value);

	/* symderived id parameter */
	if ((param =
		 gda_parameter_list_find_param (par_list, "symderived")) == NULL)
	{
		g_warning ("param symderived is NULL from pquery!");
		return;
	}

	gda_value_reset_with_type (value, G_TYPE_INT);
	g_value_set_int (value, derived_symbol_id);
	gda_parameter_set_value (param, value);
	gda_value_free (value);

	/* execute the query with parametes just set */
	gda_query_execute ((GdaQuery *) query, par_list, FALSE, NULL);
}


static gint
sdb_engine_add_new_scope_definition (SymbolDBEngine * dbe, tagEntry * tag_entry)
{
/*
	CREATE TABLE scope (scope_id integer PRIMARY KEY AUTOINCREMENT,
                    scope varchar(256) not null,
                    type_id integer REFERENCES sym_type (type_id),
					unique (scope, type_id)
                    );
*/
	const gchar *scope;
	GValue *value1, *value2;
	gint type_table_id, table_id;

	g_return_val_if_fail (tag_entry != NULL, -1);
	g_return_val_if_fail (tag_entry->kind != NULL, -1);

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

	value1 = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value1, tag_entry->kind);

	value2 = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value2, scope);

	/* search for type_id on sym_type table */
	if ((type_table_id = sdb_engine_get_table_id_by_unique_name2 (dbe,
												  PREP_QUERY_GET_SYM_TYPE_ID,
												  "type",
												  value1,
												  "typename",
												  value2)) < 0)
	{
		/* no type has been found. */
		gda_value_free (value1);
		gda_value_free (value2);

		return -1;
	}

	/* let's check for an already present scope table with scope and type_id infos. */
	gda_value_reset_with_type (value1, G_TYPE_STRING);
	g_value_set_string (value1, scope);

	gda_value_reset_with_type (value2, G_TYPE_INT);
	g_value_set_int (value2, type_table_id);

	if ((table_id = sdb_engine_get_table_id_by_unique_name2 (dbe,
												 PREP_QUERY_GET_SCOPE_ID,
												 "scope", value1,
												 "typeid",
												 value2)) < 0)
	{
		/* no luck. A table with the pair scope and type_id is not present. 
		 * ok, let's insert out new scope table.
		 */
		const GdaQuery *query;
		GdaParameterList *par_list;
		GdaParameter *param;
		GValue *value;

		if ((query = sdb_engine_get_query_by_id (dbe, PREP_QUERY_SCOPE_NEW))
			== NULL)
		{
			g_warning ("query is null");
			return -1;
		}

		if (GDA_QUERY_TYPE_NON_PARSED_SQL ==
			gda_query_get_query_type ((GdaQuery *) query))
		{
			g_warning ("non parsed sql error");
			return -1;
		}

		if ((par_list =
			 gda_query_get_parameter_list ((GdaQuery *) query)) == NULL)
		{
			g_warning ("par_list is NULL!\n");
			return -1;
		}

		/* scope parameter */
		if ((param = gda_parameter_list_find_param (par_list, "scope")) == NULL)
		{
			g_warning ("param scope is NULL from pquery!");
			return -1;
		}

		value = gda_value_new (G_TYPE_STRING);
		g_value_set_string (value, scope);
		gda_parameter_set_value (param, value);

		/* typeid parameter */
		if ((param =
			 gda_parameter_list_find_param (par_list, "typeid")) == NULL)
		{
			g_warning ("param typeid is NULL from pquery!");
			return -1;
		}

		gda_value_reset_with_type (value, G_TYPE_INT);
		g_value_set_int (value, type_table_id);
		gda_parameter_set_value (param, value);
		gda_value_free (value);

		/* execute the query with parametes just set */
		gda_query_execute ((GdaQuery *) query, par_list, FALSE, NULL);

		table_id = sdb_engine_get_last_insert_id (dbe);
	}

	gda_value_free (value1);
	gda_value_free (value2);
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
									   tagEntry * tag_entry,
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
	const GdaQuery *query;
	GdaParameterList *par_list;
	GdaParameter *param;
	gint table_id;
	GValue *value;
	const gchar *field_inherits, *field_struct, *field_typeref,
		*field_enum, *field_union, *field_class, *field_namespace;
	gboolean good_tag;

	g_return_val_if_fail (tag_entry != NULL, -1);

	/* init the flag */
	good_tag = FALSE;

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


	if ((query = sdb_engine_get_query_by_id (dbe, PREP_QUERY_TMP_HERITAGE_NEW))
		== NULL)
	{
		g_warning ("query is null");
		return -1;
	}

	if (GDA_QUERY_TYPE_NON_PARSED_SQL ==
		gda_query_get_query_type ((GdaQuery *) query))
	{
		g_warning ("non parsed sql error");
		return -1;
	}

	if ((par_list = gda_query_get_parameter_list ((GdaQuery *) query)) == NULL)
	{
		g_warning ("par_list is NULL!\n");
		return -1;
	}

	/* symreferid parameter */
	if ((param =
		 gda_parameter_list_find_param (par_list, "symreferid")) == NULL)
	{
		g_warning ("param symreferid is NULL from pquery!");
		return -1;
	}

	value = gda_value_new (G_TYPE_INT);
	g_value_set_int (value, symbol_referer_id);
	gda_parameter_set_value (param, value);

	/* finherits parameter */
	if ((param = gda_parameter_list_find_param (par_list, "finherits")) == NULL)
	{
		g_warning ("param finherits is NULL from pquery!");
		return -1;
	}

	gda_value_reset_with_type (value, G_TYPE_STRING);
	g_value_set_string (value, field_inherits);
	gda_parameter_set_value (param, value);

	/* fstruct parameter */
	if ((param = gda_parameter_list_find_param (par_list, "fstruct")) == NULL)
	{
		g_warning ("param fstruct is NULL from pquery!");
		return -1;
	}

	gda_value_reset_with_type (value, G_TYPE_STRING);
	g_value_set_string (value, field_struct);
	gda_parameter_set_value (param, value);

	/* ftyperef parameter */
	if ((param = gda_parameter_list_find_param (par_list, "ftyperef")) == NULL)
	{
		g_warning ("param ftyperef is NULL from pquery!");
		return -1;
	}

	gda_value_reset_with_type (value, G_TYPE_STRING);
	g_value_set_string (value, field_typeref);
	gda_parameter_set_value (param, value);

	/* fenum parameter */
	if ((param = gda_parameter_list_find_param (par_list, "fenum")) == NULL)
	{
		g_warning ("param fenum is NULL from pquery!");
		return -1;
	}

	gda_value_reset_with_type (value, G_TYPE_STRING);
	g_value_set_string (value, field_enum);
	gda_parameter_set_value (param, value);

	/* funion parameter */
	if ((param = gda_parameter_list_find_param (par_list, "funion")) == NULL)
	{
		g_warning ("param funion is NULL from pquery!");
		return -1;
	}

	gda_value_reset_with_type (value, G_TYPE_STRING);
	g_value_set_string (value, field_union);
	gda_parameter_set_value (param, value);

	/* fclass parameter */
	if ((param = gda_parameter_list_find_param (par_list, "fclass")) == NULL)
	{
		g_warning ("param fclass is NULL from pquery!");
		return -1;
	}

	gda_value_reset_with_type (value, G_TYPE_STRING);
	g_value_set_string (value, field_class);
	gda_parameter_set_value (param, value);

	/* fnamespace parameter */
	if ((param =
		 gda_parameter_list_find_param (par_list, "fnamespace")) == NULL)
	{
		g_warning ("param fnamespace is NULL from pquery!");
		return -1;
	}

	gda_value_reset_with_type (value, G_TYPE_STRING);
	g_value_set_string (value, field_namespace);
	gda_parameter_set_value (param, value);
	gda_value_free (value);


	/* execute the query with parametes just set */
	gda_query_execute ((GdaQuery *) query, par_list, FALSE, NULL);

	table_id = sdb_engine_get_last_insert_id (dbe);
	return table_id;
}

static gboolean
sdb_engine_second_pass_update_scope_1 (SymbolDBEngine * dbe,
									   GdaDataModel * data, gint data_row,
									   gchar * token_name,
									   const GValue * token_value)
{
	gint scope_id;
	GValue *value1, *value2, *value;
	const GValue *value_id2;
	gint symbol_id;
	const gchar *tmp_str;
	gchar **tmp_str_splitted;
	gint tmp_str_splitted_length;
	gchar *object_name = NULL;
	gboolean free_token_name = FALSE;
	const GdaQuery *query;
	GdaParameterList *par_list;
	GdaParameter *param;

	g_return_val_if_fail (G_VALUE_HOLDS_STRING (token_value), FALSE);
	
	tmp_str = g_value_get_string (token_value);

	/* we don't need empty strings */
	if (strcmp (tmp_str, "") == 0)
		return FALSE;

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
		return FALSE;
	}

	g_strfreev (tmp_str_splitted);

	/*	DEBUG_PRINT ("2nd pass scope: got %s %s (from %s)", token_name, object_name, 	 
	 * tmp_str);
	 */

	value1 = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value1, token_name);

	value2 = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value2, object_name);

	if ((scope_id = sdb_engine_get_table_id_by_unique_name2 (dbe,
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
		return FALSE;
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
	symbol_id = g_value_get_int (value_id2);

	if ((query = sdb_engine_get_query_by_id (dbe,
											 PREP_QUERY_UPDATE_SYMBOL_SCOPE_ID))
		== NULL)
	{
		g_warning ("query is null");
		return FALSE;
	}

	if (GDA_QUERY_TYPE_NON_PARSED_SQL
		== gda_query_get_query_type ((GdaQuery *) query))
	{
		g_warning ("non parsed sql error");
		return FALSE;
	}

	if ((par_list = gda_query_get_parameter_list ((GdaQuery *) query)) == NULL)
	{
		g_warning ("par_list is NULL!\n");
		return FALSE;
	}

	/* scopeid parameter */
	if ((param = gda_parameter_list_find_param (par_list, "scopeid")) == NULL)
	{
		g_warning ("param scopeid is NULL from pquery!");
		return FALSE;
	}

	value = gda_value_new (G_TYPE_INT);
	g_value_set_int (value, scope_id);
	gda_parameter_set_value (param, value);

	/* symbolid parameter */
	if ((param = gda_parameter_list_find_param (par_list, "symbolid")) == NULL)
	{
		g_warning ("param symbolid is NULL from pquery!");
		return FALSE;
	}

	gda_value_reset_with_type (value, G_TYPE_INT);
	g_value_set_int (value, symbol_id);
	gda_parameter_set_value (param, value);
	gda_value_free (value);

	/* execute the query with parametes just set */
	gda_query_execute ((GdaQuery *) query, par_list, FALSE, NULL);

	return TRUE;
}


/**
 * @param data Must be filled with some values. It must have num_rows > 0
 * @note *CALL THIS BEFORE second_pass_update_heritage ()*
 * @note *DO NOT FREE data* inside this function.
 */
static void
sdb_engine_second_pass_update_scope (SymbolDBEngine * dbe, GdaDataModel * data)
{
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

			/* ok, search for the symbol_id of the base class */
			if (namespace_name == NULL)
			{
				GValue *value1;

				value1 = gda_value_new (G_TYPE_STRING);
				g_value_set_string (value1, klass_name);

				if ((base_klass_id =
					 sdb_engine_get_table_id_by_unique_name (dbe,
										 PREP_QUERY_GET_SYMBOL_ID_BY_CLASS_NAME,
										 "klassname",
										 value1)) < 0)
				{
					gda_value_free (value1);
					continue;
				}
				gda_value_free (value1);
			}
			else
			{
/*	FIXME: 
when Libgda will parse correctly the PREP_QUERY_GET_SYMBOL_ID_BY_CLASS_NAME_AND_NAMESPACE
query please uncomment this section and let the prepared statements do their
work.				
				GValue *value1;
				GValue *value2;

				value1 = gda_value_new (G_TYPE_STRING);
				g_value_set_string (value1, klass_name);

				value2 = gda_value_new (G_TYPE_STRING);
				g_value_set_string (value2, namespace_name);

				DEBUG_PRINT ("value1 : %s value2 : %s", klass_name, namespace_name);
				if ((base_klass_id =
					 sdb_engine_get_table_id_by_unique_name2 (dbe,
						  PREP_QUERY_GET_SYMBOL_ID_BY_CLASS_NAME_AND_NAMESPACE,
						  "klassname",
						  value1,
						  "namespacename",
						  value2)) < 0)
				{
					gda_value_free (value1);
					gda_value_free (value2);
					continue;
				}
				gda_value_free (value1);
				gda_value_free (value2);
*/
				GValue *value1;
				GdaCommand *command;
				GdaDataModel *base_data;
				gchar *query_str;
				query_str = 
					g_strdup_printf ("SELECT symbol_id FROM symbol JOIN scope ON "
							 "symbol.scope_id=scope.scope_id JOIN sym_type ON "
							 "scope.type_id=sym_type.type_id WHERE "
							 "symbol.name='%s' AND scope.scope='%s' AND "
							 "sym_type.type='namespace'",
							 klass_name, namespace_name);
				
				command = gda_command_new (query_str, GDA_COMMAND_TYPE_SQL,
										   GDA_COMMAND_OPTION_STOP_ON_ERRORS);
				
				if ( (base_data = 
					  gda_connection_execute_select_command (priv->db_connection, 
															 command, NULL, 
															 NULL)) == NULL ||
					  gda_data_model_get_n_rows (base_data) <= 0 ) 
				{
					gda_command_free (command);	
					g_free (query_str);
					  
					if (base_data != NULL)
						g_object_unref (base_data);
					  
					continue;
				}
			
				value1 = (GValue*)gda_data_model_get_value_at (base_data, 0, 0);
				base_klass_id = g_value_get_int (value1);
				DEBUG_PRINT ("found base_klass_id %d", base_klass_id );
				gda_command_free (command);	
				g_free (query_str);
													 
				if (base_data != NULL)
					g_object_unref (base_data);
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
	GObject *obj;
	const GdaQuery *query1, *query2, *query3;

	/* prepare for scope second scan */
	if ((query1 =
		 sdb_engine_get_query_by_id (dbe,
									 PREP_QUERY_GET_ALL_FROM_TMP_HERITAGE))
		== NULL)
	{
		g_warning ("query is null");
		return;
	}

	if (GDA_QUERY_TYPE_NON_PARSED_SQL ==
		gda_query_get_query_type ((GdaQuery *) query1))
	{
		g_warning ("non parsed sql error");
		return;
	}

	/* execute the query */
	obj = G_OBJECT (gda_query_execute ((GdaQuery *) query1, NULL, FALSE, NULL));

	if (!GDA_IS_DATA_MODEL (obj) ||
		gda_data_model_get_n_rows (GDA_DATA_MODEL (obj)) <= 0)
	{

		if (obj != NULL)
			g_object_unref (obj);
		obj = NULL;
	}
	else
	{
		sdb_engine_second_pass_update_scope (dbe, GDA_DATA_MODEL (obj));
	}

	if (obj != NULL)
		g_object_unref (obj);

	/* prepare for heritage second scan */
	if ((query2 =
		 sdb_engine_get_query_by_id (dbe,
						 PREP_QUERY_GET_ALL_FROM_TMP_HERITAGE_WITH_INHERITS))
		== NULL)
	{
		g_warning ("query is null");
		return;
	}

	if (GDA_QUERY_TYPE_NON_PARSED_SQL ==
		gda_query_get_query_type ((GdaQuery *) query2))
	{
		g_warning ("non parsed sql error");
		return;
	}

	/* execute the query */
	obj = G_OBJECT (gda_query_execute ((GdaQuery *) query2, NULL, FALSE, NULL));

	if (!GDA_IS_DATA_MODEL (obj) ||
		gda_data_model_get_n_rows (GDA_DATA_MODEL (obj)) <= 0)
	{
		if (obj != NULL)
			g_object_unref (obj);
		obj = NULL;
	}
	else
	{
		sdb_engine_second_pass_update_heritage (dbe, GDA_DATA_MODEL (obj));
	}

	if (obj != NULL)
		g_object_unref (obj);


	/* clean tmp heritage table */
	if ((query3 =
		 sdb_engine_get_query_by_id (dbe,
									 PREP_QUERY_TMP_HERITAGE_DELETE_ALL))
		== NULL)
	{
		g_warning ("query is null");
		return;
	}

	if (GDA_QUERY_TYPE_NON_PARSED_SQL ==
		gda_query_get_query_type ((GdaQuery *) query3))
	{
		g_warning ("non parsed sql error");
		return;
	}

	/* execute the query */
	gda_query_execute ((GdaQuery *) query3, NULL, FALSE, NULL);
}

/* base_prj_path can be NULL. In that case path info tag_entry will be taken
 * as an absolute path.
 * fake_file can be used when a buffer updating is being executed. In that 
 * particular case both base_prj_path and tag_entry->file will be ignored. 
 * fake_file is real_path of file on disk
 */
static gint
sdb_engine_add_new_symbol (SymbolDBEngine * dbe, tagEntry * tag_entry,
						   gchar * base_prj_path, gchar * fake_file,
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
	const GdaQuery *query;
	GdaParameterList *par_list;
	GdaParameter *param;
	const gchar *tmp_str;
	gint table_id, symbol_id;
	gint file_defined_id = 0;
	gchar name[256];
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

	value = gda_value_new (G_TYPE_STRING);
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
		if (fake_file == NULL)
			g_value_set_string (value, tag_entry->file);
		else
			g_value_set_string (value, fake_file);
	}

	if ((file_defined_id = sdb_engine_get_table_id_by_unique_name (dbe,
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
					 g_value_get_string (value), base_prj_path, fake_file, 
					 tag_entry->file);
		gda_value_free (value);
		return -1;
	}


	/* parse the entry name */
	if (strlen (tag_entry->name) > sizeof (name))
	{
		printf
			("fatal error when inserting symbol, name of symbol is too big.\n");
		gda_value_free (value);
		return -1;
	}
	else
	{
		memset (name, 0, sizeof (name));
		memcpy (name, tag_entry->name, strlen (tag_entry->name));
	}

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
	scope_definition_id = sdb_engine_add_new_scope_definition (dbe, tag_entry);

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

	if ((symbol_id = sdb_engine_get_table_id_by_unique_name3 (dbe,
								  PREP_QUERY_GET_SYMBOL_ID_BY_UNIQUE_INDEX_KEY,
								  "symname", value1,
								  "filedefid",value2, 
								  "typeid", value3)) <= 0)
	{
		/* case 2 and 3 */
/*		DEBUG_PRINT ("inserting new symbol: %s", name);*/
		sym_was_updated = FALSE;
		
		gda_value_free (value1);
		gda_value_free (value2);
		gda_value_free (value3);

		/* create specific query for a fresh new symbol */
		if ((query = sdb_engine_get_query_by_id (dbe, PREP_QUERY_SYMBOL_NEW))
			== NULL)
		{
			g_warning ("query is null");
			return -1;
		}

		if (GDA_QUERY_TYPE_NON_PARSED_SQL ==
			gda_query_get_query_type ((GdaQuery *) query))
		{
			g_warning ("non parsed sql error");
			gda_value_free (value);
			return -1;
		}

		if ((par_list =
			 gda_query_get_parameter_list ((GdaQuery *) query)) == NULL)
		{
			g_warning ("par_list is NULL!\n");
			gda_value_free (value);
			return -1;
		}

		/* filedefid parameter */
		if ((param =
			 gda_parameter_list_find_param (par_list, "filedefid")) == NULL)
		{
			g_warning ("param filedefid is NULL from pquery!");
			gda_value_free (value);
			return -1;
		}

		gda_value_reset_with_type (value, G_TYPE_INT);
		g_value_set_int (value, file_defined_id);
		gda_parameter_set_value (param, value);

		/* name parameter */
		if ((param = gda_parameter_list_find_param (par_list, "name")) == NULL)
		{
			g_warning ("param name is NULL from pquery!");
			gda_value_free (value);
			return -1;
		}

		gda_value_reset_with_type (value, G_TYPE_STRING);
		g_value_set_string (value, name);
		gda_parameter_set_value (param, value);

		/* fileposition parameter */
		if ((param =
			 gda_parameter_list_find_param (par_list, "fileposition")) == NULL)
		{
			g_warning ("param fileposition is NULL from pquery!");
			gda_value_free (value);
			return -1;
		}

		gda_value_reset_with_type (value, G_TYPE_INT);
		g_value_set_int (value, file_position);
		gda_parameter_set_value (param, value);


		/* typeid parameter */
		if ((param =
			 gda_parameter_list_find_param (par_list, "typeid")) == NULL)
		{
			g_warning ("param typeid is NULL from pquery!");
			gda_value_free (value);
			return -1;
		}

		gda_value_reset_with_type (value, G_TYPE_INT);
		g_value_set_int (value, type_id);
		gda_parameter_set_value (param, value);
	}
	else
	{
		/* case 1 */
/*		DEBUG_PRINT ("updating symbol: %s", name);*/
		sym_was_updated = TRUE;
		
		gda_value_free (value1);
		gda_value_free (value2);
		gda_value_free (value3);

		/* create specific query for a fresh new symbol */
		if ((query = sdb_engine_get_query_by_id (dbe,
												 PREP_QUERY_UPDATE_SYMBOL_ALL))
			== NULL)
		{
			g_warning ("query is null");
			return -1;
		}

		if (GDA_QUERY_TYPE_NON_PARSED_SQL ==
			gda_query_get_query_type ((GdaQuery *) query))
		{
			g_warning ("non parsed sql error");
			gda_value_free (value);
			return -1;
		}

		if ((par_list =
			 gda_query_get_parameter_list ((GdaQuery *) query)) == NULL)
		{
			g_warning ("par_list is NULL!\n");
			gda_value_free (value);
			return -1;
		}

		/* isfilescope parameter */
		if ((param = gda_parameter_list_find_param (par_list, "symbolid"))
			== NULL)
		{
			g_warning ("param isfilescope is NULL from pquery!");
			gda_value_free (value);
			return -1;
		}

		gda_value_reset_with_type (value, G_TYPE_INT);
		g_value_set_int (value, symbol_id);
		gda_parameter_set_value (param, value);
	}

	/* common params */

	/* isfilescope parameter */
	if ((param =
		 gda_parameter_list_find_param (par_list, "isfilescope")) == NULL)
	{
		g_warning ("param isfilescope is NULL from pquery!");
		gda_value_free (value);
		return -1;
	}

	gda_value_reset_with_type (value, G_TYPE_INT);
	g_value_set_int (value, is_file_scope);
	gda_parameter_set_value (param, value);

	/* signature parameter */
	if ((param = gda_parameter_list_find_param (par_list, "signature")) == NULL)
	{
		g_warning ("param signature is NULL from pquery!");
		gda_value_free (value);
		return -1;
	}

	gda_value_reset_with_type (value, G_TYPE_STRING);
	g_value_set_string (value, signature);
	gda_parameter_set_value (param, value);

	/* scopedefinitionid parameter */
	if ((param =
		 gda_parameter_list_find_param (par_list, "scopedefinitionid")) == NULL)
	{
		g_warning ("param scopedefinitionid is NULL from pquery!");
		gda_value_free (value);
		return -1;
	}

	gda_value_reset_with_type (value, G_TYPE_INT);
	g_value_set_int (value, scope_definition_id);
	gda_parameter_set_value (param, value);

	/* scopeid parameter */
	if ((param = gda_parameter_list_find_param (par_list, "scopeid")) == NULL)
	{
		g_warning ("param scopeid is NULL from pquery!");
		gda_value_free (value);
		return -1;
	}

	gda_value_reset_with_type (value, G_TYPE_INT);
	g_value_set_int (value, scope_id);
	gda_parameter_set_value (param, value);

	/* kindid parameter */
	if ((param = gda_parameter_list_find_param (par_list, "kindid")) == NULL)
	{
		g_warning ("param kindid is NULL from pquery!");
		gda_value_free (value);
		return -1;
	}

	gda_value_reset_with_type (value, G_TYPE_INT);
	g_value_set_int (value, kind_id);
	gda_parameter_set_value (param, value);

	/* accesskindid parameter */
	if ((param =
		 gda_parameter_list_find_param (par_list, "accesskindid")) == NULL)
	{
		g_warning ("param accesskindid is NULL from pquery!");
		gda_value_free (value);
		return -1;
	}

	gda_value_reset_with_type (value, G_TYPE_INT);
	g_value_set_int (value, access_kind_id);
	gda_parameter_set_value (param, value);

	/* implementationkindid parameter */
	if ((param =
		 gda_parameter_list_find_param (par_list,
										"implementationkindid")) == NULL)
	{
		g_warning ("param implementationkindid is NULL from pquery!");
		gda_value_free (value);
		return -1;
	}

	gda_value_reset_with_type (value, G_TYPE_INT);
	g_value_set_int (value, implementation_kind_id);
	gda_parameter_set_value (param, value);

	/* updateflag parameter */
	if ((param =
		 gda_parameter_list_find_param (par_list, "updateflag")) == NULL)
	{
		g_warning ("param updateflag is NULL from pquery!");
		gda_value_free (value);
		return -1;
	}

	gda_value_reset_with_type (value, G_TYPE_INT);
	g_value_set_int (value, update_flag);
	gda_parameter_set_value (param, value);
	gda_value_free (value);

	/* execute the query with parametes just set */
	gda_query_execute ((GdaQuery *) query, par_list, FALSE, NULL);
	
	if (sym_was_updated == FALSE)
	{
		table_id = sdb_engine_get_last_insert_id (dbe);
		
		/* This is a wrong place to emit the symbol-updated signal. Infact
		 * db is in a inconsistent state, e.g. inheritance references are still
		 * *not* calculated.
		 * So add the symbol id into a queue that will be parsed once and emitted.
		 */		
/*		g_signal_emit (dbe, signals[SYMBOL_INSERTED], 0, table_id); */
		g_async_queue_push (priv->inserted_symbols_id, (gpointer) table_id);
	}
	else
	{
		table_id = symbol_id;
		
		/* This is a wrong place to emit the symbol-updated signal. Infact
		 * db is in a inconsistent state, e.g. inheritance references are still
		 * *not* calculated.
		 * So add the symbol id into a queue that will be parsed once and emitted.
		 */
/*		g_signal_emit (dbe, signals[SYMBOL_UPDATED], 0, table_id); */
		g_async_queue_push (priv->updated_symbols_id, (gpointer) table_id);
	}	

	/* before returning the table_id we have to fill some infoz on temporary tables
	 * so that in a second pass we can parse also the heritage and scope fields.
	 */
	sdb_engine_add_new_tmp_heritage_scope (dbe, tag_entry, table_id);

	return table_id;
}


/**
 * Select * from __tmp_removed and emits removed signals.
 */
static void
sdb_engine_detects_removed_ids (SymbolDBEngine *dbe)
{
	const GdaQuery *query, *query2;
	SymbolDBEnginePriv *priv;
	GObject *obj;
	gint num_rows;
	gint i;	
	g_return_if_fail (dbe != NULL);
	
	priv = dbe->priv;
	
	/* ok, now we should read from __tmp_removed all the symbol ids which have
	 * been removed, and emit a signal 
	 */
	if ((query = sdb_engine_get_query_by_id (dbe,
											 PREP_QUERY_GET_REMOVED_IDS))
		== NULL)
	{
		g_warning ("query is null");
		return;
	}

	if (GDA_QUERY_TYPE_NON_PARSED_SQL ==
		gda_query_get_query_type ((GdaQuery *) query))
	{
		g_warning ("non parsed sql error");
		return;
	}
	
	obj = G_OBJECT (gda_query_execute ((GdaQuery *) query, NULL, FALSE,
									 NULL));
	
	if (GDA_IS_DATA_MODEL (obj)) 
	{
		if ((num_rows = gda_data_model_get_n_rows (GDA_DATA_MODEL (obj))) <= 0)
		{
			g_object_unref (obj);
			return;
		}
	}
	else
	{
		if (obj != NULL)
			g_object_unref (obj);
		return;
	}

	/* get and parse the results. */
	for (i = 0; i < num_rows; i++) 
	{
		const GValue *val;
		gint tmp;
		val = gda_data_model_get_value_at (GDA_DATA_MODEL (obj), 0, i);
		tmp = g_value_get_int (val);
		
		g_signal_emit (dbe, signals[SYMBOL_REMOVED], 0, tmp);
	}

	g_object_unref (obj);
	
	/* let's clean the tmp_table */
	if ((query2 = sdb_engine_get_query_by_id (dbe,
											 PREP_QUERY_TMP_REMOVED_DELETE_ALL))
		== NULL)
	{
		g_warning ("query is null");
		return;
	}

	if (GDA_QUERY_TYPE_NON_PARSED_SQL ==
		gda_query_get_query_type ((GdaQuery *) query2))
	{
		g_warning ("non parsed sql error");
		return;
	}
	
	/* bye bye */
	gda_query_execute ((GdaQuery *) query2, NULL, FALSE, NULL);
		
}

/**
 * WARNING: do not use this function thinking that it would do a scan of symbols
 * too. Use symbol_db_engine_update_files_symbols () instead. This one will set
 * up some things on db, like removing the 'old' files which have not been 
 * updated.
 */
static gboolean
sdb_engine_update_file (SymbolDBEngine * dbe, const gchar * file_on_db)
{
	const GdaQuery *query;
	gchar *query_str;
	GdaParameterList *par_list;
	GdaParameter *param;
	GValue *value;
	SymbolDBEnginePriv *priv;

	g_return_val_if_fail (dbe != NULL, FALSE);
	
	priv = dbe->priv;

	if (symbol_db_engine_is_project_opened (dbe, priv->project_name) == FALSE) 
	{
		g_warning ("project is not opened");
		return FALSE;
	}

	
	/* if we're updating symbols we must do some other operations on db 
	 * symbols, like remove the ones which don't have an update_flag = 1 
	 * per updated file.
	 */

	/* good. Go on with removing of old symbols, marked by a 
	 * update_flag = 0. 
	 */

	/* FIXME: libgda 3.0 doesn't have support for JOIN keyword at all on
	 * prepared statements.
	 * Said this we cannot expect to run queries on a really performant
	 * way. 
	 */

	/* Triggers will take care of updating/deleting connected symbols
	 * tuples, like sym_kind, sym_type etc */
	query_str = g_strdup_printf ("DELETE FROM symbol WHERE "
								 "file_defined_id = (SELECT file_id FROM file "
								 "WHERE file_path = \"%s\") AND update_flag = 0",
								 file_on_db);

	sdb_engine_execute_non_select_sql (dbe, query_str);
	g_free (query_str);

	/* emits removed symbols signals */
	sdb_engine_detects_removed_ids (dbe);
	
	/* reset the update_flag to 0 */
	query_str = g_strdup_printf ("UPDATE symbol SET update_flag = 0 "
								 "WHERE file_defined_id = (SELECT file_id FROM file WHERE "
								 "file_path = \"%s\")", file_on_db);

	sdb_engine_execute_non_select_sql (dbe, query_str);
	g_free (query_str);

	/* last but not least, update the file analize_time */
	if ((query = sdb_engine_get_query_by_id (dbe,
											 PREP_QUERY_UPDATE_FILE_ANALIZE_TIME))
		== NULL)
	{
		g_warning ("query is null");
		return FALSE;
	}

	if (GDA_QUERY_TYPE_NON_PARSED_SQL ==
		gda_query_get_query_type ((GdaQuery *) query))
	{
		g_warning ("non parsed sql error");
		return FALSE;
	}

	if ((par_list = gda_query_get_parameter_list ((GdaQuery *) query)) == NULL)
	{
		g_warning ("par_list is NULL!\n");
		return FALSE;
	}

	/* filepath parameter */
	if ((param = gda_parameter_list_find_param (par_list, "filepath")) == NULL)
	{
		g_warning ("param filepath is NULL from pquery!");
		return FALSE;
	}

	value = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value, file_on_db);
	gda_parameter_set_value (param, value);

	gda_value_free (value);
	gda_query_execute ((GdaQuery *) query, par_list, FALSE, NULL);

	return TRUE;
}

/**
 * @param data is a GPtrArray *files_to_scan
 * It will be freed when this callback will be called.
 */
static void
on_scan_update_files_symbols_end (SymbolDBEngine * dbe, GPtrArray* data)
{
	SymbolDBEnginePriv *priv;
	GPtrArray *files_to_scan;
	gint i;

	DEBUG_PRINT ("on_scan_update_files_symbols_end  ();");

	g_return_if_fail (dbe != NULL);
	g_return_if_fail (data != NULL);

	priv = dbe->priv;
	files_to_scan = (GPtrArray *) data;

	DEBUG_PRINT ("files_to_scan->len %d", files_to_scan->len);

	for (i = 0; i < files_to_scan->len; i++)
	{
		gchar *node = (gchar *) g_ptr_array_index (files_to_scan, i);
		
/*		DEBUG_PRINT ("processing updating node: %s, data_source: %s", node,
					 priv->data_source);
		DEBUG_PRINT ("processing updating for file %s", node + 
					 strlen (priv->data_source));*/
		
		/* clean the db from old un-updated with the last update step () */
		if (sdb_engine_update_file (dbe, node + 
									strlen (priv->data_source)) == FALSE)
		{
			g_warning ("Error processing file %s", node + 
					   strlen (priv->data_source) );
			return;
		}
		g_free (node);
	}
		
	g_signal_handlers_disconnect_by_func (dbe, on_scan_update_files_symbols_end,
										  files_to_scan);
		
	/* free the GPtrArray. */
	DEBUG_PRINT ("free the files_to_scan");
	g_ptr_array_free (files_to_scan, TRUE);
	data = files_to_scan = NULL;
	DEBUG_PRINT ("done");
}


/* Update symbols of saved files. 
 * WARNING: files_path and it's contents will be freed on 
 * on_scan_update_files_symbols_end () callback.
 */
gboolean
symbol_db_engine_update_files_symbols (SymbolDBEngine * dbe, gchar * project, 
									   GPtrArray * files_path	/*, 
									   * gchar *language */ ,
									   gboolean update_prj_analize_time)
{
	SymbolDBEnginePriv *priv;
	
	priv = dbe->priv;

	DEBUG_PRINT ("symbol_db_engine_update_files_symbols ()");
	g_return_val_if_fail (priv->db_connection != NULL, FALSE);
	g_return_val_if_fail (project != NULL, FALSE);
	
	if (symbol_db_engine_is_project_opened (dbe, project) == FALSE) 
	{
		g_warning ("project is not opened");
		return FALSE;
	}
	
	/* data will be freed when callback will be called. The signal will be
	 * disconnected too, don't worry about disconneting it by hand.
	 */
	g_signal_connect (G_OBJECT (dbe), "scan-end",
					  G_CALLBACK (on_scan_update_files_symbols_end), files_path);
	
	sdb_engine_scan_files_1 (dbe, files_path, NULL, TRUE);

	/* if true, we'll update the project scanning time too. 
	 * warning: project time scanning won't could be set before files one.
	 * This why we'll fork the process calling sdb_engine_scan_files ()
	 */
	if (update_prj_analize_time == TRUE)
	{
		const GdaQuery *query;
		GdaParameterList *par_list;
		GdaParameter *param;
		GValue *value;

		/* and the project analize_time */
		if ((query = sdb_engine_get_query_by_id (dbe,
									PREP_QUERY_UPDATE_PROJECT_ANALIZE_TIME))
			== NULL)
		{
			g_warning ("query is null");
			return FALSE;
		}

		if (GDA_QUERY_TYPE_NON_PARSED_SQL ==
			gda_query_get_query_type ((GdaQuery *) query))
		{
			g_warning ("non parsed sql error");
			return FALSE;
		}

		if ((par_list =
			 gda_query_get_parameter_list ((GdaQuery *) query)) == NULL)
		{
			g_warning ("par_list is NULL!\n");
			return FALSE;
		}

		/* prjname parameter */
		if ((param = gda_parameter_list_find_param (par_list, "prjname"))
			== NULL)
		{
			g_warning ("param prjname is NULL from pquery!");
			return FALSE;
		}

		value = gda_value_new (G_TYPE_STRING);
		g_value_set_string (value, priv->project_name);
		gda_parameter_set_value (param, value);

		gda_value_free (value);

		DEBUG_PRINT ("updating project analize_time");
		gda_query_execute ((GdaQuery *) query, par_list, FALSE, NULL);
	}

	return TRUE;
}

/* Update symbols of the whole project. It scans all file symbols etc. 
 * FIXME: libgda does not support nested prepared queries like 
 * PREP_QUERY_GET_ALL_FROM_FILE_BY_PROJECT_NAME. When it will do please
 * remember to update this function.
 */
gboolean
symbol_db_engine_update_project_symbols (SymbolDBEngine * dbe, gchar * project	/*, 
											 * gboolean force */ )
{
	const GdaQuery *query;
	GdaParameterList *par_list;
	GdaParameter *param;
	GValue *value;
	GObject *obj;
	gint project_id;
	gint num_rows = 0;
	gint i;
	GPtrArray *files_to_scan;
	SymbolDBEnginePriv *priv;

	
	g_return_val_if_fail (dbe != NULL, FALSE);
	if (symbol_db_engine_is_project_opened (dbe, project) == FALSE) 
	{
		g_warning ("project is not opened");
		return FALSE;
	}
	
	priv = dbe->priv;

	value = gda_value_new (G_TYPE_STRING);
	g_value_set_string (value, project);

	/* get project id */
	if ((project_id = sdb_engine_get_table_id_by_unique_name (dbe,
									 PREP_QUERY_GET_PROJECT_ID_BY_UNIQUE_NAME,
									 "prjname",
									 value)) <= 0)
	{
		gda_value_free (value);
		return FALSE;
	}

	if ((query = sdb_engine_get_query_by_id (dbe,
								 PREP_QUERY_GET_ALL_FROM_FILE_BY_PROJECT_ID))
		== NULL)
	{
		g_warning ("query is null");
		gda_value_free (value);
		return FALSE;
	}

	if (GDA_QUERY_TYPE_NON_PARSED_SQL ==
		gda_query_get_query_type ((GdaQuery *) query))
	{
		g_warning ("non parsed sql error");
		gda_value_free (value);
		return FALSE;
	}

	if ((par_list = gda_query_get_parameter_list ((GdaQuery *) query)) == NULL)
	{
		g_warning ("par_list is NULL!\n");
		gda_value_free (value);
		return FALSE;
	}

	/* prjid parameter */
	if ((param = gda_parameter_list_find_param (par_list, "prjid")) == NULL)
	{
		g_warning ("param prjname is NULL from pquery!");
		return FALSE;
	}

	gda_value_reset_with_type (value, G_TYPE_INT);
	g_value_set_int (value, project_id);
	gda_parameter_set_value (param, value);

	/* execute the query with parametes just set */
	obj =
		G_OBJECT (gda_query_execute
				  ((GdaQuery *) query, par_list, FALSE, NULL));

	if (!GDA_IS_DATA_MODEL (obj) ||
		(num_rows = gda_data_model_get_n_rows (GDA_DATA_MODEL (obj))) <= 0)
	{

		g_message ("no rows");
		if (obj != NULL)
			g_object_unref (obj);
		obj = NULL;
	}

	/* we don't need it anymore */
	gda_value_free (value);

	g_message ("got gda_data_model_get_n_rows (GDA_DATA_MODEL(obj)) %d",
			   num_rows);

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
			 gda_data_model_get_value_at_col_name (GDA_DATA_MODEL (obj),
												   "file_path", i)) == NULL)
		{
			continue;
		}

		/* build abs path. */
		file_name = g_value_get_string (value);
		if (priv->data_source != NULL)
		{
			abs_vfs_path = g_strdup_printf ("file://%s%s", priv->data_source,
										file_name);
			file_abs_path = g_strdup_printf ("%s%s", priv->data_source,
										file_name);
		}
		else
		{
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
			 gda_data_model_get_value_at_col_name (GDA_DATA_MODEL (obj),
												   "analize_time", i)) == NULL)
		{
			continue;
		}

		/* weirdly we have a strange libgda behaviour here too. 
		 * as from ChangeLog GDA_TYPE_TIMESTAMP as SQLite does not impose a 
		 * known format for dates (there is no date datatype).
		 * We have then to do some hackery to retrieve the date.        
		 */
		date_string = (gchar *) g_value_get_string (value1);

		DEBUG_PRINT ("processing for upating symbol %s", file_name);
		DEBUG_PRINT ("date_string %s", date_string);

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
		
		/* subtract one hour to the db_file_time. (why this?) */
		db_file_time = mktime (&filetm) - 3600;

		if (difftime (db_file_time, file_info->mtime) <= 0)
		{
			g_message ("to be added! : %s", file_name);
			g_ptr_array_add (files_to_scan, file_abs_path);
		}
/*
		DEBUG_PRINT ("difftime %f", difftime (db_file_time, file_info->mtime));
		DEBUG_PRINT ("db_file_time %d - "
				   "file_info->mtime %d "
				   "file_info->ctime %d", db_file_time,
				   file_info->mtime, file_info->ctime);
*/
		gnome_vfs_close (handle);
		gnome_vfs_uri_unref (uri);
		gnome_vfs_file_info_unref (file_info);
		g_free (abs_vfs_path);
		/* no need to free file_abs_path, it's been added to files_to_scan */
	}

	if (files_to_scan->len > 0)
	{
		/* at the end let's the scanning function do its job */
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
	if (symbol_db_engine_is_project_opened (dbe, project) == FALSE) 
	{
		g_warning ("project is not opened");
		return FALSE;
	}	

	priv = dbe->priv;
	
	if (strlen (file) < strlen (priv->data_source)) 
	{
		g_warning ("wrong file");
		return FALSE;
	}
	
	/* Triggers will take care of updating/deleting connected symbols
	 * tuples, like sym_kind, sym_type etc */
	query_str = g_strdup_printf ("DELETE FROM file WHERE prj_id "
				 "= (SELECT project_id FROM project WHERE project_name = '%s') AND "
				 "file_path = '%s'", project, file + strlen (priv->data_source));

/*	DEBUG_PRINT ("symbol_db_engine_remove_file () : %s", query_str);*/
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
		
		DEBUG_PRINT ("processing updating for file [on disk] %s, "
					 "passed to on_scan_update_buffer_end (): %s", 
					 node, node + strlen (priv->data_source));
		
		if (sdb_engine_update_file (dbe, node+ 
									strlen (priv->data_source)) == FALSE)
		{
			g_warning ("Error processing file %s", node);
			return;
		}
		g_free (node);
	}
		
	g_signal_handlers_disconnect_by_func (dbe, on_scan_update_buffer_end,
										  files_to_scan);
		
	/* free the GPtrArray. */
	DEBUG_PRINT ("free the files_to_scan");
	g_ptr_array_free (files_to_scan, TRUE);
	data = files_to_scan = NULL;
	DEBUG_PRINT ("done");
}

/* Update symbols of a file by a memory-buffer to perform a real-time updating 
 * of symbols. 
 * real_files_list: full path on disk to 'real file' to update. e.g.
 * /home/foouser/fooproject/src/main.c
 */
gboolean
symbol_db_engine_update_buffer_symbols (SymbolDBEngine * dbe, gchar * project,
										GPtrArray * real_files_list,
										const GPtrArray * text_buffers,
										const GPtrArray * buffer_sizes)
{
	SymbolDBEnginePriv *priv;
	gint i;
	
	DEBUG_PRINT ("symbol_db_engine_update_buffer_symbols ()");
	
	/* array that'll represent the /dev/shm/anjuta-XYZ files */
	GPtrArray *temp_files;
	GPtrArray *real_files_on_db;
	
	priv = dbe->priv;

	g_return_val_if_fail (priv->db_connection != NULL, FALSE);
	g_return_val_if_fail (project != NULL, FALSE);
	g_return_val_if_fail (real_files_list != NULL, FALSE);
	g_return_val_if_fail (text_buffers != NULL, FALSE);
	g_return_val_if_fail (buffer_sizes != NULL, FALSE);

	if (symbol_db_engine_is_project_opened (dbe, project) == FALSE) 
	{
		g_warning ("project is not opened");
		return FALSE;
	}
	
	temp_files = g_ptr_array_new();	
	real_files_on_db = g_ptr_array_new();
	
	/* obtain a GPtrArray with real_files on database */
	for (i=0; i < real_files_list->len; i++) 
	{
		gchar *new_node = (gchar*)g_ptr_array_index (real_files_list, i) 
								   + strlen(priv->data_source);
		DEBUG_PRINT ("real_file on db: %s", new_node);
		g_ptr_array_add (real_files_on_db, g_strdup (new_node));
	}	
	
	/* create a temporary file for each buffer */
	for (i=0; i < real_files_list->len; i++) 
	{
		FILE *shared_mem_file;
		const gchar *temp_buffer;
		gint shared_mem_fd;
		gint temp_size;
		gchar *temp_file;
		gchar *base_filename;
		const gchar *curr_real_file;
				
		curr_real_file = g_ptr_array_index (real_files_list, i);
		
		/* it's ok to have just the base filename to create the
		 * target buffer one */
		base_filename = g_filename_display_basename (curr_real_file);
		
		temp_file = g_strdup_printf ("/anjuta-%d-%ld-%s", getpid (),
						 time (NULL), base_filename);
		g_free (base_filename);
		
		DEBUG_PRINT ("my temp file is %s", temp_file);
		if ((shared_mem_fd = 
			 shm_open (temp_file, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR)) < 0)
		{
			g_warning ("Error while trying to open a shared memory file. Be"
					   "sure to have /dev/shm mounted with tmpfs");
			return FALSE;
		}
	
		shared_mem_file = fdopen (shared_mem_fd, "w+b");
		
		DEBUG_PRINT ("temp_file %s", temp_file);
		
		temp_buffer = g_ptr_array_index (text_buffers, i);
		temp_size = (gint)g_ptr_array_index (buffer_sizes, i);
		fwrite (temp_buffer, sizeof(gchar), temp_size, shared_mem_file);
		fflush (shared_mem_file);
		fclose (shared_mem_file);
		
		/* add the temp file to the array. */
		g_ptr_array_add (temp_files, g_strdup_printf ("/dev/shm%s", temp_file));
		g_free (temp_file);
	}

	
	/* data will be freed when callback will be called. The signal will be
	 * disconnected too, don't worry about disconneting it by hand.
	 */
	g_signal_connect (G_OBJECT (dbe), "scan-end",
					  G_CALLBACK (on_scan_update_buffer_end), real_files_list);
	
	sdb_engine_scan_files_1 (dbe, temp_files, real_files_on_db, TRUE);

	/* let's free the temp_files array */
	for (i=0; i < temp_files->len; i++)
		g_free (g_ptr_array_index (temp_files, i));
	
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

static inline gint
sdb_engine_walk_down_scope_path (SymbolDBEngine *dbe, const GPtrArray* scope_path) 
{
	SymbolDBEnginePriv *priv;
	gint final_definition_id;
	gint scope_path_len;
	gint i;
	gchar *query_str;
	GdaCommand *command;
	GdaDataModel *data;
		
	g_return_val_if_fail (dbe != NULL, FALSE);
	priv = dbe->priv;
	
	scope_path_len = scope_path->len;
	
	/* we'll return if the length is even or minor than 3 */
	if (scope_path_len < 3 || scope_path_len % 2 == 0)
	{
		g_warning ("bad scope_path.");
		return -1;
	}
	
	final_definition_id = 0;
	for (i=0; i < scope_path_len -1; i = i + 2)
	{
		const GValue *value;
		DEBUG_PRINT ("loop final_definition_id %d", final_definition_id);
		
		query_str = g_strdup_printf ("SELECT scope_definition_id FROM symbol "
		"WHERE scope_id = '%d' AND scope_definition_id = ("
			"SELECT scope.scope_id FROM scope "
			"INNER JOIN sym_type ON scope.type_id = sym_type.type_id "
			"WHERE sym_type.type = '%s' "
				"AND scope.scope = '%s'"
		")", final_definition_id, (gchar*)g_ptr_array_index (scope_path, i), 
			 (gchar*)g_ptr_array_index (scope_path, i + 1));
		
		DEBUG_PRINT ("walk down query is %s", query_str);
		command = gda_command_new (query_str, GDA_COMMAND_TYPE_SQL,
					   GDA_COMMAND_OPTION_STOP_ON_ERRORS);

		if ( (data = gda_connection_execute_select_command (priv->db_connection, 
												command, NULL, NULL)) == NULL ||
		  	gda_data_model_get_n_rows (data) <= 0 ) 
		{
			gda_command_free (command);
			g_free (query_str);
			return -1;
		}

		gda_command_free (command);
		g_free (query_str);
		
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

/* Returns an iterator to the data retrieved from database. 
 * The iterator, if not null, will contain a list of parent classes for the given 
 * symbol name.
 * namespace_name can be NULL.
 */
SymbolDBEngineIterator *
symbol_db_engine_get_class_parents (SymbolDBEngine *dbe, gchar *klass_name, 
									 const GPtrArray *scope_path)
{
	SymbolDBEnginePriv *priv;
	gchar *query_str;
	GdaCommand *command;
	GdaDataModel *data;
	gint final_definition_id;
	
	g_return_val_if_fail (dbe != NULL, FALSE);
	priv = dbe->priv;

	final_definition_id = -1;
	if (scope_path != NULL)	
		final_definition_id = sdb_engine_walk_down_scope_path (dbe, scope_path);
		
	/* FIXME: as always prepared queries of this complexity gives 
		GDA_QUERY_TYPE_NON_PARSED_SQL error. */
	if (final_definition_id > 0)
	{		
		query_str = g_strdup_printf("SELECT symbol.symbol_id, symbol.name FROM heritage "
			"JOIN symbol ON heritage.symbol_id_base = symbol.symbol_id "
			"WHERE symbol_id_derived = ("
				"SELECT symbol_id FROM symbol "
					"JOIN sym_kind ON symbol.kind_id = sym_kind.sym_kind_id "
					"WHERE symbol.name = '%s' "
						"AND sym_kind.kind_name = 'class' "
						"AND symbol.scope_id = '%d'"
				")", klass_name, final_definition_id);
	}
	else 
	{
		query_str = g_strdup_printf("SELECT symbol.symbol_id, symbol.name FROM heritage "
			"JOIN symbol ON heritage.symbol_id_base = symbol.symbol_id "
			"WHERE symbol_id_derived = ("
				"SELECT symbol_id FROM symbol "
					"JOIN sym_kind ON symbol.kind_id = sym_kind.sym_kind_id "
					"WHERE symbol.name = '%s' "
						"AND sym_kind.kind_name = 'class' "
				")", klass_name);
	}
	
	DEBUG_PRINT ("get parents query: %s", query_str);
	command = gda_command_new (query_str, GDA_COMMAND_TYPE_SQL,
				   GDA_COMMAND_OPTION_STOP_ON_ERRORS);

	if ( (data = gda_connection_execute_select_command (priv->db_connection, 
											command, NULL, NULL)) == NULL ||
		  gda_data_model_get_n_rows (data) <= 0 ) 
	{
		gda_command_free (command);
		g_free (query_str);
		return NULL;			  
	}

	gda_command_free (command);
	g_free (query_str);

	return (SymbolDBEngineIterator *)symbol_db_engine_iterator_new (data);
}

static inline void
sdb_engine_prepare_symbol_info_sql (SymbolDBEngine *dbe, GString *info_data,
									GString *join_data, gint sym_info) 
{
	if (sym_info & SYMINFO_FILE_PATH 	|| 
		sym_info & SYMINFO_LANGUAGE  	||
		sym_info & SYMINFO_PROJECT_NAME ||
		sym_info & SYMINFO_FILE_IGNORE  ||
		sym_info & SYMINFO_FILE_INCLUDE) 
	{
		info_data = g_string_append (info_data, ",file.file_path ");
		join_data = g_string_append (join_data, "LEFT JOIN file ON "
				"symbol.file_defined_id = file.file_id ");
	}

	if (sym_info & SYMINFO_LANGUAGE)
	{
		info_data = g_string_append (info_data, ",language.language_name ");
		join_data = g_string_append (join_data, "LEFT JOIN language ON "
				"file.lang_id = language.language_id ");
	}
	
	if (sym_info & SYMINFO_IMPLEMENTATION)
	{
		info_data = g_string_append (info_data, ",sym_implementation.implementation_name ");
		join_data = g_string_append (join_data, "LEFT JOIN sym_implementation ON "
				"symbol.implementation_kind_id = sym_implementation.sym_impl_id ");
	}
	
	if (sym_info & SYMINFO_ACCESS)
	{
		info_data = g_string_append (info_data, ",sym_access.access_name ");
		join_data = g_string_append (join_data, "LEFT JOIN sym_access ON "
				"symbol.access_kind_id = sym_access.access_kind_id ");
	}
	
	if (sym_info & SYMINFO_KIND)
	{
		info_data = g_string_append (info_data, ",sym_kind.kind_name ");
		join_data = g_string_append (join_data, "LEFT JOIN sym_kind ON "
				"symbol.kind_id = sym_kind.sym_kind_id ");
	}
	
	if (sym_info & SYMINFO_TYPE || sym_info & SYMINFO_TYPE_NAME)
	{
		info_data = g_string_append (info_data, ",sym_type.type,"
									 "sym_type.type_name ");
		join_data = g_string_append (join_data, "LEFT JOIN sym_type ON "
				"symbol.type_id = sym_type.type_id ");
	}

	if (sym_info & SYMINFO_PROJECT_NAME ||
		sym_info & SYMINFO_FILE_IGNORE  ||
		sym_info & SYMINFO_FILE_INCLUDE)
	{
		info_data = g_string_append (info_data, ",project.project_name ");
		join_data = g_string_append (join_data, "LEFT JOIN project ON "
				"file.prj_id = project.project_id ");
	}	

	if (sym_info & SYMINFO_FILE_IGNORE)
	{
		info_data = g_string_append (info_data, ",file_ignore.type AS file_ignore_type ");
		join_data = g_string_append (join_data, "LEFT JOIN ext_ignore ON "
				"ext_ignore.prj_id = project.project_id "
				"LEFT JOIN file_ignore ON "
				"ext_ignore.file_ign_id = file_ignore.file_ignore_id ");
	}

	if (sym_info & SYMINFO_FILE_INCLUDE)
	{
		info_data = g_string_append (info_data, ",file_include.type AS file_include_type ");
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
 * kind can be NULL. In that case we'll return all the kinds of symbols found
 * at root level [global level].
 */
SymbolDBEngineIterator *
symbol_db_engine_get_global_members (SymbolDBEngine *dbe, 
									const gchar *kind, gint sym_info)
{
	SymbolDBEnginePriv *priv;
	GdaCommand *command;
	GdaDataModel *data;
	GString *info_data;
	GString *join_data;
	gchar *query_str;

	g_return_val_if_fail (dbe != NULL, NULL);
	priv = dbe->priv;
	
	/* check for an already flagged sym_info with KIND. SYMINFO_KIND on sym_info
	 * is already contained into the default query infos.
	 */
	sym_info = sym_info & ~SYMINFO_KIND;

	/* info_data contains the stuff after SELECT and befor FROM */
	info_data = g_string_new ("");
	
	/* join_data contains the optionals joins to do to retrieve new data on other
	 * tables.
	 */
	join_data = g_string_new ("");

	/* fill info_data and join data with optional sql */
	sdb_engine_prepare_symbol_info_sql (dbe, info_data, join_data, sym_info);

	if (kind == NULL) 
	{
		query_str = g_strdup_printf ("SELECT symbol.symbol_id, "
			"symbol.name, symbol.file_position, symbol.is_file_scope, "
			"symbol.signature, sym_kind.kind_name %s FROM symbol "
				"JOIN sym_kind ON symbol.kind_id = sym_kind.sym_kind_id %s "
				"WHERE scope_id <= 0", info_data->str, join_data->str);
	}
	else
	{
		query_str = g_strdup_printf ("SELECT symbol.symbol_id, symbol.name, "
			"symbol.file_position, "
			"symbol.is_file_scope, symbol.signature %s FROM symbol "
				"JOIN sym_kind ON symbol.kind_id = sym_kind.sym_kind_id %s "
				"WHERE scope_id <= 0 "
				"AND sym_kind.kind_name = '%s'", info_data->str, join_data->str, 
									 kind);
	}	
	
	command = gda_command_new (query_str, GDA_COMMAND_TYPE_SQL,
				   GDA_COMMAND_OPTION_STOP_ON_ERRORS);
	
/*	DEBUG_PRINT ("query is %s", query_str);*/
	if ( (data = gda_connection_execute_select_command (priv->db_connection, 
											command, NULL, NULL)) == NULL ||
	  	gda_data_model_get_n_rows (data) <= 0 ) 
	{
		gda_command_free (command);
		g_free (query_str);
		g_string_free (info_data, FALSE);
		g_string_free (join_data, FALSE);
		return NULL;
	}
	
	gda_command_free (command);
	g_free (query_str);
	g_string_free (info_data, FALSE);
	g_string_free (join_data, FALSE);
	
	return (SymbolDBEngineIterator *)symbol_db_engine_iterator_new (data);	
	
}


/**
 * Sometimes it's useful going to query just with ids [and so integers] to have
 * a little speed improvement.
 */
SymbolDBEngineIterator *
symbol_db_engine_get_scope_members_by_symbol_id (SymbolDBEngine *dbe, 
									gint scope_parent_symbol_id, gint sym_info)
{
/*
select b.* from symbol a, symbol b where a.symbol_id = 348 and 
			b.scope_id = a.scope_definition_id;
*/
	SymbolDBEnginePriv *priv;
	gchar *query_str;	
	GdaCommand *command;
	GdaDataModel *data;
	GString *info_data;
	GString *join_data;
	
	g_return_val_if_fail (dbe != NULL, NULL);
	priv = dbe->priv;

	if (scope_parent_symbol_id <= 0)
		return NULL;

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
	query_str = g_strdup_printf ("SELECT symbol.symbol_id, symbol.name, "
		"symbol.file_position, "
		"symbol.is_file_scope, symbol.signature %s FROM symbol a, symbol symbol "
		"%s WHERE a.symbol_id = '%d' AND symbol.scope_id = a.scope_definition_id "
		"AND symbol.scope_id > 0", info_data->str, join_data->str,
								 scope_parent_symbol_id);
	
/*	DEBUG_PRINT ("DYNAMIC query is %s", query_str);*/
	command = gda_command_new (query_str, GDA_COMMAND_TYPE_SQL,
				   GDA_COMMAND_OPTION_STOP_ON_ERRORS);

	if ( (data = gda_connection_execute_select_command (priv->db_connection, 
											command, NULL, NULL)) == NULL ||
		  gda_data_model_get_n_rows (data) <= 0 ) 
	{
		gda_command_free (command);
		g_free (query_str);
		return NULL;			  
	}

	gda_command_free (command);
	g_free (query_str);
	g_string_free (info_data, FALSE);
	g_string_free (join_data, FALSE);
	
	return (SymbolDBEngineIterator *)symbol_db_engine_iterator_new (data);	
}

/* scope_path cannot be NULL.
 * scope_path will be something like "scope1_kind", "scope1_name", "scope2_kind", 
 * "scope2_name", NULL 
 */
SymbolDBEngineIterator *
symbol_db_engine_get_scope_members (SymbolDBEngine *dbe, 
									const GPtrArray* scope_path, gint sym_info)
{
/*
simple scope 
	
select * from symbol where scope_id = (
	select scope.scope_id from scope 
		inner join sym_type on scope.type_id = sym_type.type_id 
		where sym_type.type = 'class' 
			and scope.scope = 'MyClass'
	);
	
select * from symbol where scope_id = (
	select scope.scope_id from scope 
		inner join sym_type on scope.type_id = sym_type.type_id 
		where sym_type.type = 'struct' 
			and scope.scope = '_faa_1');
	
	
es. scope_path = First, namespace, Second, namespace, NULL, 
	symbol_name = Second_1_class	
*/
	SymbolDBEnginePriv *priv;
	gchar *query_str;	
	GdaCommand *command;
	GdaDataModel *data;
	gint final_definition_id;
	GString *info_data;
	GString *join_data;
	
	g_return_val_if_fail (dbe != NULL, NULL);
	priv = dbe->priv;

	final_definition_id = sdb_engine_walk_down_scope_path (dbe, scope_path);
	
	if (final_definition_id <= 0) 
	{
		return NULL;
	}

	/* info_data contains the stuff after SELECT and befor FROM */
	info_data = g_string_new ("");
	
	/* join_data contains the optionals joins to do to retrieve new data on other
	 * tables.
	 */
	join_data = g_string_new ("");

	/* fill info_data and join data with optional sql */
	sdb_engine_prepare_symbol_info_sql (dbe, info_data, join_data, sym_info);
	
	query_str = g_strdup_printf ("SELECT symbol.symbol_id, symbol.name, "
		"symbol.file_position, "
		"symbol.is_file_scope, symbol.signature %s FROM symbol "
		"%s WHERE scope_id = '%d'", info_data->str, join_data->str,
								 final_definition_id);
	
/*	DEBUG_PRINT ("DYNAMIC query is %s", query_str);*/
	command = gda_command_new (query_str, GDA_COMMAND_TYPE_SQL,
				   GDA_COMMAND_OPTION_STOP_ON_ERRORS);

	if ( (data = gda_connection_execute_select_command (priv->db_connection, 
											command, NULL, NULL)) == NULL ||
		  gda_data_model_get_n_rows (data) <= 0 ) 
	{
		gda_command_free (command);
		g_free (query_str);
		return NULL;			  
	}

	gda_command_free (command);
	g_free (query_str);
	g_string_free (info_data, FALSE);
	g_string_free (join_data, FALSE);
	
	return (SymbolDBEngineIterator *)symbol_db_engine_iterator_new (data);	
}

/* Returns an iterator to the data retrieved from database. 
 * It will be possible to get the scope specified by the line of the file. 
 */
SymbolDBEngineIterator *
symbol_db_engine_get_current_scope (SymbolDBEngine *dbe, const gchar* filename, 
									gulong line)
{
	SymbolDBEnginePriv *priv;
	gchar *file_escaped;
	gchar *query_str;
	GdaCommand *command;
	GdaDataModel *data;
	
	g_return_val_if_fail (dbe != NULL, NULL);
	priv = dbe->priv;
	
	
	file_escaped = g_strescape (filename, NULL);

	/* WARNING: probably there can be some problems with escaping file names here.
	 * They should come already escaped as from project db.
	 */	 
	query_str = g_strdup_printf ("SELECT symbol.symbol_id, symbol.name, "
		"symbol.file_position, "
		"symbol.is_file_scope, symbol.signature, MIN('%d' - symbol.file_position) "
		"FROM symbol "
			"JOIN file ON file_defined_id = file_id "
			"WHERE file.file_path = \"%s\" "
				"AND '%d' - symbol.file_position >= 0", (gint)line, file_escaped,
								 (gint)line);

	DEBUG_PRINT ("current_scope query is %s", query_str);
	
	command = gda_command_new (query_str, GDA_COMMAND_TYPE_SQL,
				   GDA_COMMAND_OPTION_STOP_ON_ERRORS);

	if ( (data = gda_connection_execute_select_command (priv->db_connection, 
											command, NULL, NULL)) == NULL ||
		  gda_data_model_get_n_rows (data) <= 0 ) 
	{
		gda_command_free (command);
		g_free (query_str);
		g_free (file_escaped);		
		return NULL;			  
	}

	gda_command_free (command);
	g_free (query_str);
	g_free (file_escaped);
	return (SymbolDBEngineIterator *)symbol_db_engine_iterator_new (data);
}


/**
 * Filepath: full local file path, e.g. /home/user/foo/file.c
 */
SymbolDBEngineIterator *
symbol_db_engine_get_file_symbols (SymbolDBEngine *dbe, 
									const gchar *file_path, gint sym_info)
{
	SymbolDBEnginePriv *priv;
	gchar *query_str;	
	GdaCommand *command;
	GdaDataModel *data;
	GString *info_data;
	GString *join_data;
	
	g_return_val_if_fail (dbe != NULL, NULL);
	g_return_val_if_fail (file_path != NULL, NULL);
	priv = dbe->priv;

	g_return_val_if_fail (priv->data_source != NULL, NULL);

	/* check for an already flagged sym_info with FILE_PATH. SYMINFO_FILE_PATH on 
	 * sym_info is already contained into the default query infos.
	 */
	sym_info = sym_info & ~SYMINFO_FILE_PATH;
	
	/* info_data contains the stuff after SELECT and befor FROM */
	info_data = g_string_new ("");

	/* join_data contains the optionals joins to do to retrieve new data on other
	 * tables.
	 */
	join_data = g_string_new ("");

	/* fill info_data and join data with optional sql */
	sdb_engine_prepare_symbol_info_sql (dbe, info_data, join_data, sym_info);

	/* rember to do a file_path + strlen(priv->data_source): a project relative 
	 * file path 
	 */
	query_str = g_strdup_printf ("SELECT symbol.symbol_id, symbol.name, "
		"symbol.file_position, "
		"symbol.is_file_scope, symbol.signature %s FROM symbol "
			"JOIN file ON symbol.file_defined_id = file.file_id "
		"%s WHERE file.file_path = \"%s\"", info_data->str, join_data->str,
								 file_path + strlen(priv->data_source));
	
	DEBUG_PRINT ("DYNAMIC query [file symbols] is %s", query_str); 
	command = gda_command_new (query_str, GDA_COMMAND_TYPE_SQL,
				   GDA_COMMAND_OPTION_STOP_ON_ERRORS);

	if ( (data = gda_connection_execute_select_command (priv->db_connection, 
											command, NULL, NULL)) == NULL ||
		  gda_data_model_get_n_rows (data) <= 0 ) 
	{
		gda_command_free (command);
		g_free (query_str);
		return NULL;			  
	}

	gda_command_free (command);
	g_free (query_str);
	g_string_free (info_data, FALSE);
	g_string_free (join_data, FALSE);

	return (SymbolDBEngineIterator *)symbol_db_engine_iterator_new (data);	
}


SymbolDBEngineIterator *
symbol_db_engine_get_symbol_info_by_id (SymbolDBEngine *dbe, 
									gint sym_id, gint sym_info)
{
	SymbolDBEnginePriv *priv;
	gchar *query_str;	
	GdaCommand *command;
	GdaDataModel *data;
	GString *info_data;
	GString *join_data;
	
	g_return_val_if_fail (dbe != NULL, NULL);
	priv = dbe->priv;

	/* info_data contains the stuff after SELECT and befor FROM */
	info_data = g_string_new ("");
	
	/* join_data contains the optionals joins to do to retrieve new data on other
	 * tables.
	 */
	join_data = g_string_new ("");

	/* fill info_data and join data with optional sql */
	sdb_engine_prepare_symbol_info_sql (dbe, info_data, join_data, sym_info);
	
	query_str = g_strdup_printf ("SELECT symbol.symbol_id, symbol.name, "
		"symbol.file_position, "
		"symbol.is_file_scope, symbol.signature %s FROM symbol "
		"%s WHERE symbol.symbol_id = %d", info_data->str, join_data->str,
								 sym_id);
	
/*	DEBUG_PRINT ("DYNAMIC query is %s", query_str);*/
	command = gda_command_new (query_str, GDA_COMMAND_TYPE_SQL,
				   GDA_COMMAND_OPTION_STOP_ON_ERRORS);

	if ( (data = gda_connection_execute_select_command (priv->db_connection, 
											command, NULL, NULL)) == NULL ||
		  gda_data_model_get_n_rows (data) <= 0 ) 
	{
		gda_command_free (command);
		g_free (query_str);
		return NULL;			  
	}

	gda_command_free (command);
	g_free (query_str);
	g_string_free (info_data, FALSE);
	g_string_free (join_data, FALSE);

	return (SymbolDBEngineIterator *)symbol_db_engine_iterator_new (data);	
	
}

/* user must free the returned value */
gchar*
symbol_db_engine_get_full_local_path (SymbolDBEngine *dbe, const gchar* file)
{
	SymbolDBEnginePriv *priv;
	gchar *full_path;
	
	g_return_val_if_fail (dbe != NULL, NULL);
	
	priv = dbe->priv;
/*	DEBUG_PRINT ("joining %s with %s", priv->data_source, file);*/
	full_path = g_strdup_printf ("%s%s", priv->data_source, file);
	return full_path;	
}


SymbolDBEngineIterator *
symbol_db_engine_find_symbol_by_name_pattern (SymbolDBEngine *dbe, 
									const gchar *name, gint sym_info)
{
	SymbolDBEnginePriv *priv;
	gchar *query_str;	
	GdaCommand *command;
	GdaDataModel *data;
	GString *info_data;
	GString *join_data;
	
	g_return_val_if_fail (dbe != NULL, NULL);
	priv = dbe->priv;

	/* info_data contains the stuff after SELECT and befor FROM */
	info_data = g_string_new ("");
	
	/* join_data contains the optionals joins to do to retrieve new data on other
	 * tables.
	 */
	join_data = g_string_new ("");

	/* fill info_data and join data with optional sql */
	sdb_engine_prepare_symbol_info_sql (dbe, info_data, join_data, sym_info);
	
	query_str = g_strdup_printf ("SELECT symbol.symbol_id, symbol.name, "
		"symbol.file_position, "
		"symbol.is_file_scope, symbol.signature %s FROM symbol "
		"%s WHERE symbol.name LIKE \"%s%%\"", info_data->str, join_data->str,
								 name);
	
/*	DEBUG_PRINT ("DYNAMIC query is %s", query_str);*/
	command = gda_command_new (query_str, GDA_COMMAND_TYPE_SQL,
				   GDA_COMMAND_OPTION_STOP_ON_ERRORS);

	if ( (data = gda_connection_execute_select_command (priv->db_connection, 
											command, NULL, NULL)) == NULL ||
		  gda_data_model_get_n_rows (data) <= 0 ) 
	{
		gda_command_free (command);
		g_free (query_str);
		return NULL;			  
	}

	gda_command_free (command);			
	g_free (query_str);
	g_string_free (info_data, FALSE);
	g_string_free (join_data, FALSE);

	return (SymbolDBEngineIterator *)symbol_db_engine_iterator_new (data);	
}

/* No iterator for now. We need the quickest query possible. */
gint
symbol_db_engine_get_parent_scope_id_by_symbol_id (SymbolDBEngine *dbe, 
									gint scoped_symbol_id)
{
/*
select * from symbol where scope_definition_id = (
	select scope_id from symbol where symbol_id = 26
	);
*/
	
	/* again we're without prepared queries support from libgda... hope
	 * you guys implement that asap..!
	 */
	SymbolDBEnginePriv *priv;
	gchar *query_str;	
	GdaCommand *command;
	GdaDataModel *data;
	const GValue* value;
	
	g_return_val_if_fail (dbe != NULL, -1);
	priv = dbe->priv;

	query_str = g_strdup_printf ("SELECT symbol.symbol_id FROM symbol "
			"WHERE symbol.scope_definition_id = ( "
			"SELECT symbol.scope_id FROM symbol WHERE symbol.symbol_id = '%d')", 
								 scoped_symbol_id);
	
	DEBUG_PRINT ("symbol_db_engine_get_parent_scope_id_by_symbol_id() query is %s", 
				 query_str);
	command = gda_command_new (query_str, GDA_COMMAND_TYPE_SQL,
				   GDA_COMMAND_OPTION_STOP_ON_ERRORS);

	if ( (data = gda_connection_execute_select_command (priv->db_connection, 
											command, NULL, NULL)) == NULL ||
		  gda_data_model_get_n_rows (data) <= 0 ) 
	{
		gda_command_free (command);
		g_free (query_str);
		DEBUG_PRINT ("BAILING OUT");
		return -1;
	}

	gda_command_free (command);
	g_free (query_str);

	value = gda_data_model_get_value_at (data, 0, 0);
	gint res = value != NULL && G_VALUE_HOLDS_INT (value)
		? g_value_get_int (value) : -1;
	g_object_unref (data);
	
	return res;
}
