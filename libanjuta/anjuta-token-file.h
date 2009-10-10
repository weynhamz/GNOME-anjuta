/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-token-file.h
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

#ifndef _ANJUTA_TOKEN_FILE_H_
#define _ANJUTA_TOKEN_FILE_H_

#include <gio/gio.h>
#include <glib.h>

#include <libanjuta/anjuta-token.h>

G_BEGIN_DECLS

#define ANJUTA_TOKEN_FILE_TYPE                     (anjuta_token_file_get_type ())
#define ANJUTA_TOKEN_FILE(obj)                      (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TOKEN_FILE_TYPE, AnjutaTokenFile))
#define ANJUTA_TOKEN_FILE_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TOKEN_FILE_TYPE, AnjutaTokenFileClass))
#define IS_ANJUTA_TOKEN_FILE(obj)                  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TOKEN_FILE_TYPE))
#define IS_ANJUTA_TOKEN_FILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TOKEN_FILE_TYPE))
#define GET_ANJUTA_TOKEN_FILE_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TOKEN_FILE_TYPE, AnjutaTokenFileClass))

typedef struct _AnjutaTokenFile AnjutaTokenFile;
typedef struct _AnjutaTokenFileClass AnjutaTokenFileClass;

GType anjuta_token_file_get_type (void);

AnjutaTokenFile *anjuta_token_file_new (GFile *file);
void anjuta_token_file_free (AnjutaTokenFile *file);

const gchar* anjuta_token_file_get_content (AnjutaTokenFile *file, GError **error);
gsize anjuta_token_file_get_length (AnjutaTokenFile *file, GError **error);
void anjuta_token_file_move (AnjutaTokenFile *file, GFile *new_file);
gboolean anjuta_token_file_save (AnjutaTokenFile *file, GError **error);

void anjuta_token_file_append (AnjutaTokenFile *file, AnjutaToken *token);
void anjuta_token_file_update_line_width (AnjutaTokenFile *file, guint width);

AnjutaToken* anjuta_token_file_first (AnjutaTokenFile *file);
AnjutaToken* anjuta_token_file_last (AnjutaTokenFile *file);
GFile *anjuta_token_file_get_file (AnjutaTokenFile *file);
guint anjuta_token_file_get_line_width (AnjutaTokenFile *file);

G_END_DECLS

#endif
