/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2000 Naba Kumar

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

#include <config.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-encodings.h>

#include <libegg/menu/egg-entry-action.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-editor-convert.h>
#include <libanjuta/interfaces/ianjuta-editor-view.h>
#include <libanjuta/interfaces/ianjuta-editor-line-mode.h>
#include <libanjuta/interfaces/ianjuta-editor-assist.h>
#include <libanjuta/interfaces/ianjuta-editor-factory.h>
#include <libanjuta/interfaces/ianjuta-editor-folds.h>
#include <libanjuta/interfaces/ianjuta-editor-comment.h>
#include <libanjuta/interfaces/ianjuta-editor-zoom.h>
#include <libanjuta/interfaces/ianjuta-editor-goto.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <libanjuta/interfaces/ianjuta-editor-language.h>
#include <libanjuta/interfaces/ianjuta-language-support.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>

#include "anjuta-docman.h"
#include "action-callbacks.h"
#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-document-manager.ui"
#define PREFS_GLADE PACKAGE_DATA_DIR"/glade/anjuta-document-manager.glade"
#define ICON_FILE "anjuta-document-manager.png"

/* Pixmaps */
#define ANJUTA_PIXMAP_SWAP                "swap.png"
#define ANJUTA_PIXMAP_INDENT              "indent.xpm"
#define ANJUTA_PIXMAP_SYNTAX              "syntax.xpm"
#define ANJUTA_PIXMAP_BOOKMARK_TOGGLE     "bookmark-toggle.png"
#define ANJUTA_PIXMAP_BOOKMARK_FIRST      "bookmark-first.png"
#define ANJUTA_PIXMAP_BOOKMARK_PREV       "bookmark-prev.png"
#define ANJUTA_PIXMAP_BOOKMARK_NEXT       "bookmark-next.png"
#define ANJUTA_PIXMAP_BOOKMARK_LAST       "bookmark-last.png"
#define ANJUTA_PIXMAP_BOOKMARK_CLEAR      "bookmark-clear.png"
#define ANJUTA_PIXMAP_ERROR_PREV          "error-prev.png"
#define ANJUTA_PIXMAP_ERROR_NEXT          "error-next.png"

#define ANJUTA_PIXMAP_FOLD_TOGGLE         "fold_toggle.xpm"
#define ANJUTA_PIXMAP_FOLD_CLOSE          "fold_close.xpm"
#define ANJUTA_PIXMAP_FOLD_OPEN           "fold_open.xpm"

#define ANJUTA_PIXMAP_BLOCK_SELECT        "block_select.xpm"
#define ANJUTA_PIXMAP_BLOCK_START         "block-start.png"
#define ANJUTA_PIXMAP_BLOCK_END           "block-end.png"

#define ANJUTA_PIXMAP_INDENT_INC          "indent_inc.xpm"
#define ANJUTA_PIXMAP_INDENT_DCR          "indent_dcr.xpm"
#define ANJUTA_PIXMAP_INDENT_AUTO         "indent_auto.xpm"
#define ANJUTA_PIXMAP_AUTOFORMAT_SETTING  "indent_set.xpm"

#define ANJUTA_PIXMAP_CALLTIP             "calltip.xpm"
#define ANJUTA_PIXMAP_AUTOCOMPLETE        "autocomplete.png"

/* Stock icons */
#define ANJUTA_STOCK_SWAP                     "anjuta-swap"
#define ANJUTA_STOCK_FOLD_TOGGLE              "anjuta-fold-toggle"
#define ANJUTA_STOCK_FOLD_OPEN                "anjuta-fold-open"
#define ANJUTA_STOCK_FOLD_CLOSE               "anjuta-fold-close"
#define ANJUTA_STOCK_BLOCK_SELECT             "anjuta-block-select"
#define ANJUTA_STOCK_INDENT_INC               "anjuta-indent-inc"
#define ANJUTA_STOCK_INDENT_DCR               "anjuta-indect-dcr"
#define ANJUTA_STOCK_INDENT_AUTO              "anjuta-indent-auto"
#define ANJUTA_STOCK_AUTOFORMAT_SETTINGS      "anjuta-autoformat-settings"
#define ANJUTA_STOCK_AUTOCOMPLETE             "anjuta-autocomplete"
#define ANJUTA_STOCK_PREV_BRACE               "anjuta-prev-brace"
#define ANJUTA_STOCK_NEXT_BRACE               "anjuta-next-brace"
#define ANJUTA_STOCK_BLOCK_START              "anjuta-block-start"
#define ANJUTA_STOCK_BLOCK_END                "anjuta-block-end"
#define ANJUTA_STOCK_BOOKMARK_TOGGLE          "anjuta-bookmark-toggle"
#define ANJUTA_STOCK_BOOKMARK_FIRST           "anjuta-bookmark-first"
#define ANJUTA_STOCK_BOOKMARK_PREV            "anjuta-bookmark-previous"
#define ANJUTA_STOCK_BOOKMARK_NEXT            "anjuta-bookmark-next"
#define ANJUTA_STOCK_BOOKMARK_LAST            "anjuta-bookmark-last"
#define ANJUTA_STOCK_BOOKMARK_CLEAR           "anjuta-bookmark-clear"

static gpointer parent_class;

/* Shortcuts implementation */
enum {
	m___ = 0,
	mS__ = GDK_SHIFT_MASK,
	m_C_ = GDK_CONTROL_MASK,
	m__M = GDK_MOD1_MASK,
	mSC_ = GDK_SHIFT_MASK | GDK_CONTROL_MASK,
};

enum {
	ID_NEXTBUFFER = 1, /* Note: the value mustn't be 0 ! */
	ID_PREVBUFFER,
	ID_FIRSTBUFFER
};

typedef struct {
	int modifiers;
	unsigned int gdk_key;
	int id;
} ShortcutMapping;

static ShortcutMapping global_keymap[] = {
/* FIXME: HIG requires that Ctrl+Tab be used to transfer focus in multiline
   text entries. So we can't use the following ctrl+tabbing. Is there other
   sensible keys that can be used for them?
*/
/*
	{ m_C_, GDK_Tab,		 ID_NEXTBUFFER },
	{ mSC_, GDK_ISO_Left_Tab, ID_PREVBUFFER },
*/
	{ m__M, GDK_1, ID_FIRSTBUFFER },
	{ m__M, GDK_2, ID_FIRSTBUFFER + 1},
	{ m__M, GDK_3, ID_FIRSTBUFFER + 2},
	{ m__M, GDK_4, ID_FIRSTBUFFER + 3},
	{ m__M, GDK_5, ID_FIRSTBUFFER + 4},
	{ m__M, GDK_6, ID_FIRSTBUFFER + 5},
	{ m__M, GDK_7, ID_FIRSTBUFFER + 6},
	{ m__M, GDK_8, ID_FIRSTBUFFER + 7},
	{ m__M, GDK_9, ID_FIRSTBUFFER + 8},
	{ m__M, GDK_0, ID_FIRSTBUFFER + 9},
	{ 0,   0,		 0 }
};

static GtkActionEntry actions_file[] = {
  { "ActionFileSave", N_("_Save"), GTK_STOCK_SAVE, "<control>s",
	N_("Save current file"), G_CALLBACK (on_save_activate)},
  { "ActionFileSaveAs", N_("Save _As..."), GTK_STOCK_SAVE_AS, NULL,
	N_("Save the current file with a different name"),
    G_CALLBACK (on_save_as_activate)},
  { "ActionFileSaveAll", N_("Save A_ll"),  NULL, NULL,
	N_("Save all currently open files, except new files"),
    G_CALLBACK (on_save_all_activate)},
  { "ActionFileClose", N_("_Close File"), GTK_STOCK_CLOSE, "<control>w",
	N_("Close current file"),
    G_CALLBACK (on_close_file_activate)},
  { "ActionFileCloseAll", N_("Close All Files"), GTK_STOCK_CLOSE, "<shift><control>w",
	N_("Close all files"),
    G_CALLBACK (on_close_all_file_activate)},
  { "ActionFileReload", N_("Reload F_ile"), GTK_STOCK_REVERT_TO_SAVED, NULL,
	N_("Reload current file"),
    G_CALLBACK (on_reload_file_activate)},
  { "ActionFileSwap", N_("Swap .h/.c"), ANJUTA_STOCK_SWAP, NULL,
	N_("Swap c header and source file"),
    G_CALLBACK (on_swap_activate)},
  { "ActionMenuFileRecentFiles", N_("Recent _Files"),  NULL, NULL, NULL, NULL},
};

static GtkActionEntry actions_print[] = {
  { "ActionPrintFile", N_("_Print..."), GTK_STOCK_PRINT, "<control>p",
	N_("Print the current file"), G_CALLBACK (anjuta_print_cb)},
  { "ActionPrintPreview", N_("_Print Preview"), NULL, NULL,
	N_("Print preview of the current file"),
    G_CALLBACK (anjuta_print_preview_cb)},
};

static GtkActionEntry actions_transform[] = {
  { "ActionMenuEditTransform", N_("_Transform"), NULL, NULL, NULL, NULL},
  { "ActionEditMakeSelectionUppercase", N_("_Make Selection Uppercase"), NULL, NULL,
	N_("Make the selected text uppercase"),
    G_CALLBACK (on_editor_command_upper_case_activate)},
  { "ActionEditMakeSelectionLowercase", N_("Make Selection Lowercase"), NULL, NULL,
	N_("Make the selected text lowercase"),
    G_CALLBACK (on_editor_command_lower_case_activate)},
  { "ActionEditConvertCRLF", N_("Convert EOL to CRLF"), NULL, NULL,
	N_("Convert End Of Line characters to DOS EOL (CRLF)"),
    G_CALLBACK (on_editor_command_eol_crlf_activate)},
  { "ActionEditConvertLF", N_("Convert EOL to LF"), NULL, NULL,
	N_("Convert End Of Line characters to Unix EOL (LF)"),
    G_CALLBACK (on_editor_command_eol_lf_activate)},
  { "ActionEditConvertCR", N_("Convert EOL to CR"), NULL, NULL,
	N_("Convert End Of Line characters to Mac OS EOL (CR)"),
    G_CALLBACK (on_editor_command_eol_cr_activate)},
  { "ActionEditConvertEOL", N_("Convert EOL to Majority EOL"), NULL, NULL,
	N_("Convert End Of Line characters to majority of the EOL found in the file"),
    G_CALLBACK (on_transform_eolchars1_activate)},
};

static GtkActionEntry actions_select[] = {
  { "ActionMenuEditSelect", N_("_Select"), NULL, NULL, NULL, NULL},
  { "ActionEditSelectAll", N_("Select _All"), NULL, "<control>a",
	N_("Select all text in the editor"),
    G_CALLBACK (on_editor_command_select_all_activate)},
  { "ActionEditSelectToBrace", N_("Select to _Brace"), NULL, NULL,
	N_("Select the text in the matching braces"),
    G_CALLBACK (on_editor_command_select_to_brace_activate)},
  { "ActionEditSelectBlock", N_("Select _Code Block"),
	ANJUTA_STOCK_BLOCK_SELECT, "<shift><control>b",
	N_("Select the current code block"),
    G_CALLBACK (on_editor_command_select_block_activate)},
};

static GtkActionEntry actions_comment[] = {
  { "ActionMenuEditComment", N_("Co_mment"), NULL, NULL, NULL, NULL},
  { "ActionEditCommentBlock", N_("_Block Comment/Uncomment"), NULL, NULL,
	N_("Block comment the selected text"),
    G_CALLBACK (on_comment_block)},
  { "ActionEditCommentBox", N_("Bo_x Comment/Uncomment"), NULL, NULL,
	N_("Box comment the selected text"),
    G_CALLBACK (on_comment_box)},
  { "ActionEditCommentStream", N_("_Stream Comment/Uncomment"), NULL, NULL,
	N_("Stream comment the selected text"),
    G_CALLBACK (on_comment_block)},
};

static GtkActionEntry actions_navigation[] = {
  { "ActionMenuGoto", N_("_Goto"), NULL, NULL, NULL, NULL},
  { "ActionEditGotoLineActivate", N_("_Goto Line number"),
	GTK_STOCK_JUMP_TO, NULL,
	N_("Go to a particular line in the editor"),
    G_CALLBACK (on_goto_activate)},
  { "ActionEditGotoLine", N_("_Line Number..."),
	GTK_STOCK_JUMP_TO, "<control><alt>g",
	N_("Go to a particular line in the editor"),
    G_CALLBACK (on_goto_line_no1_activate)},
  { "ActionEditGotoMatchingBrace", N_("Matching _Brace"),
	GTK_STOCK_JUMP_TO, "<control><alt>m", 
	N_("Go to the matching brace in the editor"),
    G_CALLBACK (on_editor_command_match_brace_activate)},
  { "ActionEditGotoBlockStart", N_("_Start of Block"),
	ANJUTA_STOCK_BLOCK_START, "<control><alt>s",
	N_("Go to the start of the current block"),
    G_CALLBACK (on_goto_block_start1_activate)},
  { "ActionEditGotoBlockEnd", N_("_End of Block"),
	ANJUTA_STOCK_BLOCK_END, "<control><alt>e",
	N_("Go to the end of the current block"),
    G_CALLBACK (on_goto_block_end1_activate)},
  { "ActionEditGotoHistoryPrev", N_("Previous _History"),
	GTK_STOCK_JUMP_TO, NULL,
	N_("Goto previous history"),
    G_CALLBACK (on_prev_history)},
  { "ActionEditGotoHistoryNext", N_("Next Histor_y"),
	GTK_STOCK_JUMP_TO, NULL,
	N_("Goto next history"),
    G_CALLBACK (on_next_history)}
};

static GtkActionEntry actions_edit[] = {
  { "ActionMenuEdit", N_("_Edit"), NULL, NULL, NULL, NULL},
  { "ActionMenuViewEditor", N_("_Editor"), NULL, NULL, NULL, NULL},
  { "ActionViewEditorAddView", N_("_Add Editor View"), NULL, NULL,
	N_("Add one more view of current document"),
    G_CALLBACK (on_editor_add_view_activate)},
  { "ActionViewEditorRemoveView", N_("_Remove Editor View"), NULL, NULL,
	N_("Remove current view of the document"),
    G_CALLBACK (on_editor_remove_view_activate)},
  { "ActionEditUndo", N_("U_ndo"), GTK_STOCK_UNDO, "<control>z",
	N_("Undo the last action"),
    G_CALLBACK (on_editor_command_undo_activate)},
  { "ActionEditRedo", N_("_Redo"), GTK_STOCK_REDO, "<control>r",
	N_("Redo the last undone action"),
    G_CALLBACK (on_editor_command_redo_activate)},
  { "ActionEditCut", N_("C_ut"), GTK_STOCK_CUT, "<control>x",
	N_("Cut the selected text from the editor to the clipboard"),
    G_CALLBACK (on_editor_command_cut_activate)},
  { "ActionEditCopy", N_("_Copy"), GTK_STOCK_COPY, "<control>c",
	N_("Copy the selected text to the clipboard"),
    G_CALLBACK (on_editor_command_copy_activate)},
  { "ActionEditPaste", N_("_Paste"), GTK_STOCK_PASTE, "<control>v",
	N_("Paste the content of clipboard at the current position"),
    G_CALLBACK (on_editor_command_paste_activate)},
  { "ActionEditClear", N_("_Clear"), NULL, "Delete",
	N_("Delete the selected text from the editor"),
    G_CALLBACK (on_editor_command_clear_activate)},
  { "ActionEditAutocomplete", N_("_AutoComplete"),
	ANJUTA_STOCK_AUTOCOMPLETE, "<control>Return",
	N_("AutoComplete the current word"),
    G_CALLBACK (on_editor_command_complete_word_activate)}
};

static GtkToggleActionEntry actions_view[] = {
  { "ActionViewEditorLinenumbers", N_("_Line Number Margin"), NULL, NULL,
	N_("Show/Hide line numbers"),
    G_CALLBACK (on_editor_linenos1_activate), FALSE},
  { "ActionViewEditorMarkers", N_("_Marker Margin"), NULL, NULL,
	N_("Show/Hide marker margin"),
    G_CALLBACK (on_editor_markers1_activate), FALSE},
  { "ActionViewEditorFolds", N_("_Code Fold Margin"), NULL, NULL,
	N_("Show/Hide code fold margin"),
    G_CALLBACK (on_editor_codefold1_activate), FALSE},
  { "ActionViewEditorGuides", N_("_Indentation Guides"), NULL, NULL,
	N_("Show/Hide indentation guides"),
    G_CALLBACK (on_editor_indentguides1_activate), FALSE},
  { "ActionViewEditorSpaces", N_("_White Space"), NULL, NULL,
	N_("Show/Hide white spaces"),
    G_CALLBACK (on_editor_whitespaces1_activate), FALSE},
  { "ActionViewEditorEOL", N_("_Line End Characters"), NULL, NULL,
	N_("Show/Hide line end characters"),
    G_CALLBACK (on_editor_eolchars1_activate), FALSE},
  { "ActionViewEditorWrapping", N_("Line _Wrapping"), NULL, NULL,
	N_("Enable/disable line wrapping"),
    G_CALLBACK (on_editor_linewrap1_activate), FALSE}
};

static GtkActionEntry actions_zoom[] = {
  { "ActionViewEditorZoomIn", N_("Zoom In"), GTK_STOCK_ZOOM_IN, "<control>plus",
	N_("Zoom in: Increase font size"),
    G_CALLBACK (on_zoom_in_text_activate)},
  { "ActionViewEditorZoomOut", N_("Zoom Out"), GTK_STOCK_ZOOM_OUT, "<control>minus",
	N_("Zoom out: Decrease font size"),
    G_CALLBACK (on_zoom_out_text_activate)}
};

static GtkActionEntry actions_style[] = {
  { "ActionMenuFormatStyle", N_("_Highlight Mode"), NULL, NULL, NULL, NULL}
};

static GtkActionEntry actions_format[] = {
  { "ActionMenuFormat", N_("For_mat"), NULL, NULL, NULL, NULL},
  { "ActionFormatAutoformat", N_("Auto _Format"),
	ANJUTA_STOCK_INDENT_AUTO, NULL,
	N_("Autoformat the current source file"),
    G_CALLBACK (on_indent1_activate)},
/*
  { "ActionFormatSettings", N_("Autoformat _settings"),
	ANJUTA_STOCK_AUTOFORMAT_SETTINGS, NULL,
	N_("Autoformat settings"),
    G_CALLBACK (on_format_indent_style_clicked)},
  { "ActionFormatIndentationIncrease", N_("_Increase Indent"),
	ANJUTA_STOCK_INDENT_INC, NULL,
	N_("Increase indentation of line/selection"),
    G_CALLBACK (on_editor_command_indent_increase_activate)},
  { "ActionFormatIndentationDecrease", N_("_Decrease Indent"),
	ANJUTA_STOCK_INDENT_DCR, NULL,
	N_("Decrease indentation of line/selection"),
    G_CALLBACK (on_editor_command_indent_decrease_activate)},
*/
  { "ActionFormatFoldCloseAll", N_("_Close All Folds"),
	ANJUTA_STOCK_FOLD_CLOSE, NULL,
	N_("Close all code folds in the editor"),
    G_CALLBACK (on_editor_command_close_folds_all_activate)},
  { "ActionFormatFoldOpenAll", N_("_Open All Folds"),
	ANJUTA_STOCK_FOLD_OPEN, NULL,
	N_("Open all code folds in the editor"),
    G_CALLBACK (on_editor_command_open_folds_all_activate)},
  { "ActionFormatFoldToggle", N_("_Toggle Current Fold"),
	ANJUTA_STOCK_FOLD_TOGGLE, NULL,
	N_("Toggle current code fold in the editor"),
    G_CALLBACK (on_editor_command_toggle_fold_activate)},
};

static GtkActionEntry actions_bookmark[] = {
  { "ActionMenuBookmark", N_("Bookmar_k"), NULL, NULL, NULL, NULL},
  { "ActionBookmarkToggle", N_("_Toggle Bookmark"),
	ANJUTA_STOCK_BOOKMARK_TOGGLE, "<control>k",
	N_("Toggle a bookmark at the current line position"),
    G_CALLBACK (on_editor_command_bookmark_toggle_activate)},
  { "ActionBookmarkFirst", N_("_First Bookmark"),
	ANJUTA_STOCK_BOOKMARK_FIRST, NULL,
	N_("Jump to the first bookmark in the file"),
    G_CALLBACK (on_editor_command_bookmark_first_activate)},
  { "ActionBookmarkPrevious", N_("_Previous Bookmark"),
	ANJUTA_STOCK_BOOKMARK_PREV, "<control>comma",
	N_("Jump to the previous bookmark in the file"),
    G_CALLBACK (on_editor_command_bookmark_prev_activate)},
  { "ActionBookmarkNext", N_("_Next Bookmark"),
	ANJUTA_STOCK_BOOKMARK_NEXT, "<control>period",
	N_("Jump to the next bookmark in the file"),
    G_CALLBACK (on_editor_command_bookmark_next_activate)},
  { "ActionBookmarkLast", N_("_Last Bookmark"),
	ANJUTA_STOCK_BOOKMARK_LAST, NULL,
	N_("Jump to the last bookmark in the file"),
    G_CALLBACK (on_editor_command_bookmark_last_activate)},
  { "ActionBookmarkClear", N_("_Clear All Bookmarks"),
	ANJUTA_STOCK_BOOKMARK_CLEAR, NULL,
	N_("Clear bookmarks"),
    G_CALLBACK (on_editor_command_bookmark_clear_activate)},
};

struct ActionGroupInfo {
	GtkActionEntry *group;
	gint size;
	gchar *name;
	gchar *label;
};

struct ActionToggleGroupInfo {
	GtkToggleActionEntry *group;
	gint size;
	gchar *name;
	gchar *label;
};

static struct ActionGroupInfo action_groups[] = {
	{ actions_file, G_N_ELEMENTS (actions_file), "ActionGroupEditorFile",  N_("Editor file operations")},
	{ actions_print, G_N_ELEMENTS (actions_print), "ActionGroupEditorPrint",  N_("Editor print operations")},
	{ actions_transform, G_N_ELEMENTS (actions_transform), "ActionGroupEditorTransform", N_("Editor text transformation") },
	{ actions_select, G_N_ELEMENTS (actions_select), "ActionGroupEditorSelect", N_("Editor text selection") },
//	{ actions_insert, G_N_ELEMENTS (actions_insert), "ActionGroupEditorInsert", N_("Editor text insertions") },
	{ actions_comment, G_N_ELEMENTS (actions_comment), "ActionGroupEditorComment", N_("Editor code commenting") },
	{ actions_navigation, G_N_ELEMENTS (actions_navigation), "ActionGroupEditorNavigate", N_("Editor navigations") },
	{ actions_edit, G_N_ELEMENTS (actions_edit), "ActionGroupEditorEdit", N_("Editor edit operations") },
	{ actions_zoom, G_N_ELEMENTS (actions_zoom), "ActionGroupEditorZoom", N_("Editor zoom operations") },
	{ actions_style, G_N_ELEMENTS (actions_style), "ActionGroupEditorStyle", N_("Editor syntax highlighting styles") },
	{ actions_format, G_N_ELEMENTS (actions_format), "ActionGroupEditorFormat", N_("Editor text formating") },
	{ actions_bookmark, G_N_ELEMENTS (actions_bookmark), "ActionGroupEditorBookmark", N_("Editor bookmarks") }
};

static struct ActionToggleGroupInfo action_toggle_groups[] = {
	{ actions_view, G_N_ELEMENTS (actions_view), "ActionGroupEditorView", N_("Editor view settings") }
};

// void pref_set_style_combo(DocmanPlugin *plugin);

/* FIXME: There was a change in data representation in GtkActionEntry from
EggActionGroupEntry. The actual entries should be fixed and this hack removed */
static void
swap_label_and_stock (GtkActionEntry* actions, gint size)
{
	int i;
	for (i = 0; i < size; i++)
	{
		const gchar *stock_id = actions[i].label;
		actions[i].label = actions[i].stock_id;
		actions[i].stock_id = stock_id;
		if (actions[i].name == NULL)
			DEBUG_PRINT ("Name is null: %s", actions[i].label);
	}
}

static void
swap_toggle_label_and_stock (GtkToggleActionEntry* actions, gint size)
{
	int i;
	for (i = 0; i < size; i++)
	{
		const gchar *stock_id = actions[i].label;
		actions[i].label = actions[i].stock_id;
		actions[i].stock_id = stock_id;
		if (actions[i].name == NULL)
			DEBUG_PRINT ("Name is null: %s", actions[i].label);
	}
}

#define VIEW_LINENUMBERS_MARGIN    "margin.linenumber.visible"
#define VIEW_MARKER_MARGIN         "margin.marker.visible"
#define VIEW_FOLD_MARGIN           "margin.fold.visible"
#define VIEW_INDENTATION_GUIDES    "view.indentation.guides"
#define VIEW_WHITE_SPACES          "view.whitespace"
#define VIEW_EOL                   "view.eol"
#define VIEW_LINE_WRAP             "view.line.wrap"

static void
ui_states_init (AnjutaPlugin *plugin)
{
	gint i;
	DocmanPlugin *eplugin;
	static const gchar *prefs[] = {
		VIEW_LINENUMBERS_MARGIN,
		VIEW_MARKER_MARGIN,
		VIEW_FOLD_MARGIN,
		VIEW_INDENTATION_GUIDES,
		VIEW_WHITE_SPACES,
		VIEW_EOL,
		VIEW_LINE_WRAP
	};
	
	eplugin = ANJUTA_PLUGIN_DOCMAN (plugin);
	for (i = 0; i < sizeof (actions_view)/sizeof(GtkToggleActionEntry); i++)
	{
		GtkAction *action;
		gboolean state;
		
		state = anjuta_preferences_get_int (eplugin->prefs, prefs[i]);
		action = anjuta_ui_get_action (eplugin->ui, "ActionGroupEditorView",
									   actions_view[i].name);
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), state);
	}
}

static void
ui_give_shorter_names (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	GtkAction *action;
			
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (plugin)->shell, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupEditorFile",
								   "ActionFileSave");
	g_object_set (G_OBJECT (action), "short-label", _("Save"),
				  "is-important", TRUE, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupEditorFile",
								   "ActionFileReload");
	g_object_set (G_OBJECT (action), "short-label", _("Reload"), NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupEditorEdit",
								   "ActionEditUndo");
	g_object_set (G_OBJECT (action), "is-important", TRUE, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupEditorNavigate",
								   "ActionEditGotoLineActivate");
	g_object_set (G_OBJECT (action), "short-label", _("Goto"), NULL);
}

static void
update_editor_ui_enable_all (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	gint i, j;
	GtkAction *action;
			
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (plugin)->shell, NULL);
	for (i = 0; i < G_N_ELEMENTS (action_groups); i++)
	{
		for (j = 0; j < action_groups[i].size; j++)
		{
			action = anjuta_ui_get_action (ui, action_groups[i].name,
										   action_groups[i].group[j].name);
			if (action_groups[i].group[j].callback)
			{
				g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
			}
		}
	}
	action = anjuta_ui_get_action (ui, "ActionGroupNavigation",
								   "ActionEditGotoLineEntry");
	g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
}

static void
update_editor_ui_disable_all (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	gint i, j;
	GtkAction *action;
			
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (plugin)->shell, NULL);
	for (i = 0; i < G_N_ELEMENTS (action_groups); i++)
	{
		for (j = 0; j < action_groups[i].size; j++)
		{
			action = anjuta_ui_get_action (ui, action_groups[i].name,
										   action_groups[i].group[j].name);
			if (action_groups[i].group[j].callback)
			{
				g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
			}
		}
	}
	action = anjuta_ui_get_action (ui, "ActionGroupNavigation",
								   "ActionEditGotoLineEntry");
	g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);

}

static void
update_editor_ui_save_items (AnjutaPlugin *plugin, IAnjutaEditor *editor)
{
	AnjutaUI *ui;
	GtkAction *action;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);

	action = anjuta_ui_get_action (ui, "ActionGroupEditorEdit",
								   "ActionEditUndo");
	g_object_set (G_OBJECT (action), "sensitive",
				  ianjuta_editor_can_undo (editor, NULL), NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupEditorEdit",
								   "ActionEditRedo");
	g_object_set (G_OBJECT (action), "sensitive",
				  ianjuta_editor_can_redo (editor, NULL), NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupEditorFile",
								   "ActionFileSave");
	g_object_set (G_OBJECT (action), "sensitive",
				  ianjuta_file_savable_is_dirty(IANJUTA_FILE_SAVABLE(editor), NULL),
				  NULL);
}

static void
update_editor_ui_interface_items (AnjutaPlugin *plugin, IAnjutaEditor *editor)
{
	AnjutaUI *ui;
	GtkAction *action;
	gboolean flag;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);

	/* IAnjutaEditorLanguage */
	flag = IANJUTA_IS_EDITOR_LANGUAGE (editor);
	
	action = anjuta_ui_get_action (ui, "ActionGroupEditorStyle",
								   "ActionMenuFormatStyle");
	g_object_set (G_OBJECT (action), "visible", flag, NULL);
	
	/* IAnjutaEditorSelection */
	flag = IANJUTA_IS_EDITOR_SELECTION (editor);
	
	action = anjuta_ui_get_action (ui, "ActionGroupEditorEdit",
								   "ActionEditCut");
	g_object_set (G_OBJECT (action), "visible", flag, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupEditorEdit",
								   "ActionEditCopy");
	g_object_set (G_OBJECT (action), "visible", flag, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupEditorEdit",
								   "ActionEditPaste");
	g_object_set (G_OBJECT (action), "visible", flag, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupEditorEdit",
								   "ActionEditClear");
	g_object_set (G_OBJECT (action), "visible", flag, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupEditorSelect",
								   "ActionEditSelectAll");
	g_object_set (G_OBJECT (action), "visible", flag, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupEditorSelect",
								   "ActionEditSelectToBrace");
	g_object_set (G_OBJECT (action), "visible", flag, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupEditorSelect",
								   "ActionEditSelectBlock");
	g_object_set (G_OBJECT (action), "visible", flag, NULL);
	
	/* IAnjutaEditorConvert */
	flag = IANJUTA_IS_EDITOR_CONVERT (editor);
	action = anjuta_ui_get_action (ui, "ActionGroupEditorTransform",
								   "ActionEditMakeSelectionUppercase");
	g_object_set (G_OBJECT (action), "visible", flag, NULL);
	
	action = anjuta_ui_get_action (ui, "ActionGroupEditorTransform",
								   "ActionEditMakeSelectionLowercase");
	g_object_set (G_OBJECT (action), "visible", flag, NULL);
	
	/* IAnjutaEditorLineMode */
	flag = IANJUTA_IS_EDITOR_LINE_MODE (editor);
	
	action = anjuta_ui_get_action (ui, "ActionGroupEditorTransform",
								   "ActionEditConvertCRLF");
	g_object_set (G_OBJECT (action), "visible", flag, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupEditorTransform",
								   "ActionEditConvertLF");
	g_object_set (G_OBJECT (action), "visible", flag, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupEditorTransform",
								   "ActionEditConvertCR");
	g_object_set (G_OBJECT (action), "visible", flag, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupEditorTransform",
								   "ActionEditConvertEOL");
	g_object_set (G_OBJECT (action), "visible", flag, NULL);
	
	/* IAnjutaEditorView */
	flag = IANJUTA_IS_EDITOR_VIEW (editor);
	action = anjuta_ui_get_action (ui, "ActionGroupEditorEdit",
								   "ActionViewEditorAddView");
	g_object_set (G_OBJECT (action), "visible", flag, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupEditorEdit",
								   "ActionViewEditorRemoveView");
	g_object_set (G_OBJECT (action), "visible", flag, NULL);
	
	/* IAnjutaEditorAssist */
	flag = IANJUTA_IS_EDITOR_ASSIST (editor);
	action = anjuta_ui_get_action (ui, "ActionGroupEditorEdit",
								   "ActionEditAutocomplete");
	g_object_set (G_OBJECT (action), "visible", flag, NULL);
	
	/* IAnjutaEditorFolds */
	flag = IANJUTA_IS_EDITOR_FOLDS(editor);
	action = anjuta_ui_get_action(ui, "ActionGroupEditorFormat", 
		"ActionFormatFoldCloseAll");
	g_object_set (G_OBJECT (action), "visible", flag, NULL);
	
	flag = IANJUTA_IS_EDITOR_FOLDS(editor);
	action = anjuta_ui_get_action(ui, "ActionGroupEditorFormat", 
		"ActionFormatFoldOpenAll");
	g_object_set (G_OBJECT (action), "visible", flag, NULL);

	flag = IANJUTA_IS_EDITOR_FOLDS(editor);
	action = anjuta_ui_get_action(ui, "ActionGroupEditorFormat", 
		"ActionFormatFoldToggle");
	g_object_set (G_OBJECT (action), "visible", flag, NULL);
	
	flag = IANJUTA_IS_EDITOR_FOLDS(editor);
	action = anjuta_ui_get_action(ui, "ActionGroupEditorView", 
		"ActionViewEditorFolds");
	g_object_set (G_OBJECT (action), "visible", flag, NULL);
	
	/* IAnjutaEditorComment */
	
	flag = IANJUTA_IS_EDITOR_COMMENT(editor);
	action = anjuta_ui_get_action(ui, "ActionGroupEditorComment", "ActionMenuEditComment");
	g_object_set(G_OBJECT (action), "visible", flag, NULL);
	
	/* IAnjutaEditorZoom */
	
	flag = IANJUTA_IS_EDITOR_ZOOM(editor);
	action = anjuta_ui_get_action(ui,  "ActionGroupEditorZoom", "ActionViewEditorZoomIn");
	g_object_set(G_OBJECT (action), "visible", flag, NULL);
	action = anjuta_ui_get_action(ui,  "ActionGroupEditorZoom", "ActionViewEditorZoomOut");
	g_object_set(G_OBJECT (action), "visible", flag, NULL);

	/* IAnjutaEditorGoto */
	flag = IANJUTA_IS_EDITOR_GOTO(editor);
	action = anjuta_ui_get_action(ui,  "ActionGroupEditorNavigate", "ActionEditGotoBlockStart");
	g_object_set(G_OBJECT (action), "visible", flag, NULL);
	action = anjuta_ui_get_action(ui,  "ActionGroupEditorNavigate", "ActionEditGotoBlockEnd");
	g_object_set(G_OBJECT (action), "visible", flag, NULL);
}

static void
update_editor_ui (AnjutaPlugin *plugin, IAnjutaEditor *editor)
{
	if (editor == NULL)
	{
		update_editor_ui_disable_all (plugin);
		return;
	}
	update_editor_ui_enable_all (plugin);
	update_editor_ui_save_items (plugin, editor);
	update_editor_ui_interface_items (plugin, editor);
}

static void
on_editor_update_save_ui (IAnjutaEditor *editor, gboolean entered,
						  AnjutaPlugin *plugin)
{
	update_editor_ui_save_items (plugin, editor);
}

#define REGISTER_ICON(icon, stock_id) \
	pixbuf = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"icon, NULL); \
	icon_set = gtk_icon_set_new_from_pixbuf (pixbuf); \
	gtk_icon_factory_add (icon_factory, stock_id, icon_set); \
	g_object_unref (pixbuf);

static void
register_stock_icons (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	GtkIconFactory *icon_factory;
	GtkIconSet *icon_set;
	GdkPixbuf *pixbuf;
	static gboolean registered = FALSE;

	if (registered)
		return;
	registered = TRUE;

	/* Register stock icons */
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	icon_factory = anjuta_ui_get_icon_factory (ui);
	REGISTER_ICON (ICON_FILE, "editor-plugin-icon");
	REGISTER_ICON (ANJUTA_PIXMAP_SWAP, ANJUTA_STOCK_SWAP);
	REGISTER_ICON (ANJUTA_PIXMAP_FOLD_TOGGLE, ANJUTA_STOCK_FOLD_TOGGLE);
	REGISTER_ICON (ANJUTA_PIXMAP_FOLD_OPEN, ANJUTA_STOCK_FOLD_OPEN);
	REGISTER_ICON (ANJUTA_PIXMAP_FOLD_CLOSE, ANJUTA_STOCK_FOLD_CLOSE);
	REGISTER_ICON (ANJUTA_PIXMAP_INDENT_INC, ANJUTA_STOCK_INDENT_INC);
	REGISTER_ICON (ANJUTA_PIXMAP_INDENT_DCR, ANJUTA_STOCK_INDENT_DCR);
	REGISTER_ICON (ANJUTA_PIXMAP_INDENT_AUTO, ANJUTA_STOCK_INDENT_AUTO);
	REGISTER_ICON (ANJUTA_PIXMAP_AUTOFORMAT_SETTING, ANJUTA_STOCK_AUTOFORMAT_SETTINGS);
	REGISTER_ICON (ANJUTA_PIXMAP_AUTOCOMPLETE, ANJUTA_STOCK_AUTOCOMPLETE);
	REGISTER_ICON (ANJUTA_PIXMAP_BLOCK_SELECT, ANJUTA_STOCK_BLOCK_SELECT);
	REGISTER_ICON (ANJUTA_PIXMAP_BOOKMARK_TOGGLE, ANJUTA_STOCK_BOOKMARK_TOGGLE);
	REGISTER_ICON (ANJUTA_PIXMAP_BOOKMARK_FIRST, ANJUTA_STOCK_BOOKMARK_FIRST);
	REGISTER_ICON (ANJUTA_PIXMAP_BOOKMARK_PREV, ANJUTA_STOCK_BOOKMARK_PREV);
	REGISTER_ICON (ANJUTA_PIXMAP_BOOKMARK_NEXT, ANJUTA_STOCK_BOOKMARK_NEXT);
	REGISTER_ICON (ANJUTA_PIXMAP_BOOKMARK_LAST, ANJUTA_STOCK_BOOKMARK_LAST);
	REGISTER_ICON (ANJUTA_PIXMAP_BOOKMARK_CLEAR, ANJUTA_STOCK_BOOKMARK_CLEAR);
	REGISTER_ICON (ANJUTA_PIXMAP_BLOCK_START, ANJUTA_STOCK_BLOCK_START);
	REGISTER_ICON (ANJUTA_PIXMAP_BLOCK_END, ANJUTA_STOCK_BLOCK_END);
}

#define TEXT_ZOOM_FACTOR           "text.zoom.factor"

static void
update_status (DocmanPlugin *plugin, IAnjutaEditor *te)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell, NULL);
	
	if (status == NULL)
		return;
	
	if (te)
	{
		gchar *edit /*, *mode*/;
		guint line, col, zoom;
		
		/* TODO: Implement this in IAnjutaEditor some kind
		gint editor_mode;
		editor_mode =  scintilla_send_message (SCINTILLA (te->widgets.editor),
											   SCI_GETEOLMODE, 0, 0);
		switch (editor_mode) {
			case SC_EOL_CRLF:
				mode = g_strdup(_("DOS (CRLF)"));
				break;
			case SC_EOL_LF:
				mode = g_strdup(_("Unix (LF)"));
				break;
			case SC_EOL_CR:
				mode = g_strdup(_("Mac (CR)"));
				break;
			default:
				mode = g_strdup(_("Unknown"));
				break;
		}*/
		
		zoom = anjuta_preferences_get_int (plugin->prefs, TEXT_ZOOM_FACTOR);
		line = ianjuta_editor_get_lineno (te, NULL);
		col = ianjuta_editor_get_column (te, NULL);
			
		if (ianjuta_editor_get_overwrite (te, NULL))
		{
			edit = g_strdup (_("OVR"));
		}
		else
		{
			edit = g_strdup (_("INS"));
		}
		anjuta_status_set_default (status, _("Zoom"), "%d", zoom);
		anjuta_status_set_default (status, _("Line"), "%04d", line);
		anjuta_status_set_default (status, _("Col"), "%03d", col);
		anjuta_status_set_default (status, _("Mode"), edit);
		// anjuta_status_set_default (status, _("EOL"), mode);
		
		g_free (edit);
		/* g_free (mode); */
	}
	else
	{
		anjuta_status_set_default (status, _("Zoom"), NULL);
		anjuta_status_set_default (status, _("Line"), NULL);
		anjuta_status_set_default (status, _("Col"), NULL);
		anjuta_status_set_default (status, _("Mode"), NULL);
		/* anjuta_status_set_default (status, _("EOL"), NULL); */
	}
}

static void
on_editor_update_ui (IAnjutaEditor *editor, DocmanPlugin *plugin)
{
	IAnjutaEditor *te;
	
	te = anjuta_docman_get_current_editor (ANJUTA_DOCMAN (plugin->docman));
	if (te == editor)
		update_status (plugin, te);
}

/* Remove all instances of c from the string s. */
static void remove_char(gchar *s, gchar c)
{
	gchar *t = s;
	for (; *s ; ++s)
		if (*s != c)
			*t++ = *s;
	*t = '\0';
}

/* Compare two strings, ignoring _ characters which indicate mneumonics.
 * Returns -1, 0, or 1, just like strcmp(). */
static gint
menu_name_compare(const gchar *s, const char *t)
{
	gchar *s1 = g_utf8_strdown(s, -1);
	gchar *t1 = g_utf8_strdown(t, -1);
	remove_char(s1, '_');
	remove_char(t1, '_');
	int result = g_utf8_collate(s1, t1);
	g_free(s1);
	g_free(t1);
	return result;
}

static gint
compare_language_name(const gchar *lang1, const gchar *lang2, IAnjutaEditorLanguage *manager)
{
	const gchar *fullname1 = ianjuta_editor_language_get_language_name (manager, lang1, NULL);
	const gchar *fullname2 = ianjuta_editor_language_get_language_name (manager, lang2, NULL);
	return menu_name_compare(fullname1, fullname2);
}

static GtkWidget*
create_highlight_submenu (DocmanPlugin *plugin, IAnjutaEditor *editor)
{
	const GList *languages, *node;
	GList *sorted_languages;
	GtkWidget *submenu;
	GtkWidget *menuitem;
	
	submenu = gtk_menu_new ();
	
	if (!editor || !IANJUTA_IS_EDITOR_LANGUAGE (editor))
		return NULL;
	
	languages = ianjuta_editor_language_get_supported_languages (IANJUTA_EDITOR_LANGUAGE (editor), NULL);
	if (!languages)
		return NULL;
	
	/* Automatic highlight menu */
	menuitem = gtk_menu_item_new_with_mnemonic (_("Automatic"));
	g_signal_connect (G_OBJECT (menuitem), "activate",
					  G_CALLBACK (on_force_hilite_activate),
					  plugin);
	g_object_set_data(G_OBJECT(menuitem), "language_code", "auto-detect");
	gtk_menu_shell_append (GTK_MENU_SHELL (submenu), menuitem);
	gtk_menu_shell_append (GTK_MENU_SHELL (submenu), gtk_separator_menu_item_new());

	/* Sort languages so they appear alphabetically in the menu. */
	sorted_languages = g_list_copy((GList *) languages);
	sorted_languages = g_list_sort_with_data(sorted_languages, (GCompareDataFunc) compare_language_name,
											 IANJUTA_EDITOR_LANGUAGE (editor));
	
	node = sorted_languages;
	while (node)
	{
		const gchar *lang = node->data;
		const gchar *name = ianjuta_editor_language_get_language_name (IANJUTA_EDITOR_LANGUAGE (editor), lang, NULL);
		
		menuitem = gtk_menu_item_new_with_mnemonic (name);
		g_object_set_data_full (G_OBJECT (menuitem), "language_code",
								g_strdup (lang),
								(GDestroyNotify)g_free);
		g_signal_connect (G_OBJECT (menuitem), "activate",
						  G_CALLBACK (on_force_hilite_activate),
						  plugin);
		gtk_menu_shell_append (GTK_MENU_SHELL (submenu), menuitem);
		node = g_list_next (node);
	}
	g_list_free(sorted_languages);
	
	gtk_widget_show_all (submenu);
	return submenu;
}

static void
on_editor_added (AnjutaDocman *docman, IAnjutaEditor *te,
				   AnjutaPlugin *plugin)
{
	GtkWidget *highlight_submenu, *highlight_menu;
	DocmanPlugin *editor_plugin = ANJUTA_PLUGIN_DOCMAN (plugin);
	
	/* Create Highlight submenu */
	highlight_submenu = 
		create_highlight_submenu (editor_plugin, te);
	if (highlight_submenu)
	{
		highlight_menu =
			gtk_ui_manager_get_widget (GTK_UI_MANAGER (editor_plugin->ui),
						"/MenuMain/MenuView/MenuViewEditor/MenuFormatStyle");
		gtk_menu_item_set_submenu (GTK_MENU_ITEM (highlight_menu),
								   highlight_submenu);
	}
	g_signal_connect (G_OBJECT (te), "update-ui",
					  G_CALLBACK (on_editor_update_ui),
					  ANJUTA_PLUGIN (plugin));
	g_signal_connect (G_OBJECT (te), "save-point",
					  G_CALLBACK (on_editor_update_save_ui),
					  ANJUTA_PLUGIN (plugin));
	anjuta_shell_present_widget (plugin->shell,
								 GTK_WIDGET (docman), NULL);
}

static void
on_editor_changed (AnjutaDocman *docman, IAnjutaEditor *te,
				   AnjutaPlugin *plugin)
{
	DocmanPlugin *docman_plugin = ANJUTA_PLUGIN_DOCMAN (plugin);
	
	update_status (docman_plugin, te);
	update_editor_ui (plugin, te);
		
	/* unload previous support plugins */
	if (docman_plugin->support_plugins) {
		g_list_foreach (docman_plugin->support_plugins,
						(GFunc) anjuta_plugin_deactivate, NULL);
		g_list_free (docman_plugin->support_plugins);
		docman_plugin->support_plugins = NULL;
	}
	
	if (te)
	{
		AnjutaPluginManager *plugin_manager;
		const gchar *language;
		GList *support_plugin_descs, *node;
		
		GValue *value = g_new0 (GValue, 1);
		g_value_init (value, G_TYPE_OBJECT);
		g_value_set_object (value, te);
		anjuta_shell_add_value (plugin->shell,
								"document_manager_current_editor",
								value, NULL);
		DEBUG_PRINT ("Editor Added");
		
		/* Load current language editor support plugins */
		plugin_manager = anjuta_shell_get_plugin_manager (plugin->shell, NULL);
		if (IANJUTA_IS_EDITOR_LANGUAGE (te)) {
			language = ianjuta_editor_language_get_language (IANJUTA_EDITOR_LANGUAGE (te), NULL);
			support_plugin_descs = anjuta_plugin_manager_query (plugin_manager,
																"Anjuta Plugin",
																"Interfaces",
																"IAnjutaLanguageSupport",
																"Language Support",
																"Languages",
																language, NULL);
			node = support_plugin_descs;
			while (node) {
				gchar *plugin_id;
				GObject *plugin_object;
				
				AnjutaPluginDescription *desc = node->data;
				anjuta_plugin_description_get_string (desc, "Anjuta Plugin", "Location",
													  &plugin_id);
				plugin_object = anjuta_plugin_manager_get_plugin_by_id (plugin_manager,
																		plugin_id);
				/* anjuta_plugin_activate (ANJUTA_PLUGIN (plugin_object)); */
				if (plugin_object)
				{
					DEBUG_PRINT("Loaded language support plugin: %s", plugin_id);
					docman_plugin->support_plugins = g_list_prepend (docman_plugin->support_plugins,
																	 plugin_object);
				}
				else
				{
					DEBUG_PRINT ("Failed to load support plugin %s", plugin_id);
				}
				node = node->next;
				g_free (plugin_id);
			}
			g_list_free (support_plugin_descs);
		}
	}
	else
	{
		anjuta_shell_remove_value (plugin->shell,
								   "document_manager_current_editor", NULL);
		DEBUG_PRINT ("Editor Removed");
	}
}

static void
on_edit_editor_indent (GtkWidget *button, DocmanPlugin *plugin)
{
	IndentData *idt = plugin->idt;

	if (idt->dialog == NULL)
		idt->dialog = create_dialog(idt);
	gtk_widget_show(idt->dialog);
}

static void
on_style_combo_changed (GtkComboBox *combo, DocmanPlugin *plugin)
{
	IndentData *idt = plugin->idt;

	pref_style_combo_changed(combo, idt);
}

static gint
on_window_key_press_event (GtkWidget   *widget,
				  GdkEventKey *event,
				  DocmanPlugin *plugin)
{
	int modifiers;
	int i;

	g_return_val_if_fail (event != NULL, FALSE);

	modifiers = event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK);
  
	for (i = 0; global_keymap[i].id; i++)
		if (event->keyval == global_keymap[i].gdk_key &&
		    (event->state & global_keymap[i].modifiers) == global_keymap[i].modifiers)
			break;

	if (!global_keymap[i].id)
		return FALSE;

	switch (global_keymap[i].id) {
	case ID_NEXTBUFFER:
	case ID_PREVBUFFER: {
		GtkNotebook *notebook = GTK_NOTEBOOK (plugin->docman);
		int pages_nb;
		int cur_page;

		if (!notebook->children)
			return FALSE;

		if (!plugin->g_tabbing)
		{
			plugin->g_tabbing = TRUE;
		}

		pages_nb = g_list_length (notebook->children);
		cur_page = gtk_notebook_get_current_page (notebook);

		if (global_keymap[i].id == ID_NEXTBUFFER)
			cur_page = (cur_page < pages_nb - 1) ? cur_page + 1 : 0;
		else
			cur_page = cur_page ? cur_page - 1 : pages_nb -1;

		gtk_notebook_set_page (notebook, cur_page);

		break;
	}
	default:
		if (global_keymap[i].id >= ID_FIRSTBUFFER &&
		  global_keymap[i].id <= (ID_FIRSTBUFFER + 9))
		{
			GtkNotebook *notebook = GTK_NOTEBOOK (plugin->docman);
			int page_req = global_keymap[i].id - ID_FIRSTBUFFER;

			if (!notebook->children)
				return FALSE;
			gtk_notebook_set_page(notebook, page_req);
		}
		else
			return FALSE;
	}

	/* Note: No reason for a shortcut to do more than one thing a time */
	gtk_signal_emit_stop_by_name (GTK_OBJECT (ANJUTA_PLUGIN(plugin)->shell),
								  "key-press-event");

	return TRUE;
}

#define EDITOR_TABS_RECENT_FIRST   "editor.tabs.recent.first"
#define EDITOR_TABS_POS            "editor.tabs.pos"
#define EDITOR_TABS_HIDE           "editor.tabs.hide"
#define EDITOR_TABS_ORDERING       "editor.tabs.ordering"
#define AUTOSAVE_TIMER             "autosave.timer"
#define SAVE_AUTOMATIC             "save.automatic"

static gint
on_window_key_release_event (GtkWidget   *widget,
				  GdkEventKey *event,
				  DocmanPlugin *plugin)
{
	g_return_val_if_fail (event != NULL, FALSE);

	if (plugin->g_tabbing && ((event->keyval == GDK_Control_L) ||
		(event->keyval == GDK_Control_R)))
	{
		GtkNotebook *notebook = GTK_NOTEBOOK (plugin->docman);
		GtkWidget *widget;
		int cur_page;
		plugin->g_tabbing = FALSE;
		
		if (anjuta_preferences_get_int (plugin->prefs,
										EDITOR_TABS_RECENT_FIRST))
		{
			/*
			TTimo: move the current notebook page to first position
			that maintains Ctrl-TABing on a list of most recently edited files
			*/
			cur_page = gtk_notebook_get_current_page (notebook);
			widget = gtk_notebook_get_nth_page (notebook, cur_page);
			gtk_notebook_reorder_child (notebook, widget, 0);
		}
	}
	return FALSE;
}

static void
on_session_save (AnjutaShell *shell, AnjutaSessionPhase phase,
				 AnjutaSession *session, DocmanPlugin *plugin)
{
	GList *editors, *node, *files;
	
	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;
	
	files = anjuta_session_get_string_list (session, "File Loader", "Files");
	files = g_list_reverse (files);
	
	editors = anjuta_docman_get_all_editors (ANJUTA_DOCMAN (plugin->docman));
	node = editors;
	while (node)
	{
		IAnjutaEditor *te;
		te = IANJUTA_EDITOR (node->data);
		if (ianjuta_file_get_uri(IANJUTA_FILE(te), NULL))
		{
			gchar *uri;
			/* Save line locations also */
			uri = g_strdup_printf ("%s#%d", ianjuta_file_get_uri(IANJUTA_FILE(te), NULL),
								  ianjuta_editor_get_lineno(IANJUTA_EDITOR(te), NULL));
			files = g_list_prepend (files, uri);
			/* uri not be freed here */
		}
		node = g_list_next (node);
	}
	files = g_list_reverse (files);
	anjuta_session_set_string_list (session, "File Loader", "Files", files);
	g_list_free (editors);
	g_list_foreach (files, (GFunc)g_free, NULL);
	g_list_free (files);
}

static gboolean
on_save_prompt_save_editor (AnjutaSavePrompt *save_prompt,
							gpointer item, gpointer user_data)
{
	DocmanPlugin *plugin;
	
	plugin = ANJUTA_PLUGIN_DOCMAN (user_data);
	return anjuta_docman_save_editor (ANJUTA_DOCMAN (plugin->docman),
									  IANJUTA_EDITOR (item),
									  GTK_WIDGET (save_prompt));
}

static void
on_save_prompt (AnjutaShell *shell, AnjutaSavePrompt *save_prompt,
				DocmanPlugin *plugin)
{
	GList *list, *node;
	
	list = anjuta_docman_get_all_editors (ANJUTA_DOCMAN (plugin->docman));
	node = list;
	while (node)
	{
		IAnjutaFileSavable *editor = IANJUTA_FILE_SAVABLE (node->data);
		if (ianjuta_file_savable_is_dirty (editor, NULL))
		{
			const gchar *name;
			const gchar *uri;
			
			name = ianjuta_editor_get_filename (IANJUTA_EDITOR (editor), NULL);
			uri = ianjuta_file_get_uri (IANJUTA_FILE (editor), NULL);
			anjuta_save_prompt_add_item (save_prompt, name, uri, editor,
										 on_save_prompt_save_editor, plugin);
		}
		node = g_list_next (node);
	}
}

static void
docman_plugin_set_tab_pos (DocmanPlugin *ep)
{
	if (anjuta_preferences_get_int_with_default (ep->prefs, EDITOR_TABS_HIDE,
												 1))
	{
		gtk_notebook_set_show_tabs (GTK_NOTEBOOK (ep->docman), FALSE);
	}
	else
	{
		gchar *tab_pos;
		GtkPositionType pos;
		
		gtk_notebook_set_show_tabs (GTK_NOTEBOOK (ep->docman), TRUE);
		tab_pos = anjuta_preferences_get (ep->prefs, EDITOR_TABS_POS);
		
		pos = GTK_POS_TOP;
		if (tab_pos)
		{
			if (strcasecmp (tab_pos, "left") == 0)
				pos = GTK_POS_LEFT;
			else if (strcasecmp (tab_pos, "right") == 0)
				pos = GTK_POS_RIGHT;
			else if (strcasecmp (tab_pos, "bottom") == 0)
				pos = GTK_POS_BOTTOM;
			g_free (tab_pos);
		}
		gtk_notebook_set_tab_pos (GTK_NOTEBOOK (ep->docman), pos);
	}
}

static void
on_gconf_notify_prefs (GConfClient *gclient, guint cnxn_id,
					   GConfEntry *entry, gpointer user_data)
{
	DocmanPlugin *ep = ANJUTA_PLUGIN_DOCMAN (user_data);
	docman_plugin_set_tab_pos (ep);
}

static gboolean
on_docman_auto_save (gpointer data)
{
	DocmanPlugin *plugin = ANJUTA_PLUGIN_DOCMAN (data);
	AnjutaShell* shell;
	AnjutaPreferences* prefs;
	AnjutaDocman *docman;
	AnjutaStatus* status;
	IAnjutaEditor* editor;
	GList* editors;
	docman = ANJUTA_DOCMAN (plugin->docman);
	
	g_object_get(G_OBJECT(plugin), "shell", &shell, NULL);
	prefs = anjuta_shell_get_preferences(shell, NULL);
	status = anjuta_shell_get_status(shell, NULL);
	
	if (!docman)
		return FALSE;
	if (anjuta_preferences_get_int (prefs, SAVE_AUTOMATIC) == FALSE)
	{
		plugin->autosave_on = FALSE;
		return FALSE;
	}
	
	editors = anjuta_docman_get_all_editors(docman);
	while(editors)
	{
		editor = IANJUTA_EDITOR(editors->data);
		if (ianjuta_file_savable_is_dirty(IANJUTA_FILE_SAVABLE(editor), NULL))
		{
			if (ianjuta_file_get_uri(IANJUTA_FILE(editor), NULL) != NULL)
			{
				ianjuta_file_savable_save(IANJUTA_FILE_SAVABLE(editor), NULL);
			}
		}
		editors = g_list_next(editors);
	}
	// TODO: Check for errors
	{
	   gchar *mesg = NULL;
	   mesg = g_strdup("Autosaved complete");                                          
	   anjuta_status (status, mesg, 3);
	   g_free(mesg);
	}
	return TRUE;
}

static void
on_gconf_notify_timer (GConfClient *gclient, guint cnxn_id,
					   GConfEntry *entry, gpointer user_data)
{
	DocmanPlugin *ep = ANJUTA_PLUGIN_DOCMAN (user_data);
	AnjutaShell* shell;
	AnjutaPreferences* prefs;
	gint auto_save_timer;
	gboolean auto_save;
	
	g_object_get(G_OBJECT(ep), "shell", &shell, NULL);
	prefs = anjuta_shell_get_preferences(shell, NULL);
	
	auto_save_timer = anjuta_preferences_get_int(prefs, AUTOSAVE_TIMER);
	auto_save = anjuta_preferences_get_int(prefs, SAVE_AUTOMATIC);
	
	if (auto_save)
	{
		if (ep->autosave_on == TRUE)
		{
			if (auto_save_timer != ep->autosave_it)
			{
				gtk_timeout_remove (ep->autosave_id);
				ep->autosave_id =
					gtk_timeout_add (auto_save_timer *
							 60000,
							 on_docman_auto_save,
							 ep);
			}
		}
		else
		{
			ep->autosave_id =
				gtk_timeout_add (auto_save_timer * 60000,
						 on_docman_auto_save,
						 ep);
		}
		ep->autosave_it = auto_save_timer;
		ep->autosave_on = TRUE;
	}
	else
	{
		if (ep->autosave_on == TRUE)
			gtk_timeout_remove (ep->autosave_id);
		ep->autosave_on = FALSE;
	}
}

#define REGISTER_NOTIFY(key, func) \
	notify_id = anjuta_preferences_notify_add (ep->prefs, \
											   key, func, ep, NULL); \
	ep->gconf_notify_ids = g_list_prepend (ep->gconf_notify_ids, \
										   (gpointer)(notify_id));
static void
prefs_init (DocmanPlugin *ep)
{
	guint notify_id;
	docman_plugin_set_tab_pos (ep);
	REGISTER_NOTIFY (EDITOR_TABS_HIDE, on_gconf_notify_prefs);
	REGISTER_NOTIFY (EDITOR_TABS_POS, on_gconf_notify_prefs);
	REGISTER_NOTIFY (AUTOSAVE_TIMER, on_gconf_notify_timer);
	REGISTER_NOTIFY (SAVE_AUTOMATIC, on_gconf_notify_timer);
	
	on_gconf_notify_timer(NULL,0,NULL, ep);
}

static void
prefs_finalize (DocmanPlugin *ep)
{
	GList *node;
	node = ep->gconf_notify_ids;
	while (node)
	{
		anjuta_preferences_notify_remove (ep->prefs, (guint)node->data);
		node = g_list_next (node);
	}
	g_list_free (ep->gconf_notify_ids);
	ep->gconf_notify_ids = NULL;
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	GtkWidget *docman, *popup_menu;
	AnjutaUI *ui;
	DocmanPlugin *editor_plugin;
	GtkActionGroup *group;
	GtkAction *action;
	gint i;
	AnjutaStatus *status;
	static gboolean initialized = FALSE;
	
	DEBUG_PRINT ("DocmanPlugin: Activating Editor plugin...");
	
	editor_plugin = ANJUTA_PLUGIN_DOCMAN (plugin);
	editor_plugin->ui = anjuta_shell_get_ui (plugin->shell, NULL);
	editor_plugin->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	status = anjuta_shell_get_status (plugin->shell, NULL);
	
	ui = editor_plugin->ui;
	docman = anjuta_docman_new (editor_plugin, editor_plugin->prefs);
	editor_plugin->docman = docman;
	ANJUTA_DOCMAN(docman)->shell = plugin->shell;
	g_signal_connect (G_OBJECT (docman), "editor-added",
					  G_CALLBACK (on_editor_added), plugin);
	g_signal_connect (G_OBJECT (docman), "editor-changed",
					  G_CALLBACK (on_editor_changed), plugin);
	g_signal_connect_swapped (G_OBJECT (status), "busy",
							  G_CALLBACK (anjuta_docman_set_busy), docman);
	g_signal_connect (G_OBJECT (plugin->shell), "key-press-event",
					  G_CALLBACK (on_window_key_press_event), plugin);
	g_signal_connect (G_OBJECT (plugin->shell), "key-release-event",
					  G_CALLBACK (on_window_key_release_event), plugin);
	
	if (!initialized)
	{
		register_stock_icons (plugin);
	}
	editor_plugin->action_groups = NULL;
	/* Add all our editor actions */
	for (i = 0; i < G_N_ELEMENTS (action_groups); i++)
	{
		GList *actions, *act;
		if (!initialized)
			swap_label_and_stock (action_groups[i].group,
								  action_groups[i].size);
		group = anjuta_ui_add_action_group_entries (ui, 
													action_groups[i].name,
													_(action_groups[i].label),
													action_groups[i].group,
													action_groups[i].size,
													GETTEXT_PACKAGE, TRUE,
													plugin);
		editor_plugin->action_groups =
			g_list_prepend (editor_plugin->action_groups, group);
		actions = gtk_action_group_list_actions (group);
		act = actions;
		while (act) {
			g_object_set_data (G_OBJECT (act->data), "Plugin", editor_plugin);
			act = g_list_next (act);
		}
	}
	for (i = 0; i < G_N_ELEMENTS (action_toggle_groups); i++)
	{
		GList *actions, *act;
		if (!initialized)
			swap_toggle_label_and_stock (action_toggle_groups[i].group,
										 action_toggle_groups[i].size);
		group = anjuta_ui_add_toggle_action_group_entries (ui, 
												action_toggle_groups[i].name,
												_(action_toggle_groups[i].label),
												action_toggle_groups[i].group,
												action_toggle_groups[i].size,
												GETTEXT_PACKAGE, TRUE, plugin);
		editor_plugin->action_groups =
			g_list_prepend (editor_plugin->action_groups, group);
		actions = gtk_action_group_list_actions (group);
		act = actions;
		while (act) {
			g_object_set_data (G_OBJECT (act->data), "Plugin", editor_plugin);
			act = g_list_next (act);
		}
	}
	group = gtk_action_group_new ("ActionGroupNavigation");
	editor_plugin->action_groups =
		g_list_prepend (editor_plugin->action_groups, group);
	
	action = g_object_new (EGG_TYPE_ENTRY_ACTION,
						   "name", "ActionEditGotoLineEntry",
						   "label", _("Goto line"),
						   "tooltip", _("Enter the line number to jump and press enter"),
						   "stock_id", GTK_STOCK_JUMP_TO,
						   "width", 50,
							NULL);
	g_signal_connect (action, "activate",
					  G_CALLBACK (on_toolbar_goto_clicked), plugin);
	gtk_action_group_add_action (group, action);
	
	anjuta_ui_add_action_group (ui, "ActionGroupNavigation",
								N_("Editor quick navigations"),
								group, FALSE);
	
	/* Add UI */
	editor_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);
	anjuta_shell_add_widget (plugin->shell, docman,
							 "AnjutaDocumentManager", _("Documents"),
							 "editor-plugin-icon",
							 ANJUTA_SHELL_PLACEMENT_CENTER, NULL); 
	ui_states_init(plugin);
	ui_give_shorter_names (plugin);
	update_editor_ui (plugin, NULL);
	
	/* Setup popup menu */
	popup_menu = gtk_ui_manager_get_widget (GTK_UI_MANAGER (ui),
											"/PopupDocumentManager");
	g_assert (popup_menu != NULL && GTK_IS_MENU (popup_menu));
	anjuta_docman_set_popup_menu (ANJUTA_DOCMAN (docman), popup_menu);
	if (!initialized)
	{
		//search_and_replace_init (ANJUTA_DOCMAN (docman));
	}
	/* Connect to save session */
	g_signal_connect (G_OBJECT (plugin->shell), "save-session",
					  G_CALLBACK (on_session_save), plugin);
	/* Connect to save prompt */
	g_signal_connect (G_OBJECT (plugin->shell), "save-prompt",
					  G_CALLBACK (on_save_prompt), plugin);
	
	initialized = TRUE;
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	// GtkIconFactory *icon_factory;
	DocmanPlugin *eplugin;
	AnjutaUI *ui;
	AnjutaStatus *status;
	GList *node;
	
	DEBUG_PRINT ("DocmanPlugin: Dectivating Editor plugin...");
	
	eplugin = ANJUTA_PLUGIN_DOCMAN (plugin);

	prefs_finalize (eplugin);
	g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->shell),
										  G_CALLBACK (on_session_save), plugin);
	g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->shell),
										  G_CALLBACK (on_save_prompt), plugin);
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	status = anjuta_shell_get_status (plugin->shell, NULL);
	
	g_signal_handlers_disconnect_by_func (G_OBJECT (status),
										  G_CALLBACK (anjuta_docman_set_busy),
										  eplugin->docman);
	g_signal_handlers_disconnect_by_func (G_OBJECT (eplugin->docman),
										  G_CALLBACK (on_editor_changed),
										  plugin);
	g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->shell),
										  G_CALLBACK (on_window_key_press_event),
										  plugin);
	g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->shell),
										  G_CALLBACK (on_window_key_release_event),
										  plugin);
	
	on_editor_changed (ANJUTA_DOCMAN (eplugin->docman), NULL, plugin);
	
	/* Widget is removed from the container when destroyed */
	gtk_widget_destroy (eplugin->docman);
	anjuta_ui_unmerge (ui, eplugin->uiid);
	node = eplugin->action_groups;
	while (node)
	{
		GtkActionGroup *group = GTK_ACTION_GROUP (node->data);
		anjuta_ui_remove_action_group (ui, group);
		node = g_list_next (node);
	}
	g_list_free (eplugin->action_groups);
	
	/* FIXME: */
	/* Unregister stock icons */
	/* Unregister preferences */
	
	eplugin->docman = NULL;
	eplugin->uiid = 0;
	eplugin->action_groups = NULL;
	
	return TRUE;
}

static void
dispose (GObject *obj)
{
	// DocmanPlugin *eplugin = ANJUTA_PLUGIN_DOCMAN (obj);
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
docman_plugin_instance_init (GObject *obj)
{
	DocmanPlugin *plugin = ANJUTA_PLUGIN_DOCMAN (obj);
	plugin->uiid = 0;
	plugin->g_tabbing = FALSE;
	plugin->gconf_notify_ids = NULL;
	plugin->idt = indent_init(plugin->prefs);
}

static void
docman_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
}

/* Implement IAnjutaDocumentManager interfaces */
static const gchar *
ianjuta_docman_get_full_filename (IAnjutaDocumentManager *plugin,
		const gchar *file, GError **e)
{
	AnjutaDocman *docman = ANJUTA_DOCMAN ((ANJUTA_PLUGIN_DOCMAN (plugin)->docman));
	return anjuta_docman_get_full_filename (ANJUTA_DOCMAN (docman), file);
}

static IAnjutaEditor*
ianjuta_docman_find_editor_with_path (IAnjutaDocumentManager *plugin,
		const gchar *file_path, GError **e)
{
	AnjutaDocman *docman = ANJUTA_DOCMAN ((ANJUTA_PLUGIN_DOCMAN (plugin)->docman));
	return anjuta_docman_find_editor_with_path (ANJUTA_DOCMAN (docman), file_path);
}

static IAnjutaEditor*
ianjuta_docman_get_current_editor (IAnjutaDocumentManager *plugin, GError **e)
{
	AnjutaDocman *docman = ANJUTA_DOCMAN ((ANJUTA_PLUGIN_DOCMAN (plugin)->docman));
	return anjuta_docman_get_current_editor (ANJUTA_DOCMAN (docman));
}

static void
ianjuta_docman_set_current_editor (IAnjutaDocumentManager *plugin,
								   IAnjutaEditor *editor, GError **e)
{
	AnjutaDocman *docman = ANJUTA_DOCMAN ((ANJUTA_PLUGIN_DOCMAN (plugin)->docman));
	anjuta_docman_set_current_editor (ANJUTA_DOCMAN (docman),
									  editor);
}

static GList*
ianjuta_docman_get_editors (IAnjutaDocumentManager *plugin, GError **e)
{
	AnjutaDocman *docman;
	GList * editors = NULL;
	docman = ANJUTA_DOCMAN ((ANJUTA_PLUGIN_DOCMAN (plugin)->docman));
	editors = anjuta_docman_get_all_editors (docman);
	return editors;
}

static IAnjutaEditor*
ianjuta_docman_goto_file_line (IAnjutaDocumentManager *plugin,
							   const gchar *uri, gint linenum, GError **e)
{
	AnjutaDocman *docman;
	docman = ANJUTA_DOCMAN ((ANJUTA_PLUGIN_DOCMAN (plugin)->docman));
	return anjuta_docman_goto_file_line (docman, uri, linenum);
}

static IAnjutaEditor*
ianjuta_docman_goto_file_line_mark (IAnjutaDocumentManager *plugin,
		const gchar *uri, gint linenum, gboolean mark, GError **e)
{
	AnjutaDocman *docman;
	docman = ANJUTA_DOCMAN ((ANJUTA_PLUGIN_DOCMAN (plugin)->docman));
	return anjuta_docman_goto_file_line_mark (docman, uri, linenum, mark);
}

static IAnjutaEditor*
ianjuta_docman_add_buffer (IAnjutaDocumentManager *plugin,
						   const gchar *filename, const gchar *content,
						   GError **e)
{
	AnjutaDocman *docman;
	IAnjutaEditor *te;
	docman = ANJUTA_DOCMAN ((ANJUTA_PLUGIN_DOCMAN (plugin)->docman));
	te = anjuta_docman_add_editor (docman, NULL, filename);
	if (te)
	{
		/*if (content && strlen (content) > 0)
			aneditor_command (te->editor_id, ANE_INSERTTEXT, -1,
							  (long)content);*/
		return IANJUTA_EDITOR (te);
	}
	return NULL;
}

static gboolean
ianjuta_docman_remove_buffer (IAnjutaDocumentManager *plugin,
							  IAnjutaEditor *editor,
							  gboolean save_before, GError **e)
{
	gint ret_val = TRUE;
	AnjutaDocman *docman;
	docman = ANJUTA_DOCMAN ((ANJUTA_PLUGIN_DOCMAN (plugin)->docman));
	
	if (save_before)
	{
		ret_val = anjuta_docman_save_editor (docman, editor,
								 GTK_WIDGET (ANJUTA_PLUGIN (plugin)->shell));
	}
	if (ret_val)
		anjuta_docman_remove_editor (docman, editor);
	
	return ret_val;
}

static void
ianjuta_document_manager_iface_init (IAnjutaDocumentManagerIface *iface)
{
	iface->get_full_filename = ianjuta_docman_get_full_filename;
	iface->find_editor_with_path = ianjuta_docman_find_editor_with_path;
	iface->get_current_editor = ianjuta_docman_get_current_editor;
	iface->set_current_editor = ianjuta_docman_set_current_editor;
	iface->get_editors = ianjuta_docman_get_editors;
	iface->goto_file_line = ianjuta_docman_goto_file_line;
	iface->goto_file_line_mark = ianjuta_docman_goto_file_line_mark;
	iface->add_buffer = ianjuta_docman_add_buffer;
	iface->remove_buffer = ianjuta_docman_remove_buffer;
}

/* Implement IAnjutaFile interface */
static void
ifile_open (IAnjutaFile* plugin, const gchar* uri, GError** e)
{
	AnjutaDocman *docman;
	
	docman = ANJUTA_DOCMAN ((ANJUTA_PLUGIN_DOCMAN (plugin)->docman));
	anjuta_docman_goto_file_line (docman, uri, -1);
}

static gchar*
ifile_get_uri (IAnjutaFile* plugin, GError** e)
{
	AnjutaDocman *docman;
	IAnjutaEditor* editor;
	docman = ANJUTA_DOCMAN ((ANJUTA_PLUGIN_DOCMAN (plugin)->docman));
	editor = anjuta_docman_get_current_editor (docman);
	if (editor != NULL)
		return g_strdup (ianjuta_file_get_uri(IANJUTA_FILE(editor), NULL));
	else if (ianjuta_editor_get_filename(editor, NULL))
		return gnome_vfs_get_uri_from_local_path (ianjuta_editor_get_filename(editor, NULL));
	else
		return NULL;
}

static void
ifile_iface_init (IAnjutaFileIface *iface)
{
	iface->open = ifile_open;
	iface->get_uri = ifile_get_uri;
}

/* Implement IAnjutaFileSavable interface */	
static void
isaveable_save (IAnjutaFileSavable* plugin, GError** e)
{
	/* Save all editors */
	AnjutaDocman *docman;
	IAnjutaEditor* editor;
	GList* editors;
	docman = ANJUTA_DOCMAN ((ANJUTA_PLUGIN_DOCMAN (plugin)->docman));
	editors = anjuta_docman_get_all_editors(docman);
	while(editors)
	{
		editor = IANJUTA_EDITOR(editors->data);
		if (ianjuta_file_get_uri(IANJUTA_FILE(editor), NULL) != NULL &&
			ianjuta_file_savable_is_dirty(IANJUTA_FILE_SAVABLE(editor), NULL))
		{
			ianjuta_file_savable_save(IANJUTA_FILE_SAVABLE(editor), NULL);
		}
		editors = g_list_next(editors);
	}
	g_list_free(editors);
}

static void
isavable_save_as (IAnjutaFileSavable* plugin, const gchar* uri, GError** e)
{
	DEBUG_PRINT("save_as: Not implemented in DocmanPlugin");
}

static gboolean
isavable_is_dirty(IAnjutaFileSavable* plugin, GError** e)
{
	/* Is any editor unsaved */
	AnjutaDocman *docman;
	IAnjutaEditor* editor;
	GList* editors;
	gboolean retval = FALSE;
	docman = ANJUTA_DOCMAN ((ANJUTA_PLUGIN_DOCMAN (plugin)->docman));
	editors = anjuta_docman_get_all_editors(docman);
	while(editors)
	{
		editor = IANJUTA_EDITOR(editors->data);
		if (ianjuta_file_savable_is_dirty(IANJUTA_FILE_SAVABLE(editor), NULL))
		{
			retval = TRUE;
			break;
		}
		editors = g_list_next(editors);
	}
	g_list_free(editors);
	return retval;	
}

static void
isavable_set_dirty(IAnjutaFileSavable* plugin, gboolean dirty, GError** e)
{
	DEBUG_PRINT("set_dirty: Not implemented in DocmanPlugin");
}

static void
isavable_iface_init (IAnjutaFileSavableIface *iface)
{
	iface->save = isaveable_save;
	iface->save_as = isavable_save_as;
	iface->set_dirty = isavable_set_dirty;
	iface->is_dirty = isavable_is_dirty;
}

static void
ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	GtkWidget *indent_button;
	GtkWidget *indent_combo;
	GtkWidget *indent_entry;
	GladeXML* gxml;
		
	AnjutaPlugin* plugin = ANJUTA_PLUGIN(ipref);
		
	/* Add preferences */
	gxml = glade_xml_new (PREFS_GLADE, "preferences_dialog", NULL);
	indent_button = glade_xml_get_widget (gxml, "set_indent_button");
	g_signal_connect (G_OBJECT (indent_button), "clicked",
						  G_CALLBACK (on_edit_editor_indent), plugin);
		
	anjuta_preferences_add_page (prefs,
									gxml, "Documents", _("Documents"),  ICON_FILE);
	anjuta_encodings_init (prefs, gxml);
		
	indent_combo = glade_xml_get_widget (gxml, "pref_indent_style_combobox");
	ANJUTA_PLUGIN_DOCMAN (plugin)->idt->pref_indent_combo = indent_combo;
	g_signal_connect (G_OBJECT (indent_combo), "changed",
						  G_CALLBACK (on_style_combo_changed), plugin);
		
		
	indent_entry = glade_xml_get_widget (gxml, "preferences_style_entry");
	ANJUTA_PLUGIN_DOCMAN (plugin)->idt->pref_indent_options = indent_entry;
	ANJUTA_PLUGIN_DOCMAN (plugin)->idt->prefs = ANJUTA_PLUGIN_DOCMAN (plugin)->prefs;
	indent_init_load_style(ANJUTA_PLUGIN_DOCMAN (plugin)->idt);
		
	g_object_unref (G_OBJECT (gxml));
	prefs_init(ANJUTA_PLUGIN_DOCMAN (plugin));
	pref_set_style_combo(ANJUTA_PLUGIN_DOCMAN (plugin)->idt); 
}

static void
ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	DocmanPlugin* plugin = ANJUTA_PLUGIN_DOCMAN (ipref);
	prefs_finalize(plugin);
	anjuta_preferences_dialog_remove_page(ANJUTA_PREFERENCES_DIALOG(prefs), 
		_("Documents"));
}

static void
ipreferences_iface_init(IAnjutaPreferencesIface* iface)
{
	iface->merge = ipreferences_merge;
	iface->unmerge = ipreferences_unmerge;	
}

ANJUTA_PLUGIN_BEGIN (DocmanPlugin, docman_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(ianjuta_document_manager, IANJUTA_TYPE_DOCUMENT_MANAGER);
ANJUTA_PLUGIN_ADD_INTERFACE(ifile, IANJUTA_TYPE_FILE);
ANJUTA_PLUGIN_ADD_INTERFACE(isavable, IANJUTA_TYPE_FILE_SAVABLE);
ANJUTA_PLUGIN_ADD_INTERFACE(ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (DocmanPlugin, docman_plugin);
