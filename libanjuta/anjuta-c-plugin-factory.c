/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    anjuta-c-plugin-factory.c
    Copyright (C) 2007 Sebastien Granjoux  <seb.sfo@free.fr>

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
 * SECTION:anjuta-c-plugin-factory
 * @short_description: Underlying plugin factory
 * @see_also: 
 * @stability: Unstable
 * @include: libanjuta/anjuta-c-plugin-factory.h
 * 
 */

#include "config.h"

#include "anjuta-c-plugin-factory.h"

#include "anjuta-c-module.h"

#include <libanjuta/interfaces/ianjuta-plugin-factory.h>

#include <glib.h>

#include <string.h>

struct _AnjutaCPluginFactoryClass
{
	GObjectClass parent;
};

struct _AnjutaCPluginFactory
{
	GObject parent;

	GHashTable* loaded_plugins;
};

static void ianjuta_c_plugin_factory_iface_init (IAnjutaPluginFactoryIface *iface);
G_DEFINE_TYPE_WITH_CODE (AnjutaCPluginFactory, anjuta_c_plugin_factory, G_TYPE_OBJECT,	\
G_IMPLEMENT_INTERFACE (IANJUTA_TYPE_PLUGIN_FACTORY, ianjuta_c_plugin_factory_iface_init))

/* Private functions
 *---------------------------------------------------------------------------*/

static AnjutaPlugin*
anjuta_c_plugin_factory_create_plugin (AnjutaCPluginFactory *factory,
					AnjutaPluginHandle *handle,
					AnjutaShell *shell,
					GError **error)
{
	const gchar *path;
	GHashTable *plugin_in_path;
	gchar **pieces;
	AnjutaCModule *module;
	GType type;
	AnjutaPlugin *plugin;
	
	g_return_val_if_fail (handle != NULL, NULL);
	g_return_val_if_fail (shell != NULL, NULL);
	
	path = anjuta_plugin_handle_get_path (handle);
	plugin_in_path = g_hash_table_lookup (factory->loaded_plugins, path);
	if (plugin_in_path == NULL)
	{
		/* No loaded plugin with this path, create sub hash */
		plugin_in_path = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
		g_hash_table_insert (plugin_in_path, g_strdup (path), plugin_in_path);
	}
	
	pieces = g_strsplit (anjuta_plugin_handle_get_id (handle), ":", -1);
	if (pieces == NULL)
	{
		return NULL;
	}
	module = g_hash_table_lookup (plugin_in_path, pieces[0]);
	if (module == NULL)
	{
		/* Plugin is not loaded */
		module = anjuta_c_module_new (path, pieces[0]);
		if (module == NULL) return NULL;
		g_type_module_use (G_TYPE_MODULE (module));
		
		g_hash_table_insert (plugin_in_path, g_strdup (pieces[0]), module);
	}
	else
	{
		module = NULL;
	}
	
	/* Find plugin type */
	if (pieces[1] == NULL)
	{
		return NULL;
	}
	type = g_type_from_name (pieces[1]);
	g_strfreev (pieces);
	
	/* Create plugin */
	plugin = (AnjutaPlugin *)g_object_new (type, "shell", shell, NULL);
	
	if ((module != NULL) && (anjuta_plugin_handle_get_resident(handle) == FALSE))
	{
		/* This module can be unloaded when not needed */
		g_type_module_use (G_TYPE_MODULE (module));
	}
	
	return plugin;
}

/* IAnjutaPluginFactory interface
 *---------------------------------------------------------------------------*/	

static AnjutaPlugin*
ianjuta_c_plugin_factory_new_plugin (IAnjutaPluginFactory *ifactory, AnjutaPluginHandle *handle, AnjutaShell *shell, GError **error)
{
	AnjutaCPluginFactory *factory = ANJUTA_C_PLUGIN_FACTORY (ifactory);

	return anjuta_c_plugin_factory_create_plugin (factory, handle, shell, error);
}

static void
ianjuta_c_plugin_factory_iface_init (IAnjutaPluginFactoryIface *iface)
{
	iface->new_plugin = ianjuta_c_plugin_factory_new_plugin;
}


/* GObject functions
 *---------------------------------------------------------------------------*/

/* dispose is the first destruction step. It is used to unref object created
 * with instance_init in order to break reference counting cycles. This
 * function could be called several times. All function should still work
 * after this call. It has to called its parents.*/

static void
anjuta_c_plugin_factory_dispose (GObject *object)
{
	AnjutaCPluginFactory *factory = ANJUTA_C_PLUGIN_FACTORY (object);
	
	g_hash_table_unref (factory->loaded_plugins);
	factory->loaded_plugins = NULL;

	G_OBJECT_CLASS (anjuta_c_plugin_factory_parent_class)->dispose (object);
}

static void
anjuta_c_plugin_factory_class_init (AnjutaCPluginFactoryClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose = anjuta_c_plugin_factory_dispose;
}

static void
anjuta_c_plugin_factory_init (AnjutaCPluginFactory *factory)
{
	factory->loaded_plugins = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify)g_hash_table_unref);
}

/* Creation and Destruction
 *---------------------------------------------------------------------------*/

AnjutaCPluginFactory*
anjuta_c_plugin_factory_new (void)
{
	AnjutaCPluginFactory *factory;

	factory = g_object_new (ANJUTA_TYPE_C_PLUGIN_FACTORY, NULL);
		
	return factory;
}

void
anjuta_c_plugin_factory_free (AnjutaCPluginFactory *factory)
{
	g_object_unref (G_OBJECT(factory));
}
