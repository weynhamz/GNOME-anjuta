/*
 * anjuta
 * Copyright (C) Massimo Cora' 2007-2009 <maxcvs@email.it>
 * 
 * anjuta is free software.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#include "symbol-db-engine.h"

#include "symbol-db-search-command.h"
#include "symbol-db-iface.h"
#include "plugin.h"

#include <libanjuta/anjuta-debug.h>

static guint async_command_id = 1;
 
IAnjutaIterable*
isymbol_manager_search (IAnjutaSymbolManager *sm,
						IAnjutaSymbolType match_types,
						gboolean include_types,
						IAnjutaSymbolField info_fields,
						const gchar *match_name,
						gboolean partial_name_match,
						gboolean global_symbols_search,
						gboolean global_tags_search,
						gint results_limit,
						gint results_offset,
						GError **err)
{
	SymbolDBEngineIterator *iterator = NULL;
	SymbolDBPlugin *sdb_plugin;
	SymbolDBEngine *dbe_project;
	SymbolDBEngine *dbe_globals;
	GPtrArray *filter_array;
	gchar *pattern;
	gboolean exact_match = !partial_name_match;

	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (sm);
	dbe_project = SYMBOL_DB_ENGINE (sdb_plugin->sdbe_project);
	dbe_globals = SYMBOL_DB_ENGINE (sdb_plugin->sdbe_globals);
	
	if (match_types & IANJUTA_SYMBOL_TYPE_UNDEF)
	{
		filter_array = NULL;
		/*DEBUG_PRINT ("%s", "filter_array is NULL");*/
	}
	else 
	{
		filter_array = symbol_db_util_fill_type_array (match_types);
		/*DEBUG_PRINT ("filter_array filled with %d kinds", filter_array->len);*/
	}

	if (exact_match == FALSE)
		pattern = g_strdup_printf ("%s%%", match_name == NULL ? "" : match_name);
	else
	{
		if (match_name == NULL)
			pattern = NULL;
		else
			pattern = g_strdup_printf ("%s", match_name);
	}
	
	/* should we lookup for project of system tags? */
	iterator = 
		symbol_db_engine_find_symbol_by_name_pattern_filtered (
					global_tags_search == FALSE ? dbe_project : dbe_globals, 
					pattern,
					exact_match,
					filter_array,
					include_types,
					global_symbols_search,
					global_tags_search == FALSE ? NULL : sdb_plugin->session_packages,
					results_limit,
					results_offset,
					info_fields);	
	g_free (pattern);
	
	if (filter_array)
	{
		g_ptr_array_foreach (filter_array, (GFunc)g_free, NULL);
		g_ptr_array_free (filter_array, TRUE);
	}
	return IANJUTA_ITERABLE (iterator);
}

IAnjutaIterable*
isymbol_manager_get_members (IAnjutaSymbolManager *sm,
							 const IAnjutaSymbol *symbol, 
							 IAnjutaSymbolField info_fields,
							 gboolean global_search,
							 GError **err)
{
	SymbolDBEngineIteratorNode *node;
	SymbolDBPlugin *sdb_plugin;
	SymbolDBEngine *dbe;
	gint sym_id;
	SymbolDBEngineIterator *iterator;

	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (sm);
	dbe = SYMBOL_DB_ENGINE (sdb_plugin->sdbe_project);	
	node = SYMBOL_DB_ENGINE_ITERATOR_NODE (symbol);
	
	sym_id = symbol_db_engine_iterator_node_get_symbol_id (node);
	
	iterator = symbol_db_engine_get_scope_members_by_symbol_id (dbe,
																sym_id,
																-1, 
																-1,
																info_fields);
	return IANJUTA_ITERABLE (iterator);
}

IAnjutaIterable*
isymbol_manager_get_class_parents (IAnjutaSymbolManager *sm,
							 const IAnjutaSymbol *symbol,
							 IAnjutaSymbolField info_fields,
							 GError **err)
{
	SymbolDBEngineIteratorNode *node;
	SymbolDBPlugin *sdb_plugin;
	SymbolDBEngine *dbe;
	gint sym_id;
	SymbolDBEngineIterator *iterator;

	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (sm);
	dbe = SYMBOL_DB_ENGINE (sdb_plugin->sdbe_project);
	
	node = SYMBOL_DB_ENGINE_ITERATOR_NODE (symbol);
	
	sym_id = symbol_db_engine_iterator_node_get_symbol_id (node);
	
	iterator = symbol_db_engine_get_class_parents_by_symbol_id (dbe, 
																sym_id, 
																info_fields);
	return IANJUTA_ITERABLE (iterator);
}

IAnjutaIterable*
isymbol_manager_get_scope (IAnjutaSymbolManager *sm,
						   const gchar* filename,  
						   gulong line,  
						   IAnjutaSymbolField info_fields, 
						   GError **err)
{
	SymbolDBPlugin *sdb_plugin;
	SymbolDBEngine *dbe;
	SymbolDBEngineIterator *iterator;

	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (sm);
	dbe = SYMBOL_DB_ENGINE (sdb_plugin->sdbe_project);
	
	iterator = symbol_db_engine_get_current_scope (dbe, filename, line, info_fields);
	
	return IANJUTA_ITERABLE (iterator);
}

IAnjutaIterable*
isymbol_manager_get_parent_scope (IAnjutaSymbolManager *sm,
								  const IAnjutaSymbol *symbol, 
								  const gchar *filename, 
								  IAnjutaSymbolField info_fields,
								  GError **err)
{
	SymbolDBEngineIteratorNode *node;
	gint child_node_id, parent_node_id;
	SymbolDBPlugin *sdb_plugin;
	SymbolDBEngine *dbe;
	SymbolDBEngineIterator *iterator;

	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (sm);
	dbe = SYMBOL_DB_ENGINE (sdb_plugin->sdbe_project);

	node = SYMBOL_DB_ENGINE_ITERATOR_NODE (symbol);
	
	child_node_id = symbol_db_engine_iterator_node_get_symbol_id (node);
	
	if (child_node_id <= 0)
		return NULL;
	
	parent_node_id = symbol_db_engine_get_parent_scope_id_by_symbol_id (dbe,
									child_node_id,
									filename);

	iterator = symbol_db_engine_get_symbol_info_by_id (dbe, parent_node_id, 
													   info_fields);
	return IANJUTA_ITERABLE (iterator);
}

IAnjutaIterable*
isymbol_manager_get_symbol_more_info (IAnjutaSymbolManager *sm,
								  const IAnjutaSymbol *symbol, 
								  IAnjutaSymbolField info_fields,
								  GError **err)
{
	SymbolDBEngineIteratorNode *node;
	gint node_id;
	SymbolDBPlugin *sdb_plugin;
	SymbolDBEngine *dbe;
	SymbolDBEngineIterator *iterator;

	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (sm);
	dbe = SYMBOL_DB_ENGINE (sdb_plugin->sdbe_project);

	node = SYMBOL_DB_ENGINE_ITERATOR_NODE (symbol);
	
	node_id = symbol_db_engine_iterator_node_get_symbol_id (node);
	
	if (node_id <= 0)
		return NULL;
	
	iterator = symbol_db_engine_get_symbol_info_by_id (dbe, node_id, info_fields);	
	
	return IANJUTA_ITERABLE (iterator);
}

IAnjutaSymbol*
isymbol_manager_get_symbol_by_id (IAnjutaSymbolManager *sm,
								  gint symbol_id, 
								  IAnjutaSymbolField info_fields,
								  GError **err)
{
	SymbolDBEngineIteratorNode *node;
	SymbolDBPlugin *sdb_plugin;
	SymbolDBEngine *dbe;
	SymbolDBEngineIterator *iterator;

	g_return_val_if_fail (symbol_id > 0, NULL);
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (sm);
	dbe = SYMBOL_DB_ENGINE (sdb_plugin->sdbe_project);

	iterator = symbol_db_engine_get_symbol_info_by_id (dbe, symbol_id, 
													   info_fields);	
	
	if (iterator == NULL)
		return NULL;
	
	node = SYMBOL_DB_ENGINE_ITERATOR_NODE (iterator);
	return IANJUTA_SYMBOL (node);
}

static SymbolDBEngineIterator *
do_search_prj_glb (SymbolDBEngine *dbe, IAnjutaSymbolType match_types,
           gboolean include_types, IAnjutaSymbolField info_fields,
           const gchar *pattern, gint results_limit, gint results_offset, 
           GList *session_packages)
{
	SymbolDBEngineIterator *iterator;
	gboolean exact_match;
	GPtrArray *filter_array;
	
	exact_match = symbol_db_util_is_pattern_exact_match (pattern);

	if (match_types & IANJUTA_SYMBOL_TYPE_UNDEF)
	{
		filter_array = NULL;
	}
	else 
	{
		filter_array = symbol_db_util_fill_type_array (match_types);
	}
	
	iterator = 		
		symbol_db_engine_find_symbol_by_name_pattern_filtered (dbe,
					pattern,
					exact_match,
					filter_array,
					include_types,
					1,
					session_packages,
					results_limit,
					results_offset,
					info_fields);	
	
	return iterator;
}

IAnjutaIterable* 
isymbol_manager_search_system (IAnjutaSymbolManager *sm, IAnjutaSymbolType match_types, 
				gboolean include_types,  IAnjutaSymbolField info_fields, 
				const gchar *pattern, gint results_limit, gint results_offset, 
				GError **err)
{
	SymbolDBPlugin *sdb_plugin;
	SymbolDBEngine *dbe;
	SymbolDBEngineIterator *iterator;
	g_return_val_if_fail (pattern != NULL, NULL);
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (sm);
	/* take the global's engine */
	dbe = SYMBOL_DB_ENGINE (sdb_plugin->sdbe_globals);

	iterator = do_search_prj_glb (dbe, match_types, info_fields, 
	                      include_types, pattern, 
						  results_limit, results_offset,
	           			  sdb_plugin->session_packages);

	return IANJUTA_ITERABLE (iterator);
}

static guint
get_unique_async_command_id ()
{
	return async_command_id++;
}

static void
on_sdb_search_command_data_arrived (AnjutaCommand *command, 
									 IAnjutaSymbolManagerSearchCallback callback)
{
	SymbolDBSearchCommand *sdbsc;
	SymbolDBEngineIterator *iterator;
	gpointer callback_user_data = NULL;
	guint cmd_id;
	
	sdbsc = SYMBOL_DB_SEARCH_COMMAND (command);
	iterator = symbol_db_search_command_get_iterator_result (sdbsc);
		
	/* get the previously saved cmd_id and callback data */
	cmd_id = GPOINTER_TO_UINT  (g_object_get_data (G_OBJECT (command), "cmd_id"));
	callback_user_data = g_object_get_data (G_OBJECT (command), "callback_user_data");
	
	callback (cmd_id, IANJUTA_ITERABLE (iterator), callback_user_data);
}

static gint
do_search_prj_glb_async (SymbolDBSearchCommand *search_command, guint cmd_id, 
                         GCancellable* cancel, AnjutaAsyncNotify *notify, 
                		 IAnjutaSymbolManagerSearchCallback callback, 
						 gpointer callback_user_data)
{
	/* be sure that the cmd_id and the callback_user_data stay with us when the 
	 * data-arrived signal is raised 
	 */
	g_object_set_data (G_OBJECT (search_command), "cmd_id",
	                   GUINT_TO_POINTER (cmd_id));

	/* FIXME: why is data checked for NULL on iface layer? It should be able to 
	 * arrive here == NULL....
	 */
	if (callback_user_data != NULL)
		g_object_set_data (G_OBJECT (search_command), "callback_user_data", 
	                   callback_user_data);

	/* connect some signals */
	g_signal_connect (G_OBJECT (search_command), "data-arrived",
					  G_CALLBACK (on_sdb_search_command_data_arrived),
					  callback);
	
	g_signal_connect (G_OBJECT (search_command), "command-finished",
					  G_CALLBACK (g_object_unref),
					  NULL);
	
	if (cancel)
	{
		g_signal_connect_swapped (G_OBJECT (cancel), "cancelled",
								  G_CALLBACK (anjuta_command_cancel),
								  search_command);
	}
	
	if (notify)
	{
		g_signal_connect_swapped (G_OBJECT (search_command), "command-finished",
								  G_CALLBACK (anjuta_async_notify_notify_finished),
								  notify);
	}
	
	anjuta_command_start (ANJUTA_COMMAND (search_command));	

	return cmd_id;	
}

guint
isymbol_manager_search_system_async (IAnjutaSymbolManager *sm, IAnjutaSymbolType match_types, 
				gboolean include_types,  IAnjutaSymbolField info_fields, const gchar *pattern, 
			    gint results_limit, gint results_offset, 
                GCancellable* cancel, AnjutaAsyncNotify *notify, 
                IAnjutaSymbolManagerSearchCallback callback, 
				gpointer callback_user_data, GError **err)
{
	SymbolDBSearchCommand *search_command;
	SymbolDBPlugin *sdb_plugin;
	SymbolDBEngine *dbe;
	guint cmd_id;

	/* if the return is 0 then we'll have an error, i.e. no valid command id has 
	 * been generated
	 */
	g_return_val_if_fail (pattern != NULL, 0);

	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (sm);
	dbe = SYMBOL_DB_ENGINE (sdb_plugin->sdbe_globals);

	/* get an unique cmd_id */
	cmd_id = get_unique_async_command_id ();
	
	/* create a new command */
	search_command = symbol_db_search_command_new (dbe, CMD_SEARCH_SYSTEM, match_types, 
				include_types, info_fields, pattern, results_limit, results_offset);

	/* don't forget to set the session packages too */
	symbol_db_search_command_set_session_packages (search_command, 
	                                               sdb_plugin->session_packages);	

	return do_search_prj_glb_async (search_command, cmd_id, cancel, notify, 
	                                callback, callback_user_data);
}

IAnjutaIterable* 
isymbol_manager_search_project (IAnjutaSymbolManager *sm, IAnjutaSymbolType match_types, 
				gboolean include_types,  IAnjutaSymbolField info_fields, const gchar *pattern, 
				gint results_limit, gint results_offset, GError **err)
{
	SymbolDBPlugin *sdb_plugin;
	SymbolDBEngine *dbe;
	SymbolDBEngineIterator *iterator;
	g_return_val_if_fail (pattern != NULL, NULL);
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (sm);
	/* take the project's engine */
	dbe = SYMBOL_DB_ENGINE (sdb_plugin->sdbe_project);

	iterator = do_search_prj_glb (dbe, match_types, info_fields, 
	                      include_types, pattern, 
						  results_limit, results_offset,
	           			  NULL);

	return IANJUTA_ITERABLE (iterator);
}

guint
isymbol_manager_search_project_async (IAnjutaSymbolManager *sm, IAnjutaSymbolType match_types, 
				gboolean include_types,  IAnjutaSymbolField info_fields, const gchar *pattern, 
				gint results_limit, gint results_offset, 
                GCancellable* cancel, AnjutaAsyncNotify *notify, 
                IAnjutaSymbolManagerSearchCallback callback, 
				gpointer callback_user_data, GError **err)
{
	SymbolDBSearchCommand *search_command;
	SymbolDBPlugin *sdb_plugin;
	SymbolDBEngine *dbe;
	guint cmd_id;

	/* if the return is 0 then we'll have an error, i.e. no valid command id has 
	 * been generated
	 */
	g_return_val_if_fail (pattern != NULL, 0);

	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (sm);
	dbe = SYMBOL_DB_ENGINE (sdb_plugin->sdbe_project);

	/* get an unique cmd_id */
	cmd_id = get_unique_async_command_id ();
	
	/* create a new command */
	search_command = symbol_db_search_command_new (dbe, CMD_SEARCH_PROJECT, match_types, 
				include_types, info_fields, pattern, results_limit, results_offset);

	/* don't forget to set the session packages to NULL */
	symbol_db_search_command_set_session_packages (search_command, NULL);	

	return do_search_prj_glb_async (search_command, cmd_id, cancel, notify, 
	                                callback, callback_user_data);
}

IAnjutaIterable* 
isymbol_manager_search_file (IAnjutaSymbolManager *sm, IAnjutaSymbolType match_types, 
				gboolean include_types,  IAnjutaSymbolField info_fields, const gchar *pattern, 
			 	const GFile *file, gint results_limit, gint results_offset, GError **err)
{
	SymbolDBPlugin *sdb_plugin;
	SymbolDBEngine *dbe;
	SymbolDBEngineIterator *iterator;
	GPtrArray *filter_array;
	gchar *abs_file_path;

	g_return_val_if_fail (file != NULL, NULL);
	g_return_val_if_fail (pattern != NULL, NULL);
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (sm);
	dbe = SYMBOL_DB_ENGINE (sdb_plugin->sdbe_project);
	
	abs_file_path = g_file_get_path ((GFile *)file);

	if (abs_file_path == NULL)
	{
		g_warning ("isymbol_manager_search_file (): GFile has no absolute path");
		return NULL;
	}
	
	if (match_types & IANJUTA_SYMBOL_TYPE_UNDEF)
	{
		filter_array = NULL;
	}
	else 
	{
		filter_array = symbol_db_util_fill_type_array (match_types);
	}
	
	iterator = 		
		symbol_db_engine_find_symbol_by_name_pattern_on_file (dbe,
				    pattern,
					abs_file_path,
					filter_array,
					include_types,
					results_limit,
					results_offset,
					info_fields);
	
	g_free (abs_file_path);
	
	return IANJUTA_ITERABLE (iterator);
}


guint
isymbol_manager_search_file_async (IAnjutaSymbolManager *sm, IAnjutaSymbolType match_types, 
				gboolean include_types,  IAnjutaSymbolField info_fields, const gchar *pattern, 
			 	const GFile *file, gint results_limit, gint results_offset, 
                GCancellable* cancel, AnjutaAsyncNotify *notify, 
                IAnjutaSymbolManagerSearchCallback callback, 
                gpointer callback_user_data, GError **err)
{
	SymbolDBSearchCommand *search_command;
	SymbolDBPlugin *sdb_plugin;
	SymbolDBEngine *dbe;
	guint cmd_id;

	/* if the return is 0 then we'll have an error, i.e. no valid command id has 
	 * been generated
	 */
	g_return_val_if_fail (pattern != NULL, 0);
	g_return_val_if_fail (file != NULL, 0);

	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (sm);
	dbe = SYMBOL_DB_ENGINE (sdb_plugin->sdbe_project);

	/* get an unique cmd_id */
	cmd_id = get_unique_async_command_id ();
	
	/* create a new command */
	search_command = symbol_db_search_command_new (dbe, CMD_SEARCH_FILE, match_types, 
				include_types, info_fields, pattern, results_limit, results_offset);

	/* don't forget to set the file too */
	symbol_db_search_command_set_file (search_command, file);	

	/* be sure that the cmd_id and the callback_user_data stay with us when the 
	 * data-arrived signal is raised 
	 */
	g_object_set_data (G_OBJECT (search_command), "cmd_id",
	                   GINT_TO_POINTER (cmd_id));

	/* FIXME: why is data checked for NULL on iface layer? It should be able to 
	 * arrive here == NULL....
	 */
	if (callback_user_data != NULL)
		g_object_set_data (G_OBJECT (search_command), "callback_user_data", 
	                   callback_user_data);

	/* connect some signals */
	g_signal_connect (G_OBJECT (search_command), "data-arrived",
					  G_CALLBACK (on_sdb_search_command_data_arrived),
					  callback);
	
	g_signal_connect (G_OBJECT (search_command), "command-finished",
					  G_CALLBACK (g_object_unref),
					  NULL);
	
	if (cancel)
	{
		g_signal_connect_swapped (G_OBJECT (cancel), "cancelled",
								  G_CALLBACK (anjuta_command_cancel),
								  search_command);
	}
	
	if (notify)
	{
		g_signal_connect_swapped (G_OBJECT (search_command), "command-finished",
								  G_CALLBACK (anjuta_async_notify_notify_finished),
								  notify);
	}
	
	anjuta_command_start (ANJUTA_COMMAND (search_command));	

	return cmd_id;
}

void
isymbol_manager_iface_init (IAnjutaSymbolManagerIface *iface)
{
	iface->search = isymbol_manager_search;
	iface->get_members = isymbol_manager_get_members;
	iface->get_class_parents = isymbol_manager_get_class_parents;
	iface->get_scope = isymbol_manager_get_scope;
	iface->get_parent_scope = isymbol_manager_get_parent_scope;
	iface->get_symbol_more_info = isymbol_manager_get_symbol_more_info;
	iface->get_symbol_by_id = isymbol_manager_get_symbol_by_id;
	iface->search_system = isymbol_manager_search_system;
	iface->search_system_async = isymbol_manager_search_system_async;
	iface->search_project = isymbol_manager_search_project;
	iface->search_project_async = isymbol_manager_search_project_async;
	iface->search_file = isymbol_manager_search_file;
	iface->search_file_async = isymbol_manager_search_file_async;	
}
