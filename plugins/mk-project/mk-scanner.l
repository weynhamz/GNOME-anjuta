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

#include "mk-scanner.h"
#include "mk-parser.h"
#include "mk-rule.h"

#include "libanjuta/anjuta-debug.h"
#include "libanjuta/anjuta-token-stream.h"

#include <stdlib.h>
#include <string.h>

#define YY_INPUT(buffer, result, max_size) result = anjuta_token_stream_read (yyextra->stream, buffer, max_size)

#define YY_EXTRA_TYPE  MkpScanner*

//#define YY_USER_INIT {yy_flex_debug = 1;}

static gint mkp_scanner_parse_end (MkpScanner *scanner);

#define RETURN(tok) *yylval = anjuta_token_stream_tokenize (yyextra->stream, tok, yyleng); \
                    return tok

struct _MkpScanner
{
    yyscan_t scanner;

    AnjutaTokenStream *stream;

    MkpProject *project;
};

%}

%option reentrant noyywrap yylineno

/* Remove some warnings */
%option nounput noinput

%option prefix="mkp_mk_yy"

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

<<EOF>>                     { if (mkp_scanner_parse_end (yyextra) == YY_NULL) return YY_NULL; }


%%

/* Private functions
 *---------------------------------------------------------------------------*/

static gint
mkp_scanner_parse_end (MkpScanner *scanner)
{
    yypop_buffer_state(scanner->scanner);
    scanner->stream = anjuta_token_stream_pop (scanner->stream);

    if (scanner->stream == NULL)
    {
        yyterminate();
    }
    else
    {
        return 1;
    }
}

/* Parser functions
 *---------------------------------------------------------------------------*/

void
mkp_yyerror (YYLTYPE *loc, MkpScanner *scanner, char const *s)
{
    AnjutaTokenFileLocation location;

    //g_message ("error at %d  error at * %d string \"%s\"", anjuta_token_get_type((AnjutaToken *)loc), anjuta_token_get_type(*loc), anjuta_token_get_string ((AnjutaToken *loc));
    if (mkp_project_get_token_location (scanner->project, &location, *loc))
    {
        g_message ("%s:%d.%d %s\n", location.filename, location.line, location.column, s);
        g_free (location.filename);
    }
    else
    {
        g_message ("%s \n", s);
    }
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

void
mkp_scanner_parse_variable (MkpScanner *scanner, AnjutaToken *variable)
{
    AnjutaToken *group;
    AnjutaToken *content;

    anjuta_token_set_type (variable, ANJUTA_TOKEN_VARIABLE);
    content = anjuta_token_new_static (ANJUTA_TOKEN_CONTENT, NULL);
    anjuta_token_stream_append_token (scanner->stream, content);

    group = mkp_project_get_variable_token (scanner->project, variable);
    //fprintf(stdout, "get variable %s is %p\n", anjuta_token_evaluate (variable), group);
    if (group != NULL)
    {
        //AnjutaToken *token;

        //anjuta_token_dump (group);
        //token = anjuta_token_group_into_token (group);
        //anjuta_token_set_type (token, ANJUTA_TOKEN_CONTENT);
        //anjuta_token_dump (token);
        //fprintf (stdout,"variable %s\n", anjuta_token_get_string (variable));
        //anjuta_token_dump (group);
        mkp_scanner_parse_token (scanner, group, NULL);
        //anjuta_token_free (token);
    }
}

/* Public functions
 *---------------------------------------------------------------------------*/

AnjutaToken *
mkp_scanner_parse_token (MkpScanner *scanner, AnjutaToken *token, GError **error)
{
    AnjutaToken *first;
    AnjutaTokenStream *stream;

    stream = anjuta_token_stream_push (scanner->stream, NULL, token, NULL);
    first = anjuta_token_stream_get_root (stream);

    if (scanner->stream != NULL)
    {
        /* Parse an included file or a expanded variable */

        scanner->stream = stream;
        yypush_buffer_state(yy_create_buffer(NULL, YY_BUF_SIZE, scanner->scanner), scanner->scanner);
    }
    else
    {
        mkp_yypstate *ps;
        gint status;

        scanner->stream = stream;
        ps = mkp_yypstate_new ();
        do
        {
	        YYSTYPE yylval_param;
    	    YYLTYPE yylloc_param;
            gint yychar = mkp_mk_yylex (&yylval_param, &yylloc_param, scanner->scanner);
        
            yylloc_param = yylval_param;
            status = mkp_yypush_parse (ps, yychar, &yylval_param, &yylloc_param, scanner);
        } while (status == YYPUSH_MORE);
        mkp_yypstate_delete (ps);
    }

    return first;
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

    yylex_destroy(scanner->scanner);

	g_free (scanner);
}
