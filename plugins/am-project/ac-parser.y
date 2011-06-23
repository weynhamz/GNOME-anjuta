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

#include "ac-scanner.h"
#include "ac-parser.h"

#include <stdlib.h>

//#define YYDEBUG 1

#include "libanjuta/anjuta-debug.h"

/* Token location is found directly from token value, there is no need to
 * maintain a separate location variable */
#define YYLLOC_DEFAULT(Current, Rhs, N)	((Current) = YYRHSLOC(Rhs, (N) ? 1 : 0))

%}

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

%token  COMMENT         256
%token  NAME
%token  VARIABLE
%token  MACRO
%token  OPERATOR
%token  WORD
%token  JUNK

%token  START_SPACE_LIST

/* M4 macros */

%token  DNL


/* Autoconf macros */

%token	AC_MACRO_WITH_ARG
%token	AC_MACRO_WITHOUT_ARG

%token	AC_ARG_ENABLE
%token	AC_C_CONST
%token	AC_CHECK_FUNCS
%token	AC_CHECK_HEADERS
%token	AC_CHECK_LIB
%token	AC_CHECK_PROG
%token	AC_CONFIG_FILES
%token	AC_CONFIG_HEADERS
%token	AC_CONFIG_MACRO_DIR
%token	AC_CONFIG_SRCDIR
%token	AC_EGREP_HEADER
%token	AC_EXEEXT
%token	AC_HEADER_STDC
%token  AC_INIT
%token	AC_OBJEXT
%token	AC_OUTPUT
%token	OBSOLETE_AC_OUTPUT
%token	AC_PREREQ
%token	AC_PROG_CC
%token	AC_PROG_CPP
%token	AC_PROG_CXX
%token	IT_PROG_INTLTOOL
%token	AC_PROG_LEX
%token	AC_PROG_RANLIB
%token	AC_PROG_YACC
%token	AC_SUBST
%token	AC_TYPE_SIZE_T
%token	AC_TYPE_OFF_T
%token	AM_INIT_AUTOMAKE
%token	AM_GLIB_GNU_GETTEXT
%token	AM_MAINTAINER_MODE
%token	AM_PROG_LIBTOOL
%token	LT_INIT
%token	LT_PREREQ
%token	PKG_CHECK_MODULES
%token	PKG_PROG_PKG_CONFIG	



%defines

%define api.pure
%define api.push_pull "push"

%parse-param {AmpAcScanner* scanner}
%lex-param   {AmpAcScanner* scanner}

%name-prefix="amp_ac_yy"

%locations

%start input

%debug

%{

static gint
amp_ac_autoconf_macro (AnjutaToken *token)
{
    switch (anjuta_token_get_type (token))
    {
	case AC_ARG_ENABLE:			return AC_TOKEN_AC_ARG_ENABLE;
	case AC_C_CONST:			return AC_TOKEN_AC_C_CONST;
	case AC_CHECK_FUNCS:		return AC_TOKEN_AC_CHECK_FUNCS;
	case AC_CHECK_HEADERS:		return AC_TOKEN_AC_CHECK_HEADERS;
	case AC_CHECK_LIB:			return AC_TOKEN_AC_CHECK_LIB;
	case AC_CHECK_PROG:			return AC_TOKEN_AC_CHECK_PROG;
	case AC_CONFIG_FILES:		return AC_TOKEN_AC_CONFIG_FILES;
	case AC_CONFIG_HEADERS:		return AC_TOKEN_AC_CONFIG_HEADERS;
	case AC_CONFIG_MACRO_DIR:	return AC_TOKEN_AC_CONFIG_MACRO_DIR;
	case AC_CONFIG_SRCDIR:		return AC_TOKEN_AC_CONFIG_SRCDIR;
	case AC_EGREP_HEADER:		return AC_TOKEN_AC_EGREP_HEADER;
	case AC_EXEEXT:				return AC_TOKEN_AC_EXEEXT;
	case AC_HEADER_STDC:		return AC_TOKEN_AC_HEADER_STDC;
	case AC_INIT:				return AC_TOKEN_AC_INIT;
	case AC_OBJEXT:				return AC_TOKEN_AC_OBJEXT;
	case AC_OUTPUT:				return AC_TOKEN_AC_OUTPUT;
	case OBSOLETE_AC_OUTPUT:	return AC_TOKEN_OBSOLETE_AC_OUTPUT;
	case AC_PREREQ:				return AC_TOKEN_AC_PREREQ;
	case AC_PROG_CC:			return AC_TOKEN_AC_PROG_CC;
	case AC_PROG_CPP:			return AC_TOKEN_AC_PROG_CPP;
	case AC_PROG_CXX:			return AC_TOKEN_AC_PROG_CXX;
	case IT_PROG_INTLTOOL:		return AC_TOKEN_IT_PROG_INTLTOOL;
	case AC_PROG_LEX:			return AC_TOKEN_AC_PROG_LEX;
	case AC_PROG_RANLIB:		return AC_TOKEN_AC_PROG_RANLIB;
	case AC_PROG_YACC:			return AC_TOKEN_AC_PROG_YACC;
	case AC_TYPE_SIZE_T:		return AC_TOKEN_AC_TYPE_SIZE_T;
	case AC_TYPE_OFF_T:			return AC_TOKEN_AC_TYPE_OFF_T;
	case AM_INIT_AUTOMAKE:		return AC_TOKEN_AM_INIT_AUTOMAKE;
	case AM_GLIB_GNU_GETTEXT:	return AC_TOKEN_AM_GLIB_GNU_GETTEXT;
	case AM_MAINTAINER_MODE:	return AC_TOKEN_AM_MAINTAINER_MODE;
	case AM_PROG_LIBTOOL:		return AC_TOKEN_AM_PROG_LIBTOOL;
	case LT_INIT:				return AC_TOKEN_LT_INIT;
	case LT_PREREQ:				return AC_TOKEN_LT_PREREQ;
	case PKG_CHECK_MODULES:		return AC_TOKEN_PKG_CHECK_MODULES;
	case PKG_PROG_PKG_CONFIG:	return AC_TOKEN_PKG_PROG_PKG_CONFIG;
    default: return anjuta_token_get_type (token);
    }
}

%}

%%

input:
    file
    | START_SPACE_LIST space_list
    ;

file:
    /* empty */
	| file  statement
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
    | space_list_body spaces {
		anjuta_token_set_type ($2, ANJUTA_TOKEN_LAST);
	}
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
    not_operator_token {
        $$ = anjuta_token_new_static (ANJUTA_TOKEN_NAME, NULL);
        anjuta_token_merge ($$, $1);
    }
    | name  word_token {
        anjuta_token_merge ($1, $2);
    }
    ;

/* Macros
 *----------------------------------------------------------------------------*/

dnl:
    DNL  not_eol_list  EOL {
		$$ = anjuta_token_new_static (ANJUTA_TOKEN_COMMENT, NULL);
		anjuta_token_merge ($$, $1);
		anjuta_token_merge ($$, $2);
		anjuta_token_merge ($$, $3);
	}
    ;
    

pkg_check_modules:
    PKG_CHECK_MODULES arg_list {
		$$ = anjuta_token_new_static (AC_TOKEN_PKG_CHECK_MODULES, NULL);
		anjuta_token_merge ($$, $1);
		anjuta_token_merge ($$, $2);
        amp_ac_scanner_load_module (scanner, $2);
    }
	;

optional_arg:
    /* empty */     %prec EMPTY
    | COMMA NAME %prec ARG
    ;

ac_macro_with_arg:
	ac_macro_with_arg_token arg_list {
		$$ = anjuta_token_new_static (amp_ac_autoconf_macro ($1), NULL);
		anjuta_token_merge ($$, $1);
		anjuta_token_merge ($$, $2);
	}
	;
	
ac_macro_without_arg:
	ac_macro_without_arg_token {
		anjuta_token_set_type ($1, amp_ac_autoconf_macro ($1));
	}

ac_init:
    AC_INIT arg_list {
		$$ = anjuta_token_new_static (AC_TOKEN_AC_INIT, NULL);
		anjuta_token_merge ($$, $1);
		anjuta_token_merge ($$, $2);
        amp_ac_scanner_load_properties (scanner, $1, $2);
    }

ac_output:
	AC_OUTPUT {
        anjuta_token_set_type ($1, AC_TOKEN_AC_OUTPUT);
    }
	;

obsolete_ac_output:
    OBSOLETE_AC_OUTPUT  arg_list {
		$$ = anjuta_token_new_static (AC_TOKEN_OBSOLETE_AC_OUTPUT, NULL);
		anjuta_token_merge ($$, $1);
		anjuta_token_merge ($$, $2);
        amp_ac_scanner_load_config (scanner, $2);
    }
	;
	
ac_config_files:
    AC_CONFIG_FILES  arg_list {
		$$ = anjuta_token_new_static (AC_TOKEN_AC_CONFIG_FILES, NULL);
		anjuta_token_merge ($$, $1);
		anjuta_token_merge ($$, $2);
        amp_ac_scanner_load_config (scanner, $2);
    }
	;

/* Lists
 *----------------------------------------------------------------------------*/

arg_list:
    arg_list_body  RIGHT_PAREN {
        $$ = anjuta_token_new_static (ANJUTA_TOKEN_LAST, NULL);
        anjuta_token_merge ($$, $2);
        anjuta_token_merge ($1, $$);
        $$ = $1;
    }
    | spaces  arg_list_body  RIGHT_PAREN {
        $$ = anjuta_token_new_static (ANJUTA_TOKEN_LAST, NULL);
        anjuta_token_merge ($$, $3);
		anjuta_token_merge ($2, $$);
		$$ = anjuta_token_new_static (ANJUTA_TOKEN_LIST, NULL);
        anjuta_token_set_type ($1, ANJUTA_TOKEN_START);
		anjuta_token_merge ($$, $1);
		anjuta_token_merge_children ($$, $2);
    }
    ;

arg_list_body:
    arg {
        $$ = anjuta_token_new_static (ANJUTA_TOKEN_LIST, NULL);
        anjuta_token_merge ($$, $1);
        //fprintf(stdout, "arg_list_body arg\n");
        //anjuta_token_dump ($1);
    }
    | arg_list_body  separator  arg {
        //fprintf(stdout, "arg_list_body body\n");
        //anjuta_token_dump ($1);
        //fprintf(stdout, "arg_list_body separator\n");
        //anjuta_token_dump ($2);
        //fprintf(stdout, "arg_list_body arg\n");
        //anjuta_token_dump ($3);
        anjuta_token_merge ($1, $2);
        anjuta_token_merge ($1, $3);
        //fprintf(stdout, "arg_list_body merge\n");
        //anjuta_token_dump ($1);
    }
    ;

comment:
    HASH not_eol_list EOL {
		$$ = anjuta_token_new_static (ANJUTA_TOKEN_COMMENT, NULL);
		anjuta_token_merge ($$, $1);
		anjuta_token_merge ($$, $2);
		anjuta_token_merge ($$, $3);
	}
    ;

not_eol_list:
    /* empty */ {
    	$$ = NULL;
    }
    | not_eol_list not_eol_token {
    	$$ = $2;
    }
    ;


shell_string:
    LEFT_BRACE shell_string_body RIGHT_BRACE {
        anjuta_token_set_type ($1, ANJUTA_TOKEN_OPEN_QUOTE);
        anjuta_token_set_type ($3, ANJUTA_TOKEN_CLOSE_QUOTE);
        $$ = anjuta_token_merge_previous ($2, $1);
        anjuta_token_merge ($2, $3);
    }
    ;

shell_string_body:
    /* empty */ {
        $$ = anjuta_token_new_static (ANJUTA_TOKEN_STRING, NULL);
    }
    | shell_string_body not_brace_token {
        anjuta_token_merge ($1, $2);
    }
    | shell_string_body shell_string {
        anjuta_token_merge ($1, $2);
    }
    ;

raw_string:
    LEFT_BRACE raw_string_body RIGHT_BRACE {
        anjuta_token_set_type ($1, ANJUTA_TOKEN_OPEN_QUOTE);
        anjuta_token_set_type ($3, ANJUTA_TOKEN_CLOSE_QUOTE);
        $$ = anjuta_token_merge_previous ($2, $1);
        anjuta_token_merge ($2, $3);
    }
    ;

raw_string_body:
    /* empty */ {
        $$ = anjuta_token_new_static (ANJUTA_TOKEN_STRING, NULL);
    }
    | raw_string_body not_brace_token {
        anjuta_token_merge ($1, $2);
    }
    | raw_string_body raw_string {
        anjuta_token_merge ($1, $2);
    }
    ;

arg_string:
    LEFT_BRACE arg_string_body RIGHT_BRACE  {
        anjuta_token_set_type ($1, ANJUTA_TOKEN_OPEN_QUOTE);
        anjuta_token_set_type ($3, ANJUTA_TOKEN_CLOSE_QUOTE);
        $$ = anjuta_token_merge_previous ($2, $1);
        anjuta_token_merge ($2, $3);
    }
    ;

arg_string_body:
    /* empty */ {
        $$ = anjuta_token_new_static (ANJUTA_TOKEN_STRING, NULL);
    }
    | arg_string_body space_token {
        anjuta_token_merge ($1, $2);
    }
    | arg_string_body HASH {
        anjuta_token_merge ($1, $2);
    }
    | arg_string_body LEFT_PAREN {
        anjuta_token_merge ($1, $2);
    }
    | arg_string_body RIGHT_PAREN {
        anjuta_token_merge ($1, $2);
    }
    | arg_string_body COMMA {
        anjuta_token_merge ($1, $2);
    }
    | arg_string_body EQUAL {
        anjuta_token_merge ($1, $2);
    }
    | arg_string_body GREATER {
        anjuta_token_merge ($1, $2);
    }
    | arg_string_body LOWER {
        anjuta_token_merge ($1, $2);
    }
    | arg_string_body NAME {
        anjuta_token_merge ($1, $2);
    }
    | arg_string_body VARIABLE {
        anjuta_token_merge ($1, $2);
    }
    | arg_string_body WORD {
        anjuta_token_merge ($1, $2);
    }
    | arg_string_body macro
    | arg_string_body raw_string {
        anjuta_token_merge ($1, $2);
    }
    ;

/* Items
 *----------------------------------------------------------------------------*/

arg:
    /* empty */ {
        $$ = anjuta_token_new_static (ANJUTA_TOKEN_ITEM, NULL);
    }
    | arg_part arg_body {
        //fprintf(stdout, "arg part\n");
        //anjuta_token_dump ($1);
        //fprintf(stdout, "arg body\n");
        //anjuta_token_dump ($2);
        anjuta_token_merge_children ($1, $2);
        //fprintf(stdout, "arg merge\n");
        //anjuta_token_dump ($1);
    }        
    ;

arg_body:
    /* empty */ {
        $$ = anjuta_token_new_static (ANJUTA_TOKEN_ITEM, NULL);
    }
    | arg_body arg_part_or_space {
        anjuta_token_merge_children ($1, $2);
    }
    ;

arg_part_or_space:
    space_token {
        $$ = anjuta_token_new_static (ANJUTA_TOKEN_ITEM, NULL);
        anjuta_token_merge ($$, $1);
    }
    | arg_part
    ;

arg_part:
    arg_string
    | expression
    | macro
    | arg_token {
        $$ = anjuta_token_new_static (ANJUTA_TOKEN_ITEM, NULL);
        anjuta_token_merge ($$, $1);
    }
    ;

arg_token:
    HASH
    | EQUAL
    | LOWER
    | GREATER
    | NAME
    | VARIABLE
    | WORD
    ;

separator:
    COMMA {
        $$ = anjuta_token_new_static (ANJUTA_TOKEN_NEXT, NULL);
        anjuta_token_merge ($$, $1);
    }
    | COMMA spaces {
        $$ = anjuta_token_new_static (ANJUTA_TOKEN_NEXT, NULL);
        //fprintf(stdout, "separator spaces\n");
        //anjuta_token_dump ($2);
        //fprintf(stdout, "separator comma\n");
        //anjuta_token_dump ($1);
        //fprintf(stdout, "separator next\n");
        //anjuta_token_dump ($$);
        anjuta_token_merge ($$, $1);
        anjuta_token_merge_children ($$, $2);
        //fprintf(stdout, "separator merge\n");
        //anjuta_token_dump ($$);
    }
    ;

expression:
    LEFT_PAREN  expression_body  RIGHT_PAREN {
        $$ = anjuta_token_merge_previous ($2, $1);
        anjuta_token_merge ($2, $3);
    }
    ;

expression_body:
    /* empty */  {
        $$ = anjuta_token_new_static (ANJUTA_TOKEN_STRING, NULL);
    }
    | expression_body space_token {
        anjuta_token_merge ($1, $2);
    }
    | expression_body comment
    | expression_body COMMA {
        anjuta_token_merge ($1, $2);
    }
    | expression_body EQUAL {
        anjuta_token_merge ($1, $2);
    }
    | expression_body LOWER {
        anjuta_token_merge ($1, $2);
    }
    | expression_body GREATER {
        anjuta_token_merge ($1, $2);
    }
    | expression_body NAME {
        anjuta_token_merge ($1, $2);
    }
    | expression_body VARIABLE {
        anjuta_token_merge ($1, $2);
    }
    | expression_body WORD {
        anjuta_token_merge ($1, $2);
    }
    | expression_body macro
    | expression_body expression {
        anjuta_token_merge ($1, $2);
    }
    | expression_body arg_string {
        anjuta_token_merge ($1, $2);
    }
    ;

spaces:
	space_token {
        $$ = anjuta_token_new_static (ANJUTA_TOKEN_SPACE, NULL);
        anjuta_token_merge ($$, $1);
    }
	| spaces space_token {
        anjuta_token_merge ($1, $2);
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
    | EOL {
		anjuta_token_set_type ($1, ANJUTA_TOKEN_EOL);
	}
    ;

args_token:
    LEFT_PAREN
    | RIGHT_PAREN
    | COMMA
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
    | AC_OUTPUT
    | DNL
    | OBSOLETE_AC_OUTPUT
    | PKG_CHECK_MODULES
    | AC_INIT
    | ac_macro_with_arg_token
    | ac_macro_without_arg_token
    ;

ac_macro_without_arg_token:
	AC_MACRO_WITHOUT_ARG
	| AC_C_CONST
	| AC_EXEEXT
	| AC_HEADER_STDC
	| AC_OBJEXT
	| AC_PROG_CC
	| AC_PROG_CPP
	| AC_PROG_CXX
	| AC_PROG_LEX
	| AC_PROG_RANLIB
	| AC_PROG_YACC
	| AC_TYPE_SIZE_T
	| AC_TYPE_OFF_T
	| AM_MAINTAINER_MODE
	| AM_PROG_LIBTOOL
	;

ac_macro_with_arg_token:
	AC_MACRO_WITH_ARG
	| AC_ARG_ENABLE
	| AC_CHECK_FUNCS
	| AC_CHECK_HEADERS
	| AC_CHECK_LIB
	| AC_CHECK_PROG
	| AC_CONFIG_HEADERS
	| AC_CONFIG_MACRO_DIR
	| AC_CONFIG_SRCDIR
	| AC_EGREP_HEADER
	| AC_PREREQ
	| IT_PROG_INTLTOOL
	| AC_SUBST
	| AM_INIT_AUTOMAKE
	| AM_GLIB_GNU_GETTEXT
	| LT_INIT
	| LT_PREREQ
	| PKG_PROG_PKG_CONFIG
	;

%%
