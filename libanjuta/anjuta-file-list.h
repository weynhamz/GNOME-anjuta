/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2010 <jrliggett@cox.net>
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

#ifndef _ANJUTA_FILE_LIST_H_
#define _ANJUTA_FILE_LIST_H_

#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_FILE_LIST             (anjuta_file_list_get_type ())
#define ANJUTA_FILE_LIST(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_FILE_LIST, AnjutaFileList))
#define ANJUTA_FILE_LIST_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_FILE_LIST, AnjutaFileListClass))
#define ANJUTA_IS_FILE_LIST(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_FILE_LIST))
#define ANJUTA_IS_FILE_LIST_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_FILE_LIST))
#define ANJUTA_FILE_LIST_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_FILE_LIST, AnjutaFileListClass))

typedef struct _AnjutaFileListClass AnjutaFileListClass;
typedef struct _AnjutaFileList AnjutaFileList;
typedef struct _AnjutaFileListPriv AnjutaFileListPriv;

struct _AnjutaFileListClass
{
	GtkBoxClass parent_class;
};

struct _AnjutaFileList
{
	GtkBox parent_instance;

	AnjutaFileListPriv *priv;
};

GType anjuta_file_list_get_type (void) G_GNUC_CONST;
GtkWidget * anjuta_file_list_new (void);
GList * anjuta_file_list_get_paths (AnjutaFileList *self);
void anjuta_file_list_set_relative_path (AnjutaFileList *self, 
                                         const gchar *path);
void anjuta_file_list_clear (AnjutaFileList *self);

G_END_DECLS

#endif /* _ANJUTA_FILE_LIST_H_ */
