/* 
 * actions.h Copyright (C) 2000-2005 Kh. Naba Kumar Singh
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along with 
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
 * Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _GDB_ACTOINS_H_
#define _GDB_ACTIONS_H_

#include <gtk/gtkaction.h>

void on_toggle_breakpoint1_activate (GtkAction * action,
									 gpointer user_data);
void on_set_breakpoint1_activate (GtkAction * action, gpointer user_data);
void on_disable_all_breakpoints1_activate (GtkAction * action,
										   gpointer user_data);
void on_show_breakpoints1_activate (GtkAction * action, gpointer user_data);
void on_clear_breakpoints1_activate (GtkAction * action,
									 gpointer user_data);

/*****************************************************************************/

void on_execution_continue1_activate (GtkAction * action,
									  gpointer user_data);
void on_execution_step_in1_activate (GtkAction * action,
									 gpointer user_data);
void on_execution_step_out1_activate (GtkAction * action,
									  gpointer user_data);
void on_execution_step_over1_activate (GtkAction * action,
									   gpointer user_data);
void on_execution_run_to_cursor1_activate (GtkAction * action,
										   gpointer user_data);
/*****************************************************************************/

void on_info_targets_activate (GtkAction * action, gpointer user_data);
void on_info_program_activate (GtkAction * action, gpointer user_data);
void on_info_udot_activate (GtkAction * action, gpointer user_data);
void on_info_threads_activate (GtkAction * action, gpointer user_data);
void on_info_variables_activate (GtkAction * action, gpointer user_data);
void on_info_locals_activate (GtkAction * action, gpointer user_data);
void on_info_frame_activate (GtkAction * action, gpointer user_data);
void on_info_args_activate (GtkAction * action, gpointer user_data);
void on_info_memory_activate (GtkAction * action, gpointer user_data);

/*****************************************************************************/
void on_debugger_start_activate (GtkAction * action, gpointer user_data);
void on_debugger_open_exec_activate (GtkAction * action,
									 gpointer user_data);
void on_debugger_attach_activate (GtkAction * action, gpointer user_data);
void on_debugger_load_core_activate (GtkAction * action,
									 gpointer user_data);
void on_debugger_restart_prog_activate (GtkAction * action,
										gpointer user_data);
void on_debugger_stop_prog_activate (GtkAction * action,
									 gpointer user_data);
void on_debugger_detach_activate (GtkAction * action, gpointer user_data);
void on_debugger_stop_activate (GtkAction * action, gpointer user_data);
void on_debugger_confirm_stop_yes_clicked (GtkButton * button,
										   gpointer user_data);
void on_debugger_interrupt_activate (GtkAction * action,
									 gpointer user_data);
void on_debugger_signal_activate (GtkAction * action, gpointer user_data);

void on_debugger_inspect_activate (GtkAction * action, gpointer user_data);
void on_debugger_add_watch_activate (GtkAction * action,
									 gpointer user_data);
void on_debugger_custom_command_activate (GtkAction * action,
										  gpointer user_data);
#endif
