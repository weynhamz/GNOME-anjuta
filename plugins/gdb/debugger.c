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

#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <errno.h>

#include <gnome.h>
#include <libanjuta/anjuta-launcher.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-children.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include "debugger.h"
/* TODO #include "controls.h" */
#include "info.h"

#include "utilities.h"
#include "plugin.h"

#define GDB_PROMPT  "GDB is waiting for command =>"
#define PREF_TERMINAL_COMMAND "anjuta.command.terminal"

Debugger debugger;

enum {
	DEBUGGER_NONE,
	DEBUGGER_EXIT,
	DEBUGGER_RERUN_PROGRAM
};

static void locals_update_controls (void);
static void debugger_info_prg (void);
static void debugger_stop_terminal (void);
static void debugger_info_locals_cb (GList* list, gpointer data);
static void debugger_handle_post_execution (void);
static void debugger_command (const gchar * com);
static void debugger_clear_buffers (void);
static void debugger_clear_cmd_queqe (void);

static DebuggerCommand *debugger_get_next_command (void);
static void debugger_set_next_command (void);

static void gdb_stdout_line_arrived (const gchar * line);
static void gdb_stderr_line_arrived (const gchar * line);

static void on_gdb_output_arrived (AnjutaLauncher *launcher,
								   AnjutaLauncherOutputType output_type,
								   const gchar *chars, gpointer data);
static void on_gdb_terminated (AnjutaLauncher *launcher,
							   gint child_pid, gint status,
							   gulong t, gpointer data);

static void
debugger_add_widget (GtkWidget *w, const gchar *name, const gchar * title,
					 const gchar *icon)
{
	anjuta_shell_add_widget (debugger.plugin->parent.shell, w, name, title,
							 icon, ANJUTA_SHELL_PLACEMENT_BOTTOM,
							 NULL /* TODO */);
}

void
debugger_init (GdbPlugin *plugin)
{
	DEBUG_PRINT ("In function: debugger_init()");

	debugger.plugin = plugin;
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
	debugger.post_execution_flag = DEBUGGER_NONE;
	debugger.gnome_terminal_type = gdb_util_check_gnome_terminal();

    DEBUG_PRINT ("gnome-terminal type %d found.", debugger.gnome_terminal_type);

	debugger.breakpoints_dbase = breakpoints_dbase_new (ANJUTA_PLUGIN (plugin));
	
	debugger.stack = stack_trace_new ();
	debugger_add_widget (stack_trace_get_main_widget (debugger.stack),
						 "AnjutaDebuggerStack", _("Stack"),
						 "gdb-stack-icon");
	
	debugger.locals = locals_create ();
	debugger_add_widget (locals_get_main_widget (debugger.locals),
						 "AnjutaDebuggerLocals", _("Locals"),
						 "gdb-locals-icon");
	
	debugger.watch = expr_watch_new ();
	debugger_add_widget (debugger.watch->widgets.scrolledwindow,
						 "AnjutaDebuggerWatch", _("Watches"),
						 "gdb-watch-icon");
	
	debugger.cpu_registers = cpu_registers_new ();
	debugger.signals = signals_new ();
	debugger.sharedlibs = sharedlibs_new ();

	debugger_set_active (FALSE);
	debugger_set_ready (TRUE);
}

gboolean
debugger_save_yourself (FILE * stream)
{
	DEBUG_PRINT ("In function: debugger_save_yourself()");
	
	if (!breakpoints_dbase_save_yourself
	    (debugger.breakpoints_dbase, stream))
		return FALSE;
	if (!cpu_registers_save_yourself (debugger.cpu_registers, stream))
		return FALSE;
	if (!signals_save_yourself (debugger.signals, stream))
		return FALSE;
	if (!sharedlibs_save_yourself (debugger.sharedlibs, stream))
		return FALSE;
	return TRUE;
}

/* TODO
gboolean
debugger_load_yourself (PropsID stream)
{

#ifdef ANJUTA_DEBUG_DEBUGGER
	g_message ("In function: debugger_load_yourself()");
#endif
	
	if (!breakpoints_dbase_load_yourself
	    (debugger.breakpoints_dbase, stream))
		return FALSE;
	if (!cpu_registers_load_yourself (debugger.cpu_registers, stream))
		return FALSE;
	if (!signals_load_yourself (debugger.signals, stream))
		return FALSE;
	if (!sharedlibs_load_yourself (debugger.sharedlibs, stream))
		return FALSE;
	return TRUE;
}
*/

void
debugger_load_executable (const gchar *prog)
{
	gchar *command, *dir, *msg;

	DEBUG_PRINT ("In function: debugger_load_executable()");

	g_return_if_fail (prog != NULL);

	msg = g_strconcat (_("Loading Executable: "), prog, NULL);
	gdb_util_append_message (ANJUTA_PLUGIN (debugger.plugin), msg);
	g_free (msg);

	command = g_strconcat ("file ", prog, NULL);
	dir = g_path_get_dirname (prog);
/* TODO
	anjuta_set_execution_dir(dir);
*/
	g_free (dir);
	debugger_put_cmd_in_queqe (command, DB_CMD_ALL, NULL, NULL);
	g_free (command);
	debugger.starting = TRUE;

	debugger_put_cmd_in_queqe ("info sharedlibrary", DB_CMD_NONE,
							   sharedlibs_update, debugger.sharedlibs);
	expr_watch_cmd_queqe (debugger.watch);
	stack_trace_clear (debugger.stack);
	cpu_registers_clear (debugger.cpu_registers);
	debugger_execute_cmd_in_queqe ();
}

void
debugger_load_core (const gchar *core)
{
	gchar *command, *dir, *msg;

	DEBUG_PRINT ("In function: debugger_load_core()");

	g_return_if_fail (core != NULL);

	msg = g_strconcat (_("Loading Core: "), core, NULL);
	gdb_util_append_message (ANJUTA_PLUGIN (debugger.plugin), msg);
	g_free (msg);

	command = g_strconcat ("core ", core, NULL);
	dir = g_path_get_dirname (core);
/* TODO
	anjuta_set_execution_dir(dir);
*/
	g_free (dir);

	debugger_put_cmd_in_queqe (command, DB_CMD_ALL, NULL, NULL);
	g_free (command);
	debugger_info_prg();
}

void
debugger_shutdown ()
{
	DEBUG_PRINT ("In function: debugger_shutdown()");

	debugger_stop_terminal();
	
	debugger_set_active (FALSE);
	debugger_set_ready (FALSE);
	
	/* Widgets are automatically removed from shell when they are destroyed */
	if (debugger.breakpoints_dbase)
		breakpoints_dbase_destroy (debugger.breakpoints_dbase);
	if (debugger.locals)
		locals_destroy (debugger.locals);
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
}

void
debugger_set_active (gboolean state)
{
	if (debugger.active != state)
	{
		// DEBUG_PRINT ("In function: debugger_set_active()");
		debugger.active = state;
		debugger_update_controls ();
	}
}

void
debugger_set_ready (gboolean state)
{
	if (debugger.ready != state)
	{
		// DEBUG_PRINT ("In function: debugger_set_ready()");
		debugger.ready = state;
		/*	if (debugger.cmd_queue == NULL) */
		debugger_update_controls ();
		g_signal_emit_by_name (G_OBJECT (debugger.plugin), "busy", state);
	}
}

gboolean
debugger_is_ready ()
{
	// DEBUG_PRINT ("In function: debugger_is_ready()");
	return debugger.ready;
}

gboolean
debugger_is_active ()
{
	// DEBUG_PRINT ("In function: debugger_is_active()");
	return debugger.active;
}

static DebuggerCommand *
debugger_get_next_command ()
{
	DebuggerCommand *dc;

	DEBUG_PRINT ("In function: debugger_get_next_command()");
	
	if (debugger.cmd_queqe)
	{
		dc = g_list_nth_data (debugger.cmd_queqe, 0);
		debugger.cmd_queqe = g_list_remove (debugger.cmd_queqe, dc);
	}
	else
		dc = NULL;
	return dc;
}

static void
debugger_set_next_command ()
{
	DebuggerCommand *dc;

	DEBUG_PRINT ("In function: debugger_set_next_command()");
	
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

/* Adds a command to debugger queue if the debugger is both
   ready and active */
void
debugger_put_cmd_in_queqe (const gchar cmd[], gint flags,
			   void (*parser) (GList * outputs, gpointer data), gpointer data)
{
	DebuggerCommand *dc;

	DEBUG_PRINT ("In function: debugger_put_cmd_in_queqe()");
	
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

static void
debugger_clear_cmd_queqe ()
{
	GList *node;

	DEBUG_PRINT ("In function: debugger_clear_cmd_queqe()");
	
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

static void
debugger_clear_buffers ()
{
	gint i;
	/* Clear the Output Buffer */

	DEBUG_PRINT ("In function: debugger_clear_buffers()");
	
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
	DEBUG_PRINT ("In function: debugger_execute_cmd_in_queqe()");
	
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
debugger_start (const gchar * prog)
{
	gchar *command_str, *dir, *tmp, *text, *msg;
	gchar *exec_dir;
	gboolean ret;
	GList *list, *node;

	DEBUG_PRINT ("In function: debugger_start()");

	if (anjuta_util_prog_is_installed ("gdb", TRUE) == FALSE)
		return;
	if (debugger_is_active ())
		return;

	debugger_set_active (TRUE);
	debugger_set_ready (FALSE);
	debugger_clear_cmd_queqe ();
	locals_clear (debugger.locals);
	debugger.child_pid = -1;
	debugger.term_is_running = FALSE;
	debugger.term_pid = -1;

	tmp = g_strconcat (PACKAGE_DATA_DIR, "/", "gdb.init", NULL);
	if (g_file_test (tmp, G_FILE_TEST_IS_REGULAR) == FALSE)
	{
		anjuta_util_dialog_error (GTK_WINDOW (debugger.plugin->parent.shell),
				_("Unable to find: %s.\n"
				"Unable to initialize debugger.\n"
				"Make sure Anjuta is installed correctly."), tmp);
		g_free (tmp);
		debugger_set_active (FALSE);
		debugger_set_ready (FALSE);
		return;
	}
	g_free (tmp);

/* TODO	exec_dir = project_dbase_get_module_dir (app->project_dbase, MODULE_SOURCE); */
	exec_dir = NULL; /* TODO remove this line */
	if (exec_dir)
	{
		dir = g_strconcat (" -directory=", exec_dir, NULL);
		g_free (exec_dir);
	}
	else
		dir = g_strdup (" ");
	
/* TODO	list = src_paths_get_paths(app->src_paths); */
	list = NULL; /* TODO: remove this line */
	node = list;
	while (node)
	{
		text = node->data;
		if (text[0] == '/')
			tmp = g_strconcat (dir, " -directory=", text, NULL);
		else
		{
/* TODO
			if (app->project_dbase->project_is_open)
				tmp =
					g_strconcat (dir, " -directory=",
						     app->project_dbase->
						     top_proj_dir, "/", text,
						     NULL);
			else
*/
				tmp =
					g_strconcat (dir, " -directory=",
						     text, NULL);
		}
		g_free (dir);
		dir = tmp;
		node = g_list_next (node);
	}
	anjuta_util_glist_strings_free (list);
	if (prog && strlen(prog) > 0)
	{
		tmp = g_path_get_dirname (prog);
		chdir (tmp);
/* TODO
		anjuta_set_execution_dir (tmp);
*/
		command_str =
			g_strdup_printf
			("gdb -q -f -n %s -cd=%s -x %s/gdb.init %s", dir, tmp,
			 PACKAGE_DATA_DIR, prog);
		g_free (tmp);
	}
	else
	{
		command_str =
			g_strdup_printf ("gdb -q -f -n %s -x %s/gdb.init ",
					 dir, PACKAGE_DATA_DIR);
	}
	g_free (dir);
	debugger.starting = TRUE;

	// Prepare for launch.
	AnjutaLauncher *launcher = debugger.plugin->launcher;
	g_signal_connect (G_OBJECT (launcher), "child-exited",
					  G_CALLBACK (on_gdb_terminated), NULL);
	ret = anjuta_launcher_execute (launcher, command_str,
								   on_gdb_output_arrived, NULL);
	anjuta_launcher_set_encoding (launcher, "ISO-8859-1");

	gdb_util_clear_messages (ANJUTA_PLUGIN (debugger.plugin));

	if (ret == TRUE)
	{
/* TODO		anjuta_update_app_status (TRUE, _("Debugger")); */
		gdb_util_append_message (ANJUTA_PLUGIN (debugger.plugin),
								 _("Getting ready to start debugging session ..."));
		if (prog)
		{
			msg = g_strconcat (_("Loading Executable: "), prog, NULL);
			gdb_util_append_message (ANJUTA_PLUGIN (debugger.plugin), msg);
			g_free (msg);
		}
		else
		{
			gdb_util_append_message (ANJUTA_PLUGIN (debugger.plugin),
									 _("No executable specified."));
			gdb_util_append_message (ANJUTA_PLUGIN (debugger.plugin),
									 _("Open an executable or attach to a process to start debugging."));
		}
	}
	else
	{
		gdb_util_append_message (ANJUTA_PLUGIN (debugger.plugin),
								 _("There was an error whilst launching the debugger."));
		gdb_util_append_message (ANJUTA_PLUGIN (debugger.plugin),
								 _("Make sure 'gdb' is installed on the system.\n"));
	}
	gdb_util_show_messages (ANJUTA_PLUGIN (debugger.plugin));
	g_free (command_str);
}

static void
gdb_stdout_line_arrived (const gchar * chars)
{
	gint i = 0;

	// DEBUG_PRINT ("In function: gdb_stdout_line_arrived()");
	while (chars[i])
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
		i++;
	}
}

static void
gdb_stderr_line_arrived (const gchar * chars)
{
	gint i;

	// DEBUG_PRINT ("In function: gdb_stderr_line_arrived()");
	
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

static void
on_gdb_output_arrived (AnjutaLauncher *launcher,
					   AnjutaLauncherOutputType output_type,
					   const gchar *chars, gpointer data)
{
	switch (output_type)
	{
	case ANJUTA_LAUNCHER_OUTPUT_STDERR:
		gdb_stderr_line_arrived (chars);
		break;
	case ANJUTA_LAUNCHER_OUTPUT_STDOUT:
		gdb_stdout_line_arrived (chars);
		break;
	default:
		break;
	}
}

void
debugger_stdo_flush (void)
{
	guint lineno;
	gchar *filename, *line, *msg;
	GObject *obj = NULL;
	IAnjutaDocumentManager *docman = NULL;

	// DEBUG_PRINT ("In function: debugger_stdo_flush()");
	
	line = debugger.stdo_line;
	if (strlen (line) == 0)
	{
		debugger.gdb_stdo_outputs =
			g_list_append (debugger.gdb_stdo_outputs,
				       g_strdup (" "));
		if (debugger.current_cmd.flags & DB_CMD_SO_MESG)
		{
			gdb_util_append_message (ANJUTA_PLUGIN (debugger.plugin), "");
		}
		return;
	}
	if (line[0] == '\032' && line[1] == '\032')
	{
		gdb_util_parse_error_line (&(line[2]), &filename, &lineno);
		if (filename)
		{
			obj = anjuta_shell_get_object (debugger.plugin->parent.shell,
					"IAnjutaDocumentManager", NULL /* TODO */);
			docman = IANJUTA_DOCUMENT_MANAGER (obj);
			ianjuta_document_manager_goto_file_line (docman, filename, lineno,
					NULL /* TODO */);
			g_free (filename);
		}
	}
	else if (strcmp (line, GDB_PROMPT) == 0)
	{
		GList *list;
		/* Ready */
		debugger_set_ready (TRUE);

		/* Check if the Program has exited / terminated */
		list = debugger.gdb_stdo_outputs;
		while (list)
		{
			if (strstr ((gchar *)list->data, "No such process") != NULL ||
				strstr ((gchar *)list->data, "Program exited") != NULL ||
				strstr ((gchar *)list->data, "Program terminated") != NULL)
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
				msg = g_strconcat (_("Error: "), (gchar *) list->data, NULL);
				gdb_util_append_message (ANJUTA_PLUGIN (debugger.plugin), msg);
				g_free (msg);
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
			breakpoints_dbase_set_all (debugger.breakpoints_dbase);
			debugger_put_cmd_in_queqe ("info signals", DB_CMD_NONE,
						   				signals_update, debugger.signals);
			gdb_util_append_message (ANJUTA_PLUGIN (debugger.plugin), _("Debugger is ready.\n"));
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
						g_list_append (debugger.gdb_stdo_outputs,
								anjuta_util_convert_to_utf8 (line));
				}
				if (debugger.current_cmd.
				    flags & DB_CMD_SO_MESG)
				{
					gdb_util_append_message (ANJUTA_PLUGIN (debugger.plugin), line);
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
					g_list_append (debugger.gdb_stdo_outputs,
							anjuta_util_convert_to_utf8 (line));
			}
			if (debugger.current_cmd.flags & DB_CMD_SO_MESG)
			{
				gdb_util_append_message (ANJUTA_PLUGIN (debugger.plugin), line);
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

	// DEBUG_PRINT ("In function: debugger_stde_flush()");
	
	line = debugger.stde_line;
	if (strlen (line) == 0)
		return;
	debugger.gdb_stde_outputs =
		g_list_append (debugger.gdb_stde_outputs, g_strdup (line));
	/* Clear the line buffer */
	strcpy (debugger.stde_line, "");
	debugger.stde_cur_char_pos = 0;
}

static void
on_gdb_terminated (AnjutaLauncher *launcher,
				gint child_pid, gint status, gulong t, gpointer data)
{
	g_signal_handlers_disconnect_by_func (G_OBJECT (launcher),
										  G_CALLBACK (on_gdb_terminated),
										  data);
	
	DEBUG_PRINT ("In function: gdb_terminated()");
	
	/* Clear the command queue & Buffer */
	debugger_clear_buffers ();
	debugger_stop_terminal ();

	/* Good Bye message */
	gdb_util_append_message (ANJUTA_PLUGIN (debugger.plugin),
							 "\nWell, did you find the BUG? Debugging session completed.\n");

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
/* TODO	anjuta_update_app_status (TRUE, NULL); */
}

static void
debugger_command (const gchar * com)
{
	gchar *cmd;
	
	g_return_if_fail (com != NULL);
	DEBUG_PRINT ("In function: debugger_command()");
	
	if (debugger_is_active () == FALSE)
		return;		/* There is no point in accepting commands when */
	if (debugger_is_ready () == FALSE)
		return;		/* gdb is not running or it is busy */

	DEBUG_PRINT ("Executing gdb command %s\n",com);

	debugger_set_ready (FALSE);
	cmd = g_strconcat (com, "\n", NULL);
	anjuta_launcher_send_stdin (debugger.plugin->launcher, cmd);
	g_free (cmd);
}

void
debugger_update_controls ()
{
	DEBUG_PRINT ("In function: debugger_update_controls()");

	breakpoints_dbase_update_controls (debugger.breakpoints_dbase);
	expr_watch_update_controls (debugger.watch);
	stack_trace_update_controls (debugger.stack);
	signals_update_controls (debugger.signals);
	gdb_plugin_update_ui (debugger.plugin);
}

void
debugger_dialog_message (GList * lines, gpointer data)
{
	GtkWindow *parent;
	
	DEBUG_PRINT ("In function: debugger_dialog_message()");
	
	if (g_list_length (lines) < 1)
		return;
	parent = GTK_WINDOW (ANJUTA_PLUGIN (debugger.plugin)->shell);
	anjuta_info_show_list (parent, lines, 0, 0);
}

void
debugger_dialog_error (GList * lines, gpointer data)
{
	gchar *ptr, *tmp;
	gint i;

	DEBUG_PRINT ("In function: debugger_dialog_error()");
	
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
	if (strstr (ptr, ": No such file or directory.") == NULL)
	{
		anjuta_util_dialog_error (GTK_WINDOW (debugger.plugin->parent.shell), ptr);
	}
	g_free (ptr);
}

static void
on_debugger_terminal_terminated (int status, gpointer data)
{
	DEBUG_PRINT ("In function: on_debugger_terminal_terminated()");
	
	debugger.term_is_running = FALSE;
	debugger.term_pid = -1;
	
	if (debugger.prog_is_running)
		debugger_stop_program ();
}

gchar *
debugger_start_terminal ()
{
	AnjutaPreferences *pr;
	GtkWindow *parent;
	gchar *file, *cmd, *encoded_cmd;
	gchar  dev_name[100];
	gint   count, idx;
	FILE  *fp;
	pid_t  pid;
	gchar *argv[20];
	GList *args, *node;

	DEBUG_PRINT ("In function: debugger_start_terminal()");
	
	if (debugger_is_active () == FALSE)
		return NULL;
	if (debugger_is_ready () == FALSE)
		return NULL;
	if (debugger.prog_is_running == TRUE)
		return NULL;
	if (debugger.term_is_running == TRUE)
		return NULL;
	if (anjuta_util_prog_is_installed ("anjuta_launcher", TRUE) == FALSE)
		return NULL;
	
	parent = GTK_WINDOW (ANJUTA_PLUGIN (debugger.plugin)->shell);
	pr = anjuta_shell_get_preferences (ANJUTA_PLUGIN (debugger.plugin)->shell,
									   NULL);
	count = 0;
	file = anjuta_util_get_a_tmp_file ();
	
	while (g_file_test  (file, G_FILE_TEST_IS_REGULAR))
	{
		g_free (file);
		file = anjuta_util_get_a_tmp_file ();
		if (count++ > 100)
			goto error;
	}
	if (mkfifo (file, 0664) < 0)
		goto error;

	debugger.term_is_running = TRUE;
	cmd = g_strconcat ("anjuta_launcher --__debug_terminal ", file, NULL);
	encoded_cmd = anjuta_util_escape_quotes(cmd);
	g_free (cmd);
	
	cmd = anjuta_preferences_get (pr, PREF_TERMINAL_COMMAND);
	if (!cmd)
	{
		cmd = g_strdup ("gnome-terminal -e \"%s\"");
	}
	
	if (cmd)
	{
		gchar *final_cmd;
		if (strstr (cmd, "%s") != NULL)
		{
			final_cmd = g_strdup_printf (cmd, encoded_cmd);
		}
		else
		{
			final_cmd = g_strconcat (cmd, " ", encoded_cmd, NULL);
		}
		g_free (cmd);
		cmd = final_cmd;
	}
	g_free (encoded_cmd);
	
	args = anjuta_util_parse_args_from_string (cmd);
    
    /* Fix gnome-terminal1 and gnome-terminal2 confusion */
    if (g_list_length(args) > 0 &&
		strcmp ((gchar*)args->data, "gnome-terminal") == 0)
    {
        GList* node;
        node = g_list_next(args);
        /* Terminal command is gnome-terminal */
        if (debugger.gnome_terminal_type == 1) {
            /* Remove any --disable-factory option, if present */
            while (node) {
                if (strcmp ((gchar*)args->data, "--disable-factory") == 0) {
                    g_free (node->data);
                    args = g_list_remove (args, node);
                    break;
                }
                node = g_list_next (node);
            }
        } else if (debugger.gnome_terminal_type == 2) {
            /* Add --disable-factory option, if not present */
            gboolean found = 0;
            while (node) {
                if (strcmp ((gchar*)args->data, "--disable-factory") == 0) {
                    found = 1;
                    break;
                }
                node = g_list_next (node);
            }
            if (!found) {
                gchar* arg = g_strdup ("--disable-factory");
                args = g_list_insert (args, arg, 1);
            }
        }
    }
        
	DEBUG_PRINT ("Terminal commad: [%s]\n", cmd);

	g_free (cmd);
	
	/* Rearrange the args to be passed to fork */
	node = args;
	idx = 0;
	while (node) {
		argv[idx++] = (gchar*)node->data;
		node = g_list_next (node);
	}
	
#ifdef DEBUG
	{
		int i;
		GList *node = args;
		i=0;
		while (node) {
			g_print ("%d) [%s]\n", i++, (char*)node->data);
			node = g_list_next (node);
		}
	}
#endif
	
	argv[idx] = NULL;
	if (idx < 1)
		goto error;
	
	/* With this command SIGCHILD is not emitted */
	/* pid = gnome_execute_async (app->dirs->home, 6, av); */
	/* so using fork instead */
	if ((pid = fork ()) == 0)
	{
		execvp (argv[0], argv);
		g_error (_("Cannot execute gnome-terminal"));
	}
	g_list_foreach (args, (GFunc)g_free, NULL);
	g_list_free (args);

	debugger.term_pid = pid;
	if (pid < 1)
		goto error;
	g_message  ("terminal pid = %d\n", pid);
	anjuta_children_register (pid, on_debugger_terminal_terminated, NULL);

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
	anjuta_util_dialog_error (parent,
							  _("Cannot start terminal for debugging."));
	debugger_stop_terminal ();
	remove (file);
	g_free (file);
	return NULL;
}

static void
debugger_stop_terminal ()
{
	DEBUG_PRINT ("In function: debugger_stop_terminal()");
	
	if (debugger.term_is_running == FALSE)
		return;
	if (debugger.term_pid > 0) {
		anjuta_children_unregister (debugger.term_pid);
		if (kill (debugger.term_pid, SIGTERM) == -1) {
			switch (errno) {
				case EINVAL:
					g_warning ("Invalid signal applied to kill");
					break;
				case ESRCH:
					g_warning ("No such pid [%d] or process has already died",
							   debugger.term_pid);
					break;
				case EPERM:
					g_warning ("No permission to send signal to the process");
					break;
				default:
					g_warning ("Unknow error while kill");
			}
		}
	}
	debugger.term_pid = -1;
	debugger.term_is_running = FALSE;
}

void
debugger_start_program (void)
{
	GtkWindow *parent;
	gchar *term, *cmd, *args;

	DEBUG_PRINT ("In function: debugger_start_program()");
	
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	if (debugger.prog_is_running == TRUE)
		return;

	args = NULL;
	parent = GTK_WINDOW (ANJUTA_PLUGIN (debugger.plugin)->shell);
	anjuta_util_dialog_input (parent, "Program arguments", &args);
	
	if (args == NULL)
		args = g_strdup (" ");

	term = debugger_start_terminal ();
	if (!term)
	{
		gdb_util_append_message (ANJUTA_PLUGIN (debugger.plugin),
								 _("Warning: No debug terminal."
								   " Redirecting stdio to /dev/null.\n"));
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
	gdb_util_append_message (ANJUTA_PLUGIN (debugger.plugin),
							 _("Running program ..."));
	debugger.post_execution_flag = DEBUGGER_NONE;
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

	DEBUG_PRINT ("In function: on_debugger_update_prog_status()");
	
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
	DEBUG_PRINT ("Process recognization string: %s", str);
	
	if ((str = strstr (lines->data, "pid "))) /* For gdb version < ver 5.0 */
	{
		if (sscanf (str, "pid %ld", &pid) != 1)
		{
			error = TRUE;
			goto down;
		}
	}
	else if ((str = strstr (lines->data, "process "))) /* A quick hack for gdb version 5.0 */
	{
		if (sscanf (str, "process %ld", &pid) != 1)
		{
			error = TRUE;
			goto down;
		}
	}
	else if ((str = strstr (lines->data, "thread "))
			&& strstr (lines->data, "(lwp ")) /* A quick hack for threaded program (I)*/
	{
		/* FIXME: Don't know what to do with this variable, but may
		   come handy in future */
		glong thread;
		
		if (sscanf (str, "thread %ld (lwp %ld)", &thread, &pid) != 2)
		{
			error = TRUE;
			goto down;
		}
	}
	else if ((str = strstr (lines->data, "thread "))) /* A quick hack for threaded program (II)*/
	{
		if (sscanf (str, "thread %ld", &pid) != 1)
		{
			error = TRUE;
			goto down;
		}
	}
    else if ((str = strstr(lines->data, "child lwp ")))
	{
		if (sscanf(str, "child lwp %ld", &pid) != 1)
		{
			error = TRUE;
			goto down;
		}
	}
	else /* Nothing known about this particular process recognition string */
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
		debugger_update_controls ();
		debugger_handle_post_execution();
	}
	else
	{
		DEBUG_PRINT ("Process PID = %ld", pid);

		debugger.prog_is_running = TRUE;
		debugger.child_pid = pid;
		stack_trace_set_frame (debugger.stack, 0);
		debugger.child_pid = pid;
		debugger_update_controls ();
		debugger_info_prg();
	}
}

static void
debugger_attach_process_real (pid_t pid)
{
	gchar *buf;

	DEBUG_PRINT ("In function: debugger_attach_process_real()");
	
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	buf = g_strdup_printf (_("Attaching to process: %d"), pid);
	gdb_util_append_message (ANJUTA_PLUGIN (debugger.plugin), buf);
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
debugger_attach_process (pid_t pid)
{
	GtkWindow *parent;
	DEBUG_PRINT ("In function: debugger_attach_process()");
	
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;

	parent = GTK_WINDOW (ANJUTA_PLUGIN (debugger.plugin)->shell);
	if (debugger.prog_is_running == TRUE)
	{
		// TODO: Dialog to be made HIG compliant.
		gchar *mesg;
		GtkWidget *dialog;
		
		mesg = _("A process is already running.\n"
				 "Would you like to terminate it and attach the new process?"),
		dialog = gtk_message_dialog_new (parent,
										 GTK_DIALOG_DESTROY_WITH_PARENT,
										 GTK_MESSAGE_QUESTION,
										 GTK_BUTTONS_YES_NO, mesg);
		if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
			debugger_attach_process_real (pid);
		gtk_widget_destroy (dialog);
	}
	else if (getpid () == pid ||
			 anjuta_launcher_get_child_pid (debugger.plugin->launcher) == pid)
	{
		anjuta_util_dialog_error (parent,
								  _("Anjuta is unable to attach to itself."));
		return;
	}
	else
		debugger_attach_process_real (pid);
}

void
debugger_restart_program ()
{
	DEBUG_PRINT ("In function: debugger_restart_program()");
	
	if (debugger_is_active () == FALSE ||
	    debugger.prog_is_attached == TRUE)
		return;
	debugger.post_execution_flag = DEBUGGER_RERUN_PROGRAM;
	debugger_stop_program();
	/*
	debugger_put_cmd_in_queqe ("tbreak main", DB_CMD_NONE, NULL, NULL);
	debugger_put_cmd_in_queqe ("run >/dev/null 2>/dev/null", DB_CMD_ALL,
				   NULL, NULL);
	debugger_put_cmd_in_queqe ("info program", DB_CMD_NONE,
				   on_debugger_update_prog_status, NULL);
	debugger_put_cmd_in_queqe ("continue", DB_CMD_NONE, NULL, NULL);
	debugger_execute_cmd_in_queqe ();
	*/
}

void
debugger_stop_program ()
{
	DEBUG_PRINT ("In function: debugger_stop_program()");
	
	if (debugger_is_active () == FALSE)
		return;
	if (debugger.prog_is_running == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
	{
		debugger_signal ("SIGKILL", FALSE);
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
	gdb_util_append_message (ANJUTA_PLUGIN (debugger.plugin),
							 _("Program forcefully terminated."));
	debugger_stop_terminal ();
	stack_trace_set_frame (debugger.stack, 0);
}

void
debugger_detach_process ()
{
	gchar *buff;

	DEBUG_PRINT ("In function: debugger_detach_process()");
	
	if (debugger.prog_is_attached == FALSE)
		return;
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	buff =
		g_strdup_printf (_("Detaching the process: %d"),
				 (int) debugger.child_pid);
	gdb_util_append_message (ANJUTA_PLUGIN (debugger.plugin), buff);
	g_free (buff);
	debugger_put_cmd_in_queqe ("detach", DB_CMD_ALL, NULL, NULL);
	debugger_put_cmd_in_queqe ("info program", DB_CMD_NONE,
				   on_debugger_update_prog_status, NULL);
	debugger.prog_is_attached = FALSE;
	debugger_execute_cmd_in_queqe ();
}

static void
debugger_stop_real (void)
{
	DEBUG_PRINT ("In function: debugger_stop()");
	
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
	{
		debugger.post_execution_flag = DEBUGGER_EXIT;
		debugger_stop_program();
	}
	else
	{
		/* if program is attached - detach from it before quiting */
		if (debugger.prog_is_attached == TRUE)
			debugger_put_cmd_in_queqe ("detach", DB_CMD_NONE, NULL, NULL);

		debugger_put_cmd_in_queqe ("quit", DB_CMD_NONE, NULL, NULL);
		debugger_execute_cmd_in_queqe ();
		
	stack_trace_set_frame (debugger.stack, 0);
	debugger_stop_terminal();
	}
}

/* if the program is running this function brings up a dialog
 * box for confirmation to stop the debugger 
 * returns: false if debugger was running and user decided not to stop it,
 * true otherwise
 */
gboolean
debugger_stop ()
{
	gboolean ret = TRUE;
	
	if (debugger.prog_is_running == TRUE)
	{
		GtkWindow *parent;
		GtkWidget *dialog;
		gchar *mesg;
		
		if (debugger.prog_is_attached == TRUE)
			mesg = _("Program is ATTACHED.\n"
				   "Do you still want to stop Debugger?");
		else
			mesg = _("Program is RUNNING.\n"
				   "Do you still want to stop Debugger?");
		parent = GTK_WINDOW (ANJUTA_PLUGIN (debugger.plugin)->shell);
		dialog = gtk_message_dialog_new (parent,
										 GTK_DIALOG_DESTROY_WITH_PARENT,
										 GTK_MESSAGE_QUESTION,
										 GTK_BUTTONS_NONE, mesg);
		gtk_dialog_add_buttons (GTK_DIALOG (dialog),
								GTK_STOCK_CANCEL,	GTK_RESPONSE_NO,
								GTK_STOCK_STOP,		GTK_RESPONSE_YES,
								NULL);
		if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
			debugger_stop_real ();
		else
			ret = FALSE;
		gtk_widget_destroy (dialog);
	}
	else
		debugger_stop_real ();
	return ret;
}

void
debugger_run ()
{
	DEBUG_PRINT ("In function: debugger_run()");
	
	/* Debugger not active - activate it and run program */
	if (debugger_is_active () == FALSE)
		return;
	/* Debugger active but not ready - go back */
	if (debugger_is_ready () == FALSE)
		return;
	
	/* Program not running - start it */
	if (debugger.prog_is_running == FALSE)
	{
		debugger_put_cmd_in_queqe ("tbreak main", DB_CMD_NONE, NULL, NULL);
		debugger_start_program ();
		debugger_put_cmd_in_queqe ("continue", DB_CMD_ALL, NULL, NULL);
		debugger_put_cmd_in_queqe ("info program", DB_CMD_NONE,
								   on_debugger_update_prog_status, NULL);
		debugger_execute_cmd_in_queqe ();
		return;
	}
	/* Program running - continue */
	stack_trace_set_frame (debugger.stack, 0);
	debugger_put_cmd_in_queqe ("continue", DB_CMD_ALL, NULL, NULL);
	debugger_info_prg ();
}

void
debugger_step_in ()
{
	DEBUG_PRINT ("In function: debugger_step_in()");
	
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	if (debugger.prog_is_running == FALSE)
	{
		debugger_put_cmd_in_queqe ("tbreak main", DB_CMD_NONE, NULL, NULL);
		debugger_start_program ();
		debugger_execute_cmd_in_queqe ();
		return;
	}
	stack_trace_set_frame (debugger.stack, 0);
	debugger_put_cmd_in_queqe ("step", DB_CMD_ALL, NULL, NULL);
	debugger_info_prg ();	
}

void
debugger_step_over ()
{
	DEBUG_PRINT ("In function: debugger_step_over()");
	
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	if (debugger.prog_is_running == FALSE)
		return;
	stack_trace_set_frame (debugger.stack, 0);
	debugger_put_cmd_in_queqe ("next", DB_CMD_ALL, NULL, NULL);
	debugger_info_prg();	
}

void
debugger_step_out ()
{
	DEBUG_PRINT ("In function: debugger_step_out()");
	
	if (debugger.prog_is_running == FALSE)
		return;
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	stack_trace_set_frame (debugger.stack, 0);
	debugger_put_cmd_in_queqe ("finish", DB_CMD_ALL, NULL, NULL);
	debugger_info_prg();	
}

void
debugger_toggle_breakpoint (const gchar *file, guint l)
{
	breakpoints_dbase_toggle_breakpoint (debugger.breakpoints_dbase,
										 file, l);
}

void debugger_run_to_location(const gchar *loc)
{
	gchar *buff;

	DEBUG_PRINT ("In function: debugger_run_to_location()");
	
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	
	if (debugger.prog_is_running == FALSE)
	{
		buff = g_strconcat ("tbreak ", loc, NULL);
		debugger_put_cmd_in_queqe (buff, DB_CMD_NONE, NULL, NULL);
		g_free (buff);
		debugger_start_program ();
		debugger_execute_cmd_in_queqe ();
		return;
	}
	stack_trace_set_frame (debugger.stack, 0);
	buff = g_strdup_printf ("until %s", loc);
	debugger_put_cmd_in_queqe (buff, DB_CMD_ALL, NULL, NULL);
	g_free (buff);
	debugger_info_prg();
}

void
debugger_enable_breakpoint (gint id)
{
	gchar buff[20];

	DEBUG_PRINT ("In function: debugger_enable_breakpoint()");
	
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	sprintf (buff, "enable %d", id);
	debugger_put_cmd_in_queqe (buff, DB_CMD_ALL, NULL, NULL);
	/*
	debugger_put_cmd_in_queqe ("info breakpoints", DB_CMD_NONE,
				   breakpoints_dbase_update,
				   debugger.breakpoints_dbase);
	*/
	debugger_execute_cmd_in_queqe ();
}

void
debugger_enable_all_breakpoints ()
{
	DEBUG_PRINT ("In function: debugger_enable_all_breakpoints()");
	
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	debugger_put_cmd_in_queqe ("enable breakpoints", DB_CMD_ALL, NULL,
				   NULL);
	/*
	debugger_put_cmd_in_queqe ("info breakpoints", DB_CMD_NONE,
				   breakpoints_dbase_update,
				   debugger.breakpoints_dbase);
	*/
	gdb_util_append_message (ANJUTA_PLUGIN (debugger.plugin),
							 _("All breakpoints enabled:"));
	debugger_execute_cmd_in_queqe ();
}

void
debugger_disable_breakpoint (gint id)
{
	gchar buff[20];

	DEBUG_PRINT ("In function: debugger_disable_breakpoint()");
	
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	sprintf (buff, "disable %d", id);
	debugger_put_cmd_in_queqe (buff, DB_CMD_ALL, NULL, NULL);
	/*
	debugger_put_cmd_in_queqe ("info breakpoints", DB_CMD_NONE,
				   breakpoints_dbase_update,
				   debugger.breakpoints_dbase);
	*/
	debugger_execute_cmd_in_queqe ();
}

void
debugger_disable_all_breakpoints ()
{
	DEBUG_PRINT ("In function: debugger_disable_all_breakpoints()");
	
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	debugger_put_cmd_in_queqe ("disable breakpoints", DB_CMD_ALL, NULL,
				   NULL);
	/*
	debugger_put_cmd_in_queqe ("info breakpoints", DB_CMD_NONE,
				   breakpoints_dbase_update,
				   debugger.breakpoints_dbase);
	*/
	gdb_util_append_message (ANJUTA_PLUGIN (debugger.plugin), _("All breakpoints disabled:"));
	debugger_execute_cmd_in_queqe ();
}

void
debugger_delete_breakpoint (gint id)
{
	gchar buff[20];
	DEBUG_PRINT ("In function: debugger_delete_breakpoint()");
	
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	sprintf (buff, "delete %d", id);
	debugger_put_cmd_in_queqe (buff, DB_CMD_ALL, NULL, NULL);
	/*
	debugger_put_cmd_in_queqe ("info breakpoints", DB_CMD_NONE,
				   breakpoints_dbase_update,
				   debugger.breakpoints_dbase);
	*/
	debugger_execute_cmd_in_queqe ();
}

void
debugger_delete_all_breakpoints ()
{
	DEBUG_PRINT ("In function: debugger_delete_all_breakpoints()");
	
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	debugger_put_cmd_in_queqe ("delete breakpoints", DB_CMD_ALL, NULL,
				   NULL);
	/*
	debugger_put_cmd_in_queqe ("info breakpoints", DB_CMD_NONE,
				   breakpoints_dbase_update,
				   debugger.breakpoints_dbase);
	*/
	gdb_util_append_message (ANJUTA_PLUGIN (debugger.plugin),
							 _("All breakpoints deleted:"));
	debugger_execute_cmd_in_queqe ();
}

void
debugger_custom_command ()
{
	DEBUG_PRINT ("In function: debugger_custom_command()");
/* TODO
	anjuta_not_implemented (__FILE__, __LINE__);
*/
}

void
debugger_interrupt ()
{
	gchar *buff;
	DEBUG_PRINT ("In function: debugger_interrupt()");
	
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready ())
		return;
	if (debugger.prog_is_running == FALSE)
		return;
	if (debugger.child_pid < 1)
		return;
	buff = g_strdup_printf (_("Interrupting the process: %d"),
			(int) debugger.child_pid);
	gdb_util_append_message (ANJUTA_PLUGIN (debugger.plugin), buff);
	g_free (buff);
	stack_trace_set_frame (debugger.stack, 0);
	debugger_signal ("SIGINT", FALSE);
}

void
debugger_signal (const gchar *sig, gboolean show_msg)	/* eg:- "SIGTERM" */
{
	gchar *buff;
	gchar *cmd;

	DEBUG_PRINT ("In function: debugger_signal()");
	
	if (debugger_is_active () == FALSE)
		return;
	if (debugger.prog_is_running == FALSE)
		return;
	if (debugger.child_pid < 1)
	{
		DEBUG_PRINT ("Not sending signal - pid not known\n");
		return;
	}

	if (show_msg)
	{
		buff = g_strdup_printf (_("Sending signal %s to the process: %d"),
								sig, (int) debugger.child_pid);
		gdb_util_append_message (ANJUTA_PLUGIN (debugger.plugin), buff);
		g_free (buff);
	}

	if (debugger_is_ready ())
	{
		cmd = g_strconcat ("signal ", sig, NULL);
		stack_trace_set_frame (debugger.stack, 0);
		debugger_put_cmd_in_queqe (cmd, DB_CMD_ALL, NULL, NULL);
		debugger_put_cmd_in_queqe ("info program", DB_CMD_NONE,
								   on_debugger_update_prog_status,
								   NULL);
		g_free (cmd);
		debugger_execute_cmd_in_queqe ();
	}
	else
	{
		GtkWindow *parent;
		int status;
		
		parent = GTK_WINDOW (ANJUTA_PLUGIN (debugger.plugin)->shell);
		status = gdb_util_kill_process (debugger.child_pid, sig);
		if (status != 0 && show_msg)
			anjuta_util_dialog_error (parent,
									  _("Error whilst signaling the process."));
	}
}

/* TODO
void debugger_reload_session_breakpoints( ProjectDBase *p )
{
	g_return_if_fail( NULL != p );
	breakpoints_dbase_load ( debugger.breakpoints_dbase, p );
}

void 
debugger_save_session_breakpoints( ProjectDBase *p )
{
	g_return_if_fail( NULL != p );
	breakpoints_dbase_save ( debugger.breakpoints_dbase, p );
}
*/

static void
locals_update_controls(void)
{
	debugger_put_cmd_in_queqe ("info locals",
				   DB_CMD_NONE/*DB_CMD_SE_MESG | DB_CMD_SE_DIALOG*/,
				   debugger_info_locals_cb, NULL);
}

static void
debugger_info_prg(void)
{
	// try to speedup
	debugger_put_cmd_in_queqe ("info sharedlibrary", DB_CMD_NONE,
							   sharedlibs_update, debugger.sharedlibs);
	expr_watch_cmd_queqe (debugger.watch);
	debugger_put_cmd_in_queqe ("info registers", DB_CMD_NONE,
							   cpu_registers_update,
							   debugger.cpu_registers);
	debugger_put_cmd_in_queqe ("backtrace", DB_CMD_NONE,
							   stack_trace_update, debugger.stack);
	locals_update_controls();

	debugger_execute_cmd_in_queqe ();
}

static void debugger_info_locals_cb (GList* list, gpointer data)
{
	locals_update (debugger.locals, list, data);
}

static void debugger_handle_post_execution()
{
	switch (debugger.post_execution_flag) {
		case DEBUGGER_NONE:
			break;
		case DEBUGGER_EXIT:
			debugger_stop();
			break;
		case DEBUGGER_RERUN_PROGRAM:
			debugger_run();
			break;
		default:
			g_warning ("Execution should not reach here");
	}
}


void debugger_query_execute (void)
{
	debugger_execute_cmd_in_queqe ();
}

static void query_set_cmd (const gchar *cmd, gboolean state)
{
	gchar buffer[50];
	gchar *tmp = g_stpcpy (buffer, cmd);
	strcpy (tmp, state ? "on" : "off");
	debugger_put_cmd_in_queqe (buffer, DB_CMD_NONE, NULL, NULL);
}

static void query_set_verbose (gboolean state)
{
	query_set_cmd ("set verbose ", state);
}

static void query_set_print_staticmembers (gboolean state)
{
	query_set_cmd ("set print static-members ", state);
}

static void query_set_print_pretty (gboolean state)
{
	query_set_cmd ("set print pretty ", state);
}

void debugger_query_evaluate_expr_tip (const gchar *expr,
			void (*parser) (GList *outputs, gpointer data), gpointer data)
{
	query_set_verbose (FALSE);
	query_set_print_staticmembers (FALSE);
	gchar *printcmd = g_strconcat ("print ", expr, NULL);
	debugger_put_cmd_in_queqe (printcmd, DB_CMD_NONE, parser, data);
	query_set_verbose (TRUE);
	query_set_print_staticmembers (TRUE);
	g_free (printcmd);
}

void debugger_query_evaluate_expr (const gchar *expr,
			void (*parser) (GList *outputs, gpointer data), gpointer data)
{
	query_set_print_pretty (TRUE);
	query_set_verbose (FALSE);
	gchar *printcmd = g_strconcat ("print ", expr, NULL);
	debugger_put_cmd_in_queqe (printcmd, DB_CMD_SE_MESG | DB_CMD_SE_DIALOG,
				parser, data);
	query_set_print_pretty (FALSE);
	query_set_verbose (TRUE);
	g_free (printcmd);
}

static void debugger_query_info_cmd (
			const gchar *cmd, void (*parser) (GList *outputs, gpointer data))
{
	query_set_print_pretty (TRUE);
	query_set_verbose (FALSE);
	debugger_put_cmd_in_queqe (cmd, DB_CMD_SE_MESG | DB_CMD_SE_DIALOG,
				parser, NULL);
	query_set_print_pretty (FALSE);
	query_set_verbose (TRUE);
}

void debugger_query_info_target (void (*parser) (GList *outputs, gpointer data))
{
	debugger_query_info_cmd ("info target", parser);
}

void debugger_query_info_program (
			void (*parser) (GList *outputs, gpointer data))
{
	query_set_print_pretty (TRUE);
	query_set_verbose (FALSE);
	debugger_put_cmd_in_queqe ("info program",
				   DB_CMD_SE_MESG | DB_CMD_SE_DIALOG, parser, NULL);
	debugger_put_cmd_in_queqe ("info program",
				   DB_CMD_NONE, on_debugger_update_prog_status, NULL);
	query_set_print_pretty (FALSE);
	query_set_verbose (TRUE);
}

void debugger_query_info_udot (
			void (*parser) (GList *outputs, gpointer data))
{
	debugger_query_info_cmd ("info udot", parser);
}

void debugger_query_info_threads (
			void (*parser) (GList *outputs, gpointer data))
{
	debugger_query_info_cmd ("info threads", parser);
}

void debugger_query_info_variables (
			void (*parser) (GList *outputs, gpointer data))
{
	debugger_query_info_cmd ("info variables", parser);
}

void debugger_query_info_locals (
			void (*parser) (GList *outputs, gpointer data))
{
	debugger_query_info_cmd ("info locals", parser);
}

void debugger_query_info_frame (
			void (*parser) (GList *outputs, gpointer data))
{
	debugger_query_info_cmd ("info frame", parser);
}

void debugger_query_info_args (
			void (*parser) (GList *outputs, gpointer data))
{
	debugger_query_info_cmd ("info args", parser);
}
