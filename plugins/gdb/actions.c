/* 
 * actions.c Copyright (C) 2000-2005 Kh. Naba Kumar Singh
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

#include "debugger.h"
#include "watch_gui.h"
#include "watch_cbs.h"
#include "memory.h"
#include "signals_cbs.h"
#include "actions.h"

void
on_toggle_breakpoint1_activate (GtkAction * action, gpointer user_data)
{
	breakpoints_dbase_toggle_breakpoint(debugger.breakpoints_dbase, 0);
}

void
on_set_breakpoint1_activate (GtkAction * action, gpointer user_data)
{
	breakpoints_dbase_add (debugger.breakpoints_dbase);
}

void
on_disable_all_breakpoints1_activate (GtkAction * action,
				      gpointer user_data)
{
	breakpoints_dbase_disable_all (debugger.breakpoints_dbase);
}

void
on_show_breakpoints1_activate (GtkAction * action, gpointer user_data)
{
	breakpoints_dbase_show (debugger.breakpoints_dbase);
}

void
on_clear_breakpoints1_activate (GtkAction * action, gpointer user_data)
{
	breakpoints_dbase_remove_all (debugger.breakpoints_dbase);
}

/*******************************************************************************/
void
on_execution_continue1_activate (GtkAction * action, gpointer user_data)
{
	debugger_run ();
}

void
on_execution_step_in1_activate (GtkAction * action, gpointer user_data)
{
	debugger_step_in ();
}

void
on_execution_step_out1_activate (GtkAction * action, gpointer user_data)
{
	debugger_step_out ();
}

void
on_execution_step_over1_activate (GtkAction * action, gpointer user_data)
{
	debugger_step_over ();
}

#if 0
void
on_execution_run_to_cursor1_activate (GtkAction * action,
				      gpointer user_data)
{
	guint line;
	gchar *buff;
	TextEditor* te;

	te = anjuta_get_current_text_editor();
	g_return_if_fail (te != NULL);
	g_return_if_fail (te->full_filename != NULL);
	if (debugger_is_active()==FALSE) return;
	if (debugger_is_ready()==FALSE) return;

	line = text_editor_get_current_lineno (te);

	buff = g_strdup_printf ("%s:%d", te->filename, line);
	debugger_run_to_location (buff);
	g_free (buff);
}
#endif

/*******************************************************************************/
void
on_info_targets_activate (GtkAction * action, gpointer user_data)
{
	debugger_query_info_target (debugger_dialog_message);
	debugger_query_execute ();
}

void
on_info_program_activate (GtkAction * action, gpointer user_data)
{
	debugger_query_info_program (debugger_dialog_message);
	debugger_query_execute ();
}

void
on_info_udot_activate (GtkAction * action, gpointer user_data)
{
	debugger_query_info_udot (debugger_dialog_message);
	debugger_query_execute ();
}

void
on_info_threads_activate (GtkAction * action, gpointer user_data)
{
	debugger_query_info_threads (debugger_dialog_message);
	debugger_query_execute ();
}

void
on_info_variables_activate (GtkAction * action, gpointer user_data)
{
	debugger_query_info_variables (debugger_dialog_message);
	debugger_query_execute ();
}

void
on_info_locals_activate (GtkAction * action, gpointer user_data)
{
	debugger_query_info_locals (debugger_dialog_message);
	debugger_query_execute ();
}

void
on_info_frame_activate (GtkAction * action, gpointer user_data)
{
	debugger_query_info_frame (debugger_dialog_message);
	debugger_query_execute ();
}

void
on_info_args_activate (GtkAction * action, gpointer user_data)
{
	debugger_query_info_args (debugger_dialog_message);
	debugger_query_execute ();
}

void
on_info_memory_activate (GtkAction * action, gpointer user_data)
{
	GtkWidget *win_memory;

	win_memory = memory_info_new (NULL);
	gtk_widget_show(win_memory);
}


/********************************************************************************/

#if 0
void
on_debugger_start_activate (GtkAction * action, gpointer user_data)
{
	gchar *prog, *temp;
	gint s_re, e_re;
	struct stat s_stat, e_stat;
	TextEditor *te;

	prog = NULL;
	if (app->project_dbase->project_is_open)
	{
		gint target_type;
		target_type = project_dbase_get_target_type (app->project_dbase);
		if (target_type >= PROJECT_TARGET_TYPE_END_MARK)
			anjuta_error (_("The target executable of this Project is unknown"));
		else if ( target_type != PROJECT_TARGET_TYPE_EXECUTABLE)
			anjuta_warning (_("The target executable of this Project is not executable"));
		prog = project_dbase_get_source_target (app->project_dbase);
		if (file_is_executable (prog) == FALSE)
		{
			anjuta_warning(_("The target executable does not exist for this Project"));
			g_free (prog);
			prog = NULL;
		}
	}
	else
	{
		te = anjuta_get_current_text_editor ();
		if (te)
		{
			if (te->full_filename)
			{
				prog = g_strdup (te->full_filename);
				temp = get_file_extension (prog);
				if (temp)
					*(--temp) = '\0';
				s_re = stat (te->full_filename, &s_stat);
				e_re = stat (prog, &e_stat);
				if ((e_re != 0) || (s_re != 0))
				{
					anjuta_warning(_("No executable for this file."));
					g_free (prog);
					prog = NULL;
				}
				else if ((!text_editor_is_saved (te)) || (e_stat.st_mtime < s_stat.st_mtime))
				{
					anjuta_warning (_("The executable is not up-to-date."));
				}
			}
			else
			{
				anjuta_warning(_("No executable for this file."));
			}
		}
	}
	debugger_start (prog);
	if (prog) g_free (prog);
}

void
on_debugger_open_exec_activate (GtkAction * action, gpointer user_data)
{
	debugger_open_exec_file ();
}

void
on_debugger_attach_activate (GtkAction * action, gpointer user_data)
{
	if (debugger_is_active ())
		attach_process_show (debugger.attach_process);
	else
		anjuta_error (_("Debugger is not running. Start it first."));
}

void
on_debugger_load_core_activate (GtkAction * action, gpointer user_data)
{
	debugger_load_core_file ();
}

#endif

void
on_debugger_restart_prog_activate (GtkAction * action, gpointer user_data)
{
	debugger_restart_program ();
}

void
on_debugger_stop_prog_activate (GtkAction * action, gpointer user_data)
{
	debugger_stop_program ();
}

void
on_debugger_detach_activate (GtkAction * action, gpointer user_data)
{
	debugger_detach_process ();
}

void
on_debugger_stop_activate (GtkAction * action, gpointer user_data)
{
	debugger_stop ();
}

void
on_debugger_confirm_stop_yes_clicked (GtkButton * button, gpointer data)
{
	debugger_stop ();
}

void
on_debugger_interrupt_activate (GtkAction * action, gpointer user_data)
{
	debugger_interrupt ();
}

void
on_debugger_signal_activate (GtkAction * action, gpointer user_data)
{
	on_signals_send_activate (NULL, debugger.signals);
}

void
on_debugger_inspect_activate (GtkAction * action, gpointer user_data)
{
	GtkWidget *w = create_eval_dialog (NULL, debugger.watch);
	gtk_widget_show (w);
}

void
on_debugger_add_watch_activate (GtkAction * action, gpointer user_data)
{
	on_watch_add_activate (NULL, debugger.watch);
}

void
on_debugger_custom_command_activate (GtkAction * action,
				     gpointer user_data)
{
	debugger_custom_command ();
}
