/* -*- Mode:bosys C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <config.h>
#include <ctype.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-iterable.h>
#include <libanjuta/interfaces/ianjuta-document.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-editor-cell.h>
#include <libanjuta/interfaces/ianjuta-editor-language.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-editor-assist.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include <libanjuta/interfaces/ianjuta-symbol.h>
#include <libanjuta/interfaces/ianjuta-language.h>

#include "plugin.h"
#include "cpp-java-utils.h"

#define ANJUTA_PIXMAP_COMPLETE			  "anjuta-complete-16.png"
#define ANJUTA_PIXMAP_AUTOCOMPLETE        "anjuta-complete-auto-16.png"
#define ANJUTA_PIXMAP_AUTOINDENT          "anjuta-indent-auto-16.png"
#define ANJUTA_STOCK_COMPLETE         	  "anjuta-complete"
#define ANJUTA_STOCK_AUTOCOMPLETE         "anjuta-autocomplete"
#define ANJUTA_STOCK_AUTOINDENT           "anjuta-indent"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-language-support-cpp-java.ui"
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
	gint line_begin, line_end;
	gchar *line_string, *idx;
	gint line_indent = 0;
	
	line_begin = ianjuta_editor_get_line_begin_position (editor, line_num, NULL);
	line_end = ianjuta_editor_get_line_end_position (editor, line_num, NULL);
	/*
	DEBUG_PRINT ("%s: line begin = %d, line end = %d", __FUNCTION__,
				 line_begin, line_end);
	*/
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
		gchar *indent_string = g_new0 (gchar, spaces + 1);
		for (i = 0; i < spaces; i++)
			indent_string[i] = ' ';
		return indent_string;
	}
	else
	{
		gint num_tabs = spaces / TAB_SIZE;
		gint num_spaces = spaces % TAB_SIZE;
		gchar *indent_string = g_new0 (gchar, num_tabs + num_spaces + 1);
		
		for (i = 0; i < num_tabs; i++)
			indent_string[i] = '\t';
		for (; i < num_tabs + num_spaces; i++)
			indent_string[i] = ' ';
		return indent_string;
	}
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
					 ianjuta_editor_get_line_from_position
						(editor, ianjuta_iterable_get_position (iter, NULL),
						 NULL));
		*/
		found = skip_iter_to_previous_line (editor, iter);
		if (!found)
			break;
	}
	/*
	DEBUG_PRINT ("Line %d is *not* continuation line .. Breaking",
				 ianjuta_editor_get_line_from_position
					(editor, ianjuta_iterable_get_position (iter, NULL),
					 NULL));
	*/
	if (found)
		found = skip_iter_to_previous_line (editor, iter);
	/*
	DEBUG_PRINT ("Line %d is next logical line",
				 ianjuta_editor_get_line_from_position
					(editor, ianjuta_iterable_get_position (iter, NULL),
					 NULL));
	*/
	return found;
}

static gboolean
line_is_preprocessor (IAnjutaEditor *editor, IAnjutaIterable *iter)
{
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
		if (ch == '#')
		{
			is_preprocessor = TRUE;
			break;
		}
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
								(editor, ianjuta_iterable_get_position (new_iter, NULL),
								 NULL));
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
							(editor, ianjuta_iterable_get_position (new_iter, NULL),
							 NULL));
			*/
			break;
		}
	}
	while (line_found);
	
	g_object_unref (new_iter);
	return preprocessor_found;
}

static void
set_indentation_param (CppJavaPlugin* plugin, const gchar *param,
					   const gchar *value)
{
	DEBUG_PRINT ("Setting indent param: %s = %s", param, value);
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
parse_mode_line (CppJavaPlugin *plugin, const gchar *modeline)
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
			set_indentation_param (plugin, g_strchug (keyval[0]),
                                   g_strchug (keyval[1]));
		}
		g_strfreev (keyval);
		ptr++;
	}
	g_strfreev (strv);
}

static gchar *
extract_mode_line (const gchar *comment_text)
{
	gchar *begin_modeline, *end_modeline;
	begin_modeline = strstr (comment_text, "-*-");
	if (!begin_modeline)
		return NULL;
	begin_modeline += 3;
	end_modeline = strstr (begin_modeline, "-*-");
	if (!end_modeline)
		return NULL;
	return g_strndup (begin_modeline, end_modeline - begin_modeline);
}

#define MINI_BUFFER_SIZE 3

static void
initialize_indentation_params (CppJavaPlugin *plugin)
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
	iter = ianjuta_editor_get_cell_iter (IANJUTA_EDITOR (plugin->current_editor),
										 0, NULL);
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
		gchar *modeline = extract_mode_line (comment_text->str);
		if (modeline)
		{
			parse_mode_line (plugin, modeline);
			g_free (modeline);
		}
	}
	g_string_free (comment_text, TRUE);
	g_object_unref (iter);
}

static gint
set_line_indentation (IAnjutaEditor *editor, gint line_num, gint indentation)
{
	gint line_begin, line_end, indent_position;
	gint current_pos;
	gchar *line_string, *idx;
	gint nchars = 0, nchars_removed = 0;
	gchar *old_indent_string = NULL, *indent_string = NULL;
	
	/* DEBUG_PRINT ("In %s()", __FUNCTION__); */
	current_pos = ianjuta_editor_get_position (editor, NULL);
	line_begin = ianjuta_editor_get_line_begin_position (editor, line_num, NULL);
	line_end = ianjuta_editor_get_line_end_position (editor, line_num, NULL);
	
	/*
	DEBUG_PRINT ("line begin = %d, line end = %d, current_pos = %d",
				 line_begin, line_end, current_pos);
	*/
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
	
	/* Set new indentation */
	if (indentation > 0)
	{
		indent_string = get_line_indentation_string (editor,
													 indentation);
		nchars = strlen (indent_string);
		
		/* Only indent if there is something to indent with */
		if (indent_string)
		{
			/* Get existing indentation */
			if (indent_position > line_begin)
			{
				old_indent_string = ianjuta_editor_get_text (editor, line_begin,
															 indent_position - line_begin,
															 NULL);
				nchars_removed = strlen (old_indent_string);
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
										  indent_position - line_begin, NULL);
				
				/* Insert the new indentation string */
				ianjuta_editor_insert (editor, line_begin,
									   indent_string, -1, NULL);
			}
		}
	}
	
	/* If indentation == 0, we really didn't enter the previous code block,
	 * but we may need to clear existing indentation.
	 */
	if (indentation == 0)
	{
		/* Get existing indentation */
		if (indent_position > line_begin)
		{
			old_indent_string = ianjuta_editor_get_text (editor, line_begin,
														 indent_position - line_begin,
														 NULL);
			nchars_removed = strlen (old_indent_string);
		}
		if (old_indent_string)
			ianjuta_editor_erase (editor, line_begin,
								  indent_position - line_begin, NULL);
				
	}
	
	/* Restore current position */
	if (current_pos >= indent_position)
	{
		/* If the cursor was not before the first non-space character in
		 * the line, restore it's position after indentation.
		 */
		gint delta_position = nchars - nchars_removed;
		ianjuta_editor_goto_position (editor, current_pos + delta_position,
									  NULL);
		/*
		DEBUG_PRINT ("Restored position to %d", current_pos + delta_position);
		*/
	}
	else if (current_pos >= line_begin)
	{
		/* If the cursor was somewhere in the old indentation spaces,
		 * home the cursor to first non-space character in the line (or
		 * end of line if there is no non-space characters in the line.
		 */
		ianjuta_editor_goto_position (editor, current_pos + nchars,
									  NULL);
	}
	
	g_free (old_indent_string);
	g_free (indent_string);
	return nchars;
}

/*  incomplete_statement:
 *  1 == COMPLETE STATEMENT
 *  0 == INCOMPLETE STATEMENT
 * -1 == UNKNOWN
 */
static gint
get_line_indentation_base (CppJavaPlugin *plugin,
						   IAnjutaEditor *editor,
						   gint line_num,
						   gint *incomplete_statement)
{
	IAnjutaIterable *iter;
	gchar point_ch;
	gint position;
	gint line_indent = 0;
	gboolean looking_at_just_next_line = TRUE;
	gboolean current_line_is_preprocessor = FALSE;
	gboolean current_line_is_continuation = FALSE;
	
	*incomplete_statement = -1;
	
	if (line_num <= 1)
		return 0;
	
	/* DEBUG_PRINT ("In %s()", __FUNCTION__); */
	
	position = ianjuta_editor_get_line_begin_position (editor, line_num,
													   NULL);
	iter = ianjuta_editor_get_cell_iter (editor, position, NULL);
	
	current_line_is_preprocessor = line_is_preprocessor (editor, iter);
	current_line_is_continuation = line_is_continuation (editor, iter);
	/*
	DEBUG_PRINT ("Current line is preprocessor = %d",
				 current_line_is_preprocessor);
	DEBUG_PRINT ("Current line is continuation = %d",
				 current_line_is_continuation);
	*/
	/* line_indent = get_line_indentation (editor, line_num - 1); */
	
	if (current_line_is_preprocessor && current_line_is_continuation)
	{
		/* Continuation of preprocessor line -- just maintain indentation */
		g_object_unref (iter);
		return get_line_indentation (editor, line_num - 1);;
	}
	else if (current_line_is_preprocessor)
	{
		/* Preprocessor line -- indentation should be 0 */
		g_object_unref (iter);
		return 0;
	}
	
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
			
			/* If we encounter a block-end before anything else, the
			 * statement could hardly be incomplte.
			 */
			if (point_ch == '}' && *incomplete_statement == -1)
				*incomplete_statement = 0;
			
			/* If at level 0 indentation, encoutered a
			 * block end, don't bother going further
			 */
			if (point_ch == '}' && get_line_indentation (editor, line_saved) <= 0)
			{
				line_indent = 0;
				break;
			}
			
			/* Find matching brace and continue */
			if (!cpp_java_util_jump_to_matching_brace (iter, point_ch, -1))
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
			line_indent += INDENT_SIZE;
			
			/* If we encounter a block-start before anything else, the
			 * statement could hardly be incomplte.
			 */
			if (point_ch == '}' && *incomplete_statement == -1)
				*incomplete_statement = 0;
			
			break;
		}
		else if (point_ch == '(' || point_ch == '[')
		{
			line_indent = 0;
			while (ianjuta_iterable_previous (iter, NULL))
			{
				gchar dummy_ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0,
														 NULL);
				if (iter_is_newline (iter, dummy_ch))
				{
					skip_iter_to_newline_head (iter, dummy_ch);
					break;
				}
				if (dummy_ch == '\t')
					line_indent += TAB_SIZE;
				else
					line_indent++;
			}
			line_indent += 1;
			
			/* Although statement is incomplete at this point, we don't
			 * set it to incomplete and just leave it to unknown to avaoid
			 * increating indentation for it, because incomplete braces,
			 * overrides any existing indentation
			 */
			*incomplete_statement = -1;
			break;
		}
		else if (point_ch == ';' || point_ch == ',')
		{
			/* If we encounter statement-end before any non-whitespace
			 * char, the statement is complete.
			 */
			if (*incomplete_statement == -1)
				*incomplete_statement = 0;
		}
		else if (iter_is_newline (iter, point_ch))
		{
			skip_iter_to_newline_head (iter, point_ch);
			
			/* We just crossed a line boundary. Skip any preprocessor lines,
			 * and ensure that line_indent is updated with correct real
			 * previous non-preprocessor line.
			 */
			if (skip_preprocessor_lines (editor, iter) &&
				looking_at_just_next_line)
			{
				/*
				gint line = ianjuta_editor_get_line_from_position
					(editor, ianjuta_iterable_get_position (iter, NULL), NULL);
				line_indent = get_line_indentation (editor, line);
				*/
			}
			looking_at_just_next_line = FALSE;
		}
		else if (!isspace (point_ch))
		{
			/* If we encounter any non-whitespace char before any of the
			 * statement-complete indicators, the statement is basically
			 * incomplete
			 */
			if (*incomplete_statement == -1)
				*incomplete_statement = 1;
		}
	}
	g_object_unref (iter);
	
	return line_indent;
}

/* Check if iter is inside comment or string. Begining of comment or string
 * is not counted as inside.
 */
static gboolean
is_iter_inside_comment_or_string (IAnjutaIterable *iter)
{
	IAnjutaEditorAttribute attrib;
	
	attrib = ianjuta_editor_cell_get_attribute (IANJUTA_EDITOR_CELL (iter),
												NULL);
	/* Check if we are *inside* comment or string. Begining of comment
	 * or string does not count as inside.
	 */
	if (attrib == IANJUTA_EDITOR_COMMENT || attrib == IANJUTA_EDITOR_STRING)
	{
		/* Peek previous attrib and see what it was */
		if (ianjuta_iterable_previous (iter, NULL))
		{
			attrib = ianjuta_editor_cell_get_attribute (IANJUTA_EDITOR_CELL
														(iter),	NULL);
			if (attrib == IANJUTA_EDITOR_COMMENT ||
				attrib == IANJUTA_EDITOR_STRING)
			{
				/* We are inside comment or string */
				return TRUE;
			}
			else
			{
				/* The comment or string just began, not inside.
				 * Restore iter from the peek
				 */
				ianjuta_iterable_next (iter, NULL);
			}
		}
		/* else, there is no previous and so we can't be inside comment
		 * or string
		 */
	}
	return FALSE;
}

static gint
get_line_auto_indentation (CppJavaPlugin *plugin, IAnjutaEditor *editor,
						   gint line)
{
	IAnjutaIterable *iter;
	gint line_indent = 0;
	gint line_begin;
	gint incomplete_statement = -1;
	
	g_return_val_if_fail (line > 1, 0);
	
	line_begin = ianjuta_editor_get_line_begin_position (editor, line, NULL);
	iter = ianjuta_editor_get_cell_iter (editor, line_begin, NULL);
	
	if (is_iter_inside_comment_or_string (iter))
	{
		line_indent = get_line_indentation (editor, line - 1);
	}
	else
	{
		line_indent = get_line_indentation_base (plugin, editor, line,
												 &incomplete_statement);
	}
	
	/* Determine what the first non-white char in the line is */
	do
	{
		gchar ch;
		/* Check if we are *inside* comment or string. Begining of comment
		 * or string does not count as inside. If inside, just align with
		 * previous indentation.
		 */
		if (is_iter_inside_comment_or_string (iter))
		{
			line_indent = get_line_indentation (editor, line - 1);
			break;
		}
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
		
		if (ch == '{')
		{
			if (line_indent > 0)
			{
				/* The first level braces are excused from brace indentation */
				/*
				DEBUG_PRINT ("Increasing indent level from %d to %d",
							 line_indent,
							 line_indent + BRACE_INDENT);
				*/
				line_indent += BRACE_INDENT;
			}
			break;
		}
		else if (ch == '}')
		{
			ianjuta_iterable_previous (iter, NULL);
			if (cpp_java_util_jump_to_matching_brace (iter, ch, -1))
			{
				gint position = ianjuta_iterable_get_position (iter, NULL);
				gint line = ianjuta_editor_get_line_from_position (editor,
																   position,
																   NULL);
				line_indent = get_line_indentation (editor, line);
			}
			break;
		}
		else if (ch == '#')
		{
			line_indent = 0;
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
							 gint insert_pos,
							 gchar ch,
							 CppJavaPlugin *plugin)
{
	IAnjutaEditorAttribute attrib;
	IAnjutaIterable *iter;
	gboolean should_auto_indent = FALSE;
	
	/* Do nothing if automatic indentation is not enabled */
	if (!anjuta_preferences_get_int (plugin->prefs, PREF_INDENT_AUTOMATIC))
		return;
	
	/* DEBUG_PRINT ("Char added at position %d: '%c'", insert_pos, ch); */
	
	iter = ianjuta_editor_get_cell_iter (editor, insert_pos, NULL);
	
	if (iter_is_newline (iter, ch))
	{
		skip_iter_to_newline_head (iter, ch);
		/* All newline entries means enable indenting */
		should_auto_indent = TRUE;
	}
	else if (ch == '{' || ch == '}' || ch == '#')
	{
		/* Indent only when it's the first non-white space char in the line */
		
		/* Don't bother if we are inside comment or string */
		attrib = ianjuta_editor_cell_get_attribute (IANJUTA_EDITOR_CELL (iter),
													NULL);
		if (attrib != IANJUTA_EDITOR_COMMENT &&
			attrib != IANJUTA_EDITOR_STRING)
		{
			/* Iterate backwards till the begining of the line and disable
			 * indenting if any non-white space char is encountered
			 */
			
			/* Begin by assuming it should be indented */
			should_auto_indent = TRUE;
			
			while (ianjuta_iterable_previous (iter, NULL))
			{
				ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter),
												   0, NULL);
				
				/* DEBUG_PRINT ("Looking at char '%c'", ch); */
				
				/* Break on begining of line (== end of previous line) */
				if (iter_is_newline (iter, ch))
				{
					skip_iter_to_newline_head (iter, ch);
					break;
				}
				/* If a non-white space char is encountered, disabled indenting */
				if (!isspace (ch))
				{
					should_auto_indent = FALSE;
					break;
				}
			}
		}
	}
#if 0
	else if (ch == '\t' &&
			 anjuta_preferences_get_int (plugin->prefs,
										 PREF_INDENT_TAB_INDENTS))
	{
		/* Indent on tab enabled */
		/* Don't bother if we are inside comment or string */
		attrib = ianjuta_editor_cell_get_attribute (IANJUTA_EDITOR_CELL (iter),
													NULL);
		if (attrib != IANJUTA_EDITOR_COMMENT &&
			attrib != IANJUTA_EDITOR_STRING)
		{
			
			/* Ensure that we remove the inserted tab */
			ianjuta_editor_erase (editor, insert_pos, 1, NULL);
			
			should_auto_indent = TRUE;
		}
	}
#endif
	if (should_auto_indent)
	{
		gint insert_line;
		gint line_indent;
		
		ianjuta_document_begin_undo_action (IANJUTA_DOCUMENT(editor), NULL);
		initialize_indentation_params (plugin);
		insert_line = ianjuta_editor_get_lineno (editor, NULL);
		line_indent = get_line_auto_indentation (plugin, editor, insert_line);
		set_line_indentation (editor, insert_line, line_indent);
		ianjuta_document_end_undo_action (IANJUTA_DOCUMENT(editor), NULL);
	}
	g_object_unref (iter);
}

static void
on_editor_char_inserted_java (IAnjutaEditor *editor,
							  gint insert_pos,
							  gchar ch,
							  CppJavaPlugin *plugin)
{
	on_editor_char_inserted_cpp (editor, insert_pos, ch, plugin);
}

static void
install_support (CppJavaPlugin *lang_plugin)
{
	const gchar* lang;
	IAnjutaLanguageId id;
	
	IAnjutaLanguage* lang_manager =
		anjuta_shell_get_interface (ANJUTA_PLUGIN (lang_plugin)->shell,
									IAnjutaLanguage, NULL);
	
	if (!lang_manager)
		return;
	
	if (lang_plugin->support_installed)
		return;
	
	lang = ianjuta_editor_language_get_language
					(IANJUTA_EDITOR_LANGUAGE (lang_plugin->current_editor),
											  NULL);
	id = ianjuta_language_get_from_string (lang_manager, lang, NULL);
	lang_plugin->current_language = 
		ianjuta_language_get_name (lang_manager, id, NULL);
	
	DEBUG_PRINT("Language support intalled for: %s",
				lang_plugin->current_language);
	
	if (lang_plugin->current_language &&
		(g_str_equal (lang_plugin->current_language, "C")
		|| g_str_equal (lang_plugin->current_language, "C++")))
	{
		g_signal_connect (lang_plugin->current_editor,
						  "char-added",
						  G_CALLBACK (on_editor_char_inserted_cpp),
						  lang_plugin);
	}
	else if (lang_plugin->current_language &&
		(g_str_equal (lang_plugin->current_language, "Java")))
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
	
	initialize_indentation_params (lang_plugin);
	if (IANJUTA_IS_EDITOR_ASSIST (lang_plugin->current_editor))
	{
		AnjutaPlugin *plugin;
		AnjutaUI *ui;
		GtkAction *action;
		IAnjutaEditorAssist* iassist;
		
		plugin = ANJUTA_PLUGIN (lang_plugin);
		ui = anjuta_shell_get_ui (plugin->shell, NULL);
		iassist = IANJUTA_EDITOR_ASSIST (lang_plugin->current_editor);
		
		g_assert (lang_plugin->assist == NULL);
		
		lang_plugin->assist =
			cpp_java_assist_new (iassist,
				anjuta_shell_get_interface (plugin->shell,
											IAnjutaSymbolManager,
											NULL),
								 lang_plugin->prefs);
		
		/* Enable autocompletion action */
		action = gtk_action_group_get_action (lang_plugin->action_group, 
									   "ActionEditAutocomplete");
		g_object_set (G_OBJECT (action), "visible", TRUE,
					  "sensitive", TRUE, NULL);
	}
	/* Disable editor intern auto-indent */
	ianjuta_editor_set_auto_indent (IANJUTA_EDITOR(lang_plugin->current_editor),
								    FALSE, NULL);
	lang_plugin->support_installed = TRUE;
}

static void
uninstall_support (CppJavaPlugin *lang_plugin)
{
	if (!lang_plugin->support_installed)
		return;
	
	if (lang_plugin->current_language &&
		(g_str_equal (lang_plugin->current_language, "C")
		|| g_str_equal (lang_plugin->current_language, "C++")))
	{
		g_signal_handlers_disconnect_by_func (lang_plugin->current_editor,
									G_CALLBACK (on_editor_char_inserted_cpp),
									lang_plugin);
	}
	else if (lang_plugin->current_language &&
		(g_str_equal (lang_plugin->current_language, "Java")))
	{
		g_signal_handlers_disconnect_by_func (lang_plugin->current_editor,
									G_CALLBACK (on_editor_char_inserted_java),
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
	IAnjutaDocument* doc = IANJUTA_DOCUMENT(g_value_get_object (value));
	lang_plugin = ANJUTA_PLUGIN_CPP_JAVA (plugin);
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
	CppJavaPlugin *lang_plugin;
	lang_plugin = ANJUTA_PLUGIN_CPP_JAVA (plugin);
	if (lang_plugin->current_editor)
		g_signal_handlers_disconnect_by_func (lang_plugin->current_editor,
										  G_CALLBACK (on_editor_language_changed),
										  plugin);
	if (IANJUTA_IS_EDITOR(lang_plugin->current_editor))
		uninstall_support (lang_plugin);
	lang_plugin->current_editor = NULL;
}

static void
on_auto_indent (GtkAction *action, gpointer data)
{
	gint sel_start, sel_end;
	gint line_start, line_end;
	gint insert_line;
	gint line_indent;
	gboolean has_selection;
	
	CppJavaPlugin *lang_plugin;
	IAnjutaEditor *editor;
	lang_plugin = ANJUTA_PLUGIN_CPP_JAVA (data);
	editor = IANJUTA_EDITOR (lang_plugin->current_editor);
	
	has_selection = ianjuta_editor_selection_has_selection
						(IANJUTA_EDITOR_SELECTION (editor), NULL);
	if (has_selection)
	{
		sel_start = ianjuta_editor_selection_get_start (IANJUTA_EDITOR_SELECTION (editor),
														NULL);
		sel_end = ianjuta_editor_selection_get_end (IANJUTA_EDITOR_SELECTION (editor),
													NULL);
		line_start = ianjuta_editor_get_line_from_position (editor,
															sel_start,
															NULL);
		line_end = ianjuta_editor_get_line_from_position (editor,
														  sel_end,
														  NULL);
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
		line_indent = get_line_auto_indentation (lang_plugin, editor,
												 insert_line);
		DEBUG_PRINT ("Line indent for line %d = %d", insert_line, line_indent);
		set_line_indentation (editor, insert_line, line_indent);
	}
	ianjuta_document_end_undo_action (IANJUTA_DOCUMENT(editor), NULL);
}

static void
on_auto_complete (GtkAction *action, gpointer data)
{
	CppJavaPlugin *lang_plugin;
	lang_plugin = ANJUTA_PLUGIN_CPP_JAVA (data);
	if (lang_plugin->assist)
		cpp_java_assist_check (lang_plugin->assist, TRUE, TRUE);
}

static GtkActionEntry actions[] = {
	{
		"ActionMenuEdit",
		NULL, N_("_Edit"),
		NULL, NULL, NULL
	},
	{
		"ActionEditAutocomplete",
		ANJUTA_STOCK_AUTOCOMPLETE,
		N_("_AutoComplete"), "<control>Return",
		N_("AutoComplete the current word"),
		G_CALLBACK (on_auto_complete)
	},
	{
		"ActionEditAutoindent",
		ANJUTA_STOCK_AUTOINDENT,
		N_("Auto Indent"), "<control>i",
		N_("Auto indent current line or selection based on indentation settings"),
		G_CALLBACK (on_auto_indent)
	}
};

#define REGISTER_ICON(icon, stock_id) \
	pixbuf = gdk_pixbuf_new_from_file (PACKAGE_PIXMAPS_DIR"/"icon, NULL); \
	icon_set = gtk_icon_set_new_from_pixbuf (pixbuf); \
	gtk_icon_factory_add (icon_factory, stock_id, icon_set); \
	g_object_unref (pixbuf);

static void
register_stock_icons (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	GtkIconFactory *icon_factory;
	GtkIconSet *icon_set;
	GdkPixbuf *pixbuf;
	static gboolean registered = FALSE;

	if (registered)
		return;
	registered = TRUE;

	/* Register stock icons */
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	icon_factory = anjuta_ui_get_icon_factory (ui);
	REGISTER_ICON (ANJUTA_PIXMAP_COMPLETE, ANJUTA_STOCK_COMPLETE);	
	REGISTER_ICON (ANJUTA_PIXMAP_AUTOCOMPLETE, ANJUTA_STOCK_AUTOCOMPLETE);
	REGISTER_ICON (ANJUTA_PIXMAP_AUTOINDENT, ANJUTA_STOCK_AUTOINDENT);
}

static gboolean
cpp_java_plugin_activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	CppJavaPlugin *lang_plugin;
	static gboolean initialized = FALSE;
	
	lang_plugin = ANJUTA_PLUGIN_CPP_JAVA (plugin);
	
	DEBUG_PRINT ("AnjutaLanguageCppJavaPlugin: Activating plugin ...");

	if (!initialized)
	{
		register_stock_icons (plugin);
	}

	lang_plugin->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	lang_plugin->action_group = 
		anjuta_ui_add_action_group_entries (ui, "ActionGroupCppJavaAssist",
											_("Cpp/Java Assistance"),
											actions,
											G_N_ELEMENTS (actions),
											GETTEXT_PACKAGE, TRUE,
											plugin);
	lang_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);
	
	lang_plugin->editor_watch_id = 
		anjuta_plugin_add_watch (plugin,
								 "document_manager_current_editor",
								 on_value_added_current_editor,
								 on_value_removed_current_editor,
								 plugin);
	initialized = FALSE;
	return TRUE;
}

static gboolean
cpp_java_plugin_deactivate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	CppJavaPlugin *lang_plugin;
	lang_plugin = ANJUTA_PLUGIN_CPP_JAVA (plugin);
	
	anjuta_plugin_remove_watch (plugin,
								lang_plugin->editor_watch_id,
								TRUE);
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	anjuta_ui_unmerge (ui, lang_plugin->uiid);
	anjuta_ui_remove_action_group (ui, lang_plugin->action_group);
	
	lang_plugin->action_group = NULL;
	lang_plugin->uiid = 0;
	DEBUG_PRINT ("AnjutaLanguageCppJavaPlugin: Deactivated plugin.");
	return TRUE;
}

static void
cpp_java_plugin_finalize (GObject *obj)
{
	/* CppJavaPlugin* plugin = ANJUTA_PLUGIN_CPP_JAVA (obj); */

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
	CppJavaPlugin *plugin = ANJUTA_PLUGIN_CPP_JAVA (obj);
	plugin->action_group = NULL;
	plugin->current_editor = NULL;
	plugin->current_language = NULL;
	plugin->editor_watch_id = 0;
	plugin->uiid = 0;
	plugin->assist = NULL;
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
	anjuta_preferences_remove_page(prefs, _("C/C++/Java"));
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
