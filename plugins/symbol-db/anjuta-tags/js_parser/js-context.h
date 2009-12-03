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

#ifndef _JS_CONTEXT_H_
#define _JS_CONTEXT_H_

#include <glib-object.h>

#include "js-node.h"

typedef struct
{
	gchar *name;
	JSNode *list;
} FuncCall;

typedef struct
{
	gchar *name;
	JSNode *node;
	gint line;
} Var;

typedef struct
{
	gchar *name;
	gboolean isFuncCall;
} Type;

G_BEGIN_DECLS

#define JS_TYPE_CONTEXT             (js_context_get_type ())
#define JS_CONTEXT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), JS_TYPE_CONTEXT, JSContext))
#define JS_CONTEXT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), JS_TYPE_CONTEXT, JSContextClass))
#define JS_IS_CONTEXT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), JS_TYPE_CONTEXT))
#define JS_IS_CONTEXT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), JS_TYPE_CONTEXT))
#define JS_CONTEXT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), JS_TYPE_CONTEXT, JSContextClass))

typedef struct _JSContextClass JSContextClass;
typedef struct _JSContext JSContext;

struct _JSContextClass
{
	GObjectClass parent_class;
};

struct _JSContext
{
	GObject parent_instance;

	GList *local_var;
	gint bline, eline;
	JSContext *parent;
	GList *childs;
	gchar *func_name;
	GList *ret_type;
	GList *func_arg;/*name only*/
};

GType js_context_get_type (void) G_GNUC_CONST;

JSContext* js_context_new_from_node (JSNode *node, GList **calls);

JSNode* js_context_get_last_assignment (JSContext *my_cx, const gchar *name);

Type* js_context_get_node_type (JSContext *my_cx, JSNode *node);

JSNode* js_context_get_member (JSContext *my_cx, const gchar *tname, const gchar *mname);

GList* js_context_get_func_ret_type (JSContext *my_cx, const gchar* name);

GList* js_context_get_member_list (JSContext *my_cx, const gchar *tname);

G_END_DECLS

#endif /* _JS_CONTEXT_H_ */
