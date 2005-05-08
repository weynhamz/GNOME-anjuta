/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
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

#include <config.h>

#include "debugger.h"
#include "info.h"
#include "watch_gui.h"
#include "watch_cbs.h"
#include "memory.h"
#include "registers.h"

#if 0
#include "signals_cbs.h"
#endif
#include "actions.h"

#define GLADE_FILE PACKAGE_DATA_DIR"/glade/anjuta-gdb.glade"

static void
on_debugger_dialog_message (Debugger *debugger, const GDBMIValue *mi_result,
							const GList *cli_result, gpointer user_data)
{
	GtkWindow *parent = GTK_WINDOW (user_data);
	if (g_list_length ((GList*)cli_result) < 1)
		return;
	gdb_info_show_list (parent, (GList*)cli_result, 0, 0);
}

static void
debugger_info_command (Debugger *debugger, const gchar *command,
					   DebuggerResultFunc parser, gpointer data)
{
	debugger_command (debugger, "set print pretty on", TRUE, NULL, NULL);
	debugger_command (debugger, "set verbose off", TRUE, NULL, NULL);
	debugger_command (debugger, command, FALSE, parser, data);
	debugger_command (debugger, "set print pretty off", TRUE, NULL, NULL);
	debugger_command (debugger, "set verbose on", TRUE, NULL, NULL);
}

void
on_toggle_breakpoint_activate (GtkAction * action, GdbPlugin *plugin)
{
	breakpoints_dbase_toggle_breakpoint (plugin->breakpoints, NULL, 0);
}

void
on_set_breakpoint_activate (GtkAction * action, GdbPlugin *plugin)
{
	breakpoints_dbase_add (plugin->breakpoints);
}

void
on_disable_all_breakpoints_activate (GtkAction * action, GdbPlugin *plugin)
{
	breakpoints_dbase_disable_all (plugin->breakpoints);
}

void
on_show_breakpoints_activate (GtkAction * action, GdbPlugin *plugin)
{
	breakpoints_dbase_show (plugin->breakpoints);
}

void
on_clear_breakpoints_activate (GtkAction * action, GdbPlugin *plugin)
{
	breakpoints_dbase_remove_all (plugin->breakpoints);
}

/*****************************************************************************/
void
on_info_targets_activate (GtkAction *action, GdbPlugin *plugin)
{
	debugger_info_command (plugin->debugger, "info target",
						   on_debugger_dialog_message, plugin);
}

void
on_info_program_activate (GtkAction *action, GdbPlugin *plugin)
{
	debugger_info_command (plugin->debugger, "info program",
						   on_debugger_dialog_message, plugin);
}

void
on_info_udot_activate (GtkAction *action, GdbPlugin *plugin)
{
	debugger_info_command (plugin->debugger, "info udot",
						   on_debugger_dialog_message, plugin);
}

void
on_info_threads_activate (GtkAction *action, GdbPlugin *plugin)
{
	debugger_info_command (plugin->debugger, "info threads",
						   on_debugger_dialog_message, plugin);
}

void
on_info_variables_activate (GtkAction *action, GdbPlugin *plugin)
{
	debugger_info_command (plugin->debugger, "info variables",
						   on_debugger_dialog_message, plugin);
}

void
on_info_locals_activate (GtkAction *action, GdbPlugin *plugin)
{
	debugger_info_command (plugin->debugger, "info locals",
						   on_debugger_dialog_message, plugin);
}

void
on_info_frame_activate (GtkAction *action, GdbPlugin *plugin)
{
	debugger_info_command (plugin->debugger, "info frame",
						   on_debugger_dialog_message, plugin);
}

void
on_info_args_activate (GtkAction *action, GdbPlugin *plugin)
{
	debugger_info_command (plugin->debugger, "info args",
						   on_debugger_dialog_message, plugin);
}

void
on_info_memory_activate (GtkAction * action, GdbPlugin *plugin)
{
	GtkWidget *win_memory;

	win_memory = memory_info_new (plugin->debugger,
								  GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
								  NULL);
	gtk_widget_show(win_memory);
}

/*****************************************************************************/

void
on_debugger_restart_prog_activate (GtkAction * action, GdbPlugin *plugin)
{
	debugger_restart_program (plugin->debugger);
}

void
on_debugger_stop_prog_activate (GtkAction * action, GdbPlugin *plugin)
{
	debugger_stop_program (plugin->debugger);
}

void
on_debugger_detach_activate (GtkAction * action, GdbPlugin *plugin)
{
	debugger_detach_process (plugin->debugger);
}

void
on_debugger_interrupt_activate (GtkAction * action, GdbPlugin *plugin)
{
	debugger_interrupt (plugin->debugger);
}

void
on_debugger_signal_activate (GtkAction * action, GdbPlugin *plugin)
{
	// on_signals_send_activate (NULL, plugin->signals);
}

void
on_debugger_inspect_activate (GtkAction * action, GdbPlugin *plugin)
{
	GtkWidget *w = create_eval_dialog (NULL, plugin->watch);
	gtk_widget_show (w);
}

void
on_debugger_add_watch_activate (GtkAction * action, GdbPlugin *plugin)
{
	on_watch_add_activate (NULL, plugin->watch);
}

void
on_debugger_registers_activate (GtkAction * action, GdbPlugin *plugin)
{
	cpu_registers_show (plugin->registers);
}

void
on_debugger_sharedlibs_activate (GtkAction * action, GdbPlugin *plugin)
{
	sharedlibs_show (plugin->sharedlibs);
}

void
on_debugger_signals_activate (GtkAction * action, GdbPlugin *plugin)
{
	signals_show (plugin->signals);
}

static void
on_debugger_command_entry_activate (GtkEntry *entry, GdbPlugin *plugin)
{
	const gchar *command;
	
	command = gtk_entry_get_text (GTK_ENTRY (entry));
	if (command && strlen (command))
		debugger_command (plugin->debugger, command, FALSE, NULL, NULL);
	gtk_entry_set_text (entry, "");
}

void
on_debugger_custom_command_activate (GtkAction * action, GdbPlugin *plugin)
{
	GladeXML *gxml;
	GtkWidget *win, *entry;
	
	gxml = glade_xml_new (GLADE_FILE, "debugger_command_dialog", NULL);
	win = glade_xml_get_widget (gxml, "debugger_command_dialog");
	entry = glade_xml_get_widget (gxml, "debugger_command_entry");
	
	gtk_window_set_transient_for (GTK_WINDOW (win),
								  GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell));
	g_signal_connect_swapped (win, "response",
							  G_CALLBACK (gtk_widget_destroy),
							  win);
	g_signal_connect (entry, "activate",
					  G_CALLBACK (on_debugger_command_entry_activate),
					  plugin);
	g_object_unref (gxml);
}
