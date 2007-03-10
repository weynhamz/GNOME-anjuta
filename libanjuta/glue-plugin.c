
/**
 * SECTION:glue-plugin
 * @short_description: Glue plugin
 * @see_also: 
 * @stability: Unstable
 * @include: libanjuta/glue-plugin.h
 * 
 */

#include "glue-plugin.h"

static void glue_plugin_init       (GluePlugin *plugin);
static void glue_plugin_class_init (GluePluginClass *class);

typedef void (*GluePluginRegisterComponentsFunc) (GluePlugin *plugin);

GType
glue_plugin_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info =
      {
        sizeof (GluePluginClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) glue_plugin_class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        
        sizeof (GluePlugin),
        0, /* n_preallocs */
        (GInstanceInitFunc) glue_plugin_init,
      };

      type = g_type_register_static (G_TYPE_TYPE_MODULE,
				     "GluePlugin",
				     &type_info, 0);
    }
  return type;
}

static gboolean
glue_plugin_load (GTypeModule *module)
{
  GluePlugin *plugin = GLUE_PLUGIN (module);
  GluePluginRegisterComponentsFunc func;

  /* Check if we're already loaded */
  if (plugin->module)
    return TRUE;

  /* Load the module and register the plugins */
  plugin->module = g_module_open (plugin->path, 0);

  if (!plugin->module)
    {
      g_warning ("could not load plugin: %s", g_module_error ());
      return FALSE;
    }
  
  if (!g_module_symbol (plugin->module, "glue_register_components", (gpointer *)&func))
    {
      g_warning ("could not load plugin: %s", g_module_error ());
      g_module_close (plugin->module);
      plugin->module = NULL;

      return FALSE;
    }
  
  (* func) (plugin);
  return TRUE;
}

static void
glue_plugin_unload (GTypeModule *module)
{
  GluePlugin *plugin = GLUE_PLUGIN (module);

  if (!plugin->path)
    plugin->path = g_strdup (g_module_name (plugin->module));
  g_module_close (plugin->module);
  plugin->module = NULL;
}

static void
glue_plugin_class_init (GluePluginClass *class)
{
  GTypeModuleClass *type_module_class;

  type_module_class = (GTypeModuleClass *)class;
  
  type_module_class->load = glue_plugin_load;
  type_module_class->unload = glue_plugin_unload;
}

static void
glue_plugin_init (GluePlugin *plugin)
{
}

GluePlugin *
glue_plugin_new (GModule *module)
{
  GluePlugin *plugin;

  plugin = g_object_new (GLUE_TYPE_PLUGIN, NULL);

  plugin->module = module;
  
  return plugin;
}
