/* 
    main_menubar.h
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
#ifndef _MAIN_MENUBAR_H_
#define _MAIN_MENUBAR_H_

typedef struct _FileSubMenu FileSubMenu;
typedef struct _EditSubMenu EditSubMenu;
typedef struct _ViewSubMenu ViewSubMenu;
typedef struct _ProjectSubMenu ProjectSubMenu;
typedef struct _FormatSubMenu FormatSubMenu;
typedef struct _BuildSubMenu BuildSubMenu;
typedef struct _BookmarkSubMenu BookmarkSubMenu;
typedef struct _DebugSubMenu DebugSubMenu;
typedef struct _UtilitiesSubMenu UtilitiesSubMenu;
typedef struct _WindowsSubMenu WindowsSubMenu;
typedef struct _SettingsSubMenu SettingsSubMenu;
typedef struct _HelpSubMenu HelpSubMenu;
typedef struct _MainMenuBar MainMenuBar;

struct _FileSubMenu
{
	GtkWidget *new_file;
	GtkWidget *open_file;
	GtkWidget *save_file;
	GtkWidget *save_as_file;
	GtkWidget *save_all_file;
	GtkWidget *close_file;
	GtkWidget *reload_file;
	GtkWidget *new_project;
	GtkWidget *open_project;
	GtkWidget *save_project;
	GtkWidget *close_project;
	GtkWidget *rename;
	GtkWidget *page_setup;
	GtkWidget *print;
	GtkWidget *recent_files;
	GtkWidget *recent_projects;
	GtkWidget *exit;
};

struct _EditSubMenu
{
	GtkWidget *undo;
	GtkWidget *redo;
	GtkWidget *cut;
	GtkWidget *copy;
	GtkWidget *paste;
	GtkWidget *clear;
	GtkWidget *uppercase;
	GtkWidget *lowercase;
	GtkWidget *convert;

	GtkWidget *select_all;
	GtkWidget *select_brace;
	GtkWidget *select_block;

	GtkWidget *autocomplete;
	GtkWidget *calltip;

	GtkWidget *find;
	GtkWidget *find_in_files;
	GtkWidget *find_replace;

	GtkWidget *goto_line;
	GtkWidget *goto_brace;
	GtkWidget *goto_block_start;
	GtkWidget *goto_block_end;
	GtkWidget *goto_prev_mesg;
	GtkWidget *goto_next_mesg;

	GtkWidget *edit_app_gui;
};

struct _ViewSubMenu
{
	GtkWidget *main_toolbar;
	GtkWidget *extended_toolbar;
	GtkWidget *browser_toolbar;
	GtkWidget *debug_toolbar;
	GtkWidget *tags_toolbar;
	GtkWidget *format_toolbar;

	GtkWidget *editor_linenos;
	GtkWidget *editor_markers;
	GtkWidget *editor_folds;
	GtkWidget *editor_indentguides;
	GtkWidget *editor_whitespaces;
	GtkWidget *editor_eolchars;

	GtkWidget *messages;
	GtkWidget *project_listing;
	GtkWidget *bookmarks;
	GtkWidget *breakpoints;
	GtkWidget *variable_watch;
	GtkWidget *registers;
	GtkWidget *program_stack;
	GtkWidget *shared_lib;
	GtkWidget *signals;
	GtkWidget *dump_window;
	GtkWidget *console;
};

struct _ProjectSubMenu
{
	GtkWidget *new_file;
	GtkWidget *add_inc;
	GtkWidget *add_src;
	GtkWidget *add_hlp;
	GtkWidget *add_data;
	GtkWidget *add_pix;
	GtkWidget *add_po;
	GtkWidget *add_doc;
	GtkWidget *remove;
	GtkWidget *readme;
	GtkWidget *todo;
	GtkWidget *changelog;
	GtkWidget *news;
	GtkWidget *configure;
	GtkWidget *info;
};

struct _FormatSubMenu
{
	GtkWidget *indent;
	GtkWidget *indent_inc;
	GtkWidget *indent_dcr;
	GtkWidget *force_hilite;
	GtkWidget *update_tags;
	GtkWidget *open_folds;
	GtkWidget *close_folds;
	GtkWidget *toggle_fold;
	GtkWidget *detach;
};

struct _BuildSubMenu
{
	GtkWidget *compile;
	GtkWidget *build;
	GtkWidget *build_all;
	GtkWidget *install;
	GtkWidget *autogen;
	GtkWidget *configure;
	GtkWidget *build_dist;
	GtkWidget *clean;
	GtkWidget *clean_all;
	GtkWidget *stop_build;
	GtkWidget *execute;
	GtkWidget *execute_params;
};

struct _BookmarkSubMenu
{
	GtkWidget *toggle;
	GtkWidget *first;
	GtkWidget *prev;
	GtkWidget *next;
	GtkWidget *last;
	GtkWidget *clear;
};

struct _DebugSubMenu
{
	GtkWidget *start_debug;
	GtkWidget *open_exec;
	GtkWidget *load_core;
	GtkWidget *attach;
	GtkWidget *restart;
	GtkWidget *stop_prog;
	GtkWidget *detach;
	GtkWidget *interrupt;
	GtkWidget *send_signal;
	GtkWidget *cont;
	GtkWidget *step_in;
	GtkWidget *step_out;
	GtkWidget *step_over;
	GtkWidget *run_to_cursor;
	GtkWidget *tog_break;
	GtkWidget *set_break;
	GtkWidget *show_breakpoints;
	GtkWidget *disable_all_breakpoints;
	GtkWidget *clear_all_breakpoints;
	GtkWidget *add_watch;
	GtkWidget *inspect;
	GtkWidget *stop;

	GtkWidget *info_targets;
	GtkWidget *info_program;
	GtkWidget *info_udot;
	GtkWidget *info_threads;
	GtkWidget *info_variables;
	GtkWidget *info_locals;
	GtkWidget *info_frame;
	GtkWidget *info_args;
};

struct _UtilitiesSubMenu
{
	GtkWidget *grep;
	GtkWidget *compare;
	GtkWidget *diff;
	GtkWidget *view;
	GtkWidget *indent;
	GtkWidget *flow;
	GtkWidget *cross_ref;
	GtkWidget *trace;
	GtkWidget *archive;
};

struct _WindowsSubMenu
{
	GtkWidget *new;
	GtkWidget *close;
};

struct _SettingsSubMenu
{
	GtkWidget *compiler;
	GtkWidget *src_paths;
	GtkWidget *commands;
	GtkWidget *preferences;
	GtkWidget *default_preferences;
};

struct _HelpSubMenu
{
	GtkWidget *contents;
	GtkWidget *index;
	GtkWidget *man;
	GtkWidget *info;
	GtkWidget *search;
	GtkWidget *about;
};

struct _MainMenuBar
{
	FileSubMenu file;
	EditSubMenu edit;
	ViewSubMenu view;
	ProjectSubMenu project;
	FormatSubMenu format;
	BuildSubMenu build;
	BookmarkSubMenu bookmark;
	DebugSubMenu debug;
	UtilitiesSubMenu utilities;
	WindowsSubMenu windows;
	SettingsSubMenu settings;
	HelpSubMenu help;
};

void create_main_menubar (GtkWidget * app, MainMenuBar * mb);

GtkWidget *create_submenu (gchar * title, GList * strings,
			   GtkSignalFunc callback_func);


void main_menu_install_hints (void);

#endif
