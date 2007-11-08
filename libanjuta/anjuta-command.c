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

struct _AnjutaCommandPriv
{
	gchar *error_message;
};

enum
{
	DATA_ARRIVED,
	COMMAND_FINISHED,

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

static void
anjuta_command_class_init (AnjutaCommandClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = anjuta_command_finalize;
	
	klass->run = NULL;
	klass->start = NULL;
	klass->notify_data_arrived = NULL;
	klass->notify_complete = NULL;
	klass->set_error_message = anjuta_command_set_error_message;
	klass->get_error_message = anjuta_command_get_error_message;

	anjuta_command_signals[DATA_ARRIVED] =
		g_signal_new ("data-arrived",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_FIRST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 
					  0);

	anjuta_command_signals[COMMAND_FINISHED] =
		g_signal_new ("command-finished",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_FIRST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__UINT ,
		              G_TYPE_NONE, 1,
		              G_TYPE_UINT);
}

void
anjuta_command_start (AnjutaCommand *self)
{
	ANJUTA_COMMAND_GET_CLASS (self)->start (self);
}

void
anjuta_command_notify_data_arrived (AnjutaCommand *self)
{
	ANJUTA_COMMAND_GET_CLASS (self)->notify_data_arrived (self);
}

void
anjuta_command_notify_complete (AnjutaCommand *self, guint return_code)
{
	ANJUTA_COMMAND_GET_CLASS (self)->notify_complete (self, return_code);
}

void
anjuta_command_set_error_message (AnjutaCommand *self, gchar *error_message)
{
	if (self->priv->error_message)
		g_free (error_message);
	
	self->priv->error_message = g_strdup (error_message);
}

gchar *
anjuta_command_get_error_message (AnjutaCommand *self)
{
	return g_strdup (self->priv->error_message);
}
