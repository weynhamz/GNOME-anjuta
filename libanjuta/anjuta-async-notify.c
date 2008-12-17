/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2008 <jrliggett@cox.net>
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

#include "anjuta-async-notify.h"

/**
 * SECTION: anjuta-async-notify
 * @short_description: Mechanism used by interfaces that run asynchronously to 
 *					   notify clients of finished tasks and to report errors.
 *
 * #AnjutaAsyncNotify is a way to allow Anjuta interfaces that run 
 * asynchronously, such as #IAnjutaVCS, to notify clients that a method has 
 * completed. #AnjutaAsyncNotify also reports errors to the user. 
 * 
 * All clients need to do is create an instance of #AnjutaAsyncNotify, connect
 * to the finished signal, and pass it in to the interface method to be called. 
 */

enum
{
	FINISHED,

	LAST_SIGNAL
};


static guint async_notify_signals[LAST_SIGNAL] = { 0 };

struct _AnjutaAsyncNotifyPriv
{
	GError *error;
};

G_DEFINE_TYPE (AnjutaAsyncNotify, anjuta_async_notify, G_TYPE_OBJECT);

static void
anjuta_async_notify_init (AnjutaAsyncNotify *self)
{
	self->priv = g_new0 (AnjutaAsyncNotifyPriv, 1);
}

static void
anjuta_async_notify_finalize (GObject *object)
{
	AnjutaAsyncNotify *self;
	
	self = ANJUTA_ASYNC_NOTIFY (object);
	
	g_error_free (self->priv->error);
	g_free (self->priv);

	G_OBJECT_CLASS (anjuta_async_notify_parent_class)->finalize (object);
}

static void
anjuta_async_notify_finished (AnjutaAsyncNotify *self)
{
	
}

static void
anjuta_async_notify_class_init (AnjutaAsyncNotifyClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = anjuta_async_notify_finalize;

	klass->finished = anjuta_async_notify_finished;
	
	/**
	 * AnjutaAsyncNotify::finished:
	 * @notify: AnjutaAsyncNotify object.
	 *
	 * Notifies clients that the requested task has finished.
	 */
	async_notify_signals[FINISHED] =
		g_signal_new ("finished",
		              G_OBJECT_CLASS_TYPE (klass),
		              0,
		              G_STRUCT_OFFSET (AnjutaAsyncNotifyClass, finished),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0,
		              NULL);
}

/**
 * anjuta_async_notify_new:
 *
 * Creates a new #AnjutaAsyncNotify object.
 *
 * Return value: a new #AnjutaAsyncNotify instance
 */
AnjutaAsyncNotify *
anjuta_async_notify_new (void)
{
	return g_object_new (ANJUTA_TYPE_ASYNC_NOTIFY, NULL);
}

/**
 * anjuta_async_notify_get_error:
 *
 * Gets the error set on @self.
 *
 * @self: An #AnjutaAsyncNotify object
 * @error: Return location for the error set by the called interface to which 
 *		   this object was passed. If no error is set, @error is set to NULL.
 */
void
anjuta_async_notify_get_error (AnjutaAsyncNotify *self, GError **error)
{
	if (self->priv->error)
		*error = g_error_copy (self->priv->error);
}

/**
 * anjuta_async_notify_set_error:
 *
 * Sets the error for an interface call. This method should only be used by 
 * interface implementations themselves, not by clients. 
 *
 * @self: An #AnjutaAsyncNotify object
 * @error: Error to set
 */
void
anjuta_async_notify_set_error (AnjutaAsyncNotify *self, GError *error)
{
	if (self->priv->error)
		g_error_free (self->priv->error);
	
	self->priv->error = g_error_copy (error);
}

/**
 * anjuta_async_notify_notify_finished:
 *
 * Emits the finished signal. This method should only be used by 
 * interface methods themselves, not by clients. 
 *
 * @self: An #AnjutaAsyncNotify object
 */
void
anjuta_async_notify_notify_finished (AnjutaAsyncNotify *self)
{
	g_signal_emit_by_name (self, "finished", NULL);
}
