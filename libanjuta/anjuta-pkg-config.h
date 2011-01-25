/*
 * anjuta-pkg-config.h
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

#ifndef __ANJUTA_PKG_CONFIG__
#define __ANJUTA_PKG_CONFIG__

#include <glib.h>

/**
 * anjuta_pkg_config_list_dependencies:
 * @pkg_name: Name of the package to get the include directories for
 * @error: Error handling
 * 
 * This does (potentially) blocking, call from a thread if necessary
 * 
 * Returns: a list of packages @pkg depends on
 */
GList* 		anjuta_pkg_config_list_dependencies (const gchar* pkg_name, 
												GError** error);

/**
 * anjuta_pkg_config_get_abs_headers:
 * @pkg_name: Name of the package to get the include directories for
 * @error: Error handling
 *
 * Returns a list of absolute path files (gchar*) of the headers of the #pkg_name.
 */
GList*		anjuta_pkg_config_get_abs_headers	(const gchar* pkg_name,
			                                    gboolean no_deps,
			                                    GError** error);
/**
 * anjuta_pkg_config_get_directories:
 * @pkg_name: Name of the package to get the include directories for
 * @no_deps: Don't include directories of the dependencies
 * @error: Error handling
 * 
 * This does (potentially) blocking, call from a thread if necessary
 * 
 * Returns: a list of include directories (gchar(s)) of the package 
 */
GList* 		anjuta_pkg_config_get_directories (const gchar* pkg_name, 
                                          		gboolean no_deps, 
	                                          	GError** error);


/**
 * anjuta_pkg_config_ignore_package:
 * @pkg_name: Name of the package to ignore
 * 
 * Returns: TRUE if the package does not contain
 * valid information for the build system and should be
 * ignored.
 */
gboolean 	anjuta_pkg_config_ignore_package (const gchar* pkg_name);

gchar* anjuta_pkg_config_get_version (const gchar* package);

#endif
