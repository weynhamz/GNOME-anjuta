/*
    debugger.h
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
#ifndef _DEBUGGER_H_
#define _DEBUGGER_H_

#include "global.h"
#include "breakpoints.h"
#include "registers.h"
#include "watch.h"
#include "stack_trace.h"
#include "signals.h"
#include "sharedlib.h"
#include "attach_process.h"
#include "project_dbase.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DB_CMD_NONE			0x0
#define DB_CMD_SO_MESG		0x1
#define DB_CMD_SE_DIALOG	0x2
#define DB_CMD_SE_MESG		0x4
#define DB_CMD_ALL (DB_CMD_SE_MESG|DB_CMD_SE_DIALOG|DB_CMD_SO_MESG)
#define DEBUGGER_COMMAND_MAX_LENGTH  1024

typedef struct _Debugger Debugger;
typedef struct _DebuggerCommand DebuggerCommand;

struct _DebuggerCommand
{
	char cmd[DEBUGGER_COMMAND_MAX_LENGTH];
	gint flags;
	void (*parser) (GList * outputs, gpointer data);
	gpointer data;
};

struct _Debugger
{
	gboolean active;
	gboolean ready;
	gboolean prog_is_running;
	gboolean prog_is_attached;
	gint post_execution_flag;
	pid_t child_pid;

	GtkWidget *open_exec_filesel;
	GtkWidget *load_core_filesel;

	ExprWatch *watch;
	BreakpointsDBase *breakpoints_dbase;
	CpuRegisters *cpu_registers;
	StackTrace *stack;
	Signals *signals;
	Sharedlibs *sharedlibs;
	AttachProcess *attach_process;

	gchar stdo_line[FILE_BUFFER_SIZE];
	gint stdo_cur_char_pos;
	GList *gdb_stdo_outputs;

	gchar stde_line[FILE_BUFFER_SIZE];
	gint stde_cur_char_pos;
	GList *gdb_stde_outputs;

	GList *cmd_queqe;
	DebuggerCommand current_cmd;
	gboolean starting;
	gboolean term_is_running;
	pid_t term_pid;
    gint gnome_terminal_type;
};

extern Debugger debugger;

void debugger_init (void);
void debugger_shutdown (void);
gboolean debugger_save_yourself (FILE * stream);
gboolean debugger_load_yourself (PropsID props);
void debugger_start (const gchar * prog);
gboolean debugger_is_active (void);
gboolean debugger_is_ready (void);

/*  Private. Don't touch */
gchar *debugger_start_terminal (void);
void debugger_put_cmd_in_queqe (const gchar cmd[], gint flags,
				void (*parser) (GList * outputs, gpointer data), gpointer data);
void debugger_execute_cmd_in_queqe (void);
void debugger_update_controls (void);
void debugger_set_active (gboolean busy_state);
void debugger_set_ready (gboolean busy_state);

void debugger_stdo_flush (void);
void debugger_stde_flush (void);
void debugger_dialog_message (GList * lines, gpointer data);
void debugger_dialog_error (GList * lines, gpointer data);

/* Public */
void debugger_open_exec_file (void);
void debugger_load_core_file (void);

void debugger_attach_process (gint pid);
void debugger_restart_program (void);
void debugger_start_program (void);
void debugger_stop_program (void);
void debugger_detach_process (void);
gboolean debugger_stop (void);

void debugger_run (void);
void debugger_step_in (void);
void debugger_step_over (void);
void debugger_step_out (void);
void debugger_continue (void);
void debugger_run_to_location (const gchar * loc);

void debugger_toggle_breakpoint (void);
void debugger_toggle_tmp_breakpoint (void);

void debugger_enable_breakpoint (gint id);
void debugger_enable_all_breakpoints (void);
void debugger_disable_breakpoint (gint id);
void debugger_disable_all_breakpoints (void);
void debugger_delete_breakpoint (gint id);
void debugger_delete_all_breakpoints (void);


void debugger_interrupt (void);
void debugger_frame (void);
void debugger_signal (const gchar * sig, gboolean show_error);/*eg: "SIGTERM"*/
void debugger_custom_command (void);
void debugger_shared_library (void);


void debugger_query_execute (void);
void debugger_query_evaluate_expr_tip (const gchar *expr,
			void (*parser) (GList *outputs, gpointer data), gpointer data);
void debugger_query_evaluate_expr (const gchar *expr,
			void (*parser) (GList *outputs, gpointer data), gpointer data);
void debugger_query_info_target (
			void (*parser) (GList *outputs, gpointer data));
void debugger_query_info_program (
			void (*parser) (GList *outputs, gpointer data));
void debugger_query_info_udot (
			void (*parser) (GList *outputs, gpointer data));
void debugger_query_info_threads (
			void (*parser) (GList *outputs, gpointer data));
void debugger_query_info_variables (
			void (*parser) (GList *outputs, gpointer data));
void debugger_query_info_locals (
			void (*parser) (GList *outputs, gpointer data));
void debugger_query_info_frame (
			void (*parser) (GList *outputs, gpointer data));
void debugger_query_info_args (
			void (*parser) (GList *outputs, gpointer data));


void on_debugger_update_prog_status (GList * lines, gpointer data);
void debugger_reload_session_breakpoints( ProjectDBase *p);
void debugger_save_session_breakpoints( ProjectDBase *p );
gboolean debugger_is_engaged(void);
const gchar* debugger_get_last_frame(void);


#ifdef __cplusplus
}
#endif

#endif
