/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    Copyright (C) 2009 Maxim Ermilov   <zaspire@rambler.ru>

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

#ifndef _DEBUGGER_SERVER_H_
#define _DEBUGGER_SERVER_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define DEBUGGER_TYPE_SERVER             (debugger_server_get_type ())
#define DEBUGGER_SERVER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), DEBUGGER_TYPE_SERVER, DebuggerServer))
#define DEBUGGER_SERVER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DEBUGGER_TYPE_SERVER, DebuggerServerClass))
#define DEBUGGER_IS_SERVER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DEBUGGER_TYPE_SERVER))
#define DEBUGGER_IS_SERVER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DEBUGGER_TYPE_SERVER))
#define DEBUGGER_SERVER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DEBUGGER_TYPE_SERVER, DebuggerServerClass))

typedef struct _DebuggerServerClass DebuggerServerClass;
typedef struct _DebuggerServer DebuggerServer;

struct _DebuggerServerClass
{
	GObjectClass parent_class;

	/* Signals */
	void(* data_arrived) (DebuggerServer *self);
	void(* error) (DebuggerServer *self, const gchar * error);
};

struct _DebuggerServer
{
	GObject parent_instance;
};

GType debugger_server_get_type (void) G_GNUC_CONST;
DebuggerServer* debugger_server_new (gint port);
void debugger_server_send_line (DebuggerServer *object, const gchar* line);
void debugger_server_stop (DebuggerServer *object);
gchar* debugger_server_get_line (DebuggerServer *object);
gint debugger_server_get_line_col (DebuggerServer *object);

G_END_DECLS

#endif /* _DEBUGGER_SERVER_H_ */
