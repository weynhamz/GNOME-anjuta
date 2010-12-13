/*
 * anjuta-pkg-config.c
 *
 * Copyright (C) 2010 - Johannes Schmid
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

#include <glib.h>
#include <string.h>

#include <libanjuta/anjuta-pkg-config.h>
#include <libanjuta/anjuta-utils.h>

/* Blacklist for packages that shouldn't be parsed. Those packages
 * usually contain the same include paths as their top-level package
 * Checked with g_str_has_prefix() so partial names are ok!
 */
const gchar* ignore_packages[] = 
{
	"gdk-x11-",
	NULL
};

static GList*
remove_includes (GList* includes, GList* dependencies)
{
	GList* dir;
	for (dir = dependencies; dir != NULL; dir = g_list_next (dir))
	{
		GList* find = g_list_find_custom (includes, dir->data, (GCompareFunc)strcmp);
		if (find)
		{
			g_free (find->data);
			includes = g_list_delete_link (includes, find);
		}
	}
	return includes;
}

/*
 * anjuta_pkg_config_list_dependencies:
 * @pkg_name: Name of the package to get the include directories for
 * @error: Error handling
 * 
 * This does (potentially) blocking, call from a thread if necessary
 * 
 * Returns: a list of packages @pkg depends on
 */
GList*
anjuta_pkg_config_list_dependencies (const gchar* package, GError** error)
{
	GList* deps = NULL;
	gchar* cmd;
	gchar *err;
	gchar *out;
	gint status;
	
	cmd = g_strdup_printf ("pkg-config --print-requires --print-requires-private %s",
	                       package);
	if (g_spawn_command_line_sync (cmd, &out, &err, &status, error))
	{
		gchar** depends = g_strsplit (out, "\n", -1);
		if (depends != NULL)
		{
			gchar** depend;
			for (depend = depends; *depend != NULL; depend++)
			{
				if (strlen (*depend) && !anjuta_pkg_config_ignore_package (*depend))
				{
					deps = g_list_append (deps, g_strdup (*depend));
				}
			}
			g_strfreev (depends);
		}
		g_free (out);
	}
	g_free (cmd);

	return deps;
}

/*
 * anjuta_pkg_config_get_directories:
 * @pkg_name: Name of the package to get the include directories for
 * @no_deps: Don't include directories of the dependencies
 * @error: Error handling
 * 
 * This does (potentially) blocking, call from a thread if necessary
 * 
 * Returns: a list of include directories of the package
 */
GList*
anjuta_pkg_config_get_directories (const gchar* pkg_name, gboolean no_deps, GError** error)
{
	gchar *cmd;
	gchar *err;
	gchar *out;
	gint status;
	GList *dirs = NULL;

	cmd = g_strdup_printf ("pkg-config --cflags-only-I %s", pkg_name);

	if (g_spawn_command_line_sync (cmd, &out, &err, &status, error))
	{
		gchar **flags;

		flags = g_strsplit (out, " ", -1);

		if (flags != NULL)
		{
			gchar **flag;

			for (flag = flags; *flag != NULL; flag++)
			{
				if (g_regex_match_simple ("\\.*/include/\\w+", *flag, 0, 0) == TRUE)
				{
					dirs = g_list_prepend (dirs, g_strdup (*flag + 2));
				}
			}
			g_strfreev (flags);
		}
		g_free (out);
	}
	g_free (cmd);

	if (dirs && no_deps)
	{
		GList* pkgs = anjuta_pkg_config_list_dependencies (pkg_name, error);
		GList* pkg;
		for (pkg = pkgs; pkg != NULL; pkg = g_list_next (pkg))
		{
			GList* dep_dirs = anjuta_pkg_config_get_directories (pkg->data, FALSE, NULL);
			dirs = remove_includes (dirs, dep_dirs);
			anjuta_util_glist_strings_free (dep_dirs);
		}
		anjuta_util_glist_strings_free (pkgs);
	}
	
	return dirs;
}

/*
 * anjuta_pkg_config_ignore_package
 * @name: Name of the package to ignore
 * 
 * Returns: TRUE if the package does not contain
 * valid information for the build system and should be
 * ignored.
 */
gboolean
anjuta_pkg_config_ignore_package (const gchar* name)
{
	const gchar** pkg;
	for (pkg = ignore_packages; *pkg != NULL; pkg++)
	{
		if (g_str_has_prefix (name, *pkg))
			return TRUE;
	}
	return FALSE;
}