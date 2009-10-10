/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * mk-scanner.l
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
#include "mk-scanner.h"
#include "mk-parser.h"

#include "libanjuta/anjuta-debug.h"


/* Eliminate warning */
#define YY_NO_UNPUT 1 

#define YY_INPUT(buf,result,the_max_size) \
    result = 0;

#define YY_USER_ACTION mkp_update_location(yylloc, yytext, yyleng);
    
#define YY_EXTRA_TYPE  MkpScanner*

//#define YY_USER_INIT {yy_flex_debug = 1;}
 
static AnjutaToken* mkp_scanner_append_token (MkpScanner *scanner, gint token);
static gint mkp_scanner_last_token (MkpScanner *scanner);
static gint mkp_scanner_last_flags (MkpScanner *scanner);
static void mkp_scanner_update_line_width (MkpScanner *scanner, YYLTYPE *loc);
static void mkp_update_location (YYLTYPE *loc, const gchar *text, gint length);
static int mkp_parse (yyscan_t scanner);

void mkp_project_update_variable (MkpProject *project, AnjutaToken *variable);
void mkp_project_add_rule (MkpProject *project, AnjutaToken *rule);


#define RETURN(tok) *yylval = mkp_scanner_append_token (yyextra, tok); \
                    return tok

%}

%option reentrant stack noyywrap yylineno

%option prefix="mkp_mk_yy"

/* Necessary because autotools wrapper always looks for a file named "lex.yy.c",
 * not "lex.mkp_yy.c" 
%option outfile="lex.yy.c"*/

%option bison-bridge bison-locations

%option never-interactive

%option batch

%option debug

NAME          [^ \t\n\r:#=$"'`&@\\]*

%%

\n                          { RETURN (EOL); }


([ ]|\\\n)([ \t]|\\\n)*     { RETURN (SPACE); }

#                           { RETURN (HASH); }

\t                          { RETURN (TAB); }

\$\([^ \t\n\r:#=$)]+\)      { RETURN (VARIABLE); }

\$\{[^ \t\n\r:#=$}]+\}      { RETURN (VARIABLE); }

\$[^ \t\n\r\(\{]            { RETURN (VARIABLE); }

,                           { RETURN (COMMA); }

:                           { RETURN (COLON); }

::                          { RETURN (DOUBLE_COLON); }

;                           { RETURN (SEMI_COLON); }

\|                          { RETURN (ORDER); }

\=                          { RETURN (EQUAL); }

:=                          { RETURN (IMMEDIATE_EQUAL); }

\?=                         { RETURN (CONDITIONAL_EQUAL); }

\+=                         { RETURN (APPEND); }

\\[ ]                       { RETURN (CHARACTER); }

\\:                         { RETURN (CHARACTER); }

\\=                         { RETURN (CHARACTER); }

\\#                         { RETURN (CHARACTER); }

.PHONY                      { RETURN (_PHONY); }

.SUFFIXES                   { RETURN (_SUFFIXES); }

.DEFAULT                    { RETURN (_DEFAULT); }

.PRECIOUS                   { RETURN (_PRECIOUS); }

.INTERMEDIATE               { RETURN (_INTERMEDIATE); }
    
.SECONDARY                  { RETURN (_SECONDARY); }

.SECONDEXPANSION            { RETURN (_SECONDEXPANSION); }

.DELETE_ON_ERROR            { RETURN (_DELETE_ON_ERROR); }

.IGNORE                     { RETURN (_IGNORE); }

.LOW_RESOLUTION_TIME        { RETURN (_LOW_RESOLUTION_TIME); }

.SILENT                     { RETURN (_SILENT); }

.EXPORT_ALL_VARIABLES       { RETURN (_EXPORT_ALL_VARIABLES); }

.NOTPARALLEL                { RETURN (_NOTPARALLEL); }

{NAME}                      { RETURN (NAME);}

.                           { RETURN (CHARACTER); }

%%
     
struct _MkpScanner
{
    const gchar *pos;
    yyscan_t scanner;
    YY_BUFFER_STATE	 buffer;

    AnjutaTokenFile *file;
    gchar *filename;

    MkpProject *project;

	guint line_width;
};

/* Private functions
 *---------------------------------------------------------------------------*/

static AnjutaToken*
mkp_scanner_append_token (MkpScanner *scanner, gint token)
{
    AnjutaToken *frag;
            
    frag = anjuta_token_new_fragment (token, scanner->pos, yyget_leng (scanner->scanner));
	anjuta_token_file_append (scanner->file, frag);
    scanner->pos += yyget_leng (scanner->scanner);
	//anjuta_token_old_dump (frag);
    
    return frag;
}

static void
mkp_scanner_update_line_width (MkpScanner *scanner, YYLTYPE *loc)
{
	anjuta_token_file_update_line_width (scanner->file, loc->last_column);
}

void
mkp_scanner_update_variable (MkpScanner *scanner, AnjutaToken *variable)
{
    mkp_project_update_variable (scanner->project, variable);
}

void
mkp_scanner_add_rule (MkpScanner *scanner, AnjutaToken *rule)
{
    mkp_project_add_rule (scanner->project, rule);
}

static void
mkp_update_location (YYLTYPE *loc, const gchar *text, gint length)
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

/* Public functions
 *---------------------------------------------------------------------------*/

int
mkp_yylex (YYSTYPE * yylval_param,YYLTYPE * yylloc_param ,MkpScanner *scanner)
{
    return mkp_mk_yylex (yylval_param, yylloc_param, scanner->scanner);
}


gboolean
mkp_scanner_parse (MkpScanner *scanner, AnjutaTokenFile *file, GError **error)
{
    if (scanner->buffer != NULL) yy_delete_buffer (scanner->buffer, scanner->scanner);

	scanner->file = file;
	if (scanner->file== NULL) return FALSE;

    scanner->pos = anjuta_token_file_get_content (scanner->file, error);
	if (scanner->pos == NULL) return FALSE;

    scanner->buffer = yy_scan_string (scanner->pos, scanner->scanner);
	
	return mkp_yyparse (scanner) == 0;
}

guint
mkp_scanner_get_line_width (MkpScanner *scanner)
{
	return scanner->line_width;
}

const gchar*
mkp_scanner_get_filename (MkpScanner *scanner)
{
	g_free (scanner->filename);
    scanner->filename = NULL;
    if (scanner->file) scanner->filename = g_file_get_path (anjuta_token_file_get_file (scanner->file));
	
	return scanner->filename;
}


/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

MkpScanner *
mkp_scanner_new (MkpProject *project)
{
	MkpScanner *scanner;

	scanner = g_new0 (MkpScanner, 1);

    yylex_init(&scanner->scanner);
    yyset_extra (scanner, scanner->scanner);

    scanner->project = project;

	return scanner;
};

void
mkp_scanner_free (MkpScanner *scanner)
{
	g_return_if_fail (scanner != NULL);

    if (scanner->buffer != NULL) yy_delete_buffer (scanner->buffer, scanner->scanner);
    yylex_destroy(scanner->scanner);

	g_free (scanner->filename);
	scanner->filename = NULL;

	g_free (scanner);
}
