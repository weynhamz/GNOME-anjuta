/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2005 Sebastien Granjoux

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program as distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
 * Implement the IAnjutaDebugger interface.
 */

#include <config.h>

/*#define DEBUG*/
#include <libanjuta/anjuta-debug.h>

#include "plugin.h"

#include "debugger.h"

#include <libanjuta/interfaces/ianjuta-debugger.h>
#include <libanjuta/interfaces/ianjuta-breakpoint-debugger.h>
#include <libanjuta/interfaces/ianjuta-cpu-debugger.h>
#include <libanjuta/interfaces/ianjuta-variable-debugger.h>
#include <libanjuta/interfaces/ianjuta-terminal.h>
#include <libanjuta/anjuta-plugin.h>
#include <signal.h>

/* Plugin type
 *---------------------------------------------------------------------------*/

struct _GdbPlugin
{
	AnjutaPlugin parent;
	
	/* Debugger */
	Debugger *debugger;

	/* Output callback data */
	IAnjutaDebuggerOutputCallback output_callback;
	gpointer output_user_data;

	/* Log message window */
	IAnjutaMessageView *view;

	/* Terminal */
	pid_t term_pid;
};

struct _GdbPluginClass
{
	AnjutaPluginClass parent_class;
};

/* Terminal functions
 *---------------------------------------------------------------------------*/

static void
gdb_plugin_stop_terminal (GdbPlugin* plugin)
{
	DEBUG_PRINT ("In function: gdb_plugin_stop_terminal()");

	if (plugin->term_pid > 0) {
		kill (plugin->term_pid, SIGTERM);
		plugin->term_pid = -1;
	}
}

static gchar*
gdb_plugin_start_terminal (GdbPlugin* plugin)
{
	gchar *file, *cmd;
       	gchar *tty = NULL;
	FILE  *fp;
	IAnjutaTerminal *term;
	const gchar* err = NULL;

	DEBUG_PRINT ("In function: gdb_plugin_start_terminal() previous pid %d", plugin->term_pid);

	/* Close previous terminal if needed */
	gdb_plugin_stop_terminal (plugin);

	/* Get terminal plugin */	
	term = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell, IAnjutaTerminal, NULL);
	if (term == NULL)
	{

		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
					  _("Anjuta terminal plugin is not installed. The program will be run without a terminal."));

		return NULL;
	}

	/* Check if anjuta launcher is here */
	if (anjuta_util_prog_is_installed ("anjuta_launcher", TRUE) == FALSE)
	{
		return NULL;
	}

	file = anjuta_util_get_a_tmp_file();
	if (mkfifo (file, 0664) < 0)
	{
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
					  _("Fail to create fifo file named %s. The program will be run without a terminal"), file);
		g_free (file);

		return NULL;
	}

	/* Launch terminal */
	cmd = g_strconcat ("anjuta_launcher --__debug_terminal ", file, NULL);
	plugin->term_pid = ianjuta_terminal_execute_command (term, NULL, cmd, NULL);
	g_free (cmd);

	if (plugin->term_pid > 0)
	{
		/*
		 * Warning: call to fopen() may be blocked if the terminal is
		 * not properly started. I don't know how to handle this. May
		 * be opening as non-blocking will solve this .
		 */
		g_file_get_contents (file, &tty, NULL, NULL);  /* Ok, take the risk. */
		if (tty)
		{       
			g_strchomp (tty);	/* anjuta_launcher add a end of line after the terminal name */
			if (strcmp(tty, "__ERROR__") == 0)
			{
				g_free (tty);
				tty = NULL;
			}
		}
	}
	remove (file);
	g_free (file);

	if (tty == NULL)
	{
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
					  _("Cannot start terminal for debugging."));
		gdb_plugin_stop_terminal (plugin);
	}

	return tty;
}

/* Private functions
 *---------------------------------------------------------------------------*/

static void
on_debugger_stopped (GdbPlugin *self, GError *err)
{
	if (self->debugger != NULL)
	{
		g_signal_handlers_disconnect_by_func (self, G_CALLBACK (on_debugger_stopped), self);	
		debugger_free (self->debugger);
		self->debugger = NULL;
		gdb_plugin_stop_terminal (self);
	}
}

static void
gdb_plugin_initialize (GdbPlugin *this)
{
	GtkWindow *parent;

	/* debugger can be not NULL, if initialize is called several times
	 * or if debugger_stopped signal has not been emitted */
	if (this->debugger != NULL)
	{
		on_debugger_stopped (this, NULL);
	}

	parent = GTK_WINDOW (ANJUTA_PLUGIN (this)->shell);
	this->debugger = debugger_new (parent, G_OBJECT (this));
	g_signal_connect_swapped (this, "debugger-stopped", G_CALLBACK (on_debugger_stopped), this);
	debugger_set_output_callback (this->debugger, this->output_callback, this->output_user_data);
	if (this->view) debugger_set_log (this->debugger, this->view);
}

/* AnjutaPlugin functions
 *---------------------------------------------------------------------------*/

static gboolean
gdb_plugin_activate_plugin (AnjutaPlugin* plugin)
{
	/* GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin); */

	DEBUG_PRINT ("GDB: Activating Gdb plugin...");

	return TRUE;
}

static gboolean
gdb_plugin_deactivate_plugin (AnjutaPlugin* plugin)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	DEBUG_PRINT ("GDB: Deactivating Gdb plugin...");

	if (this->debugger != NULL)
	{
		debugger_free (this->debugger);
		this->debugger = NULL;
	}
	
	return TRUE;
}

/* GObject functions
 *---------------------------------------------------------------------------*/

/* Used in dispose and finalize */
static gpointer parent_class;

/* instance_init is the constructor. All functions should work after this
 * call. */

static void
gdb_plugin_instance_init (GObject* obj)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (obj);

	this->debugger = NULL;
	this->output_callback = NULL;
	this->term_pid = 0;
}

/* dispose is the first destruction step. It is used to unref object created
 * with instance_init in order to break reference counting cycles. This
 * function could be called several times. All function should still work
 * after this call. It has to called its parents.*/

static void
gdb_plugin_dispose (GObject* obj)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (obj);

	if (this->debugger != NULL)
	{	
		debugger_free (this->debugger);
		this->debugger = NULL;
	}
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (G_OBJECT (obj)));
}

/* finalize is the last destruction step. It must free all memory allocated
 * with instance_init. It is called only one time just before releasing all
 * memory */

static void
gdb_plugin_finalize (GObject* obj)
{
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (G_OBJECT (obj)));
}

/* class_init intialize the class itself not the instance */

static void
gdb_plugin_class_init (GObjectClass* klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = gdb_plugin_activate_plugin;
	plugin_class->deactivate = gdb_plugin_deactivate_plugin;
	klass->dispose = gdb_plugin_dispose;
	klass->finalize = gdb_plugin_finalize;
}

/* Implementation of IAnjutaDebugger interface
 *---------------------------------------------------------------------------*/

static IAnjutaDebuggerState
idebugger_get_state (IAnjutaDebugger *plugin, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	if (this->debugger == NULL)
	{
		return IANJUTA_DEBUGGER_STOPPED;
	}
	else
	{
		return debugger_get_state (this->debugger);
	}
}	


static gboolean
idebugger_load (IAnjutaDebugger *plugin, const gchar *file, const gchar* mime_type,
				const GList *search_dirs, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);
	gboolean is_libtool = FALSE;

	/* Check allowed mime type */
	if (strcmp (mime_type, "application/x-executable") == 0)
	{
	}
	else if (strcmp (mime_type, "application/x-shellscript") == 0)
	{
		/* FIXME: We should really do more checks to confirm that
		 * this target is indeed libtool target
		 */
		is_libtool = TRUE;
	}
	else if (strcmp (mime_type, "application/x-core") == 0)
	{
	}
	else
	{
		//return IANJUTA_DEBUGGER_NOT_IMPLEMENTED;
		return TRUE;
	}
	
	// a NULL filename could be used to check supported mime type
	if (file == NULL) return TRUE;
		
	// Start debugger
	gdb_plugin_initialize (this);
	
	return debugger_start (this->debugger, search_dirs, file, is_libtool);
}

static gboolean
idebugger_unload (IAnjutaDebugger *plugin, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	debugger_stop (this->debugger);

	return TRUE;
}

static gboolean
idebugger_attach (IAnjutaDebugger *plugin, pid_t pid, const GList *search_dirs, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	// Start debugger
	gdb_plugin_initialize (this);
	debugger_start (this->debugger, search_dirs, NULL, FALSE);
	debugger_attach_process (this->debugger, pid);
	
	return TRUE;
}

static gboolean
idebugger_start (IAnjutaDebugger *plugin, const gchar *argument, gboolean terminal, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);
	gchar *tty;

	tty = terminal ? gdb_plugin_start_terminal (this) : NULL;
	debugger_start_program (this->debugger, argument, tty);
	g_free (tty);

	return TRUE;
}

static gboolean
idebugger_quit (IAnjutaDebugger *plugin, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	if (!debugger_stop (this->debugger))
	{
		DEBUG_PRINT ("set error");
		g_set_error (err, IANJUTA_DEBUGGER_ERROR, IANJUTA_DEBUGGER_CANCEL, "Command cancelled by user");

		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

static gboolean
idebugger_abort (IAnjutaDebugger *plugin, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	DEBUG_PRINT ("idebugger abort\n");
	if (!debugger_abort (this->debugger))
	{
		g_set_error (err, IANJUTA_DEBUGGER_ERROR, IANJUTA_DEBUGGER_CANCEL, "Command cancelled by user");

		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

static gboolean
idebugger_run (IAnjutaDebugger *plugin, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);
	
	debugger_run (this->debugger);

	return TRUE;
}

static gboolean
idebugger_step_in (IAnjutaDebugger *plugin, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);
	
	debugger_step_in (this->debugger);

	return TRUE;
}

static gboolean
idebugger_step_over (IAnjutaDebugger *plugin, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);
	
	debugger_step_over (this->debugger);

	return TRUE;
}

static gboolean
idebugger_run_to (IAnjutaDebugger *plugin, const gchar *file,
						   gint line, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);
	
	debugger_run_to_position (this->debugger, file, line);

	return TRUE;
}

static gboolean
idebugger_step_out (IAnjutaDebugger *plugin, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);
	
	debugger_step_out (this->debugger);

	return TRUE;
}

static gboolean
idebugger_exit (IAnjutaDebugger *plugin, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	debugger_stop_program (this->debugger);

	return TRUE;
}

static gboolean
idebugger_interrupt (IAnjutaDebugger *plugin, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);
	
	debugger_interrupt (this->debugger);

	return TRUE;
}

static gboolean
idebugger_inspect (IAnjutaDebugger *plugin, const gchar *name, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);
	
	debugger_evaluate (this->debugger, name, callback, user_data);

	return TRUE;
}

static gboolean
idebugger_evaluate (IAnjutaDebugger *plugin, const gchar *name, const gchar *value, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);
	gchar* buf;

	buf = g_strconcat ("\"", name, " = ", value, "\"", NULL);
	debugger_evaluate (this->debugger, buf, callback, user_data);
	g_free (buf);

	return TRUE;
}

static gboolean
idebugger_send_command (IAnjutaDebugger *plugin, const gchar* command, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	debugger_command (this->debugger, command, FALSE, NULL, NULL);

	return TRUE;
}

static gboolean
idebugger_print (IAnjutaDebugger *plugin, const gchar* variable, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	debugger_print (this->debugger, variable, callback, user_data);

	return TRUE;
}

static gboolean
idebugger_list_local (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	debugger_list_local (this->debugger, callback, user_data);

	return TRUE;
}

static gboolean
idebugger_list_argument (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	debugger_list_argument (this->debugger, callback, user_data);

	return TRUE;
}

static gboolean
idebugger_info_signal (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	debugger_info_signal (this->debugger, callback, user_data);

	return TRUE;
}

static gboolean
idebugger_info_sharedlib (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	debugger_info_sharedlib (this->debugger, callback, user_data);

	return TRUE;
}

static gboolean
idebugger_handle_signal (IAnjutaDebugger *plugin, const gchar* name, gboolean stop, gboolean print, gboolean ignore, GError **err)
{
	gchar* cmd;
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	cmd = g_strdup_printf("handle %s %sstop %sprint %spass", name, stop ? "" : "no", print ? "" : "no", ignore ? "" : "no");
	debugger_command (this->debugger, cmd, FALSE, NULL, NULL);
	g_free (cmd);

	return TRUE;
}

static gboolean
idebugger_info_frame (IAnjutaDebugger *plugin, guint frame, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	debugger_info_frame (this->debugger, frame, callback, user_data);

	return TRUE;
}

static gboolean
idebugger_info_args (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	debugger_info_args (this->debugger, callback, user_data);

	return TRUE;
}

static gboolean
idebugger_info_target (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	debugger_info_target (this->debugger, callback, user_data);

	return TRUE;
}

static gboolean
idebugger_info_program (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	debugger_info_program (this->debugger, callback, user_data);

	return TRUE;
}

static gboolean
idebugger_info_udot (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	debugger_info_udot (this->debugger, callback, user_data);

	return TRUE;
}

static gboolean
idebugger_info_variables (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	debugger_info_variables (this->debugger, callback, user_data);

	return TRUE;
}

static gboolean
idebugger_set_frame (IAnjutaDebugger *plugin, guint frame, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	debugger_set_frame (this->debugger, frame);

	return TRUE;
}

static gboolean
idebugger_list_frame (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	debugger_list_frame (this->debugger, callback, user_data);

	return TRUE;
}

static gboolean
idebugger_set_thread (IAnjutaDebugger *plugin, gint thread, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	debugger_set_thread (this->debugger, thread);

	return TRUE;
}

static gboolean
idebugger_list_thread (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	debugger_list_thread (this->debugger, callback, user_data);

	return TRUE;
}

static gboolean
idebugger_info_thread (IAnjutaDebugger *plugin, gint thread, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	debugger_info_thread (this->debugger, thread, callback, user_data);

	return TRUE;
}

static gboolean
idebugger_list_register (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	//GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	//debugger_list_register (this->debugger, callback, user_data);

	return TRUE;
}

static gboolean
idebugger_callback (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{

	callback (NULL, user_data, NULL);

	return TRUE;
}

static void
idebugger_enable_log (IAnjutaDebugger *plugin, IAnjutaMessageView *log, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	this->view = log;
	if (this->debugger)
		debugger_set_log (this->debugger, log);
}

static void
idebugger_disable_log (IAnjutaDebugger *plugin, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	this->view = NULL;
	if (this->debugger)
		debugger_set_log (this->debugger, NULL);
}

static void
idebugger_iface_init (IAnjutaDebuggerIface *iface)
{
	iface->get_state = idebugger_get_state;
	iface->attach = idebugger_attach;
	iface->load = idebugger_load;
	iface->start = idebugger_start;
	iface->unload = idebugger_unload;
	iface->quit = idebugger_quit;
	iface->abort = idebugger_abort;
	iface->run = idebugger_run;
	iface->step_in = idebugger_step_in;
	iface->step_over = idebugger_step_over;
	iface->step_out = idebugger_step_out;
	iface->run_to = idebugger_run_to;
	iface->exit = idebugger_exit;
	iface->interrupt = idebugger_interrupt;

	iface->inspect = idebugger_inspect;
	iface->evaluate = idebugger_evaluate;

	iface->print = idebugger_print;
	iface->list_local = idebugger_list_local;
	iface->list_argument = idebugger_list_argument;
	iface->info_frame = idebugger_info_frame;
	iface->info_signal = idebugger_info_signal;
	iface->info_sharedlib = idebugger_info_sharedlib;
	iface->info_args = idebugger_info_args;
	iface->info_target = idebugger_info_target;
	iface->info_program = idebugger_info_program;
	iface->info_udot = idebugger_info_udot;
	iface->info_variables = idebugger_info_variables;
	iface->handle_signal = idebugger_handle_signal;
	iface->list_frame = idebugger_list_frame;
	iface->set_frame = idebugger_set_frame;
	iface->list_thread = idebugger_list_thread;
	iface->set_thread = idebugger_set_thread;
	iface->info_thread = idebugger_info_thread;
	iface->list_register = idebugger_list_register;

	iface->send_command = idebugger_send_command;

	iface->callback = idebugger_callback;

	iface->enable_log = idebugger_enable_log;
	iface->disable_log = idebugger_disable_log;
}


/* Implementation of IAnjutaBreakpointDebugger interface
 *---------------------------------------------------------------------------*/

static gboolean
ibreakpoint_debugger_add_breakpoint_at_line (IAnjutaBreakpointDebugger *plugin, const gchar* file, guint line, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);
	
	debugger_add_breakpoint_at_line (this->debugger, file, line, callback, user_data);

	return TRUE;
}

static gboolean
ibreakpoint_debugger_add_breakpoint_at_function (IAnjutaBreakpointDebugger *plugin, const gchar* file, const gchar* function, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);
	
	debugger_add_breakpoint_at_function (this->debugger, *file == '\0' ? NULL : file, function, callback, user_data);

	return TRUE;
}

static gboolean
ibreakpoint_debugger_add_breakpoint_at_address (IAnjutaBreakpointDebugger *plugin, guint address, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);
	
	debugger_add_breakpoint_at_address (this->debugger, address, callback, user_data);

	return TRUE;
}

static gboolean
ibreakpoint_debugger_enable_breakpoint (IAnjutaBreakpointDebugger *plugin, guint id, gboolean enable, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);
	
	debugger_enable_breakpoint (this->debugger, id, enable, callback, user_data);

	return TRUE;
}

static gboolean
ibreakpoint_debugger_ignore_breakpoint (IAnjutaBreakpointDebugger *plugin, guint id, guint ignore, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);
	
	debugger_ignore_breakpoint (this->debugger, id, ignore, callback, user_data);

	return TRUE;
}

static gboolean
ibreakpoint_debugger_condition_breakpoint (IAnjutaBreakpointDebugger *plugin, guint id, const gchar *condition, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);
	
	debugger_condition_breakpoint (this->debugger, id, condition, callback, user_data);

	return TRUE;
}

static gboolean
ibreakpoint_debugger_remove_breakpoint (IAnjutaBreakpointDebugger *plugin, guint id, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	debugger_remove_breakpoint (this->debugger, id, callback, user_data);

	return TRUE;
}

static void
ibreakpoint_debugger_iface_init (IAnjutaBreakpointDebuggerIface *iface)
{
	iface->set_breakpoint_at_line = ibreakpoint_debugger_add_breakpoint_at_line;
	iface->clear_breakpoint = ibreakpoint_debugger_remove_breakpoint;
	iface->set_breakpoint_at_address = ibreakpoint_debugger_add_breakpoint_at_address;
	iface->set_breakpoint_at_function = ibreakpoint_debugger_add_breakpoint_at_function;
	iface->enable_breakpoint = ibreakpoint_debugger_enable_breakpoint;
	iface->ignore_breakpoint = ibreakpoint_debugger_ignore_breakpoint;
	iface->condition_breakpoint = ibreakpoint_debugger_condition_breakpoint;
}

/* Implementation of IAnjutaCpuDebugger interface
 *---------------------------------------------------------------------------*/

static gboolean
icpu_debugger_list_register (IAnjutaCpuDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	debugger_list_register (this->debugger, callback, user_data);

	return TRUE;
}

static gboolean
icpu_debugger_update_register (IAnjutaCpuDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	debugger_update_register (this->debugger, callback, user_data);

	return TRUE;
}

static gboolean
icpu_debugger_write_register (IAnjutaCpuDebugger *plugin, IAnjutaDebuggerRegister *value, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	debugger_write_register (this->debugger, value->name, value->value);

	return TRUE;
}

static gboolean
icpu_debugger_inspect_memory (IAnjutaCpuDebugger *plugin, guint address, guint length, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	GdbPlugin *this = ANJUTA_PLUGIN_GDB (plugin);

	debugger_inspect_memory (this->debugger, address, length, callback, user_data);

	return TRUE;
}

static gboolean
icpu_debugger_disassemble (IAnjutaCpuDebugger *plugin, guint address, guint length, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	GdbPlugin *this = (GdbPlugin *)plugin;

	debugger_disassemble (this->debugger, address, length, callback, user_data);

	return TRUE;
}

static void
icpu_debugger_iface_init (IAnjutaCpuDebuggerIface *iface)
{
	iface->list_register = icpu_debugger_list_register;
	iface->update_register = icpu_debugger_update_register;
	iface->write_register = icpu_debugger_write_register;
	iface->inspect_memory = icpu_debugger_inspect_memory;
	iface->disassemble = icpu_debugger_disassemble;
}

/* Implementation of IAnjutaVariableDebugger interface
 *---------------------------------------------------------------------------*/

static gboolean
ivariable_debugger_delete (IAnjutaVariableDebugger *plugin, const gchar *name, GError **error)
{
	GdbPlugin *gdb = ANJUTA_PLUGIN_GDB (plugin);

	debugger_delete_variable (gdb->debugger, name);

	return TRUE;
}

static gboolean
ivariable_debugger_evaluate (IAnjutaVariableDebugger *plugin, const gchar *name, IAnjutaDebuggerCallback callback , gpointer user_data, GError **error)
{
	GdbPlugin *gdb = ANJUTA_PLUGIN_GDB (plugin);

	debugger_evaluate_variable (gdb->debugger, name, callback, user_data);

	return TRUE;
}

static gboolean
ivariable_debugger_assign (IAnjutaVariableDebugger *plugin, const gchar *name, const gchar *value, GError **error)
{
	GdbPlugin *gdb = ANJUTA_PLUGIN_GDB (plugin);

	debugger_assign_variable (gdb->debugger, name, value);

	return TRUE;
}

static gboolean
ivariable_debugger_list_children (IAnjutaVariableDebugger *plugin, const gchar *name, IAnjutaDebuggerCallback callback , gpointer user_data, GError **error)
{
	GdbPlugin *gdb = ANJUTA_PLUGIN_GDB (plugin);

	debugger_list_variable_children (gdb->debugger, name, callback, user_data);

	return TRUE;
}

static gboolean
ivariable_debugger_create (IAnjutaVariableDebugger *plugin, const gchar *name, IAnjutaDebuggerCallback callback , gpointer user_data, GError **error)
{
	GdbPlugin *gdb = ANJUTA_PLUGIN_GDB (plugin);

	debugger_create_variable (gdb->debugger, name, callback, user_data);

	return TRUE;
}

static gboolean
ivariable_debugger_update (IAnjutaVariableDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **error)
{
	GdbPlugin *gdb = ANJUTA_PLUGIN_GDB (plugin);

	debugger_update_variable (gdb->debugger, callback, user_data);

	return TRUE;
}

static void
ivariable_debugger_iface_init (IAnjutaVariableDebuggerIface *iface)
{
	iface->delete_var = ivariable_debugger_delete;
	iface->evaluate = ivariable_debugger_evaluate;
	iface->assign = ivariable_debugger_assign;
	iface->list_children = ivariable_debugger_list_children;
	iface->create = ivariable_debugger_create;
	iface->update = ivariable_debugger_update;
}

ANJUTA_PLUGIN_BEGIN (GdbPlugin, gdb_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(idebugger, IANJUTA_TYPE_DEBUGGER);
ANJUTA_PLUGIN_ADD_INTERFACE(ibreakpoint_debugger, IANJUTA_TYPE_BREAKPOINT_DEBUGGER);
ANJUTA_PLUGIN_ADD_INTERFACE(icpu_debugger, IANJUTA_TYPE_CPU_DEBUGGER);
ANJUTA_PLUGIN_ADD_INTERFACE(ivariable_debugger, IANJUTA_TYPE_VARIABLE_DEBUGGER);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (GdbPlugin, gdb_plugin);
