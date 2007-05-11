/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 *  anjuta-serializer.h
 *  Copyright (C) 2000 Naba Kumar  <naba@gnome.org>
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

#ifndef ANJUTA_SERIALIZER_H
#define ANJUTA_SERIALIZER_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_SERIALIZER         (anjuta_serializer_get_type ())
#define ANJUTA_SERIALIZER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_SERIALIZER, AnjutaSerializer))
#define ANJUTA_SERIALIZER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), ANJUTA_TYPE_SERIALIZER, AnjutaSerializerClass))
#define ANJUTA_IS_SERIALIZER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_SERIALIZER))
#define ANJUTA_IS_SERIALIZER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_SERIALIZER))
#define ANJUTA_SERIALIZER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ANJUTA_TYPE_SERIALIZER, AnjutaSerializerClass))

typedef enum
{
	ANJUTA_SERIALIZER_READ,
	ANJUTA_SERIALIZER_WRITE
} AnjutaSerializerMode;

typedef struct _AnjutaSerializer AnjutaSerializer;
typedef struct _AnjutaSerializerPrivate AnjutaSerializerPrivate;
typedef struct _AnjutaSerializerClass AnjutaSerializerClass;

struct _AnjutaSerializer {
	GObject parent;
	AnjutaSerializerPrivate *priv;
};

struct _AnjutaSerializerClass {
	GObjectClass parent_class;
};

GType anjuta_serializer_get_type (void);
AnjutaSerializer *anjuta_serializer_new (const gchar *filepath,
										 AnjutaSerializerMode mode);

gboolean
anjuta_serializer_write_int (AnjutaSerializer *serializer,
							 const gchar *name, gint value);
gboolean
anjuta_serializer_write_float (AnjutaSerializer *serializer,
							   const gchar *name, gfloat value);
gboolean
anjuta_serializer_write_string (AnjutaSerializer *serializer,
								const gchar *name, const gchar *value);
gboolean
anjuta_serializer_read_int (AnjutaSerializer *serializer,
							 const gchar *name, gint *value);
gboolean
anjuta_serializer_read_float (AnjutaSerializer *serializer,
							  const gchar *name, gfloat *value);
gboolean
anjuta_serializer_read_string (AnjutaSerializer *serializer,
							   const gchar *name, gchar **value,
							   gboolean replace);

G_END_DECLS

#endif /* ANJUTA_SERIALIZER_H */
