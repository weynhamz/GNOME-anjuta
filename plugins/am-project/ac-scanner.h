/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * ac-scanner.h
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

#ifndef _AC_SCANNER_H_
#define _AC_SCANNER_H_

#include "am-project.h"

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

/* Token location is found directly from token value. We don't maintain a
 * independent position. */
#define YYLTYPE AnjutaToken*
#define YYSTYPE AnjutaToken*

typedef struct _AmpAcScanner AmpAcScanner;

AmpAcScanner *amp_ac_scanner_new (AmpProject *project);
void amp_ac_scanner_free (AmpAcScanner *scanner);

gboolean amp_ac_scanner_parse (AmpAcScanner *scanner, AnjutaTokenFile *file, GError **error);
AnjutaToken* amp_ac_scanner_parse_token (AmpAcScanner *scanner, AnjutaToken *root, AnjutaToken *content, gint start, GFile *filename, GError **error);

void amp_ac_scanner_load_module (AmpAcScanner *scanner, AnjutaToken *module);
void amp_ac_scanner_load_config (AmpAcScanner *scanner, AnjutaToken *list);
void amp_ac_scanner_load_properties (AmpAcScanner *scanner, AnjutaToken *macro, AnjutaToken *args);
void amp_ac_scanner_include (AmpAcScanner *scanner, AnjutaToken *list);

void amp_ac_yyerror (YYLTYPE *loc, AmpAcScanner *scanner, char const *s);

enum
{
	/* Order is important as the backend will try to follow it */
	AC_TOKEN_FIRST_ORDERED_MACRO = ANJUTA_TOKEN_USER,
	/* requirements & informations */
	AC_TOKEN_AC_PREREQ,
	AC_TOKEN_AC_INIT,
	AC_TOKEN_AC_CONFIG_HEADERS,
	AC_TOKEN_AC_CONFIG_SRCDIR,
	AC_TOKEN_AC_CONFIG_MACRO_DIR,
	AC_TOKEN_AM_INIT_AUTOMAKE,

	/* Options */
	AC_TOKEN_AM_MAINTAINER_MODE,
	AC_TOKEN_AC_ARG_ENABLE,

	/* Check for programs */
	AC_TOKEN_AC_PROG_CC,
	AC_TOKEN_AC_PROG_CPP,
	AC_TOKEN_AC_PROG_CXX,
	AC_TOKEN_AC_PROG_LEX,
	AC_TOKEN_AC_PROG_YACC,
	AC_TOKEN_AC_PROG_RANLIB,
	AC_TOKEN_IT_PROG_INTLTOOL,
	AC_TOKEN_LT_PREREQ,
	AC_TOKEN_AM_PROG_LIBTOOL,
	AC_TOKEN_LT_INIT,
	AC_TOKEN_PKG_PROG_PKG_CONFIG,
	AC_TOKEN_AC_CHECK_PROG,
	AC_TOKEN_AM_GLIB_GNU_GETTEXT,

	/* Check for libraries */
	AC_TOKEN_PKG_CHECK_MODULES,
	AC_TOKEN_AC_CHECK_LIB,

	/* Check for headers */
	AC_TOKEN_AC_HEADER_STDC,
	AC_TOKEN_AC_CHECK_HEADERS,
	AC_TOKEN_AC_EGREP_HEADER,

	/* Check for types & structures */
	AC_TOKEN_AC_C_CONST,
	AC_TOKEN_AC_OBJEXT,
	AC_TOKEN_AC_EXEEXT,
	AC_TOKEN_AC_TYPE_SIZE_T,
	AC_TOKEN_AC_TYPE_OFF_T,

	/* Check for functions */
	AC_TOKEN_AC_CHECK_FUNCS,

	/* Output files */
	AC_TOKEN_AC_CONFIG_FILES,
	AC_TOKEN_OBSOLETE_AC_OUTPUT,
	AC_TOKEN_AC_OUTPUT,
	AC_TOKEN_LAST_ORDERED_MACRO,

	AC_TOKEN_SPACE_LIST,
	AC_TOKEN_OPEN_STRING,
	AC_TOKEN_CLOSE_STRING
};

enum
{
	AC_SPACE_LIST_STATE = 1
};

G_END_DECLS

#endif
