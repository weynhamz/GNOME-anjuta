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
AnjutaToken* amp_ac_scanner_parse_token (AmpAcScanner *scanner, AnjutaToken *token, gint start, GError **error);

void amp_ac_scanner_load_module (AmpAcScanner *scanner, AnjutaToken *module);
void amp_ac_scanner_load_config (AmpAcScanner *scanner, AnjutaToken *list);
void amp_ac_scanner_load_properties (AmpAcScanner *scanner, AnjutaToken *macro, AnjutaToken *args);

void amp_ac_yyerror (YYLTYPE *loc, AmpAcScanner *scanner, char const *s);

enum 
{
	AC_TOKEN_AC_INIT = ANJUTA_TOKEN_USER,
	AC_TOKEN_PKG_CHECK_MODULES,
	AC_TOKEN_AC_CONFIG_FILES,
	AC_TOKEN_OBSOLETE_AC_OUTPUT,
	AC_TOKEN_AC_OUTPUT,
	AC_TOKEN_SPACE_LIST,
	AC_TOKEN_OPEN_STRING,
	AC_TOKEN_CLOSE_STRING,
	AC_TOKEN_AC_PREREQ,
};

enum
{
	AC_SPACE_LIST_STATE = 1
};

G_END_DECLS

#endif
