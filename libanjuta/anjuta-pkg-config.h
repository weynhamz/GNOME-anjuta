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

#ifndef _ANJUTA_PKG_CONFIG_H_
#define _ANJUTA_PKG_CONFIG_H_

#include <glib.h>
 
GList* 		anjuta_pkg_config_list_dependencies (const gchar* package, 
                                            	 	GError** error);

GList* 		anjuta_pkg_config_get_directories 	(const gchar* pkg_name, 
                                          			gboolean no_deps, 
                                          			GError** error);

gboolean 	anjuta_pkg_config_ignore_package 	(const gchar* name);

gchar* 		anjuta_pkg_config_get_version 		(const gchar* package);

#endif

