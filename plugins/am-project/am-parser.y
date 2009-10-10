/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * am-parser.y
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

#include <am-scanner.h>

#include <stdlib.h>

#define YYDEBUG 1

%}

/* Defining an union allow to use 2 protocol blocks (enclosed by %{ %}) which
 * is useful when redefining YYSTYPE. */
%union {
	AnjutaToken *token;
	AnjutaTokenRange range;
}

%token	<token> EOL	'\n'
%token	<token> SPACE
%token	<token> TAB '\t'
%token	<token> MACRO
%token	<token> VARIABLE
%token	<token> COLON ':'
%token	<token> DOUBLE_COLON "::"
%token	<token> ORDER '|'
%token	<token> SEMI_COLON ';'
%token	<token> EQUAL '='
%token	<token> IMMEDIATE_EQUAL ":="
%token	<token> CONDITIONAL_EQUAL "?="
%token	<token> APPEND "+="
%token	<token> CHARACTER
%token	<token> NAME
%token	<token> AM_VARIABLE

%type <token> head_token target_token value_token name_token space_token rule_token equal_token token automake_token prerequisite_token
%type <token> am_variable
%type <token> value head space prerequisite target depend rule variable commands head_with_space
%type <token> value_list strip_value_list prerequisite_list target_list token_list target_list2
%type <token> optional_space space_list_value

%defines

%pure_parser

/* Necessary because autotools wrapper always looks for a file named "y.tab.c",
 * not "amp-scanner.c"
%output="y.tab.c"*/

%glr-parser

%parse-param {void* scanner}
%lex-param   {void* scanner}

%name-prefix="amp_am_yy"

%locations

%start file

%debug

%{

//amp_am_yydebug = 1;

static void amp_am_yyerror (YYLTYPE *loc, AmpAmScanner *scanner, char const *s);

%}


%%

file:
	optional_space statement
	| file EOL optional_space statement
	;
        
statement:
	/* empty */
	| line
	| am_variable
	;

line:
	name_token
	| line token
	;
		
variable:
	head_with_space equal_token optional_space value_list optional_space
	| head equal_token optional_space value_list optional_space
	;

rule:
	depend
	| depend SEMI_COLON commands
	| depend EOL TAB commands
	;
		
depend:
	target_list rule_token optional_space prerequisite_list optional_space ORDER optional_space prerequisite_list 
	;

commands:
	token_list
	| commands EOL TAB token_list
	;
		
am_variable:
	AM_VARIABLE space_list_value {
		$$ = anjuta_token_merge (
			anjuta_token_insert_before ($1,
					anjuta_token_new_static (ANJUTA_TOKEN_STATEMENT, NULL)),
			$2);
	}
	| AM_VARIABLE optional_space equal_token optional_space
	;
				
space_list_value: optional_space  equal_token   value_list  {
		$$ = $3;
	}
	;

value_list:
	optional_space strip_value_list optional_space {
		if ($1) anjuta_token_merge_previous ($2, $1);
		if ($3) anjuta_token_merge ($2, $3);
		$$ = $2;
	}
		
strip_value_list:
	value {
		$$ = anjuta_token_merge (
			anjuta_token_insert_before ($1,
					anjuta_token_new_static (ANJUTA_TOKEN_LIST, NULL)),
			$1);
	}
	| strip_value_list  space  value {
		anjuta_token_merge ($1, $3);
	}
	;

target_list:
	head
	| head_with_space
	| head_with_space target_list2 optional_space
	;

target_list2:
	target
	| target_list2  space  target {
		anjuta_token_merge ($1, $3);
	}
	;
		
token_list:
	token
	| token_list token
	;
		
prerequisite_list:
	prerequisite
	| prerequisite_list space prerequisite {
		anjuta_token_merge ($1, $3);
	}
	;

		

optional_space:
	/* empty */ {
		$$ = NULL;
	}
	| space
	;


head_with_space:
	head space
	;
		
head:
	head_token
	| head name_token {
		anjuta_token_merge ($1, $2);
	}
	;

target:
	head_token
	| target target_token {
		anjuta_token_merge ($1, $2);
	}
	;
		
value:
	value_token
	| value value_token {
		anjuta_token_merge ($1, $2);
	}
	;

prerequisite:
	prerequisite_token
	| prerequisite prerequisite_token {
		anjuta_token_merge ($1, $2);
	}
	;
		
space:
	space_token
	| space space_token	{
		anjuta_token_merge ($1, $2);
	}
	;

		
token:
	space_token
	| value_token
	;            
	
value_token:
	equal_token
	| rule_token
	| target_token
	;

prerequisite_token:
	equal_token
	| rule_token
	| name_token
	| automake_token
	| ORDER
	| SEMI_COLON
	;

target_token:
	head_token
	| automake_token
	;
		
		
space_token:
	SPACE
	| TAB
	;

equal_token:
	EQUAL
	| IMMEDIATE_EQUAL
	| CONDITIONAL_EQUAL
	| APPEND
	;

rule_token:
	COLON
	| DOUBLE_COLON
	;
		
head_token:
	MACRO
	| VARIABLE
	| NAME
	| CHARACTER
	| ORDER
	| SEMI_COLON
	;

name_token:
	MACRO
	| VARIABLE
	| NAME
	| CHARACTER
	;
		
automake_token:
	AM_VARIABLE
	;
		
%%

static void
amp_am_yyerror (YYLTYPE *loc, AmpAmScanner *scanner, char const *s)
{
    gchar *filename;

	g_message ("scanner %p", scanner);
    filename = amp_am_scanner_get_filename ((AmpAmScanner *)scanner);
    if (filename == NULL) filename = "?";
    g_message ("%s (%d:%d-%d:%d) %s\n", filename, loc->first_line, loc->first_column, loc->last_line, loc->last_column, s);
}
     
/*static void
amp_am_yyerror (YYLTYPE *loc, void *scanner, char const *s)
{
        g_message ("(%d:%d-%d:%d) %s\n", loc->first_line, loc->first_column, loc->last_line, loc->last_column, s);
}*/

/* Public functions
 *---------------------------------------------------------------------------*/

