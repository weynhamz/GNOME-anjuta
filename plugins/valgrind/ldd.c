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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "ldd.h"


LddParser *
ldd_parser_new (int fd, LddSharedLibCallback shlib_cb, void *user_data)
{
	LddParser *ldd;
	
	ldd = g_new (LddParser, 1);
	parser_init ((Parser *) ldd, fd);
	
	ldd->linebuf = g_malloc (128);
	ldd->lineptr = ldd->linebuf;
	ldd->lineleft = 128;
	
	ldd->shlib_cb = shlib_cb;
	ldd->user_data = user_data;
	
	return ldd;
}


void
ldd_parser_free (LddParser *ldd)
{
	if (ldd == NULL)
		return;
	
	g_free (ldd->linebuf);
	g_free (ldd);
}


void
ldd_shared_lib_free (LddSharedLib *shlib)
{
	if (shlib == NULL)
		return;
	
	g_free (shlib->libname);
	g_free (shlib->path);
	g_free (shlib);
}


/* "        libc.so.6 => /lib/libc.so.6 (0x40e50000)\n" */

static void
ldd_parse_linebuf (LddParser *ldd)
{
	register unsigned char *inptr;
	LddSharedLib *shlib;
	char *inend;
	
	shlib = g_new (LddSharedLib, 1);
	
	inptr = ldd->linebuf;
	while (*inptr == '\t' || *inptr == ' ')
		inptr++;
	
	shlib->libname = inptr;
	while (*inptr && !(inptr[0] == ' ' && inptr[1] == '=' && inptr[2] == '>') && *inptr != '(')
		inptr++;
	
	shlib->libname = (unsigned char *)g_strndup ((char *)shlib->libname, inptr - shlib->libname);
	
	if (!strncmp ((char *)inptr, " =>", 3))
		inptr += 3;
	
	while (*inptr == ' ' || *inptr == '\t')
		inptr++;
	
	shlib->path = inptr;
	while (*inptr && *inptr != '(')
		inptr++;
	
	if (*inptr != '(') {
		/* error - no address component; ignore */
		g_free (shlib->libname);
		g_free (shlib);
		goto reset;
	}
	
	if (inptr == shlib->path) {
		/* special case - no path component */
		if (shlib->libname[0] != '/') {
			/* unhandled */
			g_free (shlib->libname);
			g_free (shlib);
			goto reset;
		}
		
		/* name is a path */
		shlib->path = (unsigned char *)g_strdup ((char *)shlib->libname);
		inptr++;
	} else {
		if (inptr[-1] == ' ')
			inptr--;
		shlib->path = (unsigned char *)g_strndup ((char *)shlib->path, inptr - shlib->path);
		inptr += 2;
	}
	
	shlib->addr = strtoul ((const char *) inptr, &inend, 16);
	
	ldd->shlib_cb (ldd, shlib, ldd->user_data);
	
 reset:
	
	ldd->lineleft += (ldd->lineptr - ldd->linebuf);
	ldd->lineptr = ldd->linebuf;
}


#define ldd_backup_linebuf(ldd, start, len) G_STMT_START {                \
	if (ldd->lineleft <= len) {                                       \
		unsigned int llen, loff;                                  \
		                                                          \
		llen = loff = ldd->lineptr - ldd->linebuf;                \
		llen = llen ? llen : 1;                                   \
		                                                          \
		while (llen < (loff + len + 1))                           \
			llen <<= 1;                                       \
		                                                          \
		ldd->linebuf = g_realloc (ldd->linebuf, llen);            \
		ldd->lineptr = ldd->linebuf + loff;                       \
		ldd->lineleft = llen - loff;                              \
	}                                                                 \
	                                                                  \
	memcpy (ldd->lineptr, start, len);                                \
	ldd->lineptr += len;                                              \
	ldd->lineleft -= len;                                             \
} G_STMT_END


int
ldd_parser_step (LddParser *ldd)
{
	register unsigned char *inptr;
	unsigned char *start;
	Parser *parser;
	int ret;
	
	parser = (Parser *) ldd;
	
	if ((ret = parser_fill (parser)) == 0) {
		return 0;
	} else if (ret == -1) {
		return -1;
	}
	
	start = inptr = parser->inptr;
	*parser->inend = '\n';
	
	while (inptr < parser->inend) {
		while (*inptr != '\n')
			inptr++;
		
		if (inptr == parser->inend)
			break;
		
		*inptr++ = '\0';
		ldd_backup_linebuf (ldd, start, inptr - start);
		ldd_parse_linebuf (ldd);
		start = inptr;
	}
	
	if (inptr > start)
		ldd_backup_linebuf (ldd, start, inptr - start);
	
	parser->inptr = inptr;
	
	return 1;
}


int
ldd_parser_flush (LddParser *ldd)
{
	Parser *parser = (Parser *) ldd;
	
	if (parser->inptr < parser->inend)
		return ldd_parser_step (ldd);
	
	return 0;
}
