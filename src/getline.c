/*
 * getline.c Copyright (C) 2003 Alexander Nedotsukov
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef FREEBSD

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* GNU libc getline() compatibility */

int
getline(char** line, size_t* size, FILE* fp)
{
	static const size_t line_grow_by = 80; /* in most texts line fits console */
	int ch;
	size_t i;

	if (line == NULL || size == NULL || fp == NULL) { /* illegal call */
		errno = EINVAL;
		return -1;
	}
	if (*line == NULL && *size) { /* logically incorrect */
		errno = EINVAL;
		return -1;
	}

	i = 0;
	while (1) {
		ch = fgetc(fp);
		if (ch == EOF)
			break;
		/* ensure bufer still large enough for ch and trailing null */
		if ((*size - i) <= 1) {
			*line = (char*)realloc(*line, *size += line_grow_by);
			if (*line == NULL) {
				errno = ENOMEM;
				return -1;
			}
		}
		*(*line + i++) = (char)ch;
		if (ch == '\n')
			break;
	}
	
	*(*line + i) = 0;
	
	return ferror(fp) ? -1 : i;
}

#endif

