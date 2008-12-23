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

#ifndef _SYMBOL_DB_ENGINE_CORE_H_
#define _SYMBOL_DB_ENGINE_CORE_H_

#include <glib-object.h>
#include <glib.h>
#include <libanjuta/interfaces/ianjuta-symbol.h>
#include <libanjuta/anjuta-plugin.h>
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
	void (* scan_end) 				(gint process_id);
	void (* symbol_inserted) 		(gint symbol_id);
	void (* symbol_updated)  		(gint symbol_id);
	void (* symbol_scope_updated)  	(gint symbol_id);	
	void (* symbol_removed)  		(gint symbol_id);
};

struct _SymbolDBEngine
{
	GObject parent_instance;
	SymbolDBEnginePriv *priv;
};




GType sdb_engine_get_type (void) G_GNUC_CONST;


/**
 * Create a new instance of an engine. 
 * @param ctags_path is mandatory. No NULL value is accepted.
 */
SymbolDBEngine* 
symbol_db_engine_new (const gchar * ctags_path);

/**
 * Set a new path for ctags executable.
 */ 
void 
symbol_db_engine_set_ctags_path (SymbolDBEngine *dbe,
								  const gchar * ctags_path);

/**
 * Be sure to check lock status with this function before calling
 * something else below. If you call a scanning function while
 * dbe is locked there can be some weird behaviours.
 */
gboolean
symbol_db_engine_is_locked (SymbolDBEngine *dbe);

/**
 * Open or create a new database at given directory. 
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
 * Check if the database already exists into the prj_directory
 */
gboolean
symbol_db_engine_db_exists (SymbolDBEngine * dbe, const gchar * prj_directory);

/**
 * Check if a file is already present [and scanned] in db.
 */
gboolean
symbol_db_engine_file_exists (SymbolDBEngine * dbe, const gchar * abs_file_path);

/** Add a new workspace to an opened database. */
gboolean 
symbol_db_engine_add_new_workspace (SymbolDBEngine *dbe, const gchar* workspace);


/** 
 * Add a new project to workspace to an opened database.
 *
 * @param workspace Can be NULL. In that case a default workspace will be created, 
 * 					and project will depend on that.
 * @param project Project name. Must NOT be NULL.
 */
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
 * @param project_name something like 'foo_project', or 'helloworld_project'. Can be NULL,
 *        for example when you're populating after abort.
 * @param project_directory something like the base path '/home/user/projects/foo_project/'
 *        Be sure not to exchange the db_directory with project_directory! they're different!
 *        It can be NULL but ONLY if you're doing an 'import_after_abort' scan, i.e.
 *        files entries have already a connection with the parent project.
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
 * @return scan process id if insertion is successful, -1 on error.
 */
gint
symbol_db_engine_add_new_files (SymbolDBEngine *dbe, 
								const gchar * project_name,
							    const GPtrArray *files_path,
								const GPtrArray *languages,
								gboolean force_scan);

/**
 * Update symbols of the whole project. It scans all file symbols etc. 
 * If force is true then update forcely all the files.
 */
gint
symbol_db_engine_update_project_symbols (SymbolDBEngine *dbe, const gchar *project);


/** Remove a file, together with its symbols, from a project. */
gboolean 
symbol_db_engine_remove_file (SymbolDBEngine *dbe, const gchar* project, 
							  const gchar* file);

void
symbol_db_engine_remove_files (SymbolDBEngine * dbe, const gchar * project,
							  const GPtrArray * files);

/**
 * Update symbols of saved files. 
 * @note WARNING: files_path and it's contents will be freed on 
 * on_scan_update_files_symbols_end () callback.
 * @return scan process id if insertion is successful, -1 on 'no files scanned'.
 */
gint
symbol_db_engine_update_files_symbols (SymbolDBEngine *dbe, const gchar *project, 
									   GPtrArray *files_path,
									   gboolean update_prj_analyse_time);

/**
 * Update symbols of a file by a memory-buffer to perform a real-time updating 
 * of symbols. 
 * real_files_list: full path on disk to 'real file' to update. e.g.
 * /home/foouser/fooproject/src/main.c. 
 * They'll be freed inside this function when the scan has ended. 
 * @return scan process id if insertion is successful, -1 on error.
 */
gint
symbol_db_engine_update_buffer_symbols (SymbolDBEngine * dbe, const gchar * project,
										GPtrArray * real_files_list,
										const GPtrArray * text_buffers,
										const GPtrArray * buffer_sizes);




G_END_DECLS

#endif /* _SYMBOL_DB_ENGINE_H_ */

