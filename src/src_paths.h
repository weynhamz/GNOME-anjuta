/*
    src_paths.h
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
#ifndef _SRC_PATHS_H_
#define _SRC_PATHS_H_

#include "properties.h"

typedef struct _SrcPaths SrcPaths;
typedef struct _SrcPathsGui SrcPathsGui;

struct _SrcPathsGui
{
	GtkWidget *window;

	GtkWidget *src_clist;
	GtkWidget *src_entry;
	GtkWidget *src_add_b;
	GtkWidget *src_update_b;
	GtkWidget *src_remove_b;
	GtkWidget *src_clear_b;
};

struct _SrcPaths
{
	SrcPathsGui widgets;
	gint src_index;

	gboolean is_showing;
	gint win_pos_x, win_pos_y;
};

void create_src_paths_gui (SrcPaths *);
SrcPaths *src_paths_new (void);
void src_paths_destroy (SrcPaths *);
void src_paths_get (SrcPaths *);
void src_paths_show (SrcPaths *);
void src_paths_hide (SrcPaths *);

gboolean src_paths_save (SrcPaths * co, FILE * s);
gboolean src_paths_load (SrcPaths * co, PropsID props);
gboolean src_paths_save_yourself (SrcPaths * co, FILE * s);
gboolean src_paths_load_yourself (SrcPaths * co, PropsID props);
void src_paths_update_controls (SrcPaths *);

#endif
