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

#ifndef _SYMBOL_DB_ENGINE_QUERIES_H_
#define _SYMBOL_DB_ENGINE_QUERIES_H_

#include <glib-object.h>
#include <glib.h>

#include "symbol-db-engine.h"
#include "symbol-db-engine-iterator.h"



/**********************
 * ITERATABLE QUERIES
 **********************/

/* Usually you'll see many functions that have a gint symbol_id parameter.
 * You may wonder why we don't pass a SymbolDBEngineIteratorNode. Well,
 * because of the iterator architecture and its underlying data (the 
 * GdaDataModel): you can easily save an integer id, but not an IteratorNode, which
 * would change after a symbol_db_engine_iterator_move_next ().
 * Think of that integer id as an handle, and use it as that.
 */


/**
 * Use this function to find symbols names by patterns like '%foo_func%'
 * that will return a family of my_foo_func_1, your_foo_func_2 etc
 * @param pattern must not be NULL.
 * It must include the optional '%' character to have a wider match, e.g. "foo_func%"
 */
SymbolDBEngineIterator *
symbol_db_engine_find_symbol_by_name_pattern (SymbolDBEngine *dbe, 
									const gchar *pattern, 
									gboolean case_sensitive,
									SymExtraInfo sym_info);

/**
 * @param pattern Pattern you want to search for. It cannot be NULL. If you want to search for
 *        everything then use a pattern like "%".
 * @param filter_kinds Can be set to SYMTYPE_UNDEF. In that case these filters will not be taken into consideration.
 * @param include_kinds Should the filter_kinds (if not null) be applied as inluded or excluded?
 * @param filescope_search If SYMSEARCH_FILESCOPE_PUBLIC only global public 
 *		  function will be searched (the ones that _do not_ belong to the file scope). 
 *        If SYMSEARCH_FILESCOPE_PRIVATE even private or 
 *        static (for C language) will be searched (the ones that _do_ belong to the file scope). 
 *        SYMSEARCH_FILESCOPE_IGNORE to be ignored (e.g. Will search both private and public scopes). 
 *        You cannot use bitwise OR in this parameter.
 * @param session_projects Should the search, a global search, be filtered by some packages (projects)?
 *        If yes then provide a GList, if no then pass NULL.
 * @param results_limit Limit results to an upper bound. -1 If you don't want to use this par.
 * @param results_offset Skip results_offset results. -1 If you don't want to use this par.
 * @param sym_info Infos about symbols you want to know.
 */
SymbolDBEngineIterator *
symbol_db_engine_find_symbol_by_name_pattern_filtered (SymbolDBEngine *dbe, 
									const gchar *pattern, 
									SymType filter_kinds,
									gboolean include_kinds,
									SymSearchFileScope filescope_search,
									const GList *session_projects,													   
									gint results_limit, 
									gint results_offset,
									SymExtraInfo sym_info);

/**
 * @param pattern Pattern you want to search for. If NULL it will use '%' and LIKE for query.
 * @param full_local_file_path Path of the file you want to search in.
 * @param filter_kinds Can be set to SYMTYPE_UNDEF. In that case these filters will not be taken into consideration.
 * @param include_kinds Should the filter_kinds (if not null) be applied as inluded or excluded?
 * @param results_limit Limit results to an upper bound. -1 If you don't want to use this par.
 * @param results_offset Skip results_offset results. -1 If you don't want to use this par.
 * @param sym_info Infos about symbols you want to know.
 */
SymbolDBEngineIterator *
symbol_db_engine_find_symbol_by_name_pattern_on_file (SymbolDBEngine *dbe,
									const gchar *pattern,
									const gchar *full_local_file_path,
									SymType filter_kinds,
									gboolean include_kinds,
									gint results_limit,
									gint results_offset,
									SymExtraInfo sym_info);

/**
 * Usually, for instance in a completion engine we have to search for symbols
 * that are part of a container symbol, even for their existence.
 * This function would be useful and fast in those cases.
 *
 * @param pattern Pattern you want to search for. If NULL it will use '%' and LIKE for query.
 * @param container_symbol_id The container symbol id where you want to search in.
 * @param filter_kinds Can be set to SYMTYPE_UNDEF. In that case these filters will not be taken into consideration.
 * @param include_kinds Should the filter_kinds (if not null) be applied as inluded or excluded?
 * @param results_limit Limit results to an upper bound. -1 If you don't want to use this par.
 * @param results_offset Skip results_offset results. -1 If you don't want to use this par.
 * @param sym_info Infos about symbols you want to know. 
 */
SymbolDBEngineIterator *
symbol_db_engine_find_symbol_in_scope (SymbolDBEngine *dbe,
    								const gchar *pattern,
    								gint container_symbol_id,
									SymType filter_kinds,
									gboolean include_kinds,
									gint results_limit,
									gint results_offset,    
    								SymExtraInfo sym_info);

/**
 * TODO: Implement a function that, given a container_symbol_id, is able to determine
 * if a query_symbol_id is under the scope of container_symbol_id, in a multi level fashion.
 * Infact it wouldn't be performant to use symbol_db_engine_find_symbol_in_scope ()
 * and recursively search for every sub-scope looking for the query_symbol_id.
 */

/**
 * Return an iterator to the data retrieved from database. 
 * The iterator, if not null, will contain a list of parent classes for the 
 * given symbol name.
 */
SymbolDBEngineIterator *
symbol_db_engine_get_class_parents (SymbolDBEngine *dbe, 
    								const gchar *klass_name, 
									const GPtrArray *scope_path, 
    								SymExtraInfo sym_info);

/**
 * Use this function to get parent symbols of a given class.
 */
SymbolDBEngineIterator *
symbol_db_engine_get_class_parents_by_symbol_id (SymbolDBEngine *dbe, 
												 gint child_klass_symbol_id,
												 SymExtraInfo sym_info);

/**
 * Get the scope specified by the line of the file. 
 * Iterator should contain just one element if the query is successful, no element
 * or NULL is returned if function went wrong.
 */
SymbolDBEngineIterator *
symbol_db_engine_get_current_scope (SymbolDBEngine *dbe, 
									const gchar* full_local_file_path, 
    								gulong line, 
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
 * @param filter_kinds Can be set to SYMTYPE_UNDEF. In that case these filters will not be taken into consideration.
 * at root level [global level]. A maximum of 255 filter_kinds are admitted.
 * @param include_kinds Should we include in the result the filter_kinds or not?
 * @param group_them If TRUE then will be issued a 'group by symbol.name' option.
 * 		If FALSE you can have as result more symbols with the same name but different
 * 		symbols id. See for example more namespaces declared on different files.
 * @param results_limit Limit results to an upper bound. -1 If you don't want to use this par.
 * @param results_offset Skip results_offset results. -1 If you don't want to use this par. 
 */
SymbolDBEngineIterator *
symbol_db_engine_get_global_members_filtered (SymbolDBEngine *dbe, 
									SymType filter_kinds,
									gboolean include_kinds, 
									gboolean group_them,
									gint results_limit, 
									gint results_offset,
								 	SymExtraInfo sym_info);

/** 
 * No iterator for now. We need the quickest query possible.
 * @param scoped_symbol_id Symbol you want to know the parent of.
 * @param db_file db-relative filename path. eg. /src/foo.c Can be NULL.
 */
gint
symbol_db_engine_get_parent_scope_id_by_symbol_id (SymbolDBEngine *dbe, 
									gint scoped_symbol_id,
									const gchar* db_file);

/** 
 * This is the same version as symbol_db_engine_get_parent_scope_id_by_symbol_id () 
 * but with an Iterator as return type. 
 * It includes the calls to symbol_db_engine_get_parent_scope_id_by_symbol_id () and
 * symbol_db_engine_get_symbol_info_by_id (). It's a sort of facade.
 * @param scoped_symbol_id Symbol you want to know the parent of.
 * @param db_file db-relative filename path. eg. /src/foo.c. Can be NULL for a
 * wider search (i.e. not just limited to that file).
 * @return NULL on error or if nothing is found as parent.
 */
SymbolDBEngineIterator *
symbol_db_engine_get_parent_scope_by_symbol_id (SymbolDBEngine *dbe, 
									gint scoped_symbol_id,
									const gchar* db_file,
    								SymExtraInfo sym_info);

/**
 * Walk the path up to the root scope given a scoped_symbol_id parameter.
 * The returned iterator will be populated with SymbolDBEngineIteratorNode(s)
 * so that it could be easily browsed by a client app.
 *
 * e.g.
 * namespace FooBase {
 * class FooKlass {
 *	
 * }
 *
 * void FooKlass::foo_func () {			<-------------- this is the scoped symbol
 *              
 * }
 * 
 * the returned iterator'll contain symbols in this order: foo_func, FooKlass, FooBase.
 *
 * @param db_file db-relative filename path. eg. /src/foo.c. Can be NULL for a
 * wider search (i.e. not just limited to that file).
 * @return NULL on error or if scope isn't found.
 */
SymbolDBEngineIterator *
symbol_db_engine_get_scope_chain (SymbolDBEngine *dbe,
    								gint scoped_symbol_id,
    								const gchar* db_file,
    								SymExtraInfo sym_info);

/**
 * Walk the path up to the root scope given a full_local_file_path and a line number.
 * The returned iterator will be populated with SymbolDBEngineIteratorNode(s)
 * so that it could be easily browsed by a client app.
 *
 * @return NULL on error or if scope isn't found.
 */
SymbolDBEngineIterator *
symbol_db_engine_get_scope_chain_by_file_line (SymbolDBEngine *dbe,
									const gchar* full_local_file_path, 
    								gulong line,
    								SymExtraInfo sym_info);


/** 
 * scope_path cannot be NULL.
 * scope_path will be something like "scope1_kind", "scope1_name", "scope2_kind", 
 * "scope2_name", NULL 
 */
SymbolDBEngineIterator *
symbol_db_engine_get_scope_members_by_path (SymbolDBEngine *dbe, 
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
 * You can specify which kind of symbols to retrieve, and if to include them or exclude.
 * Kinds are 'namespace', 'class' etc.
 * @param filter_kinds Can be set to SYMTYPE_UNDEF. In that case these filters will not be taken into consideration.
 * @param results_limit Limit results to an upper bound. -1 If you don't want to use this par.
 * @param results_offset Skip results_offset results. -1 If you don't want to use this par. 
 */
SymbolDBEngineIterator *
symbol_db_engine_get_scope_members_by_symbol_id_filtered (SymbolDBEngine *dbe, 
									gint scope_parent_symbol_id, 
									SymType filter_kinds,
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

/**
 * Get the files of a project.
 * @param project_name name of project you want to know the files of.
 *        It can be NULL. In that case all the files will be returned.
 */
SymbolDBEngineIterator *
symbol_db_engine_get_files_for_project (SymbolDBEngine *dbe, 
									const gchar *project_name,
								 	SymExtraInfo sym_info);

/**
 * Get the number of languages used in a project.
 *
 * @return number of different languages used in the opened project. -1 on error.
 */
gint
symbol_db_engine_get_languages_count (SymbolDBEngine *dbe);

/**
 *
 * @return true if the language is used in the opened project.
 */
gboolean
symbol_db_engine_is_language_used (SymbolDBEngine *dbe,
								   const gchar *language);

#endif
