#include <config.h>

#include <sys/types.h>
#include <dirent.h>

#include <libanjuta/libanjuta.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/glue-factory.h>

#include <libgnome/gnome-config.h>
#include <libgnome/gnome-util.h>
#include <libgnomeui/gnome-uidefs.h>

#include <gtk/gtk.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include "plugins.h"
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-plugin-description.h>
#include <e-splash.h>
#include "resources.h"
#include <libanjuta/anjuta-debug.h>
#include <string.h>

typedef struct {
	char *id;
	char *name;
	char *about;
	char *icon_path;
	gboolean user_activatable;
	
	AnjutaPluginDescription *description;

	/* The dependencies listed in the oaf file */
	GSList *dependency_names;

	/* Interfaces exported by this plugin */
	GSList *interfaces;
	
	/* Attributes defined for this plugin */
	// GHashTable *attributes;
	
	/* The keys of these tables represent the dependencies and dependents
	 * of the module.  The values point back at the tool */
	GHashTable *dependencies;
	GHashTable *dependents;
	
	gboolean can_load;
	gboolean checked;

	/* The pass on which the module was resolved, or -1 if
	 * unresolved */
	int resolve_pass;
} AvailableTool;

enum {
	COL_ACTIVABLE,
	COL_ENABLED,
	COL_ICON,
	COL_NAME,
	COL_TOOL,
	N_COLS
};

static GList       *plugin_dirs = NULL;
static GSList      *available_tools = NULL;
static GHashTable  *tools_by_interfaces = NULL;
static GHashTable  *tools_by_name = NULL;
static GHashTable  *tools_by_description = NULL;
static GHashTable  *tool_types = NULL;
static GlueFactory *glue_factory = NULL;

static GHashTable* tool_set_update (AnjutaShell *shell,
									AvailableTool* selected_tool,
									gboolean load);

static char *
get_icon_path (char *icon_name)
{
	char *ret;
	
	if (g_path_is_absolute (icon_name)) {
		ret = g_strdup (icon_name);
	} else {
		ret = anjuta_res_get_pixmap_file (icon_name);
	}
	
	return ret;
}

/** Dependency Resolution **/
static GSList *
property_to_slist (const char *value)
{
	GSList *l = NULL;
	char **split_str;
	char **p;
		
	split_str = g_strsplit (value, ",", -1);
	for (p = split_str; *p != NULL; p++) {
		l = g_slist_prepend (l, g_strdup (g_strstrip (*p)));
	}
	
	g_strfreev (split_str);

	return l;
}

static void
destroy_tool (AvailableTool *tool)
{
	GSList *l;
	
	if (tool->id)
		g_free (tool->id);
	if (tool->name) 
		g_free (tool->name);
	if (tool->icon_path)
		g_free (tool->icon_path);

	for (l = tool->dependency_names; l != NULL; l = l->next) {
		g_free (l->data);
	}
	
	g_slist_free (tool->dependency_names);
	
	g_free (tool);
}

static gboolean
collect_cycle (AvailableTool *base_tool, AvailableTool *cur_tool, 
	       GSList **cycle)
{
	GSList *l;
	
	for (l = cur_tool->dependency_names; l != NULL; l = l->next) {
		AvailableTool *dep = g_hash_table_lookup (tools_by_name,
							  l->data);
		if (dep) {
			if (dep == base_tool) {
				*cycle = g_slist_prepend (NULL, dep);
				g_print ("%s ", dep->name);
				return TRUE;
			} else {
				if (collect_cycle (base_tool, dep, cycle)) {
					*cycle = g_slist_prepend (*cycle, dep);
				g_print ("%s ", dep->name);

					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

static void
add_dependency (AvailableTool *dependent, AvailableTool *dependency)
{
	g_hash_table_insert (dependency->dependents,
			     dependent, dependency);
	g_hash_table_insert (dependent->dependencies,
			     dependency, dependent);

}

static void
child_dep_foreach_cb (gpointer key, gpointer value, gpointer user_data)
{
	add_dependency ((AvailableTool *)user_data, (AvailableTool *)key);
}

/* Resolves dependencies for a single module recursively.  Shortcuts if 
 * the module has already been resolved.  Returns a list representing
 * any cycles found, or NULL if no cycles are found.  If a cycle is found,
 * the graph is left unresolved. */
static GSList *
resolve_for_module (AvailableTool *tool, int pass)
{
	GSList *l;
	GSList *ret = NULL;

	if (tool->checked) {
		return NULL;
	}

	if (tool->resolve_pass == pass) {
		GSList *cycle = NULL;
		g_warning ("cycle found: %s on pass %d", tool->name, pass);

		collect_cycle (tool, tool, &cycle);
		return cycle;
	}
	
	if (tool->resolve_pass != -1) {
		return NULL;
	}	

	tool->can_load = TRUE;
	tool->resolve_pass = pass;
		
	for (l = tool->dependency_names; l != NULL; l = l->next) {
		char *dep = l->data;
		AvailableTool *child = 
			g_hash_table_lookup (tools_by_name, dep);
		if (child) {
			ret = resolve_for_module (child, pass);
			if (ret) {
				break;
			}
			
			/* Add the dependency's dense dependency list 
			 * to the current module's dense dependency list */
			g_hash_table_foreach (child->dependencies, 
					      child_dep_foreach_cb, tool);
			add_dependency (tool, child);

			/* If the child can't load due to dependency problems,
			 * the current module can't either */
			tool->can_load = child->can_load;
		} else {
			g_warning ("Dependency %s not found.\n", 
				   dep);
			tool->can_load = FALSE;
			ret = NULL;
		}
	}

	tool->checked = TRUE;
	
	return ret;
}

/* Clean up the results of a resolving run */
static void
unresolve_dependencies (void)
{
	GSList *l;
	for (l = available_tools; l != NULL; l = l->next) {
		AvailableTool *tool = l->data;
		
		if (tool->dependencies) {
			g_hash_table_destroy (tool->dependencies);
			tool->dependencies = g_hash_table_new (g_direct_hash, 
							       g_direct_equal);
		}
		if (tool->dependents) {
			g_hash_table_destroy (tool->dependents);
			tool->dependents = g_hash_table_new (g_direct_hash, 
							     g_direct_equal);

		}
		tool->can_load = TRUE;
		tool->resolve_pass = -1;
	}	
}

static void
prune_modules (GSList *modules)
{
	GSList *l;
	for (l = modules; l != NULL; l = l->next) {
		AvailableTool *tool = l->data;
	
		g_hash_table_remove (tools_by_name, tool->id);
		available_tools = g_slist_remove (available_tools, tool);
	}
}

static int
dependency_compare (const AvailableTool *tool_a, const AvailableTool *tool_b)
{
	int a = g_hash_table_size (tool_a->dependencies);
	int b = g_hash_table_size (tool_b->dependencies);
	
	return a - b;
}


/* Resolves the dependencies of the available_tools list.  When this
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
 * 4) available_tools will be sorted such that no module depends on a
 * module after it.
 *
 * If a cycle in the graph is found, it is pruned from the tree and 
 * returned as a list stored in the cycles list.
 */
static void 
resolve_dependencies (GSList **cycles) {
	GSList *cycle = NULL;
	GSList *l;
	
	*cycles = NULL;
	
	/* Try resolving dependencies.  If there is a cycle, prune the
	 * cycle and try to resolve again */
	do {
		int pass = 1;
		cycle = NULL;
		for (l = available_tools; l != NULL && !cycle; l = l->next) {
			cycle = resolve_for_module (l->data, pass++);
			cycle = NULL;
		}
		if (cycle) {
			*cycles = g_slist_prepend (*cycles, cycle);
			prune_modules (cycle);
			unresolve_dependencies ();
		}
	} while (cycle);

	/* Now that there is a fully resolved dependency tree, sort
	 * available_tools to create a valid load order */
	available_tools = g_slist_sort (available_tools, 
					(GCompareFunc)dependency_compare);
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

static AvailableTool *
tool_from_description (AnjutaPluginDescription *desc)
{
	char *str;
	AvailableTool *tool = g_new0 (AvailableTool, 1);
	gboolean success = TRUE;
	
	tool->description = desc;
	tool->user_activatable = TRUE;
	
	if (anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
											  "Location", &str)) {
		tool->id = str;
	} else {
		g_warning ("Couldn't find 'Location'");
		success = FALSE;
	}
	
	if (anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
											  "Name", &str)) {
		tool->name = str;
	} else {
		g_warning ("couldn't find 'Name' attribute.");
		success = FALSE;
	}


	if (anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
											  "Description", &str)) {
		tool->about = str;
	} else {
		g_warning ("Couldn't find 'Description' attribute.");
		success = FALSE;
	}

	if (anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
									   "Icon", &str)) {
		tool->icon_path = get_icon_path (str);
		g_free (str);
	}

	if (anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
											  "Dependencies",
											  &str)) {
		tool->dependency_names = property_to_slist (str);
		g_free (str);
	}

	if (anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
											  "Interfaces",
											  &str)) {
		tool->interfaces = property_to_slist (str);
		g_free (str);
	}
	
	if (anjuta_plugin_description_get_string (desc, "Anjuta Plugin",
											  "UserActivatable", &str)) {
		if (str && strcasecmp (str, "no") == 0)
		{
			tool->user_activatable = FALSE;
			DEBUG_PRINT ("Plugin '%s' is not user activatable",
						 tool->name? tool->name : "Unknown");
		}
		g_free (str);
	}

	tool->resolve_pass = -1;

	tool->dependencies = g_hash_table_new (g_direct_hash, 
					       g_direct_equal);
	tool->dependents = g_hash_table_new (g_direct_hash, 
					     g_direct_equal);

	if (!success) {
		destroy_tool (tool);
		tool = NULL;
	}

	return tool;
}

static void
load_tool (AnjutaPluginDescription *desc)
{
	AvailableTool *tool;
	
	tool = tool_from_description (desc);
	if (tool) {
		if (g_hash_table_lookup (tools_by_name, tool->id)) {
			destroy_tool (tool);
		} else {
			GSList *node;
			available_tools = g_slist_prepend (available_tools, tool);
			g_hash_table_insert (tools_by_name, tool->id, tool);
			g_hash_table_insert (tools_by_description, desc, tool);
			node = tool->interfaces;
			/* Map interfaces exported by this plugin */
			while (node) {
				GSList *objs;
				gchar *iface;
				GSList *obj_node;
				gboolean found;
				
				iface = node->data;
				objs = g_hash_table_lookup (tools_by_interfaces, iface);
				
				obj_node = objs;
				found = FALSE;
				while (obj_node) {
					if (obj_node->data == tool) {
						found = TRUE;
						break;
					}
					obj_node = g_slist_next (obj_node);
				}
				if (!found) {
					objs = g_slist_prepend (objs, tool);
					g_hash_table_insert (tools_by_interfaces, iface, objs);
				}
				node = g_slist_next (node);
			}
		}
	}
	return;
}

static void
load_tool_description (const gchar *path)
{
	gchar *contents;

	if (g_file_get_contents (path, &contents, NULL, NULL)) {
		AnjutaPluginDescription *desc;
		
		desc = anjuta_plugin_description_new_from_string (contents, NULL);
		g_free (contents);
		if (!desc) {
			g_warning ("Bad plugin file: %s\n", path);
			return;
		}
		
		load_tool (desc);
	}
}

static void
load_tools_from_directory (const gchar *dirname)
{
	DIR *dir;
	struct dirent *entry;
	
	dir = opendir (dirname);
	
	if (!dir) {
		return;
	}
	
	for (entry = readdir (dir); entry != NULL; entry = readdir (dir)) {
		if (str_has_suffix (entry->d_name, ".plugin")) {
			gchar *pathname;

			pathname = g_strdup_printf ("%s/%s", dirname, entry->d_name);
			
			load_tool_description (pathname);

			g_free (pathname);
		}
	}
	closedir (dir);
}

static void
load_available_tools (void)
{
	GSList *cycles;
	GList *l;
	tools_by_name = g_hash_table_new (g_str_hash, g_str_equal);
	tools_by_interfaces = g_hash_table_new (g_str_hash, g_str_equal);
	tools_by_description = g_hash_table_new (g_direct_hash, g_direct_equal);

	for (l = plugin_dirs; l != NULL; l = l->next) {
		load_tools_from_directory ((char*)l->data);
	}

	resolve_dependencies (&cycles);
}

static void
unload_available_tools (void)
{
	GSList *l;
	
	for (l = available_tools; l != NULL; l = l->next) {
		destroy_tool ((AvailableTool*)l->data);
	}
	g_slist_free (available_tools);
	available_tools = NULL;

	if (tools_by_name)
	{
		g_hash_table_destroy (tools_by_name);
		tools_by_name = NULL;
	}
}

static GObject *
activate_tool (AnjutaShell *shell, AvailableTool *tool)
{
	GType type;
	GObject *ret;
	if (!tool_types) {
		tool_types = g_hash_table_new (g_str_hash, g_str_equal);
	}
	
	type = GPOINTER_TO_UINT (g_hash_table_lookup (tool_types, tool->id));
	
	if (!type) {
		char **pieces;
		DEBUG_PRINT ("Loading: %s", tool->id);
		pieces = g_strsplit (tool->id, ":", -1);
		type = glue_factory_get_object_type (glue_factory,
										     pieces[0], pieces[1]);
		g_hash_table_insert (tool_types, g_strdup (tool->id),
							 GUINT_TO_POINTER (type));
		g_strfreev (pieces);
	}
	
	if (type == G_TYPE_INVALID) {
		g_warning ("Invalid type: Can not load %s\n", tool->id);
		ret = NULL;
	} else {
		ret = g_object_new (type, "shell", shell, NULL);
	}
	return ret;
}

/* Initialize the anjuta tool system */
void
anjuta_plugins_init (GList *plugins_directories)
{
	static gboolean initialized = FALSE;
	const char *gnome2_path;
	char **pathv;
	char **p;
	GList *node;

	if (initialized) {
		return;
	}

	initialized = TRUE;

	glue_factory = glue_factory_new ();
	
	gnome2_path = g_getenv ("GNOME2_PATH");
	if (gnome2_path) {
		pathv = g_strsplit (gnome2_path, ":", 1);
	
		for (p = pathv; *p != NULL; p++) {
			char *path = g_strdup (*p);
			plugin_dirs = g_list_prepend (plugin_dirs, path);
			glue_factory_add_path (glue_factory, path);
		}
		g_strfreev (pathv);
	}

	node = plugins_directories;
	while (node) {
		if (!node->data)
			continue;
		char *path = g_strdup (node->data);
		plugin_dirs = g_list_prepend (plugin_dirs, path);
		glue_factory_add_path (glue_factory, path);
		node = g_list_next (node);
	}
	plugin_dirs = g_list_reverse (plugin_dirs);
	load_available_tools ();
}

void
anjuta_plugins_unload_all (AnjutaShell *shell)
{
	GHashTable *installed_tools;
	GHashTable *tools_cache;
	
	installed_tools = g_object_get_data (G_OBJECT (shell), "InstalledTools");
	tools_cache =  g_object_get_data (G_OBJECT (shell), "ToolsCache");
	if (installed_tools || tools_cache)
	{
		available_tools = g_slist_reverse (available_tools);
		if (installed_tools)
		{
			GSList *node;
			node = available_tools;
			while (node)
			{
				AvailableTool *selected_tool = node->data;
				if (g_hash_table_lookup (installed_tools, selected_tool))
				{
					tool_set_update (shell, selected_tool, FALSE);
					DEBUG_PRINT ("Unloading plugin: %s", selected_tool->id);
				}
				node = g_slist_next (node);
			}
			g_hash_table_destroy (installed_tools);
			g_object_set_data (G_OBJECT (shell), "InstalledTools", NULL);
		}
		if (tools_cache)
		{
			GSList *node;
			node = available_tools;
			while (node)
			{
				GObject *tool_obj;
				AvailableTool *selected_tool = node->data;
				
				tool_obj = g_hash_table_lookup (tools_cache, selected_tool);
				if (tool_obj)
				{
					DEBUG_PRINT ("Destroying plugin: %s", selected_tool->id);
					g_object_unref (tool_obj);
				}
				node = g_slist_next (node);
			}
			g_hash_table_destroy (tools_cache);
			g_object_set_data (G_OBJECT (shell), "ToolsCache", NULL);
		}
		available_tools = g_slist_reverse (available_tools);
	}
}

void
anjuta_plugins_finalize (void)
{
	if (available_tools)
	{
		unload_available_tools ();
		available_tools = NULL;
	}
	if (plugin_dirs)
	{
		g_list_foreach (plugin_dirs, (GFunc)g_free, NULL);
		g_list_free (plugin_dirs);
		plugin_dirs = NULL;
	}
	if (tools_by_interfaces)
	{
		g_hash_table_destroy (tools_by_interfaces);
		tools_by_interfaces = NULL;
	}
	if (tools_by_name)
	{
		g_hash_table_destroy (tools_by_name);
		tools_by_name = NULL;
	}
	if (tools_by_description)
	{
		g_hash_table_destroy (tools_by_description);
		tools_by_description = NULL;
	}
	if (tool_types)
	{
		g_hash_table_destroy (tool_types);
		tool_types = NULL;
	}
	if (glue_factory)
	{
		g_object_unref (glue_factory);
		glue_factory = NULL;
	}
}

static gboolean 
should_unload (GHashTable *installed_tools, AvailableTool *tool_to_unload,
			   AvailableTool *tool)
{
	GObject *tool_obj = g_hash_table_lookup (installed_tools, tool);
	
	if (!tool_obj)
		return FALSE;
	
	if (tool_to_unload == tool)
		return TRUE;
	
	gboolean dependent = 
		GPOINTER_TO_INT (g_hash_table_lookup (tool_to_unload->dependents,
											  tool));
	return dependent;
}

static gboolean 
should_load (GHashTable *installed_tools, AvailableTool *tool_to_load,
			 AvailableTool *tool)
{
	GObject *tool_obj = g_hash_table_lookup (installed_tools, tool);
	
	if (tool_obj)
		return FALSE;
	
	if (tool_to_load == tool)
		return tool->can_load;
	
	gboolean dependency = 
		GPOINTER_TO_INT (g_hash_table_lookup (tool_to_load->dependencies,
						 					  tool));
	return (dependency && tool->can_load);
}

static AvailableTool *
tool_for_iter (GtkListStore *store, GtkTreeIter *iter)
{
	AvailableTool *tool;
	
	gtk_tree_model_get (GTK_TREE_MODEL (store), iter, COL_TOOL, &tool, -1);
	return tool;
}

static void
update_enabled (GtkTreeModel *model, GHashTable *installed_tools)
{
	GtkTreeIter iter;
	
	if (gtk_tree_model_get_iter_first (model, &iter)) {
		do {
			AvailableTool *tool;
			GObject *tool_obj;
			gboolean installed;
			
			tool = tool_for_iter(GTK_LIST_STORE(model), &iter);
			tool_obj = g_hash_table_lookup (installed_tools, tool);
			installed = (tool_obj != NULL) ? TRUE : FALSE;
			gtk_tree_model_get (model, &iter, COL_TOOL, &tool, -1);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter,
								COL_ENABLED, installed, -1);
		} while (gtk_tree_model_iter_next (model, &iter));
	}
}

static GHashTable*
tool_set_update (AnjutaShell *shell, AvailableTool* selected_tool,
				 gboolean load)
{
	AnjutaStatus *status;
	GObject *tool_obj;
	GHashTable *installed_tools, *tools_cache;
	GSList *l;
	
	installed_tools = g_object_get_data (G_OBJECT (shell), "InstalledTools");
	tools_cache =  g_object_get_data (G_OBJECT (shell), "ToolsCache");
	if (!tools_cache) {
		tools_cache = g_hash_table_new (g_direct_hash, g_direct_equal);
		g_object_set_data (G_OBJECT (shell), "ToolsCache", tools_cache);
	}
	tool_obj = g_hash_table_lookup (installed_tools, selected_tool);
	
	if (tool_obj && load) {
		g_warning ("Trying to install already installed plugin '%s'",
				   selected_tool->name);
		return installed_tools;
	}
	if (!tool_obj && !load) {
		g_warning ("Trying to uninstall a not installed plugin '%s'",
				   selected_tool->name);
		return installed_tools;
	}
	
	status = anjuta_shell_get_status (shell, NULL);
	anjuta_status_busy_push (status);
	
	if (!load) {
		/* reverse available_tools when unloading, so that plugins are
		 * unloaded in the right order */
		available_tools = g_slist_reverse (available_tools);

		for (l = available_tools; l != NULL; l = l->next) {
			AvailableTool *tool = l->data;
			if (should_unload (installed_tools, selected_tool, tool)) {
				/* FIXME: Unload the class and sharedlib if possible */
				gboolean success;
				AnjutaPlugin *anjuta_tool = ANJUTA_PLUGIN (tool_obj);
				
				success = anjuta_plugin_deactivate (ANJUTA_PLUGIN (anjuta_tool));
				if (success) {
					// g_object_unref (tool_obj);
					g_hash_table_insert (tools_cache, tool, tool_obj);
					g_hash_table_remove (installed_tools, tool);
				} else {
					anjuta_util_dialog_info (GTK_WINDOW (shell),
								 "Plugin '%s' do not want to be deactivated",
											 tool->name);
				}
			}
		}
		available_tools = g_slist_reverse (available_tools);
	} else {	
		for (l = available_tools; l != NULL; l = l->next) {
			AvailableTool *tool = l->data;
			if (should_load (installed_tools, selected_tool, tool)) {
				GObject *tool_obj;
				tool_obj = g_hash_table_lookup (tools_cache, tool);
				if (!tool_obj)
					tool_obj = activate_tool (shell, tool);
				else
					anjuta_plugin_activate (ANJUTA_PLUGIN (tool_obj));
				if (tool_obj) {
					g_hash_table_insert (installed_tools, tool, tool_obj);
					g_hash_table_remove (tools_cache, tool);
				}
			}
		}
	}
	anjuta_status_busy_pop (status);
	return installed_tools;
}

static void
plugin_toggled (GtkCellRendererToggle *cell, char *path_str, gpointer data)
{
	GtkListStore *store = GTK_LIST_STORE (data);
	GtkTreeIter iter;
	GtkTreePath *path;
	AvailableTool *tool;
	gboolean enabled;
	AnjutaShell *shell;
	GHashTable *installed_tools;
	
	path = gtk_tree_path_new_from_string (path_str);
	
	shell = g_object_get_data (G_OBJECT (store), "Shell");
	
	gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);
	gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
			    COL_ENABLED, &enabled,
			    COL_TOOL, &tool,
			    -1);
	
	enabled = !enabled;

	installed_tools = tool_set_update (shell, tool, enabled);
	update_enabled (GTK_TREE_MODEL (store), installed_tools);
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
		AvailableTool *tool = tool_for_iter (store, &iter);
		
		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (txt));
		gtk_text_buffer_set_text (buffer, tool->about, -1);

		if (tool->icon_path) {
			gtk_image_set_from_file (GTK_IMAGE (image), 
						 tool->icon_path);
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

/* If show_all == FALSE, show only user activatable plugins
 * If show_all == TRUE, show all plugins
 */
static void
populate_plugin_model (GtkListStore *store,
					   GHashTable *tools_to_show,
					   GHashTable *installed_tools,
					   gboolean show_all)
{
	GSList *l;
	
	gtk_list_store_clear (store);
	
	for (l = available_tools; l != NULL; l = l->next) {
		AvailableTool *tool = l->data;
		
		/* If tools to show is NULL, show all available tools */
		if (tools_to_show == NULL ||
			g_hash_table_lookup (tools_to_show, tool)) {
			
			gboolean enable = FALSE;
			if (g_hash_table_lookup (installed_tools, tool))
				enable = TRUE;
			
			if (tool->name && tool->description &&
				(tool->user_activatable || show_all))
			{
				GtkTreeIter iter;
				gchar *text;
				
				text = g_markup_printf_escaped ("<span size=\"larger\" weight=\"bold\">%s</span>\n%s", tool->name, tool->about);

				gtk_list_store_append (store, &iter);
				gtk_list_store_set (store, &iter,
									COL_ACTIVABLE, tool->user_activatable,
									COL_ENABLED, enable,
									COL_NAME, text,
									COL_TOOL, tool,
									-1);
				if (tool->icon_path) {
					GdkPixbuf *icon;
					icon = gdk_pixbuf_new_from_file_at_size (tool->icon_path,
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

static void
on_show_all_plugins_toggled (GtkToggleButton *button, GtkListStore *store)
{
	AnjutaShell *shell;
	GHashTable *installed_tools;
	
	shell = g_object_get_data (G_OBJECT (button), "__shell");
	installed_tools = g_object_get_data (G_OBJECT(shell), "InstalledTools");
	
	/* FIXME: This should be moved to a proper location */
	if (!installed_tools)
	{
		installed_tools = g_hash_table_new (g_direct_hash, g_direct_equal);
		g_object_set_data (G_OBJECT (shell), "InstalledTools",
						   installed_tools);
	}
	populate_plugin_model (store, NULL, installed_tools,
						   !gtk_toggle_button_get_active (button));
}

static GtkWidget *
create_plugin_page (AnjutaShell *shell)
{
	GtkWidget *vbox;
	GtkWidget *checkbutton;
	GtkWidget *tree;
	GtkWidget *scrolled;
	GtkListStore *store;
	GHashTable *installed_tools;
	AnjutaStatus *status;

	vbox = gtk_vbox_new (FALSE, 0);
	
	checkbutton = gtk_check_button_new_with_label (_("Only show user activatable plugins"));
	gtk_container_set_border_width (GTK_CONTAINER (checkbutton), 10);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton), TRUE);
	gtk_widget_show (checkbutton);
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

	installed_tools = g_object_get_data (G_OBJECT(shell), "InstalledTools");
	
	/* FIXME: This should be moved to a proper location */
	if (!installed_tools)
	{
		installed_tools = g_hash_table_new (g_direct_hash, g_direct_equal);
		g_object_set_data (G_OBJECT (shell), "InstalledTools",
						   installed_tools);
	}

	populate_plugin_model (GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (tree))),
						   NULL, installed_tools, FALSE);
	
	gtk_container_add (GTK_CONTAINER (scrolled), tree);
	g_object_set_data (G_OBJECT (store), "Shell", shell);

	gtk_widget_show_all (vbox);
	
	status = anjuta_shell_get_status (shell, NULL);
	anjuta_status_add_widget (status, vbox);
	
	g_object_set_data (G_OBJECT (checkbutton), "__shell", shell);
	g_signal_connect (G_OBJECT (checkbutton), "toggled",
					  G_CALLBACK (on_show_all_plugins_toggled),
					  store);
	
	return vbox;
}

/**
 * anjuta_plugins_get_plugin:
 * @shell: A #AnjutaShell interface
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
 *     anjuta_plugins_get_plugin (shell, "IAnjutaDocumentManager", error);
 * </programlisting>
 * Notice that this function takes the interface name string as string, unlike
 * anjuta_plugins_get_interface() which takes the type directly.
 *
 * Return value: The plugin object (subclass of #AnjutaPlugin) which implements
 * the given interface. See #AnjutaPlugin for more detail on interfaces
 * implemented by plugins.
 */
GObject *
anjuta_plugins_get_plugin (AnjutaShell *shell,
						   const gchar *iface_name)
{
	AvailableTool *tool;
	GSList *valid_tools, *node;
	GHashTable *installed_tools;
	
	g_return_val_if_fail (ANJUTA_IS_SHELL (shell), NULL);
	g_return_val_if_fail (iface_name != NULL, NULL);
	
	tool = NULL;
	
	/* Get the installed plugins hash, create if not yet created */
	installed_tools = g_object_get_data (G_OBJECT (shell), "InstalledTools");
	if (installed_tools == NULL)
	{
		installed_tools = g_hash_table_new (g_direct_hash, g_direct_equal);
		g_object_set_data (G_OBJECT (shell), "InstalledTools",
						   installed_tools);
	}
	
	/* Find all plugins implementing this (primary) interface. */
	valid_tools = g_hash_table_lookup (tools_by_interfaces, iface_name);
	
	/* Find the first installed plugin from the valid plugins */
	node = valid_tools;
	while (node) {
		GObject *obj;
		tool = node->data;
		obj = g_hash_table_lookup (installed_tools, tool);
		if (obj)
			return obj;
		node = g_slist_next (node);
	}
	
	/* If no plugin is installed yet, do something */
	if (valid_tools && g_slist_length (valid_tools) == 1) {
		
		/* If there is just one plugin, consider it selected */
		GObject *obj;
		tool = valid_tools->data;
		
		/* Install and return it */
		tool_set_update (shell, tool, TRUE);
		obj = g_hash_table_lookup (installed_tools, tool);
		
		return obj;
	
	} else if (valid_tools) {
		
		/* Prompt the user to select one of these plugins */
		GObject *obj;
		GSList *descs = NULL;
		node = valid_tools;
		while (node) {
			tool = node->data;
			descs = g_slist_prepend (descs, tool->description);
			node = g_slist_next (node);
		}
		descs = g_slist_reverse (descs);
		obj = anjuta_plugins_select_and_activate (shell,
												  "Select a plugin",
												  "Please select a plugin to activate",
												  descs);
		g_slist_free (descs);
		return obj;
	}
	
	/* No plugin implementing this interface found */
	g_warning ("No plugin found implementing %s Interface.", iface_name);
	return NULL;
}

GObject *
anjuta_plugins_get_plugin_by_id (AnjutaShell *shell,
								 const gchar *plugin_id)
{
	AvailableTool *tool;
	GHashTable *installed_tools;
	
	g_return_val_if_fail (ANJUTA_IS_SHELL (shell), NULL);
	g_return_val_if_fail (plugin_id != NULL, NULL);
	
	tool = NULL;
	installed_tools = g_object_get_data (G_OBJECT (shell), "InstalledTools");
	if (installed_tools == NULL)
	{
		installed_tools = g_hash_table_new (g_direct_hash, g_direct_equal);
		g_object_set_data (G_OBJECT (shell), "InstalledTools",
						   installed_tools);
	}
	tool = g_hash_table_lookup (tools_by_name, plugin_id);
	if (tool) {
		GObject *obj;
		obj = g_hash_table_lookup (installed_tools, tool);
		if (obj) {
			return obj;
		} else {
			tool_set_update (shell, tool, TRUE);
			obj = g_hash_table_lookup (installed_tools, tool);
			return obj;
		}
	}
	g_warning ("No plugin found with id \"%s\".", plugin_id);
	return NULL;
}

gboolean
anjuta_plugins_unload_plugin_by_id (AnjutaShell *shell,
									const gchar *plugin_id)
{
	AvailableTool *tool;
	GHashTable *installed_tools;
	
	g_return_val_if_fail (ANJUTA_IS_SHELL (shell), FALSE);
	g_return_val_if_fail (plugin_id != NULL, FALSE);
	
	tool = NULL;
	installed_tools = g_object_get_data (G_OBJECT (shell), "InstalledTools");
	if (installed_tools == NULL)
	{
		installed_tools = g_hash_table_new (g_direct_hash, g_direct_equal);
		g_object_set_data (G_OBJECT (shell), "InstalledTools",
						   installed_tools);
	}
	tool = g_hash_table_lookup (tools_by_name, plugin_id);
	if (tool) {
		
		tool_set_update (shell, tool, FALSE);
		
		/* Check if the plugin has been indeed unloaded */
		if (!g_hash_table_lookup (installed_tools, tool))
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
		g_object_set_data (G_OBJECT (data), "__plugin_tool", key);
		return TRUE;
	}
	return FALSE;
}

gboolean
anjuta_plugins_unload_plugin (AnjutaShell *shell, GObject *plugin)
{
	AvailableTool *tool;
	GHashTable *installed_tools;
	
	g_return_val_if_fail (ANJUTA_IS_SHELL (shell), FALSE);
	g_return_val_if_fail (ANJUTA_IS_PLUGIN (plugin), FALSE);
	
	tool = NULL;
	installed_tools = g_object_get_data (G_OBJECT (shell), "InstalledTools");
	if (installed_tools == NULL)
	{
		installed_tools = g_hash_table_new (g_direct_hash, g_direct_equal);
		g_object_set_data (G_OBJECT (shell), "InstalledTools",
						   installed_tools);
	}
	
	/* Find the tool that correspond to this plugin object */
	g_hash_table_find (installed_tools, find_plugin_for_object, plugin);
	tool = g_object_get_data (G_OBJECT (plugin), "__plugin_tool");
	
	if (tool) {
		
		tool_set_update (shell, tool, FALSE);
		
		/* Check if the plugin has been indeed unloaded */
		if (!g_hash_table_lookup (installed_tools, tool))
			return TRUE;
		else
			return FALSE;
	}
	g_warning ("No plugin found with object \"%p\".", plugin);
	return FALSE;
}

GtkWidget *
anjuta_plugins_get_installed_dialog (AnjutaShell *shell)
{
	return create_plugin_page (shell);
}

GSList*
anjuta_plugins_query (AnjutaShell *shell,
					  const gchar *section_name,
					  const gchar *attribute_name,
					  const gchar *attribute_value,
					  ...)
{
	va_list var_args;
	GList *secs = NULL;
	GList *anames = NULL;
	GList *avalues = NULL;
	const gchar *sec = section_name;
	const gchar *aname = attribute_name;
	const gchar *avalue = attribute_value;
	GSList *selected_plugins = NULL;
	GSList *available = available_tools;
	
	if (section_name == NULL)
	{
		/* If no query is given, select all plugins */
		while (available)
		{
			AvailableTool *tool = available->data;
			AnjutaPluginDescription *desc = tool->description;
			selected_plugins = g_slist_prepend (selected_plugins, desc);
			available = g_slist_next (available);
		}
		return g_slist_reverse (selected_plugins);
	}
	
	g_return_val_if_fail (section_name != NULL, NULL);
	g_return_val_if_fail (attribute_name != NULL, NULL);
	g_return_val_if_fail (attribute_value != NULL, NULL);
	
	secs = g_list_prepend (secs, g_strdup (section_name));
	anames = g_list_prepend (anames, g_strdup (attribute_name));
	avalues = g_list_prepend (avalues, g_strdup (attribute_value));
	
	va_start (var_args, attribute_value);
	while (sec) {
		sec = va_arg (var_args, const gchar *);
		if (sec) {
			aname = va_arg (var_args, const gchar *);
			if (aname) {
				avalue = va_arg (var_args, const gchar *);
				if (avalue) {
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
		
		AvailableTool *tool = available->data;
		AnjutaPluginDescription *desc = tool->description;
		
		while (s_node) {
			
			gchar *val;
			GSList *vals;
			GSList *node;
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
			
			vals = property_to_slist (val);
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
				node = g_slist_next (node);
			}
			g_slist_free (vals);
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
			selected_plugins = g_slist_prepend (selected_plugins, desc);
			g_message ("Satisfied, Adding %s", tool->name);
		}
		available = g_slist_next (available);
	}
	anjuta_util_glist_strings_free (secs);
	anjuta_util_glist_strings_free (anames);
	anjuta_util_glist_strings_free (avalues);
	
	return g_slist_reverse (selected_plugins);
}

enum {
	PIXBUF_COLUMN,
	PLUGIN_COLUMN,
	PLUGIN_DESCRIPTION_COLUMN,
	N_COLUMNS
};

AnjutaPluginDescription *
anjuta_plugins_select (AnjutaShell *shell, gchar *title, gchar *description,
					   GSList *plugin_descriptions)
{
	g_return_val_if_fail (title != NULL, NULL);
	g_return_val_if_fail (description != NULL, NULL);
	g_return_val_if_fail (plugin_descriptions != NULL, NULL);

	if (g_slist_length (plugin_descriptions) > 0)
	{
		GtkWidget *dlg;
		GtkTreeModel *model;
		GtkWidget *view;
		GtkTreeViewColumn *column;
		GtkCellRenderer *renderer;
		GSList *node;
		GtkWidget *label;
		GtkWidget *sc;
		gint response;
		
		dlg = gtk_dialog_new_with_buttons (title, GTK_WINDOW (shell),
										   GTK_DIALOG_DESTROY_WITH_PARENT,
										   GTK_STOCK_CANCEL,
										   GTK_RESPONSE_CANCEL,
										   GTK_STOCK_OK, GTK_RESPONSE_OK,
										   NULL);
		gtk_widget_set_size_request (dlg, 400, 300);
		gtk_window_set_default_size (GTK_WINDOW (dlg), 400, 300);

		label = gtk_label_new (description);
		gtk_widget_show (label);
		gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dlg)->vbox), label,
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

		node = plugin_descriptions;
		while (node)
		{
			GdkPixbuf *icon_pixbuf = NULL;
			gchar *plugin_name = NULL;
			gchar *plugin_desc = NULL;
			gchar *icon_filename = NULL;
			
			AnjutaPluginDescription *desc =
				(AnjutaPluginDescription*)node->data;
			
			if (anjuta_plugin_description_get_string (desc,
													  "Anjuta Plugin",
													  "Icon",
													  &icon_filename))
			{
				gchar *icon_path = NULL;
				icon_path = g_strconcat (PACKAGE_PIXMAPS_DIR"/",
										 icon_filename, NULL);
				g_message ("Icon: %s", icon_path);
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
			}
			node = g_slist_next (node);
		}
		response = gtk_dialog_run (GTK_DIALOG (dlg));
		switch (response)
		{
		case GTK_RESPONSE_OK:
			{
				GtkTreeIter selected;
				GtkTreeSelection *selection;
				GtkTreeModel *store;
				
				selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
				if (gtk_tree_selection_get_selected (selection, &store,
													 &selected))
				{
					AnjutaPluginDescription *d;
					gtk_tree_model_get (model, &selected,
										PLUGIN_DESCRIPTION_COLUMN, &d, -1);
					if (d)
					{
						gtk_widget_destroy (dlg);
						return d;
					}
				}
			}
		}
		gtk_widget_destroy (dlg);
	}
	return NULL;
}

GObject*
anjuta_plugins_select_and_activate (AnjutaShell *shell, gchar *title,
									gchar *description,
									GSList *plugin_descriptions)
{
	AnjutaPluginDescription *d;
	
	d = anjuta_plugins_select (shell, title, description, plugin_descriptions);
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
			anjuta_plugins_get_plugin_by_id (shell, location);
		g_free (location);
		return plugin;
	}
	return NULL;
}
