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
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-stock.h>

#include <libegg/menu/egg-entry-action.h>
// #include <libegg/toolbar/eggtoolbar.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>

#include "print.h"
#include "anjuta-docman.h"
#include "action-callbacks.h"
#include "aneditor.h"
#include "lexer.h"
#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-document-manager.ui"

gpointer parent_class;

static GtkActionEntry actions_file[] = {
  { "ActionFileNew", N_("_New"), GTK_STOCK_NEW, "<control>n",
	N_("New file"), G_CALLBACK (on_new_file1_activate)},
  { "ActionFileOpen", N_("_Open ..."), GTK_STOCK_OPEN, "<control>o",
	N_("Open file"), G_CALLBACK (on_open1_activate)},
  { "ActionFileSave", N_("_Save"), GTK_STOCK_SAVE, "<control>s",
	N_("Save current file"), G_CALLBACK (on_save1_activate)},
  { "ActionFileSaveAs", N_("Save _As ..."), GTK_STOCK_SAVE_AS, NULL,
	N_("Save the current file with a different name"),
    G_CALLBACK (on_save_as1_activate)},
  { "ActionFileSaveAll", N_("Save A_ll"),  GTK_STOCK_SAVE, NULL,
	N_("Save all currently open files, except new files"),
    G_CALLBACK (on_save_all1_activate)},
  { "ActionFileClose", N_("_Close File"), GTK_STOCK_CLOSE, "<control>w",
	N_("Close current file"),
    G_CALLBACK (on_close_file1_activate)},
  { "ActionFileCloseAll", N_("Close All Files"), GTK_STOCK_CLOSE, "<alt>d",
	N_("Close all files"),
    G_CALLBACK (on_close_all_file1_activate)},
  { "ActionFileReload", N_("Reload F_ile"), GTK_STOCK_REVERT_TO_SAVED, NULL,
	N_("Reload current file"),
    G_CALLBACK (on_reload_file1_activate)},
  { "ActionMenuFileRecentFiles", N_("Recent _Files"),  NULL, NULL, NULL, NULL},
};

static GtkActionEntry actions_print[] = {
  { "ActionPrintFile", N_("_Print"), GTK_STOCK_PRINT, "<alt>p",
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
  { "ActionEditConvertCRLF", N_("Convert EOL chars to CRLF"), NULL, NULL,
	N_("Convert End Of Line characters to DOS EOL (CRLF)"),
    G_CALLBACK (on_editor_command_eol_crlf_activate)},
  { "ActionEditConvertLF", N_("Convert EOL chars to LF"), NULL, NULL,
	N_("Convert End Of Line characters to Unix EOL (LF)"),
    G_CALLBACK (on_editor_command_eol_lf_activate)},
  { "ActionEditConvertCR", N_("Convert EOL chars to CR"), NULL, NULL,
	N_("Convert End Of Line characters to Mac OS EOL (CR)"),
    G_CALLBACK (on_editor_command_eol_cr_activate)},
  { "ActionEditConvertEOL", N_("Convert EOL chars to majority EOL"), NULL, NULL,
	N_("Convert End Of Line characters to majority of the EOL found in the file"),
    G_CALLBACK (on_transform_eolchars1_activate)},
};

static GtkActionEntry actions_select[] = {
  { "ActionMenuEditSelect", N_("_Select"), NULL, NULL, NULL, NULL},
  { "ActionEditSelectAll", N_("Select _All"), NULL, "<control>a",
	N_("Select all text in the editor"),
    G_CALLBACK (on_editor_command_select_all_activate)},
  { "ActionEditSelectToBrace", N_("Select to _Brace"), NULL, "<alt>a",
	N_("Select the text in the matching braces"),
    G_CALLBACK (on_editor_command_select_to_brace_activate)},
  { "ActionEditSelectBlock", N_("Select _Code Block"),
	ANJUTA_STOCK_BLOCK_SELECT, "<alt>b",
	N_("Select the current code block"),
    G_CALLBACK (on_editor_command_select_block_activate)},
};

static GtkActionEntry actions_insert[] = {
  { "ActionMenuEditInsert", N_("_Insert text"), NULL, NULL, NULL, NULL},
  { "ActionMenuEditInsertCtemplate", N_("C _template"), NULL, NULL, NULL, NULL},
  { "ActionMenuEditInsertCvs", N_("_CVS keyword"), NULL, NULL, NULL, NULL},
  { "ActionMenuEditInsertGeneral", N_("_General"), NULL, NULL, NULL, NULL},
  { "ActionEditInsertHeader", N_("_Header"), NULL, NULL,
	N_("Insert a file header"),
    G_CALLBACK (on_insert_header)},
  { "ActionEditInsertCGPL", N_("/_* GPL Notice */"), NULL, NULL,
	N_("Insert GPL notice with C style comments"),
    G_CALLBACK (on_insert_c_gpl_notice)},
  { "ActionEditInsertCPPGPL", N_("/_/ GPL Notice"), NULL, NULL,
	N_("Insert GPL notice with C++ style comments"),
    G_CALLBACK (on_insert_cpp_gpl_notice)},
  { "ActionEditInsertPYGPL", N_("_# GPL Notice"), NULL, NULL,
	N_("Insert GPL notice with Python style comments"),
    G_CALLBACK (on_insert_py_gpl_notice)},
  { "ActionEditInsertUsername", N_("Current _Username"), NULL, NULL,
	N_("Insert name of current user"),
    G_CALLBACK (on_insert_username)},
  { "ActionEditInsertDateTime", N_("Current _Date & Time"), NULL, NULL,
	N_("Insert current date & time"),
    G_CALLBACK (on_insert_date_time)},
  { "ActionEditInsertHeaderTemplate", N_("Header File _Template"), NULL, NULL,
	N_("Insert a standard header file template"),
    G_CALLBACK (on_insert_header_template)},
  { "ActionEditInsertChangelog", N_("ChangeLog entry"), NULL, NULL,
	N_("Insert a ChangeLog entry"),
    G_CALLBACK (on_insert_changelog_entry)},
  { "ActionEditInsertStatementSwitch", N_("_switch"), NULL, NULL,
	N_("Insert a switch template"),
    G_CALLBACK (on_insert_switch_template)},
  { "ActionEditInsertStatementFor", N_("_for"), NULL, NULL,
	N_("Insert a for template"),
    G_CALLBACK (on_insert_for_template)},
  { "ActionEditInsertStatementWhile", N_("_while"), NULL, NULL,
	N_("Insert a while template"),
    G_CALLBACK (on_insert_while_template)},
  { "ActionEditInsertStatementIfElse", N_("_if...else"), NULL, NULL,
	N_("Insert an if...else template"),
    G_CALLBACK (on_insert_ifelse_template)},
  { "ActionEditInsertCVSAuthor", N_("_Author"), NULL, NULL,
	N_("Insert the CVS Author keyword (author of the change)"),
    G_CALLBACK (on_insert_cvs_author)},
  { "ActionEditInsertCVSDate", N_("_Date"), NULL, NULL,
	N_("Insert the CVS Date keyword (date and time of the change)"),
    G_CALLBACK (on_insert_cvs_date)},
  { "ActionEditInsertCVSHeader", N_("_Header"), NULL, NULL,
	N_("Insert the CVS Header keyword (full path revision, date, author, state)"),
    G_CALLBACK (on_insert_cvs_header)},
  { "ActionEditInsertCVSID", N_("_Id"), NULL, NULL,
	N_("Insert the CVS Id keyword (file revision, date, author)"),
    G_CALLBACK (on_insert_cvs_id)},
  { "ActionEditInsertCVSLog", N_("_Log"), NULL, NULL,
	N_("Insert the CVS Log keyword (log message)"),
    G_CALLBACK (on_insert_cvs_log)},
  { "ActionEditInsertCVSName", N_("_Name"), NULL, NULL,
	N_("Insert the CVS Name keyword (name of the sticky tag)"),
    G_CALLBACK (on_insert_cvs_name)},
  { "ActionEditInsertCVSRevision", N_("_Revision"), NULL, NULL,
	N_("Insert the CVS Revision keyword (revision number)"),
    G_CALLBACK (on_insert_cvs_revision)},
  { "ActionEditInsertCVSSource", N_("_Source"), NULL, NULL,
	N_("Insert the CVS Source keyword (full path)"),
    G_CALLBACK (on_insert_cvs_source)},
};

static GtkActionEntry actions_search[] = {
  { "ActionMenuEditSearch", N_("_Search"), NULL, NULL, NULL, NULL},
  { "ActionEditSearchFind", N_("_Find ..."), GTK_STOCK_FIND, "<control>f",
	N_("Search for a string or regular expression in the editor"),
    G_CALLBACK (on_find1_activate)},
  { "ActionEditSearchFindNext", N_("Find _Next"), GTK_STOCK_FIND, "<shift>g",
	N_("Repeat the last Find command"),
    G_CALLBACK (on_findnext1_activate)},
  { "ActionEditSearchFindPrevious", N_("Find _Previous"),
	GTK_STOCK_FIND, "<control><shift>g",
	N_("Repeat the last Find command"),
	G_CALLBACK (on_findprevious1_activate)},
  { "ActionEditSearchReplace", N_("Find and R_eplace ..."),
	GTK_STOCK_FIND_AND_REPLACE, "<shift><control>f",
	N_("Search for and replace a string or regular expression with another string"),
    G_CALLBACK (on_find_and_replace1_activate)},
  { "ActionEditAdvancedSearch", N_("Advanced Search And Replace"),
	GTK_STOCK_FIND, NULL,
 	N_("New advance search And replace stuff"),
	G_CALLBACK (on_find1_activate)},
  { "ActionEditSearchSelectionISearch", N_("_Enter Selection/I-Search"),
	NULL, "<control>e",
	N_("Enter the selected text as the search target"),
    G_CALLBACK (on_enterselection)},
  { "ActionEditSearchInFiles", N_("Fin_d in Files ..."), NULL, NULL,
	N_("Search for a string in multiple files or directories"),
    G_CALLBACK (on_find_in_files1_activate)},
};

static GtkActionEntry actions_comment[] = {
  { "ActionMenuEditComment", N_("Co_mment code"), NULL, NULL, NULL, NULL},
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
  { "ActionEditGotoLineActivate", N_("_Goto Line number"),
	GTK_STOCK_JUMP_TO, NULL,
	N_("Go to a particular line in the editor"),
    G_CALLBACK (on_goto_activate)},
  { "ActionEditGotoLine", N_("_Line number ..."),
	GTK_STOCK_JUMP_TO, "<control><alt>g",
	N_("Go to a particular line in the editor"),
    G_CALLBACK (on_goto_line_no1_activate)},
  { "ActionEditGotoMatchingBrace", N_("Matching _Brace"),
	GTK_STOCK_JUMP_TO, "<control><alt>m", 
	N_("Go to the matching brace in the editor"),
    G_CALLBACK (on_editor_command_match_brace_activate)},
  { "ActionEditGotoBlockStart", N_("_Start of block"),
	ANJUTA_STOCK_BLOCK_START, "<control><alt>s",
	N_("Go to the start of the current block"),
    G_CALLBACK (on_goto_block_start1_activate)},
  { "ActionEditGotoBlockEnd", N_("_End of block"),
	ANJUTA_STOCK_BLOCK_END, "<control><alt>e",
	N_("Go to the end of the current block"),
    G_CALLBACK (on_goto_block_end1_activate)},
  { "ActionEditGotoOccuranceNext", N_("Ne_xt occurrence"),
	GTK_STOCK_JUMP_TO, NULL,
	N_("Find the next occurrence of current word"),
    G_CALLBACK (on_next_occur)},
  { "ActionEditGotoOccurancePrev", N_("Pre_vious occurrence"),
	GTK_STOCK_JUMP_TO, NULL,
	N_("Find the previous occurrence of current word"),
    G_CALLBACK (on_prev_occur)}
};

static GtkActionEntry actions_edit[] = {
  // { "ActionMenuEdit", N_("_Edit"), NULL, NULL, NULL, NULL, NULL },
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
	ANJUTA_STOCK_AUTOCOMPLETE, "<control>enter",
	N_("AutoComplete the current word"),
    G_CALLBACK (on_editor_command_complete_word_activate)},
  { "ActionEditCalltip", N_("S_how calltip"), NULL, NULL,
	N_("Show calltip for the function"),
    G_CALLBACK (on_calltip1_activate)},
};

static GtkToggleActionEntry actions_view[] = {
  { "ActionMenuViewEditor", N_("_Editor"), NULL, NULL, NULL, NULL},
  { "ActionViewEditorLinenumbers", N_("_Line numbers margin"), NULL, NULL,
	N_("Show/Hide line numbers"),
    G_CALLBACK (on_editor_linenos1_activate), FALSE},
  { "ActionViewEditorMarkers", N_("_Markers Margin"), NULL, NULL,
	N_("Show/Hide markers margin"),
    G_CALLBACK (on_editor_markers1_activate), FALSE},
  { "ActionViewEditorFolds", N_("_Code fold margin"), NULL, NULL,
	N_("Show/Hide code fold margin"),
    G_CALLBACK (on_editor_codefold1_activate), FALSE},
  { "ActionViewEditorGuides", N_("_Indentation guides"), NULL, NULL,
	N_("Show/Hide indentation guides"),
    G_CALLBACK (on_editor_indentguides1_activate), FALSE},
  { "ActionViewEditorSpaces", N_("_White spaces"), NULL, NULL,
	N_("Show/Hide white spaces"),
    G_CALLBACK (on_editor_whitespaces1_activate), FALSE},
  { "ActionViewEditorEOL", N_("_Line end characters"), NULL, NULL,
	N_("Show/Hide line end characters"),
    G_CALLBACK (on_editor_eolchars1_activate), FALSE},
  { "ActionViewEditorWrapping", N_("Line _wrapping"), NULL, NULL,
	N_("Enable/disable line wrapping"),
    G_CALLBACK (on_editor_linewrap1_activate), FALSE}
};

static GtkActionEntry actions_zoom[] = {
  { "ActionViewEditorZoomIn", N_("Zoom in"), NULL, NULL,
	N_("Zoom in: Increase font size"),
    G_CALLBACK (on_zoom_in_text_activate)},
  { "ActionViewEditorZoomOut", N_("Zoom out"), NULL, NULL,
	N_("Zoom out: Decrease font size"),
    G_CALLBACK (on_zoom_out_text_activate)}
};

static GtkActionEntry actions_style[] = {
  { "ActionMenuFormatStyle", N_("Force _Highlight Style"), NULL, NULL, NULL, NULL},
  { "ActionFormatHighlightStyleAutomatic",
	N_("Automatic"), GTK_STOCK_EXECUTE, NULL,
	N_("Automatically determine the highlight style"),
    G_CALLBACK (on_force_hilite1_auto_activate)},
  { "ActionFormatHighlightStyleNone",N_("No Highlight style"), NULL, NULL,
	N_("Remove the current highlight style"),
    G_CALLBACK (on_force_hilite1_none_activate)},
  { "ActionFormatHighlightStyleCpp",N_("C and C++"), NULL, NULL,
	N_("Force the highlight style to C and C++"),
    G_CALLBACK (on_force_hilite1_cpp_activate)},
  { "ActionFormatHighlightStyleHtml", N_("HTML"), NULL, NULL,
	N_("Force the highlight style to HTML"),
    G_CALLBACK (on_force_hilite1_html_activate)},
  { "ActionFormatHighlightStyleXml", N_("XML"), NULL, NULL,
	N_("Force the highlight style to XML"),
    G_CALLBACK (on_force_hilite1_xml_activate)},
  { "ActionFormatHighlightStyleJs", N_("Javascript"), NULL, NULL,
	N_("Force the highlight style to Javascript"),
    G_CALLBACK (on_force_hilite1_js_activate)},
  { "ActionFormatHighlightStyleWScript", N_("WScript"), NULL, NULL,
	N_("Force the highlight style to WScript"),
    G_CALLBACK (on_force_hilite1_wscript_activate)},
  { "ActionFormatHighlightStyleMakefile", N_("Makefile"), NULL, NULL,
	N_("Force the highlight style to Makefile"),
    G_CALLBACK (on_force_hilite1_make_activate)},
  { "ActionFormatHighlightStyleJava", N_("Java"), NULL, NULL,
	N_("Force the highlight type to Java"),
    G_CALLBACK (on_force_hilite1_java_activate)},
  { "ActionFormatHighlightStyleLua", N_("LUA"), NULL, NULL,
	N_("Force the highlight style to LUA"),
    G_CALLBACK (on_force_hilite1_lua_activate)},
  { "ActionFormatHighlightStylePython", N_("Python"), NULL, NULL,
	N_("Force the highlight style to Python"),
    G_CALLBACK (on_force_hilite1_python_activate)},
  { "ActionFormatHighlightStylePerl", N_("Perl"), NULL, NULL,
	N_("Force the highlight style to Perl"),
    G_CALLBACK (on_force_hilite1_perl_activate)},
  { "ActionFormatHighlightStyleSQL", N_("SQL"), NULL, NULL,
	N_("Force the highlight style to SQL"),
    G_CALLBACK (on_force_hilite1_sql_activate)},
  { "ActionFormatHighlightStylePLSQL", N_("PL/SQL"), NULL, NULL,
	N_("Force the highlight style to PL/SQL"),
    G_CALLBACK (on_force_hilite1_plsql_activate)},
  { "ActionFormatHighlightStylePHP", N_("PHP"), NULL, NULL,
	N_("Force the highlight style to PHP"),
    G_CALLBACK (on_force_hilite1_php_activate)},
  { "ActionFormatHighlightStyleLatex", N_("LaTex"), NULL, NULL,
	N_("Force the highlight style to LaTex"),
    G_CALLBACK (on_force_hilite1_latex_activate)},
  { "ActionFormatHighlightStyleDiff", N_("Diff"), NULL, NULL,
	N_("Force the highlight style to Diff"),
    G_CALLBACK (on_force_hilite1_diff_activate)},
  { "ActionFormatHighlightStylePascal", N_("Pascal"), NULL, NULL,
	N_("Force the highlight style to Pascal"),
    G_CALLBACK (on_force_hilite1_pascal_activate)},
  { "ActionFormatHighlightStyleXcode", N_("Xcode"), NULL, NULL,
	N_("Force the highlight style to Xcode"),
    G_CALLBACK (on_force_hilite1_xcode_activate)},
  { "ActionFormatHighlightStyleProps", N_("Prj/Properties"), NULL, NULL,
	N_("Force the highlight style to project/properties files"),
    G_CALLBACK (on_force_hilite1_props_activate)},
  { "ActionFormatHighlightStyleConf", N_("Conf"), NULL, NULL,
	N_("Force the highlight style to UNIX conf files"),
    G_CALLBACK (on_force_hilite1_conf_activate)},
  { "ActionFormatHighlightStyleAda", N_("Ada"), NULL, NULL,
	N_("Force the highlight style to Ada"),
    G_CALLBACK (on_force_hilite1_ada_activate)},
  { "ActionFormatHighlightStyleBaan", N_("Baan"), NULL, NULL,
	N_("Force the highlight style to Baan"),
    G_CALLBACK (on_force_hilite1_baan_activate)},
  { "ActionFormatHighlightStyleLisp", N_("Lisp"), NULL, NULL,
	N_("Force the highlight style to Lisp"),
    G_CALLBACK (on_force_hilite1_lisp_activate)},
  { "ActionFormatHighlightStyleRuby", N_("Ruby"), NULL, NULL,
	N_("Force the highlight style to Ruby"),
    G_CALLBACK (on_force_hilite1_ruby_activate)},
  { "ActionFormatHighlightStyleMatlab", N_("Matlab"), NULL, NULL,
	N_("Force the highlight style to Matlab"),
    G_CALLBACK (on_force_hilite1_matlab_activate)},
};

static GtkActionEntry actions_format[] = {
  { "ActionMenuFormat", N_("For_mat"), NULL, NULL, NULL, NULL},
  { "ActionFormatAutoformat", N_("Auto _Format"),
	ANJUTA_STOCK_INDENT_AUTO, NULL,
	N_("Autoformat the current source file"),
    G_CALLBACK (on_indent1_activate)},
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
  { "ActionFormatEditorDetach", N_("D_etach Current Document"),
	ANJUTA_STOCK_UNDOCK, NULL,
	N_("Detach the current document into a separate editor window"),
    G_CALLBACK (on_detach1_activate)}
};

static GtkActionEntry actions_bookmark[] = {
  { "ActionMenuBookmark", N_("Bookmar_k"), NULL, NULL, NULL, NULL},
  { "ActionBookmarkToggle", N_("_Toggle bookmark"),
	ANJUTA_STOCK_TOGGLE_BOOKMARK, "<control>k",
	N_("Toggle a bookmark at the current line position"),
    G_CALLBACK (on_editor_command_bookmark_toggle_activate)},
  { "ActionBookmarkFirst", N_("_First bookmark"),
	GTK_STOCK_GOTO_FIRST, NULL,
	N_("Jump to the first bookmark in the file"),
    G_CALLBACK (on_editor_command_bookmark_first_activate)},
  { "ActionBookmarkPrevious", N_("_Previous bookmark"),
	GTK_STOCK_GO_BACK, NULL,
	N_("Jump to the previous bookmark in the file"),
    G_CALLBACK (on_editor_command_bookmark_prev_activate)},
  { "ActionBookmarkNext", N_("_Next bookmark"),
	GTK_STOCK_GO_FORWARD, NULL,
	N_("Jump to the next bookmark in the file"),
    G_CALLBACK (on_editor_command_bookmark_next_activate)},
  { "ActionBookmarkLast", N_("_Last bookmark"),
	GTK_STOCK_GOTO_LAST, NULL,
	N_("Jump to the last bookmark in the file"),
    G_CALLBACK (on_editor_command_bookmark_last_activate)},
  { "ActionBookmarkClear", N_("_Clear all bookmarks"),
	GTK_STOCK_CLOSE, NULL,
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
	{ actions_insert, G_N_ELEMENTS (actions_insert), "ActionGroupEditorInsert", N_("Editor text insertions") },
	{ actions_search, G_N_ELEMENTS (actions_search), "ActionGroupEditorSearch", N_("Editor text searching") },
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
			g_warning ("Name is null: %s", actions[i].label);
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
			g_warning ("Name is null: %s", actions[i].label);
	}
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	GtkWidget *docman;
	AnjutaUI *ui;
	EditorPlugin *editor_plugin;
	GtkActionGroup *group;
	GtkAction *action;
	gint i;
	
	g_message ("EditorPlugin: Activating Editor plugin ...");
	editor_plugin = (EditorPlugin*) plugin;
	editor_plugin->ui = anjuta_shell_get_ui (plugin->shell, NULL);
	editor_plugin->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	
	ui = editor_plugin->ui;
	docman = anjuta_docman_new (editor_plugin->prefs);
	editor_plugin->docman = docman;
	
	/* Add all our editor actions */
	for (i = 0; i < G_N_ELEMENTS (action_groups); i++)
	{
		GList *actions, *act;
		swap_label_and_stock (action_groups[i].group, action_groups[i].size);
		group = anjuta_ui_add_action_group_entries (ui, 
													action_groups[i].name,
													_(action_groups[i].label),
													action_groups[i].group,
													action_groups[i].size,
													plugin);
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
		swap_toggle_label_and_stock (action_toggle_groups[i].group,
									 action_toggle_groups[i].size);
		group = anjuta_ui_add_toggle_action_group_entries (ui, 
												action_toggle_groups[i].name,
												_(action_toggle_groups[i].label),
												action_toggle_groups[i].group,
												action_toggle_groups[i].size,
												plugin);
		actions = gtk_action_group_list_actions (group);
		act = actions;
		while (act) {
			g_object_set_data (G_OBJECT (act->data), "Plugin", editor_plugin);
			act = g_list_next (act);
		}
	}
	group = gtk_action_group_new ("ActionGroupNavigation");
	
	action = g_object_new (EGG_TYPE_ENTRY_ACTION,
						   "name", "ActionEditGotoLineEntry",
						   "label", _("Goto line"),
						   "tooltip", _("Enter the line number to jump and press enter"),
						   "stock_id", GTK_STOCK_JUMP_TO,
						   "width", 50,
							NULL);
	g_signal_connect (action, "activate",
					  G_CALLBACK (on_toolbar_goto_clicked), ui);
	gtk_action_group_add_action (group, action);
	
	action = g_object_new (EGG_TYPE_ENTRY_ACTION,
						   "name", "ActionEditSearchEntry",
						   "label", _("Search"),
						   "tooltip", _("Incremental search"),
						   "stock_id", GTK_STOCK_JUMP_TO,
						   "width", 150,
							NULL);
	g_signal_connect (action, "activate",
					  G_CALLBACK (on_toolbar_find_clicked), plugin);
	
	g_signal_connect (action, "changed",
					  G_CALLBACK (on_toolbar_find_incremental), plugin);
	g_signal_connect (action, "focus-in",
					  G_CALLBACK (on_toolbar_find_incremental_start), plugin);
	g_signal_connect (action, "focus_out",
					  G_CALLBACK (on_toolbar_find_incremental_end), plugin);
	gtk_action_group_add_action (group, action);
	
	anjuta_ui_add_action_group (ui, "ActionGroupNavigation",
								N_("Editor quick navigations"), group);
	
	editor_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);
	anjuta_shell_add_widget (plugin->shell, docman,
							 "AnjutaDocumentManager", _("Documents"),
							 ANJUTA_SHELL_PLACEMENT_CENTER, NULL);
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	EditorPlugin *eplugin;
	AnjutaUI *ui;
	
	eplugin = (EditorPlugin*)plugin;
	
	g_message ("EditorPlugin: Dectivating Editor plugin ...");
	anjuta_shell_remove_widget (plugin->shell, eplugin->docman, NULL);
	anjuta_ui_unmerge (ui, eplugin->uiid);
	return TRUE;
}

static void
dispose (GObject *obj)
{
	// EditorPlugin *plugin = (EditorPlugin*)obj;
}

static void
editor_plugin_instance_init (GObject *obj)
{
	EditorPlugin *plugin = (EditorPlugin*)obj;
	plugin->uiid = 0;
}

static void
editor_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
}

/* Implement interfaces */
static IAnjutaEditor*
ianjuta_docman_get_current_editor (IAnjutaDocumentManager *plugin, GError **e)
{
	TextEditor *te;
	AnjutaDocman *docman = ANJUTA_DOCMAN ((((EditorPlugin*)plugin)->docman));
	te = anjuta_docman_get_current_editor (ANJUTA_DOCMAN (docman));
	return IANJUTA_EDITOR (te);
}

/* Implement interfaces */
static void
ianjuta_docman_set_current_editor (IAnjutaDocumentManager *plugin,
								   IAnjutaEditor *editor, GError **e)
{

	AnjutaDocman *docman = ANJUTA_DOCMAN ((((EditorPlugin*)plugin)->docman));
	anjuta_docman_set_current_editor (ANJUTA_DOCMAN (docman),
									  TEXT_EDITOR (editor));
}

static GList*
ianjuta_docman_get_editors (IAnjutaDocumentManager *plugin, GError **e)
{
	AnjutaDocman *docman;
	GList * editors = NULL;
	docman = ANJUTA_DOCMAN ((((EditorPlugin*)plugin)->docman));
	editors = anjuta_docman_get_all_editors (docman);
	return editors;
}

static void
ianjuta_document_manager_iface_init (IAnjutaDocumentManagerIface *iface)
{
	iface->get_current_editor = ianjuta_docman_get_current_editor;
	iface->set_current_editor = ianjuta_docman_set_current_editor;
	iface->get_editors = ianjuta_docman_get_editors;
}

ANJUTA_PLUGIN_BEGIN (EditorPlugin, editor_plugin);
ANJUTA_INTERFACE(ianjuta_document_manager, IANJUTA_TYPE_DOCUMENT_MANAGER);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (EditorPlugin, editor_plugin);
