/*
 * text_editor.h
 * Copyright (C) 2000  Kh. Naba Kumar Singh
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef _TEXT_EDITOR_H_
#define _TEXT_EDITOR_H_

#include "global.h"
#include "text_editor_menu.h"
#include "preferences.h"
#include "aneditor.h"

#include "tm_tagmanager.h"

#define TEXT_EDITOR_FIND_SCOPE_WHOLE 1
#define TEXT_EDITOR_FIND_SCOPE_CURRENT 2
#define TEXT_EDITOR_FIND_SCOPE_SELECTION 3

typedef struct _TextEditorButtons TextEditorButtons;
typedef struct _TextEditorGui TextEditorGui;
typedef struct _TextEditor TextEditor;
typedef enum _TextEditorMode TextEditorMode;
typedef struct _TextEditorGeom TextEditorGeom;

enum _TextEditorMode
{
	TEXT_EDITOR_PAGED,
	TEXT_EDITOR_WINDOWED
};

struct _TextEditorGeom
{
	guint x, y, width, height;
};

struct _TextEditorButtons
{
	GtkWidget *new;
	GtkWidget *open;
	GtkWidget *save;
	GtkWidget *reload;
	GtkWidget *cut;
	GtkWidget *copy;
	GtkWidget *paste;
	GtkWidget *find;
	GtkWidget *replace;
	GtkWidget *compile;
	GtkWidget *build;
	GtkWidget *print;
	GtkWidget *attach;
};

struct _TextEditorGui
{
	GtkWidget *window;
	GtkWidget *client_area;
	GtkWidget *client;
	GtkWidget *line_label;
	GtkWidget *editor;
	GtkWidget *tab_label;
};

struct _TextEditor
{
	glong	size;
	TextEditorGui widgets;
	TextEditorMode mode;
	TextEditorMenu *menu;
	TextEditorButtons buttons;

	gchar *filename;
	gchar *full_filename;
	TMWorkObject *tm_file;
	time_t modified_time;
	gint force_hilite;

	glong current_line;

	Preferences *preferences;

/* Geometry */
	TextEditorGeom geom;

/* Editor ID for AnEditor */
	AnEditorID editor_id;

/* Properties set ID in the preferences */
	gint props_base;

/* Autosave timer ID */
	gint autosave_id;
	gboolean autosave_on;

/* Timer interval in mins */
	gint autosave_it;

/* Cool! not the usual freeze/thaw */
/* Something to stop unecessary signalings */
	gint freeze_count;
	
/* First time exposer */
	gboolean first_time_expose;
};

void create_text_editor_gui (TextEditor * te);

TextEditor *text_editor_new (gchar * full_filename,
			     TextEditor * parent, Preferences * pr);

void text_editor_freeze (TextEditor * te);

void text_editor_thaw (TextEditor * te);

void text_editor_set_hilite_type (TextEditor * te);

void text_editor_set_zoom_factor (TextEditor * te, gint zfac);

void text_editor_show (TextEditor * te);

void text_editor_hide (TextEditor * te);

void text_editor_destroy (TextEditor * te);

void text_editor_undo (TextEditor * te);
void text_editor_redo (TextEditor * te);

glong
text_editor_find (TextEditor * te, gchar * str, gint scope, gboolean forward,
		  gboolean regexp, gboolean ignore_case, gboolean whole_word);

void text_editor_replace_selection (TextEditor * te, gchar * r_str);

guint text_editor_get_total_lines (TextEditor * te);
guint text_editor_get_current_lineno (TextEditor * te);
gchar* text_editor_get_selection (TextEditor * te);

gboolean text_editor_goto_point (TextEditor * te, guint num);
gboolean text_editor_goto_line (TextEditor * te, guint num, gboolean mark);
gint text_editor_goto_block_start (TextEditor* te);
gint text_editor_goto_block_end (TextEditor* te);

void text_editor_set_line_marker (TextEditor * te, glong line);
gint text_editor_set_marker (TextEditor * te, glong line, gint marker);

gboolean text_editor_load_file (TextEditor * te);
gboolean text_editor_save_file (TextEditor * te);

gboolean text_editor_is_saved (TextEditor * te);
gboolean text_editor_has_selection (TextEditor * te);

void text_editor_update_preferences (TextEditor * te);
void text_editor_update_controls (TextEditor * te);

void text_editor_dock (TextEditor * te, GtkWidget * container);
void text_editor_undock (TextEditor * te, GtkWidget * container);

gboolean text_editor_save_yourself (TextEditor * te, FILE * stream);
gboolean text_editor_recover_yourself (TextEditor * te, FILE * stream);

/*gboolean text_editor_check_disk_status (TextEditor * te);*/
gboolean text_editor_check_disk_status (TextEditor * te, const gboolean bForce );

void text_editor_autoformat (TextEditor * te);
gboolean text_editor_is_marker_set (TextEditor* te, gint line, gint marker);
void text_editor_delete_marker (TextEditor* te, gint line, gint marker);
gint text_editor_line_from_handle (TextEditor* te, gint marker_handle);
gint text_editor_get_bookmark_line( TextEditor* te, const gint nLineStart );
gint text_editor_get_num_bookmarks(TextEditor* te);


#define linenum_text_editor_to_scintilla(x) (x-1)

#define linenum_scintilla_to_text_editor(x) (x+1)

#endif
