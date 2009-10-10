/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-token.h
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

#ifndef _ANJUTA_TOKEN_H_
#define _ANJUTA_TOKEN_H_

#include <gio/gio.h>
#include <glib.h>

G_BEGIN_DECLS

typedef enum
{
	ANJUTA_TOKEN_NONE 							= 0,
	ANJUTA_TOKEN_EOL								= '\n',
	ANJUTA_TOKEN_COMMA							=',',
	
	ANJUTA_TOKEN_TYPE 							= 0xFFFF,

	ANJUTA_TOKEN_FIRST								= 16384,	
	ANJUTA_TOKEN_FILE 								= 16384,
	ANJUTA_TOKEN_KEYWORD,
	ANJUTA_TOKEN_OPERATOR,
	ANJUTA_TOKEN_NAME,
	ANJUTA_TOKEN_VALUE,
	ANJUTA_TOKEN_MACRO,
	ANJUTA_TOKEN_VARIABLE,
	ANJUTA_TOKEN_DEFINITION,
	ANJUTA_TOKEN_STATEMENT,
	ANJUTA_TOKEN_NUMBER,
	ANJUTA_TOKEN_JUNK,
	ANJUTA_TOKEN_COMMENT,
	ANJUTA_TOKEN_OPEN_QUOTE,
	ANJUTA_TOKEN_CLOSE_QUOTE,
	ANJUTA_TOKEN_ESCAPE,
	ANJUTA_TOKEN_FUNCTION,
	ANJUTA_TOKEN_SPACE,
	ANJUTA_TOKEN_START,
	ANJUTA_TOKEN_NEXT,
	ANJUTA_TOKEN_LAST,
	ANJUTA_TOKEN_ARGUMENT,
	ANJUTA_TOKEN_ITEM,
	ANJUTA_TOKEN_STRING,
	ANJUTA_TOKEN_ERROR,
	ANJUTA_TOKEN_WORD,
	ANJUTA_TOKEN_LIST,
	ANJUTA_TOKEN_ANY,
	ANJUTA_TOKEN_USER,
		
	ANJUTA_TOKEN_FLAGS 							= 0xFFFF << 16,
	
	ANJUTA_TOKEN_PUBLIC_FLAGS 				= 0x00FF << 16,
	
	ANJUTA_TOKEN_IRRELEVANT 					= 1 << 16,
	ANJUTA_TOKEN_OPEN 							= 1 << 17,
	ANJUTA_TOKEN_CLOSE 							= 1 << 18,
	ANJUTA_TOKEN_SIGNIFICANT					= 1 << 20,

	ANJUTA_TOKEN_PRIVATE_FLAGS 			= 0x00FF << 24,
	
	ANJUTA_TOKEN_CASE_INSENSITIVE 		= 1 << 24,
	ANJUTA_TOKEN_STATIC 							= 1 << 25,
	ANJUTA_TOKEN_REMOVED						= 1 << 26,
	ANJUTA_TOKEN_ADDED							= 1 << 27
	
} AnjutaTokenType;

//typedef GNode AnjutaToken;
typedef struct _AnjutaToken AnjutaToken;

typedef struct _AnjutaTokenRange
{
	AnjutaToken *first;
	AnjutaToken *last;
} AnjutaTokenRange;

enum AnjutaTokenSearchFlag
{
	ANJUTA_SEARCH_OVER	  = 0,
	ANJUTA_SEARCH_INTO		= 1 << 0,
	ANJUTA_SEARCH_ALL	   = 1 << 1,
	ANJUTA_SEARCH_BACKWARD = 1 << 2
};

AnjutaToken *anjuta_token_new_string (AnjutaTokenType type, const gchar *value);
AnjutaToken *anjuta_token_new_static (AnjutaTokenType type, const gchar *value);
AnjutaToken *anjuta_token_new_fragment (gint type, const gchar *pos, gsize length);

void anjuta_token_free (AnjutaToken *token);

AnjutaToken *anjuta_token_merge (AnjutaToken *first, AnjutaToken *end);
AnjutaToken *anjuta_token_merge_previous (AnjutaToken *first, AnjutaToken *end);
AnjutaToken *anjuta_token_copy (const AnjutaToken *token);
AnjutaToken *anjuta_token_clear (AnjutaToken *token);
AnjutaToken *anjuta_token_delete (AnjutaToken *token);

AnjutaToken *anjuta_token_group_new (AnjutaTokenType type, AnjutaToken *first);
AnjutaToken *anjuta_token_group (AnjutaToken *parent, AnjutaToken *last);
AnjutaToken *anjuta_token_ungroup (AnjutaToken *parent);

AnjutaToken *anjuta_token_split (AnjutaToken *token, guint size);

AnjutaToken * anjuta_token_insert_child (AnjutaToken *parent, AnjutaToken *child);
AnjutaToken *anjuta_token_insert_after (AnjutaToken *token, AnjutaToken *sibling);
AnjutaToken *anjuta_token_insert_before (AnjutaToken *token, AnjutaToken *sibling);
gboolean anjuta_token_match (AnjutaToken *token, gint flags, AnjutaToken *sequence, AnjutaToken **end);

//AnjutaToken *anjuta_token_copy (AnjutaToken *token);
//AnjutaToken *anjuta_token_copy_include_range (AnjutaToken *token, AnjutaToken *end);
//AnjutaToken *anjuta_token_copy_exclude_range (AnjutaToken *token, AnjutaToken *end);
//void anjuta_token_foreach (AnjutaToken *token, GFunc func, gpointer user_data);
gboolean anjuta_token_remove (AnjutaToken *token);
//gboolean anjuta_token_free_range (AnjutaToken *token, AnjutaToken *end);
//GList *anjuta_token_split_list (AnjutaToken *token);

void anjuta_token_set_type (AnjutaToken *token, gint type);
void anjuta_token_set_flags (AnjutaToken *token, gint flags);
void anjuta_token_clear_flags (AnjutaToken *token, gint flags);

gchar *anjuta_token_evaluate_range (AnjutaToken *start, AnjutaToken *end);
gchar *anjuta_token_evaluate (AnjutaToken *token);
gchar *anjuta_token_value (AnjutaToken *token);

AnjutaToken *anjuta_token_next (AnjutaToken *token);
AnjutaToken *anjuta_token_next_after_children (AnjutaToken *token);
AnjutaToken *anjuta_token_next_sibling (AnjutaToken *token);
AnjutaToken *anjuta_token_next_child (AnjutaToken *token);
AnjutaToken *anjuta_token_previous (AnjutaToken *token);
AnjutaToken *anjuta_token_previous_sibling (AnjutaToken *token);
AnjutaToken *anjuta_token_last_child (AnjutaToken *token);
AnjutaToken *anjuta_token_parent (AnjutaToken *token);
gboolean anjuta_token_compare (AnjutaToken *tokena, AnjutaToken *tokenb);

AnjutaToken *anjuta_token_get_next_arg (AnjutaToken *arg, gchar ** value);


gint anjuta_token_get_type (AnjutaToken *token);
gint anjuta_token_get_flags (AnjutaToken *token);
gchar *anjuta_token_get_value (AnjutaToken *token);
gchar *anjuta_token_get_value_range (AnjutaToken *token, AnjutaToken *end);
const gchar *anjuta_token_get_string (AnjutaToken *token);
guint anjuta_token_get_length (AnjutaToken *token);

G_END_DECLS

#endif
