#include <config.h>

#include <sys/types.h>
#include <dirent.h>
// #include <gdl/gdl.h>
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
#include <libanjuta/anjuta-ui.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-plugin-parser.h>
#include <e-splash.h>
#include "resources.h"
#include <string.h>

typedef struct {
	char *id;
	char *name;
	char *about;
	char *icon_path;

	/* The dependencies listed in the oaf file */
	GSList *dependency_names;

	/* Interfaces exported by this plugin */
	GSList *interfaces;
	
	/* Attributes defined for this plugin */
	GHashTable *attributes;
	
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

typedef struct {
	gchar *name;
	GHashTable *tools;
} ToolSet;

enum {
	COL_ENABLED,
	COL_NAME,
	COL_TOOL
};

#define TOOL_SET_LOCATION "/apps/anjuta/toolsets"

static GConfClient *gconf_client = NULL;

static GList *plugin_dirs = NULL;
static GSList *available_tools = NULL;
static GHashTable *tools_by_interfaces = NULL;
static GHashTable *tools_by_name = NULL;
static GHashTable *tool_sets = NULL;
static GlueFactory *glue_factory = NULL;

/** Dependency Resolution **/

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
tool_from_file (AnjutaPluginFile *file) 
{
	char *str;
	AvailableTool *tool = g_new0 (AvailableTool, 1);
	gboolean success = TRUE;
	
	if (anjuta_plugin_file_get_string (file, 
					   "Anjuta Plugin",
					   "Location",
					   &str)) {
		tool->id = str;
	} else {
		g_warning ("Couldn't find 'Location'");
		success = FALSE;
	}
	    
	if (anjuta_plugin_file_get_string (file, 
					   "Anjuta Plugin",
					   "Name",
					   &str)) {
		tool->name = str;
	} else {
		g_warning ("couldn't find 'Name' attribute.");
		success = FALSE;
	}


	if (anjuta_plugin_file_get_string (file, 
					   "Anjuta Plugin",
					   "Description",
					   &str)) {
		tool->about = str;
	} else {
		g_warning ("Couldn't find 'Description' attribute.");
		success = FALSE;
	}

	if (anjuta_plugin_file_get_string (file, 
					   "Anjuta Plugin",
					   "Icon",
					   &str)) {
		tool->icon_path = get_icon_path (str);
		g_free (str);
	}

	if (anjuta_plugin_file_get_string (file, 
					   "Anjuta Plugin",
					   "Dependencies",
					   &str)) {
		tool->dependency_names = property_to_slist (str);
		g_free (str);
	}

	if (anjuta_plugin_file_get_string (file, 
					   "Anjuta Plugin",
					   "Interfaces",
					   &str)) {
		tool->interfaces = property_to_slist (str);
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
load_tool (AnjutaPluginFile *file)
{
	AvailableTool *tool;
	
	tool = tool_from_file (file);
	if (tool) {
		if (g_hash_table_lookup (tools_by_name, tool->id)) {
			destroy_tool (tool);
		} else {
			GSList *node;
			available_tools = g_slist_prepend (available_tools, tool);
			g_hash_table_insert (tools_by_name, tool->id, tool);
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
load_tool_file (const char *path)
{
	char *contents;

	if (g_file_get_contents (path, &contents, NULL, NULL)) {
		AnjutaPluginFile *file;
		
		file = anjuta_plugin_file_new_from_string (contents, NULL);
		g_free (contents);
		if (!file) {
			g_warning ("bad plugin file: %s\n", path);
			return;
		}
		
		load_tool (file);
	}
}

static void
load_tools_from_directory (const char *dirname)
{
	DIR *dir;
	struct dirent *entry;
	
	dir = opendir (dirname);
	
	if (!dir) {
		return;
	}
	
	for (entry = readdir (dir); entry != NULL; entry = readdir (dir)) {
		if (str_has_suffix (entry->d_name, ".plugin")) {
			char *pathname;

			pathname = g_strdup_printf ("%s/%s", 
						    dirname, 
						    entry->d_name);
			
			load_tool_file (pathname);

			g_free (pathname);
		}
	}
	
}

static void
load_available_tools (void)
{
	GSList *cycles;
	GList *l;
	tools_by_name = g_hash_table_new (g_str_hash, g_str_equal);
	tools_by_interfaces = g_hash_table_new (g_str_hash, g_str_equal);

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

	g_hash_table_destroy (tools_by_name);
	tools_by_name = NULL;
}

/** Tool Sets **/
static gboolean
get_bool_with_default (GConfClient *client, char *key, gboolean def)
{
	GConfValue *value;
	gboolean ret;
	
	value = gconf_client_get_without_default (client, key, NULL);
	if (value) {
		ret = gconf_value_get_bool (value);
		gconf_value_free (value);
	} else {
		ret = def;
		gconf_client_set_bool (client, key, def, NULL);
	}
	
	return ret;
}

#if 0

static void
get_keys_helper (gpointer key, gpointer value, gpointer user_data)
{
	GSList **keys = user_data;
	*keys = g_slist_prepend (*keys, key);
}

static GSList *
get_keys (GHashTable *hash)
{
	GSList *keys = NULL;
	g_hash_table_foreach (hash, get_keys_helper, &keys);
	return keys;
}

static void
check_tool_set_dependencies_helper (gpointer key, 
				    gpointer value, 
				    gpointer user_data)
{
	AvailableTool *tool = key;
	gboolean load = GPOINTER_TO_INT (value);
	ToolSet *tool_set = user_data;
	GSList *deps;
	GSList *l;

	if (!load) {
		return;
	}

	deps = get_keys (tool->dependencies);
	for (l = deps; l != NULL; l = l->next) {
		/* FIXME: Finish this */
	}

	g_slist_free (deps);
}

static void
check_tool_set_dependencies (ToolSet *tool_set)
{
	g_hash_table_foreach (tool_set->tools, 
			      check_tool_set_dependencies_helper, tool_set);
}

#endif

static ToolSet *
load_tool_set (const char *name) 
{
	ToolSet *ret = g_new0 (ToolSet, 1);
	GSList *l;
	gboolean load_new;
	char *key;
	
	ret->name = g_strdup (name);
	ret->tools = g_hash_table_new (g_direct_hash, g_direct_equal);

	key = g_strdup_printf (TOOL_SET_LOCATION "/%s/load_new", name);
	load_new = get_bool_with_default (gconf_client, key, TRUE);
	g_free (key);

	for (l = available_tools; l != NULL; l = l->next) {
		AvailableTool *tool = l->data;
		gboolean load = FALSE;
		key = g_strdup_printf (TOOL_SET_LOCATION "/%s/%s", 
				       name, tool->id);
		load = get_bool_with_default (gconf_client, key, load_new);
		g_free (key);
		g_message ("Loading .. %s:%s", name, tool->id);
		g_hash_table_insert (ret->tools, tool, GINT_TO_POINTER (load));
	}
	
	return ret;
}

static void
save_tool_set (ToolSet *tool_set)
{
	GSList *l;

	for (l = available_tools; l != NULL; l = l->next) {
		AvailableTool *tool = l->data;
		char *key;
		gboolean load = g_hash_table_lookup (tool_set->tools,
						     tool) ? TRUE : FALSE;
		key = g_strdup_printf (TOOL_SET_LOCATION "/%s/%s",
				       tool_set->name, tool->id);
		gconf_client_set_bool (gconf_client, key, load, NULL);
	}
}

static void
load_tool_sets (void)
{
	tool_sets = g_hash_table_new (g_str_hash, g_str_equal);
	
	g_hash_table_insert (tool_sets, "default", load_tool_set ("default"));
}

static GObject *
activate_tool (AnjutaShell *shell, AnjutaUI *ui,
	       AnjutaPreferences *prefs, AvailableTool *tool)
{
	GType type;
	GObject *ret;
	static GHashTable *types = NULL;
	
	if (!types) {
		types = g_hash_table_new (g_str_hash, g_str_equal);
	}
	
	type = GPOINTER_TO_UINT (g_hash_table_lookup (types, tool->id));
	
	if (!type) {
		char **pieces;

		pieces = g_strsplit (tool->id, ":", -1);
		type = glue_factory_get_object_type (glue_factory,
										     pieces[0], pieces[1]);
		g_hash_table_insert (types, g_strdup (tool->id),
							 GUINT_TO_POINTER (type));
		g_strfreev (pieces);
	}
	
	if (type == G_TYPE_INVALID) {
		g_warning ("Invalid type: Can not load %s\n", tool->id);
		ret = NULL;
	} else {
		ret = g_object_new (type, "ui", ui, "prefs", prefs,
				    "shell", shell, NULL);
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
	
	gconf_client = gconf_client_get_default ();

	load_available_tools ();
	load_tool_sets ();
}

static gboolean
free_tool_set (gpointer key,
	       gpointer value,
	       gpointer user_data)
{
	ToolSet *tool_set = (ToolSet *)value;

	g_free (tool_set->name);
	g_hash_table_destroy (tool_set->tools);
	g_free (tool_set);

	return TRUE;
}

void
anjuta_plugins_finalize (void)
{
	unload_available_tools ();
	g_object_unref (gconf_client);
	g_list_foreach (plugin_dirs, (GFunc)g_free, NULL);
	g_list_free (plugin_dirs);
	g_hash_table_foreach_remove (tool_sets, free_tool_set, NULL);
	g_hash_table_destroy (tool_sets);
	g_object_unref (glue_factory);
}

static gboolean 
should_load (AvailableTool *tool, ToolSet *set)
{
	gboolean in_set = 
		GPOINTER_TO_INT (g_hash_table_lookup (set->tools,
						      tool));
	return (in_set && tool->can_load);
}

static void
tool_set_update (AnjutaShell *shell, AnjutaUI *ui,
		 AnjutaPreferences *prefs, ToolSet *tool_set)
{
	GHashTable *installed_tools;
	GSList *l;
	// AnjutaShell *shell = ANJUTA_SHELL (win);
	// installed_tools = g_object_get_data (G_OBJECT (win), "InstalledTools");
	installed_tools = g_object_get_data (G_OBJECT (shell), "InstalledTools");
	
	/* reverse available_tools when unloading, so that plugins are
	 * unloaded in the right order */
	available_tools = g_slist_reverse (available_tools);
	for (l = available_tools; l != NULL; l = l->next) {
		AvailableTool *tool = l->data;
		GObject *tool_obj = g_hash_table_lookup (installed_tools, 
							 tool);
		if (tool_obj && !should_load (tool, tool_set)) {
			/* FIXME: Unload the class if possible */
			gboolean success = TRUE;
			AnjutaPlugin *anjuta_tool = ANJUTA_PLUGIN (tool_obj);
			if (ANJUTA_PLUGIN_GET_CLASS (anjuta_tool)->deactivate) {
				if (!ANJUTA_PLUGIN_GET_CLASS (anjuta_tool)->deactivate (anjuta_tool)) {
					anjuta_util_dialog_info (GTK_WINDOW (shell),
											 "Plugin '%s' do not want to be deactivated",
											 tool->name);
					success = FALSE;
				}
			}
			if (success) {
				g_object_unref (tool_obj);
				g_hash_table_remove (installed_tools, tool);
			}
		}
	}
	available_tools = g_slist_reverse (available_tools);
	
	for (l = available_tools; l != NULL; l = l->next) {
		AvailableTool *tool = l->data;
		if (should_load (tool, tool_set)
		    && !g_hash_table_lookup (installed_tools, tool)) {
			GObject *tool_obj = activate_tool (shell, ui, prefs, tool);
			if (tool_obj) {
				g_hash_table_insert (installed_tools,
						     tool, tool_obj);
			}
		}
	}
}

/* Load a toolset for a given AnjutaWindow */
void
anjuta_plugins_load (AnjutaShell *shell, AnjutaUI *ui,
		     AnjutaPreferences *prefs,
		     ESplash *splash, const char *name)
{
	ToolSet *tool_set = g_hash_table_lookup (tool_sets, name);
	// AnjutaShell *shell = ANJUTA_SHELL (win);
	GSList *l;
	GHashTable *installed_tools;
	int image_index;

	if (!tool_set) {
		g_warning ("Could not find plugin set '%s'", name);
		return;
	}

	installed_tools = g_hash_table_new (g_direct_hash, g_direct_equal);

	if (splash) {
		for (l = available_tools; l != NULL; l = l->next) {
			AvailableTool *tool = l->data;
			if (should_load (tool, tool_set) && tool->icon_path) {
				GdkPixbuf *icon_pixbuf;
				icon_pixbuf = 
					gdk_pixbuf_new_from_file (tool->icon_path, 
								  NULL);
				if (icon_pixbuf) {
					e_splash_add_icon (splash, icon_pixbuf);
					g_object_unref (icon_pixbuf);
				} else {
					g_free (tool->icon_path);
					tool->icon_path = NULL;
				}
				//while (gtk_events_pending ())
				//	gtk_main_iteration ();
			}
		}
	}
	
	while (gtk_events_pending ())
		gtk_main_iteration ();

	image_index = 0;
	for (l = available_tools; l != NULL; l = l->next) {
		AvailableTool *tool = l->data;
		if (should_load (tool, tool_set)) {
			GObject *tool_obj = activate_tool (shell, ui,
							   prefs, tool);
			if (tool_obj) {
				g_hash_table_insert (installed_tools,
						     tool, tool_obj);
			}
			if (splash && tool->icon_path) {
				e_splash_set_icon_highlight (splash, 
							     image_index++, 
							     TRUE);
			}
			while (gtk_events_pending ())
				gtk_main_iteration ();
		}
	}

	// g_object_set_data (G_OBJECT (win), "InstalledTools", installed_tools);
	g_object_set_data (G_OBJECT (shell), "InstalledTools", installed_tools);
}

gboolean
anjuta_plugins_unload (AnjutaShell *shell)
{
	GHashTable *installed_tools;
	GSList *l;

	// installed_tools = g_object_get_data (G_OBJECT (window), "InstalledTools");
	installed_tools = g_object_get_data (G_OBJECT (shell), "InstalledTools");
	
	/* reverse available_tools when unloading, so that plugins are
	 * unloaded in the right order */
	available_tools = g_slist_reverse (available_tools);

	/* Shutdown all plugins. If a plugin doesn't want to exit, abort the
	 * shutdown process. */
	for (l = available_tools; l != NULL; l = l->next) {
		AvailableTool *tool = l->data;
		GObject *tool_obj = g_hash_table_lookup (installed_tools, 
							 tool);
		if (tool_obj) {
			AnjutaPlugin *anjuta_tool = ANJUTA_PLUGIN (tool_obj);
			if (ANJUTA_PLUGIN_GET_CLASS (anjuta_tool)->deactivate)
				if (!ANJUTA_PLUGIN_GET_CLASS (anjuta_tool)->deactivate (anjuta_tool))
					return FALSE;
		}
	}

	/* Remove plugins. */
	for (l = available_tools; l != NULL; l = l->next) {
		AvailableTool *tool = l->data;
		GObject *tool_obj = g_hash_table_lookup (installed_tools, 
							 tool);
		if (tool_obj) {
			g_object_unref (tool_obj);
			/* FIXME: Unload the class if possible */
			g_hash_table_remove (installed_tools, tool);
		}
	}

	g_hash_table_destroy (installed_tools);

	return TRUE;
}

static AvailableTool *
tool_for_iter (GtkListStore *store, GtkTreeIter *iter)
{
	AvailableTool *tool;
	
	gtk_tree_model_get (GTK_TREE_MODEL (store), iter, 
			    COL_TOOL, &tool,
			    -1);
	return tool;
}

static void
update_enabled (GtkTreeModel *model, ToolSet *set)
{
	GtkTreeIter iter;
	
	if (gtk_tree_model_get_iter_first (model, &iter)) {
		do {
			AvailableTool *tool;
			gboolean load;
			gtk_tree_model_get (model, &iter,
					    COL_TOOL, &tool,
					    -1);

			load = g_hash_table_lookup (set->tools, tool) ? TRUE : FALSE;
			gtk_list_store_set (GTK_LIST_STORE (model), &iter,
					    COL_ENABLED, load, 
					    -1);
		} while (gtk_tree_model_iter_next (model, &iter));
	}
}

static void
enable_hashfunc (gpointer key, gpointer value, gpointer data)
{
	AvailableTool *tool = key;
	ToolSet *set = data;
	
	g_hash_table_insert (set->tools, tool, GINT_TO_POINTER (1));
}

static void
disable_hashfunc (gpointer key, gpointer value, gpointer data)
{
	AvailableTool *tool = key;
	ToolSet *set = data;

	g_hash_table_insert (set->tools, tool, NULL);
}

static void
apply_toolset (ToolSet *tool_set, AnjutaShell *shell,
	       AnjutaUI *ui, AnjutaPreferences *prefs)
{
/*
	GList *windows;
	GList *l;
	
	windows = anjuta_get_all_windows ();
	
	for (l = windows; l != NULL; l = l->next) {
		tool_set_update (GTK_WINDOW (l->data), tool_set);
	}

	g_list_free (windows);
*/
	tool_set_update (shell, ui, prefs, tool_set);
}

static void
plugin_toggled (GtkCellRendererToggle *cell, char *path_str, gpointer data)
{
	GtkListStore *store = GTK_LIST_STORE (data);
	GtkTreeIter iter;
	GtkTreePath *path;
	AvailableTool *tool;
	gboolean enabled;
	ToolSet *tool_set;
	AnjutaShell *shell;
	AnjutaUI *ui;
	AnjutaPreferences *prefs;
	
	path = gtk_tree_path_new_from_string (path_str);
	
	tool_set = g_object_get_data (G_OBJECT (store), "ToolSet");
	shell = g_object_get_data (G_OBJECT (store), "Shell");
	ui = g_object_get_data (G_OBJECT (store), "UI");
	prefs = g_object_get_data (G_OBJECT (store), "Preferences");

	gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);
	gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
			    COL_ENABLED, &enabled,
			    COL_TOOL, &tool,
			    -1);
	
	enabled = !enabled;

	if (enabled) {
		g_hash_table_foreach (tool->dependencies,
				      enable_hashfunc, tool_set);
	} else {
		g_hash_table_foreach (tool->dependents, 
				      disable_hashfunc, tool_set);
	}

	g_hash_table_insert (tool_set->tools, tool, GINT_TO_POINTER (enabled));
	update_enabled (GTK_TREE_MODEL (store), tool_set);

	gtk_tree_path_free (path);
	
	save_tool_set (tool_set);

	apply_toolset (tool_set, shell, ui, prefs);
}

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

static GtkWidget *
create_plugin_tree (void)
{
	GtkListStore *store;
	GtkWidget *tree;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	store = gtk_list_store_new (3, 
				    G_TYPE_BOOLEAN, 
				    G_TYPE_STRING, 
				    G_TYPE_POINTER);
	tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));

	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (G_OBJECT (renderer), "toggled",
			  G_CALLBACK (plugin_toggled), store);
	column = gtk_tree_view_column_new_with_attributes (_("Load"),
							   renderer,
							   "active", 
							   COL_ENABLED, NULL);
	
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Plugin"),
							   renderer,
							   "text", 
							   COL_NAME, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
	g_object_unref (store);
	return tree;
}

static void
populate_plugin_model (GtkListStore *store, ToolSet *set)
{
	GSList *l;
	for (l = available_tools; l != NULL; l = l->next) {
		GtkTreeIter iter;
		AvailableTool *tool = l->data;
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    COL_ENABLED, 
				    (gboolean)g_hash_table_lookup (set->tools,
								   tool),
				    COL_NAME, tool->name,
				    COL_TOOL, tool,
				    -1);
	}
}

static GtkWidget *
create_plugin_page (ToolSet *set, AnjutaShell *shell,
					AnjutaUI *ui, AnjutaPreferences *prefs)
{
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *tree;
	GtkWidget *frame;
	GtkWidget *text;
	GtkWidget *scrolled;
	GtkListStore *store;
	GtkTreeSelection *selection;

	vbox = gtk_vbox_new (FALSE, 0);

	hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);

	image = anjuta_res_get_image ("plugins.png");
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 3);

	label = gtk_label_new (_("This dialog displays all the available plugins for Anjuta.  You can select which plugins you want to use by clicking on the plugin in the list."));
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 3);

	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
					     GTK_SHADOW_IN);
	
	gtk_box_pack_start (GTK_BOX (vbox),
			    scrolled, TRUE, TRUE, 3);
	tree = create_plugin_tree ();
	store = GTK_LIST_STORE(gtk_tree_view_get_model (GTK_TREE_VIEW (tree)));

	populate_plugin_model (GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (tree))), set);
	
	gtk_container_add (GTK_CONTAINER (scrolled), tree);
 	
	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
					     GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	
	hbox = gtk_hbox_new (FALSE, 5);
	gtk_box_pack_start (GTK_BOX (vbox),
			    hbox, TRUE, TRUE, 3);

	frame = gtk_frame_new ("About");
	gtk_box_pack_start (GTK_BOX (hbox),
			    frame, TRUE, TRUE, 3);
	gtk_container_add (GTK_CONTAINER (frame), scrolled);

	text = gtk_text_view_new ();
	gtk_container_add (GTK_CONTAINER (scrolled), text);
	g_object_set_data (G_OBJECT (store), "AboutText", text);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
	gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 3);
	image = gtk_image_new ();
	gtk_container_add (GTK_CONTAINER (frame), image);
	g_object_set_data (G_OBJECT (store), "Icon", image);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK (selection_changed), store);

	g_object_set_data (G_OBJECT (store), "ToolSet", set);
	g_object_set_data (G_OBJECT (store), "Shell", shell);
	g_object_set_data (G_OBJECT (store), "UI", ui);
	g_object_set_data (G_OBJECT (store), "Preferences", prefs);

	gtk_widget_show_all (vbox);
	gtk_widget_hide (image);
	
	return vbox;
}

GObject *
anjuta_plugins_get_object (AnjutaShell *shell,
						   const gchar *iface_name)
{
	AvailableTool *tool;
	GSList *valid_tools;
	GHashTable *installed_tools;
	
	g_return_val_if_fail (ANJUTA_IS_SHELL (shell), NULL);
	g_return_val_if_fail (iface_name != NULL, NULL);
	
	tool = NULL;
	installed_tools = g_object_get_data (G_OBJECT (shell), "InstalledTools");
	valid_tools = g_hash_table_lookup (tools_by_interfaces, iface_name);
	if (valid_tools) {
		/* Just return the first tool. */
		tool = valid_tools->data;
	}
	
	if (tool) {
		GObject *obj;
		obj = g_hash_table_lookup (installed_tools, tool);
		if (obj) {
			return obj;
		} else {
			/* FIXME: Activate this tool */
			return NULL;
		}
	}
	g_warning ("No tool found implementing %s Interface.", iface_name);
	return NULL;
}

GtkWidget *
anjuta_plugins_get_installed_dialog (AnjutaShell *shell,
									 AnjutaUI *ui,
									 AnjutaPreferences *prefs)
{
	ToolSet *set = g_hash_table_lookup (tool_sets, "default");

	return create_plugin_page (set, shell, ui, prefs);
}
