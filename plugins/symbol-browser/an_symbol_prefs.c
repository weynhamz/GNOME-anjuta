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
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkliststore.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-launcher.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>

#include "an_symbol_prefs.h"
#include "tm_tagmanager.h"

#define GLADE_FILE PACKAGE_DATA_DIR"/glade/anjuta-symbol-browser-plugin.glade"
#define ICON_FILE "anjuta-symbol-browser-plugin.png"
#define LOCAL_TAGS_DIR ".anjuta/tags"
#define SYSTEM_TAGS_CACHE ".anjuta/system-tags.cache"
#define SYMBOL_BROWSER_TAGS "symbol.browser.tags"
#define CREATE_GLOBAL_TAGS PACKAGE_DATA_DIR"/scripts/create_global_tags.sh"

enum
{
	COLUMN_LOAD,
	COLUMN_NAME,
	COLUMN_PATH,
	N_COLUMNS
};

static SymbolBrowserPlugin* symbol_browser_plugin = NULL;

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
	
	/* Reload tags */
	tm_workspace_reload_global_tags(output_file);
	
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
	/* Reload tags */
	tm_workspace_reload_global_tags(output_file);
	
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
			gchar *tag_path;
			
			gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
								COLUMN_PATH, &tag_path,
								-1);
			if (g_hash_table_lookup (path_hash, tag_path))
				gtk_list_store_set (store, &iter, COLUMN_LOAD, TRUE, -1);
			else
				gtk_list_store_set (store, &iter, COLUMN_LOAD, FALSE, -1);
			g_free (tag_path);
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
	
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store),
			COLUMN_NAME, GTK_SORT_ASCENDING);
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
	gchar *tag_path;
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
}

static void
on_add_directory_clicked (GtkWidget *button, GtkListStore *store)
{
	GtkTreeIter iter;
	GtkWidget *fileselection;
	GtkWidget *parent;
	
	parent = gtk_widget_get_toplevel (button);
	fileselection = gtk_file_chooser_dialog_new (_("Select directory"),
												 GTK_WINDOW (parent),
												 GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
												 GTK_STOCK_CANCEL,
												 GTK_RESPONSE_CANCEL,
												 GTK_STOCK_OK,
												 GTK_RESPONSE_OK,
												 NULL);
	if (gtk_dialog_run (GTK_DIALOG (fileselection)) == GTK_RESPONSE_OK)
	{
		GSList *dirs, *node;
		
		/* Only local files since we can only create tags for local files */
		dirs = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (fileselection));
		
		node = dirs;
		while (node)
		{
			gchar *dir = node->data;
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter, 0, dir, -1);
			g_free (dir);
			node = g_slist_next (node);
		}
		g_slist_free (dirs);
	}
	gtk_widget_destroy (fileselection);
}

static void
refresh_tags_list (SymbolBrowserPlugin *plugin)
{
	GtkListStore *new_tags_store;
	new_tags_store = create_store (plugin->prefs);
	gtk_tree_view_set_model (GTK_TREE_VIEW (plugin->pref_tree_view),
							 GTK_TREE_MODEL (new_tags_store));
	g_object_unref (new_tags_store);
}

static void
on_create_tags_clicked (GtkButton *widget, SymbolBrowserPlugin *plugin)
{
	GtkWidget *treeview, *button, *dlg, *name_entry;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkListStore *store;
	AnjutaPreferences *pref;
	GladeXML *gxml;
	
	pref = plugin->prefs;
	gxml = glade_xml_new (GLADE_FILE, "create.symbol.tags.dialog", NULL);
	
	dlg = glade_xml_get_widget (gxml, "create.symbol.tags.dialog");
	treeview = glade_xml_get_widget (gxml, "directory_list_treeview");
	name_entry = glade_xml_get_widget (gxml, "symbol_tags_name_entry");
	
	store = gtk_list_store_new (1, G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (treeview),
							 GTK_TREE_MODEL (store));

	/* Add the column for stock treeview */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Directories to scan"),
													  renderer, "text",
													  0,
													  NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview),
									 COLUMN_NAME);
	
	button = glade_xml_get_widget (gxml, "add_directory_button");
	g_signal_connect (G_OBJECT (button), "clicked",
					  G_CALLBACK (on_add_directory_clicked), store);
	
	button = glade_xml_get_widget (gxml, "clear_list_button");
	g_signal_connect_swapped (G_OBJECT (button), "clicked",
							  G_CALLBACK (gtk_list_store_clear), store);
	
	gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (plugin->prefs));
	while (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_OK)
	{
		GtkTreeIter iter;
		gchar **argv, *tmp;
		const gchar *name;
		gint argc;
		pid_t pid;
		
		name = gtk_entry_get_text (GTK_ENTRY (name_entry));
		
		argc = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (store),
											   NULL) * 3 + 3;
		
		if (name == NULL || strlen (name) <= 0 || argc <= 3)
		{
			/* Validation failed */
			GtkWidget *edlg;
			
			edlg = gtk_message_dialog_new (GTK_WINDOW (dlg),
										   GTK_DIALOG_DESTROY_WITH_PARENT,
										   GTK_MESSAGE_ERROR,
										   GTK_BUTTONS_CLOSE,
										  _("Please enter a name and at least one directory."));
			gtk_dialog_run (GTK_DIALOG (edlg));
			gtk_widget_destroy (edlg);
			continue;
		}
		
		argv = g_new0 (gchar*, argc);
		
		argv[0] = g_strdup ("anjuta-tags");
		tmp = g_build_filename (g_get_home_dir (), LOCAL_TAGS_DIR, name, NULL);
		argv[1] = g_strconcat (tmp, ".anjutatags", NULL);
		g_free (tmp);
		
		argc = 2;
		if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter))
		{
			do
			{
				gchar *dir;
				gchar *files;
				
				gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
									0, &dir, -1);
				
				files = g_build_filename (dir, "*.h", NULL);
				DEBUG_PRINT ("%d: Adding scan files '%s'", argc, files);
				argv[argc++] = g_strconcat ("\"", files, "\"", NULL);
				g_free (files);
				
				files = g_build_filename (dir, "*", "*.h", NULL);
				DEBUG_PRINT ("%d: Adding scan files '%s'", argc, files);
				argv[argc++] = g_strconcat ("\"", files, "\"", NULL);
				g_free (files);
				
				files = g_build_filename (dir, "*", "*", "*.h", NULL);
				DEBUG_PRINT ("%d: Adding scan files '%s'", argc, files);
				argv[argc++] = g_strconcat ("\"", files, "\"", NULL);
				g_free (files);
				
				g_free (dir);
			}
			while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter));
		}
		
		/* Create local tags directory */
		tmp = g_build_filename (g_get_home_dir (), LOCAL_TAGS_DIR, NULL);
		if ((pid = fork()) == 0)
		{
			execlp ("mkdir", "mkdir", "-p", tmp, NULL);
			perror ("Could not execute mkdir");
		}
		waitpid (pid, NULL, 0);
		g_free (tmp);
		
		/* Execute anjuta-tags to create tags for the given files */
		if ((pid = fork()) == 0)
		{
			execvp (g_build_filename (PACKAGE_DATA_DIR, "scripts",
									  "anjuta-tags"), argv);
			perror ("Could not execute anjuta-tags");
		}
		waitpid (pid, NULL, 0);
		
		/* Compress the tags file */
		if ((pid = fork()) == 0)
		{
			execlp ("gzip", "gzip", "-f", argv[1], NULL);
			perror ("Could not execute gzip");
		}
		waitpid (pid, NULL, 0);
		
		g_strfreev (argv);
		
		/* Refresh the tags list */
		refresh_tags_list (plugin);
		break;
	}
	g_object_unref (store);
	g_object_unref (gxml);
	gtk_widget_destroy (dlg);
}

static void
on_add_tags_clicked (GtkWidget *button, SymbolBrowserPlugin *plugin)
{
	GtkWidget *fileselection;
	GtkWidget *parent;
	GtkFileFilter *filter;
	
	parent = gtk_widget_get_toplevel (button);
	
	fileselection = gtk_file_chooser_dialog_new (_("Select directory"),
												 GTK_WINDOW (parent),
												 GTK_FILE_CHOOSER_ACTION_OPEN,
												 GTK_STOCK_CANCEL,
												 GTK_RESPONSE_CANCEL,
												 GTK_STOCK_OK,
												 GTK_RESPONSE_OK,
												 NULL);
	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("Anjuta tags files"));
	gtk_file_filter_add_pattern (filter, "*.anjutatags.gz");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (fileselection), filter);
	
	if (gtk_dialog_run (GTK_DIALOG (fileselection)) == GTK_RESPONSE_OK)
	{
		GSList *uris, *node;
		gchar *tmp;
		pid_t pid;
		
		/* Create local tags directory */
		tmp = g_build_filename (g_get_home_dir (), LOCAL_TAGS_DIR, NULL);
		if ((pid = fork()) == 0)
		{
			execlp ("mkdir", "mkdir", "-p", tmp, NULL);
			perror ("Could not execute mkdir");
		}
		waitpid (pid, NULL, 0);
		g_free (tmp);
		
		uris = gtk_file_chooser_get_uris (GTK_FILE_CHOOSER (fileselection));
		
		node = uris;
		while (node)
		{
			gchar *dest, *src, *basename;
			
			src = node->data;
			basename = g_path_get_basename (src);
			dest = g_build_filename (g_get_home_dir (), LOCAL_TAGS_DIR, basename, NULL);
			g_free (basename);
			
			/* Copy the tags file in local tags directory */
			GnomeVFSURI* source_uri = gnome_vfs_uri_new(src);
			GnomeVFSURI* dest_uri = gnome_vfs_uri_new(dest);
	
			GnomeVFSResult error = gnome_vfs_xfer_uri (source_uri,
													   dest_uri,
													   GNOME_VFS_XFER_DEFAULT,
													   GNOME_VFS_XFER_ERROR_MODE_ABORT,
													   GNOME_VFS_XFER_OVERWRITE_MODE_ABORT,
													   NULL,
													   NULL);
			if (error != GNOME_VFS_OK)
			{
				const gchar *err;
				
				err = gnome_vfs_result_to_string (error);
				anjuta_util_dialog_error (GTK_WINDOW (fileselection),
										  "Adding tags file '%s' failed: %s",
										  src, err);
			}
			gnome_vfs_uri_unref (source_uri);
			gnome_vfs_uri_unref (dest_uri);
			g_free (dest);
			g_free (src);
			node = g_slist_next (node);
		}
		if (uris)
		{
			refresh_tags_list (plugin);
		}
		g_slist_free (uris);
	}
	gtk_widget_destroy (fileselection);
}

static void
on_remove_tags_clicked (GtkWidget *button, SymbolBrowserPlugin *plugin)
{
	GtkWidget *parent;
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	GtkTreeModel *model;
	
	parent = gtk_widget_get_toplevel (button);
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (plugin->pref_tree_view));
	if (gtk_tree_selection_get_selected (sel, &model, &iter))
	{
		gchar *tags_filename;
		gtk_tree_model_get (model, &iter, 1, &tags_filename, -1);
		if (tags_filename)
		{
			gchar *file_path, *path;
			path = g_build_filename (g_get_home_dir(), LOCAL_TAGS_DIR,
									 tags_filename, NULL);
			file_path = g_strconcat (path, ".anjutatags.gz", NULL);
			
			if (!g_file_test (file_path, G_FILE_TEST_EXISTS))
			{
				anjuta_util_dialog_error (GTK_WINDOW (parent),
										  "Can not remove tags file '%s': "
						  "You can only remove tags you created or added",
										  tags_filename);
			}
			else if (anjuta_util_dialog_boolean_question (GTK_WINDOW (parent),
					  "Are you sure you want to remove the tags file '%s'?",
														  tags_filename))
			{
				unlink (file_path);
				refresh_tags_list (plugin);
			}
			g_free (file_path);
			g_free (path);
			g_free (tags_filename);
		}
	}
}

static void
on_message (AnjutaLauncher *launcher,
			AnjutaLauncherOutputType output_type,
			const gchar * mesg, gpointer user_data)
{
	AnjutaStatus *status;
	gchar **lines, **line;
	SymbolBrowserPlugin* plugin = ANJUTA_PLUGIN_SYMBOL_BROWSER (user_data);
	
	lines = g_strsplit (mesg, "\n", -1);
	if (!lines)
		return;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell, NULL);
	
	line = lines;
	while (*line)
	{
		gint packages_count;
		gchar *pos;
		
		if (sscanf (*line, "Scanning %d packages", &packages_count) == 1)
			anjuta_status_progress_add_ticks (status, packages_count + 1);
		else if ((pos = strstr (*line, ".anjutatags.gz")))
		{
			const gchar *package_name;
		
			/* Get the name of the package */
			*pos = '\0';
			package_name = g_strrstr (*line, "/");
			if (package_name)
			{
				gchar *status_mesg;
				package_name++;
				status_mesg = g_strdup_printf (_("Scanning package: %s"),
											   package_name);
				anjuta_status_progress_tick (status, NULL, status_mesg);
				g_free (status_mesg);
			}
			else
				anjuta_status_progress_tick (status, NULL, *line);
		}
		line++;
	}
	g_strfreev (lines);
}

static void
on_system_tags_update_finished (AnjutaLauncher *launcher, gint child_pid,
								gint status, gulong time_taken,
								SymbolBrowserPlugin *plugin)
{
	AnjutaStatus *statusbar;
	GList *enabled_paths = NULL;
	GtkTreeIter iter;
	gchar *tag_path;
	GtkListStore *store;
	gboolean enabled;
	
	/* Refresh the list */
	refresh_tags_list(plugin);
	
	/* Regenerate system-tags.cache */
	store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (plugin->pref_tree_view)));
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
		update_system_tags (enabled_paths);
	}
	g_list_foreach (enabled_paths, (GFunc)g_free, NULL);
	g_list_free (enabled_paths);
	
	g_object_unref (plugin->launcher);
	plugin->launcher = NULL;
	statusbar = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell, NULL);
	anjuta_status_progress_tick (statusbar, NULL,
								 _("Completed system tags generation"));
}

static void 
on_update_global_clicked (GtkWidget *button, SymbolBrowserPlugin *plugin)
{
	gchar* tmp;
	gint pid;
	IAnjutaMessageManager* mesg_manager;

	if (plugin->launcher)
		return; /* Already running */
	
	/* Create local tags directory */	
	tmp = g_build_filename (g_get_home_dir (), LOCAL_TAGS_DIR, NULL);
	if ((pid = fork()) == 0)
	{
		execlp ("mkdir", "mkdir", "-p", tmp, NULL);
		perror ("Could not execute mkdir");
	}
	waitpid (pid, NULL, 0);
	g_free (tmp);
	
	plugin->launcher = anjuta_launcher_new ();
	g_signal_connect (G_OBJECT (plugin->launcher), "child-exited",
					  G_CALLBACK (on_system_tags_update_finished), plugin);
	anjuta_launcher_set_buffered_output (plugin->launcher, TRUE);
	anjuta_launcher_execute (plugin->launcher, CREATE_GLOBAL_TAGS,
							 on_message, plugin);
}

static GtkWidget *
prefs_page_init (SymbolBrowserPlugin *plugin)
{
	GtkWidget *treeview, *button;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkListStore *store;
	AnjutaPreferences *pref = plugin->prefs;
	GladeXML *gxml = glade_xml_new (GLADE_FILE, NULL, NULL);
	
	anjuta_preferences_add_page (pref, gxml, "Symbol Browser",
								 _("Symbol Browser"),  ICON_FILE);
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
	
	button = glade_xml_get_widget (gxml, "create_tags_button");
	g_signal_connect (G_OBJECT (button), "clicked",
					  G_CALLBACK (on_create_tags_clicked), plugin);
	
	button = glade_xml_get_widget (gxml, "add_tags_button");
	g_signal_connect (G_OBJECT (button), "clicked",
					  G_CALLBACK (on_add_tags_clicked), plugin);
	
	button = glade_xml_get_widget (gxml, "remove_tags_button");
	g_signal_connect (G_OBJECT (button), "clicked",
					  G_CALLBACK (on_remove_tags_clicked), plugin);
	
	button = glade_xml_get_widget (gxml, "update_tags_button");
	g_signal_connect (G_OBJECT (button), "clicked",
					  G_CALLBACK (on_update_global_clicked), plugin);
	
	
	symbol_browser_plugin = plugin;
	
	g_object_unref (store);
	g_object_unref (gxml);
	return treeview;
}

static void
on_gconf_notify_tags_list_changed (GConfClient *gclient, guint cnxn_id,
								   GConfEntry *entry, gpointer user_data)
{
	GtkListStore *store;
	SymbolBrowserPlugin *plugin = ANJUTA_PLUGIN_SYMBOL_BROWSER (user_data);
	store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (plugin->pref_tree_view)));
	select_loaded_tags (store, plugin->prefs);
}

void
symbol_browser_prefs_init (SymbolBrowserPlugin *plugin)
{
	guint notify_id;
	plugin->pref_tree_view = prefs_page_init (plugin);
	plugin->gconf_notify_ids = NULL;
	notify_id = anjuta_preferences_notify_add (plugin->prefs,
											   SYMBOL_BROWSER_TAGS,
											   on_gconf_notify_tags_list_changed,
											   plugin, NULL);
	plugin->gconf_notify_ids = g_list_prepend (plugin->gconf_notify_ids,
											   GUINT_TO_POINTER (notify_id));
}

void
symbol_browser_prefs_finalize (SymbolBrowserPlugin *plugin)
{
	GList *node;
	node = plugin->gconf_notify_ids;
	while (node)
	{
		anjuta_preferences_notify_remove (plugin->prefs,
										  GPOINTER_TO_UINT (node->data));
		node = g_list_next (node);
	}
	g_list_free (plugin->gconf_notify_ids);
	plugin->gconf_notify_ids = NULL;
	
	anjuta_preferences_remove_page(plugin->prefs, _("Symbol Browser"));
}


gboolean
symbol_browser_prefs_create_global_tags (gpointer unused)
{
	on_update_global_clicked (NULL, symbol_browser_plugin);
	return FALSE; /* Stop g_idle */
}
