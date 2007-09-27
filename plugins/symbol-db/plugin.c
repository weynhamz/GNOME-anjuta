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
#define ICON_FILE "symbol-db.png"

#define TIMEOUT_INTERVAL_SYMBOLS_UPDATE		60000
#define TIMEOUT_SECONDS_AFTER_LAST_TIP		5

static gpointer parent_class;
static gboolean need_symbols_update = FALSE;
static gchar prev_char_added = ' ';
static gint timeout_id = 0;
static GTimer *timer = NULL;

#define REGISTER_ICON(icon, stock_id) \
	pixbuf = gdk_pixbuf_new_from_file (icon, NULL); \
	icon_set = gtk_icon_set_new_from_pixbuf (pixbuf); \
	gtk_icon_factory_add (icon_factory, stock_id, icon_set); \
	g_object_unref (pixbuf);

static void
register_stock_icons (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	GtkIconFactory *icon_factory;
	GtkIconSet *icon_set;
	GdkPixbuf *pixbuf;
	static gboolean registered = FALSE;

	if (registered)
		return;
	registered = TRUE;

	/* Register stock icons */
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	icon_factory = anjuta_ui_get_icon_factory (ui);
	REGISTER_ICON (PACKAGE_PIXMAPS_DIR"/"ICON_FILE, "symbol-db-plugin-icon");
}
/*
static GtkActionEntry actions[] = 
{
	{ "ActionMenuGoto", NULL, N_("_Goto"), NULL, NULL, NULL},
	{
		"ActionSymbolBrowserGotoDef",
		NULL,
		N_("Tag _Definition"),
		"<control>d",
		N_("Goto symbol definition"),
		G_CALLBACK (on_goto_file_tag_def_activate)
	},
	{
		"ActionSymbolBrowserGotoDecl",
		NULL,
		N_("Tag De_claration"),
		"<shift><control>d",
		N_("Goto symbol declaration"),
		G_CALLBACK (on_goto_file_tag_decl_activate)
	}
};
*/

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
	
	if (!sdb_plugin->editor_connected || !sdb_plugin->dbv_view_tree)
		return;
	
	uri = g_hash_table_lookup (sdb_plugin->editor_connected, G_OBJECT (editor));
	if (uri && strlen (uri) > 0)
	{
		DEBUG_PRINT ("Removing file tags of %s", uri);
/*		
		anjuta_symbol_view_workspace_remove_file (ANJUTA_SYMBOL_VIEW (sv_plugin->sv_tree),
											   uri);
*/		
	}
	g_hash_table_remove (sdb_plugin->editor_connected, G_OBJECT (editor));
}

static void
on_editor_update_ui (IAnjutaEditor *editor, SymbolDBPlugin *sdb_plugin) 
{
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

	symbol_db_view_locals_update_list (
				SYMBOL_DB_VIEW_LOCALS (sdb_plugin->dbv_view_tree_locals),
				 sdb_plugin->sdbe, local_path);

	if (g_hash_table_lookup (sdb_plugin->editor_connected, editor) == NULL)
	{
		g_object_weak_ref (G_OBJECT (editor),
						   (GWeakNotify) (on_editor_destroy),
						   sdb_plugin);
//		if (uri)
//		{
			g_hash_table_insert (sdb_plugin->editor_connected, editor,
								 g_strdup (uri));
/*		}
		else
		{
			g_hash_table_insert (sdb_plugin->editor_connected, editor,
								 g_strdup (""));
		}
*/
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
		
	uri = g_strdup_printf ("file:///%s#%d", filename, lineno);
	ianjuta_file_loader_load (loader, uri, FALSE, NULL);
	g_free (uri);
}


static void
goto_tree_iter (SymbolDBPlugin *sdb_plugin, GtkTreeIter *iter)
{
	gint line;

	line = symbol_db_view_locals_get_line (SYMBOL_DB_VIEW_LOCALS (
									sdb_plugin->dbv_view_tree_locals), iter);

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

/**
 * will manage the click of mouse and other events on search->hitlist treeview
 */
static void
on_treesearch_symbol_selected_event (SymbolDBViewSearch *search,
									 gint line,
									 gchar* file,
									 SymbolDBPlugin *sdb_plugin) {
										 
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
	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
		return;
	}
	goto_tree_iter (sdb_plugin, &iter);
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
						  SymbolDBPlugin *sv_plugin)
{
	gchar *filename;

	g_return_if_fail (sv_plugin->project_root_uri != NULL);
	g_return_if_fail (sv_plugin->project_root_dir != NULL);
	
	DEBUG_PRINT ("on_project_element_added");	
	
	filename = gnome_vfs_get_local_path_from_uri (uri);
	if (filename)
	{
		GPtrArray *files_array;
		const gchar* file_mime;
		IAnjutaLanguageId lang_id;
		const gchar* lang;
		IAnjutaLanguage* lang_manager =
						anjuta_shell_get_interface (ANJUTA_PLUGIN(sv_plugin)->shell, IAnjutaLanguage, NULL);
		file_mime = gnome_vfs_get_mime_type_for_name (filename);
		
		lang_id = ianjuta_language_get_from_mime_type (lang_manager, file_mime, NULL);
		
		/* No supported language... */
		if (!lang_id)
		{
			g_free (filename);
			return;
		}
		
		lang = ianjuta_language_get_name (lang_manager, lang_id, NULL);
		files_array = g_ptr_array_new();
		g_ptr_array_add (files_array, filename);
		DEBUG_PRINT ("gonna opening %s", 
					 filename + strlen(sv_plugin->project_root_dir) );
		DEBUG_PRINT ("poject_root_dir %s", sv_plugin->project_root_dir );
		
		symbol_db_engine_add_new_files (sv_plugin->sdbe, 
			sv_plugin->project_root_dir, files_array, lang, TRUE);
		
		g_free (filename);
		g_ptr_array_free (files_array, TRUE);
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
		DEBUG_PRINT ("poject_root_dir %s", sv_plugin->project_root_dir );
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

typedef struct
{
	SymbolDBEngine* sdbe;
	const gchar* root_dir;
} SourcesForeachData;

static void
sources_array_add_foreach (gpointer key, gpointer value, gpointer user_data)
{
	const gchar* lang = (const gchar*) key;
	GPtrArray* sources_array = (GPtrArray*) value;
	SourcesForeachData* data = (SourcesForeachData*) user_data;
	
	if (symbol_db_engine_add_new_files (data->sdbe, data->root_dir,
									sources_array, lang, TRUE) == FALSE) 
	{
		DEBUG_PRINT ("__FUNCTION__ failed for lang %s, root_dir %s",
					 lang, data->root_dir);
	}
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
			gint i;
			GHashTable* lang_hash = 
				g_hash_table_new_full (g_str_hash, g_str_equal, NULL, sources_array_free);
			
			status = anjuta_shell_get_status (plugin->shell, NULL);
			anjuta_status_progress_add_ticks (status, 1);
	
			DEBUG_PRINT ("Symbol-DB: creating new project.");

			/* is it a fresh-new project? is it an imported project with 
			 * no 'new' symbol-db database but the 'old' one symbol-browser? 
			 */
			if (symbol_db_engine_db_exists (sdb_plugin->sdbe, root_dir) == FALSE)
			{
				needs_sources_scan = TRUE;
			}
			
			if (symbol_db_engine_open_db (sdb_plugin->sdbe, root_dir) == FALSE)
				g_error ("error in opening db");
			
			symbol_db_engine_add_new_project (sdb_plugin->sdbe,
											  NULL,	/* still no workspace */
											  root_dir);
			
			/* open the project. we can do this only if the db was loaded */
			if (symbol_db_engine_open_project (sdb_plugin->sdbe, root_dir) == FALSE)
				g_warning ("error in opening project");
			
			if (needs_sources_scan == TRUE)
			{
				GList* prj_elements_list;
				
				DEBUG_PRINT ("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
				DEBUG_PRINT ("Retrieving gbf sources of the project...");
				
				prj_elements_list = ianjuta_project_manager_get_elements (pm,
								   IANJUTA_PROJECT_MANAGER_SOURCE,
								   NULL);
			
				GPtrArray* sources_array;
				
				for (i=0; i < g_list_length (prj_elements_list); i++) 
				{	
					gchar *local_filename;
					const gchar *file_mime;
					const gchar *lang;
					IAnjutaLanguageId lang_id;
					IAnjutaLanguage* lang_manager =
						anjuta_shell_get_interface (plugin->shell, IAnjutaLanguage, NULL);
					
					if (!lang_manager)
					{
						g_critical ("LanguageManager not found");
						return;
					}
					
					local_filename = 
						gnome_vfs_get_local_path_from_uri (g_list_nth_data (
															prj_elements_list, i));
					file_mime = gnome_vfs_get_mime_type_for_name (local_filename);
					
					lang_id = ianjuta_language_get_from_mime_type (lang_manager, file_mime, NULL);
					
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
					
					sources_array = g_hash_table_lookup (lang_hash, lang);
					if (!sources_array)
					{
						sources_array = g_ptr_array_new ();
						g_hash_table_insert (lang_hash, (gpointer) lang, sources_array);
					}	
					
					g_ptr_array_add (sources_array, local_filename);
				}
			
				DEBUG_PRINT ("calling symbol_db_engine_add_new_files  with root_dir %s",
							 root_dir);
				
				SourcesForeachData* data = g_new0 (SourcesForeachData, 1);
				data->sdbe = sdb_plugin->sdbe;
				data->root_dir = root_dir;
				
				g_hash_table_foreach (lang_hash, (GHFunc) sources_array_add_foreach,
									  data);
				
				g_free (data);
				g_hash_table_unref (lang_hash);
			}
			
			anjuta_status_progress_tick (status, NULL, _("Created symbols..."));
			
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

	/* don't forget to close the project */
	symbol_db_engine_close_project (sdb_plugin->sdbe, 
									sdb_plugin->project_root_dir);

	g_free (sdb_plugin->project_root_uri);
	g_free (sdb_plugin->project_root_dir);
	sdb_plugin->project_root_uri = NULL;
	sdb_plugin->project_root_dir = NULL;
}

static gboolean
symbol_db_activate (AnjutaPlugin *plugin)
{
	SymbolDBPlugin *symbol_db;
	
	DEBUG_PRINT ("SymbolDBPlugin: Activating SymbolDBPlugin plugin ...");
	

	register_stock_icons (plugin);
	
	symbol_db = ANJUTA_PLUGIN_SYMBOL_DB (plugin);
	symbol_db->ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
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

	g_object_add_weak_pointer (G_OBJECT (symbol_db->dbv_view_tree_locals),
							   (gpointer)&symbol_db->dbv_view_tree_locals);
	g_signal_connect (G_OBJECT (symbol_db->dbv_view_tree_locals), "row_activated",
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
#if 0
	g_signal_connect (G_OBJECT (symbol_db->dbv_view_tree), "event-after",
					  G_CALLBACK (on_treeview_event), plugin);

	g_signal_connect (G_OBJECT (symbol_db->dbv_view_tree), "row_activated",
					  G_CALLBACK (on_treeview_row_activated), plugin);
#endif	
	gtk_container_add (GTK_CONTAINER(symbol_db->scrolled_global), 
					   symbol_db->dbv_view_tree);
	
	/* Search symbols */
	symbol_db->dbv_view_tree_search =
		(GtkWidget*) symbol_db_view_search_new (symbol_db->sdbe);
	symbol_db->dbv_view_search_tab_label = gtk_label_new (_("Search" ));

	g_signal_connect (G_OBJECT (symbol_db->dbv_view_tree_search), "symbol_selected",
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
							 "SymbolDBView", _("Symbols"),
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

#if 0			
	/* Remove UI */
	anjuta_ui_unmerge (sdb_plugin->ui, sdb_plugin->merge_id);

	/* Remove action group */
	anjuta_ui_remove_action_group (sv_plugin->ui, sv_plugin->action_group);
	anjuta_ui_remove_action_group (sv_plugin->ui, sv_plugin->popup_action_group);
	anjuta_ui_remove_action_group (sdb_plugin->ui, sdb_plugin->action_group_nav);	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
/*	anjuta_ui_remove_action_group (ui, ((SymbolDBPlugin*)plugin)->action_group); */
	anjuta_ui_unmerge (ui, ((SymbolDBPlugin*)plugin)->uiid);
#endif			
	
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

	DEBUG_PRINT ("called isymbol_manager_search()! %s", match_name);
	
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
#if 0	
	const GPtrArray *tags_array;
	AnjutaSymbolIter *iter = NULL;
	
	tags_array = tm_workspace_find_scope_members (NULL, symbol_name,
												  global_search, TRUE);
												  
	
	if (tags_array)
	{
		iter = anjuta_symbol_iter_new (tags_array);
		return IANJUTA_ITERABLE (iter);
	}
#endif	
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
#if 0
	SymbolBrowserPlugin *sv_plugin;
	const TMTag *func_scope_tag;
	TMSourceFile *tm_file;
	IAnjutaEditor *ed;
	AnjutaSymbolView *symbol_view;
	gulong line;
	gulong scope_position;
	gchar *needed_text = NULL;
	gint access_method;
	GPtrArray * completable_tags_array;
	AnjutaSymbolIter *iter = NULL;
	
	sv_plugin = ANJUTA_PLUGIN_SYMBOL_BROWSER (sm);
	ed = IANJUTA_EDITOR (sv_plugin->current_editor);	
	symbol_view = ANJUTA_SYMBOL_VIEW (sv_plugin->sv_tree);
	
	line = ianjuta_editor_get_line_from_position (ed, text_pos, NULL);
	
	/* get the function scope */
	tm_file = anjuta_symbol_view_get_tm_file (symbol_view, file_uri);
	
	/* check whether the current file_uri is listed in the tm_workspace or not... */	
	if (tm_file == NULL)
		return 	NULL;
		

	// FIXME: remove DEBUG_PRINT	
/*/
	DEBUG_PRINT ("tags in file &s\n");
	if (tm_file->work_object.tags_array != NULL) {
		int i;
		for (i=0; i < tm_file->work_object.tags_array->len; i++) {
			TMTag *cur_tag;
		
			cur_tag = (TMTag*)g_ptr_array_index (tm_file->work_object.tags_array, i);
			tm_tag_print (cur_tag, stdout);
		}
	}
/*/

	func_scope_tag = tm_get_current_function (tm_file->work_object.tags_array, line);
		
	if (func_scope_tag == NULL) {
		DEBUG_PRINT ("func_scope_tag is NULL, seems like it's a completion on a global scope");
		return NULL;
	}
	
	DEBUG_PRINT ("current expression scope: %s", func_scope_tag->name);
	
	
	scope_position = ianjuta_editor_get_line_begin_position (ed, func_scope_tag->atts.entry.line, NULL);
	needed_text = ianjuta_editor_get_text (ed, scope_position,
										   text_pos - scope_position, NULL);

	if (needed_text == NULL)
		DEBUG_PRINT ("needed_text is null");
	DEBUG_PRINT ("text needed is %s", needed_text );
	

	/* we'll pass only the text of the current scope: i.e. only the current function
	 * in which we request the completion. */
	TMTag * found_type = anjuta_symbol_view_get_type_of_expression (symbol_view, 
				needed_text, text_pos - scope_position, func_scope_tag, &access_method);

				
	if (found_type == NULL) {
		DEBUG_PRINT ("type not found.");
		return NULL;	
	}
	
	/* get the completable memebers. If the access is COMPLETION_ACCESS_STATIC we don't
	 * want to know the parents members of the class.
	 */
	if (access_method == COMPLETION_ACCESS_STATIC)
		completable_tags_array = anjuta_symbol_view_get_completable_members (found_type, FALSE);
	else
		completable_tags_array = anjuta_symbol_view_get_completable_members (found_type, TRUE);
	
	if (completable_tags_array)
	{
		iter = anjuta_symbol_iter_new (completable_tags_array);
		return IANJUTA_ITERABLE (iter);
	}
#endif
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
