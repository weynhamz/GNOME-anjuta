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
#include "amp-source.h"

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

	gboolean eof;		/* TRUE to emit EOF at the end */

	GHashTable *variables;
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
OPENM4QUOTE   \[
CLOSEM4QUOTE  \]
SINGLEQUOTE   \'
DOUBLEQUOTE		\"
OPENPARG   \(
CLOSEPARG  \)
COMMA       ,
EQUAL       =
LOWER       <
GREATER     >
NAME        [A-Za-z_][A-Za-z0-9_]*
VARIABLE    $[A-Za-z_0-9]+
OTHER       [^ \t\r\v\n#\[\]\"\'\\(),=><$_A-Za-z_]+

%x SPACE_LIST

%%

{WS}                    { RETURN (SPACE); }

\\\n                    { RETURN (SPACE); }

{NL}                    { RETURN (END_OF_LINE); }

{COMMENT}               { RETURN (HASH); }

{OPENM4QUOTE}             { RETURN (LEFT_BRACE); }

{CLOSEM4QUOTE}            { RETURN (RIGHT_BRACE); }

{OPENPARG}              { RETURN (LEFT_PAREN); }

{CLOSEPARG}             { RETURN (RIGHT_PAREN); }

{SINGLEQUOTE}			{ RETURN (SINGLE_QUOTE); }

{DOUBLEQUOTE}			{ RETURN (DOUBLE_QUOTE); }

{COMMA}                 { RETURN (COMMA); }

{EQUAL}                 { RETURN (EQUAL); }

{LOWER}                 { RETURN (LOWER); }

{GREATER}                 { RETURN (GREATER); }

dnl                     { RETURN (DNL); }

m4_include\(			{ RETURN (M4_INCLUDE); }

AC_ARG_ENABLE\(			{ RETURN (AC_ARG_ENABLE);}

AC_C_CONST				{ RETURN (AC_C_CONST);}

AC_CHECK_FUNCS\(		{ RETURN (AC_CHECK_FUNCS);}

AC_CHECK_HEADERS\(		{ RETURN (AC_CHECK_HEADERS);}

AC_CHECK_LIB\(			{ RETURN (AC_CHECK_LIB);}

AC_CHECK_PROG\(			{ RETURN (AC_CHECK_PROG);}

AC_CONFIG_FILES\(       { RETURN (AC_CONFIG_FILES); }

AC_CONFIG_HEADERS\(		{ RETURN (AC_CONFIG_HEADERS); }

AC_CONFIG_MACRO_DIR\(   { RETURN (AC_CONFIG_MACRO_DIR); }

AC_CONFIG_SRCDIR\(		{ RETURN (AC_CONFIG_SRCDIR); }

AC_EGREP_HEADER\(		{ RETURN (AC_EGREP_HEADER); }

AC_EXEEXT				{ RETURN (AC_EXEEXT); }

AC_HEADER_STDC			{ RETURN (AC_HEADER_STDC); }

AC_INIT\(               { RETURN (AC_INIT); }

AC_OBJEXT				{ RETURN (AC_OBJEXT); }

AC_OUTPUT\(             { RETURN (OBSOLETE_AC_OUTPUT); }

AC_OUTPUT               { RETURN (AC_OUTPUT); }

AC_PREREQ\(				{ RETURN (AC_PREREQ); }

AC_PROG_CC				{ RETURN (AC_PROG_CC);}

AC_PROG_CPP				{ RETURN (AC_PROG_CPP);}

AC_PROG_CXX				{ RETURN (AC_PROG_CXX);}

IT_PROG_INTLTOOL\(		{ RETURN (IT_PROG_INTLTOOL);}

AC_PROG_LEX				{ RETURN (AC_PROG_LEX);}

AC_PROG_LIBTOOL			{ RETURN (AC_PROG_LIBTOOL);}

AC_PROG_RANLIB			{ RETURN (AC_PROG_RANLIB);}

AC_PROG_YACC			{ RETURN (AC_PROG_YACC);}

AC_SUBST\(				{ RETURN (AC_SUBST);}

AC_TYPE_SIZE_T			{ RETURN (AC_TYPE_SIZE_T);}

AC_TYPE_OFF_T			{ RETURN (AC_TYPE_OFF_T);}

AM_INIT_AUTOMAKE\(		{ RETURN (AM_INIT_AUTOMAKE);}

AM_GLIB_GNU_GETTEXT\(   { RETURN (AM_GLIB_GNU_GETTEXT);}

AM_MAINTAINER_MODE		{ RETURN (AM_MAINTAINER_MODE);}

AM_PROG_LIBTOOL			{ RETURN (AM_PROG_LIBTOOL);}

LT_INIT\(				{ RETURN (LT_INIT);}

LT_INIT					{ RETURN (DEFAULT_LT_INIT);}

LT_PREREQ\(				{ RETURN (LT_PREREQ);}

PKG_CHECK_MODULES\(     { RETURN (PKG_CHECK_MODULES); }

PKG_PROG_PKG_CONFIG\(	{ RETURN (PKG_PROG_PKG_CONFIG);}

{NAME}                  { RETURN (NAME); }

{VARIABLE}              { RETURN (VARIABLE); }

{OTHER}|\$|\\           { RETURN (WORD); }

<<EOF>>                     { if (amp_ac_scanner_parse_end (yyextra) == YY_NULL) return YY_NULL; }

<SPACE_LIST>{

{WSNL}                    { RETURN (SPACE); }

\\\n                    { RETURN (SPACE); }

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

   	if (scanner->stream == NULL)
   	{
		yyterminate();
	}
   	else
   	{
		if (scanner->eof)
		{
			scanner->eof = FALSE;
			return END_OF_FILE;
		}

		yypop_buffer_state(scanner->scanner);
    	scanner->stream = anjuta_token_stream_pop (scanner->stream);

		if (scanner->stream == NULL)
		{
			yyterminate();
		}
		else
		{
			scanner->eof = anjuta_token_stream_get_current_file (scanner->stream) != NULL;

			/* Continue parsing the parent file */
			return 1;
		}
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

void
amp_ac_scanner_include (AmpAcScanner *scanner, AnjutaToken *list)
{
	GFile *file;
	AnjutaTokenFile *include;
	AnjutaToken *token;
	AnjutaToken *name;
	gchar *filename;
	AnjutaProjectNode *source;

	name = anjuta_token_first_item (list);  	/* m4_include macro */
	name = anjuta_token_next_item (name);		/* arguments list */
	name = anjuta_token_first_item (name);		/* filename */
	filename = g_strstrip (anjuta_token_evaluate (name));
	//g_message ("read include =%s=", filename);
	file = g_file_resolve_relative_path (anjuta_token_stream_get_current_directory (scanner->stream), filename);
	g_free (filename);
	source = amp_source_node_new (file, ANJUTA_PROJECT_PROJECT | ANJUTA_PROJECT_FRAME | ANJUTA_PROJECT_READ_ONLY);
	anjuta_project_node_append (ANJUTA_PROJECT_NODE (scanner->project), source);
	include = anjuta_token_file_new (file);
	token = anjuta_token_file_load (include, NULL);
	amp_ac_scanner_parse_token (scanner, list, token, 0, file, NULL);
	g_object_unref (file);
}

void
amp_ac_scanner_update_variable (AmpAcScanner *scanner, AnjutaToken *variable)
{
	AnjutaToken *arg;
	char *name = NULL;
	AnjutaToken *value = NULL;

	arg = anjuta_token_first_word (variable);
	name = g_strstrip (anjuta_token_evaluate (arg));
	value = anjuta_token_nth_word (variable, 2);

	g_hash_table_insert (scanner->variables, name, value);
}

void
amp_ac_scanner_subst_variable (AmpAcScanner *scanner, AnjutaToken *list)
{
	AnjutaToken *arg;
	char *name = NULL;
	AnjutaToken *value = NULL;

	arg = anjuta_token_first_word (list);
	name = g_strstrip (anjuta_token_evaluate (arg));
	value = anjuta_token_nth_word (list, 2);
	if (value == NULL)
	{
		value = g_hash_table_lookup (scanner->variables, name);
	}

	amp_project_add_subst_variable (scanner->project, name, value);
}


/* Public functions
 *---------------------------------------------------------------------------*/

AnjutaToken *
amp_ac_scanner_parse_token (AmpAcScanner *scanner, AnjutaToken *root, AnjutaToken *content, gint start, GFile *filename, GError **error)
{
    AnjutaToken *first;
    AnjutaTokenStream *stream;

    stream = anjuta_token_stream_push (scanner->stream, root, content, filename);
    first = anjuta_token_stream_get_root (stream);

	scanner->eof = filename != NULL;

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

	scanner->variables = g_hash_table_new_full (g_str_hash, g_str_equal, (GDestroyNotify)g_free, NULL);

	return scanner;
};

void
amp_ac_scanner_free (AmpAcScanner *scanner)
{
	g_return_if_fail (scanner != NULL);

	g_hash_table_remove_all (scanner->variables);
    yylex_destroy(scanner->scanner);

	g_free (scanner);
}
