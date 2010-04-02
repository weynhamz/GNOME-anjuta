/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2007 <jrliggett@cox.net>
 * 
 * anjuta is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include "anjuta-command.h"

/**
 * SECTION: anjuta-command
 * @short_description: System for creating objects that provide a standard 
 *					   interface to external components (libraries, processes,
 *					   etc.) 
 * @see_also: #AnjutaAsyncCommand, #AnjutaSyncCommand
 * @include libanjuta/anjuta-command.h
 *
 * #AnjutaCommand is the base class for objects that are designed to provide 
 * a layer of abstraction between UI code and some other component, like a 
 * library or child process. AnjutaCommand provides a simple and consistent
 * interface for plugins to interact with these components without needing 
 * to concern themselves with the exact details of how these components work.
 * 
 * To create command objects, plugins derive them from an #AnjutaCommand 
 * subclass like #AnjutaAsyncCommand, which runs commands in another thread or 
 * #AnjutaSyncCommand, which runs commands synchronously.
 *
 * These classes determine how ::run is called and how signals are emitted.
 * ::run is responsible for actually doing the work of the command. It is the 
 * responsiblity of the command object that does a certain task to implement 
 * ::run to do its job. Everything else is normally implemented by its parent
 * classes at this point
 *
 * For an example of how to use #AnjutaCommand, see the Subversion and Git 
 * plugins.
 */

struct _AnjutaCommandPriv
{
	gboolean running;
	gchar *error_message;
};

enum
{
	DATA_ARRIVED,
	COMMAND_STARTED,
	COMMAND_FINISHED,
	PROGRESS,

	LAST_SIGNAL
};


static guint anjuta_command_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (AnjutaCommand, anjuta_command, G_TYPE_OBJECT);

static void
anjuta_command_init (AnjutaCommand *self)
{
	self->priv = g_new0 (AnjutaCommandPriv, 1);
}

static void
anjuta_command_finalize (GObject *object)
{
	AnjutaCommand *self;
	
	self = ANJUTA_COMMAND (object);
	
	g_free (self->priv->error_message);
	g_free (self->priv);

	G_OBJECT_CLASS (anjuta_command_parent_class)->finalize (object);
}

static gboolean
start_automatic_monitor (AnjutaCommand *self)
{
	return FALSE;
}

static void
stop_automatic_monitor (AnjutaCommand *self)
{
}

static void
data_arrived (AnjutaCommand *command)
{
}

static void
command_started (AnjutaCommand *command)
{
	command->priv->running = TRUE;
}

static void
command_finished (AnjutaCommand *command, guint return_code)
{
	command->priv->running = FALSE;
}

static void
progress (AnjutaCommand *command, gfloat progress)
{
}

static void
anjuta_command_class_init (AnjutaCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = anjuta_command_finalize;
	
	klass->run = NULL;
	klass->start = NULL;
	klass->cancel = NULL;
	klass->notify_data_arrived = NULL;
	klass->notify_complete = NULL;
	klass->notify_progress = NULL;
	klass->set_error_message = anjuta_command_set_error_message;
	klass->get_error_message = anjuta_command_get_error_message;
	klass->start_automatic_monitor = start_automatic_monitor;
	klass->stop_automatic_monitor = stop_automatic_monitor;
	klass->data_arrived = data_arrived;
	klass->command_started = command_started;
	klass->command_finished = command_finished;
	klass->progress = progress;

	/**
	 * AnjutaCommand::data-arrived:
	 * @command: Command
	 * 
	 * Notifies clients that the command has processed data that is ready to 
	 * be used.
	 */
	anjuta_command_signals[DATA_ARRIVED] =
		g_signal_new ("data-arrived",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (AnjutaCommandClass, data_arrived),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 
					  0);

	/**
	 * AnjuaCommand::command-started:
	 * @command: Command
	 *
	 * Indicates that a command has begun executing. This signal is intended to 
	 * be used for commands that start themselves automatically.
	 *
	 * <note>
	 *  <para>
	 *	  Sublasses that override the method for this signal should chain up to
	 *	  the parent implementation to ensure proper handling of running/not 
	 *	  running states. 
	 *	</para>
	 * </note>
	 */
	anjuta_command_signals[COMMAND_STARTED] =
		g_signal_new ("command-started",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (AnjutaCommandClass, command_started),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 
		              0);

	/**
	 * AnjutaCommand::command-finished:
	 * @command: Command
	 * @return_code: The return code of the finished commmand
	 *
	 * Indicates that the command has completed. Clients should at least handle
	 * this signal to unref the command object.
	 *
	 * <note>
	 *  <para>
	 *	  Sublasses that override the method for this signal should chain up to
	 *	  the parent implementation to ensure proper handling of running/not 
	 *	  running states. 
	 *	</para>
	 * </note>
	 */
	anjuta_command_signals[COMMAND_FINISHED] =
		g_signal_new ("command-finished",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (AnjutaCommandClass, command_finished),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__UINT ,
		              G_TYPE_NONE, 1,
		              G_TYPE_UINT);
	
	
	/**
	 * AnjutaCommand::progress:
	 * @command: Command
	 * @progress: Fraction of the command's task that is complete, between 0.0
	 *			  and 1.0, inclusive.
	 *
	 * Notifies clients of changes in progress during command execution. 
	 */
	anjuta_command_signals[PROGRESS] =
		g_signal_new ("progress",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (AnjutaCommandClass, progress),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__FLOAT ,
		              G_TYPE_NONE, 1,
		              G_TYPE_FLOAT);
}

/**
 * anjuta_command_start:
 * @self: Command object to start
 *
 * Starts a command. Client code can handle data from the command by connecting
 * to the ::data-arrived signal. 
 *
 * #AnjutaCommand subclasses should override this method to determine how they
 * call ::run, which actually does the command's legwork. 
 */
void
anjuta_command_start (AnjutaCommand *self)
{
	g_signal_emit_by_name (self, "command-started");

	ANJUTA_COMMAND_GET_CLASS (self)->start (self);
}

/**
 * anjuta_command_cancel:
 * @self: Command object.
 *
 * Cancels a running command.
 */
void
anjuta_command_cancel (AnjutaCommand *self)
{
	ANJUTA_COMMAND_GET_CLASS (self)->cancel (self);
}

/**
 * anjuta_command_notify_data_arrived:
 * @self: Command object.
 * 
 * Used by base classes derived from #AnjutaCommand to emit the ::data-arrived
 * signal. This method should be used by both base command classes and
 * non-base classes as appropriate. 
 */
void
anjuta_command_notify_data_arrived (AnjutaCommand *self)
{
	ANJUTA_COMMAND_GET_CLASS (self)->notify_data_arrived (self);
}

/**
 * anjuta_command_notify_complete:
 * @self: Command object.
 * 
 * Used by base classes derived from #AnjutaCommand to emit the 
 * ::command-finished signal. This method should not be used by client code or  
 * #AnjutaCommand objects that are not base classes. 
 */
void
anjuta_command_notify_complete (AnjutaCommand *self, guint return_code)
{
	ANJUTA_COMMAND_GET_CLASS (self)->notify_complete (self, return_code);
}

/**
 * anjuta_command_notify_progress:
 * @self: Command object.
 * 
 * Emits the ::progress signal. Can be used by both base classes and 
 * commands as needed. 
 */
void 
anjuta_command_notify_progress (AnjutaCommand *self, gfloat progress)
{
	ANJUTA_COMMAND_GET_CLASS (self)->notify_progress (self, progress);
}

/**
 * anjuta_command_is_running:
 * @self: Command object.
 *
 * Return value: %TRUE if the command is currently running; %FALSE otherwise.
 */
gboolean
anjuta_command_is_running (AnjutaCommand *self)
{
	return self->priv->running;
}

/**
 * anjuta_command_set_error_message:
 * @self: Command object.
 * @error_message: Error message.
 * 
 * Command objects use this to set error messages when they encounter some kind
 * of failure. 
 */
void
anjuta_command_set_error_message (AnjutaCommand *self, gchar *error_message)
{
	if (self->priv->error_message)
		g_free (error_message);
	
	self->priv->error_message = g_strdup (error_message);
}

/**
 * anjuta_command_get_error_message:
 * @self: Command object.
 * 
 * Get the error message from the command, if there is one. This method is 
 * normally used from a ::command-finished handler to report errors to the user
 * when a command finishes. 
 *
 * Return value: Error message string that must be freed when no longer needed.
 * If no error is set, return %NULL.
 */
gchar *
anjuta_command_get_error_message (AnjutaCommand *self)
{
	return g_strdup (self->priv->error_message);
}

/**
 * anjuta_command_start_automatic_monitor:
 * @self: Command object.
 *
 * Sets up any monitoring needed for commands that should start themselves 
 * automatically in response to some event. 
 *
 * Return value: %TRUE if automatic starting is supported by the command and 
 * no errors were encountered; %FALSE if automatic starting is unsupported or on
 * error.
 */
gboolean
anjuta_command_start_automatic_monitor (AnjutaCommand *self)
{
	return ANJUTA_COMMAND_GET_CLASS (self)->start_automatic_monitor (self);
}

/**
 * anjuta_command_stop_automatic_monitor:
 * @self: Command object.
 *
 * Stops automatic monitoring for self executing commands. For commands that 
 * do not support self-starting, this function does nothing.
 */
void
anjuta_command_stop_automatic_monitor (AnjutaCommand *self)
{
	ANJUTA_COMMAND_GET_CLASS (self)->stop_automatic_monitor (self);
}