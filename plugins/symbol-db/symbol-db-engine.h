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
	void (* scan_end) ();
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


SymbolDBEngine* symbol_db_engine_new (void);


/**
 * Be sure to check lock status with this function before calling
 * something else below. If you call a scanning function while
 * dbe is locked there can be some weird behaviours.
 */
gboolean
symbol_db_engine_is_locked (SymbolDBEngine *dbe);

/**
 * Open or create a new database. 
 * Be sure to give a base_prj_path with the ending '/' for directory.
 * E.g: a project on '/tmp/foo/' dir.
 */
gboolean 
symbol_db_engine_open_db (SymbolDBEngine *dbe, const gchar* base_prj_path);

/**
 * Check if the database already exists into the prj_directory
 */
gboolean
symbol_db_engine_db_exists (SymbolDBEngine * dbe, const gchar * prj_directory);


/** Add a new workspace to database. */
gboolean 
symbol_db_engine_add_new_workspace (SymbolDBEngine *dbe, const gchar* workspace);


/** Add a new project to workspace.*/
gboolean 
symbol_db_engine_add_new_project (SymbolDBEngine *dbe, const gchar* workspace, 
								  const gchar* project);

/**
 * Return the name of the opened project.
 * NULL on error. Returned string must be freed by caller.
 */
gchar*
symbol_db_engine_get_opened_project_name (SymbolDBEngine * dbe);


/** 
 * Open a project. Return false if project isn't created/opened. 
 * This function *must* be called before any other operation on db.
 * Another option would be create a fresh new project: that way will also open it.
 */ 
gboolean 
symbol_db_engine_open_project (SymbolDBEngine *dbe, /*gchar* workspace, */
								  const gchar* project_name);


/** Disconnect db, gda client and db_connection and close the project */
gboolean 
symbol_db_engine_close_project (SymbolDBEngine *dbe, const gchar* project_name);


/** 
 * Add a group of files of a single language to a project. It will perform also 
 * a symbols scannig/populating of db if scan_symbols is TRUE.
 * This function requires an opened db, i.e. calling before
 * symbol_db_engine_open_db ().
 * @note if some file fails to enter the db the function will return without
 * processing the remaining files.
 * @param files_path requires full path to files on disk. Ctags itself requires that.
 *        it must be something like /path/to/my/foo/file.xyz
 */
gboolean 
symbol_db_engine_add_new_files (SymbolDBEngine *dbe, const gchar* project,
							    const GPtrArray *files_path,
								const gchar *language, gboolean scan_symbols);

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
									   gboolean update_prj_analize_time);

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
 * Will test the opened project within the dbe plugin and the passed one.
 */
gboolean inline
symbol_db_engine_is_project_opened (SymbolDBEngine *dbe, const gchar* project_name);

/**
 * Return an iterator to the data retrieved from database. 
 * It will be possible to get the scope specified by the line of the file. 
 */
SymbolDBEngineIterator *
symbol_db_engine_get_current_scope (SymbolDBEngine *dbe, 
									const gchar* filename, gulong line);

/**
 * Return an iterator to the data retrieved from database. 
 * The iterator, if not null, will contain a list of parent classes for the 
 * given symbol name.
 */
SymbolDBEngineIterator *
symbol_db_engine_get_class_parents (SymbolDBEngine *dbe, const gchar *klass_name, 
									 const GPtrArray *scope_path);


/**
 * scope_path cannot be NULL.
 * scope_path will be something like "scope1_kind", "scope1_name", "scope2_kind", 
 * "scope2_name", NULL 
 */
SymbolDBEngineIterator *
symbol_db_engine_get_scope_members (SymbolDBEngine *dbe, 									
									const GPtrArray* scope_path,
									gint sym_info);

/**
 * kind can be NULL. In that case we'll return all the kinds of symbols found
 * at root level [global level].
 */
SymbolDBEngineIterator *
symbol_db_engine_get_global_members (SymbolDBEngine *dbe, 
									const gchar *kind, gint sym_info);

SymbolDBEngineIterator *
symbol_db_engine_get_file_symbols (SymbolDBEngine *dbe, 
									const gchar *file_path, gint sym_info);

SymbolDBEngineIterator *
symbol_db_engine_get_symbol_info_by_id (SymbolDBEngine *dbe, 
									gint sym_id, gint sym_info);

/**
 * Use this function to find symbols names by patterns like '%foo_func%'
 * that will return a family of my_foo_func_1, your_foo_func_2 etc
 */
SymbolDBEngineIterator *
symbol_db_engine_find_symbol_by_name_pattern (SymbolDBEngine *dbe, 
									const gchar *name, gint sym_info);

/**
 * Sometimes it's useful going to query just with ids [and so integers] to have
 * a little speed improvement.
 */
SymbolDBEngineIterator *
symbol_db_engine_get_scope_members_by_symbol_id (SymbolDBEngine *dbe, 
									gint scope_parent_symbol_id, gint sym_info);

/** 
 * No iterator for now. We need the quickest query possible.
 * @param scoped_symbol_id Symbol you want to know the parent of.
 * @param db_file db-relative filename path. eg. /src/foo.c
 */
gint
symbol_db_engine_get_parent_scope_id_by_symbol_id (SymbolDBEngine *dbe, 
									gint scoped_symbol_id,
									const gchar* db_file);

G_END_DECLS

#endif /* _SYMBOL_DB_ENGINE_H_ */


