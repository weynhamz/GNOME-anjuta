/*
    compiler_options.h
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
#ifndef _COMPILER_OPTIONS_H_
#define _COMPILER_OPTIONS_H_

#include <gnome.h>
#include "properties.h"

typedef struct _CompilerOptions CompilerOptions;
typedef struct _CompilerOptionsGui CompilerOptionsGui;
enum
{
	ANJUTA_SUPPORT_ID,
	ANJUTA_SUPPORT_DESCRIPTION,
	ANJUTA_SUPPORT_DEPENDENCY,
	ANJUTA_SUPPORT_MACROS,
	ANJUTA_SUPPORT_PRJ_CFLAGS,
	ANJUTA_SUPPORT_PRJ_LIBS,
	ANJUTA_SUPPORT_FILE_CFLAGS,
	ANJUTA_SUPPORT_FILE_LIBS,
	ANJUTA_SUPPORT_ACCONFIG_H,
	ANJUTA_SUPPORT_INSTALL_STATUS,
	ANJUTA_SUPPORT_END_MARK
};
	
struct _CompilerOptionsGui
{
	gint ref_count;

	GtkWidget *window;

	GtkWidget *supp_clist;
	GtkWidget *inc_clist;
	GtkWidget *inc_entry;

	GtkWidget *lib_paths_clist;
	GtkWidget *lib_paths_entry;

	GtkWidget *lib_clist;
	GtkWidget *lib_stock_clist;
	GtkWidget *lib_entry;

	GtkWidget *def_clist;
	GtkWidget *def_stock_clist;
	GtkWidget *def_entry;

	GtkWidget *warnings_clist;
	GtkWidget *optimize_button[4];
	GtkWidget *other_button[2];

	GtkWidget *other_c_flags_entry;
	GtkWidget *other_l_flags_entry;
	GtkWidget *other_l_libs_entry;
};

struct _CompilerOptions
{
	CompilerOptionsGui widgets;
	gint supp_index;
	gint inc_index;
	gint lib_index;
	gint lib_paths_index;
	gint def_index;

	gboolean is_showing;
	gint win_pos_x, win_pos_y;

	/* This property database is not the one
	 * from which compiler options will be loaded
	 * or saved, but the one in which option
	 * variables will be set for the commands
	 * to use
	 */
	PropsID props;
};

extern gchar *anjuta_supports[][ANJUTA_SUPPORT_END_MARK];

CompilerOptions *compiler_options_new (PropsID props);
void compiler_options_destroy (CompilerOptions *);
void compiler_options_get (CompilerOptions *);
void compiler_options_clear (CompilerOptions *);
void compiler_options_sync (CompilerOptions *);
void compiler_options_show (CompilerOptions *);
void compiler_options_hide (CompilerOptions *);
gboolean compiler_options_save (CompilerOptions * co, FILE * s);
void compiler_options_load (CompilerOptions * co, PropsID props);
gboolean compiler_options_save_yourself (CompilerOptions * co, FILE * s);
gboolean compiler_options_load_yourself (CompilerOptions * co, PropsID props);
void compiler_options_update_controls (CompilerOptions *);
void compiler_options_set_prjincl_in_file (CompilerOptions * co, FILE* fp);
void compiler_options_set_prjcflags_in_file (CompilerOptions * co, FILE* fp);
void compiler_options_set_prjlflags_in_file (CompilerOptions * co, FILE* fp);
void compiler_options_set_prjlibs_in_file (CompilerOptions * co, FILE* fp);
void compiler_options_set_prjmacros_in_file (CompilerOptions * co, FILE* fp);

/* private */
void compiler_options_set_in_properties (CompilerOptions* co, PropsID props);

#endif
