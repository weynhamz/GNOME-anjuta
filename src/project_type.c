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
	NULL, // gnome_project save string
	
	"", // cflags
	"", // ldadd
	
	"", // configure macros
	
	"0", // gnome support
	"0", // gnome2 support
	"0", // glade support
	"0" // Deprecated
};

gchar* project_type_gtk2[] =
{
	"GTK2",
	
	" \\\n\t$(PACKAGE_CFLAGS)",
	" \\\n\t$(PACKAGE_LIBS)",
	
	"PKG_CHECK_MODULES(PACKAGE, [gtk+-2.0 gdk-2.0])\n"
	"AC_SUBST(PACKAGE_CFLAGS)\n"
	"AC_SUBST(PACKAGE_LIBS)",

	"0",
	"0", 
	"1",
	"0"
};

gchar* project_type_gnome2[] =
{
	"GNOME2",
	
	" \\\n\t$(PACKAGE_CFLAGS)",
	" \\\n\t$(PACKAGE_LIBS)",
	
	"PKG_CHECK_MODULES(PACKAGE, [libgnomeui-2.0 gtk+-2.0])\n"
	"AC_SUBST(PACKAGE_CFLAGS)\n"
	"AC_SUBST(PACKAGE_LIBS)",

	"1", 
	"1",
	"1",
	"0"
};

gchar* project_type_libglade2[] =
{
	"LIBGLADE2",
	
	" \\\n\t$(PACKAGE_CFLAGS)",
	" \\\n\t$(PACKAGE_LIBS)",
	
	"PKG_CHECK_MODULES(PACKAGE, [libgnomeui-2.0 gtk+-2.0 libglade-2.0])\n"
	"AC_SUBST(PACKAGE_CFLAGS)\n"
	"AC_SUBST(PACKAGE_LIBS)",
	
	"1", 
	"1",
	"1",
	"0"
};

gchar* project_type_gtkmm2[] =
{
	"GTKMM2",
	
	"\\\n\t$(PACKAGE_CFLAGS)",
	"\\\n\t$(PACKAGE_LIBS)",
	
	"PKG_CHECK_MODULES(PACKAGE, [gtkmm-2.0])\n"
	"AC_SUBST(PACKAGE_CFLAGS)\n"
	"AC_SUBST(PACKAGE_LIBS)",

	"0",
	"0", 
	"1",
	"0"
};

gchar* project_type_gnomemm2[] =
{
	"gnomemm 2",
	
	"\\\n\t$(PACKAGE_CFLAGS)",
	"\\\n\t$(PACKAGE_LIBS)",
	
	"PKG_CHECK_MODULES(PACKAGE, [libgnomeuimm-2.0])\n"
	"AC_SUBST(PACKAGE_CFLAGS)\n"
	"AC_SUBST(PACKAGE_LIBS)",
	
	"1", 
	"1",
	"1",
	"0"
};

gchar* project_type_bonobo2[] =
{
	"BONOBO2",
	
	" \\\n\t$(PACKAGE_CFLAGS)",
	" \\\n\t$(PACKAGE_LIBS)",
	
	"PKG_CHECK_MODULES(PACKAGE, [libgnomeui-2.0 gtk+-2.0 libbonoboui-2.0])\n"
	"AC_SUBST(PACKAGE_CFLAGS)\n"
	"AC_SUBST(PACKAGE_LIBS)",

	"1",
	"1", 
	"1",
	"0"
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
	"\n\n",

	"0",
	"0",
	"0",
	"0"
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

	"0",
	"0",
	"0",
	"0"
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

	"0",
	"0",
	"0",
	"0"
};

static ProjectType* load_type_from_data (char* type_data[], gint id);

ProjectType*
project_type_load (gint id)
{
	ProjectType* type;
	
	switch (id)
	{
		case PROJECT_TYPE_GENERIC:
		{
			type = load_type_from_data (project_type_generic, id);
			break;
		}
		case PROJECT_TYPE_GTK2:
		{
			type = load_type_from_data (project_type_gtk2, id);
			break;
		}
		case PROJECT_TYPE_GTKMM2:
		{
			type = load_type_from_data (project_type_gtkmm2, id);
			break;
		}
		case PROJECT_TYPE_GNOME2:
		{
			type = load_type_from_data (project_type_gnome2, id);
			break;
		}
		case PROJECT_TYPE_LIBGLADE2:
		{
			type = load_type_from_data (project_type_libglade2, id);
			break;
		}
		case PROJECT_TYPE_GNOMEMM2:
		{
			type = load_type_from_data (project_type_gnomemm2, id);
			break;
		}
		case PROJECT_TYPE_BONOBO2:
		{
			type = load_type_from_data (project_type_bonobo2, id);
			break;
		}
		case PROJECT_TYPE_WXWIN:
		{
			type = load_type_from_data (project_type_wxwin, id);
			break;
		}
		case PROJECT_TYPE_XWIN:
		{
			type = load_type_from_data (project_type_xwin, id);
			break;
		}
		case PROJECT_TYPE_XWINDOCKAPP:
		{
			type = load_type_from_data (project_type_xwindockapp, id);
			break;
		}
		default:
		{
			anjuta_error ("Unknown project type!");
			return NULL;
		}
	}
	return type;
}

void
project_type_free (ProjectType* type)
{
	g_free (type);
}

static ProjectType*
load_type_from_data (char* type_data[], gint id)
{
	ProjectType* type = g_new0 (ProjectType, 1);
	
	type->id = id;
	type->name = project_type_map[id];
	type->save_string = type_data[0];
	
	type->cflags = type_data[1];
	type->ldadd = type_data[2];
		
	type->configure_macros = type_data[3];
			
	type->gnome_support = atoi (type_data[4]);
	type->gnome2_support = atoi (type_data[5]);
	type->glade_support = atoi (type_data[6]);
	type->deprecated = atoi (type_data[7]);
	return type;
}
