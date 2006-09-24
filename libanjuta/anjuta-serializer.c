/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 *  anjuta-serializer.c
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
#include <stdio.h>
#include <anjuta-enum-types.h>
#include "anjuta-serializer.h"
 
static void anjuta_serializer_class_init(AnjutaSerializerClass *klass);
static void anjuta_serializer_init(AnjutaSerializer *sp);
static void anjuta_serializer_finalize(GObject *object);

struct _AnjutaSerializerPrivate {
	AnjutaSerializerMode mode;
	gchar *filepath;
	FILE *stream;
};

typedef struct _AnjutaSerializerSignal AnjutaSerializerSignal;
typedef enum _AnjutaSerializerSignalType AnjutaSerializerSignalType;

static GObjectClass *parent_class = NULL;

enum
{
	PROP_0,
	PROP_FILEPATH,
	PROP_MODE
};

GType
anjuta_serializer_get_type (void)
{
	static GType type = 0;

	if(type == 0) {
		static const GTypeInfo our_info = {
			sizeof (AnjutaSerializerClass),
			NULL,
			NULL,
			(GClassInitFunc)anjuta_serializer_class_init,
			NULL,
			NULL,
			sizeof (AnjutaSerializer),
			0,
			(GInstanceInitFunc)anjuta_serializer_init,
		};

		type = g_type_register_static(G_TYPE_OBJECT,
									  "AnjutaSerializer", &our_info, 0);
	}

	return type;
}

static void
anjuta_serializer_set_property (GObject * object,
								guint property_id,
								const GValue * value, GParamSpec * pspec)
{
	AnjutaSerializer *self = ANJUTA_SERIALIZER (object);
	g_return_if_fail (value != NULL);
	g_return_if_fail (pspec != NULL);
	
	switch (property_id)
	{
	case PROP_MODE:
		self->priv->mode = g_value_get_enum (value);
		break;
	case PROP_FILEPATH:
		g_free (self->priv->filepath);
		self->priv->filepath = g_value_dup_string (value);
		if (self->priv->stream)
			fclose (self->priv->stream);
		if (self->priv->mode == ANJUTA_SERIALIZER_READ)
			self->priv->stream = fopen (self->priv->filepath, "r");
		else
			self->priv->stream = fopen (self->priv->filepath, "w");
		if (self->priv->stream == NULL)
		{
			g_warning ("Could not open %s for serialization: %s",
					   self->priv->filepath, g_strerror (errno));
		}
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
anjuta_serializer_get_property (GObject * object,
								guint property_id,
								GValue * value, GParamSpec * pspec)
{
	AnjutaSerializer *self = ANJUTA_SERIALIZER (object);
	
	g_return_if_fail(value != NULL);
	g_return_if_fail(pspec != NULL);
	
	switch (property_id)
	{
	case PROP_MODE:
		g_value_set_enum (value, self->priv->mode);
		break;
	case PROP_FILEPATH:
		g_value_set_string (value, self->priv->filepath);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
anjuta_serializer_class_init (AnjutaSerializerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = anjuta_serializer_finalize;
	object_class->set_property = anjuta_serializer_set_property;
	object_class->get_property = anjuta_serializer_get_property;
	
	g_object_class_install_property (object_class,
			 PROP_FILEPATH,
			 g_param_spec_string ("filepath",
					 "File path",
					 "Used to store and retrieve the stream"
					 "translateable",
					 NULL,
					 G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
			 PROP_MODE,
			 g_param_spec_enum ("mode",
					 "Serialization mode",
					 "Used to decide read or write operation",
					 ANJUTA_TYPE_SERIALIZER_MODE,
					 ANJUTA_SERIALIZER_READ,
					 G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
anjuta_serializer_init (AnjutaSerializer *obj)
{
	obj->priv = g_new0 (AnjutaSerializerPrivate, 1);
}

static void
anjuta_serializer_finalize (GObject *object)
{
	AnjutaSerializer *cobj;
	cobj = ANJUTA_SERIALIZER(object);
	
	g_free (cobj->priv->filepath);
	if (cobj->priv->stream)
		fclose (cobj->priv->stream);
	g_free (cobj->priv);
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

AnjutaSerializer *
anjuta_serializer_new (const gchar *filepath, AnjutaSerializerMode mode)
{
	AnjutaSerializer *obj;
	
	obj = ANJUTA_SERIALIZER (g_object_new (ANJUTA_TYPE_SERIALIZER,
										   "mode", mode,
										   "filepath", filepath, NULL));
	if (obj->priv->stream == NULL)
	{
		g_object_unref (obj);
		return NULL;
	}
	return obj;
}

static gboolean
anjuta_serializer_write_buffer (AnjutaSerializer *serializer,
								const gchar *name, const gchar *value)
{
	gint length;
	gchar *buffer;
	
	g_return_val_if_fail (ANJUTA_IS_SERIALIZER (serializer), FALSE);
	g_return_val_if_fail (serializer->priv->stream != NULL, FALSE);
	g_return_val_if_fail (serializer->priv->mode == ANJUTA_SERIALIZER_WRITE,
						  FALSE);
	g_return_val_if_fail (name != NULL, FALSE);
	g_return_val_if_fail (value != NULL, FALSE);
	
	buffer = g_strconcat (name, ": ", value, NULL);
	length = strlen (buffer);
	if (fprintf (serializer->priv->stream, "%d\n", length) < 1)
	{
		g_free (buffer);
		return FALSE;
	}
	if (fwrite (buffer, length, 1, serializer->priv->stream) < 1)
	{
		g_free (buffer);
		return FALSE;
	}
	if (fprintf (serializer->priv->stream, "\n") < 0)
	{
		g_free (buffer);
		return FALSE;
	}
	g_free (buffer);
	return TRUE;
}

static gboolean
anjuta_serializer_read_buffer (AnjutaSerializer *serializer,
							   const gchar *name, gchar **value)
{
	gint length;
	gchar *buffer;
	
	g_return_val_if_fail (ANJUTA_IS_SERIALIZER (serializer), FALSE);
	g_return_val_if_fail (serializer->priv->stream != NULL, FALSE);
	g_return_val_if_fail (serializer->priv->mode == ANJUTA_SERIALIZER_READ,
						  FALSE);
	g_return_val_if_fail (name != NULL, FALSE);
	g_return_val_if_fail (value != NULL, FALSE);
	
	if (fscanf (serializer->priv->stream, "%d\n", &length) < 1)
		return FALSE;
	
	buffer = g_new0 (gchar, length + 1);
	if (fread (buffer, length, 1, serializer->priv->stream) < 1)
	{
		g_free (buffer);
		return FALSE;
	}
	if (fscanf (serializer->priv->stream, "\n") < 0)
	{
		g_free (buffer);
		return FALSE;
	}
	if (strncmp (buffer, name, strlen (name)) !=  0)
	{
		g_free (buffer);
		return FALSE;
	}		
	if (strncmp (buffer + strlen (name), ": ", 2) !=  0)
	{
		g_free (buffer);
		return FALSE;
	}
	strcpy (buffer, buffer + strlen (name) + 2);
	*value = buffer;
	
	return TRUE;
}

gboolean
anjuta_serializer_write_int (AnjutaSerializer *serializer,
							 const gchar *name, gint value)
{
	gchar buffer[64];
	snprintf (buffer, 64, "%d", value);
	return anjuta_serializer_write_buffer (serializer, name, buffer);
}

gboolean
anjuta_serializer_write_float (AnjutaSerializer *serializer,
							   const gchar *name, gfloat value)
{
	gchar buffer[64];
	snprintf (buffer, 64, "%f", value);
	return anjuta_serializer_write_buffer (serializer, name, buffer);
}

gboolean
anjuta_serializer_write_string (AnjutaSerializer *serializer,
								const gchar *name, const gchar *value)
{
	if (value)
		return anjuta_serializer_write_buffer (serializer, name, value);
	else
		return anjuta_serializer_write_buffer (serializer, name, "(null)");
}

gboolean
anjuta_serializer_read_int (AnjutaSerializer *serializer,
							 const gchar *name, gint *value)
{
	gchar *buffer;

	g_return_val_if_fail (value != NULL, FALSE);

	if (!anjuta_serializer_read_buffer (serializer, name, &buffer))
		return FALSE;
	*value = atoi (buffer);
	g_free (buffer);
	return TRUE;
}

gboolean
anjuta_serializer_read_float (AnjutaSerializer *serializer,
							  const gchar *name, gfloat *value)
{
	gchar *buffer;
	
	g_return_val_if_fail (value != NULL, FALSE);
	
	if (!anjuta_serializer_read_buffer (serializer, name, &buffer))
		return FALSE;
	*value = atof (buffer);
	g_free (buffer);
	return TRUE;
}

gboolean
anjuta_serializer_read_string (AnjutaSerializer *serializer,
							   const gchar *name, gchar **value,
							   gboolean replace)
{
	gchar *buffer;
	
	g_return_val_if_fail (value != NULL, FALSE);
	
	if (!anjuta_serializer_read_buffer (serializer, name, &buffer))
		return FALSE;
	if (replace)
		g_free (*value);
	if (strcmp (buffer, "(null)") == 0)
	{
		g_free (buffer);
		*value = NULL;
	}
	else
	{
		*value = buffer;
	}
	return TRUE;
}
