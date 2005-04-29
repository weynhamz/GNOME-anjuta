/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gdbmi-test.c
 * Copyright (C) Naba Kumar 2005 <naba@gnome.org>
 * 
 * main.c is free software.
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
#include "gdbmi.h"

static gchar *gdb_test_line = 
"^done,BreakpointTable={nr_rows=\"2\",nr_cols=\"6\",hdr=[{width=\"3\",alignment=\"-1\",col_name=\"number\",colhdr=\"Num\"},{width=\"14\",alignment=\"-1\",col_name=\"type\",colhdr=\"Type\"},{width=\"4\",alignment=\"-1\",col_name=\"disp\",colhdr=\"Disp\"},{width=\"3\",alignment=\"-1\",col_name=\"enabled\",colhdr=\"Enb\"},{width=\"10\",alignment=\"-1\",col_name=\"addr\",colhdr=\"Address\"},{width=\"40\",alignment=\"2\",col_name=\"what\",colhdr=\"What\"}],body=[bkpt={number=\"1\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"0x08050f5d\",func=\"main\",file=\"main.c\",line=\"122\",times=\"1\"},bkpt={number=\"2\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"0x0096fbae\",func=\"anjuta_plugin_activate\",file=\"anjuta-plugin.c\",line=\"395\",times=\"1\"}]}";

int main()
{
	GDBMIValue *val;
	gchar *ptr;
	
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
	return (0);
}
