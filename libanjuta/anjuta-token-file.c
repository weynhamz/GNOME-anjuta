/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-token-file.c
 * Copyright (C) Sébastien Granjoux 2009 <seb.sfo@free.fr>
 * 
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "anjuta-token-file.h"

#include "anjuta-debug.h"

#include <glib-object.h>

#include <stdio.h>
#include <string.h>

/* Types declarations
 *---------------------------------------------------------------------------*/

struct _AnjutaTokenFile
{
	GObject parent;
	
	GFile* file;
	
	gsize length;
	gchar *content;
	
	AnjutaToken *first;
	AnjutaToken *last;

	guint line_width;
};

struct _AnjutaTokenFileClass
{
	GObjectClass parent_class;
};

static GObjectClass *parent_class = NULL;

/* Helpers functions
 *---------------------------------------------------------------------------*/

/* Create a directories, including parents if necessary. This function
 * exists in GLIB 2.18, but we need only 2.16 currently.
 * */

static gboolean
make_directory_with_parents (GFile *file,
							   GCancellable *cancellable,
							   GError **error)
{
	GError *path_error = NULL;
	GList *children = NULL;

	for (;;)
	{
		if (g_file_make_directory (file, cancellable, &path_error))
		{
			/* Making child directory succeed */
			if (children == NULL)
			{
				/* All directories have been created */
				return TRUE;
			}
			else
			{
				/* Get next child directory */
				g_object_unref (file);
				file = (GFile *)children->data;
				children = g_list_delete_link (children, children);
			}
		}
		else if (g_error_matches (path_error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
		{
			g_clear_error (&path_error);
			children = g_list_prepend (children, file);
			file = g_file_get_parent (file);
		}
		else
		{
			g_object_unref (file);
			g_list_foreach (children, (GFunc)g_object_unref, NULL);
			g_list_free (children);
			g_propagate_error (error, path_error);
			
			return FALSE;
		}
	}				
}

/* Public functions
 *---------------------------------------------------------------------------*/

const gchar *
anjuta_token_file_get_content (AnjutaTokenFile *file, GError **error)
{
	if (file->content == NULL)
	{
		gchar *content;
		gsize length;
	
		if (g_file_load_contents (file->file, NULL, &content, &length, NULL, error))
		{
			file->content = content;
			file->length = length;
		}
	}
	
	return file->content;
}

gsize
anjuta_token_file_get_length (AnjutaTokenFile *file, GError **error)
{
	anjuta_token_file_get_content (file, error);

	return file->length;
}

typedef struct _AnjutaTokenFileSaveData AnjutaTokenFileSaveData;

struct _AnjutaTokenFileSaveData
{
	GError **error;
	GFileOutputStream *stream;
	gboolean fail;
};

static gboolean
save_node (AnjutaToken *token, AnjutaTokenFileSaveData *data)
{
	if (!(anjuta_token_get_flags (token) & ANJUTA_TOKEN_REMOVED))
	{
		if (!(anjuta_token_get_flags (token) & ANJUTA_TOKEN_REMOVED) && (anjuta_token_get_length (token)))
		{
			if (g_output_stream_write (G_OUTPUT_STREAM (data->stream), anjuta_token_get_string (token), anjuta_token_get_length (token) * sizeof (char), NULL, data->error) < 0)
			{
				data->fail = TRUE;

				return TRUE;
			}
		}
	}
	
	return FALSE;
}

gboolean
anjuta_token_file_save (AnjutaTokenFile *file, GError **error)
{
	GFileOutputStream *stream;
	gboolean ok;
	GError *err = NULL;
	AnjutaTokenFileSaveData data;
	
	stream = g_file_replace (file->file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, &err);
	if (stream == NULL)
	{
		if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
		{
			/* Perhaps parent directory is missing, try to create it */
			GFile *parent = g_file_get_parent (file->file);
			
			if (make_directory_with_parents (parent, NULL, NULL))
			{
				g_object_unref (parent);
				g_clear_error (&err);
				stream = g_file_replace (file->file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, error);
				if (stream == NULL) return FALSE;
			}
			else
			{
				g_object_unref (parent);
				g_propagate_error (error, err);

				return FALSE;
			}
		}
		else
		{
			g_propagate_error (error, err);
			return FALSE;
		}
	}

	data.error = error;
	data.stream = stream;
	data.fail = FALSE;
	g_node_traverse ((GNode *)file->first, G_PRE_ORDER, G_TRAVERSE_ALL, -1, (GNodeTraverseFunc)save_node, &data);
	ok = g_output_stream_close (G_OUTPUT_STREAM (stream), NULL, NULL);
	g_object_unref (stream);
	
	return !data.fail;
}

void
anjuta_token_file_move (AnjutaTokenFile *file, GFile *new_file)
{
	if (file->file) g_object_unref (file->file);
	file->file = new_file != NULL ? g_object_ref (new_file) : NULL;
}

void
anjuta_token_file_append (AnjutaTokenFile *file, AnjutaToken *token)
{
	if (file->last == NULL)
	{
		file->first = token;
	}
	else if (file->last == file->first)
	{
		g_node_insert_after ((GNode *)file->first, NULL, (GNode *)token);
	}
	else
	{
		while (((GNode *)file->last)->parent != (GNode *)file->first)
		{
			file->last = (AnjutaToken *)((GNode *)file->last)->parent;
		}
		g_node_insert_after ((GNode *)file->first, (GNode *)file->last, (GNode *)token);
	}
	file->last = token;
}

void
anjuta_token_file_update_line_width (AnjutaTokenFile *file, guint width)
{
	if (width > file->line_width) file->line_width = width;
}

AnjutaToken*
anjuta_token_file_first (AnjutaTokenFile *file)
{
	return file->first;
}

AnjutaToken*
anjuta_token_file_last (AnjutaTokenFile *file)
{
	return file->last;
}

GFile*
anjuta_token_file_get_file (AnjutaTokenFile *file)
{
	return file->file;
}

guint
anjuta_token_file_get_line_width (AnjutaTokenFile *file)
{
	return file->line_width;
}

/* GObject functions
 *---------------------------------------------------------------------------*/

/* dispose is the first destruction step. It is used to unref object created
 * with instance_init in order to break reference counting cycles. This
 * function could be called several times. All function should still work
 * after this call. It has to called its parents.*/

static void
anjuta_token_file_dispose (GObject *object)
{
	AnjutaTokenFile *file = ANJUTA_TOKEN_FILE (object);

	anjuta_token_free (file->first);

	if (file->content) g_free (file->content);
	file->content = NULL;
	
	if (file->file) g_object_unref (file->file);
	file->file = NULL;

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

/* instance_init is the constructor. All functions should work after this
 * call. */

static void
anjuta_token_file_instance_init (AnjutaTokenFile *file)
{
	file->file = NULL;
}

/* class_init intialize the class itself not the instance */

static void
anjuta_token_file_class_init (AnjutaTokenFileClass * klass)
{
	GObjectClass *gobject_class;

	g_return_if_fail (klass != NULL);
	
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
	gobject_class = G_OBJECT_CLASS (klass);
	
	gobject_class->dispose = anjuta_token_file_dispose;
}

GType
anjuta_token_file_get_type (void)
{
	static GType type = 0;

	if (!type)
	{
		static const GTypeInfo type_info = 
		{
			sizeof (AnjutaTokenFileClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) anjuta_token_file_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,           /* class_data */
			sizeof (AnjutaTokenFile),
			0,              /* n_preallocs */
			(GInstanceInitFunc) anjuta_token_file_instance_init,
			NULL            /* value_table */
		};

		type = g_type_register_static (G_TYPE_OBJECT,
		                            "AnjutaTokenFile", &type_info, 0);
	}
	
	return type;
}


/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

AnjutaTokenFile *
anjuta_token_file_new (GFile *gfile)
{
	AnjutaTokenFile *file = g_object_new (ANJUTA_TOKEN_FILE_TYPE, NULL);

	if (gfile)
	{
		file->file =  g_object_ref (gfile);
		file->first = anjuta_token_new_static (ANJUTA_TOKEN_FILE, NULL);
		file->last = file->first;
	}
	
	return file;
};

void
anjuta_token_file_free (AnjutaTokenFile *tfile)
{
	g_object_unref (tfile);
}
