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
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "parser.h"
#include "vgio.h"


void
parser_init (Parser *parser, int fd)
{
	parser->inbuf = parser->realbuf + PARSER_SCAN_HEAD;
	parser->inptr = parser->inbuf;
	parser->inend = parser->inbuf;
	
	parser->fd = fd;
}


ssize_t
parser_fill (Parser *parser)
{
	unsigned char *inbuf, *inptr, *inend;
	ssize_t nread;
	size_t inlen;
	
	inbuf = parser->inbuf;
	inptr = parser->inptr;
	inend = parser->inend;
	inlen = inend - inptr;
	
	g_assert (inptr <= inend);
	
	/* attempt to align 'inend' with realbuf + PARSER_SCAN_HEAD */
	if (inptr >= inbuf) {
		inbuf -= inlen < PARSER_SCAN_HEAD ? inlen : PARSER_SCAN_HEAD;
		memmove (inbuf, inptr, inlen);
		inptr = inbuf;
		inbuf += inlen;
	} else if (inptr > parser->realbuf) {
		size_t shift;
		
		shift = MIN (inptr - parser->realbuf, inend - inbuf);
		memmove (inptr - shift, inptr, inlen);
		inptr -= shift;
		inbuf = inptr + inlen;
	} else {
		/* we can't shift... */
		inbuf = inend;
	}
	
	parser->inptr = inptr;
	parser->inend = inbuf;
	inend = parser->realbuf + PARSER_SCAN_HEAD + PARSER_SCAN_BUF - 1;
	
	if ((nread = vg_read (parser->fd, (char *)inbuf, inend - inbuf)) == -1)
		return -1;
	
	parser->inend += nread;
	
	return parser->inend - parser->inptr;
}
