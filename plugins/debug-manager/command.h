/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    command.h
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

#ifndef COMMAND_H
#define COMMAND_H

/*---------------------------------------------------------------------------*/

#include "plugin.h"

#include <libanjuta/interfaces/ianjuta-debugger.h>

#include <glib.h>

typedef struct _DmaQueueCommand DmaQueueCommand;

typedef enum
{
	CANCEL_IF_PROGRAM_RUNNING = 1 << 21,
	CANCEL_ALL_COMMAND = 1 << 22,
	ASYNCHRONOUS = 1 << 23
} DmaCommandFlag;	

/* Create a new command structure and append to command queue */
gboolean dma_queue_initialize (DmaDebuggerQueue *self);
gboolean dma_queue_load (DmaDebuggerQueue *self, const gchar *file, const gchar* mime_type, const GList *search_dirs);
gboolean dma_queue_attach (DmaDebuggerQueue *self, pid_t pid, const GList *search_dirs);
gboolean dma_queue_start (DmaDebuggerQueue *self, const gchar *args, gboolean terminal);
gboolean dma_queue_unload (DmaDebuggerQueue *self);
gboolean dma_queue_quit (DmaDebuggerQueue *self);
gboolean dma_queue_abort (DmaDebuggerQueue *self);
gboolean dma_queue_run (DmaDebuggerQueue *self);
gboolean dma_queue_step_in (DmaDebuggerQueue *self);
gboolean dma_queue_step_over (DmaDebuggerQueue *self);
gboolean dma_queue_run_to (DmaDebuggerQueue *self, const gchar *file, gint line);
gboolean dma_queue_step_out (DmaDebuggerQueue *self);
gboolean dma_queue_exit (DmaDebuggerQueue *self);
gboolean dma_queue_interrupt (DmaDebuggerQueue *self);
gboolean dma_queue_inspect (DmaDebuggerQueue *self, const gchar *expression, IAnjutaDebuggerCallback callback, gpointer user_data);
gboolean dma_queue_evaluate (DmaDebuggerQueue *self, const gchar *name, const gchar* value, IAnjutaDebuggerCallback callback, gpointer user_data);
gboolean dma_queue_send_command (DmaDebuggerQueue *self, const gchar* command);
gboolean dma_queue_print (DmaDebuggerQueue *self, const gchar* variable, IAnjutaDebuggerCallback callback, gpointer user_data);
gboolean dma_queue_list_local (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data);
gboolean dma_queue_list_argument (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data);
gboolean dma_queue_list_thread (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data);
gboolean dma_queue_set_thread (DmaDebuggerQueue *self, gint thread);
gboolean dma_queue_info_thread (DmaDebuggerQueue *self, gint thread, IAnjutaDebuggerCallback callback , gpointer user_data);
gboolean dma_queue_info_signal (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data);
gboolean dma_queue_info_sharedlib (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data);
gboolean dma_queue_handle_signal (DmaDebuggerQueue *self, const gchar* name, gboolean stop, gboolean print, gboolean ignore);
gboolean dma_queue_info_frame (DmaDebuggerQueue *self, guint frame, IAnjutaDebuggerCallback callback , gpointer user_data);
gboolean dma_queue_info_args (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data);
gboolean dma_queue_info_target (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data);
gboolean dma_queue_info_program (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data);
gboolean dma_queue_info_udot (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data);
gboolean dma_queue_info_variables (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data);
gboolean dma_queue_set_frame (DmaDebuggerQueue *self, guint frame);
gboolean dma_queue_list_frame (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data);
gboolean dma_queue_list_register (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data);
gboolean dma_queue_callback (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data);
void dma_queue_enable_log (DmaDebuggerQueue *self, IAnjutaMessageView *log);
void dma_queue_disable_log (DmaDebuggerQueue *self);
gboolean dma_queue_add_breakpoint_at_line (DmaDebuggerQueue *self, const gchar* file, guint line, IAnjutaDebuggerCallback callback, gpointer user_data);
gboolean dma_queue_add_breakpoint_at_function (DmaDebuggerQueue *self, const gchar* file, const gchar* function, IAnjutaDebuggerCallback callback, gpointer user_data);
gboolean dma_queue_add_breakpoint_at_address (DmaDebuggerQueue *self, guint address, IAnjutaDebuggerCallback callback, gpointer user_data);
gboolean dma_queue_enable_breakpoint (DmaDebuggerQueue *self, guint id, gboolean enable, IAnjutaDebuggerCallback callback, gpointer user_data);
gboolean dma_queue_ignore_breakpoint (DmaDebuggerQueue *self, guint id, guint ignore, IAnjutaDebuggerCallback callback, gpointer user_data);
gboolean dma_queue_condition_breakpoint (DmaDebuggerQueue *self, guint id, const gchar *condition, IAnjutaDebuggerCallback callback, gpointer user_data);
gboolean dma_queue_remove_breakpoint (DmaDebuggerQueue *self, guint id, IAnjutaDebuggerCallback callback, gpointer user_data);
gboolean dma_queue_list_register (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback, gpointer user_data);
gboolean dma_queue_update_register (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback , gpointer user_data);
gboolean dma_queue_write_register (DmaDebuggerQueue *self, IAnjutaDebuggerRegister *value);
gboolean dma_queue_inspect_memory (DmaDebuggerQueue *self, guint address, guint length, IAnjutaDebuggerCallback callback , gpointer user_data);
gboolean dma_queue_disassemble (DmaDebuggerQueue *self, guint address, guint length, IAnjutaDebuggerCallback callback , gpointer user_data);
gboolean dma_queue_delete_variable (DmaDebuggerQueue *self, const gchar *name);
gboolean dma_queue_evaluate_variable (DmaDebuggerQueue *self, const gchar *name, IAnjutaDebuggerCallback callback, gpointer user_data);
gboolean dma_queue_assign_variable (DmaDebuggerQueue *self, const gchar *name, const gchar *value);
gboolean dma_queue_list_children (DmaDebuggerQueue *self, const gchar *name, IAnjutaDebuggerCallback callback, gpointer user_data);
gboolean dma_queue_create_variable (DmaDebuggerQueue *self, const gchar *name, IAnjutaDebuggerCallback callback, gpointer user_data);
gboolean dma_queue_update_variable (DmaDebuggerQueue *self, IAnjutaDebuggerCallback callback, gpointer user_data);

void dma_command_free (DmaQueueCommand *cmd);

void dma_command_cancel (DmaQueueCommand *cmd);
gboolean dma_command_run (DmaQueueCommand *cmd, IAnjutaDebugger *debugger, GError **error);

gboolean dma_command_is_valid_in_state (DmaQueueCommand *cmd, IAnjutaDebuggerState state);
IAnjutaDebuggerState dma_command_is_going_to_state (DmaQueueCommand *cmd);
gboolean dma_command_has_flag (DmaQueueCommand *cmd, DmaCommandFlag flag);

int dma_command_get_type (DmaQueueCommand *cmd);

#endif
