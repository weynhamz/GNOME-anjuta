#include <string.h>
#include <gmodule.h>
#include "glue-factory.h"
#include "glue-plugin.h"

static void glue_factory_init       (GlueFactory *factory);
static void glue_factory_class_init (GlueFactoryClass *class);

typedef GType (*GluePluginGetTypeFunc) (GluePlugin *plugin, const char *name);

typedef struct
{
  GluePlugin *plugin;
  GluePluginGetTypeFunc get_type_func;
  const gchar *name;
} LoadedPlugin;

typedef struct
{
  const gchar *path;
  GHashTable *loaded_plugins;
} PathEntry;

struct _GlueFactory
{
  GObject parent;
  
  GList *paths;
};

struct _GlueFactoryClass
{
  GObjectClass parent_class;
};

GType
glue_factory_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info =
      {
        sizeof (GlueFactoryClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) glue_factory_class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        
        sizeof (GlueFactory),
        0, /* n_preallocs */
        (GInstanceInitFunc) glue_factory_init,
      };

      type = g_type_register_static (G_TYPE_OBJECT,
				     "GlueFactory",
				     &type_info, 0);
    }
  return type;
}

static void
glue_factory_class_init (GlueFactoryClass *class)
{
}

static void
glue_factory_init (GlueFactory *factory)
{
}

GlueFactory *
glue_factory_new (void)
{
  GlueFactory *factory;

  factory = g_object_new (glue_factory_get_type (), NULL);

  return factory;
}

gboolean
glue_factory_add_path (GlueFactory *factory, const gchar *path)
{
  GList *p;
  PathEntry *entry;
  
  if (!g_file_test (path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR))
    return FALSE;

  /* Check if the path has been added */
  p = factory->paths;

  while (p)
    {
      PathEntry *entry = p->data;

      /* If it's already added we return TRUE */
      if (strcmp (path, entry->path) == 0)
	return TRUE;
      
      p = p->next;
    }

  entry = g_new (PathEntry, 1);
  entry->path = g_strdup (path);
  entry->loaded_plugins = g_hash_table_new (NULL, NULL);
  
  factory->paths = g_list_prepend (factory->paths, entry);
    
  return TRUE;
}

static LoadedPlugin *
get_already_loaded_module (GlueFactory *factory,
			   const gchar *component_name,
			   const gchar *type_name)
{
  GList *p;

  p = factory->paths;
  while (p)
    {
      PathEntry *entry = p->data;
      LoadedPlugin *plugin;

      plugin = g_hash_table_lookup (entry->loaded_plugins, component_name);

      if (plugin && (* plugin->get_type_func) (plugin->plugin, type_name) != G_TYPE_INVALID)
	return plugin;
      
      p = p->next;
    }

  return NULL;
}

static LoadedPlugin *
load_plugin (GlueFactory *factory, const gchar *component_name, const gchar *type_name)
{
  GList *p;
  gchar *plugin_name;
  
  p = factory->paths;
  plugin_name = g_module_build_path (NULL, component_name);
  
  while (p)
    {
      const gchar *file_name;
      PathEntry *entry = p->data;
      GDir *dir;
      
      dir = g_dir_open (entry->path, 0, NULL);

      if (dir == NULL)
	continue;
      
      do {
	file_name = g_dir_read_name (dir);
	
	if (file_name && strcmp (file_name, plugin_name) == 0) {
	  GModule *module;
	  GluePlugin *glue_plugin;
	  gchar *plugin_path;
	  GluePluginGetTypeFunc get_type_func;
	  LoadedPlugin *plugin;
	  
	  /* We have found a matching module */
	  plugin_path = g_module_build_path (entry->path, plugin_name);
	  module = g_module_open (plugin_path, 0);
	  if (module == NULL)
	    {
	      g_warning ("Could not open module: %s\n", g_module_error ());
	      goto move_to_next_dir;
	    }

	  if (!g_module_symbol (module, "glue_get_component_type", (gpointer *)&get_type_func))
	    {
	      g_module_close (module);
	      goto move_to_next_dir;
	    }

	  /* Now create a new glue plugin */
	  glue_plugin = glue_plugin_new (module);
	  if ((* get_type_func) (glue_plugin, type_name) == G_TYPE_INVALID)
	    {
	      g_object_unref (glue_plugin);
	      g_module_close (module);
	      goto move_to_next_dir;
	    }
	  
	  /* Everything seems to be in order */
	  plugin = g_new (LoadedPlugin, 1);
	  plugin->plugin = glue_plugin;
	  plugin->get_type_func = get_type_func;
	  plugin->name = g_strdup (component_name);
	  g_type_module_set_name (G_TYPE_MODULE (plugin->plugin), plugin->name);
	  g_hash_table_insert (entry->loaded_plugins, (gpointer)plugin->name, plugin);
	  
	  g_dir_close (dir);
	  g_free (plugin_name);
	  return plugin;

	}
	
      } while (file_name != NULL);
      
move_to_next_dir:	  
      g_dir_close (dir);
      
      p = p->next;
    }

  g_free (plugin_name);
  return NULL;
}

GType
glue_factory_get_object_type (GlueFactory  *factory,
			      const gchar  *component_name,
			      const gchar  *type_name)
{
  LoadedPlugin *plugin;

  plugin = get_already_loaded_module (factory, component_name, type_name);
  
  if (!plugin)
    plugin = load_plugin (factory, component_name, type_name);

  if (plugin) {
    return (* plugin->get_type_func) (plugin->plugin, type_name);
  }
  else
    return G_TYPE_INVALID;
}
