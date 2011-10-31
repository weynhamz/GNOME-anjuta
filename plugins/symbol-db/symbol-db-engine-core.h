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
#include <libgda/gda-statement.h>
#include <libgda/gda-data-model.h>
#include <libanjuta/interfaces/ianjuta-symbol.h>
#include <libanjuta/interfaces/ianjuta-language.h>
#include <libanjuta/anjuta-plugin.h>

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
	void (* db_connected)           ();
	void (* db_disconnected)        ();
	void (* scan_begin)             (gint process_id);
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

typedef enum _SymbolDBEngineOpenStatus
{
	DB_OPEN_STATUS_FATAL = -1,
	DB_OPEN_STATUS_NORMAL =	0,
	DB_OPEN_STATUS_CREATE = 1,
	DB_OPEN_STATUS_UPGRADE =2
	
} SymbolDBEngineOpenStatus;


GType sdb_engine_get_type (void) G_GNUC_CONST;


SymbolDBEngine* 
symbol_db_engine_new (const gchar * ctags_path);

SymbolDBEngine* 
symbol_db_engine_new_full (const gchar * ctags_path, const gchar * database_name);

gboolean
symbol_db_engine_set_ctags_path (SymbolDBEngine *dbe, const gchar * ctags_path);


SymbolDBEngineOpenStatus
symbol_db_engine_open_db (SymbolDBEngine *dbe, const gchar* base_db_path,
						  const gchar * prj_directory);

gboolean 
symbol_db_engine_close_db (SymbolDBEngine *dbe);


gboolean
symbol_db_engine_is_connected (SymbolDBEngine * dbe);

gboolean
symbol_db_engine_is_scanning (SymbolDBEngine *dbe);


gchar *
symbol_db_engine_get_cnc_string (SymbolDBEngine * dbe);


gboolean
symbol_db_engine_db_exists (SymbolDBEngine * dbe, const gchar * prj_directory);

gboolean
symbol_db_engine_file_exists (SymbolDBEngine * dbe, const gchar * abs_file_path);

gboolean 
symbol_db_engine_add_new_workspace (SymbolDBEngine *dbe, const gchar* workspace);

gboolean 
symbol_db_engine_add_new_project (SymbolDBEngine *dbe, const gchar* workspace, 
								  const gchar* project, const gchar* version);

gboolean 
symbol_db_engine_project_exists (SymbolDBEngine *dbe, /*gchar* workspace, */
								const gchar* project_name,
    							const gchar* project_version);


gint
symbol_db_engine_add_new_files_full_async (SymbolDBEngine *dbe, 
										   const gchar * project_name,
    									   const gchar * project_version,
							    		   const GPtrArray *files_path,
										   const GPtrArray *languages,
										   gboolean force_scan);

gint
symbol_db_engine_add_new_files_async (SymbolDBEngine *dbe, 
    								  IAnjutaLanguage* lang_manager,
									  const gchar * project_name,
    								  const gchar * project_version,
							    	  const GPtrArray *files_path);

gint
symbol_db_engine_update_project_symbols (SymbolDBEngine *dbe, 
    const gchar *project_name, gboolean force_all_files);


gboolean 
symbol_db_engine_remove_file (SymbolDBEngine *dbe, const gchar *project,
                              const gchar* rel_file);

void
symbol_db_engine_remove_files (SymbolDBEngine * dbe, const gchar *project,
                               const GPtrArray *rel_files);


gint
symbol_db_engine_update_files_symbols (SymbolDBEngine *dbe, const gchar *project, 
									   const GPtrArray *files_path,
									   gboolean update_prj_analyse_time);

gint
symbol_db_engine_update_buffer_symbols (SymbolDBEngine * dbe, const gchar * project,
										const GPtrArray * real_files_list,
										const GPtrArray * text_buffers,
										const GPtrArray * buffer_sizes);

GdaDataModel*
symbol_db_engine_get_files_for_project (SymbolDBEngine *dbe);

void
symbol_db_engine_set_db_case_sensitive (SymbolDBEngine *dbe, gboolean case_sensitive);


GdaStatement*
symbol_db_engine_get_statement (SymbolDBEngine *dbe, const gchar *sql_str);

const GHashTable*
symbol_db_engine_get_type_conversion_hash (SymbolDBEngine *dbe);

const gchar*
symbol_db_engine_get_project_directory (SymbolDBEngine *dbe);


GdaDataModel*
symbol_db_engine_execute_select (SymbolDBEngine *dbe, GdaStatement *stmt,
                                 GdaSet *params);

G_END_DECLS

#endif /* _SYMBOL_DB_ENGINE_H_ */
