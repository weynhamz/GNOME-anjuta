/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    anjuta-glue-c.c
    Copyright (C) 2007 Sébastien Granjoux

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/**
 * SECTION:anjuta-glue-c-plugin
 * @short_description: Anjuta glue C plugin
 * @see_also: 
 * @stability: Unstable
 * @include: libanjuta/anjuta-glue-c.h
 * 
 */

#include "anjuta-glue-c.h"

typedef void (*AnjutaGluePluginRegisterComponentsFunc) (AnjutaGluePlugin *plugin);

/* AnjutaGluePlugin functions
 *---------------------------------------------------------------------------*/

static gchar *
anjuta_glue_c_plugin_build_component_path (AnjutaGluePlugin* plugin, const gchar *path, const gchar *component_name)
{
	gchar *component_path;
	
	component_path = g_module_build_path (path, component_name);
	
	return component_path;
}
		
static GType
anjuta_glue_c_plugin_get_component_type (AnjutaGluePlugin *plugin, GType parent, const gchar* type_name)
{
	g_return_val_if_fail (ANJUTA_GLUE_C_PLUGIN (plugin)->module != NULL, G_TYPE_INVALID);
	
	/* Module already loaded, plugin type should be already registered */
	return g_type_from_name (type_name);
}

/* GTypeModule functions
 *---------------------------------------------------------------------------*/	

/* Used in load, unload, dispose and finalize */
static AnjutaGluePluginClass *parent_class = NULL;

static gboolean
anjuta_glue_c_plugin_load (GTypeModule *module)
{
	AnjutaGluePlugin *plugin = ANJUTA_GLUE_PLUGIN (module);
	AnjutaGlueCPlugin *c_plugin = ANJUTA_GLUE_C_PLUGIN (module);
	AnjutaGluePluginRegisterComponentsFunc func;

	g_return_val_if_fail (c_plugin->module == NULL, FALSE);
	g_return_val_if_fail (plugin->path != NULL, FALSE);

	/* Load the module and register the plugins */
	c_plugin->module = g_module_open (plugin->path, 0);

	if (!c_plugin->module)
	{
		g_warning ("could not load plugin: %s", g_module_error ());
		return FALSE;
	}

	if (!g_module_symbol (c_plugin->module, "anjuta_glue_register_components", (gpointer *)(gpointer)&func))
    {
		g_warning ("unable to find register function in %s", plugin->path);
		g_module_close (c_plugin->module);
		c_plugin->module = NULL;

		return FALSE;
	}
  
	/* Register all types */
	(* func) (plugin);
  
	/* Call parent function */
	return G_TYPE_MODULE_CLASS (parent_class)->load (module);
}

static void
anjuta_glue_c_plugin_unload (GTypeModule *module)
{
	AnjutaGlueCPlugin *plugin = ANJUTA_GLUE_C_PLUGIN (module);

	g_return_if_fail (plugin->module != NULL);
	
	g_module_close (plugin->module);
	plugin->module = NULL;
	
	/* Call parent function */
	G_TYPE_MODULE_CLASS (parent_class)->unload (module);
}

/* GObject functions
 *---------------------------------------------------------------------------*/

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
anjuta_glue_c_plugin_class_init (AnjutaGluePluginClass *klass)
{
	GTypeModuleClass *type_module_class;
	GObjectClass *gobject_class;

	parent_class = ANJUTA_GLUE_PLUGIN_CLASS (g_type_class_peek_parent (klass));
	
	type_module_class = (GTypeModuleClass *)klass;
	gobject_class = G_OBJECT_CLASS (klass);
	
	gobject_class->finalize = anjuta_glue_c_plugin_finalize;

	type_module_class->load = anjuta_glue_c_plugin_load;
	type_module_class->unload = anjuta_glue_c_plugin_unload;
	
	klass->build_component_path = anjuta_glue_c_plugin_build_component_path;
	klass->get_component_type = anjuta_glue_c_plugin_get_component_type;	
}

static void
anjuta_glue_c_plugin_init (AnjutaGlueCPlugin *plugin)
{
	plugin->module = NULL;
}

GType
anjuta_glue_c_plugin_get_type (void)
{
	static GType type = 0;

	if (!type)
	{
		static const GTypeInfo type_info =
		{
			sizeof (AnjutaGlueCPluginClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) anjuta_glue_c_plugin_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (AnjutaGlueCPlugin),
			0, /* n_preallocs */
			(GInstanceInitFunc) anjuta_glue_c_plugin_init,
		};

		type = g_type_register_static (ANJUTA_GLUE_TYPE_PLUGIN,
					"AnjutaGlueCPlugin",
					&type_info, 0);
	}
	
	return type;
}

/* Creation and Destruction
 *---------------------------------------------------------------------------*/

AnjutaGluePlugin *
anjuta_glue_c_plugin_new (void)
{
	AnjutaGluePlugin *plugin;

	plugin = g_object_new (ANJUTA_GLUE_TYPE_C_PLUGIN, NULL);
  
	return plugin;
}
