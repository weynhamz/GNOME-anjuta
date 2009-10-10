/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * mk-scanner.h
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

#ifndef _MK_SCANNER_H_
#define _MK_SCANNER_H_

#include "libanjuta/anjuta-token.h"
#include "libanjuta/anjuta-token-file.h"
#include "libanjuta/anjuta-token-style.h"

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define YYSTYPE AnjutaToken*

#ifndef _MK_PROJECT_H_
typedef struct _MkpProject        MkpProject;
#endif

typedef struct _MkpScanner MkpScanner;

MkpScanner *mkp_scanner_new (MkpProject *project);
void mkp_scanner_free (MkpScanner *scanner);

gboolean mkp_scanner_parse (MkpScanner *scanner, AnjutaTokenFile *file, GError **error);

void mkp_scanner_update_variable (MkpScanner *scanner, AnjutaToken *variable);
void mkp_scanner_add_rule (MkpScanner *scanner, AnjutaToken *rule);

const gchar* mkp_scanner_get_filename (MkpScanner *scanner);

typedef enum
{
	MK_TOKEN_RULE = ANJUTA_TOKEN_USER,
	MK_TOKEN_EQUAL,
	MK_TOKEN_IMMEDIATE_EQUAL,
	MK_TOKEN_CONDITIONAL_EQUAL,
	MK_TOKEN_APPEND,
	MK_TOKEN_TARGET,
	MK_TOKEN_PREREQUISITE,
	MK_TOKEN_ORDER_PREREQUISITE,
	MK_TOKEN_ORDER,
	MK_TOKEN_COLON,
	MK_TOKEN_DOUBLE_COLON,
	MK_TOKEN_VARIABLE,
	MK_TOKEN__PHONY,
	MK_TOKEN__SUFFIXES,
	MK_TOKEN__DEFAULT,
	MK_TOKEN__PRECIOUS,
	MK_TOKEN__INTERMEDIATE,
	MK_TOKEN__SECONDARY,
	MK_TOKEN__SECONDEXPANSION,
	MK_TOKEN__DELETE_ON_ERROR,
	MK_TOKEN__IGNORE,
	MK_TOKEN__LOW_RESOLUTION_TIME,
	MK_TOKEN__SILENT,
	MK_TOKEN__EXPORT_ALL_VARIABLES,
	MK_TOKEN__NOTPARALLEL,
} MakeTokenType;

G_END_DECLS

#endif
