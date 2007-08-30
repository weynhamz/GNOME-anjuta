/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef CVS_INTERFACE_H
#define CVS_INTERFACE_H

#include "plugin.h"

void anjuta_cvs_add (AnjutaPlugin *obj, const gchar* filename, 
	gboolean binary, GError **err);
	
void anjuta_cvs_commit (AnjutaPlugin *obj, const gchar* filename, const gchar* log,
	const gchar* rev, gboolean recurse, GError **err);

void anjuta_cvs_diff (AnjutaPlugin *obj, const gchar* filename, const gchar* rev, 
	gboolean recurse, gboolean patch_style, gboolean unified, GError **err);
	
void anjuta_cvs_log (AnjutaPlugin *obj, const gchar* filename, gboolean recurse, gboolean verbose, GError **err);

void anjuta_cvs_remove (AnjutaPlugin *obj, const gchar* filename, GError **err);

void anjuta_cvs_status (AnjutaPlugin *obj, const gchar* filename, gboolean recurse, gboolean verbose, GError **err);

void anjuta_cvs_update (AnjutaPlugin *obj, const gchar* filename, gboolean recurse, 
	gboolean prune, gboolean create, gboolean reset_sticky, const gchar* revision, GError **err);

void anjuta_cvs_import (AnjutaPlugin *obj, const gchar* dir, const gchar* cvsroot,
	const gchar* module, const gchar* vendor, const gchar* release,
	const gchar* log, gint server_type, const gchar* username, const 
	gchar* password, GError** error);

#endif /* CVS_INTERFACE_H */
