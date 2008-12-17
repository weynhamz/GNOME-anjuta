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

#ifndef _ANJUTA_ASYNC_NOTIFY_H_
#define _ANJUTA_ASYNC_NOTIFY_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define ANJUTA_TYPE_ASYNC_NOTIFY             (anjuta_async_notify_get_type ())
#define ANJUTA_ASYNC_NOTIFY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_TYPE_ASYNC_NOTIFY, AnjutaAsyncNotify))
#define ANJUTA_ASYNC_NOTIFY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_TYPE_ASYNC_NOTIFY, AnjutaAsyncNotifyClass))
#define ANJUTA_IS_ASYNC_NOTIFY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_TYPE_ASYNC_NOTIFY))
#define ANJUTA_IS_ASYNC_NOTIFY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANJUTA_TYPE_ASYNC_NOTIFY))
#define ANJUTA_ASYNC_NOTIFY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANJUTA_TYPE_ASYNC_NOTIFY, AnjutaAsyncNotifyClass))

typedef struct _AnjutaAsyncNotifyClass AnjutaAsyncNotifyClass;
typedef struct _AnjutaAsyncNotify AnjutaAsyncNotify;
typedef struct _AnjutaAsyncNotifyPriv AnjutaAsyncNotifyPriv;


struct _AnjutaAsyncNotifyClass
{
	GObjectClass parent_class;

	/* Signals */
	void (*finished) (AnjutaAsyncNotify *self);
};

struct _AnjutaAsyncNotify
{
	GObject parent_instance;
	
	AnjutaAsyncNotifyPriv *priv;
};

GType anjuta_async_notify_get_type (void) G_GNUC_CONST;
AnjutaAsyncNotify *anjuta_async_notify_new (void);
void anjuta_async_notify_get_error (AnjutaAsyncNotify *self, GError **error);
void anjuta_async_notify_notify_finished (AnjutaAsyncNotify *self);
void anjuta_async_notify_set_error (AnjutaAsyncNotify *self, GError *error);

G_END_DECLS

#endif /* _ANJUTA_ASYNC_NOTIFY_H_ */
