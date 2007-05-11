/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    gdb_info.h
    Copyright (C) Naba Kumar  <naba@gnome.org>

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

#ifndef __GDB_INFO_H__
#define __GDB_INFO_H__

#include <stdio.h>

gboolean gdb_info_show_file (GtkWindow *parent, const gchar * path,
							 gint height, gint width);

gboolean gdb_info_show_command (GtkWindow *parent,
								const gchar * command_line,
								gint height, gint width);

gboolean gdb_info_show_string (GtkWindow *parent, const gchar * s,
							   gint height, gint width);

gboolean gdb_info_show_filestream (GtkWindow *parent, FILE * f,
								   gint height, gint width);

gboolean gdb_info_show_fd (GtkWindow *parent, int file_descriptor,
						   gint height, gint width);

void gdb_info_show_list (GtkWindow *parent, const GList* list,
						 gint height, gint width);

#endif
