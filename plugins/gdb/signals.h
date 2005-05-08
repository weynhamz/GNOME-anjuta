/*
    signals.h
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

#ifndef _SIGNALS_H_
#define _SIGNALS_H_

#include <glib.h>
#include <gtk/gtkwidget.h>
#include "debugger.h"

typedef struct _SignalsGui SignalsGui;
typedef struct _Signals Signals;

struct _SignalsGui
{
    GtkWidget*   window;
    GtkWidget*   clist;
    GtkWidget*   menu;
    GtkWidget*   menu_modify;
    GtkWidget*   menu_signal;
    GtkWidget*   menu_update;
};

struct _Signals
{
	SignalsGui  widgets;
	Debugger *debugger;
	gboolean is_showing;
	gint win_pos_x, win_pos_y, win_width, win_height;
	gint	idx;
	gchar	*signal;
	gboolean stop;
	gboolean print;
	gboolean pass;
};

Signals* signals_new (Debugger *debugger);

void signals_clear (Signals *ew);

void signals_destroy (Signals*ew);

void signals_show (Signals *ew);

void signals_hide (Signals *ew);

#endif
