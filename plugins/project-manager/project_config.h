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
#include <glade/glade.h>
#include "properties.h"

#define PROJECT_DESCRIPTION_START           "<PROJECT_DESCRIPTION_START>\n"
#define PROJECT_DESCRIPTION_END             "<PROJECT_DESCRIPTION_END>\n"

#define CONFIG_PROGS_START                  "<CONFIG_PROGS_START>\n"
#define CONFIG_PROGS_END                    "<CONFIG_PROGS_END>\n"

#define CONFIG_LIBS_START                   "<CONFIG_LIBS_START>\n"
#define CONFIG_LIBS_END                     "<CONFIG_LIBS_END>\n"

#define CONFIG_HEADERS_START                "<CONFIG_HEADERS_START>\n"
#define CONFIG_HEADERS_END                  "<CONFIG_HEADERS_END>\n"

#define CONFIG_CHARACTERISTICS_START        "<CONFIG_CHARACTERISTICS_START>\n"
#define CONFIG_CHARACTERISTICS_END          "<CONFIG_CHARACTERISTICS_END>\n"

#define CONFIG_LIB_FUNCS_START              "<CONFIG_LIB_FUNCS_START>\n"
#define CONFIG_LIB_FUNCS_END                "<CONFIG_LIB_FUNCS_END>\n"

#define CONFIG_ADDITIONAL_START             "<CONFIG_ADDITIONAL_START>\n"
#define CONFIG_ADDITIONAL_END               "<CONFIG_ADDITIONAL_END>\n"

#define CONFIG_FILES_START                  "<CONFIG_FILES_START>\n"
#define CONFIG_FILES_END                    "<CONFIG_FILES_END>\n"

#define MAKEFILE_AM_START                   "<MAKEFILE_AM_START>\n"
#define MAKEFILE_AM_END                     "<MAKEFILE_AM_END>\n"

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

typedef struct _ProjectConfigPriv ProjectConfigPriv;
struct _ProjectConfig
{
	ProjectConfigPriv *priv;
};

ProjectConfig* project_config_new (PropsID props);
void project_config_destroy (ProjectConfig *pc);
void project_config_clear (ProjectConfig *pc);
void project_config_show( ProjectConfig *pc);
void project_config_hide( ProjectConfig *pc);

gboolean project_config_save_yourself (ProjectConfig *pc, FILE *stream);
gboolean project_config_load_yourself (ProjectConfig *pc, PropsID props);

void project_config_set_disable_overwrite (ProjectConfig* pc,
										   gint build_file, gboolean disable);
void project_config_set_disable_overwrite_all (ProjectConfig* pc,
											   gboolean disable);

/* This puts the value in the struct into the widgets */
void project_config_sync (ProjectConfig *pc);

/* Gets the project description */
gchar *project_config_get_description (ProjectConfig *pc);

/* Sets the project description */
void project_config_set_description (ProjectConfig *pc, const gchar *desc);

/* Gets the space separated list of config files */
gchar * project_config_get_config_files (ProjectConfig *pc);

/* Gets the content of top level Makefile.am */
gchar * project_config_get_makefile_am_content (ProjectConfig *pc);

/* Gets the file overwrite-diable flag */
gboolean project_config_get_overwrite_disabled (ProjectConfig *pc, int file_id);

/* Returns true if the config is blocked from writing all file */
gboolean project_config_is_blocked (ProjectConfig *pc);

/* This gets the value in the widgets into the struct */
void project_config_get (ProjectConfig *pc);

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

#endif
