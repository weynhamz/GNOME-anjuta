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

#include "ac-scanner.h"
#include "ac-parser.h"

#include "libanjuta/anjuta-debug.h"
#include "libanjuta/anjuta-token-stream.h"

#include <stdlib.h>
#include <string.h>

#define YY_INPUT(buffer, result, max_size) result = anjuta_token_stream_read (yyextra->stream, buffer, max_size)

#define YY_EXTRA_TYPE  AmpAcScanner*

#define YY_DECL static int ac_yylex (YYSTYPE * yylval_param,YYLTYPE * yylloc_param ,yyscan_t yyscanner)

//#define YY_USER_INIT {yy_flex_debug = 1;}
 
static gint amp_ac_scanner_parse_end (AmpAcScanner *scanner);

#define RETURN(tok) *yylval = anjuta_token_stream_tokenize (yyextra->stream, tok, yyleng); \
                    return tok

struct _AmpAcScanner
{
    yyscan_t scanner;

    AnjutaTokenStream *stream;

    AmpProject *project;

    AnjutaToken *parsed;
};

             
%}

%option reentrant stack noyywrap yylineno

/* Remove some warnings */
%option nounput noinput noyy_pop_state noyy_top_state

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

<<EOF>>                     { if (amp_ac_scanner_parse_end (yyextra) == YY_NULL) return YY_NULL; }

<SPACE_LIST>{

{WSNL}                    { RETURN (SPACE); }

=|<|>|<=|>=             { RETURN (OPERATOR); }

{NAME}                    { RETURN (WORD); }

.                           {RETURN (WORD);}
}

%%

/* Private functions
 *---------------------------------------------------------------------------*/


static gint
amp_ac_scanner_parse_end (AmpAcScanner *scanner)
{
    scanner->parsed = anjuta_token_stream_get_root (scanner->stream);
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
amp_ac_yyerror (YYLTYPE *loc, AmpAcScanner *scanner, char const *s)
{
    AnjutaTokenFileLocation location;

    if (amp_project_get_token_location (scanner->project, &location, *loc))
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
amp_ac_scanner_load_module (AmpAcScanner *scanner, AnjutaToken *module)
{
    amp_project_load_module (scanner->project, module);
}

void
amp_ac_scanner_load_config (AmpAcScanner *scanner, AnjutaToken *list)
{
    amp_project_load_config (scanner->project, list);
}

void
amp_ac_scanner_load_properties (AmpAcScanner *scanner, AnjutaToken *macro, AnjutaToken *list)
{
    amp_project_load_properties (scanner->project, macro, list);
}


/* Public functions
 *---------------------------------------------------------------------------*/

AnjutaToken *
amp_ac_scanner_parse_token (AmpAcScanner *scanner, AnjutaToken *token, gint start, GError **error)
{
    AnjutaToken *first;
    AnjutaTokenStream *stream;

    stream = anjuta_token_stream_push (scanner->stream, token, NULL);
    first = anjuta_token_stream_get_root (stream);

    if (scanner->stream != NULL)
    {
        /* Parse an included file or a expanded variable */

        scanner->stream = stream;
        yypush_buffer_state(yy_create_buffer(NULL, YY_BUF_SIZE, scanner->scanner), scanner->scanner);
    }
    else
    {
        amp_ac_yypstate *ps;
        gint status;
        YYSTYPE yylval_param;
        YYLTYPE yylloc_param;

        scanner->stream = stream;
        ps = amp_ac_yypstate_new ();

        yylval_param = NULL;
        yylloc_param = NULL;
        switch (start)
        {
        case AC_SPACE_LIST_STATE:
            amp_ac_yypush_parse (ps, START_SPACE_LIST, &yylval_param, &yylloc_param, scanner);
            yy_push_state (SPACE_LIST, scanner->scanner);
            break;
        default:
            break;
        }

        do
        {
            gint yychar = ac_yylex (&yylval_param, &yylloc_param, scanner->scanner);
        
            yylloc_param = yylval_param;
            status = amp_ac_yypush_parse (ps, yychar, &yylval_param, &yylloc_param, scanner);

        } while (status == YYPUSH_MORE);
        amp_ac_yypstate_delete (ps);
    }

    return first;
}

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

AmpAcScanner *
amp_ac_scanner_new (AmpProject *project)
{
	AmpAcScanner *scanner;

	scanner = g_new0 (AmpAcScanner, 1);

    yylex_init(&scanner->scanner);
    yyset_extra (scanner, scanner->scanner);

    scanner->project = project;

	return scanner;
};

void
amp_ac_scanner_free (AmpAcScanner *scanner)
{
	g_return_if_fail (scanner != NULL);

    yylex_destroy(scanner->scanner);

	g_free (scanner);
}
