/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    debugger.c
    Copyright (C) 2005 Sébastien Granjoux

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 * Keep all debugger command in a FIFO queue
 * 
 *---------------------------------------------------------------------------*/

#include <config.h>

/*#define DEBUG*/

#include "debugger.h"

#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>
#include <libanjuta/interfaces/ianjuta-variable-debugger.h>

/* Contants defintion
 *---------------------------------------------------------------------------*/

#define ICON_FILE "anjuta-debug-manager.plugin.png"

/* Private type
 *---------------------------------------------------------------------------*/

enum
{
	COMMAND_MASK = 0xff,
	NEED_DEBUGGER_STOPPED = 1 << 8,
	NEED_DEBUGGER_STARTED = 1 << 9,
	NEED_PROGRAM_LOADED = 1 << 10,
	NEED_PROGRAM_STOPPED = 1 << 11,
	NEED_PROGRAM_RUNNING = 1 << 12,

	STOP_DEBUGGER = 1 << 18,
	START_DEBUGGER = 1 << 19,
	LOAD_PROGRAM = 1 << 20,
	STOP_PROGRAM = 1 << 21,
	RUN_PROGRAM = 1 << 22,
	
	CHANGE_STATE = 31 << 18,
	
	CANCEL_IF_PROGRAM_RUNNING = 1 << 13,
	CANCEL_ALL_COMMAND = 1 << 15,
};

typedef enum
{
	EMPTY_COMMAND ,
	CALLBACK_COMMAND,
	INITIALIZE_COMMAND,     /* Debugger stopped */
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
	RESTART_COMMAND,        /* Program loaded - Program stopped */
	BREAK_LINE_COMMAND,
	BREAK_FUNCTION_COMMAND,
	BREAK_ADDRESS_COMMAND,
	ENABLE_BREAK_COMMAND,
	IGNORE_BREAK_COMMAND,	 /* 0x10 */
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
	LIST_THREAD_COMMAND,
	SET_THREAD_COMMAND,
	INFO_THREAD_COMMAND,
	INFO_SIGNAL_COMMAND,    /* 0x20 */
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
	LIST_VARIABLE_CHILDREN,
	DELETE_VARIABLE,
	ASSIGN_VARIABLE,		/* 0x30 */
	UPDATE_VARIABLE,
	INTERRUPT_COMMAND /* Program running */
} DmaDebuggerCommandType;

typedef enum
{
	DMA_CALLBACK_COMMAND =
		CALLBACK_COMMAND |
		NEED_DEBUGGER_STOPPED | NEED_DEBUGGER_STARTED | NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED | NEED_PROGRAM_RUNNING,
	DMA_INITIALIZE_COMMAND =
		INITIALIZE_COMMAND | START_DEBUGGER |
		NEED_DEBUGGER_STOPPED,
	DMA_LOAD_COMMAND =
		LOAD_COMMAND | LOAD_PROGRAM |
		NEED_DEBUGGER_STOPPED | NEED_DEBUGGER_STARTED,
	DMA_ATTACH_COMMAND =
		ATTACH_COMMAND | RUN_PROGRAM |
		NEED_DEBUGGER_STOPPED | NEED_DEBUGGER_STARTED,
	DMA_QUIT_COMMAND =
		QUIT_COMMAND | CANCEL_ALL_COMMAND | STOP_DEBUGGER |
	    NEED_DEBUGGER_STARTED | NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED,
	DMA_ABORT_COMMAND =
		ABORT_COMMAND | CANCEL_ALL_COMMAND | STOP_DEBUGGER |
	    NEED_DEBUGGER_STARTED | NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED | NEED_PROGRAM_RUNNING,
	DMA_USER_COMMAND =
		USER_COMMAND |
	    NEED_DEBUGGER_STARTED | NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED,
	DMA_INSPECT_MEMORY_COMMAND =
		INSPECT_MEMORY_COMMAND |
		NEED_DEBUGGER_STARTED | NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED,
	DMA_DISASSEMBLE_COMMAND =
		DISASSEMBLE_COMMAND |
		NEED_DEBUGGER_STARTED | NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED,
	DMA_LIST_REGISTER_COMMAND =
		LIST_REGISTER_COMMAND |
		NEED_DEBUGGER_STARTED | NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED,
	DMA_UNLOAD_COMMAND =
		UNLOAD_COMMAND | START_DEBUGGER |
		NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED,
	DMA_START_COMMAND =
		START_COMMAND | RUN_PROGRAM |
		NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED,
	DMA_RESTART_COMMAND =
		RESTART_COMMAND | RUN_PROGRAM |
		NEED_PROGRAM_STOPPED,
	DMA_BREAK_LINE_COMMAND =
		BREAK_LINE_COMMAND |
		NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED,
	DMA_BREAK_FUNCTION_COMMAND =
		BREAK_FUNCTION_COMMAND |
		NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED,
	DMA_BREAK_ADDRESS_COMMAND =
		BREAK_ADDRESS_COMMAND |
		NEED_PROGRAM_LOADED | NEED_PROGRAM_STOPPED,
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
		NEED_PROGRAM_STOPPED,
	DMA_LIST_ARG_COMMAND =
		LIST_ARG_COMMAND |
		NEED_PROGRAM_STOPPED,
	DMA_LIST_THREAD_COMMAND =
		LIST_THREAD_COMMAND |
		NEED_PROGRAM_STOPPED,
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
		NEED_PROGRAM_STOPPED,
	DMA_UPDATE_REGISTER_COMMAND =
		UPDATE_REGISTER_COMMAND |
		NEED_PROGRAM_STOPPED,
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
		NEED_PROGRAM_STOPPED,
	DMA_INTERRUPT_COMMAND =
		INTERRUPT_COMMAND | STOP_PROGRAM |
		NEED_PROGRAM_RUNNING,
} DmaDebuggerCommand;




enum
{
	DEBUGGER_STOPPED_OR_GREATER = EMPTY_COMMAND,
	DEBUGGER_STOPPED_OR_LESSER = INITIALIZE_COMMAND,
	DEBUGGER_STARTED_OR_GREATER = LOAD_COMMAND,
	DEBUGGER_STARTED_OR_LESSER = ATTACH_COMMAND,
	PROGRAM_LOADED_OR_GREATER = UNLOAD_COMMAND,
	PROGRAM_LOADED_OR_LESSER = ATTACH_COMMAND,
	PROGRAM_STOPPED_OR_GREATER = QUIT_COMMAND,
	PROGRAM_STOPPED_OR_LESSER = ATTACH_COMMAND,
	PROGRAM_RUNNING_OR_GREATER = INTERRUPT_COMMAND,
	PROGRAM_RUNNING_OR_LESSER = INTERRUPT_COMMAND
};

typedef struct _DmaQueueCommand
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
} DmaQueueCommand;

struct _DmaDebuggerQueue {
	GObject parent;

	AnjutaPlugin* plugin;
	IAnjutaDebugger* debugger;
	IAnjutaCpuDebugger* cpu_debugger;
	
	/* Command queue */
	DmaQueueCommand *head;
	DmaQueueCommand *tail;
	DmaDebuggerCommandType last;
	gboolean queue_command;
	
	IAnjutaDebuggerStatus debugger_status;
	IAnjutaDebuggerStatus queue_status;
	IAnjutaDebuggerStatus status;
	gboolean ready;
	gboolean stop_on_sharedlib;

	/* View for debugger messages */
	IAnjutaMessageView* log;

	/* User output callback */
	IAnjutaDebuggerOutputCallback output_callback;
	gpointer output_data;	
};

struct _DmaDebuggerQueueClass {
	GObjectClass parent;
 };

/*
 * Functions declaration
 *---------------------------------------------------------------------------*/

gboolean
dma_queue_check_status (DmaDebuggerQueue *self, DmaDebuggerCommandType type, GError **err);

void
dma_queue_update_debugger_status (DmaDebuggerQueue *self, IAnjutaDebuggerStatus state, gint status);

/* Call backs
 *---------------------------------------------------------------------------*/

static void
on_debugger_output (IAnjutaDebuggerOutputType type, const gchar *message, gpointer user_data)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (user_data);

	/* Call user call back if exist */
	if (self->output_callback != NULL)
	{
		self->output_callback (type, message, self->output_data);
	}
	else
	{
		/* Default handling of debugger message */

		switch (type)
	    {
		case IANJUTA_DEBUGGER_OUTPUT:
			dma_debugger_message (self, message);
	        break;
		case IANJUTA_DEBUGGER_INFO_OUTPUT:
			dma_debugger_info (self, message);
	        break;
		case IANJUTA_DEBUGGER_WARNING_OUTPUT:
			dma_debugger_warning (self, message);
	        break;
		case IANJUTA_DEBUGGER_ERROR_OUTPUT:
			dma_debugger_error (self, message);
	        break;
    	}
    }
}

/* Queue function
 *---------------------------------------------------------------------------*/

static void
dma_debugger_command_free (DmaQueueCommand *cmd)
{
	switch (cmd->type & COMMAND_MASK)
	{
	case EMPTY_COMMAND:
	case CALLBACK_COMMAND:
	case INITIALIZE_COMMAND:
	case UNLOAD_COMMAND:
	case QUIT_COMMAND:
	case ABORT_COMMAND:
	case RUN_COMMAND:
	case STEP_IN_COMMAND:
	case STEP_OVER_COMMAND:
	case STEP_OUT_COMMAND:
	case RESTART_COMMAND:
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
}

static void
dma_debugger_command_cancel (DmaQueueCommand *cmd)
{
	GError *err = g_error_new_literal (IANJUTA_DEBUGGER_ERROR , IANJUTA_DEBUGGER_CANCEL, "Command cancel");
	
	if (cmd->callback != NULL)
	{
		cmd->callback (NULL, cmd->user_data, err);
	}
	g_warning ("Cancel command %x\n", cmd->type);
	
	g_error_free (err);
	dma_debugger_command_free (cmd);
}

static void
dma_queue_cancel (DmaDebuggerQueue *self, guint flag)
{
	DmaQueueCommand* cmd;
	DmaQueueCommand* prev = self->head;

	/* Cancel all commands after head */
	for (cmd = prev->next; cmd != NULL;)
	{
		if (cmd->type & flag)
		{
			DmaQueueCommand* tmp = cmd;
			
			cmd = cmd->next;
			dma_debugger_command_cancel (tmp);
			prev->next = cmd;
		}
		else
		{
			prev = cmd;
			cmd = cmd->next;
		}
	}

	self->tail = prev;
}

/* Cancel all commands those cannot handle this unexpected state
 * Return TRUE if the state of the queue need to be changed too
 */

static gboolean
dma_queue_cancel_unexpected (DmaDebuggerQueue *self, guint state)
{
	DmaQueueCommand* cmd;
	DmaQueueCommand* prev;

	/* Check if queue is empty */
	if (self->head)
	{
		if (self->ready == FALSE)
		{
			/* Current command is still in queue, keep it */
			prev = self->head;
			cmd = prev->next;
		
			if (prev->type & CHANGE_STATE)
			{	
				return FALSE;
			}
		}
		else
		{	
			/* First command hasn't been send yet, can be cancelled */
			prev = NULL;
			cmd = self->head;
		}

		for (; cmd != NULL;)
		{
			if (!(cmd->type & state))
			{
				/* Command is not allowed in this state, cancel it */
				DmaQueueCommand* tmp = cmd;
			
				cmd = cmd->next;
				dma_debugger_command_cancel (tmp);
				if (prev == NULL)
				{
					self->head = cmd;
				}
				else
				{
					prev->next = cmd;
				}
			}
			else if (cmd->type & CHANGE_STATE)
			{
				/* A command setting the state is kept,
			   	debugger state is known again afterward, queue state is kept too */
			
				return FALSE;
			}
			else
			{
				/* Command is allowed even in this unexpected state, keep it */
				prev = cmd;
				cmd = cmd->next;
			}
		}
		/* Update end of queue if necessary, queue state need to be changed */
		self->tail = prev;
	}


	return TRUE;
}

static void
dma_debugger_queue_remove (DmaDebuggerQueue *self)
{
	DmaQueueCommand* cmd;
	
	if (self->head != NULL)
	{
		cmd = self->head->next;
		dma_debugger_command_free (self->head);
		g_free (self->head);
		self->head = cmd;
		if (cmd == NULL)
		{
			self->tail = NULL;
			/* Queue is empty so has the same status than debugger */
			self->queue_status = self->debugger_status;
		}
	}
}

static void
dma_debugger_end_command (DmaDebuggerQueue *self)
{
	if (self->ready == FALSE)
	{
		dma_debugger_queue_remove (self);
		self->ready = TRUE;
	}
}

#if 0
/* Avoid passing a NULL argument to ianjuter_debugger_initialize */
static void
null_function(void)
{
	return;
}
#endif

gboolean
dma_queue_check_status (DmaDebuggerQueue *self, DmaDebuggerCommandType type, GError **err)
{
	for (;;)
	{
		switch (self->queue_command ? self->queue_status : self->debugger_status)
  	    {
			case IANJUTA_DEBUGGER_BUSY:
				/* Only the debugger can be busy */
				g_return_val_if_reached (FALSE);
			case IANJUTA_DEBUGGER_STOPPED:
				if (type & NEED_DEBUGGER_STOPPED)
			    {
					/* State is right */
					return TRUE;
				}
			    else if (type & STOP_DEBUGGER)
			    {
					/* Already stopped */
					g_warning ("Cancel command %x, already stop\n", type);
					return FALSE;
				}
			    else
			    {
					/* Other command need a program loaded, cannot be done automatically */
					g_warning ("Cancel command %x, debugger stopped\n", type);
					return FALSE;
				}
			case IANJUTA_DEBUGGER_STARTED:
				if (type & NEED_DEBUGGER_STARTED)
			    {
					/* State is right */
					return TRUE;
				}
			    else if (type & START_DEBUGGER)
			    {
					/* Already started */
					g_warning ("Cancel command %x, already started\n", type);
					return FALSE;
				}
			    else
			    {
					/* Other command need a program loaded, cannot be done automatically */
					g_warning ("Cancel command %x, debugger started\n", type);
					return FALSE;
				}
			case IANJUTA_DEBUGGER_PROGRAM_LOADED:
				if (type & NEED_PROGRAM_LOADED)
			    {
					/* State is right */
					return TRUE;
				}
			    else
			    {
					/* Other command need a program running, cannot be done automatically */
					g_warning ("Cancel command %x, program loaded\n", type);
					return FALSE;
				}
			case IANJUTA_DEBUGGER_PROGRAM_STOPPED:
				if (type & NEED_PROGRAM_STOPPED)
			    {
					/* State is right */
					return TRUE;
				}
			    else
			    {
					/* Other command need a program running, cannot be done automatically */
					g_warning ("Cancel command %x, program stopped\n", type);
					return FALSE;
				}
			case IANJUTA_DEBUGGER_PROGRAM_RUNNING:
				if (type & NEED_PROGRAM_RUNNING)
			    {
					/* State is right */
					return TRUE;
				}
			    else
			    {
					/* Other command need a program stopped, cannot be done automatically */
					g_warning ("Cancel command %x, program running\n", type);
					return TRUE;
				}
		}
	}
}

		
static void
dma_queue_update_queue_status (DmaDebuggerQueue *self, DmaDebuggerCommandType type)
{
	if (type & STOP_DEBUGGER)
	{
		self->queue_status = IANJUTA_DEBUGGER_STOPPED;
	}
	else if (type & START_DEBUGGER)
	{
		self->queue_status = IANJUTA_DEBUGGER_STARTED;
	}
	else if (type & LOAD_PROGRAM)
	{
		self->queue_status = IANJUTA_DEBUGGER_PROGRAM_LOADED;
	}
	else if (type & STOP_PROGRAM)
	{
		self->queue_status = IANJUTA_DEBUGGER_PROGRAM_STOPPED;
	}
	else if (type & RUN_PROGRAM)
	{
		self->queue_status = IANJUTA_DEBUGGER_PROGRAM_RUNNING;
	    dma_queue_cancel (self, CANCEL_IF_PROGRAM_RUNNING);
	}
}

static void
dma_queue_emit_debugger_ready (DmaDebuggerQueue *queue)
{
	IAnjutaDebuggerStatus stat;
	static gboolean busy = FALSE;
	
	if ((queue->head == NULL) && (queue->ready))
	{
		stat = queue->queue_status;
		busy = FALSE;
	}
	else if (busy)
	{
		/* Busy signal already emitted, do nothing */
		return;
	}
	else
	{
		stat = IANJUTA_DEBUGGER_BUSY;
		busy = TRUE;
	}
	
	g_signal_emit_by_name (queue, "debugger-ready", stat);
}

void
dma_queue_update_debugger_status (DmaDebuggerQueue *self, IAnjutaDebuggerStatus state, gint status)
{
	const char* signal = NULL;

	self->queue_command = FALSE;
	switch (state)
	{
	case IANJUTA_DEBUGGER_BUSY:
		/* Debugger is busy, nothing to do */
		self->queue_command = TRUE;
		return;
	case IANJUTA_DEBUGGER_STOPPED:
		if (self->debugger_status != IANJUTA_DEBUGGER_STOPPED) 
		{
			if (!(self->last & STOP_DEBUGGER))
			{
				if (dma_queue_cancel_unexpected (self, NEED_DEBUGGER_STOPPED))
				{
					self->queue_status = IANJUTA_DEBUGGER_STOPPED;
				}
			}
			self->debugger_status = IANJUTA_DEBUGGER_STOPPED;
			signal = "debugger-stopped";
		}
	    break;
	case IANJUTA_DEBUGGER_STARTED:
		if (self->debugger_status != IANJUTA_DEBUGGER_STARTED) 
		{
			if (!(self->last & START_DEBUGGER))
			{
				if (dma_queue_cancel_unexpected (self, NEED_DEBUGGER_STARTED))
				{
					self->queue_status = IANJUTA_DEBUGGER_STARTED;
				}
			}
			self->debugger_status = IANJUTA_DEBUGGER_STARTED;
			signal = "debugger-started";
		}
	    break;
	case IANJUTA_DEBUGGER_PROGRAM_LOADED:
		if (self->debugger_status != IANJUTA_DEBUGGER_PROGRAM_LOADED)
		{
			if (!(self->last & LOAD_PROGRAM))
			{
				if (dma_queue_cancel_unexpected (self, NEED_PROGRAM_LOADED))
				{
					self->queue_status = IANJUTA_DEBUGGER_PROGRAM_LOADED;
				}
			}
			if (self->debugger_status == IANJUTA_DEBUGGER_STOPPED)
			{
				dma_debugger_end_command (self);
				self->debugger_status = IANJUTA_DEBUGGER_STARTED;
				g_signal_emit_by_name (self, "debugger-started");
			}
			self->debugger_status = IANJUTA_DEBUGGER_PROGRAM_LOADED;
			self->stop_on_sharedlib = FALSE;
			signal = "program-loaded";
		}
	    break;
	case IANJUTA_DEBUGGER_PROGRAM_STOPPED:
		if (self->debugger_status != IANJUTA_DEBUGGER_PROGRAM_STOPPED) 
		{
			if (!(self->last & STOP_PROGRAM))
			{
				if (dma_queue_cancel_unexpected (self, NEED_PROGRAM_STOPPED))
				{
					self->queue_status = IANJUTA_DEBUGGER_PROGRAM_STOPPED;
				}
			}
			self->debugger_status = IANJUTA_DEBUGGER_PROGRAM_STOPPED;
			if (!self->stop_on_sharedlib)
			{
				signal = "program-stopped";
			}
		}
	    break;
	case IANJUTA_DEBUGGER_PROGRAM_RUNNING:
		if (self->debugger_status != IANJUTA_DEBUGGER_PROGRAM_RUNNING) 
		{
			if (!(self->last & RUN_PROGRAM))
			{
				if (dma_queue_cancel_unexpected (self, NEED_PROGRAM_RUNNING))
				{
					self->queue_status = IANJUTA_DEBUGGER_PROGRAM_RUNNING;
				}
			}
			self->debugger_status = IANJUTA_DEBUGGER_PROGRAM_RUNNING;
			self->stop_on_sharedlib = FALSE;
			signal =  "program-running";
		}
	    break;
	}

	/* Remove current command */
	dma_debugger_end_command (self);
	
	/* Emit signal */
	if (signal != NULL)
	{
		g_signal_emit_by_name (self, signal, status);
	}
	self->queue_command = TRUE;
}

static void
dma_debugger_queue_clear (DmaDebuggerQueue *self)
{
	DmaQueueCommand* cmd;

	for (cmd = self->head; cmd != NULL; )
	{
		cmd = cmd->next;
		dma_debugger_command_free (self->head);
		g_free (self->head);
		self->head = cmd;
	}
	self->head = NULL;
	self->tail = NULL;
	self->queue_command = TRUE;
	
	/* Queue is empty so has the same status than debugger */
	self->queue_status = self->debugger_status;
}

static DmaQueueCommand*
dma_debugger_queue_append (DmaDebuggerQueue *self, DmaDebuggerCommandType type, GError **err)
{
	DmaQueueCommand* cmd = NULL;

	
	if (dma_queue_check_status(self, type, err))
	{
		cmd = g_new0 (DmaQueueCommand, 1);
		cmd->type = type;
		
		if (self->queue_command)
		{
			// Append command at the end (in the queue)

			if (self->head == NULL)
			{
				/* queue is empty */
				self->head = cmd;
			}
			else
			{
				self->tail->next = cmd;
			}
			self->tail = cmd;
		
			dma_queue_update_queue_status (self, type);
		}
		else
		{
			// Prepend command at the beginning
			if (self->ready)
			{
				cmd->next = self->head;
				self->head = cmd;
				if (self->tail == NULL)
				{
					self->tail = cmd;
				}
			}
			else
			{
				cmd->next = self->head->next;
				self->head->next = cmd;
				if (self->tail == self->head)
				{
					self->tail = cmd;
				}
			}				
		}
	}
		
	return cmd;
}

#if 0
static DmaQueueCommand*
dma_debugger_queue_prepend (DmaDebuggerQueue *self)
{
	DmaQueueCommand* cmd;
	
	cmd = g_new0 (DmaQueueCommand, 1);
	cmd->type = EMPTY_COMMAND;
	cmd->next = self->head;
	self->head = cmd;
	
	if (self->head == NULL)
	{
		/* queue is empty */
		self->tail = cmd;
	}
	
	return cmd;
}
#endif

static void
dma_debugger_queue_execute (DmaDebuggerQueue *self)
{
	IAnjutaDebuggerRegister reg;
	GError *err = NULL;
	gboolean ret;

	/* Check if debugger is connected to a debugger backend */
	if (self->debugger == NULL) return;

	/* Check if there debugger is busy */
	if (!self->ready)
	{
		IAnjutaDebuggerStatus status;
		/* Recheck status in case of desynchronization */
		status = ianjuta_debugger_get_status (self->debugger, NULL);
		dma_queue_update_debugger_status (self, status, 0);
	}

	/* Check if there is something to execute */
	while ((self->head != NULL) && (self->ready))
	{
		DmaQueueCommand *cmd;
		
		cmd = self->head;

		DEBUG_PRINT("debugger cmd %x status %d\n", cmd->type, self->debugger_status);
		
		/* Start command */
		self->ready = FALSE;
		self->last = cmd->type;
		switch (cmd->type & COMMAND_MASK)
		{
		case EMPTY_COMMAND:
			break;
		case CALLBACK_COMMAND:
			ianjuta_debugger_callback (self->debugger, cmd->callback, cmd->user_data, &err);	
			break;
		case INITIALIZE_COMMAND:
		    //dma_debugger_create_view (self);
		    self->output_callback = (IAnjutaDebuggerOutputCallback)cmd->callback;
		    self->output_data = cmd->user_data;
	 	    ianjuta_debugger_initialize (self->debugger, on_debugger_output, self, &err);
		    break;
		case LOAD_COMMAND:
			ianjuta_debugger_load (self->debugger, cmd->data.load.file, cmd->data.load.type, cmd->data.load.dirs, cmd->data.load.terminal, &err);	
			break;
  	    case ATTACH_COMMAND:
			ianjuta_debugger_attach (self->debugger, cmd->data.attach.pid, cmd->data.load.dirs, &err);	
			break;
		case UNLOAD_COMMAND:
		    ianjuta_debugger_unload (self->debugger, &err);
		    break;
		case QUIT_COMMAND:
			ret = ianjuta_debugger_quit (self->debugger, &err);
			break;
		case ABORT_COMMAND:
			ianjuta_debugger_abort (self->debugger, &err);
			break;
		case START_COMMAND:
			ianjuta_debugger_start (self->debugger, cmd->data.start.args, &err);
		    break;
		case RUN_COMMAND:
			ianjuta_debugger_run (self->debugger, &err);	
			break;
		case RUN_TO_COMMAND:
			ianjuta_debugger_run_to (self->debugger, cmd->data.pos.file, cmd->data.pos.line, &err);	
			break;
		case STEP_IN_COMMAND:
			ianjuta_debugger_step_in (self->debugger, &err);	
			break;
		case STEP_OVER_COMMAND:
			ianjuta_debugger_step_over (self->debugger, &err);	
			break;
		case STEP_OUT_COMMAND:
			ianjuta_debugger_step_out (self->debugger, &err);	
			break;
		case RESTART_COMMAND:
			//ianjuta_debugger_restart (self->debugger, &err);	
			break;
		case EXIT_COMMAND:
			ianjuta_debugger_exit (self->debugger, &err);	
			break;
		case INTERRUPT_COMMAND:
			ianjuta_debugger_interrupt (self->debugger, &err);	
			break;
		case ENABLE_BREAK_COMMAND:
			ianjuta_debugger_enable_breakpoint (self->debugger, cmd->data.brk.id, cmd->data.brk.enable == IANJUTA_DEBUGGER_YES ? TRUE : FALSE, cmd->callback, cmd->user_data, &err);	
			break;
		case IGNORE_BREAK_COMMAND:
			ianjuta_debugger_ignore_breakpoint (self->debugger, cmd->data.brk.id, cmd->data.brk.ignore, cmd->callback, cmd->user_data, &err);	
			break;
		case REMOVE_BREAK_COMMAND:
			ianjuta_debugger_clear_breakpoint (self->debugger, cmd->data.brk.id, cmd->callback, cmd->user_data, &err);	
			break;
		case INSPECT_COMMAND:
			ianjuta_debugger_inspect (self->debugger, cmd->data.watch.name, cmd->callback, cmd->user_data, &err);
		    break;
		case EVALUATE_COMMAND:
			ianjuta_debugger_evaluate (self->debugger, cmd->data.watch.name, cmd->data.watch.value, cmd->callback, cmd->user_data, &err);
		    break;
		case LIST_LOCAL_COMMAND:
			ianjuta_debugger_list_local (self->debugger, cmd->callback, cmd->user_data, &err);	
			break;
		case LIST_ARG_COMMAND:
			ianjuta_debugger_list_argument (self->debugger, cmd->callback, cmd->user_data, &err);	
			break;
		case LIST_THREAD_COMMAND:
			ianjuta_debugger_list_thread (self->debugger, cmd->callback, cmd->user_data, &err);	
			break;
		case SET_THREAD_COMMAND:
			ianjuta_debugger_set_thread (self->debugger, cmd->data.frame.frame, &err);	
			break;
		case INFO_THREAD_COMMAND:
			ianjuta_debugger_info_thread (self->debugger, cmd->data.info.id, cmd->callback, cmd->user_data, &err);	
			break;
		case INFO_SIGNAL_COMMAND:
			ianjuta_debugger_info_signal (self->debugger, cmd->callback, cmd->user_data, &err);	
			break;
		case INFO_SHAREDLIB_COMMAND:
			ianjuta_debugger_info_sharedlib (self->debugger, cmd->callback, cmd->user_data, &err);	
			break;
		case INFO_FRAME_COMMAND:
			ianjuta_debugger_info_frame (self->debugger, 0, cmd->callback, cmd->user_data, &err);	
			break;
		case INFO_ARGS_COMMAND:
			ianjuta_debugger_info_args (self->debugger, cmd->callback, cmd->user_data, &err);	
			break;
		case INFO_TARGET_COMMAND:
			ianjuta_debugger_info_target (self->debugger, cmd->callback, cmd->user_data, &err);	
			break;
		case INFO_PROGRAM_COMMAND:
			ianjuta_debugger_info_program (self->debugger, cmd->callback, cmd->user_data, &err);	
			break;
		case INFO_UDOT_COMMAND:
			ianjuta_debugger_info_udot (self->debugger, cmd->callback, cmd->user_data, &err);	
			break;
		case INFO_VARIABLES_COMMAND:
			ianjuta_debugger_info_variables (self->debugger, cmd->callback, cmd->user_data, &err);	
			break;
		case SET_FRAME_COMMAND:
			ianjuta_debugger_set_frame (self->debugger, cmd->data.frame.frame, &err);	
			break;
		case LIST_FRAME_COMMAND:
			ianjuta_debugger_list_frame (self->debugger, cmd->callback, cmd->user_data, &err);	
			break;
		case LIST_REGISTER_COMMAND:
			ianjuta_cpu_debugger_list_register (self->cpu_debugger, cmd->callback, cmd->user_data, &err);	
			break;
		case UPDATE_REGISTER_COMMAND:
			ianjuta_cpu_debugger_update_register (self->cpu_debugger, cmd->callback, cmd->user_data, &err);	
			break;
		case WRITE_REGISTER_COMMAND:
			reg.num = cmd->data.watch.id;
		    reg.name = cmd->data.watch.name;
		    reg.value = cmd->data.watch.value;
			ianjuta_cpu_debugger_write_register (self->cpu_debugger, &reg, &err);	
			break;
		case INSPECT_MEMORY_COMMAND:
			ianjuta_cpu_debugger_inspect_memory (self->cpu_debugger, cmd->data.mem.address, cmd->data.mem.length, cmd->callback, cmd->user_data, &err);	
			break;
		case DISASSEMBLE_COMMAND:
			ianjuta_cpu_debugger_disassemble (self->cpu_debugger, cmd->data.mem.address, cmd->data.mem.length, cmd->callback, cmd->user_data, &err);	
			break;
		case BREAK_LINE_COMMAND:
			ianjuta_debugger_set_breakpoint_at_line (self->debugger, cmd->data.pos.file, cmd->data.pos.line, cmd->callback, cmd->user_data, &err);	
			break;
		case BREAK_FUNCTION_COMMAND:
			ianjuta_debugger_set_breakpoint_at_function (self->debugger, cmd->data.pos.file, cmd->data.pos.function, cmd->callback, cmd->user_data, &err);	
			break;
		case BREAK_ADDRESS_COMMAND:
			ianjuta_debugger_set_breakpoint_at_address (self->debugger, cmd->data.pos.address, cmd->callback, cmd->user_data, &err);	
			break;
		case CONDITION_BREAK_COMMAND:
			ianjuta_debugger_condition_breakpoint (self->debugger, cmd->data.brk.id, cmd->data.brk.condition, cmd->callback, cmd->user_data, &err);	
			break;
		case USER_COMMAND:
			ianjuta_debugger_send_command (self->debugger, cmd->data.user.cmd, &err);	
			break;
		case PRINT_COMMAND:
			ianjuta_debugger_print (self->debugger, cmd->data.print.var, cmd->callback, cmd->user_data, &err);	
			break;
		case HANDLE_SIGNAL_COMMAND:
			ianjuta_debugger_handle_signal (self->debugger, cmd->data.signal.name, cmd->data.signal.stop, cmd->data.signal.print, cmd->data.signal.ignore, &err);	
			break;
		case DELETE_VARIABLE:
			ianjuta_variable_debugger_delete_var (IANJUTA_VARIABLE_DEBUGGER (self->debugger), cmd->data.var.name, NULL);
			break;
		case ASSIGN_VARIABLE:
			ianjuta_variable_debugger_assign (IANJUTA_VARIABLE_DEBUGGER (self->debugger), cmd->data.var.name, cmd->data.var.value, &err);
			break;
		case EVALUATE_VARIABLE:
			ianjuta_variable_debugger_evaluate (IANJUTA_VARIABLE_DEBUGGER (self->debugger), cmd->data.var.name, cmd->callback, cmd->user_data, &err);
			break;
		case LIST_VARIABLE_CHILDREN:
			ianjuta_variable_debugger_list_children (IANJUTA_VARIABLE_DEBUGGER (self->debugger), cmd->data.var.name, cmd->callback, cmd->user_data, &err);
			break;
		case CREATE_VARIABLE:
			ianjuta_variable_debugger_create (IANJUTA_VARIABLE_DEBUGGER (self->debugger), cmd->data.var.name, cmd->callback, cmd->user_data, &err);
			break;
		case UPDATE_VARIABLE:
			ianjuta_variable_debugger_update (IANJUTA_VARIABLE_DEBUGGER (self->debugger), cmd->callback, cmd->user_data, &err);
			break;
	    }
		
		if (err != NULL)
		{
			/* Something fail */
			gboolean cancel_state = cmd->type & CHANGE_STATE;
			
			/* Cancel command */
			self->head = cmd->next;
			dma_debugger_command_cancel (cmd);
			self->ready = TRUE;

			/* If a command changing a state has been cancelled,
			 * cancel invalidate commands too */
			if (cancel_state)
			{
				/* State change doesn't happen cancel all commands
				   those cannot handle this */
				if (dma_queue_cancel_unexpected (self, self->debugger_status))			
				{
					self->queue_status = self->debugger_status;
				}
			}
			
			g_error_free (err);
		}
	}
	dma_queue_emit_debugger_ready (self);
}

/* Public message functions
 *---------------------------------------------------------------------------*/

void
dma_debugger_message (DmaDebuggerQueue *self, const gchar* message)
{
//	dma_debugger_print_message (self, IANJUTA_MESSAGE_VIEW_TYPE_NORMAL, message);
}

void
dma_debugger_info (DmaDebuggerQueue *self, const gchar* message)
{
//	dma_debugger_print_message (self, IANJUTA_MESSAGE_VIEW_TYPE_INFO, message);
}

void dma_debugger_warning (DmaDebuggerQueue *self, const gchar* message)
{
//	dma_debugger_print_message (self, IANJUTA_MESSAGE_VIEW_TYPE_WARNING, message);
}

void dma_debugger_error (DmaDebuggerQueue *self, const gchar* message)
{
//	dma_debugger_print_message (self, IANJUTA_MESSAGE_VIEW_TYPE_ERROR, message);
}

/* IAnjutaDebugger callback
 *---------------------------------------------------------------------------*/

static void
on_dma_debugger_ready (DmaDebuggerQueue *self, IAnjutaDebuggerStatus status)
{
	DEBUG_PRINT ("From debugger: receive debugger ready");
	dma_queue_update_debugger_status (self, status, 0);
	if (self->ready) dma_debugger_queue_execute (self);
}

static void
on_dma_debugger_started (DmaDebuggerQueue *self)
{
	/* Nothing do to */
	DEBUG_PRINT ("From debugger: receive debugger started");
	dma_queue_update_debugger_status (self, IANJUTA_DEBUGGER_STARTED, 0);
	if (self->ready) dma_debugger_queue_execute (self);
}

static void
on_dma_debugger_stopped (DmaDebuggerQueue *self, gint status)
{
	DEBUG_PRINT ("From debugger: receive debugger stopped");
	dma_queue_update_debugger_status (self, IANJUTA_DEBUGGER_STOPPED, status);
	if (self->ready) dma_debugger_queue_execute (self);
}

static void
on_dma_program_loaded (DmaDebuggerQueue *self)
{
	DEBUG_PRINT ("From debugger: receive program loaded");
	dma_queue_update_debugger_status (self, IANJUTA_DEBUGGER_PROGRAM_LOADED, 0);
	if (self->ready) dma_debugger_queue_execute (self);
}

static void
on_dma_program_running (DmaDebuggerQueue *self)
{
	DEBUG_PRINT ("From debugger: debugger_program_running");
	dma_queue_update_debugger_status (self, IANJUTA_DEBUGGER_PROGRAM_RUNNING, 0);
}

static void
on_dma_program_stopped (DmaDebuggerQueue *self, guint thread)
{
	DEBUG_PRINT ("From debugger: receive program stopped");
	dma_queue_update_debugger_status (self, IANJUTA_DEBUGGER_PROGRAM_STOPPED, thread);
	if (self->ready) dma_debugger_queue_execute (self);
}

static void
on_dma_program_exited (DmaDebuggerQueue *self)
{
	DEBUG_PRINT ("From debugger: receive program exited");
	dma_queue_update_debugger_status (self, IANJUTA_DEBUGGER_PROGRAM_LOADED, 0);
	if (self->ready) dma_debugger_queue_execute (self);
}

static void
on_dma_location_changed (DmaDebuggerQueue *self, const gchar* src_path, guint line, guint address)
{
	DEBUG_PRINT ("From debugger: location changed");
	g_signal_emit_by_name (self, "location-changed", src_path, line, address);
}

static void
on_dma_frame_changed (DmaDebuggerQueue *self, guint frame, guint thread)
{
	DEBUG_PRINT ("From debugger: frame changed");
	g_signal_emit_by_name (self, "frame-changed", frame, thread);
}

static void
on_dma_signal_received (DmaDebuggerQueue *self, const gchar* name, const gchar* description)
{
	DEBUG_PRINT ("From debugger: signal received");
	g_signal_emit_by_name (self, "signal-received", name, description);
}

static void
on_dma_sharedlib_event (DmaDebuggerQueue *self)
{
	DEBUG_PRINT ("From debugger: shared lib event");
	self->stop_on_sharedlib = TRUE;
	dma_queue_update_debugger_status (self, IANJUTA_DEBUGGER_PROGRAM_STOPPED, 0);
	g_signal_emit_by_name (self, "sharedlib-event");
	ianjuta_debugger_run (IANJUTA_DEBUGGER (self), NULL);
}

/* Connect/Disconnect function
 *---------------------------------------------------------------------------*/

static gboolean
dma_debugger_activate_plugin (DmaDebuggerQueue* self, const gchar *mime_type)
{
	AnjutaPluginManager *plugin_manager;
	AnjutaPluginDescription *plugin;
	GList *descs = NULL;
	gchar *value;
	gchar **interface_list;
	gchar **interface;

	/* Get list of debugger plugins */
	plugin_manager = anjuta_shell_get_plugin_manager (ANJUTA_PLUGIN(self->plugin)->shell, NULL);
	descs = anjuta_plugin_manager_query (plugin_manager,
					"Anjuta Plugin","Interfaces", "IAnjutaDebugger",
					"File Loader", "SupportedMimeTypes", mime_type,
					NULL);

	if (descs == NULL)
	{
		/* No plugin found */
		anjuta_util_dialog_error (GTK_WINDOW (ANJUTA_PLUGIN (self->plugin)->shell),
				_("Unable to find one debugger plugin acception %s mime type"), mime_type);
			
		return FALSE;
	}
	else if (g_list_length (descs) == 1)
	{
		/* Only one plugin found, use it */
		plugin = (AnjutaPluginDescription *)descs->data;
	}
	else
	{
		/* Ask the user to select one plugin */
		plugin = anjuta_plugin_manager_select (plugin_manager,
						_("Select a plugin"), 
						_("Please select a plugin to activate"),
						descs);
	}
											   
	if (plugin == NULL)
	{
		/* No plugin selected */
		
		return FALSE;
	}

	/* Get debugger location */
	value = NULL;
	anjuta_plugin_description_get_string (plugin, "Anjuta Plugin", "Location", &value);
	g_return_val_if_fail (value != NULL, FALSE);

	/* Get debugger interface */
	self->debugger = (IAnjutaDebugger *)anjuta_plugin_manager_get_plugin_by_id (plugin_manager, value);
	g_free (value);
	
	/* Get cpu debugger interface if available */
	if (IANJUTA_IS_CPU_DEBUGGER (self->debugger))
	{
		self->cpu_debugger = IANJUTA_CPU_DEBUGGER (self->debugger);
	}
	
	return TRUE;
}

static void
dma_debugger_disconnect (DmaDebuggerQueue *self)
{
	/* Disconnect signal */
	if (self->debugger)
	{
		g_signal_handlers_disconnect_by_func (self->debugger, G_CALLBACK (on_dma_debugger_ready), self);
		g_signal_handlers_disconnect_by_func (self->debugger, G_CALLBACK (on_dma_debugger_started), self);
		g_signal_handlers_disconnect_by_func (self->debugger, G_CALLBACK (on_dma_debugger_stopped), self);
		g_signal_handlers_disconnect_by_func (self->debugger, G_CALLBACK (on_dma_program_loaded), self);
		g_signal_handlers_disconnect_by_func (self->debugger, G_CALLBACK (on_dma_program_running), self);
		g_signal_handlers_disconnect_by_func (self->debugger, G_CALLBACK (on_dma_program_stopped), self);
		g_signal_handlers_disconnect_by_func (self->debugger, G_CALLBACK (on_dma_program_exited), self);
		g_signal_handlers_disconnect_by_func (self->debugger, G_CALLBACK (on_dma_location_changed), self);
		g_signal_handlers_disconnect_by_func (self->debugger, G_CALLBACK (on_dma_sharedlib_event), self);
		self->debugger = NULL;
	}
}

static gboolean
dma_debugger_connect (DmaDebuggerQueue *self, const gchar *mime_type)
{
	dma_debugger_disconnect (self);
	
	/* Look for a debugger supporting mime_type */
	if (!dma_debugger_activate_plugin (self, mime_type))
	{
		
		
		return FALSE;
	}
	
	if (self->debugger)
	{
		/* Connect signal */
		g_signal_connect_swapped (self->debugger, "debugger-ready", G_CALLBACK (on_dma_debugger_ready), self);
		g_signal_connect_swapped (self->debugger, "debugger-started", G_CALLBACK (on_dma_debugger_started), self);
		g_signal_connect_swapped (self->debugger, "debugger-stopped", G_CALLBACK (on_dma_debugger_stopped), self);
		g_signal_connect_swapped (self->debugger, "program-loaded", G_CALLBACK (on_dma_program_loaded), self);
		g_signal_connect_swapped (self->debugger, "program-running", G_CALLBACK (on_dma_program_running), self);
		g_signal_connect_swapped (self->debugger, "program-stopped", G_CALLBACK (on_dma_program_stopped), self);
		g_signal_connect_swapped (self->debugger, "program-exited", G_CALLBACK (on_dma_program_exited), self);
		g_signal_connect_swapped (self->debugger, "location-changed", G_CALLBACK (on_dma_location_changed), self);
		g_signal_connect_swapped (self->debugger, "signal-received", G_CALLBACK (on_dma_signal_received), self);
		g_signal_connect_swapped (self->debugger, "frame-changed", G_CALLBACK (on_dma_frame_changed), self);
		g_signal_connect_swapped (self->debugger, "sharedlib-event", G_CALLBACK (on_dma_sharedlib_event), self);
		
		if (self->log == NULL)
		{
			ianjuta_debugger_disable_log (self->debugger, NULL);
		}
		else
		{
			ianjuta_debugger_enable_log (self->debugger, self->log, NULL);
		}
	}
	
	return self->debugger != NULL;
}

/* Implementation of IAnjutaDebugger interface
 *---------------------------------------------------------------------------*/

static IAnjutaDebuggerStatus
idebugger_get_status (IAnjutaDebugger *iface, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);

	return self->queue_status;
}

static gboolean
idebugger_initialize (IAnjutaDebugger *iface, IAnjutaDebuggerOutputCallback callback, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;

	cmd = dma_debugger_queue_append (self, DMA_INITIALIZE_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->callback = (IAnjutaDebuggerCallback)callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_load (IAnjutaDebugger *iface, const gchar *file, const gchar* mime_type,
				const GList *search_dirs, gboolean terminal, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	const GList *node;

	if (!dma_debugger_connect (self, mime_type)) return FALSE;
					
	cmd = dma_debugger_queue_append (self, DMA_LOAD_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->data.load.file = g_strdup (file);
	cmd->data.load.type = g_strdup (mime_type);
	cmd->data.load.dirs = NULL;
	cmd->data.load.terminal = terminal;
	for (node = search_dirs; node != NULL; node = g_list_next (node))
	{
		cmd->data.load.dirs = g_list_prepend (cmd->data.load.dirs, g_strdup (node->data));
	}
	cmd->data.load.dirs = g_list_reverse (cmd->data.load.dirs);

	dma_debugger_queue_execute (self);

	return TRUE;
}

static gboolean
idebugger_attach (IAnjutaDebugger *iface, pid_t pid, const GList *search_dirs, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	const GList *node;

	if (!dma_debugger_connect (self, NULL)) return FALSE;
	
	cmd = dma_debugger_queue_append (self, DMA_ATTACH_COMMAND, err);
	if (cmd == NULL) return FALSE;
	
	cmd->data.attach.pid = pid;
	cmd->data.attach.dirs = NULL;
	for (node = search_dirs; node != NULL; node = g_list_next (node))
	{
		cmd->data.attach.dirs = g_list_prepend (cmd->data.attach.dirs, g_strdup (node->data));
	}
	cmd->data.attach.dirs = g_list_reverse (cmd->data.attach.dirs);
	
	dma_debugger_queue_execute (self);

	return TRUE;
}

static gboolean
idebugger_start (IAnjutaDebugger *iface, const gchar *args, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;

	cmd = dma_debugger_queue_append (self, DMA_START_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->data.start.args = args == NULL ? NULL : g_strdup (args);
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_unload (IAnjutaDebugger *iface, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_UNLOAD_COMMAND, err);
	if (cmd == NULL) return FALSE;
	
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_quit (IAnjutaDebugger *iface, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;

	cmd = dma_debugger_queue_append (self, DMA_QUIT_COMMAND, err);
	if (cmd == NULL) return FALSE;
	
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_abort (IAnjutaDebugger *iface, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;

	cmd = dma_debugger_queue_append (self, DMA_ABORT_COMMAND, err);
	if (cmd == NULL) return FALSE;
	
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_run (IAnjutaDebugger *iface, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;

	cmd = dma_debugger_queue_append (self, DMA_RUN_COMMAND, err);
	if (cmd == NULL) return FALSE;
	
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_step_in (IAnjutaDebugger *iface, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_STEP_IN_COMMAND, err);
	if (cmd == NULL) return FALSE;
	
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_step_over (IAnjutaDebugger *iface, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_STEP_OVER_COMMAND, err);
	if (cmd == NULL) return FALSE;
	
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_run_to (IAnjutaDebugger *iface, const gchar *file,
						   gint line, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_RUN_TO_COMMAND, err);
	if (cmd == NULL) return FALSE;
	
	cmd->data.pos.file = g_strdup (file);
	cmd->data.pos.line = line;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_step_out (IAnjutaDebugger *iface, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_STEP_OUT_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_exit (IAnjutaDebugger *iface, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_EXIT_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_interrupt (IAnjutaDebugger *iface, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;

	cmd = dma_debugger_queue_append (self, DMA_INTERRUPT_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_add_breakpoint_at_line (IAnjutaDebugger *iface, const gchar* file, guint line, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_BREAK_LINE_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->data.pos.file = g_strdup (file);
	cmd->data.pos.line = line;
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_add_breakpoint_at_function (IAnjutaDebugger *iface, const gchar* file, const gchar* function, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_BREAK_FUNCTION_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->data.pos.file = g_strdup (file);
	cmd->data.pos.function = g_strdup (function);
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_add_breakpoint_at_address (IAnjutaDebugger *iface, guint address, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_BREAK_ADDRESS_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->data.pos.address = address;
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_enable_breakpoint (IAnjutaDebugger *iface, guint id, gboolean enable, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_ENABLE_BREAK_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->data.brk.id = id;
	cmd->data.brk.enable = enable;
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_ignore_breakpoint (IAnjutaDebugger *iface, guint id, guint ignore, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_IGNORE_BREAK_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->data.brk.id = id;
	cmd->data.brk.ignore = ignore;
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_condition_breakpoint (IAnjutaDebugger *iface, guint id, const gchar *condition, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_CONDITION_BREAK_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->data.brk.id = id;
	cmd->data.brk.condition = g_strdup (condition);
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_remove_breakpoint (IAnjutaDebugger *iface, guint id, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_REMOVE_BREAK_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->data.brk.id = id;
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_inspect (IAnjutaDebugger *iface, const gchar *expression, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_INSPECT_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->data.watch.name = g_strdup (expression);
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_evaluate (IAnjutaDebugger *iface, const gchar *name, const gchar* value, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_EVALUATE_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->data.watch.name = g_strdup (name);
	cmd->data.watch.value = g_strdup (value);
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_send_command (IAnjutaDebugger *iface, const gchar* command, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_USER_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->data.user.cmd = g_strdup (command);
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean 
idebugger_print (IAnjutaDebugger *iface, const gchar* variable, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_PRINT_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->data.print.var = g_strdup (variable);
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_list_local (IAnjutaDebugger *iface, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_LIST_LOCAL_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_list_argument (IAnjutaDebugger *iface, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_LIST_ARG_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_list_thread (IAnjutaDebugger *iface, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_LIST_THREAD_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_set_thread (IAnjutaDebugger *iface, guint thread, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_SET_THREAD_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->data.frame.frame = thread;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_info_thread (IAnjutaDebugger *iface, guint thread, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_INFO_THREAD_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->callback = callback;
	cmd->user_data = user_data;
	cmd->data.info.id = thread;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_info_signal (IAnjutaDebugger *iface, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_INFO_SIGNAL_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_info_sharedlib (IAnjutaDebugger *iface, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_INFO_SHAREDLIB_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_handle_signal (IAnjutaDebugger *iface, const gchar* name, gboolean stop, gboolean print, gboolean ignore, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_HANDLE_SIGNAL_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->data.signal.name = g_strdup (name);
	cmd->data.signal.stop = stop;
	cmd->data.signal.print = print;
	cmd->data.signal.ignore = ignore;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_info_frame (IAnjutaDebugger *iface, guint frame, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_INFO_FRAME_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->data.frame.frame = frame;
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_info_args (IAnjutaDebugger *iface, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_INFO_ARGS_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_info_target (IAnjutaDebugger *iface, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_INFO_TARGET_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_info_program (IAnjutaDebugger *iface, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_INFO_PROGRAM_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_info_udot (IAnjutaDebugger *iface, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_INFO_UDOT_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_info_variables (IAnjutaDebugger *iface, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_INFO_VARIABLES_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_set_frame (IAnjutaDebugger *iface, guint frame, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;

	cmd = dma_debugger_queue_append (self, DMA_SET_FRAME_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->data.frame.frame = frame;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_list_frame (IAnjutaDebugger *iface, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_LIST_FRAME_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_list_register (IAnjutaDebugger *iface, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_LIST_REGISTER_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
idebugger_callback (IAnjutaDebugger *iface, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_CALLBACK_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static void
idebugger_enable_log (IAnjutaDebugger *iface, IAnjutaMessageView *log, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);

	self->log = log;
	if (self->debugger != NULL)
	{
		ianjuta_debugger_enable_log (self->debugger, self->log, err);
	}
}

static void
idebugger_disable_log (IAnjutaDebugger *iface, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);

	self->log = NULL;
	if (self->debugger != NULL)
	{
		ianjuta_debugger_disable_log (self->debugger, err);
	}
}

static void
idebugger_iface_init (IAnjutaDebuggerIface *iface)
{
	iface->get_status = idebugger_get_status;
	iface->initialize = idebugger_initialize;
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
//	iface->restart = idebugger_restart;
	iface->exit = idebugger_exit;
	iface->interrupt = idebugger_interrupt;
	iface->set_breakpoint_at_line = idebugger_add_breakpoint_at_line;
	iface->clear_breakpoint = idebugger_remove_breakpoint;
	iface->set_breakpoint_at_address = idebugger_add_breakpoint_at_address;
	iface->set_breakpoint_at_function = idebugger_add_breakpoint_at_function;
	iface->enable_breakpoint = idebugger_enable_breakpoint;
	iface->ignore_breakpoint = idebugger_ignore_breakpoint;
	iface->condition_breakpoint = idebugger_condition_breakpoint;
	
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
	iface->callback = idebugger_callback;

	iface->enable_log = idebugger_enable_log;
	iface->disable_log = idebugger_disable_log;
	
	iface->send_command = idebugger_send_command;

}

/* Implementation of IAnjutaCpuDebugger interface
 *---------------------------------------------------------------------------*/

static gboolean
icpu_debugger_list_register (IAnjutaCpuDebugger *iface, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_LIST_REGISTER_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}


static gboolean
icpu_debugger_update_register (IAnjutaCpuDebugger *iface, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_UPDATE_REGISTER_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
icpu_debugger_write_register (IAnjutaCpuDebugger *iface, IAnjutaDebuggerRegister *value, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_WRITE_REGISTER_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->data.watch.id = value->num;
	cmd->data.watch.name = value->name != NULL ? g_strdup (value->name) : NULL;
	cmd->data.watch.value = value->value != NULL ? g_strdup (value->value) : NULL;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
icpu_debugger_inspect_memory (IAnjutaCpuDebugger *iface, guint address, guint length, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_INSPECT_MEMORY_COMMAND, err);
	if (cmd == NULL) return FALSE;

	cmd->data.mem.address = address;
	cmd->data.mem.length = length;
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
icpu_debugger_disassemble (IAnjutaCpuDebugger *iface, guint address, guint length, IAnjutaDebuggerCallback callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = (DmaDebuggerQueue *)iface;
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_DISASSEMBLE_COMMAND, err);
	if (cmd == NULL) return FALSE;

	cmd->data.mem.address = address;
	cmd->data.mem.length = length;
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
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
ivariable_debugger_delete (IAnjutaVariableDebugger *iface, const gchar *name, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_DELETE_VARIABLE_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->data.var.name = g_strdup(name);
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
ivariable_debugger_evaluate (IAnjutaVariableDebugger *iface, const gchar *name, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_EVALUATE_VARIABLE_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->data.var.name = g_strdup(name);
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
ivariable_debugger_assign (IAnjutaVariableDebugger *iface, const gchar *name, const gchar *value, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_ASSIGN_VARIABLE_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->data.var.name = g_strdup(name);
	cmd->data.var.value = g_strdup(value);
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
ivariable_debugger_list_children (IAnjutaVariableDebugger *iface, const gchar *name, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_LIST_VARIABLE_CHILDREN_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->data.var.name = g_strdup(name);
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
ivariable_debugger_create (IAnjutaVariableDebugger *iface, const gchar *name, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;

	cmd = dma_debugger_queue_append (self, DMA_CREATE_VARIABLE_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->data.var.name = g_strdup(name);
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
	return TRUE;
}

static gboolean
ivariable_debugger_update (IAnjutaVariableDebugger *iface, IAnjutaDebuggerCallback callback, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (iface);
	DmaQueueCommand *cmd;
	
	cmd = dma_debugger_queue_append (self, DMA_UPDATE_VARIABLE_COMMAND, err);
	if (cmd == NULL) return FALSE;
		
	cmd->callback = callback;
	cmd->user_data = user_data;
	dma_debugger_queue_execute (self);
	
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

/* GObject functions
 *---------------------------------------------------------------------------*/

/* Used in dispose and finalize */
static gpointer parent_class;

/* dispose is the first destruction step. It is used to unref object created
 * with instance_init in order to break reference counting cycles. This
 * function could be called several times. All function should still work
 * after this call. It has to called its parents.*/

static void
dma_debugger_queue_dispose (GObject *obj)
{
	DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (obj);
	
	dma_debugger_queue_clear (self);

	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

/* finalize is the last destruction step. It must free all memory allocated
 * with instance_init. It is called only one time just before releasing all
 * memory */

static void
dma_debugger_queue_finalize (GObject *obj)
{
	/*DmaDebuggerQueue *self = DMA_DEBUGGER_QUEUE (obj);*/

	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

/* instance_init is the constructor. All functions should work after this
 * call. */

static void
dma_debugger_queue_instance_init (DmaDebuggerQueue *self)
{
	self->plugin = NULL;
	self->debugger = NULL;
	self->cpu_debugger = NULL;
	self->head = NULL;
	self->queue_command = TRUE;
	self->ready = TRUE;
	self->debugger_status = IANJUTA_DEBUGGER_STOPPED;
	self->queue_status = IANJUTA_DEBUGGER_STOPPED;
	self->status = IANJUTA_DEBUGGER_STOPPED;
	self->log = NULL;
	self->output_callback = NULL;
}

/* class_init intialize the class itself not the instance */

static void
dma_debugger_queue_class_init (DmaDebuggerQueueClass * klass)
{
	GObjectClass *object_class;

	g_return_if_fail (klass != NULL);
	object_class = G_OBJECT_CLASS (klass);
	
	parent_class = g_type_class_peek_parent (klass);
	
	object_class->dispose = dma_debugger_queue_dispose;
	object_class->finalize = dma_debugger_queue_finalize;
}

GType
dma_debugger_queue_get_type (void)
{
	static GType type = 0;

	if (!type)
	{
		static const GTypeInfo type_info = 
		{
			sizeof (DmaDebuggerQueueClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) dma_debugger_queue_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,           /* class_data */
			sizeof (DmaDebuggerQueue),
			0,              /* n_preallocs */
			(GInstanceInitFunc) dma_debugger_queue_instance_init,
			NULL            /* value_table */
		};
		static const GInterfaceInfo iface_debugger_info = {
			(GInterfaceInitFunc) idebugger_iface_init,
			NULL,		/* interface_finalize */
			NULL		/* interface_data */
		};
		static const GInterfaceInfo iface_cpu_debugger_info = {
			(GInterfaceInitFunc) icpu_debugger_iface_init,
			NULL,		/* interface_finalize */
			NULL		/* interface_data */
		};
		static const GInterfaceInfo iface_variable_debugger_info = {
			(GInterfaceInitFunc) ivariable_debugger_iface_init,
			NULL,		/* interface_finalize */
			NULL		/* interface_data */
		};

		type = g_type_register_static (G_TYPE_OBJECT,
		                            "DmaDebuggerQueue", &type_info, 0);

		g_type_add_interface_static (type, IANJUTA_TYPE_DEBUGGER,
						&iface_debugger_info);
		g_type_add_interface_static (type, IANJUTA_TYPE_CPU_DEBUGGER,
						&iface_cpu_debugger_info);
		g_type_add_interface_static (type, IANJUTA_TYPE_VARIABLE_DEBUGGER,
						&iface_variable_debugger_info);
	}
	
	return type;
}

/* Creation and Destruction
 *---------------------------------------------------------------------------*/

DmaDebuggerQueue*
dma_debugger_queue_new (AnjutaPlugin *plugin)
{
	DmaDebuggerQueue *self;

	self = g_object_new (DMA_DEBUGGER_QUEUE_TYPE, NULL);
	
	self->plugin = plugin;
	
	return self;
}
	
void
dma_debugger_queue_free (DmaDebuggerQueue *self)
{
	dma_debugger_disconnect (self);
	g_object_unref (self);
}
