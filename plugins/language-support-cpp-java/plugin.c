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
#include <libanjuta/interfaces/ianjuta-preferences.h>

#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-language-cpp-java-plugin.ui"
#define PREFS_GLADE PACKAGE_DATA_DIR"/glade/anjuta-language-cpp-java.glade"
#define ICON_FILE "anjuta-language-cpp-java-plugin.png"

/* Preferences keys */

#define PREF_INDENT_AUTOMATIC "language.cpp.indent.automatic"
#define PREF_INDENT_ADAPTIVE "language.cpp.indent.adaptive"
#define PREF_INDENT_TAB_INDENTS "language.cpp.indent.tab.indents"
#define PREF_INDENT_STATEMENT_SIZE "language.cpp.indent.statement.size"
#define PREF_INDENT_BRACE_SIZE "language.cpp.indent.brace.size"

#define TAB_SIZE (ianjuta_editor_get_tabsize (editor, NULL))
#define USE_SPACES_FOR_INDENTATION (ianjuta_editor_get_use_spaces (editor, NULL))
#define INDENT_SIZE (anjuta_preferences_get_int (plugin->prefs, PREF_INDENT_STATEMENT_SIZE))
#define BRACE_INDENT_LEFT (anjuta_preferences_get_int (plugin->prefs, PREF_INDENT_BRACE_SIZE))
#define BRACE_INDENT_RIGHT (INDENT_SIZE - BRACE_INDENT_LEFT)


#define LEFT_BRACE(ch) (ch == ')'? '(' : (ch == '}'? '{' : (ch == ']'? '[' : ch)))  
static gpointer parent_class;

/* Jumps to the reverse matching brace of the given brace character */

static gboolean
jumb_to_matching_brace (IAnjutaIterable *iter, gchar brace)
{
	gchar point_ch = brace;
	GString *braces_stack = g_string_new ("");
	
	g_return_val_if_fail (point_ch == ')' || point_ch == ']' ||
						  point_ch == '}', FALSE);
	
	/* DEBUG_PRINT ("Matching brace being"); */
	/* Push brace */
	g_string_prepend_c (braces_stack, point_ch);
	
	while (ianjuta_iterable_previous (iter, NULL))
	{
		/* Skip comments and strings */
		IAnjutaEditorAttribute attrib =
			ianjuta_editor_cell_get_attribute (IANJUTA_EDITOR_CELL (iter), NULL);
		if (attrib == IANJUTA_EDITOR_COMMENT || attrib == IANJUTA_EDITOR_STRING)
			continue;
		
		/* DEBUG_PRINT ("point ch = %c", point_ch); */
		point_ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0,
												 NULL);
		if (point_ch == ')' || point_ch == ']' || point_ch == '}')
		{
			/* Push brace */
			g_string_prepend_c (braces_stack, point_ch);
			continue;
		}
		if (point_ch == LEFT_BRACE (braces_stack->str[0]))
		{
			/* Pop brace */
			g_string_erase (braces_stack, 0, 1);
		}
		/* Bail out if there is no more in stack */
		if (braces_stack->str[0] == '\0')
		{
			/* DEBUG_PRINT ("Matching brace end -- found"); */
			return TRUE;
		}
	}
	/* DEBUG_PRINT ("Matching brace end -- not found"); */
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
	/* DEBUG_PRINT ("line begin = %d, line end = %d", line_begin, line_end); */
	
	if (line_begin == line_end)
		return 0;
	
	line_string = ianjuta_editor_get_text (editor, line_begin,
										   line_end - line_begin, NULL);
	/* DEBUG_PRINT ("line_string = '%s'", line_string); */
	
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
get_line_indentation_string (IAnjutaEditor *editor, gint spaces)
{
	gint i;
	
	/* DEBUG_PRINT ("In %s()", __FUNCTION__); */
	g_return_val_if_fail (spaces >= 0, NULL);
	
	if (spaces <= 0)
		return NULL;
	
	if (USE_SPACES_FOR_INDENTATION)
	{
		gchar *indent_string = g_new(gchar, spaces + 1);
		for (i = 0; i < spaces; i++)
			indent_string[i] = ' ';
		return indent_string;
	}
	else
	{
		gint num_tabs = spaces / TAB_SIZE;
		gint num_spaces = spaces % TAB_SIZE;
		gchar *indent_string = g_new0(gchar, num_tabs + num_spaces + 1);
		
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

	/* DEBUG_PRINT ("In %s()", __FUNCTION__); */
	
	line_indent = get_line_indentation (editor, line_num - 1);
	current_pos = ianjuta_editor_get_line_begin_position (editor, line_num,
														  NULL);
	iter = ianjuta_editor_get_cell_iter (editor, current_pos, NULL);
	while (ianjuta_iterable_previous (iter, NULL))
	{
		/* Skip comments and strings */
		IAnjutaEditorAttribute attrib =
			ianjuta_editor_cell_get_attribute (IANJUTA_EDITOR_CELL (iter), NULL);
		if (attrib == IANJUTA_EDITOR_COMMENT || attrib == IANJUTA_EDITOR_STRING)
			continue;
		
		point_ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0,
												 NULL);

		/* DEBUG_PRINT("point_ch = %c", point_ch); */
		if (point_ch == ')' || point_ch == ']' || point_ch == '}')
		{
			gint line_saved;
			
			line_saved = ianjuta_editor_get_line_from_position (editor,
								ianjuta_iterable_get_position (iter, NULL),
																	NULL);
			/* Find matching brace and continue */
			if (!jumb_to_matching_brace (iter, point_ch))
			{
				line_indent = get_line_indentation (editor, line_saved);
				break;
			}
		}
		else if (point_ch == '{')
		{
			gint line_for_indent = ianjuta_editor_get_line_from_position (editor,
								ianjuta_iterable_get_position (iter, NULL),
								NULL);
			line_indent = get_line_indentation (editor, line_for_indent);
			/* Increase line indentation */
			line_indent += BRACE_INDENT_RIGHT;
			break;
		}
		else if (point_ch == '(' || point_ch == '[')
		{
			line_indent = 0;
			while (ianjuta_iterable_previous (iter, NULL))
			{
				gchar dummy_ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0,
														 NULL);
				if (dummy_ch == '\n')
					break;
				if (dummy_ch == '\t')
					line_indent += TAB_SIZE;
				else
					line_indent++;
			}
			line_indent += 1;
			break;
		}
	}
	g_object_unref (iter);
	return line_indent;
}

static gint
set_line_indentation (IAnjutaEditor *editor, gint line_num, gint indentation)
{
	gint line_begin, line_end, indent_position;
	gchar *line_string, *idx;
	gint delta_indentation;
	gint line_indent = 0;
	gint nchars = 0;
	gchar *indent_string;
	
	/* DEBUG_PRINT ("In %s()", __FUNCTION__); */
	
	line_begin = ianjuta_editor_get_line_begin_position (editor, line_num, NULL);
	line_end = ianjuta_editor_get_line_end_position (editor, line_num, NULL);
	/* DEBUG_PRINT ("line begin = %d, line end = %d", line_begin, line_end); */
	
	indent_position = line_begin;
	
	if (line_end > line_begin)
	{
		line_string = ianjuta_editor_get_text (editor, line_begin,
											   line_end - line_begin,
											   NULL);
		
		/* DEBUG_PRINT ("line_string = '%s'", line_string); */
		if (line_string)
		{
			idx = line_string;
			
			/* Find first non-white space */
			while (*idx != '\0' && isspace (*idx))
				idx++; /* Since we are looking for first non-space char, simple
						* increment of the chars would do */
			indent_position = line_begin + (idx - line_string);
			g_free (line_string);
		}
	}
	
	/* Remove existing indentation */
	if (indent_position > line_begin)
		ianjuta_editor_erase (editor, line_begin,
							  indent_position - line_begin, NULL);
	
	/* Set new indentation */
	if (indentation > 0)
	{
		indent_string = get_line_indentation_string (editor, indentation);
		if (indent_string)
		{
			ianjuta_editor_insert (editor, line_begin,
								   indent_string, -1, NULL);
			nchars = strlen (indent_string);
			g_free (indent_string);
		}
	}
	return nchars;
}

static void
on_editor_char_inserted_cpp (IAnjutaEditor *editor,
							 gint current_pos,
							 gchar ch,
							 CppJavaPlugin *plugin)
{
	gint current_line;
	gint line_indent;
	gint nchars;
	IAnjutaIterable *iter;
	
	/* Disable for now */
	/* DEBUG_PRINT ("Indenting cpp code with char '%c'", ch); */
	
	iter = ianjuta_editor_get_cell_iter (editor, current_pos, NULL);
	
	if (ch == '\n')
	{
		IAnjutaEditorAttribute attrib =
			ianjuta_editor_cell_get_attribute (IANJUTA_EDITOR_CELL (iter), NULL);
		if (attrib == IANJUTA_EDITOR_COMMENT || attrib == IANJUTA_EDITOR_STRING)
		{
			DEBUG_PRINT ("Inside comment or string");
			current_line = ianjuta_editor_get_lineno (editor, NULL);
			line_indent = get_line_indentation_base (plugin, editor, current_line);
			DEBUG_PRINT ("Line indentation = %d", line_indent);
			nchars = set_line_indentation (editor, current_line, line_indent);
			DEBUG_PRINT ("Number of chars inserted at line %d = %d", current_line,
						 nchars);
			ianjuta_editor_goto_position (editor, current_pos + nchars, NULL);
			DEBUG_PRINT ("Cursor moved to position %d", current_pos + nchars);
		}
		else
		{
			current_line = ianjuta_editor_get_lineno (editor, NULL);
			line_indent = get_line_indentation_base (plugin, editor, current_line);
			DEBUG_PRINT ("Line indentation = %d", line_indent);
			nchars = set_line_indentation (editor, current_line, line_indent);
			DEBUG_PRINT ("Number of chars inserted at line %d = %d", current_line,
						 nchars);
			ianjuta_editor_goto_position (editor, current_pos + nchars, NULL);
			DEBUG_PRINT ("Cursor moved to position %d", current_pos + nchars);
		}
	}
	else if (ch == '}')
	{
		current_line = ianjuta_editor_get_lineno (editor, NULL);
		ianjuta_iterable_previous (iter, NULL);
		if (jumb_to_matching_brace (iter, ch))
		{
			gint line_begin;
			gint position = ianjuta_iterable_get_position (iter, NULL);
			gint line = ianjuta_editor_get_line_from_position (editor, position, NULL);
			
			DEBUG_PRINT ("Matching brace found at line %d", line);
			
			line_indent = get_line_indentation_base (plugin, editor, line);
			DEBUG_PRINT ("Line indentation = %d", line_indent);
			
			nchars = set_line_indentation (editor, current_line, line_indent);
			DEBUG_PRINT ("Number of chars inserted at line %d = %d", current_line,
						 nchars);
			
			line_begin = ianjuta_editor_get_line_begin_position (editor, current_line, NULL);
			ianjuta_editor_goto_position (editor, line_begin + nchars + 1, NULL);
			DEBUG_PRINT ("Cursor moved to position %d", line_begin + nchars + 1);
		}
	}
	g_object_unref (iter);
}

static void
on_editor_char_inserted_java (IAnjutaEditor *editor,
							  gint position,
							  gchar ch,
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

	lang_plugin->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	lang_plugin->editor_watch_id = 
		anjuta_plugin_add_watch (plugin,
								 "document_manager_current_editor",
								 on_value_added_current_editor,
								 on_value_removed_current_editor,
								 plugin);
	DEBUG_PRINT ("AnjutaLanguageCppJavaPlugin: Activated plugin.");
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

static void
ipreferences_merge (IAnjutaPreferences* ipref, AnjutaPreferences* prefs,
					GError** e)
{
	GladeXML* gxml;
		
	/* Add preferences */
	gxml = glade_xml_new (PREFS_GLADE, "preferences_dialog", NULL);
	anjuta_preferences_add_page (prefs,
								 gxml, "preferences", _("C/C++/Java"),
								 ICON_FILE);
	g_object_unref (gxml);
}

static void
ipreferences_unmerge (IAnjutaPreferences* ipref, AnjutaPreferences* prefs,
					  GError** e)
{
	anjuta_preferences_dialog_remove_page(ANJUTA_PREFERENCES_DIALOG(prefs), 
										  _("C/C++/Java"));
}

static void
ipreferences_iface_init (IAnjutaPreferencesIface* iface)
{
	iface->merge = ipreferences_merge;
	iface->unmerge = ipreferences_unmerge;	
}

ANJUTA_PLUGIN_BEGIN (CppJavaPlugin, cpp_java_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (CppJavaPlugin, cpp_java_plugin);
