/*
 * debugger.c Copyright (C) 2000  Kh. Naba Kumar Singh
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59 
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#define GDB_PROMPT  "GDB is waiting for command =>"
#define GNOME_TERMINAL "gnome-terminal"

#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

#include <gnome.h>
#include "anjuta.h"
#include "debugger.h"
#include "controls.h"
#include "launcher.h"
#include "fileselection.h"
#include "anjuta_info.h"

Debugger debugger;

static void on_debugger_open_exec_filesel_ok_clicked (GtkButton * button,
						      gpointer user_data);
static void on_debugger_load_core_filesel_ok_clicked (GtkButton * button,
						      gpointer user_data);
static void on_debugger_open_exec_filesel_cancel_clicked (GtkButton * button,
							  gpointer user_data);
static void on_debugger_load_core_filesel_cancel_clicked (GtkButton * button,
							  gpointer user_data);
static void debugger_stop_terminal (void);

void
debugger_init ()
{
	/* Must declare static, because it will be used forever */
	static FileSelData fsd1 = { N_("Load Executable File"), NULL,
		on_debugger_open_exec_filesel_ok_clicked,
		on_debugger_open_exec_filesel_cancel_clicked, NULL
	};

	/* Must declare static, because it will be used forever */
	static FileSelData fsd2 = { N_("Load Core Dump File"), NULL,
		on_debugger_load_core_filesel_ok_clicked,
		on_debugger_load_core_filesel_cancel_clicked, NULL
	};
	debugger.prog_is_running = FALSE;
	debugger.starting = FALSE;
	debugger.gdb_stdo_outputs = NULL;
	debugger.gdb_stde_outputs = NULL;
	strcpy (debugger.current_cmd.cmd, "");
	debugger.current_cmd.parser = NULL;
	debugger.cmd_queqe = NULL;
	strcpy (debugger.stdo_line, "");
	strcpy (debugger.stde_line, "");
	debugger.stdo_cur_char_pos = 0;
	debugger.stde_cur_char_pos = 0;
	debugger.child_pid = -1;
	debugger.term_is_running = FALSE;
	debugger.term_pid = -1;

	debugger.open_exec_filesel = create_fileselection_gui (&fsd1);
	debugger.load_core_filesel = create_fileselection_gui (&fsd2);
	debugger.breakpoints_dbase = breakpoints_dbase_new ();
	debugger.stack = stack_trace_new ();
	debugger.watch = expr_watch_new ();
	debugger.cpu_registers = cpu_registers_new ();
	debugger.signals = signals_new ();
	debugger.sharedlibs = sharedlibs_new ();
	debugger.attach_process = attach_process_new ();

	debugger_set_active (FALSE);
	debugger_set_ready (TRUE);

	debugger_update_controls ();
}

gboolean
debugger_save_yourself (FILE * stream)
{
	if (!breakpoints_dbase_save_yourself
	    (debugger.breakpoints_dbase, stream))
		return FALSE;
	if (!expr_watch_save_yourself (debugger.watch, stream))
		return FALSE;
	if (!cpu_registers_save_yourself (debugger.cpu_registers, stream))
		return FALSE;
	if (!signals_save_yourself (debugger.signals, stream))
		return FALSE;
	if (!sharedlibs_save_yourself (debugger.sharedlibs, stream))
		return FALSE;
	if (!attach_process_save_yourself (debugger.attach_process, stream))
		return FALSE;
	if (!stack_trace_save_yourself (debugger.stack, stream))
		return FALSE;
	return TRUE;
}

gboolean
debugger_load_yourself (PropsID stream)
{
	if (!breakpoints_dbase_load_yourself
	    (debugger.breakpoints_dbase, stream))
		return FALSE;
	if (!expr_watch_load_yourself (debugger.watch, stream))
		return FALSE;
	if (!cpu_registers_load_yourself (debugger.cpu_registers, stream))
		return FALSE;
	if (!signals_load_yourself (debugger.signals, stream))
		return FALSE;
	if (!sharedlibs_load_yourself (debugger.sharedlibs, stream))
		return FALSE;
	if (!attach_process_load_yourself (debugger.attach_process, stream))
		return FALSE;
	if (!stack_trace_load_yourself (debugger.stack, stream))
		return FALSE;
	return TRUE;
}

static void
on_debugger_open_exec_filesel_ok_clicked (GtkButton * button,
					  gpointer user_data)
{
	gchar *filename, *command, *dir;

	gtk_widget_hide (debugger.open_exec_filesel);
	filename = fileselection_get_filename (debugger.open_exec_filesel);
	if (!filename)
		return;
	messages_append (app->messages, _("Loading Executable: "),
			 MESSAGE_DEBUG);
	messages_append (app->messages, filename, MESSAGE_DEBUG);
	messages_append (app->messages, "\n", MESSAGE_DEBUG);

	command = g_strconcat ("file ", filename, NULL);
	dir = g_dirname (filename);
	anjuta_set_execution_dir(dir);
	g_free (dir);
	debugger_put_cmd_in_queqe (command, DB_CMD_ALL, NULL, NULL);
	g_free (command);
	debugger.starting = TRUE;

	debugger_put_cmd_in_queqe ("info signals", DB_CMD_NONE,
				   signals_update, debugger.signals);
	debugger_put_cmd_in_queqe ("info sharedlibrary", DB_CMD_NONE,
				   sharedlibs_update, debugger.sharedlibs);
	expr_watch_cmd_queqe (debugger.watch);
	stack_trace_clear (debugger.stack);
	cpu_registers_clear (debugger.cpu_registers);
	debugger_execute_cmd_in_queqe ();
	g_free (filename);
}

static void
on_debugger_open_exec_filesel_cancel_clicked (GtkButton * button,
					      gpointer user_data)
{
	gtk_widget_hide (debugger.open_exec_filesel);
}

static void
on_debugger_load_core_filesel_ok_clicked (GtkButton * button,
					  gpointer user_data)
{
	gchar *filename, *command, *dir;

	gtk_widget_hide (debugger.load_core_filesel);
	filename = fileselection_get_filename (debugger.load_core_filesel);
	if (!filename)
		return;
	messages_append (app->messages, _("Loading Core: "), MESSAGE_DEBUG);
	messages_append (app->messages, filename, MESSAGE_DEBUG);
	messages_append (app->messages, "\n", MESSAGE_DEBUG);

	command = g_strconcat ("core-file ", filename, NULL);
	dir = g_dirname (filename);
	anjuta_set_execution_dir(dir);
	g_free (dir);

	debugger_put_cmd_in_queqe (command, DB_CMD_ALL, NULL, NULL);
	g_free (command);
	debugger_put_cmd_in_queqe ("info sharedlibrary", DB_CMD_NONE,
				   sharedlibs_update, debugger.sharedlibs);
	expr_watch_cmd_queqe (debugger.watch);
	debugger_put_cmd_in_queqe ("info registers", DB_CMD_NONE,
				   cpu_registers_update,
				   debugger.cpu_registers);
	debugger_put_cmd_in_queqe ("backtrace", DB_CMD_NONE,
				   stack_trace_update, debugger.stack);
	debugger_execute_cmd_in_queqe ();
	g_free (filename);
}

static void
on_debugger_load_core_filesel_cancel_clicked (GtkButton * button,
					      gpointer user_data)
{
	gtk_widget_hide (debugger.load_core_filesel);
}

void
debugger_open_exec_file ()
{
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;

	if (debugger.prog_is_running == TRUE)
	{
		if (debugger.prog_is_attached == TRUE)
		{
			messagebox (GNOME_MESSAGE_BOX_INFO,
				    _
				    ("You have a process ATTACHED under debugger.\n"
				     \
				     "Please detach it first and then load the EXEC file."));
		}
		else
		{
			messagebox (GNOME_MESSAGE_BOX_INFO,
				    _
				    ("You have a process RUNNING under debugger.\n"
				     \
				     "Please stop it first and then load the EXEC file."));
		}
		return;
	}

	gtk_widget_show (debugger.open_exec_filesel);
}

void
debugger_load_core_file ()
{
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	if (debugger.prog_is_running == TRUE)
	{
		if (debugger.prog_is_attached == TRUE)
		{
			messagebox (GNOME_MESSAGE_BOX_INFO,
				    _
				    ("You have a process ATTACHED under debugger.\n"
				     \
				     "Please detach it first and then load the CORE file."));
		}
		else
		{
			messagebox (GNOME_MESSAGE_BOX_INFO,
				    _
				    ("You have a process RUNNING under debugger.\n"
				     \
				     "Please stop it first and then load the CORE file."));
		}
		return;
	}
	gtk_widget_show (debugger.load_core_filesel);
}

void
debugger_shutdown ()
{
	if (debugger.term_is_running)
	{
		anjuta_unregister_child_process (debugger.term_pid);
		debugger.term_is_running = FALSE;
		kill (debugger.term_pid, SIGTERM);
	}
	if (debugger.breakpoints_dbase)
		breakpoints_dbase_destroy (debugger.breakpoints_dbase);
	if (debugger.watch)
		expr_watch_destroy (debugger.watch);
	if (debugger.cpu_registers)
		cpu_registers_destroy (debugger.cpu_registers);
	if (debugger.stack)
		stack_trace_destroy (debugger.stack);
	if (debugger.signals)
		signals_destroy (debugger.signals);
	if (debugger.sharedlibs)
		sharedlibs_destroy (debugger.sharedlibs);
	if (debugger.attach_process)
		attach_process_destroy (debugger.attach_process);
}

void
debugger_set_active (gboolean state)
{
	debugger.active = state;
	/* debugger_update_controls (); */
}

void
debugger_set_ready (gboolean state)
{
	debugger.ready = state;
/*	if (debugger.cmd_queue == NULL) */
	debugger_update_controls ();
}

gboolean
debugger_is_ready ()
{
	return debugger.ready;
}

gboolean
debugger_is_active ()
{
	return debugger.active;
}

DebuggerCommand *
debugger_get_next_command ()
{
	DebuggerCommand *dc;
	if (debugger.cmd_queqe)
	{
		dc = g_list_nth_data (debugger.cmd_queqe, 0);
		debugger.cmd_queqe = g_list_remove (debugger.cmd_queqe, dc);
	}
	else
		dc = NULL;
	return dc;
}

void
debugger_set_next_command ()
{
	DebuggerCommand *dc;
	dc = debugger_get_next_command ();
	if (!dc)
	{
		strcpy (debugger.current_cmd.cmd, "");
		debugger.current_cmd.flags = 0;
		debugger.current_cmd.parser = NULL;
		debugger.current_cmd.data = NULL;
		return;
	}
	strcpy (debugger.current_cmd.cmd, dc->cmd);
	debugger.current_cmd.flags = dc->flags;
	debugger.current_cmd.parser = dc->parser;
	debugger.current_cmd.data = dc->data;
	g_free (dc);
}

void
debugger_put_cmd_in_queqe (gchar cmd[], gint flags,
			   void (*parser) (GList * outputs, gpointer data),
			   gpointer data)
{
	DebuggerCommand *dc;
	if (!(debugger_is_active ()) || !(debugger_is_ready ()))
		return;
	dc = g_malloc (sizeof (DebuggerCommand));
	if (dc)
	{
		strcpy (dc->cmd, cmd);
		dc->flags = flags;
		dc->parser = parser;
		dc->data = data;
	}
	debugger.cmd_queqe = g_list_append (debugger.cmd_queqe, dc);
}

void
debugger_clear_cmd_queqe ()
{
	GList *node;

	node = debugger.cmd_queqe;
	while (node)
	{
		g_free (node->data);
		node = g_list_next (node);
	}
	g_list_free (debugger.cmd_queqe);
	debugger.cmd_queqe = NULL;
	strcpy (debugger.current_cmd.cmd, "");
	debugger.current_cmd.flags = DB_CMD_ALL;
	debugger.current_cmd.parser = NULL;
	debugger.current_cmd.data = NULL;
	debugger_clear_buffers ();
}

void
debugger_clear_buffers ()
{
	gint i;
	/* Clear the Output Buffer */
	for (i = 0; i < g_list_length (debugger.gdb_stdo_outputs); i++)
		g_free (g_list_nth_data (debugger.gdb_stdo_outputs, i));
	g_list_free (debugger.gdb_stdo_outputs);
	debugger.gdb_stdo_outputs = NULL;

	/* Clear the output line buffer */
	strcpy (debugger.stdo_line, "");
	debugger.stdo_cur_char_pos = 0;

	/* Clear the Error Buffer */
	for (i = 0; i < g_list_length (debugger.gdb_stde_outputs); i++)
		g_free (g_list_nth_data (debugger.gdb_stde_outputs, i));
	g_list_free (debugger.gdb_stde_outputs);
	debugger.gdb_stde_outputs = NULL;

	/* Clear the error line buffer */
	strcpy (debugger.stde_line, "");
	debugger.stde_cur_char_pos = 0;
}

void
debugger_execute_cmd_in_queqe ()
{
	if (debugger_is_active () && debugger_is_ready ())
	{
		debugger_clear_buffers ();
		debugger_set_next_command ();
		if (strcmp (debugger.current_cmd.cmd, "") == 0)
			return;
		else
			debugger_command (debugger.current_cmd.cmd);
	}
}

void
debugger_start (gchar * prog)
{
	gchar *command_str, *dir, *tmp, *text;
	gchar *exec_dir;
	gboolean ret;
	GList *list;
	gint i;

	if (anjuta_is_installed ("gdb", TRUE) == FALSE)
		return;
	if (debugger_is_active ())
		return;

	debugger_set_active (TRUE);
	debugger_set_ready (FALSE);
	debugger_clear_cmd_queqe ();
	debugger.child_pid = -1;
	debugger.term_is_running = FALSE;
	debugger.term_pid = -1;

	tmp = g_strconcat (app->dirs->data, "/", "gdb.init", NULL);
	if (file_is_regular (tmp) == FALSE)
	{
		anjuta_error (_("I can not find: %s.\n"
				   "Unable to initialize Debugger. :-(\n"
				   "Make sure you have installed Anjuta properly."), tmp);
		g_free (tmp);
		debugger_set_active (FALSE);
		debugger_set_ready (FALSE);
		return;
	}
	g_free (tmp);

	list = GTK_CLIST (app->src_paths->widgets.src_clist)->row_list;

	exec_dir = project_dbase_get_module_dir (app->project_dbase, MODULE_SOURCE);
	if (exec_dir)
	{
		dir = g_strconcat (" -directory=", exec_dir, NULL);
		g_free (exec_dir);
	}
	else
		dir = g_strdup (" ");
	for (i = 0; i < g_list_length (list); i++)
	{
		gtk_clist_get_text (GTK_CLIST
				    (app->src_paths->widgets.src_clist), i, 0,
				    &text);
		if (text[0] == '/')
			tmp = g_strconcat (dir, " -directory=", text, NULL);
		else
		{
			if (app->project_dbase->project_is_open)
				tmp =
					g_strconcat (dir, " -directory=",
						     app->project_dbase->
						     top_proj_dir, "/", text,
						     NULL);
			else
				tmp =
					g_strconcat (dir, " -directory=",
						     text, NULL);
		}
		g_free (dir);
		dir = tmp;
	}
	if (prog)
	{
		tmp = g_dirname (prog);
		chdir (tmp);
		anjuta_set_execution_dir (tmp);
		command_str =
			g_strdup_printf
			("gdb -q -f -n %s -cd=%s -x %s/gdb.init %s", dir, tmp,
			 app->dirs->data, prog);
		g_free (tmp);
	}
	else
	{
		command_str =
			g_strdup_printf ("gdb -q -f -n %s -x %s/gdb.init ",
					 dir, app->dirs->data);
	}
	g_free (dir);
	debugger.starting = TRUE;
	ret = launcher_execute (command_str, gdb_stdout_line_arrived,
				gdb_stderr_line_arrived, gdb_terminated);
	messages_clear (app->messages, MESSAGE_DEBUG);
	if (ret == TRUE)
	{
		anjuta_update_app_status (TRUE, _("Debugger"));
		messages_append (app->messages,
				 _
				 ("Getting ready to start debugging session ...\n"),
				 MESSAGE_DEBUG);
		messages_append (app->messages, _("Loading Executable: "),
				 MESSAGE_DEBUG);

		if (prog)
		{
			messages_append (app->messages, prog, MESSAGE_DEBUG);
			messages_append (app->messages, "\n", MESSAGE_DEBUG);
		}
		else
		{
			messages_append (app->messages, _("No where\n"),
					 MESSAGE_DEBUG);
			messages_append (app->messages,
					 _
					 ("Open an executable or attach to a process to start debuging.\n"),
					 MESSAGE_DEBUG);
		}
	}
	else
	{
		messages_append (app->messages,
				 _
				 ("There was an error while launching Debugger!!\n"),
				 MESSAGE_DEBUG);
		messages_append (app->messages,
				 _
				 ("Make sure you have 'gdb' installed in your box.\n"),
				 MESSAGE_DEBUG);
	}
	messages_show (app->messages, MESSAGE_DEBUG);
	g_free (command_str);
}

void
gdb_stdout_line_arrived (gchar * chars)
{
	gint i;

	for (i = 0; i < strlen (chars); i++)
	{
		if (chars[i] == '\n'
		    || debugger.stdo_cur_char_pos >= (FILE_BUFFER_SIZE - 1))
		{
			debugger.stdo_line[debugger.stdo_cur_char_pos] = '\0';
			debugger_stdo_flush ();
		}
		else
		{
			debugger.stdo_line[debugger.stdo_cur_char_pos] =
				chars[i];
			debugger.stdo_cur_char_pos++;
		}
	}
}

void
gdb_stderr_line_arrived (gchar * chars)
{
	gint i;
	for (i = 0; i < strlen (chars); i++)
	{
		if (chars[i] == '\n'
		    || debugger.stde_cur_char_pos >= FILE_BUFFER_SIZE - 1)
		{
			debugger.stde_line[debugger.stde_cur_char_pos] = '\0';
			debugger_stde_flush ();
		}
		else
		{
			debugger.stde_line[debugger.stde_cur_char_pos] =
				chars[i];
			debugger.stde_cur_char_pos++;
		}
	}
}

void
debugger_stdo_flush ()
{
	guint lineno;
	gchar *filename, *line;

	line = debugger.stdo_line;
	if (strlen (line) == 0)
	{
		debugger.gdb_stdo_outputs =
			g_list_append (debugger.gdb_stdo_outputs,
				       g_strdup (" "));
		if (debugger.current_cmd.flags & DB_CMD_SO_MESG)
		{
			messages_append (app->messages, "\n", MESSAGE_DEBUG);
		}
		return;
	}
	if (line[0] == '\032' && line[1] == '\032')
	{
		parse_error_line (&(line[2]), &filename, &lineno);
		if (filename)
		{
			anjuta_goto_file_line (filename, lineno);
			g_free (filename);
		}
	}
	else if (strcmp (line, GDB_PROMPT) == 0)
	{
		GList *list;
		/* Ready */
		debugger_set_ready (TRUE);

		/* Check if the Program has terminated */
		list = debugger.gdb_stdo_outputs;
		while (list)
		{
			if (strstr ((gchar *) list->data, "Program exited") !=
			    NULL)
			{
				debugger_put_cmd_in_queqe ("info program",
							   DB_CMD_NONE,
							   on_debugger_update_prog_status,
							   NULL);
				break;
			}
			list = g_list_next (list);
		}
		if (strcmp (debugger.current_cmd.cmd, "") != 0
		    && debugger.current_cmd.parser != NULL)
			debugger.current_cmd.parser (debugger.
						     gdb_stdo_outputs,
						     debugger.current_cmd.
						     data);

		/* Clear the Output Buffer */
		list = debugger.gdb_stdo_outputs;
		while (list)
		{
			if (list->data)
				g_free (list->data);
			list = g_list_next (list);
		}
		g_list_free (debugger.gdb_stdo_outputs);
		debugger.gdb_stdo_outputs = NULL;

		if (debugger.current_cmd.flags & DB_CMD_SE_DIALOG)
			debugger_dialog_error (debugger.gdb_stde_outputs,
					       NULL);

		/* Clear the Error Buffer */
		list = debugger.gdb_stde_outputs;
		while (list)
		{
			if (debugger.current_cmd.flags & DB_CMD_SE_MESG)
			{
				messages_append (app->messages, _("Error: "),
						 MESSAGE_DEBUG);
				messages_append (app->messages,
						 (gchar *) list->data,
						 MESSAGE_DEBUG);
				messages_append (app->messages, "\n",
						 MESSAGE_DEBUG);
			}
			if (list->data)
				g_free (list->data);
			list = g_list_next (list);
		}
		g_list_free (debugger.gdb_stde_outputs);
		debugger.gdb_stde_outputs = NULL;

		if (debugger.starting)
		{
			debugger.starting = FALSE;
			breakpoints_dbase_set_all (debugger.
						   breakpoints_dbase);
			debugger_put_cmd_in_queqe ("info signals",
						   DB_CMD_NONE,
						   signals_update,
						   debugger.signals);
			messages_append (app->messages,
					 _("Debugger is ready.\n"),
					 MESSAGE_DEBUG);
		}

		/* Clear the line buffer */
		strcpy (debugger.stdo_line, "");
		debugger.stdo_cur_char_pos = 0;

		/* Clear the line buffer */
		strcpy (debugger.stdo_line, "");
		debugger.stdo_cur_char_pos = 0;

		debugger_execute_cmd_in_queqe ();	/* Go.... */
		return;
	}
	else
	{
   /**************************************************************************************
    * This is basically a problem. When the command given to the gdb (through stdin)
    * is also echoed to the stdout, lots of things go wrong. To avoid this, the first line of the 
    * buffer is compared with the command given. If it is same, then the terminal has echoed
    * the stdin and we will remove the first line of the buffer.
    ***************************************************************************************/
		if (g_list_length (debugger.gdb_stdo_outputs) == 0)
		{
			if (strcmp (debugger.current_cmd.cmd, line) != 0)
			{
				/* The terminal has not echoed the command. So, add it */
				if (strncmp
				    (line, "Reading ",
				     strlen ("Reading ")) != 0)
				{
					debugger.gdb_stdo_outputs =
						g_list_append (debugger.
							       gdb_stdo_outputs,
							       g_strdup
							       (line));
				}
				if (debugger.current_cmd.
				    flags & DB_CMD_SO_MESG)
				{
					messages_append (app->messages, line,
							 MESSAGE_DEBUG);
					messages_append (app->messages, "\n",
							 MESSAGE_DEBUG);
				}
			}
		}
    /**********************************************************/
		else
		{
			if (strncmp (line, "Reading ", strlen ("Reading ")) !=
			    0)
			{
				debugger.gdb_stdo_outputs =
					g_list_append (debugger.
						       gdb_stdo_outputs,
						       g_strdup (line));
			}
			if (debugger.current_cmd.flags & DB_CMD_SO_MESG)
			{
				messages_append (app->messages, line,
						 MESSAGE_DEBUG);
				messages_append (app->messages, "\n",
						 MESSAGE_DEBUG);
			}
		}
	}
	/* Clear the line buffer */
	strcpy (debugger.stdo_line, "");
	debugger.stdo_cur_char_pos = 0;
}

void
debugger_stde_flush ()
{
	gchar *line;
	line = debugger.stde_line;
	if (strlen (line) == 0)
		return;
	debugger.gdb_stde_outputs =
		g_list_append (debugger.gdb_stde_outputs, g_strdup (line));
	/* Clear the line buffer */
	strcpy (debugger.stde_line, "");
	debugger.stde_cur_char_pos = 0;
}

void
gdb_terminated (int status, time_t t)
{
	/* Clear the command queue & Buffer */
	debugger_clear_buffers ();
	debugger_stop_terminal ();

	/* Good Bye message */
	messages_append (app->messages,
			 _
			 ("\nWell, did you find the BUG? :: Debugging session completed.\n\n"),
			 MESSAGE_DEBUG);
	/* Ready to start again */
	cpu_registers_clear (debugger.cpu_registers);
	stack_trace_clear (debugger.stack);
	signals_clear (debugger.signals);
	sharedlibs_clear (debugger.sharedlibs);
	debugger_set_active (FALSE);
	debugger_set_ready (TRUE);
	debugger.prog_is_running = FALSE;
	debugger.child_pid = -1;
	debugger.term_is_running = FALSE;
	debugger.term_pid = -1;
	anjuta_update_app_status (TRUE, NULL);
}

void
debugger_command (gchar * com)
{
	if (debugger_is_active () == FALSE)
		return;		/* There is no point in accepting commands when */
	if (debugger_is_ready () == FALSE)
		return;		/* gdb is not running or it is busy */
	debugger_set_ready (FALSE);
	launcher_send_stdin (com);
}

void
debugger_update_controls ()
{
	breakpoints_dbase_update_controls (debugger.breakpoints_dbase);
	expr_watch_update_controls (debugger.watch);
	stack_trace_update_controls (debugger.stack);
	signals_update_controls (debugger.signals);
	debug_toolbar_update ();
}

void
debugger_dialog_message (GList * lines, gpointer data)
{
	if (g_list_length (lines) < 1)
		return;
	anjuta_info_show_list (lines, 0, 0);
}

void
debugger_dialog_error (GList * lines, gpointer data)
{
	gchar *ptr, *tmp;
	gint i;
	if (g_list_length (lines) < 1)
		return;
	ptr = g_strconcat ((gchar *) g_list_nth_data (lines, 0), "\n", NULL);
	for (i = 1; i < g_list_length (lines); i++)
	{
		tmp = ptr;
		ptr =
			g_strconcat (tmp,
				     (gchar *) g_list_nth_data (lines, i),
				     "\n", NULL);
		g_free (tmp);
	}
	anjuta_error (ptr);
	g_free (ptr);
}

static void
on_debugger_terminal_terminated (int status, gpointer data)
{
	debugger.term_is_running = FALSE;
	debugger.term_pid = -1;
	if (debugger.prog_is_running)
		debugger_stop_program ();
}

gchar *
debugger_start_terminal ()
{
	gchar *file, *cmd, dev_name[100], *av[7];
	gint count;
	FILE *fp;
	pid_t pid;

	if (debugger_is_active () == FALSE)
		return NULL;
	if (debugger_is_ready () == FALSE)
		return NULL;
	if (debugger.prog_is_running == TRUE)
		return NULL;
	if (debugger.term_is_running == TRUE)
		return NULL;
	if (anjuta_is_installed ("anjuta_launcher", TRUE) == FALSE)
		return NULL;
	if (anjuta_is_installed (GNOME_TERMINAL, TRUE) == FALSE)
		return NULL;

	count = 0;
	file = get_a_tmp_file ();
	while (file_is_regular (file))
	{
		g_free (file);
		file = get_a_tmp_file ();
		if (count++ > 100)
			goto error;
	}
	if (mkfifo (file, 0664) < 0)
		goto error;

	debugger.term_is_running = TRUE;
	cmd = g_strconcat ("anjuta_launcher --__debug_terminal ", file, NULL);

	av[0] = GNOME_TERMINAL;
	av[1] = GNOME_TERMINAL;
	av[2] = "-t";
	av[3] = "Anjuta: Debug Terminal";
	av[4] = "-e";
	av[5] = cmd;
	av[6] = NULL;

	/* With this command SIGCHILD is not emitted */
	/* pid = gnome_execute_async (app->dirs->home, 6, av); */
	/* so using fork instead */
	if ((pid = fork ()) == 0)
	{
		execvp (GNOME_TERMINAL, av);
		g_error (_("Cannot execute gnome-terminal"));
	}

	g_free (cmd);
	debugger.term_pid = pid;
	if (pid < 1)
		goto error;
	printf ("terminal pid = %d\n", pid);
	anjuta_register_child_process (pid, on_debugger_terminal_terminated,
				       NULL);

	/*
	 * May be the terminal is not started properly.
	 * Callback will reset this flag
	 */
	if (debugger.term_is_running == FALSE)
		goto error;

	/*
	 * Warning: call to fopen() may be blocked if the terminal is not properly started.
	 * I don't know how to handle this. May be opening as non-blocking will solve this .
	 */
	fp = fopen (file, "r");		/* Ok, take the risk. */

	if (!fp)
		goto error;
	if (fscanf (fp, "%s", dev_name) < 1)
		goto error;
	fclose (fp);
	remove (file);
	if (strcmp (dev_name, "__ERROR__") == 0)
		goto error;
	g_free (file);
	return g_strdup (dev_name);
      error:
	anjuta_error (_("Error: Cannot start terminal for debugging."));
	debugger_stop_terminal ();
	remove (file);
	g_free (file);
	return NULL;
}

static void
debugger_stop_terminal ()
{
	if (debugger.term_is_running == FALSE)
		return;
	else
		kill (debugger.term_pid, SIGTERM);
}

void
debugger_start_program (void)
{
	gchar *term, *cmd, *args;

	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	if (debugger.prog_is_running == TRUE)
		return;

	args = prop_get (app->executer->props, EXECUTER_PROGRAM_ARGS_KEY);
	if (args == NULL)
		args = g_strdup (" ");

	term = debugger_start_terminal ();
	if (!term)
	{
		messages_append (app->messages,
				 _
				 ("Warning: No debug terminal. Redirecting stdio to /dev/null.\n"),
				 MESSAGE_DEBUG);
		cmd = g_strconcat ("run >/dev/null 2>/dev/null ", args, NULL);
		debugger_put_cmd_in_queqe (cmd, DB_CMD_ALL, NULL, NULL);
	}
	else
	{
		cmd = g_strconcat ("tty ", term, NULL);
		debugger_put_cmd_in_queqe (cmd, DB_CMD_ALL, NULL, NULL);
		g_free (cmd);
		g_free (term);
		cmd = g_strconcat ("run ", args, NULL);
		debugger_put_cmd_in_queqe (cmd,  DB_CMD_ALL, NULL, NULL);
	}
	g_free (args);
	g_free (cmd);
	debugger_put_cmd_in_queqe ("info program", DB_CMD_NONE,
				   on_debugger_update_prog_status, NULL);
	messages_append (app->messages, _("Running program ... \n"),
			 MESSAGE_DEBUG);
}

/***************** Functons ********************************************/
/***************** GUI Interface ***************************************/
void
on_debugger_update_prog_status (GList * lines, gpointer data)
{
	gchar *str;
	glong pid;
	gint i;
	gboolean error;
	error = FALSE;
	if (lines == NULL)
	{
		error = TRUE;
		goto down;
	}
	str = lines->data;
	if (str == NULL)
	{
		error = TRUE;
		goto down;
	}
	i = 0;
	while (str[i] != '\0')
	{
		str[i] = tolower (str[i]);
		i++;
	}
	str = strstr (str, "pid ");
	if (str == NULL)
	{
		error = TRUE;
		goto down;
	}
	if (sscanf (str, "pid %ld", &pid) != 1)
	{
		error = TRUE;
		goto down;
	}

      down:
	if (error)
	{
		debugger.prog_is_running = FALSE;
		debugger.prog_is_attached = FALSE;
		debugger.child_pid = -1;
		debugger_stop_terminal ();
	}
	else
	{
		debugger.prog_is_running = TRUE;
		debugger.child_pid = pid;
		debugger.stack->current_frame = 0;
		debugger.child_pid = pid;
	}
	update_main_menubar ();
	debug_toolbar_update ();
	debugger_put_cmd_in_queqe ("info sharedlibrary", DB_CMD_NONE,
				   sharedlibs_update, debugger.sharedlibs);
	expr_watch_cmd_queqe (debugger.watch);
	debugger_put_cmd_in_queqe ("info registers", DB_CMD_NONE,
				   cpu_registers_update,
				   debugger.cpu_registers);
	debugger_put_cmd_in_queqe ("backtrace", DB_CMD_NONE,
				   stack_trace_update, debugger.stack);

	debugger_execute_cmd_in_queqe ();
	return;
}

static void
debugger_attach_process_confirmed (GtkWidget * but, gpointer data)
{
	pid_t pid;
	gchar *buf;
	pid = (pid_t) data;
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	buf = g_strdup_printf (_("Attaching to process: %d\n"), pid);
	messages_append (app->messages, buf, MESSAGE_DEBUG);
	g_free (buf);
	buf = g_strdup_printf ("attach %d", pid);
	debugger.prog_is_attached = TRUE;
	debugger_put_cmd_in_queqe (buf, DB_CMD_ALL, NULL, NULL);
	g_free (buf);
	debugger_put_cmd_in_queqe ("info program", DB_CMD_NONE,
				   on_debugger_update_prog_status, NULL);

	debugger_execute_cmd_in_queqe ();
}

void
debugger_attach_process (gint pid)
{
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	if (debugger.prog_is_running == TRUE)
	{
		messagebox2 (GNOME_MESSAGE_BOX_QUESTION,
			     _("A process is already running.\n"
			       "Do you want to terminate it and attach the new process?"),
			     GNOME_STOCK_BUTTON_YES, GNOME_STOCK_BUTTON_NO,
			     debugger_attach_process_confirmed, NULL,
			     (gpointer) pid);
	}
	else if (getpid () == pid || launcher_get_child_pid () == pid)
	{
		anjuta_error (_("You FOOL! I can not attach to myself."));
		return;
	}
	else
		debugger_attach_process_confirmed (NULL, (gpointer) pid);
}

void
debugger_restart_program ()
{
	if (debugger_is_active () == FALSE ||
	    debugger_is_ready () == FALSE
	    || debugger.prog_is_attached == TRUE)
		return;
	debugger_put_cmd_in_queqe ("tbreak main", DB_CMD_NONE, NULL, NULL);
	debugger_put_cmd_in_queqe ("run >/dev/null 2>/dev/null", DB_CMD_ALL,
				   NULL, NULL);
	debugger_put_cmd_in_queqe ("info program", DB_CMD_NONE,
				   on_debugger_update_prog_status, NULL);
	debugger_put_cmd_in_queqe ("continue", DB_CMD_NONE, NULL, NULL);
	debugger_execute_cmd_in_queqe ();
}

void
debugger_stop_program ()
{
	if (debugger_is_active () == FALSE)
		return;
	if (debugger.prog_is_running == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
	{
		debugger_signal ("SIGTERM", FALSE);
	}
	else
	{
		if (debugger.prog_is_attached == TRUE)
		{
			debugger_put_cmd_in_queqe ("detach", DB_CMD_NONE,
						   NULL, NULL);
		}
		else
		{
			debugger_put_cmd_in_queqe ("kill", DB_CMD_ALL, NULL,
						   NULL);
		}
		debugger_put_cmd_in_queqe ("info program", DB_CMD_NONE,
					   on_debugger_update_prog_status,
					   NULL);
		debugger_execute_cmd_in_queqe ();
	}
	messages_append (app->messages, "Program forcefully terminated\n",
			 MESSAGE_DEBUG);
	debugger_stop_terminal ();
	debugger.stack->current_frame = 0;
}

void
debugger_detach_process ()
{
	gchar *buff;
	if (debugger.prog_is_attached == FALSE)
		return;
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	buff =
		g_strdup_printf (_("Detaching the process: %d\n"),
				 (int) debugger.child_pid);
	messages_append (app->messages, buff, MESSAGE_DEBUG);
	g_free (buff);
	debugger_put_cmd_in_queqe ("detach", DB_CMD_ALL, NULL, NULL);
	debugger_put_cmd_in_queqe ("info program", DB_CMD_NONE,
				   on_debugger_update_prog_status, NULL);
	debugger.prog_is_attached = FALSE;
	debugger_execute_cmd_in_queqe ();
}

void
debugger_stop ()
{
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
	{
		launcher_reset ();
	}
	else
	{
		if (debugger.prog_is_attached == TRUE)
		{
			debugger_put_cmd_in_queqe ("detach", DB_CMD_NONE,
						   NULL, NULL);
			debugger_put_cmd_in_queqe ("quit", DB_CMD_NONE, NULL,
						   NULL);
			debugger_execute_cmd_in_queqe ();
		}
		else
		{
			debugger_put_cmd_in_queqe ("quit", DB_CMD_NONE, NULL,
						   NULL);
			debugger_execute_cmd_in_queqe ();
		}
	}
	debugger.stack->current_frame = 0;
}

void
debugger_run ()
{
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	if (debugger.prog_is_running == FALSE)
	{
		debugger_put_cmd_in_queqe ("tbreak main", DB_CMD_NONE, NULL,
					   NULL);
		debugger_start_program ();
		debugger_put_cmd_in_queqe ("continue", DB_CMD_ALL, NULL,
					   NULL);
		debugger_put_cmd_in_queqe ("info program", DB_CMD_NONE,
					   on_debugger_update_prog_status,
					   NULL);
		debugger_execute_cmd_in_queqe ();
		return;
	}
	debugger.stack->current_frame = 0;
	debugger_put_cmd_in_queqe ("continue", DB_CMD_ALL, NULL, NULL);
	debugger_put_cmd_in_queqe ("info program", DB_CMD_NONE,
				   on_debugger_update_prog_status, NULL);
	expr_watch_cmd_queqe (debugger.watch);
	debugger_put_cmd_in_queqe ("info registers", DB_CMD_NONE,
				   cpu_registers_update,
				   debugger.cpu_registers);
	debugger_put_cmd_in_queqe ("backtrace", DB_CMD_NONE,
				   stack_trace_update, debugger.stack);

	debugger_execute_cmd_in_queqe ();
}

void
debugger_step_in ()
{
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	if (debugger.prog_is_running == FALSE)
	{
		debugger_put_cmd_in_queqe ("tbreak main", DB_CMD_NONE, NULL,
					   NULL);
		debugger_start_program ();
		debugger_execute_cmd_in_queqe ();
		return;
	}
	debugger.stack->current_frame = 0;
	debugger_put_cmd_in_queqe ("step", DB_CMD_ALL, NULL, NULL);
	expr_watch_cmd_queqe (debugger.watch);
	debugger_put_cmd_in_queqe ("info program", DB_CMD_NONE,
				   on_debugger_update_prog_status, NULL);
	debugger_put_cmd_in_queqe ("info registers", DB_CMD_NONE,
				   cpu_registers_update,
				   debugger.cpu_registers);
	debugger_put_cmd_in_queqe ("backtrace", DB_CMD_NONE,
				   stack_trace_update, debugger.stack);

	debugger_execute_cmd_in_queqe ();
}

void
debugger_step_over ()
{
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	if (debugger.prog_is_running == FALSE)
		return;
	debugger.stack->current_frame = 0;
	debugger_put_cmd_in_queqe ("next", DB_CMD_ALL, NULL, NULL);
	debugger_put_cmd_in_queqe ("info program", DB_CMD_NONE,
				   on_debugger_update_prog_status, NULL);
	expr_watch_cmd_queqe (debugger.watch);
	debugger_put_cmd_in_queqe ("info registers", DB_CMD_NONE,
				   cpu_registers_update,
				   debugger.cpu_registers);
	debugger_put_cmd_in_queqe ("backtrace", DB_CMD_NONE,
				   stack_trace_update, debugger.stack);

	debugger_execute_cmd_in_queqe ();
}

void
debugger_step_out ()
{
	if (debugger.prog_is_running == FALSE)
		return;
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	debugger.stack->current_frame = 0;
	debugger_put_cmd_in_queqe ("finish", DB_CMD_ALL, NULL, NULL);
	debugger_put_cmd_in_queqe ("info program", DB_CMD_NONE,
				   on_debugger_update_prog_status, NULL);
	expr_watch_cmd_queqe (debugger.watch);
	debugger_put_cmd_in_queqe ("info registers", DB_CMD_NONE,
				   cpu_registers_update,
				   debugger.cpu_registers);
	debugger_put_cmd_in_queqe ("backtrace", DB_CMD_NONE,
				   stack_trace_update, debugger.stack);

	debugger_execute_cmd_in_queqe ();
}

void debugger_run_to_location(gchar *loc)
{
	gchar *buff;

	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	if (debugger.prog_is_running == FALSE)
	{
		buff = g_strconcat ("tbreak ", loc, NULL);
		debugger_put_cmd_in_queqe (buff, DB_CMD_NONE, NULL, NULL);
		debugger_start_program ();
		debugger_execute_cmd_in_queqe ();
		return;
	}

	debugger.stack->current_frame = 0;
	buff = g_strdup_printf ("until %s", loc);
	debugger_put_cmd_in_queqe (buff, DB_CMD_ALL, NULL, NULL);
	g_free (buff);

	debugger_put_cmd_in_queqe ("info program", DB_CMD_NONE,
				   on_debugger_update_prog_status, NULL);
	expr_watch_cmd_queqe (debugger.watch);
	debugger_put_cmd_in_queqe ("info registers", DB_CMD_NONE,
				   cpu_registers_update,
				   debugger.cpu_registers);
	debugger_put_cmd_in_queqe ("backtrace", DB_CMD_NONE,
				   stack_trace_update, debugger.stack);

	debugger_execute_cmd_in_queqe ();
}

void
debugger_enable_breakpoint (gint id)
{
	gchar buff[20];
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	sprintf (buff, "enable %d", id);
	debugger_put_cmd_in_queqe (buff, DB_CMD_ALL, NULL, NULL);
	debugger_put_cmd_in_queqe ("info breakpoints", DB_CMD_NONE,
				   breakpoints_dbase_update,
				   debugger.breakpoints_dbase);
	debugger_execute_cmd_in_queqe ();
}

void
debugger_enable_all_breakpoints ()
{
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	debugger_put_cmd_in_queqe ("enable breakpoints", DB_CMD_ALL, NULL,
				   NULL);
	debugger_put_cmd_in_queqe ("info breakpoints", DB_CMD_NONE,
				   breakpoints_dbase_update,
				   debugger.breakpoints_dbase);
	messages_append (app->messages, _("All breakpoints enabled:\n"),
			 MESSAGE_DEBUG); debugger_execute_cmd_in_queqe ();
}

void
debugger_disable_breakpoint (gint id)
{
	gchar buff[20];
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	sprintf (buff, "disable %d", id);
	debugger_put_cmd_in_queqe (buff, DB_CMD_ALL, NULL, NULL);
	debugger_put_cmd_in_queqe ("info breakpoints", DB_CMD_NONE,
				   breakpoints_dbase_update,
				   debugger.breakpoints_dbase);
	debugger_execute_cmd_in_queqe ();
}

void
debugger_disable_all_breakpoints ()
{
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	debugger_put_cmd_in_queqe ("disable breakpoints", DB_CMD_ALL, NULL,
				   NULL);
	debugger_put_cmd_in_queqe ("info breakpoints", DB_CMD_NONE,
				   breakpoints_dbase_update,
				   debugger.breakpoints_dbase);
	messages_append (app->messages, _("All breakpoints disabled:\n"),
			 MESSAGE_DEBUG); debugger_execute_cmd_in_queqe ();
}

void
debugger_delete_breakpoint (gint id)
{
	gchar buff[20];
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	sprintf (buff, "delete %d", id);
	debugger_put_cmd_in_queqe (buff, DB_CMD_ALL, NULL, NULL);
	debugger_put_cmd_in_queqe ("info breakpoints", DB_CMD_NONE,
				   breakpoints_dbase_update,
				   debugger.breakpoints_dbase);
	debugger_execute_cmd_in_queqe ();
}

void
debugger_delete_all_breakpoints ()
{
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	debugger_put_cmd_in_queqe ("delete breakpoints", DB_CMD_ALL, NULL,
				   NULL);
	debugger_put_cmd_in_queqe ("info breakpoints", DB_CMD_NONE,
				   breakpoints_dbase_update,
				   debugger.breakpoints_dbase);
	messages_append (app->messages, _("All breakpoints deleted:\n"),
			 MESSAGE_DEBUG); debugger_execute_cmd_in_queqe ();
}

void
debugger_custom_command ()
{
	anjuta_not_implemented (__FILE__, __LINE__);
}

void
debugger_interrupt ()
{
	gchar *buff;
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready ())
		return;
	if (debugger.prog_is_running == FALSE)
		return;
	if (debugger.child_pid < 1)
		return;
	buff =
		g_strdup_printf (_("Interrupting the process: %d\n"),
				 (int) debugger.child_pid);
	messages_append (app->messages, buff, MESSAGE_DEBUG);
	g_free (buff);
	debugger.stack->current_frame = 0;
	kill (debugger.child_pid, SIGINT);
}

void
debugger_signal (gchar * sig, gboolean show_msg)	/* eg:- "SIGTERM" */
{
	gchar *buff;
	gchar *cmd;
	if (debugger_is_active () == FALSE)
		return;
	if (debugger.prog_is_running == FALSE)
		return;
	if (debugger.child_pid < 1)
		return;

	if (show_msg)
	{
		buff =
			g_strdup_printf (_
					 ("Sending signal %s to the process: %d\n"),
					 sig, (int) debugger.child_pid);
		messages_append (app->messages, buff, MESSAGE_DEBUG);
		g_free (buff);
	}

	if (debugger_is_ready ())
	{
		cmd = g_strconcat ("signal ", sig, NULL);
		debugger.stack->current_frame = 0;
		debugger_put_cmd_in_queqe (cmd, DB_CMD_ALL, NULL, NULL);
		debugger_put_cmd_in_queqe ("info program", DB_CMD_NONE,
					   on_debugger_update_prog_status,
					   NULL);
		g_free (cmd);
		debugger_execute_cmd_in_queqe ();
	}
	else
	{
		pid_t pid;
		int status;
		cmd =
			g_strdup_printf ("kill -s %s %d", sig,
					 debugger.child_pid);
		pid = gnome_execute_shell (app->dirs->home, cmd);
		waitpid (pid, &status, 0);
		if (WEXITSTATUS (status) != 0 && show_msg)
		{
			messagebox (GNOME_MESSAGE_BOX_ERROR,
				    _
				    ("There was some error while signaling the process."));
		}
	}
}
