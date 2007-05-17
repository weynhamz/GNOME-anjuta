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

/* Public functions
 *---------------------------------------------------------------------------*/

const gchar*
anjuta_glue_plugin_build_component_path (AnjutaGluePlugin *plugin, const gchar* path, const gchar* component_name)
{
	g_free (plugin->path);
	plugin->path = ANJUTA_GLUE_PLUGIN_GET_CLASS (plugin)->build_component_path (plugin, path, component_name);
	g_type_module_set_name (G_TYPE_MODULE (plugin), component_name);
	
	return plugin->path;
}

GType
anjuta_glue_plugin_get_component_type (AnjutaGluePlugin *plugin, GType parent, const gchar *type_name)
{
	GType component;
	
	if (plugin->loaded)
	{
		component = ANJUTA_GLUE_PLUGIN_GET_CLASS (plugin)->get_component_type (plugin, parent, type_name);
	}
	else
	{
		static const GTypeInfo type_info = {0, NULL, NULL, NULL, NULL, NULL, 0, 0, NULL};
		AnjutaGluePluginRegister* new_type;

		/* Module not loaded, return a proxy type */
		component = g_type_module_register_type (G_TYPE_MODULE (plugin), parent, type_name, &type_info, 0);
		
		/* Remember to get the real type when the module will be loaded */
		new_type = g_new (AnjutaGluePluginRegister, 1);
		new_type->name = g_strdup (type_name);
		new_type->parent = parent;
		new_type->next = plugin->type;
		plugin->type = new_type;
	}
	
	return component;
}

gboolean
anjuta_glue_plugin_set_resident (AnjutaGluePlugin *plugin, gboolean resident)
{
	gboolean old = plugin->resident;

	if (old != resident)
	{
		plugin->resident = resident;
		if (plugin->loaded)
		{
			if (resident)
			{
				g_type_module_use (G_TYPE_MODULE (plugin));
			}
			else
			{
				g_type_module_unuse (G_TYPE_MODULE (plugin));
			}
		}
	}
	
	return old;
}

/* GTypeModule functions
 *---------------------------------------------------------------------------*/	

static gboolean
anjuta_glue_plugin_load (GTypeModule *module)
{
	AnjutaGluePlugin *plugin = ANJUTA_GLUE_PLUGIN (module);
	AnjutaGluePluginRegister *type;
	
	/* This function is called after loading the module */
	for (type = plugin->type; type != NULL;)
	{
		AnjutaGluePluginRegister *old_type;
		
		/* Replace proxy type with real one */
		ANJUTA_GLUE_PLUGIN_GET_CLASS (plugin)->get_component_type (plugin, type->parent, type->name);
		
		old_type = type->next;
		g_free (type->name);
		g_free (type);
		type = old_type;
	}
	plugin->type = NULL;	
	plugin->loaded = TRUE;

	if (plugin->resident)
	{
		/* Never unload this module */	
		g_type_module_use (module);	
	}
	
	return TRUE;
}

static void
anjuta_glue_plugin_unload (GTypeModule *module)
{
	AnjutaGluePlugin *plugin = ANJUTA_GLUE_PLUGIN (module);

	plugin->loaded = FALSE;
}

/* GObject functions
 *---------------------------------------------------------------------------*/

/* Used in dispose and finalize */
static GObjectClass *parent_class = NULL;

/* finalize is the last destruction step. It must free all memory allocated
 * with instance_init. It is called only one time just before releasing all
 * memory */

static void
anjuta_glue_plugin_finalize (GObject *object)
{
	AnjutaGluePlugin* plugin = ANJUTA_GLUE_PLUGIN (object);
	AnjutaGluePluginRegister *type;

	for (type = plugin->type; type != NULL;)
	{
		AnjutaGluePluginRegister *old_type;
			
		old_type = type->next;
		g_free (type->name);
		g_free (type);
		type = old_type;
	}
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
	
	gobject_class->finalize = anjuta_glue_plugin_finalize;

 	type_module_class->load = anjuta_glue_plugin_load;
 	type_module_class->unload = anjuta_glue_plugin_unload;
	
	/* Need to be implemented by derived class */
  	klass->build_component_path = NULL;
  	klass->get_component_type = NULL;	
}

static void
anjuta_glue_plugin_init (AnjutaGluePlugin *plugin)
{
	plugin->path = NULL;
	plugin->loaded = FALSE;
	plugin->resident = TRUE;
	plugin->type = NULL;
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
