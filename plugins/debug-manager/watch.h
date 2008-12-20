/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    watch.h
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

#ifndef _WATCH_H_
#define _WATCH_H_

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-debugger.h>

#include <gtk/gtk.h>

/* TODO #include "properties.h" */

typedef struct _ExprWatchGui ExprWatchGui;
typedef struct _ExprWatch ExprWatch;

enum {
	WATCH_VARIABLE_COLUMN,
	WATCH_VALUE_COLUMN,
	WATCH_N_COLUMNS
};

ExprWatch* expr_watch_new (AnjutaPlugin *plugin);
void expr_watch_destroy (ExprWatch *ew);

gchar* expr_watch_find_variable_value (ExprWatch *ew, const gchar *name);

#endif
