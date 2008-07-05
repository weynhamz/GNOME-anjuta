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

#include <glib.h>
#include <config.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkliststore.h>
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

/* FIXME move away */
static GladeXML *gxml;

enum
{
	COLUMN_LOAD,
	COLUMN_NAME,
	COLUMN_MAX
};

typedef struct _ParseableData {
	SymbolDBPlugin *sdb_plugin;
	gchar *path_str;
	
} ParseableData;


static void
destroy_parseable_data (ParseableData *pdata)
{
	g_free (pdata->path_str);
	g_free (pdata);
}

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
on_listall_output (AnjutaLauncher * launcher,
					AnjutaLauncherOutputType output_type,
					const gchar * chars, gpointer user_data)
{
	gchar **lines;
	const gchar *curr_line;
	gint i = 0;
	SymbolDBPlugin *sdb_plugin;
	GtkListStore *store;

	if (output_type == ANJUTA_LAUNCHER_OUTPUT_STDERR)
	{
		/* no way. We don't like errors on stderr... */
		return;
	}
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (user_data);
	
	store = sdb_plugin->prefs_list_store;
	
	lines = g_strsplit (chars, "\n", -1);
	
	while ((curr_line = lines[i++]) != NULL)
	{
		gchar **pkgs;
		
		pkgs = g_strsplit (curr_line, " ", -1);
		
		/* just take the first token as it's the package-name */
		if (pkgs == NULL)
			return;		
		
		if (pkgs[0] == NULL) {
			g_strfreev (pkgs);
			continue;
		}
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
	GtkListStore *store;
	GList *item;
	
	sdb_plugin = ANJUTA_PLUGIN_SYMBOL_DB (user_data);
	store = sdb_plugin->prefs_list_store;
	
	/* we should have pkg_list filled with packages names 
	 * It's not enough anyway: we have to sort alphabetically the list.
	 * The implementation done before required the single scan of every package,
	 * for instance 'pkg-config --cflags pkg_name', but this was really
	 * unefficent when a lot of packages were found on /usr/lib/pkg-config.
	 * Let then the user click on the toggle checkbox. We'll notify her whether
	 * there are no good cflags for that package.
	 */		
	if (pkg_list == NULL)
	{
		g_warning ("No packages found");
		return;
	}

	pkg_list = g_list_sort (pkg_list, pkg_list_compare);

	
	item = pkg_list;
	
	while (item != NULL)
	{
		GtkTreeIter iter;
		/* that's good. We can add the package to the GtkListStore */
		gtk_list_store_append (GTK_LIST_STORE (store), &iter);
		gtk_list_store_set (store, &iter, COLUMN_LOAD, FALSE,
									COLUMN_NAME, g_strdup (item->data), -1);
		
		item = item->next;
	}	
}

static void 
on_tag_load_toggled_parseable_cb (SymbolDBSystem *sdbs, 
									gboolean is_parseable,
									gpointer user_data)
{
	GtkWidget *treeview, *prefs_progressbar;
	GtkWindow *prefs_window;
	ParseableData *pdata;
	SymbolDBPlugin *sdb_plugin;
	const gchar *path_str;
	
	pdata = (ParseableData *)user_data;
	path_str = pdata->path_str;
	sdb_plugin = pdata->sdb_plugin;
	
	DEBUG_PRINT ("on_tag_load_toggled_parseable_cb %d", is_parseable);
	prefs_window = GTK_WINDOW (glade_xml_get_widget (gxml, "symbol_db_pref_window"));
	treeview = glade_xml_get_widget (gxml, "tags_treeview");
	prefs_progressbar = glade_xml_get_widget (gxml, "prefs_progressbar");
	
	if (is_parseable == FALSE)
	{
		GtkWidget *wid = gtk_message_dialog_new (prefs_window, GTK_DIALOG_MODAL, 
												 GTK_MESSAGE_WARNING,
								GTK_BUTTONS_OK, _("Package is not parseable"));
		gtk_dialog_run (GTK_DIALOG (wid));
 		gtk_widget_destroy (wid);
	}
	else
	{
		GtkTreeIter iter;
		GtkTreePath *path;		
		GtkListStore *store;
		gboolean enabled;
		gchar *curr_package_name;

		/* we have a good parseable package. Let's mark the check enabled/disabled */		
		
		store = sdb_plugin->prefs_list_store;
		path = gtk_tree_path_new_from_string (path_str);
		gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);
		gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
						COLUMN_LOAD, &enabled,
						COLUMN_NAME, &curr_package_name,
						-1);
		enabled = !enabled;
		gtk_list_store_set (store, &iter, COLUMN_LOAD, enabled, -1);
		gtk_tree_path_free (path);
		
		/* good, should we scan the packages? */
		if (enabled == TRUE)
		{
			symbol_db_system_scan_package (sdb_plugin->sdbs, curr_package_name);
		}
	}
	
	gtk_widget_set_sensitive (treeview, TRUE);
	gtk_widget_hide (prefs_progressbar);	
	
	destroy_parseable_data (pdata);
}

				  
static void
on_tag_load_toggled (GtkCellRendererToggle *cell, char *path_str,
					 SymbolDBPlugin *sdb_plugin)
{
	GtkTreeIter iter;
	GtkTreePath *path;
	gchar *curr_package_name;
	GtkListStore *store;
	GtkWidget *prefs_progressbar;
	GtkWidget *	treeview;
	ParseableData *pdata;
	
	DEBUG_PRINT ("on_tag_load_toggled ()");
	
	store = sdb_plugin->prefs_list_store;
	path = gtk_tree_path_new_from_string (path_str);
	gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);
	gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
						COLUMN_NAME, &curr_package_name,
						-1);
	gtk_tree_path_free (path);
	
	prefs_progressbar = glade_xml_get_widget (gxml, "prefs_progressbar");
	gtk_widget_show_all (prefs_progressbar);	
	
	gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR (prefs_progressbar), 1.0);
	gtk_progress_bar_pulse (GTK_PROGRESS_BAR (prefs_progressbar));
	
	treeview = glade_xml_get_widget (gxml, "tags_treeview");
	gtk_widget_set_sensitive (treeview, FALSE);
	
	pdata = g_new0 (ParseableData, 1);
	pdata->sdb_plugin = sdb_plugin;
	pdata->path_str = g_strdup (path_str);
	
	symbol_db_system_is_package_parseable (sdb_plugin->sdbs, curr_package_name, 
										   on_tag_load_toggled_parseable_cb,
										   pdata);
}

void 
symbol_db_prefs_init (SymbolDBPlugin *sdb_plugin, AnjutaPreferences *prefs)
{

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
	
	if (ctags_value == NULL || strlen (ctags_value) <= 0) 
	{
		ctags_value = g_strdup (CTAGS_PATH);
	}
	
	DEBUG_PRINT ("trying to set ->%s<-", ctags_value);
	if (gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (fchooser), ctags_value) 
						== FALSE )
	{
		DEBUG_PRINT ("error: could not select file uri with gtk_file_chooser_select_filename ()");
		return;
	}
	
	g_signal_connect (G_OBJECT (fchooser), "selection-changed",
					  G_CALLBACK (on_prefs_executable_changed), prefs);

	sdb_plugin->prefs_notify_id = anjuta_preferences_notify_add (prefs, CTAGS_PREFS_KEY, 
											   on_gconf_notify_prefs, prefs, NULL);	
	
	
	/* init GtkListStore */
	if (sdb_plugin->prefs_list_store == NULL) 
	{
		sdb_plugin->prefs_list_store = gtk_list_store_new (COLUMN_MAX, G_TYPE_BOOLEAN, 
													   G_TYPE_STRING);
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
	
	/* frame3 show all */
	GtkWidget *frame3;
	frame3 = glade_xml_get_widget (gxml, "frame3");
	gtk_widget_show_all (frame3);
	GtkWidget *prefs_progressbar = glade_xml_get_widget (gxml, "prefs_progressbar");
	gtk_widget_hide (prefs_progressbar);	
	
	
	/* listall launcher thing */
	if (require_scan == TRUE) 
	{
		sdb_plugin->pkg_config_launcher = anjuta_launcher_new ();

		anjuta_launcher_set_check_passwd_prompt (sdb_plugin->pkg_config_launcher, 
												 FALSE);
		/* the on_listall_exit callback will continue calling the launcher to process
		 * every entry received
		 */
		g_signal_connect (G_OBJECT (sdb_plugin->pkg_config_launcher), "child-exited",
					  	G_CALLBACK (on_listall_exit), sdb_plugin);	
	
		exe_string = g_strdup ("pkg-config --list-all");
	
		anjuta_launcher_execute (sdb_plugin->pkg_config_launcher,
							 	exe_string, on_listall_output, 
							 	sdb_plugin);
	}
	
		
	/* unrefs unused memory objects */
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

	if (cflags_launcher != NULL)
		g_object_unref (cflags_launcher);
	cflags_launcher = NULL;
	
	/* free pkg_list */
	g_list_foreach (pkg_list, (GFunc)g_free, NULL);
	g_list_free (pkg_list);
	pkg_list = NULL;
	
	if (gxml != NULL)
		g_object_unref (gxml);
	
	/* FIXME: disconnect signals */
		
	initialized = FALSE;
}
