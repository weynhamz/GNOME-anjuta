/* 
    main_menubar_def.h
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
#ifndef _MAIN_MENUBAR_DEF_H_
#define _MAIN_MENUBAR_DEF_H_

#include <gnome.h>
#include "lexer.h"

static GnomeUIInfo recent_projects1_menu_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("project1"),
	 NULL,
	 on_project1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_END
};

static GnomeUIInfo file1_menu_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("_New"),
	 N_("New file"),
	 on_new_file1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_NEW,
	 GDK_N, GDK_MOD1_MASK, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("_Open ..."),
	 N_("Open file"),
	 on_open1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_OPEN,
	 GDK_O, GDK_CONTROL_MASK, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("_Save"),
	 N_("Save current file"),
	 on_save1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE,
	 GDK_S, GDK_CONTROL_MASK, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Save _As ..."),
	 N_("Save the current file with a different name"),
	 on_save_as1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE_AS,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Save A_ll"),
	 N_("Save all currently open files, except new files"),
	 on_save_all1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE,
	 GDK_L, GDK_CONTROL_MASK, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("_Close File"),
	 N_("Close current file"),
	 on_close_file1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CLOSE,
	 GDK_D, GDK_MOD1_MASK, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Close All Files"),
	 N_("Close all files"),
	 on_close_all_file1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CLOSE,
	 GDK_D, GDK_MOD1_MASK, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Reload F_ile"),
	 N_("Reload current file"),
	 on_reload_file1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_REVERT,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("N_ew Project ..."),
	 N_("Create a Project using the Application Wizard"),
	 on_new_project1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_NEW,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Import Project ..."),
	 N_("Import a Project using the import project wizard"),
	 on_import_project_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CONVERT,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Open P_roject ..."),
	 N_("Open a Project"),
	 on_open_project1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_OPEN,
	 GDK_J, GDK_CONTROL_MASK, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Sa_ve Project"),
	 N_("Save the current Project"),
	 on_save_project1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Close Pro_ject"),
	 N_("Close the current Project"),
	 on_close_project1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CLOSE,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Rena_me ..."),
	 N_("Rename the current file"),
	 on_rename1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Page Set_up ..."),
	 N_("Page setup for printing"),
	 on_page_setup1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("_Print ..."),
	 N_("Print the current file"),
	 on_print1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PRINT,
	 GDK_P, GDK_MOD1_MASK, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Recent _Files"),
	 NULL,
	 NULL, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_SUBTREE, N_("Recent Projec_ts"),
	 NULL,
	 recent_projects1_menu_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_EXIT_ITEM (on_exit1_activate, NULL),
	GNOMEUIINFO_END
};

# define NUM_TRANSFORM_SUBMENUS (7)
static GnomeUIInfo transform1_submenu_uiinfo[NUM_TRANSFORM_SUBMENUS+1] = {
	{
	 /* 0 */
	 GNOME_APP_UI_ITEM, N_("_Make Selection Uppercase"),
	 N_("Make the selected text uppercase"),
	 on_transform_upper1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 /* 1 */
	 GNOME_APP_UI_ITEM, N_("Make Selection Lowercase"),
	 N_("Make the selected text lowercase"),
	 on_transform_lower1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	 /* 2 */
	 GNOMEUIINFO_SEPARATOR,
	{
	 /* 3 */
	 GNOME_APP_UI_ITEM, N_("Convert EOL chars to CRLF"),
	 N_("Convert End Of Line characters to DOS EOL (CRLF)"),
	 on_transform_eolchars1_activate, GUINT_TO_POINTER (ANE_EOL_CRLF), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 /* 4 */
	 GNOME_APP_UI_ITEM, N_("Convert EOL chars to LF"),
	 N_("Convert End Of Line characters to Unix EOL (LF)"),
	 on_transform_eolchars1_activate, GUINT_TO_POINTER (ANE_EOL_LF), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 /* 5 */
	 GNOME_APP_UI_ITEM, N_("Convert EOL chars to CR"),
	 N_("Convert End Of Line characters to Mac OS EOL (CR)"),
	 on_transform_eolchars1_activate, GUINT_TO_POINTER (ANE_EOL_CR), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 /* 6 */
	 GNOME_APP_UI_ITEM, N_("Convert EOL chars to majority EOL"),
	 N_("Convert End Of Line characters to mojority of the EOL found in the file"),
	 on_transform_eolchars1_activate, GUINT_TO_POINTER (0), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	 /* 7 */
	GNOMEUIINFO_END
};

static GnomeUIInfo select1_submenu_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("_Select All"),
	 N_("Select all text in the editor"),
	 on_select_all1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_A, GDK_CONTROL_MASK, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Select to _Brace"),
	 N_("Select the text in the matching braces"),
	 on_select_matchbrace1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_A, GDK_MOD1_MASK, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Select Code Block"),
	 N_("Select the current code block"),
	 on_select_block1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_B, GDK_MOD1_MASK, NULL},
	GNOMEUIINFO_END
};

static GnomeUIInfo inserttext1_submenu_uiinfo[] = {
       {
        GNOME_APP_UI_ITEM, N_("/* GPL Notice */"),
        N_("Insert GPL notice with C style comments"),
        on_insert_c_gpl_notice, NULL, NULL,
        GNOME_APP_PIXMAP_NONE, NULL,
        0, 0, NULL},
       {
        GNOME_APP_UI_ITEM, N_("// GPL Notice"),
        N_("Insert GPL notice with C++ style comments"),
        on_insert_cpp_gpl_notice, NULL, NULL,
        GNOME_APP_PIXMAP_NONE, NULL,
        0, 0, NULL},
	   {
		GNOME_APP_UI_ITEM, N_("# GPL Notice"),
		N_("Insert GPL notice with Python style comments"),
		on_insert_py_gpl_notice, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL,
		0, 0, NULL},
	   {
		GNOME_APP_UI_ITEM, N_("Current Username"),
		N_("Insert name of current user"),
		on_insert_username, NULL, NULL,
		GNOME_APP_PIXMAP_NONE, NULL,
		0, 0, NULL},
       { 
        GNOME_APP_UI_ITEM, N_("Current Date & Time"),
        N_("Insert current date & time"),
        on_insert_date_time, NULL, NULL,
        GNOME_APP_PIXMAP_NONE, NULL,
        0, 0, NULL},
       { 
        GNOME_APP_UI_ITEM, N_("Header File Template"),
        N_("Insert a standard header file template"),
        on_insert_header_template, NULL, NULL,
        GNOME_APP_PIXMAP_NONE, NULL,
        0, 0, NULL},
       GNOMEUIINFO_END
};


static GnomeUIInfo goto1_submenu_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("_Goto Line number ..."),
	 N_("Go to a particular line in the editor"),
	 on_goto_line_no1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_G, GDK_CONTROL_MASK, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Goto Matching _Brace"),
	 N_("Go to the matching brace in the editor"),
	 on_goto_matchbrace1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_M, GDK_CONTROL_MASK, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("_Goto start of block"),
	 N_("Go to the start of the current block"),
	 on_goto_block_start1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_S, GDK_CONTROL_MASK | GDK_MOD1_MASK, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("_Goto end of block"),
	 N_("Go to the end of the current block"),
	 on_goto_block_end1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_E, GDK_CONTROL_MASK | GDK_MOD1_MASK, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("_Goto previous mesg"),
	 N_("Go to previous message"),
	 on_goto_prev_mesg1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_P, GDK_CONTROL_MASK | GDK_MOD1_MASK, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("_Goto next mesg"),
	 N_("Go to next message"),
	 on_goto_next_mesg1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_N, GDK_CONTROL_MASK | GDK_MOD1_MASK, NULL},
	GNOMEUIINFO_END
};


#define	NUM_EDIT_MENUS	(24)
static GnomeUIInfo edit1_menu_uiinfo[NUM_EDIT_MENUS+1] = {
	{/*0*/
	 GNOME_APP_UI_ITEM, N_("U_ndo"),
	 N_("Undo the last action"),
	 on_undo1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_UNDO,
	 GDK_Z, GDK_CONTROL_MASK, NULL},

	{/*1*/
	 GNOME_APP_UI_ITEM, N_("_Redo"),
	 N_("Redo the last undone action"),
	 on_redo1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_REDO,
	 GDK_R, GDK_CONTROL_MASK, NULL},

	 GNOMEUIINFO_SEPARATOR,/*2*/
	
	{/*3*/
	 GNOME_APP_UI_ITEM, N_("C_ut"),
	 N_("Cut the selected text from the editor to the clipboard"),
	 on_cut1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CUT,
	 GDK_X, GDK_CONTROL_MASK, NULL},
	
	{/*4*/
	 GNOME_APP_UI_ITEM, N_("_Copy"),
	 N_("Copy the selected text to the clipboard"),
	 on_copy1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_COPY,
	 GDK_C, GDK_CONTROL_MASK, NULL},
	
	{/*5*/
	 GNOME_APP_UI_ITEM, N_("_Paste"),
	 N_("Paste the content of clipboard at the current position"),
	 on_paste1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PASTE,
	 GDK_V, GDK_CONTROL_MASK, NULL},
	
	{/*6*/
	 GNOME_APP_UI_ITEM, N_("C_lear"),
	 N_("Delete the selected text from the editor"),
	 on_clear1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_Delete, 0, NULL},
	
	GNOMEUIINFO_SEPARATOR,/*7*/
	
	{/*8*/
	 GNOME_APP_UI_SUBTREE, N_("_Transform"),
	 NULL,
	 transform1_submenu_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	
	{/*9*/
	 GNOME_APP_UI_SUBTREE, N_("_Select"),
	 NULL,
	 select1_submenu_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	
	{/*10*/
	 GNOME_APP_UI_SUBTREE, N_("_Insert text"),
	 NULL,
	 inserttext1_submenu_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	
	GNOMEUIINFO_SEPARATOR,/*11*/
	
	{/*12*/
	 GNOME_APP_UI_ITEM, N_("_AutoComplete"),
	 N_("AutoComplete the current word"),
	 on_autocomplete1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_Return, GDK_CONTROL_MASK, NULL},
	
	{/*13*/
	 GNOME_APP_UI_ITEM, N_("S_how calltip"),
	 N_("Show calltip for the function"),
	 on_calltip1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},

	 GNOMEUIINFO_SEPARATOR,/*14*/
	
	{/*15*/
	 GNOME_APP_UI_ITEM, N_("_Find ..."),
	 N_("Search for a string or regular expression in the editor"),
	 on_find1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SEARCH,
	 GDK_F, GDK_CONTROL_MASK, NULL},
	
	{/*16*/
	 GNOME_APP_UI_ITEM, N_("Fin_d in Files ..."),
	 N_("Search for a string in multiple files or directories"),
	 on_find_in_files1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	
	{/*17*/
	GNOME_APP_UI_ITEM, N_("Find and R_eplace ..."),
	N_("Search for and replace a string or regular expression with another string"),
	on_find_and_replace1_activate, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SRCHRPL,
	GDK_F, GDK_CONTROL_MASK | GDK_SHIFT_MASK, NULL},
	
	{/*18*/
	 GNOME_APP_UI_ITEM, N_("Find _Next"),
	 N_("Repeat the last Find command"),
	 on_findnext1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SEARCH,
	 GDK_P, GDK_CONTROL_MASK, NULL},
	 
	{/*19*/
	 GNOME_APP_UI_SUBTREE, N_("G_o to"),
	 NULL,
	 goto1_submenu_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	
	GNOMEUIINFO_SEPARATOR,/*20*/

	 {/*21*/
	 GNOME_APP_UI_ITEM, N_("Ed_it Application GUI ..."),
	 N_("Edit application GUI with the Glade GUI editor"),
	 on_edit_app_gui1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_G, GDK_MOD1_MASK, NULL},
	
	GNOMEUIINFO_SEPARATOR,/*22*/
	
	{/*23*/
	 GNOME_APP_UI_ITEM, N_("Save Build Messages"),
	 N_("Save build messages to file"),
	 on_save_build_messages_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	/*24*/
	GNOMEUIINFO_END
};

static GnomeUIInfo toolbar1_submenu_uiinfo[] = {
	{
	 GNOME_APP_UI_TOGGLEITEM, N_("_Main Toolbar"),
	 N_("Hide/Unhide Main toolbar"),
	 on_main_toolbar1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_TOGGLEITEM, N_("_Extended Toolbar"),
	 N_("Hide/Unhide Extended toolbar"),
	 on_extended_toolbar1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_TOGGLEITEM, N_("_Debug Toolbar"),
	 N_("Hide/Unhide Debug toolbar"),
	 on_debug_toolbar1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_TOGGLEITEM, N_("_Browser Toolbar"),
	 N_("Hide/Unhide Browser toolbar"),
	 on_browser_toolbar1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_TOGGLEITEM, N_("_Format Toolbar"),
	 N_("Hide/Unhide Format toolbar"),
	 on_format_toolbar1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_END
};

static GnomeUIInfo editor1_submenu_uiinfo[] = {
	{
	 GNOME_APP_UI_TOGGLEITEM, N_("_Line numbers margin"),
	 N_("Show/Hide line numbers"),
	 on_editor_linenos1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_TOGGLEITEM, N_("_Markers Margin"),
	 N_("Show/Hide markers margin"),
	 on_editor_markers1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_TOGGLEITEM, N_("_Code fold margin"),
	 N_("Show/Hide code fold margin"),
	 on_editor_codefold1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_TOGGLEITEM, N_("_Indentation guides"),
	 N_("Show/Hide undentation guides"),
	 on_editor_indentguides1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_TOGGLEITEM, N_("_White spaces"),
	 N_("Show/Hide white spaces"),
	 on_editor_whitespaces1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_TOGGLEITEM, N_("_Line end characters"),
	 N_("Show/Hide line end characters"),
	 on_editor_eolchars1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_END
};

static GnomeUIInfo zoom_text1_submenu_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("++ Zoom"),
	 N_("Increase text zoom by 1 unit"),
	 on_zoom_text_plus_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("+8"),
	 N_("Zoom factor +8"),
	 on_zoom_text_8_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("+6"),
	 N_("Zoom factor +6"),
	 on_zoom_text_6_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("+4"),
	 N_("Zoom factor +4"),
	 on_zoom_text_4_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("+2"),
	 N_("Zoom factor +2"),
	 on_zoom_text_2_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("0"),
	 N_("Zoom factor 0"),
	 on_zoom_text_0_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("-2"),
	 N_("Zoom factor -2"),
	 on_zoom_text_s2_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("-4"),
	 N_("Zoom factor -4"),
	 on_zoom_text_s4_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("-6"),
	 N_("Zoom factor -6"),
	 on_zoom_text_s6_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("-8"),
	 N_("Zoom factor -8"),
	 on_zoom_text_s8_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("-- Zoom"),
	 N_("Reduce text zoom by 1 unit"),
	 on_zoom_text_minus_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_END
};

#define	NUM_VIEW_MENUS	(21)
static GnomeUIInfo view1_menu_uiinfo[NUM_VIEW_MENUS+1] = {
	{/*0*/
	 GNOME_APP_UI_ITEM, N_("_Messages"),
	 N_("Show/Hide the Message window"),
	 on_messages1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ALIGN_LEFT,
	 GDK_F1, GDK_CONTROL_MASK, NULL},
	{/*1*/
	 GNOME_APP_UI_ITEM, N_("_Project Listing"),
	 N_("Show/Hide the Project window"),
	 on_project_listing1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ALIGN_LEFT,
	 GDK_F2, GDK_CONTROL_MASK, NULL},
	{/*2*/
	 GNOME_APP_UI_ITEM, N_("B_ookmarks"),
	 N_("Show the Bookmark window"),
	 on_bookmarks1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_F3, GDK_CONTROL_MASK, NULL},
	GNOMEUIINFO_SEPARATOR,/*3*/
	{/*4*/
	 GNOME_APP_UI_SUBTREE, N_("_Toolbars"),
	 NULL,
	 toolbar1_submenu_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,/*5*/
	{/*6*/
	 GNOME_APP_UI_SUBTREE, N_("_Editor"),
	 NULL,
	 editor1_submenu_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,/*7*/
	{/*8*/
	 GNOME_APP_UI_SUBTREE, N_("_Zoom text"),
	 NULL,
	 zoom_text1_submenu_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,/*9*/
	{/*10*/
	 GNOME_APP_UI_ITEM, N_("_Breakpoints"),
	 N_("Show breakpoints editor window"),
	 on_breakpoints1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_F4, GDK_CONTROL_MASK, NULL},
	GNOMEUIINFO_SEPARATOR,/*11*/
	{/*12*/
	 GNOME_APP_UI_ITEM, N_("_Watch Window"),
	 N_("Show expression watch window"),
	 on_watch_window1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_F5, GDK_CONTROL_MASK, NULL},
	{/*13*/
	 GNOME_APP_UI_ITEM, N_("_Registers"),
	 N_("Show CPU registers and their contents"),
	 on_registers1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_F6, GDK_CONTROL_MASK, NULL},
	{/*14*/
	 GNOME_APP_UI_ITEM, N_("Program _Stack"),
	 N_("Show stack trace of the program"),
	 on_program_stack1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_F7, GDK_CONTROL_MASK, NULL},
	{/*15*/
	 GNOME_APP_UI_ITEM, N_("Shared _Libraries"),
	 N_("Show shared libraries loaded by the program"),
	 on_shared_lib1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_F8, GDK_CONTROL_MASK, NULL},
	{/*16*/
	 GNOME_APP_UI_ITEM, N_("_Kernel Signals"),
	 N_("Show the kernel signals editor window"),
	 on_kernal_signals1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_F9, GDK_CONTROL_MASK, NULL},
	{/*17*/
	 GNOME_APP_UI_ITEM, N_("_Dump Window"),
	 N_("Show memory dump window"),
	 on_dump_window1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_F10, GDK_CONTROL_MASK, NULL},
	GNOMEUIINFO_SEPARATOR,/*18*/
	{/*19*/
	 GNOME_APP_UI_ITEM, N_("_Console"),
	 N_("Show the console where the program runs"),
	 on_console1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_MIDI,
	 GDK_F11, GDK_CONTROL_MASK, NULL},
	{/*20*/
	 GNOME_APP_UI_TOGGLEITEM, N_("Show _Locals"),
	 N_("Show/Hide Local variables in Message window"),
	 on_showhide_locals, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_END/*21*/
};

static GnomeUIInfo import_file1_menu_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("Include file"),
	 NULL,
	 on_project_include_file1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Source file"),
	 NULL,
	 on_project_source_file1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Help file"),
	 NULL,
	 on_project_help_file1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Data file"),
	 NULL,
	 on_project_data_file1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Pixmap file"),
	 NULL,
	 on_project_pixmap_file1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Translation file"),
	 NULL,
	 on_project_translation_file1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Documentation file"),
	 NULL,
	 on_project_doc_file1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_END
};

static GnomeUIInfo project1_menu_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("Add New"),
	 NULL,
	 on_project_add_new1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_SUBTREE, N_("Import File"),
	 NULL,
	 import_file1_menu_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Remove"),
	 N_("Remove the current file from the Project"),
	 on_project_remove1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, "READ_ME",
	 N_("Open the README file of the Project"),
	 on_prj_readme1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_NEW,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, "_TODO",
	 N_("Open the TODO file of the Project"),
	 on_prj_todo1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_NEW,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, "Change_Log",
	 N_("Open the ChangeLog file of the Project"),
	 on_prj_changelog1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_NEW,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, "_NEWS",
	 N_("Open the NEWS file of the Project"),
	 on_prj_news1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_NEW,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Project _Configuration ..."),
	 N_("Edit the configuration of the Project"),
	 on_project_configure1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PREF,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Project _Information"),
	 N_("Detailed information about the Project"),
	 on_project_project_info1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BOOK_OPEN,
	 0, 0, NULL},
	GNOMEUIINFO_END
};

static GnomeUIInfo hilitetype1_submenu_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("Automatic"),
	 N_("Automatically determine the highlight style"),
	 on_force_hilite1_activate, GUINT_TO_POINTER (TE_LEXER_AUTOMATIC), NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_EXEC,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("No Highlight style"),
	 N_("Force the highlight style to None"),
	 on_force_hilite1_activate, GUINT_TO_POINTER (TE_LEXER_NONE), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("C and C++"),
	 N_("Force the highlight style to C and C++"),
	 on_force_hilite1_activate, GUINT_TO_POINTER (TE_LEXER_CPP), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("C and C++ with GNOME"),
	 N_("Force the highlight style to C and C++ with GNOME"),
	 on_force_hilite1_activate, GUINT_TO_POINTER (TE_LEXER_GCPP), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("HTML"),
	 N_("Force the highlight style to HTML"),
	 on_force_hilite1_activate, GUINT_TO_POINTER (TE_LEXER_HTML), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("XML"),
	 N_("Force the highlight style to XML"),
	 on_force_hilite1_activate, GUINT_TO_POINTER (TE_LEXER_XML), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Javascript"),
	 N_("Force the highlight style to Javascript"),
	 on_force_hilite1_activate, GUINT_TO_POINTER (TE_LEXER_JS), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("WScript"),
	 N_("Force the highlight style to WScript"),
	 on_force_hilite1_activate, GUINT_TO_POINTER (TE_LEXER_WSCRIPT), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Makefile"),
	 N_("Force the highlight style to Makefile"),
	 on_force_hilite1_activate, GUINT_TO_POINTER (TE_LEXER_MAKE), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Java"),
	 N_("Force the highlight type to Java"),
	 on_force_hilite1_activate, GUINT_TO_POINTER (TE_LEXER_JAVA), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("LUA"),
	 N_("Force the highlight style to LUA"),
	 on_force_hilite1_activate, GUINT_TO_POINTER (TE_LEXER_LUA), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Python"),
	 N_("Force the highlight style to Python"),
	 on_force_hilite1_activate, GUINT_TO_POINTER (TE_LEXER_PYTHON), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Perl"),
	 N_("Force the highlight style to Perl"),
	 on_force_hilite1_activate, GUINT_TO_POINTER (TE_LEXER_PERL), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("SQL"),
	 N_("Force the highlight style to SQL"),
	 on_force_hilite1_activate, GUINT_TO_POINTER (TE_LEXER_SQL), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("PL/SQL"),
	 N_("Force the highlight style to PL/SQL"),
	 on_force_hilite1_activate, GUINT_TO_POINTER (TE_LEXER_PLSQL), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("PHP"),
	 N_("Force the highlight style to PHP"),
	 on_force_hilite1_activate, GUINT_TO_POINTER (TE_LEXER_PHP), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("LaTex"),
	 N_("Force the highlight style to LaTex"),
	 on_force_hilite1_activate, GUINT_TO_POINTER (TE_LEXER_LATEX), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Diff"),
	 N_("Force the highlight style to Diff"),
	 on_force_hilite1_activate, GUINT_TO_POINTER (TE_LEXER_DIFF), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Pascal"),
	 N_("Force the highlight style to Pascal"),
	 on_force_hilite1_activate, GUINT_TO_POINTER (TE_LEXER_PASCAL), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Xcode"),
	 N_("Force the highlight style to Xcode"),
	 on_force_hilite1_activate, GUINT_TO_POINTER (TE_LEXER_XCODE), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Prj/Properties"),
	 N_("Force the highlight style to project/properties files"),
	 on_force_hilite1_activate, GUINT_TO_POINTER (TE_LEXER_PROPS), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Conf"),
	 N_("Force the highlight style to UNIX conf files"),
	 on_force_hilite1_activate, GUINT_TO_POINTER (TE_LEXER_CONF), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Ada"),
	 N_("Force the highlight style to Ada"),
	 on_force_hilite1_activate, GUINT_TO_POINTER (TE_LEXER_ADA), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Baan"),
	 N_("Force the highlight style to Baan"),
	 on_force_hilite1_activate, GUINT_TO_POINTER (TE_LEXER_BAAN), NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_END
};

static GnomeUIInfo format1_menu_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("Auto _Format"),
	 N_("Autoformat C and C++ source files"),
	 on_indent1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ALIGN_LEFT,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("_Increase Indent"),
	 N_("Increase indentation of line/selection"),
	 on_indent_inc1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("_Decrease Indent"),
	 N_("Decrease indentation of line/selection"),
	 on_indent_dcr1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("_Update tags image"),
	 N_("Update the tags image of the project/opened files"),
	 on_update_tags1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_FONT,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_SUBTREE, N_("Force _Highlight Style"),
	 NULL,
	 hilitetype1_submenu_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("_Close All Folds"),
	 N_("Close all code folds in the editor"),
	 on_close_folds1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("_Open All Folds"),
	 N_("Open all code folds of the editor"),
	 on_open_folds1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("_Toggle Current Fold"),
	 N_("Toggle current code fold of the editor"),
	 on_toggle_fold1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("D_etach Current Document"),
	 N_("Detach the current editor into a separate window"),
	 on_detach1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_END
};


static GnomeUIInfo build1_menu_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("_Compile"),
	 N_("Compile the current source file"),
	 on_compile1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CONVERT,
	 GDK_F9, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Compile With _Make"),
	 N_("Compile the current source file using Make"),
	 on_make1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_F9, GDK_SHIFT_MASK, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("_Build"),
	 N_("Build the source directory of the Project/current source file"),
	 on_build_project1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_EXEC,
	 GDK_F10, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Build _All"),
	 N_("Build the whole Project"),
	 on_build_all_project1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_F11, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("_Install"),
	 N_("Install the Project on your system"),
	 on_install_project1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Build _Distribution"),
	 N_("Build the distribution tarball of the Project"),
	 on_build_dist_project1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Con_figure ..."),
	 N_("Configure the Project"),
	 on_configure_project1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Auto _generate ..."),
	 N_("Auto generate all the build files"),
	 on_autogen_project1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Clea_n"),
	 N_("Clean the source directory"),
	 on_clean_project1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Clean A_ll"),
	 N_("Clean the whole Project directory"),
	 on_clean_all_project1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("_Stop Build"),
	 N_("Stop the current compile or build process"),
	 on_stop_build_make1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_STOP,
	 GDK_F12, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("_Execute"),
	 N_("Execute the program"),
	 on_go_execute1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_JUMP_TO,
	 GDK_F3, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Set _Program params ..."),
	 N_("Set the execution parameters of the program"),
	 on_go_execute2_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_JUMP_TO,
	 0, 0, NULL},
	GNOMEUIINFO_END
};

static GnomeUIInfo bookmark1_menu_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("_Toggle bookmark"),
	 NULL,
	 on_book_toggle1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_INDEX,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("_First bookmark"),
	 NULL,
	 on_book_first1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_TOP,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("_Previous bookmark"),
	 NULL,
	 on_book_prev1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_UP,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("_Next bookmark"),
	 NULL,
	 on_book_next1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_DOWN,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("_Last bookmark"),
	 NULL,
	 on_book_last1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BOTTOM,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("_Clear all bookmarks"),
	 NULL,
	 on_book_clear1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CLOSE,
	 0, 0, NULL},
	GNOMEUIINFO_END
};

static GnomeUIInfo execution1_submenu_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("Run/_Continue"),
	 N_("Continue the execution of the program"),
	 on_execution_continue1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_EXEC,
	 GDK_F4, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Step _In"),
	 N_("Single step into function"),
	 on_execution_step_in1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_F5, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Step O_ver"),
	 N_("Single step over function"),
	 on_execution_step_over1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_F6, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Step _Out"),
	 N_("Single step out of the function"),
	 on_execution_step_out1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_F7, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("_Run to cursor"),
	 N_("Run to the cursor"),
	 on_execution_run_to_cursor1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_F8, 0, NULL},
	GNOMEUIINFO_END
};

static GnomeUIInfo breakpoints1_submenu_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("Toggle breakpoint"),
	 N_("Toggle breakpoint at the current location"),
	 on_toggle_breakpoint1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_INDEX,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Set Breakpoint ..."),
	 N_("Set a breakpoint"),
	 on_set_breakpoint1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_INDEX,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("_Breakpoints ..."),
	 N_("Edit breakpoints"),
	 on_show_breakpoints1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Disable all Breakpoints"),
	 N_("Deactivate all breakpoints"),
	 on_disable_all_breakpoints1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("C_lear all Breakpoints"),
	 N_("Delete all breakpoints"),
	 on_clear_breakpoints1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CLOSE,
	 0, 0, NULL},
	GNOMEUIINFO_END
};

static GnomeUIInfo info1_submenu_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("Info _Target Files"),
	 N_("Display information on the files the debugger is active with"),
	 on_info_targets_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Info _Program"),
	 N_("Display information on the execution status of the program"),
	 on_info_program_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Info _Kernel User Struct"),
	 N_("Display the contents of kernel 'struct user' for current child"),
	 on_info_udot_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Info _Threads"),
	 N_("Display the IDs of currently known threads"),
	 on_info_threads_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Info _Global variables"),
	 N_("Display all global and static variables of the program"),
	 on_info_variables_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Info _Local variables"),
	 N_("Display local variables of the current frame"),
	 on_info_locals_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Info _Current Frame"),
	 N_("Display information about the current frame of execution"),
	 on_info_frame_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BOOK_BLUE,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Info Function _Arguments"),
	 N_("Display function arguments of the current frame"),
	 on_info_args_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_END
};

static GnomeUIInfo debug1_menu_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("_Start Debugger"),
	 N_("Start the debugging session"),
	 on_debugger_start_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_EXEC,
	 GDK_F12, GDK_CONTROL_MASK, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Load E_xecutable ..."),
	 N_("Open the executable for debugging"),
	 on_debugger_open_exec_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_OPEN,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Load _Core file ..."),
	 N_("Load a core file to dissect"),
	 on_debugger_load_core_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_REVERT,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("_Attach to Process ..."),
	 N_("Attach to a running program"),
	 on_debugger_attach_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ATTACH,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("_Restart Program"),
	 N_("Stop and restart the program"),
	 on_debugger_restart_prog_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_REFRESH,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("S_top Program"),
	 N_("Stop the program being debugged"),
	 on_debugger_stop_prog_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_STOP,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("_Detach Debugger"),
	 N_("Detach from an attached program"),
	 on_debugger_detach_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("I_nterrupt Program"),
	 N_("Interrupt execution of the program"),
	 on_debugger_interrupt_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_I, GDK_CONTROL_MASK, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Si_gnal to Process"),
	 N_("Send a kernel signal to the process being debugged"),
	 on_debugger_signal_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_SUBTREE, N_("_Execution"),
	 NULL,
	 execution1_submenu_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_SUBTREE, N_("_Breakpoints"),
	 NULL,
	 breakpoints1_submenu_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_SUBTREE, N_("_Information"),
	 NULL,
	 info1_submenu_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Ins_pect/Evaluate ..."),
	 N_("Inspect or evaluate an expression or variable"),
	 on_debugger_inspect_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BOOK_OPEN,
	 GDK_P, GDK_CONTROL_MASK, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Add Expression in _Watch ..."),
	 N_("Add expression or variable to the watch"),
	 on_debugger_add_watch_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 GDK_W, GDK_CONTROL_MASK, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("St_op Debugger"),
	 N_("Say goodbye to the debugger"),
	 on_debugger_stop_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_STOP,
	 0, 0, NULL},
	GNOMEUIINFO_END
};

static GnomeUIInfo plugins_menu_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("plugins"),
	 NULL,
	 on_nonimplemented_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_END
};

static GnomeUIInfo utilities1_menu_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("Grep Utility"),
	 NULL,
	 on_grep_utility1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Compare two files"),
	 NULL,
	 on_compare_two_files1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Diff Utility"),
	 NULL,
	 on_diff_utility1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("File View (Char/Octal/Hex)"),
	 NULL,
	 on_file_view__char_octal_hex_1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Declarations"),
	 NULL,
	 on_c_beautifier1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("C Flow"),
	 NULL,
	 on_c_flow1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("C Cross-reference"),
	 NULL,
	 on_c_cross_reference1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("C Trace"),
	 NULL,
	 on_c_trace1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Backup Manager"),
	 NULL,
	 on_archive_maintenace1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_END
};

static GnomeUIInfo windows1_menu_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("_Create new window"),
	 N_("Open a new file window"),
	 on_windows1_new_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_NEW,
	 0, 0, NULL},
/*
 Do not provide any gettext macro with "Cl_ose Current Window"
 given below. Otherwise, I won't be able to find it.
*/
	{
	 GNOME_APP_UI_ITEM, "Cl_ose current window",
	 N_("Close current document window"),
	 on_windows1_close_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CLOSE,
	 0, 0, NULL},
	GNOMEUIINFO_END
};

static GnomeUIInfo settings1_menu_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("_Compiler and Linker Settings ..."),
	 N_("Settings for the compiler and linker"),
	 on_set_compiler1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PREF,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("_Source Paths ..."),
	 N_("Specify the source paths for Anjuta to search"),
	 on_set_src_paths1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PREF,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("_Commands ..."),
	 N_("Specify the various commands for use"),
	 on_set_commands1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PREF,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("_Preferences ..."),
	 N_("Do you prefer coffee to tea? Check it out."),
	 on_set_preferences1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PROP,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Set _Default Preferences"),
	 N_("But I prefer tea."),
	 on_set_default_preferences1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PROP,
	 0, 0, NULL},
	GNOMEUIINFO_END,
};

static GnomeUIInfo help1_menu_uiinfo[] = {
	GNOMEUIINFO_HELP ("anjuta"),
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Browse GNOME API Pages"),
	 N_("The GNOME API pages"),
	 on_gnome_pages1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BOOK_RED,
	 GDK_F1, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Browse Man Pages"),
	 N_("The good old manual pages"),
	 on_man_pages1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BOOK_RED,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Browse Info Pages"),
	 N_("Info pages, where you cannot find what you are looking for"),
	 on_info_pages1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BOOK_RED,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Context Help"),
	 N_("Search help for the current word in the editor"),
	 on_context_help_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SEARCH,
 	 GDK_H, GDK_CONTROL_MASK, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Search a topic"),
	 N_("May I help you?"),
	 on_search_a_topic1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SEARCH,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Anjuta Home Page"),
	 N_("Online documentation and resources"),
	 on_url_activate, "http://anjuta.sourceforge.net", NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Libraries API references"),
	 N_("Online reference library for GDK, GLib, GNOME etc.."),
	 on_url_activate, "http://lidn.sourceforge.net", NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Report Bugs"),
	 N_("Submit a bug report for Anjuta"),
	 on_url_activate, "http://sourceforge.net/tracker/?atid=114222&group_id=14222&func=browse", NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Request Features"),
	 N_("Submit a feature request for Anjuta"),
	 on_url_activate, "http://sourceforge.net/tracker/?atid=364222&group_id=14222&func=browse", NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Submit patches"),
	 N_("Submit patches for Anjuta"),
	 on_url_activate, "http://sourceforge.net/tracker/?atid=314222&group_id=14222&func=browse", NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Ask a question"),
	 N_("Submit a question for FAQs"),
	 on_url_activate, "mailto:anjuta-list@lists.sourceforge.net", NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_ABOUT_ITEM (on_about1_activate, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo menubar1_uiinfo[] = {
	GNOMEUIINFO_MENU_FILE_TREE (file1_menu_uiinfo),
	GNOMEUIINFO_MENU_EDIT_TREE (edit1_menu_uiinfo),
	GNOMEUIINFO_MENU_VIEW_TREE (view1_menu_uiinfo),
	{
	 GNOME_APP_UI_SUBTREE, N_("_Project"),
	 NULL,
	 project1_menu_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_SUBTREE, N_("For_mat"),
	 NULL,
	 format1_menu_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_SUBTREE, N_("_Build"),
	 NULL,
	 build1_menu_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_SUBTREE, N_("Bookmar_k"),
	 NULL,
	 bookmark1_menu_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_SUBTREE, N_("_Debug"),
	 NULL,
	 debug1_menu_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_SUBTREE, N_("Plug_ins"),
	 NULL,
	 plugins_menu_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_SUBTREE, N_("_Utilities"),
	 NULL,
	 utilities1_menu_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
/*
 Do not provide any gettext macro with "_Windows"
 given below. Otherwise, I won't be able to find it.
*/
	{
	 GNOME_APP_UI_SUBTREE, "_Windows",
	 NULL,
	 windows1_menu_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_SUBTREE, N_("_Settings"),
	 NULL,
	 settings1_menu_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_MENU_HELP_TREE (help1_menu_uiinfo),
	GNOMEUIINFO_END
};

#endif
