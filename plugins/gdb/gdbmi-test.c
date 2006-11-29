/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gdbmi-test.c
 * Copyright (C) Naba Kumar 2005 <naba@gnome.org>
 * 
 * gdbmi-test.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2, or (at your option) any later version.
 * 
 * main.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with main.c.  See the file "COPYING".  If not,
 * write to:  The Free Software Foundation, Inc.,
 *            59 Temple Place - Suite 330,
 *            Boston,  MA  02111-1307, USA.
 */

/* MI parser */
#include <stdio.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "gdbmi.h"
#include "debugger.h"

static gchar *gdb_test_line = 
"^done,BreakpointTable={nr_rows=\"2\",nr_cols=\"6\",hdr=[{width=\"3\",alignment=\"-1\",col_name=\"number\",colhdr=\"Num\"},{width=\"14\",alignment=\"-1\",col_name=\"type\",colhdr=\"Type\"},{width=\"4\",alignment=\"-1\",col_name=\"disp\",colhdr=\"Disp\"},{width=\"3\",alignment=\"-1\",col_name=\"enabled\",colhdr=\"Enb\"},{width=\"10\",alignment=\"-1\",col_name=\"addr\",colhdr=\"Address\"},{width=\"40\",alignment=\"2\",col_name=\"what\",colhdr=\"What\"}],body=[bkpt={number=\"1\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"0x08050f5d\",func=\"main\",file=\"main.c\",line=\"122\",times=\"1\"},bkpt={number=\"2\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"0x0096fbae\",func=\"anjuta_plugin_activate\",file=\"anjuta-plugin.c\",line=\"395\",times=\"1\"}]}";

#if 0
static void
output_callback (Debugger *debugger, DebuggerOutputType type,
				 const gchar *data, gpointer user_data)
{
	printf ("%s", data);
}

static void
output_parser (Debugger* debugger, const GDBMIValue *mi_result,
			   const GList* cli_result, gpointer data, gpointer extra_data)
{
	gdbmi_value_dump (mi_result, 0);
}

static void
on_location_changed (Debugger* debugger, const gchar *file, gint line,
					 gchar *addr, gpointer data)
{
	printf ("Location changed %s:%d\n", file, line);
}

static void
on_program_running (Debugger* debugger, gpointer data)
{
}

static void
on_program_exited (Debugger* debugger, const GDBMIValue *value,
				   gpointer data)
{
}

static void
on_program_stopped (Debugger* debugger, const GDBMIValue *value,
					gpointer data)
{
}

static void
on_entry_activate (GtkEntry *entry, Debugger *debugger)
{
	debugger_command (debugger, gtk_entry_get_text (entry),
					  FALSE, output_parser, NULL);
}
#endif

int
main(int argc, char **argv)
{
	GDBMIValue *val;
	gchar *ptr;
	/* Debugger *debugger; */
	/* GtkWidget *win, *entry; */
	
	printf("Test GDB MI interface.\n");
	ptr = gdb_test_line;
	val = gdbmi_value_parse (ptr);
	if (val)
	{
		printf ("GDB MI parse test successful:\n");
		printf ("Data ==>\n");
		gdbmi_value_dump (val, 0);
	}
	else
	{
		printf ("GDB MI parse test failed\n");
	}
	printf ("Testing debugger\n");
	gtk_init (&argc, &argv);

	#if 0	
	debugger = debugger_new_with_program (NULL, NULL, output_callback, NULL,
										  "./gdbmi-test", TRUE);
	
	g_signal_connect (debugger, "location-changed",
					  G_CALLBACK (on_location_changed), NULL);
	g_signal_connect (debugger, "program-running",
					  G_CALLBACK (on_program_running), NULL);
	g_signal_connect (debugger, "program-exited",
					  G_CALLBACK (on_program_exited), NULL);
	g_signal_connect (debugger, "program-stopped",
					  G_CALLBACK (on_program_stopped), NULL);
	
	win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	entry = gtk_entry_new ();
	gtk_container_add (GTK_CONTAINER (win), entry);
	g_signal_connect (entry, "activate",
					  G_CALLBACK (on_entry_activate),
					  debugger);
	gtk_widget_show_all (win);
	gtk_main();
#endif
	return (0);
}
