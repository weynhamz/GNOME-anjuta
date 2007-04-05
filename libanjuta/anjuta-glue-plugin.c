
/**
 * SECTION:anjuta-glue-plugin
 * @short_description: Anjuta glue plugin
 * @see_also: 
 * @stability: Unstable
 * @include: libanjuta/anjuta-glue-plugin.h
 * 
 */

#include "anjuta-glue-plugin.h"

static void anjuta_glue_plugin_init       (AnjutaGluePlugin *plugin);
static void anjuta_glue_plugin_class_init (AnjutaGluePluginClass *class);

typedef void (*AnjutaGluePluginRegisterComponentsFunc) (AnjutaGluePlugin *plugin);

GType
anjuta_glue_plugin_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info =
      {
        sizeof (AnjutaGluePluginClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) anjuta_glue_plugin_class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        
        sizeof (AnjutaGluePlugin),
        0, /* n_preallocs */
        (GInstanceInitFunc) anjuta_glue_plugin_init,
      };

      type = g_type_register_static (G_TYPE_TYPE_MODULE,
				     "AnjutaGluePlugin",
				     &type_info, 0);
    }
  return type;
}

static gboolean
anjuta_glue_plugin_load (GTypeModule *module)
{
  AnjutaGluePlugin *plugin = ANJUTA_GLUE_PLUGIN (module);
  AnjutaGluePluginRegisterComponentsFunc func;

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
  
  if (!g_module_symbol (plugin->module, "anjuta_glue_register_components", (gpointer *)&func))
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
anjuta_glue_plugin_unload (GTypeModule *module)
{
  AnjutaGluePlugin *plugin = ANJUTA_GLUE_PLUGIN (module);

  if (!plugin->path)
    plugin->path = g_strdup (g_module_name (plugin->module));
  g_module_close (plugin->module);
  plugin->module = NULL;
}

static void
anjuta_glue_plugin_class_init (AnjutaGluePluginClass *class)
{
  GTypeModuleClass *type_module_class;

  type_module_class = (GTypeModuleClass *)class;
  
  type_module_class->load = anjuta_glue_plugin_load;
  type_module_class->unload = anjuta_glue_plugin_unload;
}

static void
anjuta_glue_plugin_init (AnjutaGluePlugin *plugin)
{
}

AnjutaGluePlugin *
anjuta_glue_plugin_new (GModule *module)
{
  AnjutaGluePlugin *plugin;

  plugin = g_object_new (ANJUTA_GLUE_TYPE_PLUGIN, NULL);

  plugin->module = module;
  
  return plugin;
}
