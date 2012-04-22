/*
 * python-indentation.c
 *
 * Copyright (C) 2011 - Johannes Schmid
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <ctype.h>
#include <stdlib.h>
#include <ctype.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-launcher.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/interfaces/ianjuta-iterable.h>
#include <libanjuta/interfaces/ianjuta-document.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-editor-cell.h>
#include <libanjuta/interfaces/ianjuta-editor-language.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-editor-assist.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include <libanjuta/interfaces/ianjuta-symbol.h>
#include <libanjuta/interfaces/ianjuta-language.h>

#include "python-indentation.h"

#define PREF_INDENT_AUTOMATIC "indent-automatic"
#define PREF_INDENT_ADAPTIVE "indent-adaptive"
#define PREF_INDENT_TAB_INDENTS "indent-tab-indents"
#define PREF_INDENT_BRACE_SIZE "indent-brace-size"

#define TAB_SIZE (ianjuta_editor_get_tabsize (editor, NULL))

#define USE_SPACES_FOR_INDENTATION (ianjuta_editor_get_use_spaces (editor, NULL))

#define INDENT_SIZE \
(plugin->param_statement_indentation >= 0? \
plugin->param_statement_indentation : \
g_settings_get_int (plugin->editor_settings, IANJUTA_EDITOR_INDENT_WIDTH_KEY))

#define BRACE_INDENT \
(plugin->param_brace_indentation >= 0? \
plugin->param_brace_indentation : \
g_settings_get_int (plugin->settings, PREF_INDENT_BRACE_SIZE))

#define CASE_INDENT (INDENT_SIZE)
#define LABEL_INDENT (INDENT_SIZE)

static gboolean
iter_is_newline (IAnjutaIterable *iter, gchar ch)
{
	if (ch == '\n' || ch == '\r')
		return TRUE;
	return FALSE;
}

/* Returns TRUE if iter was moved */
static gboolean
skip_iter_to_newline_head (IAnjutaIterable *iter, gchar ch)
{
	gboolean ret_val = FALSE;

	if (ch == '\n')
	{
		/* Possibly at tail */
		if (ianjuta_iterable_previous (iter, NULL))
		{
			ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter),
			                                   0, NULL);
			if (ch != '\r')
				/* Already at head, undo iter */
				ianjuta_iterable_next (iter, NULL);
			else
				/* Correctly at head */
				ret_val = TRUE;
		}
	}
	return ret_val;
}

/* Returns TRUE if iter was moved */
static gboolean
skip_iter_to_newline_tail (IAnjutaIterable *iter, gchar ch)
{
	gboolean ret_val = FALSE;

	if (ch == '\r')
	{
		/* Possibly at head */
		if (ianjuta_iterable_previous (iter, NULL))
		{
			ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter),
			                                   0, NULL);
			if (ch != '\n')
				/* Already at tail, undo iter */
				ianjuta_iterable_next (iter, NULL);
			else
				/* Correctly at tail */
				ret_val = TRUE;
		}
	}
	return ret_val;
}

/* Jumps to the reverse matching brace of the given brace character */

static gint
get_line_indentation (IAnjutaEditor *editor, gint line_num)
{
	IAnjutaIterable *line_begin, *line_end;
	gchar *line_string, *idx;
	gint line_indent = 0;

	line_begin = ianjuta_editor_get_line_begin_position (editor, line_num, NULL);
	line_end = ianjuta_editor_get_line_end_position (editor, line_num, NULL);
	/*
	 DEBUG_PRINT ("%s: line begin = %d, line end = %d", __FUNCTION__,
	              line_begin, line_end);
				  */
	if (ianjuta_iterable_compare (line_begin, line_end, NULL) == 0)
	{
		g_object_unref (line_begin);
		g_object_unref (line_end);
		return 0;
	}

	line_string = ianjuta_editor_get_text (editor, line_begin, line_end,
	                                       NULL);
	g_object_unref (line_begin);
	g_object_unref (line_end);

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
		 * increment of the utf8 chars would do */
	}
	g_free (line_string);
	return line_indent;
}

static gchar *
get_line_indentation_string (IAnjutaEditor *editor, gint spaces, gint line_indent_spaces)
{
	gint i;
	gchar *indent_string;

	if ((spaces + line_indent_spaces) <= 0)
		return NULL;

	if (USE_SPACES_FOR_INDENTATION)
	{
		indent_string = g_new0 (gchar, spaces + line_indent_spaces + 1);
		for (i = 0; i < (spaces + line_indent_spaces); i++)
			indent_string[i] = ' ';
	}
	else
	{
		gint num_tabs = spaces / TAB_SIZE;
		gint num_spaces = spaces % TAB_SIZE;
		indent_string = g_new0 (gchar, num_tabs + num_spaces + line_indent_spaces + 1);

		for (i = 0; i < num_tabs; i++)
			indent_string[i] = '\t';
		for (; i < num_tabs + (num_spaces + line_indent_spaces); i++)
			indent_string[i] = ' ';
	}
	return indent_string;
}

static void
set_indentation_param_emacs (PythonPlugin* plugin, const gchar *param,
                             const gchar *value)
{
	//DEBUG_PRINT ("Setting indent param: %s = %s", param, value);
	if (strcasecmp (param, "indent-tabs-mode") == 0)
	{
		if (strcasecmp (value, "t") == 0)
		{
			plugin->param_use_spaces = 0;
			ianjuta_editor_set_use_spaces (IANJUTA_EDITOR (plugin->current_editor),
			                               FALSE, NULL);
		}
		else if (strcasecmp (value, "nil") == 0)
		{
			plugin->param_use_spaces = 1;
			ianjuta_editor_set_use_spaces (IANJUTA_EDITOR (plugin->current_editor),
			                               TRUE, NULL);
		}
	}
	else if (strcasecmp (param, "c-basic-offset") == 0)
	{
		plugin->param_statement_indentation = atoi (value);
	}
	else if (strcasecmp (param, "tab-width") == 0)
	{
		plugin->param_tab_size = atoi (value);
		ianjuta_editor_set_tabsize (IANJUTA_EDITOR (plugin->current_editor),
		                            plugin->param_tab_size, NULL);
	}
}

static void
set_indentation_param_vim (PythonPlugin* plugin, const gchar *param,
                           const gchar *value)
{
	//DEBUG_PRINT ("Setting indent param: %s = %s", param, value);
	if (g_str_equal (param, "expandtab") ||
	    g_str_equal (param, "et"))
	{
		plugin->param_use_spaces = 1;
		ianjuta_editor_set_use_spaces (IANJUTA_EDITOR (plugin->current_editor),
		                               TRUE, NULL);
	}
	else if (g_str_equal (param, "noexpandtabs") ||
	         g_str_equal (param, "noet"))
	{
		plugin->param_use_spaces = 0;
		ianjuta_editor_set_use_spaces (IANJUTA_EDITOR (plugin->current_editor),
		                               FALSE, NULL);
	}
	if (!value)
		return;
	else if (g_str_equal (param, "shiftwidth") ||
	         g_str_equal (param, "sw"))
	{
		plugin->param_statement_indentation = atoi (value);
	}
	else if (g_str_equal (param, "softtabstop") ||
	         g_str_equal (param, "sts") ||
	         g_str_equal (param, "tabstop") ||
	         g_str_equal (param, "ts"))
	{
		plugin->param_tab_size = atoi (value);
		ianjuta_editor_set_tabsize (IANJUTA_EDITOR (plugin->current_editor),
		                            plugin->param_tab_size, NULL);
	}
}

static void
parse_mode_line_emacs (PythonPlugin *plugin, const gchar *modeline)
{
	gchar **strv, **ptr;

	strv = g_strsplit (modeline, ";", -1);
	ptr = strv;
	while (*ptr)
	{
		gchar **keyval;
		keyval = g_strsplit (*ptr, ":", 2);
		if (keyval[0] && keyval[1])
		{
			g_strstrip (keyval[0]);
			g_strstrip (keyval[1]);
			set_indentation_param_emacs (plugin, g_strchug (keyval[0]),
			                             g_strchug (keyval[1]));
		}
		g_strfreev (keyval);
		ptr++;
	}
	g_strfreev (strv);
}

static void
parse_mode_line_vim (PythonPlugin *plugin, const gchar *modeline)
{
	gchar **strv, **ptr;

	strv = g_strsplit (modeline, " ", -1);
	ptr = strv;
	while (*ptr)
	{
		gchar **keyval;
		keyval = g_strsplit (*ptr, "=", 2);
		if (keyval[0])
		{
			g_strstrip (keyval[0]);
			if (keyval[1])
			{
				g_strstrip (keyval[1]);
				set_indentation_param_vim (plugin, g_strchug (keyval[0]),
				                           g_strchug (keyval[1]));
			}
			else
				set_indentation_param_vim (plugin, g_strchug (keyval[0]),
				                           NULL);        
		}
		g_strfreev (keyval);
		ptr++;
	}
	g_strfreev (strv);
}

static gchar *
extract_mode_line (const gchar *comment_text, gboolean* vim)
{
	/* Search for emacs-like modelines */
	gchar *begin_modeline, *end_modeline;
	begin_modeline = strstr (comment_text, "-*-");
	if (begin_modeline)
	{
		begin_modeline += 3;
		end_modeline = strstr (begin_modeline, "-*-");
		if (end_modeline)
		{
			*vim = FALSE;
			return g_strndup (begin_modeline, end_modeline - begin_modeline);
		}
	}
	/* Search for vim-like modelines */
	begin_modeline = strstr (comment_text, "vim:set");
	if (begin_modeline)
	{
		begin_modeline += 7;
		end_modeline = strstr (begin_modeline, ":");
		/* Check for escape characters */
		while (end_modeline)
		{
			if (!g_str_equal ((end_modeline - 1), "\\"))
				break;
			end_modeline++;
			end_modeline = strstr (end_modeline, ":");
		}
		if (end_modeline)
		{
			gchar* vim_modeline = g_strndup (begin_modeline, end_modeline - begin_modeline);
			*vim = TRUE;
			return vim_modeline;
		}
	}
	return NULL;
}

#define MINI_BUFFER_SIZE 3

void
python_indent_init (PythonPlugin* plugin)
{
	IAnjutaIterable *iter;
	GString *comment_text;
	gboolean comment_begun = FALSE;
	gboolean line_comment = FALSE;
	gchar mini_buffer[MINI_BUFFER_SIZE] = {0};

	/* Initialize indentation parameters */
	plugin->param_tab_size = -1;
	plugin->param_statement_indentation = -1;
	plugin->param_brace_indentation = -1;
	plugin->param_case_indentation = -1;
	plugin->param_label_indentation = -1;
	plugin->param_use_spaces = -1;

	/* Find the first comment text in the buffer */
	comment_text = g_string_new (NULL);
	iter = ianjuta_editor_get_start_position (IANJUTA_EDITOR (plugin->current_editor),
	                                          NULL);
	do
	{
		gboolean shift_buffer = TRUE;
		gint i;
		gchar ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter),
		                                         0, NULL);

		for (i = 0; i < MINI_BUFFER_SIZE - 1; i++)
		{
			if (mini_buffer[i] == '\0')
			{
				mini_buffer[i] = ch;
				shift_buffer = FALSE;
				break;
			}
		}
		if (shift_buffer == TRUE)
		{
			/* Shift buffer and add */
			for (i = 0; i < MINI_BUFFER_SIZE - 1; i++)
				mini_buffer [i] = mini_buffer[i+1];
			mini_buffer[i] = ch;
		}

		if (!comment_begun && strncmp (mini_buffer, "/*", 2) == 0)
		{
			comment_begun = TRUE;
			/* Reset buffer */
			mini_buffer[0] = mini_buffer[1] = '\0';
		}
		else if (!comment_begun && strncmp (mini_buffer, "//", 2) == 0)
		                                    {
												comment_begun = TRUE;
												line_comment = TRUE;
											}
		                                    else if (!comment_begun && mini_buffer[1] != '\0')
		                                    {
												/* The buffer doesn't begin with a comment */
												break;
											}
		                                    else if (comment_begun)
		                                    {
												if ((line_comment && ch == '\n') ||
												    (!line_comment && strncmp (mini_buffer, "*/", 2) == 0))
												{
													break;
												}
											}

		                                    if (comment_begun)
		                                    g_string_append_c (comment_text, ch);

	                                    }
	while (ianjuta_iterable_next (iter, NULL));

	/* DEBUG_PRINT ("Comment text: %s", comment_text->str);*/
	if (comment_text->len > 0)
	{

		/* First comment found */
		gboolean vim;
		gchar *modeline = extract_mode_line (comment_text->str, &vim);
		if (modeline)
		{
			if (!vim)
				parse_mode_line_emacs (plugin, modeline);
			else
				parse_mode_line_vim (plugin, modeline);
			g_free (modeline);
		}
	}
	g_string_free (comment_text, TRUE);
	g_object_unref (iter);
}

static gint
set_line_indentation (IAnjutaEditor *editor, gint line_num, gint indentation, gint line_indent_spaces)
{
	IAnjutaIterable *line_begin, *line_end, *indent_position;
	IAnjutaIterable *current_pos;
	gint carat_offset, nchars = 0, nchars_removed = 0;
	gchar *old_indent_string = NULL, *indent_string = NULL;

	/* DEBUG_PRINT ("In %s()", __FUNCTION__); */
	line_begin = ianjuta_editor_get_line_begin_position (editor, line_num, NULL);
	line_end = ianjuta_editor_get_line_end_position (editor, line_num, NULL);

	/*
	 DEBUG_PRINT ("line begin = %d, line end = %d, current_pos = %d",
	              line_begin, line_end, current_pos);
				  */
	indent_position = ianjuta_iterable_clone (line_begin, NULL);

	if (ianjuta_iterable_compare (line_end, line_begin, NULL) > 0)
	{
		gchar *idx;
		gchar *line_string = ianjuta_editor_get_text (editor, line_begin,
		                                              line_end, NULL);

		//DEBUG_PRINT ("line_string = '%s'", line_string);
		if (line_string)
		{
			idx = line_string;

			/* Find first non-white space */
			while (*idx != '\0' && isspace (*idx))
			{
				idx = g_utf8_find_next_char (idx, NULL);
				ianjuta_iterable_next (indent_position, NULL);
			}
			g_free (line_string);
		}
	}
	/* Indent iter defined at this point, Identify how much is current
	 * position is beyound this point. We need to restore it later after
	 * indentation
	 */
	current_pos = ianjuta_editor_get_position (editor, NULL);
	carat_offset = ianjuta_iterable_diff (indent_position, current_pos, NULL);
	//DEBUG_PRINT ("carat offset is = %d", carat_offset);

	/* Set new indentation */
	if ((indentation + line_indent_spaces) > 0)
	{
		indent_string = get_line_indentation_string (editor, indentation, line_indent_spaces);
		nchars = indent_string ? g_utf8_strlen (indent_string, -1) : 0;

		/* Only indent if there is something to indent with */
		if (indent_string)
		{
			/* Get existing indentation */
			if (ianjuta_iterable_compare (indent_position, line_begin, NULL) > 0)
			{
				old_indent_string =
					ianjuta_editor_get_text (editor, line_begin,
					                         indent_position, NULL);

				//DEBUG_PRINT ("old_indent_string = '%s'", old_indent_string);
				nchars_removed = g_utf8_strlen (old_indent_string, -1);
			}

			/* Only indent if there was no indentation before or old
			 * indentation string was different from the new indent string
			 */
			if (old_indent_string == NULL ||
			    strcmp (old_indent_string, indent_string) != 0)
			{
				/* Remove the old indentation string, if there is any */
				if (old_indent_string)
					ianjuta_editor_erase (editor, line_begin,
					                      indent_position, NULL);

				/* Insert the new indentation string */
				ianjuta_editor_insert (editor, line_begin,
				                       indent_string, -1, NULL);
			}
		}
	}

	/* If indentation == 0, we really didn't enter the previous code block,
	 * but we may need to clear existing indentation.
	 */
	if ((indentation + line_indent_spaces) == 0)
	{
		/* Get existing indentation */
		if (ianjuta_iterable_compare (indent_position, line_begin, NULL) > 0)
		{
			old_indent_string =
				ianjuta_editor_get_text (editor, line_begin,
				                         indent_position, NULL);
			nchars_removed = g_utf8_strlen (old_indent_string, -1);
		}
		if (old_indent_string)
			ianjuta_editor_erase (editor, line_begin, indent_position, NULL);
	}

	/* Restore current position */
	if (carat_offset >= 0)
	{
		/* If the cursor was not before the first non-space character in
		 * the line, restore it's position after indentation.
		 */
		gint i;
		IAnjutaIterable *pos = ianjuta_editor_get_line_begin_position (editor, line_num, NULL);
		for (i = 0; i < nchars + carat_offset; i++)
			ianjuta_iterable_next (pos, NULL);
		ianjuta_editor_goto_position (editor, pos, NULL);
		g_object_unref (pos);
	}
	else /* cursor_offset < 0 */
	{
		/* If the cursor was somewhere in the old indentation spaces,
		 * home the cursor to first non-space character in the line (or
		                                                             * end of line if there is no non-space characters in the line.
		 */
		gint i;
		IAnjutaIterable *pos = ianjuta_editor_get_line_begin_position (editor, line_num, NULL);
		for (i = 0; i < nchars; i++)
			ianjuta_iterable_next (pos, NULL);
		ianjuta_editor_goto_position (editor, pos, NULL);
		g_object_unref (pos);
	}

	g_object_unref (current_pos);
	g_object_unref (indent_position);
	g_object_unref (line_begin);
	g_object_unref (line_end);

	g_free (old_indent_string);
	g_free (indent_string);
	return nchars;
}

/*  incomplete_statement:
 *  1 == COMPLETE STATEMENT
 *  0 == INCOMPLETE STATEMENT
 * -1 == UNKNOWN
 */

static gchar*
get_current_statement (IAnjutaEditor *editor, gint line_num)
{
	gchar point_ch;
	IAnjutaIterable *iter = ianjuta_editor_get_line_begin_position (editor, line_num, NULL);
	GString* statement = g_string_new (NULL);

	do
	{
		point_ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0, NULL);

		if (!ianjuta_iterable_next (iter, NULL) )
			break;
	} 
	while (g_ascii_isspace (point_ch) && point_ch != '\n');

	if (!ianjuta_iterable_previous (iter, NULL))
	{
		g_object_unref (iter);
		g_string_free (statement, TRUE);
		return g_strdup("");
	}

	do
	{
		point_ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0, NULL);
		g_string_append_c (statement, point_ch);

		if (!ianjuta_iterable_next (iter, NULL) )
			break;
	} 
	while (g_ascii_isalpha(point_ch) || g_ascii_isdigit(point_ch));

	g_object_unref (iter);
	return g_string_free (statement, FALSE);
}

static gchar 
get_last_char (IAnjutaEditor *editor, gint line_num, gint *found_line_num)
{
	gchar point_ch;
	IAnjutaIterable *iter = ianjuta_editor_get_line_end_position (editor, line_num, NULL);

	do
	{
		if (ianjuta_iterable_previous (iter, NULL) )
		{
			point_ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0,
			                                         NULL); 
		}
		else
			break;
	} 
	while (point_ch == ' ' || point_ch == '\n' || point_ch == '\r' || point_ch == '\t'); // Whitespace

	*found_line_num = ianjuta_editor_get_line_from_position (editor, iter, NULL);
	g_object_unref (iter);
	return point_ch;
}

static gboolean
spaces_only (IAnjutaEditor* editor, IAnjutaIterable* begin, IAnjutaIterable* end);

static gboolean
is_spaces_only (IAnjutaEditor *editor, gint line_num)
{
	IAnjutaIterable* begin = ianjuta_editor_get_line_begin_position (editor, line_num, NULL);
	IAnjutaIterable* end = ianjuta_editor_get_line_end_position (editor, line_num , NULL);

	if (spaces_only (editor, begin, end))
	{
		return TRUE;
	}

	return FALSE;
}

static gint
get_line_indentation_base (PythonPlugin *plugin,
                           IAnjutaEditor *editor,
                           gint line_num,
                           gint *incomplete_statement,
                           gint *line_indent_spaces,
                           gboolean *colon_indent)
{
	gchar point_ch;
	gint line_indent = 0;
	gint currentline = line_num - 1;
	gchar *statement;
	gchar *current_statement;

	*incomplete_statement = 0;
	*line_indent_spaces = 0;

	if (currentline <= 1)
		return 0;

	point_ch = get_last_char (editor, currentline, &currentline);
	statement = get_current_statement (editor, currentline);
	current_statement = get_current_statement (editor, line_num);

	if (g_str_equal(statement, "return") || 
	    g_str_equal(statement, "break") ||
	    g_str_equal(statement, "pass") || 
	    g_str_equal(statement, "raise") || 	    
	    g_str_equal(statement, "continue"))
	{
		if (get_line_indentation (editor, currentline)>= INDENT_SIZE)
			line_indent = get_line_indentation (editor, currentline) - INDENT_SIZE;

	}
	else if ((g_str_has_prefix(current_statement, "def") && point_ch != ':') ||
	         g_str_has_prefix(current_statement, "else") ||
	         g_str_has_prefix(current_statement, "elif") ||
	         g_str_has_prefix(current_statement, "except") ||
	         g_str_has_prefix(current_statement, "finally"))
	{
		if (get_line_indentation (editor, currentline)>= INDENT_SIZE)
			line_indent = get_line_indentation (editor, currentline) - INDENT_SIZE;
	}
	else if (point_ch == ':')
	{
		line_indent = get_line_indentation (editor, currentline) + INDENT_SIZE;
	}
	else
	{
		gint line = currentline;
		while (is_spaces_only(editor, line) && line >= 0)
			line--;
		line_indent = get_line_indentation (editor, line);
	}

	g_free (statement);
	g_free (current_statement);

	return line_indent;
}

static gboolean
spaces_only (IAnjutaEditor* editor, IAnjutaIterable* begin, IAnjutaIterable* end)
{
	gboolean empty = TRUE;
	gchar* idx;
	gchar* text = ianjuta_editor_get_text (editor, begin, end, NULL);

	if (text == NULL)
		return TRUE;


	for (idx = text; *idx != '\0'; idx++)
	{
		if (!isspace(*idx))
		{
			empty = FALSE;
			break;
		}
	}
	g_free(text);
	return empty;
}

static gint
get_line_auto_indentation (PythonPlugin *plugin, IAnjutaEditor *editor,
                           gint line, gint *line_indent_spaces)
{
	IAnjutaIterable *iter;
	gint line_indent = 0;
	gint incomplete_statement = -1;
	gboolean colon_indent = FALSE;

	g_return_val_if_fail (line > 0, 0);

	if (line == 1) /* First line */
	{
		return 0;
	}
	else
	{
		IAnjutaIterable* begin = ianjuta_editor_get_line_begin_position (editor, line -1 , NULL);
		IAnjutaIterable* end = ianjuta_editor_get_line_end_position (editor, line -1 , NULL);

		if (spaces_only (editor, begin, end))
		{
			set_line_indentation (editor, line -1, 0, 0);
		}
		g_object_unref (begin);
		g_object_unref (end);
	}

	iter = ianjuta_editor_get_line_begin_position (editor, line, NULL);

	line_indent = get_line_indentation_base (plugin, editor, line,
	                                         &incomplete_statement, 
	                                         line_indent_spaces,
	                                         &colon_indent);

	/* Determine what the first non-white char in the line is */
	do
	{
		gchar ch;
		/* Check if we are *inside* comment or string. Begining of comment
		 * or string does not count as inside. If inside, just align with
		 * previous indentation.
		 */
		ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter),
		                                   0, NULL);
		if (iter_is_newline (iter, ch))
		{
			skip_iter_to_newline_tail (iter, ch);

			/* First levels are excused from incomplete statement indent */
			if (incomplete_statement == 1 && line_indent > 0)
				line_indent += INDENT_SIZE;
			break;
		}

		else if (!isspace (ch))
		{
			/* First levels are excused from incomplete statement indent */
			if (incomplete_statement == 1 && line_indent > 0)
				line_indent += INDENT_SIZE;
			break;
		}

	}
	while (ianjuta_iterable_next (iter, NULL));
	g_object_unref (iter);

	return line_indent;
}

void
python_indent (PythonPlugin *plugin,
               IAnjutaEditor *editor,
               IAnjutaIterable *insert_pos,
               gchar ch)
{
	IAnjutaIterable *iter;
	gboolean should_auto_indent = FALSE;

	iter = ianjuta_iterable_clone (insert_pos, NULL);

	/* If autoindent is enabled*/
	if (g_settings_get_boolean (plugin->settings, PREF_INDENT_AUTOMATIC))
	{
		if (iter_is_newline (iter, ch))
		{
			skip_iter_to_newline_head (iter, ch);
			/* All newline entries means enable indenting */
			should_auto_indent = TRUE;
		}

		if (should_auto_indent)
		{
			gint insert_line;
			gint line_indent;
			gint line_indent_spaces;

			ianjuta_document_begin_undo_action (IANJUTA_DOCUMENT(editor), NULL);
			python_indent_init (plugin);

			insert_line = ianjuta_editor_get_lineno (editor, NULL);
			line_indent = get_line_auto_indentation (plugin, editor, insert_line, &line_indent_spaces);
			set_line_indentation (editor, insert_line, line_indent, line_indent_spaces);
			ianjuta_document_end_undo_action (IANJUTA_DOCUMENT(editor), NULL);
		}
	}


	g_object_unref (iter);
}

void
python_indent_auto (PythonPlugin* lang_plugin,
                    IAnjutaIterable* start,
                    IAnjutaIterable* end)
{
	gint line_start, line_end;
	gint insert_line;
	gint line_indent;
	gboolean has_selection;

	IAnjutaEditor *editor;
	editor = IANJUTA_EDITOR (lang_plugin->current_editor);

	if (!start || !end)
	{
		has_selection = ianjuta_editor_selection_has_selection
			(IANJUTA_EDITOR_SELECTION (editor), NULL);
		if (has_selection)
		{
			IAnjutaIterable *sel_start, *sel_end;
			sel_start = ianjuta_editor_selection_get_start (IANJUTA_EDITOR_SELECTION (editor),
				                                            NULL);
			sel_end = ianjuta_editor_selection_get_end (IANJUTA_EDITOR_SELECTION (editor),
				                                        NULL);
			line_start = ianjuta_editor_get_line_from_position (editor, sel_start, NULL);
			line_end = ianjuta_editor_get_line_from_position (editor, sel_end, NULL);
			g_object_unref (sel_start);
			g_object_unref (sel_end);
		}
		else
		{
			line_start = ianjuta_editor_get_lineno (IANJUTA_EDITOR(editor), NULL);
			line_end = line_start;
		}
	}
	else
	{
		line_start = ianjuta_editor_get_line_from_position (editor, start, NULL);
		line_end = ianjuta_editor_get_line_from_position (editor, end, NULL);
	}
	ianjuta_document_begin_undo_action (IANJUTA_DOCUMENT(editor), NULL);
	python_indent_init (lang_plugin);

	for (insert_line = line_start; insert_line <= line_end; insert_line++)
	{
		gint line_indent_spaces = 0;
		line_indent = get_line_auto_indentation (lang_plugin, editor,
		                                         insert_line,
		                                         &line_indent_spaces);
		set_line_indentation (editor, insert_line, line_indent, line_indent_spaces);
	}
	ianjuta_document_end_undo_action (IANJUTA_DOCUMENT(editor), NULL);
}
