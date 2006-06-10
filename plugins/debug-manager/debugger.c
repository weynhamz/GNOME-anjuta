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

#include "debugger.h"

#include <libanjuta/plugins.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>

/* Contants defintion
 *---------------------------------------------------------------------------*/

#define ICON_FILE "anjuta-debug-manager.plugin.png"

/* Private type
 *---------------------------------------------------------------------------*/

typedef enum
{
	DUMMY_COMMAND,
	INITIALIZE_COMMAND,     /* Debugger stopped */
	LOAD_COMMAND,           /* Debugger started */
	ATTACH_COMMAND,
	QUIT_COMMAND,           /* Debugger started - Program stopped */
	USER_COMMAND,
	INSPECT_MEMORY_COMMAND,
	LIST_REGISTER_COMMAND,
	UNLOAD_COMMAND,         /* Program loaded */
	START_COMMAND,         
	RESTART_COMMAND,        /* Program loaded - Program stopped */
	BREAK_LINE_COMMAND,
	BREAK_FUNCTION_COMMAND,
	BREAK_ADDRESS_COMMAND,
	ENABLE_BREAK_COMMAND,
	IGNORE_BREAK_COMMAND,
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
	INFO_SIGNAL_COMMAND,
	INFO_FRAME_COMMAND,
	INFO_ARGS_COMMAND,
	INFO_THREADS_COMMAND,
	INFO_VARIABLES_COMMAND,
	SET_FRAME_COMMAND,
	LIST_FRAME_COMMAND,
	UPDATE_REGISTER_COMMAND,
	WRITE_REGISTER_COMMAND,
	EVALUATE_COMMAND,
	INSPECT_COMMAND,
	PRINT_COMMAND,
	INTERRUPT_COMMAND /* Program running */
} DmaDebuggerCommandType;

enum
{
	DEBUGGER_STOPPED_OR_GREATER = DUMMY_COMMAND,
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

typedef struct _DmaDebuggerCommand
{
	DmaDebuggerCommandType type;
	union {
		struct {
			IAnjutaDebuggerOutputFunc callback;
			gpointer user_data;
		} init;
		struct {
			gchar *file;
			gchar *type;
			GList *dirs;
		} load;
		struct {
			gchar *args;
			gboolean terminal;
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
			IAnjutaDebuggerBreakpointFunc callback;
			gpointer user_data;
		} pos;
		struct {
			guint id;
			guint ignore;
			gchar *condition;
			gboolean enable;
			IAnjutaDebuggerBreakpointFunc callback;
			gpointer user_data;
		} brk;
		struct {
			guint id;
			gchar *name;
			gchar *value;
			IAnjutaDebuggerGCharFunc callback;
			gpointer user_data;
		} watch;
		struct {
			gchar *cmd;
		} user;
		struct {
			gchar *var;
			IAnjutaDebuggerGCharFunc callback;
			gpointer user_data;
		} print;
		struct {
			guint id;
			IAnjutaDebuggerGListFunc callback;
			gpointer user_data;
		} info;
		struct {
			gchar *name;
			gboolean stop;
			gboolean print;
			gboolean ignore;
		} signal;
		struct {
			guint frame;
			IAnjutaDebuggerGListFunc callback;
			gpointer user_data;
		} frame;
		struct {
			const void *address;
			guint length;
			IAnjutaDebuggerGListFunc callback;
			gpointer user_data;
		} mem;
	};
	struct _DmaDebuggerCommand *next;
} DmaDebuggerCommand;

struct _DmaDebuggerQueue {
	GObject parent;

	AnjutaPlugin* plugin;
	IAnjutaDebugger* debugger;
	IAnjutaCpuDebugger* cpu_debugger;
	
	/* Command queue */
	DmaDebuggerCommand *head;
	DmaDebuggerCommand *tail;
	
	IAnjutaDebuggerStatus status;
	gboolean ready;
	gboolean stop_on_sharedlib;

	/* View for debugger messages */
	IAnjutaMessageView* view;
	IAnjutaMessageViewType view_type;

	/* User output callback */
	IAnjutaDebuggerOutputFunc output_callback;
	gpointer output_data;	
};

struct _DmaDebuggerQueueClass {
	GObjectClass parent;
	void (*debugger_ready2)  (DmaDebuggerQueue *this, IAnjutaDebuggerStatus status);
};

/* Message functions
 *---------------------------------------------------------------------------*/

/*static void
dma_debugger_print_message (DmaDebuggerQueue *this, IAnjutaMessageViewType type, const gchar* message)
{
	if (this->view)
	{
		this->view_type = type;
		ianjuta_message_view_buffer_append (this->view, message, NULL);	
	}
}

static void
on_message_buffer_flushed (IAnjutaMessageView *view, const gchar *line,
                                                 DmaDebuggerQueue *this)
{
	if (this->view)
	{
		ianjuta_message_view_append (this->view, this->view_type, line, "", NULL);	
	}
}

static void
dma_debugger_create_view (DmaDebuggerQueue *this)
{
	if (this->view == NULL)
	{
		IAnjutaMessageManager* man;

		man = anjuta_shell_get_interface (this->plugin->shell, IAnjutaMessageManager, NULL);
		this->view = ianjuta_message_manager_add_view (man, _("Debugger"), ICON_FILE, NULL);
        if (this->view != NULL)
        {
			g_signal_connect (G_OBJECT (this->view), "buffer_flushed", G_CALLBACK (on_message_buffer_flushed), this);
	        g_object_add_weak_pointer (G_OBJECT (this->view), (gpointer *)&this->view);
        }
	}
	else
	{
		ianjuta_message_view_clear (this->view, NULL);
	}
}*/

/* Call backs
 *---------------------------------------------------------------------------*/

static void
on_debugger_output (IAnjutaDebuggerOutputType type, const gchar *message, gpointer user_data)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)user_data;

	/* Call user call back if exist */
	if (this->output_callback != NULL)
	{
		this->output_callback (type, message, this->output_data);
	}
	else
	{
		/* Default handling of debugger message */

		switch (type)
	    {
		case IANJUTA_DEBUGGER_OUTPUT:
			dma_debugger_message (this, message);
	        break;
		case IANJUTA_DEBUGGER_INFO_OUTPUT:
			dma_debugger_info (this, message);
	        break;
		case IANJUTA_DEBUGGER_WARNING_OUTPUT:
			dma_debugger_warning (this, message);
	        break;
		case IANJUTA_DEBUGGER_ERROR_OUTPUT:
			dma_debugger_error (this, message);
	        break;
    	}
    }
}

/* Queue function
 *---------------------------------------------------------------------------*/

static void
dma_debugger_command_free (DmaDebuggerCommand *this)
{
	switch (this->type)
	{
	case DUMMY_COMMAND:
	case INITIALIZE_COMMAND:
	case UNLOAD_COMMAND:
	case QUIT_COMMAND:
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
	case LIST_LOCAL_COMMAND:
	case INFO_SIGNAL_COMMAND:
	case INFO_SHAREDLIB_COMMAND:
	case INFO_FRAME_COMMAND:
	case INFO_ARGS_COMMAND:
	case INFO_TARGET_COMMAND:
	case INFO_PROGRAM_COMMAND:
	case INFO_UDOT_COMMAND:
	case INFO_THREADS_COMMAND:
	case INFO_VARIABLES_COMMAND:
	case SET_FRAME_COMMAND:
	case LIST_FRAME_COMMAND:
	case LIST_REGISTER_COMMAND:
	case UPDATE_REGISTER_COMMAND:
	case INSPECT_MEMORY_COMMAND:
		break;
	case INSPECT_COMMAND:
	case EVALUATE_COMMAND:
	case WRITE_REGISTER_COMMAND:
		if (this->watch.name != NULL) g_free (this->watch.name);
		if (this->watch.value != NULL) g_free (this->watch.value);
		break;
	case START_COMMAND:
		if (this->start.args) g_free (this->start.args);
		break;
	case LOAD_COMMAND:
		if (this->load.file) g_free (this->load.file);
		if (this->load.type) g_free (this->load.type);
        g_list_foreach (this->load.dirs, (GFunc)g_free, NULL);
        g_list_free (this->load.dirs);
		break;
	case ATTACH_COMMAND:
        g_list_foreach (this->attach.dirs, (GFunc)g_free, NULL);
        g_list_free (this->attach.dirs);
		break;
	case RUN_TO_COMMAND:
	case BREAK_LINE_COMMAND:
	case BREAK_FUNCTION_COMMAND:
	case BREAK_ADDRESS_COMMAND:
		if (this->pos.file) g_free (this->pos.file);
		if (this->pos.function) g_free (this->pos.function);
		break;
	case CONDITION_BREAK_COMMAND:
		if (this->brk.condition) g_free (this->brk.condition);
		break;
	case USER_COMMAND:
		if (this->user.cmd) g_free (this->user.cmd);
		break;
	case PRINT_COMMAND:
		if (this->print.var) g_free (this->print.var);
		break;
	case HANDLE_SIGNAL_COMMAND:
		if (this->signal.name) g_free (this->signal.name);
		break;
    }
}

static gboolean
dma_debugger_queue_cancel (DmaDebuggerQueue *this)
{
	DmaDebuggerCommand* cmd;
	
	cmd = this->head;
	if (cmd == NULL) return FALSE;

	this->head = cmd;
	dma_debugger_command_free (cmd);
	g_free (cmd);
}

static void
dma_debugger_queue_clear (DmaDebuggerQueue *this)
{
	DmaDebuggerCommand* cmd;

	for (cmd = this->head; cmd != NULL; )
	{
		cmd = cmd->next;
		dma_debugger_command_free (this->head);
		g_free (this->head);
		this->head = cmd;
	}
	this->head = NULL;
	this->tail = NULL;
}

static DmaDebuggerCommand*
dma_debugger_queue_append (DmaDebuggerQueue *this)
{
	DmaDebuggerCommand* cmd;
	
	cmd = g_new0 (DmaDebuggerCommand, 1);
	cmd->type = DUMMY_COMMAND;
	
	if (this->head == NULL)
	{
		/* queue is empty */
		this->head = cmd;
	}
	else
	{
		this->tail->next = cmd;
	}
	this->tail = cmd;
	
	return cmd;
}

static DmaDebuggerCommand*
dma_debugger_queue_prepend (DmaDebuggerQueue *this)
{
	DmaDebuggerCommand* cmd;
	
	cmd = g_new0 (DmaDebuggerCommand, 1);
	cmd->type = DUMMY_COMMAND;
	cmd->next = this->head;
	this->head = cmd;
	
	if (this->head == NULL)
	{
		/* queue is empty */
		this->tail = cmd;
	}
	
	return cmd;
}

static void
dma_debugger_queue_remove (DmaDebuggerQueue *this)
{
	DmaDebuggerCommand* cmd;
	
	if (this->head != NULL)
	{
		cmd = this->head->next;
		dma_debugger_command_free (this->head);
		g_free (this->head);
		this->head = cmd;
		if (cmd == NULL) this->tail = NULL;
	}
}

static void
dma_debugger_end_command (DmaDebuggerQueue *this)
{
	if (this->ready == FALSE)
	{
		dma_debugger_queue_remove (this);
		this->ready = TRUE;
	}
}

static IAnjutaDebuggerStatus
dma_debugger_update_status (DmaDebuggerQueue *this, IAnjutaDebuggerStatus status)
{
	DmaDebuggerCommand *head;
	DmaDebuggerCommand *tail;

	if (status != IANJUTA_DEBUGGER_BUSY)
	{
		/* Get a valid state, debugger is ready */
		dma_debugger_end_command (this);
	}
	
	switch (status)
	{
	case IANJUTA_DEBUGGER_BUSY:
		/* Debugger is busy */
		break;
	case IANJUTA_DEBUGGER_PROGRAM_RUNNING:
		if (this->status != IANJUTA_DEBUGGER_PROGRAM_RUNNING) 
		{
			this->status = IANJUTA_DEBUGGER_PROGRAM_RUNNING;
			this->stop_on_sharedlib = FALSE;
			g_signal_emit_by_name (this, "program-running");
		}
	    break;
	case IANJUTA_DEBUGGER_PROGRAM_STOPPED:
		if (this->status != IANJUTA_DEBUGGER_PROGRAM_STOPPED) 
		{
			this->status = IANJUTA_DEBUGGER_PROGRAM_STOPPED;
			if (!this->stop_on_sharedlib)
				g_signal_emit_by_name (this, "program-stopped");
		}
	    break;
	case IANJUTA_DEBUGGER_PROGRAM_LOADED:
		if (this->status != IANJUTA_DEBUGGER_PROGRAM_LOADED)
		{
			this->status = IANJUTA_DEBUGGER_PROGRAM_LOADED;
			this->stop_on_sharedlib = FALSE;
			head = this->head;
			tail = this->tail;
			this->head = NULL;
			this->tail = NULL;
			g_signal_emit_by_name (this, "program-loaded");
			if (this->head == NULL)
			{
				this->tail = tail;
				this->head = head;
			}
			else
			{
				this->tail->next = head;
				this->tail = tail;
			}
		}
	    break;
	case IANJUTA_DEBUGGER_STARTED:
		if (this->status != IANJUTA_DEBUGGER_STARTED) 
		{
			this->status = IANJUTA_DEBUGGER_STARTED;
			g_signal_emit_by_name (this, "debugger-started");
		}
	    break;
	case IANJUTA_DEBUGGER_STOPPED:
		if (this->status != IANJUTA_DEBUGGER_STOPPED) 
		{
			this->status = IANJUTA_DEBUGGER_STOPPED;
			g_signal_emit_by_name (this, "debugger-stopped");
		}
	    break;
	}
	
	return this->status;
}


static void
dma_debugger_queue_execute (DmaDebuggerQueue *this)
{
	IAnjutaCpuDebuggerRegister reg;
	
	/* Check if debugger is connected to a debugger backend */
	if (this->debugger == NULL) return;

	/* Check if there debugger is busy */
	if (!this->ready)
	{
		IAnjutaDebuggerStatus status;
		/* Recheck status in case of desynchronization */
		status = ianjuta_debugger_get_status (this->debugger, NULL);
		dma_debugger_update_status (this, status);
	}

	/* Check if there is something to execute */
	while ((this->head != NULL) && (this->ready))
	{
		DmaDebuggerCommand *cmd;
		
		cmd = this->head;

		printf("debugger cmd %d status %d\n", cmd, this->status);
		/* Check if the debugger is in the right mode */
		switch (this->status)
		{
			case IANJUTA_DEBUGGER_BUSY:
				return;
			case IANJUTA_DEBUGGER_STOPPED:
				if (cmd->type >= DEBUGGER_STARTED_OR_GREATER)
				{
					/* Start debugger first */
					printf("start debugger\n");
					cmd = dma_debugger_queue_prepend (this);
					cmd->type = INITIALIZE_COMMAND;
					cmd->init.callback = this->output_callback;
					cmd->init.user_data = this->output_data;
				}
				break;
			case IANJUTA_DEBUGGER_STARTED:
				if (cmd->type <= DEBUGGER_STOPPED_OR_LESSER)
				{
					/* Stop debugger first */
					cmd = dma_debugger_queue_prepend (this);
					cmd->type = QUIT_COMMAND;
				}
				else if (cmd->type >= PROGRAM_LOADED_OR_GREATER)
				{
					/* Unable to load a program, so remove command */
					g_warning ("Cancelling command %d\n", cmd->type);
					dma_debugger_queue_cancel (this);
					continue;
				}
				break;
			case IANJUTA_DEBUGGER_PROGRAM_LOADED:
				if (cmd->type <= DEBUGGER_STOPPED_OR_LESSER)
				{
					/* Stop debugger first */
					cmd = dma_debugger_queue_prepend (this);
					cmd->type = QUIT_COMMAND;
				}
				else if (cmd->type <= DEBUGGER_STARTED_OR_LESSER)
				{
					/* Unload program first */
					cmd = dma_debugger_queue_prepend (this);
					cmd->type = UNLOAD_COMMAND;
				}
				else if (cmd->type >= PROGRAM_RUNNING_OR_GREATER)
				{
					/* Unable to run a program, so remove command */
					g_warning ("Cancelling command %d\n", cmd->type);
					dma_debugger_queue_cancel (this);
					continue;
				}
				break;
			case IANJUTA_DEBUGGER_PROGRAM_STOPPED:
				if (cmd->type <= DEBUGGER_STOPPED_OR_LESSER)
				{
					/* Stop debugger first */
					cmd = dma_debugger_queue_prepend (this);
					cmd->type = QUIT_COMMAND;
				}
				else if (cmd->type <= DEBUGGER_STARTED_OR_LESSER)
				{
					/* Unload program first */
					cmd = dma_debugger_queue_prepend (this);
					cmd->type = UNLOAD_COMMAND;
				}
				else if (cmd->type <= PROGRAM_LOADED_OR_LESSER)
				{
					/* Exit program first */
					cmd = dma_debugger_queue_prepend (this);
					cmd->type = EXIT_COMMAND;
				}
				else if (cmd->type >= PROGRAM_RUNNING_OR_GREATER)
				{
					/* Unable to run a program, so remove command */
					g_warning ("Cancelling command %d\n", cmd->type);
					dma_debugger_queue_cancel (this);
					continue;
				}
				break;
			case IANJUTA_DEBUGGER_PROGRAM_RUNNING:
				if (cmd->type <= PROGRAM_LOADED_OR_LESSER)
				{
					/* Interrupt program first */
					cmd = dma_debugger_queue_prepend (this);
					cmd->type = INTERRUPT_COMMAND;
				}
				else if (cmd->type <= PROGRAM_STOPPED_OR_LESSER)
				{
					/* Unable to interrupt than continue a program,
					 * so remove command */
					g_warning ("Cancelling command %d\n", cmd->type);
					dma_debugger_queue_cancel (this);
					continue;
				}
				break;
		}

		/* Start command */
		this->ready = FALSE;
		switch (cmd->type)
		{
		case DUMMY_COMMAND:
			break;
		case INITIALIZE_COMMAND:
		    //dma_debugger_create_view (this);
		    this->output_callback = cmd->init.callback;
		    this->output_data = cmd->init.user_data;
	 	    ianjuta_debugger_initialize (this->debugger, on_debugger_output, this, NULL);
//		    dma_debugger_update_status (this, ANJUTA_DEBUGGER_STARTED);
		    break;
		case LOAD_COMMAND:
			ianjuta_debugger_load (this->debugger, cmd->load.file, cmd->load.type, cmd->load.dirs, NULL);	
			break;
  	    case ATTACH_COMMAND:
			ianjuta_debugger_attach (this->debugger, cmd->attach.pid, cmd->load.dirs, NULL);	
			break;
		case UNLOAD_COMMAND:
		    ianjuta_debugger_unload (this->debugger, NULL);
		    break;
		case QUIT_COMMAND:
			ianjuta_debugger_quit (this->debugger, NULL);
			break;
		case START_COMMAND:
			ianjuta_debugger_start (this->debugger, cmd->start.args, cmd->start.terminal, NULL);
		    break;
		case RUN_COMMAND:
			ianjuta_debugger_run (this->debugger, NULL);	
			break;
		case RUN_TO_COMMAND:
			ianjuta_debugger_run_to (this->debugger, cmd->pos.file, cmd->pos.line, NULL);	
			break;
		case STEP_IN_COMMAND:
			ianjuta_debugger_step_in (this->debugger, NULL);	
			break;
		case STEP_OVER_COMMAND:
			ianjuta_debugger_step_over (this->debugger, NULL);	
			break;
		case STEP_OUT_COMMAND:
			ianjuta_debugger_step_out (this->debugger, NULL);	
			break;
		case RESTART_COMMAND:
			//ianjuta_debugger_restart (this->debugger, NULL);	
			break;
		case EXIT_COMMAND:
			ianjuta_debugger_exit (this->debugger, NULL);	
			break;
		case INTERRUPT_COMMAND:
			ianjuta_debugger_interrupt (this->debugger, NULL);	
			break;
		case ENABLE_BREAK_COMMAND:
			ianjuta_debugger_enable_breakpoint (this->debugger, cmd->brk.id, cmd->brk.enable == IANJUTA_DEBUGGER_YES ? TRUE : FALSE, cmd->brk.callback, cmd->brk.user_data, NULL);	
			break;
		case IGNORE_BREAK_COMMAND:
			ianjuta_debugger_ignore_breakpoint (this->debugger, cmd->brk.id, cmd->brk.ignore, cmd->brk.callback, cmd->brk.user_data, NULL);	
			break;
		case REMOVE_BREAK_COMMAND:
			ianjuta_debugger_clear_breakpoint (this->debugger, cmd->brk.id, cmd->brk.callback, cmd->brk.user_data, NULL);	
			break;
		case INSPECT_COMMAND:
			ianjuta_debugger_inspect (this->debugger, cmd->watch.name, cmd->watch.callback, cmd->watch.user_data, NULL);
		    break;
		case EVALUATE_COMMAND:
			ianjuta_debugger_evaluate (this->debugger, cmd->watch.name, cmd->watch.value, cmd->watch.callback, cmd->watch.user_data, NULL);
		    break;
		case LIST_LOCAL_COMMAND:
			ianjuta_debugger_list_local (this->debugger, cmd->info.callback, cmd->info.user_data, NULL);	
			break;
		case INFO_SIGNAL_COMMAND:
			ianjuta_debugger_info_signal (this->debugger, cmd->info.callback, cmd->info.user_data, NULL);	
			break;
		case INFO_SHAREDLIB_COMMAND:
			ianjuta_debugger_info_sharedlib (this->debugger, cmd->info.callback, cmd->info.user_data, NULL);	
			break;
		case INFO_FRAME_COMMAND:
			ianjuta_debugger_info_signal (this->debugger, cmd->info.callback, cmd->info.user_data, NULL);	
			break;
		case INFO_ARGS_COMMAND:
			ianjuta_debugger_info_args (this->debugger, cmd->info.callback, cmd->info.user_data, NULL);	
			break;
		case INFO_TARGET_COMMAND:
			ianjuta_debugger_info_target (this->debugger, cmd->info.callback, cmd->info.user_data, NULL);	
			break;
		case INFO_PROGRAM_COMMAND:
			ianjuta_debugger_info_program (this->debugger, cmd->info.callback, cmd->info.user_data, NULL);	
			break;
		case INFO_UDOT_COMMAND:
			ianjuta_debugger_info_udot (this->debugger, cmd->info.callback, cmd->info.user_data, NULL);	
			break;
		case INFO_THREADS_COMMAND:
			ianjuta_debugger_info_threads (this->debugger, cmd->info.callback, cmd->info.user_data, NULL);	
			break;
		case INFO_VARIABLES_COMMAND:
			ianjuta_debugger_info_variables (this->debugger, cmd->info.callback, cmd->info.user_data, NULL);	
			break;
		case SET_FRAME_COMMAND:
			ianjuta_debugger_set_frame (this->debugger, cmd->frame.frame, NULL);	
			break;
		case LIST_FRAME_COMMAND:
			ianjuta_debugger_list_frame (this->debugger, cmd->frame.callback, cmd->frame.user_data, NULL);	
			break;
		case LIST_REGISTER_COMMAND:
			ianjuta_cpu_debugger_list_register (this->cpu_debugger, cmd->info.callback, cmd->info.user_data, NULL);	
			break;
		case UPDATE_REGISTER_COMMAND:
			ianjuta_cpu_debugger_update_register (this->cpu_debugger, cmd->info.callback, cmd->info.user_data, NULL);	
			break;
		case WRITE_REGISTER_COMMAND:
			reg.num = cmd->watch.id;
		    reg.name = cmd->watch.name;
		    reg.value = cmd->watch.value;
			ianjuta_cpu_debugger_write_register (this->cpu_debugger, &reg, NULL);	
			break;
		case INSPECT_MEMORY_COMMAND:
			ianjuta_cpu_debugger_inspect_memory (this->debugger, cmd->mem.address, cmd->mem.length, cmd->mem.callback, cmd->mem.user_data, NULL);	
			break;
		case BREAK_LINE_COMMAND:
			ianjuta_debugger_set_breakpoint_at_line (this->debugger, cmd->pos.file, cmd->pos.line, cmd->pos.callback, cmd->pos.user_data, NULL);	
			break;
		case BREAK_FUNCTION_COMMAND:
			ianjuta_debugger_set_breakpoint_at_function (this->debugger, cmd->pos.file, cmd->pos.function, cmd->pos.callback, cmd->pos.user_data, NULL);	
			break;
		case BREAK_ADDRESS_COMMAND:
			ianjuta_debugger_set_breakpoint_at_address (this->debugger, cmd->pos.address, cmd->pos.callback, cmd->pos.user_data, NULL);	
			break;
		case CONDITION_BREAK_COMMAND:
			ianjuta_debugger_condition_breakpoint (this->debugger, cmd->brk.id, cmd->brk.condition, cmd->brk.callback, cmd->brk.user_data, NULL);	
			break;
		case USER_COMMAND:
			ianjuta_debugger_send_command (this->debugger, cmd->user.cmd, NULL);	
			break;
		case PRINT_COMMAND:
			ianjuta_debugger_print (this->debugger, cmd->print.var, cmd->print.callback, cmd->print.user_data, NULL);	
			break;
		case HANDLE_SIGNAL_COMMAND:
			ianjuta_debugger_handle_signal (this->debugger, cmd->signal.name, cmd->signal.stop, cmd->signal.print, cmd->signal.ignore, NULL);	
			break;
	    }
		
	}
}

/* Public message functions
 *---------------------------------------------------------------------------*/

void
dma_debugger_message (DmaDebuggerQueue *this, const gchar* message)
{
//	dma_debugger_print_message (this, IANJUTA_MESSAGE_VIEW_TYPE_NORMAL, message);
}

void
dma_debugger_info (DmaDebuggerQueue *this, const gchar* message)
{
//	dma_debugger_print_message (this, IANJUTA_MESSAGE_VIEW_TYPE_INFO, message);
}

void dma_debugger_warning (DmaDebuggerQueue *this, const gchar* message)
{
//	dma_debugger_print_message (this, IANJUTA_MESSAGE_VIEW_TYPE_WARNING, message);
}

void dma_debugger_error (DmaDebuggerQueue *this, const gchar* message)
{
//	dma_debugger_print_message (this, IANJUTA_MESSAGE_VIEW_TYPE_ERROR, message);
}

/* IAnjutaDebugger callback
 *---------------------------------------------------------------------------*/

static void
on_dma_debugger_ready (DmaDebuggerQueue *this, IAnjutaDebuggerStatus status)
{
	dma_debugger_update_status (this, status);
	if (this->ready) dma_debugger_queue_execute (this);
}

static void
on_dma_debugger_started (DmaDebuggerQueue *this)
{
	/* Nothing do to */
	dma_debugger_update_status (this, IANJUTA_DEBUGGER_STARTED);
	if (this->ready) dma_debugger_queue_execute (this);
}

static void
on_dma_debugger_stopped (DmaDebuggerQueue *this)
{
	/* Nothing do to */
	dma_debugger_update_status (this, IANJUTA_DEBUGGER_STOPPED);
	dma_debugger_queue_clear (this);
	this->ready = TRUE;
}

static void
on_dma_program_loaded (DmaDebuggerQueue *this)
{
	/* Nothing do to */
	dma_debugger_update_status (this, IANJUTA_DEBUGGER_PROGRAM_LOADED);
	if (this->ready) dma_debugger_queue_execute (this);
}

static void
on_dma_program_running (DmaDebuggerQueue *this)
{
	/* Nothing do to */
	dma_debugger_update_status (this, IANJUTA_DEBUGGER_PROGRAM_RUNNING);
}

static void
on_dma_program_stopped (DmaDebuggerQueue *this)
{
	/* Nothing do to */
	dma_debugger_update_status (this, IANJUTA_DEBUGGER_PROGRAM_STOPPED);
	if (this->ready) dma_debugger_queue_execute (this);
}

static void
on_dma_program_exited (DmaDebuggerQueue *this)
{
	/* Nothing do to */
	dma_debugger_update_status (this, IANJUTA_DEBUGGER_PROGRAM_LOADED);
	if (this->ready) dma_debugger_queue_execute (this);
}

static void
on_dma_location_changed (DmaDebuggerQueue *this, const gchar* src_path, guint line, const gchar* address)
{
	g_signal_emit_by_name (this, "location-changed", src_path, line, address);
}

static void
on_dma_frame_changed (DmaDebuggerQueue *this, guint frame)
{
	g_signal_emit_by_name (this, "frame-changed", frame);
}

static void
on_dma_sharedlib_event (DmaDebuggerQueue *this)
{
	this->stop_on_sharedlib = TRUE;
	dma_debugger_update_status (this, IANJUTA_DEBUGGER_PROGRAM_STOPPED);
	g_signal_emit_by_name (this, "sharedlib-event");
	ianjuta_debugger_run (IANJUTA_DEBUGGER (this), NULL);
}

/* Implementation of IAnjutaDebugger interface
 *---------------------------------------------------------------------------*/

static IAnjutaDebuggerStatus
idebugger_get_status (IAnjutaDebugger *iface, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;

	return this->status;
}

static IAnjutaDebuggerError
idebugger_initialize (IAnjutaDebugger *iface, IAnjutaDebuggerOutputFunc callback, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;

	cmd = dma_debugger_queue_append (this);
	cmd->type = INITIALIZE_COMMAND;
	cmd->init.callback = callback;
	cmd->init.user_data = user_data;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_load (IAnjutaDebugger *iface, const gchar *file, const gchar* mime_type,
				const GList *search_dirs, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	const GList *node;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = LOAD_COMMAND;
	cmd->load.file = g_strdup (file);
	cmd->load.type = g_strdup (mime_type);
	cmd->load.dirs = NULL;
	for (node = search_dirs; node != NULL; node = g_list_next (node))
	{
		cmd->load.dirs = g_list_prepend (cmd->load.dirs, g_strdup (node->data));
	}
	cmd->load.dirs = g_list_reverse (cmd->load.dirs);

	dma_debugger_queue_execute (this);

/*	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;

	if (this->debugger == NULL) return ANJUTA_DEBUGGER_NOT_CONNECTED;

	dma_debugger_create_view (this);
	ianjuta_debugger_initialize (this->debugger, on_debugger_output, this, NULL);
		
	return ianjuta_debugger_load (this->debugger, file, mime_type, search_dirs, err);	*/
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_attach (IAnjutaDebugger *iface, pid_t pid, const GList *search_dirs, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	const GList *node;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = ATTACH_COMMAND;
	cmd->attach.pid = pid;
	cmd->attach.dirs = NULL;
	for (node = search_dirs; node != NULL; node = g_list_next (node))
	{
		cmd->attach.dirs = g_list_prepend (cmd->attach.dirs, g_strdup (node->data));
	}
	cmd->attach.dirs = g_list_reverse (cmd->attach.dirs);
	
	dma_debugger_queue_execute (this);

	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_start (IAnjutaDebugger *iface, const gchar *args, gboolean terminal, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;

	cmd = dma_debugger_queue_append (this);
	cmd->type = START_COMMAND;
	cmd->start.args = args == NULL ? NULL : g_strdup (args);
	cmd->start.terminal = terminal;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_unload (IAnjutaDebugger *iface, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = UNLOAD_COMMAND;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_quit (IAnjutaDebugger *iface, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = QUIT_COMMAND;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_run (IAnjutaDebugger *iface, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = RUN_COMMAND;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_step_in (IAnjutaDebugger *iface, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = STEP_IN_COMMAND;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_step_over (IAnjutaDebugger *iface, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = STEP_OVER_COMMAND;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_run_to (IAnjutaDebugger *iface, const gchar *file,
						   gint line, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = RUN_TO_COMMAND;
	cmd->pos.file = g_strdup (file);
	cmd->pos.line = line;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_step_out (IAnjutaDebugger *iface, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = STEP_OUT_COMMAND;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_exit (IAnjutaDebugger *iface, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = EXIT_COMMAND;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_interrupt (IAnjutaDebugger *iface, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = INTERRUPT_COMMAND;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_add_breakpoint_at_line (IAnjutaDebugger *iface, const gchar* file, guint line, IAnjutaDebuggerBreakpointFunc callback, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = BREAK_LINE_COMMAND;
	cmd->pos.file = g_strdup (file);
	cmd->pos.line = line;
	cmd->pos.callback = callback;
	cmd->pos.user_data = user_data;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_add_breakpoint_at_function (IAnjutaDebugger *iface, const gchar* file, const gchar* function, IAnjutaDebuggerBreakpointFunc callback, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = BREAK_FUNCTION_COMMAND;
	cmd->pos.file = g_strdup (file);
	cmd->pos.function = g_strdup (function);
	cmd->pos.callback = callback;
	cmd->pos.user_data = user_data;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_add_breakpoint_at_address (IAnjutaDebugger *iface, guint address, IAnjutaDebuggerBreakpointFunc callback, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = BREAK_ADDRESS_COMMAND;
	cmd->pos.address = address;
	cmd->pos.callback = callback;
	cmd->pos.user_data = user_data;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_enable_breakpoint (IAnjutaDebugger *iface, guint id, gboolean enable, IAnjutaDebuggerBreakpointFunc callback, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = ENABLE_BREAK_COMMAND;
	cmd->brk.id = id;
	cmd->brk.enable = enable;
	cmd->brk.callback = callback;
	cmd->brk.user_data = user_data;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_ignore_breakpoint (IAnjutaDebugger *iface, guint id, guint ignore, IAnjutaDebuggerBreakpointFunc callback, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = IGNORE_BREAK_COMMAND;
	cmd->brk.id = id;
	cmd->brk.ignore = ignore;
	cmd->brk.callback = callback;
	cmd->brk.user_data = user_data;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_condition_breakpoint (IAnjutaDebugger *iface, guint id, const gchar *condition, IAnjutaDebuggerBreakpointFunc callback, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = CONDITION_BREAK_COMMAND;
	cmd->brk.id = id;
	cmd->brk.condition = g_strdup (condition);
	cmd->brk.callback = callback;
	cmd->brk.user_data = user_data;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_remove_breakpoint (IAnjutaDebugger *iface, guint id, IAnjutaDebuggerBreakpointFunc callback, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = REMOVE_BREAK_COMMAND;
	cmd->brk.id = id;
	cmd->brk.callback = callback;
	cmd->brk.user_data = user_data;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_inspect (IAnjutaDebugger *iface, const gchar *expression, IAnjutaCpuDebuggerMemoryCallBack callback, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = INSPECT_COMMAND;
	cmd->watch.name = g_strdup (expression);
	cmd->watch.callback = callback;
	cmd->watch.user_data = user_data;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_evaluate (IAnjutaDebugger *iface, const gchar *name, const gchar* value, IAnjutaDebuggerGCharFunc callback, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = EVALUATE_COMMAND;
	cmd->watch.name = g_strdup (name);
	cmd->watch.value = g_strdup (value);
	cmd->watch.callback = callback;
	cmd->watch.user_data = user_data;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_send_command (IAnjutaDebugger *iface, const gchar* command, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = USER_COMMAND;
	cmd->user.cmd = g_strdup (command);
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError 
idebugger_print (IAnjutaDebugger *iface, const gchar* variable, IAnjutaDebuggerGCharFunc callback, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = PRINT_COMMAND;
	cmd->print.var = g_strdup (variable);
	cmd->print.callback = callback;
	cmd->print.user_data = user_data;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_list_local (IAnjutaDebugger *iface, IAnjutaDebuggerGListFunc callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = LIST_LOCAL_COMMAND;
	cmd->info.callback = callback;
	cmd->info.user_data = user_data;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_info_signal (IAnjutaDebugger *iface, IAnjutaDebuggerGListFunc callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = INFO_SIGNAL_COMMAND;
	cmd->info.callback = callback;
	cmd->info.user_data = user_data;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_info_sharedlib (IAnjutaDebugger *iface, IAnjutaDebuggerGListFunc callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = INFO_SHAREDLIB_COMMAND;
	cmd->info.callback = callback;
	cmd->info.user_data = user_data;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_handle_signal (IAnjutaDebugger *iface, const gchar* name, gboolean stop, gboolean print, gboolean ignore, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = HANDLE_SIGNAL_COMMAND;
	cmd->signal.name = g_strdup (name);
	cmd->signal.stop = stop;
	cmd->signal.print = print;
	cmd->signal.ignore = ignore;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_info_frame (IAnjutaDebugger *iface, guint frame, IAnjutaDebuggerGListFunc callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = INFO_FRAME_COMMAND;
	cmd->frame.frame = frame;
	cmd->frame.callback = callback;
	cmd->frame.user_data = user_data;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_info_args (IAnjutaDebugger *iface, IAnjutaDebuggerGListFunc callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = INFO_ARGS_COMMAND;
	cmd->frame.callback = callback;
	cmd->frame.user_data = user_data;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_info_target (IAnjutaDebugger *iface, IAnjutaDebuggerGListFunc callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = INFO_TARGET_COMMAND;
	cmd->info.callback = callback;
	cmd->info.user_data = user_data;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_info_program (IAnjutaDebugger *iface, IAnjutaDebuggerGListFunc callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = INFO_PROGRAM_COMMAND;
	cmd->info.callback = callback;
	cmd->info.user_data = user_data;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_info_udot (IAnjutaDebugger *iface, IAnjutaDebuggerGListFunc callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = INFO_UDOT_COMMAND;
	cmd->info.callback = callback;
	cmd->info.user_data = user_data;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_info_threads (IAnjutaDebugger *iface, IAnjutaDebuggerGListFunc callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = INFO_THREADS_COMMAND;
	cmd->info.callback = callback;
	cmd->info.user_data = user_data;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_info_variables (IAnjutaDebugger *iface, IAnjutaDebuggerGListFunc callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = INFO_VARIABLES_COMMAND;
	cmd->info.callback = callback;
	cmd->info.user_data = user_data;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_set_frame (IAnjutaDebugger *iface, guint frame, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;

	cmd = dma_debugger_queue_append (this);
	cmd->type = SET_FRAME_COMMAND;
	cmd->frame.frame = frame;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_list_frame (IAnjutaDebugger *iface, IAnjutaDebuggerGListFunc callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = LIST_FRAME_COMMAND;
	cmd->frame.callback = callback;
	cmd->frame.user_data = user_data;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
idebugger_list_register (IAnjutaDebugger *iface, IAnjutaDebuggerGListFunc callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = LIST_REGISTER_COMMAND;
	cmd->info.callback = callback;
	cmd->info.user_data = user_data;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static void
idebugger_enable_log (IAnjutaDebugger *iface, IAnjutaMessageView *log, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;

	ianjuta_debugger_enable_log (this->debugger, log, err);
}

static void
idebugger_disable_log (IAnjutaDebugger *iface, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;

	ianjuta_debugger_disable_log (this->debugger, err);
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
	iface->info_frame = idebugger_info_frame;
	iface->info_signal = idebugger_info_signal;
	iface->info_sharedlib = idebugger_info_sharedlib;
	iface->info_args = idebugger_info_args;
	iface->info_target = idebugger_info_target;
	iface->info_program = idebugger_info_program;
	iface->info_udot = idebugger_info_udot;
	iface->info_threads = idebugger_info_threads;
	iface->info_variables = idebugger_info_variables;
	iface->handle_signal = idebugger_handle_signal;
	iface->list_frame = idebugger_list_frame;
	iface->set_frame = idebugger_set_frame;
	iface->list_register = idebugger_list_register;

	iface->enable_log = idebugger_enable_log;
	iface->disable_log = idebugger_disable_log;
	
	iface->send_command = idebugger_send_command;

}

/* Implementation of IAnjutaDebugger interface
 *---------------------------------------------------------------------------*/

static IAnjutaDebuggerError
icpu_debugger_list_register (IAnjutaCpuDebugger *iface, IAnjutaCpuDebuggerGListFunc callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = LIST_REGISTER_COMMAND;
	cmd->info.callback = callback;
	cmd->info.user_data = user_data;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}


static IAnjutaDebuggerError
icpu_debugger_update_register (IAnjutaCpuDebugger *iface, IAnjutaCpuDebuggerGListFunc callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = UPDATE_REGISTER_COMMAND;
	cmd->info.callback = callback;
	cmd->info.user_data = user_data;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

static IAnjutaDebuggerError
icpu_debugger_write_register (IAnjutaCpuDebugger *iface, IAnjutaCpuDebuggerRegister *value, gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = WRITE_REGISTER_COMMAND;
	cmd->watch.id = value->num;
	cmd->watch.name = value->name != NULL ? g_strdup (value->name) : NULL;
	cmd->watch.value = value->value != NULL ? g_strdup (value->value) : NULL;
	dma_debugger_queue_execute (this);
	
	return IANJUTA_DEBUGGER_OK;
}

void
icpu_debugger_inspect_memory (IAnjutaDebugger *iface, const void *address, guint length, IAnjutaDebuggerGCharFunc callback , gpointer user_data, GError **err)
{
	DmaDebuggerQueue *this = (DmaDebuggerQueue *)iface;
	DmaDebuggerCommand *cmd;
	
	cmd = dma_debugger_queue_append (this);
	cmd->type = INSPECT_MEMORY_COMMAND;
	cmd->mem.address = address;
	cmd->mem.length = length;
	cmd->mem.callback = callback;
	cmd->mem.user_data = user_data;
	dma_debugger_queue_execute (this);
}

static void
icpu_debugger_iface_init (IAnjutaCpuDebuggerIface *iface)
{
	iface->list_register = icpu_debugger_list_register;
	iface->update_register = icpu_debugger_update_register;
	iface->write_register = icpu_debugger_write_register;
	iface->inspect_memory = icpu_debugger_inspect_memory;
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
	DmaDebuggerQueue *this = DMA_DEBUGGER_QUEUE (obj);
	
/*	if (this->view != NULL)
	{
		g_object_remove_weak_pointer (G_OBJECT (this->view), (gpointer*)&this->view);
        this->view = NULL;
	}*/
	dma_debugger_queue_clear (this);

	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

/* finalize is the last destruction step. It must free all memory allocated
 * with instance_init. It is called only one time just before releasing all
 * memory */

static void
dma_debugger_queue_finalize (GObject *obj)
{
	/*DmaDebuggerQueue *this = DMA_DEBUGGER_QUEUE (obj);*/

	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

/* instance_init is the constructor. All functions should work after this
 * call. */

static void
dma_debugger_queue_instance_init (DmaDebuggerQueue *this)
{
	this->plugin = NULL;
	this->debugger = NULL;
	this->cpu_debugger = NULL;
	this->head = NULL;
	this->ready = TRUE;
	this->status = IANJUTA_DEBUGGER_STOPPED;
	this->view = NULL;
	this->output_callback = NULL;
}

/* class_init intialize the class itself not the instance */

static void
dma_debugger_queue_class_init (DmaDebuggerQueueClass * klass)
{
	GObjectClass *object_class;

	g_return_if_fail (klass != NULL);
	object_class = (GObjectClass *) klass;
	
	DEBUG_PRINT ("Initializing debugger class");
	
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

		type = g_type_register_static (G_TYPE_OBJECT,
		                            "DmaDebuggerQueue", &type_info, 0);

		g_type_add_interface_static (type, IANJUTA_TYPE_DEBUGGER,
						&iface_debugger_info);
		g_type_add_interface_static (type, IANJUTA_TYPE_CPU_DEBUGGER,
						&iface_cpu_debugger_info);
	}
	
	return type;
}

/* Creation and Destruction
 *---------------------------------------------------------------------------*/

DmaDebuggerQueue*
dma_debugger_queue_new (AnjutaPlugin *plugin)
{
	DmaDebuggerQueue *this;

	this = g_object_new (DMA_DEBUGGER_QUEUE_TYPE, NULL);
	
	this->plugin = plugin;
	/* Query for object implementing IAnjutaDebugger interface */
	this->debugger = anjuta_shell_get_interface (plugin->shell, IAnjutaDebugger, NULL);
	this->cpu_debugger = anjuta_shell_get_interface (plugin->shell, IAnjutaCpuDebugger, NULL);

	/* Connect signal */
	if (this->debugger)
	{
		g_signal_connect_swapped (this->debugger, "debugger-ready", G_CALLBACK (on_dma_debugger_ready), this);
		g_signal_connect_swapped (this->debugger, "debugger-started", G_CALLBACK (on_dma_debugger_started), this);
		g_signal_connect_swapped (this->debugger, "debugger-stopped", G_CALLBACK (on_dma_debugger_stopped), this);
		g_signal_connect_swapped (this->debugger, "program-loaded", G_CALLBACK (on_dma_program_loaded), this);
		g_signal_connect_swapped (this->debugger, "program-running", G_CALLBACK (on_dma_program_running), this);
		g_signal_connect_swapped (this->debugger, "program-stopped", G_CALLBACK (on_dma_program_stopped), this);
		g_signal_connect_swapped (this->debugger, "program-exited", G_CALLBACK (on_dma_program_exited), this);
		g_signal_connect_swapped (this->debugger, "location-changed", G_CALLBACK (on_dma_location_changed), this);
		g_signal_connect_swapped (this->debugger, "frame-changed", G_CALLBACK (on_dma_frame_changed), this);
		g_signal_connect_swapped (this->debugger, "sharedlib-event", G_CALLBACK (on_dma_sharedlib_event), this);
	}
	
	return this;
}
	
void
dma_debugger_queue_free (DmaDebuggerQueue *this)
{
	/* Disconnect signal */
	if (this->debugger)
	{
		g_signal_handlers_disconnect_by_func (this->debugger, G_CALLBACK (on_dma_debugger_ready), this);
		g_signal_handlers_disconnect_by_func (this->debugger, G_CALLBACK (on_dma_debugger_started), this);
		g_signal_handlers_disconnect_by_func (this->debugger, G_CALLBACK (on_dma_debugger_stopped), this);
		g_signal_handlers_disconnect_by_func (this->debugger, G_CALLBACK (on_dma_program_loaded), this);
		g_signal_handlers_disconnect_by_func (this->debugger, G_CALLBACK (on_dma_program_running), this);
		g_signal_handlers_disconnect_by_func (this->debugger, G_CALLBACK (on_dma_program_stopped), this);
		g_signal_handlers_disconnect_by_func (this->debugger, G_CALLBACK (on_dma_program_exited), this);
		g_signal_handlers_disconnect_by_func (this->debugger, G_CALLBACK (on_dma_location_changed), this);
		g_signal_handlers_disconnect_by_func (this->debugger, G_CALLBACK (on_dma_sharedlib_event), this);
	}
	g_object_unref (this);
}
