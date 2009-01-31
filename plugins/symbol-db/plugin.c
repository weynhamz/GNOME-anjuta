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
#include <gio/gio.h>
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
#include "symbol-db-prefs.h"

#define ICON_FILE "anjuta-symbol-db-plugin-48.png"
#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-symbol-db-plugin.ui"

#define TIMEOUT_INTERVAL_SYMBOLS_UPDATE		10
#define TIMEOUT_SECONDS_AFTER_LAST_TIP		5

#define PROJECT_GLOBALS		"/"
#define SESSION_SECTION		"SymbolDB"
#define SESSION_KEY			"SystemPackages"

#define ANJUTA_PIXMAP_GOTO_DECLARATION		"element-interface"
#define ANJUTA_PIXMAP_GOTO_IMPLEMENTATION	"element-method"

#define ANJUTA_STOCK_GOTO_DECLARATION		"element-interface"
#define ANJUTA_STOCK_GOTO_IMPLEMENTATION	"element-method"

static gpointer parent_class;

/* signals */
enum
{
	PROJECT_IMPORT_END,
	GLOBALS_IMPORT_END,
	LAST_SIGNAL 
};

typedef enum 
{
	TASK_IMPORT_PROJECT = 1,
	TASK_IMPORT_PROJECT_AFTER_ABORT,
	TASK_BUFFER_UPDATE,
	TASK_ELEMENT_ADDED,
	TASK_OFFLINE_CHANGES,
	TASK_PROJECT_UPDATE,
	TASK_FILE_UPDATE
} ProcTask;

static unsigned int signals[LAST_SIGNAL] = { 0 };

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
	REGISTER_ICON_FULL (ANJUTA_PIXMAP_GOTO_DECLARATION, ANJUTA_STOCK_GOTO_DECLARATION);
	REGISTER_ICON_FULL (ANJUTA_PIXMAP_GOTO_IMPLEMENTATION, ANJUTA_STOCK_GOTO_IMPLEMENTATION);
	END_REGISTER_ICON;
}

static void
goto_file_line (AnjutaPlugin *plugin, const gchar *filename, gint lineno)
{
	IAnjutaDocumentManager *docman;
	GFile* file;
	
	g_return_if_fail (filename != NULL);
		
	DEBUG_PRINT ("going to: file %s, line %d", filename, lineno);
	
	/* Go to file and line number */
	docman = anjuta_shell_get_interface (plugin->shell, IAnjutaDocumentManager,
										 NULL);
	file = g_file_new_for_path (filename);
	ianjuta_document_manager_goto_file_line (docman, file, lineno, NULL);
	
	g_object_unref (file);
}

/* Find an implementation (if impl == TRUE) or declaration (if impl == FALSE)
 * from the given symbol iterator.
 * If current_document != NULL it prefers matches from the currently open document
 */
static gchar *
find_file_line (SymbolDBEngineIterator *iterator, gboolean impl, const gchar *current_document,
				gint *line)
{
	gchar *path = NULL;
	gint _line = -1;
	
	do
	{
		const gchar *symbol_kind;
		gboolean is_decl;		
		SymbolDBEngineIteratorNode *iter_node =
			SYMBOL_DB_ENGINE_ITERATOR_NODE (iterator);
		
		if (iter_node == NULL)
		{
			/* not found or some error occurred */
			break;  
		}
		
		symbol_kind = symbol_db_engine_iterator_node_get_symbol_extra_string (
																			  iter_node, SYMINFO_KIND);				
		is_decl = g_strcmp0 (symbol_kind, "prototype") == 0 || 
			g_strcmp0 (symbol_kind, "interface") == 0;
		
		if (is_decl == !impl) 
		{
			const gchar *_path;
			_path = symbol_db_engine_iterator_node_get_symbol_extra_string (iter_node,
																			SYMINFO_FILE_PATH);
			/* if the path matches the current document we return immidiately */
			if (!current_document || g_strcmp0 (_path, current_document) == 0)
			{
				*line = symbol_db_engine_iterator_node_get_symbol_file_pos (iter_node);
				g_free (path);
				
				return g_strdup (_path);
			}
			/* we store the first match incase there is no match against the current document */
			else if (_line == -1)
			{
				path = g_strdup (_path);
				_line = symbol_db_engine_iterator_node_get_symbol_file_pos (iter_node);
			}
		}
	} while (symbol_db_engine_iterator_move_next (iterator) == TRUE);
	
	if (_line != -1)
		*line = _line;
	
	return path;	
}

static void
goto_file_tag (SymbolDBPlugin *sdb_plugin, const gchar *word,
			   gboolean prefer_implementation)
{
	SymbolDBEngineIterator *iterator;	
	gchar *path = NULL;
	gint line;
	
	iterator = symbol_db_engine_find_symbol_by_name_pattern (sdb_plugin->sdbe_project, 
															 word,
															 TRUE,
															 SYMINFO_SIMPLE |
															 SYMINFO_KIND |
															 SYMINFO_FILE_PATH);
	
	if (iterator != NULL && symbol_db_engine_iterator_get_n_items (iterator) > 0)
	{
		gchar *current_document = NULL;
		/* FIXME: namespaces are not handled here, but they should. */
		
		if (IANJUTA_IS_FILE (sdb_plugin->current_editor))
		{
			GFile *file;
			
			if ((file = ianjuta_file_get_file (IANJUTA_FILE (sdb_plugin->current_editor),
											   NULL)))
			{
				current_document = g_file_get_path (file);
				g_object_unref (file);
			}
		}
		
		path = find_file_line (iterator, prefer_implementation, current_document, &line);
		if (!path)
		{
			/* reset iterator */
			symbol_db_engine_iterator_first (iterator);   
			path = find_file_line (iterator, !prefer_implementation, current_document,
								   &line);
		}
		
		if (path)
		{
			goto_file_line (ANJUTA_PLUGIN (sdb_plugin), path, line);
			g_free (path);
		}
		
		g_free (current_document);
	}
	
	if (iterator)
		g_object_unref (iterator);
}

static void
on_goto_file_tag_impl_activate (GtkAction *action, SymbolDBPlugin *sdb_plugin)
{
	IAnjutaEditor *ed;
	gchar *word;

	if (sdb_plugin->current_editor)
	{
		ed = IANJUTA_EDITOR (sdb_plugin->current_editor);
		word = ianjuta_editor_get_current_word (ed, NULL);
		if (word)
		{
			goto_file_tag (sdb_plugin, word, TRUE);
			g_free (word);
		}
	}
}

static void
on_goto_file_tag_decl_activate (GtkAction *action, SymbolDBPlugin *sdb_plugin)
{
	IAnjutaEditor *ed;
	gchar *word;

	if (sdb_plugin->current_editor)
	{
		ed = IANJUTA_EDITOR (sdb_plugin->current_editor);
		word = ianjuta_editor_get_current_word (ed, NULL);
		if (word)
		{
			goto_file_tag (sdb_plugin, word, FALSE);
			g_free (word);
		}
	}
}

static void
on_find_symbol (GtkAction *action, SymbolDBPlugin *sdb_plugin)
{
	DEBUG_PRINT ("on_find_symbol (GtkAction *action, gpointer user_data)");
	GtkEntry * entry;
	
	anjuta_shell_present_widget(ANJUTA_PLUGIN(sdb_plugin)->shell,
								sdb_plugin->dbv_main, NULL);
	
	entry = symbol_db_view_search_get_entry ( 
					SYMBOL_DB_VIEW_SEARCH (sdb_plugin->dbv_view_tree_search));
	gtk_notebook_set_current_page (GTK_NOTEBOOK(sdb_plugin->dbv_notebook), 2);
	gtk_widget_grab_focus (GTK_WIDGET (entry));
}

static GtkActionEntry actions[] = 
{
	{ "ActionMenuGoto", NULL, N_("_Goto"), NULL, NULL, NULL},
	{
		"ActionSymbolDBGotoDecl",
		ANJUTA_STOCK_GOTO_DECLARATION,
		N_("Tag De_claration"),
		"<shift><control>d",
		N_("Goto symbol declaration"),
		G_CALLBACK (on_goto_file_tag_decl_activate)
	},
	{
		"ActionSymbolDBGotoImpl",
		ANJUTA_STOCK_GOTO_IMPLEMENTATION,
		N_("Tag _Implementation"),
		"<control>d",
		N_("Goto symbol definition"),
		G_CALLBACK (on_goto_file_tag_impl_activate)
	}	
};

static GtkActionEntry actions_search[] = {
  { "ActionEditSearchFindSymbol", GTK_STOCK_FIND, N_("_Find Symbol..."),
	"<control>l", N_("Find Symbol"),
    G_CALLBACK (on_find_symbol)}
};

static void
enable_view_signals (SymbolDBPlugin *sdb_plugin, gboolean enable, gboolean force)
{
	if ((sdb_plugin->is_offline_scanning == FALSE && 
		sdb_plugin->is_project_importing == FALSE &&
		sdb_plugin->is_project_updating == FALSE &&
		sdb_plugin->is_adding_element == FALSE) || force == TRUE)
	{
		symbol_db_view_locals_recv_signals_from_engine (																
				SYMBOL_DB_VIEW_LOCALS (sdb_plugin->dbv_view_tree_locals), 
								 sdb_plugin->sdbe_project, enable);		
			
		symbol_db_view_recv_signals_from_engine (
				SYMBOL_DB_VIEW(sdb_plugin->dbv_view_tree), 
								 sdb_plugin->sdbe_project, enable);
	}
}

static void
on_editor_buffer_symbol_update_scan_end (SymbolDBEngine *dbe, gint process_id, 
										  gpointer data)
{
	SymbolDBPlugin *sdb_plugin;
	gint i;
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (data);	
	
	/* search for the proc id */
	for (i = 0; i < sdb_plugin->buffer_update_ids->len; i++)
	{
		if (g_ptr_array_index (sdb_plugin->buffer_update_ids, i) == GINT_TO_POINTER (process_id))
		{
			gchar *str;
			/* hey we found it */
			/* remove both the items */
			g_ptr_array_remove_index (sdb_plugin->buffer_update_ids, i);
			
			str = (gchar*)g_ptr_array_remove_index (sdb_plugin->buffer_update_files, 
													i);
			/* we can now free it */
			g_free (str);			
		}
	}
}

static gboolean
on_editor_buffer_symbols_update_timeout (gpointer user_data)
{
	SymbolDBPlugin *sdb_plugin;
	IAnjutaEditor *ed;
	gchar *current_buffer = NULL;
	gsize buffer_size = 0;
	gdouble seconds_elapsed;
	GFile* file;
	gchar * local_path;
	GPtrArray *real_files_list;
	GPtrArray *text_buffers;
	GPtrArray *buffer_sizes;
	gint i;
	
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

	if (seconds_elapsed < TIMEOUT_SECONDS_AFTER_LAST_TIP)
		return TRUE;
		
		
	 /* we won't proceed with the updating of the symbols if we didn't type in 
	 	anything */
	 if (sdb_plugin->need_symbols_update == FALSE)
	 	return TRUE;
	
	if (sdb_plugin->current_editor) 
	{
		ed = IANJUTA_EDITOR (sdb_plugin->current_editor);
		
		buffer_size = ianjuta_editor_get_length (ed, NULL);
		current_buffer = ianjuta_editor_get_text_all (ed, NULL);
				
		file = ianjuta_file_get_file (IANJUTA_FILE (ed), NULL);
	} 
	else
		return FALSE;

	if (file == NULL)
		return FALSE;
	
	/* take the path reference */
	local_path = g_file_get_path (file);
	
	/* ok that's good. Let's have a last check: is the current file present
	 * on the buffer_update_files?
	 */
	for (i = 0; i < sdb_plugin->buffer_update_files->len; i++)
	{
		if (g_strcmp0 (g_ptr_array_index (sdb_plugin->buffer_update_files, i),
					 local_path) == 0)
		{
			/* hey we found it */
			/* something is already scanning this buffer file. Drop the procedure now. */
			DEBUG_PRINT ("something is already scanning the file %s", local_path);
			return FALSE;			
		}
	}

	real_files_list = g_ptr_array_new ();
	g_ptr_array_add (real_files_list, local_path);

	text_buffers = g_ptr_array_new ();
	g_ptr_array_add (text_buffers, current_buffer);	

	buffer_sizes = g_ptr_array_new ();
	g_ptr_array_add (buffer_sizes, GINT_TO_POINTER (buffer_size));

	
	gint proc_id = symbol_db_engine_update_buffer_symbols (sdb_plugin->sdbe_project,
											sdb_plugin->project_opened,
											real_files_list,
											text_buffers,
											buffer_sizes);
	
	if (proc_id > 0)
	{		
		/* good. All is ready for a buffer scan. Add the file_scan into the arrays */
		gchar * local_path_dup = g_strdup (local_path);
		g_ptr_array_add (sdb_plugin->buffer_update_files, local_path_dup);	
		/* add the id too */
		g_ptr_array_add (sdb_plugin->buffer_update_ids, GINT_TO_POINTER (proc_id));
		
		/* add a task so that scan_end_manager can manage this */
		g_tree_insert (sdb_plugin->proc_id_tree, GINT_TO_POINTER (proc_id),
					   GINT_TO_POINTER (TASK_BUFFER_UPDATE));		
	}
	
	g_free (current_buffer);  
	g_object_unref (file);

	/* no need to free local_path, it'll be automatically freed later by the buffer_update
	 * function */
	
	sdb_plugin->need_symbols_update = FALSE;

	return proc_id > 0 ? TRUE : FALSE;
}

static void
on_editor_destroy (SymbolDBPlugin *sdb_plugin, IAnjutaEditor *editor)
{
	const gchar *uri;
	DEBUG_PRINT ("%s", "on_editor_destroy ()");
	if (!sdb_plugin->editor_connected || !sdb_plugin->dbv_view_tree)
	{
		DEBUG_PRINT ("%s", "on_editor_destroy (): returning....");
		return;
	}
	
	uri = g_hash_table_lookup (sdb_plugin->editor_connected, G_OBJECT (editor));
	g_hash_table_remove (sdb_plugin->editor_connected, G_OBJECT (editor));
	
	/* was it the last file loaded? */
	if (g_hash_table_size (sdb_plugin->editor_connected) <= 0)
	{
		DEBUG_PRINT ("%s", "displaying nothing...");
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
	gchar *local_filename;
	gchar *saved_uri;
	GPtrArray *files_array;
	gint proc_id;
	gint i;
	
	local_filename = g_file_get_path (file);
	/* Verify that it's local file */
	g_return_if_fail (local_filename != NULL);
	
	saved_uri = g_file_get_uri (file);
	
	for (i = 0; i < sdb_plugin->buffer_update_files->len; i++)
	{
		if (g_strcmp0 (g_ptr_array_index (sdb_plugin->buffer_update_files, i),
					 local_filename) == 0)
		{
			DEBUG_PRINT ("already scanning");
			/* something is already scanning this buffer file. Drop the procedure now. */
			return;
		}
	}

	files_array = g_ptr_array_new();		
	g_ptr_array_add (files_array, local_filename);		
	/* no need to free local_filename now */
		
	if (!sdb_plugin->editor_connected)
		return;
	
	old_uri = g_hash_table_lookup (sdb_plugin->editor_connected, editor);
		
	if (old_uri && strlen (old_uri) <= 0)
		old_uri = NULL;

	/* files_array will be freed once updating has taken place */
	proc_id = symbol_db_engine_update_files_symbols (sdb_plugin->sdbe_project, 
						sdb_plugin->project_root_dir, files_array, TRUE);
	if (proc_id > 0)
	{		
		/* add a task so that scan_end_manager can manage this */
		g_tree_insert (sdb_plugin->proc_id_tree, GINT_TO_POINTER (proc_id),
					   GINT_TO_POINTER (TASK_FILE_UPDATE));
	}
	
	g_hash_table_insert (sdb_plugin->editor_connected, editor,
						 g_strdup (saved_uri));

	/* if we saved it we shouldn't update a second time */
	sdb_plugin->need_symbols_update = FALSE;
	
	on_editor_update_ui (editor, sdb_plugin);
	g_free (saved_uri);
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
		return;
	}
	else
		DEBUG_PRINT ("%s", "Updating symbols");
	
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
	
	if (local_path == NULL)
	{
		g_critical ("local_path == NULL");
		return;
	}	
				
	symbol_db_view_locals_update_list (
				SYMBOL_DB_VIEW_LOCALS (sdb_plugin->dbv_view_tree_locals),
				 sdb_plugin->sdbe_project, local_path, FALSE);
				 
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
				g_timeout_add_seconds (TIMEOUT_INTERVAL_SYMBOLS_UPDATE,
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

	DEBUG_PRINT ("%s", "SymbolDB: session_save");

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
		sdb_plugin->session_packages = anjuta_session_get_string_list (session, 
																	   SESSION_SECTION, 
																	   SESSION_KEY);
	
		DEBUG_PRINT ("SymbolDB: session_loading started. Getting info from %s",
					 anjuta_session_get_session_directory (session));
		sdb_plugin->session_loading = TRUE;
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
goto_local_tree_iter (SymbolDBPlugin *sdb_plugin, GtkTreeIter *iter)
{
	gint line;

	line = symbol_db_view_locals_get_line (SYMBOL_DB_VIEW_LOCALS (
									sdb_plugin->dbv_view_tree_locals), 
										   sdb_plugin->sdbe_project,
										   iter);	
	
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
		DEBUG_PRINT ("goto_global_tree_iter (): error while trying to get file/line. "
					 "Maybe you clicked on Global/Var etc. node.");
		return;
	};
		
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
	symbol_db_view_row_expanded (SYMBOL_DB_VIEW (user_data->dbv_view_tree),
								user_data->sdbe_project, iter);
}

static void
on_global_treeview_row_collapsed (GtkTreeView *tree_view,
									GtkTreeIter *iter,
                                    GtkTreePath *path,
                                    SymbolDBPlugin *user_data)
{		
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
	
	DEBUG_PRINT ("%s", "value_removed_current_editor ()");
	/* let's remove the timeout for symbols refresh */
	g_source_remove (sdb_plugin->buf_update_timeout_id);
	sdb_plugin->buf_update_timeout_id = 0;
	sdb_plugin->need_symbols_update = FALSE;
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (plugin);
	sdb_plugin->current_editor = NULL;
}

/**
 * Perform the real add to the db and also checks that no dups are inserted.
 * Return the real number of files added.
 */
static gint
do_add_new_files (SymbolDBPlugin *sdb_plugin, const GPtrArray *sources_array, 
				  ProcTask task)
{	
	GPtrArray* languages_array = NULL;	
	GPtrArray* to_scan_array = NULL;
	GHashTable* check_unique_file_hash = NULL;
	IAnjutaLanguage* lang_manager;	
	AnjutaPlugin *plugin;
	gint added_num;
	gint i;
	
	plugin = ANJUTA_PLUGIN (sdb_plugin);

	/* create array of languages and the wannabe scanned files */
	languages_array = g_ptr_array_new ();
	to_scan_array = g_ptr_array_new ();
	
	/* to speed the things up we must avoid the dups */
	check_unique_file_hash = g_hash_table_new_full (g_str_hash, 
						g_str_equal, NULL, NULL);	
	
	lang_manager =	anjuta_shell_get_interface (plugin->shell, IAnjutaLanguage, 
										NULL);	
	
	if (!lang_manager)
	{
		g_critical ("LanguageManager not found");
		return -1;
	}

	for (i=0; i < sources_array->len; i++) 
	{
		const gchar *file_mime;
		const gchar *lang;
		const gchar *local_filename;
		GFile *gfile;
		GFileInfo *gfile_info;
		IAnjutaLanguageId lang_id;
		
		if ( (local_filename = g_ptr_array_index (sources_array, i)) == NULL)		
			continue;
		
		if ((gfile = g_file_new_for_path (local_filename)) == NULL)
			continue;
		
		gfile_info = g_file_query_info (gfile, 
										"*", 
										G_FILE_QUERY_INFO_NONE,
										NULL,
										NULL);
		if (gfile_info == NULL)
		{
			g_object_unref (gfile);
			continue;
		}
		
		/* check if it's already present in the list. This avoids
		 * duplicates.
		 */
		if (g_hash_table_lookup (check_unique_file_hash, 
								 local_filename) == NULL)
		{
			g_hash_table_insert (check_unique_file_hash, 
								 (gpointer)local_filename,
								 (gpointer)local_filename);
		}
		else 
		{
			/* you're a dup! we don't want you */
			continue;
		}
		
		file_mime = g_file_info_get_attribute_string (gfile_info,
										  G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE);										  		
					
		lang_id = ianjuta_language_get_from_mime_type (lang_manager, 
													 file_mime, NULL);
					
		if (!lang_id)
		{
			g_object_unref (gfile);
			g_object_unref (gfile_info);			
			continue;
		}
				
		lang = ianjuta_language_get_name (lang_manager, lang_id, NULL);

		/* test its existence */
		if (g_file_test (local_filename, G_FILE_TEST_EXISTS) == FALSE) 
		{
			g_object_unref (gfile);
			g_object_unref (gfile_info);			
			continue;
		}					

		/* ok, we've just tested that the local_filename does exist.
		 * We can safely add it to the array.
		 */
		g_ptr_array_add (languages_array, g_strdup (lang));					
		g_ptr_array_add (to_scan_array, g_strdup (local_filename));
		g_object_unref (gfile);
		g_object_unref (gfile_info);		
	}
	
	/* last but not least check if we had some files in that GPtrArray. It that's not
	 * the case just pass over
	 */
	if (to_scan_array->len > 0)
	{		
		gint proc_id = 	symbol_db_engine_add_new_files (sdb_plugin->sdbe_project, 
					sdb_plugin->project_opened, to_scan_array, languages_array, 
														   TRUE);
		
		/* insert the proc id associated within the task */
		g_tree_insert (sdb_plugin->proc_id_tree, GINT_TO_POINTER (proc_id),
					   GINT_TO_POINTER (task));
	}

	g_ptr_array_foreach (languages_array, (GFunc)g_free, NULL);
	g_ptr_array_free (languages_array, TRUE);
	
	/* get the real added number of files */
	added_num = to_scan_array->len;
	
	g_ptr_array_foreach (to_scan_array, (GFunc)g_free, NULL);
	g_ptr_array_free (to_scan_array, TRUE);	
	
	g_hash_table_destroy (check_unique_file_hash);
	
	return added_num;
}

static void
on_project_element_added (IAnjutaProjectManager *pm, const gchar *uri,
						  SymbolDBPlugin *sdb_plugin)
{
	GFile *gfile = NULL;		
	gchar *filename;
	gint real_added;
	GPtrArray *files_array;			
		
	g_return_if_fail (sdb_plugin->project_root_uri != NULL);
	g_return_if_fail (sdb_plugin->project_root_dir != NULL);

	gfile = g_file_new_for_uri (uri);	
	filename = g_file_get_path (gfile);

	files_array = g_ptr_array_new ();
	g_ptr_array_add (files_array, filename);

	sdb_plugin->is_adding_element = TRUE;	
	enable_view_signals (sdb_plugin, FALSE, TRUE);

	/* use a custom function to add the files to db */
	real_added = do_add_new_files (sdb_plugin, files_array, TASK_ELEMENT_ADDED);
	if (real_added <= 0) 
	{
		sdb_plugin->is_adding_element = FALSE;
		
		enable_view_signals (sdb_plugin, TRUE, FALSE);
	}
	
	g_ptr_array_foreach (files_array, (GFunc)g_free, NULL);
	g_ptr_array_free (files_array, TRUE);
	
	if (gfile)
		g_object_unref (gfile);	
}

static void
on_project_element_removed (IAnjutaProjectManager *pm, const gchar *uri,
							SymbolDBPlugin *sdb_plugin)
{
	gchar *filename;
	GFile *gfile;
	
	if (!sdb_plugin->project_root_uri)
		return;
	
	gfile = g_file_new_for_uri (uri);
	filename = g_file_get_path (gfile);

	if (filename)
	{
		DEBUG_PRINT ("%s", "on_project_element_removed");
		symbol_db_engine_remove_file (sdb_plugin->sdbe_project, 
			sdb_plugin->project_root_dir, filename);
		
		g_free (filename);
	}
	
	g_object_unref (gfile);
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
	sdb_plugin->files_count_system += num_files;	
	
	DEBUG_PRINT ("********************* START [%s] with n %d files ", package, num_files);
	
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
	
	DEBUG_PRINT ("******************** END () [%s]", package);
	
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

	sdb_plugin->files_count_system_done++;	
	if (sdb_plugin->files_count_system_done >= sdb_plugin->files_count_system)
	{
		message = g_strdup_printf (_("%s: Generating inheritances..."), 
								   sdb_plugin->current_scanned_package);
	}
	else
	{
		message = g_strdup_printf (_("%s: %d files scanned out of %d"), 
							sdb_plugin->current_scanned_package,
							sdb_plugin->files_count_system_done, 
							sdb_plugin->files_count_system);
	}

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
		if (fraction > 1.0)
			fraction = 1.0;
	}
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (sdb_plugin->progress_bar_project),
								   fraction);
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (sdb_plugin->progress_bar_project), message);
	gtk_widget_show (sdb_plugin->progress_bar_project);
	g_free (message);
}

static void
clear_project_progress_bar (SymbolDBEngine *dbe, gpointer data)
{
	SymbolDBPlugin *sdb_plugin;
	GFile* file;
	gchar *local_path;
	
	g_return_if_fail (data != NULL);	
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (data);
	
	/* hide the progress bar */
	gtk_widget_hide (sdb_plugin->progress_bar_project);	
	
	/* re-active global symbols */
	symbol_db_view_open (SYMBOL_DB_VIEW (sdb_plugin->dbv_view_tree), 
						 sdb_plugin->sdbe_project);
	
	/* ok, enable local symbols view */
	if (!IANJUTA_IS_EDITOR (sdb_plugin->current_editor))
	{
		DEBUG_PRINT ("!IANJUTA_IS_EDITOR (sdb_plugin->current_editor))");
		return;
	}
	
	if ((file = ianjuta_file_get_file (IANJUTA_FILE (sdb_plugin->current_editor), 
								  NULL)) == NULL)
	{
		DEBUG_PRINT ("file is NULL");
		return;
	}

	local_path = g_file_get_path (file);	
	symbol_db_view_locals_update_list (
				SYMBOL_DB_VIEW_LOCALS (sdb_plugin->dbv_view_tree_locals),
				 sdb_plugin->sdbe_project, local_path, FALSE);	
	
	g_free (local_path);
}

static void
on_check_offline_single_file_scan_end (SymbolDBEngine *dbe, gpointer data)
{
	on_project_single_file_scan_end (dbe, data);
}

/* note the *system* word in the function */
static void
do_import_system_sources_after_abort (SymbolDBPlugin *sdb_plugin,
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
		GFile *gfile;
		GFileInfo *gfile_info;
		IAnjutaLanguageId lang_id;
		
		local_filename = g_ptr_array_index (sources_array, i);
		
		if (local_filename == NULL)
			continue;		
		
		gfile = g_file_new_for_path (local_filename);
		if (gfile == NULL)
			continue;
		
		gfile_info = g_file_query_info (gfile, 
										"*", 
										G_FILE_QUERY_INFO_NONE,
										NULL,
										NULL);
		if (gfile_info == NULL)
		{
			g_object_unref (gfile);
			continue;
		}
		
		file_mime = g_file_info_get_attribute_string (gfile_info,
										  G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE);										  		
					
		lang_id = ianjuta_language_get_from_mime_type (lang_manager, 
													 file_mime, NULL);
					
		if (!lang_id)
		{
			g_object_unref (gfile);
			g_object_unref (gfile_info);
			continue;
		}
						
		lang = ianjuta_language_get_name (lang_manager, lang_id, NULL);

		/* test its existence */
		if (g_file_test (local_filename, G_FILE_TEST_EXISTS) == FALSE)
		{
			g_object_unref (gfile);
			g_object_unref (gfile_info);			
			continue;		
		}
					
		g_ptr_array_add (languages_array, g_strdup (lang));					
		g_ptr_array_add (to_scan_array, g_strdup (local_filename));
		g_object_unref (gfile);
		g_object_unref (gfile_info);		
	}

	symbol_db_system_parse_aborted_package (sdb_plugin->sdbs, 
									 to_scan_array,
									 languages_array);
	
	/* no need to free the GPtrArray, Huston. They'll be auto-destroyed in that
	 * function 
	 */
}

/* we assume that sources_array has already unique elements */
/* note the *project* word in the function */
static void
do_import_project_sources_after_abort (AnjutaPlugin *plugin, 
							   const GPtrArray *sources_array)
{
	SymbolDBPlugin *sdb_plugin;
	gint real_added;
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (plugin);	

	sdb_plugin->is_project_importing = TRUE;
	
	/* 
	 * if we're importing first shut off the signal receiving.
	 * We'll re-enable that on scan-end 
	 */
	enable_view_signals (sdb_plugin, FALSE, TRUE);
	
	/* connect to receive signals on single file scan complete. We'll
	 * update a status bar notifying the user about the status
	 */
	g_signal_connect (G_OBJECT (sdb_plugin->sdbe_project), "single-file-scan-end",
		  G_CALLBACK (on_project_single_file_scan_end), plugin);

	real_added = do_add_new_files (sdb_plugin, sources_array, 
								   TASK_IMPORT_PROJECT_AFTER_ABORT);
	if (real_added <= 0)
	{
		sdb_plugin->is_project_importing = FALSE;
		
		enable_view_signals (sdb_plugin, TRUE, FALSE);
	}
	else 
	{
		sdb_plugin->files_count_project += real_added;
	}	
}

static void
do_import_project_sources (AnjutaPlugin *plugin, IAnjutaProjectManager *pm, 
				   const gchar *root_dir)
{
	SymbolDBPlugin *sdb_plugin;
	GList* prj_elements_list;
	GPtrArray* sources_array;
	gint i;
	gint real_added;
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (plugin);	

	prj_elements_list = ianjuta_project_manager_get_elements (pm,
					   IANJUTA_PROJECT_MANAGER_SOURCE,
					   NULL);
	
	if (prj_elements_list == NULL)
	{
		g_critical ("No sources found within this project");
		return;
	}
	
	/* if we're importing first shut off the signal receiving.
	 * We'll re-enable that on scan-end 
	 */
	sdb_plugin->is_project_importing = TRUE;
	enable_view_signals (sdb_plugin, FALSE, TRUE);
	
	DEBUG_PRINT ("Retrieving %d gbf sources of the project...",
					 g_list_length (prj_elements_list));

	/* create the storage array. The file names will be strdup'd and put here. 
	 + This is just a sort of GList -> GPtrArray conversion.
	 */
	sources_array = g_ptr_array_new ();
	for (i=0; i < g_list_length (prj_elements_list); i++)
	{	
		gchar *local_filename;
		GFile *gfile = NULL;
		
		if ((gfile = g_file_new_for_uri (g_list_nth_data (prj_elements_list, i))) == NULL)
		{
			continue;
		}		

		if ((local_filename = g_file_get_path (gfile)) == NULL)
		{
			if (gfile)
				g_object_unref (gfile);
			continue;
		}			

		g_ptr_array_add (sources_array, local_filename);
		g_object_unref (gfile);
	}

	/* connect to receive signals on single file scan complete. We'll
	 * update a status bar notifying the user about the status
	 */
	g_signal_connect (G_OBJECT (sdb_plugin->sdbe_project), "single-file-scan-end",
		  G_CALLBACK (on_project_single_file_scan_end), plugin);
	
	real_added = do_add_new_files (sdb_plugin, sources_array, TASK_IMPORT_PROJECT);
	if (real_added <= 0)
	{		
		sdb_plugin->is_project_importing = FALSE;		
		
		enable_view_signals (sdb_plugin, TRUE, FALSE);
	}
	sdb_plugin->files_count_project += real_added;

	
	/* free the ptr array */
	g_ptr_array_foreach (sources_array, (GFunc)g_free, NULL);
	g_ptr_array_free (sources_array, TRUE);

	/* and the list of project files */
	g_list_foreach (prj_elements_list, (GFunc) g_free, NULL);
	g_list_free (prj_elements_list);
}

static void
do_import_system_sources (SymbolDBPlugin *sdb_plugin)
{	
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
		do_import_system_sources_after_abort (sdb_plugin, sys_src_array);
			
		g_ptr_array_foreach (sys_src_array, (GFunc)g_free, NULL);
		g_ptr_array_free (sys_src_array, TRUE);
	}	
}

/**
 * @return TRUE if a scan is in progress, FALSE elsewhere.
 */
static gboolean
do_update_project_symbols (SymbolDBPlugin *sdb_plugin, const gchar *root_dir)
{
	gint proc_id;
	/* Update the symbols */
	proc_id = symbol_db_engine_update_project_symbols (sdb_plugin->sdbe_project, 
														 root_dir);
	if (proc_id > 0)
	{
		sdb_plugin->is_project_updating = TRUE;		
		
		/* insert the proc id associated within the task */
		g_tree_insert (sdb_plugin->proc_id_tree, GINT_TO_POINTER (proc_id),
					   GINT_TO_POINTER (TASK_PROJECT_UPDATE));
		return TRUE;
	}
	
	return FALSE;
}

/**
 * Check the number of languages used by a project and then enable/disable the 
 * global tab in case there's only C files.
 */
static void
do_check_languages_count (SymbolDBPlugin *sdb_plugin)
{
	gint count;
	
	count = symbol_db_engine_get_languages_count (sdb_plugin->sdbe_project);
	
	/* is only C used? */
	if (count == 1)
	{
		if (symbol_db_engine_is_language_used (sdb_plugin->sdbe_project, 
											   "C") == TRUE)
		{
			/* hide the global tab */
			gtk_widget_hide (sdb_plugin->scrolled_global);
		}
	}
	else 
	{
		gtk_widget_show (sdb_plugin->scrolled_global);
	}
}

/**
 * @return TRUE is a scan process is started, FALSE elsewhere.
 */
static gboolean
do_check_offline_files_changed (SymbolDBPlugin *sdb_plugin)
{
	GList * prj_elements_list;
	IAnjutaProjectManager *pm;
	GHashTable *prj_elements_hash;
	GPtrArray *to_add_files = NULL;
	gint i;
	gint real_added ;
	
	pm = anjuta_shell_get_interface (ANJUTA_PLUGIN (sdb_plugin)->shell,
									 IAnjutaProjectManager, NULL);	

	prj_elements_list = ianjuta_project_manager_get_elements (pm,
		   IANJUTA_PROJECT_MANAGER_SOURCE,
		   NULL);
	
	/* fill an hash table with all the items of the list just taken. 
	 * We won't g_strdup () the elements because they'll be freed later
	 */
	prj_elements_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, 
											  NULL);
	
	for (i = 0; i <  g_list_length (prj_elements_list); i++)
	{	
		GFile *gfile;
		gchar *filename;
		const gchar *uri = (const gchar*)g_list_nth_data (prj_elements_list, i);

		if ((gfile = g_file_new_for_uri (uri)) == NULL) 
		{			
			continue;
		}		
		
		if ((filename = g_file_get_path (gfile)) == NULL || 
			g_strcmp0 (filename, "") == 0)
		{
			g_object_unref (gfile);
			/* FIXME here */
			/*DEBUG_PRINT ("hey, filename (uri %s) is NULL", uri);*/
			continue;
		}
		
		/* test its existence */
		if (g_file_test (filename, G_FILE_TEST_EXISTS) == FALSE) 
		{
			/* FIXME here */
			/*DEBUG_PRINT ("hey, filename %s (uri %s) does NOT exist", filename, uri);*/
			g_object_unref (gfile);
			continue;
		}

		g_hash_table_insert (prj_elements_hash, filename, GINT_TO_POINTER (1));		
		g_object_unref (gfile);
	}	
	

	/* some files may have added/removed editing Makefile.am while
	 * Anjuta was offline. Check this case too.
	 */
	SymbolDBEngineIterator *it = 
		symbol_db_engine_get_files_for_project (sdb_plugin->sdbe_project, 
												NULL,
												SYMINFO_FILE_PATH);

	if (it != NULL && symbol_db_engine_iterator_get_n_items (it) > 0)
	{
		GPtrArray *remove_array;
		remove_array = g_ptr_array_new ();
		do {
			SymbolDBEngineIteratorNode *dbin;
			dbin = (SymbolDBEngineIteratorNode *) it;
			
			const gchar * file = 
				symbol_db_engine_iterator_node_get_symbol_extra_string (dbin,
													SYMINFO_FILE_PATH);
			
			if (g_hash_table_remove (prj_elements_hash, file) == FALSE)
			{
				/* hey, we dind't find an element to remove the the project list.
				 * So, probably, this is a new file added in offline mode via Makefile.am
				 * editing.
				 * Keep a reference to it.
				 */
				/*DEBUG_PRINT ("ARRAY REMOVE %s", file);*/
				g_ptr_array_add (remove_array, (gpointer)file);
			}
		} while (symbol_db_engine_iterator_move_next (it));
		
		symbol_db_engine_remove_files (sdb_plugin->sdbe_project,
									   sdb_plugin->project_opened,
									   remove_array);		
		g_ptr_array_free (remove_array, TRUE);		
	}

	/* great, at this point we should have this situation:
	 * remove array = files to remove, remaining items in the hash_table = files 
	 * to add.
	 */
	to_add_files = g_ptr_array_new ();
	if (g_hash_table_size (prj_elements_hash) > 0)
	{
		gint i;
		GList *keys = g_hash_table_get_keys (prj_elements_hash);		
		
		/* get all the nodes from the hash table and add them to the wannabe-added 
		 * array
		 */
		for (i = 0; i < g_hash_table_size (prj_elements_hash); i++)
		{
			/*DEBUG_PRINT ("ARRAY ADD %s", (gchar*)g_list_nth_data (keys, i));*/
			g_ptr_array_add (to_add_files, g_list_nth_data (keys, i));
		}		
	}

	/* good. Let's go on with add of new files. */
	if (to_add_files->len > 0)
	{
		/* block the signals spreading from engine to local-view tab */
		sdb_plugin->is_offline_scanning = TRUE;
		enable_view_signals (sdb_plugin, FALSE, TRUE);
		
		real_added = do_add_new_files (sdb_plugin, to_add_files, 
										   TASK_OFFLINE_CHANGES);
		
		DEBUG_PRINT ("going to do add %d files with TASK_OFFLINE_CHANGES",
					 real_added);
		
		if (real_added <= 0)
		{
			sdb_plugin->is_offline_scanning = FALSE;
			
			enable_view_signals (sdb_plugin, TRUE, FALSE);			
		}
		else {
			/* connect to receive signals on single file scan complete. We'll
	 		 * update a status bar notifying the user about the status
	 		 */
			sdb_plugin->files_count_project += real_added;
			
			g_signal_connect (G_OBJECT (sdb_plugin->sdbe_project), "single-file-scan-end",
		  		G_CALLBACK (on_check_offline_single_file_scan_end), ANJUTA_PLUGIN (sdb_plugin));
		}
	}
	
	g_object_unref (it);
	g_ptr_array_free (to_add_files, TRUE);
	g_hash_table_destroy (prj_elements_hash);
	
	return real_added > 0 ? TRUE : FALSE;	
}

/* add a new project */
static void
on_project_root_added (AnjutaPlugin *plugin, const gchar *name,
					const GValue *value, gpointer user_data)
{
	IAnjutaProjectManager *pm;
	SymbolDBPlugin *sdb_plugin;
	const gchar *root_uri;
	gchar *root_dir;
	GFile *gfile;
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (plugin);

	/*
	 * The Global System symbols thing
	 */

	/* is the global db connected? */	
	if (symbol_db_engine_is_connected (sdb_plugin->sdbe_globals) == FALSE)
	{
		gchar *anjuta_cache_path;
		/* open the connection to global db */
		anjuta_cache_path = anjuta_util_get_user_cache_file_path (".", NULL);
		symbol_db_engine_open_db (sdb_plugin->sdbe_globals, 
							  anjuta_cache_path, 
							  PROJECT_GLOBALS);
		g_free (anjuta_cache_path);
	
		/* unref and recreate the sdbs object */
		if (sdb_plugin->sdbs != NULL)
			g_object_unref (sdb_plugin->sdbs);
		
		sdb_plugin->sdbs = symbol_db_system_new (sdb_plugin, 
												 sdb_plugin->sdbe_globals);		
	}
	
	
	/* Hide the progress bar. Default system tags thing: we'll import after abort even 
	 * if the preferences says not to automatically scan the packages.
	 */
	gtk_widget_hide (sdb_plugin->progress_bar_system);
	
	pm = anjuta_shell_get_interface (ANJUTA_PLUGIN (sdb_plugin)->shell,
									 IAnjutaProjectManager, NULL);
	
	if (sdb_plugin->session_packages == NULL)
	{
		/* hey, does user want to import system sources for this project? */
		gboolean automatic_scan = anjuta_preferences_get_int (sdb_plugin->prefs, 
															  PROJECT_AUTOSCAN);
		
		if (automatic_scan == TRUE)
		{
			sdb_plugin->session_packages = ianjuta_project_manager_get_packages (pm, NULL);
		}
	}

	/* get preferences about the parallel scan */
	gboolean parallel_scan = anjuta_preferences_get_int (sdb_plugin->prefs, 
														 PARALLEL_SCAN); 
	
	if (parallel_scan == TRUE)
	{
		/* we simulate a project-import-end signal received */
		do_import_system_sources (sdb_plugin);		
	}
	
	
	
	/*
	 *   The Project thing
	 */	
		
	g_free (sdb_plugin->project_root_uri);
	sdb_plugin->project_root_uri = NULL;
	if ((root_uri = g_value_get_string (value)) == NULL)
	{
		DEBUG_PRINT ("Warning, root_uri for project is NULL");
		return;
	}

	
	gfile = g_file_new_for_uri (root_uri);
		
	root_dir = g_file_get_path (gfile);
	DEBUG_PRINT ("Symbol-DB: added project root_dir %s, name %s", root_dir, 
				 name);
		
	g_object_unref (gfile);
		
	/* FIXME: where's the project name itself? */
	DEBUG_PRINT ("FIXME: where's the project name itself? using %s", root_dir);
	sdb_plugin->project_opened = g_strdup (root_dir);
	
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
			DEBUG_PRINT ("Project %s does not exist", root_dir);
			needs_sources_scan = TRUE;
			project_exist = FALSE;
		}
		else 
		{
			project_exist = TRUE;
		}

		/* we'll use the same values for db_directory and project_directory */
		DEBUG_PRINT ("Opening db %s and project_dir %s", root_dir, root_dir);
		if (symbol_db_engine_open_db (sdb_plugin->sdbe_project, root_dir, 
										  root_dir) == FALSE)
		{
			g_error ("*** Error in opening db ***");
		}
			
		/* if project did not exist add a new project */
		if (project_exist == FALSE)
		{
			DEBUG_PRINT ("Creating new project.");
			symbol_db_engine_add_new_project (sdb_plugin->sdbe_project,
											  NULL,	/* still no workspace logic */
											  sdb_plugin->project_opened);
		}

		/*
		 * we need an initial import 
		 */
		if (needs_sources_scan == TRUE)
		{
			DEBUG_PRINT ("Importing sources.");
			do_import_project_sources (plugin, pm, root_dir);
		}
		else	
		{
			/*
			 * no import needed. But we may have aborted the scan of sources in 
			 * a previous session..
			 */				
			GPtrArray *sources_array = NULL;				
			gboolean flag_offline;
			gboolean flag_update;
			
			
			sources_array = 
				symbol_db_engine_get_files_with_zero_symbols (sdb_plugin->sdbe_project);

			if (sources_array != NULL && sources_array->len > 0) 
			{				
				do_import_project_sources_after_abort (plugin, sources_array);
				
				g_ptr_array_foreach (sources_array, (GFunc)g_free, NULL);
				g_ptr_array_free (sources_array, TRUE);
			}
				
			/* check for offline changes */				
			flag_offline = do_check_offline_files_changed (sdb_plugin);
				
			/* update any files of the project which isn't up-to-date */
			flag_update = do_update_project_symbols (sdb_plugin, root_dir);
			
			/* if they're both false then there won't be a place where
			 * the do_check_languages_count () is called. Check the returns
			 * and to it here
			 */
			if (flag_offline == FALSE && flag_update == FALSE)
			{
				/* check for the number of languages used in the opened project. */
				do_check_languages_count (sdb_plugin);				
			}				
		}
		gtk_progress_bar_set_text (GTK_PROGRESS_BAR (sdb_plugin->progress_bar_project),
								   _("Populating symbols' db..."));
		id = g_idle_add ((GSourceFunc) gtk_progress_bar_pulse, 
						 sdb_plugin->progress_bar_project);
		gtk_widget_show (sdb_plugin->progress_bar_project);
		
		/* open symbol view, the global symbols gtktree */
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
	DEBUG_PRINT ("%s", "project_root_removed ()");
	/* Disconnect events from project manager */
	
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

	/* stop any opened scanning process */
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (sdb_plugin->progress_bar_system), "");
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (sdb_plugin->progress_bar_project), "");
	gtk_widget_hide (sdb_plugin->progress_bar_system);
	gtk_widget_hide (sdb_plugin->progress_bar_project);
	
	sdb_plugin->files_count_system_done = 0;
	sdb_plugin->files_count_system = 0;
	
	sdb_plugin->files_count_project_done = 0;
	sdb_plugin->files_count_project = 0;
	
	
	g_free (sdb_plugin->project_root_uri);
	g_free (sdb_plugin->project_root_dir);
	g_free (sdb_plugin->project_opened);
	sdb_plugin->project_root_uri = NULL;
	sdb_plugin->project_root_dir = NULL;	
	sdb_plugin->project_opened = NULL;
}

static void
on_scan_end_manager (SymbolDBEngine *dbe, gint process_id, 
										  gpointer data)
{
	SymbolDBPlugin *sdb_plugin;
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (data);	
	gint task_registered;
	
	task_registered = GPOINTER_TO_INT (g_tree_lookup (sdb_plugin->proc_id_tree, 
													   GINT_TO_POINTER (process_id)		
													  ));
	/* hey, we haven't find anything */
	if (task_registered <= 0)
	{
		DEBUG_PRINT ("No task found, proc id was %d", process_id);
		return;
	}
		
	switch (task_registered) 
	{
		case TASK_IMPORT_PROJECT:
		case TASK_IMPORT_PROJECT_AFTER_ABORT:			
		{			
			DEBUG_PRINT ("received TASK_IMPORT_PROJECT (AFTER_ABORT)");
					
			/* re-enable signals receiving on local-view */
			sdb_plugin->is_project_importing = FALSE;
	
			/* disconnect this coz it's not important after the process of importing */
			g_signal_handlers_disconnect_by_func (dbe, on_project_single_file_scan_end, 
										  sdb_plugin);
			
			/* get preferences about the parallel scan */
			gboolean parallel_scan = anjuta_preferences_get_int (sdb_plugin->prefs, 
														 PARALLEL_SCAN); 
			
			do_check_languages_count (sdb_plugin);
			
			/* check the system population has a parallel fashion or not. */			 
			if (parallel_scan == FALSE)
				do_import_system_sources (sdb_plugin);			
		}
			break;			
			
		case TASK_BUFFER_UPDATE:
			DEBUG_PRINT ("received TASK_BUFFER_UPDATE");
			on_editor_buffer_symbol_update_scan_end (dbe, process_id, sdb_plugin);
			break;
			
		case TASK_ELEMENT_ADDED:
			DEBUG_PRINT ("received TASK_ELEMENT_ADDED");
			sdb_plugin->is_adding_element = FALSE;
			do_check_languages_count (sdb_plugin);
			break;
			
		case TASK_OFFLINE_CHANGES:		
			DEBUG_PRINT ("received TASK_OFFLINE_CHANGES");
			
			/* disconnect this coz it's not important after the process of importing */
			g_signal_handlers_disconnect_by_func (dbe, on_check_offline_single_file_scan_end, 
										  sdb_plugin);		
										  
			sdb_plugin->is_offline_scanning = FALSE;
			
			do_check_languages_count (sdb_plugin);
			break;
			
		case TASK_PROJECT_UPDATE:		
			DEBUG_PRINT ("received TASK_PROJECT_UPDATE");
			sdb_plugin->is_project_updating = FALSE;
			break;

		case TASK_FILE_UPDATE:
			DEBUG_PRINT ("received TASK_FILE_UPDATE");
			break;
			
		default:
			DEBUG_PRINT ("Don't know what to to with task_registered %d", 
						 task_registered);
	}
	
	/* ok, we're done. Remove the proc_id from the GTree coz we won't use it anymore */
	if (g_tree_remove (sdb_plugin->proc_id_tree,  GINT_TO_POINTER (process_id)) == FALSE)
		g_warning ("Cannot remove proc_id from GTree");
	
	DEBUG_PRINT ("is_offline_scanning  %d, is_project_importing %d, is_project_updating %d, "
				 "is_adding_element %d", sdb_plugin->is_offline_scanning,
				 sdb_plugin->is_project_importing, sdb_plugin->is_project_updating,
				 sdb_plugin->is_adding_element);
				 
	/**
 	 * perform some checks on some booleans. If they're all successfully passed
 	 * then activate the display of local view
 	 */
	enable_view_signals (sdb_plugin, TRUE, FALSE);
	
	if (sdb_plugin->is_offline_scanning == FALSE &&
		 sdb_plugin->is_project_importing == FALSE &&
		 sdb_plugin->is_project_updating == FALSE &&
		 sdb_plugin->is_adding_element == FALSE)
	{
		sdb_plugin->files_count_project_done = 0;
		sdb_plugin->files_count_project = 0;
		
		clear_project_progress_bar (dbe, sdb_plugin);		
	}	
}

static gboolean
symbol_db_activate (AnjutaPlugin *plugin)
{
	SymbolDBPlugin *sdb_plugin;
	gchar *anjuta_cache_path;
	gchar *ctags_path;
	
	DEBUG_PRINT ("SymbolDBPlugin: Activating SymbolDBPlugin plugin ...");
	
	/* Initialize gda library. */
	gda_init ();

	register_stock_icons (plugin);

	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (plugin);
	sdb_plugin->ui = anjuta_shell_get_ui (plugin->shell, NULL);
	sdb_plugin->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	sdb_plugin->project_opened = NULL;

	ctags_path = anjuta_preferences_get (sdb_plugin->prefs, CTAGS_PREFS_KEY); 

	if (ctags_path == NULL) 
	{
		DEBUG_PRINT ("ctags is not in preferences. Trying a default one %s", 
				   CTAGS_PATH);
		ctags_path = g_strdup (CTAGS_PATH);
	}
	
	/* initialize the session packages to NULL. We'll store there the user 
	 * preferences for the session about global-system packages 
	 */
	sdb_plugin->session_packages = NULL;
	
	sdb_plugin->buf_update_timeout_id = 0;
	sdb_plugin->need_symbols_update = FALSE;
	/* creates and start a new timer. */
	sdb_plugin->update_timer = g_timer_new ();

	/* these two arrays will maintain the same number of objects, 
	 * so that if you search, say on the first, an occurrence of a file,
	 * you'll be able to get in O(1) the _index in the second array, where the 
	 * scan process ids are stored. This is true in the other way too.
	 */
	sdb_plugin->buffer_update_files = g_ptr_array_new ();
	sdb_plugin->buffer_update_ids = g_ptr_array_new ();
	
	sdb_plugin->is_offline_scanning = FALSE;
	sdb_plugin->is_project_importing = FALSE;
	sdb_plugin->is_project_updating = FALSE;
	sdb_plugin->is_adding_element = FALSE;	
	
	DEBUG_PRINT ("SymbolDBPlugin: Initializing engines with %s", ctags_path);
	/* create SymbolDBEngine(s) */
	sdb_plugin->sdbe_project = symbol_db_engine_new (ctags_path);
	if (sdb_plugin->sdbe_project == NULL)
	{		
		g_critical ("sdbe_project == NULL");
		return FALSE;
	}
		
	/* the globals one too */
	sdb_plugin->sdbe_globals = symbol_db_engine_new (ctags_path);
	if (sdb_plugin->sdbe_globals == NULL)
	{		
		g_critical ("sdbe_globals == NULL");
		return FALSE;
	}
	
	g_free (ctags_path);
	
	/* open it */
	anjuta_cache_path = anjuta_util_get_user_cache_file_path (".", NULL);
	symbol_db_engine_open_db (sdb_plugin->sdbe_globals, 
							  anjuta_cache_path, 
							  PROJECT_GLOBALS);
	g_free (anjuta_cache_path);
	
	/* create the object that'll manage the globals population */
	sdb_plugin->sdbs = symbol_db_system_new (sdb_plugin, sdb_plugin->sdbe_globals);

	g_signal_connect (G_OBJECT (sdb_plugin->sdbs), "scan-package-start",
					  G_CALLBACK (on_system_scan_package_start), plugin);	
	
	g_signal_connect (G_OBJECT (sdb_plugin->sdbs), "scan-package-end",
					  G_CALLBACK (on_system_scan_package_end), plugin);	
	
	g_signal_connect (G_OBJECT (sdb_plugin->sdbs), "single-file-scan-end",
					  G_CALLBACK (on_system_single_file_scan_end), plugin);	
	
	/* beign necessary to listen to many scan-end signals, we'll build up a method
	 * to manage them with just one signal connection
	 */
	sdb_plugin->proc_id_tree = g_tree_new_full ((GCompareDataFunc)&symbol_db_gtree_compare_func, 
										 NULL,
										 NULL,
										 NULL);
	
	g_signal_connect (G_OBJECT (sdb_plugin->sdbe_project), "scan-end",
		  		G_CALLBACK (on_scan_end_manager), sdb_plugin);
	
	/* sets preferences to NULL, it'll be instantiated when required.\ */
	sdb_plugin->sdbp = NULL;
	
	/* Create widgets */
	sdb_plugin->dbv_main = gtk_vbox_new(FALSE, 5);
	sdb_plugin->dbv_notebook = gtk_notebook_new();
	sdb_plugin->progress_bar_project = gtk_progress_bar_new();
	gtk_progress_bar_set_ellipsize (GTK_PROGRESS_BAR(sdb_plugin->progress_bar_project),
									PANGO_ELLIPSIZE_MIDDLE);
	g_object_ref (sdb_plugin->progress_bar_project);
	
	sdb_plugin->progress_bar_system = gtk_progress_bar_new();
	gtk_progress_bar_set_ellipsize (GTK_PROGRESS_BAR(sdb_plugin->progress_bar_system),
									PANGO_ELLIPSIZE_MIDDLE);
	
	g_object_ref (sdb_plugin->progress_bar_system);
		
	gtk_box_pack_start (GTK_BOX (sdb_plugin->dbv_main), sdb_plugin->dbv_notebook,
						TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (sdb_plugin->dbv_main), sdb_plugin->progress_bar_project,
						FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (sdb_plugin->dbv_main), sdb_plugin->progress_bar_system,
						FALSE, FALSE, 0);	
	gtk_widget_show_all (sdb_plugin->dbv_main);
	
	/* Local symbols */
	sdb_plugin->scrolled_locals = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sdb_plugin->scrolled_locals),
										 GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sdb_plugin->scrolled_locals),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	
	sdb_plugin->dbv_view_locals_tab_label = gtk_label_new (_("Local" ));
	sdb_plugin->dbv_view_tree_locals = symbol_db_view_locals_new ();
	
	/* activate signals receiving by default */
	symbol_db_view_locals_recv_signals_from_engine (
					SYMBOL_DB_VIEW_LOCALS (sdb_plugin->dbv_view_tree_locals), 
											 sdb_plugin->sdbe_project, TRUE);										 

	g_object_add_weak_pointer (G_OBJECT (sdb_plugin->dbv_view_tree_locals),
							   (gpointer)&sdb_plugin->dbv_view_tree_locals);
	g_signal_connect (G_OBJECT (sdb_plugin->dbv_view_tree_locals), "row-activated",
					  G_CALLBACK (on_local_treeview_row_activated), plugin);

	gtk_container_add (GTK_CONTAINER(sdb_plugin->scrolled_locals), 
					   sdb_plugin->dbv_view_tree_locals);
	
	
	/* Global symbols */
	sdb_plugin->scrolled_global = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (
					GTK_SCROLLED_WINDOW (sdb_plugin->scrolled_global),
					GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sdb_plugin->scrolled_global),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	
	sdb_plugin->dbv_view_tab_label = gtk_label_new (_("Global" ));
	sdb_plugin->dbv_view_tree = symbol_db_view_new ();
	g_object_add_weak_pointer (G_OBJECT (sdb_plugin->dbv_view_tree),
							   (gpointer)&sdb_plugin->dbv_view_tree);
	/* activate signals receiving by default */
	symbol_db_view_recv_signals_from_engine (
					SYMBOL_DB_VIEW (sdb_plugin->dbv_view_tree), 
											 sdb_plugin->sdbe_project, TRUE);										 

	g_signal_connect (G_OBJECT (sdb_plugin->dbv_view_tree), "row-activated",
					  G_CALLBACK (on_global_treeview_row_activated), plugin);

	g_signal_connect (G_OBJECT (sdb_plugin->dbv_view_tree), "row-expanded",
					  G_CALLBACK (on_global_treeview_row_expanded), plugin);

	g_signal_connect (G_OBJECT (sdb_plugin->dbv_view_tree), "row-collapsed",
					  G_CALLBACK (on_global_treeview_row_collapsed), plugin);	
	
	gtk_container_add (GTK_CONTAINER(sdb_plugin->scrolled_global), 
					   sdb_plugin->dbv_view_tree);
	
	/* Search symbols */
	sdb_plugin->dbv_view_tree_search =
		(GtkWidget*) symbol_db_view_search_new (sdb_plugin->sdbe_project);
	sdb_plugin->dbv_view_search_tab_label = gtk_label_new (_("Search" ));

	g_signal_connect (G_OBJECT (sdb_plugin->dbv_view_tree_search), "symbol-selected",
					  G_CALLBACK (on_treesearch_symbol_selected_event),
					  plugin);
	
	g_object_add_weak_pointer (G_OBJECT (sdb_plugin->dbv_view_tree_search),
							   (gpointer)&sdb_plugin->dbv_view_tree_search);

	/* add the scrolled windows to the notebook */
	gtk_notebook_append_page (GTK_NOTEBOOK (sdb_plugin->dbv_notebook),
							  sdb_plugin->scrolled_locals, 
							  sdb_plugin->dbv_view_locals_tab_label);
	
	gtk_notebook_append_page (GTK_NOTEBOOK (sdb_plugin->dbv_notebook),
							  sdb_plugin->scrolled_global, 
							  sdb_plugin->dbv_view_tab_label);
	
	gtk_notebook_append_page (GTK_NOTEBOOK (sdb_plugin->dbv_notebook),
							  sdb_plugin->dbv_view_tree_search, 
							  sdb_plugin->dbv_view_search_tab_label);

	gtk_widget_show_all (sdb_plugin->dbv_notebook);
	
	/* setting focus to the tree_view*/
	gtk_notebook_set_current_page (GTK_NOTEBOOK (sdb_plugin->dbv_notebook), 0);

	sdb_plugin->editor_watch_id = 
		anjuta_plugin_add_watch (plugin, IANJUTA_DOCUMENT_MANAGER_CURRENT_DOCUMENT,
								 value_added_current_editor,
								 value_removed_current_editor, NULL);
	/* Added widgets */
	anjuta_shell_add_widget (plugin->shell, sdb_plugin->dbv_main,
							 "AnjutaSymbolDB", _("Symbols"),
							 "symbol-db-plugin-icon",
							 ANJUTA_SHELL_PLACEMENT_LEFT, NULL);	

	/* Add action group */
	sdb_plugin->popup_action_group = 
		anjuta_ui_add_action_group_entries (sdb_plugin->ui,
											"ActionGroupPopupSymbolDB",
											_("SymbolDb popup actions"),
											actions,
											G_N_ELEMENTS (actions),
											GETTEXT_PACKAGE, FALSE, plugin);
	
	sdb_plugin->menu_action_group = 
		anjuta_ui_add_action_group_entries (sdb_plugin->ui,
											"ActionGroupEditSearchSymbolDB",
											_("SymbolDb menu actions"),
											actions_search,
											G_N_ELEMENTS (actions_search),
											GETTEXT_PACKAGE, FALSE, plugin);
	
	/* Add UI */
	sdb_plugin->merge_id = 
		anjuta_ui_merge (sdb_plugin->ui, UI_FILE);
	
	/* set up project directory watch */
	sdb_plugin->root_watch_id = anjuta_plugin_add_watch (plugin,
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

	DEBUG_PRINT ("%s", "SymbolDBPlugin: Dectivating SymbolDBPlugin plugin ...");
	
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
	
	g_signal_handlers_disconnect_by_func (G_OBJECT (sdb_plugin->sdbs),
										  on_scan_end_manager,
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

	/* this must be done *before* destroying sdbe_globals */
	g_object_unref (sdb_plugin->sdbs);
	sdb_plugin->sdbs = NULL;
	
	g_free (sdb_plugin->current_scanned_package);
	sdb_plugin->current_scanned_package = NULL;
	
	g_object_unref (sdb_plugin->sdbe_globals);
	sdb_plugin->sdbe_globals = NULL;
	
	g_free (sdb_plugin->project_opened);
	sdb_plugin->project_opened = NULL;

	if (sdb_plugin->buffer_update_files)
	{
		g_ptr_array_foreach (sdb_plugin->buffer_update_files, (GFunc)g_free, NULL);
		g_ptr_array_free (sdb_plugin->buffer_update_files, TRUE);
		sdb_plugin->buffer_update_files = NULL;
	}

	if (sdb_plugin->buffer_update_ids)
	{
		g_ptr_array_free (sdb_plugin->buffer_update_ids, TRUE);
		sdb_plugin->buffer_update_ids = NULL;		
	}	
		
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
	
	// FIXME
	g_tree_destroy (sdb_plugin->proc_id_tree);
	
	/* Remove watches */
	anjuta_plugin_remove_watch (plugin, sdb_plugin->root_watch_id, FALSE);
	anjuta_plugin_remove_watch (plugin, sdb_plugin->editor_watch_id, TRUE);

	/* Remove UI */
	anjuta_ui_unmerge (sdb_plugin->ui, sdb_plugin->merge_id);	
	
	/* Remove widgets: Widgets will be destroyed when dbv_main is removed */
	g_object_unref (sdb_plugin->progress_bar_project);
	g_object_unref (sdb_plugin->progress_bar_system);
	anjuta_shell_remove_widget (plugin->shell, sdb_plugin->dbv_main, NULL);

	sdb_plugin->root_watch_id = 0;
	sdb_plugin->editor_watch_id = 0;
	sdb_plugin->merge_id = 0;
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
	DEBUG_PRINT ("%s", "Symbol-DB finalize");
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
		/*DEBUG_PRINT ("%s", "filter_array is NULL");*/
	}
	else 
	{
		filter_array = symbol_db_engine_fill_type_array (match_types);
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
	
	DEBUG_PRINT ("%s", "on_prefs_package_add");
	
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
	DEBUG_PRINT ("%s", "on_prefs_package_remove");	
	if ((item = g_list_find_custom (sdb_plugin->session_packages, package, 
							symbol_db_glist_compare_func)) != NULL)
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
				g_timeout_add_seconds (TIMEOUT_INTERVAL_SYMBOLS_UPDATE,
									   on_editor_buffer_symbols_update_timeout,
									   sdb_plugin);			
		
	}
	
}

static void
ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	DEBUG_PRINT ("%s", "SymbolDB: ipreferences_merge");	
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
	DEBUG_PRINT ("%s", "SymbolDB: ipreferences_iface_init");	
	iface->merge = ipreferences_merge;
	iface->unmerge = ipreferences_unmerge;	
}


ANJUTA_PLUGIN_BEGIN (SymbolDBPlugin, symbol_db);
ANJUTA_PLUGIN_ADD_INTERFACE (isymbol_manager, IANJUTA_TYPE_SYMBOL_MANAGER);
ANJUTA_PLUGIN_ADD_INTERFACE(ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (SymbolDBPlugin, symbol_db);
