/* 
    toolbar.h
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
#ifndef _TOOLBAR_H_
#define _TOOLBAR_H_

#include <gnome.h>

typedef struct _MainToolbar MainToolbar;
typedef struct _ExtendedToolbar ExtendedToolbar;
typedef struct _DebugToolbar DebugToolbar;
typedef struct _BrowserToolbar BrowserToolbar;
typedef struct _FormatToolbar FormatToolbar;
typedef struct _Toolbar Toolbar;

struct _MainToolbar
{
	GtkWidget *toolbar;

	GtkWidget *new;
	GtkWidget *open;

	GtkWidget *save;
	GtkWidget *save_all;
	GtkWidget *close;
	GtkWidget *reload;

	GtkWidget *undo;
	GtkWidget *redo;

	GtkWidget *led;

	GtkWidget *print;
	GtkWidget *detach;

	GtkWidget *find;
	GtkWidget *find_combo;
	GtkWidget *find_entry;
	GtkWidget *go_to;
	GtkWidget *line_entry;

	GtkWidget *project;
	GtkWidget *messages;

	GtkWidget *help;
};

struct _ExtendedToolbar
{
	GtkWidget *toolbar;

	GtkWidget *open_project;
	GtkWidget *save_project;
	GtkWidget *close_project;

	GtkWidget *compile;
	GtkWidget *configure;
	GtkWidget *build;
	GtkWidget *build_all;
	GtkWidget *exec;
	GtkWidget *debug;
	GtkWidget *stop;

	GtkWidget *find;
	GtkWidget *replace;
	GtkWidget *find_in_files;
	GtkWidget *reference;
	GtkWidget *trace;
	GtkWidget *view;
};

struct _DebugToolbar
{
	GtkWidget *toolbar;

	GtkWidget *start;
	GtkWidget *interrupt;
	GtkWidget *stop;
	GtkWidget *go;
	GtkWidget *run_to_cursor;
	GtkWidget *step_in;
	GtkWidget *step_out;
	GtkWidget *step_over;
	GtkWidget *toggle_bp;
	GtkWidget *breakpoints;
	GtkWidget *watch;
	GtkWidget *wachpoints;
	GtkWidget *registers;
	GtkWidget *stack;
	GtkWidget *inspect;
	GtkWidget *frame;
};

struct _BrowserToolbar
{
	GtkWidget *toolbar;

	GtkWidget *toggle_bookmark;
	GtkWidget *prev_bookmark;
	GtkWidget *next_bookmark;
	GtkWidget *first_bookmark;
	GtkWidget *last_bookmark;

	GtkWidget *prev_error;
	GtkWidget *next_error;
	GtkWidget *block_start;
	GtkWidget *block_end;
	
	GtkWidget *tag_label;
	GtkWidget *tag;
	GtkWidget *tag_combo;
	GtkWidget *tag_entry;
};

struct _FormatToolbar
{
	GtkWidget *toolbar;

	GtkWidget *toggle_fold;
	GtkWidget *open_all_folds;
	GtkWidget *close_all_folds;

	GtkWidget *block_select;
	GtkWidget *indent_increase;
	GtkWidget *indent_decrease;

	GtkWidget *autoformat;
	GtkWidget *set_autoformat_style;

	GtkWidget *calltip;
	GtkWidget *autocomplete;
};

struct _Toolbar
{
	MainToolbar main_toolbar;
	ExtendedToolbar extended_toolbar;
	DebugToolbar debug_toolbar;
	BrowserToolbar browser_toolbar;
	FormatToolbar format_toolbar;
};

GtkWidget *create_main_toolbar (GtkWidget * window,
				MainToolbar * main_toolbar);

GtkWidget *create_extended_toolbar (GtkWidget * window,
				    ExtendedToolbar * extended_toolbar);

GtkWidget *create_browser_toolbar (GtkWidget * window,
				   BrowserToolbar * wizard_toolbar);

GtkWidget *create_debug_toolbar (GtkWidget * window,
				 DebugToolbar * debug_toolbar);

GtkWidget *create_format_toolbar (GtkWidget * window,
				  FormatToolbar * format_toolbar);

#endif
