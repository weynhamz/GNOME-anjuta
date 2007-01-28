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

#include <libanjuta/interfaces/ianjuta-message-view.h>
#include <libanjuta/interfaces/ianjuta-debugger.h>
#include <libanjuta/interfaces/ianjuta-cpu-debugger.h>
#include <libanjuta/interfaces/ianjuta-variable-debugger.h>

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
	DEBUGGER_BREAKPOINT,
} DebuggerBreakpointType;

typedef void (*DebuggerParserFunc) (Debugger *debugger,
									const GDBMIValue *mi_result,
									const GList *cli_result,
									GError* error);

struct _DebuggerCommand
{
	gchar cmd[DEBUGGER_COMMAND_MAX_LENGTH];
	gboolean suppress_error;
	gboolean keep_result;
	DebuggerParserFunc parser;
	IAnjutaDebuggerCallback callback;
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
};

GType debugger_get_type (void);

Debugger* debugger_new (GtkWindow *parent_win, GObject* instance);
void debugger_free (Debugger *debugger);

gboolean debugger_start (Debugger *debugger, const GList *search_dirs,
							const gchar *prog, gboolean is_libtool_prog, gboolean terminal);

gboolean debugger_stop (Debugger *debugger);
gboolean debugger_abort (Debugger *debugger);

void debugger_set_output_callback (Debugger *debugger, IAnjutaDebuggerOutputCallback callback, gpointer user_data);

/* Status */
gboolean debugger_is_ready (Debugger *debugger);
gboolean debugger_program_is_running (Debugger *debugger);
gboolean debugger_program_is_attached (Debugger *debugger);
gboolean debugger_program_is_loaded (Debugger *debugger);
IAnjutaDebuggerStatus debugger_get_status (Debugger *debugger);

/* Send standard gdb MI2 or CLI commands */
void debugger_command (Debugger *debugger, const gchar *command,
					   gboolean suppress_error, DebuggerParserFunc parser,
					   gpointer user_data);

void debugger_change_location (Debugger *debugger, const gchar *file,
							   gint line, const gchar *address);
gchar* debugger_get_source_path (Debugger *debugger, const gchar *file);

/* Program loading */
void debugger_load_executable (Debugger *debugger, const gchar *prog);
void debugger_load_core (Debugger *debugger, const gchar *core);
void debugger_attach_process (Debugger *debugger, pid_t pid);
void debugger_detach_process (Debugger *debugger);

/* Execution */
void debugger_start_program (Debugger *debugger, const gchar* args);
void debugger_stop_program (Debugger *debugger);
void debugger_restart_program (Debugger *debugger);
void debugger_interrupt (Debugger *debugger);
void debugger_run (Debugger *debugger);
void debugger_step_in (Debugger *debugger);
void debugger_step_over (Debugger *debugger);
void debugger_step_out (Debugger *debugger);
void debugger_run_to_location (Debugger *debugger, const gchar *loc);
void debugger_run_to_position (Debugger *debugger, const gchar *file, guint line);

/* Breakpoint */
void debugger_add_breakpoint_at_line (Debugger *debugger, const gchar* file, guint line, IAnjutaDebuggerCallback callback, gpointer user_data);
void debugger_add_breakpoint_at_function (Debugger *debugger, const gchar* file, const gchar* function, IAnjutaDebuggerCallback callback, gpointer user_data);
void debugger_add_breakpoint_at_address (Debugger *debugger, guint address, IAnjutaDebuggerCallback callback, gpointer user_data);
void debugger_remove_breakpoint (Debugger *debugger, guint id, IAnjutaDebuggerCallback callback, gpointer user_data);
void debugger_enable_breakpoint (Debugger *debugger, guint id, gboolean enable, IAnjutaDebuggerCallback callback, gpointer user_data);
void debugger_ignore_breakpoint (Debugger *debugger, guint id, guint ignore, IAnjutaDebuggerCallback callback, gpointer user_data);
void debugger_condition_breakpoint (Debugger *debugger, guint id, const gchar* condition, IAnjutaDebuggerCallback callback, gpointer user_data);

/* Variable */
void debugger_print (Debugger *debugger, const gchar* variable, IAnjutaDebuggerCallback callback, gpointer user_data);
void debugger_evaluate (Debugger *debugger, const gchar* name, IAnjutaDebuggerCallback callback, gpointer user_data);

/* Info */
void debugger_list_local (Debugger *debugger, IAnjutaDebuggerCallback func, gpointer user_data);
void debugger_list_argument (Debugger *debugger, IAnjutaDebuggerCallback func, gpointer user_data);
void debugger_info_signal (Debugger *debugger, IAnjutaDebuggerCallback func, gpointer user_data);
void debugger_info_frame (Debugger *debugger, guint frame, IAnjutaDebuggerCallback func, gpointer user_data);
void debugger_info_sharedlib (Debugger *debugger, IAnjutaDebuggerCallback func, gpointer user_data);
void debugger_info_args (Debugger *debugger, IAnjutaDebuggerCallback func, gpointer user_data);
void debugger_info_target (Debugger *debugger, IAnjutaDebuggerCallback func, gpointer user_data);
void debugger_info_program (Debugger *debugger, IAnjutaDebuggerCallback func, gpointer user_data);
void debugger_info_udot (Debugger *debugger, IAnjutaDebuggerCallback func, gpointer user_data);
void debugger_info_threads (Debugger *debugger, IAnjutaDebuggerCallback func, gpointer user_data);
void debugger_info_variables (Debugger *debugger, IAnjutaDebuggerCallback func, gpointer user_data);
void debugger_inspect_memory (Debugger *debugger, guint address, guint length, IAnjutaDebuggerCallback func, gpointer user_data);
void debugger_disassemble (Debugger *debugger, guint address, guint length, IAnjutaDebuggerCallback func, gpointer user_data);

/* Register */

void debugger_list_register (Debugger *debugger, IAnjutaDebuggerCallback func, gpointer user_data);
void debugger_update_register (Debugger *debugger, IAnjutaDebuggerCallback func, gpointer user_data);
void debugger_write_register (Debugger *debugger, const gchar *name, const gchar *value);

/* Stack */
void debugger_list_argument (Debugger *debugger, IAnjutaDebuggerCallback func, gpointer user_data);
void debugger_list_frame (Debugger *debugger, IAnjutaDebuggerCallback func, gpointer user_data);
void debugger_set_frame (Debugger *debugger, guint frame);

/* Log */
void debugger_set_log (Debugger *debugger, IAnjutaMessageView *view);

/* Variable object */
void debugger_delete_variable (Debugger *debugger, const gchar *name);
void debugger_evaluate_variable (Debugger *debugger, const gchar *name, IAnjutaDebuggerCallback callback, gpointer user_data);
void debugger_assign_variable (Debugger *debugger, const gchar *name, const gchar *value);
void debugger_list_variable_children (Debugger *debugger, const gchar* name, IAnjutaDebuggerCallback callback, gpointer user_data);
void debugger_create_variable (Debugger *debugger, const gchar *name, IAnjutaDebuggerCallback callback, gpointer user_data);
void debugger_update_variable (Debugger *debugger, IAnjutaDebuggerCallback callback, gpointer user_data);

#if 0

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
