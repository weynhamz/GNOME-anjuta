/*
 * getline.c Copyright (C) 2003 Michael Tindal
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

int getline (char** line, int* size, int fd)
{
	int len, res;
	short brk = 0;
	char* buf = malloc(2096);
	char buf2[1];
	
	strcpy (buf, "");

	while (!brk)
	{
		if ((res = read(fd, buf2, 1)) < 0)
			return -1;
		if (res != 0) // check for end-of-line
			strcat(buf, buf2);
		if (buf2[0] == '\n' || res == 0)
			brk = 1;
	}
	
	if (*line == NULL)	
	{
		*line = (char*)malloc(strlen(buf));
	}
	
	strcpy(*line, buf);
	*size = strlen(*line);
	
	return *size;
}

#endif
