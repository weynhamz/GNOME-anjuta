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
	
	" \\\n\t$(GTK_CFLAGS)",
	" \\\n\t$(GTK_LIBS)",
	
	"\n"
	"AM_PATH_GTK(1.2.0, ,\n"
	"            AC_MSG_ERROR(Cannot find GTK: Is gtk-config in path?))\n",
	
	"/autogen.sh.gtk",
	
	"0",
	"1",
};

gchar* project_type_gtk2[] =
{
	"GTK2",
	
	" \\\n\t$(GTK_CFLAGS)",
	" \\\n\t$(GTK_LIBS)",
	
	"PKG_CHECK_MODULES(GTK, gtk+-2.0 gdk-2.0,,exit)\n"
	"AC_SUBST(GTK_LIBS)\n"
	"AC_SUBST(GTK_CFLAGS)",
	
	"/autogen.sh.gtk",
	
	"0",
	"0",
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

gchar* project_type_gnome2[] =
{
	"GNOME2",
	
	" \\\n\t$(GNOME_CFLAGS)",
	" \\\n\t$(GNOME_LIBS)",
	
	"PKG_CHECK_MODULES(GNOME, libgnomeui-2.0 gtk+-2.0,,exit)\n"
	"AC_SUBST(GNOME_LIBS)\n"
	"AC_SUBST(GNOME_CFLAGS)",
	
	"/autogen.sh.gnome",
	
	"1",
	"0",
};

gchar* project_type_libglade2[] =
{
	"LIBGLADE2",
	
	" \\\n\t$(GNOME_CFLAGS)",
	" \\\n\t$(GNOME_LIBS)",
	
	"PKG_CHECK_MODULES(GNOME, libgnomeui-2.0 gtk+-2.0 libglade-2.0,,exit)\n"
	"AC_SUBST(GNOME_LIBS)\n"
	"AC_SUBST(GNOME_CFLAGS)",
	
	"/autogen.sh.gnome",
	
	"1",
	"0",
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

gchar* project_type_gtkmm2[] =
{
	"GTKMM2",
	
	"\\\n\t$(GTKMM_CFLAGS)",
	"\\\n\t$(GTKMM_LIBS)",
	
	"PKG_CHECK_MODULES(GTKMM, gtkmm-2.0,,exit)\n"
	"AC_SUBST(GTKMM_LIBS)\n"
	"AC_SUBST(GTKMM_CFLAGS)",
	
	"/autogen.sh.gtkmm",
	
	"0",
	"0",
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

gchar* project_type_gnomemm2[] =
{
	"GNOMEMM2",
	
	"\\\n\t$(GNOMEMM_CFLAGS)",
	"\\\n\t$(GNOMEMM_LIBS)",
	
	"PKG_CHECK_MODULES(GNOME, libgnomeuimm-2.0,,exit)\n"
	"AC_SUBST(GNOMEMM_LIBS)\n"
	"AC_SUBST(GNOMEMM_CFLAGS)",
	
	"/autogen.sh.gnomemm",
	
	"1",
	"0",
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

gchar* project_type_wxwin[] = 
{
	"WXWINDOWS",
	  
	" \\\n\t$(WX_CXXFLAGS)",
	" \\\n\t$(WX_LIBS)",
	  
	"\n"
	"AM_OPTIONS_WXCONFIG\n"
	"AM_PATH_WXCONFIG(2.3.2, wxWin=1)\n"
	"        if test \"$wxWin\" != 1; then\n"
	"        AC_MSG_ERROR([\n"
	"                wxWindows must be installed on your system.\n"
	"\n"     
	"                Please check that wx-config is in path, the directory\n"
	"                where wxWindows libraries are installed (returned by\n"
	"                'wx-config --libs' or 'wx-config --static --libs' command)\n"
	"                is in LD_LIBRARY_PATH or equivalent variable and\n"
	"                wxWindows version is 2.3.2 or above.\n"
	"                ])\n"
	"        else\n"
	"               dnl Quick hack until wx-config does it\n"
	"               ac_save_LIBS=$LIBS\n"
	"               ac_save_CXXFLAGS=$CXXFLAGS\n"
	"               LIBS=$WX_LIBS\n"
	"               CXXFLAGS=$WX_CXXFLAGS\n"
	"               AC_LANG_SAVE\n"
	"               AC_LANG_CPLUSPLUS\n"
	"               AC_TRY_LINK([#include <wx/wx.h>],\n"
	"               [wxString test=\"\"],\n"
	"               ,[WX_LIBS=$WX_LIBS_STATIC])\n"
	"               AC_LANG_RESTORE\n"
	"               LIBS=$ac_save_LIBS\n"
	"               CXXFLAGS=$ac_save_CXXFLAGS\n"
	"        fi\n"
	"\n"
	"AM_PATH_GTK(1.2.7, ,\n"
	"            AC_MSG_ERROR(Cannot find GTK: Is gtk-config in path?),\n"
	"            gthread)\n",
	
	"/autogen.sh.wxwin",

	"0",
	"0",
};

gchar* project_type_xwin[] = 
{
	"XWINDOWS",
	  
	" \\\n\t$(X_CFLAGS)",
	" \\\n\t-lX11 -lXpm -lXext"
	" \\\n\t$(X_LIBS)"
	" \\\n\t$(X_EXTRA_LIBS)",
    
	"AC_PATH_XTRA\n\n"
	"if test $no_x; then\n"
	"\tAC_MSG_ERROR([The path for the X11 files not found!\n"
	"\tMake sure you have X and it's headers and libraries"
	"(the -devel packages in Linux) installed.])\n"
	"fi\n",
	
	"/autogen.sh.generic",

	"0",
	"0",
};

gchar* project_type_xwindockapp[] = 
{
	"XWINDOWSDOCKAPP",
	  
	" \\\n\t$(X_CFLAGS)",
	" \\\n\t-lX11 -lXpm -lXext"
	" \\\n\t$(X_LIBS)"
	" \\\n\t$(X_EXTRA_LIBS)",
    
	"AC_PATH_XTRA\n\n"
	"if test $no_x; then\n"
	"\tAC_MSG_ERROR([The path for the X11 files not found!\n"
	"\tMake sure you have X and it's headers and libraries"
	"(the -devel packages in Linux) installed.])\n"
	"fi\n",
	
	"/autogen.sh.generic",

	"0",
	"0",	  
};

gchar* project_type_qt[] = {	
	"QT", // gnome_project save string
	"", // cflags
	"", // ldadd
	"", // configure macros
	"", // autogen file
	"0", // gnome support
	"0" // glade support

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
		case PROJECT_TYPE_GTK2:
		{
			type = load_type_from_data(project_type_gtk2, id);
			break;
		}
		case PROJECT_TYPE_GTKMM:
		{
			type = load_type_from_data(project_type_gtkmm, id);
			break;
		}
		case PROJECT_TYPE_GTKMM2:
		{
			type = load_type_from_data(project_type_gtkmm2, id);
			break;
		}
		case PROJECT_TYPE_GNOME:
		{
			type = load_type_from_data(project_type_gnome, id);
			break;
		}
		case PROJECT_TYPE_GNOME2:
		{
			type = load_type_from_data(project_type_gnome2, id);
			break;
		}
		case PROJECT_TYPE_LIBGLADE2:
		{
			type = load_type_from_data(project_type_libglade2, id);
			break;
		}
		case PROJECT_TYPE_GNOMEMM:
		{
			type = load_type_from_data(project_type_gnomemm, id);
			break;
		}
		case PROJECT_TYPE_GNOMEMM2:
		{
			type = load_type_from_data(project_type_gnomemm2, id);
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
		case PROJECT_TYPE_WXWIN:
		{
			type = load_type_from_data(project_type_wxwin, id);
			break;
		}
		case PROJECT_TYPE_XWIN:
		{
			type = load_type_from_data(project_type_xwin, id);
			break;
		}
		case PROJECT_TYPE_XWINDOCKAPP:
		{
			type = load_type_from_data(project_type_xwindockapp, id);
			break;
		}
		case PROJECT_TYPE_QT:
		{
			type = load_type_from_data(project_type_qt, id);
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

