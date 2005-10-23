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

#include <glib.h>

#include <libanjuta/anjuta-launcher.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-children.h>
#include <libanjuta/anjuta-marshal.h>

#include "debugger.h"
#include "utilities.h"

#define GDB_PROMPT  "(gdb)"
#define PREF_TERMINAL_COMMAND "anjuta.command.terminal"
#define FILE_BUFFER_SIZE 1024

enum {
	DEBUGGER_NONE,
	DEBUGGER_EXIT,
	DEBUGGER_RERUN_PROGRAM
};

enum
{
	PROGRAM_RUNNING_SIGNAL,
	PROGRAM_EXITED_SIGNAL,
	PROGRAM_STOPPED_SIGNAL,
	RESULTS_ARRIVED_SIGNAL,
	LOCATION_CHANGED_SIGNAL,
	LAST_SIGNAL
};

struct _DebuggerPriv
{
	GtkWindow *parent_win;
	
	DebuggerOutputFunc output_callback;
	gpointer output_user_data;
	
	GList *search_dirs;
	
	gboolean prog_is_running;
	gboolean prog_is_attached;
	gboolean debugger_is_ready;
	gint post_execution_flag;

	AnjutaLauncher *launcher;
	GString *stdo_line;
	GString *stde_line;
	GList *stdo_lines;
	
	/* GDB command queue */
	GList *cmd_queqe;
	DebuggerCommand current_cmd;
	gboolean skip_next_prompt;
	gboolean command_output_sent;
	
	gboolean starting;
	gboolean term_is_running;
	pid_t term_pid;
    gint gnome_terminal_type;
};

gpointer parent_class;
static guint debugger_signals[LAST_SIGNAL] = { 0 };

static void debugger_start (Debugger *debugger, const GList *search_dirs,
							const gchar *prog, gboolean is_libtool_prog);
static gboolean debugger_stop (Debugger *debugger);

static gchar *debugger_start_terminal (Debugger *debugger);
static void debugger_stop_terminal (Debugger *debugger);

static void debugger_queue_clear (Debugger *debugger);
static void debugger_queue_execute_command (Debugger *debugger);

static void gdb_stdout_line_arrived (Debugger *debugger, const gchar * line);
static void gdb_stderr_line_arrived (Debugger *debugger, const gchar * line);
static void debugger_stdo_flush (Debugger *debugger);
static void debugger_stde_flush (Debugger *debugger);
static void on_gdb_output_arrived (AnjutaLauncher *launcher,
								   AnjutaLauncherOutputType output_type,
								   const gchar *chars, gpointer data);
static void on_gdb_terminated (AnjutaLauncher *launcher,
							   gint child_pid, gint status,
							   gulong t, gpointer data);

static void debugger_class_init (DebuggerClass *klass);
static void debugger_instance_init (Debugger *debugger);

GType
debugger_get_type (void)
{
	static GType obj_type = 0;
	
	if (!obj_type)
	{
		static const GTypeInfo obj_info = 
		{
			sizeof (DebuggerClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) debugger_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,           /* class_data */
			sizeof (Debugger),
			0,              /* n_preallocs */
			(GInstanceInitFunc) debugger_instance_init,
			NULL            /* value_table */
		};
		obj_type = g_type_register_static (G_TYPE_OBJECT,
		                                   "Debugger", &obj_info, 0);
	}
	return obj_type;
}

static void
debugger_dispose (GObject *obj)
{
	Debugger *debugger = DEBUGGER (obj);
	
	DEBUG_PRINT ("In function: debugger_shutdown()");

	if (debugger->priv->launcher)
	{
		debugger_stop_terminal(debugger);
		g_object_unref (debugger->priv->launcher);
		debugger->priv->launcher = NULL;
		debugger_queue_clear (debugger);
		
		g_list_foreach (debugger->priv->search_dirs, (GFunc)g_free, NULL);
		g_list_free (debugger->priv->search_dirs);
		
		/* Good Bye message */
		debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
								   "Debugging session completed.\n",
								   debugger->priv->output_user_data);
	}
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
debugger_finalize (GObject *obj)
{
	Debugger *debugger = DEBUGGER (obj);
	g_string_free (debugger->priv->stdo_line, TRUE);
	g_string_free (debugger->priv->stde_line, TRUE);
	g_free (debugger->priv);
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
debugger_class_init (DebuggerClass * klass)
{
	GObjectClass *object_class;
	
	g_return_if_fail (klass != NULL);
	object_class = (GObjectClass *) klass;
	
	DEBUG_PRINT ("Initializing debugger class");
	
	parent_class = g_type_class_peek_parent (klass);
	debugger_signals[PROGRAM_RUNNING_SIGNAL] =
		g_signal_new ("program-running",
					G_TYPE_FROM_CLASS (object_class),
					G_SIGNAL_RUN_FIRST,
					G_STRUCT_OFFSET (DebuggerClass,
									 program_running_signal),
					NULL, NULL,
					anjuta_cclosure_marshal_VOID__VOID,
					G_TYPE_NONE, 0);
	
	debugger_signals[PROGRAM_EXITED_SIGNAL] =
		g_signal_new ("program-exited",
					G_TYPE_FROM_CLASS (object_class),
					G_SIGNAL_RUN_FIRST,
					G_STRUCT_OFFSET (DebuggerClass,
									 program_exited_signal),
					NULL, NULL,
					anjuta_cclosure_marshal_VOID__POINTER,
					G_TYPE_NONE, 1, G_TYPE_POINTER);
	
	debugger_signals[PROGRAM_STOPPED_SIGNAL] =
		g_signal_new ("program-stopped",
					G_TYPE_FROM_CLASS (object_class),
					G_SIGNAL_RUN_FIRST,
					G_STRUCT_OFFSET (DebuggerClass,
									 program_stopped_signal),
					NULL, NULL,
					anjuta_cclosure_marshal_VOID__POINTER,
					G_TYPE_NONE, 1, G_TYPE_POINTER);
	
	debugger_signals[RESULTS_ARRIVED_SIGNAL] =
		g_signal_new ("results-arrived",
					G_TYPE_FROM_CLASS (object_class),
					G_SIGNAL_RUN_FIRST,
					G_STRUCT_OFFSET (DebuggerClass,
									 results_arrived_signal),
					NULL, NULL,
					anjuta_cclosure_marshal_VOID__STRING_POINTER,
					G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_POINTER);
	
	debugger_signals[LOCATION_CHANGED_SIGNAL] =
		g_signal_new ("location-changed",
					G_TYPE_FROM_CLASS (object_class),
					G_SIGNAL_RUN_FIRST,
					G_STRUCT_OFFSET (DebuggerClass,
									 location_changed_signal),
					NULL, NULL,
					anjuta_cclosure_marshal_VOID__STRING_INT_STRING,
					G_TYPE_NONE, 3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);
	
	object_class->dispose = debugger_dispose;
	object_class->finalize = debugger_finalize;
}

static void
debugger_initialize (Debugger *debugger)
{
	DEBUG_PRINT ("In function: debugger_init()");

	debugger->priv = g_new0 (DebuggerPriv, 1);
	
	debugger->priv->output_callback = NULL;
	debugger->priv->parent_win = NULL;
	debugger->priv->search_dirs = NULL;
	debugger->priv->launcher = anjuta_launcher_new ();
	
	debugger->priv->prog_is_running = FALSE;
	debugger->priv->debugger_is_ready = TRUE;
	debugger->priv->starting = FALSE;
	debugger->priv->skip_next_prompt = FALSE;
	debugger->priv->command_output_sent = FALSE;
	
	strcpy (debugger->priv->current_cmd.cmd, "");
	debugger->priv->current_cmd.parser = NULL;
	
	debugger->priv->cmd_queqe = NULL;
	debugger->priv->stdo_lines = NULL;
	
	debugger->priv->stdo_line = g_string_sized_new (FILE_BUFFER_SIZE);
	g_string_assign (debugger->priv->stdo_line, "");
	
	debugger->priv->stde_line = g_string_sized_new (FILE_BUFFER_SIZE);
	g_string_assign (debugger->priv->stde_line, "");
	
	debugger->priv->term_is_running = FALSE;
	debugger->priv->term_pid = -1;
	
	debugger->priv->post_execution_flag = DEBUGGER_NONE;
	debugger->priv->gnome_terminal_type = gdb_util_check_gnome_terminal();

    DEBUG_PRINT ("gnome-terminal type %d found.",
				 debugger->priv->gnome_terminal_type);
}

static void
debugger_instance_init (Debugger *debugger)
{
	debugger_initialize (debugger);
}

Debugger*
debugger_new (GtkWindow *parent_win, const GList *search_dirs,
			  DebuggerOutputFunc output_callback,
			  gpointer user_data)
{
	Debugger *debugger;
	
	g_return_val_if_fail (output_callback != NULL, NULL);
	debugger = g_object_new (DEBUGGER_TYPE, NULL);
	debugger->priv->parent_win = parent_win;
	debugger->priv->output_callback = output_callback;
	debugger->priv->output_user_data = user_data;
	debugger_start (debugger, search_dirs, NULL, FALSE);
	return debugger;
}

Debugger*
debugger_new_with_program (GtkWindow *parent_win,
						   const GList *search_dirs,
						   DebuggerOutputFunc output_callback,
						   gpointer user_data,
						   const gchar *program_path,
						   gboolean is_libtool_prog)
{
	Debugger *debugger;
	
	g_return_val_if_fail (output_callback != NULL, NULL);
	debugger = g_object_new (DEBUGGER_TYPE, NULL);
	debugger->priv->parent_win = parent_win;
	debugger->priv->output_callback = output_callback;
	debugger->priv->output_user_data = user_data;
	debugger_start (debugger, search_dirs, program_path, is_libtool_prog);
	return debugger;
}

gboolean
debugger_is_ready (Debugger *debugger)
{
	g_return_val_if_fail (IS_DEBUGGER (debugger), FALSE);
	return debugger->priv->debugger_is_ready;
}

gboolean
debugger_program_is_running (Debugger *debugger)
{
	g_return_val_if_fail (IS_DEBUGGER (debugger), FALSE);
	return debugger->priv->prog_is_running;
}

gboolean
debugger_program_is_attached (Debugger *debugger)
{
	g_return_val_if_fail (IS_DEBUGGER (debugger), FALSE);
	return debugger->priv->prog_is_attached;
}

static void
debugger_clear_buffers (Debugger *debugger)
{
	DEBUG_PRINT ("In function: debugger_clear_buffers()");

	/* Clear the output line buffer */
	g_string_assign (debugger->priv->stdo_line, "");

	/* Clear the error line buffer */
	g_string_assign (debugger->priv->stde_line, "");
	
	/* Clear CLI output lines */
	g_list_foreach (debugger->priv->stdo_lines, (GFunc)g_free, NULL);
	g_list_free (debugger->priv->stdo_lines);
	debugger->priv->stdo_lines = NULL;
}

static DebuggerCommand *
debugger_queue_get_next_command (Debugger *debugger)
{
	DebuggerCommand *dc;

	DEBUG_PRINT ("In function: debugger_get_next_command()");
	
	if (debugger->priv->cmd_queqe)
	{
		dc = g_list_nth_data (debugger->priv->cmd_queqe, 0);
		debugger->priv->cmd_queqe = g_list_remove (debugger->priv->cmd_queqe, dc);
	}
	else
		dc = NULL;
	return dc;
}

static void
debugger_queue_set_next_command (Debugger *debugger)
{
	DebuggerCommand *dc;

	DEBUG_PRINT ("In function: debugger_set_next_command()");
	
	dc = debugger_queue_get_next_command (debugger);
	if (!dc)
	{
		strcpy (debugger->priv->current_cmd.cmd, "");
		debugger->priv->current_cmd.parser = NULL;
		debugger->priv->current_cmd.user_data = NULL;
		debugger->priv->current_cmd.suppress_error = FALSE;
		return;
	}
	strcpy (debugger->priv->current_cmd.cmd, dc->cmd);
	debugger->priv->current_cmd.parser = dc->parser;
	debugger->priv->current_cmd.user_data = dc->user_data;
	debugger->priv->current_cmd.suppress_error = dc->suppress_error;
	g_free (dc);
}

static void
debugger_queue_command (Debugger *debugger, const gchar *cmd,
						gboolean suppress_error, DebuggerResultFunc parser,
						gpointer user_data)
{
	DebuggerCommand *dc;

	DEBUG_PRINT ("In function: debugger_queqe_command (%s)", cmd);
	
	dc = g_malloc (sizeof (DebuggerCommand));
	if (dc)
	{
		strcpy (dc->cmd, cmd);
		dc->parser = parser;
		dc->user_data = user_data;
		dc->suppress_error = suppress_error;
	}
	debugger->priv->cmd_queqe = g_list_append (debugger->priv->cmd_queqe, dc);
	if (strlen (debugger->priv->current_cmd.cmd) <= 0 &&
		debugger->priv->debugger_is_ready &&
		g_list_length (debugger->priv->cmd_queqe) == 1)
		debugger_queue_execute_command (debugger);
}

static void
debugger_queue_clear (Debugger *debugger)
{
	GList *node;

	DEBUG_PRINT ("In function: debugger_queqe_clear()");
	
	node = debugger->priv->cmd_queqe;
	while (node)
	{
		g_free (node->data);
		node = g_list_next (node);
	}
	g_list_free (debugger->priv->cmd_queqe);
	debugger->priv->cmd_queqe = NULL;
	strcpy (debugger->priv->current_cmd.cmd, "");
	debugger->priv->current_cmd.parser = NULL;
	debugger->priv->current_cmd.user_data = NULL;
	debugger->priv->current_cmd.suppress_error = FALSE;
	debugger_clear_buffers (debugger);
}

static void
debugger_execute_command (Debugger *debugger, const gchar *command)
{
	gchar *cmd;
	
	DEBUG_PRINT ("In function: debugger_execute_command(%s)\n",command);
	debugger->priv->debugger_is_ready = FALSE;
	debugger->priv->command_output_sent = FALSE;
	cmd = g_strconcat (command, "\n", NULL);
	anjuta_launcher_send_stdin (debugger->priv->launcher, cmd);
	g_free (cmd);
}

static void
debugger_queue_execute_command (Debugger *debugger)
{
	DEBUG_PRINT ("In function: debugger_queue_execute_command()");
	
	debugger_clear_buffers (debugger);
	debugger_queue_set_next_command (debugger);
	if (strlen (debugger->priv->current_cmd.cmd))
		debugger_execute_command (debugger, debugger->priv->current_cmd.cmd);
}

void
debugger_load_executable (Debugger *debugger, const gchar *prog)
{
	gchar *command, *dir, *msg;

	g_return_if_fail (IS_DEBUGGER (debugger));
	g_return_if_fail (prog != NULL);
	
	DEBUG_PRINT ("In function: debugger_load_executable(%s)", prog);

	msg = g_strconcat (_("Loading Executable: "), prog, "\n", NULL);
	debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL, msg, 
							   debugger->priv->output_user_data);
	g_free (msg);

	command = g_strconcat ("-file-exec-and-symbols ", prog, NULL);
	dir = g_path_get_dirname (prog);
/* TODO
	anjuta_set_execution_dir(dir);
*/
	g_free (dir);
	debugger_queue_command (debugger, command, FALSE, NULL, NULL);
	g_free (command);
	debugger->priv->starting = TRUE;
}

void
debugger_load_core (Debugger *debugger, const gchar *core)
{
	gchar *command, *dir, *msg;

	g_return_if_fail (IS_DEBUGGER (debugger));
	g_return_if_fail (core != NULL);
	
	DEBUG_PRINT ("In function: debugger_load_core(%s)", core);

	msg = g_strconcat (_("Loading Core: "), core, "\n", NULL);
	debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL, msg, 
									 debugger->priv->output_user_data);
	g_free (msg);

	command = g_strconcat ("core ", core, NULL);
	dir = g_path_get_dirname (core);
	debugger->priv->search_dirs = 
		g_list_prepend (debugger->priv->search_dirs, dir);
	debugger_queue_command (debugger, command, FALSE, NULL, NULL);
	g_free (command);
}

static void
debugger_start (Debugger *debugger, const GList *search_dirs,
				const gchar *prog, gboolean is_libtool_prog)
{
	gchar *command_str, *dir, *tmp, *text, *msg;
	gchar *exec_dir;
	gboolean ret;
	const GList *node;
	AnjutaLauncher *launcher;
	GList *dir_list = NULL;
	
	DEBUG_PRINT ("In function: debugger_start()");

	if (anjuta_util_prog_is_installed ("gdb", TRUE) == FALSE)
		return;

	debugger_queue_clear (debugger);

	tmp = g_strconcat (PACKAGE_DATA_DIR, "/", "gdb.init", NULL);
	if (g_file_test (tmp, G_FILE_TEST_IS_REGULAR) == FALSE)
	{
		anjuta_util_dialog_error (debugger->priv->parent_win,
								  _("Unable to find: %s.\n"
									"Unable to initialize debugger.\n"
									"Make sure Anjuta is installed correctly."),
								  tmp);
		g_free (tmp);
		return;
	}
	g_free (tmp);

	/* Prepare source search directories */
	exec_dir = NULL;
	if (prog)
		exec_dir = g_path_get_dirname (prog);
	
	if (exec_dir)
	{
		dir = g_strconcat (" -directory=", exec_dir, NULL);
		dir_list = g_list_prepend (dir_list, exec_dir);
	}
	else
	{
		dir = g_strdup (" ");
	}
	
	node = search_dirs;
	while (node)
	{
		text = node->data;
		if (text[0] == '/')
		{
			tmp = g_strconcat (dir, " -directory=", text, NULL);
			g_free (dir);
			dir = tmp;
			
			dir_list = g_list_prepend (dir_list, g_strdup (text));
		}
		else
		{
			g_warning ("Debugger source search dir '%s' is not absolute",
					   text);
		}
		node = g_list_next (node);
	}
	
	/* Now save the dir list. Order is automatically revesed */
	node = dir_list;
	while (node)
	{
		debugger->priv->search_dirs = 
			g_list_prepend (debugger->priv->search_dirs, node->data);
		node = g_list_next (node);
	}
	g_list_free (dir_list);
	
	if (prog && strlen(prog) > 0)
	{
		if (exec_dir)
			chdir (exec_dir);
		if (is_libtool_prog == FALSE)
		{
			command_str = g_strdup_printf ("gdb -q -f -n -i=mi2 %s -cd=%s "
										   "-x %s/gdb.init %s", dir, tmp,
										   PACKAGE_DATA_DIR, prog);
		}
		else
		{
			command_str = g_strdup_printf ("libtool --mode=execute gdb -q "
										   "-f -n -i=mi2 %s -cd=%s "
										   "-x %s/gdb.init %s", dir, tmp,
										   PACKAGE_DATA_DIR, prog);
		}
	}
	else
	{
		if (is_libtool_prog == FALSE)
		{
			command_str = g_strdup_printf ("gdb -q -f -n -i=mi2 %s "
										   "-x %s/gdb.init ",
										   dir, PACKAGE_DATA_DIR);
		}
		else
		{
			command_str = g_strdup_printf ("libtool --mode=execute gdb "
										   "-q -f -n -i=mi2 %s -x "
										   "%s/gdb.init ",
										   dir, PACKAGE_DATA_DIR);
		}
	}
	g_free (dir);
	debugger->priv->starting = TRUE;

	/* Prepare for launch. */
	launcher = debugger->priv->launcher;
	g_signal_connect (G_OBJECT (launcher), "child-exited",
					  G_CALLBACK (on_gdb_terminated), debugger);
	ret = anjuta_launcher_execute (launcher, command_str,
								   on_gdb_output_arrived, debugger);
	anjuta_launcher_set_encoding (launcher, "ISO-8859-1");

	if (ret == TRUE)
	{
		/* TODO		anjuta_update_app_status (TRUE, _("Debugger")); */
		debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
										 _("Getting ready to start debugging "
										   "session...\n"),
										 debugger->priv->output_user_data);
		
		if (prog)
		{
			msg = g_strconcat (_("Loading Executable: "), prog, "\n", NULL);
			debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
											 msg,
											 debugger->priv->output_user_data);
			g_free (msg);
		}
		else
		{
			debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
									   _("No executable specified.\n"),
									   debugger->priv->output_user_data);
			debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
											 _("Open an executable or attach "
											   "to a process to start "
											   "debugging.\n"),
											 debugger->priv->output_user_data);
		}
	}
	else
	{
		debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
										 _("There was an error whilst "
										   "launching the debugger.\n"),
										 debugger->priv->output_user_data);
		debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
										 _("Make sure 'gdb' is installed "
										   "on the system.\n"),
										 debugger->priv->output_user_data);
	}
	g_free (command_str);
}

static void
gdb_stdout_line_arrived (Debugger *debugger, const gchar * chars)
{
	gint i = 0;

	while (chars[i])
	{
		if (chars[i] == '\n')
			debugger_stdo_flush (debugger);
		else
			g_string_append_c (debugger->priv->stdo_line, chars[i]);
		i++;
	}
}

static void
gdb_stderr_line_arrived (Debugger *debugger, const gchar * chars)
{
	gint i;

	for (i = 0; i < strlen (chars); i++)
	{
		if (chars[i] == '\n')
			debugger_stde_flush (debugger);
		else
			g_string_append_c (debugger->priv->stde_line, chars[i]);
	}
}

static void
on_gdb_output_arrived (AnjutaLauncher *launcher,
					   AnjutaLauncherOutputType output_type,
					   const gchar *chars, gpointer data)
{
	Debugger *debugger = DEBUGGER (data);
	
	switch (output_type)
	{
	case ANJUTA_LAUNCHER_OUTPUT_STDERR:
		gdb_stderr_line_arrived (debugger, chars);
		break;
	case ANJUTA_LAUNCHER_OUTPUT_STDOUT:
		gdb_stdout_line_arrived (debugger, chars);
		break;
	default:
		break;
	}
}

static void
debugger_handle_post_execution (Debugger *debugger)
{
	switch (debugger->priv->post_execution_flag)
	{
		case DEBUGGER_NONE:
			break;
		case DEBUGGER_EXIT:
			debugger_stop (debugger);
			break;
		case DEBUGGER_RERUN_PROGRAM:
			debugger_run (debugger);
			break;
		default:
			g_warning ("Execution should not reach here");
	}
}

static void
debugger_process_frame (Debugger *debugger, const GDBMIValue *val)
{
	const GDBMIValue *file, *line, *frame, *addr;
	const gchar *file_str, *line_str, *addr_str;
	
	if (!val)
		return;
	
	file_str = line_str = addr_str = NULL;
	
	g_return_if_fail (val != NULL);
	
	file = gdbmi_value_hash_lookup (val, "file");
	line = gdbmi_value_hash_lookup (val, "line");
	frame = gdbmi_value_hash_lookup (val, "frame");
	addr = gdbmi_value_hash_lookup (val, "addr");
	
	if (file && line)
	{
		file_str = gdbmi_value_literal_get (file);
		line_str = gdbmi_value_literal_get (line);
		
		if (addr)
			addr_str = gdbmi_value_literal_get (addr);
		
		if (file_str && line_str)
		{
			debugger_change_location (debugger, file_str,
									  atoi(line_str), addr_str);
		}
	}
	else if (frame)
	{
		file = gdbmi_value_hash_lookup (frame, "file");
		line = gdbmi_value_hash_lookup (frame, "line");
		
		if (addr)
			addr_str = gdbmi_value_literal_get (addr);
	
		if (file && line)
		{
			file_str = gdbmi_value_literal_get (file);
			line_str = gdbmi_value_literal_get (line);
			if (file_str && line_str)
			{
				debugger_change_location (debugger, file_str,
										  atoi(line_str), addr_str);
			}
		}
	}
}

static void
debugger_stdo_flush (Debugger *debugger)
{
	guint lineno;
	gchar *filename, *line;

	line = debugger->priv->stdo_line->str;
	
	if (strlen (line) == 0)
	{
		return;
	}
	if (line[0] == '\032' && line[1] == '\032')
	{
		gdb_util_parse_error_line (&(line[2]), &filename, &lineno);
		if (filename)
		{
			debugger_change_location (debugger, filename, lineno, NULL);
			g_free (filename);
		}
	}
	else if (strncasecmp (line, "^error", 6) == 0)
	{
		/* GDB reported error */
		if (debugger->priv->current_cmd.suppress_error == FALSE)
		{
			GDBMIValue *val = gdbmi_value_parse (line);
			if (val)
			{
				const GDBMIValue *message;
				message = gdbmi_value_hash_lookup (val, "msg");
				
				anjuta_util_dialog_error (debugger->priv->parent_win,
										  "%s",
										  gdbmi_value_literal_get (message));
				gdbmi_value_free (val);
			}
		}
	}
	else if (strncasecmp(line, "^running", 8) == 0)
	{
		/* Program started running */
		debugger->priv->prog_is_running = TRUE;
		debugger->priv->debugger_is_ready = TRUE;
		debugger->priv->skip_next_prompt = TRUE;
		g_signal_emit_by_name (debugger, "program-running");
	}
	else if (strncasecmp (line, "*stopped", 8) == 0)
	{
		/* Process has stopped */
		gboolean program_exited = FALSE;
		
		GDBMIValue *val = gdbmi_value_parse (line);
		
		debugger->priv->debugger_is_ready = TRUE;
		
		/* Check if program has exited */
		if (val)
		{
			const GDBMIValue *reason;
			const gchar *str = NULL;
			
			debugger_process_frame (debugger, val);
			
			reason = gdbmi_value_hash_lookup (val, "reason");
			if (reason)
				str = gdbmi_value_literal_get (reason);
			
			if (str && strcmp (str, "exited-normally") == 0)
			{
				program_exited = TRUE;
				debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
										   _("Program exited normally\n"),
										   debugger->priv->output_user_data);
			}
			else if (str && strcmp (str, "exited") == 0)
			{
				const GDBMIValue *errcode;
				const gchar *errcode_str;
				gchar *msg;
				
				program_exited = TRUE;
				errcode = gdbmi_value_hash_lookup (val, "exit-code");
				errcode_str = gdbmi_value_literal_get (errcode);
				msg = g_strdup_printf (_("Program exited with error code %s\n"),
									   errcode_str);
				debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
										   msg, debugger->priv->output_user_data);
				g_free (msg);
			}
			else if (str && strcmp (str, "exited-signalled") == 0)
			{
				const GDBMIValue *signal_name, *signal_meaning;
				const gchar *signal_str, *signal_meaning_str;
				gchar *msg;
				
				program_exited = TRUE;
				signal_name = gdbmi_value_hash_lookup (val, "signal-name");
				signal_str = gdbmi_value_literal_get (signal_name);
				signal_meaning = gdbmi_value_hash_lookup (val,
														  "signal-meaning");
				signal_meaning_str = gdbmi_value_literal_get (signal_meaning);
				msg = g_strdup_printf (_("Program received signal %s (%s) and exited\n"),
									   signal_str, signal_meaning_str);
				debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
										   msg, debugger->priv->output_user_data);
				g_free (msg);
			}
			else if (str && strcmp (str, "signal-received") == 0)
			{
				const GDBMIValue *signal_name, *signal_meaning;
				const gchar *signal_str, *signal_meaning_str;
				gchar *msg;
				
				signal_name = gdbmi_value_hash_lookup (val, "signal-name");
				signal_str = gdbmi_value_literal_get (signal_name);
				signal_meaning = gdbmi_value_hash_lookup (val,
														  "signal-meaning");
				signal_meaning_str = gdbmi_value_literal_get (signal_meaning);
				
				msg = g_strdup_printf (_("Program received signal %s (%s)\n"),
									   signal_str, signal_meaning_str);
				debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
										   msg, debugger->priv->output_user_data);
				g_free (msg);
			}
			else if (str && strcmp (str, "breakpoint-hit") == 0)
			{
				const GDBMIValue *bkptno;
				const gchar *bkptno_str;
				gchar *msg;
				
				bkptno = gdbmi_value_hash_lookup (val, "bkptno");
				bkptno_str = gdbmi_value_literal_get (bkptno);
				
				msg = g_strdup_printf (_("Breakpoint number %s hit\n"),
									   bkptno_str);
				debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
										   msg, debugger->priv->output_user_data);
				g_free (msg);
			}
			else if (str && strcmp (str, "function-finished") == 0)
			{
				debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
										   _("Function finished\n"),
										   debugger->priv->output_user_data);
			}
			else if (str && strcmp (str, "end-stepping-range") == 0)
			{
				debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
										   _("Stepping finished\n"),
										   debugger->priv->output_user_data);
			}
			else if (str && strcmp (str, "location-reached") == 0)
			{
				debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
										   _("Location reached\n"),
										   debugger->priv->output_user_data);
			}
		}
		
		if (program_exited)
		{
			debugger->priv->prog_is_running = FALSE;
			debugger->priv->prog_is_attached = FALSE;
			debugger->priv->debugger_is_ready = TRUE;
			debugger_stop_terminal (debugger);
			g_signal_emit_by_name (debugger, "program-exited", val);
			debugger_handle_post_execution (debugger);
		}
		else
		{
			debugger->priv->debugger_is_ready = TRUE;
			g_signal_emit_by_name (debugger, "program-stopped", val);
		}
		
		debugger->priv->stdo_lines = g_list_reverse (debugger->priv->stdo_lines);
		if (debugger->priv->current_cmd.cmd[0] != '\0' &&
			debugger->priv->current_cmd.parser != NULL)
		{
			debugger->priv->debugger_is_ready = TRUE;
			debugger->priv->current_cmd.parser (debugger, val,
										  debugger->priv->stdo_lines,
										  debugger->priv->current_cmd.user_data);
			debugger->priv->command_output_sent = TRUE;
			DEBUG_PRINT ("In function: Sending output...");
		}
		
		if (val)
			gdbmi_value_free (val);
	}
	else if (strncasecmp (line, "^done", 5) == 0)
	{
		/* GDB command has reported output */
		GDBMIValue *val = gdbmi_value_parse (line);
		
		debugger->priv->debugger_is_ready = TRUE;
		
		debugger->priv->stdo_lines = g_list_reverse (debugger->priv->stdo_lines);
		if (debugger->priv->current_cmd.cmd[0] != '\0' &&
			debugger->priv->current_cmd.parser != NULL)
		{
			debugger->priv->current_cmd.parser (debugger, val,
										  debugger->priv->stdo_lines,
										  debugger->priv->current_cmd.user_data);
			debugger->priv->command_output_sent = TRUE;
			DEBUG_PRINT ("In function: Sending output...");
		}
		else /* if (val) */
		{
			g_signal_emit_by_name (debugger, "results-arrived",
								   debugger->priv->current_cmd.cmd, val);
		}
		
		if (val)
		{
			debugger_process_frame (debugger, val);
			gdbmi_value_free (val);
		}
	}
	else if (strncasecmp (line, GDB_PROMPT, strlen (GDB_PROMPT)) == 0)
	{
		if (debugger->priv->skip_next_prompt)
		{
			debugger->priv->skip_next_prompt = FALSE;
		}
		else
		{
			debugger->priv->debugger_is_ready = TRUE;
			if (debugger->priv->starting)
			{
				debugger->priv->starting = FALSE;
				/* Debugger has just started */
				debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
										   _("Debugger is ready.\n"),
										   debugger->priv->output_user_data);
			}
	
			if (strcmp (debugger->priv->current_cmd.cmd, "-exec-run") == 0 &&
				debugger->priv->prog_is_running == FALSE)
			{
				/* Program has failed to run */
				debugger_stop_terminal (debugger);
			}
			debugger->priv->debugger_is_ready = TRUE;
			
			/* If the parser has not yet been called, call it now. */
			if (debugger->priv->command_output_sent == FALSE &&
				debugger->priv->current_cmd.parser)
			{
				debugger->priv->current_cmd.parser (debugger, NULL,
											  debugger->priv->stdo_lines,
											  debugger->priv->current_cmd.user_data);
				debugger->priv->command_output_sent = TRUE;
			}

			debugger_queue_execute_command (debugger);	/* Next command. Go. */
			return;
		}
	}
	else
	{
		/* FIXME: change message type */
		gchar *proper_msg, *full_msg;
		
		if (line[1] == '\"' && line [strlen(line) - 1] == '\"')
		{
			line[strlen(line) - 1] = '\0';
			proper_msg = g_strcompress (line + 2);
		}
		else
		{
			proper_msg = g_strdup (line);
		}
		
		if (proper_msg[strlen(proper_msg)-1] != '\n')
		{
			full_msg = g_strconcat (proper_msg, "\n", NULL);
		}
		else
		{
			full_msg = g_strdup (proper_msg);
		}
		
		if (debugger->priv->current_cmd.parser)
		{
			if (line[0] == '~')
			{
				/* Save GDB CLI output */
				/* Remove trailing newline */
				full_msg[strlen (full_msg) - 1] = '\0';
				debugger->priv->stdo_lines = g_list_prepend (debugger->priv->stdo_lines,
													   g_strdup (full_msg));
			}
		}
		else if (line[0] != '&')
		{
			/* Print the CLI output if there is no parser to receive
			 * the output and it is not command echo
			 */
			debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
									   full_msg, debugger->priv->output_user_data);
		}
		g_free (proper_msg);
		g_free (full_msg);
	}
	
	/* Clear the line buffer */
	g_string_assign (debugger->priv->stdo_line, "");
}

void
debugger_stde_flush (Debugger *debugger)
{
	if (strlen (debugger->priv->stde_line->str) > 0)
	{
		debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_ERROR,
								   debugger->priv->stde_line->str,
								   debugger->priv->output_user_data);
	}
	/* Clear the line buffer */
	g_string_assign (debugger->priv->stde_line, "");
}

static void
on_gdb_terminated (AnjutaLauncher *launcher,
				   gint child_pid, gint status, gulong t,
				   gpointer data)
{
	Debugger *debugger = DEBUGGER (data);
	
	g_signal_handlers_disconnect_by_func (G_OBJECT (launcher),
										  G_CALLBACK (on_gdb_terminated),
										  debugger);
	
	DEBUG_PRINT ("In function: gdb_terminated()");
	
	/* Clear the command queue & Buffer */
	debugger_clear_buffers (debugger);
	debugger_stop_terminal (debugger);

	/* Good Bye message */
	debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_ERROR,
							   _("gdb terminated unexpectedly. Restarting gdb\n"),
							   debugger->priv->output_user_data);
	debugger_stop_terminal (debugger);
	debugger->priv->prog_is_running = FALSE;
	debugger->priv->term_is_running = FALSE;
	debugger->priv->term_pid = -1;
	debugger_start (debugger, NULL, NULL, FALSE);
	/* TODO	anjuta_update_app_status (TRUE, NULL); */
}

static void
debugger_stop_real (Debugger *debugger)
{
	DEBUG_PRINT ("In function: debugger_stop()");
	
	/* if program is attached - detach from it before quiting */
	if (debugger->priv->prog_is_attached == TRUE)
		debugger_queue_command (debugger, "detach", FALSE, NULL, NULL);

	debugger_stop_terminal (debugger);
	debugger_queue_command (debugger, "-gdb-exit", FALSE, NULL, NULL);
}

static gboolean
debugger_stop (Debugger *debugger)
{
	gboolean ret = TRUE;
	
	if (debugger->priv->prog_is_running == TRUE)
	{
		GtkWidget *dialog;
		gchar *mesg;
		
		if (debugger->priv->prog_is_attached == TRUE)
			mesg = _("Program is ATTACHED.\n"
				   "Do you still want to stop Debugger?");
		else
			mesg = _("Program is RUNNING.\n"
				   "Do you still want to stop Debugger?");
		dialog = gtk_message_dialog_new (debugger->priv->parent_win,
										 GTK_DIALOG_DESTROY_WITH_PARENT,
										 GTK_MESSAGE_QUESTION,
										 GTK_BUTTONS_NONE, mesg);
		gtk_dialog_add_buttons (GTK_DIALOG (dialog),
								GTK_STOCK_CANCEL,	GTK_RESPONSE_NO,
								GTK_STOCK_STOP,		GTK_RESPONSE_YES,
								NULL);
		if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
			debugger_stop_real (debugger);
		else
			ret = FALSE;
		gtk_widget_destroy (dialog);
	}
	else
		debugger_stop_real (debugger);
	return ret;
}

static void
on_debugger_terminal_terminated (int status, gpointer user_data)
{
	Debugger *debugger = DEBUGGER (user_data);
	
	DEBUG_PRINT ("In function: on_debugger_terminal_terminated()");
	
	debugger->priv->term_is_running = FALSE;
	debugger->priv->term_pid = -1;
	
	if (debugger->priv->prog_is_running)
		debugger_stop_program (debugger);
}

gchar*
debugger_get_source_path (Debugger *debugger, const gchar *file)
{
	GList *node;
	gchar *path = NULL;
	
	if (g_path_is_absolute (file))
		return g_strdup (file);
	
	node = debugger->priv->search_dirs;
	while (node)
	{
		path = g_build_filename (node->data, file, NULL);
		if (g_file_test (path, G_FILE_TEST_EXISTS))
			break;
		g_free (path);
		node = g_list_next (node);
	}
	
	if (path == NULL)
	{
		/* The file could be found nowhere. Use current directory */
		gchar *cwd;
		cwd = g_get_current_dir ();
		path = g_build_filename (cwd, file, NULL);
		g_free (cwd);
	}
	return path;
}

void
debugger_change_location (Debugger *debugger, const gchar *file,
						  gint line, const gchar *address)
{
	gchar *src_path;
	
	g_return_if_fail (file != NULL);
	src_path = debugger_get_source_path (debugger, file);
	g_signal_emit_by_name (debugger, "location-changed", src_path,
						   line, address);
	g_free (src_path);
}

static gchar *
debugger_start_terminal (Debugger *debugger)
{
	gchar *file, *cmd, *encoded_cmd;
	gchar  dev_name[100];
	gint   count, idx;
	FILE  *fp;
	pid_t  pid;
	gchar *argv[20];
	GList *args, *node;

	DEBUG_PRINT ("In function: debugger_start_terminal()");
	
	if (debugger->priv->prog_is_running == TRUE)
		return NULL;
	if (debugger->priv->term_is_running == TRUE)
		return NULL;
	if (anjuta_util_prog_is_installed ("anjuta_launcher", TRUE) == FALSE)
		return NULL;
	
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

	debugger->priv->term_is_running = TRUE;
	cmd = g_strconcat ("anjuta_launcher --__debug_terminal ", file, NULL);
	encoded_cmd = anjuta_util_escape_quotes (cmd);
	g_free (cmd);
	
	cmd = g_strdup ("gnome-terminal -e \"%s\"");
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
        if (debugger->priv->gnome_terminal_type == 1) {
            /* Remove any --disable-factory option, if present */
            while (node) {
                if (strcmp ((gchar*)args->data, "--disable-factory") == 0) {
                    g_free (node->data);
                    args = g_list_remove (args, node);
                    break;
                }
                node = g_list_next (node);
            }
        } else if (debugger->priv->gnome_terminal_type == 2) {
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

	debugger->priv->term_pid = pid;
	if (pid < 1)
		goto error;
	DEBUG_PRINT  ("terminal pid = %d\n", pid);
	anjuta_children_register (pid, on_debugger_terminal_terminated, debugger);

	/*
	 * May be the terminal is not started properly.
	 * Callback will reset this flag
	 */
	if (debugger->priv->term_is_running == FALSE)
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
	anjuta_util_dialog_error (debugger->priv->parent_win,
							  _("Cannot start terminal for debugging."));
	debugger_stop_terminal (debugger);
	remove (file);
	g_free (file);
	return NULL;
}

static void
debugger_stop_terminal (Debugger *debugger)
{
	DEBUG_PRINT ("In function: debugger_stop_terminal()");
	
	if (debugger->priv->term_is_running == FALSE)
		return;
	if (debugger->priv->term_pid > 0) {
		anjuta_children_unregister (debugger->priv->term_pid);
		if (kill (debugger->priv->term_pid, SIGTERM) == -1) {
			switch (errno) {
				case EINVAL:
					g_warning ("Invalid signal applied to kill");
					break;
				case ESRCH:
					g_warning ("No such pid [%d] or process has already died",
							   debugger->priv->term_pid);
					break;
				case EPERM:
					g_warning ("No permission to send signal to the process");
					break;
				default:
					g_warning ("Unknow error while kill");
			}
		}
	}
	debugger->priv->term_pid = -1;
	debugger->priv->term_is_running = FALSE;
}

void
debugger_start_program (Debugger *debugger)
{
	gchar *term, *cmd, *args;

	DEBUG_PRINT ("In function: debugger_start_program()");

	g_return_if_fail (IS_DEBUGGER (debugger));
	g_return_if_fail (debugger->priv->prog_is_running == FALSE);

	args = NULL;
	anjuta_util_dialog_input (debugger->priv->parent_win,
							  _("Program arguments"),
							  NULL, &args);
	
	if (args)
	{
		cmd = g_strconcat ("-exec-arguments ", args, NULL);
		debugger_queue_command (debugger, cmd, FALSE, NULL, NULL);
		g_free (cmd);
		g_free (args);
	}

	term = debugger_start_terminal (debugger);
	if (term)
	{
		cmd = g_strconcat ("tty ", term, NULL);
		debugger_queue_command (debugger, cmd, FALSE, NULL, NULL);
		debugger_queue_command (debugger, "-exec-run", FALSE, NULL, NULL);
		g_free (cmd);
		g_free (term);
	}
	else
	{
		debugger_queue_command (debugger, "-exec-run", FALSE, NULL, NULL);
	}
	debugger->priv->post_execution_flag = DEBUGGER_NONE;
}

static void
debugger_attach_process_finish (Debugger *debugger, const GDBMIValue *mi_results,
								const GList *cli_results, gpointer data)
{
	DEBUG_PRINT ("Program attach finished");
	debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
									 _("Program attached\n"),
									 debugger->priv->output_user_data);
	debugger->priv->prog_is_attached = TRUE;
	debugger->priv->prog_is_running = TRUE;
	g_signal_emit_by_name (debugger, "program-stopped", mi_results);
}

static void
debugger_attach_process_real (Debugger *debugger, pid_t pid)
{
	gchar *buf;

	DEBUG_PRINT ("In function: debugger_attach_process_real()");
	
	buf = g_strdup_printf (_("Attaching to process: %d...\n"), pid);
	debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
							   buf, debugger->priv->output_user_data);
	g_free (buf);
	
	buf = g_strdup_printf ("attach %d", pid);
	debugger_queue_command (debugger, buf, FALSE,
							debugger_attach_process_finish, NULL);
	g_free (buf);
}

void
debugger_attach_process (Debugger *debugger, pid_t pid)
{
	DEBUG_PRINT ("In function: debugger_attach_process()");
	
	g_return_if_fail (IS_DEBUGGER (debugger));
	
	if (debugger->priv->prog_is_running == TRUE)
	{
		// TODO: Dialog to be made HIG compliant.
		gchar *mesg;
		GtkWidget *dialog;
		
		mesg = _("A process is already running.\n"
				 "Would you like to terminate it and attach the new process?"),
		dialog = gtk_message_dialog_new (debugger->priv->parent_win,
										 GTK_DIALOG_DESTROY_WITH_PARENT,
										 GTK_MESSAGE_QUESTION,
										 GTK_BUTTONS_YES_NO, mesg);
		if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
		{
			debugger_stop_program (debugger);
			debugger_attach_process_real (debugger, pid);
		}
		gtk_widget_destroy (dialog);
	}
	else if (getpid () == pid ||
			 anjuta_launcher_get_child_pid (debugger->priv->launcher) == pid)
	{
		anjuta_util_dialog_error (debugger->priv->parent_win,
								  _("Anjuta is unable to attach to itself."));
		return;
	}
	else
		debugger_attach_process_real (debugger, pid);
}

void
debugger_restart_program (Debugger *debugger)
{
	DEBUG_PRINT ("In function: debugger_restart_program()");
	
	g_return_if_fail (debugger->priv->prog_is_attached == FALSE);
	
	debugger->priv->post_execution_flag = DEBUGGER_RERUN_PROGRAM;
	debugger_stop_program (debugger);
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
debugger_stop_program (Debugger *debugger)
{
	DEBUG_PRINT ("In function: debugger_stop_program()");
	
	g_return_if_fail (debugger->priv->prog_is_running == TRUE);
	
	if (debugger->priv->prog_is_attached == TRUE)
		debugger_queue_command (debugger, "detach", FALSE, NULL, NULL);
	else
	{
		/* FIXME: Why doesn't -exec-abort work??? */
		/* debugger_queue_command (debugger, "-exec-abort", NULL, NULL); */
		debugger_queue_command (debugger, "kill", FALSE, NULL, NULL);
		debugger->priv->prog_is_running = FALSE;
		debugger->priv->prog_is_attached = FALSE;
		debugger->priv->debugger_is_ready = TRUE;
		debugger_stop_terminal (debugger);
		g_signal_emit_by_name (debugger, "program-exited", NULL);
		debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
								   _("Program terminated\n"),
								   debugger->priv->output_user_data);
		debugger_handle_post_execution (debugger);
	}
}

static void
debugger_detach_process_finish (Debugger *debugger, const GDBMIValue *mi_results,
								const GList *cli_results, gpointer data)
{
	DEBUG_PRINT ("Program detach finished");
	debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
									 _("Program detached\n"),
									 debugger->priv->output_user_data);
	debugger->priv->prog_is_attached = FALSE;
	debugger->priv->prog_is_running = FALSE;
	g_signal_emit_by_name (debugger, "program-exited", mi_results);
}

void
debugger_detach_process (Debugger *debugger)
{
	gchar *buff;

	DEBUG_PRINT ("In function: debugger_detach_process()");
	
	g_return_if_fail (debugger->priv->prog_is_attached == TRUE);
	
	buff = g_strdup_printf (_("Detaching the process...\n"));
	debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
							   buff, debugger->priv->output_user_data);
	g_free (buff);
	
	debugger_queue_command (debugger, "detach", FALSE,
							debugger_detach_process_finish, NULL);
	debugger->priv->prog_is_attached = FALSE;
}

void
debugger_interrupt (Debugger *debugger)
{
	DEBUG_PRINT ("In function: debugger_interrupt()");
	
	g_return_if_fail (IS_DEBUGGER (debugger));
	g_return_if_fail (debugger->priv->prog_is_running == TRUE);
	
	debugger->priv->output_callback (debugger, DEBUGGER_OUTPUT_NORMAL,
							   _("Interrupting the process\n"),
							   debugger->priv->output_user_data);
	debugger_queue_command (debugger, "-exec-interrupt", FALSE, NULL, NULL);
}

void
debugger_run (Debugger *debugger)
{
	DEBUG_PRINT ("In function: debugger_run()");
	
	g_return_if_fail (IS_DEBUGGER (debugger));
	
	/* Program not running - start it */
	if (debugger->priv->prog_is_running == FALSE)
	{
		debugger_queue_command (debugger, "-break-insert -t main",
								FALSE, NULL, NULL);
		debugger_start_program (debugger);
		debugger_queue_command (debugger, "-exec-continue", FALSE, NULL, NULL);
		return;
	}
	/* Program running - continue */
	debugger_queue_command (debugger, "-exec-continue", FALSE, NULL, NULL);
}

void
debugger_step_in (Debugger *debugger)
{
	DEBUG_PRINT ("In function: debugger_step_in()");
	
	if (debugger->priv->prog_is_running == FALSE)
	{
		debugger_queue_command (debugger, "-break-insert -t main",
								FALSE, NULL, NULL);
		debugger_start_program (debugger);
		return;
	}
	debugger_queue_command (debugger, "-exec-step", FALSE, NULL, NULL);
}

void
debugger_step_over (Debugger *debugger)
{
	DEBUG_PRINT ("In function: debugger_step_over()");
	
	g_return_if_fail (IS_DEBUGGER (debugger));
	g_return_if_fail (debugger->priv->prog_is_running == TRUE);
	
	debugger_queue_command (debugger, "-exec-next", FALSE, NULL, NULL);
}

void
debugger_step_out (Debugger *debugger)
{
	DEBUG_PRINT ("In function: debugger_step_out()");
	
	g_return_if_fail (IS_DEBUGGER (debugger));
	g_return_if_fail (debugger->priv->prog_is_running == TRUE);
	
	debugger_queue_command (debugger, "-exec-finish", FALSE, NULL, NULL);
}

void
debugger_run_to_location (Debugger *debugger, const gchar *loc)
{
	gchar *buff;

	DEBUG_PRINT ("In function: debugger_run_to_location()");
	
	g_return_if_fail (IS_DEBUGGER (debugger));
	
	if (debugger->priv->prog_is_running == FALSE)
	{
		buff = g_strconcat ("-break-insert -t ", loc, NULL);
		debugger_queue_command (debugger, buff, FALSE, NULL, NULL);
		g_free (buff);
		debugger_start_program (debugger);
		return;
	}
	buff = g_strdup_printf ("-exec-until %s", loc);
	debugger_queue_command (debugger, buff, FALSE, NULL, NULL);
	g_free (buff);
}

void
debugger_command (Debugger *debugger, const gchar *command,
				  gboolean suppress_error, DebuggerResultFunc parser,
				  gpointer user_data)
{
	if (strncasecmp (command, "-exec-run",
					 strlen ("-exec-run")) == 0 ||
		strncasecmp (command, "run", strlen ("run")) == 0)
	{
		/* FIXME: The user might have passed args to the command */
		debugger_run (debugger);
	}
	else if (strncasecmp (command, "-exec-step",
						  strlen ("-exec-step")) == 0 ||
			 strncasecmp (command, "step", strlen ("step")) == 0)
	{
		debugger_step_in (debugger);
	}
	else if (strncasecmp (command, "-exec-next",
						  strlen ("-exec-next")) == 0 ||
			 strncasecmp (command, "next", strlen ("next")) == 0)
	{
		debugger_step_over (debugger);
	}
	else if (strncasecmp (command, "-exec-finish",
						  strlen ("-exec-finish")) == 0 ||
			 strncasecmp (command, "finish", strlen ("finish")) == 0)
	{
		debugger_step_out (debugger);
	}
	else if (strncasecmp (command, "-exec-continue",
						  strlen ("-exec-continue")) == 0 ||
			 strncasecmp (command, "continue", strlen ("continue")) == 0)
	{
		debugger_run (debugger);
	}
	else if (strncasecmp (command, "-exec-until",
						  strlen ("-exec-until")) == 0 ||
			 strncasecmp (command, "until", strlen ("until")) == 0)
	{
		debugger_run_to_location (debugger, strchr (command, ' '));
	}
	else if (strncasecmp (command, "-exec-abort",
						  strlen ("-exec-abort")) == 0 ||
			 strncasecmp (command, "kill", strlen ("kill")) == 0)
	{
		debugger_stop_program (debugger);
	}
	else if (strncasecmp (command, "-target-attach",
						  strlen ("-target-attach")) == 0 ||
			 strncasecmp (command, "attach", strlen ("attach")) == 0)
	{
		pid_t pid = 0;
		gchar *pid_str = strchr (command, ' ');
		if (pid_str)
			pid = atoi (pid_str);
		debugger_attach_process (debugger, pid);
	}
	else if (strncasecmp (command, "-target-detach",
						  strlen ("-target-detach")) == 0 ||
			 strncasecmp (command, "detach", strlen ("detach")) == 0)
	{
		debugger_detach_process (debugger);
	}
	else if (strncasecmp (command, "-file-exec-and-symbols",
						  strlen ("-file-exec-and-symbols")) == 0 ||
			 strncasecmp (command, "file", strlen ("file")) == 0)
	{
		debugger_load_executable (debugger, strchr (command, ' '));
	}
	else if (strncasecmp (command, "core", strlen ("core")) == 0)
	{
		debugger_load_core (debugger, strchr (command, ' '));
	}
	else
	{
		debugger_queue_command (debugger, command, suppress_error,
								parser, user_data);
	}
}


#if 0 /* FIXME */
void
debugger_signal (const gchar *sig, gboolean show_msg)
{
	/* eg:- "SIGTERM" */
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

static void
query_set_cmd (const gchar *cmd, gboolean state)
{
	gchar buffer[50];
	gchar *tmp = g_stpcpy (buffer, cmd);
	strcpy (tmp, state ? "on" : "off");
	debugger_put_cmd_in_queqe (buffer, DB_CMD_NONE, NULL, NULL);
}

static void
query_set_verbose (gboolean state)
{
	query_set_cmd ("set verbose ", state);
}

static void
query_set_print_staticmembers (gboolean state)
{
	query_set_cmd ("set print static-members ", state);
}

static void
query_set_print_pretty (gboolean state)
{
	query_set_cmd ("set print pretty ", state);
}

void debugger_query_evaluate_expr_tip (const gchar *expr,
									   DebuggerCLIFunc parser, gpointer data)
{
	query_set_verbose (FALSE);
	query_set_print_staticmembers (FALSE);
	gchar *printcmd = g_strconcat ("print ", expr, NULL);
	debugger_put_cmd_in_queqe (printcmd, DB_CMD_NONE, parser, data);
	query_set_verbose (TRUE);
	query_set_print_staticmembers (TRUE);
	g_free (printcmd);
}

void
debugger_query_evaluate_expression (const gchar *expr, DebuggerFunc parser,
									gpointer data)
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

#endif
