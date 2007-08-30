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

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "vgio.h"

ssize_t
vg_read (int fd, char *buf, size_t n)
{
	ssize_t nread;
	
	do {
		nread = read (fd, buf, n);
	} while (nread == -1 && errno == EINTR);
	
	return nread;
}


ssize_t
vg_write (int fd, const char *buf, size_t n)
{
	size_t nwritten = 0;
	ssize_t w;
	
	do {
		do {
			w = write (fd, buf + nwritten, n - nwritten);
		} while (w == -1 && errno == EINTR);
		
		if (w == -1)
			return -1;
		
		nwritten += w;
	} while (nwritten < n);
	
	return nwritten;
}
