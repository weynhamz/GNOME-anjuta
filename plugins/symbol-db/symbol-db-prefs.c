/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta_trunk
 * Copyright (C) Massimo Cora' 2008 <maxcvs@email.it>
 * 
 * anjuta_trunk is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta_trunk is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
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
#define CHOOSER_WIDGET		"preferences_file:text:/usr/bin/ctags:0:symboldb.ctags"

enum
{
	COLUMN_LOAD,
	COLUMN_NAME,
	COLUMN_MAX
};

enum
{
	PACKAGE_ADD,
	PACKAGE_REMOVE,
	LAST_SIGNAL
};

static unsigned int signals[LAST_SIGNAL] = { 0 };

struct _SymbolDBPrefsPriv {
	GtkListStore *prefs_list_store;
	GladeXML *prefs_gxml;
	AnjutaLauncher *pkg_config_launcher;	
	AnjutaPreferences *prefs;
	
	SymbolDBSystem *sdbs;
	SymbolDBEngine *sdbe_project;
	SymbolDBEngine *sdbe_globals;
	
	GList *pkg_list;
	GHashTable *enabled_packages_hash;

	gint prefs_notify_id;
};


typedef struct _ParseableData {
	SymbolDBPrefs *sdbp;
	gchar *path_str;
	
} ParseableData;


static void
destroy_parseable_data (ParseableData *pdata)
{
	g_free (pdata->path_str);
	g_free (pdata);
}


G_DEFINE_TYPE (SymbolDBPrefs, sdb_prefs, G_TYPE_OBJECT);

static void 
on_prefs_executable_changed (GtkFileChooser *chooser,
                             gpointer user_data)
{
	gchar *new_file;
	SymbolDBPrefs *sdbp;
	SymbolDBPrefsPriv *priv;
	
	sdbp = SYMBOL_DB_PREFS (user_data);
	priv = sdbp->priv;
	
	new_file = gtk_file_chooser_get_filename (chooser);
	DEBUG_PRINT ("on_prefs_executable_changed (): new executable selected %s", 
				 new_file);
	if (new_file != NULL) 
	{
		GtkWidget *fchooser;
		fchooser = 	glade_xml_get_widget (priv->prefs_gxml, CHOOSER_WIDGET);	
		gtk_widget_set_sensitive (fchooser, TRUE);
		
		anjuta_preferences_set (priv->prefs, CTAGS_PREFS_KEY,
							new_file);
	
		/* remember to set the new ctags path into various symbol engines */
		symbol_db_engine_set_ctags_path (priv->sdbe_project, new_file);
		symbol_db_engine_set_ctags_path (priv->sdbe_globals, new_file);
	}
	
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
	SymbolDBPrefs *sdbp;
	SymbolDBPrefsPriv *priv;
	GtkListStore *store;

	if (output_type == ANJUTA_LAUNCHER_OUTPUT_STDERR)
	{
		/* no way. We don't like errors on stderr... */
		return;
	}
	
	sdbp = SYMBOL_DB_PREFS (user_data);
	priv = sdbp->priv;
	
	store = priv->prefs_list_store;	
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
		priv->pkg_list = g_list_prepend (priv->pkg_list, g_strdup (pkgs[0]));
		g_strfreev (pkgs);
	}

	g_strfreev (lines);	
}

static void
on_listall_exit (AnjutaLauncher * launcher, int child_pid,
				   int exit_status, gulong time_taken_in_seconds,
				   gpointer user_data)
{	
	SymbolDBPrefs *sdbp;
	SymbolDBPrefsPriv *priv;	
	GtkListStore *store;
	GList *item;
	GtkWidget *treeview;
	
	sdbp = SYMBOL_DB_PREFS (user_data);
	priv = sdbp->priv;
	store = priv->prefs_list_store;
	
	DEBUG_PRINT ("on_listall_exit ()");

	g_signal_handlers_disconnect_by_func (launcher, on_listall_exit,
										  user_data);	

	treeview = glade_xml_get_widget (priv->prefs_gxml, "tags_treeview");
	gtk_widget_set_sensitive (treeview, TRUE);
	
	/* we should have pkg_list filled with packages names 
	 * It's not enough anyway: we have to sort alphabetically the list.
	 * The implementation done before required the single scan of every package,
	 * for instance 'pkg-config --cflags pkg_name', but this was really
	 * unefficent when a lot of packages were found on /usr/lib/pkg-config.
	 * Let then the user click on the toggle checkbox. We'll notify her whether
	 * there are no good cflags for that package.
	 */		
	if (priv->pkg_list == NULL)
	{
		g_warning ("No packages found");
		return;
	}

	priv->pkg_list = g_list_sort (priv->pkg_list, pkg_list_compare);
	item = priv->pkg_list;
	
	while (item != NULL)
	{
		GtkTreeIter iter;
		gboolean enabled = FALSE;
		/* that's good. We can add the package to the GtkListStore */
		gtk_list_store_append (GTK_LIST_STORE (store), &iter);
		
		/* check if we should enable or not the checkbox */
		if (g_hash_table_lookup (priv->enabled_packages_hash, item->data) == NULL) 		
			enabled = FALSE;
		else
			enabled = TRUE;		
		
		gtk_list_store_set (store, &iter, COLUMN_LOAD, enabled,
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
	SymbolDBPrefs *sdbp;
	SymbolDBPrefsPriv *priv;
	const gchar *path_str;
	GtkTreeIter iter;
	GtkTreePath *path;		
	GtkListStore *store;
	gboolean enabled;
	gchar *curr_package_name;
	
	pdata = (ParseableData *)user_data;
	path_str = pdata->path_str;
	sdbp = pdata->sdbp;
	priv = sdbp->priv;
	
	DEBUG_PRINT ("on_tag_load_toggled_parseable_cb %d", is_parseable);
	prefs_window = GTK_WINDOW (glade_xml_get_widget (priv->prefs_gxml, "symbol_db_pref_window"));
	treeview = glade_xml_get_widget (priv->prefs_gxml, "tags_treeview");
	prefs_progressbar = glade_xml_get_widget (priv->prefs_gxml, "prefs_progressbar");

	store = priv->prefs_list_store;
	path = gtk_tree_path_new_from_string (path_str);
	gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);
	gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
						COLUMN_LOAD, &enabled,
						COLUMN_NAME, &curr_package_name,
						-1);
	
	if (is_parseable == FALSE)
	{
		GtkWidget *wid = gtk_message_dialog_new (prefs_window, GTK_DIALOG_MODAL, 
												 GTK_MESSAGE_WARNING,
								GTK_BUTTONS_OK, _("Package is not parseable"));
		gtk_dialog_run (GTK_DIALOG (wid));
 		gtk_widget_destroy (wid);
		
		/* we for sure don't want this package on list next time */
		gtk_list_store_set (store, &iter, COLUMN_LOAD, FALSE, -1);
		
		/* emit the package-remove signal */
		g_signal_emit (sdbp, signals[PACKAGE_REMOVE], 0, curr_package_name); 		
	}
	else
	{
		/* we have a good parseable package. Let's mark the check enabled/disabled */
		enabled = !enabled;
		gtk_list_store_set (store, &iter, COLUMN_LOAD, enabled, -1);
		
		/* good, should we scan the packages? */
		if (enabled == TRUE)
		{			
			symbol_db_system_scan_package (priv->sdbs, curr_package_name);
			
			/* emit the package-add signal */
			g_signal_emit (sdbp, signals[PACKAGE_ADD], 0, curr_package_name);
		}
		else 
		{
			/* emit the package-remove signal */
			g_signal_emit (sdbp, signals[PACKAGE_REMOVE], 0, curr_package_name); 
		}
	}
	
	gtk_widget_set_sensitive (treeview, TRUE);
	gtk_widget_hide (prefs_progressbar);	
	gtk_tree_path_free (path);
	
	destroy_parseable_data (pdata);
}
				  
static void
on_tag_load_toggled (GtkCellRendererToggle *cell, char *path_str,
					 SymbolDBPrefs *sdbp)
{
	GtkTreeIter iter;
	GtkTreePath *path;
	gchar *curr_package_name;
	GtkListStore *store;
	GtkWidget *prefs_progressbar;
	GtkWidget *	treeview;
	ParseableData *pdata;
	SymbolDBPrefsPriv *priv;	
	
	priv = sdbp->priv;
	
	DEBUG_PRINT ("on_tag_load_toggled ()");
	
	store = priv->prefs_list_store;
	path = gtk_tree_path_new_from_string (path_str);
	gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);
	gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
						COLUMN_NAME, &curr_package_name,
						-1);
	gtk_tree_path_free (path);
	
	prefs_progressbar = glade_xml_get_widget (priv->prefs_gxml, "prefs_progressbar");
	gtk_widget_show_all (prefs_progressbar);	
	
	gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR (prefs_progressbar), 1.0);
	gtk_progress_bar_pulse (GTK_PROGRESS_BAR (prefs_progressbar));
	
	treeview = glade_xml_get_widget (priv->prefs_gxml, "tags_treeview");
	gtk_widget_set_sensitive (treeview, FALSE);
	
	pdata = g_new0 (ParseableData, 1);
	pdata->sdbp = sdbp;
	pdata->path_str = g_strdup (path_str);
	
	symbol_db_system_is_package_parseable (priv->sdbs, curr_package_name, 
										   on_tag_load_toggled_parseable_cb,
										   pdata);
}

static void
sdb_prefs_init1 (SymbolDBPrefs *sdbp)
{
	SymbolDBPrefsPriv *priv;
	GtkWidget *fchooser;
	gchar *ctags_value;

	priv = sdbp->priv;

	fchooser = 	glade_xml_get_widget (priv->prefs_gxml, CHOOSER_WIDGET);
	/* we will reactivate it after the listall has been finished */
	gtk_widget_set_sensitive (fchooser, FALSE);
			
	anjuta_preferences_add_page (priv->prefs, 
								 priv->prefs_gxml, 
								 GLADE_ROOT, 
								 _("Symbol Database"),  
								 ICON_FILE);
	
	ctags_value = anjuta_preferences_get (priv->prefs, CTAGS_PREFS_KEY);
	
	if (ctags_value == NULL || strlen (ctags_value) <= 0) 
	{
		ctags_value = g_strdup (CTAGS_PATH);
	}
	
	DEBUG_PRINT ("select ->%s<-", ctags_value);	
	/* FIXME: wtf?! */
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (fchooser), ctags_value);
	gtk_file_chooser_select_filename (GTK_FILE_CHOOSER (fchooser), ctags_value);
	
	g_signal_connect (G_OBJECT (fchooser), "selection-changed",
					  G_CALLBACK (on_prefs_executable_changed), sdbp);

	priv->prefs_notify_id = anjuta_preferences_notify_add (priv->prefs, 
												CTAGS_PREFS_KEY, 
											   on_gconf_notify_prefs, 
											   priv->prefs, NULL);		
	
	g_free (ctags_value);
}

static void
sdb_prefs_init (SymbolDBPrefs *object)
{
	SymbolDBPrefs *sdbp;
	SymbolDBPrefsPriv *priv;
	GtkWidget *treeview;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	gchar* exe_string = NULL;
	gboolean require_scan = FALSE;		/* scan for packages */

	sdbp = SYMBOL_DB_PREFS (object);
	sdbp->priv = g_new0 (SymbolDBPrefsPriv, 1);
	priv = sdbp->priv;
	
	priv->pkg_list = NULL;
	
	DEBUG_PRINT ("symbol_db_prefs_init ()");
	
	if (priv->prefs_gxml == NULL)
	{
		/* Create the preferences page */
		priv->prefs_gxml = glade_xml_new (GLADE_FILE, GLADE_ROOT, NULL);	
	}		
	
	/* init GtkListStore */
	if (priv->prefs_list_store == NULL) 
	{
		priv->prefs_list_store = gtk_list_store_new (COLUMN_MAX, G_TYPE_BOOLEAN, 
													   G_TYPE_STRING);
		require_scan = TRUE;
	}
	
	treeview = glade_xml_get_widget (priv->prefs_gxml, "tags_treeview");
	/* on_listall_exit will reactivate this */
	gtk_widget_set_sensitive (treeview, FALSE);
	gtk_tree_view_set_model (GTK_TREE_VIEW (treeview),
							 GTK_TREE_MODEL (priv->prefs_list_store));	

	/* Add the column for stock treeview */
	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (G_OBJECT (renderer), "toggled",
					  G_CALLBACK (on_tag_load_toggled), sdbp);
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
	frame3 = glade_xml_get_widget (priv->prefs_gxml, "frame3");
	gtk_widget_show_all (frame3);
	GtkWidget *prefs_progressbar = glade_xml_get_widget (priv->prefs_gxml, 
														 "prefs_progressbar");
	gtk_widget_hide (prefs_progressbar);	
	
	/* listall launcher thing */
	if (require_scan == TRUE) 
	{
		priv->pkg_config_launcher = anjuta_launcher_new ();

		anjuta_launcher_set_check_passwd_prompt (priv->pkg_config_launcher,
												 FALSE);

		g_signal_connect (G_OBJECT (priv->pkg_config_launcher), "child-exited",
					  	G_CALLBACK (on_listall_exit), sdbp);	
	
		exe_string = g_strdup ("pkg-config --list-all");
	
		anjuta_launcher_execute (priv->pkg_config_launcher,
							 	exe_string, on_listall_output,
							 	sdbp);
	}	
		
	/* unrefs unused memory objects */
	g_free (exe_string);
}

static void
sdb_prefs_finalize (GObject *object)
{
	SymbolDBPrefs *sdbp;
	SymbolDBPrefsPriv *priv;
	
	sdbp = SYMBOL_DB_PREFS (object);
	priv = sdbp->priv;
	
	DEBUG_PRINT ("symbol_db_prefs_finalize ()");
	
	anjuta_preferences_notify_remove(priv->prefs, priv->prefs_notify_id);
	anjuta_preferences_remove_page(priv->prefs, _("Symbol Database"));

	if (priv->pkg_config_launcher != NULL)
		g_object_unref (priv->pkg_config_launcher);
	priv->pkg_config_launcher = NULL;
	
	/* free pkg_list */
	g_list_foreach (priv->pkg_list, (GFunc)g_free, NULL);
	g_list_free (priv->pkg_list);
	priv->pkg_list = NULL;

	if (priv->prefs_gxml != NULL)
		g_object_unref (priv->prefs_gxml);

	if (priv->prefs_list_store != NULL)
		g_object_unref (priv->prefs_list_store);
	
	if (priv->enabled_packages_hash)
	{
		g_hash_table_destroy (priv->enabled_packages_hash);
	}
	
	G_OBJECT_CLASS (sdb_prefs_parent_class)->finalize (object);
}

static void
sdb_prefs_class_init (SymbolDBPrefsClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	signals[PACKAGE_ADD]
		= g_signal_new ("package-add",
						G_OBJECT_CLASS_TYPE (object_class),
						G_SIGNAL_RUN_FIRST,
						G_STRUCT_OFFSET (SymbolDBPrefsClass, package_add),
						NULL, NULL,
						g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 
						1,
						G_TYPE_STRING);

	signals[PACKAGE_REMOVE]
		= g_signal_new ("package-remove",
						G_OBJECT_CLASS_TYPE (object_class),
						G_SIGNAL_RUN_FIRST,
						G_STRUCT_OFFSET (SymbolDBPrefsClass, package_remove),
						NULL, NULL,
						g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 
						1,
						G_TYPE_STRING);	
	
	object_class->finalize = sdb_prefs_finalize;
}

SymbolDBPrefs *
symbol_db_prefs_new (SymbolDBSystem *sdbs, SymbolDBEngine *sdbe_project,
					 SymbolDBEngine *sdbe_globals, AnjutaPreferences *prefs,
					 GList *enabled_packages)
{
	SymbolDBPrefs *sdbp;
	SymbolDBPrefsPriv *priv;

	sdbp = g_object_new (SYMBOL_TYPE_DB_PREFS, NULL);
	
	priv = sdbp->priv;	
	
	priv->sdbs = sdbs;
	priv->prefs = prefs;
	priv->sdbe_project = sdbe_project;
	priv->sdbe_globals = sdbe_globals;
	priv->enabled_packages_hash = g_hash_table_new_full (g_str_hash, g_str_equal, 
													g_free, NULL);

	/* we'll convert the list of strings in input into an hash table, so that
	 * a lookup there will be done quicker
	 */
	GList *item = enabled_packages;
	while (item != NULL)
	{
		g_hash_table_insert (priv->enabled_packages_hash, (gpointer)g_strdup (item->data), 
							 (gpointer)TRUE);
		item = item->next;
	}	
	
	sdb_prefs_init1 (sdbp);	
	return sdbp;
}

