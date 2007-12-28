/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.c
 * Copyright (C) Massimo Cora' 2007 <maxcvs@email.it>
 * 
 * plugin.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * plugin.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.c.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include <config.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/interfaces/ianjuta-file-manager.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-markable.h>
#include <libanjuta/interfaces/ianjuta-language.h>
#include <libanjuta/interfaces/ianjuta-iterable.h>

#include <libegg/menu/egg-combo-action.h>

#include "plugin.h"
#include "symbol-db-view.h"
#include "symbol-db-view-locals.h"
#include "symbol-db-view-search.h"
#include "symbol-db-engine.h"
#include "symbol-db-engine-iterator.h"

#define UI_FILE ANJUTA_DATA_DIR"/ui/symbol-db.ui"

#define GLADE_FILE ANJUTA_DATA_DIR"/glade/symbol-db.glade"
#define ICON_FILE "anjuta-symbol-db-plugin-48.png"

#define TIMEOUT_INTERVAL_SYMBOLS_UPDATE		10000
#define TIMEOUT_SECONDS_AFTER_LAST_TIP		5

static gpointer parent_class;
static gboolean need_symbols_update = FALSE;
static gchar prev_char_added = ' ';
static gint timeout_id = 0;
static GTimer *timer = NULL;

static void
register_stock_icons (AnjutaPlugin *plugin)
{
	static gboolean registered = FALSE;
	
	if (registered)
		return;
	registered = TRUE;

	/* Register stock icons */
	BEGIN_REGISTER_ICON (plugin);
	REGISTER_ICON (ICON_FILE, "symbol-db-plugin-icon");
	END_REGISTER_ICON;
}

static gboolean
on_editor_buffer_symbols_update_timeout (gpointer user_data)
{
	SymbolDBPlugin *sdb_plugin;
	IAnjutaEditor *ed;
	gchar *current_buffer = NULL;
	gint buffer_size = 0;
	gchar *uri = NULL;
	gdouble seconds_elapsed;
	
	g_return_val_if_fail (user_data != NULL, FALSE);
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (user_data);
		
	if (sdb_plugin->current_editor == NULL)
		return FALSE;
	
	/* check the timer. If it's elapsed enought time since the last time the user
	 * typed in something, than proceed with updating, elsewhere don't do nothing 
	 */
	if (timer == NULL)
		return TRUE;
	
	seconds_elapsed = g_timer_elapsed (timer, NULL);
	
	/* DEBUG_PRINT ("seconds_elapsed  %f", seconds_elapsed ); */
	if (seconds_elapsed < TIMEOUT_SECONDS_AFTER_LAST_TIP)
		return TRUE;
		
		
	 /* we won't proceed with the updating of the symbols if we didn't type in 
	 	anything */
	 if (!need_symbols_update)
	 	return TRUE;
	
	DEBUG_PRINT ("on_editor_buffer_symbols_update_timeout()");
	
	if (sdb_plugin->current_editor) 
	{
		ed = IANJUTA_EDITOR (sdb_plugin->current_editor);
		
		buffer_size = ianjuta_editor_get_length (ed, NULL);
		current_buffer = ianjuta_editor_get_text (ed, 0, -1, NULL);
				
		uri = ianjuta_file_get_uri (IANJUTA_FILE (ed), NULL);		
	} 
	else
		return FALSE;
	
	if (uri) 
	{
		DEBUG_PRINT ("uri for buffer updating: %s", uri);
		
		GPtrArray *real_files_list;
		GPtrArray *text_buffers;
		GPtrArray *buffer_sizes;
		gchar *project_name;
								
		gchar * local_path = gnome_vfs_get_local_path_from_uri (uri);

		real_files_list = g_ptr_array_new ();
		g_ptr_array_add (real_files_list, local_path);

		text_buffers = g_ptr_array_new ();
		g_ptr_array_add (text_buffers, current_buffer);	

		buffer_sizes = g_ptr_array_new ();
		g_ptr_array_add (buffer_sizes, (gpointer)buffer_size);	

		project_name = symbol_db_engine_get_opened_project_name (sdb_plugin->sdbe);

		symbol_db_engine_update_buffer_symbols (sdb_plugin->sdbe,
												project_name,
												real_files_list,
												text_buffers,
												buffer_sizes);
		g_free (project_name);
												
		g_free (uri);
	}
	
	g_free (current_buffer);  

	need_symbols_update = FALSE;

	return TRUE;
}

static void
on_editor_destroy (SymbolDBPlugin *sdb_plugin, IAnjutaEditor *editor)
{
	const gchar *uri;
	DEBUG_PRINT ("on_editor_destroy ()");
	if (!sdb_plugin->editor_connected || !sdb_plugin->dbv_view_tree)
		return;
	
	uri = g_hash_table_lookup (sdb_plugin->editor_connected, G_OBJECT (editor));
	if (uri && strlen (uri) > 0)
	{
/*				
		DEBUG_PRINT ("Removing file tags of %s", uri);

		anjuta_symbol_view_workspace_remove_file (ANJUTA_SYMBOL_VIEW (sv_plugin->sv_tree),
											   uri);
*/		
	}
	g_hash_table_remove (sdb_plugin->editor_connected, G_OBJECT (editor));
}

static void
on_editor_update_ui (IAnjutaEditor *editor, SymbolDBPlugin *sdb_plugin) 
{
	if (timer == NULL)
	{
		/* creates and start a new timer. */
		timer = g_timer_new ();
	}
	else 
	{
		g_timer_reset (timer);
	}
	
#if 0	
	gint lineno = ianjuta_editor_get_lineno (editor, NULL);
	
	GtkTreeModel* model = anjuta_symbol_view_get_file_symbol_model
							(ANJUTA_SYMBOL_VIEW(sv_plugin->sv_tree));
	GtkTreeIter iter;
	gboolean found = FALSE;
	
	if (sv_plugin->locals_line_number == lineno)
		return;
	sv_plugin->locals_line_number = lineno;
	
	if (!gtk_tree_model_get_iter_first (model, &iter))
		return;
	while (!found && lineno >= 0)
	{
		gtk_tree_model_get_iter_first (model, &iter);
		do
		{
			found = iter_matches (sv_plugin, &iter, model, lineno);
			if (found)
				break;
		}
		while (gtk_tree_model_iter_next (model, &iter));
		lineno--;
	}
#endif	
}

static void
on_char_added (IAnjutaEditor *editor, gint position, gchar ch,
			   SymbolDBPlugin *sdb_plugin)
{
	DEBUG_PRINT ("char added @ %d : %c [int %d]", position, ch, ch);
	
	if (timer == NULL)
	{
		/* creates and start a new timer. */
		timer = g_timer_new ();
	}
	else 
	{
		g_timer_reset (timer);
	}
	
	
	/* try to force the update if a "." or a "->" is pressed */
	if ((ch == '.') || (prev_char_added == '-' && ch == '>'))
		on_editor_buffer_symbols_update_timeout (sdb_plugin);
		
	need_symbols_update = TRUE;
	
	prev_char_added = ch;
}


static void
on_editor_saved (IAnjutaEditor *editor, const gchar *saved_uri,
				 SymbolDBPlugin *sdb_plugin)
{
	const gchar *old_uri;
	gboolean tags_update;
	
	/* FIXME: Do this only if automatic tags update is enabled */
	/* tags_update =
		anjuta_preferences_get_int (te->preferences, AUTOMATIC_TAGS_UPDATE);
	*/
	tags_update = TRUE;
	if (tags_update)
	{
		gchar *local_filename;
		GPtrArray *files_array;

		/* Verify that it's local file */
		local_filename = gnome_vfs_get_local_path_from_uri (saved_uri);
		g_return_if_fail (local_filename != NULL);

		files_array = g_ptr_array_new();		
		DEBUG_PRINT ("local_filename saved is %s", local_filename);
		g_ptr_array_add (files_array, local_filename);		
		/* no need to free local_filename now */
		
		if (!sdb_plugin->editor_connected)
			return;
	
		old_uri = g_hash_table_lookup (sdb_plugin->editor_connected, editor);
		
		if (old_uri && strlen (old_uri) <= 0)
			old_uri = NULL;

		/* files_array will be freed once updating has taken place */
		symbol_db_engine_update_files_symbols (sdb_plugin->sdbe, 
				sdb_plugin->project_root_dir, files_array, TRUE);
		g_hash_table_insert (sdb_plugin->editor_connected, editor,
							 g_strdup (saved_uri));

		on_editor_update_ui (editor, sdb_plugin);
	}
}

static void
value_added_current_editor (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	gchar *uri;
	gchar *local_path;
	GObject *editor;
	SymbolDBPlugin *sdb_plugin;
	
	editor = g_value_get_object (value);	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (plugin);
	
	DEBUG_PRINT ("value_removed_current_editor ()");
		
	if (!sdb_plugin->editor_connected)
	{
		sdb_plugin->editor_connected = g_hash_table_new_full (g_direct_hash,
															 g_direct_equal,
															 NULL, g_free);
	}
	sdb_plugin->current_editor = editor;

	uri = ianjuta_file_get_uri (IANJUTA_FILE (editor), NULL);
	
	if (uri == NULL)
		return;

	local_path = gnome_vfs_get_local_path_from_uri (uri);
	DEBUG_PRINT ("value_added_current_editor () gonna refresh local syms: local_path %s "
				 "uri %s", local_path, uri);
	
	if (strstr (local_path, "//") != NULL)
	{
		g_critical ("WARNING FIXME: bad file uri passed to symbol-db from editor. There's "
				   "a trailing slash left. Please fix this at editor side");
	}
	 
	symbol_db_view_locals_update_list (
				SYMBOL_DB_VIEW_LOCALS (sdb_plugin->dbv_view_tree_locals),
				 sdb_plugin->sdbe, local_path);
				 
	if (g_hash_table_lookup (sdb_plugin->editor_connected, editor) == NULL)
	{
		g_object_weak_ref (G_OBJECT (editor),
						   (GWeakNotify) (on_editor_destroy),
						   sdb_plugin);
		if (uri)
		{
			g_hash_table_insert (sdb_plugin->editor_connected, editor,
								 g_strdup (uri));
		}
		else
		{
			g_hash_table_insert (sdb_plugin->editor_connected, editor,
								 g_strdup (""));
		}

		g_signal_connect (G_OBJECT (editor), "saved",
						  G_CALLBACK (on_editor_saved),
						  sdb_plugin);						  
		g_signal_connect (G_OBJECT (editor), "char-added",
						  G_CALLBACK (on_char_added),
						  sdb_plugin);
		g_signal_connect (G_OBJECT(editor), "update_ui",
						  G_CALLBACK (on_editor_update_ui),
						  sdb_plugin);
	}
	g_free (uri);
	g_free (local_path);
	
	/* add a default timeout to the updating of buffer symbols */	
	timeout_id = g_timeout_add (TIMEOUT_INTERVAL_SYMBOLS_UPDATE,
								on_editor_buffer_symbols_update_timeout,
								plugin);
	need_symbols_update = FALSE;
}

static void
on_editor_foreach_disconnect (gpointer key, gpointer value, gpointer user_data)
{
	g_signal_handlers_disconnect_by_func (G_OBJECT(key),
										  G_CALLBACK (on_editor_saved),
										  user_data);
	g_signal_handlers_disconnect_by_func (G_OBJECT(key),
										  G_CALLBACK (on_editor_update_ui),
										  user_data);
	g_signal_handlers_disconnect_by_func (G_OBJECT(key),
										  G_CALLBACK (on_char_added),
										  user_data);
	g_object_weak_unref (G_OBJECT(key),
						 (GWeakNotify) (on_editor_destroy),
						 user_data);
}

static void
goto_file_line (AnjutaPlugin *plugin, const gchar *filename, gint lineno)
{
	gchar *uri;
	IAnjutaFileLoader *loader;
	
	g_return_if_fail (filename != NULL);
		
	/* Go to file and line number */
	loader = anjuta_shell_get_interface (plugin->shell, IAnjutaFileLoader,
										 NULL);
		
	uri = g_strdup_printf ("file://%s#%d", filename, lineno);
	ianjuta_file_loader_load (loader, uri, FALSE, NULL);
	g_free (uri);
}


static void
goto_local_tree_iter (SymbolDBPlugin *sdb_plugin, GtkTreeIter *iter)
{
	gint line;

	line = symbol_db_view_locals_get_line (SYMBOL_DB_VIEW_LOCALS (
									sdb_plugin->dbv_view_tree_locals), 
										   sdb_plugin->sdbe,
										   iter);

	DEBUG_PRINT ("got line %d", line);
	
	if (line > 0 && sdb_plugin->current_editor)
	{
		/* Goto line number */
		ianjuta_editor_goto_line (IANJUTA_EDITOR (sdb_plugin->current_editor),
								  line, NULL);
		if (IANJUTA_IS_MARKABLE (sdb_plugin->current_editor))
		{
			ianjuta_markable_delete_all_markers (IANJUTA_MARKABLE (sdb_plugin->current_editor),
												 IANJUTA_MARKABLE_LINEMARKER,
												 NULL);

			ianjuta_markable_mark (IANJUTA_MARKABLE (sdb_plugin->current_editor),
								   line, IANJUTA_MARKABLE_LINEMARKER, NULL);
		}
	}
}

static void
goto_global_tree_iter (SymbolDBPlugin *sdb_plugin, GtkTreeIter *iter)
{
	gint line;
	gchar *file;

	if (symbol_db_view_get_file_and_line (
			SYMBOL_DB_VIEW (sdb_plugin->dbv_view_tree), sdb_plugin->sdbe,
							iter, &line, &file) == FALSE)
	{
		g_warning ("goto_global_tree_iter (): error while trying to get file/line");
		return;
	};
	
	
	DEBUG_PRINT ("got line %d and file %s", line, file);
	
	if (line > 0 && sdb_plugin->current_editor)
	{
		goto_file_line (ANJUTA_PLUGIN (sdb_plugin), file, line);
		if (IANJUTA_IS_MARKABLE (sdb_plugin->current_editor))
		{
			ianjuta_markable_delete_all_markers (IANJUTA_MARKABLE (sdb_plugin->current_editor),
												 IANJUTA_MARKABLE_LINEMARKER,
												 NULL);

			ianjuta_markable_mark (IANJUTA_MARKABLE (sdb_plugin->current_editor),
								   line, IANJUTA_MARKABLE_LINEMARKER, NULL);
		}
	}
	
	g_free (file);
}

/**
 * will manage the click of mouse and other events on search->hitlist treeview
 */
static void
on_treesearch_symbol_selected_event (SymbolDBViewSearch *search,
									 gint line,
									 gchar* file,
									 SymbolDBPlugin *sdb_plugin) 
{
										 
	DEBUG_PRINT ("on_treesearch_symbol_selected_event  (), %s %d", file, line);
	goto_file_line (ANJUTA_PLUGIN (sdb_plugin), file, line);
}


static void
on_local_treeview_row_activated (GtkTreeView *view, GtkTreePath *arg1,
								 GtkTreeViewColumn *arg2,
								 SymbolDBPlugin *sdb_plugin)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;

	DEBUG_PRINT ("on_local_treeview_row_activated ()");
	selection = gtk_tree_view_get_selection (view);
	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) 
	{
		return;
	}
	goto_local_tree_iter (sdb_plugin, &iter);
}

static void
on_global_treeview_row_activated (GtkTreeView *view, GtkTreePath *arg1,
								 GtkTreeViewColumn *arg2,
								 SymbolDBPlugin *sdb_plugin)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	
	DEBUG_PRINT ("on_global_treeview_row_activated ()");
	selection = gtk_tree_view_get_selection (view);
	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) 
	{
		return;
	}
	
	goto_global_tree_iter (sdb_plugin, &iter);
}

static void
on_global_treeview_row_expanded (GtkTreeView *tree_view,
									GtkTreeIter *iter,
                                    GtkTreePath *path,
                                    SymbolDBPlugin *user_data)
{
	DEBUG_PRINT ("on_global_treeview_row_expanded ()");

	symbol_db_view_row_expanded (SYMBOL_DB_VIEW (user_data->dbv_view_tree),
								user_data->sdbe, iter);
}

static void
on_editor_foreach_clear (gpointer key, gpointer value, gpointer user_data)
{
	const gchar *uri;
	
	uri = (const gchar *)value;
	if (uri && strlen (uri) > 0)
	{
#if 0
		/* FIXME ?! */
		DEBUG_PRINT ("Removing file tags of %s", uri);
		anjuta_symbol_view_workspace_remove_file (ANJUTA_SYMBOL_VIEW (sv_plugin->sv_tree),
											   uri);
#endif
	}
}

static void
value_removed_current_editor (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	SymbolDBPlugin *sdb_plugin;

	DEBUG_PRINT ("value_removed_current_editor ()");
	/* let's remove the timeout for symbols refresh */
	g_source_remove (timeout_id);
	need_symbols_update = FALSE;
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (plugin);
/* FIXME 	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupSymbolNavigation",
								   "ActionGotoSymbol");
	g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
*/	
	sdb_plugin->current_editor = NULL;
}


static void
on_project_element_added (IAnjutaProjectManager *pm, const gchar *uri,
						  SymbolDBPlugin *sdb_plugin)
{
	gchar *filename;
	IAnjutaLanguage* lang_manager;
	
	g_return_if_fail (sdb_plugin->project_root_uri != NULL);
	g_return_if_fail (sdb_plugin->project_root_dir != NULL);

	lang_manager = anjuta_shell_get_interface (ANJUTA_PLUGIN(sdb_plugin)->shell, 
													IAnjutaLanguage, NULL);
	
	g_return_if_fail (lang_manager != NULL);
	
	DEBUG_PRINT ("on_project_element_added");
	
	filename = gnome_vfs_get_local_path_from_uri (uri);
	if (filename)
	{
		GPtrArray *files_array;
		GPtrArray *languages_array;
		const gchar* file_mime;
		IAnjutaLanguageId lang_id;
		const gchar* lang;
		file_mime = gnome_vfs_get_mime_type_for_name (filename);
		
		lang_id = ianjuta_language_get_from_mime_type (lang_manager, file_mime, 
													   NULL);
		
		/* No supported language... */
		if (!lang_id)
		{
			g_free (filename);
			return;
		}
		
		lang = ianjuta_language_get_name (lang_manager, lang_id, NULL);
		files_array = g_ptr_array_new();
		languages_array = g_ptr_array_new();
		
		g_ptr_array_add (files_array, filename);
		g_ptr_array_add (languages_array, g_strdup (lang));
		
		DEBUG_PRINT ("gonna opening %s", 
					 filename + strlen(sdb_plugin->project_root_dir) );
		DEBUG_PRINT ("project_root_dir %s", sdb_plugin->project_root_dir );
		
		symbol_db_engine_add_new_files (sdb_plugin->sdbe, 
			sdb_plugin->project_root_dir, files_array, languages_array, TRUE);
		
		g_free (filename);
		g_ptr_array_free (files_array, TRUE);
		
		g_ptr_array_foreach (languages_array, (GFunc)g_free, NULL);
		g_ptr_array_free (languages_array, TRUE);

	}
}

static void
on_project_element_removed (IAnjutaProjectManager *pm, const gchar *uri,
							SymbolDBPlugin *sv_plugin)
{
	gchar *filename;
	
	if (!sv_plugin->project_root_uri)
		return;
	
	filename = gnome_vfs_get_local_path_from_uri (uri);
	if (filename)
	{
		DEBUG_PRINT ("on_project_element_removed");
		DEBUG_PRINT ("gonna removing %s", 
					 filename + strlen(sv_plugin->project_root_dir));
		DEBUG_PRINT ("project_root_dir %s", sv_plugin->project_root_dir );
		symbol_db_engine_remove_file (sv_plugin->sdbe, 
			sv_plugin->project_root_dir, filename);
		
		g_free (filename);
	}
}

static void
sources_array_free (gpointer data)
{
	GPtrArray* sources = (GPtrArray*) data;
	g_ptr_array_foreach (sources, (GFunc)g_free, NULL);
	g_ptr_array_free (sources, TRUE);
}

static void
on_single_file_scan_end (SymbolDBEngine *dbe, gpointer data)
{	
	AnjutaPlugin *plugin;
	AnjutaStatus *status;	
	SymbolDBPlugin *sdb_plugin;
	gchar *message;
	
	plugin = ANJUTA_PLUGIN (data);
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (plugin);
		
	status = anjuta_shell_get_status (plugin->shell, NULL);
	
	if (status == NULL)
		return;
	
	sdb_plugin->files_count_done++;	
	message = g_strdup_printf ("%d files scanned out of %d", 
							   sdb_plugin->files_count_done, sdb_plugin->files_count);
	
	DEBUG_PRINT ("on_single_file_scan_end (): %d out of %d", sdb_plugin->files_count_done, 
				 sdb_plugin->files_count);	

	anjuta_status_progress_tick (status, NULL, message);
	g_free (message);
}

static void
on_importing_project_end (SymbolDBEngine *dbe, gpointer data)
{
	SymbolDBPlugin *sdb_plugin;
	AnjutaStatus *status;
	
	g_return_if_fail (data != NULL);
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (data);
	
	DEBUG_PRINT ("on_importing_project_end ()");

	status = anjuta_shell_get_status (ANJUTA_PLUGIN (sdb_plugin)->shell, NULL);
	anjuta_status_progress_reset (status);
	
	/* re-enable signals receiving on local-view */
	symbol_db_view_locals_recv_signals_from_engine (
						SYMBOL_DB_VIEW_LOCALS (sdb_plugin->dbv_view_tree_locals), 
								 sdb_plugin->sdbe, TRUE);

	/* and on global view */
	symbol_db_view_recv_signals_from_engine (SYMBOL_DB_VIEW (sdb_plugin->dbv_view_tree), 
								 sdb_plugin->sdbe, TRUE);
	
	/* re-active global symbols */
	symbol_db_view_open (SYMBOL_DB_VIEW (sdb_plugin->dbv_view_tree), sdb_plugin->sdbe);
	
	/* disconnect this coz it's not important after the process of importing */
	g_signal_handlers_disconnect_by_func (dbe, on_single_file_scan_end, data);																 
	
	/* disconnect it as we don't need it anymore. */
	g_signal_handlers_disconnect_by_func (dbe, on_importing_project_end, data);	

	
	
	sdb_plugin->files_count_done = 0;
	sdb_plugin->files_count = 0;
}



/* add a new project */
static void
project_root_added (AnjutaPlugin *plugin, const gchar *name,
					const GValue *value, gpointer user_data)
{
	AnjutaStatus *status;
	IAnjutaProjectManager *pm;
	SymbolDBPlugin *sdb_plugin;
	const gchar *root_uri;
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (plugin);

	pm = anjuta_shell_get_interface (ANJUTA_PLUGIN (sdb_plugin)->shell,
									 IAnjutaProjectManager, NULL);
	
	g_free (sdb_plugin->project_root_uri);
	sdb_plugin->project_root_uri = NULL;
	root_uri = g_value_get_string (value);
	if (root_uri)
	{
		gchar *root_dir = gnome_vfs_get_local_path_from_uri (root_uri);
		DEBUG_PRINT ("Symbol-DB: added project %s", root_dir);
		if (root_dir)
		{
			gboolean needs_sources_scan = FALSE;
			gboolean project_exist = FALSE;
			gint i;
			GHashTable* lang_hash; 
				
			lang_hash = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, 
											  sources_array_free);
			
			status = anjuta_shell_get_status (plugin->shell, NULL);
			anjuta_status_progress_add_ticks (status, 1);

			/* is it a fresh-new project? is it an imported project with 
			 * no 'new' symbol-db database but the 'old' one symbol-browser? 
			 */
			if (symbol_db_engine_db_exists (sdb_plugin->sdbe, root_dir) == FALSE)
			{
				DEBUG_PRINT ("Symbol-DB: project did not exist");
				needs_sources_scan = TRUE;
				project_exist = FALSE;
			}
			else 
			{
				project_exist = TRUE;
			}

			if (symbol_db_engine_open_db (sdb_plugin->sdbe, root_dir) == FALSE)
				g_error ("Symbol-DB: error in opening db");

			/* if project did not exist add a new project */
			if (project_exist == FALSE)
			{
				DEBUG_PRINT ("Symbol-DB: creating new project.");
				symbol_db_engine_add_new_project (sdb_plugin->sdbe,
												  NULL,	/* still no workspace */
												  root_dir);
			}			

			/* open the project. we can do this only if the db was loaded */
			if (symbol_db_engine_open_project (sdb_plugin->sdbe, root_dir) == FALSE)
			{
				g_error ("Symbol-DB: error in opening project");
			}

			/* needs an import */
			if (needs_sources_scan == TRUE)
			{
				GList* prj_elements_list;
				GPtrArray* sources_array = NULL;
				GPtrArray* languages_array = NULL;
				GHashTable *check_unique_file;
				IAnjutaLanguage* lang_manager;
				
				/* if we're importing first shut off the signal receiving.
				 * We'll re-enable that on scan-end 
				 */
				symbol_db_view_locals_recv_signals_from_engine (																
							SYMBOL_DB_VIEW_LOCALS (sdb_plugin->dbv_view_tree_locals), 
											 sdb_plugin->sdbe, FALSE);

				symbol_db_view_recv_signals_from_engine (																
							SYMBOL_DB_VIEW (sdb_plugin->dbv_view_tree), 
											 sdb_plugin->sdbe, FALSE);
				
				g_signal_connect (G_OBJECT (sdb_plugin->sdbe), "scan-end",
					  G_CALLBACK (on_importing_project_end), plugin);
				
				
				lang_manager =	anjuta_shell_get_interface (plugin->shell, IAnjutaLanguage, 
													NULL);
			
				if (!lang_manager)
				{
					g_critical ("LanguageManager not found");
					return;
				}
								  
				prj_elements_list = ianjuta_project_manager_get_elements (pm,
								   IANJUTA_PROJECT_MANAGER_SOURCE,
								   NULL);
				/* to speed the things up we must avoid the dups */
				check_unique_file = g_hash_table_new_full (g_str_hash, 
									g_str_equal, g_free, g_free);

				DEBUG_PRINT ("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
				DEBUG_PRINT ("Retrieving %d gbf sources of the project...",
							 g_list_length (prj_elements_list));
				
				for (i=0; i < g_list_length (prj_elements_list); i++)
				{	
					gchar *local_filename;
					const gchar *file_mime;
					const gchar *lang;
					IAnjutaLanguageId lang_id;
					
					local_filename = 
						gnome_vfs_get_local_path_from_uri (g_list_nth_data (
														prj_elements_list, i));
					
					if (local_filename == NULL)
						continue;
					
					/* check if it's already present in the list. This to avoid
					 * duplicates.
					 */
					if (g_hash_table_lookup (check_unique_file, 
											 local_filename) == NULL)
					{
						g_hash_table_insert (check_unique_file, 
											 g_strdup (local_filename), 
											 g_strdup (local_filename));
					}
					else 
					{
						DEBUG_PRINT ("%s go away!", local_filename);
						/* you're a dup! we don't want you */
						g_free (local_filename);
						continue;
					}
					
					file_mime = gnome_vfs_get_mime_type_for_name (local_filename);
					
					lang_id = ianjuta_language_get_from_mime_type (lang_manager, 
																 file_mime, NULL);
					
					if (!lang_id)
					{
						g_free (local_filename);
						continue;
					}
				
					lang = ianjuta_language_get_name (lang_manager, lang_id, NULL);
					DEBUG_PRINT ("Language of %s is %s", local_filename, file_mime);
					/* test its existence */
					if (g_file_test (local_filename, G_FILE_TEST_EXISTS) == FALSE) 
					{
						g_free (local_filename);
						continue;
					}
					
					if (!sources_array)
					{
						sources_array = g_ptr_array_new ();
					}	
					
					if (!languages_array)
					{
						languages_array = g_ptr_array_new ();
					}

					sdb_plugin->files_count++;
					g_ptr_array_add (sources_array, local_filename);
					g_ptr_array_add (languages_array, g_strdup (lang));					
				}
			
				DEBUG_PRINT ("calling symbol_db_engine_add_new_files  with root_dir %s",
							 root_dir);

				/* connect to receive signals on single file scan complete. We'll
				 * update a status bar notifying the user about the status
				 */
				g_signal_connect (G_OBJECT (sdb_plugin->sdbe), "single-file-scan-end",
					  G_CALLBACK (on_single_file_scan_end), plugin);
				
				symbol_db_engine_add_new_files (sdb_plugin->sdbe, root_dir,
									sources_array, languages_array, TRUE);
				
				g_hash_table_unref (lang_hash);
				g_hash_table_unref (check_unique_file);
				
				g_ptr_array_foreach (sources_array, (GFunc)g_free, NULL);
				g_ptr_array_free (sources_array, TRUE);

				g_ptr_array_foreach (languages_array, (GFunc)g_free, NULL);
				g_ptr_array_free (languages_array, TRUE);
			}
			else	/* no import needed. Update the symbols */
			{
				symbol_db_engine_update_project_symbols (sdb_plugin->sdbe, root_dir);
			}
			anjuta_status_progress_tick (status, NULL, _("Populating symbols' db..."));
			anjuta_status_progress_add_ticks (status, sdb_plugin->files_count);
			
			symbol_db_view_open (SYMBOL_DB_VIEW (sdb_plugin->dbv_view_tree),
								 sdb_plugin->sdbe);

			/* root dir */
			sdb_plugin->project_root_dir = root_dir;
		}
		/* this is uri */
		sdb_plugin->project_root_uri = g_strdup (root_uri);
	}

	g_signal_connect (G_OBJECT (pm), "element_added",
					  G_CALLBACK (on_project_element_added), sdb_plugin);
	g_signal_connect (G_OBJECT (pm), "element_removed",
					  G_CALLBACK (on_project_element_removed), sdb_plugin);
}

static void
project_root_removed (AnjutaPlugin *plugin, const gchar *name,
					  gpointer user_data)
{
	IAnjutaProjectManager *pm;
	SymbolDBPlugin *sdb_plugin;
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (plugin);
	DEBUG_PRINT ("project_root_removed ()");
	/* Disconnect events from project manager */
	
	/* FIXME: There should be a way to ensure that this project manager
	 * is indeed the one that has opened the project_uri
	 */
	pm = anjuta_shell_get_interface (ANJUTA_PLUGIN (sdb_plugin)->shell,
									 IAnjutaProjectManager, NULL);
	g_signal_handlers_disconnect_by_func (G_OBJECT (pm),
										  on_project_element_added,
										  sdb_plugin);
	g_signal_handlers_disconnect_by_func (G_OBJECT (pm),
										  on_project_element_removed,
										  sdb_plugin);

	/* clear locals symbols and the associated cache*/
	symbol_db_view_locals_clear_cache (SYMBOL_DB_VIEW_LOCALS (
											sdb_plugin->dbv_view_tree_locals));
	
	/* clear global symbols */
	symbol_db_view_clear_cache (SYMBOL_DB_VIEW (sdb_plugin->dbv_view_tree));
	
	/* don't forget to close the project */
	symbol_db_engine_close_project (sdb_plugin->sdbe, 
									sdb_plugin->project_root_dir);

	g_free (sdb_plugin->project_root_uri);
	g_free (sdb_plugin->project_root_dir);
	sdb_plugin->project_root_uri = NULL;
	sdb_plugin->project_root_dir = NULL;
}

static void
on_session_load (AnjutaShell *shell, AnjutaSessionPhase phase,
				 AnjutaSession *session, SymbolDBPlugin *plugin)
{
	GList *files;
	gint i;			
	DEBUG_PRINT ("on_session_load ()");
	
	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;
	
	files = anjuta_session_get_string_list (session, "File Loader", "Files");

	for (i=0; i < g_list_length (files); i++)
	{
		gchar * node = g_list_nth_data (files, i);
		DEBUG_PRINT ("file %s", node);
	}
	
	g_list_foreach (files, (GFunc)g_free, NULL);
	g_list_free (files);	
}

static gboolean
symbol_db_activate (AnjutaPlugin *plugin)
{
	SymbolDBPlugin *symbol_db;
	
	DEBUG_PRINT ("SymbolDBPlugin: Activating SymbolDBPlugin plugin ...");

	register_stock_icons (plugin);

	symbol_db = ANJUTA_PLUGIN_SYMBOL_DB (plugin);
	symbol_db->ui = anjuta_shell_get_ui (plugin->shell, NULL);

	/* connect for session load event 
	g_signal_connect (plugin->shell, "load-session",
					  G_CALLBACK (on_session_load),
					  plugin);
	*/	
	
	/* create SymbolDBEngine */
	symbol_db->sdbe = symbol_db_engine_new ();
	
	/* Create widgets */
	symbol_db->dbv_notebook = gtk_notebook_new();
	
	/* Local symbols */
	symbol_db->scrolled_locals = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (symbol_db->scrolled_locals),
										 GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (symbol_db->scrolled_locals),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	
	symbol_db->dbv_view_locals_tab_label = gtk_label_new (_("Local" ));
	symbol_db->dbv_view_tree_locals = symbol_db_view_locals_new ();
	/* activate signals receiving by default */
	symbol_db_view_locals_recv_signals_from_engine (
					SYMBOL_DB_VIEW_LOCALS (symbol_db->dbv_view_tree_locals), 
											 symbol_db->sdbe, TRUE);										 

	g_object_add_weak_pointer (G_OBJECT (symbol_db->dbv_view_tree_locals),
							   (gpointer)&symbol_db->dbv_view_tree_locals);
	g_signal_connect (G_OBJECT (symbol_db->dbv_view_tree_locals), "row-activated",
					  G_CALLBACK (on_local_treeview_row_activated), plugin);

	gtk_container_add (GTK_CONTAINER(symbol_db->scrolled_locals), 
					   symbol_db->dbv_view_tree_locals);
	
	
	/* Global symbols */
	symbol_db->scrolled_global = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (
					GTK_SCROLLED_WINDOW (symbol_db->scrolled_global),
					GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (symbol_db->scrolled_global),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	
	symbol_db->dbv_view_tab_label = gtk_label_new (_("Global" ));
	symbol_db->dbv_view_tree = symbol_db_view_new ();
	g_object_add_weak_pointer (G_OBJECT (symbol_db->dbv_view_tree),
							   (gpointer)&symbol_db->dbv_view_tree);
	/* activate signals receiving by default */
	symbol_db_view_recv_signals_from_engine (
					SYMBOL_DB_VIEW (symbol_db->dbv_view_tree), 
											 symbol_db->sdbe, TRUE);										 

	g_signal_connect (G_OBJECT (symbol_db->dbv_view_tree), "row-activated",
					  G_CALLBACK (on_global_treeview_row_activated), plugin);

	g_signal_connect (G_OBJECT (symbol_db->dbv_view_tree), "row-expanded",
					  G_CALLBACK (on_global_treeview_row_expanded), plugin);
	
	gtk_container_add (GTK_CONTAINER(symbol_db->scrolled_global), 
					   symbol_db->dbv_view_tree);
	
	/* Search symbols */
	symbol_db->dbv_view_tree_search =
		(GtkWidget*) symbol_db_view_search_new (symbol_db->sdbe);
	symbol_db->dbv_view_search_tab_label = gtk_label_new (_("Search" ));

	g_signal_connect (G_OBJECT (symbol_db->dbv_view_tree_search), "symbol-selected",
					  G_CALLBACK (on_treesearch_symbol_selected_event),
					  plugin);
	
	g_object_add_weak_pointer (G_OBJECT (symbol_db->dbv_view_tree_search),
							   (gpointer)&symbol_db->dbv_view_tree_search);

	/* add the scrolled windows to the notebook */
	gtk_notebook_append_page (GTK_NOTEBOOK (symbol_db->dbv_notebook),
							  symbol_db->scrolled_locals, 
							  symbol_db->dbv_view_locals_tab_label);
	
	gtk_notebook_append_page (GTK_NOTEBOOK (symbol_db->dbv_notebook),
							  symbol_db->scrolled_global, 
							  symbol_db->dbv_view_tab_label);
	
	gtk_notebook_append_page (GTK_NOTEBOOK (symbol_db->dbv_notebook),
							  symbol_db->dbv_view_tree_search, 
							  symbol_db->dbv_view_search_tab_label);

	gtk_widget_show_all (symbol_db->dbv_notebook);
	
	/* setting focus to the tree_view*/
	gtk_notebook_set_current_page (GTK_NOTEBOOK (symbol_db->dbv_notebook), 0);

	symbol_db->editor_watch_id = 
		anjuta_plugin_add_watch (plugin, "document_manager_current_editor",
								 value_added_current_editor,
								 value_removed_current_editor, NULL);
#if 0
	/* Add UI */
	symbol_db->merge_id = 
		anjuta_ui_merge (symbol_db->ui, UI_FILE);
#endif	
	/* Added widgets */
	anjuta_shell_add_widget (plugin->shell, symbol_db->dbv_notebook,
							 "AnjutaSymbolBrowser", _("Symbols"),
							 "symbol-db-plugin-icon",
							 ANJUTA_SHELL_PLACEMENT_LEFT, NULL);	

	/* set up project directory watch */
	symbol_db->root_watch_id = anjuta_plugin_add_watch (plugin,
									"project_root_uri",
									project_root_added,
									project_root_removed, NULL);

	
	return TRUE;
}

static gboolean
symbol_db_deactivate (AnjutaPlugin *plugin)
{
	SymbolDBPlugin *sdb_plugin;
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (plugin);

	DEBUG_PRINT ("SymbolDBPlugin: Dectivating SymbolDBPlugin plugin ...");

	DEBUG_PRINT ("SymbolDBPlugin: destroying engine ...");
	g_object_unref (sdb_plugin->sdbe);
	sdb_plugin->sdbe = NULL;

	/* disconnect some signals */
	g_signal_handlers_disconnect_by_func (G_OBJECT (sdb_plugin->dbv_view_tree_locals),
									  on_local_treeview_row_activated,
									  plugin);

	g_signal_handlers_disconnect_by_func (G_OBJECT (sdb_plugin->dbv_view_tree_search),
									  on_treesearch_symbol_selected_event,
									  plugin);
	
/*	
	DEBUG_PRINT ("SymbolDBPlugin: destroying locals view ...");
	g_object_unref (sdb_plugin->dbv_view_tree_locals);
		
	DEBUG_PRINT ("SymbolDBPlugin: destroying globals view ...");
	g_object_unref (sdb_plugin->dbv_view_tree);
	
	DEBUG_PRINT ("SymbolDBPlugin: destroying search ...");
	g_object_unref (sdb_plugin->dbv_view_tree_search);
*/	
	
	/* Ensure all editor cached info are released */
	if (sdb_plugin->editor_connected)
	{
		g_hash_table_foreach (sdb_plugin->editor_connected,
							  on_editor_foreach_disconnect, plugin);
		g_hash_table_foreach (sdb_plugin->editor_connected,
							  on_editor_foreach_clear, plugin);
		g_hash_table_destroy (sdb_plugin->editor_connected);
		sdb_plugin->editor_connected = NULL;
	}
	
	/* Remove watches */
	anjuta_plugin_remove_watch (plugin, sdb_plugin->root_watch_id, FALSE);
	anjuta_plugin_remove_watch (plugin, sdb_plugin->editor_watch_id, TRUE);
	
	/* Remove widgets: Widgets will be destroyed when dbv_notebook is removed */
	anjuta_shell_remove_widget (plugin->shell, sdb_plugin->dbv_notebook, NULL);

	sdb_plugin->root_watch_id = 0;
	sdb_plugin->editor_watch_id = 0;
	sdb_plugin->dbv_notebook = NULL;
	sdb_plugin->scrolled_global = NULL;
	sdb_plugin->scrolled_locals = NULL;
	sdb_plugin->dbv_view_tree = NULL;
	sdb_plugin->dbv_view_tab_label = NULL;
	sdb_plugin->dbv_view_tree_locals = NULL;
	sdb_plugin->dbv_view_locals_tab_label = NULL;
	sdb_plugin->dbv_view_tree_search = NULL;
	sdb_plugin->dbv_view_search_tab_label = NULL;
	return TRUE;
}

static void
symbol_db_finalize (GObject *obj)
{
	DEBUG_PRINT ("Symbol-DB finalize");
	/* Finalization codes here */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
symbol_db_dispose (GObject *obj)
{
	/* Disposition codes */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
symbol_db_instance_init (GObject *obj)
{
	SymbolDBPlugin *plugin = (SymbolDBPlugin*)obj;

	plugin->uiid = 0;
	plugin->widget = NULL;
	plugin->files_count_done = 0;
	plugin->files_count = 0;
}

static void
symbol_db_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = symbol_db_activate;
	plugin_class->deactivate = symbol_db_deactivate;
	klass->finalize = symbol_db_finalize;
	klass->dispose = symbol_db_dispose;
}

static IAnjutaIterable*
isymbol_manager_search (IAnjutaSymbolManager *sm,
						IAnjutaSymbolType match_types,
						const gchar *match_name,
						gboolean partial_name_match,
						gboolean global_search,
						GError **err)
{
	SymbolDBEngineIterator *iterator = NULL;
	SymbolDBPlugin *sdb_plugin;
	SymbolDBEngine *dbe;
	const gchar* name;

	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (sm);
	dbe = SYMBOL_DB_ENGINE (sdb_plugin->sdbe);
	
	if (match_name && strlen (match_name) > 0)
		name = match_name;
	else
		name = NULL;
	
	iterator = 
		symbol_db_engine_find_symbol_by_name_pattern (dbe, 
									name, SYMINFO_SIMPLE |
								  	SYMINFO_FILE_PATH |
									SYMINFO_IMPLEMENTATION |
									SYMINFO_ACCESS |
									SYMINFO_KIND |
									SYMINFO_TYPE |
									SYMINFO_TYPE_NAME |
									SYMINFO_LANGUAGE |
									SYMINFO_FILE_IGNORE |
									SYMINFO_FILE_INCLUDE |
									SYMINFO_PROJECT_NAME |
									SYMINFO_WORKSPACE_NAME );

	return IANJUTA_ITERABLE (iterator);
}

static IAnjutaIterable*
isymbol_manager_get_members (IAnjutaSymbolManager *sm,
							 const gchar *symbol_name,
							 gboolean global_search,
							 GError **err)
{
	/* TODO */
	DEBUG_PRINT ("TODO: isymbol_manager_get_members ()");
	return NULL;
}

static IAnjutaIterable*
isymbol_manager_get_parents (IAnjutaSymbolManager *sm,
							 const gchar *symbol_name,
							 GError **err)
{
#if 0
	const GPtrArray *tags_array;
	AnjutaSymbolIter *iter = NULL;
	
	tags_array = tm_workspace_get_parents (symbol_name);
	if (tags_array)
	{
		iter = anjuta_symbol_iter_new (tags_array);
		return IANJUTA_ITERABLE (iter);
	}
#endif	
	return NULL;
}

static IAnjutaIterable*
isymbol_manager_get_completions_at_position (IAnjutaSymbolManager *sm,
											const gchar *file_uri,
							 				const gchar *text_buffer, 
											gint text_length, 
											gint text_pos,
							 				GError **err)
{
	/* TODO */
	DEBUG_PRINT ("TODO: isymbol_manager_get_completions_at_position ()");
	return NULL;
}


static void
isymbol_manager_iface_init (IAnjutaSymbolManagerIface *iface)
{
	iface->search = isymbol_manager_search;
	iface->get_members = isymbol_manager_get_members;
	iface->get_parents = isymbol_manager_get_parents;
	iface->get_completions_at_position = isymbol_manager_get_completions_at_position;
}

ANJUTA_PLUGIN_BEGIN (SymbolDBPlugin, symbol_db);
ANJUTA_PLUGIN_ADD_INTERFACE (isymbol_manager, IANJUTA_TYPE_SYMBOL_MANAGER);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (SymbolDBPlugin, symbol_db);
