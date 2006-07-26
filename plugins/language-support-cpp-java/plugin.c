/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2000 Naba Kumar

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <config.h>
#include <ctype.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-iterable.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-cell.h>
#include <libanjuta/interfaces/ianjuta-editor-language.h>

#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-language-cpp-java-plugin.ui"

#define TAB_SIZE 4
#define INDENT_SIZE 4
#define BRACE_INDENT_LEFT 4
#define BRACE_INDENT_RIGHT BRACE_INDENT_LEFT
#define USE_SPACES_FOR_INDENTATION FALSE

static gpointer parent_class;

/* Jumps to the reverse matching brace of the given brace character */

static gboolean
jumb_to_matching_brace (IAnjutaIterable *iter, gchar brace)
{
	gchar point_ch = brace;
	GString *braces_stack = g_string_new ("");
	
	g_return_val_if_fail (point_ch == ')' || point_ch == ']' ||
						  point_ch == '}', FALSE);
	
	/* Push brace */
	g_string_prepend_c (braces_stack, point_ch);
	
	while (ianjuta_iterable_previous (iter, NULL))
	{
		point_ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0,
												 NULL);
		if (point_ch == ')' || point_ch == ']' || point_ch == '}')
		{
			/* Push brace */
			g_string_prepend_c (braces_stack, point_ch);
			continue;
		}
		if (point_ch == braces_stack->str[0])
		{
			/* Pop brace */
			g_string_erase (braces_stack, 0, 1);
		}
		/* Bail out if there is no more in stack */
		if (braces_stack->str[0] == '\0')
			return TRUE;
	}
				
	return FALSE;
}

static gint
get_line_indentation (IAnjutaEditor *editor, gint line_num)
{
	gint line_begin, line_end;
	gchar *line_string, *idx;
	gint line_indent = 0;
	
	line_begin = ianjuta_editor_get_line_begin_position (editor, line_num, NULL);
	line_end = ianjuta_editor_get_line_end_position (editor, line_num, NULL);
	if (line_begin == line_end)
		return 0;
	line_string = ianjuta_editor_get_text (editor, line_begin, line_end, NULL);
	
	if (!line_string)
		return 0;
	
	idx = line_string;
	
	/* Find first non-white space */
	while (*idx != '\0' && isspace (*idx))
	{
		if (*idx == '\t')
			line_indent += TAB_SIZE;
		else
			line_indent++;
		idx++; /* Since we are looking for first non-space char, simple
	            * increment of the chars would do */
	}
	g_free (line_string);
	return line_indent;
}

static gchar *
get_line_indentation_string (gint spaces)
{
	gint i;
	
	g_return_val_if_fail (spaces >= 0, NULL);
	
	if (spaces <= 0)
		return NULL;
	
	if (USE_SPACES_FOR_INDENTATION)
	{
		gchar *indent_string = g_new(gchar, spaces);
		for (i = 0; i < spaces; i++)
			indent_string[i] = ' ';
		return indent_string;
	}
	else
	{
		gint num_tabs = spaces / INDENT_SIZE;
		gint num_spaces = spaces % INDENT_SIZE;
		gchar *indent_string = g_new0(gchar, num_tabs + num_spaces);
		
		for (i = 0; i < num_tabs; i++)
			indent_string[i] = '\t';
		for (; i < num_tabs + num_spaces; i++)
			indent_string[i] = ' ';
		return indent_string;
	}
}

static gint
get_line_indentation_base (CppJavaPlugin *plugin,
						   IAnjutaEditor *editor,
						   gint line_num)
{
	IAnjutaIterable *iter;
	gchar point_ch;
	gint current_pos;
	gint line_indent = 0;

	line_indent = get_line_indentation (editor, line_num - 1);
	current_pos = ianjuta_editor_get_line_begin_position (editor, line_num,  NULL);
	iter = ianjuta_editor_get_cell_iter (editor, current_pos, NULL);
	while (ianjuta_iterable_previous (iter, NULL))
	{
		point_ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0,
												 NULL);
		DEBUG_PRINT("point_ch = %c", point_ch);
		if (point_ch == ')' || point_ch == ']' || point_ch == '}')
		{
			/* Find matching brace and continue */
			if (!jumb_to_matching_brace (iter, point_ch))
				return line_indent;
		}
		else if (point_ch == '{')
		{
			/* Increase line indentation */
			return line_indent + BRACE_INDENT_RIGHT;
		}
		else if (point_ch == '(' || point_ch == '[')
		{
			return line_indent + 1;
		}
	}
	
	return line_indent;
}

static void
on_editor_char_inserted_cpp (IAnjutaEditor *editor,
							 const  gint current_pos, 
							 const gchar ch,
							 CppJavaPlugin *plugin)
{
	gint current_line;
	gint line_indent;
	gchar *indent_string;
	
	DEBUG_PRINT ("Indenting cpp code, char: %c", ch);
	
	if (ch == '\n')
	{
		current_line = ianjuta_editor_get_lineno (editor, NULL);
		line_indent = get_line_indentation_base (plugin, editor, current_line);
		indent_string = get_line_indentation_string (line_indent);
		if (indent_string != NULL)
		{
			ianjuta_editor_insert (editor, current_pos, indent_string, -1, NULL);
			g_free(indent_string);
		}
	}
}

static void
on_editor_char_inserted_java (IAnjutaEditor *editor,
							  const int current_pos,
							  const gchar ch,
							  CppJavaPlugin *plugin)
{
	if (ch != '\n' || ch != '\t')
		return;
	DEBUG_PRINT ("Indenting java code");
}

static void
install_support (CppJavaPlugin *lang_plugin)
{
	if (lang_plugin->support_installed)
		return;
	
	lang_plugin->current_language
		= ianjuta_editor_language_get_language
					(IANJUTA_EDITOR_LANGUAGE (lang_plugin->current_editor),
											  NULL);
	
	DEBUG_PRINT("Language: %s", lang_plugin->current_language);
	
	if (lang_plugin->current_language &&
		(strcmp (lang_plugin->current_language, "cpp") == 0
		|| strcmp (lang_plugin->current_language, "C") == 0
		|| strcmp (lang_plugin->current_language, "C++") == 0))
	{
		g_signal_connect (lang_plugin->current_editor,
						  "char-added",
						  G_CALLBACK (on_editor_char_inserted_cpp),
						  lang_plugin);
	}
	else if (lang_plugin->current_language &&
		(strcmp (lang_plugin->current_language, "java") == 0
		|| strcmp (lang_plugin->current_language, "Java") == 0))
	{
		g_signal_connect (lang_plugin->current_editor,
						  "char-added",
						  G_CALLBACK (on_editor_char_inserted_java),
						  lang_plugin);
	}
	else
	{
		return;
	}
	lang_plugin->support_installed = TRUE;
}

static void
uninstall_support (CppJavaPlugin *lang_plugin)
{
	if (!lang_plugin->support_installed)
		return;
	
	if (lang_plugin->current_language &&
		strcmp (lang_plugin->current_language, "cpp") == 0)
	{
		g_signal_handlers_disconnect_by_func (lang_plugin->current_editor,
									G_CALLBACK (on_editor_char_inserted_cpp),
									lang_plugin);
	}
	else if (lang_plugin->current_language &&
		strcmp (lang_plugin->current_language, "java") == 0)
	{
		g_signal_handlers_disconnect_by_func (lang_plugin->current_editor,
									G_CALLBACK (on_editor_char_inserted_java),
									lang_plugin);
	}
	lang_plugin->support_installed = FALSE;
}

static void
on_editor_language_changed (IAnjutaEditor *editor,
							const gchar *new_language,
							CppJavaPlugin *plugin)
{
	uninstall_support (plugin);
	install_support (plugin);
}

static void
on_value_added_current_editor (AnjutaPlugin *plugin, const gchar *name,
							   const GValue *value, gpointer data)
{
	CppJavaPlugin *lang_plugin;
	lang_plugin = (CppJavaPlugin*) plugin;
	lang_plugin->current_editor = g_value_get_object (value);
	install_support (lang_plugin);
	g_signal_connect (lang_plugin->current_editor, "language-changed",
					  G_CALLBACK (on_editor_language_changed),
					  plugin);
}

static void
on_value_removed_current_editor (AnjutaPlugin *plugin, const gchar *name,
								 gpointer data)
{
	CppJavaPlugin *lang_plugin;
	lang_plugin = (CppJavaPlugin*) plugin;
	g_signal_handlers_disconnect_by_func (lang_plugin->current_editor,
										  G_CALLBACK (on_editor_language_changed),
										  plugin);
	uninstall_support (lang_plugin);
	lang_plugin->current_editor = NULL;
}

static gboolean
cpp_java_plugin_activate_plugin (AnjutaPlugin *plugin)
{
	CppJavaPlugin *lang_plugin;
	lang_plugin = (CppJavaPlugin*) plugin;
	
	DEBUG_PRINT ("AnjutaLanguageCppJavaPlugin: Activating plugin ...");
	
	lang_plugin->editor_watch_id = 
		anjuta_plugin_add_watch (plugin,
								 "document_manager_current_editor",
								 on_value_added_current_editor,
								 on_value_removed_current_editor,
								 plugin);
	return TRUE;
}

static gboolean
cpp_java_plugin_deactivate_plugin (AnjutaPlugin *plugin)
{
	CppJavaPlugin *lang_plugin;
	lang_plugin = (CppJavaPlugin*) plugin;
	anjuta_plugin_remove_watch (plugin,
								lang_plugin->editor_watch_id,
								TRUE);
	return TRUE;
}

static void
cpp_java_plugin_finalize (GObject *obj)
{
	/* Finalization codes here */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
cpp_java_plugin_dispose (GObject *obj)
{
	/* Disposition codes */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
cpp_java_plugin_instance_init (GObject *obj)
{
	/* CppJavaPlugin *plugin = (CppJavaPlugin*)obj; */
}

static void
cpp_java_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = cpp_java_plugin_activate_plugin;
	plugin_class->deactivate = cpp_java_plugin_deactivate_plugin;
	klass->finalize = cpp_java_plugin_finalize;
	klass->dispose = cpp_java_plugin_dispose;
}

ANJUTA_PLUGIN_BOILERPLATE (CppJavaPlugin, cpp_java_plugin);
ANJUTA_SIMPLE_PLUGIN (CppJavaPlugin, cpp_java_plugin);
