/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Johannes Schmid 2007 <jhs@gnome.org>
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

#ifndef _FILE_MODEL_H_
#define _FILE_MODEL_H_

#include <glib-object.h>
#include <gtk/gtk.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define FILE_TYPE_MODEL             (file_model_get_type ())
#define FILE_MODEL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), FILE_TYPE_MODEL, FileModel))
#define FILE_MODEL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), FILE_TYPE_MODEL, FileModelClass))
#define FILE_IS_MODEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FILE_TYPE_MODEL))
#define FILE_IS_MODEL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), FILE_TYPE_MODEL))
#define FILE_MODEL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), FILE_TYPE_MODEL, FileModelClass))

typedef struct _FileModelClass FileModelClass;
typedef struct _FileModel FileModel;

/* Has to be public here because we also need it in the tree */

enum
{
	COLUMN_PIXBUF,
	COLUMN_FILENAME,
	COLUMN_DISPLAY,
	COLUMN_FILE,
	COLUMN_IS_DIR,
	COLUMN_SORT,
	N_COLUMNS
};

struct _FileModelClass
{
	GtkTreeStoreClass parent_class;
};

struct _FileModel
{
	GtkTreeStore parent_instance;
};

GType
file_model_get_type (void) G_GNUC_CONST;
FileModel*
file_model_new (GtkTreeView* tree_view, const gchar* base_uri);

void 
file_model_refresh (FileModel* model);

GFile*
file_model_get_file (FileModel* model, GtkTreeIter* iter);

gchar*
file_model_get_filename (FileModel* model, GtkTreeIter* iter);

G_END_DECLS

#endif /* _FILE_MODEL_H_ */
