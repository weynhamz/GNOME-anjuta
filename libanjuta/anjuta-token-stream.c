/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-token-stream.c
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

#include "anjuta-token-stream.h"

#include "anjuta-debug.h"

#include <glib-object.h>

#include <stdio.h>
#include <string.h>

/**
 * SECTION:anjuta-token-stream
 * @title: Anjuta token stream
 * @short_description: Anjuta token stream
 * @see_also: 
 * @stability: Unstable
 * @include: libanjuta/anjuta-token-stream.h
 *  
 * A #AnjutaTokenStream object reads and writes a list of tokens. It uses two
 * list. The first list is assigned when the object is created. Each token is
 * read as characters discarding the separation between tokens. The second list
 * is written using the data of the first list, so no new memory is allocated,
 * in order to recreate a new list of tokens.
 *
 * This is used when the lexer needs several passes. At the beginning the file
 * is read as a single token containing the whole file content. The first pass
 * split this content into tokens. Additional passes are done on some parts of
 * the token list to get a more precise splitting.
 *
 * It is important to not allocate new memory and keep the same character
 * pointers in the additional passes because the token list does not own the
 * memory. The address of each character is used to find the position of the
 * changed data in the file.
 *
 * Several objects can be linked together to create a stack. It is used for
 * included file or variable expansion.
 */ 

/* Types declarations
 *---------------------------------------------------------------------------*/

struct _AnjutaTokenStream
{
	/* Input stream */
	AnjutaToken *first;
	AnjutaToken *last;

	/* Read position in input stream */
	AnjutaToken *next;
	gsize pos;

	/* Write position in input stream */
	AnjutaToken *start;
	gsize begin;

	/* Output stream */
	AnjutaToken *root;

	/* Parent stream */
	AnjutaTokenStream *parent;

	/* Current directory */
	GFile *current_directory;
};

/* Helpers functions
 *---------------------------------------------------------------------------*/

/* Private functions
 *---------------------------------------------------------------------------*/

/* Public functions
 *---------------------------------------------------------------------------*/

/**
 * anjuta_token_stream_append_token:
 * @stream: a #AnjutaTokenStream object.
 * @token: a #AnjutaToken object.
 *
 * Append an already existing token in the output stream. 
 */
void
anjuta_token_stream_append_token (AnjutaTokenStream *stream, AnjutaToken *token)
{
	anjuta_token_append_child (stream->root, token);
}

/**
 * anjuta_token_stream_tokenize:
 * @stream: a #AnjutaTokenStream object.
 * @type: a token type.
 * @length: the token length in character.
 *
 * Create a token of type from the last length characters previously read and
 * append it in the output stream. The characters are not copied in the output
 * stream, the new token uses the same characters.
 *
 * Return value: The created token.
 */
AnjutaToken* 
anjuta_token_stream_tokenize (AnjutaTokenStream *stream, gint type, gsize length)
{
    AnjutaToken *frag;
    AnjutaToken *end;

    frag = anjuta_token_new_fragment (type, NULL, 0);

    for (end = stream->start; end != NULL;)
    {
        if (anjuta_token_get_type (end) < ANJUTA_TOKEN_PARSED)
        {
            gint toklen = anjuta_token_get_length (end);
            AnjutaToken *copy = anjuta_token_cut (end, stream->begin, length);
    
            if (toklen >= (length + stream->begin))
            {

                if (end == stream->start)
                {
                    /* Get whole token */
                    anjuta_token_free (frag);
                    anjuta_token_set_type (copy, type);
                    frag = copy;
                }
                else
                {
                    /* Get several token */
                    anjuta_token_insert_after (frag, copy);
                    anjuta_token_merge (frag, copy);
                }

                if (toklen == (length + stream->begin))
                {
                    stream->start = anjuta_token_next (end);
                    stream->begin = 0;
                }
                else
                {
                    stream->start = end;
                    stream->begin += length;
                }
                break;
            }
            else
            {
                anjuta_token_insert_after (frag, copy);
                anjuta_token_merge (frag, copy);
                length -= toklen;
                end = anjuta_token_next (end);
                stream->begin = 0;
            }
        }
        else
        {
            end = anjuta_token_next (end);
            stream->begin = 0;
        }
    }
    
    anjuta_token_stream_append_token (stream, frag);

    return frag;
}

/**
 * anjuta_token_stream_read:
 * @stream: a #AnjutaTokenStream object.
 * @buffer: a character buffer to fill with token data.
 * @max_size: the size of the buffer.
 *
 * Read token from the input stream and write the content as a C string in the
 * buffer passed as argument.
 *
 * Return value: The number of characters written in the buffer.
 */
gint 
anjuta_token_stream_read (AnjutaTokenStream *stream, gchar *buffer, gsize max_size)
{
    gint result = 0;

    if (stream->next != NULL)
    {
        gsize length = anjuta_token_get_length (stream->next);

        if ((anjuta_token_get_type (stream->next) >= ANJUTA_TOKEN_PARSED) || (stream->pos >= length))
        {
            for (;;)
            {
                /* Last token */
                if (stream->next == stream->last) return 0;

                if (anjuta_token_get_type (stream->next) >= ANJUTA_TOKEN_PARSED)
                {
                    stream->next = anjuta_token_next (stream->next);
                }
                else
                {
                    stream->next = anjuta_token_next (stream->next);
                }

                if ((stream->next == NULL) || (anjuta_token_get_type (stream->next) == ANJUTA_TOKEN_EOV))
                {
                    /* Last token */
                    return 0;
                }
                else if ((anjuta_token_get_length (stream->next) != 0) && (anjuta_token_get_type (stream->next) < ANJUTA_TOKEN_PARSED))
                {
                    /* Find some data */
                    stream->pos = 0;
                    length = anjuta_token_get_length (stream->next);
                    break;  
                }
            }
        }

        if (stream->pos < length)
        {
            const gchar *start = anjuta_token_get_string (stream->next);

            length -= stream->pos;
            
            if (length > max_size) length = max_size;
            memcpy (buffer, start + stream->pos, length);
            stream->pos += length;
            result = length;
        }
    }

    return result;
}

/**
 * anjuta_token_stream_get_root:
 * @stream: a #AnjutaTokenStream object.
 *
 * Return the root token for the output stream.
 *
 * Return value: The output root token.
 */
AnjutaToken* 
anjuta_token_stream_get_root (AnjutaTokenStream *stream)
{
	g_return_val_if_fail (stream != NULL, NULL);
	
	return stream->root;
}

/**
 * anjuta_token_stream_get_current_directory:
 * @stream: a #AnjutaTokenStream object.
 *
 * Return the current directory.
 *
 * Return value: The current directory.
 */
GFile*
anjuta_token_stream_get_current_directory (AnjutaTokenStream *stream)
{
	g_return_val_if_fail (stream != NULL, NULL);
	
	return stream->current_directory;
}



/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

/**
 * anjuta_token_stream_push:
 * @parent: a parent #AnjutaTokenStream object or NULL.
 * @root: a token or NULL
 * @content: a token list.
 *
 * Create a new stream from a list of tokens. If a parent stream is passed,
 * the new stream keep a link on it, so we can return it when the new stream
 * will be destroyed.
 *
 * Return value: The newly created stream.
 */
AnjutaTokenStream *
anjuta_token_stream_push (AnjutaTokenStream *parent, AnjutaToken *root, AnjutaToken *content, GFile *filename)
{
	AnjutaTokenStream *child;

	child = g_new (AnjutaTokenStream, 1);
	child->first = content;
	child->pos = 0;
	child->begin = 0;
	child->parent = parent;

	child->next = anjuta_token_next (content);
	child->start = child->next;
	child->last = anjuta_token_last (content);
	if (child->last == content) child->last = NULL;

	child->root = root == NULL ? anjuta_token_new_static (ANJUTA_TOKEN_FILE, NULL) : root;
	if (filename == NULL)
	{
		child->current_directory = parent == NULL ? NULL : (parent->current_directory == NULL ? NULL : g_object_ref (parent->current_directory));
	}
	else
	{
		child->current_directory = g_file_get_parent (filename);
	}

	return child;
}

/**
 * anjuta_token_stream_pop:
 * @parent: a #AnjutaTokenStream object.
 *
 * Destroy the stream object and return the parent stream if it exists.
 *
 * Return value: The parent stream or NULL if there is no parent.
 */
AnjutaTokenStream *
anjuta_token_stream_pop (AnjutaTokenStream *stream)
{
	AnjutaTokenStream *parent;

	g_return_val_if_fail (stream != NULL, NULL);

	if (stream->current_directory) g_object_unref (stream->current_directory);
	parent = stream->parent;
	g_free (stream);

	return parent;
}
