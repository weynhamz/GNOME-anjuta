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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _WATCH_H_
#define _WATCH_H_

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-debugger.h>

#include <gtk/gtkwidget.h>
#include <gtk/gtktreemodel.h>

/* TODO #include "properties.h" */

typedef struct _ExprWatchGui ExprWatchGui;
typedef struct _ExprWatch ExprWatch;

enum {
	WATCH_VARIABLE_COLUMN,
	WATCH_VALUE_COLUMN,
	WATCH_N_COLUMNS
};

ExprWatch* expr_watch_new (AnjutaPlugin *plugin, IAnjutaDebugger *debugger);

void expr_watch_connect (ExprWatch *ew, IAnjutaDebugger *debugger);

/*void expr_watch_clear (ExprWatch *ew);

void expr_watch_cmd_queqe (ExprWatch *ew);

void expr_watch_update (Debugger *debugger, const GDBMIValue *mi_result,
						const GList *cli_result, gpointer  ew);

void expr_watch_evaluate_expression (ExprWatch *ew, const gchar *expr,
									 DebuggerResultFunc parser, gpointer data);

void expr_watch_update_controls (ExprWatch *ew);*/

void expr_watch_destroy (ExprWatch*ew);

//void expr_watch_update (struct _DebugPrintCallback* cb_data, const gchar* value);
void expr_watch_cmd_queqe (ExprWatch * ew);


#endif
