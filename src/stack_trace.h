/*
    stack_trace.h
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

#ifndef _STACK_TRACE_H_
#define _STACK_TRACE_H_

#include <gnome.h>
#include "properties.h"

typedef struct _StackTraceGui StackTraceGui;
typedef struct _StackTrace StackTrace;

struct _StackTraceGui
{
  GtkWidget *window;
  GtkWidget *clist;
  GtkWidget *menu;
  GtkWidget *menu_set;
  GtkWidget *menu_info;
  GtkWidget *menu_update;
  GtkWidget *menu_view;
};

struct _StackTrace
{
  StackTraceGui widgets;
  gint current_index;
  gint current_frame;
  gboolean is_showing;
  gint win_pos_x, win_pos_y, win_width, win_height;
};

StackTrace *stack_trace_new (void);

void create_stack_trace_gui (StackTrace * st);

void stack_trace_clear (StackTrace * st);

void stack_trace_update (GList * lines, gpointer st);

void add_frame (StackTrace * st, gchar *);

void stack_trace_update_controls (StackTrace * st);

void stack_trace_destroy (StackTrace * st);

gboolean stack_trace_save_yourself (StackTrace * st, FILE * stream);

gboolean stack_trace_load_yourself (StackTrace * st, PropsID props);

void stack_trace_show (StackTrace * st);

void stack_trace_hide (StackTrace * st);

#endif
