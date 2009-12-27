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

#include <stdio.h>
#include <string.h>
#include "js-context.h"

typedef struct _JSContextPrivate JSContextPrivate;

struct _JSContextPrivate {
  JSNode *node;
};

static void interpretator (JSNode *node, JSContext *my_cx, GList **calls);

G_DEFINE_TYPE (JSContext, js_context, G_TYPE_OBJECT);

#define JS_CONTEXT_GET_PRIVATE(i)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((i), JS_TYPE_CONTEXT, JSContextPrivate))

static void
js_context_init (JSContext *object)
{
	JSContextPrivate *priv = JS_CONTEXT_GET_PRIVATE (object);
	object->func_arg = NULL;
	object->func_name = NULL;
	object->parent = NULL;
	object->childs = NULL;
	object->local_var = NULL;
	object->bline = 0;
	object->eline = 0;
	object->ret_type = NULL;

	priv->node = NULL;
}

static void
js_context_finalize (GObject *object)
{
	JSContext *self = JS_CONTEXT (object);
	JSContextPrivate *priv = JS_CONTEXT_GET_PRIVATE (self);

	if (priv->node)
		g_object_unref (priv->node);

	g_list_foreach (self->local_var, (GFunc)g_free, NULL);
	g_list_free (self->local_var);
	g_list_foreach (self->childs, (GFunc)g_object_unref, NULL);
	g_list_free (self->childs);
	g_free (self->func_name);
	g_list_free (self->ret_type);
	g_list_free (self->func_arg);

	G_OBJECT_CLASS (js_context_parent_class)->finalize (object);
}

static void
js_context_class_init (JSContextClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	/*GObjectClass* parent_class = G_OBJECT_CLASS (klass);*/

	g_type_class_add_private (klass, sizeof (JSContextPrivate));

	object_class->finalize = js_context_finalize;
}


JSContext*
js_context_new_from_node (JSNode *node, GList **calls)
{
	JSContext *self = g_object_new (JS_TYPE_CONTEXT, NULL);
	JSContextPrivate *priv = JS_CONTEXT_GET_PRIVATE (self);
	g_object_ref (node);
	priv->node = node;
	interpretator (node, self, calls);
	return self;
}

JSNode*
js_context_get_last_assignment (JSContext *my_cx, const gchar *name)
{
	GList *i;
	for (i = g_list_last (my_cx->local_var); i; i = g_list_previous (i))
	{
		Var *t = (Var *)i->data;
		if (!t->name)
			continue;
		if (g_strcmp0 (t->name, name) != 0)
			continue;
		if (t->node)
			g_object_ref (t->node);
		return t->node;
	}
	for (i = g_list_last (my_cx->childs); i; i = g_list_previous (i))
	{
		JSContext *t = (JSContext *)i->data;
		JSNode *node = js_context_get_last_assignment (t, name);
		if (node)
			return node;
	}
	return NULL;
}


static JSContext *
js_context_new (JSContext *parent)
{
	JSContext *self = g_object_new (JS_TYPE_CONTEXT, NULL);
	self->parent = parent;
	return self;
}

Type*
js_context_get_node_type (JSContext *my_cx, JSNode *node)
{
	Type *ret;
	const gchar *name;
	if (!node)
		return NULL;

	ret = g_new (Type, 1);
	ret->isFuncCall = FALSE;
	switch ((JSNodeArity)node->pn_arity)
	{
		case PN_NAME:
				switch ((JSTokenType)node->pn_type)
				{
					case TOK_NAME:
						name = js_node_get_name (node);
						if (!name)
							g_assert_not_reached ();
						else
						{
							JSNode *t = js_context_get_last_assignment (my_cx, name);
							if (t)
							{
								Type *tname = js_context_get_node_type (my_cx, t);
								if (tname)
									return tname;
								else
									ret->name = g_strdup (name);
							}
							else
								ret->name = g_strdup (name);
							return ret;
						}
						break;
					case TOK_DOT:
						name = js_node_get_name (node);
						if (!name)
							g_assert_not_reached ();
						else
						{
							JSNode *t = js_context_get_last_assignment (my_cx, name);
							if (t)
							{
								Type *tname = js_context_get_node_type (my_cx, t);
								if (tname)
									return tname;
								else
									ret->name = g_strdup (name);
							}
							else
								ret->name = g_strdup (name);
							return ret;
						}
						break;
					default:
						g_assert_not_reached ();
						break;
				}
				break;
		case PN_NULLARY:
				switch ((JSTokenType)node->pn_type)
				{
					case TOK_STRING:
//							puts ("string");
							ret->name = g_strdup ("String");
							return ret;
							break;
					case TOK_NUMBER:
///							puts ("float");
							ret->name = g_strdup ("Number");
							return ret;
							break;
					case TOK_PRIMARY:
							switch (node->pn_op)
							{
								case JSOP_FALSE:
								case JSOP_TRUE:
									ret->name = g_strdup ("Boolean");
									return ret;
									break;
								case JSOP_NULL:
									ret->name = g_strdup ("null");
									return ret;
									break;
								case JSOP_THIS:
									ret->name = g_strdup ("Object");
									return ret;
									break;
								default:
									printf ("%d\n", node->pn_op);
									g_assert_not_reached ();
									break;
							}
							break;
					default:
							printf ("%d\n", node->pn_type);
							g_assert_not_reached ();
							break;
				}
				break;
		case PN_LIST:
				switch ((JSTokenType)node->pn_type)
				{
					case TOK_NEW:
							name = js_node_get_name (node->pn_u.list.head);
							if (!name)
								g_assert_not_reached ();
							else
							{
//								puts (name);
								ret->name = g_strdup (name);
								return ret;
							}
							break;
					case TOK_LP:
							name = js_node_get_name (node->pn_u.list.head);
							if (!name)
								g_assert_not_reached ();
							else
							{
//								printf ("call %s\n",name);
								ret->isFuncCall = TRUE;
								ret->name = g_strdup (name);
								return ret;								
							}
							break;
					case TOK_PLUS:
/*TODO						if (node->pn_extra == PNX_STRCAT)
							return g_strdup ("String");*/
						ret->name = g_strdup ("Number");
						return ret;
						break;
					case TOK_RC:
//TODO:
							break;
					default:
							printf ("%d\n", node->pn_type);
							g_assert_not_reached ();
							break;
				}
				break;
		case PN_FUNC:
				ret->name = g_strdup ("Function");
				return ret;
				break;
		case PN_BINARY:
				switch ((JSTokenType)node->pn_type)
				{
					case TOK_MINUS:
					case TOK_PLUS:
/*TODO						if (node->pn_extra == PNX_STRCAT)
							return g_strdup ("String");*/
						ret->name = g_strdup ("Number");
						return ret;
						break;
					default:
						printf ("%d\n", node->pn_type);
						g_assert_not_reached ();
						break;
				}
				break;
		case PN_UNARY:
				switch ((JSTokenType)node->pn_type)
				{
					case TOK_RP:
						return js_context_get_node_type (my_cx, node->pn_u.unary.kid);
						break;
					case TOK_UNARYOP:
						return js_context_get_node_type (my_cx, node->pn_u.unary.kid);
						break;
					default:
						printf ("%d\n", node->pn_type);
						g_assert_not_reached ();
						break;
				}
				break;
		case PN_TERNARY:
				switch ((JSTokenType)node->pn_type)
				{
/*TODO					case TOK_HOOK:
						return get_node_type (node->pn_u.ternary.kid2, my_cx);
						break;*/
					default:
						printf ("%d\n", node->pn_type);
						g_assert_not_reached ();
						break;
				}
				break;
		default:
				printf ("%d\n", node->pn_type);
				g_assert_not_reached ();
				break;
	}
	return NULL;
}

static void
interpretator (JSNode *node, JSContext *my_cx, GList **calls)
{
	JSNode *iter;

	if (node == NULL)
		return;
	switch ((JSNodeArity)node->pn_arity)
	{
		case PN_FUNC:
				{
					Var *tvar = g_new (Var, 1);
					tvar->name = NULL;
					tvar->line = node->pn_pos.end;
					tvar->node = node;

					if (node->pn_u.func.name) {
						tvar->name = g_strdup (js_node_get_name (node->pn_u.func.name));
//puts (tvar->name);
					}
					if (tvar->name)
					{
						my_cx->local_var = g_list_append (my_cx->local_var, tvar);
						JSContext *t = js_context_new (my_cx);
						t->func_name = g_strdup (tvar->name);
//puts (t->func_name);
						t->bline = node->pn_pos.begin;
						t->eline = node->pn_pos.end;
						interpretator (node->pn_u.func.body, t, calls);
						my_cx->childs = g_list_append (my_cx->childs, t);
//puts (t->func_name);
/*						JSAtom ** params = (JSAtom **)g_malloc (function->nargs * sizeof (JSAtom *));
						gint i;
						for (i = 0; i < function->nargs; i++) {
						    params[i] = NULL;
						}
						JSScope * scope = OBJ_SCOPE(object);
						JSScopeProperty * scope_property;
						for (scope_property = SCOPE_LAST_PROP (scope);
						        scope_property != NULL;
						        scope_property = scope_property->parent) {
						    if (!JSID_IS_ATOM (scope_property->id) || ((uint16)scope_property->shortid) >= function->nargs) {
						    continue;
						    }
						    params[(uint16) scope_property->shortid] = JSID_TO_ATOM (scope_property->id);
						}
						for (i = 0; i < function->nargs; i++) {
						    g_assert (params[i] != NULL);
							const gchar *code = js_AtomToPrintableString(cx, params[i]);
							g_assert (code != NULL);
							t->func_arg = g_list_append (t->func_arg, g_strdup (code));
					    }*/
						JSNode *i = JS_NODE (node->pn_u.func.args);
						if (i)
						{
							g_assert (i->pn_arity == PN_LIST);
							for (i = JS_NODE (i->pn_u.list.head); i; i = JS_NODE (i->pn_next))
							{
								g_assert (i->pn_arity == PN_NAME);
								t->func_arg = g_list_append (t->func_arg, g_strdup (js_node_get_name (i)));
							}
						}
					}
				}
				break;
		case PN_LIST:
				switch ((JSTokenType)node->pn_type)
				{
					case TOK_LC:
						{
							JSContext *t = js_context_new (my_cx);
							t->bline = node->pn_pos.begin;
							t->eline = node->pn_pos.end;
							for (iter = node->pn_u.list.head; iter != NULL; iter = iter->pn_next)
							{
								interpretator (iter, t, calls);
							}
							my_cx->childs = g_list_append (my_cx->childs, t);
						}
						break;
					case TOK_LP:
						{
							const gchar *fname = js_node_get_name (node->pn_u.list.head);
							if (!fname)
								break;
							FuncCall *t = g_new (FuncCall, 1);
							t->name = g_strdup (fname);
							t->list = ((JSNode*)node->pn_u.list.head)->pn_next;
							*calls = g_list_append (*calls, t);
//printf ("call to %s\n", t->name);
						}
						break;
					case TOK_VAR:
						for (iter = node->pn_u.list.head; iter != NULL; iter = iter->pn_next)
						{
							g_assert (iter->pn_type == TOK_NAME);

							const gchar *name = js_node_get_name (iter);
							Var *t = g_new (Var, 1);
							t->name = g_strdup (name);
							t->node = iter->pn_u.name.expr;
							t->line = iter->pn_pos.end;
							my_cx->local_var = g_list_append (my_cx->local_var, t);
						}
						break;
					case TOK_RC:
						for (iter = node->pn_u.list.head; iter != NULL; iter = iter->pn_next)
						{
							//add to Context
						}
						break;
					default:
						break;
				}
		case PN_BINARY:
				switch ((JSTokenType)node->pn_type)
				{
					case TOK_ASSIGN:
					///NOT COMPLETE
						{
							if (!node->pn_u.binary.left)
								break;
							const gchar *name = js_node_get_name (node->pn_u.binary.left);
							Var *t = (Var *)g_new (Var, 1);
							t->name = g_strdup (name);
							t->node = node->pn_u.binary.right;
							t->line = node->pn_pos.end;
							my_cx->local_var = g_list_append (my_cx->local_var, t);
						}
						break;
					case TOK_WHILE:
					case TOK_FOR:
						{
							JSContext *t = js_context_new (my_cx);
							t->bline = node->pn_pos.begin;
							t->eline = node->pn_pos.end;
							interpretator (node->pn_u.binary.right, t, calls);
							my_cx->childs = g_list_append (my_cx->childs, t);
						}
						break;
					case TOK_DO:
						{
							JSContext *t = js_context_new (my_cx);
							t->bline = node->pn_pos.begin;
							t->eline = node->pn_pos.end;
							interpretator (node->pn_u.binary.left, t, calls);
							my_cx->childs = g_list_append (my_cx->childs, t);
						}
						break;
					default:
						break;
				}
				break;
		case PN_UNARY:
				switch ((JSTokenType)node->pn_type)
				{
					case TOK_SEMI:
						interpretator (node->pn_u.unary.kid, my_cx, calls);
						break;
					case TOK_RETURN:
						{
							Type *type = js_context_get_node_type (my_cx, node->pn_u.unary.kid);
							if (type)
							{
								JSContext *i;
//								puts (type);
								for (i = my_cx; i; i = i->parent)
								{
									if (!i->func_name)
										continue;
									i->ret_type = g_list_append (i->ret_type, type->name);
									break;
								}
							}
						}
						break;
					default:
						break;
				}
				break;
		case PN_TERNARY:
				switch ((JSTokenType)node->pn_type)
				{
					case TOK_IF:
/*TODO						if (node->pn_kid2)
						{
							Context *t = new Context (my_cx);
							t->bline = node->pn_kid2->pn_pos.begin.lineno;
							t->eline = node->pn_kid2->pn_pos.end.lineno;
							interpretator (node->pn_kid2, t, calls);
							my_cx->childs = g_list_append (my_cx->childs, t);
						}
						if (node->pn_kid3)
						{
							Context *t = new Context (my_cx);
							t->bline = node->pn_kid3->pn_pos.begin.lineno;
							t->eline = node->pn_kid3->pn_pos.end.lineno;
							interpretator (node->pn_kid3, t, calls);
							my_cx->childs = g_list_append (my_cx->childs, t);
						}*/
						break;
					default:
						break;
				}
				break;
		case PN_NAME:
		case PN_NULLARY:
			break;
		default:
			printf ("%d\n", node->pn_type);
			g_assert_not_reached ();
			break;
	}
}

JSNode*
js_context_get_member (JSContext *my_cx, const gchar *tname, const gchar *mname)
{
	GList *i;
	gchar *name = g_strconcat (tname, ".prototype", NULL);
	gchar *full_name = g_strdup_printf ("%s.%s", name, mname);
//puts (name);
	for (i = g_list_last (my_cx->local_var); i; i = g_list_previous (i))
	{
		Var *t = (Var *)i->data;
		if (!t->name)
			continue;
		if (strncmp (t->name, name, strlen (name)) != 0)
			continue;
//printf ("%s==%s\n", name, t->name);
		if (strcmp (t->name, full_name) == 0)
		{
			return t->node;
		}
		else
		{
			JSNode *node = js_node_get_member_from_rc (t->node, mname);
			if (node)
				return node;
		}
	}
	for (i = g_list_last (my_cx->childs); i; i = g_list_previous (i))
	{
		JSContext *t = JS_CONTEXT (i->data);
		JSNode *tmp = js_context_get_member (t, tname, mname);
		if (tmp)
			return tmp;
	}
	return NULL;
}

GList*
js_context_get_func_ret_type (JSContext *my_cx, const gchar* name)
{
	GList *i;
	g_assert (name != NULL);
	if (my_cx->func_name)
	{
		if (g_strcmp0 (my_cx->func_name, name) == 0)
		{
			return my_cx->ret_type;//TODO:Fix this
		}
	}
	for (i = g_list_last (my_cx->childs); i; i = g_list_previous (i))
	{
		JSContext *t = JS_CONTEXT (i->data);
		GList *ret = js_context_get_func_ret_type (t, name);
		if (ret)
			return ret;
	}
	return NULL;
}

GList*
js_context_get_member_list (JSContext *my_cx, const gchar *tname)
{
	GList *i, *ret = NULL;

	g_return_val_if_fail (tname != NULL, NULL);

	gchar *name = g_strconcat (tname, ".prototype", NULL);

	for (i = g_list_last (my_cx->local_var); i; i = g_list_previous (i))
	{
		Var *t = (Var *)i->data;
		if (!t->name)
			continue;
		if (strncmp (t->name, name, strlen (name)) != 0)
			continue;
		if (strlen (name) != strlen (t->name))
			ret = g_list_append (ret, g_strdup (t->name));
		else
		{
			GList *tmp = js_node_get_list_member_from_rc (t->node);
			ret = g_list_concat (ret, tmp);
		}
	}
	for (i = g_list_last (my_cx->childs); i; i = g_list_previous (i))
	{
		JSContext *t = JS_CONTEXT (i->data);
		ret = g_list_concat (ret, js_context_get_member_list (t, tname));
	}
	return ret;
}
