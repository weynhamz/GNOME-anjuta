/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    Copyright (C) 2009 Maxim Ermilov   <zaspire@rambler.ru>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <glib.h>
#include <config.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-debugger.h>
#include <libanjuta/interfaces/ianjuta-debugger-breakpoint.h>
#include <libanjuta/interfaces/ianjuta-debugger-register.h>
#include <libanjuta/interfaces/ianjuta-debugger-memory.h>
#include <libanjuta/interfaces/ianjuta-debugger-instruction.h>
#include <libanjuta/interfaces/ianjuta-debugger-variable.h>
#include <libanjuta/interfaces/ianjuta-terminal.h>
#include <libanjuta/anjuta-plugin.h>
#include "plugin.h"
#include "debugger-js.h"

struct _JSDbg
{
	AnjutaPlugin parent;
	DebuggerJs *debugger;
};

struct _JSDbgClass
{
	AnjutaPluginClass parent_class;
};

static gpointer parent_class;

#define DEBUGGER_PORT 2234

static gboolean
js_debugger_activate (AnjutaPlugin *plugin)
{
	JSDbg *js_debugger;

	DEBUG_PRINT ("%s", "JSDbg: Activating JSDbg plugin ...");
	js_debugger = (JSDbg*) plugin;

	return TRUE;
}

static gboolean
js_debugger_deactivate (AnjutaPlugin *plugin)
{
	DEBUG_PRINT ("%s", "JSDbg: Dectivating JSDbg plugin ...");

	return TRUE;
}

static void
js_debugger_finalize (GObject *obj)
{
	/* Finalization codes here */
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
js_debugger_dispose (GObject *obj)
{
	/* Disposition codes */
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
js_debugger_instance_init (GObject *obj)
{
}

static void
js_debugger_class_init (GObjectClass *klass)
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = js_debugger_activate;
	plugin_class->deactivate = js_debugger_deactivate;
	klass->finalize = js_debugger_finalize;
	klass->dispose = js_debugger_dispose;
}

/* Implementation of IAnjutaDebugger interface
 *---------------------------------------------------------------------------*/

static IAnjutaDebuggerState
idebugger_get_state (IAnjutaDebugger *plugin, GError **err)
{
	DEBUG_PRINT ("%s", "get_state: Implemented");

	JSDbg *self = ANJUTA_PLUGIN_JSDBG (plugin);
	if (self->debugger == NULL)
		return IANJUTA_DEBUGGER_STOPPED;
	return debugger_js_get_state (self->debugger);
}

static void
on_error (DebuggerJs *self, const gchar *text, gpointer user_data)
{
	JSDbg *object = ANJUTA_PLUGIN_JSDBG (user_data);

	anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (object)->shell),
							  _("Error: %s"), text);
	if (object->debugger != NULL)
		g_object_unref (object->debugger);
	object->debugger = NULL;
}

static gboolean
idebugger_load (IAnjutaDebugger *plugin, const gchar *file, const gchar* mime_type,
				const GList *search_dirs, GError **err)
{
	DEBUG_PRINT ("%s", "load: Implemented");

	JSDbg *self = ANJUTA_PLUGIN_JSDBG (plugin);
	if (self->debugger != NULL)
		g_object_unref (self->debugger);
	if ( (self->debugger = debugger_js_new (DEBUGGER_PORT, file, plugin)) )
	{
		g_signal_connect (self->debugger, "DebuggerError", G_CALLBACK (on_error), self);
		return TRUE;
	}
	on_error (NULL, _("Error: cant bind port"), self);
	return FALSE;
}

static gboolean
idebugger_unload (IAnjutaDebugger *plugin, GError **err)
{
	DEBUG_PRINT ("%s", "unload: Implemented");

	JSDbg *self = ANJUTA_PLUGIN_JSDBG (plugin);
	if (self->debugger != NULL)
		g_object_unref (self->debugger);

	return TRUE;
}

static gboolean
idebugger_set_working_directory (IAnjutaDebugger *plugin, const gchar *directory, GError **err)
{
	DEBUG_PRINT ("%s %s", "set_working_directory: Implemented", directory);

	JSDbg *self = ANJUTA_PLUGIN_JSDBG (plugin);
	debugger_js_set_work_dir (self->debugger, directory);
	return TRUE;
}

static gboolean
idebugger_set_environment (IAnjutaDebugger *plugin, gchar **variables, GError **err)
{
	DEBUG_PRINT ("%s", "set_environment: Not Implemented");

	return FALSE;
}

static gboolean
idebugger_attach (IAnjutaDebugger *plugin, pid_t pid, const GList *search_dirs, GError **err)
{
	DEBUG_PRINT ("%s", "attach: Not Implemented");

	return FALSE;
}

static gboolean
idebugger_start (IAnjutaDebugger *plugin, const gchar *argument, gboolean terminal, gboolean stop, GError **err)
{
	DEBUG_PRINT ("%s", "start: Implemented");

	JSDbg *self = ANJUTA_PLUGIN_JSDBG (plugin);
	debugger_js_start (self->debugger, argument);
	return TRUE;
}

static gboolean
idebugger_connect (IAnjutaDebugger *plugin, const gchar *server, const gchar *argument, gboolean terminal, gboolean stop, GError **err)
{
	DEBUG_PRINT ("%s", "connect: Not Implemented");
	JSDbg *self = ANJUTA_PLUGIN_JSDBG (plugin);
	gint port = DEBUGGER_PORT;

	if (server)
	{
		gint len = strlen (server), k;

		for (k = len - 1; k >= 0; k--)
		{
			if (server[k] > '9' || server[k] < '0')
				break;
			port = port * 10 + server[k] - '0';
		}
		k++;
		if (k != len - 1)
			sscanf (server + k, "%d", &port);
	}
	debugger_js_start_remote (self->debugger, port);
	return TRUE;
}

static gboolean
idebugger_quit (IAnjutaDebugger *plugin, GError **err)
{
	DEBUG_PRINT ("%s", "quit: Implemented");
	JSDbg *self = ANJUTA_PLUGIN_JSDBG (plugin);
	debugger_js_stop (self->debugger);
	return TRUE;
}

static gboolean
idebugger_abort (IAnjutaDebugger *plugin, GError **err)
{
	DEBUG_PRINT ("%s", "abort: Implemented");
	JSDbg *self = ANJUTA_PLUGIN_JSDBG (plugin);
	debugger_js_stop (self->debugger);
	return TRUE;
}

static gboolean
idebugger_run (IAnjutaDebugger *plugin, GError **err)
{
	DEBUG_PRINT ("%s", "run: Implemented");

	JSDbg *self = ANJUTA_PLUGIN_JSDBG (plugin);
	debugger_js_continue (self->debugger);
	return TRUE;
}

static gboolean
idebugger_step_in (IAnjutaDebugger *plugin, GError **err)
{
	DEBUG_PRINT ("%s", "step_in: Implemented");
	JSDbg *self = ANJUTA_PLUGIN_JSDBG (plugin);
	debugger_js_stepin (self->debugger);
	return TRUE;
}

static gboolean
idebugger_step_over (IAnjutaDebugger *plugin, GError **err)
{
	DEBUG_PRINT ("%s", "step_over: Implemented");
	JSDbg *self = ANJUTA_PLUGIN_JSDBG (plugin);
	debugger_js_stepover (self->debugger);
	return TRUE;
}

static gboolean
idebugger_run_to (IAnjutaDebugger *plugin, const gchar* file,
						   gint line, GError **err)
{
	DEBUG_PRINT ("%s", "run_to: Not Implemented");

	return FALSE;
}

static gboolean
idebugger_step_out (IAnjutaDebugger *plugin, GError **err)
{
	DEBUG_PRINT ("%s", "step_out: Implemented");

	JSDbg *self = ANJUTA_PLUGIN_JSDBG (plugin);
	debugger_js_stepout (self->debugger);
	return TRUE;
}

static gboolean
idebugger_exit (IAnjutaDebugger *plugin, GError **err)
{
	DEBUG_PRINT ("%s", "exit: Implemented");
	JSDbg *self = ANJUTA_PLUGIN_JSDBG (plugin);
	debugger_js_stop (self->debugger);
	return TRUE;
}

static gboolean
idebugger_interrupt (IAnjutaDebugger *plugin, GError **err)
{
	DEBUG_PRINT ("%s", "interrupt: Not Implemented");

	JSDbg *self = ANJUTA_PLUGIN_JSDBG (plugin);
	debugger_js_stop (self->debugger);
	return TRUE;
}

static gboolean
idebugger_inspect (IAnjutaDebugger *plugin, const gchar *name, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	DEBUG_PRINT ("%s", "inspect: Not Implemented");

	return FALSE;
}

static gboolean
idebugger_evaluate (IAnjutaDebugger *plugin, const gchar *name, const gchar *value, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	DEBUG_PRINT ("%s", "evaluate : Not Implemented");

	return FALSE;
}

static gboolean
idebugger_send_command (IAnjutaDebugger *plugin, const gchar* command, GError **err)
{
	DEBUG_PRINT ("%s", "send_command: Not Implemented");

	return FALSE;
}

static gboolean
idebugger_print (IAnjutaDebugger *plugin, const gchar* variable, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	DEBUG_PRINT ("%s", "print: Not Implemented");

	return FALSE;
}

static gboolean
idebugger_list_local (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DEBUG_PRINT ("%s", "list_local: Implemented");
	JSDbg *self = ANJUTA_PLUGIN_JSDBG (plugin);
	debugger_js_list_local (self->debugger, callback, user_data);
	return TRUE;
}

static gboolean
idebugger_list_argument (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DEBUG_PRINT ("%s", "list_argument: Not Implemented");

	return FALSE;
}

static gboolean
idebugger_info_signal (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DEBUG_PRINT ("%s", "info_signal: Not Implemented");

	return FALSE;
}

static gboolean
idebugger_info_sharedlib (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DEBUG_PRINT ("%s", "info_sharedlib: Not Implemented");

	return FALSE;
}

static gboolean
idebugger_handle_signal (IAnjutaDebugger *plugin, const gchar* name, gboolean stop, gboolean print, gboolean ignore, GError **err)
{
	DEBUG_PRINT ("%s", "handle_signal: Not Implemented");

	return TRUE;
}

static gboolean
idebugger_info_frame (IAnjutaDebugger *plugin, guint frame, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DEBUG_PRINT ("%s", "info_frame: Not Implemented");

	return FALSE;
}

static gboolean
idebugger_info_args (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DEBUG_PRINT ("%s", "info_args: Not Implemented");

	return FALSE;
}

static gboolean
idebugger_info_target (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DEBUG_PRINT ("%s", "info_target: Not Implemented");

	return FALSE;
}

static gboolean
idebugger_info_program (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DEBUG_PRINT ("%s", "info_program: Not Implemented");

	return FALSE;
}

static gboolean
idebugger_info_udot (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DEBUG_PRINT ("%s", "info_udot: Not Implemented");

	return FALSE;
}

static gboolean
idebugger_info_variables (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DEBUG_PRINT ("%s", "info_variables: Not Implemented");

	return FALSE;
}

static gboolean
idebugger_set_frame (IAnjutaDebugger *plugin, guint frame, GError **err)
{
	DEBUG_PRINT ("%s", "set_frame: Not Implemented");

	return FALSE;
}

static gboolean
idebugger_list_frame (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DEBUG_PRINT ("%s", "list_frame: Implemented");
	JSDbg *self = ANJUTA_PLUGIN_JSDBG (plugin);
	debugger_js_list_frame (self->debugger, callback, user_data);
	return TRUE;
}

static gboolean
idebugger_set_thread (IAnjutaDebugger *plugin, gint thread, GError **err)
{
	DEBUG_PRINT ("%s", "set_thread: Not Implemented");

	return FALSE;
}

static gboolean
idebugger_list_thread (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DEBUG_PRINT ("%s", "list_thread: Implemented(Fake)");
	JSDbg *self = ANJUTA_PLUGIN_JSDBG (plugin);
	debugger_js_list_thread (self->debugger, callback, user_data);
	return TRUE;
}

static gboolean
idebugger_info_thread (IAnjutaDebugger *plugin, gint thread, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DEBUG_PRINT ("%s", "info_thread: Implemented(Fake)");
	JSDbg *self = ANJUTA_PLUGIN_JSDBG (plugin);
	debugger_js_info_thread (self->debugger, callback, thread, user_data);
	return TRUE;
}

static gboolean
idebugger_list_register (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DEBUG_PRINT ("%s", "list_register: Not Implemented");

	return FALSE;
}

static gboolean
idebugger_run_from (IAnjutaDebugger *plugin, const gchar *file, gint line, GError **err)
{
	DEBUG_PRINT ("%s", "run_from: Not Implemented");

	return FALSE;
}

static gboolean
idebugger_dump_stack_trace (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DEBUG_PRINT ("%s", "dump_stack_trace: Not Implemented");

	return FALSE;
}

static gboolean
idebugger_callback (IAnjutaDebugger *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DEBUG_PRINT ("%s", "callback: Implemented");
	JSDbg *self = ANJUTA_PLUGIN_JSDBG (plugin);
	debugger_js_signal (self->debugger, callback, user_data);
	return TRUE;
}

static void
idebugger_enable_log (IAnjutaDebugger *plugin, IAnjutaMessageView *log, GError **err)
{
	DEBUG_PRINT ("%s", "enable_log: Not Implemented");
}

static void
idebugger_disable_log (IAnjutaDebugger *plugin, GError **err)
{
	DEBUG_PRINT ("%s", "disable_log: Not Implemented");
}

static void
idebugger_iface_init (IAnjutaDebuggerIface *iface)
{
	iface->get_state = idebugger_get_state;
	iface->attach = idebugger_attach;
	iface->load = idebugger_load;
	iface->set_working_directory = idebugger_set_working_directory;
	iface->set_environment = idebugger_set_environment;
	iface->start = idebugger_start;
	iface->connect = idebugger_connect;
	iface->unload = idebugger_unload;
	iface->quit = idebugger_quit;
	iface->abort = idebugger_abort;
	iface->run = idebugger_run;
	iface->step_in = idebugger_step_in;
	iface->step_over = idebugger_step_over;
	iface->step_out = idebugger_step_out;
	iface->run_to = idebugger_run_to;
	iface->run_from = idebugger_run_from;
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
	iface->dump_stack_trace = idebugger_dump_stack_trace;

	iface->send_command = idebugger_send_command;

	iface->callback = idebugger_callback;

	iface->enable_log = idebugger_enable_log;
	iface->disable_log = idebugger_disable_log;
}

/* Implementation of IAnjutaDebuggerBreakpoint interface
 *---------------------------------------------------------------------------*/

static gint
idebugger_breakpoint_implement (IAnjutaDebuggerBreakpoint *plugin, GError **err)
{
	return IANJUTA_DEBUGGER_BREAKPOINT_ON_LINE
		| IANJUTA_DEBUGGER_BREAKPOINT_ENABLE
		| IANJUTA_DEBUGGER_BREAKPOINT_IGNORE
		| IANJUTA_DEBUGGER_BREAKPOINT_CONDITION;
}

static gboolean
idebugger_breakpoint_add_at_line (IAnjutaDebuggerBreakpoint *plugin, const gchar* file, guint line, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	DEBUG_PRINT ("%s", "breakpoint_add_at_line: Implemented");
	JSDbg *self = ANJUTA_PLUGIN_JSDBG (plugin);
	debugger_js_add_breakpoint (self->debugger, file, line);
	return FALSE;
}

static gboolean
idebugger_breakpoint_enable (IAnjutaDebuggerBreakpoint *plugin, guint id, gboolean enable, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	DEBUG_PRINT ("%s", "breakpoint_enable: Not Implemented");

	return FALSE;
}

static gboolean
idebugger_breakpoint_ignore (IAnjutaDebuggerBreakpoint *plugin, guint id, guint ignore, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	DEBUG_PRINT ("%s", "breakpoint_ignore: Not Implemented");

	return FALSE;
}

static gboolean
idebugger_breakpoint_condition (IAnjutaDebuggerBreakpoint *plugin, guint id, const gchar *condition, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	DEBUG_PRINT ("%s", "breakpoint_condition: Not Implemented");

	return FALSE;
}

static gboolean
idebugger_breakpoint_remove (IAnjutaDebuggerBreakpoint *plugin, guint id, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	DEBUG_PRINT ("%s", "breakpoint_remove: Not Implemented");

	return FALSE;
}

static gboolean
idebugger_breakpoint_list (IAnjutaDebuggerBreakpoint *plugin, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	DEBUG_PRINT ("%s", "breakpoint_list: Implemented");
	JSDbg *self = ANJUTA_PLUGIN_JSDBG (plugin);
	debugger_js_breakpoint_list (self->debugger, callback, user_data);
	return TRUE;
}

static void
idebugger_breakpoint_iface_init (IAnjutaDebuggerBreakpointIface *iface)
{
	iface->implement_breakpoint = idebugger_breakpoint_implement;
	iface->set_breakpoint_at_line = idebugger_breakpoint_add_at_line;
	iface->clear_breakpoint = idebugger_breakpoint_remove;
	iface->list_breakpoint = idebugger_breakpoint_list;
	iface->set_breakpoint_at_address = NULL;
	iface->set_breakpoint_at_function = NULL;
	iface->enable_breakpoint = idebugger_breakpoint_enable;
	iface->ignore_breakpoint = idebugger_breakpoint_ignore;
	iface->condition_breakpoint = idebugger_breakpoint_condition;
}

/* Implementation of IAnjutaDebuggerVariable interface
 *---------------------------------------------------------------------------*/

static gboolean
idebugger_variable_destroy (IAnjutaDebuggerVariable *plugin, const gchar *name, GError **error)
{
	DEBUG_PRINT ("%s", "variable_destroy: Not Implemented");
	return TRUE;
}

static gboolean
idebugger_variable_evaluate (IAnjutaDebuggerVariable *plugin, const gchar *name, IAnjutaDebuggerCallback callback , gpointer user_data, GError **error)
{
	puts (name);
	DEBUG_PRINT ("%s", "variable_evaluate: Not Implemented");
	return FALSE;
}

static gboolean
idebugger_variable_assign (IAnjutaDebuggerVariable *plugin, const gchar *name, const gchar *value, GError **error)
{
	puts (name);
	DEBUG_PRINT ("%s", "variable_assign: Not Implemented");
	return FALSE;
}

static gboolean
idebugger_variable_list_children (IAnjutaDebuggerVariable *plugin, const gchar *name, IAnjutaDebuggerCallback callback , gpointer user_data, GError **error)
{
	puts (name);
	DEBUG_PRINT ("%s", "variable_list_children: Implemented");
	JSDbg *self = ANJUTA_PLUGIN_JSDBG (plugin);
	debugger_js_variable_list_children (self->debugger, callback, name, user_data);
	return TRUE;
}

static gboolean
idebugger_variable_create (IAnjutaDebuggerVariable *plugin, const gchar *name, IAnjutaDebuggerCallback callback , gpointer user_data, GError **error)
{
	puts (name);
	DEBUG_PRINT ("%s", "idebugger_variable_create: Implemented");
	JSDbg *self = ANJUTA_PLUGIN_JSDBG (plugin);
	debugger_js_variable_create (self->debugger, callback, name, user_data);
	return TRUE;
}

static gboolean
idebugger_variable_update (IAnjutaDebuggerVariable *plugin, IAnjutaDebuggerCallback callback , gpointer user_data, GError **error)
{
	DEBUG_PRINT ("%s", "idebugger_variable_update: Not Implemented");
	return FALSE;
}

static void
idebugger_variable_iface_init (IAnjutaDebuggerVariableIface *iface)
{
	iface->destroy = idebugger_variable_destroy;
	iface->evaluate = idebugger_variable_evaluate;
	iface->assign = idebugger_variable_assign;
	iface->list_children = idebugger_variable_list_children;
	iface->create = idebugger_variable_create;
	iface->update = idebugger_variable_update;
}

ANJUTA_PLUGIN_BEGIN (JSDbg, js_debugger);
ANJUTA_PLUGIN_ADD_INTERFACE(idebugger, IANJUTA_TYPE_DEBUGGER);
ANJUTA_PLUGIN_ADD_INTERFACE(idebugger_breakpoint, IANJUTA_TYPE_DEBUGGER_BREAKPOINT);
ANJUTA_PLUGIN_ADD_INTERFACE(idebugger_variable, IANJUTA_TYPE_DEBUGGER_VARIABLE);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (JSDbg, js_debugger);
