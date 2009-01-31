/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _SIGNALS_H_
#define _SIGNALS_H_

#include "plugin.h"
#include "queue.h"

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-debugger.h>

#include <gtk/gtk.h>

#include <glib.h>

typedef struct _SignalsGui SignalsGui;
typedef struct _Signals Signals;

struct _SignalsGui
{
	GtkWidget*   window;
	GtkWidget*   treeview;
	GtkListStore* store;
	GtkActionGroup *action_group_debugger_ok;
	GtkActionGroup *action_group_program_running;

	GtkMenu*   menu;
};

struct _Signals
{
	SignalsGui  widgets;
	DebugManagerPlugin *plugin;
	DmaDebuggerQueue *debugger;
	gboolean is_showing;
	gint win_pos_x, win_pos_y, win_width, win_height;
};

Signals* signals_new (DebugManagerPlugin *plugin);

void signals_clear (Signals *ew);

void signals_free (Signals*ew);

void signals_show (Signals *ew);

void signals_hide (Signals *ew);

#endif
