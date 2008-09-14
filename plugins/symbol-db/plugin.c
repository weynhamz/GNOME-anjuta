/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.c
 * Copyright (C) Massimo Cora' 2007-2008 <maxcvs@email.it>
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
#include <libanjuta/anjuta-utils.h>
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
#include <libanjuta/interfaces/ianjuta-preferences.h>

#include "plugin.h"
#include "symbol-db-view.h"
#include "symbol-db-view-locals.h"
#include "symbol-db-view-search.h"
#include "symbol-db-engine.h"
#include "symbol-db-engine-iterator.h"
#include "symbol-db-prefs.h"

#define ICON_FILE "anjuta-symbol-db-plugin-48.png"

#define TIMEOUT_INTERVAL_SYMBOLS_UPDATE		10000
#define TIMEOUT_SECONDS_AFTER_LAST_TIP		5

#define PROJECT_GLOBALS		"/"
#define SESSION_SECTION		"SymbolDB"
#define SESSION_KEY			"SystemPackages"

static gpointer parent_class;

/* signals */
enum
{
	PROJECT_IMPORT_END,
	GLOBALS_IMPORT_END,
	LAST_SIGNAL
};

static unsigned int signals[LAST_SIGNAL] = { 0 };

static gint 
g_list_compare (gconstpointer a, gconstpointer b)
{
	return strcmp ((const gchar*)a, (const gchar*)b);
}

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
	if (sdb_plugin->update_timer == NULL)
		return TRUE;
	
	seconds_elapsed = g_timer_elapsed (sdb_plugin->update_timer, NULL);
	
	/* DEBUG_PRINT ("seconds_elapsed  %f", seconds_elapsed ); */
	if (seconds_elapsed < TIMEOUT_SECONDS_AFTER_LAST_TIP)
		return TRUE;
		
		
	 /* we won't proceed with the updating of the symbols if we didn't type in 
	 	anything */
	 if (sdb_plugin->need_symbols_update == FALSE)
	 	return TRUE;
	
	DEBUG_PRINT ("on_editor_buffer_symbols_update_timeout()");
	
	if (sdb_plugin->current_editor) 
	{
		GFile* file;
		ed = IANJUTA_EDITOR (sdb_plugin->current_editor);
		
		buffer_size = ianjuta_editor_get_length (ed, NULL);
		current_buffer = ianjuta_editor_get_text_all (ed, NULL);
				
		file = ianjuta_file_get_file (IANJUTA_FILE (ed), NULL);
		uri = g_file_get_uri (file);
		g_object_unref (file);
	} 
	else
		return FALSE;
	
	if (uri) 
	{
		DEBUG_PRINT ("uri for buffer updating: %s", uri);
		
		GPtrArray *real_files_list;
		GPtrArray *text_buffers;
		GPtrArray *buffer_sizes;
								
		gchar * local_path = gnome_vfs_get_local_path_from_uri (uri);

		real_files_list = g_ptr_array_new ();
		g_ptr_array_add (real_files_list, local_path);

		text_buffers = g_ptr_array_new ();
		g_ptr_array_add (text_buffers, current_buffer);	

		buffer_sizes = g_ptr_array_new ();
		g_ptr_array_add (buffer_sizes, (gpointer)buffer_size);	

		symbol_db_engine_update_buffer_symbols (sdb_plugin->sdbe_project,
												sdb_plugin->project_opened,
												real_files_list,
												text_buffers,
												buffer_sizes);
												
		g_free (uri);
	}
	
	g_free (current_buffer);  

	sdb_plugin->need_symbols_update = FALSE;

	return TRUE;
}

static void
on_editor_destroy (SymbolDBPlugin *sdb_plugin, IAnjutaEditor *editor)
{
	const gchar *uri;
	DEBUG_PRINT ("on_editor_destroy ()");
	if (!sdb_plugin->editor_connected || !sdb_plugin->dbv_view_tree)
	{
		DEBUG_PRINT ("on_editor_destroy (): returning....");
		return;
	}
	
	uri = g_hash_table_lookup (sdb_plugin->editor_connected, G_OBJECT (editor));
	g_hash_table_remove (sdb_plugin->editor_connected, G_OBJECT (editor));
	
	/* was it the last file loaded? */
	if (g_hash_table_size (sdb_plugin->editor_connected) <= 0)
	{
		DEBUG_PRINT ("displaying nothing...");
		symbol_db_view_locals_display_nothing (
				SYMBOL_DB_VIEW_LOCALS (sdb_plugin->dbv_view_tree_locals), TRUE);
	}
}

static void
on_editor_update_ui (IAnjutaEditor *editor, SymbolDBPlugin *sdb_plugin) 
{
	g_timer_reset (sdb_plugin->update_timer);
}

static void
on_char_added (IAnjutaEditor *editor, IAnjutaIterable *position, gchar ch,
			   SymbolDBPlugin *sdb_plugin)
{
	g_timer_reset (sdb_plugin->update_timer);
	
	/* Update when the user enters a newline */
	if (ch == '\n')	
		sdb_plugin->need_symbols_update = TRUE;
}


static void
on_editor_saved (IAnjutaEditor *editor, GFile* file,
				 SymbolDBPlugin *sdb_plugin)
{
	const gchar *old_uri;
	gboolean tags_update;
	
	tags_update = TRUE;		
	
	if (tags_update)
	{
		gchar *local_filename = g_file_get_path (file);
		gchar *saved_uri = g_file_get_uri (file);
		GPtrArray *files_array;

		/* Verify that it's local file */
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
		symbol_db_engine_update_files_symbols (sdb_plugin->sdbe_project, 
				sdb_plugin->project_root_dir, files_array, TRUE);
		g_hash_table_insert (sdb_plugin->editor_connected, editor,
							 g_strdup (saved_uri));

		/* if we saved it we shouldn't update a second time */
		sdb_plugin->need_symbols_update = FALSE;			
			
		on_editor_update_ui (editor, sdb_plugin);
		g_free (saved_uri);
	}
}

static void
value_added_current_editor (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	gchar *uri;
	gboolean tags_update;
	GFile* file;
	gchar *local_path;
	GObject *editor;
	SymbolDBPlugin *sdb_plugin;
	
	editor = g_value_get_object (value);	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (plugin);
	
	/* we have an editor added, so let locals to display something */
	symbol_db_view_locals_display_nothing (
			SYMBOL_DB_VIEW_LOCALS (sdb_plugin->dbv_view_tree_locals), FALSE);

	
	if (sdb_plugin->session_loading)
	{
		DEBUG_PRINT ("session_loading");
		return;
	}
	else
		DEBUG_PRINT ("Updating symbols");
	
	if (!sdb_plugin->editor_connected)
	{
		sdb_plugin->editor_connected = g_hash_table_new_full (g_direct_hash,
															 g_direct_equal,
															 NULL, g_free);
	}
	sdb_plugin->current_editor = editor;
	
	if (!IANJUTA_IS_EDITOR (editor))
		return;
	
	file = ianjuta_file_get_file (IANJUTA_FILE (editor), NULL);
	
	if (file == NULL)
		return;

	local_path = g_file_get_path (file);
	uri = g_file_get_uri (file);
	DEBUG_PRINT ("value_added_current_editor () gonna refresh local syms: local_path %s "
				 "uri %s", local_path, uri);
	if (local_path == NULL)
	{
		g_critical ("FIXME local_path == NULL");
		return;
	}
	
	if (strstr (local_path, "//") != NULL)
	{
		g_critical ("WARNING FIXME: bad file uri passed to symbol-db from editor. There's "
				   "a trailing slash left. Please fix this at editor side");
	}
				
	symbol_db_view_locals_update_list (
				SYMBOL_DB_VIEW_LOCALS (sdb_plugin->dbv_view_tree_locals),
				 sdb_plugin->sdbe_project, local_path);
				 
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
	tags_update = anjuta_preferences_get_int (sdb_plugin->prefs, BUFFER_AUTOSCAN);
				
	if (tags_update)
		sdb_plugin->buf_update_timeout_id = 
				g_timeout_add (TIMEOUT_INTERVAL_SYMBOLS_UPDATE,
								on_editor_buffer_symbols_update_timeout,
								plugin);
	sdb_plugin->need_symbols_update = FALSE;
}

static void
on_session_save (AnjutaShell *shell, AnjutaSessionPhase phase,
				 AnjutaSession *session,
				 SymbolDBPlugin *sdb_plugin)
{
	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;

	DEBUG_PRINT ("SymbolDB: session_save");

	anjuta_session_set_string_list (session, 
									SESSION_SECTION, 
									SESSION_KEY,
									sdb_plugin->session_packages);	
}

static void
on_session_load (AnjutaShell *shell, AnjutaSessionPhase phase,
				 AnjutaSession *session,
				 SymbolDBPlugin *sdb_plugin)
{
	IAnjutaProjectManager *pm;
	pm = anjuta_shell_get_interface (ANJUTA_PLUGIN (sdb_plugin)->shell,
									 IAnjutaProjectManager, NULL);	
	
	if (phase == ANJUTA_SESSION_PHASE_START)
	{
		GList *session_packages = anjuta_session_get_string_list (session, 
														SESSION_SECTION, 
														SESSION_KEY);
		
		GList *to_scan_packages = NULL;
	
		DEBUG_PRINT ("SymbolDB: session_loading started. Getting info from %s",
					 anjuta_session_get_session_directory (session));
		sdb_plugin->session_loading = TRUE;
		
		if (session_packages == NULL)
		{
			/* hey, does user want to import system sources for this project? */
			gboolean automatic_scan = anjuta_preferences_get_int (sdb_plugin->prefs, 
														  PROJECT_AUTOSCAN);
			/* letting session_packages to NULL won't start the population */
			if (automatic_scan == TRUE)
			{
				GList *project_default_packages = 
					ianjuta_project_manager_get_packages (pm, NULL);
			
				/* take the project's defaults */
				to_scan_packages = project_default_packages;
			}
		}
		else
		{
			to_scan_packages = session_packages;
		}

		
		sdb_plugin->session_packages = to_scan_packages;
		
		/* no need to free the GList(s) */
	}
	else if (phase == ANJUTA_SESSION_PHASE_END)
	{
		IAnjutaDocumentManager* docman;
		sdb_plugin->session_loading = FALSE;
		DEBUG_PRINT ("SymbolDB: session_loading finished");
		
		/* Show the symbols for the current editor */
		docman = anjuta_shell_get_interface (shell, IAnjutaDocumentManager, NULL);
		if (docman)
		{
			IAnjutaDocument* cur_doc = 
				ianjuta_document_manager_get_current_document (docman, NULL);
			if (cur_doc)
			{
				GValue value = {0, };
				g_value_init (&value, G_TYPE_OBJECT);
				g_value_set_object (&value, cur_doc);
				value_added_current_editor (ANJUTA_PLUGIN (sdb_plugin),
											"document_manager_current_document",
											&value, NULL);
				g_value_unset(&value);
			}
		}
	}	
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
	IAnjutaDocumentManager *docman;
	GFile* file;
	
	g_return_if_fail (filename != NULL);
		
	/* Go to file and line number */
	docman = anjuta_shell_get_interface (plugin->shell, IAnjutaDocumentManager,
										 NULL);
	file = g_file_new_for_path (filename);
	ianjuta_document_manager_goto_file_line (docman, file, lineno, NULL);
	
	g_object_unref (file);
}


static void
goto_local_tree_iter (SymbolDBPlugin *sdb_plugin, GtkTreeIter *iter)
{
	gint line;

	line = symbol_db_view_locals_get_line (SYMBOL_DB_VIEW_LOCALS (
									sdb_plugin->dbv_view_tree_locals), 
										   sdb_plugin->sdbe_project,
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
			SYMBOL_DB_VIEW (sdb_plugin->dbv_view_tree), sdb_plugin->sdbe_project,
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
								user_data->sdbe_project, iter);
}

static void
on_global_treeview_row_collapsed (GtkTreeView *tree_view,
									GtkTreeIter *iter,
                                    GtkTreePath *path,
                                    SymbolDBPlugin *user_data)
{
	DEBUG_PRINT ("on_global_treeview_row_collapsed ()");
	
	symbol_db_view_row_collapsed (SYMBOL_DB_VIEW (user_data->dbv_view_tree),
								user_data->sdbe_project, iter);
	
}

static void
value_removed_current_editor (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	SymbolDBPlugin *sdb_plugin;

	if (!IANJUTA_IS_EDITOR (data))
		return;
	
	sdb_plugin = (SymbolDBPlugin *) plugin;
	
	DEBUG_PRINT ("value_removed_current_editor ()");
	/* let's remove the timeout for symbols refresh */
	g_source_remove (sdb_plugin->buf_update_timeout_id);
	sdb_plugin->buf_update_timeout_id = 0;
	sdb_plugin->need_symbols_update = FALSE;
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (plugin);
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
		
		symbol_db_engine_add_new_files (sdb_plugin->sdbe_project, 
			sdb_plugin->project_opened, files_array, languages_array, TRUE);
		
		g_free (filename);
		g_ptr_array_free (files_array, TRUE);
		
		g_ptr_array_foreach (languages_array, (GFunc)g_free, NULL);
		g_ptr_array_free (languages_array, TRUE);
	}
}

static void
on_project_element_removed (IAnjutaProjectManager *pm, const gchar *uri,
							SymbolDBPlugin *sdb_plugin)
{
	gchar *filename;
	
	if (!sdb_plugin->project_root_uri)
		return;
	
	filename = gnome_vfs_get_local_path_from_uri (uri);
	if (filename)
	{
		DEBUG_PRINT ("on_project_element_removed");
		DEBUG_PRINT ("project_root_dir %s", sdb_plugin->project_root_dir );
		symbol_db_engine_remove_file (sdb_plugin->sdbe_project, 
			sdb_plugin->project_root_dir, filename);
		
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
on_system_scan_package_start (SymbolDBEngine *dbe, guint num_files, 
							  const gchar *package, gpointer user_data)
{
	SymbolDBPlugin *sdb_plugin;
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (user_data);

	sdb_plugin->files_count_system_done = 0;
	sdb_plugin->files_count_system = num_files;	
	
	DEBUG_PRINT ("on_system_scan_package_start  () [%s]", package);
	
	/* show the global bar */
	gtk_widget_show (sdb_plugin->progress_bar_system);
	if (sdb_plugin->current_scanned_package != NULL)
		g_free (sdb_plugin->current_scanned_package);
	sdb_plugin->current_scanned_package = g_strdup (package);
}

static void
on_system_scan_package_end (SymbolDBEngine *dbe, const gchar *package, 
							  gpointer user_data)
{
	SymbolDBPlugin *sdb_plugin;
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (user_data);
	
	DEBUG_PRINT ("on_system_scan_package_end () [%s]", package);
	
	/* hide the progress bar */
	gtk_widget_hide (sdb_plugin->progress_bar_system);
	
	sdb_plugin->files_count_system_done = 0;
	sdb_plugin->files_count_system = 0;
}

static void
on_system_single_file_scan_end (SymbolDBEngine *dbe, gpointer data)
{
	AnjutaPlugin *plugin;
	SymbolDBPlugin *sdb_plugin;
	gchar *message;
	gdouble fraction = 0;
	
	plugin = ANJUTA_PLUGIN (data);
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (plugin);

	DEBUG_PRINT ("on_system_single_file_scan_end ()");
	
	sdb_plugin->files_count_system_done++;	
	if (sdb_plugin->files_count_system_done >= sdb_plugin->files_count_system)
		message = g_strdup_printf (_("%s: Generating inheritances..."), 
								   sdb_plugin->current_scanned_package);
	else
		message = g_strdup_printf (_("%s: %d files scanned out of %d"), 
							sdb_plugin->current_scanned_package,
							sdb_plugin->files_count_system_done, 
							sdb_plugin->files_count_system);

	if (sdb_plugin->files_count_system > 0)
	{
		fraction =  (gdouble) sdb_plugin->files_count_system_done / 
			(gdouble) sdb_plugin->files_count_system;
	}
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (sdb_plugin->progress_bar_system),
								   fraction);
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (sdb_plugin->progress_bar_system), message);
	gtk_widget_show (sdb_plugin->progress_bar_system);
	g_free (message);	
}

static void
on_project_single_file_scan_end (SymbolDBEngine *dbe, gpointer data)
{	
	AnjutaPlugin *plugin;
	SymbolDBPlugin *sdb_plugin;
	gchar *message;
	gdouble fraction = 0;
	
	plugin = ANJUTA_PLUGIN (data);
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (plugin);
	
	sdb_plugin->files_count_project_done++;	
	if (sdb_plugin->files_count_project_done >= sdb_plugin->files_count_project)
		message = g_strdup_printf (_("Generating inheritances..."));
	else
		message = g_strdup_printf (_("%d files scanned out of %d"), 
							   sdb_plugin->files_count_project_done, sdb_plugin->files_count_project);
	
	if (sdb_plugin->files_count_project > 0)
	{
		fraction =  (gdouble) sdb_plugin->files_count_project_done / 
			(gdouble) sdb_plugin->files_count_project;
	}
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (sdb_plugin->progress_bar_project),
								   fraction);
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (sdb_plugin->progress_bar_project), message);
	gtk_widget_show (sdb_plugin->progress_bar_project);
	g_free (message);
}

static void
on_importing_project_end (SymbolDBEngine *dbe, gpointer data)
{
	SymbolDBPlugin *sdb_plugin;
	GFile* file;
	gchar *local_path;
	
	g_return_if_fail (data != NULL);
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (data);
	
	DEBUG_PRINT ("on_importing_project_end ()");

	/* hide the progress bar */
	gtk_widget_hide (sdb_plugin->progress_bar_project);
	
	/* re-enable signals receiving on local-view */
	symbol_db_view_locals_recv_signals_from_engine (
						SYMBOL_DB_VIEW_LOCALS (sdb_plugin->dbv_view_tree_locals), 
								 sdb_plugin->sdbe_project, TRUE);

	/* and on global view */
	symbol_db_view_recv_signals_from_engine (SYMBOL_DB_VIEW (sdb_plugin->dbv_view_tree), 
								 sdb_plugin->sdbe_project, TRUE);
	
	/* re-active global symbols */
	symbol_db_view_open (SYMBOL_DB_VIEW (sdb_plugin->dbv_view_tree), 
						 sdb_plugin->sdbe_project);
	
	/* disconnect this coz it's not important after the process of importing */
	g_signal_handlers_disconnect_by_func (dbe, on_project_single_file_scan_end, 
										  data);																 
	
	/* disconnect it as we don't need it anymore. */
	g_signal_handlers_disconnect_by_func (dbe, on_importing_project_end, data);
	
	sdb_plugin->files_count_project_done = 0;
	sdb_plugin->files_count_project = 0;	

	DEBUG_PRINT ("emitting signals[PROJECT_IMPORT_END]");
	/* emit signal. */
	g_signal_emit (sdb_plugin, signals[PROJECT_IMPORT_END], 0);	

	/* ok, enable local symbols view */
	if (!IANJUTA_IS_EDITOR (sdb_plugin->current_editor))
		return;
	
	file = ianjuta_file_get_file (IANJUTA_FILE (sdb_plugin->current_editor), 
								  NULL);
	
	if (file == NULL)
		return;

	local_path = g_file_get_path (file);
	if (local_path == NULL)
	{
		g_critical ("FIXME local_path == NULL");
		return;
	}
	
	if (strstr (local_path, "//") != NULL)
	{
		g_critical ("WARNING FIXME: bad file uri passed to symbol-db from editor. There's "
				   "a trailing slash left. Please fix this at editor side");
	}
				
	symbol_db_view_locals_update_list (
				SYMBOL_DB_VIEW_LOCALS (sdb_plugin->dbv_view_tree_locals),
				 sdb_plugin->sdbe_project, local_path);	
	g_free (local_path);
}

static void
do_import_system_src_after_abort (SymbolDBPlugin *sdb_plugin,
								  const GPtrArray *sources_array)
{
	AnjutaPlugin *plugin;
	GPtrArray* languages_array = NULL;
	GPtrArray *to_scan_array = NULL;
	IAnjutaLanguage* lang_manager;
	gint i;
	
	plugin = ANJUTA_PLUGIN (sdb_plugin);

	lang_manager =	anjuta_shell_get_interface (plugin->shell, IAnjutaLanguage, 
										NULL);

	DEBUG_PRINT ("do_import_system_src_after_abort %d", sources_array->len);
	/* create array of languages */
	languages_array = g_ptr_array_new ();
	to_scan_array = g_ptr_array_new ();
	
	if (!lang_manager)
	{
		g_critical ("LanguageManager not found");
		return;
	}

	for (i=0; i < sources_array->len; i++) 
	{
		const gchar *file_mime;
		const gchar *lang;
		const gchar *local_filename;
		IAnjutaLanguageId lang_id;
		
		local_filename = g_ptr_array_index (sources_array, i);
		
		if (local_filename == NULL)
			continue;
		file_mime = gnome_vfs_get_mime_type_for_name (local_filename);
					
		lang_id = ianjuta_language_get_from_mime_type (lang_manager, 
													 file_mime, NULL);
					
		if (!lang_id)		
			continue;
						
		lang = ianjuta_language_get_name (lang_manager, lang_id, NULL);

		/* test its existence */
		if (g_file_test (local_filename, G_FILE_TEST_EXISTS) == FALSE) 		
			continue;		
					
		g_ptr_array_add (languages_array, g_strdup (lang));					
		g_ptr_array_add (to_scan_array, g_strdup (local_filename));
	}

	symbol_db_system_parse_aborted_package (sdb_plugin->sdbs, 
									 to_scan_array,
									 languages_array);
	
	/* no need to free the GPtrArray, Huston. They'll be auto-destroyed in that
	 * function 
	 */
}

/* we assume that sources_array has already unique elements */
static void
do_import_project_src_after_abort (AnjutaPlugin *plugin, 
							   const GPtrArray *sources_array)
{
	SymbolDBPlugin *sdb_plugin;
	GPtrArray* languages_array = NULL;
	GPtrArray *to_scan_array = NULL;
	IAnjutaLanguage* lang_manager;
	gint i;
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (plugin);	
		

	lang_manager =	anjuta_shell_get_interface (plugin->shell, IAnjutaLanguage, 
										NULL);
	
	/* create array of languages */
	languages_array = g_ptr_array_new ();
	to_scan_array = g_ptr_array_new ();
	
	if (!lang_manager)
	{
		g_critical ("LanguageManager not found");
		return;
	}

	for (i=0; i < sources_array->len; i++) 
	{
		const gchar *file_mime;
		const gchar *lang;
		const gchar *local_filename;
		IAnjutaLanguageId lang_id;
		
		local_filename = g_ptr_array_index (sources_array, i);
		
		if (local_filename == NULL)
			continue;
		file_mime = gnome_vfs_get_mime_type_for_name (local_filename);
					
		lang_id = ianjuta_language_get_from_mime_type (lang_manager, 
													 file_mime, NULL);
					
		if (!lang_id)
		{
			continue;
		}
				
		lang = ianjuta_language_get_name (lang_manager, lang_id, NULL);

		/* test its existence */
		if (g_file_test (local_filename, G_FILE_TEST_EXISTS) == FALSE) 
		{
			continue;
		}
					
		sdb_plugin->files_count_project++;
		g_ptr_array_add (languages_array, g_strdup (lang));					
		g_ptr_array_add (to_scan_array, g_strdup (local_filename));
	}
			
	/* connect to receive signals on single file scan complete. We'll
	 * update a status bar notifying the user about the status
	 */
	g_signal_connect (G_OBJECT (sdb_plugin->sdbe_project), "single-file-scan-end",
		  G_CALLBACK (on_project_single_file_scan_end), plugin);
	
	symbol_db_engine_add_new_files (sdb_plugin->sdbe_project, sdb_plugin->project_opened,
					sources_array, languages_array, TRUE);
			
	g_ptr_array_foreach (languages_array, (GFunc)g_free, NULL);
	g_ptr_array_free (languages_array, TRUE);
	
	g_ptr_array_foreach (to_scan_array, (GFunc)g_free, NULL);
	g_ptr_array_free (to_scan_array, TRUE);
	
	/* signal */
	g_signal_emit (sdb_plugin, signals[PROJECT_IMPORT_END], 0);
}

static void
do_import_project_sources (AnjutaPlugin *plugin, IAnjutaProjectManager *pm, 
				   const gchar *root_dir)
{
	SymbolDBPlugin *sdb_plugin;
	GList* prj_elements_list;
	GPtrArray* sources_array = NULL;
	GPtrArray* languages_array = NULL;
	GHashTable *check_unique_file;
	IAnjutaLanguage* lang_manager;
	gint i;
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (plugin);	
		
	/* if we're importing first shut off the signal receiving.
	 * We'll re-enable that on scan-end 
	 */
	symbol_db_view_locals_recv_signals_from_engine (																
				SYMBOL_DB_VIEW_LOCALS (sdb_plugin->dbv_view_tree_locals), 
								 sdb_plugin->sdbe_project, FALSE);

	symbol_db_view_recv_signals_from_engine (																
				SYMBOL_DB_VIEW (sdb_plugin->dbv_view_tree), 
								 sdb_plugin->sdbe_project, FALSE);
				
	g_signal_connect (G_OBJECT (sdb_plugin->sdbe_project), "scan-end",
		  G_CALLBACK (on_importing_project_end), plugin);				
				
	lang_manager =	anjuta_shell_get_interface (plugin->shell, IAnjutaLanguage, 
										NULL);
			
	if (lang_manager == NULL)
	{
		g_critical ("LanguageManager not found");
		return;
	}
								  
	prj_elements_list = ianjuta_project_manager_get_elements (pm,
					   IANJUTA_PROJECT_MANAGER_SOURCE,
					   NULL);
	
	if (prj_elements_list == NULL)
	{
		g_critical ("No sources found within this project");
		return;
	}
	
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
					
		/* check if it's already present in the list. This avoids
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
			sources_array = g_ptr_array_new ();
					
		if (!languages_array)
			languages_array = g_ptr_array_new ();

		sdb_plugin->files_count_project++;
		g_ptr_array_add (sources_array, local_filename);
		g_ptr_array_add (languages_array, g_strdup (lang));					
	}
			
	DEBUG_PRINT ("calling symbol_db_engine_add_new_files  with root_dir %s",
			 root_dir);

	/* connect to receive signals on single file scan complete. We'll
	 * update a status bar notifying the user about the status
	 */
	g_signal_connect (G_OBJECT (sdb_plugin->sdbe_project), "single-file-scan-end",
		  G_CALLBACK (on_project_single_file_scan_end), plugin);
	
	symbol_db_engine_add_new_files (sdb_plugin->sdbe_project, 
									sdb_plugin->project_opened,
					sources_array, languages_array, TRUE);
				
	g_hash_table_unref (check_unique_file);
				
	g_ptr_array_foreach (sources_array, (GFunc)g_free, NULL);
	g_ptr_array_free (sources_array, TRUE);

	g_ptr_array_foreach (languages_array, (GFunc)g_free, NULL);
	g_ptr_array_free (languages_array, TRUE);	
}

static  void
on_received_project_import_end (SymbolDBPlugin *sdb_plugin, gpointer data)
{
	DEBUG_PRINT ("on_received_project_import_end ()");
	/* system's packages management */				
	GList *item = sdb_plugin->session_packages; 
	while (item != NULL)
	{
		/* the function will take care of checking if the package is already 
	 	 * scanned and present on db 
	 	 */
		DEBUG_PRINT ("ianjuta_project_manager_get_packages: package required: %s", 
				 (gchar*)item->data);
		symbol_db_system_scan_package (sdb_plugin->sdbs, item->data);
				
		item = item->next;
	}
	

	/* the resume thing */
	GPtrArray *sys_src_array = NULL;
	sys_src_array = 
		symbol_db_engine_get_files_with_zero_symbols (sdb_plugin->sdbe_globals);

	if (sys_src_array != NULL && sys_src_array->len > 0) 
	{
		do_import_system_src_after_abort (sdb_plugin, sys_src_array);
			
		g_ptr_array_foreach (sys_src_array, (GFunc)g_free, NULL);
		g_ptr_array_free (sys_src_array, TRUE);
	}	
}

#if 0
static void
do_check_offline_files_changed (SymbolDBPlugin *sdb_plugin)
{
	GList * prj_elements_list;
	IAnjutaProjectManager *pm;
	gboolean parsed = NULL;
	GPtrArray *to_remove_files = NULL;
	
	pm = anjuta_shell_get_interface (ANJUTA_PLUGIN (sdb_plugin)->shell,
									 IAnjutaProjectManager, NULL);	

	prj_elements_list = ianjuta_project_manager_get_elements (pm,
		   IANJUTA_PROJECT_MANAGER_SOURCE,
		   NULL);				
	
	/* some files may have added/removed editing Makefile.am while
	 * Anjuta was offline. Check this case too.
	 */
	SymbolDBEngineIterator *it = 
		symbol_db_engine_get_files_for_project (sdb_plugin->sdbe_project, 
												NULL,
												SYMINFO_FILE_PATH);
	
	/* files eventually removed to the project with Anjuta offline */
	to_remove_files = NULL;
	
	/* something between O(n^2) and O(n log n). Totally inefficient,
	 * in particular for big-sized projects. Are we really sure we want 
	 * this? The only hope is that both the lists come ordered.
	 */
	if (it != NULL && symbol_db_engine_iterator_get_n_items (it) > 0)
	{
		parsed = TRUE;
		do {
			SymbolDBEngineIteratorNode *dbin;
			GList *link;
			gchar *full_path;
			gchar *full_uri;
		
			dbin = (SymbolDBEngineIteratorNode *) it;
			
			const gchar * db_file = 
				symbol_db_engine_iterator_node_get_symbol_extra_string (dbin,
													SYMINFO_FILE_PATH);
			full_path = symbol_db_engine_get_full_local_path (sdb_plugin->sdbe_project,
												  db_file);
			full_uri = gnome_vfs_get_uri_from_local_path (full_path);
			
			if ((link = g_list_find_custom (prj_elements_list, full_uri,
									g_list_compare)) == NULL) 
			{
				if (to_remove_files == NULL)
					to_remove_files = g_ptr_array_new ();
				
				DEBUG_PRINT ("to_remove_files, added %s", full_path);
				g_ptr_array_add (to_remove_files, full_path);						
				/* no need to free full_path now */				
			} 
			else
			{
				/* go and remove the entry from the project_entries, so 
				 * to skip an iteration next time */
				prj_elements_list = g_list_delete_link (prj_elements_list, 
												   link);
				DEBUG_PRINT ("deleted, removed %s. Still length %d", full_path,
							 g_list_length (prj_elements_list));
				g_free (full_path);
			}
			g_free (full_uri);
		} while (symbol_db_engine_iterator_move_next (it));
	}	
				
	/* good, in prj_elements_list we'll have the files to add 
	 * from project */
	if (to_remove_files != NULL)
	{		
		DEBUG_PRINT ("if (to_remove_files != NULL)");
		symbol_db_engine_remove_files (sdb_plugin->sdbe_project,
									   sdb_plugin->project_opened,
									   to_remove_files);
		g_ptr_array_foreach (to_remove_files, (GFunc)g_free, NULL);
		g_ptr_array_free (to_remove_files, TRUE);
		to_remove_files = NULL;
	}
	
	/* add those left files */
	if (prj_elements_list != NULL && it != NULL && parsed == TRUE)
	{
		DEBUG_PRINT ("if (prj_elements_list != NULL)");
		GPtrArray *to_add_files = g_ptr_array_new ();
		GPtrArray *languages_array = g_ptr_array_new();
		GList *item = prj_elements_list;														
		IAnjutaLanguage* lang_manager;	
	
		lang_manager = anjuta_shell_get_interface (ANJUTA_PLUGIN(sdb_plugin)->shell, 
										IAnjutaLanguage, NULL);	
	
		while (item != NULL)
		{					
			const gchar* lang;
			gchar *full_path;
			IAnjutaLanguageId lang_id;	
			const gchar *file_mime;

			full_path = gnome_vfs_get_local_path_from_uri (item->data);			
			file_mime = gnome_vfs_get_mime_type_for_name (full_path);
			lang_id = ianjuta_language_get_from_mime_type (lang_manager, 
								file_mime, NULL);								
			/* No supported language... */
			if (!lang_id)
			{
				g_free (full_path);
				continue;
			}						
			lang = ianjuta_language_get_name (lang_manager, lang_id, NULL);
			g_ptr_array_add (languages_array, g_strdup (lang));
			

			g_ptr_array_add (to_add_files, full_path);
				
			item = item->next;
		}
			
		symbol_db_engine_add_new_files (sdb_plugin->sdbe_project,
						sdb_plugin->project_opened,
						to_add_files,
						languages_array,
						TRUE);					
			
		g_ptr_array_foreach (languages_array, (GFunc)g_free, NULL);
		g_ptr_array_free (languages_array, TRUE);
			
		g_ptr_array_foreach (to_add_files, (GFunc)g_free, NULL);
		g_ptr_array_free (to_add_files, TRUE);	
	}
}
#endif

/* add a new project */
static void
on_project_root_added (AnjutaPlugin *plugin, const gchar *name,
					const GValue *value, gpointer user_data)
{
	IAnjutaProjectManager *pm;
	SymbolDBPlugin *sdb_plugin;
	const gchar *root_uri;
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (plugin);

	/*
	 *   The Globals thing
	 */
	
	/* hide it. Default */
	/* system tags thing: we'll import after abort even if the preferences says not
	 * to automatically scan the packages.
	 */
	gtk_widget_hide (sdb_plugin->progress_bar_system);
	
	/* get preferences about the parallel scan */
	gboolean parallel_scan = anjuta_preferences_get_int (sdb_plugin->prefs, 
														 PARALLEL_SCAN); 
	
	if (parallel_scan == TRUE)
	{
		/* we simulate a project-import-end signal received */
		on_received_project_import_end (sdb_plugin, sdb_plugin);		
	}
	else
	{
		g_signal_connect (G_OBJECT (sdb_plugin), 
						"project-import-end", 
						  G_CALLBACK (on_received_project_import_end), 
						  sdb_plugin);
	}
		
	/*
	 *   The Project thing
	 */
	pm = anjuta_shell_get_interface (ANJUTA_PLUGIN (sdb_plugin)->shell,
									 IAnjutaProjectManager, NULL);	
		
	g_free (sdb_plugin->project_root_uri);
	sdb_plugin->project_root_uri = NULL;
	root_uri = g_value_get_string (value);
	
	if (root_uri)
	{
		gchar *root_dir = gnome_vfs_get_local_path_from_uri (root_uri);
		DEBUG_PRINT ("Symbol-DB: added project root_dir %s, name %s", root_dir, 
					 name);
		
		/* FIXME: where's the project name itself? */
		DEBUG_PRINT ("FIXME: where's the project name itself? ");
		sdb_plugin->project_opened = g_strdup (root_dir);
		DEBUG_PRINT ("FIXME: setting project opened to  %s", root_dir);
		
		if (root_dir)
		{
			gboolean needs_sources_scan = FALSE;
			gboolean project_exist = FALSE;
			GHashTable* lang_hash;
			guint id;
				
			lang_hash = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, 
											  sources_array_free);

			/* is it a fresh-new project? is it an imported project with 
			 * no 'new' symbol-db database but the 'old' one symbol-browser? 
			 */
			if (symbol_db_engine_db_exists (sdb_plugin->sdbe_project, 
											root_dir) == FALSE)
			{
				DEBUG_PRINT ("Symbol-DB: project does not exist");
				needs_sources_scan = TRUE;
				project_exist = FALSE;
			}
			else 
			{
				project_exist = TRUE;
			}

			/* we'll use the same values for db_directory and project_directory */
			DEBUG_PRINT ("opening db %s and project_dir %s", root_dir, root_dir);
			if (symbol_db_engine_open_db (sdb_plugin->sdbe_project, root_dir, 
										  root_dir) == FALSE)
			{
				g_error ("Symbol-DB: error in opening db");
			}

			/* if project did not exist add a new project */
			if (project_exist == FALSE)
			{
				DEBUG_PRINT ("Symbol-DB: creating new project.");
				symbol_db_engine_add_new_project (sdb_plugin->sdbe_project,
												  NULL,	/* still no workspace logic */
												  sdb_plugin->project_opened);
			}

			/*
			 * we need an initial import 
			 */
			if (needs_sources_scan == TRUE)
			{
				DEBUG_PRINT ("Symbol-DB: importing sources...");
				do_import_project_sources (plugin, pm, root_dir);
			}
			else	
			{
				/*
				 * no import needed. But we may have aborted the scan of sources ..
				 */
				
				GPtrArray *sources_array = NULL;
				
				sources_array = 
					symbol_db_engine_get_files_with_zero_symbols (sdb_plugin->sdbe_project);

				if (sources_array != NULL && sources_array->len > 0) 
				{
					/* 
					 * if we're importing first shut off the signal receiving.
	 				 * We'll re-enable that on scan-end 
	 				 */
					symbol_db_view_locals_recv_signals_from_engine (																
						SYMBOL_DB_VIEW_LOCALS (sdb_plugin->dbv_view_tree_locals), 
								 sdb_plugin->sdbe_project, FALSE);

					symbol_db_view_recv_signals_from_engine (																
						SYMBOL_DB_VIEW (sdb_plugin->dbv_view_tree), 
								 sdb_plugin->sdbe_project, FALSE);
				
					g_signal_connect (G_OBJECT (sdb_plugin->sdbe_project), 
						"scan-end", G_CALLBACK (on_importing_project_end), plugin);				
					
					do_import_project_src_after_abort (plugin, sources_array);
					
					g_ptr_array_foreach (sources_array, (GFunc)g_free, NULL);
					g_ptr_array_free (sources_array, TRUE);
				}

				
				/* check for offline changes */
				/* FIXME */
				/*do_check_offline_files_changed (sdb_plugin);*/
				
				
				/* Update the symbols */
				symbol_db_engine_update_project_symbols (sdb_plugin->sdbe_project, 
														 root_dir);				
			}
			gtk_progress_bar_set_text (GTK_PROGRESS_BAR (sdb_plugin->progress_bar_project),
									   _("Populating symbols' db..."));
			id = g_idle_add ((GSourceFunc) gtk_progress_bar_pulse, 
							 sdb_plugin->progress_bar_project);
			gtk_widget_show (sdb_plugin->progress_bar_project);
			
			symbol_db_view_open (SYMBOL_DB_VIEW (sdb_plugin->dbv_view_tree),
								 sdb_plugin->sdbe_project);
			g_source_remove (id);
			gtk_widget_hide (sdb_plugin->progress_bar_project);

			/* root dir */
			sdb_plugin->project_root_dir = root_dir;
			
			g_hash_table_unref (lang_hash);			
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
on_project_root_removed (AnjutaPlugin *plugin, const gchar *name,
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
	symbol_db_engine_close_db (sdb_plugin->sdbe_project);
	
	/* and the globals one */
	symbol_db_engine_close_db (sdb_plugin->sdbe_globals);

	g_free (sdb_plugin->project_root_uri);
	g_free (sdb_plugin->project_root_dir);
	g_free (sdb_plugin->project_opened);
	sdb_plugin->project_root_uri = NULL;
	sdb_plugin->project_root_dir = NULL;	
	sdb_plugin->project_opened = NULL;
}

static gboolean
symbol_db_activate (AnjutaPlugin *plugin)
{
	SymbolDBPlugin *symbol_db;
	gchar *anjuta_cache_path;
	gchar *ctags_path;
	
	DEBUG_PRINT ("SymbolDBPlugin: Activating SymbolDBPlugin plugin ...");
	
	/* Initialize gda library. */
	gda_init ();

	register_stock_icons (plugin);

	symbol_db = ANJUTA_PLUGIN_SYMBOL_DB (plugin);
	symbol_db->ui = anjuta_shell_get_ui (plugin->shell, NULL);
	symbol_db->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	symbol_db->project_opened = NULL;

	ctags_path = anjuta_preferences_get (symbol_db->prefs, CTAGS_PREFS_KEY); 

	if (ctags_path == NULL) 
	{
		g_warning ("ctags is not in preferences. Trying a default one %s", 
				   CTAGS_PATH);
		ctags_path = g_strdup (CTAGS_PATH);
	}
	
	/* initialize the session packages to NULL. We'll store there the user 
	 * preferences for the session about global-system packages 
	 */
	symbol_db->session_packages = NULL;
	
	symbol_db->buf_update_timeout_id = 0;
	symbol_db->need_symbols_update = FALSE;
	/* creates and start a new timer. */
	symbol_db->update_timer = g_timer_new ();

	
	DEBUG_PRINT ("SymbolDBPlugin: Initializing engines with %s", ctags_path);
	/* create SymbolDBEngine(s) */
	symbol_db->sdbe_project = symbol_db_engine_new (ctags_path);
	if (symbol_db->sdbe_project == NULL)
	{		
		g_critical ("sdbe_project == NULL");
		return FALSE;
	}
		
	/* the globals one too */
	symbol_db->sdbe_globals = symbol_db_engine_new (ctags_path);
	if (symbol_db->sdbe_globals == NULL)
	{		
		g_critical ("sdbe_globals == NULL");
		return FALSE;
	}
	
	g_free (ctags_path);
	
	/* open it */
	anjuta_cache_path = anjuta_util_get_user_cache_file_path (".", NULL);
	symbol_db_engine_open_db (symbol_db->sdbe_globals, 
							  anjuta_cache_path, 
							  PROJECT_GLOBALS);

	g_free (anjuta_cache_path);
	
	/* create the object that'll manage the globals population */
	symbol_db->sdbs = symbol_db_system_new (symbol_db, symbol_db->sdbe_globals);

	g_signal_connect (G_OBJECT (symbol_db->sdbs), "scan-package-start",
					  G_CALLBACK (on_system_scan_package_start), plugin);	
	
	g_signal_connect (G_OBJECT (symbol_db->sdbs), "scan-package-end",
					  G_CALLBACK (on_system_scan_package_end), plugin);	
	
	g_signal_connect (G_OBJECT (symbol_db->sdbs), "single-file-scan-end",
					  G_CALLBACK (on_system_single_file_scan_end), plugin);	
	
	/* sets preferences to NULL, it'll be instantiated when required.\ */
	symbol_db->sdbp = NULL;
	
	/* Create widgets */
	symbol_db->dbv_main = gtk_vbox_new(FALSE, 5);
	symbol_db->dbv_notebook = gtk_notebook_new();
	symbol_db->progress_bar_project = gtk_progress_bar_new();
	gtk_progress_bar_set_ellipsize (GTK_PROGRESS_BAR(symbol_db->progress_bar_project),
									PANGO_ELLIPSIZE_END);
	g_object_ref (symbol_db->progress_bar_project);
	
	symbol_db->progress_bar_system = gtk_progress_bar_new();
	gtk_progress_bar_set_ellipsize (GTK_PROGRESS_BAR(symbol_db->progress_bar_system),
									PANGO_ELLIPSIZE_END);
	
	g_object_ref (symbol_db->progress_bar_system);
		
	gtk_box_pack_start (GTK_BOX (symbol_db->dbv_main), symbol_db->dbv_notebook,
						TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (symbol_db->dbv_main), symbol_db->progress_bar_project,
						FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (symbol_db->dbv_main), symbol_db->progress_bar_system,
						FALSE, FALSE, 0);	
	gtk_widget_show_all (symbol_db->dbv_main);
	
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
											 symbol_db->sdbe_project, TRUE);										 

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
											 symbol_db->sdbe_project, TRUE);										 

	g_signal_connect (G_OBJECT (symbol_db->dbv_view_tree), "row-activated",
					  G_CALLBACK (on_global_treeview_row_activated), plugin);

	g_signal_connect (G_OBJECT (symbol_db->dbv_view_tree), "row-expanded",
					  G_CALLBACK (on_global_treeview_row_expanded), plugin);

	g_signal_connect (G_OBJECT (symbol_db->dbv_view_tree), "row-collapsed",
					  G_CALLBACK (on_global_treeview_row_collapsed), plugin);	
	
	gtk_container_add (GTK_CONTAINER(symbol_db->scrolled_global), 
					   symbol_db->dbv_view_tree);
	
	/* Search symbols */
	symbol_db->dbv_view_tree_search =
		(GtkWidget*) symbol_db_view_search_new (symbol_db->sdbe_project);
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
		anjuta_plugin_add_watch (plugin, IANJUTA_DOCUMENT_MANAGER_CURRENT_DOCUMENT,
								 value_added_current_editor,
								 value_removed_current_editor, NULL);
	/* Added widgets */
	anjuta_shell_add_widget (plugin->shell, symbol_db->dbv_main,
							 "AnjutaSymbolDB", _("Symbols"),
							 "symbol-db-plugin-icon",
							 ANJUTA_SHELL_PLACEMENT_LEFT, NULL);	

	/* set up project directory watch */
	symbol_db->root_watch_id = anjuta_plugin_add_watch (plugin,
									IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI,
									on_project_root_added,
									on_project_root_removed, NULL);
	
	/* Determine session state */
	g_signal_connect (plugin->shell, "load-session", 
					  G_CALLBACK (on_session_load), plugin);
	
	g_signal_connect (plugin->shell, "save-session", 
					  G_CALLBACK (on_session_save), plugin);
		
	return TRUE;
}

static gboolean
symbol_db_deactivate (AnjutaPlugin *plugin)
{
	SymbolDBPlugin *sdb_plugin;
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (plugin);

	DEBUG_PRINT ("SymbolDBPlugin: Dectivating SymbolDBPlugin plugin ...");
	
	/* disconnect some signals */
	g_signal_handlers_disconnect_by_func (G_OBJECT (sdb_plugin->dbv_view_tree_locals),
									  on_local_treeview_row_activated,
									  plugin);

	g_signal_handlers_disconnect_by_func (G_OBJECT (sdb_plugin->dbv_view_tree_search),
									  on_treesearch_symbol_selected_event,
									  plugin);
	
	g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->shell),
										  on_session_load,
										  plugin);

	g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->shell),
										  on_session_save,
										  plugin);

	g_signal_handlers_disconnect_by_func (G_OBJECT (sdb_plugin->dbv_view_tree),
										  on_global_treeview_row_activated,
										  plugin);
	
	g_signal_handlers_disconnect_by_func (G_OBJECT (sdb_plugin->dbv_view_tree),
										  on_global_treeview_row_expanded,
										  plugin);

	g_signal_handlers_disconnect_by_func (G_OBJECT (sdb_plugin->dbv_view_tree),
										  on_global_treeview_row_collapsed,
										  plugin);

	g_signal_handlers_disconnect_by_func (G_OBJECT (sdb_plugin->dbv_view_tree_locals),
										  on_local_treeview_row_activated,
										  plugin);

	g_signal_handlers_disconnect_by_func (G_OBJECT (sdb_plugin->sdbs),
										  on_system_scan_package_start,
										  plugin);

	g_signal_handlers_disconnect_by_func (G_OBJECT (sdb_plugin->sdbs),
										  on_system_scan_package_end,
										  plugin);
	
	g_signal_handlers_disconnect_by_func (G_OBJECT (sdb_plugin->sdbs),										  
										  on_system_single_file_scan_end,
										  plugin);
	
	if (sdb_plugin->update_timer)
	{
		g_timer_destroy (sdb_plugin->update_timer);
		sdb_plugin->update_timer = NULL;
	}

	/* destroy objects */
	if (sdb_plugin->sdbe_project)
		g_object_unref (sdb_plugin->sdbe_project);
	sdb_plugin->sdbe_project = NULL;

	/* this must be done *bedore* destroying sdbe_globals */
	g_object_unref (sdb_plugin->sdbs);
	sdb_plugin->sdbs = NULL;
	
	g_free (sdb_plugin->current_scanned_package);
	sdb_plugin->current_scanned_package = NULL;
	
	g_object_unref (sdb_plugin->sdbe_globals);
	sdb_plugin->sdbe_globals = NULL;
	
	g_free (sdb_plugin->project_opened);
	sdb_plugin->project_opened = NULL;

	if (sdb_plugin->session_packages)
	{
		g_list_foreach (sdb_plugin->session_packages, (GFunc)g_free, NULL);
		g_list_free (sdb_plugin->session_packages);
		sdb_plugin->session_packages = NULL;
	}
	
	/* Ensure all editor cached info are released */
	if (sdb_plugin->editor_connected)
	{
		g_hash_table_foreach (sdb_plugin->editor_connected,
							  on_editor_foreach_disconnect, plugin);
		g_hash_table_destroy (sdb_plugin->editor_connected);
		sdb_plugin->editor_connected = NULL;
	}
	
	/* Remove watches */
	anjuta_plugin_remove_watch (plugin, sdb_plugin->root_watch_id, FALSE);
	anjuta_plugin_remove_watch (plugin, sdb_plugin->editor_watch_id, TRUE);
	
	/* Remove widgets: Widgets will be destroyed when dbv_main is removed */
	g_object_unref (sdb_plugin->progress_bar_project);
	g_object_unref (sdb_plugin->progress_bar_system);
	anjuta_shell_remove_widget (plugin->shell, sdb_plugin->dbv_main, NULL);

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
	sdb_plugin->progress_bar_project = NULL;
	sdb_plugin->progress_bar_system = NULL;
	return TRUE;
}

static void
symbol_db_finalize (GObject *obj)
{
	DEBUG_PRINT ("Symbol-DB finalize");
	/* Finalization codes here */
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
symbol_db_dispose (GObject *obj)
{
	/* Disposition codes */
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
symbol_db_instance_init (GObject *obj)
{
	SymbolDBPlugin *plugin = (SymbolDBPlugin*)obj;

	plugin->files_count_project_done = 0;
	plugin->files_count_project = 0;
	
	plugin->files_count_system_done = 0;
	plugin->files_count_system = 0;	
	plugin->current_scanned_package = NULL;
}

static void
symbol_db_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = symbol_db_activate;
	plugin_class->deactivate = symbol_db_deactivate;
	klass->finalize = symbol_db_finalize;
	klass->dispose = symbol_db_dispose;
	
	signals[PROJECT_IMPORT_END]
		= g_signal_new ("project-import-end",
						G_OBJECT_CLASS_TYPE (object_class),
						G_SIGNAL_RUN_FIRST,
						G_STRUCT_OFFSET (SymbolDBPluginClass, project_import_end),
						NULL, NULL,
						g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	
	signals[GLOBALS_IMPORT_END]
		= g_signal_new ("globals-import-end",
						G_OBJECT_CLASS_TYPE (object_class),
						G_SIGNAL_RUN_FIRST,
						G_STRUCT_OFFSET (SymbolDBPluginClass, globals_import_end),
						NULL, NULL,
						g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);	
}

static IAnjutaIterable*
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
		DEBUG_PRINT ("filter_array is NULL");
	}
	else 
	{
		filter_array = symbol_db_engine_fill_type_array (match_types);
		DEBUG_PRINT ("filter_array filled with %d kinds", filter_array->len);
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
	//*/
	DEBUG_PRINT ("tags scan [%s] [exact_match %d] [global %d]", pattern, 
					 exact_match, global_symbols_search);
	{
		gint i;
		if (filter_array)
			for (i = 0; i < filter_array->len; i++)
			{
				DEBUG_PRINT ("filter_array for type [%d] %s", i, 
							 (gchar*)g_ptr_array_index (filter_array,
							i));
			}
	}
	//*/
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

static IAnjutaIterable*
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

static IAnjutaIterable*
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

static IAnjutaIterable*
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

static IAnjutaIterable*
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

static IAnjutaIterable*
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

static IAnjutaSymbol*
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

static void
isymbol_manager_iface_init (IAnjutaSymbolManagerIface *iface)
{
	iface->search = isymbol_manager_search;
	iface->get_members = isymbol_manager_get_members;
	iface->get_class_parents = isymbol_manager_get_class_parents;
	iface->get_scope = isymbol_manager_get_scope;
	iface->get_parent_scope = isymbol_manager_get_parent_scope;
	iface->get_symbol_more_info = isymbol_manager_get_symbol_more_info;
	iface->get_symbol_by_id = isymbol_manager_get_symbol_by_id;
}


static void
on_prefs_package_add (SymbolDBPrefs *sdbp, const gchar *package, 
							  gpointer user_data)
{
	SymbolDBPlugin *sdb_plugin;
	
	g_return_if_fail (package != NULL);
	
	DEBUG_PRINT ("on_prefs_package_add");
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (user_data);
	
	sdb_plugin->session_packages = g_list_prepend (sdb_plugin->session_packages,
												   g_strdup (package));	
}

static void
on_prefs_package_remove (SymbolDBPrefs *sdbp, const gchar *package, 
							  gpointer user_data)
{
	SymbolDBPlugin *sdb_plugin;
	
	g_return_if_fail (package != NULL);
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (user_data);
	
	GList *item;
	DEBUG_PRINT ("on_prefs_package_remove");	
	if ((item = g_list_find_custom (sdb_plugin->session_packages, package, 
							g_list_compare)) != NULL)
	{
		sdb_plugin->session_packages = g_list_remove_link (sdb_plugin->session_packages,
														   item);
		
		/* ok, now think to the item left alone by its friends... */
		g_list_foreach (item, (GFunc)g_free, NULL);
		g_list_free (item);
	}
}

static void
on_prefs_buffer_update_toggled (SymbolDBPrefs *sdbp, guint value, 
							  gpointer user_data)
{
	SymbolDBPlugin *sdb_plugin;
	DEBUG_PRINT ("on_prefs_buffer_update_toggled () %d", value);
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (user_data);
	
	if (value == FALSE)
	{
		if (sdb_plugin->buf_update_timeout_id)
			g_source_remove (sdb_plugin->buf_update_timeout_id);
		sdb_plugin->buf_update_timeout_id = 0;	
	}
	else 
	{
		if (sdb_plugin->buf_update_timeout_id == 0)
			sdb_plugin->buf_update_timeout_id = 
				g_timeout_add (TIMEOUT_INTERVAL_SYMBOLS_UPDATE,
								on_editor_buffer_symbols_update_timeout,
								sdb_plugin);			
		
	}
	
}

static void
ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	DEBUG_PRINT ("SymbolDB: ipreferences_merge");	
	SymbolDBPlugin *sdb_plugin;
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (ipref);
	
	if (sdb_plugin->sdbp == NULL)
	{
		sdb_plugin->sdbp = symbol_db_prefs_new (sdb_plugin->sdbs, 
												sdb_plugin->sdbe_project,
												sdb_plugin->sdbe_globals,
												prefs,
												sdb_plugin->session_packages);
		
		/* connect the signals to retrieve package modifications */
		g_signal_connect (G_OBJECT (sdb_plugin->sdbp), "package-add",
						  G_CALLBACK (on_prefs_package_add),
						  sdb_plugin);
		g_signal_connect (G_OBJECT (sdb_plugin->sdbp), "package-remove",
						  G_CALLBACK (on_prefs_package_remove),
						  sdb_plugin);		
		g_signal_connect (G_OBJECT (sdb_plugin->sdbp), "buffer-update-toggled",
						  G_CALLBACK (on_prefs_buffer_update_toggled),
						  sdb_plugin);				
	}
}

static void
ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	SymbolDBPlugin *sdb_plugin;
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (ipref);
	
	if (sdb_plugin->sdbp != NULL)
	{
		g_object_unref (sdb_plugin->sdbp);
		sdb_plugin->sdbp = NULL;
	}
}

static void
ipreferences_iface_init(IAnjutaPreferencesIface* iface)
{
	DEBUG_PRINT ("SymbolDB: ipreferences_iface_init");	
	iface->merge = ipreferences_merge;
	iface->unmerge = ipreferences_unmerge;	
}


ANJUTA_PLUGIN_BEGIN (SymbolDBPlugin, symbol_db);
ANJUTA_PLUGIN_ADD_INTERFACE (isymbol_manager, IANJUTA_TYPE_SYMBOL_MANAGER);
ANJUTA_PLUGIN_ADD_INTERFACE(ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (SymbolDBPlugin, symbol_db);
