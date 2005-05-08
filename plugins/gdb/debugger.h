/*
    debugger.h
    Copyright (C) Naba Kumar <naba@gnome.org>

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
#ifndef __DEBUGGER_H__
#define __DEBUGGER_H__

#include <sys/types.h>
#include <glib-object.h>
#include <gtk/gtkwindow.h>
#include "gdbmi.h"

G_BEGIN_DECLS

#define DEBUGGER_COMMAND_MAX_LENGTH  1024

typedef struct _Debugger        Debugger;
typedef struct _DebuggerClass   DebuggerClass;
typedef struct _DebuggerPriv    DebuggerPriv;
typedef struct _DebuggerCommand DebuggerCommand;

#define DEBUGGER_TYPE            (debugger_get_type ())
#define DEBUGGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DEBUGGER_TYPE, Debugger))
#define DEBUGGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DEBUGGER_TYPE, DebuggerClass))
#define IS_DEBUGGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DEBUGGER_TYPE))
#define IS_DEBUGGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DEBUGGER_TYPE))

typedef enum
{
	DEBUGGER_OUTPUT_NORMAL,
	DEBUGGER_OUTPUT_WARNING,
	DEBUGGER_OUTPUT_ERROR,
	DEBUGGER_OUTPUT_INFO
} DebuggerOutputType;

typedef void (*DebuggerResultFunc) (Debugger *debugger,
									const GDBMIValue *mi_result,
									const GList *cli_result,
									gpointer data);
typedef void (*DebuggerOutputFunc) (Debugger *debugger,
									DebuggerOutputType output_type,
									const gchar *output,
									gpointer data);

struct _DebuggerCommand
{
	gchar cmd[DEBUGGER_COMMAND_MAX_LENGTH];
	DebuggerResultFunc parser;
	gboolean suppress_error;
	gpointer user_data;
};

struct _Debugger
{
	GObject parent;
	DebuggerPriv *priv;
};

struct _DebuggerClass
{
	GObjectClass parent_class;
	
	/* Signals */
	void (*program_running_signal)  (Debugger *debugger);
	void (*program_stopped_signal)  (Debugger *debugger,
									 const GDBMIValue *mi_result);
	void (*program_exited_signal)   (Debugger *debugger,
									 const GDBMIValue *mi_result);
	void (*results_arrived_signal)  (Debugger *debugger,
									 const gchar *command,
									 GDBMIValue *mi_result);
	void (*location_changed_signal) (Debugger *debugger, const gchar *file,
									 gint line, const gchar *address);
};

GType debugger_get_type (void);

Debugger* debugger_new (GtkWindow *parent_win,
						DebuggerOutputFunc output_callback,
						gpointer user_data);

Debugger* debugger_new_with_program (GtkWindow *parent_win,
									 DebuggerOutputFunc output_callback,
									 gpointer user_data,
									 const gchar *program_path,
									 gboolean is_libtool_prog);

/* Status */
gboolean debugger_is_ready (Debugger *debugger);
gboolean debugger_program_is_running (Debugger *debugger);
gboolean debugger_program_is_attached (Debugger *debugger);

/* Send standard gdb MI2 or CLI commands */
void debugger_command (Debugger *debugger, const gchar *command,
					   gboolean suppress_error, DebuggerResultFunc parser,
					   gpointer user_data);

void debugger_change_location (Debugger *debugger, const gchar *file,
							   gint line, const gchar *address);

/* Program loading */
void debugger_load_executable (Debugger *debugger, const gchar *prog);
void debugger_load_core (Debugger *debugger, const gchar *core);
void debugger_attach_process (Debugger *debugger, pid_t pid);
void debugger_detach_process (Debugger *debugger);

/* Execution */
void debugger_start_program (Debugger *debugger);
void debugger_stop_program (Debugger *debugger);
void debugger_restart_program (Debugger *debugger);
void debugger_interrupt (Debugger *debugger);
void debugger_run (Debugger *debugger);
void debugger_step_in (Debugger *debugger);
void debugger_step_over (Debugger *debugger);
void debugger_step_out (Debugger *debugger);
void debugger_run_to_location (Debugger *debugger, const gchar *loc);

#if 0

/* Breakpoints */
void debugger_toggle_breakpoint (const char *file, guint l);
void debugger_enable_breakpoint (gint id);
void debugger_enable_all_breakpoints (void);
void debugger_disable_breakpoint (gint id);
void debugger_disable_all_breakpoints (void);
void debugger_delete_breakpoint (gint id);
void debugger_delete_all_breakpoints (void);

// void debugger_frame (void);
// void debugger_signal (const gchar * sig, gboolean show_error);/*eg: "SIGTERM"*/
// void debugger_shared_library (void);

/* Evaluations */
void debugger_query_execute (void);
void debugger_query_evaluate_expr_tip (const gchar *expr,
									   DebuggerFunc parser,
									   gpointer data);
void debugger_query_evaluate_expr (const gchar *expr,
								   DebuggerFunc parser,
								   gpointer data);
#endif

G_END_DECLS

#endif
