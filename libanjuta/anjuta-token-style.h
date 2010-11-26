/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-token-style.h
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

#ifndef _ANJUTA_TOKEN_STYLE_H_
#define _ANJUTA_TOKEN_STYLE_H_

#include <glib.h>

#include "anjuta-token.h"

G_BEGIN_DECLS

typedef struct _AnjutaTokenStyle AnjutaTokenStyle;

AnjutaTokenStyle *anjuta_token_style_new (const gchar *start, const gchar *next, const gchar *eol, const gchar *last, guint max_width);
void anjuta_token_style_free (AnjutaTokenStyle *style);

void anjuta_token_style_update (AnjutaTokenStyle *style, AnjutaToken *list);
void anjuta_token_style_format (AnjutaTokenStyle *style, AnjutaToken *list);

AnjutaToken *anjuta_token_list_first (AnjutaToken *list);
AnjutaToken *anjuta_token_list_last (AnjutaToken *list);
AnjutaToken *anjuta_token_list_next (AnjutaToken *sibling);
AnjutaToken *anjuta_token_list_replace (AnjutaToken *sibling, AnjutaToken *baby);
AnjutaToken *anjuta_token_list_replace_nth (AnjutaToken *list, guint n, AnjutaToken *baby);
AnjutaToken *anjuta_token_list_insert_after (AnjutaToken *sibling, AnjutaToken *baby);
void anjuta_token_list_delete (AnjutaToken *sibling);

G_END_DECLS

#endif
