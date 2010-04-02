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

#include "general.h"	/* must always come first */
#include "debug.h"
#include "entry.h"
#include "keyword.h"
#include "parse.h"
#include "read.h"
#include "routines.h"
#include "vstring.h"
#include <assert.h>
#include "js_parser/jstypes.h"
#include <glib.h>
#include "js_parser/jsparse.h"
#include "js_parser/js-context.h"

#include <string.h>

static JSTokenPos*
getTagPos (JSNode *node)
{
	if (node->pn_type == TOK_NAME)
		return &node->pn_pos;
	return NULL;
}

#define PROTOTYPE ".prototype"

static void
get_file_pos (gint line, fpos_t *fpos, FILE *f)
{
	vString * str = vStringNew ();
	gint i;
	g_assert (fseek (f, 0, SEEK_SET) == 0);

	for (i = 0;i < line - 1; i++)
		if (readLine (str, f) == NULL)
			return;

	g_assert (fgetpos (f, fpos) == 0);
}

static GList *symbols = NULL;
static GList *tags = NULL;

static void
get_member_list (JSContext *my_cx)
{
	GList *i;

	g_assert (my_cx != NULL);

	gint plen = strlen (PROTOTYPE);

	for (i = my_cx->local_var; i; i = g_list_next (i))
	{
		gint len;
		gchar *tstr;
		Var *t = (Var *)i->data;
		if (!t->name)
			continue;
		len = strlen (t->name);
		if (len <= plen)
			continue;
		if (tstr = strstr (t->name, PROTOTYPE), tstr == NULL)
			continue;
		if (strlen (tstr) != plen)
		{
//TODO:puts (t->name);
//Make tag
		}
		else
		{
			JSNode *node = t->node;
			JSNode* iter;

			g_assert (node->pn_type == TOK_RC);
	
			for (iter = node->pn_u.list.head; iter != NULL; iter = iter->pn_next)
			{
				const gchar* name = js_node_get_name (iter->pn_u.binary.left);
				const JSTokenPos* pos = getTagPos (iter->pn_u.binary.left);
				if (!name)
					g_assert_not_reached ();
				else
				{
					gchar *rname = g_strndup (t->name, len - plen);

					if (g_strcmp0 (name, "__proto__") == 0 )
					{
						GList *i;
						Type *t = js_context_get_node_type (my_cx, JS_NODE (iter->pn_u.binary.right));
						if (!t || !t->name)
							continue;
						for (i = tags; i != NULL; i = g_list_next (i))
							if (g_strcmp0 (((tagEntryInfo *)i->data)->name, rname) == 0)
							{
								gchar *tn = g_strdup (t->name), *tstr;
								if (tstr = strstr (tn, PROTOTYPE), tstr != NULL)
								{
									g_assert (strlen (tstr) == plen);
									*tstr = '\0';
								}
								((tagEntryInfo *)i->data)->extensionFields.inheritance = tn;
								break;
							}
					}
					else
					{
						tagEntryInfo *tag = (tagEntryInfo*)malloc (sizeof (tagEntryInfo));

						symbols = g_list_append (symbols, g_strdup (name));
						
						initTagEntry (tag, name);
						tag->isFileScope = 1;
						tag->kindName = "member";
						tag->kind = 'm';
						get_file_pos (pos->begin, &tag->filePosition, File.fp);
						tag->lineNumber = pos->begin;
						tag->extensionFields.scope[0]="class";
						tag->extensionFields.scope[1]=rname;
						JSNode *node = (JSNode *)iter->pn_u.binary.right;
						if (node && node->pn_arity == PN_FUNC && node->pn_u.func.args)
						{
							gchar *str = NULL, *t;
							JSNode *i = node->pn_u.func.args;
							g_assert (i->pn_arity == PN_LIST);
							for (i = (JSNode *)i->pn_u.list.head; i; i = (JSNode *)i->pn_next)
							{
								const gchar *name;
								g_assert (i->pn_arity == PN_NAME);
								name = js_node_get_name (i);
								g_assert (name != NULL);
								if (str == NULL)
									str = g_strdup_printf ("( %s", name);
								else
								{
									t = g_strdup_printf ("%s, %s", str, name);
									g_free (str);
									str = t;
								}
							}
							t = g_strdup_printf ("%s)", str);
							g_free (str);
							str = t;
							tag->extensionFields.signature = str;
						}
						makeTagEntry (tag);
					}
				}
			}
		}
	}
	for (i = my_cx->childs; i; i = g_list_next (i))
	{
		JSContext *t = (JSContext *)i->data;
		get_member_list (t);
	}

}

static kindOption JsKinds [] = {
	{ TRUE,  'f', "function",	  "functions"},
	{ TRUE,  'c', "class",		  "classes"},
	{ TRUE,  'm', "method",		  "methods"},
	{ TRUE,  'p', "property",	  "properties"},
	{ TRUE,  'v', "variable",	  "global variables"}
};

static void
initialize (const langType language)
{
	g_type_init ();
}

static void
findTags (JSContext *my_cx)
{
	GList *i;
	g_assert (my_cx != NULL);
	if (my_cx->func_name)
	{
		const char *name = my_cx->func_name;
		gint line = my_cx->bline;
		if (name)
		{
			tagEntryInfo *tag = (tagEntryInfo*)malloc (sizeof (tagEntryInfo));
			initTagEntry (tag, name);
			get_file_pos (line, &tag->filePosition, File.fp);
			tag->lineNumber = line;
			tag->isFileScope	= 1;
			tag->kindName = "class";
			tag->kind = 'c';

			symbols = g_list_append (symbols, g_strdup (name));

			if (my_cx->ret_type)
				tag->extensionFields.returnType = my_cx->ret_type->data;
			if (my_cx->func_arg)
			{
				gchar *str = NULL, *t;
				GList *i;
				for (i = my_cx->func_arg; i; i = g_list_next (i))
				{
					g_assert (i->data != NULL);
					if (i == my_cx->func_arg)
						str = g_strdup_printf ("( %s", (gchar*)i->data);
					else
					{
						t = g_strdup_printf ("%s, %s", str, (gchar*)i->data);
						g_free (str);
						str = t;
					}
				}
				t = g_strdup_printf ("%s)", str);
				g_free (str);
				str = t;
				tag->extensionFields.signature = str;
			}
			tags = g_list_append (tags, tag);
		}
	}
	for (i = my_cx->childs; i; i = g_list_next (i))
	{
		findTags (i->data);
	}
}

static void
findGlobal (JSContext *my_cx)
{
	static int depth = 0;
	GList *i;

	g_assert (my_cx != NULL);
	g_assert (depth <= 1);

	for (i = my_cx->local_var; i; i = g_list_next (i))
	{
		const char *name = ((Var*)i->data)->name;
		gint line = ((Var*)i->data)->line;
		g_assert (name != NULL);

		if (g_strstr_len (name, -1, PROTOTYPE) != NULL)
			continue;

		tagEntryInfo *tag = (tagEntryInfo*)malloc (sizeof (tagEntryInfo));
		initTagEntry (tag, name);
		get_file_pos (line, &tag->filePosition, File.fp);
		tag->lineNumber = line;
		tag->isFileScope	= 1;
		tag->kindName = "variable";
		tag->kind = 'v';
		Type *t = js_context_get_node_type (my_cx, ((Var*)i->data)->node);
		if (t)
		{
			tag->extensionFields.typeRef [0] = "variable";
			tag->extensionFields.typeRef [1] = t->name;
			g_free (t);
		}
		if (my_cx->ret_type)
			tag->extensionFields.returnType = my_cx->ret_type->data;
		if (g_list_find_custom (symbols, name, (GCompareFunc)g_strcmp0) == NULL)
			makeTagEntry (tag);
	}
	if (depth == 0)
	{
		depth++;
		for (i = my_cx->childs; i; i = g_list_next (i))
		{
			findGlobal (i->data);
		}
		depth--;
	}
}

static void
findJsTags (void)
{
	g_assert (symbols == NULL);
	g_assert (tags == NULL);

	JSNode * global = js_node_new_from_file (getInputFileName());

	JSContext *my_cx;
	GList *calls = NULL;
	my_cx = js_context_new_from_node (global, &calls);
	findTags (my_cx);

	/*Members*/

	get_member_list (my_cx);

	g_list_foreach (tags, (GFunc)makeTagEntry, NULL);

	findGlobal (my_cx);

	g_list_free (symbols);
	symbols = NULL;

	g_list_free (tags);
	tags = NULL;
}

extern parserDefinition*
JavaScriptParser (void)
{
	static const char *const extensions [] = { "js", NULL };
	parserDefinition *const def = parserNew ("JavaScript");
	def->extensions = extensions;

	def->kinds = JsKinds;
	def->kindCount = KIND_COUNT (JsKinds);
	def->parser = findJsTags;
	def->initialize = initialize;

	return def;
}
