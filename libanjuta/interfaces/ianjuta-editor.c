/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 *  ianjuta-editor.c
 *  Copyright (C) 2003 Naba Kumar  <naba@gnome.org>
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

#include <config.h>

#include <string.h>
#include <gobject/gvaluecollector.h>
#include <libanjuta/anjuta-marshal.h>
#include "ianjuta-editor.h"

GQuark
ianjuta_editor_error_quark (void)
{
	static GQuark quark = 0;
	
	if (quark == 0) {
		quark = g_quark_from_static_string ("ianjuta-editor-quark");
	}
	
	return quark;
}

gchar*
ianjuta_editor_get_selection (IAnjutaEditor *editor, GError **error)
{
	g_return_val_if_fail (editor != NULL, NULL);
	g_return_val_if_fail (IANJUTA_IS_EDITOR (editor), NULL);

	return IANJUTA_EDITOR_GET_IFACE (editor)->get_selection (editor, error);
}

gchar*
ianjuta_editor_get_filename (IAnjutaEditor *editor, GError **error)
{
	g_return_val_if_fail (editor != NULL, NULL);
	g_return_val_if_fail (IANJUTA_IS_EDITOR (editor), NULL);

	return IANJUTA_EDITOR_GET_IFACE (editor)->get_filename (editor, error);
}

void
ianjuta_editor_goto_line (IAnjutaEditor *editor, gint lineno, GError **e)
{
	g_return_if_fail (editor != NULL);
	g_return_if_fail (IANJUTA_IS_EDITOR (editor));

	IANJUTA_EDITOR_GET_IFACE (editor)->goto_line (editor, lineno, e);
}

gchar*
ianjuta_editor_get_text (IAnjutaEditor *editor, gint start, gint end,
						 GError **e)
{
	g_return_val_if_fail (editor != NULL, NULL);
	g_return_val_if_fail (IANJUTA_IS_EDITOR (editor), NULL);
	g_return_val_if_fail (start <= end, NULL);

	return IANJUTA_EDITOR_GET_IFACE (editor)->get_text (editor, start,
														end, e);
}

gchar*
ianjuta_editor_get_attributes (IAnjutaEditor *editor, gint start,
							   gint end, GError **e)
{
	g_return_val_if_fail (editor != NULL, NULL);
	g_return_val_if_fail (IANJUTA_IS_EDITOR (editor), NULL);

	return IANJUTA_EDITOR_GET_IFACE (editor)->get_attributes (editor, start,
															  end, e);
}

static void
ianjuta_editor_base_init (gpointer gclass)
{
	static gboolean initialized = FALSE;
	
	if (!initialized) {
		g_signal_new ("save-point",
			      IANJUTA_TYPE_EDITOR,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (IAnjutaEditorIface, save_point),
			      NULL, NULL,
			      anjuta_cclosure_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
	}
}

GType
ianjuta_editor_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (IAnjutaEditorIface),
			ianjuta_editor_base_init,
			NULL, 
			NULL,
			NULL,
			NULL,
			0,
			0,
			NULL
		};
		
		type = g_type_register_static (G_TYPE_INTERFACE, 
					       "IAnjutaEditor", 
					       &info,
					       0);
		
		g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	}
	return type;			
}
