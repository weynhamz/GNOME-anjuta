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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _ANJUTA_DOCMAN_H_
#define _ANJUTA_DOCMAN_H_

#include <gtk/gtkwidget.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-document.h>

#include "plugin.h"

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
	AnjutaShell *shell;
};

struct _AnjutaDocmanClass {
	GtkNotebookClass parent_class;
	
	/* Signals */
	void (*document_added) (IAnjutaDocument *document);
	void (*document_changed) (IAnjutaDocument *new_document);
};

GType anjuta_docman_get_type (void);
GtkWidget* anjuta_docman_new (DocmanPlugin *plugin, AnjutaPreferences *pref);

void anjuta_docman_set_popup_menu (AnjutaDocman *docman, GtkWidget *menu);

IAnjutaEditor *anjuta_docman_add_editor (AnjutaDocman *docman, const gchar *uri,
									  	const gchar *name);
void anjuta_docman_add_document (AnjutaDocman *docman, IAnjutaDocument *doc,
								const gchar* uri);

void anjuta_docman_remove_document (AnjutaDocman *docman, IAnjutaDocument *doc);

IAnjutaDocument *anjuta_docman_get_current_document (AnjutaDocman *docman);
IAnjutaDocument *anjuta_docman_get_document_for_path (AnjutaDocman *docman,
													  const gchar *full_path);

GtkWidget *anjuta_docman_get_current_focus_widget (AnjutaDocman *docman);

void anjuta_docman_set_current_document (AnjutaDocman *docman, IAnjutaDocument *doc);

IAnjutaEditor *anjuta_docman_goto_file_line (AnjutaDocman *docman,
											const gchar *fname,
											gint lineno);
IAnjutaEditor *anjuta_docman_goto_file_line_mark (AnjutaDocman *docman,
												const gchar *fname,
												gint lineno,
												gboolean mark);
void anjuta_docman_present_notebook_page (AnjutaDocman *docman, IAnjutaDocument *doc);

void anjuta_docman_delete_all_markers (AnjutaDocman *docman, gint marker);
void anjuta_docman_delete_all_indicators (AnjutaDocman *docman);

void anjuta_docman_save_file_if_modified (AnjutaDocman *docman,
										  const gchar *szFullPath);
void anjuta_docman_reload_file (AnjutaDocman *docman, const gchar *szFullPath);

gboolean anjuta_docman_set_editor_properties (AnjutaDocman *docman);

gchar *anjuta_docman_get_full_filename (AnjutaDocman *docman, const gchar *fn);

GList *anjuta_docman_get_all_doc_widgets (AnjutaDocman *docman);

void anjuta_docman_open_file (AnjutaDocman *docman);

/* Returns TRUE if editor is saved */
gboolean anjuta_docman_save_document (AnjutaDocman *docman, IAnjutaDocument *doc,
									GtkWidget *parent_window);

/* Returns TRUE if editor is saved */
gboolean anjuta_docman_save_document_as (AnjutaDocman *docman,
										IAnjutaDocument *doc,
										GtkWidget *parent_window);
#endif
