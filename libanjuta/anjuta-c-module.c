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
 * @short_description: Anjuta C module
 * @see_also: 
 * @stability: Unstable
 * @include: libanjuta/anjuta-c-module.h
 * 
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

AnjutaCModule*
anjuta_c_module_new (const gchar *path, const char *name)
{
	AnjutaCModule *module;

	module = g_object_new (ANJUTA_TYPE_C_MODULE, NULL);
	module->full_name = g_module_build_path (path, name);
	g_type_module_set_name (G_TYPE_MODULE (module), module->full_name);
		
	return module;
}
