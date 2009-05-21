/*
 * cvs-callbacks.h (c) 2005 Johannes Schmid
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

#ifndef CVS_CALLBACKS_H
#define CVS_CALLBACKS_H

#include "plugin.h"

typedef struct
{
	GtkBuilder* bxml;
	CVSPlugin* plugin;
} CVSData;

enum
{
	DIFF_STANDARD = 0,
	DIFF_PATCH = 1
};

enum
{
	SERVER_LOCAL = 0,
	SERVER_EXTERN = 1,
	SERVER_PASSWORD = 2,
};

CVSData* cvs_data_new(CVSPlugin* plugin, GtkBuilder* bxml);
void cvs_data_free(CVSData* data);

void
on_cvs_add_response(GtkDialog* dialog, gint response, CVSData* data);

void
on_cvs_remove_response(GtkDialog* dialog, gint response, CVSData* data);

void
on_cvs_update_response(GtkDialog* dialog, gint response, CVSData* data);

void
on_cvs_commit_response(GtkDialog* dialog, gint response, CVSData* data);

void
on_cvs_diff_response(GtkDialog* dialog, gint response, CVSData* data);

void
on_cvs_status_response(GtkDialog* dialog, gint response, CVSData* data);

void
on_cvs_log_response(GtkDialog* dialog, gint response, CVSData* data);

void
on_cvs_import_response(GtkDialog* dialog, gint response, CVSData* data);

#endif
