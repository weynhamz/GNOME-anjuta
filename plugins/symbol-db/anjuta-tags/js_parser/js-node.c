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
#include "stdio.h"

#include "js-node.h"
#include "y.tab.h"
#include "lex.yy.h"

typedef struct _JSNodePrivate JSNodePrivate;

struct _JSNodePrivate {
  GList *missed;
};

#define JS_NODE_GET_PRIVATE(i)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((i), JS_TYPE_NODE, JSNodePrivate))

G_DEFINE_TYPE (JSNode, js_node, G_TYPE_OBJECT);

static void
js_node_init (JSNode *object)
{
	JSNodePrivate *priv = JS_NODE_GET_PRIVATE (object);
	object->pn_u.func.body = NULL;
	object->pn_u.func.args = NULL;
	object->pn_u.func.name = NULL;
	object->pn_next = NULL;
	priv->missed = NULL;
}

static void
js_node_finalize (GObject *object)
{
	JSNode *self = JS_NODE (object);
	switch (self->pn_arity)
	{
	case PN_FUNC:
		if (self->pn_u.func.body)
			g_object_unref (self->pn_u.func.body);
		if (self->pn_u.func.name)
			g_object_unref (self->pn_u.func.name);
//		void *args;

		break;
	case PN_LIST:
		if (self->pn_u.list.head)
			g_object_unref (self->pn_u.list.head);
		break;
	case PN_TERNARY:
		break;
	case PN_BINARY:
		if (self->pn_u.binary.left)
			g_object_unref (self->pn_u.binary.left);
		if (self->pn_u.binary.right)
			g_object_unref (self->pn_u.binary.right);
		break;
	case PN_UNARY:
		if (self->pn_u.unary.kid)
			g_object_unref (self->pn_u.unary.kid);
		break;
	case PN_NAME:
		if (self->pn_u.name.expr)
			g_object_unref (self->pn_u.name.expr);
//TODO:Fix		g_object_unref (self->pn_u.name.name);
		break;
	case PN_NULLARY:
		break;
	}

   if (self->pn_next)
		g_object_unref (self->pn_next);
	G_OBJECT_CLASS (js_node_parent_class)->finalize (object);
}

static void
js_node_class_init (JSNodeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	/*GObjectClass* parent_class = G_OBJECT_CLASS (klass);*/
	g_type_class_add_private (klass, sizeof (JSNodePrivate));
	object_class->finalize = js_node_finalize;
}

const gchar*
js_node_get_name (JSNode *node)
{
	g_return_val_if_fail (node, NULL);
	g_assert (JS_IS_NODE (node));
	if (node->pn_arity == PN_NULLARY)
	{
			return NULL;
	}
	if (node->pn_arity != PN_NAME)
		return NULL;
	switch ((JSTokenType)node->pn_type)
	{
		case TOK_NAME:
			return node->pn_u.name.name;
			break;
		case TOK_DOT:
			if (!node->pn_u.name.expr || !node->pn_u.name.name)
				return NULL;
			return g_strdup_printf ("%s.%s", js_node_get_name (node->pn_u.name.expr), js_node_get_name (node->pn_u.name.name));
			break;
		default:
			g_assert_not_reached ();
			break;
	}
	return NULL;
}

GList*
js_node_get_list_member_from_rc (JSNode* node)
{
	GList *ret = NULL;
	JSNode* iter;
	if (node->pn_type != TOK_RC)
		return NULL;
	for (iter = node->pn_u.list.head; iter != NULL; iter = iter->pn_next)
	{
		const gchar* name = js_node_get_name (iter->pn_u.binary.left);
		if (!name)
			g_assert_not_reached ();
		else
			ret = g_list_append (ret, g_strdup (name));
	}
	return ret;
}

JSNode*
js_node_get_member_from_rc (JSNode* node, const gchar *mname)
{
	JSNode* iter;
	if (node->pn_type != TOK_RC)
		return NULL;
	for (iter = node->pn_u.list.head; iter != NULL; iter = iter->pn_next)
	{
		const gchar* name = js_node_get_name (iter->pn_u.binary.left);
		if (!name)
		{
			g_assert_not_reached ();
			continue;
		}
		if (g_strcmp0 (mname, name) != 0)
			continue;
		if (iter->pn_u.binary.right)
			g_object_ref (iter->pn_u.binary.right);
		return iter->pn_u.binary.right;
	}
	return NULL;
}

extern JSNode *global;
extern GList *line_missed_semicolon;

JSNode*
js_node_new_from_file (const gchar *name)
{
	FILE *f = fopen (name, "r");
	JSNodePrivate *priv;

	line_missed_semicolon = NULL;
	global = NULL;
	yyset_lineno (1);
	YY_BUFFER_STATE b = yy_create_buffer (f, 10000);
	yy_switch_to_buffer (b);

	yyparse ();

	fclose (f);

	yy_delete_buffer (b);
	if (!global)
		return g_object_new (JS_TYPE_NODE, NULL);
	priv = JS_NODE_GET_PRIVATE (global);

	priv->missed = line_missed_semicolon;
	return global;
}

GList*
js_node_get_lines_missed_semicolon (JSNode *node)
{
	JSNodePrivate *priv = JS_NODE_GET_PRIVATE (node);
	return priv->missed;
}

