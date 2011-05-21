/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * am-scanner.l
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

#include "am-scanner.h"
#include "am-parser.h"
#include "amp-node.h"
#include "amp-target.h"

#include "libanjuta/anjuta-debug.h"
#include "libanjuta/anjuta-token-stream.h"

#include <stdlib.h>
#include <string.h>


#define YY_INPUT(buffer, result, max_size) result = anjuta_token_stream_read (yyextra->stream, buffer, max_size)

#define YY_EXTRA_TYPE  AmpAmScanner*

#define YY_DECL static int am_yylex (YYSTYPE * yylval_param,YYLTYPE * yylloc_param ,yyscan_t yyscanner)

//#define YY_USER_INIT {yy_flex_debug = 1;}

static int amp_am_scanner_parse_end (AmpAmScanner *scanner);

#define RETURN(tok) *yylval = anjuta_token_stream_tokenize (yyextra->stream, tok, yyleng); \
                    return tok

struct _AmpAmScanner
{
	yyscan_t scanner;

	AnjutaTokenStream *stream;

	AmpProject *project;
	AmpGroupNode *group;
	GHashTable *orphan_properties;
	GList *am_variables;
	gboolean eof;		/* TRUE to emit EOF at the end */
	gboolean expansion;		/* Expand variables */
};

%}

%option reentrant noyywrap yylineno

/* Remove some warnings */
%option nounput noinput noyy_pop_state noyy_top_state

%option prefix="amp_am_yy"

%option bison-bridge bison-locations

%option never-interactive

%option batch

%option debug

NAME          [^ \t\n\r:#=$"'`&@\\]*

%%

<INITIAL><<EOF>>					{ gint ret = amp_am_scanner_parse_end (yyextra); if (ret !=1) return ret; }

<INITIAL>\n 						{ RETURN (END_OF_LINE); }

<INITIAL>([ ]|\\\n)([ \t]|\\\n)*	{ RETURN (SPACE); }

<INITIAL>([ \t])*#.*\n 				{ RETURN (COMMENT); }

<INITIAL>\t 						{ RETURN (TAB); }

<INITIAL>@{NAME}@ 					{ RETURN (MACRO); }

<INITIAL>\$\([^ \t\n\r:#=$)]+\) 	{ RETURN (VARIABLE); }

<INITIAL>\$\{[^ \t\n\r:#=$}]+\} 	{ RETURN (VARIABLE); }

<INITIAL>\$[^ \t\n\r\(\{] 			{ RETURN (VARIABLE); }

<INITIAL>: 							{ RETURN (COLON); }

<INITIAL>:: 						{ RETURN (DOUBLE_COLON); }

<INITIAL>; 							{ RETURN (SEMI_COLON); }

<INITIAL>\| 						{ RETURN (ORDER); }

<INITIAL>\= 						{ RETURN (EQUAL); }

<INITIAL>:= 						{ RETURN (IMMEDIATE_EQUAL); }

<INITIAL>\?= 						{ RETURN (CONDITIONAL_EQUAL); }

<INITIAL>\+= 						{ RETURN (APPEND); }

<INITIAL>\\[ ] 						{ RETURN (CHARACTER); }

<INITIAL>\\: 						{ RETURN (CHARACTER); }

<INITIAL>\\= 						{ RETURN (CHARACTER); }

<INITIAL>\\# 						{ RETURN (CHARACTER); }

<INITIAL>include					{ RETURN (INCLUDE); }

<INITIAL>\-include					{ RETURN (INCLUDE); }

<INITIAL>SUBDIRS 					{ RETURN (SUBDIRS); }

<INITIAL>DIST_SUBDIRS 				{ RETURN (DIST_SUBDIRS); }

<INITIAL>{NAME}_DATA 				{ RETURN (_DATA); }

<INITIAL>{NAME}_HEADERS 			{ RETURN (_HEADERS); }

<INITIAL>{NAME}_LIBRARIES 			{ RETURN (_LIBRARIES); }

<INITIAL>{NAME}_LISP 				{ RETURN (_LISP); }

<INITIAL>{NAME}_LTLIBRARIES 		{ RETURN (_LTLIBRARIES); }

<INITIAL>{NAME}_MANS 				{ RETURN (_MANS); }

<INITIAL>{NAME}_PROGRAMS 			{ RETURN (_PROGRAMS); }

<INITIAL>{NAME}_PYTHON 				{ RETURN (_PYTHON); }

<INITIAL>{NAME}_JAVA 				{ RETURN (_JAVA); }

<INITIAL>{NAME}_SCRIPTS 			{ RETURN (_SCRIPTS); }

<INITIAL>{NAME}_SOURCES 			{ RETURN (_SOURCES); }

<INITIAL>{NAME}_TEXINFOS 			{ RETURN (_TEXINFOS); }

<INITIAL>{NAME}dir					{ RETURN (_DIR); }

<INITIAL>AM_LDFLAGS					{ RETURN (_LDFLAGS);}

<INITIAL>AM_CPPFLAGS				{ RETURN (_CPPFLAGS);}

<INITIAL>AM_CFLAGS					{ RETURN (_CFLAGS);}

<INITIAL>AM_CXXFLAGS				{ RETURN (_CXXFLAGS);}

<INITIAL>AM_JAVACFLAGS				{ RETURN (_JAVACFLAGS);}

<INITIAL>AM_VALAFLAGS				{ RETURN (_VALAFLAGS);}

<INITIAL>AM_FCFLAGS					{ RETURN (_FCFLAGS);}

<INITIAL>AM_OBJCFLAGS				{ RETURN (_OBJCFLAGS);}

<INITIAL>AM_LFLAGS					{ RETURN (_LFLAGS);}

<INITIAL>AM_YFLAGS					{ RETURN (_YFLAGS);}

<INITIAL>{NAME}_LDFLAGS				{ RETURN (TARGET_LDFLAGS);}

<INITIAL>{NAME}_CPPFLAGS			{ RETURN (TARGET_CPPFLAGS);}

<INITIAL>{NAME}_CFLAGS				{ RETURN (TARGET_CFLAGS);}

<INITIAL>{NAME}_CXXFLAGS			{ RETURN (TARGET_CXXFLAGS);}

<INITIAL>{NAME}_JAVACFLAGS			{ RETURN (TARGET_JAVACFLAGS);}

<INITIAL>{NAME}_VALAFLAGS			{ RETURN (TARGET_VALAFLAGS);}

<INITIAL>{NAME}_FCFLAGS				{ RETURN (TARGET_FCFLAGS);}

<INITIAL>{NAME}_OBJCFLAGS			{ RETURN (TARGET_OBJCFLAGS);}

<INITIAL>{NAME}_LFLAGS				{ RETURN (TARGET_LFLAGS);}

<INITIAL>{NAME}_YFLAGS				{ RETURN (TARGET_YFLAGS);}

<INITIAL>{NAME}_DEPENDENCIES		{ RETURN (TARGET_DEPENDENCIES);}

<INITIAL>{NAME}_LDADD				{ RETURN (TARGET_LDADD);}

<INITIAL>{NAME}_LIBADD				{ RETURN (TARGET_LIBADD);}

<INITIAL>{NAME} 					{ RETURN (NAME); }

<INITIAL>. 							{ RETURN (CHARACTER); }

%%

/* Private functions
 *---------------------------------------------------------------------------*/

static void
amp_am_scanner_reparse_token (AmpAmScanner *scanner, AnjutaToken *token, GFile *filename)
{
	AnjutaToken *root;
	AnjutaToken *list;
	
	if (token == NULL) return;
	
	token = anjuta_token_concat (token);
	
	yylex_init(&scanner->scanner);
	yyset_extra (scanner, scanner->scanner);
		
	root = anjuta_token_new_static (ANJUTA_TOKEN_FILE, NULL);
	list = amp_am_scanner_parse_token (scanner, root, token, filename, NULL);
	list = anjuta_token_delete_parent (list);
	anjuta_token_insert_before (token, list);
	anjuta_token_free (token);
}

static gint
amp_am_scanner_parse_end (AmpAmScanner *scanner)
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

static GList *
list_used_variable (AnjutaToken *token)
{
	AnjutaToken *last = anjuta_token_last (token);
	GList *list = NULL;

	for (;;)
	{
		if (anjuta_token_get_type (token) == ANJUTA_TOKEN_VARIABLE)
		{
			list = g_list_prepend (list, token);
		}
		if (token == last) break;
		token = anjuta_token_next (token);
	}
	list = g_list_reverse (list);
	
	return list;
}


/* Parser functions
 *---------------------------------------------------------------------------*/

void
amp_am_yyerror (YYLTYPE *loc, AmpAmScanner *scanner, char const *s)
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
amp_am_scanner_set_am_variable (AmpAmScanner *scanner, AnjutaToken *variable)
{
	if (!scanner->expansion)
	{
		/* Keep variable token to expand them at the end */
		scanner->am_variables = g_list_prepend (scanner->am_variables, variable);
	}
	else
	{
	    amp_project_set_am_variable (scanner->project, scanner->group, variable, scanner->orphan_properties);
	}
}

void
amp_am_scanner_include (AmpAmScanner *scanner, AnjutaToken *list)
{
	GFile *file;
	AnjutaTokenFile *include;
	AnjutaToken *token;
	AnjutaToken *name;
	gchar *filename;

	name = anjuta_token_first_item (list);
	name = anjuta_token_next_item (name);
	filename = g_strstrip (anjuta_token_evaluate (name));
	//g_message ("read include =%s=", filename);
	file = g_file_resolve_relative_path (anjuta_token_stream_get_current_directory (scanner->stream), filename);
	g_free (filename);
	include = anjuta_token_file_new (file);
	token = anjuta_token_file_load (include, NULL);
	amp_am_scanner_parse_token (scanner, list, token, file, NULL);
	g_object_unref (file);
}

void
amp_am_scanner_update_variable (AmpAmScanner *scanner, AnjutaToken *variable)
{
    amp_group_node_update_variable (scanner->group, variable);
}


void
amp_am_scanner_parse_variable (AmpAmScanner *scanner, AnjutaToken *variable)
{
	AnjutaToken *value;

	anjuta_token_set_type (variable, ANJUTA_TOKEN_VARIABLE);
	
	value = amp_group_node_get_variable_token (scanner->group, variable);
	if (value != NULL)
	{
		amp_am_scanner_parse_token (scanner, variable, value, NULL, NULL);
	}
}


/* Public functions
 *---------------------------------------------------------------------------*/

AnjutaToken *
amp_am_scanner_parse_token (AmpAmScanner *scanner, AnjutaToken *root, AnjutaToken *content, GFile *filename, GError **error)
{
    AnjutaToken *first;
    AnjutaTokenStream *stream;
	GList *variable;

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
        amp_am_yypstate *ps;
        gint status;

        scanner->stream = stream;
        ps = amp_am_yypstate_new ();
        do
        {
            YYSTYPE yylval_param;
            YYLTYPE yylloc_param;
            gint yychar = am_yylex (&yylval_param, &yylloc_param, scanner->scanner);

			yylloc_param = yylval_param;
            status = amp_am_yypush_parse (ps, yychar, &yylval_param, &yylloc_param, scanner);
 
        } while (status == YYPUSH_MORE);
        amp_am_yypstate_delete (ps);

		/* Evaluate autotools variable at the end */
		if (scanner->expansion == FALSE)
		{
			scanner->expansion = TRUE;
			scanner->am_variables = g_list_reverse (scanner->am_variables);
			for (variable = g_list_first (scanner->am_variables); variable != NULL; variable = g_list_next (variable))
			{
				AnjutaToken *token = (AnjutaToken *)variable->data;

				amp_am_scanner_reparse_token (scanner, token, filename);				
			}
		}
    }

    return first;
}

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

AmpAmScanner *
amp_am_scanner_new (AmpProject *project, AmpGroupNode *group)
{
	AmpAmScanner *scanner;

	scanner = g_new0 (AmpAmScanner, 1);

	scanner->project = project;
	scanner->group = group;
	scanner->eof = FALSE;

	/* Create hash table for sources list */
	scanner->orphan_properties = g_hash_table_new_full (g_str_hash, g_str_equal, (GDestroyNotify)g_free, (GDestroyNotify)amp_target_node_free);

	/* Create hash table for variable */
	scanner->am_variables = NULL;
	scanner->expansion = FALSE;

    yylex_init(&scanner->scanner);
    yyset_extra (scanner, scanner->scanner);

	return scanner;
}

void
amp_am_scanner_free (AmpAmScanner *scanner)
{
	g_return_if_fail (scanner != NULL);

    yylex_destroy(scanner->scanner);

	/* Free unused sources files */
	g_hash_table_destroy (scanner->orphan_properties);
	g_list_free (scanner->am_variables);

	g_free (scanner);
}
