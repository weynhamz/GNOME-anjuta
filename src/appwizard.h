/*
    appwizard.h
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

#ifndef _APP_WIZARD_H_
#define _APP_WIZARD_H_

#include "project_dbase.h"
typedef struct _AppWizardGui AppWizardGui;
typedef struct _AppWizard AppWizard;

struct _AppWizardGui
{
	GtkWidget *window;
	GtkWidget *druid;
	GtkWidget *page[6];

	GtkWidget *icon_list;
	GtkWidget *target_type_entry;
	
	GtkWidget *prj_name_entry;
	GtkWidget *author_entry;
	GtkWidget *version_entry;
	GtkWidget *target_entry;
	GtkWidget *language_c_radio;
	GtkWidget *language_cpp_radio;
	GtkWidget *language_c_cpp_radio;
	
	GtkWidget *description_text;
	GtkWidget *target_exec_radio;
	GtkWidget *target_slib_radio;
	GtkWidget *target_dlib_radio;

	GtkWidget *gettext_support_check;
	GtkWidget *file_header_check;

	GtkWidget *menu_frame;
	GtkWidget *menu_entry_entry;
	GtkWidget *menu_comment_entry;
	GtkWidget *icon_entry;
	GtkWidget *app_group_combo;
	GtkWidget *app_group_entry;
	GtkWidget *term_check;
	
};

struct _AppWizard
{
	AppWizardGui widgets;
	gint cur_page;

	gint  prj_type;
	GList *active_project_types;
	gint  target_type;
	
	gchar *prj_name;
	gchar *target;
	gchar *author;
	gchar *version;
	gint language;

	gchar *description;

	gchar *app_group;
	gchar *menu_entry;
	gchar *menu_comment;
	gchar *icon_file;
	gboolean need_terminal;

	gboolean gettext_support;
	gboolean use_header;
};

AppWizard *app_wizard_new (void);
void app_wizard_proceed (void);
void app_wizard_destroy (AppWizard * aw);

/* Private */
void create_app_wizard_gui (AppWizard * aw);
void create_app_wizard_page1 (AppWizard * aw);
void create_app_wizard_page2 (AppWizard * aw);
void create_app_wizard_page3 (AppWizard * aw);
void create_app_wizard_page4 (AppWizard * aw);

#endif
