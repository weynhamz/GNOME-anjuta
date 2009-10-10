/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * ac-parser.y
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
#include "ac-scanner.h"
#include "ac-parser.h"

//#define YYDEBUG 1

#include "libanjuta/anjuta-debug.h"

//static void amp_ac_yyerror (YYLTYPE *loc, void *scanner, char const *s);

//amp_ac_yydebug = 1;

%}

/*%union {
	AnjutaToken *token;
}*/

%token  EOL '\n'

%token  SPACE ' '

%token  HASH '#'
%token  LEFT_PAREN     '('
%token  RIGHT_PAREN    ')'
%token  LEFT_CURLY		'{'
%token  RIGHT_CURLY    '}'
%token  LEFT_BRACE     '['
%token  RIGHT_BRACE    ']'
%token  EQUAL             '='
%token  COMMA             ','
%token  LOWER           '<'
%token  GREATER         '>'
    
%token  NAME
%token  VARIABLE
%token  MACRO
%token  OPERATOR
%token  WORD
%token  JUNK

%token  START_SPACE_LIST

%left   ARG
%left   EMPTY

/* M4 macros */

%token  DNL


/* Autoconf macros */

%token	AC_MACRO_WITH_ARG
%token	AC_MACRO_WITHOUT_ARG

%token	PKG_CHECK_MODULES
%token	OBSOLETE_AC_OUTPUT
%token	AC_OUTPUT
%token	AC_CONFIG_FILES
%token	AC_SUBST
%token  AC_INIT

/*%type pkg_check_modules obsolete_ac_output ac_output ac_config_files
%type dnl
%type ac_macro_with_arg ac_macro_without_arg
%type spaces
%type separator
%type arg_string arg arg_list arg_list_body shell_string_body raw_string_body

%type expression comment macro
%type arg_string_body arg_body expression_body

%type any_space*/

%defines

%pure_parser

%parse-param {AmpAcScanner* scanner}
%lex-param   {AmpAcScanner* scanner}

%name-prefix="amp_ac_yy"

%locations

%start input

%debug


%{
static void amp_ac_yyerror (YYLTYPE *loc, AmpAcScanner *scanner, char const *s);

%}


%%

input:
    START_SPACE_LIST space_list
    | file
    ;

file:
    /* empty */
	| file statement
	;
   
statement:
    line
    | macro
    ;

line:
    space_token
    | comment
    | shell_string
    | args_token
    | EQUAL
    | LOWER
    | GREATER
    | NAME
    | VARIABLE
    | WORD
    ;

macro:
    dnl
	| ac_macro_with_arg
	| ac_macro_without_arg
    | ac_init
	| pkg_check_modules 
	| obsolete_ac_output
	| ac_output
	| ac_config_files
	;

/* Space list
 *----------------------------------------------------------------------------*/

space_list:
    /* empty */
    | space_list_body
    | space_list_body spaces
    ;

space_list_body:
    item
    | spaces item {
        anjuta_token_set_type ($1, ANJUTA_TOKEN_NEXT);
    }
    | space_list_body spaces item {
        anjuta_token_set_type ($2, ANJUTA_TOKEN_NEXT);
    }
    ;

item:
    name
    | operator
    ;

operator:
    OPERATOR {
        anjuta_token_set_type ($1, ANJUTA_TOKEN_OPERATOR);
    }
    ;

name:
    not_operator_token
    | name  word_token {
        anjuta_token_group ($1, $2);
    }
    ;

junks:
    JUNK
    | junks JUNK {
        anjuta_token_group ($1, $2);
    }
    ;

/* Macros
 *----------------------------------------------------------------------------*/

dnl:
    DNL  not_eol_list  EOL {
        anjuta_token_set_type ($1, ANJUTA_TOKEN_COMMENT);
        anjuta_token_group ($1, $3);
    }
    ;
    

pkg_check_modules:
    PKG_CHECK_MODULES arg_list {
        anjuta_token_set_type ($1, AC_TOKEN_PKG_CHECK_MODULES);
        $$ = anjuta_token_group ($1, $2);
    }
	;

ac_macro_without_arg:
	AC_MACRO_WITHOUT_ARG
	;

optional_arg:
    /* empty */     %prec EMPTY
    | COMMA NAME %prec ARG
    ;

ac_macro_with_arg:
	AC_MACRO_WITH_ARG optional_arg RIGHT_PAREN {
        anjuta_token_group ($1, $1);
    }
	;

ac_init:
    AC_INIT arg_list {
        anjuta_token_set_type ($1, AC_TOKEN_AC_INIT);
        $$ = anjuta_token_group ($1, $2);
    }

ac_output:
	AC_OUTPUT {
        anjuta_token_set_type ($1, AC_TOKEN_AC_OUTPUT);
    }
	;

obsolete_ac_output:
    OBSOLETE_AC_OUTPUT  arg_list {
        anjuta_token_set_type ($1, AC_TOKEN_OBSOLETE_AC_OUTPUT);
        $$ = anjuta_token_group ($1, $2);
    }
	;
	
ac_config_files:
    AC_CONFIG_FILES  arg_list {
        anjuta_token_set_type ($1, AC_TOKEN_AC_CONFIG_FILES);
        $$ = anjuta_token_group ($1, $2);
    }
	;

/* Lists
 *----------------------------------------------------------------------------*/

arg_list:
    arg_list_body  RIGHT_PAREN {
        anjuta_token_set_type ($2, ANJUTA_TOKEN_LAST);
        $$ = $2;
    }
    | spaces  arg_list_body  RIGHT_PAREN {
        anjuta_token_set_type ($1, ANJUTA_TOKEN_START);
        anjuta_token_set_type ($3, ANJUTA_TOKEN_LAST);
        $$ = $3;
    }
    ;

arg_list_body:
    arg
    | arg_list_body  separator  arg
    ;
    
comment:
    HASH not_eol_list EOL {
        anjuta_token_set_type ($1, ANJUTA_TOKEN_COMMENT);
        anjuta_token_group ($1, $3);
    }
    ;

not_eol_list:
    /* empty */
    | not_eol_list not_eol_token
    ;


shell_string:
    LEFT_BRACE shell_string_body RIGHT_BRACE {
        anjuta_token_set_type ($1, ANJUTA_TOKEN_STRING);
        anjuta_token_set_type ($3, ANJUTA_TOKEN_LAST);
        $$ = anjuta_token_group ($1, $3);
    }
    ;

shell_string_body:
    /* empty */
    | shell_string_body not_brace_token
    | shell_string_body shell_string
    ;

raw_string:
    LEFT_BRACE raw_string_body RIGHT_BRACE {
        anjuta_token_set_type ($1, ANJUTA_TOKEN_STRING);
        anjuta_token_set_type ($3, ANJUTA_TOKEN_LAST);
        $$ = anjuta_token_group ($1, $3);
    }
    ;

raw_string_body:
    /* empty */
    | raw_string_body not_brace_token
    | raw_string_body raw_string
    ;

arg_string:
    LEFT_BRACE arg_string_body RIGHT_BRACE  {
        $$ = anjuta_token_group_new (NAME, $1);
        anjuta_token_set_type ($1, ANJUTA_TOKEN_OPEN_QUOTE);
        anjuta_token_set_type ($3, ANJUTA_TOKEN_CLOSE_QUOTE);
        anjuta_token_group ($$, $3);
    }
    ;

arg_string_body:
    /* empty */
    | arg_string_body space_token
    | arg_string_body HASH
    | arg_string_body LEFT_PAREN
    | arg_string_body RIGHT_PAREN
    | arg_string_body COMMA
    | arg_string_body EQUAL
    | arg_string_body GREATER
    | arg_string_body LOWER
    | arg_string_body NAME
    | arg_string_body VARIABLE
    | arg_string_body WORD
    | arg_string_body macro
    | arg_string_body raw_string
    ;

/* Items
 *----------------------------------------------------------------------------*/

arg:
    /* empty */ {
        $$ = NULL;
    }
    | arg_part arg_body {
        $$ = anjuta_token_group_new (ANJUTA_TOKEN_ARGUMENT, $1);
        if ($2 != NULL) anjuta_token_group ($$, $2);
    }        
    ;

arg_body:
    /* empty */ {
        $$ = NULL;
    }
    | arg_body arg_part_or_space {
        $$ = $2;
    }
    ;

arg_part_or_space:
    space_token
    | arg_part
    ;

arg_part:
    arg_string
    | expression
    | macro
    | HASH
    | EQUAL
    | LOWER
    | GREATER
    | NAME
    | VARIABLE
    | WORD
    ;

separator:
    COMMA {
        $$ = anjuta_token_group_new (ANJUTA_TOKEN_NEXT, $1);
    }
    | COMMA spaces {
        $$ = anjuta_token_group_new (ANJUTA_TOKEN_NEXT, $1);
        anjuta_token_group ($$, $2);
    }
    ;

expression:
    LEFT_PAREN  expression_body  RIGHT_PAREN {
        anjuta_token_set_type ($1, ANJUTA_TOKEN_STRING);
        anjuta_token_set_type ($3, ANJUTA_TOKEN_LAST);
        $$ = anjuta_token_group ($1, $3);
    }
    ;

expression_body:
    /* empty */ {
        $$ = NULL;
    }
    | expression_body space_token
    | expression_body comment
    | expression_body COMMA
    | expression_body EQUAL
    | expression_body LOWER
    | expression_body GREATER
    | expression_body NAME
    | expression_body VARIABLE
    | expression_body WORD
    | expression_body macro
    | expression_body expression
    ;

spaces:
	space_token
	| spaces space_token {
        anjuta_token_group ($$, $2);
	}
	| spaces JUNK {
        anjuta_token_group ($$, $2);
	}
	;

/* Tokens
 *----------------------------------------------------------------------------*/

not_eol_token:
    SPACE
    | word_token    
    ;

not_brace_token:
    space_token
    | args_token
    | HASH
    | EQUAL
    | LOWER
    | GREATER
    | OPERATOR
    | NAME
    | VARIABLE
    | WORD
    | any_macro
    ;

space_token:
    SPACE
    | EOL
    ;

args_token:
    LEFT_PAREN
    | RIGHT_PAREN
    | COMMA
    ;

operator_token:
    EQUAL
    | LOWER
    | GREATER
    ;

not_operator_token:
    HASH
    | LEFT_BRACE
    | RIGHT_BRACE
    | LEFT_PAREN
    | RIGHT_PAREN
    | COMMA
    | NAME
    | VARIABLE
    | WORD
    | any_macro
    ;

word_token:
    HASH
    | LEFT_BRACE
    | RIGHT_BRACE
    | LEFT_PAREN
    | RIGHT_PAREN
    | COMMA
    | OPERATOR
    | EQUAL
    | LOWER
    | GREATER
    | NAME
    | VARIABLE
    | WORD
    | any_macro
    ;

any_macro:
    AC_CONFIG_FILES
	| AC_MACRO_WITH_ARG
	| AC_MACRO_WITHOUT_ARG
    | AC_OUTPUT
    | DNL
    | OBSOLETE_AC_OUTPUT
    | PKG_CHECK_MODULES
    ;

%%
    
static void
amp_ac_yyerror (YYLTYPE *loc, AmpAcScanner *scanner, char const *s)
{
    gchar *filename;

	g_message ("scanner %p", scanner);
    filename = amp_ac_scanner_get_filename ((AmpAcScanner *)scanner);
    if (filename == NULL) filename = "?";
    g_message ("%s (%d:%d-%d:%d) %s\n", filename, loc->first_line, loc->first_column, loc->last_line, loc->last_column, s);
}

/* Public functions
 *---------------------------------------------------------------------------*/

