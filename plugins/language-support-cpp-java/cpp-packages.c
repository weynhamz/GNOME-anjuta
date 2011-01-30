/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * cpp-packages.c
 *
 * Copyright (C) 2011 - Johannes Schmid
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "cpp-packages.h"

#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>
#include <libanjuta/anjuta-pkg-config.h>
#include <libanjuta/anjuta-debug.h>

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

static void
list_all_children (GList **children, GFile *dir)
{
	GFileEnumerator *list;
					
	list = g_file_enumerate_children (dir,
	    G_FILE_ATTRIBUTE_STANDARD_NAME,
	    G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
	    NULL,
	    NULL);

	if (list != NULL)
	{
		GFileInfo *info;
		
		while ((info = g_file_enumerator_next_file (list, NULL, NULL)) != NULL)
		{
			const gchar *name;
			GFile *file;

			name = g_file_info_get_name (info);
			file = g_file_get_child (dir, name);
			g_object_unref (info);

			if (g_file_query_file_type (file, G_FILE_QUERY_INFO_NONE, NULL) == G_FILE_TYPE_DIRECTORY)
			{
				list_all_children (children, file);
			}
			else
			{
				gchar* filename = g_file_get_path (file);
				*children = g_list_prepend (*children, filename);
			}
			g_object_unref (file);
		}
		g_file_enumerator_close (list, NULL, NULL);
		g_object_unref (list);
	}
}

static void
cpp_packages_add_package (IAnjutaSymbolManager* sm,
                          const gchar* pkg,
                          const gchar* version)
{
	GList* dirs = anjuta_pkg_config_get_directories (pkg, TRUE, NULL);
	GList* dir;
	GList* children = NULL;
	for (dir = dirs; dir != NULL; dir = g_list_next (dir))
	{
		GFile* file = g_file_new_for_path (dir->data);
		list_all_children (&children, file);
	}
	g_message ("Adding package: %s", pkg);
	if (children)
		ianjuta_symbol_manager_add_package (sm, pkg, version, children, NULL);
	anjuta_util_glist_strings_free (children);
	anjuta_util_glist_strings_free (dirs);
}

static GList*
cpp_packages_activate_package (IAnjutaSymbolManager* sm, const gchar* pkg, const gchar* version, 
                               GList** packages_to_add)
{
	g_message ("Activate package: %s", pkg);
	/* Only query each package once */
	if (g_list_find_custom (*packages_to_add,
	                        pkg, (GCompareFunc) pkg_data_compare))
		return *packages_to_add;
	if (!ianjuta_symbol_manager_activate_package (sm, pkg, version, NULL))
	{
		GList* deps = anjuta_pkg_config_list_dependencies (pkg, NULL);
		GList* dep;
		PackageData* data = g_new0 (PackageData, 1);
		for (dep = deps; dep != NULL; dep = g_list_next (dep))
		{
			g_message ("Adding dependency: %s", dep->data);
			gchar* dep_version =
				anjuta_pkg_config_get_version (dep->data);
			if (dep_version)
				cpp_packages_activate_package (sm, dep->data, dep_version, packages_to_add);
			g_free (dep_version);
		}
		anjuta_util_glist_strings_free (deps);
		data->pkg = g_strdup(pkg);
		data->version = g_strdup (version);
		*packages_to_add = g_list_prepend (*packages_to_add,
		                                   data);
	}
	return *packages_to_add;
}

static void
cpp_packages_load_real (AnjutaShell* shell, GError* error, IAnjutaProjectManager* pm)
{
	IAnjutaSymbolManager* sm =
		anjuta_shell_get_interface (shell, IAnjutaSymbolManager, NULL);		
	GList* packages;
	GList* pkg;
	GList* packages_to_add = NULL;

	g_message ("Project loaded");
	
	if (!pm || !sm)
		return;
	
	packages = ianjuta_project_manager_get_packages (pm, NULL);
	for (pkg = packages; pkg != NULL; pkg = g_list_next (pkg))
	{
		gchar* version =
			anjuta_pkg_config_get_version (pkg->data);
		if (version)
		{
			cpp_packages_activate_package (sm, pkg->data, version, &packages_to_add);
		}
		g_free (version);
	}
	g_list_free (packages);
	for (pkg = packages_to_add; pkg != NULL; pkg = g_list_next (pkg))
	{
		PackageData* pkg_data = pkg->data;
		cpp_packages_add_package (sm, pkg_data->pkg, pkg_data->version);
	}
	g_list_foreach (packages_to_add, (GFunc) pkg_data_free, NULL);
	g_list_free (packages_to_add);
}

void 
cpp_packages_load (CppJavaPlugin* plugin)
{
	static gboolean init = FALSE;
	AnjutaShell* shell = ANJUTA_PLUGIN (plugin)->shell;
	IAnjutaProjectManager* pm =
		anjuta_shell_get_interface (shell, IAnjutaProjectManager, NULL);
	IAnjutaProject* project;

	if (init)
		return;
	init = TRUE;
	
	if (!pm)
		return;

	g_message ("Connecting signal");
	
	g_signal_connect_swapped (pm, "project-loaded", G_CALLBACK (cpp_packages_load_real), shell);

	project = ianjuta_project_manager_get_current_project (pm, NULL);
	if (project && ianjuta_project_is_loaded (project, NULL))
	{
		cpp_packages_load_real (shell, NULL, pm);
	}		
}
