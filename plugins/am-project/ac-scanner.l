/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * ac-scanner.l
 * Copyright (C) Sébastien Granjoux 2009 <seb.sfo@free.fr>
 * 
 * main.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * main.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
%{

#include <stdlib.h>
#include <string.h>

#include "ac-scanner.h"
#include "ac-parser.h"

#include "libanjuta/anjuta-debug.h"

/* Eliminate warning */
#define YY_NO_UNPUT 1 

#define YY_INPUT(buf, result, max_size) result = amp_ac_scanner_input (yyextra, buf, max_size)

#define YY_USER_ACTION amp_update_location(yylloc, yytext, yyleng);

#define YY_EXTRA_TYPE  AmpAcScanner*

#define YY_DECL static int ac_yylex (YYSTYPE * yylval_param,YYLTYPE * yylloc_param ,yyscan_t yyscanner)

//#define YY_USER_INIT {yy_flex_debug = 1;}
 
static AnjutaToken* amp_ac_scanner_append_token (AmpAcScanner *scanner, gint token);
static void amp_update_location (YYLTYPE *loc, const gchar *text, gint length);
static gint amp_ac_scanner_input (AmpAcScanner *scanner, gchar *buffer, gsize max_size);

gboolean amp_ac_yyparse (AmpAcScanner *scanner);


#define RETURN(tok) *yylval = amp_ac_scanner_append_token (yyextra, tok); \
                    return tok
                    

struct _AmpAcScanner
{
    yyscan_t scanner;

    AnjutaTokenFile *file;
    gchar *filename;

    AnjutaToken *token;
    gint first;

    /* Beginning of current token */
    AnjutaToken *start;
    gsize begin;

    /* Position in buffer */
    AnjutaToken *next;
    gsize pos;
};

%}

%option reentrant stack noyywrap yylineno

%option prefix="amp_ac_yy"

/* Necessary because autotools wrapper always looks for a file named "lex.yy.c",
 * not "lex.amp_ac_yy.c"
%option outfile="lex.yy.c"*/			

%option bison-bridge bison-locations

%option never-interactive

%option batch

%option debug

WS          [ \t\r\v]+
NL          \n
WSNL        [ \t\v\r\n]+
COMMENT     #
OPENQUOTE   \[
CLOSEQUOTE  \]
OPENPARG   \(
CLOSEPARG  \)
COMMA       ,
EQUAL       =
LOWER       <
GREATER     >
NAME        [A-Za-z_][A-Za-z0-9_]*
VARIABLE    $[A-Za-z_0-9]+
OTHER       [^ \t\r\v\n#\[\]\\(),=><$_A-Za-z_]+

%x SPACE_LIST

%%

{WS}                    { RETURN (SPACE); }

\\\n                    { RETURN (SPACE); }

{NL}                    { RETURN (EOL); }

{COMMENT}               { RETURN (HASH); }

{OPENQUOTE}             { RETURN (LEFT_BRACE); }

{CLOSEQUOTE}            { RETURN (RIGHT_BRACE); }

{OPENPARG}              { RETURN (LEFT_PAREN); }

{CLOSEPARG}             { RETURN (RIGHT_PAREN); }

{COMMA}                 { RETURN (COMMA); }

{EQUAL}                 { RETURN (EQUAL); }

{LOWER}                 { RETURN (LOWER); }

{GREATER}                 { RETURN (GREATER); }

dnl                     { RETURN (DNL); }

PKG_CHECK_MODULES\(     { RETURN (PKG_CHECK_MODULES); }

AC_OUTPUT\(             { RETURN (OBSOLETE_AC_OUTPUT); }
 
AC_OUTPUT               { RETURN (AC_OUTPUT); }

AC_INIT\(               { RETURN (AC_INIT); }
 
AC_CONFIG_FILES\(       { RETURN (AC_CONFIG_FILES); }

{NAME}                  { RETURN (NAME); }

{VARIABLE}              { RETURN (VARIABLE); }

{OTHER}|\$|\\           { RETURN (WORD); }


<SPACE_LIST>{

{WSNL}                    { RETURN (SPACE); }

=|<|>|<=|>=             { RETURN (OPERATOR); }

{NAME}                    { RETURN (WORD); }

.                           {RETURN (WORD);}
}

%%

/* Private functions
 *---------------------------------------------------------------------------*/

static AnjutaToken*
amp_ac_scanner_append_token (AmpAcScanner *scanner, gint token)
{
    AnjutaToken *frag;

    if (scanner->file != NULL)
    {
        const gchar *start = anjuta_token_file_get_content (scanner->file, NULL);
            
        frag = anjuta_token_new_fragment (token, start + scanner->begin, yyget_leng (scanner->scanner));
        anjuta_token_file_append (scanner->file, frag);
        scanner->begin += yyget_leng (scanner->scanner);
    }
    else if (scanner->token != NULL)
    {
        gsize length = yyget_leng (scanner->scanner);
        AnjutaToken *end;

        for (end = scanner->start; end != NULL; end = anjuta_token_next_sibling (end))
        {
            if (anjuta_token_get_type (end) < ANJUTA_TOKEN_FIRST)
            {
                gint toklen = anjuta_token_get_length (end);
    
                if (toklen >= length)
                {
                    if (toklen > length)
                    {
                        AnjutaToken *start = end;

                        end = anjuta_token_split (end, length);
                        if (start == scanner->start)
                        {
                            scanner->start = end;
                        }
                    }

                    if (end == scanner->start)
                    {
                        /* Get whole token */
                        frag = end;
                        anjuta_token_set_type (frag, token);
                        scanner->start = anjuta_token_next_sibling (end);
                    }
                    else
                    {
                        frag = anjuta_token_new_fragment (token, NULL, 0);
                        anjuta_token_insert_before (scanner->start, frag);
                        scanner->start = anjuta_token_next_sibling (end);
                        anjuta_token_group (frag, end);
                    }
                    break;
                }
                else
                {
                    length -= toklen;
                }
            }
        }
    }
    
    return frag;
}

static void
amp_update_location (YYLTYPE *loc, const gchar *text, gint length)
{
	const gchar *ptr;
	const gchar *end = text + length;

    loc->first_line = loc->last_line;
    loc->first_column = loc->last_column + 1;

	for (ptr = text; ptr != end; ptr++)
	{
		if (*ptr == '\n')
		{
			loc->last_column = 0;
			loc->last_line++;
			length -= (ptr + 1 - text);
		}
	}

	loc->last_column += length;
}

static gint
amp_ac_scanner_input (AmpAcScanner *scanner, gchar *buffer, gsize max_size)
{
    gint result = YY_NULL;

    if (scanner->file != NULL)
    {
        gsize length = anjuta_token_file_get_length (scanner->file, NULL);
        if (scanner->pos < length)
        {
            const gchar *start = anjuta_token_file_get_content (scanner->file, NULL);

            length -= scanner->pos;

            if (length > max_size) length = max_size;
            memcpy (buffer, start + scanner->pos, length);
            scanner->pos += length;
            result = length;
        }
    }
    else if (scanner->token != NULL)
    {
        if (scanner->next)
        {
            gsize length = anjuta_token_get_length (scanner->next);

            if ((anjuta_token_get_type (scanner->next) >= ANJUTA_TOKEN_FIRST) || (scanner->pos >= length))
            {
                for (;;)
                {
                    scanner->next = anjuta_token_next_sibling (scanner->next);
                    if (scanner->next == NULL)
                    {
                        /* Last token */
                        break;
                    }
                    else if ((anjuta_token_get_length (scanner->next) != 0) && (anjuta_token_get_type (scanner->next) < ANJUTA_TOKEN_FIRST))
                    {
                        /* Find some data */
                        scanner->pos = 0;
                        length = anjuta_token_get_length (scanner->next);
                        break;  
                    }
                }
            }

            if (scanner->pos < length)
            {
                const gchar *start = anjuta_token_get_string (scanner->next);

                length -= scanner->pos;
            
                if (length > max_size) length = max_size;
                memcpy (buffer, start + scanner->pos, length);
                scanner->pos += length;
                result = length;
            }
        }

    }

    return result;
}

/* Public functions
 *---------------------------------------------------------------------------*/

int
amp_ac_yylex (YYSTYPE * yylval_param,YYLTYPE * yylloc_param ,AmpAcScanner *scanner)
{
    /* Return a special start token for bison */
    switch (scanner->first)
    {
    case AC_SPACE_LIST_STATE:
        scanner->first = 0;
        yy_push_state (SPACE_LIST, scanner->scanner);
        return START_SPACE_LIST;
    default:
        break;
    }

	/* Parse unknown data */
    return ac_yylex (yylval_param, yylloc_param, scanner->scanner);
}

gboolean
amp_ac_scanner_parse (AmpAcScanner *scanner, AnjutaTokenFile *file, GError **error)
{

    g_return_val_if_fail (file != NULL, FALSE);

	scanner->file = file;
    scanner->pos = 0;
    scanner->begin = 0;
    scanner->token = NULL;
    scanner->first = 0;
	
	return amp_ac_yyparse (scanner) == 0;
}

gboolean
amp_ac_scanner_parse_token (AmpAcScanner *scanner, AnjutaToken *token, gint start, GError **error)
{
    AnjutaToken *child;

    scanner->token = token;
    scanner->pos = 0;
    scanner->begin = 0;
    scanner->file = NULL;
    scanner->first = start;

    if (anjuta_token_get_length (token) != 0)
    {
        AnjutaToken *copy = anjuta_token_copy (token);
        
        anjuta_token_insert_child (token, copy);
        anjuta_token_clear (token);
    }

    /* Move all know data in a list */
    for (child = anjuta_token_next_child (token); child != NULL;)
    {
        if (anjuta_token_get_type (child) < ANJUTA_TOKEN_FIRST)
        {
            child = anjuta_token_ungroup (child);
            if (anjuta_token_get_length (child) == 0)
            {
                child = anjuta_token_delete (child);
                continue;
            }
        }
        child = anjuta_token_next_sibling (child);
    }

    scanner->next = anjuta_token_next_child (token);
    scanner->start = scanner->next;

	return amp_ac_yyparse (scanner) == 0;
}

const gchar*
amp_ac_scanner_get_filename (AmpAcScanner *scanner)
{
	g_free (scanner->filename);
    scanner->filename = NULL;
    if (scanner->file != NULL)
    {
        scanner->filename = g_file_get_path (anjuta_token_file_get_file (scanner->file));
    }
    else if (scanner->token != NULL)
    {
        scanner->filename = anjuta_token_value (scanner->token);
    }
	
	return scanner->filename;
}


/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

AmpAcScanner *
amp_ac_scanner_new (void)
{
	AmpAcScanner *scanner;

	scanner = g_new0 (AmpAcScanner, 1);

    yylex_init(&scanner->scanner);

    yyset_extra (scanner, scanner->scanner);

	return scanner;
};

void
amp_ac_scanner_free (AmpAcScanner *scanner)
{
	g_return_if_fail (scanner != NULL);

    //if (scanner->buffer != NULL) yy_delete_buffer (scanner->buffer, scanner->scanner);
    yylex_destroy(scanner->scanner);

	g_free (scanner->filename);
	scanner->filename = NULL;
	
	g_free (scanner);
}
