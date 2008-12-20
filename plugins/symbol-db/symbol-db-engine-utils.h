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

#ifndef _SYMBOL_DB_ENGINE_UTILS_H_
#define _SYMBOL_DB_ENGINE_UTILS_H_

#include <glib-object.h>
#include <glib.h>


/**
 * Simple compare function for GTree(s)
 */
gint
symbol_db_gtree_compare_func (gconstpointer a, gconstpointer b, gpointer user_data);

/**
 * Simple compare function for GList(s)
 */
gint
symbol_db_glist_compare_func (gconstpointer a, gconstpointer b);


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



#endif
