/*
    anjuta_dirs.h
    Copyright (C) 2000  Kh. Naba Kumar Singh

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _ANJUTA_DIRS_H_
#define _ANJUTA_DIRS_H_

typedef struct _AnjutaDirs AnjutaDirs;

struct _AnjutaDirs
{
	gchar *tmp;
	gchar *home;
	gchar *data;
	gchar *doc;
	gchar *pixmaps;
	gchar *help;
	gchar *settings;

	gboolean first_time;
};

AnjutaDirs *anjuta_dirs_new (void);
void anjuta_dirs_destroy (AnjutaDirs * ad);

#endif
