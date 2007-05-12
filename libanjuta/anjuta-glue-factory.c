/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/**
 * SECTION:anjuta-glue-factory
 * @short_description: Underlying plugin factory
 * @see_also: 
 * @stability: Unstable
 * @include: libanjuta/anjuta-glue-factory.h
 * 
 */

#include <string.h>
#include "anjuta-glue-factory.h"
#include "anjuta-glue-plugin.h"
#include "anjuta-plugin.h"

static void anjuta_glue_factory_init       (AnjutaGlueFactory *factory);
static void anjuta_glue_factory_class_init (AnjutaGlueFactoryClass *class);

struct _AnjutaGlueFactory
{
  GObject parent;
  
  GList *paths;
};

struct _AnjutaGlueFactoryClass
{
  GObjectClass parent_class;
};

GType
anjuta_glue_factory_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info =
      {
        sizeof (AnjutaGlueFactoryClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) anjuta_glue_factory_class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        
        sizeof (AnjutaGlueFactory),
        0, /* n_preallocs */
        (GInstanceInitFunc) anjuta_glue_factory_init,
      };

      type = g_type_register_static (G_TYPE_OBJECT,
				     "AnjutaGlueFactory",
				     &type_info, 0);
    }
  return type;
}

static void
anjuta_glue_factory_class_init (AnjutaGlueFactoryClass *class)
{
}

static void
anjuta_glue_factory_init (AnjutaGlueFactory *factory)
{
}

AnjutaGlueFactory *
anjuta_glue_factory_new (void)
{
  AnjutaGlueFactory *factory;

  factory = g_object_new (anjuta_glue_factory_get_type (), NULL);

  return factory;
}

gboolean
anjuta_glue_factory_add_path (AnjutaGlueFactory *factory, const gchar *path)
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
  entry->loaded_plugins = g_hash_table_new_full (NULL, NULL, g_free, g_object_unref);
  
  factory->paths = g_list_prepend (factory->paths, entry);
    
  return TRUE;
}

GList* anjuta_glue_factory_get_path(AnjutaGlueFactory *factory)
{
	return factory->paths;
}

static AnjutaGluePlugin *
get_already_loaded_module (AnjutaGlueFactory *factory,
			   const gchar *component_name,
			   const gchar *type_name)
{
	GList *p;

	for (p = factory->paths; p != NULL; p = p->next)
	{
    	PathEntry *entry = p->data;
      	AnjutaGluePlugin *plugin;

		plugin = g_hash_table_lookup (entry->loaded_plugins, component_name);

		if (plugin && anjuta_glue_plugin_get_component_type (plugin, ANJUTA_TYPE_PLUGIN, type_name) != G_TYPE_INVALID)
			return plugin;
	}

	return NULL;
}

static AnjutaGluePlugin *
load_plugin (AnjutaGlueFactory *factory, const gchar *component_name, const gchar *type_name)
{
	GList *p;
	AnjutaGluePlugin *plugin;
  
	plugin = anjuta_glue_plugin_new ();
	
	for (p = factory->paths; p != NULL; p = p->next)
    {
    	PathEntry *entry = p->data;

		if (anjuta_glue_plugin_set_module_path (plugin, entry->path, component_name))
		{
			g_hash_table_insert (entry->loaded_plugins,
								 (gpointer)strdup (component_name),
								 plugin);
			
			return plugin;
		}
	}
	
	/* Plugin file not found, free object */
	g_object_unref (G_OBJECT (plugin));
					
 	return NULL;
}

GType
anjuta_glue_factory_get_object_type (AnjutaGlueFactory  *factory,
			      const gchar  *component_name,
			      const gchar  *type_name,
			      const gchar  *language)
{
	AnjutaGluePlugin *plugin;

	plugin = get_already_loaded_module (factory, component_name, type_name);
  
	if (!plugin)
    	plugin = load_plugin (factory, component_name, type_name);

	if (plugin)
		return anjuta_glue_plugin_get_component_type (plugin, ANJUTA_TYPE_PLUGIN, type_name);
	else
    	return G_TYPE_INVALID;
}
