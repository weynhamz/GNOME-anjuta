/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    command.c
    Copyright (C) 2007 Sébastien Granjoux

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

/*
 * Wrap each debugger commands in a command object put in a FIFO queue
 *---------------------------------------------------------------------------*/

#include <config.h>

#include "command.h"

#include "queue.h"

/*#define DEBUG*/
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-breakpoint-debugger.h>
#include <libanjuta/interfaces/ianjuta-cpu-debugger.h>
#include <libanjuta/interfaces/ianjuta-variable-debugger.h>

#include <stdarg.h>

/* Contants defintion
 *---------------------------------------------------------------------------*/

#define STATE_TO_CHANGE	8  /* To convert IAnjutaDebuggerStatus to command go to */
#define STATE_TO_NEED 16   /* To convert IAnjutaDebuggerStatus to command need */

/* Private type
 *---------------------------------------------------------------------------*/

enum
{
	NO_CHANGE = 0,
	COMMAND_MASK = 0xff,
	CHANGE_MASK = 0x3F << STATE_TO_CHANGE,
	STOP_DEBUGGER = 1 << (IANJUTA_DEBUGGER_STOPPED + STATE_TO_CHANGE - 1),
	START_DEBUGGER = 1 << (IANJUTA_DEBUGGER_STARTED + STATE_TO_CHANGE - 1),
	LOAD_PROGRAM = 1 << (IANJUTA_DEBUGGER_PROGRAM_LOADED + STATE_TO_CHANGE - 1),
	STOP_PROGRAM = 1 << (IANJUTA_DEBUGGER_PROGRAM_STOPPED + STATE_TO_CHANGE - 1),
	RUN_PROGRAM = 1 << (IANJUTA_DEBUGGER_PROGRAM_RUNNING + STATE_TO_CHANGE - 1),
	NEED_DEBUGGER_STOPPED = STOP_DEBUGGER << (STATE_TO_NEED - STATE_TO_CHANGE),
	NEED_DEBUGGER_STARTED = START_DEBUGGER << (STATE_TO_NEED - STATE_TO_CHANGE),
	NEED_PROGRAM_LOADED = LOAD_PROGRAM << (STATE_TO_NEED - STATE_TO_CHANGE),
	NEED_PROGRAM_STOPPED = STOP_PROGRAM << (STATE_TO_NEED - STATE_TO_CHANGE),
	NEED_PROGRAM_RUNNING = RUN_PROGRAM << (STATE_TO_NEED - STATE_TO_CHANGE)
};

typedef enum
{
	EMPTY_COMMAND ,
	CALLBACK_COMMAND,
	LOAD_COMMAND,           /* Debugger started */
	ATTACH_COMMAND,
	QUIT_COMMAND,           /* Debugger started - Program stopped */
	ABORT_COMMAND,
	USER_COMMAND,
	INSPECT_MEMORY_COMMAND,
	DISASSEMBLE_COMMAND,
	LIST_REGISTER_COMMAND,
	UNLOAD_COMMAND,         /* Program loaded */
	START_COMMAND,         
	BREAK_LINE_COMMAND,		  /* Program loaded - Program stopped */
	BREAK_FUNCTION_COMMAND,
	BREAK_ADDRESS_COMMAND,
	ENABLE_BREAK_COMMAND,
	IGNORE_BREAK_COMMAND,	/* 0x10 */
	CONDITION_BREAK_COMMAND,
	REMOVE_BREAK_COMMAND,
	INFO_SHAREDLIB_COMMAND,
	INFO_TARGET_COMMAND,
	INFO_PROGRAM_COMMAND,
	INFO_UDOT_COMMAND,
	STEP_IN_COMMAND,   /* Program stopped */
	STEP_OVER_COMMAND,
	STEP_OUT_COMMAND,
	RUN_COMMAND,		
	RUN_TO_COMMAND,
	EXIT_COMMAND,
	HANDLE_SIGNAL_COMMAND,
	LIST_LOCAL_COMMAND,
	LIST_ARG_COMMAND,
	LIST_THREAD_COMMAND,		/* 0x20 */
	SET_THREAD_COMMAND,
	INFO_THREAD_COMMAND,
	INFO_SIGNAL_COMMAND,
	INFO_FRAME_COMMAND,
	INFO_ARGS_COMMAND,
	INFO_VARIABLES_COMMAND,
	SET_FRAME_COMMAND,
	LIST_FRAME_COMMAND,
	UPDATE_REGISTER_COMMAND,
	WRITE_REGISTER_COMMAND,
	EVALUATE_COMMAND,
	INSPECT_COMMAND,
	PRINT_COMMAND,
	CREATE_VARIABLE,
	EVALUATE_VARIABLE,
	LIST_VARIABLE_CHILDREN,	/* 0x30 */
	DELETE_VARIABLE,
	ASSIGN_VARIABLE,		
	UPDATE_VARIABLE,
	INTERRUPT_COMMAND /* Program running */
} DmaDebuggerCommandType;

typedef enum
{
	DMA_CALLBACK_COMMAND =
		CALLBACK_COMMAND |
		NEED_DEBUGGER_STOPPED | NEED_DEBUGGER_STARTED | NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED | NEED_PROGRAM_RUNNING,
	DMA_LOAD_COMMAND =
		LOAD_COMMAND | LOAD_PROGRAM |
		NEED_DEBUGGER_STOPPED | NEED_DEBUGGER_STARTED,
	DMA_ATTACH_COMMAND =
		ATTACH_COMMAND | RUN_PROGRAM |
		NEED_DEBUGGER_STOPPED | NEED_DEBUGGER_STARTED,
	DMA_QUIT_COMMAND =
		QUIT_COMMAND | CANCEL_ALL_COMMAND | STOP_DEBUGGER |
	    ASYNCHRONOUS | NEED_DEBUGGER_STARTED | NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED,
	DMA_ABORT_COMMAND =
		ABORT_COMMAND | CANCEL_ALL_COMMAND | STOP_DEBUGGER |
	    ASYNCHRONOUS | NEED_DEBUGGER_STARTED | NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED | NEED_PROGRAM_RUNNING,
	DMA_USER_COMMAND =
		USER_COMMAND |
	    NEED_DEBUGGER_STARTED | NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED  | NEED_PROGRAM_RUNNING,
	DMA_INSPECT_MEMORY_COMMAND =
		INSPECT_MEMORY_COMMAND |
		NEED_DEBUGGER_STARTED | NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED | NEED_PROGRAM_RUNNING,
	DMA_DISASSEMBLE_COMMAND =
		DISASSEMBLE_COMMAND |
		NEED_DEBUGGER_STARTED | NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED | NEED_PROGRAM_RUNNING,
	DMA_LIST_REGISTER_COMMAND =
		LIST_REGISTER_COMMAND |
		NEED_DEBUGGER_STARTED | NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED | NEED_PROGRAM_RUNNING,
	DMA_UNLOAD_COMMAND =
		UNLOAD_COMMAND | START_DEBUGGER |
		NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED,
	DMA_START_COMMAND =
		START_COMMAND | RUN_PROGRAM |
		NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED,
	DMA_BREAK_LINE_COMMAND =
		BREAK_LINE_COMMAND |
		NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED | NEED_PROGRAM_RUNNING,
	DMA_BREAK_FUNCTION_COMMAND =
		BREAK_FUNCTION_COMMAND |
		NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED | NEED_PROGRAM_RUNNING,
	DMA_BREAK_ADDRESS_COMMAND =
		BREAK_ADDRESS_COMMAND |
		NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED | NEED_PROGRAM_RUNNING,
	DMA_ENABLE_BREAK_COMMAND =
		ENABLE_BREAK_COMMAND |
		NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED,
	DMA_IGNORE_BREAK_COMMAND =
		IGNORE_BREAK_COMMAND |
		NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED,
	DMA_CONDITION_BREAK_COMMAND =
		CONDITION_BREAK_COMMAND |
		NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED,
	DMA_REMOVE_BREAK_COMMAND =
		REMOVE_BREAK_COMMAND |
		NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED,
	DMA_INFO_SHAREDLIB_COMMAND =
		INFO_SHAREDLIB_COMMAND |
		NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED,
	DMA_INFO_TARGET_COMMAND =
		INFO_TARGET_COMMAND |
		NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED,
	DMA_INFO_PROGRAM_COMMAND =
		INFO_PROGRAM_COMMAND |
		NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED,
	DMA_INFO_UDOT_COMMAND =
		INFO_UDOT_COMMAND |
		NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED,
	DMA_STEP_IN_COMMAND =
		STEP_IN_COMMAND | RUN_PROGRAM |
		NEED_PROGRAM_STOPPED,
	DMA_STEP_OVER_COMMAND =
		STEP_OVER_COMMAND | RUN_PROGRAM |
		NEED_PROGRAM_STOPPED,
	DMA_STEP_OUT_COMMAND =
		STEP_OUT_COMMAND | RUN_PROGRAM |
		NEED_PROGRAM_STOPPED,
	DMA_RUN_COMMAND =
		RUN_COMMAND | RUN_PROGRAM |
		NEED_PROGRAM_STOPPED,
	DMA_RUN_TO_COMMAND =
		RUN_TO_COMMAND | RUN_PROGRAM |
		NEED_PROGRAM_STOPPED,
	DMA_EXIT_COMMAND =
		EXIT_COMMAND | LOAD_PROGRAM |
		NEED_PROGRAM_STOPPED,
	DMA_HANDLE_SIGNAL_COMMAND =
		HANDLE_SIGNAL_COMMAND |
		NEED_PROGRAM_STOPPED,
	DMA_LIST_LOCAL_COMMAND =
		LIST_LOCAL_COMMAND |
		NEED_PROGRAM_STOPPED | NEED_PROGRAM_RUNNING,
	DMA_LIST_ARG_COMMAND =
		LIST_ARG_COMMAND |
		NEED_PROGRAM_STOPPED | NEED_PROGRAM_RUNNING,
	DMA_LIST_THREAD_COMMAND =
		LIST_THREAD_COMMAND |
		NEED_PROGRAM_STOPPED | NEED_PROGRAM_RUNNING,
	DMA_SET_THREAD_COMMAND =
		SET_THREAD_COMMAND |
		NEED_PROGRAM_STOPPED,
	DMA_INFO_THREAD_COMMAND =
		INFO_THREAD_COMMAND |
		NEED_PROGRAM_STOPPED,
	DMA_INFO_SIGNAL_COMMAND =
		INFO_SIGNAL_COMMAND |
		NEED_PROGRAM_STOPPED,
	DMA_INFO_FRAME_COMMAND =
		INFO_FRAME_COMMAND |
		NEED_PROGRAM_STOPPED,
	DMA_INFO_ARGS_COMMAND =
		INFO_ARGS_COMMAND |
		NEED_PROGRAM_STOPPED,
	DMA_INFO_VARIABLES_COMMAND =
		INFO_VARIABLES_COMMAND |
		NEED_PROGRAM_STOPPED,
	DMA_SET_FRAME_COMMAND =
		SET_FRAME_COMMAND |
		NEED_PROGRAM_STOPPED,
	DMA_LIST_FRAME_COMMAND =
		LIST_FRAME_COMMAND |
		NEED_PROGRAM_STOPPED | NEED_PROGRAM_RUNNING,
	DMA_UPDATE_REGISTER_COMMAND =
		UPDATE_REGISTER_COMMAND |
		NEED_PROGRAM_STOPPED | NEED_PROGRAM_RUNNING,
	DMA_WRITE_REGISTER_COMMAND =
		WRITE_REGISTER_COMMAND |
		NEED_PROGRAM_STOPPED,
	DMA_EVALUATE_COMMAND =
		EVALUATE_COMMAND |
		NEED_PROGRAM_STOPPED,
	DMA_INSPECT_COMMAND =
		INSPECT_COMMAND |
		NEED_PROGRAM_STOPPED,
	DMA_PRINT_COMMAND =
		PRINT_COMMAND |
		NEED_PROGRAM_STOPPED,
	DMA_CREATE_VARIABLE_COMMAND =
	   CREATE_VARIABLE |
		NEED_PROGRAM_STOPPED,
	DMA_EVALUATE_VARIABLE_COMMAND =
	    EVALUATE_VARIABLE | CANCEL_IF_PROGRAM_RUNNING |
	    NEED_PROGRAM_STOPPED,
	DMA_LIST_VARIABLE_CHILDREN_COMMAND =
	    LIST_VARIABLE_CHILDREN |
		NEED_PROGRAM_STOPPED,
	DMA_DELETE_VARIABLE_COMMAND =
		DELETE_VARIABLE |
		NEED_PROGRAM_STOPPED,
	DMA_ASSIGN_VARIABLE_COMMAND =
		ASSIGN_VARIABLE |
		NEED_PROGRAM_STOPPED,
	DMA_UPDATE_VARIABLE_COMMAND =
	    UPDATE_VARIABLE | CANCEL_IF_PROGRAM_RUNNING |
		NEED_PROGRAM_STOPPED | NEED_PROGRAM_RUNNING,
	DMA_INTERRUPT_COMMAND =
		INTERRUPT_COMMAND | STOP_PROGRAM |
		ASYNCHRONOUS | NEED_PROGRAM_RUNNING,
} DmaDebuggerCommand;

struct _DmaQueueCommand
{
	DmaDebuggerCommandType type;
	IAnjutaDebuggerCallback callback;
	gpointer user_data;
	union {
		struct {
			gchar *file;
			gchar *type;
			GList *dirs;
			gboolean terminal;
		} load;
		struct {
			gchar *args;
		} start;
		struct {
			pid_t pid;
			GList *dirs;
		} attach;
		struct {
			gchar *file;
			guint line;
			guint address;
			gchar *function;
		} pos;
		struct {
			guint id;
			guint ignore;
			gchar *condition;
			gboolean enable;
		} brk;
		struct {
			guint id;
			gchar *name;
			gchar *value;
		} watch;
		struct {
			gchar *cmd;
		} user;
		struct {
			gchar *var;
		} print;
		struct {
			guint id;
		} info;
		struct {
			gchar *name;
			gboolean stop;
			gboolean print;
			gboolean ignore;
		} signal;
		struct {
			guint frame;
		} frame;
		struct {
			guint address;
			guint length;
		} mem;
		struct {
			gchar *name;
			gchar *value;
		} var;
	} data;
	struct _DmaQueueCommand *next;
};

/* Private function
 *---------------------------------------------------------------------------*/

static DmaQueueCommand*
dma_command_new (DmaDebuggerCommand cmd_type,...)
{
	DmaQueueCommand* cmd;
	DmaDebuggerCommandType type = cmd_type & COMMAND_MASK;
	va_list args;
	GList *list;
	
	cmd = g_new0 (DmaQueueCommand, 1);
	cmd->type = cmd_type;
	
	va_start (args, cmd_type);
	switch (type)
	{
	case EMPTY_COMMAND:
		break;
	case CALLBACK_COMMAND:
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case LOAD_COMMAND:
		cmd->data.load.file = g_strdup (va_arg (args, gchar *));
		cmd->data.load.type = g_strdup (va_arg (args, gchar *));
		cmd->data.load.dirs = NULL;
		for (list = va_arg (args, GList *); list != NULL; list = g_list_next (list))
		{
			cmd->data.load.dirs = g_list_prepend (cmd->data.load.dirs, g_strdup (list->data));
		}
		cmd->data.load.dirs = g_list_reverse (cmd->data.load.dirs);
		cmd->data.load.terminal = va_arg (args, gboolean);
		break;
	case ATTACH_COMMAND:
		cmd->data.attach.pid = va_arg (args,pid_t);
		cmd->data.load.dirs = NULL;
		for (list = va_arg (args, GList *); list != NULL; list = g_list_next (list))
		{
			cmd->data.load.dirs = g_list_prepend (cmd->data.load.dirs, g_strdup (list->data));
		}
		cmd->data.load.dirs = g_list_reverse (cmd->data.load.dirs);
		break;
	case UNLOAD_COMMAND:
		break;
	case QUIT_COMMAND:
		break;
	case ABORT_COMMAND:
		break;
	case START_COMMAND:
		cmd->data.start.args = g_strdup (va_arg (args, gchar *));
	    break;
	case RUN_COMMAND:
		break;
	case RUN_TO_COMMAND:
		cmd->data.pos.file = g_strdup (va_arg (args, gchar *));
		cmd->data.pos.line = va_arg (args, guint);
		break;
	case STEP_IN_COMMAND:
		break;
	case STEP_OVER_COMMAND:
		break;
	case STEP_OUT_COMMAND:
		break;
	case EXIT_COMMAND:
		break;
	case INTERRUPT_COMMAND:
		break;
	case ENABLE_BREAK_COMMAND:
		cmd->data.brk.id = va_arg (args, gint);
		cmd->data.brk.enable = va_arg (args, gboolean);
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case IGNORE_BREAK_COMMAND:
		cmd->data.brk.id = va_arg (args, gint);
		cmd->data.brk.ignore = va_arg (args, guint);
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case REMOVE_BREAK_COMMAND:
		cmd->data.brk.id = va_arg (args, gint);
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case BREAK_LINE_COMMAND:
		cmd->data.pos.file = g_strdup (va_arg (args, gchar *));
		cmd->data.pos.line = va_arg (args, guint);
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case BREAK_FUNCTION_COMMAND:
		cmd->data.pos.file = g_strdup (va_arg (args, gchar *));
		cmd->data.pos.function = g_strdup (va_arg (args, gchar *));
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case BREAK_ADDRESS_COMMAND:
		cmd->data.pos.address = va_arg (args, guint);
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case CONDITION_BREAK_COMMAND:
		cmd->data.brk.id = va_arg (args, gint);
		cmd->data.brk.condition = g_strdup (va_arg (args, gchar *));
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case INSPECT_COMMAND:
		cmd->data.watch.name = g_strdup (va_arg (args, gchar *));
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
	    break;
	case EVALUATE_COMMAND:
		cmd->data.watch.name = g_strdup (va_arg (args, gchar *));
		cmd->data.watch.value = g_strdup (va_arg (args, gchar *));
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
	    break;
	case LIST_LOCAL_COMMAND:
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case LIST_ARG_COMMAND:
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case LIST_THREAD_COMMAND:
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case SET_THREAD_COMMAND:
		cmd->data.frame.frame = va_arg (args, guint);
		break;
	case INFO_THREAD_COMMAND:
		cmd->data.info.id = va_arg (args, guint);
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case INFO_SIGNAL_COMMAND:
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case INFO_SHAREDLIB_COMMAND:
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case INFO_FRAME_COMMAND:
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case INFO_ARGS_COMMAND:
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case INFO_TARGET_COMMAND:
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case INFO_PROGRAM_COMMAND:
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case INFO_UDOT_COMMAND:
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case INFO_VARIABLES_COMMAND:
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case SET_FRAME_COMMAND:
		cmd->data.frame.frame = va_arg (args, guint);
		break;
	case LIST_FRAME_COMMAND:
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case LIST_REGISTER_COMMAND:
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case UPDATE_REGISTER_COMMAND:
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case WRITE_REGISTER_COMMAND:
		cmd->data.watch.id = va_arg (args, guint);
	    cmd->data.watch.name = g_strdup (va_arg (args, gchar *));
	    cmd->data.watch.value = g_strdup (va_arg (args, gchar *));
		break;
	case INSPECT_MEMORY_COMMAND:
		cmd->data.mem.address = va_arg (args, guint);
	    cmd->data.mem.length = va_arg (args, guint);
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case DISASSEMBLE_COMMAND:
		cmd->data.mem.address = va_arg (args, guint);
	    cmd->data.mem.length = va_arg (args, guint);
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case USER_COMMAND:
		cmd->data.user.cmd = g_strdup (va_arg (args, gchar *));
		break;
	case PRINT_COMMAND:
		cmd->data.print.var = g_strdup (va_arg (args, gchar *));
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case HANDLE_SIGNAL_COMMAND:
		cmd->data.signal.name = g_strdup (va_arg (args, gchar *));
		cmd->data.signal.stop = va_arg (args, gboolean);
		cmd->data.signal.print = va_arg (args, gboolean);
		cmd->data.signal.ignore = va_arg (args, gboolean);
		break;
	case DELETE_VARIABLE:
		cmd->data.var.name = g_strdup (va_arg (args, gchar *));
		break;
	case ASSIGN_VARIABLE:
		cmd->data.var.name = g_strdup (va_arg (args, gchar *));
		cmd->data.var.value = g_strdup (va_arg (args, gchar *));
		break;
	case EVALUATE_VARIABLE:
		cmd->data.var.name = g_strdup (va_arg (args, gchar *));
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case LIST_VARIABLE_CHILDREN:
		cmd->data.var.name = g_strdup (va_arg (args, gchar *));
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case CREATE_VARIABLE:
		cmd->data.var.name = g_strdup (va_arg (args, gchar *));
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	case UPDATE_VARIABLE:
		cmd->callback = va_arg (args, IAnjutaDebuggerCallback);
		cmd->user_data = va_arg (args, gpointer);
		break;
	}

	return cmd;
}

/* Public function
 *---------------------------------------------------------------------------*/

gboolean
dma_queue_load (DmaDebuggerQueue *self, const gchar *file, const gchar* mime_type, const GList *search_dirs, gboolean terminal)
{
	if (!dma_debugger_queue_start (self, mime_type)) return FALSE;
	
	return dma_debugger_queue_append (self, dma_command_new (DMA_LOAD_COMMAND, file, mime_type, search_dirs, terminal));
}

gboolean
dma_queue_attach (DmaDebuggerQueue *self, pid_t pid, const GList *search_dirs)
{
	if (!dma_debugger_queue_start (self, NULL)) return FALSE;
	
	return dma_debugger_queue_append (self, dma_command_new (DMA_ATTACH_COMMAND, pid, search_dirs));
}

gboolean
dma_queue_start (DmaDebuggerQueue *self, const gchar *args)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_START_COMMAND, args));
}

gboolean
dma_queue_unload (DmaDebuggerQueue *self)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_UNLOAD_COMMAND));
}

gboolean
dma_queue_quit (DmaDebuggerQueue *self)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_QUIT_COMMAND));
}

gboolean
dma_queue_abort (DmaDebuggerQueue *self)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_ABORT_COMMAND));
}

gboolean
dma_queue_run (DmaDebuggerQueue *self)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_RUN_COMMAND));
}

gboolean
dma_queue_step_in (DmaDebuggerQueue *self)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_STEP_IN_COMMAND));
}

gboolean
dma_queue_step_over (DmaDebuggerQueue *self)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_STEP_OVER_COMMAND));
}

gboolean
dma_queue_run_to (DmaDebuggerQueue *self, const gchar *file, gint line)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_RUN_TO_COMMAND, file, line));
}

gboolean
dma_queue_step_out (DmaDebuggerQueue *self)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_STEP_OUT_COMMAND));
}

gboolean
dma_queue_exit (DmaDebuggerQueue *self)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_EXIT_COMMAND));
}

gboolean
dma_queue_interrupt (DmaDebuggerQueue *self)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_INTERRUPT_COMMAND));
}

gboolean
dma_queue_inspect (DmaDebuggerQueue *self, const gchar *expression, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_INSPECT_COMMAND, expression, callback, user_data));
}

gboolean
dma_queue_evaluate (DmaDebuggerQueue *self, const gchar *name, const gchar* value, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_EVALUATE_COMMAND, name, value, callback, user_data));
}

gboolean
dma_queue_send_command (DmaDebuggerQueue *self, const gchar* command)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_USER_COMMAND, command));
}

gboolean
dma_queue_print (DmaDebuggerQueue *self, const gchar* variable, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_PRINT_COMMAND, variable, callback, user_data));
}

gboolean
dma_queue_list_local (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_LIST_LOCAL_COMMAND, callback, user_data));
}

gboolean
dma_queue_list_argument (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_LIST_ARG_COMMAND, callback, user_data));
}

gboolean
dma_queue_list_thread (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_LIST_THREAD_COMMAND, callback, user_data));
}

gboolean
dma_queue_set_thread (DmaDebuggerQueue *self, gint thread)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_SET_THREAD_COMMAND, thread));
}

gboolean
dma_queue_info_thread (DmaDebuggerQueue *self, gint thread, IAnjutaDebuggerCallback callback , gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_INFO_THREAD_COMMAND, thread, callback, user_data));
}

gboolean
dma_queue_info_signal (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_INFO_SIGNAL_COMMAND, callback, user_data));
}

gboolean
dma_queue_info_sharedlib (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_INFO_SHAREDLIB_COMMAND, callback, user_data));
}

gboolean
dma_queue_handle_signal (DmaDebuggerQueue *self, const gchar* name, gboolean stop, gboolean print, gboolean ignore)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_HANDLE_SIGNAL_COMMAND, name, stop, print, ignore));
}

gboolean
dma_queue_info_frame (DmaDebuggerQueue *self, guint frame, IAnjutaDebuggerCallback callback , gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_INFO_FRAME_COMMAND, frame, callback, user_data));
}

gboolean
dma_queue_info_args (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_INFO_ARGS_COMMAND, callback, user_data));
}

gboolean
dma_queue_info_target (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_INFO_TARGET_COMMAND, callback, user_data));
}

gboolean
dma_queue_info_program (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_INFO_PROGRAM_COMMAND, callback, user_data));
}

gboolean
dma_queue_info_udot (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_INFO_UDOT_COMMAND, callback, user_data));
}

gboolean
dma_queue_info_variables (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_INFO_VARIABLES_COMMAND, callback, user_data));
}

gboolean
dma_queue_set_frame (DmaDebuggerQueue *self, guint frame)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_SET_FRAME_COMMAND, frame));
}

gboolean
dma_queue_list_frame (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_LIST_FRAME_COMMAND, callback, user_data));
}

gboolean
dma_queue_callback (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_CALLBACK_COMMAND, callback, user_data));
}

gboolean
dma_queue_add_breakpoint_at_line (DmaDebuggerQueue *self, const gchar* file, guint line, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_BREAK_LINE_COMMAND, file, line, callback, user_data));
}

gboolean
dma_queue_add_breakpoint_at_function (DmaDebuggerQueue *self, const gchar* file, const gchar* function, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_BREAK_FUNCTION_COMMAND, file, function, callback, user_data));
}

gboolean
dma_queue_add_breakpoint_at_address (DmaDebuggerQueue *self, guint address, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_BREAK_ADDRESS_COMMAND, address, callback, user_data));
}

gboolean
dma_queue_enable_breakpoint (DmaDebuggerQueue *self, guint id, gboolean enable, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_ENABLE_BREAK_COMMAND, id, enable, callback, user_data));
}

gboolean
dma_queue_ignore_breakpoint (DmaDebuggerQueue *self, guint id, guint ignore, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_IGNORE_BREAK_COMMAND, id, ignore, callback, user_data));
}

gboolean
dma_queue_condition_breakpoint (DmaDebuggerQueue *self, guint id, const gchar *condition, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_CONDITION_BREAK_COMMAND, id, condition, callback, user_data));
}

gboolean
dma_queue_remove_breakpoint (DmaDebuggerQueue *self, guint id, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_REMOVE_BREAK_COMMAND, id, callback, user_data));
}

gboolean
dma_queue_list_register (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_LIST_REGISTER_COMMAND, callback, user_data));
}

gboolean
dma_queue_update_register (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_UPDATE_REGISTER_COMMAND, callback, user_data));
}

gboolean
dma_queue_write_register (DmaDebuggerQueue *self, IAnjutaDebuggerRegister *value)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_WRITE_REGISTER_COMMAND, value));
}

gboolean
dma_queue_inspect_memory (DmaDebuggerQueue *self, guint address, guint length, IAnjutaDebuggerCallback callback , gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_INSPECT_MEMORY_COMMAND, address, length, callback, user_data));
}

gboolean
dma_queue_disassemble (DmaDebuggerQueue *self, guint address, guint length, IAnjutaDebuggerCallback callback , gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_DISASSEMBLE_COMMAND, address, length, callback, user_data));
}

gboolean
dma_queue_delete_variable (DmaDebuggerQueue *self, const gchar *name)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_DELETE_VARIABLE_COMMAND, name));
}

gboolean
dma_queue_evaluate_variable (DmaDebuggerQueue *self, const gchar *name, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_EVALUATE_VARIABLE_COMMAND, name, callback, user_data));
}

gboolean
dma_queue_assign_variable (DmaDebuggerQueue *self, const gchar *name, const gchar *value)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_ASSIGN_VARIABLE_COMMAND, name, value));
}

gboolean
dma_queue_list_children (DmaDebuggerQueue *self, const gchar *name, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_LIST_VARIABLE_CHILDREN_COMMAND, name, callback, user_data));
}

gboolean
dma_queue_create_variable (DmaDebuggerQueue *self, const gchar *name, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_CREATE_VARIABLE_COMMAND, name, callback, user_data));
}

gboolean
dma_queue_update_variable (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback, gpointer user_data)
{
	return dma_debugger_queue_append (self, dma_command_new (DMA_UPDATE_VARIABLE_COMMAND, callback, user_data));
}

void
dma_command_free (DmaQueueCommand *cmd)
{
	DmaDebuggerCommandType type = cmd->type & COMMAND_MASK;
	
	switch (type)
	{
	case EMPTY_COMMAND:
	case CALLBACK_COMMAND:
	case UNLOAD_COMMAND:
	case QUIT_COMMAND:
	case ABORT_COMMAND:
	case RUN_COMMAND:
	case STEP_IN_COMMAND:
	case STEP_OVER_COMMAND:
	case STEP_OUT_COMMAND:
	case EXIT_COMMAND:
	case INTERRUPT_COMMAND:
	case ENABLE_BREAK_COMMAND:
	case IGNORE_BREAK_COMMAND:
	case REMOVE_BREAK_COMMAND:
	    break;
	case LIST_LOCAL_COMMAND:
	case LIST_ARG_COMMAND:
	case INFO_THREAD_COMMAND:
	case LIST_THREAD_COMMAND:
	case SET_THREAD_COMMAND:
	case INFO_SIGNAL_COMMAND:
	case INFO_SHAREDLIB_COMMAND:
	case INFO_FRAME_COMMAND:
	case INFO_ARGS_COMMAND:
	case INFO_TARGET_COMMAND:
	case INFO_PROGRAM_COMMAND:
	case INFO_UDOT_COMMAND:
	case INFO_VARIABLES_COMMAND:
	case LIST_REGISTER_COMMAND:
	case UPDATE_REGISTER_COMMAND:
	    break;
	case SET_FRAME_COMMAND:
	case LIST_FRAME_COMMAND:
	case INSPECT_MEMORY_COMMAND:
	case DISASSEMBLE_COMMAND:
		break;
	case INSPECT_COMMAND:
	case EVALUATE_COMMAND:
	case WRITE_REGISTER_COMMAND:
		if (cmd->data.watch.name != NULL) g_free (cmd->data.watch.name);
		if (cmd->data.watch.value != NULL) g_free (cmd->data.watch.value);
		break;
	case START_COMMAND:
		if (cmd->data.start.args) g_free (cmd->data.start.args);
		break;
	case LOAD_COMMAND:
		if (cmd->data.load.file) g_free (cmd->data.load.file);
		if (cmd->data.load.type) g_free (cmd->data.load.type);
        g_list_foreach (cmd->data.load.dirs, (GFunc)g_free, NULL);
        g_list_free (cmd->data.load.dirs);
		break;
	case ATTACH_COMMAND:
        g_list_foreach (cmd->data.attach.dirs, (GFunc)g_free, NULL);
        g_list_free (cmd->data.attach.dirs);
		break;
	case RUN_TO_COMMAND:
	case BREAK_LINE_COMMAND:
	case BREAK_FUNCTION_COMMAND:
	case BREAK_ADDRESS_COMMAND:
		if (cmd->data.pos.file) g_free (cmd->data.pos.file);
		if (cmd->data.pos.function) g_free (cmd->data.pos.function);
		break;
	case CONDITION_BREAK_COMMAND:
		if (cmd->data.brk.condition) g_free (cmd->data.brk.condition);
		break;
	case USER_COMMAND:
		if (cmd->data.user.cmd) g_free (cmd->data.user.cmd);
		break;
	case PRINT_COMMAND:
		if (cmd->data.print.var) g_free (cmd->data.print.var);
		break;
	case HANDLE_SIGNAL_COMMAND:
		if (cmd->data.signal.name) g_free (cmd->data.signal.name);
		break;
	case DELETE_VARIABLE:
	case ASSIGN_VARIABLE:
	case CREATE_VARIABLE:
	case EVALUATE_VARIABLE:
	case LIST_VARIABLE_CHILDREN:
	case UPDATE_VARIABLE:
		if (cmd->data.var.name) g_free (cmd->data.var.name);
		break;
    }
	
	g_free (cmd);
}


void
dma_command_cancel (DmaQueueCommand *cmd)
{
	GError *err = g_error_new_literal (IANJUTA_DEBUGGER_ERROR , IANJUTA_DEBUGGER_CANCEL, "Command cancel");

	if (cmd->callback != NULL)
	{
		cmd->callback (NULL, cmd->user_data, err);
	}
	
	g_error_free (err);

	g_warning ("Cancel command %x\n", cmd->type);
	
	dma_command_free (cmd);
}

gboolean
dma_command_run (DmaQueueCommand *cmd, IAnjutaDebugger *debugger,
				 GError **err)
{
	IAnjutaDebuggerRegister reg;
	gboolean ret = FALSE;
	DmaDebuggerCommandType type = cmd->type & COMMAND_MASK;
	
	switch (type)
	{
	case EMPTY_COMMAND:
		break;
	case CALLBACK_COMMAND:
		ret = ianjuta_debugger_callback (debugger, cmd->callback, cmd->user_data, err);	
		break;
	case LOAD_COMMAND:
		ret = ianjuta_debugger_load (debugger, cmd->data.load.file, cmd->data.load.type, cmd->data.load.dirs, cmd->data.load.terminal, err);
		break;
	case ATTACH_COMMAND:
		ret = ianjuta_debugger_attach (debugger, cmd->data.attach.pid, cmd->data.load.dirs, err);	
		break;
	case UNLOAD_COMMAND:
	    ret = ianjuta_debugger_unload (debugger, err);
	    break;
	case QUIT_COMMAND:
		ret = ianjuta_debugger_quit (debugger, err);
		break;
	case ABORT_COMMAND:
		ret = ianjuta_debugger_abort (debugger, err);
		break;
	case START_COMMAND:
		ret = ianjuta_debugger_start (debugger, cmd->data.start.args, err);
	    break;
	case RUN_COMMAND:
		ret = ianjuta_debugger_run (debugger, err);	
		break;
	case RUN_TO_COMMAND:
		ret = ianjuta_debugger_run_to (debugger, cmd->data.pos.file, cmd->data.pos.line, err);	
		break;
	case STEP_IN_COMMAND:
		ret = ianjuta_debugger_step_in (debugger, err);	
		break;
	case STEP_OVER_COMMAND:
		ret = ianjuta_debugger_step_over (debugger, err);	
		break;
	case STEP_OUT_COMMAND:
		ret = ianjuta_debugger_step_out (debugger, err);	
		break;
	case EXIT_COMMAND:
		ret = ianjuta_debugger_exit (debugger, err);	
		break;
	case INTERRUPT_COMMAND:
		ret = ianjuta_debugger_interrupt (debugger, err);	
		break;
	case ENABLE_BREAK_COMMAND:
		ret = ianjuta_breakpoint_debugger_enable_breakpoint (IANJUTA_BREAKPOINT_DEBUGGER (debugger), cmd->data.brk.id, cmd->data.brk.enable, cmd->callback, cmd->user_data, err);	
		break;
	case IGNORE_BREAK_COMMAND:
		ret = ianjuta_breakpoint_debugger_ignore_breakpoint (IANJUTA_BREAKPOINT_DEBUGGER (debugger), cmd->data.brk.id, cmd->data.brk.ignore, cmd->callback, cmd->user_data, err);	
		break;
	case REMOVE_BREAK_COMMAND:
		ret = ianjuta_breakpoint_debugger_clear_breakpoint (IANJUTA_BREAKPOINT_DEBUGGER (debugger), cmd->data.brk.id, cmd->callback, cmd->user_data, err);	
		break;
	case BREAK_LINE_COMMAND:
		ret = ianjuta_breakpoint_debugger_set_breakpoint_at_line (IANJUTA_BREAKPOINT_DEBUGGER (debugger), cmd->data.pos.file, cmd->data.pos.line, cmd->callback, cmd->user_data, err);	
		break;
	case BREAK_FUNCTION_COMMAND:
		ret = ianjuta_breakpoint_debugger_set_breakpoint_at_function (IANJUTA_BREAKPOINT_DEBUGGER (debugger), cmd->data.pos.file, cmd->data.pos.function, cmd->callback, cmd->user_data, err);	
		break;
	case BREAK_ADDRESS_COMMAND:
		ret = ianjuta_breakpoint_debugger_set_breakpoint_at_address (IANJUTA_BREAKPOINT_DEBUGGER (debugger), cmd->data.pos.address, cmd->callback, cmd->user_data, err);	
		break;
	case CONDITION_BREAK_COMMAND:
		ret = ianjuta_breakpoint_debugger_condition_breakpoint (IANJUTA_BREAKPOINT_DEBUGGER (debugger), cmd->data.brk.id, cmd->data.brk.condition, cmd->callback, cmd->user_data, err);	
		break;
	case INSPECT_COMMAND:
		ret = ianjuta_debugger_inspect (debugger, cmd->data.watch.name, cmd->callback, cmd->user_data, err);
	    break;
	case EVALUATE_COMMAND:
		ret = ianjuta_debugger_evaluate (debugger, cmd->data.watch.name, cmd->data.watch.value, cmd->callback, cmd->user_data, err);
	    break;
	case LIST_LOCAL_COMMAND:
		ret = ianjuta_debugger_list_local (debugger, cmd->callback, cmd->user_data, err);	
		break;
	case LIST_ARG_COMMAND:
		ret = ianjuta_debugger_list_argument (debugger, cmd->callback, cmd->user_data, err);	
		break;
	case LIST_THREAD_COMMAND:
		ret = ianjuta_debugger_list_thread (debugger, cmd->callback, cmd->user_data, err);	
		break;
	case SET_THREAD_COMMAND:
		ret = ianjuta_debugger_set_thread (debugger, cmd->data.frame.frame, err);	
		break;
	case INFO_THREAD_COMMAND:
		ret = ianjuta_debugger_info_thread (debugger, cmd->data.info.id, cmd->callback, cmd->user_data, err);	
		break;
	case INFO_SIGNAL_COMMAND:
		ret = ianjuta_debugger_info_signal (debugger, cmd->callback, cmd->user_data, err);	
		break;
	case INFO_SHAREDLIB_COMMAND:
		ret = ianjuta_debugger_info_sharedlib (debugger, cmd->callback, cmd->user_data, err);	
		break;
	case INFO_FRAME_COMMAND:
		ret = ianjuta_debugger_info_frame (debugger, 0, cmd->callback, cmd->user_data, err);	
		break;
	case INFO_ARGS_COMMAND:
		ret = ianjuta_debugger_info_args (debugger, cmd->callback, cmd->user_data, err);	
		break;
	case INFO_TARGET_COMMAND:
		ret = ianjuta_debugger_info_target (debugger, cmd->callback, cmd->user_data, err);	
		break;
	case INFO_PROGRAM_COMMAND:
		ret = ianjuta_debugger_info_program (debugger, cmd->callback, cmd->user_data, err);	
		break;
	case INFO_UDOT_COMMAND:
		ret = ianjuta_debugger_info_udot (debugger, cmd->callback, cmd->user_data, err);	
		break;
	case INFO_VARIABLES_COMMAND:
		ret = ianjuta_debugger_info_variables (debugger, cmd->callback, cmd->user_data, err);	
		break;
	case SET_FRAME_COMMAND:
		ret = ianjuta_debugger_set_frame (debugger, cmd->data.frame.frame, err);	
		break;
	case LIST_FRAME_COMMAND:
		ret = ianjuta_debugger_list_frame (debugger, cmd->callback, cmd->user_data, err);	
		break;
	case LIST_REGISTER_COMMAND:
		ret = ianjuta_cpu_debugger_list_register (IANJUTA_CPU_DEBUGGER (debugger), cmd->callback, cmd->user_data, err);	
		break;
	case UPDATE_REGISTER_COMMAND:
		ret = ianjuta_cpu_debugger_update_register (IANJUTA_CPU_DEBUGGER (debugger), cmd->callback, cmd->user_data, err);	
		break;
	case WRITE_REGISTER_COMMAND:
		reg.num = cmd->data.watch.id;
	    reg.name = cmd->data.watch.name;
	    reg.value = cmd->data.watch.value;
		ret = ianjuta_cpu_debugger_write_register (IANJUTA_CPU_DEBUGGER (debugger), &reg, err);	
		break;
	case INSPECT_MEMORY_COMMAND:
		ret = ianjuta_cpu_debugger_inspect_memory (IANJUTA_CPU_DEBUGGER (debugger), cmd->data.mem.address, cmd->data.mem.length, cmd->callback, cmd->user_data, err);	
		break;
	case DISASSEMBLE_COMMAND:
		ret = ianjuta_cpu_debugger_disassemble (IANJUTA_CPU_DEBUGGER (debugger), cmd->data.mem.address, cmd->data.mem.length, cmd->callback, cmd->user_data, err);	
		break;
	case USER_COMMAND:
		ret = ianjuta_debugger_send_command (debugger, cmd->data.user.cmd, err);	
		break;
	case PRINT_COMMAND:
		ret = ianjuta_debugger_print (debugger, cmd->data.print.var, cmd->callback, cmd->user_data, err);	
		break;
	case HANDLE_SIGNAL_COMMAND:
		ret = ianjuta_debugger_handle_signal (debugger, cmd->data.signal.name, cmd->data.signal.stop, cmd->data.signal.print, cmd->data.signal.ignore, err);	
		break;
	case DELETE_VARIABLE:
		ret = ianjuta_variable_debugger_delete_var (IANJUTA_VARIABLE_DEBUGGER (debugger), cmd->data.var.name, NULL);
		break;
	case ASSIGN_VARIABLE:
		ret = ianjuta_variable_debugger_assign (IANJUTA_VARIABLE_DEBUGGER (debugger), cmd->data.var.name, cmd->data.var.value, err);
		break;
	case EVALUATE_VARIABLE:
		ret = ianjuta_variable_debugger_evaluate (IANJUTA_VARIABLE_DEBUGGER (debugger), cmd->data.var.name, cmd->callback, cmd->user_data, err);
		break;
	case LIST_VARIABLE_CHILDREN:
		ret = ianjuta_variable_debugger_list_children (IANJUTA_VARIABLE_DEBUGGER (debugger), cmd->data.var.name, cmd->callback, cmd->user_data, err);
		break;
	case CREATE_VARIABLE:
		ret = ianjuta_variable_debugger_create (IANJUTA_VARIABLE_DEBUGGER (debugger), cmd->data.var.name, cmd->callback, cmd->user_data, err);
		break;
	case UPDATE_VARIABLE:
		ret = ianjuta_variable_debugger_update (IANJUTA_VARIABLE_DEBUGGER (debugger), cmd->callback, cmd->user_data, err);
		break;
	}
	
	return ret;
}

gboolean
dma_command_is_valid_in_state (DmaQueueCommand *cmd, IAnjutaDebuggerState state)
{
	return cmd->type & 1 << (state + STATE_TO_NEED - 1);
}

IAnjutaDebuggerState
dma_command_is_going_to_state (DmaQueueCommand *cmd)
{
	IAnjutaDebuggerState state;
	
	switch (cmd->type & CHANGE_MASK)
	{
	case STOP_DEBUGGER:
		state = IANJUTA_DEBUGGER_STOPPED;
		break;
	case START_DEBUGGER:
		state = IANJUTA_DEBUGGER_STARTED;
		break;
	case LOAD_PROGRAM:
		state = IANJUTA_DEBUGGER_PROGRAM_LOADED;
		break;
	case STOP_PROGRAM:
		state = IANJUTA_DEBUGGER_PROGRAM_STOPPED;
		break;
	case RUN_PROGRAM:
		state = IANJUTA_DEBUGGER_PROGRAM_RUNNING;
		break;
	default:
		state = IANJUTA_DEBUGGER_BUSY;
	}
		
	return state;
}

gboolean
dma_command_has_flag (DmaQueueCommand *cmd, DmaCommandFlag flag)
{
	return cmd->type & flag;
}

int
dma_command_get_type (DmaQueueCommand *cmd)
{
	return (int)cmd->type;
}
