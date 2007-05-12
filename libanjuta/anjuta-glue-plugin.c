/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */

/**
 * SECTION:anjuta-glue-plugin
 * @short_description: Anjuta glue plugin
 * @see_also: 
 * @stability: Unstable
 * @include: libanjuta/anjuta-glue-plugin.h
 * 
 */

#include "anjuta-glue-plugin.h"

typedef void (*AnjutaGluePluginRegisterComponentsFunc) (AnjutaGluePlugin *plugin);

/* Public functions
 *---------------------------------------------------------------------------*/

gboolean
anjuta_glue_plugin_set_module_path (AnjutaGluePlugin *plugin, const gchar* path, const gchar* name)
{
	return ANJUTA_GLUE_PLUGIN_GET_CLASS (plugin)->set_module_path (plugin, path, name);
}

GType
anjuta_glue_plugin_get_component_type (AnjutaGluePlugin *plugin, GType parent, const gchar *type_name)
{
	return ANJUTA_GLUE_PLUGIN_GET_CLASS (plugin)->get_component_type (plugin, parent, type_name);
}

/* AnjutaGluePlugin functions
 *---------------------------------------------------------------------------*/

gboolean
anjuta_glue_c_plugin_set_module_path (AnjutaGluePlugin* plugin, const gchar *path, const gchar *name)
{
	const gchar *file_name;
	gchar *plugin_name;
	gboolean found = FALSE;
	GDir *dir;

	/* No module should be loaded */
	g_return_val_if_fail (plugin->module == NULL, FALSE);

	/* Remove previous path if necessary */
	if (plugin->path != NULL)
	{
		g_free (plugin->path);
		plugin->path = NULL;
	}
	
	/* Open directory */
	dir = g_dir_open (path, 0, NULL);
	if (dir == NULL) return FALSE;

	/* Search for corresponding file */
	plugin_name = g_module_build_path (NULL, name);
	for(file_name = g_dir_read_name (dir); file_name != NULL; file_name = g_dir_read_name (dir))
	{
		if (file_name && strcmp (file_name, plugin_name) == 0)
		{
        	/* We have found a matching module */
			plugin->path = g_module_build_path (path, plugin_name);
			g_type_module_set_name (G_TYPE_MODULE (plugin), name);
			found = TRUE;
			break;
		}
	}
	g_free (plugin_name);
	g_dir_close (dir);
	
	return found;
}

GType
anjuta_glue_c_plugin_get_component_type (AnjutaGluePlugin *plugin, GType parent, const gchar* type_name)
{
	if (plugin->module != NULL)
	{
		/* Module already loaded, plugin type should be already registered */
		return g_type_from_name (type_name);
	}
	else
	{
    	/* Module not loaded, return a proxy type. The complete type will
		 * be registered when the module is loaded at the first use */
		static const GTypeInfo type_info = {0, NULL, NULL, NULL, NULL, NULL, 0, 0, NULL};

		return g_type_module_register_type (G_TYPE_MODULE (plugin), parent, type_name, &type_info, 0);
	}
}

/* GTypeModule functions
 *---------------------------------------------------------------------------*/	

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
  
  /* TODO: Add a line in plugin file to know which module can be unloaded ? */	
  /* Never unload any module */	
  g_type_module_use (module);	
	
  return TRUE;
}

static void
anjuta_glue_plugin_unload (GTypeModule *module)
{
  AnjutaGluePlugin *plugin = ANJUTA_GLUE_PLUGIN (module);

  g_module_close (plugin->module);
  plugin->module = NULL;
}

/* GObject functions
 *---------------------------------------------------------------------------*/

/* Used in dispose and finalize */
static GObjectClass *parent_class = NULL;

/* finalize is the last destruction step. It must free all memory allocated
 * with instance_init. It is called only one time just before releasing all
 * memory */

static void
anjuta_glue_c_plugin_finalize (GObject *object)
{
        AnjutaGluePlugin* plugin = ANJUTA_GLUE_PLUGIN (object);

        g_free (plugin->path);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
anjuta_glue_plugin_class_init (AnjutaGluePluginClass *klass)
{
  GTypeModuleClass *type_module_class;
  GObjectClass *gobject_class;

  parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
	
  type_module_class = (GTypeModuleClass *)klass;
  gobject_class = G_OBJECT_CLASS (klass);
	
  gobject_class->finalize = anjuta_glue_c_plugin_finalize;

  type_module_class->load = anjuta_glue_plugin_load;
  type_module_class->unload = anjuta_glue_plugin_unload;
	
  klass->set_module_path = anjuta_glue_c_plugin_set_module_path;
  klass->get_component_type = anjuta_glue_c_plugin_get_component_type;	
}

static void
anjuta_glue_plugin_init (AnjutaGluePlugin *plugin)
{
	plugin->path = NULL;
    plugin->module = NULL;
}

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

/* Creation and Destruction
 *---------------------------------------------------------------------------*/

AnjutaGluePlugin *
anjuta_glue_plugin_new (void)
{
  AnjutaGluePlugin *plugin;

  plugin = g_object_new (ANJUTA_GLUE_TYPE_PLUGIN, NULL);
  
  return plugin;
}
