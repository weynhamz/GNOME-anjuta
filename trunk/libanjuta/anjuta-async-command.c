/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2007 <jrliggett@cox.net>
 *
 * Portions based on the original Subversion plugin 
 * Copyright (C) Johannes Schmid 2005 
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

#include "anjuta-async-command.h"

struct _AnjutaAsyncCommandPriv
{
	GMutex *mutex;
	guint return_code;
	gboolean complete;
	gboolean new_data_arrived;
};

G_DEFINE_TYPE (AnjutaAsyncCommand, anjuta_async_command, ANJUTA_TYPE_COMMAND);

static void
anjuta_async_command_init (AnjutaAsyncCommand *self)
{
	self->priv = g_new0 (AnjutaAsyncCommandPriv, 1);
	
	self->priv->mutex = g_mutex_new ();
}

static void
anjuta_async_command_finalize (GObject *object)
{
	AnjutaAsyncCommand *self;
	
	self = ANJUTA_ASYNC_COMMAND (object);
	
	g_mutex_free (self->priv->mutex);
	g_idle_remove_by_data (self);
	
	g_free (self->priv);

	G_OBJECT_CLASS (anjuta_async_command_parent_class)->finalize (object);
}

static gboolean
anjuta_async_command_notification_poll (AnjutaCommand *command)
{
	AnjutaAsyncCommand *self;
	
	self = ANJUTA_ASYNC_COMMAND (command);
	
	if (self->priv->new_data_arrived &&
		g_mutex_trylock (self->priv->mutex))
	{
		g_signal_emit_by_name (command, "data-arrived");
		g_mutex_unlock (self->priv->mutex);
		self->priv->new_data_arrived = FALSE;
	}
	
	if (self->priv->complete)
	{
		g_signal_emit_by_name (command, "command-finished", 
							   self->priv->return_code);
		return FALSE;
	}
	else
		return TRUE;
	
}

static gpointer
anjuta_async_command_thread (AnjutaCommand *command)
{
	guint return_code;
	
	return_code = ANJUTA_COMMAND_GET_CLASS (command)->run (command);
	anjuta_command_notify_complete (command, return_code);
	return NULL;
}

static void
start_command (AnjutaCommand *command)
{
	g_idle_add ((GSourceFunc) anjuta_async_command_notification_poll, 
				command);
	g_thread_create ((GThreadFunc) anjuta_async_command_thread, 
					 command, FALSE, NULL);
}

static void
notify_data_arrived (AnjutaCommand *command)
{
	AnjutaAsyncCommand *self;
	
	self = ANJUTA_ASYNC_COMMAND (command);
	
	self->priv->new_data_arrived = TRUE;
}

static void
notify_complete (AnjutaCommand *command, guint return_code)
{
	AnjutaAsyncCommand *self;
	
	self = ANJUTA_ASYNC_COMMAND (command);
	
	self->priv->complete = TRUE;
	self->priv->return_code = return_code;
}

static void
anjuta_async_command_class_init (AnjutaAsyncCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AnjutaCommandClass* parent_class = ANJUTA_COMMAND_CLASS (klass);

	object_class->finalize = anjuta_async_command_finalize;
	
	parent_class->start = start_command;
	parent_class->notify_data_arrived = notify_data_arrived;
	parent_class->notify_complete = notify_complete;
}

void
anjuta_async_command_set_error_message (AnjutaCommand *command, 
										gchar *error_message)
{
	anjuta_async_command_lock (ANJUTA_ASYNC_COMMAND (command));
	ANJUTA_COMMAND_GET_CLASS (command)->set_error_message (command, 
														   error_message);
	anjuta_async_command_unlock (ANJUTA_ASYNC_COMMAND (command));
}

gchar *
anjuta_async_command_get_error_message (AnjutaCommand *command)
{
	gchar *error_message;
	
	anjuta_async_command_lock (ANJUTA_ASYNC_COMMAND (command));
	error_message = ANJUTA_COMMAND_GET_CLASS (command)->get_error_message (command);
	anjuta_async_command_unlock (ANJUTA_ASYNC_COMMAND (command));
	
	return error_message;
}

void
anjuta_async_command_lock (AnjutaAsyncCommand *self)
{
	g_mutex_lock (self->priv->mutex);
}

void
anjuta_async_command_unlock (AnjutaAsyncCommand *self)
{
	g_mutex_unlock (self->priv->mutex);
}
