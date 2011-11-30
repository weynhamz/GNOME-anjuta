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

#ifndef _JS_NODE_H_
#define _JS_NODE_H_

#include <glib-object.h>

#include "jstypes.h"

G_BEGIN_DECLS

#define JS_TYPE_NODE             (js_node_get_type ())
#define JS_NODE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), JS_TYPE_NODE, JSNode))
#define JS_NODE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), JS_TYPE_NODE, JSNodeClass))
#define JS_IS_NODE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), JS_TYPE_NODE))
#define JS_IS_NODE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), JS_TYPE_NODE))
#define JS_NODE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), JS_TYPE_NODE, JSNodeClass))

typedef struct _JSNodeClass JSNodeClass;
typedef struct _JSNode JSNode;

struct _JSNodeClass
{
	GObjectClass parent_class;
};

struct _JSNode
{
	GObject parent_instance;

	int        pn_type;
	int        pn_op;
	int        pn_arity;
	JSTokenPos pn_pos;
	union {
		struct {                        /* TOK_FUNCTION node */
			JSNode *body;
			JSNode *name;
			JSNode *args;
		} func;
		struct {                        /* list of next-linked nodes */
			JSNode *head;
		} list;
		struct {                        /* ternary: if, for(;;), ?: */
			char dummy;
		} ternary;
		struct {                        /* two kids if binary */
			JSNode *left;
			JSNode *right;
		} binary;
		struct {                        /* one kid if unary */
			JSNode *kid;
		} unary;
		struct {                        /* name, labeled statement, etc. */
			JSNode *expr;
			void *name;
			char isconst;
		} name;
		struct {
			char dummy;
		} apair;
    } pn_u;
    JSNode *pn_next;
};

GType js_node_get_type (void) G_GNUC_CONST;
gchar* js_node_get_name (JSNode *node);
JSNode* js_node_new_from_file (const gchar *name);
GList* js_node_get_list_member_from_rc (JSNode* node);
JSNode* js_node_get_member_from_rc (JSNode* node, const gchar *mname);
GList* js_node_get_lines_missed_semicolon (JSNode *node);

G_END_DECLS

#endif /* _JS_NODE_H_ */
