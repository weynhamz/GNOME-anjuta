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
#include <libegg/toolbar/eggtoolbar.h>

#include "print.h"
#include "anjuta-docman.h"
#include "action-callbacks.h"
#include "aneditor.h"
#include "lexer.h"
#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-document-manager.ui"

gpointer parent_class;

static EggActionGroupEntry actions_file[] = {
  { "ActionFileNew", N_("_New"), GTK_STOCK_NEW, "<control>n",
	N_("New file"),
    G_CALLBACK (on_new_file1_activate), NULL },
  { "ActionFileOpen", N_("_Open ..."), GTK_STOCK_OPEN, "<control>o",
	N_("Open file"),
    G_CALLBACK (on_open1_activate), NULL },
  { "ActionFileSave", N_("_Save"), GTK_STOCK_SAVE, "<control>s",
	N_("Save current file"),
    G_CALLBACK (on_save1_activate), NULL },
  { "ActionFileSaveAs", N_("Save _As ..."), GTK_STOCK_SAVE_AS, NULL,
	N_("Save the current file with a different name"),
    G_CALLBACK (on_save_as1_activate), NULL },
  { "ActionFileSaveAll", N_("Save A_ll"),  GTK_STOCK_SAVE, NULL,
	N_("Save all currently open files, except new files"),
    G_CALLBACK (on_save_all1_activate), NULL },
  { "ActionFileClose", N_("_Close File"), GTK_STOCK_CLOSE, "<control>w",
	N_("Close current file"),
    G_CALLBACK (on_close_file1_activate), NULL },
  { "ActionFileCloseAll", N_("Close All Files"), GTK_STOCK_CLOSE, "<alt>d",
	N_("Close all files"),
    G_CALLBACK (on_close_all_file1_activate), NULL },
  { "ActionFileReload", N_("Reload F_ile"), GTK_STOCK_REVERT_TO_SAVED, NULL,
	N_("Reload current file"),
    G_CALLBACK (on_reload_file1_activate), NULL },
  { "ActionMenuFileRecentFiles", N_("Recent _Files"),  NULL, NULL, NULL, NULL, NULL },
};

static EggActionGroupEntry actions_print[] = {
  { "ActionPrintFile", N_("_Print"), GTK_STOCK_PRINT, "<alt>p",
	N_("Print the current file"),
    G_CALLBACK (anjuta_print_cb), NULL },
  { "ActionPrintPreview", N_("_Print Preview"), NULL, NULL,
	N_("Print preview of the current file"),
    G_CALLBACK (anjuta_print_preview_cb), NULL },
};

static EggActionGroupEntry actions_transform[] = {
  { "ActionMenuEditTransform", N_("_Transform"), NULL, NULL, NULL, NULL, NULL },
  { "ActionEditMakeSelectionUppercase", N_("_Make Selection Uppercase"), NULL, NULL,
	N_("Make the selected text uppercase"),
    G_CALLBACK (on_editor_command_activate),  (gpointer) ANE_UPRCASE },
  { "ActionEditMakeSelectionLowercase", N_("Make Selection Lowercase"), NULL, NULL,
	N_("Make the selected text lowercase"),
    G_CALLBACK (on_editor_command_activate),  (gpointer) ANE_LWRCASE },
  { "ActionEditConvertCRLF", N_("Convert EOL chars to CRLF"), NULL, NULL,
	N_("Convert End Of Line characters to DOS EOL (CRLF)"),
    G_CALLBACK (on_editor_command_activate),  (gpointer) ANE_EOL_CRLF },
  { "ActionEditConvertLF", N_("Convert EOL chars to LF"), NULL, NULL,
	N_("Convert End Of Line characters to Unix EOL (LF)"),
    G_CALLBACK (on_editor_command_activate),  (gpointer) ANE_EOL_LF },
  { "ActionEditConvertCR", N_("Convert EOL chars to CR"), NULL, NULL,
	N_("Convert End Of Line characters to Mac OS EOL (CR)"),
    G_CALLBACK (on_editor_command_activate),  (gpointer) ANE_EOL_CR },
  { "ActionEditConvertEOL", N_("Convert EOL chars to majority EOL"), NULL, NULL,
	N_("Convert End Of Line characters to majority of the EOL found in the file"),
    G_CALLBACK (on_transform_eolchars1_activate), NULL },
};

static EggActionGroupEntry actions_select[] = {
  { "ActionMenuEditSelect", N_("_Select"), NULL, NULL, NULL, NULL, NULL },
  { "ActionEditSelectAll", N_("Select _All"), NULL, "<control>a",
	N_("Select all text in the editor"),
    G_CALLBACK (on_editor_command_activate), (gpointer) ANE_SELECTALL },
  { "ActionEditSelectToBrace", N_("Select to _Brace"), NULL, "<alt>a",
	N_("Select the text in the matching braces"),
    G_CALLBACK (on_editor_command_activate), (gpointer) ANE_SELECTTOBRACE },
  { "ActionEditSelectBlock", N_("Select _Code Block"),
	ANJUTA_STOCK_BLOCK_SELECT, "<alt>b",
	N_("Select the current code block"),
    G_CALLBACK (on_editor_command_activate), (gpointer) ANE_SELECTBLOCK },
};

static EggActionGroupEntry actions_insert[] = {
  { "ActionMenuEditInsert", N_("_Insert text"), NULL, NULL, NULL, NULL, NULL },
  { "ActionMenuEditInsertCtemplate", N_("C _template"), NULL, NULL, NULL, NULL, NULL },
  { "ActionMenuEditInsertCvs", N_("_CVS keyword"), NULL, NULL, NULL, NULL, NULL },
  { "ActionMenuEditInsertGeneral", N_("_General"), NULL, NULL, NULL, NULL, NULL },
  { "ActionEditInsertHeader", N_("_Header"), NULL, NULL,
	N_("Insert a file header"),
    G_CALLBACK (on_insert_header), NULL },
  { "ActionEditInsertCGPL", N_("/_* GPL Notice */"), NULL, NULL,
	N_("Insert GPL notice with C style comments"),
    G_CALLBACK (on_insert_c_gpl_notice), NULL },
  { "ActionEditInsertCPPGPL", N_("/_/ GPL Notice"), NULL, NULL,
	N_("Insert GPL notice with C++ style comments"),
    G_CALLBACK (on_insert_cpp_gpl_notice), NULL },
  { "ActionEditInsertPYGPL", N_("_# GPL Notice"), NULL, NULL,
	N_("Insert GPL notice with Python style comments"),
    G_CALLBACK (on_insert_py_gpl_notice), NULL },
  { "ActionEditInsertUsername", N_("Current _Username"), NULL, NULL,
	N_("Insert name of current user"),
    G_CALLBACK (on_insert_username), NULL },
  { "ActionEditInsertDateTime", N_("Current _Date & Time"), NULL, NULL,
	N_("Insert current date & time"),
    G_CALLBACK (on_insert_date_time), NULL },
  { "ActionEditInsertHeaderTemplate", N_("Header File _Template"), NULL, NULL,
	N_("Insert a standard header file template"),
    G_CALLBACK (on_insert_header_template), NULL },
  { "ActionEditInsertChangelog", N_("ChangeLog entry"), NULL, NULL,
	N_("Insert a ChangeLog entry"),
    G_CALLBACK (on_insert_changelog_entry), NULL },
  { "ActionEditInsertStatementSwitch", N_("_switch"), NULL, NULL,
	N_("Insert a switch template"),
    G_CALLBACK (on_insert_switch_template), NULL },
  { "ActionEditInsertStatementFor", N_("_for"), NULL, NULL,
	N_("Insert a for template"),
    G_CALLBACK (on_insert_for_template), NULL },
  { "ActionEditInsertStatementWhile", N_("_while"), NULL, NULL,
	N_("Insert a while template"),
    G_CALLBACK (on_insert_while_template), NULL },
  { "ActionEditInsertStatementIfElse", N_("_if...else"), NULL, NULL,
	N_("Insert an if...else template"),
    G_CALLBACK (on_insert_ifelse_template), NULL },
  { "ActionEditInsertCVSAuthor", N_("_Author"), NULL, NULL,
	N_("Insert the CVS Author keyword (author of the change)"),
    G_CALLBACK (on_insert_cvs_author), NULL },
  { "ActionEditInsertCVSDate", N_("_Date"), NULL, NULL,
	N_("Insert the CVS Date keyword (date and time of the change)"),
    G_CALLBACK (on_insert_cvs_date), NULL },
  { "ActionEditInsertCVSHeader", N_("_Header"), NULL, NULL,
	N_("Insert the CVS Header keyword (full path revision, date, author, state)"),
    G_CALLBACK (on_insert_cvs_header), NULL },
  { "ActionEditInsertCVSID", N_("_Id"), NULL, NULL,
	N_("Insert the CVS Id keyword (file revision, date, author)"),
    G_CALLBACK (on_insert_cvs_id), NULL },
  { "ActionEditInsertCVSLog", N_("_Log"), NULL, NULL,
	N_("Insert the CVS Log keyword (log message)"),
    G_CALLBACK (on_insert_cvs_log), NULL },
  { "ActionEditInsertCVSName", N_("_Name"), NULL, NULL,
	N_("Insert the CVS Name keyword (name of the sticky tag)"),
    G_CALLBACK (on_insert_cvs_name), NULL },
  { "ActionEditInsertCVSRevision", N_("_Revision"), NULL, NULL,
	N_("Insert the CVS Revision keyword (revision number)"),
    G_CALLBACK (on_insert_cvs_revision), NULL },
  { "ActionEditInsertCVSSource", N_("_Source"), NULL, NULL,
	N_("Insert the CVS Source keyword (full path)"),
    G_CALLBACK (on_insert_cvs_source), NULL },
};

static EggActionGroupEntry actions_search[] = {
  { "ActionMenuEditSearch", N_("_Search"), NULL, NULL, NULL, NULL, NULL },
  { "ActionEditSearchFind", N_("_Find ..."), GTK_STOCK_FIND, "<control>f",
	N_("Search for a string or regular expression in the editor"),
    G_CALLBACK (on_find1_activate), NULL },
  { "ActionEditSearchFindNext", N_("Find _Next"), GTK_STOCK_FIND, "<shift>g",
	N_("Repeat the last Find command"),
    G_CALLBACK (on_findnext1_activate), NULL },
  { "ActionEditSearchFindPrevious", N_("Find _Previous"),
	GTK_STOCK_FIND, "<control><shift>g",
	N_("Repeat the last Find command"),
	G_CALLBACK (on_findprevious1_activate), NULL },
  { "ActionEditSearchReplace", N_("Find and R_eplace ..."),
	GTK_STOCK_FIND_AND_REPLACE, "<shift><control>f",
	N_("Search for and replace a string or regular expression with another string"),
    G_CALLBACK (on_find_and_replace1_activate), NULL },
  { "ActionEditAdvancedSearch", N_("Advanced Search And Replace"),
	GTK_STOCK_FIND, NULL,
 	N_("New advance search And replace stuff"),
	G_CALLBACK (on_find1_activate), NULL },
  { "ActionEditSearchSelectionISearch", N_("_Enter Selection/I-Search"),
	NULL, "<control>e",
	N_("Enter the selected text as the search target"),
    G_CALLBACK (on_enterselection), NULL },
  { "ActionEditSearchInFiles", N_("Fin_d in Files ..."), NULL, NULL,
	N_("Search for a string in multiple files or directories"),
    G_CALLBACK (on_find_in_files1_activate), NULL },
};

static EggActionGroupEntry actions_comment[] = {
  { "ActionMenuEditComment", N_("Co_mment code"), NULL, NULL, NULL, NULL, NULL },
  { "ActionEditCommentBlock", N_("_Block Comment/Uncomment"), NULL, NULL,
	N_("Block comment the selected text"),
    G_CALLBACK (on_comment_block), NULL },
  { "ActionEditCommentBox", N_("Bo_x Comment/Uncomment"), NULL, NULL,
	N_("Box comment the selected text"),
    G_CALLBACK (on_comment_box), NULL },
  { "ActionEditCommentStream", N_("_Stream Comment/Uncomment"), NULL, NULL,
	N_("Stream comment the selected text"),
    G_CALLBACK (on_comment_block), NULL },
};

static EggActionGroupEntry actions_navigation[] = {
  // { "ActionMenuEditGoto", N_("G_oto"), NULL, NULL, NULL, NULL, NULL },
  { "ActionEditGotoLineActivate", N_("_Goto Line number"),
	GTK_STOCK_JUMP_TO, NULL,
	N_("Go to a particular line in the editor"),
    G_CALLBACK (on_goto_activate), NULL },
  { "ActionEditGotoLine", N_("_Line number ..."),
	GTK_STOCK_JUMP_TO, "<control><alt>g",
	N_("Go to a particular line in the editor"),
    G_CALLBACK (on_goto_line_no1_activate), NULL },
  { "ActionEditGotoMatchingBrace", N_("Matching _Brace"),
	GTK_STOCK_JUMP_TO, "<control><alt>m", 
	N_("Go to the matching brace in the editor"),
    G_CALLBACK (on_editor_command_activate), (gpointer) ANE_MATCHBRACE },
  { "ActionEditGotoBlockStart", N_("_Start of block"),
	ANJUTA_STOCK_BLOCK_START, "<control><alt>s",
	N_("Go to the start of the current block"),
    G_CALLBACK (on_goto_block_start1_activate), NULL },
  { "ActionEditGotoBlockEnd", N_("_End of block"),
	ANJUTA_STOCK_BLOCK_END, "<control><alt>e",
	N_("Go to the end of the current block"),
    G_CALLBACK (on_goto_block_end1_activate), NULL },
  { "ActionEditGotoOccuranceNext", N_("Ne_xt occurrence"),
	GTK_STOCK_JUMP_TO, NULL,
	N_("Find the next occurrence of current word"),
    G_CALLBACK (on_next_occur), NULL },
  { "ActionEditGotoOccurancePrev", N_("Pre_vious occurrence"),
	GTK_STOCK_JUMP_TO, NULL,
	N_("Find the previous occurrence of current word"),
    G_CALLBACK (on_prev_occur), NULL },
};

static EggActionGroupEntry actions_edit[] = {
  // { "ActionMenuEdit", N_("_Edit"), NULL, NULL, NULL, NULL, NULL },
  { "ActionEditUndo", N_("U_ndo"), GTK_STOCK_UNDO, "<control>z",
	N_("Undo the last action"),
    G_CALLBACK (on_editor_command_activate), (gpointer) ANE_UNDO },
  { "ActionEditRedo", N_("_Redo"), GTK_STOCK_REDO, "<control>r",
	N_("Redo the last undone action"),
    G_CALLBACK (on_editor_command_activate), (gpointer) ANE_REDO },
  { "ActionEditCut", N_("C_ut"), GTK_STOCK_CUT, "<control>x",
	N_("Cut the selected text from the editor to the clipboard"),
    G_CALLBACK (on_editor_command_activate), (gpointer) ANE_CUT },
  { "ActionEditCopy", N_("_Copy"), GTK_STOCK_COPY, "<control>c",
	N_("Copy the selected text to the clipboard"),
    G_CALLBACK (on_editor_command_activate), (gpointer) ANE_COPY },
  { "ActionEditPaste", N_("_Paste"), GTK_STOCK_PASTE, "<control>v",
	N_("Paste the content of clipboard at the current position"),
    G_CALLBACK (on_editor_command_activate), (gpointer) ANE_PASTE },
  { "ActionEditClear", N_("_Clear"), NULL, "Delete",
	N_("Delete the selected text from the editor"),
    G_CALLBACK (on_editor_command_activate), (gpointer) ANE_CLEAR },
  { "ActionEditAutocomplete", N_("_AutoComplete"),
	ANJUTA_STOCK_AUTOCOMPLETE, "<control>enter",
	N_("AutoComplete the current word"),
    G_CALLBACK (on_editor_command_activate), (gpointer) ANE_COMPLETEWORD },
  { "ActionEditCalltip", N_("S_how calltip"), NULL, NULL,
	N_("Show calltip for the function"),
    G_CALLBACK (on_calltip1_activate), NULL },
};

static EggActionGroupEntry actions_view[] = {
  { "ActionMenuViewEditor", N_("_Editor"), NULL, NULL, NULL, NULL, NULL },
  { "ActionViewEditorLinenumbers", N_("_Line numbers margin"), NULL, NULL,
	N_("Show/Hide line numbers"),
    G_CALLBACK (on_editor_linenos1_activate), NULL, TOGGLE_ACTION},
  { "ActionViewEditorMarkers", N_("_Markers Margin"), NULL, NULL,
	N_("Show/Hide markers margin"),
    G_CALLBACK (on_editor_markers1_activate), NULL, TOGGLE_ACTION},
  { "ActionViewEditorFolds", N_("_Code fold margin"), NULL, NULL,
	N_("Show/Hide code fold margin"),
    G_CALLBACK (on_editor_codefold1_activate), NULL, TOGGLE_ACTION},
  { "ActionViewEditorGuides", N_("_Indentation guides"), NULL, NULL,
	N_("Show/Hide indentation guides"),
    G_CALLBACK (on_editor_indentguides1_activate), NULL, TOGGLE_ACTION},
  { "ActionViewEditorSpaces", N_("_White spaces"), NULL, NULL,
	N_("Show/Hide white spaces"),
    G_CALLBACK (on_editor_whitespaces1_activate), NULL, TOGGLE_ACTION},
  { "ActionViewEditorEOL", N_("_Line end characters"), NULL, NULL,
	N_("Show/Hide line end characters"),
    G_CALLBACK (on_editor_eolchars1_activate), NULL, TOGGLE_ACTION},
  { "ActionViewEditorWrapping", N_("Line _wrapping"), NULL, NULL,
	N_("Enable/disable line wrapping"),
    G_CALLBACK (on_editor_linewrap1_activate), NULL, TOGGLE_ACTION},
  { "ActionViewEditorZoomIn", N_("Zoom in"), NULL, NULL,
	N_("Zoom in: Increase font size"),
    G_CALLBACK (on_zoom_text_activate), (gpointer) "++"},
  { "ActionViewEditorZoomOut", N_("Zoom out"), NULL, NULL,
	N_("Zoom out: Decrease font size"),
    G_CALLBACK (on_zoom_text_activate), (gpointer) "--"},
};

static EggActionGroupEntry actions_style[] = {
  { "ActionMenuFormatStyle", N_("Force _Highlight Style"), NULL, NULL, NULL, NULL, NULL },
  { "ActionFormatHighlightStyleAutomatic",
	N_("Automatic"), GTK_STOCK_EXECUTE, NULL,
	N_("Automatically determine the highlight style"),
    G_CALLBACK (on_force_hilite1_activate), GUINT_TO_POINTER (TE_LEXER_AUTOMATIC)},
  { "ActionFormatHighlightStyleNone",N_("No Highlight style"), NULL, NULL,
	N_("Remove the current highlight style"),
    G_CALLBACK (on_force_hilite1_activate), GUINT_TO_POINTER (TE_LEXER_NONE)},
  { "ActionFormatHighlightStyleCpp",N_("C and C++"), NULL, NULL,
	N_("Force the highlight style to C and C++"),
    G_CALLBACK (on_force_hilite1_activate), GUINT_TO_POINTER (TE_LEXER_CPP)},
  { "ActionFormatHighlightStyleHtml", N_("HTML"), NULL, NULL,
	N_("Force the highlight style to HTML"),
    G_CALLBACK (on_force_hilite1_activate), GUINT_TO_POINTER (TE_LEXER_HTML)},
  { "ActionFormatHighlightStyleXml", N_("XML"), NULL, NULL,
	N_("Force the highlight style to XML"),
    G_CALLBACK (on_force_hilite1_activate), GUINT_TO_POINTER (TE_LEXER_XML)},
  { "ActionFormatHighlightStyleJs", N_("Javascript"), NULL, NULL,
	N_("Force the highlight style to Javascript"),
    G_CALLBACK (on_force_hilite1_activate), GUINT_TO_POINTER (TE_LEXER_JS)},
  { "ActionFormatHighlightStyleWScript", N_("WScript"), NULL, NULL,
	N_("Force the highlight style to WScript"),
    G_CALLBACK (on_force_hilite1_activate), GUINT_TO_POINTER (TE_LEXER_WSCRIPT)},
  { "ActionFormatHighlightStyleMakefile", N_("Makefile"), NULL, NULL,
	N_("Force the highlight style to Makefile"),
    G_CALLBACK (on_force_hilite1_activate), GUINT_TO_POINTER (TE_LEXER_MAKE)},
  { "ActionFormatHighlightStyleJava", N_("Java"), NULL, NULL,
	N_("Force the highlight type to Java"),
    G_CALLBACK (on_force_hilite1_activate), GUINT_TO_POINTER (TE_LEXER_JAVA)},
  { "ActionFormatHighlightStyleLua", N_("LUA"), NULL, NULL,
	N_("Force the highlight style to LUA"),
    G_CALLBACK (on_force_hilite1_activate), GUINT_TO_POINTER (TE_LEXER_LUA)},
  { "ActionFormatHighlightStylePython", N_("Python"), NULL, NULL,
	N_("Force the highlight style to Python"),
    G_CALLBACK (on_force_hilite1_activate), GUINT_TO_POINTER (TE_LEXER_PYTHON)},
  { "ActionFormatHighlightStylePerl", N_("Perl"), NULL, NULL,
	N_("Force the highlight style to Perl"),
    G_CALLBACK (on_force_hilite1_activate), GUINT_TO_POINTER (TE_LEXER_PERL)},
  { "ActionFormatHighlightStyleSQL", N_("SQL"), NULL, NULL,
	N_("Force the highlight style to SQL"),
    G_CALLBACK (on_force_hilite1_activate), GUINT_TO_POINTER (TE_LEXER_SQL)},
  { "ActionFormatHighlightStylePLSQL", N_("PL/SQL"), NULL, NULL,
	N_("Force the highlight style to PL/SQL"),
    G_CALLBACK (on_force_hilite1_activate), GUINT_TO_POINTER (TE_LEXER_PLSQL)},
  { "ActionFormatHighlightStylePHP", N_("PHP"), NULL, NULL,
	N_("Force the highlight style to PHP"),
    G_CALLBACK (on_force_hilite1_activate), GUINT_TO_POINTER (TE_LEXER_PHP)},
  { "ActionFormatHighlightStyleLatex", N_("LaTex"), NULL, NULL,
	N_("Force the highlight style to LaTex"),
    G_CALLBACK (on_force_hilite1_activate), GUINT_TO_POINTER (TE_LEXER_LATEX)},
  { "ActionFormatHighlightStyleDiff", N_("Diff"), NULL, NULL,
	N_("Force the highlight style to Diff"),
    G_CALLBACK (on_force_hilite1_activate), GUINT_TO_POINTER (TE_LEXER_DIFF)},
  { "ActionFormatHighlightStylePascal", N_("Pascal"), NULL, NULL,
	N_("Force the highlight style to Pascal"),
    G_CALLBACK (on_force_hilite1_activate), GUINT_TO_POINTER (TE_LEXER_PASCAL)},
  { "ActionFormatHighlightStyleXcode", N_("Xcode"), NULL, NULL,
	N_("Force the highlight style to Xcode"),
    G_CALLBACK (on_force_hilite1_activate), GUINT_TO_POINTER (TE_LEXER_XCODE)},
  { "ActionFormatHighlightStyleProps", N_("Prj/Properties"), NULL, NULL,
	N_("Force the highlight style to project/properties files"),
    G_CALLBACK (on_force_hilite1_activate), GUINT_TO_POINTER (TE_LEXER_PROPS)},
  { "ActionFormatHighlightStyleConf", N_("Conf"), NULL, NULL,
	N_("Force the highlight style to UNIX conf files"),
    G_CALLBACK (on_force_hilite1_activate), GUINT_TO_POINTER (TE_LEXER_CONF)},
  { "ActionFormatHighlightStyleAda", N_("Ada"), NULL, NULL,
	N_("Force the highlight style to Ada"),
    G_CALLBACK (on_force_hilite1_activate), GUINT_TO_POINTER (TE_LEXER_ADA)},
  { "ActionFormatHighlightStyleBaan", N_("Baan"), NULL, NULL,
	N_("Force the highlight style to Baan"),
    G_CALLBACK (on_force_hilite1_activate), GUINT_TO_POINTER (TE_LEXER_BAAN)},
  { "ActionFormatHighlightStyleLisp", N_("Lisp"), NULL, NULL,
	N_("Force the highlight style to Lisp"),
    G_CALLBACK (on_force_hilite1_activate), GUINT_TO_POINTER (TE_LEXER_LISP)},
  { "ActionFormatHighlightStyleRuby", N_("Ruby"), NULL, NULL,
	N_("Force the highlight style to Ruby"),
    G_CALLBACK (on_force_hilite1_activate), GUINT_TO_POINTER (TE_LEXER_RUBY)},
  { "ActionFormatHighlightStyleMatlab", N_("Matlab"), NULL, NULL,
	N_("Force the highlight style to Matlab"),
    G_CALLBACK (on_force_hilite1_activate), GUINT_TO_POINTER (TE_LEXER_MATLAB)},
};

static EggActionGroupEntry actions_format[] = {
  { "ActionMenuFormat", N_("For_mat"), NULL, NULL, NULL, NULL, NULL },
  { "ActionFormatAutoformat", N_("Auto _Format"),
	ANJUTA_STOCK_INDENT_AUTO, NULL,
	N_("Autoformat the current source file"),
    G_CALLBACK (on_indent1_activate), NULL},
  { "ActionFormatSettings", N_("Autoformat _settings"),
	ANJUTA_STOCK_AUTOFORMAT_SETTINGS, NULL,
	N_("Autoformat settings"),
    G_CALLBACK (on_format_indent_style_clicked), NULL},
  { "ActionFormatIndentationIncrease", N_("_Increase Indent"),
	ANJUTA_STOCK_INDENT_INC, NULL,
	N_("Increase indentation of line/selection"),
    G_CALLBACK (on_editor_command_activate), (gpointer) ANE_INDENT_INCREASE},
  { "ActionFormatIndentationDecrease", N_("_Decrease Indent"),
	ANJUTA_STOCK_INDENT_DCR, NULL,
	N_("Decrease indentation of line/selection"),
    G_CALLBACK (on_editor_command_activate), (gpointer) ANE_INDENT_DECREASE},
  { "ActionFormatFoldCloseAll", N_("_Close All Folds"),
	ANJUTA_STOCK_FOLD_CLOSE, NULL,
	N_("Close all code folds in the editor"),
    G_CALLBACK (on_editor_command_activate), (gpointer) ANE_CLOSE_FOLDALL},
  { "ActionFormatFoldOpenAll", N_("_Open All Folds"),
	ANJUTA_STOCK_FOLD_OPEN, NULL,
	N_("Open all code folds in the editor"),
    G_CALLBACK (on_editor_command_activate), (gpointer) ANE_OPEN_FOLDALL},
  { "ActionFormatFoldToggle", N_("_Toggle Current Fold"),
	ANJUTA_STOCK_FOLD_TOGGLE, NULL,
	N_("Toggle current code fold in the editor"),
    G_CALLBACK (on_editor_command_activate), (gpointer) ANE_TOGGLE_FOLD},
/*  { "ActionFormatEditorDetach", N_("D_etach Current Document"),
	ANJUTA_STOCK_UNDOCK, NULL,
	N_("Detach the current document into a separate editor window"),
    G_CALLBACK (on_detach1_activate), NULL},*/
};

static EggActionGroupEntry actions_bookmark[] = {
  { "ActionMenuBookmark", N_("Bookmar_k"), NULL, NULL, NULL, NULL, NULL },
  { "ActionBookmarkToggle", N_("_Toggle bookmark"),
	ANJUTA_STOCK_TOGGLE_BOOKMARK, "<control>k",
	N_("Toggle a bookmark at the current line position"),
    G_CALLBACK (on_editor_command_activate), (gpointer) ANE_BOOKMARK_TOGGLE},
  { "ActionBookmarkFirst", N_("_First bookmark"),
	GTK_STOCK_GOTO_FIRST, NULL,
	N_("Jump to the first bookmark in the file"),
    G_CALLBACK (on_editor_command_activate), (gpointer) ANE_BOOKMARK_FIRST},
  { "ActionBookmarkPrevious", N_("_Previous bookmark"),
	GTK_STOCK_GO_BACK, NULL,
	N_("Jump to the previous bookmark in the file"),
    G_CALLBACK (on_editor_command_activate), (gpointer) ANE_BOOKMARK_PREV},
  { "ActionBookmarkNext", N_("_Next bookmark"),
	GTK_STOCK_GO_FORWARD, NULL,
	N_("Jump to the next bookmark in the file"),
    G_CALLBACK (on_editor_command_activate), (gpointer) ANE_BOOKMARK_NEXT},
  { "ActionBookmarkLast", N_("_Last bookmark"),
	GTK_STOCK_GOTO_LAST, NULL,
	N_("Jump to the last bookmark in the file"),
    G_CALLBACK (on_editor_command_activate), (gpointer) ANE_BOOKMARK_LAST},
  { "ActionBookmarkClear", N_("_Clear all bookmarks"),
	GTK_STOCK_CLOSE, NULL,
	N_("Clear bookmarks"),
    G_CALLBACK (on_editor_command_activate), (gpointer) ANE_BOOKMARK_CLEAR},
};

static void
init_user_data (EggActionGroupEntry* actions, gint size, gpointer data)
{
	int i;
	for (i = 0; i < size; i++)
		actions[i].user_data = data;
}

static void
ui_set (AnjutaPlugin *plugin)
{
	g_message ("EditorPlugin: UI set");
}

static void
prefs_set (AnjutaPlugin *plugin)
{
	g_message ("EditorPlugin: Prefs set");
}

static void
shell_set (AnjutaPlugin *plugin)
{
	GtkWidget *docman;
	AnjutaUI *ui;
	EditorPlugin *editor_plugin;
	EggActionGroup *group;
	EggAction *action;
	
	g_message ("EditorPlugin: Shell set. Activating Editor plugin ...");
	editor_plugin = (EditorPlugin*) plugin;
	ui = plugin->ui;
	docman = anjuta_docman_new (plugin->prefs);
	editor_plugin->docman = docman;
	
	/* Add all our editor actions */
	init_user_data (actions_file, G_N_ELEMENTS (actions_file), editor_plugin);
	anjuta_ui_add_action_group_entries (ui, "ActionGroupEditorFile",
										_("Editor file operations"),
										actions_file,
										G_N_ELEMENTS (actions_file));
	init_user_data (actions_print, G_N_ELEMENTS (actions_print), editor_plugin);
	anjuta_ui_add_action_group_entries (ui, "ActionGroupEditorPrint",
										_("Editor print operations"),
										actions_print,
										G_N_ELEMENTS (actions_print));
	init_user_data (actions_edit, G_N_ELEMENTS (actions_edit), editor_plugin);
	anjuta_ui_add_action_group_entries (ui, "ActionGroupEditorEdit",
										_("Editor edit operations"),
										actions_edit,
										G_N_ELEMENTS (actions_edit));
	init_user_data (actions_transform, G_N_ELEMENTS (actions_transform), editor_plugin);
	anjuta_ui_add_action_group_entries (ui, "ActionGroupEditorTransform",
										_("Editor text transformation"),
										actions_transform,
										G_N_ELEMENTS (actions_transform));
	init_user_data (actions_select, G_N_ELEMENTS (actions_select), editor_plugin);
	anjuta_ui_add_action_group_entries (ui, "ActionGroupEditorSelect",
										_("Editor text selection"),
										actions_select,
										G_N_ELEMENTS (actions_select));
	init_user_data (actions_insert, G_N_ELEMENTS (actions_insert), editor_plugin);
	anjuta_ui_add_action_group_entries (ui, "ActionGroupEditorInsert",
										_("Editor text insertions"),
										actions_insert,
										G_N_ELEMENTS (actions_insert));
	init_user_data (actions_comment, G_N_ELEMENTS (actions_comment), editor_plugin);
	anjuta_ui_add_action_group_entries (ui, "ActionGroupEditorComment",
										_("Editor code commenting"),
										actions_comment,
										G_N_ELEMENTS (actions_comment));
	init_user_data (actions_search, G_N_ELEMENTS (actions_search), editor_plugin);
	anjuta_ui_add_action_group_entries (ui, "ActionGroupEditorSearch",
										_("Editor text searching"),
										actions_search,
										G_N_ELEMENTS (actions_search));
	init_user_data (actions_navigation, G_N_ELEMENTS (actions_navigation), editor_plugin);
	anjuta_ui_add_action_group_entries (ui, "ActionGroupEditorNavigate",
										_("Editor navigations"),
										actions_navigation,
										G_N_ELEMENTS (actions_navigation));
	init_user_data (actions_view, G_N_ELEMENTS (actions_view), editor_plugin);
	anjuta_ui_add_action_group_entries (ui, "ActionGroupEditorView",
										_("Editor view settings"),
										actions_view,
										G_N_ELEMENTS (actions_view));
	init_user_data (actions_style, G_N_ELEMENTS (actions_style), editor_plugin);
	anjuta_ui_add_action_group_entries (ui, "ActionGroupEditorFormatStyle",
										_("Editor syntax highlighting styles"),
										actions_style,
										G_N_ELEMENTS (actions_style));
	init_user_data (actions_format, G_N_ELEMENTS (actions_format), editor_plugin);
	anjuta_ui_add_action_group_entries (ui, "ActionGroupEditorFormat",
										_("Editor text formating"),
										actions_format,
										G_N_ELEMENTS (actions_format));
	init_user_data (actions_bookmark, G_N_ELEMENTS (actions_bookmark), editor_plugin);
	anjuta_ui_add_action_group_entries (ui, "ActionGroupEditorBookmark",
										_("Editor bookmarks"),
										actions_bookmark,
										G_N_ELEMENTS (actions_bookmark));
	/* Navigation items */
	group = egg_action_group_new ("ActionGroupNavigation");
	
	action = g_object_new (EGG_TYPE_ENTRY_ACTION,
						   "name", "ActionEditGotoLineEntry",
						   "label", _("Goto line"),
						   "tooltip", _("Enter the line number to jump and press enter"),
						   "stock_id", GTK_STOCK_JUMP_TO,
						   "width", 50,
							NULL);
	g_signal_connect (action, "activate",
					  G_CALLBACK (on_toolbar_goto_clicked), ui);
	egg_action_group_add_action (group, action);
	
	action = g_object_new (EGG_TYPE_ENTRY_ACTION,
						   "name", "ActionEditSearchEntry",
						   "label", _("Search"),
						   "tooltip", _("Incremental search"),
						   "stock_id", GTK_STOCK_JUMP_TO,
						   "width", 150,
							NULL);
	g_signal_connect (action, "activate",
					  G_CALLBACK (on_toolbar_find_clicked), ui);
	g_signal_connect (action, "changed",
					  G_CALLBACK (on_toolbar_find_incremental), ui);
	g_signal_connect (action, "focus-in",
					  G_CALLBACK (on_toolbar_find_incremental_start), ui);
	g_signal_connect (action, "focus_out",
					  G_CALLBACK (on_toolbar_find_incremental_end), ui);
	egg_action_group_add_action (group, action);
	
	anjuta_ui_add_action_group (ui, "ActionGroupNavigation",
								N_("Editor quick navigations"), group);
	
	editor_plugin->uiid = anjuta_ui_merge (plugin->ui, UI_FILE);
	anjuta_shell_add_widget (plugin->shell, docman,
				  "AnjutaDocumentManager", _("Documents"), NULL);
}

static void
dispose (GObject *obj)
{
	EditorPlugin *plugin = (EditorPlugin*)obj;
	anjuta_ui_unmerge (ANJUTA_PLUGIN (obj)->ui, plugin->uiid);
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

	plugin_class->shell_set = shell_set;
	plugin_class->ui_set = ui_set;
	plugin_class->prefs_set = prefs_set;
	klass->dispose = dispose;
}

ANJUTA_PLUGIN_BOILERPLATE (EditorPlugin, editor_plugin);
ANJUTA_SIMPLE_PLUGIN (EditorPlugin, editor_plugin);
