/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 *  ianjuta-editor.h
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

#ifndef _IANJUTA_EDITOR_H_
#define _IANJUTA_EDITOR_H_

#include <glib-object.h>
#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

#define IANJUTA_TYPE_EDITOR            (ianjuta_editor_get_type ())
#define IANJUTA_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), IANJUTA_TYPE_EDITOR, IAnjutaEditor))
#define IANJUTA_IS_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IANJUTA_TYPE_EDITOR))
#define IANJUTA_EDITOR_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), IANJUTA_TYPE_EDITOR, IAnjutaEditorIface))

#define IANJUTA_EDITOR_ERROR ianjuta_editor_error_quark()

typedef struct _IAnjutaEditor       IAnjutaEditor;
typedef struct _IAnjutaEditorIface  IAnjutaEditorIface;

struct _IAnjutaEditorIface {
	GTypeInterface g_iface;
	
	/* Signals */
	void (*save_point) (IAnjutaEditor *editor, gboolean entered /* or left */);

	/* Virtual Table */
	gchar* (*get_selection) (IAnjutaEditor *editor, GError  **e);
	gchar* (*get_filename) (IAnjutaEditor *editor, GError **e);
	gchar* (*get_text) (IAnjutaEditor *editor, gint start, gint end, GError **e);
	gchar* (*get_attributes) (IAnjutaEditor *editor, gint start, gint end,
								  GError **e);
	void (*goto_line) (IAnjutaEditor *editor, gint lineno, GError **e);
};

enum IAnjutaEditorError {
	IANJUTA_EDITOR_ERROR_DOESNT_EXIST,
};

GQuark ianjuta_editor_error_quark (void);
GType  ianjuta_editor_get_type (void);
gchar* ianjuta_editor_get_selection (IAnjutaEditor *editor, GError **e);
gchar* ianjuta_editor_get_filename (IAnjutaEditor *editor, GError **e);
void ianjuta_editor_goto_line (IAnjutaEditor *editor, gint lineno, GError **e);
gchar *ianjuta_editor_get_text (IAnjutaEditor *editor, gint start, gint end,
								GError **e);
gchar *ianjuta_editor_get_attributes (IAnjutaEditor *editor, gint start,
									  gint end, GError **e);

G_END_DECLS

#endif
