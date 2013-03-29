/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-token-stream.h
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

#ifndef _ANJUTA_TOKEN_STREAM_H_
#define _ANJUTA_TOKEN_STREAM_H_

#include <glib.h>

#include <libanjuta/anjuta-token.h>

G_BEGIN_DECLS

typedef struct _AnjutaTokenStream AnjutaTokenStream;

AnjutaTokenStream *anjuta_token_stream_push (AnjutaTokenStream *parent, AnjutaToken *root, AnjutaToken *content, GFile *file);
AnjutaTokenStream *anjuta_token_stream_pop (AnjutaTokenStream *stream);
AnjutaTokenStream *anjuta_token_stream_get_parent (AnjutaTokenStream *stream);

AnjutaToken* anjuta_token_stream_get_root (AnjutaTokenStream *stream);
GFile* anjuta_token_stream_get_current_directory (AnjutaTokenStream *stream);
GFile* anjuta_token_stream_get_current_file (AnjutaTokenStream *stream);

AnjutaToken* anjuta_token_stream_tokenize (AnjutaTokenStream *stream, gint type, gsize length);
gint anjuta_token_stream_read (AnjutaTokenStream *stream, gchar *buffer, gsize max_size);

void anjuta_token_stream_append_token (AnjutaTokenStream *stream, AnjutaToken *token);


G_END_DECLS

#endif
