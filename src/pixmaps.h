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
    This file contains definitions for all of the different images used by
    the Anjuta IDE. New graphics should be defined here and referenced via
	the macro in any source files (which will need to include this header)
	
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
#define ANJUTA_PIXMAP_SYNTAX              "syntax.xpm"
#define ANJUTA_PIXMAP_PAGE_SETUP          "page_setup.xpm"
#define ANJUTA_PIXMAP_FIND_IN_FILES       "find_in_files.xpm"
#define ANJUTA_PIXMAP_MESSAGES            "messages.xpm"
#define ANJUTA_PIXMAP_PROJECT_LISTING     "project.xpm"
#define ANJUTA_PIXMAP_BREAKPOINT          "breakpoint.xpm"
#define ANJUTA_PIXMAP_WATCH               "watch.xpm"
#define ANJUTA_PIXMAP_REGISTERS           "registers.xpm"
#define ANJUTA_PIXMAP_STACK               "stack.xpm"
#define ANJUTA_PIXMAP_INSPECT             "inspect.xpm"
#define ANJUTA_PIXMAP_FRAME               "frame.xpm"
#define ANJUTA_PIXMAP_INTERRUPT           "interrupt.xpm"
#define ANJUTA_PIXMAP_CONSOLE             "console.xpm"
#define ANJUTA_PIXMAP_COMPILE             "compile.xpm"
#define ANJUTA_PIXMAP_CONFIGURE           "configure.xpm"
#define ANJUTA_PIXMAP_BUILD               "build.xpm"
#define ANJUTA_PIXMAP_DEBUG               "debug.xpm"
#define ANJUTA_PIXMAP_BUILD_ALL           "build_all.xpm"
#define ANJUTA_PIXMAP_GREP                "grep.xpm"
#define ANJUTA_PIXMAP_COMPARE             "compare.xpm"
#define ANJUTA_PIXMAP_DIFFERENCE          "difference.xpm"
#define ANJUTA_PIXMAP_FILE_VIEW           "file_view.xpm"
#define ANJUTA_PIXMAP_INDENT              "indent.xpm"
#define ANJUTA_PIXMAP_FLOW                "flow.xpm"
#define ANJUTA_PIXMAP_REFERENCE           "reference.xpm"
#define ANJUTA_PIXMAP_TRACE               "trace.xpm"
#define ANJUTA_PIXMAP_ARCHIVE             "archive.xpm"
#define ANJUTA_PIXMAP_HELP                "help.xpm"
#define ANJUTA_PIXMAP_CONTENTS            "contents.xpm"
#define ANJUTA_PIXMAP_INDEX               "index.xpm"
#define ANJUTA_PIXMAP_MAN_PAGE            "man_page.xpm"
#define ANJUTA_PIXMAP_DOCK                "dock.xpm"
#define ANJUTA_PIXMAP_UNDOCK              "undock.png"
#define ANJUTA_PIXMAP_CONTINUE            "continue.xpm"
#define ANJUTA_PIXMAP_RUN_TO_CURSOR       "run_to_cursor.xpm"
#define ANJUTA_PIXMAP_STEP_IN             "step_in.xpm"
#define ANJUTA_PIXMAP_STEP_OVER           "step_over.xpm"
#define ANJUTA_PIXMAP_STEP_OUT            "step_out.xpm"
#define ANJUTA_PIXMAP_DEBUG_STOP          "debug_stop.xpm"
#define ANJUTA_PIXMAP_POINTER             "pointer.xpm"
#define ANJUTA_PIXMAP_WIZARD              "wizard.xpm"
#define ANJUTA_PIXMAP_PASSWORD            "password.png"

#define ANJUTA_PIXMAP_PRINT_LAYOUT        "print_layout.xpm"
#define ANJUTA_PIXMAP_PRINT_PORTRAIT      "print_portrait.xpm"
#define ANJUTA_PIXMAP_PRINT_LANDSCAPE     "print_landscape.xpm"
#define ANJUTA_PIXMAP_PRINT_COLOR         "print_color.xpm"
#define ANJUTA_PIXMAP_PRINT_NOCOLOR       "print_nocolor.xpm"

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
#define ANJUTA_PIXMAP_APPWIZ_GEAR         "appwid_gear.xpm"
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

#define ANJUTA_PIXMAP_RED_LED             "ledred.xpm"
#define ANJUTA_PIXMAP_GREEN_LED           "ledgreen.xpm"

#define ANJUTA_PIXMAP_BOOKMARK_TOGGLE     "bookmark_toggle.xpm"
#define ANJUTA_PIXMAP_BOOKMARK_FIRST      "bookmark-first.png"
#define ANJUTA_PIXMAP_BOOKMARK_PREV       "bookmark-prev.png"
#define ANJUTA_PIXMAP_BOOKMARK_NEXT       "bookmark-next.png"
#define ANJUTA_PIXMAP_BOOKMARK_LAST       "bookmark-last.png"
#define ANJUTA_PIXMAP_ERROR_PREV          "error-prev.png"
#define ANJUTA_PIXMAP_ERROR_NEXT          "error-next.png"

#define ANJUTA_PIXMAP_FOLD_TOGGLE         "fold_toggle.xpm"
#define ANJUTA_PIXMAP_FOLD_CLOSE          "fold_close.xpm"
#define ANJUTA_PIXMAP_FOLD_OPEN           "fold_open.xpm"

#define ANJUTA_PIXMAP_BLOCK_SELECT        "block_select.xpm"
#define ANJUTA_PIXMAP_BLOCK_START         "block-start.png"
#define ANJUTA_PIXMAP_BLOCK_END           "block-end.png"

#define ANJUTA_PIXMAP_INDENT_INC          "indent_inc.xpm"
#define ANJUTA_PIXMAP_INDENT_DCR          "indent_dcr.xpm"
#define ANJUTA_PIXMAP_INDENT_AUTO         "indent_auto.xpm"
#define ANJUTA_PIXMAP_AUTOFORMAT_SETTING  "indent_set.xpm"

#define ANJUTA_PIXMAP_CALLTIP             "calltip.xpm"
#define ANJUTA_PIXMAP_AUTOCOMPLETE        "autocomplete.png"

/* Pixmaps for file and symbol browsers */
/* Symbol browser */
#define ANJUTA_PIXMAP_SV_UNKNOWN          "file_unknown.xpm"
#define ANJUTA_PIXMAP_SV_CLASS            "sv_class.xpm"
#define ANJUTA_PIXMAP_SV_FUNCTION         "sv_function.xpm"
#define ANJUTA_PIXMAP_SV_MACRO            "sv_macro.xpm"
#define ANJUTA_PIXMAP_SV_PRIVATE_FUN      "sv_private_fun.xpm"
#define ANJUTA_PIXMAP_SV_PRIVATE_VAR      "sv_private_var.xpm"
#define ANJUTA_PIXMAP_SV_PROTECTED_FUN    "sv_protected_fun.xpm"
#define ANJUTA_PIXMAP_SV_PROTECTED_VAR    "sv_protected_var.xpm"
#define ANJUTA_PIXMAP_SV_PUBLIC_FUN       "sv_public_fun.xpm"
#define ANJUTA_PIXMAP_SV_PUBLIC_VAR       "sv_public_var.xpm"
#define ANJUTA_PIXMAP_SV_STATIC_FUN       "sv_static_fun.xpm"
#define ANJUTA_PIXMAP_SV_STATIC_VAR       "sv_static_var.xpm"
#define ANJUTA_PIXMAP_SV_STRUCT           "sv_struct.xpm"
#define ANJUTA_PIXMAP_SV_VARIABLE         "sv_variable.xpm"

/* Common */
#define ANJUTA_PIXMAP_CLOSED_FOLDER       "cfolder.xpm"
#define ANJUTA_PIXMAP_OPEN_FOLDER         "ofolder.xpm"

/* File browser */
#define ANJUTA_PIXMAP_FV_UNKNOWN          "file_unknown.xpm"
#define ANJUTA_PIXMAP_FV_TEXT             "file_text.xpm"
#define ANJUTA_PIXMAP_FV_IMAGE            "file_pix.xpm"
#define ANJUTA_PIXMAP_FV_EXECUTABLE       "file_exec.xpm"
#define ANJUTA_PIXMAP_FV_CORE             "file_core.xpm"

/* Required by Project Manager */
#define ANJUTA_PIXMAP_EDIT                "file_text.xpm"
#define ANJUTA_PIXMAP_VIEW                "file_file.xpm"
#define ANJUTA_PIXMAP_INCLUDE             "file_h.xpm"
#define ANJUTA_PIXMAP_SOURCE              "file_cpp.xpm"
#define ANJUTA_PIXMAP_DATA                "file_unknown.xpm"
#define ANJUTA_PIXMAP_PIXMAP              "file_pix.xpm"
#define ANJUTA_PIXMAP_TRANSLATION         "file_unknown.xpm"
#define ANJUTA_PIXMAP_DOC                 "file_text.xpm"

/* Useful macros for specifying pixmaps in UIInfo structures */
/* #define PIX_FILE(F) GNOME_APP_PIXMAP_FILENAME, \
	PACKAGE_PIXMAPS_DIR "/" ANJUTA_PIXMAP_ ## F */
#define PIX_FILE(F) GNOME_APP_PIXMAP_NONE, NULL
#define PIX_STOCK(F) GNOME_APP_PIXMAP_STOCK, F
/* GNOME_STOCK_MENU_ ## F */

#endif
