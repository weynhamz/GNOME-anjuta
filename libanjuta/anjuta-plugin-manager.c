/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-plugin-manager.c
 * Copyright (C) Naba Kumar  <naba@gnome.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * SECTION:anjuta-plugin-manager
 * @short_description: Plugins management and activation
 * @see_also: #AnjutaPlugin, #AnjutaProfileManager
 * @stability: Unstable
 * @include: libanjuta/anjuta-plugin-manager.h
 * 
 */

#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <libgnomevfs/gnome-vfs.h>

#include <libanjuta/anjuta-plugin-manager.h>
#include <libanjuta/anjuta-marshal.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-plugin-handle.h>
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-glue-factory.h>
#include <libanjuta/anjuta-glue-c.h>
#include <libanjuta/interfaces/ianjuta-plugin-loader.h>

enum
{
	PROP_0,

	PROP_SHELL,
	PROP_STATUS,
	PROP_PROFILES,
	PROP_AVAILABLE_PLUGINS,
	PROP_ACTIVATED_PLUGINS
};

enum
{
	PROFILE_PUSHED,
	PROFILE_POPPED,
	PLUGINS_TO_LOAD,
	PLUGINS_TO_UNLOAD,
	PLUGIN_ACTIVATED,
	PLUGIN_DEACTIVATED,

	LAST_SIGNAL
};

struct _AnjutaPluginManagerPriv
{
	GObject      *shell;
	AnjutaStatus *status;
	GList        *plugin_dirs;
	GList        *available_plugins;
	
	/* Indexes => plugin handles */
	GHashTable   *plugins_by_interfaces;
	GHashTable   *plugins_by_name;
	GHashTable   *plugins_by_description;
	
	/* Plugins that are currently activated */
	GHashTable   *activated_plugins;
	
	/* Plugins that have been previously loaded but current deactivated */
	GHashTable   *plugins_cache;
	
	/* Remember plugin selection */
	GHashTable   *remember_plugins;
};

/* Available plugins page treeview */
enum {
	COL_ACTIVABLE,
	COL_ENABLED,
	COL_ICON,
	COL_NAME,
	COL_PLUGIN,
	N_COLS
};

/* Remembered plugins page treeview */
enum {
	COL_REM_ICON,
	COL_REM_NAME,
	COL_REM_PLUGIN_KEY,
	N_REM_COLS
};

/* Plugin class types */

static GHashTable  *plugin_types = NULL;
static AnjutaGlueFactory *anjuta_glue_factory = NULL;

static GObjectClass* parent_class = NULL;
static guint plugin_manager_signals[LAST_SIGNAL] = { 0 };

static GHashTable* plugin_set_update (AnjutaPluginManager *plugin_manager,
									  AnjutaPluginHandle* selected_plugin,
									  gboolean load);

static GType get_plugin_loader_type (AnjutaPluginManager *plugin_manager,
								  const gchar *language);

GQuark 
anjuta_plugin_manager_error_quark (void)
{
	static GQuark quark = 0;
	
	if (quark == 0) {
		quark = g_quark_from_static_string ("anjuta-plugin-manager-quark");
	}
	return quark;
}

/** Dependency Resolution **/

static gboolean
collect_cycle (AnjutaPluginManager *plugin_manager,
			   AnjutaPluginHandle *base_plugin, AnjutaPluginHandle *cur_plugin, 
			   GList **cycle)
{
	AnjutaPluginManagerPriv *priv;
	GList *l;
	
	priv = plugin_manager->priv;
	
	for (l = anjuta_plugin_handle_get_dependency_names (cur_plugin);
		 l != NULL; l = l->next)
	{
		AnjutaPluginHandle *dep = g_hash_table_lookup (priv->plugins_by_name,
													   l->data);
		if (dep)
		{
			if (dep == base_plugin)
			{
				*cycle = g_list_prepend (NULL, dep);
				/* DEBUG_PRINT ("%s ", anjuta_plugin_handle_get_name (dep)); */
				return TRUE;
			}
			else
			{
				if (collect_cycle (plugin_manager, base_plugin, dep, cycle))
				{
					*cycle = g_list_prepend (*cycle, dep);
					/* DEBUG_PRINT ("%s ", anjuta_plugin_handle_get_name (dep)); */
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

static void
add_dependency (AnjutaPluginHandle *dependent, AnjutaPluginHandle *dependency)
{
	g_hash_table_insert (anjuta_plugin_handle_get_dependents (dependency),
						 dependent, dependency);
	g_hash_table_insert (anjuta_plugin_handle_get_dependencies (dependent),
						 dependency, dependent);
}

static void
child_dep_foreach_cb (gpointer key, gpointer value, gpointer user_data)
{
	add_dependency (ANJUTA_PLUGIN_HANDLE (user_data),
					ANJUTA_PLUGIN_HANDLE (key));
}

/* Resolves dependencies for a single module recursively.  Shortcuts if 
 * the module has already been resolved.  Returns a list representing
 * any cycles found, or NULL if no cycles are found.  If a cycle is found,
 * the graph is left unresolved.
 */
static GList*
resolve_for_module (AnjutaPluginManager *plugin_manager,
					AnjutaPluginHandle *plugin, int pass)
{
	AnjutaPluginManagerPriv *priv;
	GList *l;
	GList *ret = NULL;

	priv = plugin_manager->priv;
	
	if (anjuta_plugin_handle_get_checked (plugin))
	{
		return NULL;
	}

	if (anjuta_plugin_handle_get_resolve_pass (plugin) == pass)
	{
		GList *cycle = NULL;
		g_warning ("cycle found: %s on pass %d",
				   anjuta_plugin_handle_get_name (plugin),
				   anjuta_plugin_handle_get_resolve_pass (plugin));
		collect_cycle (plugin_manager, plugin, plugin, &cycle);
		return cycle;
	}
	
	if (anjuta_plugin_handle_get_resolve_pass (plugin) != -1)
	{
		return NULL;
	}	

	anjuta_plugin_handle_set_can_load (plugin, TRUE);
	anjuta_plugin_handle_set_resolve_pass (plugin, pass);
		
	for (l = anjuta_plugin_handle_get_dependency_names (plugin);
		 l != NULL; l = l->next)
	{
		char *dep = l->data;
		AnjutaPluginHandle *child = 
			g_hash_table_lookup (priv->plugins_by_name, dep);
		if (child)
		{
			ret = resolve_for_module (plugin_manager, child, pass);
			if (ret)
			{
				break;
			}
			
			/* Add the dependency's dense dependency list 
			 * to the current module's dense dependency list */
			g_hash_table_foreach (anjuta_plugin_handle_get_dependencies (child),
					      child_dep_foreach_cb, plugin);
			add_dependency (plugin, child);

			/* If the child can't load due to dependency problems,
			 * the current module can't either */
			anjuta_plugin_handle_set_can_load (plugin,
					anjuta_plugin_handle_get_can_load (child));
		} else {
			g_warning ("Dependency %s not found.\n", dep);
			anjuta_plugin_handle_set_can_load (plugin, FALSE);
			ret = NULL;
		}
	}
	anjuta_plugin_handle_set_checked (plugin, TRUE);
	
	return ret;
}

/* Clean up the results of a resolving run */
static void
unresolve_dependencies (AnjutaPluginManager *plugin_manager)
{
	AnjutaPluginManagerPriv *priv;
	GList *l;
	
	priv = plugin_manager->priv;
	
	for (l = priv->available_plugins; l != NULL; l = l->next)
	{
		AnjutaPluginHandle *plugin = l->data;
		anjuta_plugin_handle_unresolve_dependencies (plugin);
	}	
}

/* done upto here */

static void
prune_modules (AnjutaPluginManager *plugin_manager, GList *modules)
{
	AnjutaPluginManagerPriv *priv;
	GList *l;
	
	priv = plugin_manager->priv;
	
	for (l = modules; l != NULL; l = l->next) {
		AnjutaPluginHandle *plugin = l->data;
	
		g_hash_table_remove (priv->plugins_by_name,
							 anjuta_plugin_handle_get_id (plugin));
		priv->available_plugins = g_list_remove (priv->available_plugins, plugin);
	}
}

static int
dependency_compare (AnjutaPluginHandle *plugin_a,
					AnjutaPluginHandle *plugin_b)
{
	int a = g_hash_table_size (anjuta_plugin_handle_get_dependencies (plugin_a));
	int b = g_hash_table_size (anjuta_plugin_handle_get_dependencies (plugin_b));
	
	return a - b;
}

/* Resolves the dependencies of the priv->available_plugins list.  When this
 * function is complete, the following will be true: 
 *
 * 1) The dependencies and dependents hash tables of the modules will
 * be filled.
 * 
 * 2) Cycles in the graph will be removed.
 * 
 * 3) Modules which cannot be loaded due to failed dependencies will
 * be marked as such.
 *
 * 4) priv->available_plugins will be sorted such that no module depends on a
 * module after it.
 *
 * If a cycle in the graph is found, it is pruned from the tree and 
 * returned as a list stored in the cycles list.
 */
static void 
resolve_dependencies (AnjutaPluginManager *plugin_manager, GList **cycles)
{
	AnjutaPluginManagerPriv *priv;
	GList *cycle = NULL;
	GList *l;
	
	priv = plugin_manager->priv;
	*cycles = NULL;
	
	/* Try resolving dependencies.  If there is a cycle, prune the
	 * cycle and try to resolve again */
	do
	{
		int pass = 1;
		cycle = NULL;
		for (l = priv->available_plugins; l != NULL && !cycle; l = l->next) {
			cycle = resolve_for_module (plugin_manager, l->data, pass++);
			cycle = NULL;
		}
		if (cycle) {
			*cycles = g_list_prepend (*cycles, cycle);
			prune_modules (plugin_manager, cycle);
			unresolve_dependencies (plugin_manager);
		}
	} while (cycle);

	/* Now that there is a fully resolved dependency tree, sort
	 * priv->available_plugins to create a valid load order */
	priv->available_plugins = g_list_sort (priv->available_plugins, 
										   (GCompareFunc)dependency_compare);
}

/* Plugins loading */

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
load_plugin (AnjutaPluginManager *plugin_manager,
			 const gchar *plugin_desc_path)
{
	AnjutaPluginManagerPriv *priv;
	AnjutaPluginHandle *plugin_handle;
	
	g_return_if_fail (ANJUTA_IS_PLUGIN_MANAGER (plugin_manager));
	priv = plugin_manager->priv;
	
	plugin_handle = anjuta_plugin_handle_new (plugin_desc_path);
	if (plugin_handle)
	{
		if (g_hash_table_lookup (priv->plugins_by_name,
								 anjuta_plugin_handle_get_id (plugin_handle)))
		{
			g_object_unref (plugin_handle);
		}
		else
		{
			GList *node;
			/* Available plugin */
			priv->available_plugins = g_list_prepend (priv->available_plugins,
													  plugin_handle);
			/* Index by id */
			g_hash_table_insert (priv->plugins_by_name,
								 (gchar *)anjuta_plugin_handle_get_id (plugin_handle),
								 plugin_handle);
			
			/* Index by description */
			g_hash_table_insert (priv->plugins_by_description,
								 anjuta_plugin_handle_get_description (plugin_handle),
								 plugin_handle);
			
			/* Index by interfaces exported by this plugin */
			node = anjuta_plugin_handle_get_interfaces (plugin_handle);
			while (node)
			{
				GList *objs;
				gchar *iface;
				GList *obj_node;
				gboolean found;
				
				iface = node->data;
				objs = (GList*)g_hash_table_lookup (priv->plugins_by_interfaces, iface);
				
				obj_node = objs;
				found = FALSE;
				while (obj_node)
				{
					if (obj_node->data == plugin_handle)
					{
						found = TRUE;
						break;
					}
					obj_node = g_list_next (obj_node);
				}
				if (!found)
				{
					g_hash_table_steal (priv->plugins_by_interfaces, iface);
					objs = g_list_prepend (objs, plugin_handle);
					g_hash_table_insert (priv->plugins_by_interfaces, iface, objs);
				}
				node = g_list_next (node);
			}
		}
	}
	return;
}

static void
load_plugins_from_directory (AnjutaPluginManager* plugin_manager,
							 const gchar *dirname)
{
	DIR *dir;
	struct dirent *entry;
	
	dir = opendir (dirname);
	
	if (!dir)
	{
		return;
	}
	
	for (entry = readdir (dir); entry != NULL; entry = readdir (dir))
	{
		if (str_has_suffix (entry->d_name, ".plugin"))
		{
			gchar *pathname;
			pathname = g_strdup_printf ("%s/%s", dirname, entry->d_name);
			load_plugin (plugin_manager,pathname);
			g_free (pathname);
		}
	}
	closedir (dir);
}

/* Plugin activation and deactivation */

static void
on_plugin_activated (AnjutaPlugin *plugin_object, AnjutaPluginHandle *plugin)
{
	AnjutaPluginManager *plugin_manager;
	AnjutaPluginManagerPriv *priv;
	
	/* FIXME: Pass plugin_manager directly in signal arguments */
	plugin_manager = anjuta_shell_get_plugin_manager (plugin_object->shell, NULL);
	priv = plugin_manager->priv;
	
	g_hash_table_insert (priv->activated_plugins, plugin,
						 G_OBJECT (plugin_object));
	if (g_hash_table_lookup (priv->plugins_cache, plugin))
		g_hash_table_remove (priv->plugins_cache, plugin);
	
	g_signal_emit_by_name (plugin_manager, "plugin-activated",
						   anjuta_plugin_handle_get_description (plugin),
						   plugin_object);
}

static void
on_plugin_deactivated (AnjutaPlugin *plugin_object, AnjutaPluginHandle *plugin)
{
	AnjutaPluginManager *plugin_manager;
	AnjutaPluginManagerPriv *priv;
	
	/* FIXME: Pass plugin_manager directly in signal arguments */
	plugin_manager = anjuta_shell_get_plugin_manager (plugin_object->shell, NULL);
	priv = plugin_manager->priv;
	
	g_hash_table_insert (priv->plugins_cache, plugin, G_OBJECT (plugin_object));
	g_hash_table_remove (priv->activated_plugins, plugin);
	
	g_signal_emit_by_name (plugin_manager, "plugin-deactivated",
						   anjuta_plugin_handle_get_description (plugin),
						   plugin_object);
}

static GObject *
activate_plugin (AnjutaPluginManager *plugin_manager,
				 AnjutaPluginHandle *plugin, GError **error)
{
	AnjutaPluginManagerPriv *priv;
	GType type;
	GType glue_type;
	GObject *ret;
	gchar* lang = NULL;
	const gchar *plugin_id;
	const gchar *language;
	gboolean resident;
	
	priv = plugin_manager->priv;
	
	if (!plugin_types)
	{
		plugin_types = g_hash_table_new (g_str_hash, g_str_equal);
	}
	
	plugin_id = anjuta_plugin_handle_get_id (plugin);
	
	resident = anjuta_plugin_handle_get_resident (plugin);
	language = anjuta_plugin_handle_get_language (plugin);
	
	glue_type = get_plugin_loader_type (plugin_manager, language);
	if (glue_type == G_TYPE_INVALID)
	{
		g_set_error (error, ANJUTA_PLUGIN_MANAGER_ERROR,
					 ANJUTA_PLUGIN_MANAGER_ERROR_INVALID_TYPE,
					 _("Invalid plugin: %s"), plugin_id);
		
		return NULL;
	}
	
	type = GPOINTER_TO_UINT (g_hash_table_lookup (plugin_types, plugin_id));
	
	if (!type)
	{
		char **pieces;
		/* DEBUG_PRINT ("Loading: %s", plugin_id); */
		pieces = g_strsplit (plugin_id, ":", -1);
		type = anjuta_glue_factory_get_object_type (anjuta_glue_factory,
										     pieces[0], pieces[1],
										     resident, glue_type);
		g_hash_table_insert (plugin_types, g_strdup (plugin_id),
							 GUINT_TO_POINTER (type));
		g_strfreev (pieces);
	}
	g_free (lang);
	
	if (type == G_TYPE_INVALID)
	{
		g_set_error (error, ANJUTA_PLUGIN_MANAGER_ERROR,
					 ANJUTA_PLUGIN_MANAGER_ERROR_INVALID_TYPE,
					 _("Invalid plugin: %s"), plugin_id);
		ret = NULL;
	}
	else
	{
		ret = g_object_new (type, "shell", priv->shell, NULL);
		g_signal_connect (ret, "activated",
						  G_CALLBACK (on_plugin_activated), plugin);
		g_signal_connect (ret, "deactivated",
						  G_CALLBACK (on_plugin_deactivated), plugin);
	}
	return ret;
}

static gboolean
g_hashtable_foreach_true (gpointer key, gpointer value, gpointer user_data)
{
	return TRUE;
}

void
anjuta_plugin_manager_unload_all_plugins (AnjutaPluginManager *plugin_manager)
{
	AnjutaPluginManagerPriv *priv;
	
	priv = plugin_manager->priv;
	if (g_hash_table_size (priv->activated_plugins) > 0 ||
		g_hash_table_size (priv->plugins_cache) > 0)
	{
		priv->available_plugins = g_list_reverse (priv->available_plugins);
		if (g_hash_table_size (priv->activated_plugins) > 0)
		{
			GList *node;
			node = priv->available_plugins;
			while (node)
			{
				AnjutaPluginHandle *selected_plugin = node->data;
				if (g_hash_table_lookup (priv->activated_plugins, selected_plugin))
				{
					plugin_set_update (plugin_manager, selected_plugin, FALSE);
					/* DEBUG_PRINT ("Unloading plugin: %s",
								 anjuta_plugin_handle_get_id (selected_plugin));
					*/
				}
				node = g_list_next (node);
			}
			g_hash_table_foreach_remove (priv->activated_plugins,
										 g_hashtable_foreach_true, NULL);
		}
		if (g_hash_table_size (priv->plugins_cache) > 0)
		{
			GList *node;
			node = priv->available_plugins;
			while (node)
			{
				GObject *plugin_obj;
				AnjutaPluginHandle *selected_plugin = node->data;
				
				plugin_obj = g_hash_table_lookup (priv->plugins_cache,
												  selected_plugin);
				if (plugin_obj)
				{
					/* DEBUG_PRINT ("Destroying plugin: %s",
								 anjuta_plugin_handle_get_id (selected_plugin));
					*/
					g_object_unref (plugin_obj);
				}
				node = g_list_next (node);
			}
			g_hash_table_foreach_remove (priv->plugins_cache,
										 g_hashtable_foreach_true, NULL);
		}
		priv->available_plugins = g_list_reverse (priv->available_plugins);
	}
}

static gboolean 
should_unload (GHashTable *activated_plugins, AnjutaPluginHandle *plugin_to_unload,
			   AnjutaPluginHandle *plugin)
{
	GObject *plugin_obj = g_hash_table_lookup (activated_plugins, plugin);
	
	if (!plugin_obj)
		return FALSE;
	
	if (plugin_to_unload == plugin)
		return TRUE;
	
	gboolean dependent = 
		GPOINTER_TO_INT (g_hash_table_lookup (anjuta_plugin_handle_get_dependents (plugin),
											  plugin));
	return dependent;
}

static gboolean 
should_load (GHashTable *activated_plugins, AnjutaPluginHandle *plugin_to_load,
			 AnjutaPluginHandle *plugin)
{
	GObject *plugin_obj = g_hash_table_lookup (activated_plugins, plugin);
	
	if (plugin_obj)
		return FALSE;
	
	if (plugin_to_load == plugin)
		return anjuta_plugin_handle_get_can_load (plugin);
	
	gboolean dependency = 
		GPOINTER_TO_INT (g_hash_table_lookup (anjuta_plugin_handle_get_dependencies (plugin_to_load),
						 					  plugin));
	return (dependency && anjuta_plugin_handle_get_can_load (plugin));
}

static AnjutaPluginHandle *
plugin_for_iter (GtkListStore *store, GtkTreeIter *iter)
{
	AnjutaPluginHandle *plugin;
	
	gtk_tree_model_get (GTK_TREE_MODEL (store), iter, COL_PLUGIN, &plugin, -1);
	return plugin;
}

static void
update_enabled (GtkTreeModel *model, GHashTable *activated_plugins)
{
	GtkTreeIter iter;
	
	if (gtk_tree_model_get_iter_first (model, &iter)) {
		do {
			AnjutaPluginHandle *plugin;
			GObject *plugin_obj;
			gboolean installed;
			
			plugin = plugin_for_iter(GTK_LIST_STORE(model), &iter);
			plugin_obj = g_hash_table_lookup (activated_plugins, plugin);
			installed = (plugin_obj != NULL) ? TRUE : FALSE;
			gtk_tree_model_get (model, &iter, COL_PLUGIN, &plugin, -1);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter,
								COL_ENABLED, installed, -1);
		} while (gtk_tree_model_iter_next (model, &iter));
	}
}

static GHashTable*
plugin_set_update (AnjutaPluginManager *plugin_manager,
				 AnjutaPluginHandle* selected_plugin,
				 gboolean load)
{
	AnjutaPluginManagerPriv *priv;
	GObject *plugin_obj;
	GList *l;
	
	priv = plugin_manager->priv;
	plugin_obj = g_hash_table_lookup (priv->activated_plugins, selected_plugin);
	
	if (plugin_obj && load)
	{
		g_warning ("Trying to install already installed plugin '%s'",
				   anjuta_plugin_handle_get_name (selected_plugin));
		return priv->activated_plugins;
	}
	if (!plugin_obj && !load)
	{
		g_warning ("Trying to uninstall a not installed plugin '%s'",
				   anjuta_plugin_handle_get_name (selected_plugin));
		return priv->activated_plugins;
	}
	
	if (priv->status)
		anjuta_status_busy_push (priv->status);
	
	if (!load)
	{
		/* reverse priv->available_plugins when unloading, so that plugins are
		 * unloaded in the right order */
		priv->available_plugins = g_list_reverse (priv->available_plugins);

		for (l = priv->available_plugins; l != NULL; l = l->next)
		{
			AnjutaPluginHandle *plugin = l->data;
			if (should_unload (priv->activated_plugins, selected_plugin, plugin))
			{
				/* FIXME: Unload the class and sharedlib if possible */
				AnjutaPlugin *anjuta_plugin = ANJUTA_PLUGIN (plugin_obj);
				if (!anjuta_plugin_deactivate (ANJUTA_PLUGIN (anjuta_plugin)))
				{
					anjuta_util_dialog_info (GTK_WINDOW (priv->shell),
								 "Plugin '%s' do not want to be deactivated",
								 anjuta_plugin_handle_get_name (plugin));
				}
			}
		}
		priv->available_plugins = g_list_reverse (priv->available_plugins);
	}
	else
	{
		for (l = priv->available_plugins; l != NULL; l = l->next)
		{
			AnjutaPluginHandle *plugin = l->data;
			if (should_load (priv->activated_plugins, selected_plugin, plugin))
			{
				GObject *plugin_obj;
				plugin_obj = g_hash_table_lookup (priv->plugins_cache, plugin);
				if (!plugin_obj)
				{
					GError *error = NULL;
					plugin_obj = activate_plugin (plugin_manager, plugin,
												  &error);
				}
				
				if (plugin_obj)
				{
					anjuta_plugin_activate (ANJUTA_PLUGIN (plugin_obj));
				}
			}
		}
	}
	if (priv->status)
		anjuta_status_busy_pop (priv->status);
	return priv->activated_plugins;
}

static void
plugin_toggled (GtkCellRendererToggle *cell, char *path_str, gpointer data)
{
	AnjutaPluginManager *plugin_manager;
	AnjutaPluginManagerPriv *priv;
	GtkListStore *store = GTK_LIST_STORE (data);
	GtkTreeIter iter;
	GtkTreePath *path;
	AnjutaPluginHandle *plugin;
	gboolean enabled;
	GHashTable *activated_plugins;
	
	path = gtk_tree_path_new_from_string (path_str);
	
	plugin_manager = g_object_get_data (G_OBJECT (store), "plugin-manager");
	priv = plugin_manager->priv;
	
	gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);
	gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
			    COL_ENABLED, &enabled,
			    COL_PLUGIN, &plugin,
			    -1);
	
	enabled = !enabled;

	activated_plugins = plugin_set_update (plugin_manager, plugin, enabled);
	update_enabled (GTK_TREE_MODEL (store), activated_plugins);
	gtk_tree_path_free (path);
}

#if 0
static void
selection_changed (GtkTreeSelection *selection, GtkListStore *store)
{
	GtkTreeIter iter;
	
	if (gtk_tree_selection_get_selected (selection, NULL,
					     &iter)) {
		GtkTextBuffer *buffer;
		
		GtkWidget *txt = g_object_get_data (G_OBJECT (store),
						    "AboutText");
		
		GtkWidget *image = g_object_get_data (G_OBJECT (store),
						      "Icon");
		AnjutaPluginHandle *plugin = plugin_for_iter (store, &iter);
		
		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (txt));
		gtk_text_buffer_set_text (buffer, plugin->about, -1);

		if (plugin->icon_path) {
			gtk_image_set_from_file (GTK_IMAGE (image), 
						 plugin->icon_path);
			gtk_widget_show (GTK_WIDGET (image));
		} else {
			gtk_widget_hide (GTK_WIDGET (image));
		}
	}
}
#endif

static GtkWidget *
create_plugin_tree (void)
{
	GtkListStore *store;
	GtkWidget *tree;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	store = gtk_list_store_new (N_COLS,
								G_TYPE_BOOLEAN,
								G_TYPE_BOOLEAN,
								GDK_TYPE_PIXBUF,
								G_TYPE_STRING,
								G_TYPE_POINTER);
	tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));

	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (G_OBJECT (renderer), "toggled",
			  G_CALLBACK (plugin_toggled), store);
	column = gtk_tree_view_column_new_with_attributes (_("Load"),
													   renderer,
													   "active", 
													   COL_ENABLED,
													   "activatable",
													   COL_ACTIVABLE,
													   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
	gtk_tree_view_column_set_sizing (column,
									 GTK_TREE_VIEW_COLUMN_AUTOSIZE);

	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "pixbuf",
										COL_ICON);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "markup",
										COL_NAME);
	gtk_tree_view_column_set_sizing (column,
									 GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Available Plugins"));
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (tree), column);

	g_object_unref (store);
	return tree;
}

/* Sort function for plugins */
static gint
sort_plugins(gconstpointer a, gconstpointer b)
{
	g_return_val_if_fail (a != NULL, 0);
	g_return_val_if_fail (b != NULL, 0);
	
	AnjutaPluginHandle* plugin_a = ANJUTA_PLUGIN_HANDLE (a);
	AnjutaPluginHandle* plugin_b = ANJUTA_PLUGIN_HANDLE (b);
	
	return strcmp (anjuta_plugin_handle_get_name (plugin_a),
				   anjuta_plugin_handle_get_name (plugin_b));
}

/* If show_all == FALSE, show only user activatable plugins
 * If show_all == TRUE, show all plugins
 */
static void
populate_plugin_model (AnjutaPluginManager *plugin_manager,
					   GtkListStore *store,
					   GHashTable *plugins_to_show,
					   GHashTable *activated_plugins,
					   gboolean show_all)
{
	AnjutaPluginManagerPriv *priv;
	GList *l;
	
	priv = plugin_manager->priv;
	gtk_list_store_clear (store);
	
	priv->available_plugins = g_list_sort (priv->available_plugins, sort_plugins);
	
	for (l = priv->available_plugins; l != NULL; l = l->next)
	{
		AnjutaPluginHandle *plugin = l->data;
		
		/* If plugins to show is NULL, show all available plugins */
		if (plugins_to_show == NULL ||
			g_hash_table_lookup (plugins_to_show, plugin))
		{
			
			gboolean enable = FALSE;
			if (g_hash_table_lookup (activated_plugins, plugin))
				enable = TRUE;
			
			if (anjuta_plugin_handle_get_name (plugin) &&
				anjuta_plugin_handle_get_description (plugin) &&
				(anjuta_plugin_handle_get_user_activatable (plugin) ||
				 show_all))
			{
				GtkTreeIter iter;
				gchar *text;
				
				text = g_markup_printf_escaped ("<span size=\"larger\" weight=\"bold\">%s</span>\n%s",
												anjuta_plugin_handle_get_name (plugin),
												anjuta_plugin_handle_get_about (plugin));

				gtk_list_store_append (store, &iter);
				gtk_list_store_set (store, &iter,
									COL_ACTIVABLE,
									anjuta_plugin_handle_get_user_activatable (plugin),
									COL_ENABLED, enable,
									COL_NAME, text,
									COL_PLUGIN, plugin,
									-1);
				if (anjuta_plugin_handle_get_icon_path (plugin))
				{
					GdkPixbuf *icon;
					icon = gdk_pixbuf_new_from_file_at_size (anjuta_plugin_handle_get_icon_path (plugin),
															 48, 48, NULL);
					if (icon) {
						gtk_list_store_set (store, &iter,
											COL_ICON, icon, -1);
						gdk_pixbuf_unref (icon);
					}
				}
				g_free (text);
			}
		}
	}
}

static GtkWidget *
create_remembered_plugins_tree (void)
{
	GtkListStore *store;
	GtkWidget *tree;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	store = gtk_list_store_new (N_REM_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING,
								G_TYPE_STRING);
	tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
	
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "pixbuf",
										COL_REM_ICON);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "markup",
										COL_REM_NAME);
	gtk_tree_view_column_set_sizing (column,
									 GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Preferred plugins"));
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (tree), column);
	
	g_object_unref (store);
	return tree;
}

static void
foreach_remembered_plugin (gpointer key, gpointer value, gpointer user_data)
{
	AnjutaPluginDescription *desc = (AnjutaPluginDescription *) value;
	GtkListStore *store = GTK_LIST_STORE (user_data);
	AnjutaPluginManager *manager = g_object_get_data (G_OBJECT (store),
													  "plugin-manager");
	AnjutaPluginHandle *plugin =
		g_hash_table_lookup (manager->priv->plugins_by_description, desc);
	g_return_if_fail (plugin != NULL);
	
	if (anjuta_plugin_handle_get_name (plugin) &&
		anjuta_plugin_handle_get_description (plugin))
	{
		GtkTreeIter iter;
		gchar *text;
		
		text = g_markup_printf_escaped ("<span size=\"larger\" weight=\"bold\">%s</span>\n%s",
										anjuta_plugin_handle_get_name (plugin),
										anjuta_plugin_handle_get_about (plugin));

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
							COL_REM_NAME, text,
							COL_REM_PLUGIN_KEY, key,
							-1);
		if (anjuta_plugin_handle_get_icon_path (plugin))
		{
			GdkPixbuf *icon;
			icon = gdk_pixbuf_new_from_file_at_size (anjuta_plugin_handle_get_icon_path (plugin),
													 48, 48, NULL);
			if (icon) {
				gtk_list_store_set (store, &iter,
									COL_REM_ICON, icon, -1);
				gdk_pixbuf_unref (icon);
			}
		}
		g_free (text);
	}
}

static void
populate_remembered_plugins_model (AnjutaPluginManager *plugin_manager,
								   GtkListStore *store)
{
	AnjutaPluginManagerPriv *priv = plugin_manager->priv;
	gtk_list_store_clear (store);
	g_hash_table_foreach (priv->remember_plugins, foreach_remembered_plugin,
						  store);
}

static void
on_show_all_plugins_toggled (GtkToggleButton *button, GtkListStore *store)
{
	AnjutaPluginManager *plugin_manager;
	
	plugin_manager = g_object_get_data (G_OBJECT (button), "__plugin_manager");
	
	populate_plugin_model (plugin_manager, store, NULL,
						   plugin_manager->priv->activated_plugins,
						   !gtk_toggle_button_get_active (button));
}

static void
on_forget_plugin_clicked (GtkWidget *button, GtkTreeView *view)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		gchar *plugin_key;
		AnjutaPluginManager *manager = g_object_get_data (G_OBJECT (model),
														  "plugin-manager");
		gtk_tree_model_get (model, &iter, COL_REM_PLUGIN_KEY, &plugin_key, -1);
		g_hash_table_remove (manager->priv->remember_plugins, plugin_key);
		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
		g_free (plugin_key);
	}
}

static void
on_forget_plugin_sel_changed (GtkTreeSelection *selection,
							  GtkWidget *button)
{
	GtkTreeIter iter;
	
	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
		gtk_widget_set_sensitive (button, TRUE);
	else
		gtk_widget_set_sensitive (button, FALSE);
}

static GtkWidget *
create_plugin_page (AnjutaPluginManager *plugin_manager)
{
	GtkWidget *notebook;
	GtkWidget *page_label;
	GtkWidget *vbox;
	GtkWidget *checkbutton;
	GtkWidget *tree;
	GtkWidget *scrolled;
	GtkListStore *store;
	GtkWidget *hbox;
	GtkWidget *display_label;
	GtkWidget *forget_button;
	GtkTreeSelection *selection;
	
	notebook = gtk_notebook_new ();
	
	/* Plugins page */
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
	page_label = gtk_label_new (_("Plugins"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, page_label);
	
	checkbutton = gtk_check_button_new_with_label (_("Only show user activatable plugins"));
	gtk_container_set_border_width (GTK_CONTAINER (checkbutton), 10);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton), TRUE);
	gtk_box_pack_start (GTK_BOX (vbox), checkbutton, FALSE, FALSE, 0);
	
	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
									     GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
	
	tree = create_plugin_tree ();
	store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (tree)));

	populate_plugin_model (plugin_manager, store, NULL,
						   plugin_manager->priv->activated_plugins, FALSE);
	
	gtk_container_add (GTK_CONTAINER (scrolled), tree);
	g_object_set_data (G_OBJECT (store), "plugin-manager", plugin_manager);

	
	g_object_set_data (G_OBJECT (checkbutton), "__plugin_manager", plugin_manager);
	g_signal_connect (G_OBJECT (checkbutton), "toggled",
					  G_CALLBACK (on_show_all_plugins_toggled),
					  store);
	
	/* Remembered plugin */
	vbox = gtk_vbox_new (FALSE, 10);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
	
	display_label = gtk_label_new (_("These are the plugins selected by you "
									 "when Anjuta prompted to choose one of "
									 "many suitable plugins. Removing the "
									 "preferred plugin will let Anjuta prompt "
									 "you again to choose different plugin."));
	gtk_label_set_line_wrap (GTK_LABEL (display_label), TRUE);
	gtk_box_pack_start (GTK_BOX (vbox), display_label, FALSE, FALSE, 0);

	page_label = gtk_label_new (_("Preferred plugins"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, page_label);
	
	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
									     GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
	
	tree = create_remembered_plugins_tree ();
	store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (tree)));

	gtk_container_add (GTK_CONTAINER (scrolled), tree);
	g_object_set_data (G_OBJECT (store), "plugin-manager", plugin_manager);
	populate_remembered_plugins_model (plugin_manager, store);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	forget_button = gtk_button_new_with_label ("Forget selected plugin");
	gtk_widget_set_sensitive (forget_button, FALSE);
	gtk_box_pack_end (GTK_BOX (hbox), forget_button, FALSE, FALSE, 0);
	
	g_signal_connect (forget_button, "clicked",
					  G_CALLBACK (on_forget_plugin_clicked),
					  tree);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	g_signal_connect (selection, "changed",
					  G_CALLBACK (on_forget_plugin_sel_changed),
					  forget_button);
	
	/* For cursor status */
	
	gtk_widget_show_all (notebook);
	if (plugin_manager->priv->status)
		anjuta_status_add_widget (plugin_manager->priv->status, notebook);
	return notebook;
}

static GList *
property_to_list (const char *value)
{
	GList *l = NULL;
	char **split_str;
	char **p;
	
	split_str = g_strsplit (value, ",", -1);
	for (p = split_str; *p != NULL; p++) {
		l = g_list_prepend (l, g_strdup (g_strstrip (*p)));
	}
	g_strfreev (split_str);
	return l;
}

static GType
get_plugin_loader_type (AnjutaPluginManager *plugin_manager,
								  const gchar *language)
{
	AnjutaPluginManagerPriv *priv;
	AnjutaPluginHandle *plugin;
	GList *loader_plugins, *node;
	GList *valid_plugins;
	GObject *obj = NULL;
	static int bug = 0;
	
	g_return_val_if_fail (ANJUTA_IS_PLUGIN_MANAGER (plugin_manager), G_TYPE_INVALID);

	
	if ((language == NULL) || (g_ascii_strcasecmp (language, "C") == 0))
	{
		/* Support of C plugin is built-in */
		return ANJUTA_GLUE_TYPE_C_PLUGIN;
	}
	
	priv = plugin_manager->priv;
	plugin = NULL;
		
	/* Find all plugins implementing the IAnjutaPluginLoader interface. */
	loader_plugins = g_hash_table_lookup (priv->plugins_by_interfaces, "IAnjutaPluginLoader");

	/* Create a list of loader supporting this language */
	node = loader_plugins;
	valid_plugins = NULL;
	while (node)
	{
		AnjutaPluginDescription *desc;
		gchar *val;
		GList *vals = NULL;
		GList *l_node;
		gboolean found;
		
		plugin = node->data;

		desc = anjuta_plugin_handle_get_description (plugin);
		if (anjuta_plugin_description_get_string (desc, "Plugin Loader", "SupportedLanguage", &val))		
		{
			if (val != NULL)
			{	
				vals = property_to_list (val);
				g_free (val);
			}
		}
		
		found = FALSE;
		l_node = vals;
		while (l_node)
		{
			if (!found && (g_ascii_strcasecmp (l_node->data, language) == 0))
			{
				found = TRUE;
			}
			g_free (l_node->data);
			l_node = g_list_next (l_node);
		}
		g_list_free (vals);

		if (found)
		{
			valid_plugins = g_list_prepend (valid_plugins, plugin);
		}
		
		node = g_list_next (node);
	}
	
	/* Find the first installed plugin from the valid plugins */
	node = valid_plugins;
	while (node)
	{
		plugin = node->data;
		obj = g_hash_table_lookup (priv->activated_plugins, plugin);
		if (obj) break;
		node = g_list_next (node);
	}

	/* If no plugin is installed yet, do something */
	if ((obj == NULL) && valid_plugins && g_list_length (valid_plugins) == 1)
	{
		/* If there is just one plugin, consider it selected */
		plugin = valid_plugins->data;
		
		/* Install and return it */
		plugin_set_update (plugin_manager, plugin, TRUE);
		obj = g_hash_table_lookup (priv->activated_plugins, plugin);
	}
	else if ((obj == NULL) && valid_plugins)
	{
		/* Prompt the user to select one of these plugins */

		GList *descs = NULL;
		node = valid_plugins;
		while (node)
		{
			plugin = node->data;
			descs = g_list_prepend (descs, anjuta_plugin_handle_get_description (plugin));
			node = g_list_next (node);
		}
		descs = g_list_reverse (descs);
		obj = anjuta_plugin_manager_select_and_activate (plugin_manager,
								  _("Select a plugin"),
								  _("Please select a plugin to activate"),
								  descs);
		g_list_free (descs);
	}
	g_list_free (valid_plugins);

	if (obj != NULL)
	{
		/* Get type of glue object */
		IAnjutaPluginLoader *loader;
			
		loader = IANJUTA_PLUGIN_LOADER (ANJUTA_PLUGIN(obj));
		
		return ianjuta_plugin_loader_glue_plugin_get_type (loader, NULL);
	}
	
	/* No plugin implementing this interface found */
	g_warning ("No plugin found to load a %s plugin.", language);
	
	return G_TYPE_INVALID;
}

/**
 * anjuta_plugin_manager_get_plugin:
 * @plugin_manager: A #AnjutaPluginManager object
 * @iface_name: The interface implemented by the object to be found
 * 
 * Searches the currently available plugins to find the one which
 * implements the given interface as primary interface and returns it. If
 * the plugin is not yet loaded, it will be loaded and activated.
 * The returned object is garanteed to be an implementor of the
 * interface (as exported by the plugin metafile). It only searches
 * from the pool of plugin objects loaded in this shell and can only search
 * by primary interface. If there are more objects implementing this primary
 * interface, user might be prompted to select one from them (and might give
 * the option to use it as default for future queries). A typical usage of this
 * function is:
 * <programlisting>
 * GObject *docman =
 *     anjuta_plugin_manager_get_plugin (plugin_manager, "IAnjutaDocumentManager", error);
 * </programlisting>
 * Notice that this function takes the interface name string as string, unlike
 * anjuta_plugins_get_interface() which takes the type directly.
 *
 * Return value: The plugin object (subclass of #AnjutaPlugin) which implements
 * the given interface. See #AnjutaPlugin for more detail on interfaces
 * implemented by plugins.
 */
GObject *
anjuta_plugin_manager_get_plugin (AnjutaPluginManager *plugin_manager,
								  const gchar *iface_name)
{
	AnjutaPluginManagerPriv *priv;
	AnjutaPluginHandle *plugin;
	GList *valid_plugins, *node;
	
	g_return_val_if_fail (ANJUTA_IS_PLUGIN_MANAGER (plugin_manager), NULL);
	g_return_val_if_fail (iface_name != NULL, NULL);
	
	priv = plugin_manager->priv;
	plugin = NULL;
		
	/* Find all plugins implementing this (primary) interface. */
	valid_plugins = g_hash_table_lookup (priv->plugins_by_interfaces, iface_name);
	
	/* Find the first installed plugin from the valid plugins */
	node = valid_plugins;
	while (node)
	{
		GObject *obj;
		plugin = node->data;
		obj = g_hash_table_lookup (priv->activated_plugins, plugin);
		if (obj)
			return obj;
		node = g_list_next (node);
	}
	
	/* If no plugin is installed yet, do something */
	if (valid_plugins && g_list_length (valid_plugins) == 1)
	{
		/* If there is just one plugin, consider it selected */
		GObject *obj;
		plugin = valid_plugins->data;
		
		/* Install and return it */
		plugin_set_update (plugin_manager, plugin, TRUE);
		obj = g_hash_table_lookup (priv->activated_plugins, plugin);
		
		return obj;
	}
	else if (valid_plugins)
	{
		/* Prompt the user to select one of these plugins */
		GObject *obj;
		GList *descs = NULL;
		node = valid_plugins;
		while (node)
		{
			plugin = node->data;
			descs = g_list_prepend (descs, anjuta_plugin_handle_get_description (plugin));
			node = g_list_next (node);
		}
		descs = g_list_reverse (descs);
		obj = anjuta_plugin_manager_select_and_activate (plugin_manager,
									  _("Select a plugin"),
									  _("Please select a plugin to activate"),
									  descs);
		g_list_free (descs);
		return obj;
	}
	
	/* No plugin implementing this interface found */
	g_warning ("No plugin found implementing %s Interface.", iface_name);
	return NULL;
}

GObject *
anjuta_plugin_manager_get_plugin_by_id (AnjutaPluginManager *plugin_manager,
										const gchar *plugin_id)
{
	AnjutaPluginManagerPriv *priv;
	AnjutaPluginHandle *plugin;
	
	g_return_val_if_fail (ANJUTA_IS_PLUGIN_MANAGER (plugin_manager), NULL);
	g_return_val_if_fail (plugin_id != NULL, NULL);
	
	priv = plugin_manager->priv;
	plugin = g_hash_table_lookup (priv->plugins_by_name, plugin_id);
	if (plugin)
	{
		GObject *obj;
		obj = g_hash_table_lookup (priv->activated_plugins, plugin);
		if (obj)
		{
			return obj;
		} else
		{
			plugin_set_update (plugin_manager, plugin, TRUE);
			obj = g_hash_table_lookup (priv->activated_plugins, plugin);
			return obj;
		}
	}
	g_warning ("No plugin found with id \"%s\".", plugin_id);
	return NULL;
}

static void
on_activated_plugins_foreach (gpointer key, gpointer data, gpointer user_data)
{
	AnjutaPluginHandle *plugin = ANJUTA_PLUGIN_HANDLE (key);
	GList **active_plugins = (GList **)user_data;
	*active_plugins = g_list_prepend (*active_plugins,
						anjuta_plugin_handle_get_description (plugin));
}

GList*
anjuta_plugin_manager_get_active_plugins (AnjutaPluginManager *plugin_manager)
{
	GList *active_plugins = NULL;
	
	g_return_val_if_fail (ANJUTA_IS_PLUGIN_MANAGER (plugin_manager), NULL);
	g_hash_table_foreach (plugin_manager->priv->activated_plugins,
						  on_activated_plugins_foreach,
						  &active_plugins);
	return g_list_reverse (active_plugins);
}

gboolean
anjuta_plugin_manager_unload_plugin_by_id (AnjutaPluginManager *plugin_manager,
										   const gchar *plugin_id)
{
	AnjutaPluginManagerPriv *priv;
	AnjutaPluginHandle *plugin;
	
	g_return_val_if_fail (ANJUTA_IS_PLUGIN_MANAGER (plugin_manager), FALSE);
	g_return_val_if_fail (plugin_id != NULL, FALSE);
	
	priv = plugin_manager->priv;
	
	plugin = g_hash_table_lookup (priv->plugins_by_name, plugin_id);
	if (plugin)
	{
		plugin_set_update (plugin_manager, plugin, FALSE);
		
		/* Check if the plugin has been indeed unloaded */
		if (!g_hash_table_lookup (priv->activated_plugins, plugin))
			return TRUE;
		else
			return FALSE;
	}
	g_warning ("No plugin found with id \"%s\".", plugin_id);
	return FALSE;
}

static gboolean
find_plugin_for_object (gpointer key, gpointer value, gpointer data)
{
	if (value == data)
	{
		g_object_set_data (G_OBJECT (data), "__plugin_plugin", key);
		return TRUE;
	}
	return FALSE;
}

gboolean
anjuta_plugin_manager_unload_plugin (AnjutaPluginManager *plugin_manager,
									 GObject *plugin_object)
{
	AnjutaPluginManagerPriv *priv;
	AnjutaPluginHandle *plugin;
	
	g_return_val_if_fail (ANJUTA_IS_PLUGIN_MANAGER (plugin_manager), FALSE);
	g_return_val_if_fail (ANJUTA_IS_PLUGIN (plugin_object), FALSE);
	
	priv = plugin_manager->priv;
	
	plugin = NULL;

	/* Find the plugin that correspond to this plugin object */
	g_hash_table_find (priv->activated_plugins, find_plugin_for_object,
					   plugin_object);
	plugin = g_object_get_data (G_OBJECT (plugin_object), "__plugin_plugin");
	
	if (plugin)
	{
		plugin_set_update (plugin_manager, plugin, FALSE);
		
		/* Check if the plugin has been indeed unloaded */
		if (!g_hash_table_lookup (priv->activated_plugins, plugin))
			return TRUE;
		else
			return FALSE;
	}
	g_warning ("No plugin found with object \"%p\".", plugin_object);
	return FALSE;
}

GtkWidget *
anjuta_plugin_manager_get_dialog (AnjutaPluginManager *plugin_manager)
{
	return create_plugin_page (plugin_manager);
}

GList*
anjuta_plugin_manager_query (AnjutaPluginManager *plugin_manager,
							 const gchar *section_name,
							 const gchar *attribute_name,
							 const gchar *attribute_value,
							 ...)
{
	AnjutaPluginManagerPriv *priv;
	va_list var_args;
	GList *secs = NULL;
	GList *anames = NULL;
	GList *avalues = NULL;
	const gchar *sec = section_name;
	const gchar *aname = attribute_name;
	const gchar *avalue = attribute_value;
	GList *selected_plugins = NULL;
	GList *available;
	
	
	g_return_val_if_fail (ANJUTA_IS_PLUGIN_MANAGER (plugin_manager), NULL);
	
	priv = plugin_manager->priv;
	available = priv->available_plugins;
	
	if (section_name == NULL)
	{
		/* If no query is given, select all plugins */
		while (available)
		{
			AnjutaPluginHandle *plugin = available->data;
			AnjutaPluginDescription *desc =
				anjuta_plugin_handle_get_description (plugin);
			selected_plugins = g_list_prepend (selected_plugins, desc);
			available = g_list_next (available);
		}
		return g_list_reverse (selected_plugins);
	}
	
	g_return_val_if_fail (section_name != NULL, NULL);
	g_return_val_if_fail (attribute_name != NULL, NULL);
	g_return_val_if_fail (attribute_value != NULL, NULL);
	
	secs = g_list_prepend (secs, g_strdup (section_name));
	anames = g_list_prepend (anames, g_strdup (attribute_name));
	avalues = g_list_prepend (avalues, g_strdup (attribute_value));
	
	va_start (var_args, attribute_value);
	while (sec)
	{
		sec = va_arg (var_args, const gchar *);
		if (sec)
		{
			aname = va_arg (var_args, const gchar *);
			if (aname)
			{
				avalue = va_arg (var_args, const gchar *);
				if (avalue)
				{
					secs = g_list_prepend (secs, g_strdup (sec));
					anames = g_list_prepend (anames, g_strdup (aname));
					avalues = g_list_prepend (avalues, g_strdup (avalue));
				}
			}
		}
	}
	va_end (var_args);
	
	secs = g_list_reverse (secs);
	anames = g_list_reverse (anames);
	avalues = g_list_reverse (avalues);
	
	while (available)
	{
		GList* s_node = secs;
		GList* n_node = anames;
		GList* v_node = avalues;
		
		gboolean satisfied = FALSE;
		
		AnjutaPluginHandle *plugin = available->data;
		AnjutaPluginDescription *desc =
			anjuta_plugin_handle_get_description (plugin);
		
		while (s_node)
		{
			gchar *val;
			GList *vals;
			GList *node;
			gboolean found = FALSE;
			
			satisfied = TRUE;
			
			sec = s_node->data;
			aname = n_node->data;
			avalue = v_node->data;
			
			if (!anjuta_plugin_description_get_string (desc, sec, aname, &val))
			{
				satisfied = FALSE;
				break;
			}
			
			vals = property_to_list (val);
			g_free (val);
			
			node = vals;
			while (node)
			{
				if (strchr(node->data, '*') != NULL)
				{
					// Star match.
					gchar **segments;
					gchar **seg_ptr;
					const gchar *cursor;
					
					segments = g_strsplit (node->data, "*", -1);
					
					seg_ptr = segments;
					cursor = avalue;
					while (*seg_ptr != NULL)
					{
						if (strlen (*seg_ptr) > 0) {
							cursor = strstr (cursor, *seg_ptr);
							if (cursor == NULL)
								break;
						}
						cursor += strlen (*seg_ptr);
						seg_ptr++;
					}
					if (*seg_ptr == NULL)
						found = TRUE;
					g_strfreev (segments);
				}
				else if (g_ascii_strcasecmp (node->data, avalue) == 0)
				{
					// String match.
					found = TRUE;
				}
				g_free (node->data);
				node = g_list_next (node);
			}
			g_list_free (vals);
			if (!found)
			{
				satisfied = FALSE;
				break;
			}
			s_node = g_list_next (s_node);
			n_node = g_list_next (n_node);
			v_node = g_list_next (v_node);
		}
		if (satisfied)
		{
			selected_plugins = g_list_prepend (selected_plugins, desc);
			/* DEBUG_PRINT ("Satisfied, Adding %s",
						 anjuta_plugin_handle_get_name (plugin));*/
		}
		available = g_list_next (available);
	}
	anjuta_util_glist_strings_free (secs);
	anjuta_util_glist_strings_free (anames);
	anjuta_util_glist_strings_free (avalues);
	
	return g_list_reverse (selected_plugins);
}

enum {
	PIXBUF_COLUMN,
	PLUGIN_COLUMN,
	PLUGIN_DESCRIPTION_COLUMN,
	N_COLUMNS
};

static void
on_plugin_list_row_activated (GtkTreeView *tree_view,
							  GtkTreePath *path,
                              GtkTreeViewColumn *column,
                              GtkDialog *dialog)
{
	gtk_dialog_response (dialog, GTK_RESPONSE_OK);
}

AnjutaPluginDescription *
anjuta_plugin_manager_select (AnjutaPluginManager *plugin_manager,
							  gchar *title, gchar *description,
							  GList *plugin_descriptions)
{
	AnjutaPluginDescription *desc;
	AnjutaPluginManagerPriv *priv;
	GtkWidget *dlg;
	GtkTreeModel *model;
	GtkWidget *view;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GList *node;
	GtkWidget *label;
	GtkWidget *sc;
	GtkWidget *remember_checkbox;
	gint response;
	GtkTreeIter selected;
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GList *selection_ids = NULL;
	GString *remember_key = g_string_new ("");
	
	g_return_val_if_fail (title != NULL, NULL);
	g_return_val_if_fail (description != NULL, NULL);
	g_return_val_if_fail (plugin_descriptions != NULL, NULL);

	priv = plugin_manager->priv;
	
	if (g_list_length (plugin_descriptions) <= 0)
		return NULL;
		
	dlg = gtk_dialog_new_with_buttons (title, GTK_WINDOW (priv->shell),
									   GTK_DIALOG_DESTROY_WITH_PARENT,
									   GTK_STOCK_CANCEL,
									   GTK_RESPONSE_CANCEL,
									   GTK_STOCK_OK, GTK_RESPONSE_OK,
									   NULL);
	gtk_widget_set_size_request (dlg, 400, 300);
	gtk_window_set_default_size (GTK_WINDOW (dlg), 400, 300);

	label = gtk_label_new (description);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), label,
						FALSE, FALSE, 5);
	
	sc = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (sc);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sc),
										 GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sc),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dlg)->vbox), sc,
						TRUE, TRUE, 5);
	
	model = GTK_TREE_MODEL (gtk_list_store_new (N_COLUMNS, GDK_TYPE_PIXBUF,
										   G_TYPE_STRING, G_TYPE_POINTER));
	view = gtk_tree_view_new_with_model (model);
	gtk_widget_show (view);
	gtk_container_add (GTK_CONTAINER (sc), view);
	
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing (column,
									 GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Available Plugins"));

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "pixbuf",
										PIXBUF_COLUMN);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "markup",
										PLUGIN_COLUMN);

	gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (view), column);
	
	g_signal_connect (view, "row-activated",
					  G_CALLBACK (on_plugin_list_row_activated),
					  GTK_DIALOG(dlg));
	remember_checkbox =
		gtk_check_button_new_with_label (_("Remember this selection"));
	gtk_container_set_border_width (GTK_CONTAINER (remember_checkbox), 10);
	gtk_widget_show (remember_checkbox);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dlg)->vbox), remember_checkbox,
						FALSE, FALSE, 0);
	
	node = plugin_descriptions;
	while (node)
	{
		GdkPixbuf *icon_pixbuf = NULL;
		gchar *plugin_name = NULL;
		gchar *plugin_desc = NULL;
		gchar *icon_filename = NULL;
		gchar *location = NULL;
		
		desc = (AnjutaPluginDescription*)node->data;
		
		if (anjuta_plugin_description_get_string (desc,
												  "Anjuta Plugin",
												  "Icon",
												  &icon_filename))
		{
			gchar *icon_path = NULL;
			icon_path = g_strconcat (PACKAGE_PIXMAPS_DIR"/",
									 icon_filename, NULL);
			/* DEBUG_PRINT ("Icon: %s", icon_path); */
			icon_pixbuf = 
				gdk_pixbuf_new_from_file (icon_path, NULL);
			if (icon_pixbuf == NULL)
			{
				g_warning ("Plugin pixmap not found: %s", plugin_name);
			}
			g_free (icon_path);
		}
		else
		{
			g_warning ("Plugin does not define Icon attribute");
		}
		if (!anjuta_plugin_description_get_string (desc,
												  "Anjuta Plugin",
												  "Name",
												  &plugin_name))
		{
			g_warning ("Plugin does not define Name attribute");
		}
		if (!anjuta_plugin_description_get_string (desc,
												  "Anjuta Plugin",
												  "Description",
												  &plugin_desc))
		{
			g_warning ("Plugin does not define Description attribute");
		}
		if (!anjuta_plugin_description_get_string (desc,
												  "Anjuta Plugin",
												  "Location",
												  &location))
		{
			g_warning ("Plugin does not define Location attribute");
		}
		
		if (plugin_name && plugin_desc)
		{
			GtkTreeIter iter;
			gchar *text;
			
			text = g_markup_printf_escaped ("<span size=\"larger\" weight=\"bold\">%s</span>\n%s", plugin_name, plugin_desc);

			gtk_list_store_append (GTK_LIST_STORE (model), &iter);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter,
								PLUGIN_COLUMN, text,
								PLUGIN_DESCRIPTION_COLUMN, desc, -1);
			if (icon_pixbuf) {
				gtk_list_store_set (GTK_LIST_STORE (model), &iter,
									PIXBUF_COLUMN, icon_pixbuf, -1);
				g_object_unref (icon_pixbuf);
			}
			g_free (text);
			
			selection_ids = g_list_prepend (selection_ids, location);
		}
		node = g_list_next (node);
	}
	
	/* Prepare remembering key */
	selection_ids = g_list_sort (selection_ids,
								 (GCompareFunc)strcmp);
	node = selection_ids;
	while (node)
	{
		g_string_append (remember_key, (gchar*)node->data);
		g_string_append (remember_key, ",");
		node = g_list_next (node);
	}
	g_list_foreach (selection_ids, (GFunc) g_free, NULL);
	g_list_free (selection_ids);
	
	/* Find if the selection is remembered */
	desc = g_hash_table_lookup (priv->remember_plugins, remember_key->str);
	if (desc)
	{
		g_string_free (remember_key, TRUE);
		gtk_widget_destroy (dlg);
		return desc;
	}
	
	/* Prompt dialog */
	response = gtk_dialog_run (GTK_DIALOG (dlg));
	switch (response)
	{
	case GTK_RESPONSE_OK:
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
		if (gtk_tree_selection_get_selected (selection, &store,
											 &selected))
		{
			gtk_tree_model_get (model, &selected,
								PLUGIN_DESCRIPTION_COLUMN, &desc, -1);
			if (desc)
			{
				/* Remember selection */
				if (gtk_toggle_button_get_active
					(GTK_TOGGLE_BUTTON (remember_checkbox)))
				{
					/* DEBUG_PRINT ("Remembering selection '%s'",
								 remember_key->str);*/
					g_hash_table_insert (priv->remember_plugins,
										 g_strdup (remember_key->str), desc);
				}
				g_string_free (remember_key, TRUE);
				gtk_widget_destroy (dlg);
				return desc;
			}
		}
		break;
	}
	g_string_free (remember_key, TRUE);
	gtk_widget_destroy (dlg);
	return NULL;
}

GObject*
anjuta_plugin_manager_select_and_activate (AnjutaPluginManager *plugin_manager,
										   gchar *title,
										   gchar *description,
										   GList *plugin_descriptions)
{
	AnjutaPluginDescription *d;
	
	g_return_val_if_fail (ANJUTA_IS_PLUGIN_MANAGER (plugin_manager), NULL);
	
	d = anjuta_plugin_manager_select (plugin_manager, title, description,
									  plugin_descriptions);
	if (d)
	{
		GObject *plugin = NULL;	
		gchar *location = NULL;
		
		anjuta_plugin_description_get_string (d,
											  "Anjuta Plugin",
											  "Location",
											  &location);
		g_return_val_if_fail (location != NULL, NULL);
		plugin =
			anjuta_plugin_manager_get_plugin_by_id (plugin_manager, location);
		g_free (location);
		return plugin;
	}
	return NULL;
}

/* Plugin manager */

static void
anjuta_plugin_manager_init (AnjutaPluginManager *object)
{
	object->priv = g_new0 (AnjutaPluginManagerPriv, 1);
	object->priv->plugins_by_name = g_hash_table_new (g_str_hash, g_str_equal);
	object->priv->plugins_by_interfaces = g_hash_table_new_full (g_str_hash,
											   g_str_equal,
											   NULL,
											   (GDestroyNotify) g_list_free);
	object->priv->plugins_by_description = g_hash_table_new (g_direct_hash,
														   g_direct_equal);
	object->priv->activated_plugins = g_hash_table_new (g_direct_hash,
													  g_direct_equal);
	object->priv->plugins_cache = g_hash_table_new (g_direct_hash,
												  g_direct_equal);
	object->priv->remember_plugins = g_hash_table_new_full (g_str_hash,
															g_str_equal,
															g_free, NULL);
}

static void
anjuta_plugin_manager_finalize (GObject *object)
{
	AnjutaPluginManagerPriv *priv;
	priv = ANJUTA_PLUGIN_MANAGER (object)->priv;
	if (priv->available_plugins)
	{
		/* anjuta_plugin_manager_unload_all_plugins (ANJUTA_PLUGIN_MANAGER (object)); */
		g_list_foreach (priv->available_plugins, (GFunc)g_object_unref, NULL);
		g_list_free (priv->available_plugins);
		priv->available_plugins = NULL;
	}
	if (priv->activated_plugins)
	{
		g_hash_table_destroy (priv->activated_plugins);
		priv->activated_plugins = NULL;
	}
	if (priv->plugins_cache)
	{
		g_hash_table_destroy (priv->plugins_cache);
		priv->plugins_cache = NULL;
	}
	if (priv->plugins_by_name)
	{
		g_hash_table_destroy (priv->plugins_by_name);
		priv->plugins_by_name = NULL;
	}
	if (priv->plugins_by_description)
	{
		g_hash_table_destroy (priv->plugins_by_description);
		priv->plugins_by_description = NULL;
	}
	if (priv->plugins_by_interfaces)
	{
		g_hash_table_destroy (priv->plugins_by_interfaces);
		priv->plugins_by_interfaces = NULL;
	}
	if (priv->plugin_dirs)
	{
		g_list_foreach (priv->plugin_dirs, (GFunc)g_free, NULL);
		g_list_free (priv->plugin_dirs);
		priv->plugin_dirs = NULL;
	}
#if 0
	if (plugin_types)
	{
		g_hash_table_destroy (plugin_types);
		plugin_types = NULL;
	}
	if (anjuta_glue_factory)
	{
		g_object_unref (anjuta_glue_factory);
		anjuta_glue_factory = NULL;
	}
#endif
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
anjuta_plugin_manager_set_property (GObject *object, guint prop_id,
									const GValue *value, GParamSpec *pspec)
{
	AnjutaPluginManagerPriv *priv;
	
	g_return_if_fail (ANJUTA_IS_PLUGIN_MANAGER (object));
	priv = ANJUTA_PLUGIN_MANAGER (object)->priv;
	
	switch (prop_id)
	{
	case PROP_STATUS:
		priv->status = g_value_get_object (value);
		break;
	case PROP_SHELL:
		priv->shell = g_value_get_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
anjuta_plugin_manager_get_property (GObject *object, guint prop_id,
									GValue *value, GParamSpec *pspec)
{
	AnjutaPluginManagerPriv *priv;
	
	g_return_if_fail (ANJUTA_IS_PLUGIN_MANAGER (object));
	priv = ANJUTA_PLUGIN_MANAGER (object)->priv;
	
	switch (prop_id)
	{
	case PROP_SHELL:
		g_value_set_object (value, priv->shell);
		break;
	case PROP_STATUS:
		g_value_set_object (value, priv->status);
		break;
	case PROP_AVAILABLE_PLUGINS:
		g_value_set_pointer (value, priv->available_plugins);
		break;
	case PROP_ACTIVATED_PLUGINS:
		g_value_set_pointer (value, priv->activated_plugins);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}
static void
anjuta_plugin_manager_plugin_activated (AnjutaPluginManager *self,
										AnjutaPluginDescription* plugin_desc,
										GObject *plugin)
{
	/* TODO: Add default signal handler implementation here */
}

static void
anjuta_plugin_manager_plugin_deactivated (AnjutaPluginManager *self,
										  AnjutaPluginDescription* plugin_desc,
										  GObject *plugin)
{
	/* TODO: Add default signal handler implementation here */
}

static void
anjuta_plugin_manager_class_init (AnjutaPluginManagerClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));

	object_class->finalize = anjuta_plugin_manager_finalize;
	object_class->set_property = anjuta_plugin_manager_set_property;
	object_class->get_property = anjuta_plugin_manager_get_property;

	klass->plugin_activated = anjuta_plugin_manager_plugin_activated;
	klass->plugin_deactivated = anjuta_plugin_manager_plugin_deactivated;

	g_object_class_install_property (object_class,
	                                 PROP_PROFILES,
	                                 g_param_spec_pointer ("profiles",
	                                                       _("Profiles"),
	                                                       _("Current stack of profiles"),
	                                                       G_PARAM_READABLE));
	g_object_class_install_property (object_class,
	                                 PROP_AVAILABLE_PLUGINS,
	                                 g_param_spec_pointer ("available-plugins",
	                                                       _("Available plugins"),
	                                                       _("Currently available plugins found in plugin paths"),
	                                                       G_PARAM_READABLE));

	g_object_class_install_property (object_class,
	                                 PROP_ACTIVATED_PLUGINS,
	                                 g_param_spec_pointer ("activated-plugins",
	                                                       _("Activated plugins"),
	                                                       _("Currently activated plugins"),
	                                                       G_PARAM_READABLE));
	g_object_class_install_property (object_class,
	                                 PROP_SHELL,
	                                 g_param_spec_object ("shell",
														  _("Anjuta Shell"),
														  _("Anjuta shell for which the plugins are"),
														  G_TYPE_OBJECT,
														  G_PARAM_READABLE |
														  G_PARAM_WRITABLE |
														  G_PARAM_CONSTRUCT));
	g_object_class_install_property (object_class,
	                                 PROP_STATUS,
	                                 g_param_spec_object ("status",
														  _("Anjuta Status"),
														  _("Anjuta status to use in loading and unloading of plugins"),
														  ANJUTA_TYPE_STATUS,
														  G_PARAM_READABLE |
														  G_PARAM_WRITABLE |
														  G_PARAM_CONSTRUCT));
	
	plugin_manager_signals[PLUGIN_ACTIVATED] =
		g_signal_new ("plugin-activated",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (AnjutaPluginManagerClass,
									   plugin_activated),
		              NULL, NULL,
					  anjuta_cclosure_marshal_VOID__POINTER_OBJECT,
		              G_TYPE_NONE, 2,
		              G_TYPE_POINTER, ANJUTA_TYPE_PLUGIN);

	plugin_manager_signals[PLUGIN_DEACTIVATED] =
		g_signal_new ("plugin-deactivated",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (AnjutaPluginManagerClass,
									   plugin_deactivated),
		              NULL, NULL,
					  anjuta_cclosure_marshal_VOID__POINTER_OBJECT,
		              G_TYPE_NONE, 2,
		              G_TYPE_POINTER, ANJUTA_TYPE_PLUGIN);
}

GType
anjuta_plugin_manager_get_type (void)
{
	static GType our_type = 0;

	if(our_type == 0)
	{
		static const GTypeInfo our_info =
		{
			sizeof (AnjutaPluginManagerClass), /* class_size */
			(GBaseInitFunc) NULL, /* base_init */
			(GBaseFinalizeFunc) NULL, /* base_finalize */
			(GClassInitFunc) anjuta_plugin_manager_class_init, /* class_init */
			(GClassFinalizeFunc) NULL, /* class_finalize */
			NULL /* class_data */,
			sizeof (AnjutaPluginManager), /* instance_size */
			0, /* n_preallocs */
			(GInstanceInitFunc) anjuta_plugin_manager_init, /* instance_init */
			NULL /* value_table */
		};
		our_type = g_type_register_static (G_TYPE_OBJECT,
										   "AnjutaPluginManager",
		                                   &our_info, 0);
	}

	return our_type;
}

AnjutaPluginManager*
anjuta_plugin_manager_new (GObject *shell, AnjutaStatus *status,
						   GList* plugins_directories)
{
	GObject *manager_object;
	AnjutaPluginManager *plugin_manager;
	GList *cycles = NULL;
	const char *gnome2_path;
	char **pathv;
	char **p;
	GList *node;
	GList *plugin_dirs = NULL;

	/* Initialize the anjuta plugin system */
	manager_object = g_object_new (ANJUTA_TYPE_PLUGIN_MANAGER,
								   "shell", shell, "status", status, NULL);
	plugin_manager = ANJUTA_PLUGIN_MANAGER (manager_object);
	
	if (anjuta_glue_factory == NULL)
	{
		anjuta_glue_factory = anjuta_glue_factory_new ();
	}
	
	gnome2_path = g_getenv ("GNOME2_PATH");
	if (gnome2_path) {
		pathv = g_strsplit (gnome2_path, ":", 1);
	
		for (p = pathv; *p != NULL; p++) {
			char *path = g_strdup (*p);
			plugin_dirs = g_list_prepend (plugin_dirs, path);
			anjuta_glue_factory_add_path (anjuta_glue_factory, path);
		}
		g_strfreev (pathv);
	}
	
	node = plugins_directories;
	while (node) {
		if (!node->data)
			continue;
		char *path = g_strdup (node->data);
		plugin_dirs = g_list_prepend (plugin_dirs, path);
		anjuta_glue_factory_add_path (anjuta_glue_factory, path);
		node = g_list_next (node);
	}
	plugin_dirs = g_list_reverse (plugin_dirs);
	/* load_plugins (); */

	node = plugin_dirs;
	while (node)
	{
		load_plugins_from_directory (plugin_manager, (char*)node->data);
		node = g_list_next (node);
	}
	resolve_dependencies (plugin_manager, &cycles);
	return plugin_manager;
}

void
anjuta_plugin_manager_activate_plugins (AnjutaPluginManager *plugin_manager,
										GList *plugins_to_activate)
{
	AnjutaPluginManagerPriv *priv;
	GdkPixbuf *icon_pixbuf;
	GList *node;
	
	priv = plugin_manager->priv;
	
	/* Freeze shell operations */
	anjuta_shell_freeze (ANJUTA_SHELL (priv->shell), NULL);
	if (plugins_to_activate)
	{
		anjuta_status_progress_add_ticks (ANJUTA_STATUS (priv->status),
										  g_list_length (plugins_to_activate));
	}
	node = plugins_to_activate;
	while (node)
	{
		AnjutaPluginDescription *d;
		gchar *plugin_id;
		gchar *icon_filename, *label;
		gchar *icon_path = NULL;
		
		d = node->data;
		
		icon_pixbuf = NULL;
		label = NULL;
		if (anjuta_plugin_description_get_string (d, "Anjuta Plugin",
												  "Icon",
											  &icon_filename))
		{
			gchar *title, *description;
			anjuta_plugin_description_get_string (d, "Anjuta Plugin",
												  "Name",
												  &title);
			anjuta_plugin_description_get_string (d, "Anjuta Plugin",
												  "Description",
												  &description);
			icon_path = g_strconcat (PACKAGE_PIXMAPS_DIR"/",
									 icon_filename, NULL);
			/* DEBUG_PRINT ("Icon: %s", icon_path); */
			label = g_strconcat (_("Loaded: "), title, _("..."), NULL);
			icon_pixbuf = gdk_pixbuf_new_from_file (icon_path, NULL);
			if (!icon_pixbuf)
				g_warning ("Plugin does not define Icon: No such file %s",
						   icon_path);
			g_free (icon_path);
			g_free (icon_filename);
		}
		
		if (anjuta_plugin_description_get_string (d, "Anjuta Plugin",
												  "Location", &plugin_id))
		{
			GObject *plugin_obj;
			
			plugin_obj =
				anjuta_plugin_manager_get_plugin_by_id (plugin_manager,
														plugin_id);
			g_free (plugin_id);
		}
		anjuta_status_progress_tick (ANJUTA_STATUS (priv->status),
									 icon_pixbuf, label);
		g_free (label);
		if (icon_pixbuf)
			g_object_unref (icon_pixbuf);
		
		node = g_list_next (node);
	}
	
	/* Thaw shell operations */
	anjuta_shell_thaw (ANJUTA_SHELL (priv->shell), NULL);
}

static void
on_collect (gpointer key, gpointer value, gpointer user_data)
{
	gchar *id;
	gchar *query = (gchar*) key;
	AnjutaPluginDescription *desc = (AnjutaPluginDescription *) value;
	GString *write_buffer = (GString *) user_data;
	
	anjuta_plugin_description_get_string (desc, "Anjuta Plugin", "Location",
										  &id);
	g_string_append_printf (write_buffer, "%s=%s;", query, id);
	g_free (id);
}

gchar*
anjuta_plugin_manager_get_remembered_plugins (AnjutaPluginManager *plugin_manager)
{
	AnjutaPluginManagerPriv *priv;
	GString *write_buffer = g_string_new ("");
	
	g_return_val_if_fail (ANJUTA_IS_PLUGIN_MANAGER (plugin_manager), FALSE);
	
	priv = plugin_manager->priv;
	g_hash_table_foreach (priv->remember_plugins, on_collect,
						  write_buffer);
	return g_string_free (write_buffer, FALSE);
}

static gboolean
on_foreach_remove_true (gpointer k, gpointer v, gpointer d)
{
	return TRUE;
}

void
anjuta_plugin_manager_set_remembered_plugins (AnjutaPluginManager *plugin_manager,
											  const gchar *remembered_plugins)
{
	AnjutaPluginManagerPriv *priv;
	gchar **strv_lines, **line_idx;
	
	g_return_if_fail (ANJUTA_IS_PLUGIN_MANAGER (plugin_manager));
	g_return_if_fail (remembered_plugins != NULL);
	
	priv = plugin_manager->priv;
	
	g_hash_table_foreach_remove (priv->remember_plugins,
								 on_foreach_remove_true, NULL);
	
	strv_lines = g_strsplit (remembered_plugins, ";", -1);
	line_idx = strv_lines;
	while (*line_idx)
	{
		gchar **strv_keyvals;
		strv_keyvals = g_strsplit (*line_idx, "=", -1);
		if (strv_keyvals && strv_keyvals[0] && strv_keyvals[1])
		{
			AnjutaPluginHandle *plugin;
			plugin = g_hash_table_lookup (priv->plugins_by_name,
										  strv_keyvals[1]);
			if (plugin)
			{
				AnjutaPluginDescription *desc;
				desc = anjuta_plugin_handle_get_description (plugin);
				/*
				DEBUG_PRINT ("Restoring remember plugin: %s=%s",
							 strv_keyvals[0],
							 strv_keyvals[1]);
				*/
				g_hash_table_insert (priv->remember_plugins,
									 g_strdup (strv_keyvals[0]), desc);
			}
			g_strfreev (strv_keyvals);
		}
		line_idx++;
	}
	g_strfreev (strv_lines);
}
