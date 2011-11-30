
#include <stdio.h>
#include "jstypes.h"
#include "jsparse.h"
#include "js-node.h"

void
print_node (JSNode *node, char *pref)
{
	char *pr = g_strconcat(pref, "\t", NULL);
	JSNode *iter;

	if (node == NULL)
		return;

	printf ("%s%d\n",pref,node->pn_type);
	switch (node->pn_arity)
	{
		case PN_UNARY:
			printf ("%sUNARY\n", pref);
			switch ((JSTokenType)node->pn_type)
			{
			case TOK_SEMI:
				print_node (node->pn_u.unary.kid, pr);
				break;
			default:
				break;
			}
			break;
		case PN_LIST:
			printf ("%sLIST\n", pref);
			switch ((JSTokenType)node->pn_type)
			{
			case TOK_VAR:
				for (iter = node->pn_u.list.head; iter != NULL; iter = iter->pn_next)
				{
					g_assert (iter->pn_type == TOK_NAME);

					break;
				}
			case TOK_LP:
				print_node (node->pn_u.list.head, pr);
				int k = 0;
				for (iter = ((JSNode*)node->pn_u.list.head)->pn_next; iter != NULL; iter = iter->pn_next, k++)
				{
					print_node (iter, pr);
				}
				break;
			case TOK_RC:
				{
				print_node (node->pn_u.list.head, pr);
				int k = 0;
				for (iter = ((JSNode*)node->pn_u.list.head)->pn_next; iter != NULL; iter = iter->pn_next, k++)
				{
					print_node (iter, pr);
				}
				}
				break;
			case TOK_LC:
				for (iter = node->pn_u.list.head; iter != NULL; iter = iter->pn_next)
				{
					print_node (iter, pr);
				}
				break;
			case TOK_NEW:
				///NOT COMPLETE
				print_node (node->pn_u.list.head, pr);
				for (iter = ((JSNode*)node->pn_u.list.head)->pn_next; iter != NULL; iter = iter->pn_next)
				{
					print_node (iter, pr);
				}
				break;
			default:
				break;
			}
			break;
		case PN_BINARY:
			printf ("%sBINARY\n", pref);
			switch ((JSTokenType)node->pn_type)
			{
				case TOK_ASSIGN:
					///NOT COMPLETE
					print_node (node->pn_u.binary.left, pr);
					print_node (node->pn_u.binary.right, pr);

					break;
				default:
					break;
			}
			break;
		case PN_FUNC:
			printf ("%sFUNC\n", pref);
			print_node (node->pn_u.func.body, pr);
			break;
		case PN_NAME:
//TODO			printf ("%sNAME\n%s\n", pref, node->pn_u.name.name);
			printf ("%sNAME\n", pref);
			print_node (node->pn_u.name.expr, pr);
/*			JSAtom * atom = node->pn_atom;
			*code = js_AtomToPrintableString(context, node->pn_atom);			*/
			break;
		case PN_NULLARY:
			printf ("%sNULL\n", pref);
			switch ((JSTokenType)node->pn_type)
			{
				case TOK_STRING:
//					*code = g_strconcat ("\"", js_AtomToPrintableString(context, node->pn_atom), "\"", NULL);			
					break;
				case TOK_NUMBER:
//					*code = g_new (char, 100);
//					sprintf (*code, "%lf", node->pn_u.dval);
					break;
				default:
					break;
			}
			break;
		case PN_TERNARY:
			printf ("%sTERNARY\n", pref);
/*			print (node->pn_u.ternary.kid1, pr, &in);
			print (node->pn_u.ternary.kid2, pr, &in);			
			print (node->pn_u.ternary.kid3, pr, &in);			*/
			break;
		
	}
	g_free (pr);
	return;
}
/*
void
print_context (Context *my_cx, const gchar *str)
{
	GList *i;
	printf ("%s%d-%d\n", str, my_cx->bline, my_cx->eline);
	for (i = my_cx->local_var; i; i = g_list_next (i))
	{
		Var *t = (Var *)i->data;
		if (!t->name)
			continue;
		printf ("%s%s\n", str, t->name);
	}
	for (i = g_list_last (my_cx->childs); i; i = g_list_previous (i))
	{
		Context *t = (Context *)i->data;
		print_context (t, g_strdup_printf ("%s%s","\t", str));
	}
}*/
