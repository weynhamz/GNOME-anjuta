/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 *  ianjuta-file.h
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
#ifndef _IANJUTA_FILE_H_
#define _IANJUTA_FILE_H_

#include <glib-object.h>
#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

#define IANJUTA_TYPE_FILE            (ianjuta_file_get_type ())
#define IANJUTA_FILE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), IANJUTA_TYPE_FILE, IAnjutaFile))
#define IANJUTA_IS_FILE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IANJUTA_TYPE_FILE))
#define IANJUTA_FILE_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), IANJUTA_TYPE_FILE, IAnjutaFileIface))

#define IANJUTA_FILE_ERROR ianjuta_file_error_quark()

typedef struct _IAnjutaFile       IAnjutaFile;
typedef struct _IAnjutaFileIface  IAnjutaFileIface;

struct _IAnjutaFileIface {
	GTypeInterface g_iface;
	
	/* Virtual Table */
	gboolean (*open) (IAnjutaFile *file, const gchar *filename, GError  **e);
};

enum IAnjutaFileError {
	IANJUTA_FILE_ERROR_DOESNT_EXIST,
};

GQuark ianjuta_file_error_quark (void);
GType  ianjuta_file_get_type (void);

gboolean ianjuta_file_open (IAnjutaFile *file, const gchar *filename, GError **e);

G_END_DECLS

#endif
