/* 
    project_dbase.h
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
#ifndef _PROJECT_DBASE_H_
#define _PROJECT_DBASE_H_

#include "tags_manager.h"
#include "properties.h"
#include "project_config.h"
#include "project_type.h"
#include "project_type.h"

#include "tm_tagmanager.h"

#define COMPATIBILITY_LEVEL    1

typedef enum _ProjectType ProjectType;
typedef struct _ProjectDBase ProjectDBase;
typedef struct _ProjectDBaseGui ProjectDBaseGui;
typedef struct _ProjectFileData ProjectFileData;
typedef enum _PrjModule PrjModule;

/* Do not break the sequence of the following enums.*/
/* only append */
enum _PrjModule
{
	MODULE_INCLUDE, 
	MODULE_SOURCE,
	MODULE_PIXMAP,
	MODULE_DATA,
	MODULE_HELP,
	MODULE_DOC,
	MODULE_PO,
	MODULE_END_MARK
};

/* Do not break the sequence of the following enums.*/
/* only append */
enum
{
	PROJECT_TYPE_GENERIC,
	PROJECT_TYPE_GTK,
	PROJECT_TYPE_GNOME,
	PROJECT_TYPE_BONOBO,
	PROJECT_TYPE_GTKMM,
	PROJECT_TYPE_GNOMEMM,
	PROJECT_TYPE_LIBGLADE,
	PROJECT_TYPE_END_MARK
};

/* Do not break the sequence of the following enums.*/
/* only append */
enum
{
	PROJECT_TARGET_TYPE_EXECUTABLE,
	PROJECT_TARGET_TYPE_STATIC_LIB,
	PROJECT_TARGET_TYPE_DYNAMIC_LIB,
	PROJECT_TARGET_TYPE_END_MARK
};

/* Do not break the sequence of the following enums.*/
/* only append */
enum
{
	PROJECT_PROGRAMMING_LANGUAGE_C,
	PROJECT_PROGRAMMING_LANGUAGE_CPP,
	PROJECT_PROGRAMMING_LANGUAGE_C_CPP,
	PROJECT_PROGRAMMING_LANGUAGE_END_MARK
};

struct _ProjectFileData
{
	PrjModule module;
	gchar *filename;
	GtkCTreeNode* parent_node;
};

struct _ProjectDBaseGui
{
	GtkWidget *window;
	GtkWidget *client_area;
	GtkWidget *client;
	GtkWidget *notebook; 
	GtkWidget *scrolledwindow;
	GtkWidget *ctree;
	GtkWidget *menu;
	GtkWidget *menu_import;
	GtkWidget *menu_view;
	GtkWidget *menu_edit;
	GtkWidget *menu_remove;
	GtkWidget *menu_configure;
	GtkWidget *menu_info;

	/* Ctree nodes and info */
	GtkCTreeNode *root_node;
	GtkCTreeNode *module_node[MODULE_END_MARK];
	GtkCTreeNode *current_node;
};

struct _ProjectDBase
{
	glong	size;	/* sizeof() used as version # for components */
	ProjectDBaseGui widgets;
	GtkWidget *fileselection_open;
	GtkWidget *fileselection_add_file;
	
	ProjectConfig* project_config;
	gboolean project_is_open;

	guint props;

	gchar *top_proj_dir;
	TMWorkObject *tm_project;
	gchar *proj_filename;
	gboolean is_saved;
	gboolean is_showing;
	gboolean is_docked;
	gint win_pos_x, win_pos_y, win_width, win_height;

	/* Current node info */
	ProjectFileData* current_file_data;

	/* Private */
	gdouble progress_state;
	PrjModule sel_module;
	gboolean	m_prj_ShowLocal;	/* Cfg to show local variables */
	GList* excluded_modules;
};

extern gchar* module_map[];
extern gchar* project_type_map[];
extern gchar* project_target_type_map[];
extern gchar* programming_language_map[];

/* File data to be set with the project tree nodes */
ProjectFileData *
project_file_data_new (GtkCTreeNode * parent, PrjModule mod, gchar * full_fname);

void
project_file_data_destroy (ProjectFileData * pfd);

/* Project data base system */
void create_project_dbase_gui (ProjectDBase* p);
GtkWidget * create_project_dbase_info_gui (gchar * lab[]);
GtkWidget * create_project_confirm_dlg (void);

ProjectDBase *
project_dbase_new (PropsID pr_props);

void
project_dbase_destroy (ProjectDBase * p);

void
project_dbase_clear (ProjectDBase * p);

void
project_dbase_show (ProjectDBase * p);

void
project_dbase_hide (ProjectDBase * p);

void
project_dbase_open_project (ProjectDBase * p);

gboolean project_dbase_load_project (ProjectDBase * p, gboolean show_project);

void
project_dbase_close_project (ProjectDBase * p);

gboolean project_dbase_save_project (ProjectDBase * p);

void project_dbase_update_tags_image(ProjectDBase* p);

gboolean project_dbase_save_yourself (ProjectDBase * p, FILE * stream);

gboolean project_dbase_load_yourself (ProjectDBase * p, PropsID props);

void project_dbase_set_show_locals( ProjectDBase * p,  const gboolean bActive );

void
project_dbase_dock (ProjectDBase * p);

void
project_dbase_undock (ProjectDBase * p);

/* Checks if the given file belongs to the given module */
gboolean
project_dbase_is_file_in_module (ProjectDBase * p, PrjModule module, gchar * file);

/* Show information of the project */
void
project_dbase_show_info (ProjectDBase * p);

/* Starts glade for gnome projects */
gboolean
project_dbase_summon_glade (ProjectDBase *p);

/* Write source code from glade file */
gboolean
project_dbase_generate_source_code (ProjectDBase *p);

/* Name of the project */
/* Free the returned string when not required */
gchar *
project_dbase_get_proj_name (ProjectDBase * p);

/* project type. */
Project_Type*
project_dbase_get_project_type (ProjectDBase* p);

/* Target type*/
gint
project_dbase_get_target_type (ProjectDBase* p);

/* Project language */
gint
project_dbase_get_language (ProjectDBase* p);

/* Version of the project. */
/* Free the returned string when not required */
gchar *
project_dbase_get_version (ProjectDBase * p);

/* Target name of the project. eg executable name, lib name etc */
/* Free the returned string when not required */
gchar *
project_dbase_get_source_target (ProjectDBase *p);

/* Optional parameter associated with each module. */
/* right now only used for MODULE_SOURCE and MODULE_HELP */
/* Free the returned string when not required */
gchar *
project_dbase_get_module_type (ProjectDBase *p, PrjModule module);

/* Full path name of the given module */
/* Free the returned string when not required */
gchar *
project_dbase_get_module_dir (ProjectDBase * p, PrjModule module);

/* Subdir name of the module under the top level project dir */
/* Free the returned string when not required */
gchar *
project_dbase_get_module_name (ProjectDBase * p, PrjModule module);

/* Free the returned list of strings using glist_string_free(lilst) */
/* function when not required */
GList*
project_dbase_get_module_files (ProjectDBase * p, PrjModule module);

/* is the given module empty*/
gboolean
project_dbase_module_is_empty (ProjectDBase * p, PrjModule module);

/* This function returns all available files in the given module */
/* In the module dir (not just the added files */
/* if (with_makefiles) is true, the makefiles are also included */
GList*
project_dbase_scan_files_in_module(ProjectDBase* p, PrjModule module, gboolean with_makefiles);

/* Clear the project database and restore the Anjuta */
/* in file mode. */
void
project_dbase_clean_left (ProjectDBase * p);

/*
 * Private functions: Do not use
 */

/* Add the given file to the given module */
/* No checking is performed wether the file exist or not */
/* it is simply added */
void
project_dbase_add_file_to_module (ProjectDBase * p, PrjModule module, gchar * filename);

/* remove file form the module */
void
project_dbase_remove_file (ProjectDBase * p);

void
project_dbase_detach (ProjectDBase * p);

void
project_dbase_attach (ProjectDBase * p);

void
project_dbase_update_menu (ProjectDBase * p);

void
project_dbase_update_tree (ProjectDBase * p);

void
project_dbase_update_controls (ProjectDBase * pd);

/* reference */
gchar*
project_dbase_get_dir (ProjectDBase * p);

/* copy of string */
gchar*
project_dbase_get_name (ProjectDBase * p);

/* Callback signals */

/* Project open file selection */
void
on_open_prjfilesel_ok_clicked (GtkButton * button, gpointer user_data);
void
on_open_prjfilesel_cancel_clicked (GtkButton * button, gpointer user_data);

/* Add file to project fileselection */
void
on_add_prjfilesel_cancel_clicked (GtkButton * button, gpointer user_data);
void
on_add_prjfilesel_ok_clicked (GtkButton * button, gpointer user_data);

/* Menu callbacks */
void
on_project_add_new1_activate (GtkMenuItem * menuitem, gpointer user_data);
void
on_project_view1_activate (GtkMenuItem * menuitem, gpointer user_data);
void
on_project_edit1_activate (GtkMenuItem * menuitem, gpointer user_data);
void
on_project_remove1_activate (GtkMenuItem * menuitem, gpointer user_data);
void
on_project_configure1_activate (GtkMenuItem * menuitem, gpointer user_data);
void
on_project_project_info1_activate (GtkMenuItem * menuitem, gpointer user_data);
void
on_project_dock_undock1_activate (GtkMenuItem * menuitem, gpointer user_data);
void
on_project_help1_activate (GtkMenuItem * menuitem, gpointer user_data);
void
on_project_include_file1_activate (GtkMenuItem * menuitem, gpointer user_data);
void
on_project_source_file1_activate (GtkMenuItem * menuitem, gpointer user_data);
void
on_project_help_file1_activate (GtkMenuItem * menuitem, gpointer user_data);
void
on_project_data_file1_activate (GtkMenuItem * menuitem, gpointer user_data);
void
on_project_pixmap_file1_activate (GtkMenuItem * menuitem, gpointer user_data);
void
on_project_translation_file1_activate (GtkMenuItem * menuitem, gpointer user_data);
void
on_project_doc_file1_activate (GtkMenuItem * menuitem, gpointer user_data);

#endif

