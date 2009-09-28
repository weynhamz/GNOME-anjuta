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

#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-editor-assist.h>
#include <ctype.h>
#include <glib.h>

#include "code-completion.h"
#include "database-symbol.h"
#include "plugin.h"
#include "util.h"

GList*
filter_list (GList *list, gchar *prefix)
{
	GList *ret = NULL, *i;
	for (i = list; i; i = g_list_next (i))
	{
		if (!i->data)
			continue;
		if (strncmp ((gchar*)i->data, prefix, strlen (prefix)) == 0)
		{
			ret = g_list_append (ret, i->data);
		}
	}
	return ret;
}

gchar*
file_completion (IAnjutaEditor *editor, gint *cur_depth)
{
	int i;
	IAnjutaIterable *position = ianjuta_editor_get_position (IANJUTA_EDITOR (editor), NULL);
	int line = ianjuta_editor_get_line_from_position (IANJUTA_EDITOR (editor), position, NULL);
	gchar *text = ianjuta_editor_get_text (editor,
					ianjuta_editor_get_start_position (editor, NULL),
					ianjuta_editor_get_line_begin_position (editor, line, NULL),
					NULL);
	int k, j;
	if (strncmp (text, "#!/", 3) == 0) // For Seed support (Remove "#!/usr/bin/env seed" )
		strncpy (text, "//", 2);
	for (j = 0, i = 0, k = strlen (text); i < k; i++)
	{
		if (text[i] == '{')
			j++;
		if (text[i] == '}')
			j--;
		if (j < 0)
			return NULL;/*ERROR*/
	}
	gchar *tmp = g_new (gchar, j + 1);
	for (i = 0; i < j; i++)
		tmp[i] = '}';
	tmp [j] = '\0';
	tmp = g_strconcat (text, tmp, NULL);
	g_free (text);
	text = tmp;
	gchar *tmp_file = tmpnam (NULL);
	FILE *f1 = fopen (tmp_file, "w");
	fprintf (f1, "%s", text);
	fclose (f1);
	return tmp_file;
}

gchar*
code_completion_get_str (IAnjutaEditor *editor, gboolean last_dot)
{
	IAnjutaIterable *position = ianjuta_editor_get_position (IANJUTA_EDITOR (editor), NULL);

	gchar *text =  ianjuta_editor_get_text (editor,
						ianjuta_editor_get_line_begin_position (editor, 1, NULL),
						position, NULL);

	if (code_is_in_comment_or_str (text, TRUE))
	{
		g_free (text);
		return NULL;
	}

	gchar *i = strlen (text) - 1 + text;
	gchar *k = i;
	if (last_dot)
		if (*i == '.')
		{
			*i = '\0';
			i--;
		}
	enum zstate
	{
		IDENT,
		BRACE,
		AFTER_BRACE,
		EXIT
	} s;
	for (s = IDENT; i != text; i--)
	{
		switch (s)
		{
			case IDENT:
				if (*i == ')')
				{
					*k = *i;
					k--;
					s = BRACE;
					continue;
				}
				if (!isalnum (*i) && *i != '.' && *i != '_')
				{
					s = EXIT;
					break;
				}
				if (*i == ' ')
				{
					s = EXIT;
					break;
				}
				*k = *i;
				k--;
				break;
			case AFTER_BRACE:
				if (*i == ' ' || *i == '\t' || *i == '\n')
				{
					break;
				}
				i++;
				s = IDENT;
				break;
			case BRACE:
				if (*i == '(')
				{
					*k = *i;
					k--;
					s = AFTER_BRACE;
				}
				break;
			default:
				g_assert_not_reached ();
		};
		if (s == EXIT)
			break;
	}
	k++;
	i = g_strdup (k);
	g_free (text);
	g_assert (i != NULL);
	return i;
}

gchar*
code_completion_get_func_tooltip (JSLang *plugin, const gchar *var_name)
{
	if (plugin->symbol == NULL)
		plugin->symbol = database_symbol_new ();
	if (plugin->symbol == NULL)
		return NULL;

	IJsSymbol *symbol= ijs_symbol_get_member (IJS_SYMBOL (plugin->symbol), var_name);
	if (!symbol)
	{
		DEBUG_PRINT ("Can't find symbol %s", var_name);
		return NULL;
	}

	GList *args = ijs_symbol_get_arg_list (symbol);

	GList *i;
	gchar *res = NULL;
	for (i = args; i; i = g_list_next (i))
	{
		if (!res)
			res = i->data;
		else
		{
			gchar * t = g_strdup_printf ("%s, %s", res, (gchar*)i->data);
			g_free (res);
			res = t;
		}
	}
	g_object_unref (symbol);
	return res;
}

gboolean
code_completion_is_symbol_func (JSLang *plugin, const gchar *var_name)
{
	if (plugin->symbol == NULL)
		plugin->symbol = database_symbol_new ();
	if (plugin->symbol == NULL)
		return FALSE;

	IJsSymbol *symbol= ijs_symbol_get_member (IJS_SYMBOL (plugin->symbol), var_name);
	if (!symbol)
	{
		DEBUG_PRINT ("Can't find symbol %s", var_name);
		return FALSE;
	}
	g_object_unref (symbol);
	return ijs_symbol_get_base_type (symbol) == BASE_FUNC;
}

GList*
code_completion_get_list (JSLang *plugin, const gchar *tmp_file, const gchar *var_name, gint depth_level)
{
	GList *suggestions = NULL;
	if (plugin->symbol == NULL)
		plugin->symbol = database_symbol_new ();
	if (plugin->symbol == NULL)
		return NULL;
	database_symbol_set_file (plugin->symbol, tmp_file);

	if (!var_name || strlen (var_name) == 0)
		return database_symbol_list_member_with_line (plugin->symbol,
			ianjuta_editor_get_lineno (IANJUTA_EDITOR (plugin->current_editor), NULL));
	IJsSymbol *t = ijs_symbol_get_member (IJS_SYMBOL (plugin->symbol), var_name);//TODO:Corect
	if (t)
	{
		suggestions = ijs_symbol_list_member (IJS_SYMBOL (t));
		g_object_unref (t);
	}
	return suggestions;
}

