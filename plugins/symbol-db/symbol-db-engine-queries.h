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
 * @param filter_kinds Can be NULL. In that case these filters will not be taken into consideration.
 * @param include_kinds Should the filter_kinds (if not null) be applied as inluded or excluded?
 * @param global_symbols_search If TRUE only global public function will be searched. If false
 *		  even private or static (for C language) will be searched.
 * @param session_projects Should the search, a global search, be filtered by some packages (projects)?
 *        If yes then provide a GList, if no then pass NULL.	 
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
									GList *session_projects,													   
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
 * Get the scope specified by the line of the file. 
 * Iterator should contain just one element if the query is successful, no element
 * or NULL is returned if function went wrong.
 */
SymbolDBEngineIterator *
symbol_db_engine_get_current_scope (SymbolDBEngine *dbe, 
									const gchar* full_local_file_path, gulong line, 
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

/** 
 * scope_path cannot be NULL.
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

/**
 * Gets the files of a project.
 * @param project_name name of project you want to know the files of.
 *        It can be NULL. In that case all the files will be returned.
 */
SymbolDBEngineIterator *
symbol_db_engine_get_files_for_project (SymbolDBEngine *dbe, 
									const gchar *project_name,
								 	SymExtraInfo sym_info);
#endif
