/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    an_symbol_prefs.c
    Copyright (C) 2004 Naba Kumar

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <config.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkliststore.h>
#include <libanjuta/anjuta-debug.h>

#include "an_symbol_prefs.h"
#include "tm_tagmanager.h"

#define GLADE_FILE PACKAGE_DATA_DIR"/glade/anjuta-symbol-browser-plugin.glade"
#define ICON_FILE "anjuta-symbol-browser-plugin.png"
#define SYSTEM_TAGS_DIR PACKAGE_DATA_DIR"/tags"
#define LOCAL_TAGS_DIR ".anjuta/tags"
#define SYSTEM_TAGS_CACHE ".anjuta/system-tags.cache"
#define SYMBOL_BROWSER_TAGS "symbol.browser.tags"

enum
{
	COLUMN_LOAD,
	COLUMN_NAME,
	COLUMN_PATH,
	N_COLUMNS
};

static void 
update_system_tags (GList *tags_files)
{
	gchar *output_file;
	
	output_file = g_build_filename (g_get_home_dir (), SYSTEM_TAGS_CACHE, NULL);
	
	DEBUG_PRINT ("Recreating system tags cache: %s", output_file);
	
	if (!tm_workspace_merge_global_tags (output_file, tags_files))
	{
		g_warning ("Error while re-creating system tags cache");
	}
	g_free (output_file);
}

static void 
update_system_tags_only_add (const gchar *tag_file)
{
	GList *tags_files;
	gchar *output_file;
	
	output_file = g_build_filename (g_get_home_dir (), SYSTEM_TAGS_CACHE, NULL);
	
	DEBUG_PRINT ("Recreating system tags cache: %s", output_file);
	
	tags_files = g_list_append (NULL, output_file);
	tags_files = g_list_append (tags_files, (gpointer) tag_file);
	if (!tm_workspace_merge_global_tags (output_file, tags_files))
	{
		g_warning ("Error while re-creating system tags cache");
	}
	g_free (output_file);
}

static gboolean
str_has_suffix (const char *haystack, const char *needle)
{
	const char *h, *n;

	if (needle == NULL) {
		return TRUE;
	}
	if (haystack == NULL) {
		return needle[0] == '\0';
	}
		
	/* Eat one character at a time. */
	h = haystack + strlen(haystack);
	n = needle + strlen(needle);
	do {
		if (n == needle) {
			return TRUE;
		}
		if (h == haystack) {
			return FALSE;
		}
	} while (*--h == *--n);
	return FALSE;
}

static void
select_loaded_tags (GtkListStore * store, AnjutaPreferences *prefs)
{
	GtkTreeIter iter;
	gchar *all_tags_path;
	gchar **tags_paths, **tags_path;
	GHashTable *path_hash;
	
	all_tags_path = anjuta_preferences_get (prefs, SYMBOL_BROWSER_TAGS);
	if (all_tags_path == NULL)
		return;
	
	tags_paths = g_strsplit (all_tags_path, ":", -1);
	path_hash = g_hash_table_new (g_str_hash, g_str_equal);
	tags_path = tags_paths;
	while (*tags_path)
	{
		g_hash_table_insert (path_hash, *tags_path, *tags_path);
		tags_path++;
	}
	
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter))
	{
		do
		{
			const gchar *tag_path;
			
			gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
								COLUMN_PATH, &tag_path,
								-1);
			if (g_hash_table_lookup (path_hash, tag_path))
				gtk_list_store_set (store, &iter, COLUMN_LOAD, TRUE, -1);
			else
				gtk_list_store_set (store, &iter, COLUMN_LOAD, FALSE, -1);
		}
		while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter));
	}
	g_hash_table_destroy (path_hash);
	g_strfreev (tags_paths);
	g_free (all_tags_path);
}

static GtkListStore *
create_store (AnjutaPreferences *prefs)
{
	GList *node;
	gchar *local_tags_dir;
	GList *tags_dirs = NULL;
	GtkListStore *store;
	
	/* Create store */
	store = gtk_list_store_new (N_COLUMNS, G_TYPE_BOOLEAN, G_TYPE_STRING,
								G_TYPE_STRING);
	
	tags_dirs = g_list_prepend (tags_dirs, g_strdup (SYSTEM_TAGS_DIR));
	
	local_tags_dir = g_build_filename (g_get_home_dir (), LOCAL_TAGS_DIR, NULL);
	tags_dirs = g_list_prepend (tags_dirs, local_tags_dir);
	
	/* Load the tags files */
	node = tags_dirs;
	while (node)
	{
		DIR *dir;
		struct dirent *entry;
		const gchar *dirname;
		
		dirname = (const gchar*)node->data;
		node = g_list_next (node);
		
		dir = opendir (dirname);
		if (!dir)
			continue;
		
		for (entry = readdir (dir); entry != NULL; entry = readdir (dir))
		{
			if (str_has_suffix (entry->d_name, ".anjutatags.gz"))
			{
				gchar *pathname;
				gchar *tag_name;
				GtkTreeIter iter;
				
				tag_name = g_strndup (entry->d_name,
									  strlen (entry->d_name) -
									  strlen (".anjutatags.gz"));
				pathname = g_build_filename (dirname, entry->d_name, NULL);
				
				gtk_list_store_append (store, &iter);
				gtk_list_store_set (store, &iter, COLUMN_LOAD, FALSE,
									COLUMN_NAME, tag_name, 
									COLUMN_PATH, pathname, -1);
				g_free (tag_name);
				g_free (pathname);
			}
		}
		closedir (dir);
	}
	g_list_foreach (tags_dirs, (GFunc)g_free, NULL);
	g_list_free (tags_dirs);
	
	select_loaded_tags (store, prefs);
	return store;
}

static void
on_tag_load_toggled (GtkCellRendererToggle *cell, char *path_str,
					 SymbolBrowserPlugin *plugin)
{
	GtkTreeIter iter;
	GtkTreePath *path;
	gboolean enabled;
	AnjutaPreferences *prefs;
	const gchar *tag_path, *saved_path;
	GList *enabled_paths;
	GtkListStore *store;
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell, NULL);
	store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (plugin->pref_tree_view)));
	prefs = plugin->prefs;
	
	anjuta_status_busy_push (status);
	
	path = gtk_tree_path_new_from_string (path_str);
	gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);
	gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
						COLUMN_LOAD, &enabled,
						COLUMN_PATH, &saved_path,
						-1);
	enabled = !enabled;
	gtk_list_store_set (store, &iter, COLUMN_LOAD, enabled, -1);
	gtk_tree_path_free (path);
	
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
				enabled_paths = g_list_prepend (enabled_paths,
												(gpointer)tag_path);
			
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
		
		/* Update system tags cache */
		if (enabled)
		{
			update_system_tags_only_add (tag_path);
		}
		else
		{
			update_system_tags (enabled_paths);
		}
		
		/* Update preferences */
		final_str = g_string_free (str, FALSE);
		anjuta_preferences_set (prefs, SYMBOL_BROWSER_TAGS, final_str);
		
		g_list_free (enabled_paths);
		g_free (final_str);
	}
	else
	{
		/* Unset key and clear all tags */
		anjuta_preferences_set (prefs, SYMBOL_BROWSER_TAGS, "");
	}
	anjuta_status_busy_pop (status);
}

static GtkWidget *
prefs_page_init (SymbolBrowserPlugin *plugin)
{
	GtkWidget *treeview;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkListStore *store;
	AnjutaPreferences *pref = plugin->prefs;
	GladeXML *gxml = glade_xml_new (GLADE_FILE, NULL, NULL);
	
	anjuta_preferences_add_page (pref, gxml, "Symbol Browser", ICON_FILE);
	treeview = glade_xml_get_widget (gxml, "tags_treeview");
	
	store = create_store (pref);
	gtk_tree_view_set_model (GTK_TREE_VIEW (treeview),
							 GTK_TREE_MODEL (store));

	/* Add the column for stock treeview */
	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (G_OBJECT (renderer), "toggled",
					  G_CALLBACK (on_tag_load_toggled), plugin);
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
	
	g_object_unref (store);
	g_object_unref (gxml);
	return treeview;
}

static void
on_gconf_notify_tags_list_changed (GConfClient *gclient, guint cnxn_id,
								   GConfEntry *entry, gpointer user_data)
{
	GtkListStore *store;
	SymbolBrowserPlugin *plugin = (SymbolBrowserPlugin*)user_data;
	store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (plugin->pref_tree_view)));
	select_loaded_tags (store, plugin->prefs);
}

void
symbol_browser_prefs_init (SymbolBrowserPlugin *plugin)
{
	guint notify_id;
	static gboolean page_initialized = FALSE;
	
	if (!page_initialized)
	{
		plugin->pref_tree_view = prefs_page_init (plugin);
		page_initialized = TRUE;
	}
	plugin->gconf_notify_ids = NULL;
	notify_id = anjuta_preferences_notify_add (plugin->prefs,
											   SYMBOL_BROWSER_TAGS,
											   on_gconf_notify_tags_list_changed,
											   plugin, NULL);
	plugin->gconf_notify_ids = g_list_prepend (plugin->gconf_notify_ids,
											   (gpointer)notify_id);
}

void
symbol_browser_prefs_finalize (SymbolBrowserPlugin *plugin)
{
	GList *node;
	node = plugin->gconf_notify_ids;
	while (node)
	{
		anjuta_preferences_notify_remove (plugin->prefs, (guint)node->data);
		node = g_list_next (node);
	}
	g_list_free (plugin->gconf_notify_ids);
	plugin->gconf_notify_ids = NULL;
}
