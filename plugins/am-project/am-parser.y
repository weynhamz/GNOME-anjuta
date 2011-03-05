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

#include "am-scanner.h"
#include "am-parser.h"

#include <stdlib.h>

#define YYDEBUG 1

/* Token location is found directly from token value, there is no need to
 * maintain a separate location variable */
#define YYLLOC_DEFAULT(Current, Rhs, N)	((Current) = YYRHSLOC(Rhs, (N) ? 1 : 0))
%}


%token	END_OF_FILE
%token	END_OF_LINE	'\n'
%token	SPACE
%token	TAB '\t'
%token	COMMENT '#'
%token	MACRO
%token	VARIABLE
%token	COLON ':'
%token	DOUBLE_COLON "::"
%token	ORDER '|'
%token	SEMI_COLON ';'
%token	EQUAL '='
%token	IMMEDIATE_EQUAL ":="
%token	CONDITIONAL_EQUAL "?="
%token	APPEND "+="
%token	CHARACTER
%token	NAME
%token	AM_VARIABLE

%token	INCLUDE

%token  SUBDIRS
%token  DIST_SUBDIRS
%token  _DATA
%token  _HEADERS
%token  _LIBRARIES
%token  _LISP
%token  _LTLIBRARIES
%token  _MANS
%token  _PROGRAMS
%token  _PYTHON
%token  _JAVA
%token  _SCRIPTS
%token  _SOURCES
%token  _TEXINFOS
%token  _DIR
%token  _LDFLAGS
%token  _CPPFLAGS
%token  _CFLAGS
%token  _CXXFLAGS
%token  _JAVACFLAGS
%token  _VALAFLAGS
%token  _FCFLAGS
%token  _OBJCFLAGS
%token  _LFLAGS
%token  _YFLAGS
%token  TARGET_LDFLAGS
%token  TARGET_CPPFLAGS
%token  TARGET_CFLAGS
%token  TARGET_CXXFLAGS
%token  TARGET_JAVACFLAGS
%token  TARGET_VALAFLAGS
%token  TARGET_FCFLAGS
%token  TARGET_OBJCFLAGS
%token  TARGET_LFLAGS
%token  TARGET_YFLAGS
%token  TARGET_DEPENDENCIES
%token  TARGET_LIBADD
%token  TARGET_LDADD


%defines

%define api.pure
%define api.push_pull "push"

%parse-param {AmpAmScanner* scanner}
%lex-param   {AmpAmScanner* scanner}

%name-prefix="amp_am_yy"

%locations

%start file

%expect 1

%debug

%verbose

%{

//amp_am_yydebug = 1;

static gint
amp_am_automake_variable (AnjutaToken *token)
{
    switch (anjuta_token_get_type (token))
    {
    case SUBDIRS:               return AM_TOKEN_SUBDIRS;
    case DIST_SUBDIRS:          return AM_TOKEN_DIST_SUBDIRS;
    case _DATA:                 return AM_TOKEN__DATA;
    case _HEADERS:              return AM_TOKEN__HEADERS;
    case _LIBRARIES:            return AM_TOKEN__LIBRARIES;
    case _LISP:                 return AM_TOKEN__LISP;
    case _LTLIBRARIES:          return AM_TOKEN__LTLIBRARIES;
    case _MANS:                 return AM_TOKEN__MANS;
    case _PROGRAMS:             return AM_TOKEN__PROGRAMS;
    case _PYTHON:               return AM_TOKEN__PYTHON;
    case _JAVA:                 return AM_TOKEN__JAVA;
    case _SCRIPTS:              return AM_TOKEN__SCRIPTS;
    case _SOURCES:              return AM_TOKEN__SOURCES;
    case _TEXINFOS:             return AM_TOKEN__TEXINFOS;
    case _DIR:                  return AM_TOKEN_DIR;
    case _LDFLAGS:              return AM_TOKEN__LDFLAGS;
    case _CPPFLAGS:             return AM_TOKEN__CPPFLAGS;
    case _CFLAGS:               return AM_TOKEN__CFLAGS;
    case _CXXFLAGS:             return AM_TOKEN__CXXFLAGS;
    case _JAVACFLAGS:           return AM_TOKEN__JAVACFLAGS;
    case _VALAFLAGS:           return AM_TOKEN__VALAFLAGS;
    case _FCFLAGS:              return AM_TOKEN__FCFLAGS;
    case _OBJCFLAGS:            return AM_TOKEN__OBJCFLAGS;
    case _LFLAGS:               return AM_TOKEN__LFLAGS;
    case _YFLAGS:               return AM_TOKEN__YFLAGS;
    case TARGET_LDFLAGS:        return AM_TOKEN_TARGET_LDFLAGS;
    case TARGET_CPPFLAGS:       return AM_TOKEN_TARGET_CPPFLAGS;
    case TARGET_CFLAGS:         return AM_TOKEN_TARGET_CFLAGS;
    case TARGET_CXXFLAGS:       return AM_TOKEN_TARGET_CXXFLAGS;
    case TARGET_JAVACFLAGS:     return AM_TOKEN_TARGET_JAVACFLAGS;
    case TARGET_VALAFLAGS:     return AM_TOKEN_TARGET_VALAFLAGS;
    case TARGET_FCFLAGS:        return AM_TOKEN_TARGET_FCFLAGS;
    case TARGET_OBJCFLAGS:      return AM_TOKEN_TARGET_OBJCFLAGS;
    case TARGET_LFLAGS:         return AM_TOKEN_TARGET_LFLAGS;
    case TARGET_YFLAGS:         return AM_TOKEN_TARGET_YFLAGS;
    case TARGET_DEPENDENCIES:   return AM_TOKEN_TARGET_DEPENDENCIES;
    case TARGET_LIBADD:         return AM_TOKEN_TARGET_LIBADD;
    case TARGET_LDADD:  		 return AM_TOKEN_TARGET_LDADD;

    default: return ANJUTA_TOKEN_NAME;
    }
}

%}

%%

/* File cannot be empty, there at least the END_OF_FILE token */
file:
	statement
	| file statement
	;

statement:
	end_of_line
	| space  end_of_line
	| definition  end_of_line
	| am_variable  end_of_line
	| include  end_of_line
	| line  end_of_line
	| rule  command_list
	;
	
am_variable:
	optional_space  automake_token  optional_space  equal_token  value_list {
		$$ = anjuta_token_new_static (amp_am_automake_variable ($2), NULL);
		if ($1 != NULL) anjuta_token_set_type ($1, ANJUTA_TOKEN_START);
		anjuta_token_merge ($$, $2);
		if ($3 != NULL) anjuta_token_set_type ($3, ANJUTA_TOKEN_NEXT);
		anjuta_token_merge ($$, $4);
		anjuta_token_merge ($$, $5);
		amp_am_scanner_set_am_variable (scanner, $$);
	}
	| optional_space automake_token optional_space equal_token
	{
		AnjutaToken *list;
		list = anjuta_token_new_static (ANJUTA_TOKEN_LIST, NULL);
		anjuta_token_insert_after ($4, list);
		$$ = anjuta_token_new_static (amp_am_automake_variable ($2), NULL);
		anjuta_token_merge ($$, $2);
		anjuta_token_merge ($$, list);
		amp_am_scanner_set_am_variable (scanner, $$);
	}
	;

include:
	optional_space  include_token  value_list {
		$$ = anjuta_token_new_static (ANJUTA_TOKEN_LIST, NULL);
		anjuta_token_merge ($$, $2);
		anjuta_token_merge ($$, $3);
		amp_am_scanner_include (scanner, $$);
	}

definition:
	head_list  equal_token value_list {
        $$ = anjuta_token_new_static (ANJUTA_TOKEN_DEFINITION, NULL);
        anjuta_token_merge_own_children ($1);
        anjuta_token_merge ($$, $1);
        anjuta_token_merge ($$, $2);
        anjuta_token_merge ($$, $3);
        amp_am_scanner_update_variable (scanner, $$);
	}
	| head_list  equal_token {
		AnjutaToken *list;
		list = anjuta_token_new_static (ANJUTA_TOKEN_LIST, NULL);
		anjuta_token_insert_after ($2, list);
		$$ = anjuta_token_new_static (ANJUTA_TOKEN_DEFINITION, NULL);
        anjuta_token_merge_own_children ($1);
        anjuta_token_merge ($$, $1);
        anjuta_token_merge ($$, $2);
		anjuta_token_merge ($$, list);
		amp_am_scanner_update_variable (scanner, $$);
	}
	;

rule:
	depend_list  end_of_line
	| depend_list  SEMI_COLON  command_line  END_OF_LINE
	| depend_list  SEMI_COLON  command_line  END_OF_FILE
	;

depend_list:
	head_list  rule_token  prerequisite_list
	;

command_list:
	/* empty */
	| command_list TAB command_line END_OF_LINE
	| command_list TAB command_line END_OF_FILE
	;

line:
	head_list
	;

/* Lists
 *----------------------------------------------------------------------------*/
 
end_of_line:
	END_OF_LINE {
		anjuta_token_set_type ($1, ANJUTA_TOKEN_EOL);
		$$ = NULL;
	}
	| END_OF_FILE {
		$$ = NULL;
	}
	| COMMENT {
		anjuta_token_set_type ($1, ANJUTA_TOKEN_COMMENT);
		$$ = NULL;
	}
	;

not_eol_list:
	/* empty */
	| not_eol_list not_eol_token
	;

prerequisite_list:
	/* empty */
	| space
	| optional_space  prerequisite_list_body  optional_space
	;

prerequisite_list_body:
	prerequisite
	| prerequisite_list_body  space  prerequisite
	;

head_list:
	optional_space head_list_body optional_space {
		$$ = anjuta_token_merge_previous ($2, $1);
		anjuta_token_merge ($$, $3);
	}
	;

head_list_body:
	head {
		$$ = anjuta_token_new_static (ANJUTA_TOKEN_NAME, NULL);
		anjuta_token_merge ($$, $1);
	}
	| head_list_body  space  next_head {
		anjuta_token_merge ($1, $2);
		anjuta_token_merge ($1, $3);
	}
	| head_list_body  space  head {
		anjuta_token_merge ($1, $2);
		anjuta_token_merge ($1, $3);
	}
	;

value_list:
	space {
		$$ = anjuta_token_new_static (ANJUTA_TOKEN_LIST, NULL);
		if ($1 != NULL) anjuta_token_set_type ($1, ANJUTA_TOKEN_START);
		anjuta_token_merge ($$, $1);
	}
	| optional_space value_list_body optional_space {
		if ($1 != NULL) anjuta_token_set_type ($1, ANJUTA_TOKEN_START);
		if ($3 != NULL) anjuta_token_set_type ($3, ANJUTA_TOKEN_LAST);
		anjuta_token_merge_previous ($2, $1);
		anjuta_token_merge ($2, $3);
		$$ = $2;
	}

value_list_body:
	value {
		$$ = anjuta_token_new_static (ANJUTA_TOKEN_LIST, NULL);
		anjuta_token_merge ($$, $1);
	}
	| value_list_body space  value {
		anjuta_token_set_type ($2, ANJUTA_TOKEN_NEXT);
		anjuta_token_merge ($1, $2);
		anjuta_token_merge ($1, $3);
	}
	;

	
	
command_line:
	/* empty */
	| command_line command_token
	;

/* Items
 *----------------------------------------------------------------------------*/

optional_space:
	/* empty */ {
		$$ = NULL;
	}
	| space
	;

space:
	space_token {
		$$ = anjuta_token_new_static (ANJUTA_TOKEN_SPACE, NULL);
		anjuta_token_merge ($$, $1);
	}
	/*| space_variable {
		$$ = anjuta_token_new_static (ANJUTA_TOKEN_SPACE, NULL);
		anjuta_token_merge ($$, $1);
	}*/
	| space space_token {
		anjuta_token_merge ($1, $2);
	}
	/*| space space_variable {
		anjuta_token_merge ($1, $2);
	}*/
	;

head:
	head_token {
		$$ = anjuta_token_new_static (ANJUTA_TOKEN_NAME, NULL);
		anjuta_token_merge ($$, $1);
	}
	| variable {
		$$ = anjuta_token_new_static (ANJUTA_TOKEN_NAME, NULL);
		anjuta_token_merge ($$, $1);
	}
	| head head_token {
		anjuta_token_merge ($1, $2);
	}
	| head automake_token {
		anjuta_token_merge ($1, $2);
	}
	| head include_token {
		anjuta_token_merge ($1, $2);
	}
	| head variable {
		anjuta_token_merge ($1, $2);
	}
	;

next_head:
	automake_token {
		$$ = anjuta_token_new_static (ANJUTA_TOKEN_NAME, NULL);
		anjuta_token_merge ($$, $1);
	}
	| include_token {
		$$ = anjuta_token_new_static (ANJUTA_TOKEN_NAME, NULL);
		anjuta_token_merge ($$, $1);
	}
	| next_head head_token {
		anjuta_token_merge ($1, $2);
	}
	| next_head automake_token {
		anjuta_token_merge ($1, $2);
	}
	| next_head include_token {
		anjuta_token_merge ($1, $2);
	}
	| next_head variable {
		anjuta_token_merge ($1, $2);
	}
	;

value:
	value_token {
		$$ = anjuta_token_new_static (ANJUTA_TOKEN_ARGUMENT, NULL);
		anjuta_token_merge ($$, $1);
	}
	| variable {
		$$ = anjuta_token_new_static (ANJUTA_TOKEN_ARGUMENT, NULL);
		anjuta_token_merge ($$, $1);
	}
	| value value_token {
		anjuta_token_merge ($1, $2);
	}
	| value variable {
		anjuta_token_merge ($1, $2);
	}
	;

prerequisite:
	name_prerequisite
	;

name_prerequisite:
	prerequisite_token
	| variable
	| name_prerequisite prerequisite_token
	| name_prerequisite variable
	;

variable:
	variable_token {
		amp_am_scanner_parse_variable (scanner, $$);
	}
	;

/* Tokens
 *----------------------------------------------------------------------------*/

not_eol_token:
	word_token
	| space_token
	;

prerequisite_token:
	name_token
	| equal_token
	| rule_token
	;

command_token:
	name_token
	| variable_token
	| automake_token
	| equal_token
	| rule_token
	| depend_token
	| space_token
	| comment_token
	;

value_token:
	name_token
	| equal_token
	| rule_token
	| depend_token
	| include_token
	| automake_token
	;

head_token:
	name_token
	| depend_token
	;

space_token:
	SPACE
	| TAB
	;

comment_token:
	COMMENT
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

depend_token:
	SEMI_COLON
	;

word_token:
	name_token
	| equal_token
	| rule_token
	| depend_token
	;


name_token:
	MACRO
	| NAME
	| CHARACTER
	| ORDER
	;

variable_token:
	VARIABLE
	;

automake_token:
	SUBDIRS
	| DIST_SUBDIRS
	| _DATA
	| _HEADERS
	| _LIBRARIES
	| _LISP
	| _LTLIBRARIES
	| _MANS
	| _PROGRAMS
	| _PYTHON
	| _JAVA
	| _SCRIPTS
	| _SOURCES
	| _TEXINFOS
	|  _DIR
	|  _LDFLAGS
	|  _CPPFLAGS
	|  _CFLAGS
	|  _CXXFLAGS
	|  _JAVACFLAGS
	|  _VALAFLAGS
	|  _FCFLAGS
	|  _OBJCFLAGS
	|  _LFLAGS
	|  _YFLAGS
	|  TARGET_LDFLAGS
	|  TARGET_CPPFLAGS
	|  TARGET_CFLAGS
	|  TARGET_CXXFLAGS
	|  TARGET_JAVACFLAGS
	|  TARGET_VALAFLAGS
	|  TARGET_FCFLAGS
	|  TARGET_OBJCFLAGS
	|  TARGET_LFLAGS
	|  TARGET_YFLAGS
	|  TARGET_DEPENDENCIES
	|  TARGET_LIBADD
	|  TARGET_LDADD
	;

include_token:
	INCLUDE
	;
%%
