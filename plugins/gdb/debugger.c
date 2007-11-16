/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA 
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/*#define DEBUG*/

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/wait.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>

#include <glib.h>

#include <libanjuta/anjuta-launcher.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-marshal.h>

#include "debugger.h"
#include "utilities.h"

#define GDB_PROMPT  "(gdb)"
#define FILE_BUFFER_SIZE 1024
#define GDB_PATH "gdb"
#define SUMMARY_MAX_LENGTH   90	 /* Should be smaller than 4K to be displayed
				  * in GtkCellRendererCell */

enum {
	DEBUGGER_NONE,
	DEBUGGER_EXIT,
	DEBUGGER_RERUN_PROGRAM
};

enum
{
	PROGRAM_LOADED_SIGNAL,
	PROGRAM_RUNNING_SIGNAL,
	PROGRAM_EXITED_SIGNAL,
	PROGRAM_STOPPED_SIGNAL,
	RESULTS_ARRIVED_SIGNAL,
	LOCATION_CHANGED_SIGNAL,
	BREAKPOINT_CHANGED_SIGNAL,
	VARIABLE_CHANGED_SIGNAL,
	LAST_SIGNAL
};

struct _DebuggerPriv
{
	GtkWindow *parent_win;
	
	IAnjutaDebuggerOutputCallback output_callback;
	gpointer output_user_data;
	
	GList *search_dirs;
	
	gboolean prog_is_running;
	gboolean prog_is_attached;
	gboolean prog_is_loaded;
	gboolean debugger_is_started;
	guint debugger_is_busy;
	gint post_execution_flag;

	AnjutaLauncher *launcher;
	GString *stdo_line;
	GString *stdo_acc;
	GString *stde_line;
	
	GList *cli_lines;
	
	/* State */
	gboolean solib_event;
	gboolean stopping;
	gboolean exiting;
	gboolean starting;
	gboolean terminating;
	gboolean loading;
	
	/* GDB command queue */
	GList *cmd_queqe;
	DebuggerCommand current_cmd;
	gboolean skip_next_prompt;
	gboolean command_output_sent;
	
	pid_t inferior_pid;
	gint current_thread;
	guint current_frame;
	
	GObject* instance;

	IAnjutaMessageView *log;
};

static gpointer parent_class;

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

typedef struct _GdbGListPacket
{
	GList* list;
	gint tag;
} GdbGListPacket;

/* Useful functions
 *---------------------------------------------------------------------------*/

typedef struct _GdbMessageCode GdbMessageCode;

struct _GdbMessageCode
{
	const gchar *msg;
	guint code;
};

const static GdbMessageCode GdbErrorMessage[] = 
	{{"mi_cmd_var_create: unable to create variable object",
	IANJUTA_DEBUGGER_UNABLE_TO_CREATE_VARIABLE},
	{"Cannot access memory at address ",
	IANJUTA_DEBUGGER_UNABLE_TO_ACCESS_MEMORY},
	{"No source file named  ",
	IANJUTA_DEBUGGER_UNABLE_TO_OPEN_FILE},
	{NULL, 0}};

static guint
gdb_match_error(const gchar *message)
{
	const GdbMessageCode* msg;

	for (msg = GdbErrorMessage; msg->msg != NULL; msg++)
	{
		gsize len = strlen (msg->msg);
		
		if (!isspace (msg->msg[len - 1]))
		{
			/* Match the whole string */
			len++;
		}

		if (memcmp (msg->msg, message, len) == 0)
		{
			return msg->code;
		}
	}

	return IANJUTA_DEBUGGER_UNKNOWN_ERROR;
}

static void
debugger_message_view_append (Debugger *debugger, IAnjutaMessageViewType type, const char *message)
{
	guint len = strlen(message);
	gchar buf[SUMMARY_MAX_LENGTH];
	const gchar* summary = message;
	const gchar* detail = "";


	if (len > SUMMARY_MAX_LENGTH)
	{

		memcpy(buf, message, SUMMARY_MAX_LENGTH - 4);
		memcpy(buf + SUMMARY_MAX_LENGTH - 4, "...", 4);
		summary = buf;
		detail = message;
	}

	ianjuta_message_view_append (debugger->priv->log, type, summary, detail, NULL);
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

	debugger->priv->debugger_is_started = FALSE;	
	debugger->priv->prog_is_running = FALSE;
	debugger->priv->debugger_is_busy = 0;
	debugger->priv->starting = FALSE;
	debugger->priv->terminating = FALSE;
	debugger->priv->skip_next_prompt = FALSE;
	debugger->priv->command_output_sent = FALSE;

	debugger->priv->current_cmd.cmd = NULL;
	debugger->priv->current_cmd.parser = NULL;
	
	debugger->priv->cmd_queqe = NULL;
	debugger->priv->cli_lines = NULL;
	debugger->priv->solib_event = FALSE;
	
	debugger->priv->stdo_line = g_string_sized_new (FILE_BUFFER_SIZE);
	g_string_assign (debugger->priv->stdo_line, "");
	debugger->priv->stdo_acc = g_string_new ("");
	
	debugger->priv->stde_line = g_string_sized_new (FILE_BUFFER_SIZE);
	g_string_assign (debugger->priv->stde_line, "");
	
	debugger->priv->post_execution_flag = DEBUGGER_NONE;
}

static void
debugger_instance_init (Debugger *debugger)
{
	debugger_initialize (debugger);
}

Debugger*
debugger_new (GtkWindow *parent_win, GObject* instance)
{
	Debugger *debugger;
	
	debugger = g_object_new (DEBUGGER_TYPE, NULL);
	debugger->priv->parent_win = parent_win;
	debugger->priv->instance = instance;
	
	return debugger;
}

void
debugger_free (Debugger *debugger)
{
	g_return_if_fail (IS_DEBUGGER (debugger));

	g_object_unref (debugger);
}

gboolean
debugger_is_ready (Debugger *debugger)
{
	g_return_val_if_fail (IS_DEBUGGER (debugger), FALSE);
	return !debugger->priv->debugger_is_busy;
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

gboolean
debugger_program_is_loaded (Debugger *debugger)
{
	g_return_val_if_fail (IS_DEBUGGER (debugger), FALSE);
	return debugger->priv->prog_is_loaded;
}

static void
debugger_log_command (Debugger *debugger, const gchar *command)
{
	gchar* str;
	gsize len;

	if (debugger->priv->log == NULL) return;
	
	if (*command == '-')
	{
		str = g_strdup (command);
		len = strlen (command);

		/* Remove trailing carriage return */
		if (str[len - 1] == '\n') str[len - 1] = '\0';

		/* Log only MI command as other are echo */
		DEBUG_PRINT ("Cmd: %s", str);
		debugger_message_view_append (debugger, IANJUTA_MESSAGE_VIEW_TYPE_NORMAL, str);
		g_free (str);
	}
}

static void
debugger_log_output (Debugger *debugger, const gchar *line)
{
	gchar* str;
	const gchar* start;
	IAnjutaMessageViewType type;
	gsize len;

	if (debugger->priv->log == NULL) return;

	type = IANJUTA_MESSAGE_VIEW_TYPE_NORMAL;	
	switch (*line)
	{
	case '~':
		type = IANJUTA_MESSAGE_VIEW_TYPE_INFO;
		/* Go through */
	case '&':
		len = strlen(line);
		start = line + 1;

		/* Remove double quote if necessary */
		if ((line[1] == '\"') && (line[len - 1] == '\"')) start++;
		str = g_strcompress (line + 2);
		len = strlen (str);
		if (start == line + 2)
		{
			str[len - 1] = '\0';
			len--;
		}

		/* Remove trailing carriage return */
		if (str[len - 1] == '\n') str[len - 1] = '\0';

		debugger_message_view_append (debugger, type, str);
		g_free (str);
		break;
	case '^':
		if (strncmp(line + 1, "error", 5) == 0)
		{
			debugger_message_view_append (debugger, IANJUTA_MESSAGE_VIEW_TYPE_ERROR, line + 1);
		}
		else
		{
			debugger_message_view_append (debugger, IANJUTA_MESSAGE_VIEW_TYPE_WARNING, line + 1);
		}
		break;
	case '@':
		debugger_message_view_append (debugger, IANJUTA_MESSAGE_VIEW_TYPE_NORMAL, line + 1);
		break;
	default:
		debugger_message_view_append (debugger, IANJUTA_MESSAGE_VIEW_TYPE_NORMAL, line);
		break;
	}
}

static void
debugger_emit_ready (Debugger *debugger)
{
	if (!debugger->priv->debugger_is_busy)
	{
		if (debugger->priv->loading)
		{
			debugger->priv->starting = FALSE;
			debugger->priv->loading = FALSE;
			debugger->priv->exiting = FALSE;
			debugger->priv->stopping = FALSE;
			debugger->priv->solib_event = FALSE;
			g_signal_emit_by_name (debugger->priv->instance, "debugger-ready", IANJUTA_DEBUGGER_PROGRAM_LOADED);
		}
		else if (debugger->priv->starting)
		{
			debugger->priv->starting = FALSE;
			debugger->priv->loading = FALSE;
			debugger->priv->exiting = FALSE;
			debugger->priv->stopping = FALSE;
			debugger->priv->solib_event = FALSE;
			g_signal_emit_by_name (debugger->priv->instance, "debugger-ready", IANJUTA_DEBUGGER_STARTED);
		}
		else if (debugger->priv->exiting)
		{
			debugger->priv->exiting = FALSE;
			debugger->priv->stopping = FALSE;
			debugger->priv->solib_event = FALSE;
			g_signal_emit_by_name (debugger->priv->instance, "debugger-ready", IANJUTA_DEBUGGER_PROGRAM_LOADED);
		}
		else if (debugger->priv->solib_event)
		{
			debugger->priv->exiting = FALSE;
			debugger->priv->stopping = FALSE;
			debugger->priv->solib_event = FALSE;
			g_signal_emit_by_name (debugger->priv->instance, "sharedlib-event");
		}
		else if (debugger->priv->stopping)
		{
			debugger->priv->exiting = FALSE;
			debugger->priv->stopping = FALSE;
			debugger->priv->solib_event = FALSE;
			g_signal_emit_by_name (debugger->priv->instance, "debugger-ready", IANJUTA_DEBUGGER_PROGRAM_STOPPED);
		}
		else
		{
			if (debugger->priv->prog_is_running || debugger->priv->prog_is_attached)
			{
				g_signal_emit_by_name (debugger->priv->instance, "debugger-ready", IANJUTA_DEBUGGER_PROGRAM_STOPPED);
			}
			else if (debugger->priv->prog_is_loaded)
			{
				g_signal_emit_by_name (debugger->priv->instance, "debugger-ready", IANJUTA_DEBUGGER_PROGRAM_LOADED);
			}
			else
			{
				g_signal_emit_by_name (debugger->priv->instance, "debugger-ready", IANJUTA_DEBUGGER_STARTED);
			}
		}
	}
}

IAnjutaDebuggerState
debugger_get_state (Debugger *debugger)
{
	if (debugger->priv->debugger_is_busy)
	{
		return IANJUTA_DEBUGGER_BUSY;
	}
	else
	{
		if (debugger->priv->prog_is_running || debugger->priv->prog_is_attached)
		{
			return IANJUTA_DEBUGGER_PROGRAM_STOPPED;
		}
		else if (debugger->priv->prog_is_loaded)
		{
			return IANJUTA_DEBUGGER_PROGRAM_LOADED;
		}
		else if (debugger->priv->debugger_is_started)
		{
			return IANJUTA_DEBUGGER_STARTED;
		}
		else
		{
			return IANJUTA_DEBUGGER_STOPPED;
		}
	}
}

static void
debugger_clear_buffers (Debugger *debugger)
{
	DEBUG_PRINT ("In function: debugger_clear_buffers()");

	/* Clear the output line buffer */
	g_string_assign (debugger->priv->stdo_line, "");
	if (!debugger->priv->current_cmd.keep_result)
		g_string_assign (debugger->priv->stdo_acc, "");

	/* Clear the error line buffer */
	g_string_assign (debugger->priv->stde_line, "");
	
	/* Clear CLI output lines */
	g_list_foreach (debugger->priv->cli_lines, (GFunc)g_free, NULL);
	g_list_free (debugger->priv->cli_lines);
	debugger->priv->cli_lines = NULL;
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

static gboolean
debugger_queue_set_next_command (Debugger *debugger)
{
	DebuggerCommand *dc;

	DEBUG_PRINT ("In function: debugger_set_next_command()");
	
	dc = debugger_queue_get_next_command (debugger);
	if (!dc)
	{
		debugger->priv->current_cmd.cmd = NULL;
		debugger->priv->current_cmd.parser = NULL;
		debugger->priv->current_cmd.callback = NULL;
		debugger->priv->current_cmd.user_data = NULL;
		debugger->priv->current_cmd.suppress_error = FALSE;
		debugger->priv->current_cmd.keep_result = FALSE;

		return FALSE;
	}
	g_free (debugger->priv->current_cmd.cmd);
	debugger->priv->current_cmd.cmd = dc->cmd;
	debugger->priv->current_cmd.parser = dc->parser;
	debugger->priv->current_cmd.callback = dc->callback;
	debugger->priv->current_cmd.user_data = dc->user_data;
	debugger->priv->current_cmd.suppress_error = dc->suppress_error;
	debugger->priv->current_cmd.keep_result = dc->keep_result;
	g_free (dc);

	return TRUE;
}

static void
debugger_queue_command (Debugger *debugger, const gchar *cmd,
						gboolean suppress_error, gboolean keep_result,
						DebuggerParserFunc parser,
						IAnjutaDebuggerCallback callback, gpointer user_data)
{
	DebuggerCommand *dc;

	
	DEBUG_PRINT ("In function: debugger_queue_command (%s)", cmd);
	
	dc = g_malloc (sizeof (DebuggerCommand));
	if (dc)
	{
		dc->cmd = g_strdup(cmd);
		dc->parser = parser;
		dc->callback = callback;
		dc->user_data = user_data;
		dc->suppress_error = suppress_error;
		dc->keep_result = keep_result;
	}
	debugger->priv->cmd_queqe = g_list_append (debugger->priv->cmd_queqe, dc);
	debugger_queue_execute_command (debugger);
}

static void
debugger_queue_clear (Debugger *debugger)
{
	GList *node;

	DEBUG_PRINT ("In function: debugger_queue_clear()");
	
	node = debugger->priv->cmd_queqe;
	while (node)
	{
		g_free (((DebuggerCommand *)node->data)->cmd);
		g_free (node->data);
		node = g_list_next (node);
	}
	g_list_free (debugger->priv->cmd_queqe);
	debugger->priv->cmd_queqe = NULL;
	g_free (debugger->priv->current_cmd.cmd);
	debugger->priv->current_cmd.cmd = NULL;
	debugger->priv->current_cmd.parser = NULL;
	debugger->priv->current_cmd.callback = NULL;
	debugger->priv->current_cmd.user_data = NULL;
	debugger->priv->current_cmd.suppress_error = FALSE;
	debugger->priv->current_cmd.keep_result = FALSE;
	debugger_clear_buffers (debugger);
}

static void
debugger_execute_command (Debugger *debugger, const gchar *command)
{
	gchar *cmd;
	
	DEBUG_PRINT ("In function: debugger_execute_command(%s) %d\n",command, debugger->priv->debugger_is_busy);
	debugger->priv->debugger_is_busy++;
	debugger->priv->command_output_sent = FALSE;
	cmd = g_strconcat (command, "\n", NULL);
	debugger_log_command (debugger, cmd);
	anjuta_launcher_send_stdin (debugger->priv->launcher, cmd);
	g_free (cmd);
}

static void
debugger_queue_execute_command (Debugger *debugger)
{
	DEBUG_PRINT ("In function: debugger_queue_execute_command()");

	if (!debugger->priv->debugger_is_busy &&
		!debugger->priv->starting &&
		g_list_length (debugger->priv->cmd_queqe) >= 1)
	{
		debugger_clear_buffers (debugger);
		if (debugger_queue_set_next_command (debugger))
		debugger_execute_command (debugger, debugger->priv->current_cmd.cmd);
	}
}

static void
debugger_load_executable_finish (Debugger *debugger, const GDBMIValue *mi_results,
								const GList *cli_results, GError *error)
{
	DEBUG_PRINT ("Program loaded");
	debugger->priv->prog_is_loaded = TRUE;
	
	g_signal_emit_by_name (debugger->priv->instance, "program-loaded");
}

void
debugger_load_executable (Debugger *debugger, const gchar *prog)
{
	gchar *command, *dir, *msg;

	g_return_if_fail (IS_DEBUGGER (debugger));
	g_return_if_fail (prog != NULL);

	DEBUG_PRINT ("In function: debugger_load_executable(%s)", prog);

	if (debugger->priv->output_callback)
	{
		msg = g_strconcat (_("Loading Executable: "), prog, "\n", NULL);
		debugger->priv->output_callback (IANJUTA_DEBUGGER_OUTPUT, msg, 
							   debugger->priv->output_user_data);
		g_free (msg);
	}

	command = g_strconcat ("-file-exec-and-symbols ", prog, NULL);
	dir = g_path_get_dirname (prog);
/* TODO
	anjuta_set_execution_dir(dir);
*/
	g_free (dir);
	debugger_queue_command (debugger, command, FALSE, FALSE, debugger_load_executable_finish, NULL, NULL);
	g_free (command);
	debugger->priv->starting = TRUE;
	debugger->priv->terminating = FALSE;
}

void
debugger_load_core (Debugger *debugger, const gchar *core)
{
	gchar *command, *dir, *msg;

	g_return_if_fail (IS_DEBUGGER (debugger));
	g_return_if_fail (core != NULL);
	
	DEBUG_PRINT ("In function: debugger_load_core(%s)", core);

	if (debugger->priv->output_callback)
	{
		msg = g_strconcat (_("Loading Core: "), core, "\n", NULL);
		debugger->priv->output_callback (IANJUTA_DEBUGGER_OUTPUT, msg, 
									 debugger->priv->output_user_data);
		g_free (msg);
	}

	command = g_strconcat ("core ", core, NULL);
	dir = g_path_get_dirname (core);
	debugger->priv->search_dirs = 
		g_list_prepend (debugger->priv->search_dirs, dir);
	debugger_queue_command (debugger, command, FALSE, FALSE, NULL, NULL, NULL);
	g_free (command);
}

gboolean
debugger_start (Debugger *debugger, const GList *search_dirs,
				const gchar *prog, gboolean is_libtool_prog)
{
	gchar *command_str, *dir, *tmp, *text, *msg;
	gchar *exec_dir;
	gboolean ret;
	const GList *node;
	AnjutaLauncher *launcher;
	GList *dir_list = NULL;
	gchar *term = NULL;
	
	DEBUG_PRINT ("In function: debugger_start(%s) libtool %d", prog == NULL ? "(null)" : prog, is_libtool_prog);

	if (anjuta_util_prog_is_installed ("gdb", TRUE) == FALSE)
		return FALSE;

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
		return FALSE;
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
		if (strncmp (text, "file://", 7) == 0)
		{
			text += 7;
		}
		else
		{
			g_warning ("Debugger source search uri '%s' is not a local uri", text);
		}
		
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
			command_str = g_strdup_printf (GDB_PATH " -f -n -i=mi2 %s %s "
										   "-x %s/gdb.init %s", dir, term == NULL ? "" : term,
										   PACKAGE_DATA_DIR, prog);
		}
		else
		{
			command_str = g_strdup_printf ("libtool --mode=execute " GDB_PATH
										   " -f -n -i=mi2 %s %s "
										   "-x %s/gdb.init %s", dir, term == NULL ? "" : term,
										   PACKAGE_DATA_DIR, prog);
		}
	}
	else
	{
		if (is_libtool_prog == FALSE)
		{
			command_str = g_strdup_printf (GDB_PATH " -f -n -i=mi2 %s %s "
										   "-x %s/gdb.init ", term == NULL ? "" : term,
										   dir, PACKAGE_DATA_DIR);
		}
		else
		{
			command_str = g_strdup_printf ("libtool --mode=execute " GDB_PATH
										   " -f -n -i=mi2 %s %s -x "
										   "%s/gdb.init ",
										   dir, term == NULL ? "" : term, PACKAGE_DATA_DIR);
		}
	}
	g_free (dir);
	g_free (term);
	debugger->priv->starting = TRUE;
	debugger->priv->terminating = FALSE;
	debugger->priv->loading = prog != NULL ? TRUE : FALSE;
	debugger->priv->debugger_is_busy = 1;

	/* Prepare for launch. */
	launcher = debugger->priv->launcher;
	anjuta_launcher_set_terminate_on_exit (launcher, TRUE);
	g_signal_connect (G_OBJECT (launcher), "child-exited",
					  G_CALLBACK (on_gdb_terminated), debugger);
	ret = anjuta_launcher_execute (launcher, command_str,
								   on_gdb_output_arrived, debugger);

	if (ret)
	{
		debugger->priv->debugger_is_started = TRUE;
		debugger->priv->prog_is_loaded = prog != NULL;
	}
	anjuta_launcher_set_encoding (launcher, "ISO-8859-1");
	
	if (debugger->priv->output_callback != NULL)
	{
		if (ret == TRUE)
		{
			/* TODO		anjuta_update_app_status (TRUE, _("Debugger")); */
			debugger->priv->output_callback (IANJUTA_DEBUGGER_OUTPUT,
										 _("Getting ready to start debugging "
										   "session...\n"),
										 debugger->priv->output_user_data);
		
			if (prog)
			{
				msg = g_strconcat (_("Loading Executable: "), prog, "\n", NULL);
				debugger->priv->output_callback (IANJUTA_DEBUGGER_OUTPUT,
											 msg,
											 debugger->priv->output_user_data);
				g_free (msg);
			}
			else
			{
				debugger->priv->output_callback (IANJUTA_DEBUGGER_OUTPUT,
									   _("No executable specified.\n"),
									   debugger->priv->output_user_data);
				debugger->priv->output_callback (IANJUTA_DEBUGGER_OUTPUT,
											 _("Open an executable or attach "
											   "to a process to start "
											   "debugging.\n"),
											 debugger->priv->output_user_data);
			}
		}
		else
		{
			debugger->priv->output_callback (IANJUTA_DEBUGGER_OUTPUT,
										 _("There was an error whilst "
										   "launching the debugger.\n"),
										 debugger->priv->output_user_data);
			debugger->priv->output_callback (IANJUTA_DEBUGGER_OUTPUT,
										 _("Make sure 'gdb' is installed "
										   "on the system.\n"),
										 debugger->priv->output_user_data);
		}
	}
	g_free (command_str);

	return TRUE;
}

static void
gdb_stdout_line_arrived (Debugger *debugger, const gchar * chars)
{
	gint i = 0;

	while (chars[i])
	{
		if (chars[i] == '\n')
		{
			debugger_stdo_flush (debugger);
		}
		else
		{
			g_string_append_c (debugger->priv->stdo_line, chars[i]);
		}
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
	DEBUG_PRINT ("on gdb output arrived");

	/* Do not emit signal when the debugger is destroyed */
	if (debugger->priv->instance == NULL) return;

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
			DEBUG_PRINT ("debugger stop in handle post execution\n");
			debugger_stop (debugger);
			break;
		case DEBUGGER_RERUN_PROGRAM:
			DEBUG_PRINT ("debugger run in handle post execution\n");
			debugger_run (debugger);
			break;
		default:
			g_warning ("Execution should not reach here");
	}
}

static void
debugger_process_frame (Debugger *debugger, const GDBMIValue *val)
{
	const GDBMIValue *file, *line, *frame, *addr, *fullname, *thread;
	const gchar *file_str = NULL;
	guint line_num = 0;
	gulong addr_num = 0;
	
	g_return_if_fail (val != NULL);

	thread = gdbmi_value_hash_lookup (val, "thread-id");
	if (thread)
	{
		debugger->priv->current_thread = strtoul (gdbmi_value_literal_get (thread), NULL, 0);
	}
	debugger->priv->current_frame = 0;

	frame = gdbmi_value_hash_lookup (val, "frame");
	if (frame)
	{
		fullname = gdbmi_value_hash_lookup (frame, "fullname");
		if (fullname)
		{
			file_str = gdbmi_value_literal_get (fullname);
			if (*file_str == '\0') file_str = NULL;
		}
		else
		{
			file = gdbmi_value_hash_lookup (frame, "file");
			if (file)
			{
				file_str = gdbmi_value_literal_get (file);
				if (*file_str == '\0') file_str = NULL;
			}
		}

		if (file_str != NULL)
		{
			line = gdbmi_value_hash_lookup (frame, "line");
			if (line)
			{
				line_num = strtoul (gdbmi_value_literal_get (line), NULL, 0);
			}
		}

		addr = gdbmi_value_hash_lookup (frame, "addr");
		if (addr)
		{
			addr_num = strtoul (gdbmi_value_literal_get (addr), NULL, 0);
		}
		debugger_program_moved (debugger, file_str, line_num, addr_num);
	}
}

static GError*
gdb_parse_error (Debugger *debugger, const GDBMIValue *mi_results)
{
	const GDBMIValue *message;
	const gchar *literal;
	guint code = IANJUTA_DEBUGGER_UNKNOWN_ERROR;

	message = gdbmi_value_hash_lookup (mi_results, "msg");
	literal = gdbmi_value_literal_get (message);

	if ((mi_results != NULL)
	&& ((message = gdbmi_value_hash_lookup (mi_results, "msg")) != NULL)
	&& ((literal = gdbmi_value_literal_get (message)) != NULL)
	&& (*literal != '\0'))
	{
		code = gdb_match_error (literal);
		DEBUG_PRINT ("error code %d", code);
	}
	else
	{
		/* No error message */
		literal = "Error without a message";
	}

	return g_error_new_literal (IANJUTA_DEBUGGER_ERROR, code, literal);
}


/* Parsing output
 *---------------------------------------------------------------------------*/

static void
debugger_parse_output (Debugger *debugger)
{
	gchar *line;
	
	line = debugger->priv->stdo_line->str;

	if (line[0] == '\032' && line[1] == '\032')
	{
		gchar *filename;
		guint lineno;		
		
		gdb_util_parse_error_line (&(line[2]), &filename, &lineno);
		if (filename)
		{
			debugger_program_moved (debugger, filename, lineno, 0);
			g_free (filename);
		}
	}
	else
	{
		gchar *proper_msg;
		gsize len;
		
		len = strlen (line);
		if (line[1] == '\"' && line [strlen(line) - 1] == '\"')
		{
			line[1] = line[0];
			/* Reserve space for an additional carriage return */
			line[strlen(line) - 1] = ' ';
			proper_msg = g_strcompress (line + 1);
			len = strlen (proper_msg) - 1;
			proper_msg[len] = '\0';
		}
		else
		{
			/* Reserve space for an additional carriage return */
			proper_msg = g_strndup (line, len + 1);
		}

		if (strcmp(proper_msg, "~Stopped due to shared library event\n") == 0)
		{
			/* Recognize a solib event */
			debugger->priv->solib_event = TRUE;
			g_free (proper_msg);
		}
		else if (debugger->priv->current_cmd.parser)
		{
			/* Save GDB CLI output */
			debugger->priv->cli_lines = g_list_prepend (debugger->priv->cli_lines,
													   proper_msg);
		}
		else
		{
			/* Discard CLI output */
			g_free (proper_msg);
		}
	}
}

static void
debugger_parse_stopped (Debugger *debugger)
{
	gchar *line = debugger->priv->stdo_line->str;
		

	if (!debugger->priv->solib_event)	
	{
		gboolean program_exited = FALSE;		
		GDBMIValue *val;

		/* Check if program has exited */
		val = gdbmi_value_parse (line);
		if (val)
		{
			const GDBMIValue *reason;
			const gchar *str = NULL;

			debugger_process_frame (debugger, val);
			
			reason = gdbmi_value_hash_lookup (val, "reason");
			if (reason)
				str = gdbmi_value_literal_get (reason);

			if (str && (strncmp (str, "exited", 6) == 0))
			{
				program_exited = TRUE;
			}
	
			/* Emit signal received if necessary */	
			if (str && strcmp (str, "exited-signalled") == 0)
			{
				const GDBMIValue *signal_name, *signal_meaning;
				const gchar *signal_str, *signal_meaning_str;

				signal_name = gdbmi_value_hash_lookup (val, "signal-name");
				signal_str = gdbmi_value_literal_get (signal_name);
				signal_meaning = gdbmi_value_hash_lookup (val, "signal-meaning");
				signal_meaning_str = gdbmi_value_literal_get (signal_meaning);
				g_signal_emit_by_name (debugger->priv->instance, "signal-received", signal_str, signal_meaning_str);
			}
			else if (str && strcmp (str, "signal-received") == 0)
			{
				const GDBMIValue *signal_name, *signal_meaning;
				const gchar *signal_str, *signal_meaning_str;

				signal_name = gdbmi_value_hash_lookup (val, "signal-name");
				signal_str = gdbmi_value_literal_get (signal_name);
				signal_meaning = gdbmi_value_hash_lookup (val, "signal-meaning");
				signal_meaning_str = gdbmi_value_literal_get (signal_meaning);
	
				g_signal_emit_by_name (debugger->priv->instance, "signal-received", signal_str, signal_meaning_str);
			}

			if (debugger->priv->output_callback)
			{	
				if (str && strcmp (str, "exited-normally") == 0)
				{
					debugger->priv->output_callback (IANJUTA_DEBUGGER_OUTPUT,
										   _("Program exited normally\n"),
										   debugger->priv->output_user_data);
				}
				else if (str && strcmp (str, "exited") == 0)
				{
					const GDBMIValue *errcode;
					const gchar *errcode_str;
					gchar *msg;
			
					errcode = gdbmi_value_hash_lookup (val, "exit-code");
					errcode_str = gdbmi_value_literal_get (errcode);
					msg = g_strdup_printf (_("Program exited with error code %s\n"),
									   errcode_str);
					debugger->priv->output_callback (IANJUTA_DEBUGGER_OUTPUT,
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
					debugger->priv->output_callback (IANJUTA_DEBUGGER_OUTPUT,
										   msg, debugger->priv->output_user_data);
					g_free (msg);
				}
				else if (str && strcmp (str, "function-finished") == 0)
				{
					debugger->priv->output_callback (IANJUTA_DEBUGGER_OUTPUT,
										   _("Function finished\n"),
											   debugger->priv->output_user_data);
				}
				else if (str && strcmp (str, "end-stepping-range") == 0)
				{
					debugger->priv->output_callback (IANJUTA_DEBUGGER_OUTPUT,
										   _("Stepping finished\n"),
										   debugger->priv->output_user_data);
				}
				else if (str && strcmp (str, "location-reached") == 0)
				{
					debugger->priv->output_callback (IANJUTA_DEBUGGER_OUTPUT,
										   _("Location reached\n"),
											   debugger->priv->output_user_data);
				}
			}
		}
	
		if (program_exited)
		{
			debugger->priv->prog_is_running = FALSE;
			debugger->priv->prog_is_attached = FALSE;
			debugger_handle_post_execution (debugger);
			debugger->priv->exiting = TRUE;
		}
		else
		{
//			g_signal_emit_by_name (debugger->priv->instance, "program-stopped");
			debugger->priv->stopping = TRUE;
		}
		
		debugger->priv->cli_lines = g_list_reverse (debugger->priv->cli_lines);
		if ((debugger->priv->current_cmd.cmd != NULL) &&
			(debugger->priv->current_cmd.parser != NULL))
		{
			debugger->priv->current_cmd.parser (debugger, val,
												debugger->priv->cli_lines, FALSE);
			debugger->priv->command_output_sent = TRUE;
			DEBUG_PRINT ("In function: Sending output...");
		}
		
		if (val)
			gdbmi_value_free (val);
	}
}

static void
debugger_parse_prompt (Debugger *debugger)
{
	/* If the parser has not yet been called, call it now. */
	if (debugger->priv->command_output_sent == FALSE &&
		debugger->priv->current_cmd.parser)
	{
		debugger->priv->current_cmd.parser (debugger, NULL,
											debugger->priv->cli_lines, FALSE);
		debugger->priv->command_output_sent = TRUE;
	}
	
	debugger->priv->debugger_is_busy--;
	debugger_queue_execute_command (debugger);	/* Next command. Go. */
	debugger_emit_ready (debugger);
}

static void
debugger_stdo_flush (Debugger *debugger)
{
	gchar *line;

	line = debugger->priv->stdo_line->str;

	DEBUG_PRINT ("Log: %s\n", line);
	debugger_log_output (debugger, line);	
	if (strlen (line) == 0)
	{
		return;
	}
	if (strncasecmp (line, "^error", 6) == 0)
	{
		/* GDB reported error */
		if ((debugger->priv->current_cmd.keep_result)  || (debugger->priv->stdo_acc->len != 0))
		{
			/* Keep result for next command */

			if (debugger->priv->stdo_acc->len == 0)
			{
				g_string_append (debugger->priv->stdo_acc, line);
			}
			else
			{
				line = strchr (line, ',');
				if (line != NULL)
				{
					g_string_append (debugger->priv->stdo_acc, line);
				}
			}
			line = debugger->priv->stdo_acc->str;
		}

		GDBMIValue *val = gdbmi_value_parse (line);
		GError *error;

		error = gdb_parse_error (debugger, val);

		if (debugger->priv->current_cmd.parser != NULL)
		{
			debugger->priv->current_cmd.parser (debugger, val, debugger->priv->cli_lines, error);
			debugger->priv->command_output_sent = TRUE;				}
		else
		{
			anjuta_util_dialog_error (debugger->priv->parent_win, "%s", error->message);
		}
		g_error_free (error);
		gdbmi_value_free (val);
	}
	else if (strncasecmp(line, "^running", 8) == 0)
	{
		/* Program started running */
		debugger->priv->prog_is_running = TRUE;
		/* debugger->priv->skip_next_prompt = TRUE; Replaced by debugger_is_busy++ */
		debugger->priv->debugger_is_busy++;
		g_signal_emit_by_name (debugger->priv->instance, "program-running");
	}
	else if (strncasecmp (line, "*stopped", 8) == 0)
	{
		/* Process has stopped */
		debugger_parse_stopped (debugger);
	}
	else if (strncasecmp (line, "^done", 5) == 0)
	{
		if ((debugger->priv->current_cmd.keep_result)  || (debugger->priv->stdo_acc->len != 0))
		{
			/* Keep result for next command */

			if (debugger->priv->stdo_acc->len == 0)
			{
				g_string_append (debugger->priv->stdo_acc, line);
			}
			else
			{
				line = strchr (line, ',');
				if (line != NULL)
				{
					g_string_append (debugger->priv->stdo_acc, line);
				}
			}
			line = debugger->priv->stdo_acc->str;
		}

	  	if (!debugger->priv->current_cmd.keep_result)	
		{	
			/* GDB command has reported output */
			GDBMIValue *val = gdbmi_value_parse (line);
		
			debugger->priv->cli_lines = g_list_reverse (debugger->priv->cli_lines);
			if ((debugger->priv->current_cmd.cmd != NULL) &&
				(debugger->priv->current_cmd.parser != NULL))
			{
				debugger->priv->current_cmd.parser (debugger, val,
										  debugger->priv->cli_lines, FALSE);
				debugger->priv->command_output_sent = TRUE;
				DEBUG_PRINT ("In function: Sending output...");
			}
			else /* if (val) */
			{
				/*g_signal_emit_by_name (debugger, "results-arrived",
								   debugger->priv->current_cmd.cmd, val);*/
			}
	
			if (val)
			{
				/*debugger_process_frame (debugger, val);*/
				gdbmi_value_free (val);
			}
		}

		if (!debugger->priv->current_cmd.keep_result)
		{
			g_string_assign (debugger->priv->stdo_acc, "");
		}
	}
	else if (strncasecmp (line, GDB_PROMPT, strlen (GDB_PROMPT)) == 0)
	{
		debugger_parse_prompt (debugger);
	}
	else
	{
		debugger_parse_output (debugger);
	}

	/* Clear the line buffer */
	g_string_assign (debugger->priv->stdo_line, "");
}

void
debugger_stde_flush (Debugger *debugger)
{
	if ((debugger->priv->output_callback) && (strlen (debugger->priv->stde_line->str) > 0))
	{
		debugger->priv->output_callback (IANJUTA_DEBUGGER_ERROR_OUTPUT,
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
	GError *err = NULL;

	g_signal_handlers_disconnect_by_func (G_OBJECT (launcher),
										  G_CALLBACK (on_gdb_terminated),
										  debugger);
	
	DEBUG_PRINT ("In function: gdb_terminated()");
	
	/* Clear the command queue & Buffer */
	debugger_clear_buffers (debugger);

	/* Good Bye message */
	/*if (!debugger->priv->terminating)
	{
		anjuta_util_dialog_error (debugger->priv->parent_win,
		_("gdb terminated unexpectedly with status %d\n"), status);
	}*/
	debugger->priv->prog_is_running = FALSE;
	debugger->priv->prog_is_attached = FALSE;
	debugger->priv->prog_is_loaded = FALSE;
	debugger->priv->debugger_is_busy = 0;
	debugger->priv->skip_next_prompt = FALSE;
	if (!debugger->priv->terminating)
	{
		err = g_error_new  (IANJUTA_DEBUGGER_ERROR,
				IANJUTA_DEBUGGER_OTHER_ERROR,
				"gdb terminated with status %d", status);
	}
	debugger->priv->terminating = FALSE;
	debugger->priv->debugger_is_started = FALSE;
	if (debugger->priv->instance)
	{
		g_signal_emit_by_name (debugger->priv->instance, "debugger-stopped", err);
	}
	if (err != NULL) g_error_free (err);
}

static void
debugger_stop_real (Debugger *debugger)
{
	DEBUG_PRINT ("In function: debugger_stop_real()");
	
	/* if program is attached - detach from it before quiting */
	if (debugger->priv->prog_is_attached == TRUE)
		debugger_queue_command (debugger, "detach", FALSE, FALSE, NULL, NULL, NULL);

	debugger->priv->terminating = TRUE;
	debugger_queue_command (debugger, "-gdb-exit", FALSE, FALSE, NULL, NULL, NULL);
}

gboolean
debugger_stop (Debugger *debugger)
{
#if 0
	gboolean ret = TRUE;

	if (debugger->priv->prog_is_running == TRUE)
	{
		GtkWidget *dialog;
		gchar *mesg;
		
		if (debugger->priv->prog_is_attached == TRUE)
			mesg = _("The program is attached.\n"
				   "Do you still want to stop the debugger?");
		else
			mesg = _("The program is running.\n"
				   "Do you still want to stop the debugger?");
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
#endif
	debugger_stop_real (debugger);

	return TRUE;
}

gboolean
debugger_abort (Debugger *debugger)
{
	DEBUG_PRINT ("In function: debugger_abort()");

	/* Stop inferior */	
	if ((debugger->priv->prog_is_attached == FALSE) && (debugger->priv->inferior_pid != 0))
	{
		kill (debugger->priv->inferior_pid, SIGTERM);
		debugger->priv->inferior_pid = 0;
	}

	/* Stop gdb */
	debugger->priv->terminating = TRUE;
	g_signal_handlers_disconnect_by_func (G_OBJECT (debugger->priv->launcher), G_CALLBACK (on_gdb_terminated), debugger);
	anjuta_launcher_reset (debugger->priv->launcher);

	/* Free memory */	
	debugger_queue_clear (debugger);
	g_list_foreach (debugger->priv->search_dirs, (GFunc)g_free, NULL);
	g_list_free (debugger->priv->search_dirs);
	debugger->priv->search_dirs = NULL;

	/* Emit signal, state of the debugger must be DEBUGGER_STOPPED */
	debugger->priv->prog_is_running = FALSE;
	debugger->priv->prog_is_attached = FALSE;
	debugger->priv->prog_is_loaded = FALSE;
	debugger->priv->debugger_is_busy = 0;
	debugger->priv->debugger_is_started = FALSE;
	if (debugger->priv->instance != NULL)
	{
		g_signal_emit_by_name (debugger->priv->instance, "debugger-stopped", NULL);
		debugger->priv->instance = NULL;
	}

	return TRUE;
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
		path = NULL;
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
debugger_set_output_callback (Debugger *debugger, IAnjutaDebuggerOutputCallback callback, gpointer user_data)
{
	debugger->priv->output_callback = callback;
	debugger->priv->output_user_data = user_data;
}

void
debugger_program_moved (Debugger *debugger, const gchar *file,
						  gint line, guint address)
{
	gchar *src_path;
	
	if ((file != NULL) && (*file != G_DIR_SEPARATOR))
	{
		src_path = debugger_get_source_path (debugger, file);
		g_signal_emit_by_name (debugger->priv->instance, "program-moved", debugger->priv->inferior_pid, debugger->priv->current_thread, address, src_path, line);
		g_free (src_path);
	}
	else
	{
		g_signal_emit_by_name (debugger->priv->instance, "program-moved", debugger->priv->inferior_pid, debugger->priv->current_thread, address, file, line);
	}
}

static void
debugger_info_program_finish (Debugger *debugger, const GDBMIValue *mi_results,
								const GList *cli_results, GError *error)
{
	DEBUG_PRINT ("In function: debugger_info_program()");

	/* Hack: find message string giving inferior pid */
	while (cli_results != NULL)
	{
		gchar* child_proc;
	       
		child_proc = strstr(cli_results->data, " child process ");
		if (child_proc != NULL)
		{
			debugger->priv->inferior_pid = strtoul (child_proc + 15, NULL, 10);
			break;
		}
		cli_results = g_list_next (cli_results);
	}		
}

void
debugger_start_program (Debugger *debugger, const gchar* args, const gchar* tty)
{
	gchar *cmd;

	DEBUG_PRINT ("In function: debugger_start_program()");

	g_return_if_fail (IS_DEBUGGER (debugger));
	g_return_if_fail (debugger->priv->prog_is_running == FALSE);

	/* Without a terminal, the output of the debugged program
	 * are lost */
	if (tty)
	{
		cmd = g_strdup_printf ("-inferior-tty-set %s", tty);
		debugger_queue_command (debugger, cmd, FALSE, FALSE, NULL, NULL, NULL);
		g_free (cmd);
	}

	debugger->priv->inferior_pid = 0;
	debugger_queue_command (debugger, "-break-insert -t main", FALSE, FALSE, NULL, NULL, NULL);
	if (args && (*args))
	{
		cmd = g_strconcat ("-exec-arguments ", args, NULL);
		debugger_queue_command (debugger, cmd, FALSE, FALSE, NULL, NULL, NULL);
		g_free (cmd);
	}

	debugger_queue_command (debugger, "-exec-run", FALSE, FALSE, NULL, NULL, NULL);
	
	/* Get pid of program on next stop */
	debugger_queue_command (debugger, "info program", FALSE, FALSE, debugger_info_program_finish, NULL, NULL);
	debugger->priv->post_execution_flag = DEBUGGER_NONE;
}

static void
debugger_attach_process_finish (Debugger *debugger, const GDBMIValue *mi_results,
								const GList *cli_results, GError *error)
{
	DEBUG_PRINT ("Program attach finished");
	if (debugger->priv->output_callback)
	{
		debugger->priv->output_callback (IANJUTA_DEBUGGER_OUTPUT,
									 _("Program attached\n"),
									 debugger->priv->output_user_data);
	}
	debugger->priv->prog_is_attached = TRUE;
	debugger->priv->prog_is_running = TRUE;
	//debugger_emit_status (debugger);
}

static void
debugger_attach_process_real (Debugger *debugger, pid_t pid)
{
	gchar *buff;

	DEBUG_PRINT ("In function: debugger_attach_process_real()");

	if (debugger->priv->output_callback)
	{	
		buff = g_strdup_printf (_("Attaching to process: %d...\n"), pid);
		debugger->priv->output_callback (IANJUTA_DEBUGGER_OUTPUT,
							   buff, debugger->priv->output_user_data);
		g_free (buff);
	}

	debugger->priv->inferior_pid = pid;	
	buff = g_strdup_printf ("attach %d", pid);
	debugger_queue_command (debugger, buff, FALSE, FALSE, 
							debugger_attach_process_finish, NULL, NULL);
	g_free (buff);
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

	/*	
	debugger->priv->post_execution_flag = DEBUGGER_RERUN_PROGRAM;
	debugger_stop_program (debugger);
	
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
		debugger_queue_command (debugger, "detach", FALSE, FALSE, NULL, NULL, NULL);
	else
	{
		/* FIXME: Why doesn't -exec-abort work??? */
		/* debugger_queue_command (debugger, "-exec-abort", NULL, NULL); */
		debugger_queue_command (debugger, "kill", FALSE, FALSE, NULL, NULL, NULL);
		debugger->priv->prog_is_running = FALSE;
		debugger->priv->prog_is_attached = FALSE;
		g_signal_emit_by_name (debugger->priv->instance, "program-exited");
		if (debugger->priv->output_callback)
		{
			debugger->priv->output_callback (IANJUTA_DEBUGGER_OUTPUT,
								   _("Program terminated\n"),
								   debugger->priv->output_user_data);
		}
		debugger_handle_post_execution (debugger);
	}
}

static void
debugger_detach_process_finish (Debugger *debugger, const GDBMIValue *mi_results,
								const GList *cli_results, GError *error)
{
	DEBUG_PRINT ("Program detach finished");
	if (debugger->priv->output_callback)
	{
		debugger->priv->output_callback (IANJUTA_DEBUGGER_OUTPUT,
									 _("Program detached\n"),
									 debugger->priv->output_user_data);
	}
	debugger->priv->prog_is_attached = FALSE;
	debugger->priv->prog_is_running = FALSE;
	g_signal_emit_by_name (debugger->priv->instance, "program-exited");
}

void
debugger_detach_process (Debugger *debugger)
{
	gchar *buff;

	DEBUG_PRINT ("In function: debugger_detach_process()");
	
	g_return_if_fail (debugger->priv->prog_is_attached == TRUE);

	if (debugger->priv->output_callback)
	{	
		buff = g_strdup_printf (_("Detaching the process...\n"));
		debugger->priv->output_callback (IANJUTA_DEBUGGER_OUTPUT,
							   buff, debugger->priv->output_user_data);
		g_free (buff);
	}
	
	debugger_queue_command (debugger, "detach", FALSE, FALSE, 
							debugger_detach_process_finish, NULL, NULL);
	debugger->priv->prog_is_attached = FALSE;
}

void
debugger_interrupt (Debugger *debugger)
{
	DEBUG_PRINT ("In function: debugger_interrupt()");

	g_return_if_fail (IS_DEBUGGER (debugger));
	g_return_if_fail (debugger->priv->prog_is_running == TRUE);

	if (debugger->priv->output_callback)
	{	
		debugger->priv->output_callback (IANJUTA_DEBUGGER_OUTPUT,
							   _("Interrupting the process\n"),
							   debugger->priv->output_user_data);
	}

	if (debugger->priv->inferior_pid == 0)
	{
		/* In case we do not have the inferior pid, send signal to gdb */
		anjuta_launcher_signal (debugger->priv->launcher, SIGINT);
	}
	else
	{
		/* Send signal directly to inferior */
		kill (debugger->priv->inferior_pid, SIGINT);
	}
	//g_signal_emit_by_name (debugger->priv->instance, "program-running");
}

void
debugger_run (Debugger *debugger)
{
	DEBUG_PRINT ("In function: debugger_run()");
	
	g_return_if_fail (IS_DEBUGGER (debugger));
	g_return_if_fail (debugger->priv->prog_is_running == TRUE);

	/* Program running - continue */
	debugger_queue_command (debugger, "-exec-continue", FALSE, FALSE, NULL, NULL, NULL);
}

void
debugger_step_in (Debugger *debugger)
{
	DEBUG_PRINT ("In function: debugger_step_in()");
	
	g_return_if_fail (IS_DEBUGGER (debugger));
	g_return_if_fail (debugger->priv->prog_is_running == TRUE);

	debugger_queue_command (debugger, "-exec-step", FALSE, FALSE, NULL, NULL, NULL);
}

void
debugger_step_over (Debugger *debugger)
{
	DEBUG_PRINT ("In function: debugger_step_over()");
	
	g_return_if_fail (IS_DEBUGGER (debugger));
	g_return_if_fail (debugger->priv->prog_is_running == TRUE);
	
	debugger_queue_command (debugger, "-exec-next", FALSE, FALSE, NULL, NULL, NULL);
}

void
debugger_step_out (Debugger *debugger)
{
	DEBUG_PRINT ("In function: debugger_step_out()");
	
	g_return_if_fail (IS_DEBUGGER (debugger));
	g_return_if_fail (debugger->priv->prog_is_running == TRUE);
	
	debugger_queue_command (debugger, "-exec-finish", FALSE, FALSE, NULL, NULL, NULL);
}

void
debugger_run_to_location (Debugger *debugger, const gchar *loc)
{
	gchar *buff;

	DEBUG_PRINT ("In function: debugger_run_to_location()");
	
	g_return_if_fail (IS_DEBUGGER (debugger));
	g_return_if_fail (debugger->priv->prog_is_running == TRUE);
	
	buff = g_strdup_printf ("-exec-until %s", loc);
	debugger_queue_command (debugger, buff, FALSE, FALSE, NULL, NULL, NULL);
	g_free (buff);
}

void
debugger_run_to_position (Debugger *debugger, const gchar *file, guint line)
{
	gchar *buff;

	DEBUG_PRINT ("In function: debugger_run_to_position()");
	
	g_return_if_fail (IS_DEBUGGER (debugger));
	g_return_if_fail (debugger->priv->prog_is_running == TRUE);
	
	buff = g_strdup_printf ("-exec-until %s:%d", file, line);
	debugger_queue_command (debugger, buff, FALSE, FALSE, NULL, NULL, NULL);
	g_free (buff);
}

void
debugger_command (Debugger *debugger, const gchar *command,
				  gboolean suppress_error, DebuggerParserFunc parser,
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
		debugger_queue_command (debugger, command, suppress_error, FALSE,
								parser, user_data, NULL);
	}
}

static void
debugger_add_breakpoint_finish (Debugger *debugger, const GDBMIValue *mi_results, const GList *cli_results, GError *error)
{
	const GDBMIValue *brkpnt;
	const GDBMIValue *literal;
	const gchar* value;
	IAnjutaDebuggerBreakpoint bp;
	IAnjutaDebuggerCallback callback = debugger->priv->current_cmd.callback;
	gpointer user_data = debugger->priv->current_cmd.user_data;

	memset (&bp, 0, sizeof (bp));

	bp.type = 0;

	if ((error != NULL) || (mi_results == NULL))
	{
		/* Call callback in all case (useful for enable that doesn't return
 		* anything */
		if (callback != NULL)
			callback (NULL, user_data, error);
	}
	else
	{
		brkpnt = gdbmi_value_hash_lookup (mi_results, "bkpt");

		literal = gdbmi_value_hash_lookup (brkpnt, "number");
		if (literal)
		{
			value = gdbmi_value_literal_get (literal);
			bp.id = strtoul (value, NULL, 10);
		}

		literal = gdbmi_value_hash_lookup (brkpnt, "file");
		if (literal)
		{
			value = gdbmi_value_literal_get (literal);
			bp.file = (gchar *)value;
		}
	
		literal = gdbmi_value_hash_lookup (brkpnt, "line");
		if (literal)
		{
			value = gdbmi_value_literal_get (literal);
			bp.line = strtoul (value, NULL, 10);
			bp.type |= IANJUTA_DEBUGGER_BREAK_ON_LINE;
		}
		
		literal = gdbmi_value_hash_lookup (brkpnt, "type");
		if (literal)
		{
		      	value = gdbmi_value_literal_get (literal);
		}
								
		literal = gdbmi_value_hash_lookup (brkpnt, "disp");
		if (literal)
		{
			value = gdbmi_value_literal_get (literal);
			if (strcmp (value, "keep") == 0)
			{
				bp.type |= IANJUTA_DEBUGGER_BREAK_WITH_TEMPORARY;
				bp.temporary = FALSE;
			}
			else if (strcmp (value, "nokeep") == 0)
			{
				bp.type |= IANJUTA_DEBUGGER_BREAK_WITH_TEMPORARY;
				bp.temporary = TRUE;
			}
		}
										
		literal = gdbmi_value_hash_lookup (brkpnt, "enabled");
		if (literal)
		{       	
			value = gdbmi_value_literal_get (literal);
			if (strcmp (value, "n") == 0)
			{
				bp.type |= IANJUTA_DEBUGGER_BREAK_WITH_ENABLE;
				bp.enable = FALSE;
			}
			else if (strcmp (value, "y") == 0)
			{
				bp.type |= IANJUTA_DEBUGGER_BREAK_WITH_ENABLE;
				bp.enable = TRUE;
			}
		}
	
		literal = gdbmi_value_hash_lookup (brkpnt, "addr");
		if (literal)
		{
			value = gdbmi_value_literal_get (literal);
			bp.address = strtoul (value, NULL, 16);
			bp.type |= IANJUTA_DEBUGGER_BREAK_ON_ADDRESS;
		}

		literal = gdbmi_value_hash_lookup (brkpnt, "func");
		if (literal)
		{
			value = gdbmi_value_literal_get (literal);
			bp.function = (gchar *)value;
			bp.type |= IANJUTA_DEBUGGER_BREAK_ON_FUNCTION;
		}
	
		literal = gdbmi_value_hash_lookup (brkpnt, "times");
		if (literal)
		{
			value = gdbmi_value_literal_get (literal);
			bp.times = strtoul (value, NULL, 10);
		}
												literal = gdbmi_value_hash_lookup (brkpnt, "ignore");
		if (literal)
		{
			value = gdbmi_value_literal_get (literal);
			bp.ignore = strtoul (value, NULL, 10);
		}
		else
		{
			bp.ignore = G_MAXUINT32;
		}

		literal = gdbmi_value_hash_lookup (brkpnt, "cond");
		if (literal)
		{       
			value = gdbmi_value_literal_get (literal);
			bp.type |= IANJUTA_DEBUGGER_BREAK_WITH_CONDITION;
		}
		
		/* Call callback in all case (useful for enable that doesn't return
 		* anything */
		if (callback != NULL)
			callback (&bp, user_data, error);
	}
	
}
	
void
debugger_add_breakpoint_at_line (Debugger *debugger, const gchar *file, guint line, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	gchar *buff;

	DEBUG_PRINT ("In function: debugger_add_breakpoint()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	buff = g_strdup_printf ("-break-insert %s:%u", file, line);
	debugger_queue_command (debugger, buff, FALSE, FALSE, debugger_add_breakpoint_finish, callback, user_data);
	g_free (buff);
}

void
debugger_add_breakpoint_at_function (Debugger *debugger, const gchar *file, const gchar *function, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	gchar *buff;

	DEBUG_PRINT ("In function: debugger_add_breakpoint()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	buff = g_strdup_printf ("-break-insert %s%s%s", file == NULL ? "" : file, file == NULL ? "" : ":" , function);
	debugger_queue_command (debugger, buff, FALSE, FALSE, debugger_add_breakpoint_finish, callback, user_data);
	g_free (buff);
}

void
debugger_add_breakpoint_at_address (Debugger *debugger, guint address, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	gchar *buff;

	DEBUG_PRINT ("In function: debugger_add_breakpoint()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	buff = g_strdup_printf ("-break-insert *0x%x", address);
	debugger_queue_command (debugger, buff, FALSE, FALSE, debugger_add_breakpoint_finish, callback, user_data);
	g_free (buff);
}

void
debugger_enable_breakpoint (Debugger *debugger, guint id, gboolean enable, IAnjutaDebuggerCallback callback, gpointer user_data)

{
	gchar *buff;
	
	DEBUG_PRINT ("In function: debugger_enable_breakpoint()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	buff = g_strdup_printf (enable ? "-break-enable %d" : "-break-disable %d",id);
	debugger_queue_command (debugger, buff, FALSE, FALSE, debugger_add_breakpoint_finish, callback, user_data);
	g_free (buff);
}

void
debugger_ignore_breakpoint (Debugger *debugger, guint id, guint ignore, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	gchar *buff;
	
	DEBUG_PRINT ("In function: debugger_ignore_breakpoint()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	buff = g_strdup_printf ("-break-after %d %d", id, ignore);
	debugger_queue_command (debugger, buff, FALSE, FALSE, debugger_add_breakpoint_finish, callback, user_data);
	g_free (buff);
}

void
debugger_condition_breakpoint (Debugger *debugger, guint id, const gchar *condition, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	gchar *buff;
	
	DEBUG_PRINT ("In function: debugger_ignore_breakpoint()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	buff = g_strdup_printf ("-break-condition %d %s", id, condition);
	debugger_queue_command (debugger, buff, FALSE, FALSE, debugger_add_breakpoint_finish, callback, user_data);
	g_free (buff);
}

static void
debugger_remove_breakpoint_finish (Debugger *debugger, const GDBMIValue *mi_results, const GList *cli_results, GError *error)
{
	IAnjutaDebuggerCallback callback = debugger->priv->current_cmd.callback;
	gpointer user_data = debugger->priv->current_cmd.user_data;
	IAnjutaDebuggerBreakpoint bp;

	bp.type = IANJUTA_DEBUGGER_BREAK_REMOVED;
	bp.id = atoi (debugger->priv->current_cmd.cmd + 14);
	if (callback != NULL)
		callback (&bp, user_data, NULL);
}

void
debugger_remove_breakpoint (Debugger *debugger, guint id, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	gchar *buff;
	
	DEBUG_PRINT ("In function: debugger_delete_breakpoint()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	buff = g_strdup_printf ("-break-delete %d", id);
	debugger_queue_command (debugger, buff, FALSE, FALSE, debugger_remove_breakpoint_finish, callback, user_data);
	g_free (buff);
}

static void
debugger_print_finish (Debugger *debugger, const GDBMIValue *mi_results, const GList *cli_results, GError *error)
{
	gchar *ptr = NULL;
	gchar *tmp;
	GList *list, *node;
	IAnjutaDebuggerCallback callback = debugger->priv->current_cmd.callback;
	gpointer user_data = debugger->priv->current_cmd.user_data;


	list = gdb_util_remove_blank_lines (cli_results);
	if (g_list_length (list) < 1)
	{
		tmp = NULL;
	}
	else
	{
		tmp = strchr ((gchar *) list->data, '=');
	}
	if (tmp != NULL)
	{
		ptr = g_strdup (tmp);
		for (node = list ? list->next : NULL; node != NULL; node = node->next)
		{	
			tmp = ptr;
			ptr = g_strconcat (tmp, (gchar *) node->data, NULL);
			g_free (tmp);
		}
	}

	callback (ptr, user_data, NULL);
	g_free (ptr);
}

void
debugger_print (Debugger *debugger, const gchar* variable, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	gchar *buff;
	
	DEBUG_PRINT ("In function: debugger_print()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	buff = g_strdup_printf ("print %s", variable);
	debugger_queue_command (debugger, buff, TRUE, FALSE, debugger_print_finish, callback, user_data);
	g_free (buff);
}

static void
debugger_evaluate_finish (Debugger *debugger, const GDBMIValue *mi_results, const GList *cli_results, GError *error)

{
	const GDBMIValue *value = NULL;
	IAnjutaDebuggerCallback callback = debugger->priv->current_cmd.callback;
	gpointer user_data = debugger->priv->current_cmd.user_data;
	
	if (mi_results)
		value = gdbmi_value_hash_lookup (mi_results, "value");
	
	/* Call user function */
	if (callback != NULL)
		callback (value == NULL ? "?" : (char *)gdbmi_value_literal_get (value), user_data, NULL);
}

void
debugger_evaluate (Debugger *debugger, const gchar* name, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	gchar *buff;
	DEBUG_PRINT ("In function: debugger_add_watch()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	buff = g_strdup_printf ("-data-evaluate-expression %s", name);
	debugger_queue_command (debugger, buff, TRUE, FALSE, debugger_evaluate_finish, callback, user_data);
	g_free (buff);
}

static void
debugger_list_local_finish (Debugger *debugger, const GDBMIValue *mi_results, const GList *cli_results, GError *error)

{
	const GDBMIValue *local, *var, *frame, *args, *stack;
	const gchar * name;
	GList* list;
	guint i;
	IAnjutaDebuggerCallback callback = debugger->priv->current_cmd.callback;
	gpointer user_data = debugger->priv->current_cmd.user_data;


	list = NULL;
	/* Add arguments */
	stack = gdbmi_value_hash_lookup (mi_results, "stack-args");
	if (stack)
	{
		frame = gdbmi_value_list_get_nth (stack, 0);
		if (frame)
		{
			args = gdbmi_value_hash_lookup (frame, "args");
			if (args)
			{
				for (i = 0; i < gdbmi_value_get_size (args); i++)
				{
					var = gdbmi_value_list_get_nth (args, i);
					if (var)
					{
						name = gdbmi_value_literal_get (var);
						list = g_list_prepend (list, (gchar *)name);
					}
				}

			}
		}
	}
	
	/* List local variables */	
	local = gdbmi_value_hash_lookup (mi_results, "locals");
	if (local)
	{
		for (i = 0; i < gdbmi_value_get_size (local); i++)
		{
			var = gdbmi_value_list_get_nth (local, i);
			if (var)
			{
				name = gdbmi_value_literal_get (var);
				list = g_list_prepend (list, (gchar *)name);
			}
		}
	}
	list = g_list_reverse (list);
	callback (list, user_data, NULL);
	g_list_free (list);
}

void
debugger_list_local (Debugger *debugger, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	gchar *buff;
	DEBUG_PRINT ("In function: debugger_list_local()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	buff = g_strdup_printf("-stack-list-arguments 0 %d %d", debugger->priv->current_frame, debugger->priv->current_frame);
	debugger_queue_command (debugger, buff, TRUE, TRUE, NULL, NULL, NULL);
	g_free (buff);
	debugger_queue_command (debugger, "-stack-list-locals 0", TRUE, FALSE, debugger_list_local_finish, callback, user_data);
}

static void
debugger_list_argument_finish (Debugger *debugger, const GDBMIValue *mi_results, const GList *cli_results, GError *error)

{
	const GDBMIValue *frame, *var, *args, *stack;
	const gchar * name;
	GList* list;
	guint i;
	IAnjutaDebuggerCallback callback = debugger->priv->current_cmd.callback;
	gpointer user_data = debugger->priv->current_cmd.user_data;


	list = NULL;
	args = NULL;	
	stack = gdbmi_value_hash_lookup (mi_results, "stack-args");
	if (stack)
	{
		frame = gdbmi_value_list_get_nth (stack, 0);
		if (frame)
		{
			args = gdbmi_value_hash_lookup (frame, "args");
		}
	}

	if (args)
	{
		for (i = 0; i < gdbmi_value_get_size (args); i++)
		{
			var = gdbmi_value_list_get_nth (args, i);
			if (var)
			{
				name = gdbmi_value_literal_get (var);
				list = g_list_prepend (list, (gchar *)name);
			}
		}
	}
	list = g_list_reverse (list);
	callback (list, user_data, NULL);
	g_list_free (list);
}

void
debugger_list_argument (Debugger *debugger, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	gchar *buff;

	DEBUG_PRINT ("In function: debugger_list_argument()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	buff = g_strdup_printf("-stack-list-arguments 0 %d %d", debugger->priv->current_frame, debugger->priv->current_frame);
	debugger_queue_command (debugger, buff, TRUE, FALSE, debugger_list_argument_finish, callback, user_data);
	g_free (buff);
}

static void
debugger_info_finish (Debugger *debugger, const GDBMIValue *mi_results, const GList *cli_results, GError *error)

{
	IAnjutaDebuggerCallback callback = debugger->priv->current_cmd.callback;
	gpointer user_data = debugger->priv->current_cmd.user_data;

	if (callback != NULL)
		callback ((GList *)cli_results, user_data, NULL);
}

void
debugger_info_frame (Debugger *debugger, guint frame, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	gchar *buff;
	
	DEBUG_PRINT ("In function: debugger_info_frame()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	if (frame == 0)
	{
		buff = g_strdup_printf("info frame");
	}
	else
	{
		buff = g_strdup_printf ("info frame %d", frame);
	}
	debugger_queue_command (debugger, buff, TRUE, FALSE, (DebuggerParserFunc)debugger_info_finish, callback, user_data);
	g_free (buff);
}

void
debugger_info_signal (Debugger *debugger, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	DEBUG_PRINT ("In function: debugger_info_signal()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	debugger_queue_command (debugger, "info signals", TRUE, FALSE, (DebuggerParserFunc)debugger_info_finish, callback, user_data);
}

void
debugger_info_sharedlib (Debugger *debugger, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	gchar *buff;
	
	DEBUG_PRINT ("In function: debugger_info_sharedlib()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	buff = g_strdup_printf ("info sharedlib");
	debugger_queue_command (debugger, buff, TRUE, FALSE, (DebuggerParserFunc)debugger_info_finish, callback, user_data);	g_free (buff);
}

void
debugger_info_args (Debugger *debugger, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	DEBUG_PRINT ("In function: debugger_info_args()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	debugger_queue_command (debugger, "info args", TRUE, FALSE, (DebuggerParserFunc)debugger_info_finish, callback, user_data);
}

void
debugger_info_target (Debugger *debugger, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	DEBUG_PRINT ("In function: debugger_info_target()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	debugger_queue_command (debugger, "info target", TRUE, FALSE, (DebuggerParserFunc)debugger_info_finish, callback, user_data);
}

void
debugger_info_program (Debugger *debugger, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	DEBUG_PRINT ("In function: debugger_info_program()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	debugger_queue_command (debugger, "info program", TRUE, FALSE, (DebuggerParserFunc)debugger_info_finish, callback, user_data);
}

void
debugger_info_udot (Debugger *debugger, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	DEBUG_PRINT ("In function: debugger_info_udot()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	debugger_queue_command (debugger, "info udot", TRUE, FALSE, (DebuggerParserFunc)debugger_info_finish, callback, user_data);
}

void
debugger_info_variables (Debugger *debugger, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	DEBUG_PRINT ("In function: debugger_info_variables()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	debugger_queue_command (debugger, "info variables", TRUE, FALSE, (DebuggerParserFunc)debugger_info_finish, callback, user_data);
}

static void
debugger_read_memory_finish (Debugger *debugger, const GDBMIValue *mi_results, const GList *cli_results, GError *error)

{
	const GDBMIValue *literal;
	const GDBMIValue *mem;
	const gchar *value;
	gchar *data;
	gchar *ptr;
	guint address;
	guint len;
	guint i;
	IAnjutaDebuggerCallback callback = debugger->priv->current_cmd.callback;
	gpointer user_data = debugger->priv->current_cmd.user_data;
	IAnjutaDebuggerMemory read = {0,};

	literal = gdbmi_value_hash_lookup (mi_results, "total-bytes");
	if (literal)
	{
		guint size;
		
		len = strtoul (gdbmi_value_literal_get (literal), NULL, 10);
		data = g_new (gchar, len * 2);
		memset (data + len, 0, len);

		literal = gdbmi_value_hash_lookup (mi_results, "addr");
		address = strtoul (gdbmi_value_literal_get (literal), NULL, 0);
	
		ptr = data;
		size = 0;
		mem = gdbmi_value_hash_lookup (mi_results, "memory");
		if (mem)
		{
			mem = gdbmi_value_list_get_nth (mem, 0);
			if (mem)
			{
				mem = gdbmi_value_hash_lookup (mem, "data");
				if (mem)
				{
					size = gdbmi_value_get_size (mem);
				}
			}
		}

		if (size < len) len = size;
		for (i = 0; i < len; i++)
		{
			literal = gdbmi_value_list_get_nth (mem, i);
			if (literal)
			{
				gchar *endptr;
				value = gdbmi_value_literal_get (literal);
				*ptr = strtoul (value, &endptr, 16);
				if ((*value != '\0') && (*endptr == '\0'))
				{
					/* valid data */
					ptr[len] = 1;
				}
				ptr++;
			}
		}
		read.address = address;
		read.length = len;
		read.data = data;
		callback (&read, user_data, NULL);

		g_free (data);
	}
	else
	{
		callback (NULL, user_data, NULL);
	}
}

void
debugger_inspect_memory (Debugger *debugger, guint address, guint length, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	gchar *buff;
	
	DEBUG_PRINT ("In function: debugger_inspect_memory()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	buff = g_strdup_printf ("-data-read-memory 0x%x x 1 1 %d", address, length);
	debugger_queue_command (debugger, buff, FALSE, FALSE, debugger_read_memory_finish, callback, user_data);
	g_free (buff);
}

static void
debugger_disassemble_finish (Debugger *debugger, const GDBMIValue *mi_results, const GList *cli_results, GError *error)

{
	const GDBMIValue *literal;
	const GDBMIValue *line;
	const GDBMIValue *mem;
	const gchar *value;
	guint i;
	IAnjutaDebuggerCallback callback = debugger->priv->current_cmd.callback;
	gpointer user_data = debugger->priv->current_cmd.user_data;
	IAnjutaDebuggerDisassembly *read = NULL;

	if (error != NULL)
	{
		/* Command fail */
		callback (NULL, user_data, error);

		return;
	}


	mem = gdbmi_value_hash_lookup (mi_results, "asm_insns");
	if (mem)
	{
		guint size;
	
		size = gdbmi_value_get_size (mem);
		read = (IAnjutaDebuggerDisassembly *)g_malloc0(sizeof (IAnjutaDebuggerDisassembly) + sizeof(IAnjutaDebuggerALine) * size);
		read->size = size;

		for (i = 0; i < size; i++)
		{
			line = gdbmi_value_list_get_nth (mem, i);
			if (line)
			{
				/* Get address */
				literal = gdbmi_value_hash_lookup (line, "address");
				if (literal)
				{
					value = gdbmi_value_literal_get (literal);
					read->data[i].address = strtoul (value, NULL, 0);
				}

				/* Get label if one exist */
				literal = gdbmi_value_hash_lookup (line, "offset");
				if (literal)
				{
					value = gdbmi_value_literal_get (literal);
					if ((value != NULL) && (strtoul (value, NULL, 0) == 0))
					{
						literal = gdbmi_value_hash_lookup (line, "func-name");
						if (literal)
						{
							read->data[i].label = gdbmi_value_literal_get (literal);
						}
					}

				}
						

				/* Get disassembly line */
				literal = gdbmi_value_hash_lookup (line, "inst");
				if (literal)
				{
					read->data[i].text = gdbmi_value_literal_get (literal);
				}
			}
		}

		/* Remove last line to mark end */
		read->data[i - 1].text = NULL;

		callback (read, user_data, NULL);

		g_free (read);
	}
	else
	{
		callback (NULL, user_data, NULL);
	}
}

void
debugger_disassemble (Debugger *debugger, guint address, guint length, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	gchar *buff;
	
	DEBUG_PRINT ("In function: debugger_disassemble()");

	g_return_if_fail (IS_DEBUGGER (debugger));


	/* Handle overflow */
	if (address + length < address) length = G_MAXUINT - address;
	buff = g_strdup_printf ("-data-disassemble -s 0x%x -e 0x%x  -- 0", address, address + length);
	debugger_queue_command (debugger, buff, FALSE, FALSE, debugger_disassemble_finish, callback, user_data);
	g_free (buff);
}

static void
parse_frame (IAnjutaDebuggerFrame *frame, const GDBMIValue *frame_hash)
{
	const GDBMIValue *literal;
	
	literal = gdbmi_value_hash_lookup (frame_hash, "level");
	if (literal)
		frame->level = strtoul (gdbmi_value_literal_get (literal), NULL, 10);
	else
		frame->level = 0;

	literal = gdbmi_value_hash_lookup (frame_hash, "fullname");
	if (literal == NULL)
		literal = gdbmi_value_hash_lookup (frame_hash, "file");
	if (literal)
		frame->file = (gchar *)gdbmi_value_literal_get (literal);
	else
		frame->file = NULL;
	
	literal = gdbmi_value_hash_lookup (frame_hash, "line");
	if (literal)
		frame->line = strtoul (gdbmi_value_literal_get (literal), NULL, 10);
	else
		frame->line = 0;
	
	literal = gdbmi_value_hash_lookup (frame_hash, "func");
	if (literal)
		frame->function = (gchar *)gdbmi_value_literal_get (literal);
	else
		frame->function = NULL;
	
	literal = gdbmi_value_hash_lookup (frame_hash, "from");
	if (literal)
		frame->library = (gchar *)gdbmi_value_literal_get (literal);
	else
		frame->library = NULL;

	literal = gdbmi_value_hash_lookup (frame_hash, "addr");
	if (literal)
		frame->address = strtoul (gdbmi_value_literal_get (literal), NULL, 16);
	else
		frame->address = 0;
	
}


static void
add_frame (const GDBMIValue *frame_hash, GdbGListPacket* pack)
{
	IAnjutaDebuggerFrame* frame;
	
	frame = g_new0 (IAnjutaDebuggerFrame, 1);
	pack->list = g_list_prepend (pack->list, frame);

	frame->thread = pack->tag;
	parse_frame (frame, frame_hash);	
}

static void
set_func_args (const GDBMIValue *frame_hash, GList** node)
{
	const gchar *level;
	const GDBMIValue *literal, *args_list, *arg_hash;
	gint i;
	GString *args_str;
	IAnjutaDebuggerFrame* frame;
	
	literal = gdbmi_value_hash_lookup (frame_hash, "level");
	if (!literal)
		return;
	
	level = gdbmi_value_literal_get (literal);
	if (!level)
		return;
	
	frame = (IAnjutaDebuggerFrame *)(*node)->data;
	
	args_list = gdbmi_value_hash_lookup (frame_hash, "args");
	if (args_list)
	{
		args_str = g_string_new ("(");
		for (i = 0; i < gdbmi_value_get_size (args_list); i++)
		{
			const gchar *name, *value;
		
			arg_hash = gdbmi_value_list_get_nth (args_list, i);
			if (!arg_hash)
				continue;
		
			literal = gdbmi_value_hash_lookup (arg_hash, "name");
			if (!literal)
				continue;
			name = gdbmi_value_literal_get (literal);
			if (!name)
				continue;
		
			literal = gdbmi_value_hash_lookup (arg_hash, "value");
			if (!literal)
				continue;
			value = gdbmi_value_literal_get (literal);
			if (!value)
				continue;
			args_str = g_string_append (args_str, name);
			args_str = g_string_append (args_str, "=");
			args_str = g_string_append (args_str, value);
			if (i < (gdbmi_value_get_size (args_list) - 1))
				args_str = g_string_append (args_str, ", ");
		}
		args_str = g_string_append (args_str, ")");
		frame->args = args_str->str;
		g_string_free (args_str, FALSE);
	}
	*node = g_list_next (*node);
}

static void
debugger_stack_finish (Debugger *debugger, const GDBMIValue *mi_results, const GList *cli_results, GError *error)

{
	GdbGListPacket pack = {NULL, 0};
	GList* node;
	const GDBMIValue *stack_list;
	IAnjutaDebuggerCallback callback = debugger->priv->current_cmd.callback;
	gpointer user_data = debugger->priv->current_cmd.user_data;

	if (!mi_results)
		return;
	
	stack_list = gdbmi_value_hash_lookup (mi_results, "stack");
	if (stack_list)
	{
		pack.tag = debugger->priv->current_thread;
		gdbmi_value_foreach (stack_list, (GFunc)add_frame, &pack);
	}

	if (pack.list)
	{
		pack.list = g_list_reverse (pack.list);
		node = g_list_first (pack.list);	
		stack_list = gdbmi_value_hash_lookup (mi_results, "stack-args");
		if (stack_list)
			gdbmi_value_foreach (stack_list, (GFunc)set_func_args, &node);

		// Call call back function
		if (callback != NULL)
			callback (pack.list, user_data, NULL);

		// Free data
		for (node = g_list_first (pack.list); node != NULL; node = g_list_next (node))
		{
			g_free ((gchar *)((IAnjutaDebuggerFrame *)node->data)->args);
			g_free (node->data);
		}
	
		g_list_free (pack.list);
	}
	else
	{
		// Call call back function
		if (callback != NULL)
			callback (NULL, user_data, NULL);
	}
}

void
debugger_list_frame (Debugger *debugger, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	DEBUG_PRINT ("In function: debugger_list_frame()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	debugger_queue_command (debugger, "-stack-list-frames", TRUE, TRUE, NULL, NULL, NULL);
	debugger_queue_command (debugger, "-stack-list-arguments 1", TRUE, FALSE, debugger_stack_finish, callback, user_data);
}

/* Thread functions
 *---------------------------------------------------------------------------*/

static void
debugger_set_thread_finish (Debugger *debugger, const GDBMIValue *mi_results, const GList *cli_results, GError *error)
{
	const GDBMIValue *literal;
	guint id;

	if (mi_results == NULL) return;

	literal = gdbmi_value_hash_lookup (mi_results, "new-thread-id");
	if (literal == NULL) return;

	id = strtoul (gdbmi_value_literal_get (literal), NULL, 10);
	if (id == 0) return;

	debugger->priv->current_thread = id;
	g_signal_emit_by_name (debugger->priv->instance, "frame-changed", 0, debugger->priv->current_thread);
	       
	return;	
}

void
debugger_set_thread (Debugger *debugger, gint thread)
{
	gchar *buff;
	
	DEBUG_PRINT ("In function: debugger_set_thread()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	buff = g_strdup_printf ("-thread-select %d", thread);

	debugger_queue_command (debugger, buff, FALSE, FALSE, (DebuggerParserFunc)debugger_set_thread_finish, NULL, NULL);
	g_free (buff);
}

static void
add_thread_id (const GDBMIValue *thread_hash, GList** list)
{
	IAnjutaDebuggerFrame* frame;
	gint thread;

	thread = strtoul (gdbmi_value_literal_get (thread_hash), NULL, 10);
	if (thread == 0) return;

	frame = g_new0 (IAnjutaDebuggerFrame, 1);
	*list = g_list_prepend (*list, frame);

	frame->thread = thread;
}

static void
debugger_list_thread_finish (Debugger *debugger, const GDBMIValue *mi_results, const GList *cli_results, GError *error)

{
	const GDBMIValue *id_list;
	gpointer user_data = debugger->priv->current_cmd.user_data;
	IAnjutaDebuggerCallback callback = debugger->priv->current_cmd.callback;
	GList* thread_list = NULL;

	for (;;)
	{
		if (mi_results == NULL) break;

		id_list = gdbmi_value_hash_lookup (mi_results, "thread-ids");
		if (id_list == NULL) break;

		gdbmi_value_foreach (id_list, (GFunc)add_thread_id, &thread_list);
		thread_list = g_list_reverse (thread_list);
		break;
	}

	if (callback != NULL)
		callback (thread_list, user_data, error);

	if (thread_list != NULL)
	{
		g_list_foreach (thread_list, (GFunc)g_free, NULL);
		g_list_free (thread_list);
	}
}

void
debugger_list_thread (Debugger *debugger, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	DEBUG_PRINT ("In function: debugger_list_thread()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	debugger_queue_command (debugger, "-thread-list-ids", TRUE, FALSE, debugger_list_thread_finish, callback, user_data);
}

static void
debugger_info_set_thread_finish (Debugger *debugger, const GDBMIValue *mi_results, const GList *cli_results, GError *error)
{
	const GDBMIValue *literal;
	guint id;

	if (mi_results == NULL) return;

	literal = gdbmi_value_hash_lookup (mi_results, "new-thread-id");
	if (literal == NULL) return;

	id = strtoul (gdbmi_value_literal_get (literal), NULL, 10);
	if (id == 0) return;

	debugger->priv->current_thread = id;
	/* Do not emit a signal notification as the current thread will
	 * be restored when needed */
	       
	return;	
}

static void
debugger_info_thread_finish (Debugger *debugger, const GDBMIValue *mi_results, const GList *cli_results, GError *error)
{
	const GDBMIValue *frame;
	IAnjutaDebuggerFrame top_frame;
	IAnjutaDebuggerFrame *top;
	IAnjutaDebuggerCallback callback = debugger->priv->current_cmd.callback;
	gpointer user_data = debugger->priv->current_cmd.user_data;

	for (top = NULL;;)
	{
		DEBUG_PRINT("look for stack %p", mi_results);
		if (mi_results == NULL) break;

		frame = gdbmi_value_hash_lookup (mi_results, "stack");
		if (frame == NULL) break;

		DEBUG_PRINT("get stack");

		frame = gdbmi_value_list_get_nth (frame, 0);
		if (frame == NULL) break;

		DEBUG_PRINT("get nth element");

		/*frame = gdbmi_value_hash_lookup (frame, "frame");
		DEBUG_PRINT("get frame %p", frame);
		if (frame == NULL) break;*/

		DEBUG_PRINT("get frame");

		top = &top_frame;
		top->thread = debugger->priv->current_thread;

		parse_frame (top, frame);
		break;	
	}

	if (callback != NULL)
		callback (top, user_data, error);
	       
	return;	
}

void
debugger_info_thread (Debugger *debugger, gint thread, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	gchar *buff;
	guint orig_thread;

	DEBUG_PRINT ("In function: debugger_info_thread()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	orig_thread = debugger->priv->current_thread;
	buff = g_strdup_printf ("-thread-select %d", thread);
	debugger_queue_command (debugger, buff, FALSE, FALSE, (DebuggerParserFunc)debugger_info_set_thread_finish, NULL, NULL);
	g_free (buff);
	debugger_queue_command (debugger, "-stack-list-frames 0 0", FALSE, FALSE, (DebuggerParserFunc)debugger_info_thread_finish, callback, user_data);

	buff = g_strdup_printf ("-thread-select %d", orig_thread);
	debugger_queue_command (debugger, buff, FALSE, FALSE, (DebuggerParserFunc)debugger_info_set_thread_finish, NULL, NULL);
	g_free (buff);
}

static void
add_register_name (const GDBMIValue *reg_literal, GList** list)
{
	IAnjutaDebuggerRegister* reg;
	GList *prev = *list;
	
	reg = g_new0 (IAnjutaDebuggerRegister, 1);
	*list = g_list_prepend (prev, reg);
	reg->name = (gchar *)gdbmi_value_literal_get (reg_literal);
	reg->num = prev == NULL ? 0 : ((IAnjutaDebuggerRegister *)prev->data)->num + 1;
}

static void
add_register_value (const GDBMIValue *reg_hash, GList** list)
{
	const GDBMIValue *literal;
	const gchar *val;
	IAnjutaDebuggerRegister* reg;
	guint num;
	GList* prev = *list;
		
	literal = gdbmi_value_hash_lookup (reg_hash, "number");
	if (!literal)
		return;
	val = gdbmi_value_literal_get (literal);
	num = strtoul (val, NULL, 10);

	literal = gdbmi_value_hash_lookup (reg_hash, "value");
	if (!literal)
		return;
	
	reg = g_new0 (IAnjutaDebuggerRegister, 1);
	*list = g_list_prepend (prev, reg);
	reg->num = num;
	reg->value = (gchar *)gdbmi_value_literal_get (literal);
}

static void
debugger_register_name_finish (Debugger *debugger, const GDBMIValue *mi_results, const GList *cli_results, GError *error)

{
	GList* list = NULL;
	GList* node;
	const GDBMIValue *reg_list;
	IAnjutaDebuggerCallback callback = debugger->priv->current_cmd.callback;
	gpointer user_data = debugger->priv->current_cmd.user_data;
	
	if (!mi_results)
		return;
	
	reg_list = gdbmi_value_hash_lookup (mi_results, "register-names");
	if (reg_list)
		gdbmi_value_foreach (reg_list, (GFunc)add_register_name, &list);
	list = g_list_reverse (list);

	// Call call back function
	if (callback != NULL)
		callback (list, user_data, NULL);

	// Free data
	for (node = g_list_first (list); node != NULL; node = g_list_next (node))
	{
		g_free (node->data);
	}
	g_list_free (list);
}

static void
debugger_register_value_finish (Debugger *debugger, const GDBMIValue *mi_results, const GList *cli_results, GError *error)

{
	GList* list = NULL;
	GList* node;
	const GDBMIValue *reg_list;
	IAnjutaDebuggerCallback callback = debugger->priv->current_cmd.callback;
	gpointer user_data = debugger->priv->current_cmd.user_data;
	
	if (!mi_results)
		return;
	
	reg_list = gdbmi_value_hash_lookup (mi_results, "register-values");
	if (reg_list)
		gdbmi_value_foreach (reg_list, (GFunc)add_register_value, &list);
	list = g_list_reverse (list);

	// Call call back function
	if (callback != NULL)
		callback (list, user_data, NULL);

	// Free data
	for (node = g_list_first (list); node != NULL; node = g_list_next (node))
	{
		g_free (node->data);
	}
	g_list_free (list);
}

void
debugger_list_register (Debugger *debugger, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	DEBUG_PRINT ("In function: debugger_list_register()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	debugger_queue_command (debugger, "-data-list-register-names", TRUE, FALSE, debugger_register_name_finish, callback, user_data);
}

void
debugger_update_register (Debugger *debugger, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	DEBUG_PRINT ("In function: debugger_update_register()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	debugger_queue_command (debugger, "-data-list-register-values r", TRUE, FALSE, (DebuggerParserFunc)debugger_register_value_finish, callback, user_data);
}

void
debugger_write_register (Debugger *debugger, const gchar *name, const gchar *value)
{
	gchar *buf;

	DEBUG_PRINT ("In function: debugger_write_register()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	buf = g_strdup_printf ("-data-evaluate-expression \"$%s=%s\"", name, value);
	debugger_queue_command (debugger, buf, TRUE, FALSE, NULL, NULL, NULL);
	g_free (buf);
}

static void
debugger_set_frame_finish (Debugger *debugger, const GDBMIValue *mi_results, const GList *cli_results, GError *error)

{
	guint frame  = (guint)debugger->priv->current_cmd.user_data;
	debugger->priv->current_frame = frame;
	
	g_signal_emit_by_name (debugger->priv->instance, "frame-changed", frame, debugger->priv->current_thread);
}

void
debugger_set_frame (Debugger *debugger, guint frame)
{
	gchar *buff;
	
	DEBUG_PRINT ("In function: debugger_set_frame()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	buff = g_strdup_printf ("-stack-select-frame %d", frame);

	debugger_queue_command (debugger, buff, FALSE, FALSE, (DebuggerParserFunc)debugger_set_frame_finish, NULL, (gpointer)frame);
	g_free (buff);
}

void
debugger_set_log (Debugger *debugger, IAnjutaMessageView *log)
{
	debugger->priv->log = log;
}

/* Variable objects functions
 *---------------------------------------------------------------------------*/

void
debugger_delete_variable (Debugger *debugger, const gchar* name)
{
	gchar *buff;
	
	DEBUG_PRINT ("In function: delete_variable()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	buff = g_strdup_printf ("-var-delete %s", name);
	debugger_queue_command (debugger, buff, FALSE, FALSE, NULL, NULL, NULL);
	g_free (buff);
}

static void
gdb_var_evaluate_expression (Debugger *debugger,
                        const GDBMIValue *mi_results, const GList *cli_results,
			GError *error)
{
	const gchar *value = NULL;
	IAnjutaDebuggerCallback callback = debugger->priv->current_cmd.callback;
	gpointer user_data = debugger->priv->current_cmd.user_data;

	if (mi_results != NULL)
	{
		const GDBMIValue *const gdbmi_value =
	                        gdbmi_value_hash_lookup (mi_results, "value");

		if (gdbmi_value != NULL)
			value = gdbmi_value_literal_get (gdbmi_value);
	}
	callback ((const gpointer)value, user_data, NULL);
}

void
debugger_evaluate_variable (Debugger *debugger, const gchar* name, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	gchar *buff;
	
	DEBUG_PRINT ("In function: evaluate_variable()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	buff = g_strdup_printf ("-var-evaluate-expression %s", name);
	debugger_queue_command (debugger, buff, FALSE, FALSE, gdb_var_evaluate_expression, callback, user_data);
	g_free (buff);
}

void
debugger_assign_variable (Debugger *debugger, const gchar* name, const gchar *value)
{
	gchar *buff;
	
	DEBUG_PRINT ("In function: assign_variable()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	buff = g_strdup_printf ("-var-assign %s %s", name, value);
	debugger_queue_command (debugger, buff, FALSE, FALSE, NULL, NULL, NULL);
	g_free (buff);
}

static void
gdb_var_list_children (Debugger *debugger,
                        const GDBMIValue *mi_results, const GList *cli_results,
			GError *error)
{
	GList* list = NULL;
	IAnjutaDebuggerCallback callback = debugger->priv->current_cmd.callback;
	gpointer user_data = debugger->priv->current_cmd.user_data;

	if (mi_results != NULL)	
	{
		const GDBMIValue *literal;
		const GDBMIValue *children;
		glong numchild = 0;
		glong i = 0;

		literal = gdbmi_value_hash_lookup (mi_results, "numchild");

		if (literal)
			numchild = strtoul(gdbmi_value_literal_get (literal), NULL, 0);
		children = gdbmi_value_hash_lookup (mi_results, "children"); 

		for(i = 0 ; i < numchild; ++i)
		{
			const GDBMIValue *const gdbmi_chl = 
                                gdbmi_value_list_get_nth (children, i);
			IAnjutaDebuggerVariable *var;
		
			var = g_new0 (IAnjutaDebuggerVariable, 1);

		       	literal  = gdbmi_value_hash_lookup (gdbmi_chl, "name");
			if (literal)
  				var->name = (gchar *)gdbmi_value_literal_get (literal);

			literal = gdbmi_value_hash_lookup (gdbmi_chl, "exp");
			if (literal)
				var->expression = (gchar *)gdbmi_value_literal_get(literal);
                
			literal = gdbmi_value_hash_lookup (gdbmi_chl, "type");
			if (literal)
				var->type = (gchar *)gdbmi_value_literal_get(literal);

        		literal = gdbmi_value_hash_lookup (gdbmi_chl, "value");
			if (literal)
				var->value = (gchar *)gdbmi_value_literal_get(literal);

        		literal = gdbmi_value_hash_lookup (gdbmi_chl, "numchild");
			if (literal)
				var->children = strtoul(gdbmi_value_literal_get(literal), NULL, 10);

			list = g_list_prepend (list, var);
		}
		list = g_list_reverse (list);
	}

	callback (list, user_data, NULL);
	g_list_foreach (list, (GFunc)g_free, NULL);
	g_list_free (list);
}

void debugger_list_variable_children (Debugger *debugger, const gchar* name, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	gchar *buff;
	
	DEBUG_PRINT ("In function: list_variable_children()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	buff = g_strdup_printf ("-var-list-children --all-values %s", name);
	debugger_queue_command (debugger, buff, FALSE, FALSE, gdb_var_list_children, callback, user_data);
	g_free (buff);
}

static void
gdb_var_create (Debugger *debugger,
                const GDBMIValue *mi_results, const GList *cli_results,
		GError *error)
{
	const GDBMIValue * result;
	IAnjutaDebuggerVariable var = {0,};
	IAnjutaDebuggerCallback callback = debugger->priv->current_cmd.callback;
	gpointer user_data = debugger->priv->current_cmd.user_data;

	if ((error == NULL) && (mi_results != NULL)) 
	{
		result = gdbmi_value_hash_lookup (mi_results, "name");
		var.name = (gchar *)gdbmi_value_literal_get(result);
	
		result = gdbmi_value_hash_lookup (mi_results, "type");
		var.type = (gchar *)gdbmi_value_literal_get (result);

		result = gdbmi_value_hash_lookup (mi_results, "numchild");
		var.children = strtoul (gdbmi_value_literal_get(result), NULL, 10);
	}
	callback (&var, user_data, error);

}

void debugger_create_variable (Debugger *debugger, const gchar* name, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	gchar *buff;
	
	DEBUG_PRINT ("In function: create_variable()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	buff = g_strdup_printf ("-var-create - * %s", name);
	debugger_queue_command (debugger, buff, FALSE, FALSE, gdb_var_create, callback, user_data);
	g_free (buff);
}

static void
gdb_var_update (Debugger *debugger,
                const GDBMIValue *mi_results, const GList *cli_results,
		GError *error)
{
	GList* list = NULL;
	glong idx = 0, changed_count = 0;
	const GDBMIValue *const gdbmi_changelist =
                     gdbmi_value_hash_lookup (mi_results, "changelist"); 
	IAnjutaDebuggerCallback callback = debugger->priv->current_cmd.callback;
	gpointer user_data = debugger->priv->current_cmd.user_data;


	changed_count = gdbmi_value_get_size (gdbmi_changelist);
	for(; idx<changed_count; ++idx)
    	{
		const GDBMIValue *const gdbmi_change = 
                         gdbmi_value_list_get_nth (gdbmi_changelist, idx);
		const GDBMIValue *gdbmi_val = 
                         gdbmi_value_hash_lookup (gdbmi_change, "in_scope");
		IAnjutaDebuggerVariable *var;
              
		if(0 != strcmp(gdbmi_value_literal_get(gdbmi_val), "false"))
	    	{
			gdbmi_val = gdbmi_value_hash_lookup (gdbmi_change, "name");
			var = g_new0 (IAnjutaDebuggerVariable, 1);
			var->changed = TRUE;
			var->name = (gchar *)gdbmi_value_literal_get(gdbmi_val);
			
			list = g_list_prepend (list, var);
		}
	}
	list = g_list_reverse (list);
	callback (list, user_data, NULL);
	g_list_foreach (list, (GFunc)g_free, NULL);
	g_list_free (list);
}

void debugger_update_variable (Debugger *debugger, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	DEBUG_PRINT ("In function: update_variable()");

	g_return_if_fail (IS_DEBUGGER (debugger));

	debugger_queue_command (debugger, "-var-update *", FALSE, FALSE, gdb_var_update, callback, user_data);
}

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

	/* Do not emit signal when the debugger is destroyed */
	debugger->priv->instance = NULL;
	debugger_abort (debugger);

	/* Good Bye message */
	if (debugger->priv->output_callback)
	{
		debugger->priv->output_callback (IANJUTA_DEBUGGER_OUTPUT,
							   "Debugging session completed.\n",
							   debugger->priv->output_user_data);
	}

	if (debugger->priv->launcher)
	{
 		anjuta_launcher_reset (debugger->priv->launcher);
		g_object_unref (debugger->priv->launcher);
		debugger->priv->launcher = NULL;
	}

	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
debugger_finalize (GObject *obj)
{
	Debugger *debugger = DEBUGGER (obj);
	g_string_free (debugger->priv->stdo_line, TRUE);
	g_string_free (debugger->priv->stdo_acc, TRUE);
	g_string_free (debugger->priv->stde_line, TRUE);
	g_free (debugger->priv);
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
debugger_class_init (DebuggerClass * klass)
{
	GObjectClass *object_class;
	
	g_return_if_fail (klass != NULL);
	object_class = G_OBJECT_CLASS (klass);
	
	DEBUG_PRINT ("Initializing debugger class");
	
	parent_class = g_type_class_peek_parent (klass);
	object_class->dispose = debugger_dispose;
	object_class->finalize = debugger_finalize;
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
