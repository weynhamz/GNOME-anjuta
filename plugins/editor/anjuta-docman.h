/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    anjuta-docman.h
    Copyright (C) 2003  Naba Kumar <naba@gnome.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _ANJUTA_DOCMAN_H_
#define _ANJUTA_DOCMAN_H_

#include <gtk/gtkwidget.h>
#include <libanjuta/anjuta-preferences.h>

#include "text_editor.h"

#define ANJUTA_TYPE_DOCMAN        (anjuta_docman_get_type ())
#define ANJUTA_DOCMAN(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_DOCMAN, AnjutaDocman))
#define ANJUTA_DOCMAN_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), ANJUTA_TYPE_DOCMAN, AnjutaDocmanClass))
#define ANJUTA_IS_DOCMAN(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_DOCMAN))
#define ANJUTA_IS_DOCMAN_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_DOCMAN))

typedef struct _AnjutaDocman AnjutaDocman;
typedef struct _AnjutaDocmanPriv AnjutaDocmanPriv;
typedef struct _AnjutaDocmanClass AnjutaDocmanClass;

struct _AnjutaDocman {
	GtkNotebook parent;
	AnjutaDocmanPriv *priv;
};

struct _AnjutaDocmanClass {
	GtkNotebookClass parent_class;
};

GType anjuta_docman_get_type (void);
GtkWidget* anjuta_docman_new (AnjutaPreferences *pref);

TextEditor* anjuta_docman_add_editor (AnjutaDocman *docman, gchar *uri,
									  gchar *name);
void anjuta_docman_remove_editor (AnjutaDocman *docman, TextEditor* te);

TextEditor* anjuta_docman_get_current_editor (AnjutaDocman *docman);
TextEditor* anjuta_docman_get_editor_from_path (AnjutaDocman *docman,
												const gchar *full_path);

void anjuta_docman_set_current_editor (AnjutaDocman *docman, TextEditor *te);

TextEditor* anjuta_docman_goto_file_line (AnjutaDocman *docman, gchar * fname,
										  glong lineno);
TextEditor* anjuta_docman_goto_file_line_mark (AnjutaDocman *docman,
											   gchar *fname, glong lineno,
											   gboolean mark);
void anjuta_docman_show_editor (AnjutaDocman *docman, GtkWidget* te);

void anjuta_docman_delete_all_markers (AnjutaDocman *docman, gint marker);
void anjuta_docman_delete_all_indicators (AnjutaDocman *docman);

void anjuta_docman_save_file_if_modified (AnjutaDocman *docman,
										  const gchar *szFullPath);
void anjuta_docman_reload_file (AnjutaDocman *docman, const gchar *szFullPath);

gboolean anjuta_docman_set_editor_properties (AnjutaDocman *docman);

gchar* anjuta_docman_get_full_filename (AnjutaDocman *docman, const gchar *fn);

TextEditor* anjuta_docman_find_editor_with_path (AnjutaDocman *docman,
												 const gchar *file_path);

GList* anjuta_docman_get_all_editors (AnjutaDocman *docman);

void anjuta_docman_open_file (AnjutaDocman *docman);

void anjuta_docman_save_as_file (AnjutaDocman *docman);

#endif
