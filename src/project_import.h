/*  
 *  project_import.h
 *  Copyright (C) 2002 Johannes Schmid
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef PROJECT_IMPORT_H
#define PROJECT_IMPORT_H

#include <gnome.h>

typedef struct _ProjectImportWizard ProjectImportWizard;
typedef struct _ProjectImportWizardGUI ProjectImportWizardGUI;
	
struct _ProjectImportWizardGUI
{
	GtkWidget *window;
	GtkWidget *druid;
	GtkWidget *page[7];
	
	// Page2:
	GtkWidget *file_entry;
	GtkWidget *label;
	GtkWidget *progressbar;
	
	// Page3:
	GtkWidget *iconlist;
	
	// Page4:
	GtkWidget *prj_name_entry;
	GtkWidget *author_entry;
	GtkWidget *version_entry;
	GtkWidget *target_entry;
	GtkWidget *language_c_radio;
	GtkWidget *language_cpp_radio;
	GtkWidget *language_c_cpp_radio;
	
	// Page5:
	GtkWidget *description_text;

	// Page6:
	GtkWidget *menu_frame;
	GtkWidget *menu_entry_entry;
	GtkWidget *menu_comment_entry;
	GtkWidget *icon_entry;
	GtkWidget *app_group_combo;
	GtkWidget *app_group_entry;
	GtkWidget *term_check;
	
	GtkWidget *gettext_support_check;
	GtkWidget *file_header_check;
};

struct _ProjectImportWizard
{
	gchar* filename;
	gchar* directory;
	
	gchar* prj_name;
	gint prj_type;
	gchar* prj_version;
	gchar* prj_source_target;
	gchar* prj_author;
	gchar* prj_programming_language;
	gboolean prj_gpl;
	gboolean prj_has_gettext;
	gchar* prj_menu_entry;
	gchar* prj_menu_group;
	gchar* prj_menu_comment;
	gchar* prj_menu_icon;
	gboolean prj_menu_need_terminal;
	gchar* prj_description;
	
	ProjectImportWizardGUI widgets;
};

extern ProjectImportWizard* piw;

void create_project_import_gui (void);
void destroy_project_import_gui (void);
gboolean project_import_start (const gchar *topleveldir,
							   ProjectImportWizard *piw);
void project_import_save_values (ProjectImportWizard *piw);

#endif // PROJECT_IMPORT_H
