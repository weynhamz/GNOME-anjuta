/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
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

#include <libanjuta/anjuta-preferences.h>

// #include "global.h"
#include "text_editor_menu.h"
#include "aneditor.h"

#include "tm_tagmanager.h"

#define TEXT_EDITOR_FIND_SCOPE_WHOLE 1
#define TEXT_EDITOR_FIND_SCOPE_CURRENT 2
#define TEXT_EDITOR_FIND_SCOPE_SELECTION 3

#define TYPE_TEXT_EDITOR        (text_editor_get_type ())
#define TEXT_EDITOR(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_TEXT_EDITOR, TextEditor))
#define TEXT_EDITOR_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), TYPE_TEXT_EDITOR, TextEditorClass))
#define IS_TEXT_EDITOR(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_TEXT_EDITOR))
#define IS_TEXT_EDITOR_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_TEXT_EDITOR))

typedef struct _TextEditor TextEditor;
typedef struct _TextEditorClass TextEditorClass;
	
struct _TextEditor
{
	GtkVBox vbox;
	
	gchar *filename;
	gchar *uri;
	time_t modified_time;
	
	/* File extension that will be used to force hilite type */
	gchar *force_hilite;

	glong current_line;

	AnjutaPreferences *preferences;

	/* Editor ID and widget for AnEditor */
	AnEditorID editor_id;
	AnEditorID editor_id_split;
	GtkWidget *scintilla;
	GList *views;

	/* Properties set ID in the preferences */
	gint props_base;

	/* Autosave timer ID */
	gint autosave_id;
	gboolean autosave_on;

	/* Timer interval in mins */
	gint autosave_it;

	/* Something to stop unecessary signalings */
	gint freeze_count;
	
	/* First time exposer */
	gboolean first_time_expose;

	/* File encoding */
	gchar *encoding;
	
	/* Popup menu widget */
	GtkWidget *popup_menu;
	
	/* Gconf notify IDs */
	GList* gconf_notify_ids;
};

struct _TextEditorClass
{
	GtkVBoxClass parent_class;
};

GType text_editor_get_type (void);

/* New instance of TextEditor */
GtkWidget* text_editor_new (AnjutaPreferences * pr, const gchar *uri,
							const gchar *tab_name);

/* Freeze and thaw editor */
void text_editor_freeze (TextEditor * te);
void text_editor_thaw (TextEditor * te);

/* Set context pop up menu */
void text_editor_set_popup_menu (TextEditor *te, GtkWidget *popup_menu);

/*
 * Sets the custom (forced) highlight style for the editor. Pass a dummy file
 * name (with extension) to force particular highlight style. This function
 * does not actualy rehighlight the editor, but only sets the highlight type
 * which will be used in subsequent call to text_editor_hil().
 */
void text_editor_set_hilite_type (TextEditor * te, const gchar *file_extension);

/*
 * (Re) highlights the Editor. The 'force' parameter is used to tell if the
 * preferences setting for 'Disable highlight' should not be considered.
 * If force == FALSE, there will be no highlight if 'Disable highlight' is
 * set ON.
 */
void text_editor_hilite (TextEditor *te, gboolean force);

/*
 * Set the zoom factor. Zoom factor basically increases or decreases the
 * text font size by a factor of (2*zfac)
 */
void text_editor_set_zoom_factor (TextEditor * te, gint zfac);

 /* Undo or redo last action */
void text_editor_undo (TextEditor * te);
void text_editor_redo (TextEditor * te);

/* wrap flag only applies when scope == TEXT_EDITOR_FIND_CURRENT_POS */
glong
text_editor_find (TextEditor * te, const gchar * str, gint scope,
				  gboolean forward, gboolean regexp, gboolean ignore_case,
				  gboolean whole_word, gboolean wrap);

/* Replaces current selection with given string */
void text_editor_replace_selection (TextEditor * te, const gchar * r_str);

/* Various editor information */
guint    text_editor_get_total_lines (TextEditor * te);
glong    text_editor_get_current_position (TextEditor * te);
guint    text_editor_get_current_lineno (TextEditor * te);
guint    text_editor_get_current_column (TextEditor * te);
guint    text_editor_get_line_from_position (TextEditor * te, glong pos);
gchar*   text_editor_get_selection (TextEditor * te);
gboolean text_editor_get_overwrite (TextEditor * te);
glong    text_editor_get_selection_start (TextEditor * te);
glong    text_editor_get_selection_end (TextEditor * te);
gboolean text_editor_has_selection (TextEditor * te);
gboolean text_editor_is_saved (TextEditor * te);

/* Jump to various locations */
gboolean text_editor_goto_point (TextEditor * te, glong num);
gboolean text_editor_goto_line (TextEditor * te, glong num,
								gboolean mark, gboolean ensure_visible);
gint text_editor_goto_block_start (TextEditor* te);
gint text_editor_goto_block_end (TextEditor* te);

/* Save or load file */
gboolean text_editor_load_file (TextEditor * te);
gboolean text_editor_save_file (TextEditor * te, gboolean update);

void text_editor_update_controls (TextEditor * te);
gboolean text_editor_save_yourself (TextEditor * te, FILE * stream);
gboolean text_editor_recover_yourself (TextEditor * te, FILE * stream);
gboolean text_editor_check_disk_status (TextEditor * te, const gboolean bForce );

/* Autoformats code using 'indent' program */
void text_editor_autoformat (TextEditor * te);

/* Markers and indicators */
void     text_editor_set_line_marker (TextEditor * te, glong line);
gint     text_editor_set_marker (TextEditor * te, glong line, gint marker);
gboolean text_editor_is_marker_set (TextEditor* te, glong line, gint marker);
void     text_editor_delete_marker (TextEditor* te, glong line, gint marker);
void     text_editor_delete_marker_all (TextEditor *te, gint marker);
gint     text_editor_line_from_handle (TextEditor* te, gint marker_handle);
gint     text_editor_get_bookmark_line (TextEditor* te, const glong nLineStart);
gint     text_editor_get_num_bookmarks (TextEditor* te);
gint     text_editor_set_indicator (TextEditor *te, glong line, gint indicator);

/* Get currect word near by cursor location */
gchar* text_editor_get_current_word (TextEditor *te);

/* Updates linewidth according to total line numbers */
void text_editor_set_line_number_width (TextEditor* te);

/* Grab focus */
void text_editor_grab_focus (TextEditor *te);

/* Select the function block where the cursor is content */
void text_editor_function_select(TextEditor *te);

/* Get the global properties set */
gint text_editor_get_props (void);

/* Set busy cursor on Editor window */
void text_editor_set_busy (TextEditor *te, gboolean state);

/* Multiple views addition and removal */
void text_editor_add_view (TextEditor *te);
void text_editor_remove_view (TextEditor *te);

/* Direct editor commands to AnEditor and Scintilla */
void text_editor_command(TextEditor *te, gint command,
						 glong wparam, glong lparam);
void text_editor_scintilla_command (TextEditor *te, gint command,
									glong wparam, glong lparam);

/*
 * Conversion from scintilla line number to TextEditor line
 * number representation
 */
#define linenum_text_editor_to_scintilla(x) (x-1)
#define linenum_scintilla_to_text_editor(x) (x+1)

/* Editor preferences */
#define DISABLE_SYNTAX_HILIGHTING  "disable.syntax.hilighting"
#define SAVE_AUTOMATIC             "save.automatic"
#define INDENT_AUTOMATIC           "indent.automatic"
#define USE_TABS                   "use.tabs"
#define BRACES_CHECK               "braces.check"
#define DOS_EOL_CHECK              "editor.doseol"
#define WRAP_BOOKMARKS             "editor.wrapbookmarks"
#define TAB_SIZE                   "tabsize"
#define INDENT_SIZE                "indent.size"
#define INDENT_OPENING             "indent.opening"
#define INDENT_CLOSING             "indent.closing"
#define INDENT_MAINTAIN            "indent.maintain"
#define TAB_INDENTS                "tab.indents"
#define BACKSPACE_UNINDENTS        "backspace.unindents"

#define AUTOSAVE_TIMER             "autosave.timer"
#define SAVE_SESSION_TIMER         "save.session.timer"

#define AUTOFORMAT_DISABLE         "autoformat.disable"
#define AUTOFORMAT_CUSTOM_STYLE    "autoformat.custom.style"
#define AUTOFORMAT_STYLE           "autoformat.style"

#define FOLD_SYMBOLS               "fold.symbols"
#define FOLD_UNDERLINE             "fold.underline"
#define EDITOR_TABS_POS            "editor.tabs.pos"
#define EDITOR_TABS_HIDE           "editor.tabs.hide"
#define EDITOR_TABS_ORDERING       "editor.tabs.ordering"
#define EDITOR_TABS_RECENT_FIRST   "editor.tabs.recent.first"

#define STRIP_TRAILING_SPACES      "strip.trailing.spaces"
#define FOLD_ON_OPEN               "fold.on.open"
#define CARET_FORE_COLOR           "caret.fore"
#define CALLTIP_BACK_COLOR         "calltip.back"
#define SELECTION_FORE_COLOR       "selection.fore"
#define SELECTION_BACK_COLOR       "selection.back"
#define TEXT_ZOOM_FACTOR           "text.zoom.factor"

#define VIEW_LINENUMBERS_MARGIN    "margin.linenumber.visible"
#define VIEW_MARKER_MARGIN         "margin.marker.visible"
#define VIEW_FOLD_MARGIN           "margin.fold.visible"
#define VIEW_INDENTATION_GUIDES    "view.indentation.guides"
#define VIEW_WHITE_SPACES          "view.whitespace"
#define VIEW_EOL                   "view.eol"
#define VIEW_LINE_WRAP             "view.line.wrap"

#endif
