/* 
    pixmaps.h
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

/* 
    This file contains definitions of all different images used by
    Anjuta IDE. New graphics should be defined here and referenced via
	macros in source files.
	
	Helper functions may be found in resources.c
*/

#ifndef _PIXMAPS_H_
#define _PIXMAPS_H_

/* the main app artwork */
#define ANJUTA_PIXMAP_LOGO                "anjuta_logo1.png"
#define ANJUTA_PIXMAP_ICON                "anjuta_icon.png"
#define ANJUTA_PIXMAP_SPLASH_SCREEN       "anjuta_splash.png"

#define ANJUTA_PIXMAP_NEW_FOLDER          "new_folder.xpm"
#define ANJUTA_PIXMAP_CLOSE_FILE_SMALL    "close_file_small.xpm"
#define ANJUTA_PIXMAP_PROJECT             "project.xpm"
#define ANJUTA_PIXMAP_NEW_PROJECT         "new_project.png"
#define ANJUTA_PIXMAP_OPEN_PROJECT        "open_project.png"
#define ANJUTA_PIXMAP_SAVE_PROJECT        "save_project.png"
#define ANJUTA_PIXMAP_CLOSE_PROJECT       "close_project.png"
#define ANJUTA_PIXMAP_PROJECT_LISTING     "project.xpm"
#define ANJUTA_PIXMAP_BREAKPOINT          "breakpoint.xpm"
#define ANJUTA_PIXMAP_WATCH               "watch.xpm"
#define ANJUTA_PIXMAP_REGISTERS           "registers.xpm"
#define ANJUTA_PIXMAP_STACK               "stack.xpm"
#define ANJUTA_PIXMAP_INSPECT             "inspect.xpm"
#define ANJUTA_PIXMAP_FRAME               "frame.xpm"
#define ANJUTA_PIXMAP_INTERRUPT           "interrupt.xpm"
#define ANJUTA_PIXMAP_COMPILE             "compile.xpm"
#define ANJUTA_PIXMAP_CONFIGURE           "configure.xpm"
#define ANJUTA_PIXMAP_BUILD               "build.xpm"
#define ANJUTA_PIXMAP_DEBUG               "debug.xpm"
#define ANJUTA_PIXMAP_BUILD_ALL           "build_all.xpm"
#define ANJUTA_PIXMAP_HELP                "help.xpm"
#define ANJUTA_PIXMAP_DOCK                "dock.xpm"
#define ANJUTA_PIXMAP_CONTINUE            "continue.xpm"
#define ANJUTA_PIXMAP_RUN_TO_CURSOR       "run_to_cursor.xpm"
#define ANJUTA_PIXMAP_STEP_IN             "step_in.xpm"
#define ANJUTA_PIXMAP_STEP_OVER           "step_over.xpm"
#define ANJUTA_PIXMAP_STEP_OUT            "step_out.xpm"
#define ANJUTA_PIXMAP_DEBUG_STOP          "debug_stop.xpm"
#define ANJUTA_PIXMAP_POINTER             "pointer.xpm"
#define ANJUTA_PIXMAP_WIZARD              "wizard.xpm"
#define ANJUTA_PIXMAP_PASSWORD            "password.png"

/* used for the messages window buttons */
#define ANJUTA_PIXMAP_MINI_BUILD          "mini_build.xpm"
#define ANJUTA_PIXMAP_MINI_DEBUG          "mini_debug.xpm"
#define ANJUTA_PIXMAP_MINI_FIND           "mini_find.xpm"
#define ANJUTA_PIXMAP_MINI_CVS            "mini_cvs.xpm"
#define ANJUTA_PIXMAP_MINI_LOCALS         "mini_locals.xpm"
#define ANJUTA_PIXMAP_MINI_TERMINAL       "mini_term.xpm"
#define ANJUTA_PIXMAP_MINI_DOCK           "mini_dock.xpm"
#define ANJUTA_PIXMAP_MINI_MODULES        "mini_modules.xpm"

/* definitions for the project wizard */
#define ANJUTA_PIXMAP_APPWIZ_WATERMARK    "appwizard.png"
#define ANJUTA_PIXMAP_APPWIZ_LOGO         "applogo.png"
#define ANJUTA_PIXMAP_APP_COMPONENT       "appwiz_bonobo.png"
#define ANJUTA_PIXMAP_APP_GENERIC         "appwiz_terminal.png"
#define ANJUTA_PIXMAP_APP_GTK             "appwiz_gtk.png"
#define ANJUTA_PIXMAP_APP_GNOME           "appwiz_gnome.png"
#define ANJUTA_PIXMAP_APP_GTKMM           "appwiz_gtkmm.png"
#define ANJUTA_PIXMAP_APP_GNOMEMM         "appwiz_gnomemm.png"
#define ANJUTA_PIXMAP_APP_LIBGLADE        "appwiz_libglade.png"
#define ANJUTA_PIXMAP_APP_LIBGLADE2       "appwiz_libglade2.png"
#define ANJUTA_PIXMAP_APP_WXWIN           "appwiz_wxwin.png"
#define ANJUTA_PIXMAP_APP_GTK2            "appwiz_gtk2.png"
#define ANJUTA_PIXMAP_APP_GTKMM2          "appwiz_gtkmm2.png"
#define ANJUTA_PIXMAP_APP_GNOME2          "appwiz_gnome2.png"
#define ANJUTA_PIXMAP_APP_GNOMEMM2        "appwiz_gnomemm2.png"
#define ANJUTA_PIXMAP_APP_OPENGL          "appwiz_opengl.png"
#define ANJUTA_PIXMAP_APP_SDL             "appwiz_sdl.png"
#define ANJUTA_PIXMAP_APP_XWIN            "appwiz_xlib.png"
#define ANJUTA_PIXMAP_APP_XWINDOCKAPP     "appwiz_xlib.png"
#define ANJUTA_PIXMAP_APP_QT              "appwiz_terminal.png"

/* Common */
#define ANJUTA_PIXMAP_CLOSED_FOLDER       "directory.png"
#define ANJUTA_PIXMAP_OPEN_FOLDER         "directory-accept.png"

#endif
