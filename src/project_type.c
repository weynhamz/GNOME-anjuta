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

#include <glib.h>
#include <time.h>
#include <string.h>

#include "project_dbase.h"
#include "anjuta.h"

gchar* project_type_generic[] = {	
	0, // gnome_project save string
	
	"", // cflags
	"", // ldadd
	
	"", // configure macros
	
	"/autogen.sh.generic", // autogen file
	
	"0", // gnome support
	"0" // glade support
};

gchar* project_type_gtk[] =
{
	"GTK",
	
	" \\\n\t`gtk-config --cflags`",
	" \\\n\t$(GTK_LIBS)",
	
	"\n"
	"AM_PATH_GTK(1.2.0, ,\n"
	"            AC_MSG_ERROR(Cannot find GTK: Is gtk-config in path?))\n",
	
	"/autogen.sh.gtk",
	
	"0",
	"1",
};

gchar* project_type_libglade[] =
{
	"LIBGLADE",
	
	" \\\n\t$(GNOME_INCLUDEDIR) $(LIBGLADE_CFLAGS)",
	" \\\n\t$(LIBGLADE_LIBS)",
	
	"\n"
	"dnl Pick up the GNOME macros.\n"
	"AM_ACLOCAL_INCLUDE(macros)\n"
	"\n"
	"dnl GNOME macros.\n"
	"GNOME_INIT\n"
	"GNOME_COMPILE_WARNINGS\nGNOME_X_CHECKS\n"
	"AM_PATH_LIBGLADE(,,\"gnome\")",
	
	"/autogen.sh.gnome",
	
	"1",
	"1",
};

gchar* project_type_gnome[] =
{
	"GNOME",
	
	" \\\n\t$(GNOME_INCLUDEDIR)",
	" \\\n\t$(GNOME_LIBDIR) $(GNOMEUI_LIBS)",
	
	"\n"
	"dnl Pick up the GNOME macros.\n"
	"AM_ACLOCAL_INCLUDE(macros)\n"
	"\n"
	"dnl GNOME macros.\n"
	"GNOME_INIT\n"
	"GNOME_COMPILE_WARNINGS\nGNOME_X_CHECKS\n",
	
	"/autogen.sh.gnome",
	
	"1",
	"1",
};

gchar* project_type_gtkmm[] =
{
	"GTKMM",
	
	"\\\n\t$(GTKMM_CFLAGS)",
	"\\\n\t$(GTKMM_LIBS)",
	
	"\n"
	"AM_PATH_GTKMM(1.2.5, ,\n"
	"			   AC_MSG_ERROR(Cannot find GTKmm: Is gtkmm-config in path?))\n",
	
	"/autogen.sh.gtkmm",
	
	"0",
	"1",
};

gchar* project_type_gnomemm[] =
{
	"GNOMEMM",
	
	"\\\n\t$(GNOMEMM_CFLAGS)",
	"\\\n\t$(GNOMEMM_LIBS)",
	
	"\n"
	"dnl Pick up the GNOME macros.\n"
	"AM_ACLOCAL_INCLUDE(macros)\n"
	"\n"
	"dnl GNOME macros.\n"
	"GNOME_INIT\n"
	"GNOME_COMPILE_WARNINGS\nGNOME_X_CHECKS\n"
	"\n"
	"dnl GNOMEmm macros. \n"
	"AM_PATH_GTKMM(1.2.5, , AC_MSG_ERROR(\"GTKmm not found\"))\n"
	"AM_PATH_GNOMEMM(1.2.0, , AC_MSG_ERROR(\"GNOMEmm not found\"))\n",
	
	"/autogen.sh.gnomemm",
	
	"1",
	"1",
};

gchar* project_type_bonobo[] =
{
	"BONOBO",
	
	" \\\n\t$(GNOME_INCLUDEDIR)"
	" \\\n\t$(BONOBO_CFLAGS)",
	" \\\n\t$(GNOME_LIBDIR) $(GNOMEUI_LIBS)"
	" \\\n\t$(BONOBO_LIBS)",
	
	"\n"
	"dnl Pick up the GNOME macros.\n"
	"AM_ACLOCAL_INCLUDE(macros)\n"
	"\n"
	"dnl GNOME macros.\n"
	"GNOME_INIT\n"
	"GNOME_COMPILE_WARNINGS\nGNOME_X_CHECKS\n",
	
	"/autogen.sh.gnome",
	
	"1",
	"1",
};




Project_Type* load_type_from_data(char* type_data[], gint id);

Project_Type* load_project_type(gint id)
{
	Project_Type* type;
	
	switch (id)
	{
		case PROJECT_TYPE_GENERIC:
		{
			type = load_type_from_data(project_type_generic, id);
			break;
		}
		case PROJECT_TYPE_GTK:
		{
			type = load_type_from_data(project_type_gtk, id);
			break;
		}
		case PROJECT_TYPE_GTKMM:
		{
			type = load_type_from_data(project_type_gtkmm, id);
			break;
		}
		case PROJECT_TYPE_GNOME:
		{
			type = load_type_from_data(project_type_gnome, id);
			break;
		}
		case PROJECT_TYPE_GNOMEMM:
		{
			type = load_type_from_data(project_type_gnomemm, id);
			break;
		}
		case PROJECT_TYPE_BONOBO:
		{
			type = load_type_from_data(project_type_bonobo, id);
			break;
		}
		case PROJECT_TYPE_LIBGLADE:
		{
			type = load_type_from_data(project_type_libglade, id);
			break;
		}
		default:
		{
			anjuta_error("Unknown project type!");
			return NULL;
		}
	}
	return type;
}

void free_project_type(Project_Type* type)
{
	g_free(type);
}

Project_Type* load_type_from_data(char* type_data[], gint id)
{
	Project_Type* type = g_malloc(sizeof(Project_Type));
	
	type->id = id;
	type->name = project_type_map[id];
	type->save_string = type_data[0];
	
	type->cflags = type_data[1];
	type->ldadd = type_data[2];
		
	type->configure_macros = type_data[3];
	type->autogen_file = type_data[4];
			
	type->gnome_support = atoi(type_data[5]);
	type->glade_support = atoi(type_data[6]);
	
	return type;
}

