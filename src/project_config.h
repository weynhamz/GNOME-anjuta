/* 
    project_config.h
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
#ifndef _PROJECT_CONFIG_H_
#define _PROJECT_CONFIG_H_

#include <gnome.h>
#include "properties.h"

typedef struct _ProjectConfig ProjectConfig;
typedef struct _ProjectConfigWidgets ProjectConfigWidgets;

enum
{
	BUILD_FILE_CONFIGURE_IN,
	BUILD_FILE_TOP_MAKEFILE_AM,
	BUILD_FILE_SOURCE,
	BUILD_FILE_INCLUDE,
	BUILD_FILE_HELP,
	BUILD_FILE_PIXMAP,
	BUILD_FILE_DATA,
	BUILD_FILE_DOC,
	BUILD_FILE_PO,
	BUILD_FILE_END_MARK
};
	
struct _ProjectConfigWidgets
{
	GtkWidget* window;
	GtkWidget* disable_overwrite_check[BUILD_FILE_END_MARK];

	GtkWidget* description_text;
	GtkWidget* config_progs_text;
	GtkWidget* config_libs_text;
	GtkWidget* config_headers_text;
	GtkWidget* config_characteristics_text;
	GtkWidget* config_lib_funcs_text;
	GtkWidget* config_additional_text;
	GtkWidget* config_files_text;

	GtkWidget* extra_modules_before_entry;
	GtkWidget* extra_modules_after_entry;
	GtkWidget* makefile_am_text;
};

struct _ProjectConfig
{
	ProjectConfigWidgets widgets;
	
	gboolean blocked;
	
	gboolean disable_overwrite[BUILD_FILE_END_MARK];

	gchar* description;
	gchar* config_progs;
	gchar* config_libs;
	gchar* config_headers;
	gchar* config_characteristics;
	gchar* config_lib_funcs;
	gchar* config_additional;
	gchar* config_files;

	gchar* extra_modules_before;
	gchar* extra_modules_after;
	gchar* makefile_am;

	gboolean is_showing;
	gint win_pos_x, win_pos_y, win_width, win_height;
};

ProjectConfig* project_config_new (void);
void project_config_destroy (ProjectConfig* pc);
void project_config_clear (ProjectConfig* pc);
void project_config_show( ProjectConfig *pc);
void project_config_hide( ProjectConfig *pc);
gboolean project_config_save_yourself (ProjectConfig * pc, FILE* stream);
gboolean project_config_load_yourself (ProjectConfig * pc, PropsID props);

void project_config_set_disable_overwrite (ProjectConfig* pc, gint build_file, gboolean disable);
void project_config_set_disable_overwrite_all (ProjectConfig* pc, gboolean disable);

/* This puts the value in the struct into the widgets */
void project_config_sync( ProjectConfig *pc);

/* This gets the value in the widgets into the struct */
void project_config_get ( ProjectConfig *pc);

/* Saves everything except the scripts in prj file */
gboolean project_config_save (ProjectConfig * pc, FILE* stream);

/* Loads everything except the scripts. */
gboolean project_config_load (ProjectConfig * pc, PropsID props);

/* Load the scripts from buffer. */
/* Returns -1 on error and no. of chars read if successful */
gint project_config_load_scripts (ProjectConfig * pc, gchar* buff);

/* Save the scripts in project file. */
gboolean project_config_save_scripts (ProjectConfig * pc, FILE* stream);

/* Write the scripts in a executable file. */
/* usualy in configur.in */
gboolean project_config_write_scripts (ProjectConfig * pc, FILE* stream);

#define PROJECT_DESCRIPTION_START "<PROJECT_DESCRIPTION_START>\n"
#define PROJECT_DESCRIPTION_END "<PROJECT_DESCRIPTION_END>\n"

#define CONFIG_PROGS_START "<CONFIG_PROGS_START>\n"
#define CONFIG_PROGS_END "<CONFIG_PROGS_END>\n"

#define CONFIG_LIBS_START "<CONFIG_LIBS_START>\n"
#define CONFIG_LIBS_END "<CONFIG_LIBS_END>\n"

#define CONFIG_HEADERS_START "<CONFIG_HEADERS_START>\n"
#define CONFIG_HEADERS_END "<CONFIG_HEADERS_END>\n"

#define CONFIG_CHARACTERISTICS_START "<CONFIG_CHARACTERISTICS_START>\n"
#define CONFIG_CHARACTERISTICS_END "<CONFIG_CHARACTERISTICS_END>\n"

#define CONFIG_LIB_FUNCS_START "<CONFIG_LIB_FUNCS_START>\n"
#define CONFIG_LIB_FUNCS_END "<CONFIG_LIB_FUNCS_END>\n"

#define CONFIG_ADDITIONAL_START "<CONFIG_ADDITIONAL_START>\n"
#define CONFIG_ADDITIONAL_END "<CONFIG_ADDITIONAL_END>\n"

#define CONFIG_FILES_START "<CONFIG_FILES_START>\n"
#define CONFIG_FILES_END "<CONFIG_FILES_END>\n"

#define MAKEFILE_AM_START "<MAKEFILE_AM_START>\n"
#define MAKEFILE_AM_END "<MAKEFILE_AM_END>\n"

#endif
