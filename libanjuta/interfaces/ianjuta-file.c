/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 *  ianjuta-file.c
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
#include <config.h>

#include "ianjuta-file.h"

GQuark
ianjuta_file_error_quark (void)
{
	static GQuark quark = 0;
	
	if (quark == 0) {
		quark = g_quark_from_static_string ("ianjuta-message-view-quark");
	}
	
	return quark;
}

/**
 * ianjuta_file_open:
 * @filename: file to open
 * @e: Error reporting and propagation object
 * 
 * Opens @filename
 *
 * Return value: TRUE if successful, otherwise FALSE.
 */
gboolean
ianjuta_file_open (IAnjutaFile *file, const gchar *filename, GError **e)
{
	g_return_val_if_fail (file != NULL, FALSE);
	g_return_val_if_fail (IANJUTA_IS_FILE (file), FALSE);

	return IANJUTA_FILE_GET_IFACE (file)->open (file, filename, e);
}

static void
ianjuta_file_base_init (gpointer gclass)
{
}

GType
ianjuta_file_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (IAnjutaFileIface),
			ianjuta_file_base_init,
			NULL, 
			NULL,
			NULL,
			NULL,
			0,
			0,
			NULL
		};
		
		type = g_type_register_static (G_TYPE_INTERFACE, 
					       "IAnjutaFile", 
					       &info,
					       0);
		
		g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	}
	return type;			
}
