/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    sharedlibs.h
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

#ifndef _SHAREDLIBS_H_
#define _SHAREDLIBS_H_

#include "plugin.h"
#include "queue.h"

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-debugger.h>

#include <glib.h>
#include <gtk/gtk.h>

typedef struct _SharedlibsGui SharedlibsGui;
typedef struct _Sharedlibs Sharedlibs;

struct _SharedlibsGui
{
	GtkWidget*   window;
	GtkWidget*   menu;
	GtkActionGroup *action_group;
	GtkWidget* treeview;
	GtkListStore* store;
};

struct _Sharedlibs
{
	SharedlibsGui  widgets;
	DmaDebuggerQueue *debugger;
	gboolean is_showing;
	gint win_pos_x, win_pos_y, win_width, win_height;
};

Sharedlibs*
sharedlibs_new (DebugManagerPlugin *plugin);

void
sharedlibs_clear (Sharedlibs *ew);

void
sharedlibs_free (Sharedlibs*ew);

void
sharedlibs_show (Sharedlibs * ew);

void
sharedlibs_hide (Sharedlibs * ew);

#endif
