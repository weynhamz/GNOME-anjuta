/*
 * anjuta.h Copyright (C) 2000  Kh. Naba Kumar Singh
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59 
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#ifndef _ANJUTA_H_
#define _ANJUTA_H_

#include <gmodule.h>

#include <glade/glade.h>
#include <gdl/gdl-icons.h>

#include "toolbar.h"
#include "text_editor.h"
#include "preferences.h"
#include "compiler_options.h"
#include "src_paths.h"
#include "find_replace.h"
#include "find_in_files.h"
#include "message-manager.h"
#include "project_dbase.h"
#include "commands.h"
#include "breakpoints.h"
#include "anjuta_dirs.h"
#include "executer.h"
#include "configurer.h"
#include "utilities.h"
#include "main_menubar.h"
#include "properties.h"
#include "help.h"
#include "cvs.h"
#include "style-editor.h"
#include "tm_tagmanager.h"
#include "file_history.h"
#include "windows-dialog.h"

#define g_strdup_printfs2(_FORMAT_, _STR_) \
	{ \
		assert(_STR_); g_strdup_printf(_FORMAT_, _STR_); \
	}
#define g_strdup_printfs3(_FORMAT_, _STR1_, _STR2_) \
	{ \
		assert(_STR1_); assert(_STR2_); \
		g_strdup_printf(_FORMAT_, _STR1_, _STR2_); \
	}

typedef struct _AnjutaAppGui AnjutaAppGui;
typedef struct _AnjutaApp AnjutaApp;
typedef struct _FileLineInfo FileLineInfo;

struct _AnjutaAppGui
{
	GtkWidget *window;
	GtkWidget *client_area;
	GtkWidget *the_client;
	GtkWidget *hpaned_client;
	GtkWidget *hpaned;
	GtkWidget *vpaned;
	GtkWidget *project_dbase_win_container;
	GtkWidget *mesg_win_container;
	GtkWidget *notebook;
	GtkWidget *appbar;

	Toolbar toolbar;
	MainMenuBar menubar;
};

struct _AnjutaApp
{
	/* sizeof() used as version # for components */
	glong	size;
	
	AnjutaAppGui widgets;
	GdlIcons *icon_set;
	GtkWidget *fileselection;
	GtkWidget *save_as_fileselection;
	GtkWidget *save_as_build_msg_sel;
	GList *text_editor_list;

	GtkAccelGroup *accel_group;
	FindAndReplace *find_replace;
	AnMessageManager *messages;
	ProjectDBase *project_dbase;
	CommandEditor *command_editor;
	AnjutaPreferences *preferences;
	CompilerOptions *compiler_options;
	SrcPaths *src_paths;
	TextEditor *current_text_editor;
	AnjutaDirs *dirs;
	Executer *executer;
	Configurer *configurer;
	FindInFiles *find_in_files;
	const TMWorkspace *tm_workspace;
	AnjutaHelp* help_system;
	CVS* cvs;
	StyleEditor *style_editor;
	
	GList *registered_windows;
	GList *registered_child_processes;
	GList *registered_child_processes_cb;
	GList *registered_child_processes_cb_data;
	GList *recent_files;
	GList *recent_projects;
	gint hpaned_width;
	gint vpaned_height;
	gint win_pos_x, win_pos_y, win_width, win_height;
	gboolean auto_gtk_update;

	GnomeAppProgressKey progress_key;
	gdouble progress_value;
	gdouble progress_full_value;
	GnomeAppProgressCancelFunc progress_cancel_cb;
	gboolean in_progress;
	gint busy_count;
	gboolean first_time_expose;
	
	/* If Devhelp is installed, Context Help should be enabled */
	gboolean has_devhelp;

	/* Dir where command executes */
	gchar *execution_dir;

	/* Current Job */
	/* This is not the command being executed, but the type of execution */
	/* eg, Debugger, Build, Compile, Find .. etc and NULL for no job */
	gchar* cur_job;
	
	/*
	 * Any object in the application wishing to access other object(s)
	 * should check for this flag to be FALSE. If it is TRUE, then there
	 * is no garrantee that the object to be accessed is still alive.
	 */
	gboolean shutdown_in_progress;
	
	/* Plugins */
	GList *addIns_list;
	
	/* To be set in preferences */
	gboolean b_reload_last_project;
	
	/* Last session open project file name if any */
	gchar *last_open_project;
};

struct _FileLineInfo
{
	gchar *file;
	glong line;
};

extern AnjutaApp *app;

/* Command line file arguments */
extern GList* command_args;

void create_anjuta_gui (AnjutaApp * appl);
void anjuta_connect_kernel_signals(void);
void anjuta_die_imidiately(void);
void anjuta_new (void);
void anjuta_show (void);
void anjuta_session_restore (GnomeClient* client);

GList *anjuta_get_file_list(void);
const GList *anjuta_get_tag_list(TextEditor *te, guint tag_types);

TextEditor *anjuta_append_text_editor (gchar * filename);

void anjuta_remove_text_editor (TextEditor* te);
void anjuta_remove_current_text_editor (void);

TextEditor *anjuta_get_current_text_editor (void);

void anjuta_set_current_text_editor (TextEditor * te);

GtkWidget *anjuta_get_current_text (void);

gchar *anjuta_get_current_selection (void);

TextEditor*  anjuta_goto_file_line (gchar * fname, glong lineno);
TextEditor*  anjuta_goto_file_line_mark (gchar * fname,
										 glong lineno, gboolean mark);
gboolean anjuta_goto_local_tag(TextEditor *te, const char *qual_name);
void anjuta_goto_tag(const char *symbol, TextEditor *te,
					 gboolean prefer_definition);
void anjuta_set_zoom_factor(gint zoom);
void anjuta_apply_styles (void);
void anjuta_load_cmdline_files (void);

TextEditor *anjuta_get_notebook_text_editor (gint page_num);

void anjuta_save_settings (void);

void anjuta_save_all_files(void);

gboolean anjuta_save_yourself (FILE * stream);

gboolean anjuta_load_yourself (PropsID pr);

void anjuta_application_exit(void);

void anjuta_clean_exit (void);

void anjuta_update_title (void);
void anjuta_update_page_label (TextEditor *te);

void anjuta_set_file_properties (gchar * full_filename);

void anjuta_information (gchar * mesg, ...);
void anjuta_status (gchar * mesg, ...);
void anjuta_warning (gchar * mesg, ...);
void anjuta_error (gchar * mesg, ...);
void anjuta_system_error (gint errornum, gchar * mesg, ...);
gboolean anjuta_boolean_question (gchar * mesg, ...);

void anjuta_information_parented (GtkWidget *parent, gchar * mesg, ...);
void anjuta_warning_parented (GtkWidget* parent, gchar * mesg, ...);
void anjuta_error_parented (GtkWidget* parent, gchar * mesg, ...);
void anjuta_system_error_parented (GtkWidget* parent, gint errornum,
								   gchar * mesg, ...);

void anjuta_set_busy (void);
void anjuta_set_active (void);
gboolean anjuta_set_auto_gtk_update (gboolean auto_flag);
void main_menu_unref (void);

gchar *anjuta_get_full_filename (gchar * fn);

void anjuta_done_progress (gchar * titile);

gboolean anjuta_init_progress (gchar * description, gdouble full_value,
							   GnomeAppProgressCancelFunc progress_cancel_cb,
							   gpointer data);

void anjuta_set_progress (gdouble value);

void anjuta_delete_all_marker (gint marker);
void anjuta_delete_all_indicators (void);

void anjuta_grab_text_focus (void);

void anjuta_register_window (GtkWidget * win);

void anjuta_unregister_window (GtkWidget * win);

void anjuta_foreach_windows (GFunc cb_func, gpointer data);

void anjuta_register_child_process (pid_t pid,
									void (*callback) (int status, gpointer d),
									gpointer data);
void anjuta_unregister_child_process (pid_t pid);

void anjuta_foreach_child_processes (GFunc cb_func, gpointer data);

void anjuta_not_implemented (gchar * file, guint line);

gboolean anjuta_is_installed (gchar * prog, gboolean show);

/* If set_job is TRUE, job_name is set in the status bar */
/* Else previous job is set in the status bar */
void anjuta_update_app_status (gboolean set_job, gchar * job_name);

/* Private callbacks */

gint on_anjuta_session_die(GnomeClient * client, gpointer data);
gint on_anjuta_session_save_yourself (GnomeClient * client, gint phase,
									  GnomeSaveStyle s_style, gint shutdown,
									  GnomeInteractStyle i_style,
									  gint fast, gpointer data);

void on_anjuta_destroy (GtkWidget * w, gpointer data);

gint on_anjuta_delete (GtkWidget * w, GdkEvent * event, gpointer data);

void on_anjuta_exit_yes_clicked (GtkButton * b, gpointer data);

void on_anjuta_notebook_switch_page (GtkNotebook * notebook,
									 GtkNotebookPage * page,
									 gint page_num, gpointer user_data);

void on_anjuta_dnd_drop (gchar* filename, gpointer data);

void anjuta_refresh_breakpoints (TextEditor* te);

gboolean on_anjuta_window_focus_in_event (GtkWidget * w, GdkEventFocus * e,
										  gpointer d);

gint on_anjuta_window_key_press_event (GtkWidget   *widget,
									   GdkEventKey *event,
									   gpointer     user_data);

gint on_anjuta_window_key_release_event (GtkWidget   *widget,
										 GdkEventKey *event,
										 gpointer     user_data);
               
void on_open_filesel_ok_clicked (GtkButton * button, gpointer data);
void on_open_filesel_cancel_clicked (GtkButton * button, gpointer data);

void on_save_as_filesel_ok_clicked (GtkButton * button, gpointer data);
void on_save_as_filesel_cancel_clicked (GtkButton * button, gpointer data);

void on_build_msg_save_ok_clicked (GtkButton * button, gpointer user_data);
void on_build_msg_save_cancel_clicked (GtkButton * button, gpointer user_data);

void on_prj_list_undock_button_clicked (GtkButton * button, gpointer user_data);
void on_prj_list_hide_button_clicked (GtkButton * button, gpointer user_data);

void on_mesg_win_hide_button_clicked (GtkButton * button, gpointer user_data);
void on_mesg_win_undock_button_clicked (GtkButton * button, gpointer user_data);

void on_recent_files_menu_item_activate (GtkMenuItem * item, gchar * data);
void on_recent_projects_menu_item_activate (GtkMenuItem * item, gchar * data);

gdouble on_anjuta_progress_cb (gpointer data);

void on_anjuta_progress_cancel (gpointer data);

void anjuta_toolbar_set_view (gchar* toolbar_name, gboolean view,
							  gboolean resize, gboolean set_in_props);

gint anjuta_get_file_property (gchar* key, gchar* filename, gint default_value);

TextEditor *anjuta_get_te_from_path( const gchar *szFullPath );

void anjuta_reload_file( const gchar *szFullPath );
void anjuta_save_file_if_modified( const gchar *szFullPath );

void anjuta_load_this_project( const gchar * szProjectPath );
void anjuta_load_last_project(void);
void anjuta_open_project(void);
void show_hide_tooltips(gboolean show);

/* Search for the occurence of the string in all source files */
void anjuta_search_sources_for_symbol(const gchar *s);

#define anjuta_set_execution_dir(d)  string_assign(&app->execution_dir, (d))
#define anjuta_clear_execution_dir() string_assign(&app->execution_dir, NULL)

void anjuta_order_tabs(void);
gboolean anjuta_set_editor_properties(void);

/* Glade file */
#define GLADE_FILE_ANJUTA   PACKAGE_DATA_DIR"/glade/anjuta.glade"

/* File properties keys */
#define CURRENT_FULL_FILENAME_WITH_EXT     "current.full.filename.ext"
#define CURRENT_FULL_FILENAME              "current.full.filename"
#define CURRENT_FILENAME_WITH_EXT          "current.file.name.ext"
#define CURRENT_FILENAME                   "current.file.name"
#define CURRENT_FILE_DIRECTORY             "current.file.dir"
#define CURRENT_FILE_EXTENSION             "current.file.extension"

#define FILE_PROP_IS_SOURCE                "file.is.source."
#define FILE_PROP_CAN_AUTOFORMAT           "file.can.autoformat."
#define FILE_PROP_HAS_TAGS                 "file.has.tags."
#define FILE_PROP_HAS_FOLDS                "file.has.folds."
#define FILE_PROP_IS_INTERPRETED           "file.interpreted."

#define ANJUTA_MAIN_TOOLBAR                "main.toolbar"
#define ANJUTA_EXTENDED_TOOLBAR            "extended.toolbar"
#define ANJUTA_FORMAT_TOOLBAR              "format.toolbar"
#define ANJUTA_DEBUG_TOOLBAR               "debug.toolbar"
#define ANJUTA_BROWSER_TOOLBAR             "browser.toolbar"

#define ANJUTA_LAST_OPEN_PROJECT "anjuta.last.open.project"

#include "session.h" /* WTF is this??? */

#endif
