/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 *  ianjuta-document-manager.h
 *  Copyright (C) 2004 Naba Kumar  <naba@gnome.org>
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

#ifndef _IANJUTA_DOCUMENT_MANAGER_H_
#define _IANJUTA_DOCUMENT_MANAGER_H_

#include <glib-object.h>
#include <gtk/gtkwidget.h>

#include <libanjuta/interfaces/ianjuta-editor.h>

G_BEGIN_DECLS

#define IANJUTA_TYPE_DOCUMENT_MANAGER            (ianjuta_document_manager_get_type ())
#define IANJUTA_DOCUMENT_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), IANJUTA_TYPE_DOCUMENT_MANAGER, IAnjutaDocumentManager))
#define IANJUTA_IS_DOCUMENT_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IANJUTA_TYPE_DOCUMENT_MANAGER))
#define IANJUTA_DOCUMENT_MANAGER_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), IANJUTA_TYPE_DOCUMENT_MANAGER, IAnjutaDocumentManagerIface))

#define IANJUTA_DOCUMENT_MANAGER_ERROR ianjuta_document_manager_error_quark()

typedef struct _IAnjutaDocumentManager       IAnjutaDocumentManager;
typedef struct _IAnjutaDocumentManagerIface  IAnjutaDocumentManagerIface;

struct _IAnjutaDocumentManagerIface {
	GTypeInterface g_iface;
	
	/* Virtual Table */
	void (*goto_file_line) (IAnjutaDocumentManager *docman,
							const gchar *file, gint lineno, GError **e);
	IAnjutaEditor* (*get_current_editor) (IAnjutaDocumentManager *docman,
										  GError **e);
	void (*set_current_editor) (IAnjutaDocumentManager *docman,
								IAnjutaEditor *editor, GError **e);
	GList* (*get_editors) (IAnjutaDocumentManager *docman, GError **e);
};

enum IAnjutaDocumentManagerError {
	IANJUTA_DOCUMENT_MANAGER_ERROR_DOESNT_EXIST,
};

GQuark ianjuta_document_manager_error_quark (void);
GType  ianjuta_document_manager_get_type (void);
void ianjuta_document_manager_goto_file_line (IAnjutaDocumentManager *docman,
											  const gchar *file, gint lineno,
											  GError **e);
IAnjutaEditor* ianjuta_document_manager_get_current_editor (IAnjutaDocumentManager *docman,
															GError **e);
void ianjuta_document_manager_set_current_editor (IAnjutaDocumentManager *docman,
												  IAnjutaEditor *editor,
												  GError **e);
GList* ianjuta_document_manager_get_editors (IAnjutaDocumentManager *docman,
											 GError **e);

G_END_DECLS

#endif
