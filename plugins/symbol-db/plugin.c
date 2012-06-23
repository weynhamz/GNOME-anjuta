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
#include <libanjuta/anjuta-tabber.h>
#include <libanjuta/anjuta-project.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>
#include <libanjuta/interfaces/ianjuta-symbol.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/interfaces/ianjuta-project.h>
#include <libanjuta/interfaces/ianjuta-file-manager.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-markable.h>
#include <libanjuta/interfaces/ianjuta-language.h>
#include <libanjuta/interfaces/ianjuta-iterable.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>

#include "plugin.h"
#include "symbol-db-engine.h"
#include "symbol-db-views.h"

#define ICON_FILE 							"anjuta-symbol-db-plugin-48.png"
#define UI_FILE 							PACKAGE_DATA_DIR\
											"/ui/anjuta-symbol-db-plugin.xml"

#define BUILDER_FILE 						PACKAGE_DATA_DIR\
											"/glade/anjuta-symbol-db.ui"
#define BUILDER_ROOT 						"symbol_prefs"
#define ICON_FILE 							"anjuta-symbol-db-plugin-48.png"
#define BUFFER_UPDATE 						"symboldb-buffer-update"
#define PARALLEL_SCAN 						"symboldb-parallel-scan"
#define PREFS_BUFFER_UPDATE 				"preferences_toggle:bool:1:1:symboldb-buffer-update"
#define PREFS_PARALLEL_SCAN 				"preferences_toggle:bool:1:1:symboldb-parallel-scan"

#define TIMEOUT_INTERVAL_SYMBOLS_UPDATE		10
#define TIMEOUT_SECONDS_AFTER_LAST_TIP		5

#define PROJECT_GLOBALS						"/"
#define SESSION_SECTION						"SymbolDB"
#define SESSION_KEY							"SystemPackages"
#define PROJECT_ROOT_NAME_DEFAULT			"localprj"

#define ANJUTA_PIXMAP_GOTO_DECLARATION		"element-interface"
#define ANJUTA_PIXMAP_GOTO_IMPLEMENTATION	"element-method"

#define ANJUTA_STOCK_GOTO_DECLARATION		"element-interface"
#define ANJUTA_STOCK_GOTO_IMPLEMENTATION	"element-method"

#define PREF_SCHEMA 						"org.gnome.anjuta.symbol-db"



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
find_file_line (IAnjutaIterable *iterator, gboolean impl, const gchar *current_document,
				gint *line)
{
	gchar *path = NULL;
	gint _line = -1;
	
	do
	{
		const gchar *symbol_kind;
		gboolean is_decl;		
		IAnjutaSymbol *iter_node = IANJUTA_SYMBOL (iterator);
		
		if (iter_node == NULL)
		{
			/* not found or some error occurred */
			break;  
		}
		
		symbol_kind = ianjuta_symbol_get_string (iter_node, IANJUTA_SYMBOL_FIELD_KIND, NULL);				
		is_decl = g_strcmp0 (symbol_kind, "prototype") == 0 || 
			g_strcmp0 (symbol_kind, "interface") == 0;
		
		if (is_decl == !impl) 
		{
			GFile *file;
			gchar *_path;
			file = ianjuta_symbol_get_file (iter_node, NULL);
			/* if the path matches the current document we return immidiately */
			_path = g_file_get_path (file);
			g_object_unref (file);
			if (!current_document || g_strcmp0 (_path, current_document) == 0)
			{
				*line = ianjuta_symbol_get_int (iter_node,
				                                IANJUTA_SYMBOL_FIELD_FILE_POS,
				                                NULL);
				g_free (path);
				
				return _path;
			}
			/* we store the first match incase there is no match against the current document */
			else if (_line == -1)
			{
				path = _path;
				_line = ianjuta_symbol_get_int (iter_node,
				                                IANJUTA_SYMBOL_FIELD_FILE_POS,
				                                NULL);
			}
			else
			{
				g_free (_path);
			}
		}
	} while (ianjuta_iterable_next (iterator, NULL) == TRUE);
	
	if (_line != -1)
		*line = _line;
	
	return path;	
}

static void
goto_file_tag (SymbolDBPlugin *sdb_plugin, const gchar *word,
			   gboolean prefer_implementation)
{
	IAnjutaIterable *iterator;	
	gchar *path = NULL;
	gint line;
	gint i;
	gboolean found = FALSE;
	SymbolDBEngine *engine;
	
	for (i = 0; i < 2; i++)
	{
		if (i == 0)
		{
			engine = sdb_plugin->sdbe_project;
		}
		else
		{
			engine = sdb_plugin->sdbe_globals;
		}		

		iterator = NULL;
		if (symbol_db_engine_is_connected (engine)) 
		{		
			iterator = ianjuta_symbol_query_search (sdb_plugin->search_query,
			                                        word, NULL);
		}
	
		if (iterator != NULL && ianjuta_iterable_get_length (iterator, NULL) > 0)
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
				ianjuta_iterable_first (iterator, NULL);   
				path = find_file_line (iterator, !prefer_implementation, current_document,
									   &line);
			}
		
			if (path)
			{
				goto_file_line (ANJUTA_PLUGIN (sdb_plugin), path, line);
				g_free (path);
				found = TRUE;
			}
		
			g_free (current_document);
		}
	
		if (iterator)
			g_object_unref (iterator);
		
		/* have we found it in the project db? */
		if (found)
			break;
	}
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
	anjuta_shell_present_widget(ANJUTA_PLUGIN(sdb_plugin)->shell,
								sdb_plugin->dbv_main, NULL);
	
	gtk_notebook_set_current_page (GTK_NOTEBOOK(sdb_plugin->dbv_notebook), 2);
	gtk_widget_grab_focus (GTK_WIDGET (sdb_plugin->search_entry));
}

static GtkActionEntry actions[] = 
{
	{ "ActionMenuGoto", NULL, N_("_Go to"), NULL, NULL, NULL},
	{
		"ActionSymbolDBGotoDecl",
		ANJUTA_STOCK_GOTO_DECLARATION,
		N_("Tag De_claration"),
		"<shift><control>d",
		N_("Go to symbol declaration"),
		G_CALLBACK (on_goto_file_tag_decl_activate)
	},
	{
		"ActionSymbolDBGotoImpl",
		ANJUTA_STOCK_GOTO_IMPLEMENTATION,
		/* Translators: Go to the line where the tag is implemented */
		N_("Tag _Implementation"),
		"<control>d",
		N_("Go to symbol definition"),
		G_CALLBACK (on_goto_file_tag_impl_activate)
	}	
};

static GtkActionEntry actions_search[] = {
  { 
	"ActionEditSearchFindSymbol", GTK_STOCK_FIND, N_("_Find Symbol…"),
	"<control>l", N_("Find Symbol"),
    G_CALLBACK (on_find_symbol)
  }
};

static gboolean
editor_buffer_symbols_update (IAnjutaEditor *editor, SymbolDBPlugin *sdb_plugin)
{
	gchar *current_buffer = NULL;
	gsize buffer_size = 0;
	GFile* file;
	gchar * local_path;
	GPtrArray *real_files_list;
	GPtrArray *text_buffers;
	GPtrArray *buffer_sizes;
	gint i;
	gint proc_id ;
	
	/* we won't proceed with the updating of the symbols if we didn't type in 
	   anything */
	if (sdb_plugin->need_symbols_update == FALSE)
		return TRUE;

	if (editor) 
	{
		buffer_size = ianjuta_editor_get_length (editor, NULL);
		current_buffer = ianjuta_editor_get_text_all (editor, NULL);
				
		file = ianjuta_file_get_file (IANJUTA_FILE (editor), NULL);
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

	real_files_list = g_ptr_array_new_with_free_func (g_free);
	g_ptr_array_add (real_files_list, local_path);

	text_buffers = g_ptr_array_new ();
	g_ptr_array_add (text_buffers, current_buffer);	

	buffer_sizes = g_ptr_array_new ();
	g_ptr_array_add (buffer_sizes, GINT_TO_POINTER (buffer_size));


	proc_id = 0;
	if (symbol_db_engine_is_connected (sdb_plugin->sdbe_project))
	{		
		proc_id = symbol_db_engine_update_buffer_symbols (sdb_plugin->sdbe_project,
											sdb_plugin->project_opened,
											real_files_list,
											text_buffers,
											buffer_sizes);
	}

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

	g_ptr_array_unref (real_files_list);
	g_free (current_buffer);  
	g_object_unref (file);

	/* no need to free local_path, it'll be automatically freed later by the buffer_update
	 * function */

	sdb_plugin->need_symbols_update = FALSE;

	return proc_id > 0 ? TRUE : FALSE;
}
static gboolean
on_editor_buffer_symbols_update_timeout (gpointer user_data)
{
	SymbolDBPlugin *sdb_plugin;
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

	if (seconds_elapsed < TIMEOUT_SECONDS_AFTER_LAST_TIP)
		return TRUE;

	return editor_buffer_symbols_update (IANJUTA_EDITOR (sdb_plugin->current_editor),
										 sdb_plugin);
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
			/* hey we found it */
			/* remove both the items */
			g_ptr_array_remove_index (sdb_plugin->buffer_update_ids, i);
			
			g_ptr_array_remove_index (sdb_plugin->buffer_update_files, i);

			/* no need to free the string coz the g_ptr_array is built with
			 * g_ptr_array_new_with_free_func (g_free)
			 */
		}
	}

	/* was the updating of view-locals symbols blocked while we were scanning?
	 * e.g. was the editor switched? */
	if (sdb_plugin->buffer_update_semaphore == TRUE)
	{
		GFile *file;
		gchar *local_path;
		gboolean tags_update;
		if (!IANJUTA_IS_EDITOR (sdb_plugin->current_editor))
			return;
	
		file = ianjuta_file_get_file (IANJUTA_FILE (sdb_plugin->current_editor), 
		    NULL);
	
		if (file == NULL)
			return;

		local_path = g_file_get_path (file);
	
		if (local_path == NULL)
		{
			g_critical ("local_path == NULL");
			return;
		}	

		/* add a default timeout to the updating of buffer symbols */	
		tags_update = g_settings_get_boolean (sdb_plugin->settings, BUFFER_UPDATE);
		
		if (tags_update)
		{
			sdb_plugin->buf_update_timeout_id = 
					g_timeout_add_seconds (TIMEOUT_INTERVAL_SYMBOLS_UPDATE,
										   on_editor_buffer_symbols_update_timeout,
										   sdb_plugin);
		}		
		
		g_free (local_path);
		sdb_plugin->buffer_update_semaphore = FALSE;
	}	 
}

static void
on_editor_destroy (SymbolDBPlugin *sdb_plugin, IAnjutaEditor *editor)
{
	if (!sdb_plugin->editor_connected)
	{
		DEBUG_PRINT ("%s", "on_editor_destroy (): returning….");
		return;
	}
	
	g_hash_table_remove (sdb_plugin->editor_connected, G_OBJECT (editor));

	if (g_hash_table_size (sdb_plugin->editor_connected) <= 0)
	{
		DEBUG_PRINT ("%s", "displaying nothing…");
		if (sdb_plugin->file_model)
			g_object_set (sdb_plugin->file_model, "file-path", NULL, NULL);
	}
}

static void
on_editor_update_ui (IAnjutaEditor *editor, SymbolDBPlugin *sdb_plugin) 
{
	g_timer_reset (sdb_plugin->update_timer);
}

static void
on_code_added (IAnjutaEditor *editor, IAnjutaIterable *position, gchar *code,
			   SymbolDBPlugin *sdb_plugin)
{
	sdb_plugin->need_symbols_update = TRUE;
	editor_buffer_symbols_update (editor, sdb_plugin);
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
	proc_id = 0;
	if (symbol_db_engine_is_connected (sdb_plugin->sdbe_project))
	{
		proc_id = symbol_db_engine_update_files_symbols (sdb_plugin->sdbe_project, 
							sdb_plugin->project_root_dir, files_array, TRUE);
	}
	
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

	/* we can have a weird behaviour here if we're not paying the right attention:
	 * A timeout scan could have been launched and a millisecond later the user could
	 * have switched editor: we'll be getting the symbol inserted in the previous
	 * editor into the new one's view.
	 */
	if (sdb_plugin->buffer_update_files->len > 0)
	{
		sdb_plugin->buffer_update_semaphore = TRUE;
	}
	else 
	{
		g_object_set (sdb_plugin->file_model, "file-path", local_path, NULL);
		
		/* add a default timeout to the updating of buffer symbols */	
		tags_update = g_settings_get_boolean (sdb_plugin->settings, BUFFER_UPDATE);

				
		if (tags_update)
		{
			sdb_plugin->buf_update_timeout_id = 
					g_timeout_add_seconds (TIMEOUT_INTERVAL_SYMBOLS_UPDATE,
										   on_editor_buffer_symbols_update_timeout,
										   plugin);
		}		
	}
				 
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
		g_signal_connect (G_OBJECT (editor), "code-added",
						  G_CALLBACK (on_code_added),
						  sdb_plugin);
		g_signal_connect (G_OBJECT(editor), "update_ui",
						  G_CALLBACK (on_editor_update_ui),
						  sdb_plugin);
	}
	g_free (uri);
	g_free (local_path);
	
	sdb_plugin->need_symbols_update = FALSE;
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
	g_signal_handlers_disconnect_by_func (G_OBJECT(key),
										  G_CALLBACK (on_code_added),
										  user_data);
	g_object_weak_unref (G_OBJECT(key),
						 (GWeakNotify) (on_editor_destroy),
						 user_data);
}

static void
value_removed_current_editor (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	SymbolDBPlugin *sdb_plugin;

	sdb_plugin = (SymbolDBPlugin *) plugin;
	
	DEBUG_PRINT ("%s", "value_removed_current_editor ()");
	/* let's remove the timeout for symbols refresh */
	if (sdb_plugin->buf_update_timeout_id)
		g_source_remove (sdb_plugin->buf_update_timeout_id);
	sdb_plugin->buf_update_timeout_id = 0;
	sdb_plugin->need_symbols_update = FALSE;
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (plugin);
	sdb_plugin->current_editor = NULL;
}

/**
 * Perform the real add to the db and also checks that no dups are inserted.
 * Return the real number of files added or -1 on error.
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
	gint proc_id;
	
	plugin = ANJUTA_PLUGIN (sdb_plugin);

	/* create array of languages and the wannabe scanned files */
	languages_array = g_ptr_array_new_with_free_func (g_free);
	to_scan_array = g_ptr_array_new_with_free_func (g_free);
	
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
										"standard::content-type", 
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
	proc_id = 0;
	if (to_scan_array->len > 0)
	{		
		proc_id = 	symbol_db_engine_add_new_files_full_async (sdb_plugin->sdbe_project, 
					sdb_plugin->project_opened, "1.0", to_scan_array, languages_array, 
														   TRUE);
	}

	if (proc_id > 0)
	{
		/* insert the proc id associated within the task */
		g_tree_insert (sdb_plugin->proc_id_tree, GINT_TO_POINTER (proc_id),
					   GINT_TO_POINTER (task));
	}

	/* get the real added number of files */
	added_num = to_scan_array->len;

	g_ptr_array_unref (languages_array);
	g_ptr_array_unref (to_scan_array);	
	
	g_hash_table_destroy (check_unique_file_hash);
	
	return proc_id > 0 ? added_num : -1;
}

static void
on_project_element_added (IAnjutaProjectManager *pm, GFile *gfile,
						  SymbolDBPlugin *sdb_plugin)
{
	gchar *filename;
	gint real_added;
	GPtrArray *files_array;			
		
	g_return_if_fail (sdb_plugin->project_root_uri != NULL);
	g_return_if_fail (sdb_plugin->project_root_dir != NULL);

	filename = g_file_get_path (gfile);

	files_array = g_ptr_array_new_with_free_func (g_free);
	g_ptr_array_add (files_array, filename);

	sdb_plugin->is_adding_element = TRUE;	
	
	/* use a custom function to add the files to db */
	real_added = do_add_new_files (sdb_plugin, files_array, TASK_ELEMENT_ADDED);
	if (real_added <= 0) 
	{
		sdb_plugin->is_adding_element = FALSE;
	}
	
	g_ptr_array_unref (files_array);
}

static void
on_project_element_removed (IAnjutaProjectManager *pm, GFile *gfile,
							SymbolDBPlugin *sdb_plugin)
{
	gchar *filename;
	
	if (!sdb_plugin->project_root_uri)
		return;
	
	filename = g_file_get_path (gfile);

	if (filename)
	{
		DEBUG_PRINT ("%s", "on_project_element_removed");
		symbol_db_engine_remove_file (sdb_plugin->sdbe_project, 
		                              sdb_plugin->project_root_dir,
		                              symbol_db_util_get_file_db_path (sdb_plugin->sdbe_project,
		                                                               filename));
		
		g_free (filename);
	}
}

static void
on_system_scan_package_start (SymbolDBEngine *dbe, guint num_files, 
							  const gchar *package, gpointer user_data)
{
	SymbolDBPlugin *sdb_plugin;
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (user_data);

	sdb_plugin->files_count_system_done = 0;
	sdb_plugin->files_count_system += num_files;	
	
		
	/* show the global bar */
	gtk_widget_show (sdb_plugin->progress_bar_system);
	if (sdb_plugin->current_scanned_package != NULL)
		g_free (sdb_plugin->current_scanned_package);
	sdb_plugin->current_scanned_package = g_strdup (package);
	
	DEBUG_PRINT ("********************* START [%s] with n %d files ", package, num_files);
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
		message = g_strdup_printf (_("%s: Generating inheritances…"), 
								   sdb_plugin->current_scanned_package);
	}
	else
	{
		/* Translators: %s is the name of a system library */
		message = g_strdup_printf (ngettext ("%s: %d file scanned out of %d", "%s: %d files scanned out of %d", sdb_plugin->files_count_system_done), 
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
		message = g_strdup_printf (_("Generating inheritances…"));
	else
		message = g_strdup_printf (ngettext ("%d file scanned out of %d", "%d files scanned out of %d", sdb_plugin->files_count_project_done), 
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
	
	g_return_if_fail (data != NULL);	
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (data);
	
	/* hide the progress bar */
	gtk_widget_hide (sdb_plugin->progress_bar_project);	
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

/* FIXME HERE currently disabled */
#if 0	
	symbol_db_system_parse_aborted_package (sdb_plugin->sdbs, 
									 to_scan_array,
									 languages_array);
#endif	
	/* no need to free the GPtrArray, Huston. They'll be auto-destroyed in that
	 * function 
	 */
}

/* we assume that sources_array has already unique elements */
/* note the *project* word in the function */
static void
do_import_project_sources_after_abort (SymbolDBPlugin *sdb_plugin, 
							   const GPtrArray *sources_array)
{
	gint real_added;

	sdb_plugin->is_project_importing = TRUE;
	
	/* connect to receive signals on single file scan complete. We'll
	 * update a status bar notifying the user about the status
	 */
	g_signal_connect (G_OBJECT (sdb_plugin->sdbe_project), "single-file-scan-end",
		  G_CALLBACK (on_project_single_file_scan_end), sdb_plugin);

	real_added = do_add_new_files (sdb_plugin, sources_array, 
								   TASK_IMPORT_PROJECT_AFTER_ABORT);
	if (real_added <= 0)
	{
		sdb_plugin->is_project_importing = FALSE;
	}
	else 
	{
		sdb_plugin->files_count_project += real_added;
	}	
}

static void
do_import_project_sources (SymbolDBPlugin *sdb_plugin, IAnjutaProjectManager *pm, 
				   const gchar *root_dir)
{
	GList* prj_elements_list;
	GPtrArray* sources_array;
	gint i;
	gint real_added;

	prj_elements_list = ianjuta_project_manager_get_elements (pm,
					   ANJUTA_PROJECT_SOURCE | ANJUTA_PROJECT_PROJECT,
					   NULL);
	
	if (prj_elements_list == NULL)
	{
		g_warning ("No sources found within this project");
		return;
	}
	
	/* if we're importing first shut off the signal receiving.
	 * We'll re-enable that on scan-end 
	 */
	sdb_plugin->is_project_importing = TRUE;
	DEBUG_PRINT ("Retrieving %d gbf sources of the project…",
					 g_list_length (prj_elements_list));

	/* create the storage array. The file names will be strdup'd and put here. 
	 * This is just a sort of GList -> GPtrArray conversion.
	 */
	sources_array = g_ptr_array_new_with_free_func (g_free);
	for (i=0; i < g_list_length (prj_elements_list); i++)
	{	
		gchar *local_filename;
		GFile *gfile = NULL;
		
		gfile = g_list_nth_data (prj_elements_list, i);

		if ((local_filename = g_file_get_path (gfile)) == NULL)
		{
			continue;
		}			

		g_ptr_array_add (sources_array, local_filename);
	}

	/* connect to receive signals on single file scan complete. We'll
	 * update a status bar notifying the user about the status
	 */
	g_signal_connect (G_OBJECT (sdb_plugin->sdbe_project), "single-file-scan-end",
		  G_CALLBACK (on_project_single_file_scan_end), sdb_plugin);
	
	real_added = do_add_new_files (sdb_plugin, sources_array, TASK_IMPORT_PROJECT);
	if (real_added <= 0)
	{		
		sdb_plugin->is_project_importing = FALSE;		
	}
	sdb_plugin->files_count_project += real_added;

	
	/* free the ptr array */
	g_ptr_array_unref (sources_array);

	/* and the list of project files */
	g_list_foreach (prj_elements_list, (GFunc) g_object_unref, NULL);
	g_list_free (prj_elements_list);
}

/**
 * This function will call do_import_project_sources_after_abort ().
 * The list of files for sysstem packages enqueued on the first scan aren't 
 * persisted on session for later retrieval. So we can only rely
 * on fixing the zero-symbols file.
 */
static void
do_import_system_sources (SymbolDBPlugin *sdb_plugin)
{	
	/* the resume thing */
	GPtrArray *sys_src_array = NULL;
	sys_src_array = 
		symbol_db_util_get_files_with_zero_symbols (sdb_plugin->sdbe_globals);

	if (sys_src_array != NULL && sys_src_array->len > 0) 
	{
		do_import_system_sources_after_abort (sdb_plugin, sys_src_array);
			
		g_ptr_array_unref (sys_src_array);
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
														 root_dir, FALSE);
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
 * do_check_offline_files_changed:
 * @sdb_plugin: self
 * 
 * Returns: TRUE if a scan process is started, FALSE elsewhere.
 */
static gboolean
do_check_offline_files_changed (SymbolDBPlugin *sdb_plugin)
{
	GList * prj_elements_list;
	GList *node;
	IAnjutaProjectManager *pm;
	GHashTable *prj_elements_hash;
	GPtrArray *to_add_files = NULL;
	gint real_added = 0;
	
	pm = anjuta_shell_get_interface (ANJUTA_PLUGIN (sdb_plugin)->shell,
									 IAnjutaProjectManager, NULL);	

	prj_elements_list = ianjuta_project_manager_get_elements (pm,
		   ANJUTA_PROJECT_SOURCE | ANJUTA_PROJECT_PROJECT,
		   NULL);
	
	/* fill an hash table with all the items of the list just taken. 
	 * We won't g_strdup () the elements because they'll be freed later
	 */
	prj_elements_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                           NULL, g_free);

	for (node = prj_elements_list; node != NULL; node = g_list_next (node))
	{	
		GFile *gfile;
		gchar *filename;
		gchar *db_path;

		gfile = node->data;
		if (!gfile)
			continue;
	
		if ((filename = g_file_get_path (gfile)) == NULL || 
			!strlen (filename))
		{
			g_object_unref (gfile);

			continue;
		}
		
		/* test its existence */
		if (g_file_query_exists (gfile, NULL) == FALSE) 
		{
			g_object_unref (gfile);

			continue;
		}

		/* Use g_hash_table_replace instead of g_hash_table_insert because the key
		 * and the value use the same block of memory, both must be changed at
		 * the same time. */
		db_path = symbol_db_util_get_file_db_path (sdb_plugin->sdbe_project,
		                                           filename);
		
		if (db_path)
			g_hash_table_replace (prj_elements_hash,
			                      db_path,
			                      filename);
		g_object_unref (gfile);
	}	
	

	/* some files may have added/removed editing Makefile.am while
	 * Anjuta was offline. Check this case too.
	 * FIXME: Get rid of data model here.
	 */
	GdaDataModel *model =
		symbol_db_engine_get_files_for_project (sdb_plugin->sdbe_project);
	GdaDataModelIter *it =
		gda_data_model_create_iter (model);
	
	if (it && gda_data_model_iter_move_to_row (it, 0))
	{
		GPtrArray *remove_array;
		remove_array = g_ptr_array_new_with_free_func (g_free);
		do {
			const GValue *val = gda_data_model_iter_get_value_at (it, 0);
			const gchar * file = g_value_get_string (val);
			
			if (file && g_hash_table_remove (prj_elements_hash, file) == FALSE)
				g_ptr_array_add (remove_array, g_strdup (file));
			
		} while (gda_data_model_iter_move_next (it));
		
		symbol_db_engine_remove_files (sdb_plugin->sdbe_project,
									   sdb_plugin->project_opened,
									   remove_array);
		g_ptr_array_unref (remove_array);
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
			g_ptr_array_add (to_add_files,
			                 g_hash_table_lookup (prj_elements_hash,
			                                      g_list_nth_data (keys, i)));
		}		
	}

	/* good. Let's go on with add of new files. */
	if (to_add_files->len > 0)
	{
		/* block the signals spreading from engine to local-view tab */
		sdb_plugin->is_offline_scanning = TRUE;
		real_added = do_add_new_files (sdb_plugin, to_add_files, 
										   TASK_OFFLINE_CHANGES);
		
		DEBUG_PRINT ("going to do add %d files with TASK_OFFLINE_CHANGES",
					 real_added);
		
		if (real_added <= 0)
		{
			sdb_plugin->is_offline_scanning = FALSE;
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
	
	/*if (it != NULL) g_object_unref (it);*/
	g_object_unref (it);
	g_object_unref (model);
	g_ptr_array_unref (to_add_files);
	g_hash_table_destroy (prj_elements_hash);
	
	return real_added > 0 ? TRUE : FALSE;	
}

static void
on_session_save (AnjutaShell *shell, AnjutaSessionPhase phase,
				 AnjutaSession *session,
				 SymbolDBPlugin *sdb_plugin)
{
	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;

	DEBUG_PRINT ("%s", "SymbolDB: session_save");
}

static void
on_session_load (AnjutaShell *shell, AnjutaSessionPhase phase,
				 AnjutaSession *session,
				 SymbolDBPlugin *sdb_plugin)
{
	if (phase == ANJUTA_SESSION_PHASE_START)
	{
		DEBUG_PRINT ("SymbolDB: session_loading started. Getting info from %s",
					 anjuta_session_get_session_directory (session));
		sdb_plugin->session_loading = TRUE;
		
		/* get preferences about the parallel scan */
		gboolean parallel_scan = g_settings_get_boolean (sdb_plugin->settings,
															 PARALLEL_SCAN);  
		
		if (parallel_scan == TRUE && 
			symbol_db_engine_is_connected (sdb_plugin->sdbe_globals) == TRUE)
		{
			/* we simulate a project-import-end signal received */
			do_import_system_sources (sdb_plugin);
		}
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
		
		if (sdb_plugin->project_opened == NULL)
		{
			gtk_widget_hide (sdb_plugin->progress_bar_project);
			gtk_widget_hide (sdb_plugin->progress_bar_system);
		}
	}	
}

static void
on_project_loaded (IAnjutaProjectManager *pm, GError *error,
						  SymbolDBPlugin *sdb_plugin)
{
	g_return_if_fail (sdb_plugin->project_root_uri != NULL);
	g_return_if_fail (sdb_plugin->project_root_dir != NULL);

	/* Malformed project abort */
	if (error != NULL) return;

	/*
	 * we need an initial import 
	 */
	if (sdb_plugin->needs_sources_scan == TRUE)
	{
		DEBUG_PRINT ("Importing sources.");
		do_import_project_sources (sdb_plugin, pm, sdb_plugin->project_root_dir);
	}
	else	
	{
		/*
		 * no import needed. But we may have aborted the scan of sources in 
		 * a previous session..
		 */				
		GPtrArray *sources_array = NULL;				
		
		DEBUG_PRINT ("Checking for files with zero symbols.");
		sources_array = 
			symbol_db_util_get_files_with_zero_symbols (sdb_plugin->sdbe_project);

		if (sources_array != NULL && sources_array->len > 0) 
		{				
			DEBUG_PRINT ("Importing files after abort.");
			do_import_project_sources_after_abort (sdb_plugin, sources_array);
			
			g_ptr_array_unref (sources_array);
		}

		DEBUG_PRINT ("Checking for offline changes.");
		/* check for offline changes */				
		if (do_check_offline_files_changed (sdb_plugin) == FALSE)
		{
			DEBUG_PRINT ("no changes. Skipping.");
		}

		DEBUG_PRINT ("Updating project symbols.");
		/* update any files of the project which isn't up-to-date */
		if (do_update_project_symbols (sdb_plugin, sdb_plugin->project_opened) == FALSE)
		{
			DEBUG_PRINT ("no changes. Skipping.");
		}
	}
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
	IAnjutaProject *project;
	
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
		if (symbol_db_engine_open_db (sdb_plugin->sdbe_globals, 
							  anjuta_cache_path, 
							  PROJECT_GLOBALS) == DB_OPEN_STATUS_FATAL)
		{
			g_error ("Opening global project under %s", anjuta_cache_path);
		}
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

	project = ianjuta_project_manager_get_current_project (pm, NULL);

	/* let the project be something like "." to avoid problems when renaming the 
	 * project dir */
	sdb_plugin->project_opened = g_strdup (PROJECT_ROOT_NAME_DEFAULT);
	
	if (root_dir)
	{
		gboolean project_exist = FALSE;
		guint id;
			
		/* we'll use the same values for db_directory and project_directory */
		DEBUG_PRINT ("Opening db %s and project_dir %s", root_dir, root_dir);
		gint open_status = symbol_db_engine_open_db (sdb_plugin->sdbe_project, root_dir, 
										  root_dir);

		/* is it a fresh-new project? is it an imported project with 
		 * no 'new' symbol-db database but the 'old' one symbol-browser? 
		 */
		sdb_plugin->needs_sources_scan = FALSE;
		switch (open_status)
		{
			case DB_OPEN_STATUS_FATAL:
				g_warning ("*** Error in opening db ***");
				return;
				
			case DB_OPEN_STATUS_NORMAL:
				project_exist = TRUE;
				break;

			case DB_OPEN_STATUS_CREATE:
			case DB_OPEN_STATUS_UPGRADE:
				sdb_plugin->needs_sources_scan = TRUE;
				project_exist = FALSE;
				break;
				
			default:
				break;
		}
			
		/* if project did not exist add a new project */
		if (project_exist == FALSE)
		{
			DEBUG_PRINT ("Creating new project.");
			symbol_db_engine_add_new_project (sdb_plugin->sdbe_project,
											  NULL,	/* still no workspace logic */
											  sdb_plugin->project_opened,
			    							  "1.0");
		}

		gtk_progress_bar_set_text (GTK_PROGRESS_BAR (sdb_plugin->progress_bar_project),
								   _("Populating symbol database…"));
		id = g_idle_add ((GSourceFunc) gtk_progress_bar_pulse, 
						 sdb_plugin->progress_bar_project);
		gtk_widget_show (sdb_plugin->progress_bar_project);
		g_source_remove (id);
		gtk_widget_hide (sdb_plugin->progress_bar_project);

		/* root dir */
		sdb_plugin->project_root_dir = root_dir;
	}
	/* this is uri */
	sdb_plugin->project_root_uri = g_strdup (root_uri);	
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
									 GINT_TO_POINTER (process_id)));
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
			gboolean parallel_scan = g_settings_get_boolean (sdb_plugin->settings,
														     PARALLEL_SCAN); 

			
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
			break;
			
		case TASK_OFFLINE_CHANGES:		
			DEBUG_PRINT ("received TASK_OFFLINE_CHANGES");
			
			/* disconnect this coz it's not important after the process of importing */
			g_signal_handlers_disconnect_by_func (dbe, on_check_offline_single_file_scan_end, 
										  sdb_plugin);		
										  
			sdb_plugin->is_offline_scanning = FALSE;
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

	/* is the project still opened? */
	if (sdb_plugin->project_opened == NULL)
	{
		/* just return, the project may have been closed while we were waiting for the
		 * scanning to finish
		 */
		return;
	}
		
	/**
 	 * perform some checks on some booleans. If they're all successfully passed
 	 * then activate the display of local view
 	 */
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

static void
on_isymbol_manager_prj_scan_end (SymbolDBEngine *dbe,
    								gint process_id,
    								IAnjutaSymbolManager *sm)
{
	g_signal_emit_by_name (sm, "prj-scan-end", process_id);
}

static void
on_isymbol_manager_sys_scan_begin (SymbolDBEngine *dbe, gint process_id, 
                                   SymbolDBPlugin *sdb_plugin)
{
	sdb_plugin->current_pkg_scanned = g_async_queue_pop (sdb_plugin->global_scan_aqueue);

	if (sdb_plugin->current_pkg_scanned == NULL)
		return;

	DEBUG_PRINT ("==%d==>\n"
				 "begin %s", process_id, sdb_plugin->current_pkg_scanned->package_name);
	gtk_widget_show (sdb_plugin->progress_bar_system);
}

static void
on_isymbol_manager_sys_single_scan_end (SymbolDBEngine *dbe, SymbolDBPlugin *sdb_plugin)
{
	PackageScanData *pkg_scan;

	/* ignore signals when scan-end has already been received */
	if (sdb_plugin->current_pkg_scanned == NULL)
	{
		return;
	}
	
	pkg_scan = sdb_plugin->current_pkg_scanned;
	pkg_scan->files_done++;

	gtk_widget_show (sdb_plugin->progress_bar_system);
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (sdb_plugin->progress_bar_system),
								   (gdouble)pkg_scan->files_done / 
	                               (gdouble)pkg_scan->files_length);	
}

static void
on_isymbol_manager_sys_scan_end (SymbolDBEngine *dbe,
    							 gint process_id,
    							 SymbolDBPlugin *sdb_plugin)
{
	IAnjutaSymbolManager *sm;
	PackageScanData *pkg_scan;

	g_return_if_fail (sdb_plugin->current_pkg_scanned != NULL);

	DEBUG_PRINT ("<==%d==\nscan end %s. Queue now is %d", 
	             process_id,
	             sdb_plugin->current_pkg_scanned->package_name,
	             g_async_queue_length (sdb_plugin->global_scan_aqueue));
	
	sm = anjuta_shell_get_interface (ANJUTA_PLUGIN (sdb_plugin)->shell,
									 IAnjutaSymbolManager, NULL);
	
	g_signal_emit_by_name (sm, "sys-scan-end", process_id);

	pkg_scan = sdb_plugin->current_pkg_scanned;
	g_free (pkg_scan->package_name);
	g_free (pkg_scan->package_version);
	g_free (pkg_scan);
	
	sdb_plugin->current_pkg_scanned = NULL;

	gtk_widget_hide (sdb_plugin->progress_bar_system);
}

static gboolean
symbol_db_activate (AnjutaPlugin *plugin)
{
	IAnjutaProjectManager *pm;
	SymbolDBPlugin *sdb_plugin;
	gchar *anjuta_cache_path;
	gchar *ctags_path;
	GtkWidget *view, *label;
	
	DEBUG_PRINT ("SymbolDBPlugin: Activating SymbolDBPlugin plugin …");
	
	/* Initialize gda library. */
	gda_init ();

	register_stock_icons (plugin);

	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (plugin);
	sdb_plugin->ui = anjuta_shell_get_ui (plugin->shell, NULL);
	sdb_plugin->project_opened = NULL;

	pm = anjuta_shell_get_interface (ANJUTA_PLUGIN (sdb_plugin)->shell,
									 IAnjutaProjectManager, NULL);
	
	ctags_path = NULL;

	/* leaving here this code. Maybe in future ctags-devs will include our patches
	 * upstream and this can be useful again.
	 */
	if (ctags_path == NULL) 
	{
		DEBUG_PRINT ("ctags is not in preferences. Trying a default one %s", 
				   CTAGS_PATH);
		ctags_path = g_strdup (CTAGS_PATH);
	}
		
	sdb_plugin->buf_update_timeout_id = 0;
	sdb_plugin->need_symbols_update = FALSE;
	/* creates and start a new timer. */
	sdb_plugin->update_timer = g_timer_new ();

	/* these two arrays will maintain the same number of objects, 
	 * so that if you search, say on the first, an occurrence of a file,
	 * you'll be able to get in O(1) the _index in the second array, where the 
	 * scan process ids are stored. This is true in the other way too.
	 */
	sdb_plugin->buffer_update_files = g_ptr_array_new_with_free_func (g_free);
	sdb_plugin->buffer_update_ids = g_ptr_array_new ();
	sdb_plugin->buffer_update_semaphore = FALSE;
	
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
	if (symbol_db_engine_open_db (sdb_plugin->sdbe_globals, 
							  anjuta_cache_path, 
							  PROJECT_GLOBALS) == DB_OPEN_STATUS_FATAL)
	{
		g_error ("Opening global project under %s", anjuta_cache_path);
	}
	
	g_free (anjuta_cache_path);

	sdb_plugin->global_scan_aqueue = g_async_queue_new ();
	/* create the object that'll manage the globals population */
	sdb_plugin->sdbs = symbol_db_system_new (sdb_plugin, sdb_plugin->sdbe_globals);
#if 0
	g_signal_connect (G_OBJECT (sdb_plugin->sdbs), "scan-package-start",
					  G_CALLBACK (on_system_scan_package_start), plugin);	
	
	g_signal_connect (G_OBJECT (sdb_plugin->sdbs), "scan-package-end",
					  G_CALLBACK (on_system_scan_package_end), plugin);	
	
	g_signal_connect (G_OBJECT (sdb_plugin->sdbs), "single-file-scan-end",
					  G_CALLBACK (on_system_single_file_scan_end), plugin);	
#endif	
	/* beign necessary to listen to many scan-end signals, we'll build up a method
	 * to manage them with just one signal connection
	 */
	sdb_plugin->proc_id_tree = g_tree_new_full ((GCompareDataFunc)&symbol_db_gtree_compare_func, 
										 NULL,
										 NULL,
										 NULL);
	
	g_signal_connect (G_OBJECT (sdb_plugin->sdbe_project), "scan-end",
		  		G_CALLBACK (on_scan_end_manager), sdb_plugin);

	g_signal_connect (G_OBJECT (sdb_plugin->sdbe_project), "scan-end",
				G_CALLBACK (on_isymbol_manager_prj_scan_end), sdb_plugin);
	
	/* connect signals for interface to receive them */
	g_signal_connect (G_OBJECT (sdb_plugin->sdbe_globals), "single-file-scan-end",
				G_CALLBACK (on_isymbol_manager_sys_single_scan_end), sdb_plugin);	
	
	g_signal_connect (G_OBJECT (sdb_plugin->sdbe_globals), "scan-end",
				G_CALLBACK (on_isymbol_manager_sys_scan_end), sdb_plugin);

	g_signal_connect (G_OBJECT (sdb_plugin->sdbe_globals), "scan-begin",
				G_CALLBACK (on_isymbol_manager_sys_scan_begin), sdb_plugin);

	
	
	/* connect signals for project loading and element adding */
	g_signal_connect (G_OBJECT (pm), "element-added",
					  G_CALLBACK (on_project_element_added), sdb_plugin);
	g_signal_connect (G_OBJECT (pm), "element-removed",
					  G_CALLBACK (on_project_element_removed), sdb_plugin);
	g_signal_connect (G_OBJECT (pm), "project-loaded",
					  G_CALLBACK (on_project_loaded), sdb_plugin);
	
	/* Create widgets */
	sdb_plugin->dbv_main = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	sdb_plugin->dbv_notebook = gtk_notebook_new();
	gtk_notebook_set_show_border (GTK_NOTEBOOK (sdb_plugin->dbv_notebook), FALSE);
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (sdb_plugin->dbv_notebook), FALSE);
	sdb_plugin->dbv_hbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1);

	label = gtk_label_new (_("Symbols"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0f, 0.5f);
	gtk_box_pack_start (GTK_BOX(sdb_plugin->dbv_hbox), 
	                    gtk_image_new_from_stock ("symbol-db-plugin-icon",
	                                              GTK_ICON_SIZE_MENU),
	                    FALSE, FALSE, 0);	
	gtk_box_pack_start (GTK_BOX(sdb_plugin->dbv_hbox), label,
	                    FALSE, FALSE, 0);	

	sdb_plugin->tabber = anjuta_tabber_new (GTK_NOTEBOOK (sdb_plugin->dbv_notebook));
	label = gtk_label_new (_("File"));
	gtk_label_set_ellipsize (GTK_LABEL (label),
	                         PANGO_ELLIPSIZE_END);
	anjuta_tabber_add_tab (ANJUTA_TABBER (sdb_plugin->tabber),
	                       label);
	label = gtk_label_new (_("Project"));
	gtk_label_set_ellipsize (GTK_LABEL (label),
	                         PANGO_ELLIPSIZE_END);
	anjuta_tabber_add_tab (ANJUTA_TABBER (sdb_plugin->tabber),
	                       label);
	label = gtk_label_new (_("Search"));
	gtk_label_set_ellipsize (GTK_LABEL (label),
	                         PANGO_ELLIPSIZE_END);
	anjuta_tabber_add_tab (ANJUTA_TABBER (sdb_plugin->tabber),
	                       label);
	gtk_box_pack_end (GTK_BOX(sdb_plugin->dbv_hbox), sdb_plugin->tabber,
	                    TRUE, TRUE, 5);
	
	gtk_widget_show_all (sdb_plugin->dbv_hbox);
	
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
	view = symbol_db_view_new (SYMBOL_DB_VIEW_FILE, sdb_plugin->sdbe_project,
	                           sdb_plugin);
	gtk_notebook_append_page (GTK_NOTEBOOK (sdb_plugin->dbv_notebook),
							  view, gtk_label_new (_("Local")));
	sdb_plugin->file_model =
		gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_bin_get_child (GTK_BIN(view))));
	
	g_object_add_weak_pointer (G_OBJECT (sdb_plugin->file_model),
							   (gpointer)&sdb_plugin->file_model);
	
	/* Global symbols */
	view = symbol_db_view_new (SYMBOL_DB_VIEW_PROJECT,
	                           sdb_plugin->sdbe_project,
	                           sdb_plugin);
	gtk_notebook_append_page (GTK_NOTEBOOK (sdb_plugin->dbv_notebook),
							  view, gtk_label_new (_("Global" )));

	/* Search symbols */
	view = symbol_db_view_new (SYMBOL_DB_VIEW_SEARCH,
	                           sdb_plugin->sdbe_project,
	                           sdb_plugin);
	sdb_plugin->search_entry = symbol_db_view_get_search_entry (view);
	gtk_notebook_append_page (GTK_NOTEBOOK (sdb_plugin->dbv_notebook),
							  view, gtk_label_new (_("Search" )));

	gtk_widget_show_all (sdb_plugin->dbv_notebook);

	/* setting focus to the tree_view*/
	gtk_notebook_set_current_page (GTK_NOTEBOOK (sdb_plugin->dbv_notebook), 0);

	sdb_plugin->editor_watch_id = 
		anjuta_plugin_add_watch (plugin, IANJUTA_DOCUMENT_MANAGER_CURRENT_DOCUMENT,
								 value_added_current_editor,
								 value_removed_current_editor, NULL);
	/* Added widgets */
	anjuta_shell_add_widget_custom (plugin->shell, sdb_plugin->dbv_main,
	                                "AnjutaSymbolDB", _("Symbols"),
	                                "symbol-db-plugin-icon",
	                                sdb_plugin->dbv_hbox,
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

	/* be sure to hide the progress bars in case no project has been opened. */
	gtk_widget_hide (sdb_plugin->progress_bar_project);
	gtk_widget_hide (sdb_plugin->progress_bar_system);	
	
	static IAnjutaSymbolField search_fields[] =
	{
		IANJUTA_SYMBOL_FIELD_KIND,
		IANJUTA_SYMBOL_FIELD_FILE_PATH,
		IANJUTA_SYMBOL_FIELD_FILE_POS
	};
	sdb_plugin->search_query =
		ianjuta_symbol_manager_create_query (IANJUTA_SYMBOL_MANAGER (sdb_plugin),
		                                     IANJUTA_SYMBOL_QUERY_SEARCH,
		                                     IANJUTA_SYMBOL_QUERY_DB_PROJECT,
		                                     NULL);
	ianjuta_symbol_query_set_fields (sdb_plugin->search_query,
	                                 G_N_ELEMENTS (search_fields),
	                                 search_fields, NULL);
	return TRUE;
}

static gboolean
symbol_db_deactivate (AnjutaPlugin *plugin)
{
	SymbolDBPlugin *sdb_plugin;
	IAnjutaProjectManager *pm;
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (plugin);
		
	pm = anjuta_shell_get_interface (ANJUTA_PLUGIN (sdb_plugin)->shell,
									 IAnjutaProjectManager, NULL);	
									 	


	DEBUG_PRINT ("%s", "SymbolDBPlugin: Dectivating SymbolDBPlugin plugin …");

	/* Unmerge UI */
	gtk_ui_manager_remove_ui (GTK_UI_MANAGER (sdb_plugin->ui),
	    						sdb_plugin->merge_id);
	gtk_ui_manager_remove_action_group (GTK_UI_MANAGER (sdb_plugin->ui),
	    							sdb_plugin->popup_action_group);
	gtk_ui_manager_remove_action_group (GTK_UI_MANAGER (sdb_plugin->ui),
	    							sdb_plugin->menu_action_group);
	g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->shell),
										  on_session_load,
										  plugin);

	g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->shell),
										  on_session_save,
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

	/* disconnect the interface ones */
	/* connect signals for interface to receive them */
	g_signal_handlers_disconnect_by_func (G_OBJECT (sdb_plugin->sdbe_globals),
				G_CALLBACK (on_isymbol_manager_sys_scan_end), plugin);
	
	g_signal_handlers_disconnect_by_func (G_OBJECT (sdb_plugin->sdbe_project),
				G_CALLBACK (on_isymbol_manager_prj_scan_end), plugin);

	g_signal_handlers_disconnect_by_func (G_OBJECT (pm),
	    		G_CALLBACK (on_project_element_added), plugin);

	g_signal_handlers_disconnect_by_func (G_OBJECT (pm),
	    		G_CALLBACK (on_project_element_removed), plugin);

	g_signal_handlers_disconnect_by_func (G_OBJECT (pm),
	    		G_CALLBACK (on_project_loaded), plugin);
	
	if (sdb_plugin->update_timer)
	{
		g_timer_destroy (sdb_plugin->update_timer);
		sdb_plugin->update_timer = NULL;
	}

	/* destroy search query */
	if (sdb_plugin->search_query)
	{
		g_object_unref (sdb_plugin->search_query);
	}
	sdb_plugin->search_query = NULL;
	
	/* destroy objects */
	if (sdb_plugin->sdbe_project) 
	{
		DEBUG_PRINT ("Destroying project engine object. ");
		g_object_unref (sdb_plugin->sdbe_project);
	}
	sdb_plugin->sdbe_project = NULL;

	PackageScanData *pkg_scan_data;
	while ((pkg_scan_data = g_async_queue_try_pop (sdb_plugin->global_scan_aqueue)) != NULL)
	{
		g_free (pkg_scan_data->package_name);
		g_free (pkg_scan_data->package_version);
		g_free (pkg_scan_data);		
	}
	
	g_async_queue_unref (sdb_plugin->global_scan_aqueue);
	sdb_plugin->global_scan_aqueue = NULL;
	
	/* this must be done *before* destroying sdbe_globals */
	g_object_unref (sdb_plugin->sdbs);
	sdb_plugin->sdbs = NULL;
	
	g_free (sdb_plugin->current_scanned_package);
	sdb_plugin->current_scanned_package = NULL;

	DEBUG_PRINT ("Destroying global engine object. ");
	g_object_unref (sdb_plugin->sdbe_globals);
	sdb_plugin->sdbe_globals = NULL;
	
	g_free (sdb_plugin->project_opened);
	sdb_plugin->project_opened = NULL;

	if (sdb_plugin->buffer_update_files)
	{
		g_ptr_array_unref (sdb_plugin->buffer_update_files);
		sdb_plugin->buffer_update_files = NULL;
	}

	if (sdb_plugin->buffer_update_ids)
	{
		g_ptr_array_unref (sdb_plugin->buffer_update_ids);
		sdb_plugin->buffer_update_ids = NULL;		
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
	anjuta_shell_remove_widget (plugin->shell, sdb_plugin->dbv_main, NULL);

	sdb_plugin->root_watch_id = 0;
	sdb_plugin->editor_watch_id = 0;
	sdb_plugin->merge_id = 0;
	sdb_plugin->dbv_notebook = NULL;
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
	SymbolDBPlugin *plugin = (SymbolDBPlugin*)obj;
	DEBUG_PRINT ("Symbol-DB dispose");
	/* Disposition codes */
	g_object_unref (plugin->settings);

	
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
	plugin->settings = g_settings_new (PREF_SCHEMA);
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

static void
on_prefs_buffer_update_toggled (GtkToggleButton* button,
							  	gpointer user_data)
{
	SymbolDBPlugin *sdb_plugin;
	gboolean sensitive;
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (user_data);
	
	sensitive = gtk_toggle_button_get_active (button);

	DEBUG_PRINT ("on_prefs_buffer_update_toggled () %d", sensitive);
	
	if (sensitive == FALSE)
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
	GtkWidget *buf_up_widget;
	GError* error = NULL;
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (ipref);

	if (sdb_plugin->prefs_bxml == NULL)
	{	
		/* Create the preferences page */
		sdb_plugin->prefs_bxml = gtk_builder_new ();
		if (!gtk_builder_add_from_file (sdb_plugin->prefs_bxml, BUILDER_FILE, &error))
		{
			g_warning ("Couldn't load builder file: %s", error->message);
			g_error_free(error);
		}		
	}

	anjuta_preferences_add_from_builder (prefs,
	                                     sdb_plugin->prefs_bxml,
	                                     sdb_plugin->settings,
	                                     BUILDER_ROOT, 
	                                     _("Symbol Database"),  
	                                     ICON_FILE);

	buf_up_widget = GTK_WIDGET (gtk_builder_get_object (sdb_plugin->prefs_bxml, 
	    PREFS_BUFFER_UPDATE));
	
	g_signal_connect (buf_up_widget, "toggled",
					  G_CALLBACK (on_prefs_buffer_update_toggled),
					  sdb_plugin);
}

static void
ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	SymbolDBPlugin *sdb_plugin;
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (ipref);

	anjuta_preferences_remove_page(prefs, _("Symbol Database"));
	g_object_unref (sdb_plugin->prefs_bxml);
	sdb_plugin->prefs_bxml = NULL;
}

static void
ipreferences_iface_init(IAnjutaPreferencesIface* iface)
{
	DEBUG_PRINT ("%s", "SymbolDB: ipreferences_iface_init");	
	iface->merge = ipreferences_merge;
	iface->unmerge = ipreferences_unmerge;	
}

/* IAnjutaSymbolManager implementation */
static IAnjutaSymbolQuery*
isymbol_manager_create_query (IAnjutaSymbolManager *isymbol_manager,
                              IAnjutaSymbolQueryName query_name,
                              IAnjutaSymbolQueryDb db,
                              GError **err)
{
	SymbolDBPlugin *sdb_plugin;
	SymbolDBQuery *query;

	g_return_val_if_fail (isymbol_manager != NULL, NULL);
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (isymbol_manager);
	
	query = symbol_db_query_new (sdb_plugin->sdbe_globals,
	                             sdb_plugin->sdbe_project, 
	                             query_name, 
	                             db,
	                             db == IANJUTA_SYMBOL_QUERY_DB_PROJECT ? 
	                         			NULL : /* FIXME */ NULL);
	return IANJUTA_SYMBOL_QUERY (query);
}

static gboolean
isymbol_manager_add_package (IAnjutaSymbolManager *isymbol_manager,
    						 const gchar* pkg_name, 
    						 const gchar* pkg_version, 
    						 GList* files,
    						 GError **err)
{
	SymbolDBPlugin *sdb_plugin;
	IAnjutaLanguage *lang_manager;
	GPtrArray *files_array;
	PackageScanData *pkg_scan_data;
	

	g_return_val_if_fail (isymbol_manager != NULL, FALSE);

	/* DEBUG */
	GList *node;
	node = files;
	while (node != NULL)
	{
		node = node->next;
	}
	
	/*  FIXME: pkg_version comes with \n at the end. This should be avoided */
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (isymbol_manager);
	lang_manager = anjuta_shell_get_interface (ANJUTA_PLUGIN (sdb_plugin)->shell, IAnjutaLanguage, 
										NULL);	

	if (symbol_db_engine_add_new_project (sdb_plugin->sdbe_globals, NULL, pkg_name, 
	    pkg_version) == FALSE)
	{
		return FALSE;
	}

	files_array = anjuta_util_convert_string_list_to_array (files);

	pkg_scan_data = g_new0 (PackageScanData, 1);
	
	g_async_queue_push (sdb_plugin->global_scan_aqueue, pkg_scan_data);
	pkg_scan_data->files_length = g_list_length (files);
	pkg_scan_data->package_name = g_strdup (pkg_name);
	pkg_scan_data->package_version = g_strdup (pkg_version);
	
	pkg_scan_data->proc_id = symbol_db_engine_add_new_files_async (sdb_plugin->sdbe_globals, lang_manager, 
	    pkg_name, pkg_version, files_array);

	g_ptr_array_unref (files_array);
	
	return TRUE;
}

/* FIXME: do this thread safe */
static gboolean
isymbol_manager_activate_package (IAnjutaSymbolManager *isymbol_manager,
    							  const gchar *pkg_name, 
    							  const gchar *pkg_version,
    							  GError **err)
{
	SymbolDBPlugin *sdb_plugin;

	g_return_val_if_fail (isymbol_manager != NULL, FALSE);
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (isymbol_manager);

	if (symbol_db_engine_project_exists (sdb_plugin->sdbe_globals, pkg_name, 
	    								 pkg_version) == TRUE)
	{
		/* FIXME: Activate package in database */
		DEBUG_PRINT ("STUB");
		return TRUE;
	}

	/* user should add a package before activating it. */
	return FALSE;
}

static void
isymbol_manager_deactivate_package (IAnjutaSymbolManager *isymbol_manager,
                                    const gchar *pkg_name, 
                                    const gchar *pkg_version,
                                    GError **err)
{
	SymbolDBPlugin *sdb_plugin;

	g_return_if_fail (isymbol_manager != NULL);
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (isymbol_manager);

	if (symbol_db_engine_project_exists (sdb_plugin->sdbe_globals, pkg_name, 
	    								 pkg_version) == TRUE)
	{
		DEBUG_PRINT ("STUB");
		/* FIXME: deactivate package in database */
	}
}

static void
isymbol_manager_deactivate_all (IAnjutaSymbolManager *isymbol_manager,
                                GError **err)
{
#if 0	
	SymbolDBPlugin *sdb_plugin;

	g_return_if_fail (isymbol_manager != NULL);
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (isymbol_manager);
#endif
	/* FIXME: deactivate all packages in database */
	DEBUG_PRINT ("STUB");
}

static void
isymbol_manager_iface_init (IAnjutaSymbolManagerIface *iface)
{
	iface->create_query = isymbol_manager_create_query;
	iface->activate_package = isymbol_manager_activate_package;
	iface->deactivate_package = isymbol_manager_deactivate_package;
	iface->deactivate_all = isymbol_manager_deactivate_all;
	iface->add_package = isymbol_manager_add_package;
}

ANJUTA_PLUGIN_BEGIN (SymbolDBPlugin, symbol_db);
ANJUTA_PLUGIN_ADD_INTERFACE (isymbol_manager, IANJUTA_TYPE_SYMBOL_MANAGER);
ANJUTA_PLUGIN_ADD_INTERFACE(ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (SymbolDBPlugin, symbol_db);
