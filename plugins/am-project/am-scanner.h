/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * am-scanner.h
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

#ifndef _AM_SCANNER_H_
#define _AM_SCANNER_H_

#include "am-project.h"

#include "libanjuta/anjuta-token.h"
#include "libanjuta/anjuta-token-file.h"

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

//#define YYSTYPE AnjutaToken*
#define YYLTYPE AnjutaToken*
#define YYSTYPE AnjutaToken*

typedef struct _AmpAmScanner AmpAmScanner;

AmpAmScanner *amp_am_scanner_new (AmpProject *project, AmpGroup *group);
void amp_am_scanner_free (AmpAmScanner *scanner);

AnjutaToken *amp_am_scanner_parse_token (AmpAmScanner *scanner, AnjutaToken *token, GFile *filename, GError **error);

void amp_am_scanner_set_am_variable (AmpAmScanner *scanner, AnjutaTokenType variable, AnjutaToken *name, AnjutaToken *list);
void amp_am_scanner_include (AmpAmScanner *scanner, AnjutaToken *name);


void amp_am_yyerror (YYLTYPE *loc, AmpAmScanner *scanner, char const *s);

typedef enum
{
	AM_TOKEN_SUBDIRS = ANJUTA_TOKEN_USER,
	AM_TOKEN_DIST_SUBDIRS,
	AM_TOKEN__DATA,
	AM_TOKEN__HEADERS,
	AM_TOKEN__LIBRARIES,
	AM_TOKEN__LISP,
	AM_TOKEN__LTLIBRARIES,
	AM_TOKEN__MANS,
	AM_TOKEN__PROGRAMS,
	AM_TOKEN__PYTHON,
	AM_TOKEN__SCRIPTS,
	AM_TOKEN__SOURCES,
	AM_TOKEN__TEXINFOS,
	AM_TOKEN__JAVA,
	AM_TOKEN_DIR,
	AM_TOKEN__LDFLAGS,
	AM_TOKEN__CPPFLAGS,
	AM_TOKEN__CFLAGS,
	AM_TOKEN__CXXFLAGS,
	AM_TOKEN__JAVACFLAGS,
	AM_TOKEN__FCFLAGS,
	AM_TOKEN__OBJCFLAGS,
	AM_TOKEN__LFLAGS,
	AM_TOKEN__YFLAGS,
	AM_TOKEN_TARGET_LDFLAGS,
	AM_TOKEN_TARGET_CPPFLAGS,
	AM_TOKEN_TARGET_CFLAGS,
	AM_TOKEN_TARGET_CXXFLAGS,
	AM_TOKEN_TARGET_JAVACFLAGS,
	AM_TOKEN_TARGET_FCFLAGS,
	AM_TOKEN_TARGET_OBJCFLAGS,
	AM_TOKEN_TARGET_LFLAGS,
	AM_TOKEN_TARGET_YFLAGS,
	AM_TOKEN_TARGET_DEPENDENCIES,
} AmTokenType;

G_END_DECLS

#endif
