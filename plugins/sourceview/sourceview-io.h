/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-trunk
 * Copyright (C) Johannes Schmid 2008 <jhs@gnome.org>
 * 
 * anjuta-trunk is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta-trunk is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SOURCEVIEW_IO_H_
#define _SOURCEVIEW_IO_H_

#include <glib-object.h>
#include <gio/gio.h>
#include <libanjuta/anjuta-encodings.h>
#include "sourceview.h"

G_BEGIN_DECLS

#define SOURCEVIEW_TYPE_IO             (sourceview_io_get_type ())
#define SOURCEVIEW_IO(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SOURCEVIEW_TYPE_IO, SourceviewIO))
#define SOURCEVIEW_IO_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SOURCEVIEW_TYPE_IO, SourceviewIOClass))
#define SOURCEVIEW_IS_IO(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SOURCEVIEW_TYPE_IO))
#define SOURCEVIEW_IS_IO_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SOURCEVIEW_TYPE_IO))
#define SOURCEVIEW_IO_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SOURCEVIEW_TYPE_IO, SourceviewIOClass))

typedef struct _SourceviewIOClass SourceviewIOClass;
typedef struct _SourceviewIO SourceviewIO;

struct _SourceviewIOClass
{
	GObjectClass parent_class;

	/* Signals */
	void(* changed) (SourceviewIO *self);
	void(* save_finished) (SourceviewIO *self);
	void(* open_finished) (SourceviewIO *self);
	void(* open_failed) (SourceviewIO *self, GError* error);
	void(* save_failed) (SourceviewIO *self, GError* error);
	void(* deleted) (SourceviewIO *self);
};

struct _SourceviewIO
{
	GObject parent_instance;
	
	GFile* file;
	gchar* filename;
	Sourceview* sv;
	gchar* write_buffer;
	gchar* read_buffer;
	GCancellable* cancel;
	GFileMonitor* monitor;
	guint monitor_idle;
	gssize bytes_read;
	
	const AnjutaEncoding* last_encoding;
};

GType sourceview_io_get_type (void) G_GNUC_CONST;
void sourceview_io_save (SourceviewIO* sio);
void sourceview_io_save_as (SourceviewIO* sio, GFile* file);
void sourceview_io_open (SourceviewIO* sio, GFile* file);
void sourceview_io_cancel (SourceviewIO* sio);
GFile* sourceview_io_get_file (SourceviewIO* sio);
gchar* sourceview_io_get_filename (SourceviewIO* sio);
void sourceview_io_set_filename (SourceviewIO* sio, const gchar* filename);
gchar* sourceview_io_get_mime_type (SourceviewIO* sio);
gboolean sourceview_io_get_read_only (SourceviewIO* sio);
SourceviewIO* sourceview_io_new (Sourceview* sv);

G_END_DECLS

#endif /* _SOURCEVIEW_IO_H_ */
