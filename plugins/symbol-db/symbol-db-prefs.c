/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * symbol-db-prefs.c
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
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkliststore.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-launcher.h>
#include <libanjuta/interfaces/ianjuta-language.h>

#include "symbol-db-prefs.h"

#define GLADE_FILE PACKAGE_DATA_DIR"/glade/anjuta-symbol-db.glade"
#define GLADE_ROOT "symbol_prefs"
#define ICON_FILE "anjuta-symbol-db-plugin-48.png"

#define CTAGS_PREFS_KEY		"ctags.executable"
#define CTAGS_PATH			"/usr/bin/ctags"
#define CHOOSER_WIDGET		"preferences_folder:text:/:0:symboldb.root"

static AnjutaLauncher* cflags_launcher = NULL;
static GList *pkg_list = NULL;
static gboolean initialized = FALSE;

enum
{
	COLUMN_LOAD,
	COLUMN_NAME,
	COLUMN_FLAGS_LIST,
	COLUMN_MAX
};

typedef struct _CflagsData {
	SymbolDBPlugin *sdb_plugin;
	GList *item;
} CflagsData;

static void 
on_prefs_executable_changed (GtkFileChooser *chooser,
                             gpointer user_data)
{
	gchar *new_file;
	
	new_file = gtk_file_chooser_get_filename (chooser);
	DEBUG_PRINT ("on_prefs_executable_changed ()");
	DEBUG_PRINT ("new file selected %s", new_file);
	anjuta_preferences_set (ANJUTA_PREFERENCES (user_data), CTAGS_PREFS_KEY,
							new_file);
	
	g_free (new_file);
}

static void
on_gconf_notify_prefs (GConfClient *gclient, guint cnxn_id,
					   GConfEntry *entry, gpointer user_data)
{
	DEBUG_PRINT ("on_gconf_notify_prefs ()");
}

static gint 
pkg_list_compare (gconstpointer a, gconstpointer b)
{
	return strcmp ((const gchar*)a, (const gchar*)b);
}

static void
on_cflags_output (AnjutaLauncher * launcher,
					AnjutaLauncherOutputType output_type,
					const gchar * chars, gpointer user_data)
{
	SymbolDBPlugin *sdb_plugin;
	GtkListStore *store;
	GList *item;
	CflagsData *cdata;
	gchar **flags;
	const gchar *curr_flag;
	gint i;
	GList *good_flags;
	
	cdata = (CflagsData*)user_data;
	item = cdata->item;		
	sdb_plugin = cdata->sdb_plugin;
	store = sdb_plugin->prefs_list_store;
	
/*	DEBUG_PRINT ("on_cflags_output for item %s ->%s<-", item->data, chars);*/
	
	if (output_type == ANJUTA_LAUNCHER_OUTPUT_STDERR)
	{
		/* no way. We don't like errors on stderr... */
		return;
	}
	
	/* We should receive here something like 
	* '-I/usr/include/gimp-2.0 -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include'.
	* Split up the chars and take a decision if we like it or not.
	*/
	flags = g_strsplit (chars, " ", -1);

	i = 0;
	/* if, after the while loop, good_flags is != NULL that means that we found
	 * some good flags to include for a future scan 
	 */
	good_flags = NULL;
	while ((curr_flag = flags[i++]) != NULL)
	{
		/* '-I/usr/include/gimp-2.0' would be good, but '/usr/include/' wouldn't. */
		if (g_regex_match_simple ("\\.*/usr/include/\\w+", curr_flag, 0, 0) == TRUE)
		{
			/* FIXME the +2. It's to skip the -I */
			DEBUG_PRINT ("adding %s to good_flags", curr_flag +2);
			/* FIXME the +2. It's to skip the -I */
			good_flags = g_list_prepend (good_flags, g_strdup (curr_flag + 2));
		}
	}	

	g_strfreev (flags);

	if (good_flags != NULL)
	{
		GtkTreeIter iter;
		/* that's good. We can add the package to the GtkListStore */
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, COLUMN_LOAD, FALSE,
										COLUMN_NAME, g_strdup (item->data), 
										COLUMN_FLAGS_LIST, good_flags, -1);
	}
}

static void
on_cflags_exit (AnjutaLauncher * launcher, int child_pid,
				   int exit_status, gulong time_taken_in_seconds,
				   gpointer user_data)
{
	CflagsData *cdata;
	GList *item;	
	gchar *exe_string;	
	SymbolDBPlugin *sdb_plugin;
	
	cdata = (CflagsData*)user_data;
	item = cdata->item;
	sdb_plugin = cdata->sdb_plugin;
	
	/* select next item in the list. If it's null then free everything */
	item = item->next;
	if (item == NULL)
	{
		g_object_unref (cflags_launcher);
		cflags_launcher = NULL;
		g_free (cdata);
		DEBUG_PRINT ("reached end of packages list");
		return;
	}

	if (cflags_launcher)
	{
		/* recreate anjuta_launcher because.. well.. it closes stdout pipe
		 * and stderr. There's no way to reactivate them 
		 */
		g_object_unref (cflags_launcher);
		cflags_launcher = anjuta_launcher_new ();
	}

	
	DEBUG_PRINT ("package for CFLAGS %s", (gchar*)item->data);


	/* reuse CflagsData object to store the new glist pointer */
	cdata->item = item;
	/* sdb_plugin remains the same */
	
	g_signal_connect (G_OBJECT (cflags_launcher), "child-exited",
				  G_CALLBACK (on_cflags_exit), cdata);	
	
	exe_string = g_strdup_printf ("pkg-config --cflags %s", (gchar*)item->data);
	DEBUG_PRINT ("launching exe_string  %s", exe_string );
	
	anjuta_launcher_execute (cflags_launcher,
						 exe_string, on_cflags_output, 
						 cdata);
		
	g_free (exe_string);
}

static void
on_listall_output (AnjutaLauncher * launcher,
					AnjutaLauncherOutputType output_type,
					const gchar * chars, gpointer user_data)
{
	if (output_type == ANJUTA_LAUNCHER_OUTPUT_STDERR)
		return;
	
	/* there's no way to avoid getting stderr here. */
	DEBUG_PRINT ("chars %s", chars);
	gchar **lines;
	const gchar *curr_line;
	gint i = 0;
	SymbolDBPlugin *sdb_plugin;
	GtkListStore *store;
		
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (user_data);
	
	store = sdb_plugin->prefs_list_store;
	
	lines = g_strsplit (chars, "\n", -1);
	
	while ((curr_line = lines[i++]) != NULL)
	{
		gchar **pkgs;
		
		pkgs = g_strsplit (curr_line, " ", -1);
		
		/* just take the first one as it's the package-name */
		if (pkgs == NULL)
			return;		
		
		if (pkgs[0] == NULL) {
			g_strfreev (pkgs);
			continue;
		}
		DEBUG_PRINT ("pkg inserted into pkg_list is %s", pkgs[0]);		
/*		
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, COLUMN_LOAD, FALSE,
									COLUMN_NAME, pkgs[0], 
									COLUMN_FLAGS_LIST, NULL, -1);
*/		
		pkg_list = g_list_prepend (pkg_list, g_strdup (pkgs[0]));
		g_strfreev (pkgs);
	}

	g_strfreev (lines);	
}

static void
on_listall_exit (AnjutaLauncher * launcher, int child_pid,
				   int exit_status, gulong time_taken_in_seconds,
				   gpointer user_data)
{	
	SymbolDBPlugin *sdb_plugin;
	CflagsData *cdata;
	gchar *exe_string;
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (user_data);
	

	DEBUG_PRINT ("launcher ended");
	/* we should have pkg_list filled with packages names 
	 * It's not enough anyway: we have to 1. sort alphabetically the list first
	 * then 2. check for Iflags [pkg-config --cflags] if the regex returns void 
	 * then kill the package from the list
	 */
		
	if (pkg_list == NULL)
	{
		g_warning ("No packages found");
		return;
	}

	pkg_list = g_list_sort (pkg_list, pkg_list_compare);

	if (cflags_launcher == NULL)
		cflags_launcher = anjuta_launcher_new ();
	
	cdata = g_new0 (CflagsData, 1);
	
	cdata->sdb_plugin = sdb_plugin;
	cdata->item = pkg_list;
	
	DEBUG_PRINT ("package for CFLAGS %s", (gchar*)pkg_list->data);

	g_signal_connect (G_OBJECT (cflags_launcher), "child-exited",
				  G_CALLBACK (on_cflags_exit), cdata);	
	
	exe_string = g_strdup_printf ("pkg-config --cflags %s", (gchar*)pkg_list->data);
	
	anjuta_launcher_execute (cflags_launcher,
						 exe_string, on_cflags_output, 
						 cdata);
		
	g_free (exe_string);
}

static GList **
files_visit_dir (GList **files_list, const gchar* uri)
{
	
	GList *files_in_curr_dir = NULL;
	
	if (gnome_vfs_directory_list_load (&files_in_curr_dir, uri,
								   GNOME_VFS_FILE_INFO_GET_MIME_TYPE) == GNOME_VFS_OK) 
	{
		GList *node;
		node = files_in_curr_dir;
		do {
			GnomeVFSFileInfo* info;
						
			info = node->data;
			
			if (info->type == GNOME_VFS_FILE_TYPE_DIRECTORY) {
				
				if (strcmp (info->name, ".") == 0 ||
					strcmp (info->name, "..") == 0)
					continue;

				g_message ("node [DIRECTORY]: %s", info->name);				
				gchar *tmp = g_strdup_printf ("%s/%s", uri, info->name);
				
				g_message ("recursing for: %s", tmp);
				/* recurse */
				files_list = files_visit_dir (files_list, tmp);
				
				g_free (tmp);
			}
			else {
				gchar *local_path;
				gchar *tmp = g_strdup_printf ("%s/%s", 
									uri, info->name);
			
				g_message ("prepending %s", tmp);
				local_path = gnome_vfs_get_local_path_from_uri (tmp);
				*files_list = g_list_prepend (*files_list, local_path);
				g_free (tmp);
			}
		} while ((node = node->next) != NULL);		
	}	 
	
	return files_list;
}

static void
on_tag_load_toggled (GtkCellRendererToggle *cell, char *path_str,
					 SymbolDBPlugin *sdb_plugin)
{
	GtkTreeIter iter;
	GtkTreePath *path;
	gboolean enabled;
	GList *enabled_packages;
	gchar *curr_package_name;
	GtkListStore *store;
	AnjutaStatus *status;
	GList * node;
	GPtrArray *files_to_scan_array;
	GPtrArray *languages_array;
	IAnjutaLanguage* lang_manager;
	
	DEBUG_PRINT ("on_tag_load_toggled ()");
	lang_manager = anjuta_shell_get_interface (ANJUTA_PLUGIN(sdb_plugin)->shell, 
													IAnjutaLanguage, NULL);
	
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (sdb_plugin)->shell, NULL);
	store = sdb_plugin->prefs_list_store;
	
	anjuta_status_busy_push (status);
	
	path = gtk_tree_path_new_from_string (path_str);
	gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);
	gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
						COLUMN_LOAD, &enabled,
						COLUMN_NAME, &curr_package_name,
						COLUMN_FLAGS_LIST, &enabled_packages,
						-1);
	enabled = !enabled;
	gtk_list_store_set (store, &iter, COLUMN_LOAD, enabled, -1);
	gtk_tree_path_free (path);
		
	node = enabled_packages;	
	
	/* if we have some packages then initialize the array that will be passed to
	 * the engine */
	if (node != NULL)
	{
		files_to_scan_array = g_ptr_array_new ();
		languages_array = g_ptr_array_new();
	}
	else
		return;
	
	do {
		GList *files_tmp_list = NULL;
		gchar *uri;
/*		g_message ("package %s has node : %s", curr_package_name, node->data);*/
		
		uri = gnome_vfs_get_uri_from_local_path (node->data);
		
		/* files_tmp_list needs to be freed */
		files_visit_dir (&files_tmp_list, uri);
		g_free (uri);
		
		if (files_tmp_list != NULL) 
		{			
			/* last loop here. With files_visit_dir we'll retrieve all files nodes
			 * under the passed directory 
			 */
			GList *tmp_node;
			tmp_node = files_tmp_list;
			do {
				const gchar* file_mime;
				IAnjutaLanguageId lang_id;
				const gchar* lang;
				file_mime = gnome_vfs_get_mime_type_for_name (tmp_node->data);
		
				lang_id = ianjuta_language_get_from_mime_type (lang_manager, file_mime, 
													   NULL);
		
				/* No supported language... */
				if (!lang_id)
				{
					continue;
				}
			
				lang = ianjuta_language_get_name (lang_manager, lang_id, NULL);				
		
				g_ptr_array_add (languages_array, g_strdup (lang));				
				g_ptr_array_add (files_to_scan_array, g_strdup (tmp_node->data));
			} while ((tmp_node = tmp_node->next) != NULL);		
			
			/* free the tmp files list */
			g_list_foreach (files_tmp_list, (GFunc)g_free, NULL);
			g_list_free (files_tmp_list);
		}
		
		/* great. Launch the population, if we had something on files_to_scan_array */
		if (files_to_scan_array != NULL) 
		{
			DEBUG_PRINT ("adding new project to db_plugin->sdbe_globals %s ", 
						 curr_package_name);
			symbol_db_engine_add_new_project (sdb_plugin->sdbe_globals,
											  NULL,
											  curr_package_name);
			
			symbol_db_engine_add_new_files (sdb_plugin->sdbe_globals,
											curr_package_name, 
											files_to_scan_array,
											languages_array,
											TRUE);
		}
		
	} while ((node = node->next) != NULL);		
	
	
#if 0		
	enabled_paths = NULL;
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter))
	{
		do
		{
			gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
								COLUMN_LOAD, &enabled,
								COLUMN_PATH, &tag_path,
								-1);
			if (enabled)
				enabled_paths = g_list_prepend (enabled_paths, tag_path);
			
		}
		while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter));
	}
	
	if (enabled_paths)
	{
		GList *node;
		GString *str;
		gboolean first;
		gchar *final_str;
		
		enabled_paths = g_list_sort (enabled_paths, (GCompareFunc)strcmp);
		node = enabled_paths;
		str = g_string_new ("");
		first = TRUE;
		while (node)
		{
			if (first)
			{
				first = FALSE;
				str = g_string_append (str, (const gchar*) node->data);
			}
			else
			{
				str = g_string_append (str, ":");
				str = g_string_append (str, (const gchar*) node->data);
			}
			node = g_list_next (node);
		}
		
		/* Update preferences */
		final_str = g_string_free (str, FALSE);
		anjuta_preferences_set (prefs, SYMBOL_BROWSER_TAGS, final_str);
		
		/* Update system tags cache */
		if (enabled)
		{
			update_system_tags_only_add (tag_path);
		}
		else
		{
			update_system_tags (enabled_paths);
			g_free (final_str);
		}
	}
	else
	{
		/* Unset key and clear all tags */
		anjuta_preferences_set (prefs, SYMBOL_BROWSER_TAGS, "");
	}
	g_list_foreach (enabled_paths, (GFunc)g_free, NULL);
	g_list_free (enabled_paths);
	anjuta_status_busy_pop (status);
#endif	
}

void 
symbol_db_prefs_init (SymbolDBPlugin *sdb_plugin, AnjutaPreferences *prefs)
{
	GladeXML *gxml;
	GtkWidget *fchooser, *treeview;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	gchar *ctags_value;
	gchar* exe_string = NULL;
	gboolean require_scan = FALSE;		/* scan for packages */
	
	g_return_if_fail (sdb_plugin != NULL);
	DEBUG_PRINT ("symbol_db_prefs_init ()");
	
	if (initialized)
		return;
	
	/* Create the preferences page */
	gxml = glade_xml_new (GLADE_FILE, GLADE_ROOT, NULL);	
		
	fchooser = 	glade_xml_get_widget (gxml, CHOOSER_WIDGET);
			
	anjuta_preferences_add_page (prefs, gxml, GLADE_ROOT, _("Symbol Database"),  
								 ICON_FILE);
	ctags_value = anjuta_preferences_get (prefs, CTAGS_PREFS_KEY);
	
	if (ctags_value == NULL) 
	{
		ctags_value = g_strdup (CTAGS_PATH);
	}
	
	if (gtk_file_chooser_select_filename (GTK_FILE_CHOOSER (fchooser), ctags_value) 
						== FALSE )
	{
		DEBUG_PRINT ("error: could not select file uri with gtk_file_chooser_select_filename ()");
	}
	
	g_signal_connect (G_OBJECT (fchooser), "selection-changed",
					  G_CALLBACK (on_prefs_executable_changed), prefs);

	sdb_plugin->prefs_notify_id = anjuta_preferences_notify_add (prefs, CTAGS_PREFS_KEY, 
											   on_gconf_notify_prefs, prefs, NULL);	
	
	
	/* init GtkListStore */
	if (sdb_plugin->prefs_list_store == NULL) 
	{
		sdb_plugin->prefs_list_store = gtk_list_store_new (COLUMN_MAX, G_TYPE_BOOLEAN, 
													   G_TYPE_STRING, G_TYPE_POINTER);
		require_scan = TRUE;
	}
	treeview = glade_xml_get_widget (gxml, "tags_treeview");
	gtk_tree_view_set_model (GTK_TREE_VIEW (treeview),
							 GTK_TREE_MODEL (sdb_plugin->prefs_list_store));

	/* Add the column for stock treeview */
	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (G_OBJECT (renderer), "toggled",
					  G_CALLBACK (on_tag_load_toggled), sdb_plugin);
	column = gtk_tree_view_column_new_with_attributes (_("Load"),
													   renderer,
													   "active",
													   COLUMN_LOAD,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
	
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("API Tags"),
													  renderer, "text",
													  COLUMN_NAME,
													  NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview),
									 COLUMN_NAME);
	
	/* listall launcher thing */
	if (require_scan == TRUE) 
	{
		sdb_plugin->pkg_config_launcher = anjuta_launcher_new ();

		anjuta_launcher_set_check_passwd_prompt (sdb_plugin->pkg_config_launcher, FALSE);
		g_signal_connect (G_OBJECT (sdb_plugin->pkg_config_launcher), "child-exited",
					  	G_CALLBACK (on_listall_exit), sdb_plugin);	
	
		exe_string = g_strdup ("pkg-config --list-all");
	
		anjuta_launcher_execute (sdb_plugin->pkg_config_launcher,
							 	exe_string, on_listall_output, 
							 	sdb_plugin);
	}
	
	/* unrefs unused memory objects */
	g_object_unref (gxml);
	g_free (ctags_value);
	g_free (exe_string);
	
	/* pkg_tmp_file will be released on launcher_exit */
	
	initialized = TRUE;
}

void 
symbol_db_prefs_finalize (SymbolDBPlugin *sdb_plugin, AnjutaPreferences *prefs)
{	
	DEBUG_PRINT ("symbol_db_prefs_finalize ()");
	anjuta_preferences_notify_remove(prefs, sdb_plugin->prefs_notify_id);
	anjuta_preferences_remove_page(prefs, _("Symbol Database"));

	g_object_unref (cflags_launcher);
	cflags_launcher = NULL;
	
	/* free pkg_list */
	g_list_foreach (pkg_list, (GFunc)g_free, NULL);
	g_list_free (pkg_list);
	pkg_list = NULL;
	
	initialized = FALSE;
}
