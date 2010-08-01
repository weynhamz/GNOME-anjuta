/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.c
 * Copyright (C) Ishan Chattopadhyaya 2009 <ichattopadhyaya@gmail.com>
 * 
 * plugin.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * plugin.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.c.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include <config.h>
#include <ctype.h>
#include <stdlib.h>
#include <ctype.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
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

//REMOVE GLADE #include <glade/glade.h>

#include "plugin.h"
#include "python-utils.h"
#include "python-assist.h"

/* Pixmaps */
#define ANJUTA_PIXMAP_SWAP                "anjuta-swap"
#define ANJUTA_PIXMAP_COMPLETE			  "anjuta-complete"
#define ANJUTA_PIXMAP_AUTOCOMPLETE        "anjuta-complete-auto"
#define ANJUTA_PIXMAP_AUTOINDENT          "anjuta-indent-auto"
#define ANJUTA_STOCK_SWAP                 "anjuta-swap"
#define ANJUTA_STOCK_COMPLETE         	  "anjuta-complete"
#define ANJUTA_STOCK_AUTOCOMPLETE         "anjuta-autocomplete"
#define ANJUTA_STOCK_AUTOINDENT           "anjuta-indent"

#define UI_FILE PACKAGE_DATA_DIR"/ui/python-plugin.xml"
#define PROPERTIES_FILE_UI PACKAGE_DATA_DIR"/glade/python-plugin-properties.ui"
#define ICON_FILE "python_plugin.png"

/* Preferences keys */

#define PREF_INDENT_AUTOMATIC "language.python.indent.automatic"
#define PREF_INDENT_ADAPTIVE "language.python.indent.adaptive"
#define PREF_INDENT_TAB_INDENTS "language.python.indent.tab.indents"
#define PREF_INDENT_STATEMENT_SIZE "language.python.indent.statement.size"
#define PREF_INDENT_BRACE_SIZE "language.python.indent.brace.size"
#define PREF_BRACE_AUTOCOMPLETION "language.python.brace.autocompletion"

#define TAB_SIZE (ianjuta_editor_get_tabsize (editor, NULL))

#define USE_SPACES_FOR_INDENTATION (ianjuta_editor_get_use_spaces (editor, NULL))

#define INDENT_SIZE \
	(plugin->param_statement_indentation >= 0? \
		plugin->param_statement_indentation : \
		anjuta_preferences_get_int (plugin->prefs, PREF_INDENT_STATEMENT_SIZE))

#define BRACE_INDENT \
	(plugin->param_brace_indentation >= 0? \
		plugin->param_brace_indentation : \
		anjuta_preferences_get_int (plugin->prefs, PREF_INDENT_BRACE_SIZE))

#define CASE_INDENT (INDENT_SIZE)
#define LABEL_INDENT (INDENT_SIZE)

static gpointer parent_class;

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

/* Sets the iter to line end of previous line and TRUE is returned.
 * If there is no previous line, iter is set to first character in the
 * buffer and FALSE is returned.
 */
static gboolean
skip_iter_to_previous_line (IAnjutaEditor *editor, IAnjutaIterable *iter)
{
	gboolean found = FALSE;
	gchar ch;
	
	while (ianjuta_iterable_previous (iter, NULL))
	{
		ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0, NULL);
		if (iter_is_newline (iter, ch))
		{
			skip_iter_to_newline_head (iter, ch);
			found = TRUE;
			break;
		}
	}
	return found;
}

/* Returns TRUE if the line is continuation of previous line (that is, it is
 * part of the same logical line).
 */
static gboolean
line_is_continuation (IAnjutaEditor *editor, IAnjutaIterable *iter)
{
	return FALSE; 
	
	int is_continuation = FALSE;
	
	IAnjutaIterable *new_iter = ianjuta_iterable_clone (iter, NULL);
	if (skip_iter_to_previous_line (editor, new_iter))
	{
		while (ianjuta_iterable_previous (new_iter, NULL))
		{
			gchar ch = ianjuta_editor_cell_get_char
				(IANJUTA_EDITOR_CELL (new_iter), 0, NULL);
			if (ch == ' ' || ch == '\t')
				continue;
			
			if (ch == '\\')
			{
				is_continuation = TRUE;
				break;
			}
			
			if (iter_is_newline (new_iter, ch))
				break;
		}
	}
	g_object_unref (new_iter);
	return is_continuation;
}

/* Sets the iter to line end of previous logical line and TRUE is returned.
 * If there is no previous logical line, iter is set to first character in the
 * buffer and FALSE is returned. logical line is defined as one or more
 * real lines that are joined with line escapes ('\' at the end of real
 * lines.
 */
static gboolean
skip_iter_to_previous_logical_line (IAnjutaEditor *editor,
									IAnjutaIterable *iter)
{
	gboolean found = TRUE;
	
	while (line_is_continuation (editor, iter))
	{
		/*
		DEBUG_PRINT ("Line %d is continuation line .. Skipping",
					 ianjuta_editor_get_line_from_position (editor, iter, NULL));
		*/
		found = skip_iter_to_previous_line (editor, iter);
		if (!found)
			break;
	}
	/*
	DEBUG_PRINT ("Line %d is *not* continuation line .. Breaking",
				 ianjuta_editor_get_line_from_position (editor, iter, NULL));
	*/
	if (found)
		found = skip_iter_to_previous_line (editor, iter);
	/*
	DEBUG_PRINT ("Line %d is next logical line",
				 ianjuta_editor_get_line_from_position (editor, iter, NULL));
	*/
	return found;
}

static gboolean
line_is_preprocessor (IAnjutaEditor *editor, IAnjutaIterable *iter)
{return FALSE;
	gboolean is_preprocessor = FALSE;
	IAnjutaIterable *new_iter = ianjuta_iterable_clone (iter, NULL);
	
	if (skip_iter_to_previous_logical_line (editor, new_iter))
	{
		/* Forward the newline char and point to line begin of next line */
		gchar ch;
		ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (new_iter),
										   0, NULL);
		skip_iter_to_newline_tail (new_iter, ch);
		ianjuta_iterable_next (new_iter, NULL);
	}
	/* else, line is already pointed at first char of the line */
	
	do
	{
		gchar ch;
		ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (new_iter),
										   0, NULL);
/*		if (ch == '#')
		{
			is_preprocessor = TRUE;
			break;
		}*/
		if (iter_is_newline (new_iter, ch) || !isspace (ch))
			break;
	}
	while (ianjuta_iterable_next (new_iter, NULL));
	
	g_object_unref (new_iter);
	
	return is_preprocessor;
}

/* Skips to the end-of-line of previous non-preprocessor line. Any multiple
 * preprocessor lines are skipped. If current
 * line is not preprocessor line, nothing happens. If there is no previous
 * non-preprocessor line (we are at first line of the document which happens
 * to be preprocessor line), iter is set to the first character in the
 * document. It returns TRUE if the line is preprocessor line, otherwise
 * FALSE.
 */
static gboolean
skip_preprocessor_lines (IAnjutaEditor *editor, IAnjutaIterable *iter)
{
	return FALSE;
	gboolean line_found = FALSE;
	gboolean preprocessor_found = FALSE;
	IAnjutaIterable *new_iter = ianjuta_iterable_clone (iter, NULL);
	
	do
	{
		gboolean is_preprocessor = FALSE;
		if (skip_iter_to_previous_logical_line (editor, new_iter))
		{
			gchar ch;
			ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (new_iter),
											   0, NULL);
			skip_iter_to_newline_tail (new_iter, ch);
			ianjuta_iterable_next (new_iter, NULL);
		}
		do
		{
			gchar ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (new_iter),
													 0, NULL);
			if (ch == '#')
			{
				is_preprocessor = TRUE;
				/*
				DEBUG_PRINT ("Line %d is preprocessor line .. Skipping",
							 ianjuta_editor_get_line_from_position
							 (editor, new_iter, NULL));
				*/
				break;
			}
			if (iter_is_newline (new_iter, ch) || !isspace (ch))
			{
				skip_iter_to_newline_tail (new_iter, ch);
				break;
			}
		}
		while (ianjuta_iterable_next (new_iter, NULL));
		
		if (is_preprocessor)
		{
			line_found = skip_iter_to_previous_line (editor, new_iter);
			ianjuta_iterable_assign (iter, new_iter, NULL);
			preprocessor_found = TRUE;
		}
		else
		{
			/*
			DEBUG_PRINT ("Line %d is *not* preprocessor line .. Breaking",
						 ianjuta_editor_get_line_from_position
							(editor, new_iter, NULL));
			*/
			break;
		}
	}
	while (line_found);
	
	g_object_unref (new_iter);
	return preprocessor_found;
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

static void
initialize_indentation_params (PythonPlugin *plugin)
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
get_current_statement (IAnjutaEditor *editor, gint line_num, gint *found_line_num)
{
	gchar point_ch;
	IAnjutaIterable *iter = ianjuta_editor_get_line_begin_position (editor, line_num, NULL);
	gchar statement[1024];
	gint counter=0;

	do
	{
		point_ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0, NULL);

		if (!ianjuta_iterable_next (iter, NULL) )
			break;
	} while (point_ch == ' ' || point_ch == '\t'); // Whitespace

	if (!ianjuta_iterable_previous (iter, NULL))
		return "";
	
	do
	{
		point_ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0, NULL);
		statement[counter++] = point_ch;
		
		if (!ianjuta_iterable_next (iter, NULL) )
			break;
	} while (isalpha(point_ch) || isdigit(point_ch)); // FIXME: Is this UTF-8 compatible?
	statement[counter-1] = '\0';

	DEBUG_PRINT ("Statement is: *%s*\n", statement);
	g_object_unref (iter);
	return g_strdup_printf("%s", statement);
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
	IAnjutaIterable *iter;
	gchar point_ch;
	gint line_indent = 0;
	gint currentline = line_num - 1;
	gchar *last_line_statement;
	
	*incomplete_statement = 0;
	*line_indent_spaces = 0;
	
	if (currentline <= 1)
		return 0;
	
	iter = ianjuta_editor_get_line_end_position (editor, line_num, NULL);

	point_ch = get_last_char (editor, currentline, &currentline);
	last_line_statement = get_current_statement (editor, currentline, &currentline);

	if (!g_strcmp0(last_line_statement, "return") || 
	    !g_strcmp0(last_line_statement, "break") ||
	    !g_strcmp0(last_line_statement, "pass") || 
	    !g_strcmp0(last_line_statement, "raise") || 	    
	    !g_strcmp0(last_line_statement, "continue") )
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
		while (is_spaces_only(editor, line))
			line--;
		line_indent = get_line_indentation (editor, line);
	}

	return line_indent;
}

/* Check if iter is inside string. Begining of string
 * is not counted as inside.
 */
static gboolean
is_iter_inside_string (IAnjutaIterable *iter)
{
	IAnjutaEditorAttribute attrib;
	
	attrib = ianjuta_editor_cell_get_attribute (IANJUTA_EDITOR_CELL (iter),
												NULL);
	/* Check if we are *inside* string. Begining
	 * of string does not count as inside.
	 */
	if (attrib == IANJUTA_EDITOR_STRING)
	{
		/* Peek previous attrib and see what it was */
		if (ianjuta_iterable_previous (iter, NULL))
		{
			attrib = ianjuta_editor_cell_get_attribute (IANJUTA_EDITOR_CELL
														(iter),	NULL);
			if (attrib == IANJUTA_EDITOR_STRING)
			{
				/* We are inside string */
				return TRUE;
			}
			else
			{
				/* The string just began, not inside.
				 * Restore iter from the peek
				 */
				ianjuta_iterable_next (iter, NULL);
			}
		}
		/* else, there is no previous and so we can't be inside string
		 */
	}
	return FALSE;
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

/*	if (is_iter_inside_string (iter))
	{
		line_indent = get_line_indentation (editor, line - 1);
	}
	else
	{*/
		line_indent = get_line_indentation_base (plugin, editor, line,
												 &incomplete_statement, 
												 line_indent_spaces,
												 &colon_indent);
	/*}*/

	/* Determine what the first non-white char in the line is */
	do
	{
		gchar ch;
		/* Check if we are *inside* comment or string. Begining of comment
		 * or string does not count as inside. If inside, just align with
		 * previous indentation.
		 */
		/*if (is_iter_inside_string (iter))
		{
			line_indent = get_line_indentation (editor, line - 1);
			break;
		}*/
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

static void
on_editor_char_inserted_cpp (IAnjutaEditor *editor,
							 IAnjutaIterable *insert_pos,
							 gchar ch,
							 PythonPlugin *plugin)
{
	IAnjutaIterable *iter;
	gboolean should_auto_indent = FALSE;

	iter = ianjuta_iterable_clone (insert_pos, NULL);

	/* If autoindent is enabled*/
	if (anjuta_preferences_get_bool (plugin->prefs, PREF_INDENT_AUTOMATIC))
	{
	
		 DEBUG_PRINT ("Char added at position %d: '%c'", 
		              ianjuta_iterable_get_position (insert_pos, NULL), ch); 
	
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
			initialize_indentation_params (plugin);
			
			insert_line = ianjuta_editor_get_lineno (editor, NULL);
			line_indent = get_line_auto_indentation (plugin, editor, insert_line, &line_indent_spaces);
			set_line_indentation (editor, insert_line, line_indent, line_indent_spaces);
			ianjuta_document_end_undo_action (IANJUTA_DOCUMENT(editor), NULL);
		}
	}
	

	g_object_unref (iter);
}


static void
install_support (PythonPlugin *lang_plugin)
{	
	IAnjutaLanguage* lang_manager =
		anjuta_shell_get_interface (ANJUTA_PLUGIN (lang_plugin)->shell,
									IAnjutaLanguage, NULL);
	
	if (!lang_manager)
		return;
	
	if (lang_plugin->support_installed)
		return;
	
	lang_plugin->current_language = 
		ianjuta_language_get_name_from_editor (lang_manager, 
											   IANJUTA_EDITOR_LANGUAGE (lang_plugin->current_editor), NULL);
	
	DEBUG_PRINT("Language support intalled for: %s",
				lang_plugin->current_language);
	
	if (lang_plugin->current_language &&
		(g_str_equal (lang_plugin->current_language, "Python")))
	{
		g_signal_connect (lang_plugin->current_editor,
						  "char-added",
						  G_CALLBACK (on_editor_char_inserted_cpp),
						  lang_plugin);
	}
	else
	{
		return;
	}
	
	initialize_indentation_params (lang_plugin);
	/* Disable editor intern auto-indent */
	ianjuta_editor_set_auto_indent (IANJUTA_EDITOR(lang_plugin->current_editor),
								    FALSE, NULL);
	
	if (IANJUTA_IS_EDITOR_ASSIST (lang_plugin->current_editor) )
	{
		AnjutaPlugin *plugin;
		AnjutaUI *ui;
		GtkAction *action;
		IAnjutaEditorAssist* iassist;

		const gchar *project_root;
		gchar *editor_filename;		
		GValue value = {0,};

		plugin = ANJUTA_PLUGIN (lang_plugin);
		ui = anjuta_shell_get_ui (plugin->shell, NULL);
		iassist = IANJUTA_EDITOR_ASSIST (lang_plugin->current_editor);
		
		g_assert (lang_plugin->assist == NULL);

		anjuta_shell_get_value (ANJUTA_PLUGIN (lang_plugin)->shell,
						IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI, &value, NULL);
		project_root = ANJUTA_PLUGIN_PYTHON(plugin)->project_root_directory; //g_value_get_string (&value);
		editor_filename = ANJUTA_PLUGIN_PYTHON(plugin)->current_editor_filename;
		
		lang_plugin->assist = python_assist_new (iassist,
								anjuta_shell_get_interface (plugin->shell,
											IAnjutaSymbolManager,
											NULL),
							    anjuta_shell_get_interface (plugin->shell,
											IAnjutaDocumentManager,
											NULL),
								lang_plugin->prefs, editor_filename, project_root);
		
		/* Enable autocompletion action */
		action = gtk_action_group_get_action (lang_plugin->action_group, 
									   "ActionEditAutocomplete");
		g_object_set (G_OBJECT (action), "visible", TRUE,
					  "sensitive", TRUE, NULL);
	}	
		
	lang_plugin->support_installed = TRUE;
}

static void
uninstall_support (PythonPlugin *lang_plugin)
{
	if (!lang_plugin->support_installed)
		return;
	
	if (lang_plugin->current_language &&
		(g_str_equal (lang_plugin->current_language, "Python")))
	{
		g_signal_handlers_disconnect_by_func (lang_plugin->current_editor,
									G_CALLBACK (on_editor_char_inserted_cpp),
									lang_plugin);
	}
	
	if (lang_plugin->assist)
	{
		AnjutaPlugin *plugin;
		AnjutaUI *ui;
		GtkAction *action;
		
		g_object_unref (lang_plugin->assist);
		lang_plugin->assist = NULL;
		
		/* Disable autocompletion action */
		plugin = ANJUTA_PLUGIN (lang_plugin);
		ui = anjuta_shell_get_ui (plugin->shell, NULL);
		action = gtk_action_group_get_action (lang_plugin->action_group,
									   "ActionEditAutocomplete");
		g_object_set (G_OBJECT (action), "visible", FALSE,
					  "sensitive", FALSE, NULL);
	}
	
	lang_plugin->support_installed = FALSE;
}

static void
on_editor_language_changed (IAnjutaEditor *editor,
							const gchar *new_language,
							PythonPlugin *plugin)
{
	//anjuta_util_dialog_info (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
	//						 "New language is %s", new_language);

	uninstall_support (plugin);
	install_support (plugin);
}

static void
on_value_added_current_editor (AnjutaPlugin *plugin, const gchar *name,
							   const GValue *value, gpointer data)
{
	PythonPlugin *lang_plugin;
	IAnjutaDocument* doc = IANJUTA_DOCUMENT(g_value_get_object (value));
	lang_plugin = (PythonPlugin*) (plugin);

/*	anjuta_util_dialog_info (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
							 "on_value_added_current_editor");*/

	
	if (IANJUTA_IS_EDITOR(doc))
		lang_plugin->current_editor = G_OBJECT(doc);
	else
	{
		lang_plugin->current_editor = NULL;
		return;
	}
	if (IANJUTA_IS_EDITOR(lang_plugin->current_editor))
		install_support (lang_plugin);
	g_signal_connect (lang_plugin->current_editor, "language-changed",
					  G_CALLBACK (on_editor_language_changed),
					  plugin);
}

static void
on_value_removed_current_editor (AnjutaPlugin *plugin, const gchar *name,
								 gpointer data)
{
	PythonPlugin *lang_plugin;
	lang_plugin = (PythonPlugin*) (plugin);
	if (lang_plugin->current_editor)
		g_signal_handlers_disconnect_by_func (lang_plugin->current_editor,
										  G_CALLBACK (on_editor_language_changed),
										  plugin);
	if (IANJUTA_IS_EDITOR(lang_plugin->current_editor))
		uninstall_support (lang_plugin);
	lang_plugin->current_editor = NULL;
}

const gchar* SOURCE_EXT[] =
{
	".c",
	".cc",
	".C",
	".cpp",
	".cxx",
	".ccg",
	NULL
};

const gchar* HEADER_EXT[] =
{
	".h",
	".hh",
	".H",
	".hpp",
	".hxx",
	".hg",
	NULL
};

static void
on_swap_activate (GtkAction* action, gpointer data)
{
	GFile* file;
	GFile* parent;
	gchar* parent_uri;
	gchar* basename;
	gchar* ext;
	PythonPlugin *lang_plugin = (PythonPlugin*) (data);
	IAnjutaDocumentManager* docman =
		anjuta_shell_get_interface (ANJUTA_PLUGIN(lang_plugin)->shell,
									IAnjutaDocumentManager,
									NULL);
	if (!lang_plugin->current_editor || !docman)
		return;
	
	file = ianjuta_file_get_file (IANJUTA_FILE (lang_plugin->current_editor),
								  NULL);
	parent = g_file_get_parent (file);
	parent_uri = g_file_get_uri (parent);
	basename = g_file_get_basename (file);
	g_object_unref (file);
	g_object_unref (parent);
	ext = strstr (basename, ".");
	if (ext)
	{
		int i;
		for (i = 0; SOURCE_EXT[i] != NULL; i++)
		{
			if (g_str_equal (ext, SOURCE_EXT[i]))
			{
				int j;
				for (j = 0; HEADER_EXT[j] != NULL; j++)
				{
					gchar* filename;
					gchar* uri;
					GFile* new_file;
					*ext = '\0';
					filename = g_strdup_printf ("%s%s", basename, HEADER_EXT[j]);
					uri = g_build_filename (parent_uri, filename, NULL);
					new_file = g_file_new_for_uri (uri);
					g_free (uri);
					g_free(filename);
					if (g_file_query_exists (new_file, NULL))
					{
						ianjuta_document_manager_goto_file_line (docman,
																 new_file,
																 -1,
																 NULL);
						g_object_unref (new_file);
						break;
					}
					g_object_unref (new_file);
				}
				break;
			}
			if (g_str_equal (ext, HEADER_EXT[i]))
			{
				int j;
				for (j = 0; SOURCE_EXT[j] != NULL; j++)
				{
					gchar* filename;
					gchar* uri;
					GFile* new_file;
					*ext = '\0';
					filename = g_strdup_printf ("%s%s", basename, SOURCE_EXT[j]);
					uri = g_build_filename (parent_uri, filename, NULL);
					new_file = g_file_new_for_uri (uri);
					g_free (uri);
					g_free(filename);
					if (g_file_query_exists (new_file, NULL))
					{
						ianjuta_document_manager_goto_file_line (docman,
																 new_file,
																 -1,
																 NULL);
						g_object_unref (new_file);
						break;
					}
					g_object_unref (new_file);
				}
				break;
			}
		}
	}
	g_free(basename);
	g_free (parent_uri);
}

static void
on_auto_indent (GtkAction *action, gpointer data)
{
	gint line_start, line_end;
	gint insert_line;
	gint line_indent;
	gboolean has_selection;
	
	PythonPlugin *lang_plugin;
	IAnjutaEditor *editor;
	lang_plugin = (PythonPlugin*) (data);
	editor = IANJUTA_EDITOR (lang_plugin->current_editor);
	
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
	ianjuta_document_begin_undo_action (IANJUTA_DOCUMENT(editor), NULL);
	initialize_indentation_params (lang_plugin);
	
	for (insert_line = line_start; insert_line <= line_end; insert_line++)
	{
		gint line_indent_spaces = 0;
		line_indent = get_line_auto_indentation (lang_plugin, editor,
												 insert_line,
												 &line_indent_spaces);
		/* DEBUG_PRINT ("Line indent for line %d = %d", insert_line, line_indent); */
		set_line_indentation (editor, insert_line, line_indent, line_indent_spaces);
	}
	ianjuta_document_end_undo_action (IANJUTA_DOCUMENT(editor), NULL);
}
 
static void
on_auto_complete (GtkAction *action, gpointer data)
{
	PythonPlugin *lang_plugin = (PythonPlugin*) (data);;
	PythonAssist *assist = PYTHON_ASSIST (lang_plugin->assist);
	
	python_assist_create_word_completion_cache (assist);
	python_assist_update_autocomplete (assist);	



/*	if (lang_plugin->assist)
	{
//		python_assist_check (lang_plugin->assist, TRUE, TRUE, FALSE);
			dialog = gtk_message_dialog_new          (NULL,
                                             GTK_DIALOG_NO_SEPARATOR,
                                             GTK_MESSAGE_INFO,
                                              GTK_BUTTONS_OK, "%s", "on_auto_complete");
		gtk_widget_show (dialog);
	}*/
}

static GtkActionEntry actions[] = {
	{
		"ActionMenuEdit",
		NULL, N_("_Edit"),
		NULL, NULL, NULL
	},
	{
		"ActionEditAutocomplete",
		GTK_STOCK_NEW, //ANJUTA_STOCK_AUTOCOMPLETE,
		N_("_AutoComplete"), "<control>space",
		N_("AutoComplete the current word"),
		G_CALLBACK (on_auto_complete)
	},
	{
		"ActionEditAutoindent",
		GTK_STOCK_NEW, //ANJUTA_STOCK_AUTOINDENT,
		N_("Auto Indent"), "<control>i",
		N_("Auto indent current line or selection based on indentation settings"),
		G_CALLBACK (on_auto_indent)
	},
	{   "ActionFileSwap", 
		GTK_STOCK_NEW, 
		N_("Swap .h/.c"), NULL,
		N_("Swap c header and source files"),
		G_CALLBACK (on_swap_activate)
	}
};

static void
register_stock_icons (AnjutaPlugin *plugin)
{
	static gboolean registered = FALSE;

	if (registered)
		return;
	registered = TRUE;
	
	/* Register stock icons */
	BEGIN_REGISTER_ICON (plugin);
//	REGISTER_ICON_FULL (ANJUTA_PIXMAP_SWAP, ANJUTA_STOCK_SWAP);
//	REGISTER_ICON_FULL (ANJUTA_PIXMAP_COMPLETE, ANJUTA_STOCK_COMPLETE);	
//	REGISTER_ICON_FULL (ANJUTA_PIXMAP_AUTOCOMPLETE, ANJUTA_STOCK_AUTOCOMPLETE);
//	REGISTER_ICON_FULL (ANJUTA_PIXMAP_AUTOINDENT, ANJUTA_STOCK_AUTOINDENT);
	END_REGISTER_ICON;
}

// CODE from git PLUGIN
static void
on_project_root_added (AnjutaPlugin *plugin, const gchar *name, 
					   const GValue *value, gpointer user_data)
{
	PythonPlugin *python_plugin;
	gchar *project_root_uri;
	GFile *file;
	
	python_plugin = ANJUTA_PLUGIN_PYTHON (plugin);
	
	g_free (python_plugin->project_root_directory);
	project_root_uri = g_value_dup_string (value);
	file = g_file_new_for_uri (project_root_uri);
	python_plugin->project_root_directory = g_file_get_path (file);
	g_object_unref (file);
	g_free (project_root_uri);
}

static void
on_project_root_removed (AnjutaPlugin *plugin, const gchar *name, 
						 gpointer user_data)
{
	AnjutaUI *ui;
	PythonPlugin *python_plugin;
	
	python_plugin = ANJUTA_PLUGIN_PYTHON (plugin);
	
	g_free (python_plugin->project_root_directory);
	python_plugin->project_root_directory = NULL;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
}

static void
on_editor_added (AnjutaPlugin *plugin, const gchar *name, const GValue *value,
				 gpointer user_data)
{
	PythonPlugin *python_plugin;
	IAnjutaEditor *editor;
	GFile *current_editor_file;
	
	python_plugin = ANJUTA_PLUGIN_PYTHON (plugin);
	editor = g_value_get_object (value);
	
	g_free (python_plugin->current_editor_filename);	
	python_plugin->current_editor_filename = NULL;
	
	if (IANJUTA_IS_EDITOR (editor))
	{
		current_editor_file = ianjuta_file_get_file (IANJUTA_FILE (editor), 
													 NULL);

		if (current_editor_file)
		{		
			python_plugin->current_editor_filename = g_file_get_path (current_editor_file);
			g_object_unref (current_editor_file);
		}
	}
}

static void
on_editor_removed (AnjutaPlugin *plugin, const gchar *name, gpointer user_data)
{
	PythonPlugin *python_plugin;
	
	python_plugin = ANJUTA_PLUGIN_PYTHON (plugin);
	
	g_free (python_plugin->current_editor_filename);
	python_plugin->current_editor_filename = NULL;
}

static void
on_fm_file_added (AnjutaPlugin *plugin, const char *name,
				  const GValue *value, gpointer data)
{
	PythonPlugin *python_plugin;
	GFile *file;
	
	python_plugin = ANJUTA_PLUGIN_PYTHON (plugin);
	
	g_free (python_plugin->current_fm_filename);
	
	file = G_FILE (g_value_get_object (value));
	python_plugin->current_fm_filename = g_file_get_path (file);
}

static void
on_fm_file_removed (AnjutaPlugin *plugin, const char *name, gpointer data)
{
	PythonPlugin *python_plugin;
	
	python_plugin = ANJUTA_PLUGIN_PYTHON (plugin);
	
	g_free (python_plugin->current_fm_filename);
	python_plugin->current_fm_filename = NULL;
}

// end of code from git plugin



static gboolean
python_plugin_activate (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;

	GtkWidget *wid;
//	GladeXML *gxml;

	PythonPlugin *python_plugin;
	static gboolean initialized = FALSE;

	python_plugin = (PythonPlugin*) plugin;

	if (!initialized)
	{
		register_stock_icons (plugin);
	}

	
	python_plugin->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	
	/* Add all UI actions and merge UI */
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	python_plugin->action_group = 

	anjuta_ui_add_action_group_entries (ui, "ActionGroupPythonAssist",
											_("Python Assistance"),
											actions,
											G_N_ELEMENTS (actions),
											GETTEXT_PACKAGE, TRUE,
											plugin);
	python_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);

	python_plugin->editor_watch_id = 
		anjuta_plugin_add_watch (plugin,
								  IANJUTA_DOCUMENT_MANAGER_CURRENT_DOCUMENT,
								 on_value_added_current_editor,
								 on_value_removed_current_editor,
								 plugin);
	initialized = FALSE;

	/* Add watches */
	python_plugin->project_root_watch_id = anjuta_plugin_add_watch (plugin,
																 IANJUTA_PROJECT_MANAGER_PROJECT_ROOT_URI,
																 on_project_root_added,
																 on_project_root_removed,
																 NULL);
	
	python_plugin->editor_watch_id = anjuta_plugin_add_watch (plugin,
														   IANJUTA_DOCUMENT_MANAGER_CURRENT_DOCUMENT,
														   on_editor_added,
														   on_editor_removed,
														   NULL);
	
	python_plugin->fm_watch_id = anjuta_plugin_add_watch (plugin,
													   IANJUTA_FILE_MANAGER_SELECTED_FILE,
													   on_fm_file_added,
													   on_fm_file_removed,
													   NULL);
	return TRUE;
}

static gboolean
python_plugin_deactivate (AnjutaPlugin *plugin)
{

	AnjutaUI *ui;
	PythonPlugin *lang_plugin;
	lang_plugin = (PythonPlugin*) (plugin);
	DEBUG_PRINT ("%s", "PythonPlugin: Dectivating PythonPlugin plugin ...");

	anjuta_shell_remove_widget (plugin->shell, ((PythonPlugin*)plugin)->widget,
								NULL);

	anjuta_plugin_remove_watch (plugin,
								lang_plugin->editor_watch_id,
								TRUE);


	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	anjuta_ui_remove_action_group (ui, ((PythonPlugin*)plugin)->action_group);
	anjuta_ui_unmerge (ui, ((PythonPlugin*)plugin)->uiid);
	
	//lang_plugin->action_group = NULL;
	//lang_plugin->uiid = 0;
	DEBUG_PRINT ("%s", "PythonPlugin: Deactivated plugin.");
	
	return TRUE;
}

static void
python_plugin_finalize (GObject *obj)
{
	/* Finalization codes here */
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
python_plugin_dispose (GObject *obj)
{
	/* Disposition codes */
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
python_plugin_instance_init (GObject *obj)
{
	PythonPlugin *plugin = (PythonPlugin*)obj;
	plugin->action_group = NULL;
	plugin->current_editor = NULL;
	plugin->current_language = NULL;
	plugin->editor_watch_id = 0;
	plugin->uiid = 0;
	plugin->assist = NULL;

	plugin->widget = NULL;

}

static void
python_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = python_plugin_activate;
	plugin_class->deactivate = python_plugin_deactivate;
	klass->finalize = python_plugin_finalize;
	klass->dispose = python_plugin_dispose;
}


static void
ipreferences_merge (IAnjutaPreferences* ipref, AnjutaPreferences* prefs,
					GError** e)
{
	/* Add preferences */
	GtkBuilder *pref_page = gtk_builder_new();
	gchar *objects[] = {"python_preferences_dialog", NULL};
	gtk_builder_add_objects_from_file(pref_page, PROPERTIES_FILE_UI, objects, NULL);
	anjuta_preferences_add_from_builder (prefs,
								 pref_page, "preferences", _("Python"),
								 ICON_FILE);
	g_object_unref (pref_page);
}

static void
ipreferences_unmerge (IAnjutaPreferences* ipref, AnjutaPreferences* prefs,
					  GError** e)
{
	anjuta_preferences_remove_page(prefs, _("Python"));
}

static void
ipreferences_iface_init (IAnjutaPreferencesIface* iface)
{
	iface->merge = ipreferences_merge;
	iface->unmerge = ipreferences_unmerge;	
}

//ANJUTA_PLUGIN_BOILERPLATE (PythonPlugin, python_plugin);

ANJUTA_PLUGIN_BEGIN (PythonPlugin, python_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;


ANJUTA_SIMPLE_PLUGIN (PythonPlugin, python_plugin);
