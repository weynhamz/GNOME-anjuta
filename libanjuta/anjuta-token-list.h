/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-token-list.h
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

#ifndef _ANJUTA_TOKEN_LIST_H_
#define _ANJUTA_TOKEN_LIST_H_

#include <glib.h>

#include "anjuta-token.h"

G_BEGIN_DECLS

typedef struct _AnjutaTokenStyle AnjutaTokenStyle;

enum AnjutaTokenSearchFlag
{
	ANJUTA_SEARCH_OVER	  = 0,
	ANJUTA_SEARCH_INTO		= 1 << 0,
	ANJUTA_SEARCH_ALL	   = 1 << 1,
	ANJUTA_SEARCH_BACKWARD = 1 << 2,
	ANJUTA_TOKEN_SEARCH_LAST = 1 << 3,
	ANJUTA_TOKEN_SEARCH_NOT	= 1 << 4,
};

AnjutaTokenStyle *anjuta_token_style_new (const gchar *start, const gchar *next, const gchar *eol, const gchar *last, guint max_width);
AnjutaTokenStyle *anjuta_token_style_new_from_base (AnjutaTokenStyle *base);
void anjuta_token_style_free (AnjutaTokenStyle *style);

void anjuta_token_style_update (AnjutaTokenStyle *style, AnjutaToken *list);
void anjuta_token_style_format (AnjutaTokenStyle *style, AnjutaToken *list);

AnjutaToken *anjuta_token_first_word (AnjutaToken *list);
AnjutaToken *anjuta_token_nth_word (AnjutaToken *list, guint n);
AnjutaToken *anjuta_token_next_word (AnjutaToken *item);

AnjutaToken *anjuta_token_replace_nth_word (AnjutaToken *list, guint n, AnjutaToken *item);
AnjutaToken *anjuta_token_insert_word_before (AnjutaToken *list, AnjutaToken *sibling, AnjutaToken *baby);
AnjutaToken *anjuta_token_insert_word_after (AnjutaToken *list, AnjutaToken *sibling, AnjutaToken *baby);
AnjutaToken *anjuta_token_remove_word (AnjutaToken *token);
AnjutaToken *anjuta_token_remove_list (AnjutaToken *name);

AnjutaToken *anjuta_token_insert_token_list (gboolean after, AnjutaToken *list,...);
AnjutaToken *anjuta_token_find_type (AnjutaToken *list, gint flags, AnjutaTokenType* types);
AnjutaToken *anjuta_token_skip_comment (AnjutaToken *list);

G_END_DECLS

#endif
