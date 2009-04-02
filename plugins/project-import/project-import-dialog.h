/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Carl-Anton Ingmarsson 2009 <ca.ingmarsson@gmail.com>
 * 
 * anjuta is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _PROJECT_IMPORT_DIALOG_H_
#define _PROJECT_IMPORT_DIALOG_H_

#include <gtk/gtk.h>
#include <glib-object.h>

#include <libanjuta/anjuta-plugin.h>

G_BEGIN_DECLS

#define PROJECT_IMPORT_TYPE_DIALOG             (project_import_dialog_get_type ())
#define PROJECT_IMPORT_DIALOG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), PROJECT_IMPORT_TYPE_DIALOG, ProjectImportDialog))
#define PROJECT_IMPORT_DIALOG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), PROJECT_IMPORT_TYPE_DIALOG, ProjectImportDialogClass))
#define PROJECT_IS_IMPORT_DIALOG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PROJECT_IMPORT_TYPE_DIALOG))
#define PROJECT_IS_IMPORT_DIALOG_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), PROJECT_IMPORT_TYPE_DIALOG))
#define PROJECT_IMPORT_DIALOG_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), PROJECT_IMPORT_TYPE_DIALOG, ProjectImportDialogClass))

typedef struct _ProjectImportDialogClass ProjectImportDialogClass;
typedef struct _ProjectImportDialog ProjectImportDialog;

struct _ProjectImportDialogClass
{
	GtkDialogClass parent_class;
};

struct _ProjectImportDialog
{
	GtkDialog parent_instance;
};

GType project_import_dialog_get_type (void) G_GNUC_CONST;

ProjectImportDialog *project_import_dialog_new (AnjutaPluginManager *plugin_manager, const gchar *name, GFile *dir);

gchar *project_import_dialog_get_name (ProjectImportDialog *import_dialog);

GFile *project_import_dialog_get_source_dir (ProjectImportDialog *import_dialog);

GFile *project_import_dialog_get_dest_dir (ProjectImportDialog *import_dialog);
gchar * project_import_dialog_get_vcs_uri (ProjectImportDialog *import_dialog);
gchar *project_import_dialog_get_vcs_id (ProjectImportDialog *import_dialog);

G_END_DECLS

#endif /* _PROJECT_IMPORT_DIALOG_H_ */
