/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2009 <jrliggett@cox.net>
 * 
 * anjuta is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "anjuta-command-queue.h"

enum
{
	FINISHED,

	LAST_SIGNAL
};

static guint anjuta_command_queue_signals[LAST_SIGNAL] = { 0 };

struct _AnjutaCommandQueuePriv
{
	GQueue *queue;
	gboolean busy;
	AnjutaCommandQueueExecuteMode mode;
};

G_DEFINE_TYPE (AnjutaCommandQueue, anjuta_command_queue, G_TYPE_OBJECT);

static void
anjuta_command_queue_init (AnjutaCommandQueue *self)
{
	self->priv = g_new0 (AnjutaCommandQueuePriv, 1);

	self->priv->queue = g_queue_new ();
}

static void
anjuta_command_queue_finalize (GObject *object)
{
	AnjutaCommandQueue *self;
	GList *current_command;

	self = ANJUTA_COMMAND_QUEUE (object);

	current_command = self->priv->queue->head;

	while (current_command)
	{
		g_object_unref (current_command->data);
		current_command = g_list_next (current_command);
	}

	g_queue_free (self->priv->queue);
	g_free (self->priv);

	G_OBJECT_CLASS (anjuta_command_queue_parent_class)->finalize (object);
}

static void
finished (AnjutaCommandQueue *queue)
{

}

static void
anjuta_command_queue_class_init (AnjutaCommandQueueClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = anjuta_command_queue_finalize;
	klass->finished = finished;

	anjuta_command_queue_signals[FINISHED] = g_signal_new ("finished",
	                                                       G_OBJECT_CLASS_TYPE (klass),
	                                                       G_SIGNAL_RUN_FIRST,
	                                                       G_STRUCT_OFFSET (AnjutaCommandQueueClass, finished),
	                                                       NULL,
	                                                       NULL,
	                                                       g_cclosure_marshal_VOID__VOID,
	                                                       G_TYPE_NONE,
	                                                       0);
	                                                       
}

static void
on_command_finished (AnjutaCommand *command, guint return_code,
                     AnjutaCommandQueue *self)
{
	AnjutaCommand *next_command;

	next_command = g_queue_pop_head (self->priv->queue);

	if (next_command)
	{
		g_signal_connect (G_OBJECT (next_command), "command-finished",
		                  G_CALLBACK (on_command_finished),
		                  self);

		anjuta_command_start (next_command);

		g_object_unref (next_command);
	}
	else
	{
		self->priv->busy = FALSE;

		if (self->priv->mode == ANJUTA_COMMAND_QUEUE_EXECUTE_MANUAL)
			g_signal_emit_by_name (self, "finished");
	}
}

AnjutaCommandQueue *
anjuta_command_queue_new (AnjutaCommandQueueExecuteMode mode)
{
	AnjutaCommandQueue *self;

	self = g_object_new (ANJUTA_TYPE_COMMAND_QUEUE, NULL);

	self->priv->mode = mode;

	return self;
}

void
anjuta_command_queue_push (AnjutaCommandQueue *self, AnjutaCommand *command)
{
	if (self->priv->mode == ANJUTA_COMMAND_QUEUE_EXECUTE_AUTOMATIC)
	{
		if (!self->priv->busy)
		{
			g_signal_connect (G_OBJECT (command), "command-finished",
			                  G_CALLBACK (on_command_finished),
			                  self);

			if (self->priv->mode == ANJUTA_COMMAND_QUEUE_EXECUTE_AUTOMATIC)
			{
				self->priv->busy = TRUE;
				anjuta_command_start (command);
			}
		}
		else
			g_queue_push_tail (self->priv->queue, g_object_ref (command));
	}
	else
		g_queue_push_tail (self->priv->queue, g_object_ref (command));
}

gboolean
anjuta_command_queue_start (AnjutaCommandQueue *self)
{
	gboolean ret;
	AnjutaCommand *first_command;

	ret = FALSE;

	if ((self->priv->mode == ANJUTA_COMMAND_QUEUE_EXECUTE_MANUAL) &&
	    (!self->priv->busy))
	{
		first_command = g_queue_pop_head (self->priv->queue);

		if (first_command)
		{
			g_signal_connect (G_OBJECT (first_command), "command-finished",
			                  G_CALLBACK (on_command_finished),
			                  self);

			self->priv->busy = TRUE;
			ret = TRUE;

			anjuta_command_start (first_command);
		}
	}

	return ret;
		
}