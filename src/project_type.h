/*
    project_type.c
    Copyright (C) 2001  Johannes Schmid

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

#ifndef _PROJECT_TYPE_H
#define _PROJECT_TYPE_H

#include <glib.h>

typedef struct _ProjectType ProjectType;

struct _ProjectType
{
	gint id;
	gchar* name;
	gchar* save_string;
	
	gchar* cflags;
	gchar* ldadd;
	
	gchar* configure_macros;
	
	gboolean gnome_support;
	gboolean gnome2_support;
	gboolean glade_support;
	gboolean deprecated;
};

ProjectType* project_type_load (gint ID);
void project_type_free (ProjectType *type);

#endif
