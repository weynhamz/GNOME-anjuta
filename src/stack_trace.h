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
  gint current_frame;
  gint current_index;
  GtkTreeIter *current_frame_iter;
  GtkTreeIter *current_index_iter;
};

enum {
	STACK_TRACE_ACTIVE_COLUMN,
	STACK_TRACE_COUNT_COLUMN,
	STACK_TRACE_FRAME_COLUMN,
	STACK_TRACE_N_COLUMNS
};


StackTrace *stack_trace_new (void);

void create_stack_trace_gui (StackTrace * st);

void stack_trace_clear (StackTrace * st);

void stack_trace_update (GList * lines, gpointer st);

void stack_trace_update_controls (StackTrace * st);

void stack_trace_destroy (StackTrace * st);

#endif
