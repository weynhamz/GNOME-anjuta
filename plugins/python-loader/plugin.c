/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/**
 *  anjuta-pyloader : a Python plugin loader for Anjuta.
 *
 *  Copyright (C) 2007 Sebastien Granjoux
 *  Copyright (C) 2009 Abderrahim Kitouni
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <Python.h>

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/libanjuta-interfaces.h>
#include "plugin.h"


/* IAnjutaPluginFactory interface
 *---------------------------------------------------------------------------*/

static AnjutaPlugin*
pyl_ianjuta_plugin_factory_new_plugin (IAnjutaPluginFactory *factory,
									   AnjutaPluginHandle *handle,
									   AnjutaShell *shell, GError **error)
{
	const gchar* module_path;
	PyObject *module;
	gchar **pieces;
	GType type;
	AnjutaPlugin *plugin;

	g_return_val_if_fail (handle != NULL, NULL);
	g_return_val_if_fail (shell != NULL, NULL);

	pieces = g_strsplit (anjuta_plugin_handle_get_id (handle), ":", -1);
	if (pieces == NULL)
		return NULL;

	type = g_type_from_name (pieces[1]);
	if (type == G_TYPE_INVALID)	/* The plugin is not loaded */
		{
			module_path = anjuta_plugin_handle_get_path (handle);
			/* If we have a special path, add it to sys.path */
			if (module_path != NULL)
				{
					PyObject *sys_path = PySys_GetObject ("path");
#if PY_MAJOR_VERSION >= 3
					PyObject *path = PyBytes_FromString (module_path);
#else
					PyObject *path = PyString_FromString (module_path);
#endif
					if (PySequence_Contains(sys_path, path) == 0)
						PyList_Insert (sys_path, 0, path);

					Py_DECREF(path);
				}

			module = PyImport_ImportModule (pieces[0]);
			if (!module)
				{
					PyErr_Print ();
					return NULL;
				}

			type = g_type_from_name (pieces[1]);
			if (type == G_TYPE_INVALID)
				return NULL;
		}

	/* Create plugin */
	plugin = (AnjutaPlugin *)g_object_new (type, "shell", shell, NULL);
	return plugin;
}

static void
pyl_ianjuta_plugin_factory_iface_init (IAnjutaPluginFactoryIface *iface)
{
	iface->new_plugin = pyl_ianjuta_plugin_factory_new_plugin;
}

/* AnjutaPlugin functions
 *---------------------------------------------------------------------------*/

static gboolean
pyl_plugin_activate (AnjutaPlugin *plugin)
{
	//PythonLoaderPlugin *self = ANJUTA_PYTHON_LOADER_PLUGIN (plugin);

	char *argv[] = { "anjuta", NULL };
	PyTypeObject *plugin_type = NULL;
	PyObject *m;

	/* Python initialization */
	if (Py_IsInitialized ())
		return TRUE;

	Py_InitializeEx (0);
	if (PyErr_Occurred()) {
		PyErr_Print();
		return FALSE;
	}

	PySys_SetArgv (1, argv);

	/* Retrieve the Python type for anjuta plugin */
	m = PyImport_ImportModule ("gi.repository.Anjuta");
	if (m != NULL)
		plugin_type = (PyTypeObject *) PyObject_GetAttrString (m, "Plugin");

	if (plugin_type == NULL) {
		PyErr_Print ();
		return FALSE;
	}

	return TRUE;
}

static gboolean
pyl_plugin_deactivate (AnjutaPlugin *plugin)
{
	//PythonLoaderPlugin *self = ANJUTA_PYTHON_LOADER_PLUGIN (plugin);

	if (Py_IsInitialized ()) {
		while (PyGC_Collect ())
			;

		Py_Finalize ();
	}

	return TRUE;
}


/* GObject functions
 *---------------------------------------------------------------------------*/

static gpointer parent_class;

static void
pyl_plugin_instance_init (GObject *obj)
{
}

static void
pyl_plugin_class_init (GObjectClass *klass)
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = pyl_plugin_activate;
	plugin_class->deactivate = pyl_plugin_deactivate;
}

ANJUTA_PLUGIN_BEGIN (PythonLoaderPlugin, pyl_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(pyl_ianjuta_plugin_factory, IANJUTA_TYPE_PLUGIN_FACTORY);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (PythonLoaderPlugin, pyl_plugin);
