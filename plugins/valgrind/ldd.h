/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Jeffrey Stedfast <fejj@ximian.com>
 *
 *  Copyright 2003 Ximian, Inc. (www.ximian.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */


#ifndef __LDD_H__
#define __LDD_H__

#include "parser.h"

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

typedef struct _LddParser LddParser;
typedef struct _LddSharedLib LddSharedLib;

struct _LddSharedLib {
	unsigned char *libname;
	unsigned char *path;
	unsigned int addr;
};

typedef void (* LddSharedLibCallback) (LddParser *ldd, LddSharedLib *shlib, void *user_data);

struct _LddParser {
	Parser parser;
	
	unsigned char *linebuf;
	unsigned char *lineptr;
	unsigned int lineleft;
	
	LddSharedLibCallback shlib_cb;
	void *user_data;
};

LddParser *ldd_parser_new (int fd, LddSharedLibCallback shlib_cb, void *user_data);
void ldd_parser_free (LddParser *ldd);

void ldd_shared_lib_free (LddSharedLib *shlib);

int ldd_parser_step (LddParser *ldd);
int ldd_parser_flush (LddParser *ldd);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __LDD_H__ */
