/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-app.h Copyright (C) 2003 Naba Kumar  <naba@gnome.org>
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

#ifndef _ANJUTA_APP_H_
#define _ANJUTA_APP_H_

#include <gmodule.h>

#include <glade/glade.h>
#include <gdl/gdl-dock-layout.h>
#include <libanjuta/anjuta-status.h>
#include <libanjuta/anjuta-ui.h>

#include "toolbar.h"
#include "preferences.h"

#define g_strdup_printfs2(_FORMAT_, _STR_) \
	{ \
		assert(_STR_); g_strdup_printf(_FORMAT_, _STR_); \
	}
#define g_strdup_printfs3(_FORMAT_, _STR1_, _STR2_) \
	{ \
		assert(_STR1_); assert(_STR2_); \
		g_strdup_printf(_FORMAT_, _STR1_, _STR2_); \
	}

#define ANJUTA_TYPE_APP        (anjuta_app_get_type ())
#define ANJUTA_APP(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_APP, AnjutaApp))
#define ANJUTA_APP_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), ANJUTA_TYPE_APP, AnjutaAppClass))
#define ANJUTA_IS_APP(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_APP))
#define ANJUTA_IS_APP_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_APP))

// typedef struct _AnjutaAppGui AnjutaAppGui;
typedef struct _AnjutaApp AnjutaApp;
typedef struct _AnjutaAppClass AnjutaAppClass;
typedef struct _AnjutaAppGui AnjutaAppGui;

typedef struct _FileLineInfo FileLineInfo;

struct _AnjutaApp
{
	GnomeApp parent;
	GtkWidget *toolbars_menu;
	GtkWidget *view_menu;
	GtkWidget *dock;
 	GdlDockLayout *layout_manager;

	GHashTable *values;
	GHashTable *widgets;

	GtkAccelGroup *accel_group;
	
	AnjutaStatus *status;
	AnjutaUI *ui;
	AnjutaPreferences *preferences;
	
	/* Window state */
	gint win_pos_x, win_pos_y, win_width, win_height;

#if 0
	/* Application progress bar */
	GnomeAppProgressKey progress_key;
	gdouble progress_value;
	gdouble progress_full_value;
	GnomeAppProgressCancelFunc progress_cancel_cb;
	gboolean in_progress;
	gint busy_count;
	gboolean first_time_expose;
	gboolean shutdown_in_progress;
#endif
};

struct _AnjutaAppClass {
	GnomeAppClass klass;
};

GType anjuta_app_get_type (void);
GtkWidget* anjuta_app_new (void);

void anjuta_app_save_layout (AnjutaApp *app, const gchar *name);
void anjuta_app_load_layout (AnjutaApp *app, const gchar *name);

struct _FileLineInfo
{
	gchar *file;
	glong line;
};

// extern AnjutaApp *app;
void anjuta_app_session_restore (AnjutaApp *app, GnomeClient* client);
void anjuta_app_save_settings (AnjutaApp *app);
// gboolean anjuta_app_save_yourself (AnjutaApp *app, FILE * stream);
// gboolean anjuta_app_load_yourself (AnjutaApp *app, PropsID pr);

void anjuta_application_exit(void);

void anjuta_clean_exit (void);

void anjuta_update_title (void);

void anjuta_app_set_busy (AnjutaApp *app);
void anjuta_app_set_active (AnjutaApp *app);

void anjuta_app_progress_done (AnjutaApp *app, gchar * titile);
gboolean anjuta_app_progress_init (AnjutaApp *app, gchar * description,
								   gdouble full_value,
								   GnomeAppProgressCancelFunc progress_cancel_cb,
								   gpointer data);
void anjuta_app_progress_set (gdouble value);

/* If set_job is TRUE, job_name is set in the status bar */
/* Else previous job is set in the status bar */
void anjuta_app_update_job_status (gboolean set_job, gchar * job_name);

void anjuta_toolbar_set_view (gchar* toolbar_name, gboolean view,
							  gboolean resize, gboolean set_in_props);

gint anjuta_get_file_property (gchar* key, gchar* filename, gint default_value);

// TextEditor *anjuta_get_te_from_path( const gchar *szFullPath );

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

// #include "session.h" /* WTF is this??? */

#endif
