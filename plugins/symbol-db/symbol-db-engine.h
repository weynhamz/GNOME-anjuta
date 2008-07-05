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

#ifndef _SYMBOL_DB_ENGINE_H_
#define _SYMBOL_DB_ENGINE_H_

#include <glib-object.h>
#include <glib.h>
#include <libanjuta/interfaces/ianjuta-symbol.h>
#include "symbol-db-engine-iterator.h"

G_BEGIN_DECLS

#define SYMBOL_TYPE_DB_ENGINE             (sdb_engine_get_type ())
#define SYMBOL_DB_ENGINE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SYMBOL_TYPE_DB_ENGINE, SymbolDBEngine))
#define SYMBOL_DB_ENGINE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SYMBOL_TYPE_DB_ENGINE, SymbolDBEngineClass))
#define SYMBOL_IS_DB_ENGINE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SYMBOL_TYPE_DB_ENGINE))
#define SYMBOL_IS_DB_ENGINE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SYMBOL_TYPE_DB_ENGINE))
#define SYMBOL_DB_ENGINE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SYMBOL_TYPE_DB_ENGINE, SymbolDBEngineClass))

typedef struct _SymbolDBEngineClass SymbolDBEngineClass;
typedef struct _SymbolDBEngine SymbolDBEngine;
typedef struct _SymbolDBEnginePriv SymbolDBEnginePriv;

struct _SymbolDBEngineClass
{
	GObjectClass parent_class;
	
	/* signals */
	void (* single_file_scan_end) 	();
	void (* scan_end) 				();
	void (* symbol_inserted) 		(gint symbol_id);
	void (* symbol_updated)  		(gint symbol_id);
	void (* symbol_scope_updated)  	(gint symbol_id);	/* never emitted. */
	void (* symbol_removed)  		(gint symbol_id);
};

struct _SymbolDBEngine
{
	GObject parent_instance;
	SymbolDBEnginePriv *priv;
};


typedef enum {
	SYMINFO_SIMPLE = 0,
	SYMINFO_FILE_PATH = 1,
	SYMINFO_IMPLEMENTATION = 2,
	SYMINFO_ACCESS = 4,
	SYMINFO_KIND = 8,
	SYMINFO_TYPE = 16,
	SYMINFO_TYPE_NAME = 32,
	SYMINFO_LANGUAGE = 64,
	SYMINFO_FILE_IGNORE = 128,
	SYMINFO_FILE_INCLUDE = 256,
	SYMINFO_PROJECT_NAME = 512,
	SYMINFO_WORKSPACE_NAME = 1024
	
} SymExtraInfo;

GType sdb_engine_get_type (void) G_GNUC_CONST;


SymbolDBEngine* symbol_db_engine_new ();


/**
 * Be sure to check lock status with this function before calling
 * something else below. If you call a scanning function while
 * dbe is locked there can be some weird behaviours.
 */
gboolean
symbol_db_engine_is_locked (SymbolDBEngine *dbe);

/**
 * Open or create a new database. 
 * Be sure to give a base_db_path with the ending '/' for directory.
 * @param base_db_path directory where .anjuta_sym_db.db will be stored. It can be
 *        different from project_directory
 *        E.g: a db on '/tmp/foo/' dir.
 * @param prj_directory project directory. It may be different from base_db_path.
 *        It's mainly used to map files inside the db. Say for example that you want to
 *        add to a project a file /home/user/project/foo_prj/src/file.c with a project
 *        directory of /home/user/project/foo_prj/. On db it'll be represented as
 *        src/file.c. In this way you can move around the project dir without dealing
 *        with relative paths.
 */
gboolean 
symbol_db_engine_open_db (SymbolDBEngine *dbe, const gchar* base_db_path,
						  const gchar * prj_directory);


/** Disconnect db, gda client and db_connection */
gboolean 
symbol_db_engine_close_db (SymbolDBEngine *dbe);

/**
 * Check if the database already exists into the db_directory
 */
gboolean
symbol_db_engine_db_exists (SymbolDBEngine * dbe, const gchar * db_directory);

/**
 * Check if a file is already present [and scanned] in db.
 */
gboolean
symbol_db_engine_file_exists (SymbolDBEngine * dbe, const gchar * abs_file_path);

/** Add a new workspace to an opened database. */
gboolean 
symbol_db_engine_add_new_workspace (SymbolDBEngine *dbe, const gchar* workspace);


/** Add a new project to workspace to an opened database.*/
gboolean 
symbol_db_engine_add_new_project (SymbolDBEngine *dbe, const gchar* workspace, 
								  const gchar* project);

/** 
 * Test project existence. 
 * @return false if project isn't found
 */ 
gboolean 
symbol_db_engine_project_exists (SymbolDBEngine *dbe, /*gchar* workspace, */
								  const gchar* project_name);


/** 
 * Add a group of files of a single language to a project. It will perform also 
 * a symbols scannig/populating of db if scan_symbols is TRUE.
 * This function requires an opened db, i.e. calling before
 * symbol_db_engine_open_db ().
 * @note if some file fails to enter the db the function will return without
 * processing the remaining files.
 * @param project_name something like 'foo_project', or 'helloworld_project'
 * @param project_directory something like the base path '/home/user/projects/foo_project/'
 *        Be sure not to exchange the db_directory with project_directory! they're different!
 * @param files_path requires full path to files on disk. Ctags itself requires that.
 *        it must be something like "/home/path/to/my/foo/file.xyz". Also it requires
 *		  a language string to represent the file.
 *        An example of files_path array composition can be: 
 *        "/home/user/foo_project/foo1.c", "/home/user/foo_project/foo2.cpp", 
 * 		  "/home/user/foo_project/foo3.java".
 * @param languages is an array of 'languages'. It must have the same number of 
 *		  elments that files_path has. It should be populated like this: "C", "C++",
 *		  "Java"
 * 		  This is done to be uniform to the language-manager plugin.
 * @param force_scan If FALSE a check on db will be done to see
 *		  whether the file is already present or not.
 * @return true is insertion is successful.
 */
gboolean 
symbol_db_engine_add_new_files (SymbolDBEngine *dbe, 
								const gchar * project_name,
							    const GPtrArray *files_path,
								const GPtrArray *languages,
								gboolean force_scan);

/**
 * Update symbols of the whole project. It scans all file symbols etc. 
 * If force is true then update forcely all the files.
 */
gboolean 
symbol_db_engine_update_project_symbols (SymbolDBEngine *dbe, const gchar *project);


/** Remove a file, together with its symbols, from a project. */
gboolean 
symbol_db_engine_remove_file (SymbolDBEngine *dbe, const gchar* project, 
							  const gchar* file);

/**
 * Update symbols of saved files. 
 * WARNING: files_path and it's contents will be freed on callback.
 */
gboolean 
symbol_db_engine_update_files_symbols (SymbolDBEngine *dbe, const gchar *project, 
									   GPtrArray *files_path,
									   gboolean update_prj_analyse_time);

/**
 * Update symbols of a file by a memory-buffer to perform a real-time updating 
 * of symbols. 
 */
gboolean
symbol_db_engine_update_buffer_symbols (SymbolDBEngine * dbe, const gchar * project,
										GPtrArray * real_files_list,
										const GPtrArray * text_buffers,
										const GPtrArray * buffer_sizes);

/**
 * Return full_local_path given a relative-to-db file path.
 * User must care to free the returned string.
 * @param db_file Relative path inside project.
 */
gchar*
symbol_db_engine_get_full_local_path (SymbolDBEngine *dbe, const gchar* db_file);


/**
 * Return a db-relativ file path. Es. given the full_local_file_path 
 * /home/user/foo_project/src/foo.c returned file should be /src/foo.c.
 * Return NULL on error.
 */
gchar*
symbol_db_engine_get_file_db_path (SymbolDBEngine *dbe, const gchar* full_local_file_path);

/** 
 * Hash table that converts from a char like 'class' 'struct' etc to an 
 * IANJUTA_SYMBOL_TYPE
 */
const GHashTable*
symbol_db_engine_get_sym_type_conversion_hash (SymbolDBEngine *dbe);

/**
 * Return a GPtrArray that must be freed from caller.
 */
GPtrArray *
symbol_db_engine_fill_type_array (IAnjutaSymbolType match_types);

/**
 * Try to get all the files with zero symbols: these should be the ones
 * excluded by an abort on population process.
 * @return A GPtrArray with paths on disk of the files. Must be freed by caller.
 * @return NULL if no files are found.
 */
GPtrArray *
symbol_db_engine_get_files_with_zero_symbols (SymbolDBEngine *dbe);


/**********************
 * ITERATABLE QUERIES
 **********************/

/**
 * Use this function to find symbols names by patterns like '%foo_func%'
 * that will return a family of my_foo_func_1, your_foo_func_2 etc
 * @name must not be NULL.
 * @name must include the optional '%' character to have a wider match, e.g. "foo_func%"
 */
SymbolDBEngineIterator *
symbol_db_engine_find_symbol_by_name_pattern (SymbolDBEngine *dbe, 
									const gchar *pattern, SymExtraInfo sym_info);

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
SymbolDBEngineIterator *
symbol_db_engine_find_symbol_by_name_pattern_filtered (SymbolDBEngine *dbe, 
									const gchar *pattern, 
									gboolean exact_match,
									const GPtrArray *filter_kinds,
									gboolean include_kinds,
									gboolean global_symbols_search,
									gint results_limit, 
									gint results_offset,
									SymExtraInfo sym_info);


/**
 * Return an iterator to the data retrieved from database. 
 * The iterator, if not null, will contain a list of parent classes for the 
 * given symbol name.
 */
SymbolDBEngineIterator *
symbol_db_engine_get_class_parents (SymbolDBEngine *dbe, const gchar *klass_name, 
									 const GPtrArray *scope_path, SymExtraInfo sym_info);

/**
 * Use this function to get parent symbols of a given class.
 */
SymbolDBEngineIterator *
symbol_db_engine_get_class_parents_by_symbol_id (SymbolDBEngine *dbe, 
												 gint child_klass_symbol_id,
												 SymExtraInfo sym_info);

/**
 * Return an iterator to the data retrieved from database. 
 * It will be possible to get the scope specified by the line of the file. 
 */
SymbolDBEngineIterator *
symbol_db_engine_get_current_scope (SymbolDBEngine *dbe, 
									const gchar* filename, gulong line, 
									 SymExtraInfo sym_info);


/**
 * Use this function to get symbols of a file.
 */
SymbolDBEngineIterator *
symbol_db_engine_get_file_symbols (SymbolDBEngine *dbe, 
								   const gchar *file_path, 
								   SymExtraInfo sym_info);

/**
 * Use this function to get global symbols only. I.e. private or file-only scoped symbols
 * will NOT be returned.
 * @param filter_kinds Can be NULL. In that case we'll return all the kinds of symbols found
 * at root level [global level]. A maximum of 5 filter_kinds are admitted.
 * @param include_kinds Should we include in the result the filter_kinds or not?
 * @param group_them If TRUE then will be issued a 'group by symbol.name' option.
 * 		If FALSE you can have as result more symbols with the same name but different
 * 		symbols id. See for example more namespaces declared on different files.
 * @param results_limit Limit results to an upper bound. -1 If you don't want to use this par.
 * @param results_offset Skip results_offset results. -1 If you don't want to use this par. 
 */
SymbolDBEngineIterator *
symbol_db_engine_get_global_members_filtered (SymbolDBEngine *dbe, 
									const GPtrArray *filter_kinds,
									gboolean include_kinds, 
									gboolean group_them,
									gint results_limit, 
									gint results_offset,
								 	SymExtraInfo sym_info);

/** 
 * No iterator for now. We need the quickest query possible.
 * @param scoped_symbol_id Symbol you want to know the parent of.
 * @param db_file db-relative filename path. eg. /src/foo.c
 */
gint
symbol_db_engine_get_parent_scope_id_by_symbol_id (SymbolDBEngine *dbe, 
									gint scoped_symbol_id,
									const gchar* db_file);

/** scope_path cannot be NULL.
 * scope_path will be something like "scope1_kind", "scope1_name", "scope2_kind", 
 * "scope2_name", NULL 
 */
SymbolDBEngineIterator *
symbol_db_engine_get_scope_members (SymbolDBEngine *dbe, 
									const GPtrArray* scope_path, 
									SymExtraInfo sym_info);

/**
 * Sometimes it's useful going to query just with ids [and so integers] to have
 * a little speed improvement.
 * @param results_limit Limit results to an upper bound. -1 If you don't want to use this par.
 * @param results_offset Skip results_offset results. -1 If you don't want to use this par. 
 */
SymbolDBEngineIterator *
symbol_db_engine_get_scope_members_by_symbol_id (SymbolDBEngine *dbe, 
									gint scope_parent_symbol_id, 
									gint results_limit,
									gint results_offset,
									SymExtraInfo sym_info);

/**
 * A filtered version of the symbol_db_engine_get_scope_members_by_symbol_id ().
 * You can specify which kind of symbols to retrieve, and if include them or exclude.
 * Kinds are 'namespace', 'class' etc.
 * @param filter_kinds cannot be NULL.
 * @param results_limit Limit results to an upper bound. -1 If you don't want to use this par.
 * @param results_offset Skip results_offset results. -1 If you don't want to use this par. 
 */
SymbolDBEngineIterator *
symbol_db_engine_get_scope_members_by_symbol_id_filtered (SymbolDBEngine *dbe, 
									gint scope_parent_symbol_id, 
									const GPtrArray *filter_kinds,
									gboolean include_kinds,														  
									gint results_limit,
									gint results_offset,
									SymExtraInfo sym_info);

/**
 * Use this function to get infos about a symbol.
 */
SymbolDBEngineIterator *
symbol_db_engine_get_symbol_info_by_id (SymbolDBEngine *dbe, 
									gint sym_id, 
									SymExtraInfo sym_info);

G_END_DECLS

#endif /* _SYMBOL_DB_ENGINE_H_ */

