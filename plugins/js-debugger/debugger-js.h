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

#ifndef _DEBUGGER_JS_H_
#define _DEBUGGER_JS_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define DEBUGGER_TYPE_JS             (debugger_js_get_type ())
#define DEBUGGER_JS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), DEBUGGER_TYPE_JS, DebuggerJs))
#define DEBUGGER_JS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DEBUGGER_TYPE_JS, DebuggerJsClass))
#define DEBUGGER_IS_JS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DEBUGGER_TYPE_JS))
#define DEBUGGER_IS_JS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DEBUGGER_TYPE_JS))
#define DEBUGGER_JS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DEBUGGER_TYPE_JS, DebuggerJsClass))

typedef struct _DebuggerJsClass DebuggerJsClass;
typedef struct _DebuggerJs DebuggerJs;

struct _DebuggerJsClass
{
	GObjectClass parent_class;

	/* Signals */
	void(* DebuggerError) (DebuggerJs *self, const gchar *text);
};

struct _DebuggerJs
{
	GObject parent_instance;
};

GType debugger_js_get_type (void) G_GNUC_CONST;
DebuggerJs* debugger_js_new (int port, const gchar* filename, IAnjutaDebugger *data);
IAnjutaDebuggerState debugger_js_get_state (DebuggerJs *object);
void debugger_js_set_work_dir (DebuggerJs *object, const gchar* work_dir);
void debugger_js_start_remote (DebuggerJs *object, gint port);
void debugger_js_start (DebuggerJs *object, const gchar *arguments);
void debugger_js_continue (DebuggerJs *object);
void debugger_js_stepin (DebuggerJs *object);
void debugger_js_stepover (DebuggerJs *object);
void debugger_js_stepout (DebuggerJs *object);
void debugger_js_stop (DebuggerJs *object);
void debugger_js_add_breakpoint (DebuggerJs *object, const gchar* file, guint line);
void debugger_js_breakpoint_list (DebuggerJs *object, IAnjutaDebuggerCallback callback, gpointer user_data);
void debugger_js_variable_list_children (DebuggerJs *object, IAnjutaDebuggerCallback callback, const gchar *name, gpointer user_data);
void debugger_js_signal (DebuggerJs *object, IAnjutaDebuggerCallback callback, gpointer user_data);
void debugger_js_list_local (DebuggerJs *object, IAnjutaDebuggerCallback callback, gpointer user_data);
void debugger_js_list_thread (DebuggerJs *object, IAnjutaDebuggerCallback callback, gpointer user_data);
void debugger_js_list_frame (DebuggerJs *object, IAnjutaDebuggerCallback callback, gpointer user_data);
void debugger_js_info_thread (DebuggerJs *object, IAnjutaDebuggerCallback callback, gint thread, gpointer user_data);
void debugger_js_variable_create (DebuggerJs *object, IAnjutaDebuggerCallback callback, const gchar *name, gpointer user_data);

G_END_DECLS

#endif /* _DEBUGGER_JS_H_ */
