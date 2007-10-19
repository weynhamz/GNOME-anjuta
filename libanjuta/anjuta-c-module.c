/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    anjuta-c-module.c
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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/**
 * SECTION:anjuta-c-module
 * @title: AnjutaCModule
 * @short_description: Anjuta C module
 * @see_also: #AnjutaCPluginFactory
 * @include: libanjuta/anjuta-c-module.h
 * 
 * A module is the part of the plugin containing the code. For machine code
 * compiled from C sources by example, it is a shared library and contained in
 * a .so file on Linux. GLib provides a portable way to load dynamically such
 * code with #GModule objects.
 *
 * In GLib you can register a dynamic type which means that you links a GLib
 * type with a module. When the type is used for the first time the module is
 * loaded. When the type is not used anymore, the module is unloaded. Most of
 * the code necessary to do this is already written in GLib #GTypeModule object
 * except the load and unload function.
 *
 * #AnjutaCModule is used only by a #AnjutaCPluginFactory. It derives from
 * #GTypeModule and implements the load and unload function using a #GModule
 * object for a module written in C.
 *
 * Anjuta plugin types are not registered before loading the module.
 * The loading of the module is done explicitly and types are registered just 
 * after in a function included in the plugin. This function is named
 * anjuta_glue_register_components and has a #GTypeModule as argument.
 * The plugin must registers at the least the plugin type dynamically. It can
 * register other types, but this is currently not used.
 *
 * After loading a module one or more plugin objects will be created using the
 * name of the plugin type which has just been registered. The module
 * will stay loaded while at least one plugin object is present. If all
 * plugins objects are destroyed the unload function will be called and the
 * module can be unloaded and removed from memory.
 *
 * It could be useful that some modules stay in memory even if there is no
 * object using it. A typical example is if some plugin code or some code of
 * a library used by a plugin registers a GLib type statically. GLib types can
 * be registered statically at run time but cannot be unregistered later.
 * The code (or rather the data used in the registration) must stay in the 
 * memory even if the type is not used. In order to avoid this, you must
 * register every type dynamically. You could have other situations where a
 * module cannot be unloaded. By default, Anjuta plugin modules are not
 * unloaded unless it is explicitly allowed.
 */

#include "config.h"

#include "anjuta-c-module.h"

#include <gmodule.h>

/* Types
 *---------------------------------------------------------------------------*/

struct _AnjutaCModuleClass
{
	GTypeModuleClass parent;
};

struct _AnjutaCModule
{
	GTypeModule parent;

	GModule *library;
	gchar *full_name;
};

typedef void (*AnjutaRegisterFunc) (GTypeModule *);

G_DEFINE_TYPE (AnjutaCModule, anjuta_c_module, G_TYPE_TYPE_MODULE)

/* Private functions
 *---------------------------------------------------------------------------*/

/* GTypeModule functions
 *---------------------------------------------------------------------------*/	

static gboolean
anjuta_c_module_load (GTypeModule *gmodule)
{
	AnjutaCModule *module = ANJUTA_C_MODULE (gmodule);
	AnjutaRegisterFunc func;

	g_return_val_if_fail (module->full_name != NULL, FALSE);

	/* Load the module and register the plugins */
	module->library = g_module_open (module->full_name, 0);

	if (module->library == NULL)
	{
		g_warning ("could not load plugin: %s", g_module_error ());
		return FALSE;
	}

	if (!g_module_symbol (module->library, "anjuta_glue_register_components", (gpointer *)(gpointer)&func))
    {
		g_warning ("unable to find register function in %s", module->full_name);
		g_module_close (module->library);

		return FALSE;
	}
  
	/* Register all types */
	(* func) (gmodule);

	return TRUE;
}

static void
anjuta_c_module_unload (GTypeModule *gmodule)
{
	AnjutaCModule *module = ANJUTA_C_MODULE (gmodule);

	g_module_close (module->library);
}

/* GObject functions
 *---------------------------------------------------------------------------*/

/* finalize is the last destruction step. It must free all memory allocated
 * with instance_init. It is called only one time just before releasing all
 * memory */

static void
anjuta_c_module_finalize (GObject *object)
{
	AnjutaCModule* module = ANJUTA_C_MODULE (object);

	g_free (module->full_name);
	
	G_OBJECT_CLASS (anjuta_c_module_parent_class)->finalize (object);
}


static void
anjuta_c_module_class_init (AnjutaCModuleClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GTypeModuleClass *gmodule_class = (GTypeModuleClass *)klass;

	gobject_class->finalize = anjuta_c_module_finalize;

	gmodule_class->load = anjuta_c_module_load;
	gmodule_class->unload = anjuta_c_module_unload;
}

static void
anjuta_c_module_init (AnjutaCModule *module)
{
	module->full_name = NULL;
}

/* Creation and Destruction
 *---------------------------------------------------------------------------*/

/**
 * anjuta_c_module_new:
 * @path: The full path of the module
 * @name: The name of the module
 *
 * Create a new #AnjutaCModule object.
 *
 * Return value: a new #AnjutaCModule object.
 */
AnjutaCModule*
anjuta_c_module_new (const gchar *path, const char *name)
{
	AnjutaCModule *module;

	module = g_object_new (ANJUTA_TYPE_C_MODULE, NULL);
	module->full_name = g_module_build_path (path, name);
	g_type_module_set_name (G_TYPE_MODULE (module), module->full_name);
		
	return module;
}
