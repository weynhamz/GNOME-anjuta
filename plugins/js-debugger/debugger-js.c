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
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-debugger.h>
#include <libanjuta/interfaces/ianjuta-debugger-breakpoint.h>
#include <libanjuta/interfaces/ianjuta-debugger-variable.h>
#include <libanjuta/interfaces/ianjuta-terminal.h>
#include "debugger-server.h"
#include "debugger-js.h"

typedef struct _DebuggerJsPrivate DebuggerJsPrivate;
struct _DebuggerJsPrivate
{
	IAnjutaTerminal *terminal;
	gchar *filename;
	gboolean started, exited;
	gboolean dataRecived;
	IAnjutaDebugger *data;
	gchar *working_directory;
	gchar *current_source_file;
	gint current_line;
	gboolean busy;
	GList *breakpoint;
	guint BID;
	pid_t pid;
	DebuggerServer *server;
	GList *task_queue;
};

#define DEBUGGER_JS_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DEBUGGER_TYPE_JS, DebuggerJsPrivate))

enum
{
	DEBUGGER_ERROR,
	LAST_SIGNAL
};

static guint js_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (DebuggerJs, debugger_js, G_TYPE_OBJECT);

enum
{
	SIGNAL,
	BREAKPOINT_LIST,
	VARIABLE_LIST_CHILDREN,
	LIST_LOCAL,
	LIST_THREAD,
	LIST_FRAME,
	INFO_THREAD,
	VARIABLE_CREATE,
};

struct Task
{
	IAnjutaDebuggerCallback callback;
	gpointer user_data;
	gint line_required;
	gint task_type;
	union
	{
		struct
		{
			gchar *name;
		}VareableListChildren;
	}this_data;
	gchar *name;
};

static void
debugger_js_init (DebuggerJs *object)
{
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);

	priv->current_source_file = NULL;
	priv->working_directory = g_strdup (".");
	priv->dataRecived = FALSE;
	priv->started = FALSE;
	priv->exited = FALSE;
	priv->busy = FALSE;
	priv->breakpoint = NULL;
	priv->BID = 1234;
	priv->pid = 0;
	priv->task_queue = NULL;

	priv->terminal = NULL;
	priv->filename = NULL;
	priv->data = NULL;
	priv->current_line = 0;
	priv->server = NULL;
}

static void on_child_exited (IAnjutaTerminal *obj, gint pid,  gint status, gpointer user_data);

static void
debugger_js_finalize (GObject *object)
{
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);

	g_assert (priv != NULL);

	g_signal_handlers_disconnect_by_func(G_OBJECT (priv->terminal), on_child_exited, object);

	g_free (priv->filename);
	g_free (priv->working_directory);
	g_free (priv->current_source_file);
	g_list_foreach (priv->breakpoint, (GFunc)g_free, NULL);
	g_list_free (priv->breakpoint);
	debugger_server_stop (priv->server);
	g_object_unref (priv->server);
	g_list_foreach (priv->task_queue, (GFunc)g_free, NULL);
	g_list_free (priv->task_queue);

	G_OBJECT_CLASS (debugger_js_parent_class)->finalize (object);
}

static void
debugger_js_debugger_error (DebuggerJs *self, const gchar *text)
{
	g_warning ("%s", text);
}

static void
debugger_js_class_init (DebuggerJsClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (DebuggerJsPrivate));

	object_class->finalize = debugger_js_finalize;

	klass->DebuggerError = debugger_js_debugger_error;

	js_signals[DEBUGGER_ERROR] =
		g_signal_new ("DebuggerError",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (DebuggerJsClass, DebuggerError),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__POINTER,
		              G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static void
task_added (DebuggerJs* object)
{
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);
	priv->busy = TRUE;
	g_signal_emit_by_name (priv->data, "debugger-ready", IANJUTA_DEBUGGER_BUSY);
}

static gpointer
variable_create (DebuggerJs* object, struct Task *task)
{
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);

	IAnjutaDebuggerVariableObject *var = g_new (IAnjutaDebuggerVariableObject, 1);
	gchar *str = debugger_server_get_line (priv->server);
	var->expression = g_strdup (task->name);
	var->name = g_strdup (task->name);
	var->type = g_strdup ("object");
	var->value = g_strdup ("object");
	var->children = 0;
	var->changed = TRUE;
	var->exited = FALSE;
	var->deleted = FALSE;

	if (str[0] != '{')
	{
		var->type = g_strdup ("string");
		var->value = g_strdup (str);
	}
	else
	{
		size_t i, k, size;
		for (i = 0, k = 0, size = strlen (str); i < size; i++)
		{
			if (str[i] == '{')
				k++;
			if (str[i] == '}')
			{
				k--;
				if (k == 0)
					var->children ++;
			}
		}
	}
	g_free (str);
	return var;
}

static gpointer
info_thread (DebuggerJs* object, struct Task *task)
{
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);
	IAnjutaDebuggerFrame* frame;
	gint thread;

	thread = 123;
	frame = g_new0 (IAnjutaDebuggerFrame, 1);

	frame->thread = thread;
	frame->address = 0xFFFF;
	frame->file = g_strdup (priv->current_source_file);
	frame->line = priv->current_line;
	frame->library = NULL;
	frame->function = NULL;

	return frame;
}

static gpointer
list_frame (DebuggerJs* object, struct Task *task)
{
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);

	IAnjutaDebuggerFrame* frame;
	GList *var = NULL;
	gint i, size;
	gchar *k;
	gchar *line = debugger_server_get_line (priv->server);

	for (k = line, i = 0, size = strlen (line); i <= size; i++)
	{
		if (line [i] == ',')
		{
			gchar *filename = g_new (gchar, strlen (line) + 1);
			gint lineno;

			frame = g_new0 (IAnjutaDebuggerFrame, 1);

			line [i] = '\0';
			if (sscanf (k, " LINE# %d %s", &lineno, filename) != 2)
			{
				g_signal_emit_by_name (object, "DebuggerError", "Invalid data arrived", G_TYPE_NONE);
				continue;
			}
			frame->file = filename;
			frame->line = lineno;
			frame->args = NULL;
			frame->function = NULL;
			frame->library = NULL;
			frame->thread = 123;
			var = g_list_append (var, frame);
			k = line + i + 1;
		}
	}
	g_free (line);
/*	g_list_foreach (var, GFunc (g_free), NULL);
	g_list_free (var);*/
	return var;
}

static gpointer
list_thread (DebuggerJs* object, struct Task *task)
{
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);
	
	IAnjutaDebuggerFrame* frame;
	GList* list = NULL;

	frame = g_new0 (IAnjutaDebuggerFrame, 1);
	frame->thread = 123;
	frame->address = 0;
	frame->file = g_strdup (priv->current_source_file);
	frame->line = priv->current_line;
	frame->library = NULL;
	frame->function = NULL;
	list = g_list_append (list, frame);
//TODO:Free list?
	return list;
}

static gpointer
list_local (DebuggerJs* object, struct Task *task)
{
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);
	GList *var = NULL;
	gint i, size;
	gchar *k;
	gchar *line = debugger_server_get_line (priv->server);

	for (k = line, i = 0, size = strlen (line); i <= size; i++)
	{
		if (line [i] == ',')
		{
			line [i] = '\0';
			var = g_list_append (var, g_strdup (k));
			k = line + i + 1;
		}
	}
	g_free (line);

	return var;
}

static gpointer
varibale_list_children (DebuggerJs* object, struct Task *task)
{
	size_t i, k, j, size;
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);
	GList *ret = NULL;
	IAnjutaDebuggerVariableObject *var = NULL;
	gchar *str = debugger_server_get_line (priv->server);

	for (i = 0, k = 0, j = 0, size = strlen (str); i < size; i++)
	{
		if (str[i] == '{')
		{
			k = i;
			j = 0;
		}
		if (str[i] == ',')
		{
			str[i] = '\0';
			if (j == 0)
			{
				var = g_new (IAnjutaDebuggerVariableObject, 1);
				var->type = g_strdup (str + k + 1);
				var->value = g_strdup ("");
				var->children = 1;
				k = i;
			}
			else if (j == 1)
			{
				var->expression = g_strconcat (task->this_data.VareableListChildren.name, ".", str + k + 2, NULL);
				var->name = g_strconcat (task->this_data.VareableListChildren.name, ".", str + k + 2, NULL);
			}
			j++;
			if (j == 2)
			{
				int z;
				ret = g_list_append (ret, var);
				for (z = 1; z && i< size; i++)
				{
					if (str[i] == '{')
						z++;
					if (str[i] == '}')
						z--;
				}
				j = 0;
				var = NULL;
			}
		}
	}
	g_assert (var == NULL);

	g_free (str);
	return ret;
}

static void
on_data_arrived (DebuggerServer *server, gpointer user_data)
{
	DebuggerJs* object = DEBUGGER_JS (user_data);
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);

	if (priv->task_queue != NULL)
	{
		struct Task *task = (struct Task*)priv->task_queue->data;

		g_assert (task);

		while (task->line_required <= debugger_server_get_line_col (priv->server))
		{
			gpointer result;
			switch (task->task_type)
			{
				case SIGNAL:
					task->callback (NULL, task->user_data, NULL);
					break;
				case VARIABLE_CREATE:
					result = variable_create (object, task);
					task->callback (result, task->user_data, NULL);
					break;
				case INFO_THREAD:
					result = info_thread (object, task);
					task->callback (result, task->user_data, NULL);
					break;
				case LIST_THREAD:
					result = list_thread (object, task);
					task->callback (result, task->user_data, NULL);
					break;
				case LIST_FRAME:
					result = list_frame (object, task);
					task->callback (result, task->user_data, NULL);
					break;
				case LIST_LOCAL:
					result = list_local (object, task);
					task->callback (result, task->user_data, NULL);
					break;
				case BREAKPOINT_LIST:
					task->callback (priv->breakpoint, task->user_data, NULL);//TODO: Return Copy
					break;
				case VARIABLE_LIST_CHILDREN:
					result = varibale_list_children (object, task);
					task->callback (result, task->user_data, NULL);
					break;
				default:
					printf ("%d\n", task->task_type);
					g_assert_not_reached ();
					break;
			}
			priv->busy = FALSE;
			g_signal_emit_by_name (priv->data, "debugger-ready", debugger_js_get_state (object));

			priv->task_queue = g_list_delete_link (priv->task_queue, priv->task_queue);
			if (!priv->task_queue)
				break;
			task = (struct Task*)priv->task_queue->data;
		}
	}
	if (priv->task_queue == NULL && debugger_server_get_line_col (priv->server) > 0)
	{
		gint lineno;
		gchar *line = debugger_server_get_line (server);
		gchar *file;

		g_assert (line);
		g_assert (strlen (line) != 0);

		priv->dataRecived = TRUE;
		file = (gchar*)g_malloc (strlen (line));

		if (priv->current_source_file)
			g_free (priv->current_source_file);

		if (sscanf (line, "Line #%d File:%s\n", &lineno, file) == 2) //TODO: Correct recive filename
		{
			priv->current_line = lineno;
			priv->current_source_file = file;
			if (priv->started)
				g_signal_emit_by_name (priv->data, "program-moved", 0, 0, 0, file, lineno);
		}
		else
			g_signal_emit_by_name (object, "DebuggerError", "Invalid data arrived", G_TYPE_NONE);

		g_free (line);
	}
}

static void
on_error (DebuggerServer *server, const gchar * error, gpointer user_data)
{
	DebuggerJs* object = DEBUGGER_JS (user_data);
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);

	g_assert (error != NULL);

	g_signal_emit_by_name (priv->data, "debugger-ready", IANJUTA_DEBUGGER_STOPPED);
	priv->exited = TRUE;
	priv->started = TRUE;
	priv->busy = FALSE;

	g_signal_emit_by_name (object, "DebuggerError", error, G_TYPE_NONE);
}

DebuggerJs*
debugger_js_new (int port, const gchar* filename, IAnjutaDebugger *data)
{
	DebuggerJs* object = g_object_new (DEBUGGER_TYPE_JS, NULL);
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);

	g_assert (data != NULL);
	g_assert (filename != NULL);

	priv->data = data;
	priv->terminal = anjuta_shell_get_interface (ANJUTA_PLUGIN (data)->shell, IAnjutaTerminal, NULL);
	if (priv->terminal)
	{
		g_warning ("Terminal plugin does not installed.");
	}
	priv->server = debugger_server_new (port);

	if (priv->server == NULL)
		g_error ("Can not create server."); //TODO:FIX THIS
	g_signal_connect (priv->server, "data-arrived", G_CALLBACK (on_data_arrived), object);
	g_signal_connect (priv->server, "error", G_CALLBACK (on_error), object);

	priv->filename = g_strdup (filename);
	g_signal_emit_by_name (data, "debugger-started");

	return object;
}

IAnjutaDebuggerState
debugger_js_get_state (DebuggerJs *object)
{
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);

	if (priv->busy)
		return IANJUTA_DEBUGGER_BUSY;
	if (!priv->started)
		return IANJUTA_DEBUGGER_PROGRAM_LOADED;
	if (priv->exited)
		return IANJUTA_DEBUGGER_STOPPED;
	if (debugger_server_get_line_col (priv->server) || priv->dataRecived)
		return IANJUTA_DEBUGGER_PROGRAM_STOPPED;

	return IANJUTA_DEBUGGER_PROGRAM_RUNNING;
}

void
debugger_js_set_work_dir (DebuggerJs *object, const gchar* work_dir)
{
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);

	g_assert (work_dir != NULL);

	if (priv->working_directory)
		g_free (priv->working_directory);
	priv->working_directory = g_strdup (work_dir);
}

void
debugger_js_start_remote (DebuggerJs *object)
{
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);

	g_signal_emit_by_name (priv->data, "program-running");
	priv->started = TRUE;
}

static void
on_child_exited (IAnjutaTerminal *obj, gint pid,  gint status, gpointer user_data)
{
	DebuggerJs* object = DEBUGGER_JS (user_data);
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);

	g_assert (priv != NULL);

	debugger_server_stop (priv->server);
	priv->busy = FALSE;
	priv->started = TRUE;
	priv->exited = TRUE;
	kill (priv->pid, SIGKILL);
	g_signal_emit_by_name (priv->data, "debugger-ready", IANJUTA_DEBUGGER_STOPPED);
}

void
debugger_js_start (DebuggerJs *object, const gchar *arguments)
{
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);

	gchar *str = g_strconcat (priv->filename, " --debug 127.0.0.1 ", arguments, NULL);

	g_assert (priv->terminal != NULL);

	g_signal_emit_by_name (priv->data, "program-running");
	g_signal_connect (G_OBJECT (priv->terminal), "child-exited",
						  G_CALLBACK (on_child_exited), object);
	priv->pid = ianjuta_terminal_execute_command (priv->terminal, priv->working_directory,  str,  NULL, NULL);

	if (!priv->pid)
		g_signal_emit_by_name (object, "DebuggerError", "Cannot start programm", G_TYPE_NONE);
	priv->started = TRUE;

	g_free (str);
}

void
debugger_js_continue (DebuggerJs *object)
{
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);

	priv->dataRecived = FALSE;
	debugger_server_send_line (priv->server, "continue");
}

void
debugger_js_stepin (DebuggerJs *object)
{
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);

	priv->dataRecived = FALSE;
	debugger_server_send_line (priv->server, "stepin");
}

void
debugger_js_stepover (DebuggerJs *object)
{
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);

	priv->dataRecived = FALSE;
	debugger_server_send_line (priv->server, "stepover");
}

void
debugger_js_stepout (DebuggerJs *object)
{
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);

	priv->dataRecived = FALSE;
	debugger_server_send_line (priv->server, "stepout");
}

void
debugger_js_stop (DebuggerJs *object)
{
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);

	debugger_server_stop (priv->server);
	priv->exited = TRUE;
	kill (priv->pid, SIGKILL);
	g_signal_emit_by_name (priv->data, "debugger-ready", IANJUTA_DEBUGGER_STOPPED);
}

void
debugger_js_add_breakpoint (DebuggerJs *object, const gchar* file, guint line)
{
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);
	gchar *str;

	g_assert (file != NULL);

	IAnjutaDebuggerBreakpointItem* bp = g_new (IAnjutaDebuggerBreakpointItem, 1);
	bp->type = IANJUTA_DEBUGGER_BREAKPOINT_ON_LINE;
	bp->enable = TRUE;
	bp->line = line;
	bp->times = 0;
	bp->file = g_strdup (file);
	debugger_server_send_line (priv->server, "add");
	bp->id = priv->BID++;

	str = g_strdup_printf ("%d %s", line, bp->file);
	debugger_server_send_line (priv->server, str);
	g_free (str);

	priv->breakpoint = g_list_append (priv->breakpoint, bp);
}

void
debugger_js_breakpoint_list (DebuggerJs *object, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);

	g_assert (callback);

	task_added (object);
	struct Task *task = g_new (struct Task, 1);
	task->user_data = user_data;
	task->callback = callback;
	task->line_required = 0;
	task->task_type = BREAKPOINT_LIST;

	priv->task_queue = g_list_append (priv->task_queue, task);
}

void
debugger_js_signal (DebuggerJs *object, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);

	g_assert (callback);

	task_added (object);
	struct Task *task = g_new (struct Task, 1);
	task->user_data = user_data;
	task->callback = callback;
	task->line_required = 0;
	task->task_type = SIGNAL;

	priv->task_queue = g_list_append (priv->task_queue, task);
}

void
debugger_js_variable_list_children (DebuggerJs *object, IAnjutaDebuggerCallback callback, const gchar *name, gpointer user_data)
{
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);

	g_assert (name != NULL);

	task_added (object);
	struct Task *task = g_new (struct Task, 1);
	task->user_data = user_data;
	task->callback = callback;
	task->line_required = 1;
	task->task_type = VARIABLE_LIST_CHILDREN;
	task->this_data.VareableListChildren.name = g_strdup (name);

	debugger_server_send_line (priv->server, "eval");
	debugger_server_send_line (priv->server, name);

	priv->task_queue = g_list_append (priv->task_queue, task);
}

void
debugger_js_list_local (DebuggerJs *object, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);

	g_assert (callback);

	task_added (object);
	struct Task *task = g_new (struct Task, 1);
	task->user_data = user_data;
	task->callback = callback;
	task->line_required = 1;
	task->task_type = LIST_LOCAL;

	debugger_server_send_line (priv->server, "list");

	priv->task_queue = g_list_append (priv->task_queue, task);
}

void
debugger_js_list_thread (DebuggerJs *object, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);

	g_assert (callback);

	task_added (object);
	struct Task *task = g_new (struct Task, 1);
	task->user_data = user_data;
	task->callback = callback;
	task->line_required = 0;
	task->task_type = LIST_THREAD;

	priv->task_queue = g_list_append (priv->task_queue, task);
}

void
debugger_js_list_frame (DebuggerJs *object, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);

	g_assert (callback);

	task_added (object);
	struct Task *task = g_new (struct Task, 1);
	task->user_data = user_data;
	task->callback = callback;
	task->line_required = 1;
	task->task_type = LIST_FRAME;

	debugger_server_send_line (priv->server, "stacktrace");

	priv->task_queue = g_list_append (priv->task_queue, task);
}

void
debugger_js_info_thread (DebuggerJs *object, IAnjutaDebuggerCallback callback, gint thread, gpointer user_data)
{
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);

	g_assert (callback);

	task_added (object);
	struct Task *task = g_new (struct Task, 1);
	task->user_data = user_data;
	task->callback = callback;
	task->line_required = 0;
	task->task_type = INFO_THREAD;

	priv->task_queue = g_list_append (priv->task_queue, task);
}

void
debugger_js_variable_create (DebuggerJs *object, IAnjutaDebuggerCallback callback, const gchar *name, gpointer user_data)
{
	DebuggerJsPrivate *priv = DEBUGGER_JS_PRIVATE(object);

	g_assert (callback);
	g_assert (name);
	g_assert (strlen (name) >= 1);

	task_added (object);
	struct Task *task = g_new (struct Task, 1);
	task->user_data = user_data;
	task->callback = callback;
	task->line_required = 1;
	task->name = g_strdup (name);
	task->task_type = VARIABLE_CREATE;

	debugger_server_send_line (priv->server, "eval");
	debugger_server_send_line (priv->server, name);

	priv->task_queue = g_list_append (priv->task_queue, task);
}

