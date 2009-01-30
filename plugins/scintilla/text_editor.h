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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef _TEXT_EDITOR_H_
#define _TEXT_EDITOR_H_

#include <glib.h>
#include <glib-object.h>

#include <gio/gio.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-shell.h>

#include "aneditor.h"

#include <libanjuta/anjuta-status.h>
#include <libanjuta/anjuta-encodings.h>

#define TEXT_EDITOR_FIND_SCOPE_WHOLE 1
#define TEXT_EDITOR_FIND_SCOPE_CURRENT 2
#define TEXT_EDITOR_FIND_SCOPE_SELECTION 3

G_BEGIN_DECLS

#define TYPE_TEXT_EDITOR        (text_editor_get_type ())
#define TEXT_EDITOR(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_TEXT_EDITOR, TextEditor))
#define TEXT_EDITOR_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), TYPE_TEXT_EDITOR, TextEditorClass))
#define IS_TEXT_EDITOR(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_TEXT_EDITOR))
#define IS_TEXT_EDITOR_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_TEXT_EDITOR))

typedef enum _TextEditorAttrib
{
	TEXT_EDITOR_ATTRIB_TEXT,
	TEXT_EDITOR_ATTRIB_COMMENT,
	TEXT_EDITOR_ATTRIB_KEYWORD,
	TEXT_EDITOR_ATTRIB_STRING
} TextEditorAttrib;

typedef struct _TextEditor TextEditor;
typedef struct _TextEditorClass TextEditorClass;

struct _TextEditor
{
	GtkVBox parent;
	
	gchar *filename;
	gchar *uri;
	GFileMonitor *monitor;
	
	AnjutaStatus *status;
	AnjutaShell *shell;
	
	/* File extension that will be used to force hilite type */
	gchar *force_hilite;

	glong current_line;

	AnjutaPreferences *preferences;

	/* Editor ID and widget for AnEditor */
	AnEditorID editor_id;
	AnEditorID editor_id_split;
	GtkWidget *scintilla;
	GtkWidget *vbox;
	GList *views;

	/* Properties set ID in the preferences */
	gint props_base;

	/* Something to stop unecessary signalings */
	gint freeze_count;
	
	/* First time exposer */
	gboolean first_time_expose;

	/* File encoding */
	const AnjutaEncoding *encoding;
	
	/* Popup menu widget */
	GtkWidget *popup_menu;
	
	/* Gconf notify IDs */
	GList* gconf_notify_ids;
	
	/* Current zoom factor */
	gint zoom_factor;
	
	/* message area widget */
	GtkWidget *message_area;

	/* Last saved content for comparision on external modifications on 
	 * the file. The content is copied here during file saves.
	 */ 	 
	gchar *last_saved_content;

	/* Set buffer as modified until next save */
	gboolean force_not_saved;
	
	gboolean hover_tip_on;
};

struct _TextEditorClass
{
	GtkVBoxClass parent_class;
};

GType text_editor_get_type (void);

/* New instance of TextEditor */
GtkWidget* text_editor_new (AnjutaStatus *status, AnjutaPreferences * pr, AnjutaShell* shell, const gchar *uri,
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

/* Get text attribute of the character at given position */
TextEditorAttrib text_editor_get_attribute (TextEditor *te, gint position);

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
guint    text_editor_get_position_lineno (TextEditor * te, gint position);
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
void text_editor_set_saved (TextEditor *te, gboolean saved);

void text_editor_update_controls (TextEditor * te);
gboolean text_editor_save_yourself (TextEditor * te, FILE * stream);
gboolean text_editor_recover_yourself (TextEditor * te, FILE * stream);

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
gint     text_editor_set_indicator (TextEditor *te, gint start, gint end,
									gint indicator);
gboolean text_editor_can_undo (TextEditor *te);
gboolean text_editor_can_redo (TextEditor *te);

gchar* text_editor_get_word_before_carat (TextEditor *te);

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

/* Show/hide hover tips */
void text_editor_show_hover_tip (TextEditor *te, gint position, const gchar *info);
void text_editor_hide_hover_tip (TextEditor *te);

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
/*
#define INDENT_AUTOMATIC           "indent.automatic"
*/
#define USE_TABS                   "use.tabs"
#define BRACES_CHECK               "braces.check"
#define DOS_EOL_CHECK              "editor.doseol"
#define WRAP_BOOKMARKS             "editor.wrapbookmarks"
#define TAB_SIZE                   "tabsize"
#define INDENT_SIZE                "indent.size"
/*
#define INDENT_OPENING             "indent.opening"
#define INDENT_CLOSING             "indent.closing"
*/
#define INDENT_MAINTAIN            "indent.maintain"

#define TAB_INDENTS                "tab.indents"
#define BACKSPACE_UNINDENTS        "backspace.unindents"
#define AUTOSAVE_TIMER             "autosave.timer"
#define SAVE_SESSION_TIMER         "save.session.timer"

#define AUTOFORMAT_DISABLE         "autoformat.disable"
#define AUTOFORMAT_STYLE           "autoformat.style"
#define AUTOFORMAT_LIST_STYLE      "autoformat.list.style"
#define AUTOFORMAT_OPTS            "autoformat.opts"

#define FOLD_SYMBOLS               "fold.symbols"
#define FOLD_UNDERLINE             "fold.underline"

#define STRIP_TRAILING_SPACES      "strip.trailing.spaces"
#define FOLD_ON_OPEN               "fold.on.open"
#define CARET_FORE_COLOR           "caret.fore"
#define CALLTIP_BACK_COLOR         "calltip.back"
#define SELECTION_FORE_COLOR       "selection.fore"
#define SELECTION_BACK_COLOR       "selection.back"

#define VIEW_LINENUMBERS_MARGIN    "margin.linenumber.visible"
#define VIEW_MARKER_MARGIN         "margin.marker.visible"
#define VIEW_FOLD_MARGIN           "margin.fold.visible"
#define VIEW_INDENTATION_GUIDES    "view.indentation.guides"
#define VIEW_WHITE_SPACES          "view.whitespace"
#define VIEW_EOL                   "view.eol"
#define VIEW_LINE_WRAP             "view.line.wrap"
#define EDGE_COLUMN                "edge.column"
#define TEXT_ZOOM_FACTOR           "text.zoom.factor"

G_END_DECLS

#endif
