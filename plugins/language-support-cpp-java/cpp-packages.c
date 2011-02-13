/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Johannes Schmid 2011 <jhs@Obelix>
 * 
 * anjuta is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cpp-packages.h"
#include "plugin.h"

#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>
#include <libanjuta/anjuta-pkg-config.h>
#include <libanjuta/anjuta-pkg-scanner.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-preferences.h>

#define PREF_PROJECT_PACKAGES "cpp-load-project-packages"
#define PREF_USER_PACKAGES "cpp-user-packages"

#define PROJECT_LOADED "__cpp_packages_loaded"
#define USER_LOADED "__cpp_user_packages_loaded"

enum
{
	PROP_0,
	PROP_PLUGIN
};

typedef struct
{
	gchar* pkg;
	gchar* version;
} PackageData;

static int
pkg_data_compare (gpointer data1, gpointer pkg2)
{
	PackageData* pkg_data1 = data1;

	return strcmp (pkg_data1->pkg, (const gchar*) pkg2);
}

static void
pkg_data_free (PackageData* data)
{
	g_free (data->pkg);
	g_free (data->version);
	g_free (data);
}


G_DEFINE_TYPE (CppPackages, cpp_packages, G_TYPE_OBJECT);

static GList*
cpp_packages_activate_package (IAnjutaSymbolManager* sm, const gchar* pkg,
                               GList** packages_to_add)
{
	gchar* name = g_strdup (pkg);
	gchar* version;
	gchar* c;

	/* Clean package name */
	for (c = name; *c != '\0'; c++)
	{
		if (g_ascii_isspace (*c))
		{
			*c = '\0';
			break;
		}
	}

	version = anjuta_pkg_config_get_version (name);
	if (!version)
	{
		g_free (name);
		return *packages_to_add;
	}
	
	/* Only query each package once */
	if (g_list_find_custom (*packages_to_add,
	                        name, (GCompareFunc) pkg_data_compare))
		return *packages_to_add;
	if (!ianjuta_symbol_manager_activate_package (sm, name, version, NULL))
	{
		GList* deps = anjuta_pkg_config_list_dependencies (name, NULL);
		GList* dep;
		PackageData* data = g_new0 (PackageData, 1);
		for (dep = deps; dep != NULL; dep = g_list_next (dep))
		{
			cpp_packages_activate_package (sm, dep->data, packages_to_add);
		}
		anjuta_util_glist_strings_free (deps);
		data->pkg = g_strdup (name);
		data->version = g_strdup (version);
		*packages_to_add = g_list_prepend (*packages_to_add,
		                                   data);
	}
	g_free (name);
	return *packages_to_add;
}

static void
on_package_ready (AnjutaCommand* command,
                  gint return_code,
                  IAnjutaSymbolManager* sm)
{
	AnjutaPkgScanner* scanner = ANJUTA_PKG_SCANNER (command);
	if (g_list_length (anjuta_pkg_scanner_get_files (scanner)))
	{
		g_message ("Adding package: %s", anjuta_pkg_scanner_get_package (scanner));
		ianjuta_symbol_manager_add_package (sm,
		                                    anjuta_pkg_scanner_get_package (scanner),
		                                    anjuta_pkg_scanner_get_version (scanner),
		                                    anjuta_pkg_scanner_get_files (scanner),
		                                    NULL);
	}
}

static void
on_queue_finished (AnjutaCommandQueue* queue, gpointer unused)
{
	g_object_unref (queue);
}

static void
cpp_packages_load_real (CppPackages* packages, GError* error, IAnjutaProjectManager* pm)
{
	IAnjutaSymbolManager* sm =
		anjuta_shell_get_interface (packages->plugin->shell, IAnjutaSymbolManager, NULL);		
	GList* pkgs;
	GList* pkg;
	GList* packages_to_add = NULL;
	
	if (!pm || !sm)
		return;

	ianjuta_symbol_manager_deactivate_all (sm, NULL);
	
	pkgs = ianjuta_project_manager_get_packages (pm, NULL);
	for (pkg = pkgs; pkg != NULL; pkg = g_list_next (pkg))
	{
		cpp_packages_activate_package (sm, pkg->data, &packages_to_add);
	}
	g_list_free (pkgs);
	for (pkg = packages_to_add; pkg != NULL; pkg = g_list_next (pkg))
	{
		PackageData* pkg_data = pkg->data;
		AnjutaCommand* command =
			anjuta_pkg_scanner_new (pkg_data->pkg, pkg_data->version);
		g_signal_connect (command, "command-finished",
		                  G_CALLBACK (on_package_ready), sm);
		anjuta_command_queue_push (packages->queue, command);
	}
	g_list_foreach (packages_to_add, (GFunc) pkg_data_free, NULL);
	g_list_free (packages_to_add);

	g_signal_connect (packages->queue, "finished", G_CALLBACK (on_queue_finished), NULL);
	anjuta_command_queue_start (packages->queue);
}

static void
cpp_packages_load_user (CppPackages* packages, gboolean force)
{
	CppJavaPlugin* plugin = ANJUTA_PLUGIN_CPP_JAVA(packages->plugin);
	AnjutaShell* shell = anjuta_plugin_get_shell (ANJUTA_PLUGIN (plugin));
	IAnjutaSymbolManager* sm =
		anjuta_shell_get_interface (shell, IAnjutaSymbolManager, NULL);
	gboolean loaded = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (shell), 
	                                                      USER_LOADED));
	
	if (!loaded && !force)
	{
		gchar* packages_str = g_settings_get_string (plugin->settings,
		                                             PREF_USER_PACKAGES);
		GStrv pkgs = g_strsplit (packages_str, ";", -1);
		gchar** package;
		GList* packages_to_add = NULL;
		GList* pkg;

		ianjuta_symbol_manager_deactivate_all (sm, NULL);

		for (package = pkgs; *package != NULL; package++)
		{
			g_message ("Activating: %s", *package);
			cpp_packages_activate_package (sm, *package, &packages_to_add);
		}
		g_strfreev (pkgs);
		g_free (packages_str);

		for (pkg = packages_to_add; pkg != NULL; pkg = g_list_next (pkg))
		{
			PackageData* pkg_data = pkg->data;
			AnjutaCommand* command =
				anjuta_pkg_scanner_new (pkg_data->pkg, pkg_data->version);
			g_signal_connect (command, "command-finished",
				              G_CALLBACK (on_package_ready), sm);
			anjuta_command_queue_push (packages->queue, command);
		}
		g_list_foreach (packages_to_add, (GFunc) pkg_data_free, NULL);
		g_list_free (packages_to_add);

		g_object_set_data (G_OBJECT (shell), 
		                   USER_LOADED, GINT_TO_POINTER (TRUE));

		g_signal_connect (packages->queue, "finished", G_CALLBACK (on_queue_finished), NULL);
		anjuta_command_queue_start (packages->queue);
	}
}

void 
cpp_packages_load (CppPackages* packages, gboolean force)
{
	CppJavaPlugin* plugin = (CppJavaPlugin*) packages->plugin;

	if (g_settings_get_boolean (plugin->settings,
	                            PREF_PROJECT_PACKAGES))
	{
		IAnjutaProjectManager* pm =
			anjuta_shell_get_interface (packages->plugin->shell, IAnjutaProjectManager, NULL);
		IAnjutaProject* project;
		
		g_signal_connect_swapped (pm, "project-loaded", G_CALLBACK (cpp_packages_load_real), packages);

		project = ianjuta_project_manager_get_current_project (pm, NULL);
		/* Only load the packages if necessary */
		if (project && ianjuta_project_is_loaded (project, NULL))
		{
			gboolean loaded = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (project), 
			                                                      PROJECT_LOADED));
			if (!loaded)
			{
				cpp_packages_load_real (packages, NULL, pm);
				g_object_set_data (G_OBJECT (project), PROJECT_LOADED, GINT_TO_POINTER (TRUE));
			}
		}
	}
	else
	{
		cpp_packages_load_user (packages, force);
	}
}

static void
cpp_packages_init (CppPackages *packages)
{	
	packages->queue = anjuta_command_queue_new (ANJUTA_COMMAND_QUEUE_EXECUTE_MANUAL);
}

static void
cpp_packages_finalize (GObject* object)
{
	CppPackages *packages = CPP_PACKAGES (object);
	AnjutaShell* shell = packages->plugin->shell;
	IAnjutaProjectManager* pm =
		anjuta_shell_get_interface (shell, IAnjutaProjectManager, NULL);
	
	g_signal_handlers_disconnect_by_func (pm, cpp_packages_load_real, packages);

	G_OBJECT_CLASS (cpp_packages_parent_class)->finalize (object);
}

static void
cpp_packages_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	CppPackages* packages;
	
	g_return_if_fail (CPP_IS_PACKAGES (object));

	packages = CPP_PACKAGES (object);
	
	switch (prop_id)
	{
	case PROP_PLUGIN:
		packages->plugin = ANJUTA_PLUGIN (g_value_get_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
cpp_packages_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	CppPackages* packages;
	
	g_return_if_fail (CPP_IS_PACKAGES (object));

	packages = CPP_PACKAGES (object);

	switch (prop_id)
	{
	case PROP_PLUGIN:
		g_value_set_object (value, G_OBJECT (packages->plugin));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
cpp_packages_class_init (CppPackagesClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = cpp_packages_finalize;
	object_class->set_property = cpp_packages_set_property;
	object_class->get_property = cpp_packages_get_property;

	g_object_class_install_property (object_class,
	                                 PROP_PLUGIN,
	                                 g_param_spec_object ("plugin",
	                                                      "CppJavaPlugin",
	                                                      "CppJavaPlugin",
	                                                      ANJUTA_TYPE_PLUGIN,
	                                                      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

CppPackages*
cpp_packages_new (AnjutaPlugin* plugin)
{
	GObject* object =
		g_object_new (CPP_TYPE_PACKAGES, 
		              "plugin", plugin,
		              NULL);
	return CPP_PACKAGES (object);
}
