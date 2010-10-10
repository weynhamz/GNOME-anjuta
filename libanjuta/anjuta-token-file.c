/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
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
	
	GFile* file;				/* Corresponding GFile */

	AnjutaToken *content;		/* Current file content */

	AnjutaToken *save;			/* List of memory block used */
};

struct _AnjutaTokenFileClass
{
	GObjectClass parent_class;
};

static GObjectClass *parent_class = NULL;

/* Helpers functions
 *---------------------------------------------------------------------------*/

/* Private functions
 *---------------------------------------------------------------------------*/

static AnjutaToken*
anjuta_token_file_find_position (AnjutaTokenFile *file, AnjutaToken *token)
{
	AnjutaToken *start;
	const gchar *pos;
	const gchar *ptr;
	const gchar *end;
	
	if (token == NULL) return NULL;

	if (anjuta_token_get_length (token) == 0)
	{
		AnjutaToken *last = anjuta_token_last (token);

		for (; (token != NULL) && (token != last); token = anjuta_token_next (token))
		{
			if (anjuta_token_get_length (token) != 0) break;
		}

		if (anjuta_token_get_length (token) == 0) return NULL;
	}

	pos = anjuta_token_get_string (token);
	for (start = file->content; start != NULL; start = anjuta_token_next (start))
	{
		guint len = anjuta_token_get_length (start);
			
		if (len)
		{
			ptr = anjuta_token_get_string (start);
			end = ptr + len;

			if ((pos >= ptr) && (pos < end)) break;
		}
	}
	if ((start != NULL) && (ptr != pos))
	{
		start = anjuta_token_split (start, pos - ptr);
		start = anjuta_token_next (start);
	}

	return start;
}

/* Public functions
 *---------------------------------------------------------------------------*/

AnjutaToken*
anjuta_token_file_load (AnjutaTokenFile *file, GError **error)
{
	gchar *content;
	gsize length;

	anjuta_token_file_unload (file);
	
	file->save = anjuta_token_new_static (ANJUTA_TOKEN_FILE,  NULL);
	file->content = anjuta_token_new_static (ANJUTA_TOKEN_FILE,  NULL);
	
	if (g_file_load_contents (file->file, NULL, &content, &length, NULL, error))
	{
		AnjutaToken *token;
			
		token =	anjuta_token_new_with_string (ANJUTA_TOKEN_FILE, content, length);
		anjuta_token_prepend_child (file->save, token);
		
		token =	anjuta_token_new_static (ANJUTA_TOKEN_FILE, content);
		anjuta_token_prepend_child (file->content, token);
	}
	
	return file->content;
}

gboolean
anjuta_token_file_unload (AnjutaTokenFile *file)
{
	if (file->content != NULL) anjuta_token_free (file->content);
	file->content = NULL;
	
	if (file->save != NULL) anjuta_token_free (file->save);
	file->save = NULL;

	return TRUE;
}

gboolean
anjuta_token_file_save (AnjutaTokenFile *file, GError **error)
{
	GFileOutputStream *stream;
	gboolean ok = TRUE;
	GError *err = NULL;
	AnjutaToken *token;
	
	stream = g_file_replace (file->file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, &err);
	if (stream == NULL)
	{
		if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
		{
			/* Perhaps parent directory is missing, try to create it */
			GFile *parent = g_file_get_parent (file->file);
			
			if (g_file_make_directory_with_parents (parent, NULL, NULL))
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

	for (token = file->content; token != NULL; token = anjuta_token_next (token))
	{
		if (!(anjuta_token_get_flags (token) & ANJUTA_TOKEN_REMOVED) && (anjuta_token_get_length (token)))
		{
			if (g_output_stream_write (G_OUTPUT_STREAM (stream), anjuta_token_get_string (token), anjuta_token_get_length (token) * sizeof (char), NULL, error) < 0)
			{
				ok = FALSE;
				break;
			}
		}
	}
		
	ok = ok && g_output_stream_close (G_OUTPUT_STREAM (stream), NULL, NULL);
	g_object_unref (stream);
	
	return ok;
}

void
anjuta_token_file_move (AnjutaTokenFile *file, GFile *new_file)
{
	if (file->file) g_object_unref (file->file);
	file->file = new_file != NULL ? g_object_ref (new_file) : NULL;
}

static AnjutaToken *
anjuta_token_file_remove_token (AnjutaTokenFile *file, AnjutaToken *token)
{
	AnjutaToken *last;
	AnjutaToken *next;

	if ((anjuta_token_get_length (token) > 0))
	{
		AnjutaToken *pos = anjuta_token_file_find_position (file, token);
		guint len = anjuta_token_get_length (token);

		if (pos != NULL)
		{
			while (len != 0)
			{
				guint flen = anjuta_token_get_length (pos);
				if (len < flen)
				{
					pos = anjuta_token_split (pos, len);
					flen = len;
				}
				pos = anjuta_token_free (pos);
				len -= flen;
			}
		}
	}
	
	last = anjuta_token_last (token); 
	if ((last != NULL) && (last != token))
	{
		next = anjuta_token_next (token);
		while (next != last)
		{
			next = anjuta_token_file_remove_token (file, next);
		}
		anjuta_token_file_remove_token (file, next);
	}		

	return anjuta_token_free (token);
}

/**
 * anjuta_token_file_update:
 * @file: a #AnjutaTokenFile derived class object.
 * @token: Token to update.
 * 
 * Update the file with all changed token starting from @token. The function can
 * return an error if the token is not in the file.
 * 
 * Return value: TRUE is the update is done without error.
 */
gboolean
anjuta_token_file_update (AnjutaTokenFile *file, AnjutaToken *token)
{
	AnjutaToken *prev;
	AnjutaToken *next;
	AnjutaToken *last;
	guint added;

	/* Find all token needing an update */
	
	/* Find following tokens */
	for (last = token; last != NULL; last = anjuta_token_next (last))
	{
		/* Get all tokens in group */
		last = anjuta_token_last (last);

		gint flags = anjuta_token_get_flags (last);
		if (!(flags & (ANJUTA_TOKEN_ADDED | ANJUTA_TOKEN_REMOVED))) break;
	}

	/* Find first modified token */
	for (;;)
	{
		gint flags = anjuta_token_get_flags (token);
		if (flags & (ANJUTA_TOKEN_ADDED | ANJUTA_TOKEN_REMOVED)) break;
		if (token == last)
		{
			/* No changed */
			return TRUE;
		}
		token = anjuta_token_next (token);
	}
	
	/* Find previous token */
	for (prev = token; prev != NULL; prev = anjuta_token_previous (prev))
	{
		gint flags = anjuta_token_get_flags (prev);
		if ((anjuta_token_get_length (prev) != 0) && !(flags & (ANJUTA_TOKEN_ADDED | ANJUTA_TOKEN_REMOVED))) break;
		token = prev;    
	}
	
	/* Delete removed token and compute length of added token */
	added = 0;
	for (next = token; (next != NULL) && (next != last);)
	{
		gint flags = anjuta_token_get_flags (next);
		
		if (flags & ANJUTA_TOKEN_REMOVED)
		{
			next = anjuta_token_file_remove_token (file, next);
			continue;
		}
		else if (flags & ANJUTA_TOKEN_ADDED)
		{
			added += anjuta_token_get_length (next);
		}
		next = anjuta_token_next (next);
	}

	/* Add new token */
	if (added != 0)
	{
		gchar *value;
		AnjutaToken *add;
		AnjutaToken *start = NULL;
		
		value = g_new (gchar, added);
		add = anjuta_token_prepend_child (file->save, anjuta_token_new_with_string (ANJUTA_TOKEN_NAME, value, added));
		
		/* Find token position */
		if (prev != NULL)
		{
			start = anjuta_token_file_find_position (file, prev);
			if (start != NULL) start = anjuta_token_split (start, anjuta_token_get_length (prev));
		}

		/* Insert token */
		add = anjuta_token_new_fragment (ANJUTA_TOKEN_NAME, value, added);
		if (start == NULL)
		{
			anjuta_token_prepend_child (file->content, add);
		}
		else
		{
			anjuta_token_insert_after (start, add);
		}

		for (next = token; (next != NULL) && (next != last); next = anjuta_token_next (next))
		{
			gint flags = anjuta_token_get_flags (next);


			if (flags & ANJUTA_TOKEN_ADDED)
			{
				guint len = anjuta_token_get_length (next);

				if (len > 0)
				{
					memcpy(value, anjuta_token_get_string (next), len);
					anjuta_token_set_string (next, value, len);
					value += len;
				}
				anjuta_token_clear_flags (next, ANJUTA_TOKEN_ADDED);
			}
		}
	}

	
	return TRUE;
}

gboolean
anjuta_token_file_get_token_location (AnjutaTokenFile *file, AnjutaTokenFileLocation *location, AnjutaToken *token)
{
	AnjutaTokenFileLocation loc = {NULL, 1, 1};
	AnjutaToken *pos;
	const gchar *target;

	anjuta_token_dump (token);
	do
	{
		target = anjuta_token_get_string (token); 
		if (target != NULL) break;

		/* token is a group or an empty token, looks for group members or
		 * following token */
		token = anjuta_token_next_after_children (token);
	} while (token != NULL);

	for (pos = file->content; pos != NULL; pos = anjuta_token_next (pos))
	{
		if (!(anjuta_token_get_flags (pos) & ANJUTA_TOKEN_REMOVED) && (anjuta_token_get_length (pos) > 0))
		{
			const gchar *ptr;
			const gchar *end;

			ptr = anjuta_token_get_string (pos);
			end = ptr + anjuta_token_get_length (pos);
			
			for (; ptr != end; ptr++)
			{
				if (*ptr == '\n')
				{
					/* New line */
					loc.line++;
					loc.column = 1;
				}
				else
				{
					loc.column++;
				}
					
				if (ptr == target)
				{
					if (location != NULL)
					{
						location->filename = file->file == NULL ? NULL : g_file_get_parse_name (file->file);
						location->line = loc.line;
						location->column = loc.column;
					}

					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

GFile*
anjuta_token_file_get_file (AnjutaTokenFile *file)
{
	return file->file;
}

AnjutaToken*
anjuta_token_file_get_content (AnjutaTokenFile *file)
{
	if (file->content == NULL)
	{
		anjuta_token_file_load (file, NULL);
	}
	
	return file->content;
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

	anjuta_token_file_unload (file);
	
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
	file->content = NULL;
	file->save = NULL;
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

	if (gfile) file->file =  g_object_ref (gfile);
	
	return file;
};

void
anjuta_token_file_free (AnjutaTokenFile *tfile)
{
	g_object_unref (tfile);
}
