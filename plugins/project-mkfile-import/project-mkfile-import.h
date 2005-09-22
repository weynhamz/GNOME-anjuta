/*
 *  project_mkfile_import.h Copyright (c) 2005 Johannes Schmid, Eric Greveson
 *  Based on project_import.h by Johannes Schmid
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef PROJECT_MKFILE_IMPORT_H
#define PROJECT_MKFILE_IMPORT_H

#include <glib.h>
#include <glib-object.h>

#include "plugin.h"

#define PROJECT_MKFILE_IMPORT_TYPE         (project_mkfile_import_get_type ())
#define PROJECT_MKFILE_IMPORT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), PROJECT_MKFILE_IMPORT_TYPE, ProjectMkfileImport))
#define PROJECT_MKFILE_IMPORT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), PROJECT_MKFILE_IMPORT_TYPE, ProjectMkfikeImportClass))
#define IS_PROJECT_MKFILE_IMPORT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), PROJECT_MKFILE_IMPORT_TYPE))
#define IS_PROJECT_MKFILE_IMPORT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), PROJECT_MKFILE_IMPORT_TYPE))
#define PROJECT_MKFILE_IMPORT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), PROJECT_MKFILE_IMPORT_TYPE, ProjectMkfileImportClass))

typedef struct _ProjectMkfileImport ProjectMkfileImport;
typedef struct _ProjectMkfileImportClass ProjectMkfileImportClass;
	
struct _ProjectMkfileImport
{
	GObject parent;
	
	GtkWidget* window;
	GtkWidget* druid;
	GtkWidget* import_name;
	GtkWidget* import_path;
	GtkWidget* import_finish;
	
	AnjutaPlugin* plugin;
	
};

struct _ProjectMkfileImportClass
{
	GObjectClass parent_class;
};

GType project_mkfile_import_get_type(void);
ProjectMkfileImport *project_mkfile_import_new(AnjutaPlugin* plugin);

void project_mkfile_import_set_name (ProjectMkfileImport *pi, const gchar *name);
void project_mkfile_import_set_directory (ProjectMkfileImport *pi, const gchar *directory);

gboolean project_mkfile_import_generate_file(ProjectMkfileImport* pi, const gchar* prjfile);

#endif /* PROJECT_MKFILE_IMPORT_H */
