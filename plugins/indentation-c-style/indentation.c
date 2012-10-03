/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * indent-c-indentation.c
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

#include <config.h>
#include <ctype.h>
#include <stdlib.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/interfaces/ianjuta-iterable.h>
#include <libanjuta/interfaces/ianjuta-document.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-editor-language.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include <libanjuta/interfaces/ianjuta-language.h>

#include "indentation.h"

#define PREF_INDENT_BRACE_SIZE "indent-brace-size"
#define PREF_INDENT_PARANTHESE_LINEUP "indent-paranthese-lineup"
#define PREF_INDENT_PARANTHESE_SIZE "indent-paranthese-size"
#define PREF_BRACE_AUTOCOMPLETION "brace-autocompletion"
#define PREF_COMMENT_LEADING_ASTERISK "multiline-leading-asterisk"

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
is_closing_bracket(gchar ch)
{
	if (ch == ')' || ch == '}' || ch == ']')
		return TRUE;
	return FALSE;
}

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
	gint line_indent = 0, left_braces = 0, right_braces = 0;
	gchar ch;

	line_end = ianjuta_editor_get_line_end_position (editor, line_num, NULL);

	/* Find the line which contains the left brace matching last right brace on current line */

	do
	{
		while (ianjuta_iterable_previous (line_end, NULL))
		{
			ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (line_end), 0, NULL);
			if (ch == ')')
				right_braces++;
			if (ch == '(')
				left_braces++;
			if (iter_is_newline (line_end, ch))
			{
				break;
			}
		}
		if (right_braces != left_braces)
		{
			line_num --;
			g_object_unref (line_end);
			line_end = ianjuta_editor_get_line_end_position (editor, line_num, NULL);
		}
	} while (right_braces != left_braces && line_num >= 0);

	g_object_unref (line_end);
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

static gint
set_line_indentation (IAnjutaEditor *editor, gint line_num, gint indentation, gint line_indent_spaces)
{
	IAnjutaIterable *line_begin, *line_end, *indent_position;
	IAnjutaIterable *current_pos;
	gint carat_offset, nchars = 0;
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
		ianjuta_document_begin_undo_action (IANJUTA_DOCUMENT(editor), NULL);
		ianjuta_editor_goto_position (editor, pos, NULL);
		ianjuta_document_end_undo_action (IANJUTA_DOCUMENT(editor), NULL);
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
		ianjuta_document_begin_undo_action (IANJUTA_DOCUMENT(editor), NULL);
		ianjuta_editor_goto_position (editor, pos, NULL);
		ianjuta_document_end_undo_action (IANJUTA_DOCUMENT(editor), NULL);
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
static gint
get_line_indentation_base (IndentCPlugin *plugin,
						   IAnjutaEditor *editor,
						   gint line_num,
						   gint *incomplete_statement,
						   gint *line_indent_spaces,
						   gboolean *colon_indent)
{
	IAnjutaIterable *iter;
	gchar point_ch;
	gint line_indent = 0;
	gint extra_indent = 0;
	gboolean looking_at_just_next_line = TRUE;
	gboolean current_line_is_preprocessor = FALSE;
	gboolean current_line_is_continuation = FALSE;
	gboolean line_checked_for_comment = FALSE;

    /* Determine whether or not to add multi-line comment asterisks */
	const gchar *comment_continued = " * ";
	IAnjutaIterable *line_begin = ianjuta_editor_get_line_begin_position (editor, line_num, NULL);
	IAnjutaIterable  *line_end = ianjuta_editor_get_line_end_position (editor, line_num, NULL);

	*incomplete_statement = -1;
	*line_indent_spaces = 0;

	if (line_num <= 1)
		return 0;

	/* DEBUG_PRINT ("In %s()", __FUNCTION__); */

	iter = ianjuta_editor_get_line_begin_position (editor, line_num, NULL);

	current_line_is_preprocessor = line_is_preprocessor (editor, iter);
	current_line_is_continuation =
		line_is_continuation (editor, iter);
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
		return get_line_indentation (editor, line_num - 1);
	}
	else if (current_line_is_preprocessor)
	{
		/* Preprocessor line -- indentation should be 0 */
		g_object_unref (iter);
		return 0;
	}

	while (ianjuta_iterable_previous (iter, NULL))
	{
		/* Skip strings */
		IAnjutaEditorAttribute attrib =
			ianjuta_editor_cell_get_attribute (IANJUTA_EDITOR_CELL (iter), NULL);
		if (attrib == IANJUTA_EDITOR_STRING)
			continue;

		point_ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0,
												 NULL);

		/* DEBUG_PRINT("point_ch = %c", point_ch); */

		/* Check for line comment comment */
		if (!line_checked_for_comment && !isspace(point_ch))
		{
			gboolean comment = FALSE;
			IAnjutaIterable* new_iter = ianjuta_iterable_clone (iter, NULL);
			do
			{
				gchar c;

				/* Skip strings */
				if (ianjuta_editor_cell_get_attribute (IANJUTA_EDITOR_CELL (new_iter), NULL) == IANJUTA_EDITOR_STRING)
					continue;

				c = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (new_iter), 0,
												  NULL);

				if (iter_is_newline (new_iter, c))
				{
					line_checked_for_comment = TRUE;
					break;
				}
				if (c == '/')
				{
					IAnjutaIterable* tmp_iter = ianjuta_iterable_clone (new_iter, NULL);
					if (!ianjuta_iterable_previous (tmp_iter, NULL))
					{
						g_object_unref (tmp_iter);
						break;
					}
					c = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (tmp_iter), 0,
													  NULL);
					if (c == '/')
					{
						/* is a line comment, skip until begin of comment */
						comment = TRUE;
						g_object_unref (tmp_iter);
						break;
					}
					g_object_unref (tmp_iter);
				}
			} while (ianjuta_iterable_previous (new_iter, NULL));
			if (comment)
			{
				ianjuta_iterable_assign (iter, new_iter, NULL);
				ianjuta_iterable_previous (iter, NULL);
				g_object_unref (new_iter);
				continue;
			}
			g_object_unref (new_iter);
		}
		/* Check if we are inside a comment */
		if (point_ch == '/' || point_ch == '*')
		{
			gboolean comment = FALSE;
			gboolean comment_end = FALSE;
			IAnjutaIterable* new_iter = ianjuta_iterable_clone (iter, NULL);
			do
			{
				gchar c = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL(new_iter),
												  0, NULL);
				if (!comment_end && iter_is_newline (new_iter, c))
				{
					break;
				}
				if (c == '*')
				{
					IAnjutaIterable* prev = ianjuta_iterable_clone (new_iter, NULL);
					IAnjutaIterable* next = ianjuta_iterable_clone (new_iter, NULL);
					ianjuta_iterable_previous (prev, NULL);
					ianjuta_iterable_next (next, NULL);
					gchar prev_c = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (prev), 0,
													  NULL);
					gchar next_c = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (next), 0,
													  NULL);
					if (prev_c == '/')
					{
						/* starts comment */
						comment = TRUE;
						if (!comment_end)
						{
							extra_indent++;

							/* If a multiline comment is continuing, check the next line and insert " * "
							 * only if it does not already exist there. The purpose of this fix is to avoid
							 * extra " * " on auto-indent. */

							if ((g_settings_get_boolean (plugin->settings, PREF_COMMENT_LEADING_ASTERISK)) &&
								(ianjuta_iterable_compare (line_end, line_begin, NULL)) == 0)
							{
								ianjuta_editor_insert (editor, line_begin, comment_continued, -1, NULL);
							}

							/* In the middle of a comment we can't know
						     * if the statement is incomplete
							 */
							*incomplete_statement = -1;

							/* ":" have to be ignored inside comments */
							if (*colon_indent)
							{
								*colon_indent = FALSE;
								extra_indent -= INDENT_SIZE;
							}
						}
						g_object_unref (prev);
						g_object_unref (next);
						break;

					}
					else if (next_c == '/')
					{
						/* ends comment: */
						comment_end = TRUE;
						g_object_unref (prev);
						g_object_unref (next);
						continue;
					}
					/* Possibly continued comment */
					else if (isspace(prev_c))
					{
						gboolean possible_comment = FALSE;
						while (ianjuta_iterable_previous (prev, NULL))
						{
							prev_c = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (prev), 0,
															  	   NULL);
							if (!isspace(prev_c))
								break;
							if (iter_is_newline (prev, prev_c))
							{
								possible_comment = TRUE;
								break;
							}
						}
						if (possible_comment)
						{
							ianjuta_iterable_assign (new_iter, prev, NULL);
							g_object_unref (prev);
							g_object_unref (next);
							continue;
						}
					}
					g_object_unref (prev);
					g_object_unref (next);
				}
			} while (ianjuta_iterable_previous (new_iter, NULL));
			if (comment)
			{
				ianjuta_iterable_assign (iter, new_iter, NULL);
				ianjuta_iterable_previous (iter, NULL);
				g_object_unref (new_iter);
				continue;
			}
			g_object_unref (new_iter);
		}
		if (point_ch == ')' || point_ch == ']' || point_ch == '}')
		{
			gint line_saved;

			line_saved = ianjuta_editor_get_line_from_position (editor, iter,
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
				line_indent += extra_indent;
				break;
			}

			/* Find matching brace and continue */
			if (!anjuta_util_jump_to_matching_brace (iter, point_ch, -1))
			{
				line_indent = get_line_indentation (editor, line_saved);
				line_indent += extra_indent;
				break;
			}
		}
		else if (point_ch == '{')
		{
			gint line_for_indent =
				ianjuta_editor_get_line_from_position (editor, iter, NULL);
			line_indent = get_line_indentation (editor, line_for_indent);
			/* Increase line indentation */
			line_indent += INDENT_SIZE;
			line_indent += extra_indent;

			/* If we encounter a block-start before anything else, the
			 * statement could hardly be incomplte.
			 */
			if (point_ch == '{' && *incomplete_statement == -1)
				*incomplete_statement = 0;

			break;
		}
		else if (point_ch == '(' || point_ch == '[')
		{
			line_indent = 0;
			if (g_settings_get_boolean (plugin->settings,
			                            PREF_INDENT_PARANTHESE_LINEUP))
			{
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
						(*line_indent_spaces)++;
				}
				(*line_indent_spaces)++;
				line_indent += extra_indent;
			}
			else
			{
				gint line_for_indent =
					ianjuta_editor_get_line_from_position (editor, iter, NULL);
				line_indent = get_line_indentation (editor, line_for_indent);
				line_indent += extra_indent;

				(*line_indent_spaces) += g_settings_get_int (plugin->settings,
				                                             PREF_INDENT_PARANTHESE_SIZE);
			}

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
		else if (point_ch == ':' && *colon_indent == FALSE)
		{
			/* This is a forward reference, all lines below should have
			 * increased indentation until the next statement has
			 * a ':'
			 * If current line indentation is zero, that we don't indent
			 */
			IAnjutaIterable* new_iter = ianjuta_iterable_clone (iter, NULL);
			IAnjutaIterable* line_begin;
			gboolean indent = FALSE;
			gchar c;

			/* Is the last non-whitespace in line */
			while (ianjuta_iterable_next (new_iter, NULL))
			{
				c = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (new_iter),
													   0, NULL);
				if (!isspace(c))
					break;
				if (iter_is_newline (new_iter, c))
				{
					indent = TRUE;
					break;
				}
			}
			line_begin = ianjuta_editor_get_line_begin_position(editor,
																ianjuta_editor_get_line_from_position(editor, iter, NULL),
																NULL);
			c = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (line_begin),
														0, NULL);
			if (indent)
			{
				*colon_indent = TRUE;
				if (*incomplete_statement == -1)
					*incomplete_statement = 0;
			}
			if (indent && isspace(c))
			{
				extra_indent += INDENT_SIZE;
			}
			g_object_unref (new_iter);
			g_object_unref (line_begin);
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
				gint line = ianjuta_editor_get_line_from_position (editor, iter, NULL);
				line_indent = get_line_indentation (editor, line);
				*/
			}
			looking_at_just_next_line = FALSE;
			line_checked_for_comment = FALSE;
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
	if (!line_indent && extra_indent)
	{
		line_indent += extra_indent;
	}
	g_object_unref (iter);

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
get_line_auto_indentation (IndentCPlugin *plugin, IAnjutaEditor *editor,
						   gint line, gint *line_indent_spaces)
{
	IAnjutaIterable *iter;
	IAnjutaIterable *end_iter;
	gint line_indent = 0;
	gint incomplete_statement = -1;
	gboolean colon_indent = FALSE;

	g_return_val_if_fail (line > 0, 0);

	/* be sure to set a default if we're in the first line otherwise
	 * the pointer'll be left hanging with no value.
	 */
	*line_indent_spaces = 0;

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

	if (is_iter_inside_string (iter))
	{
		line_indent = get_line_indentation (editor, line - 1);
	}
	else
	{
		line_indent = get_line_indentation_base (plugin, editor, line,
												 &incomplete_statement,
												 line_indent_spaces,
												 &colon_indent);
	}

	if (colon_indent)
	{
		/* If the last non-whitespace character in the line is ":" then
		 * we remove the extra colon_indent
		 */
		end_iter = ianjuta_editor_get_line_end_position (editor, line, NULL);
		gchar ch;
		while (ianjuta_iterable_previous (end_iter, NULL))
		{
			ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (end_iter),
			                                   0, NULL);
			if (ch == ':')
			{
				line_indent -= INDENT_SIZE;
				break;
			}
			if (!isspace(ch) || iter_is_newline (end_iter, ch))
				break;
		}
		g_object_unref (end_iter);
	}

	/* Determine what the first non-white char in the line is */
	do
	{
		gchar ch;
		/* Check if we are *inside* comment or string. Begining of comment
		 * or string does not count as inside. If inside, just align with
		 * previous indentation.
		 */
		if (is_iter_inside_string (iter))
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
				/* It looks ugly to add extra indent after case: so remove that */
				if (colon_indent)
					line_indent -= INDENT_SIZE;
			}
			break;
		}
		else if (ch == '}')
		{
			ianjuta_iterable_previous (iter, NULL);
			if (anjuta_util_jump_to_matching_brace (iter, ch, -1))
			{
				gint line = ianjuta_editor_get_line_from_position (editor,
																   iter,
																   NULL);
				line_indent = get_line_indentation (editor, line);
			}
			break;
		}
		else if (ch == '#')
		{
			line_indent = 0;
			*line_indent_spaces = 0;
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
insert_editor_blocked (IAnjutaEditor* editor,
                       IAnjutaIterable* iter,
                       gchar* text,
                       IndentCPlugin* plugin)
{
	g_signal_handlers_block_by_func (editor, cpp_java_indentation_char_added, plugin);
	ianjuta_editor_insert (editor, iter, text, -1, NULL);
	g_signal_handlers_unblock_by_func (editor, cpp_java_indentation_char_added, plugin);
}

void
cpp_java_indentation_char_added (IAnjutaEditor *editor,
                                 IAnjutaIterable *insert_pos,
                                 gchar ch,
                                 IndentCPlugin *plugin)
{
	IAnjutaEditorAttribute attrib;
	IAnjutaIterable *iter;
	gboolean should_auto_indent = FALSE;

	iter = ianjuta_iterable_clone (insert_pos, NULL);

	/* If autoindent is enabled*/
	if (plugin->smart_indentation)
	{

		/* DEBUG_PRINT ("Char added at position %d: '%c'", insert_pos, ch); */

		if (iter_is_newline (iter, ch))
		{
			skip_iter_to_newline_head (iter, ch);
			/* All newline entries means enable indenting */
			should_auto_indent = TRUE;
		}
		else if (ch == '{' || ch == '}' || ch == '#')
		{
			/* Indent only when it's the first non-white space char in the line */

			/* Don't bother if we are inside string */
			attrib = ianjuta_editor_cell_get_attribute (IANJUTA_EDITOR_CELL (iter),
			                                            NULL);
			if (attrib != IANJUTA_EDITOR_STRING)
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

					//DEBUG_PRINT ("Looking at char '%c'", ch);

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
		if (should_auto_indent)
		{
			gint insert_line;
			gint line_indent;
			gint line_indent_spaces;

			ianjuta_document_begin_undo_action (IANJUTA_DOCUMENT(editor), NULL);

			insert_line = ianjuta_editor_get_lineno (editor, NULL);
			line_indent = get_line_auto_indentation (plugin, editor, insert_line, &line_indent_spaces);
			set_line_indentation (editor, insert_line, line_indent, line_indent_spaces);
			ianjuta_document_end_undo_action (IANJUTA_DOCUMENT(editor), NULL);
		}
	}

	if (g_settings_get_boolean (plugin->settings, PREF_BRACE_AUTOCOMPLETION))
	{
		if (ch == '[' || ch == '(')
		            {
						gchar *prev_char, *next_char;
						IAnjutaIterable *previous, *next, *next_end;

						previous = ianjuta_iterable_clone (iter, NULL);
						ianjuta_iterable_previous (previous, NULL);
						prev_char = ianjuta_editor_get_text (editor, previous, iter, NULL);

						next = ianjuta_iterable_clone (iter, NULL);
						ianjuta_iterable_next (next, NULL);
						next_end = ianjuta_iterable_clone (next, NULL);
						ianjuta_iterable_next (next_end, NULL);
						next_char = ianjuta_editor_get_text (editor, next, next_end, NULL);

						/* If the previous char is a ' we don't have to autocomplete,
						   also we only autocomplete if the next character is white-space
						   a closing bracket, "," or ";" */
						if (*prev_char != '\'' &&
						    (g_ascii_isspace(*next_char) || is_closing_bracket (*next_char) ||
							 *next_char == ',' || *next_char == ';'|| *next_char == '\0'))
						{
							ianjuta_document_begin_undo_action (IANJUTA_DOCUMENT (editor), NULL);
							ianjuta_iterable_next (iter, NULL);
							switch (ch)
							{
								case '[':
									       insert_editor_blocked (editor, iter,
									                              "]", plugin);
									       break;
								case '(':
									       insert_editor_blocked (editor, iter,
									                       ")", plugin);
									       break;
								default:
									       break;
							}
							ianjuta_editor_goto_position (editor, iter, NULL);
							ianjuta_document_end_undo_action (IANJUTA_DOCUMENT (editor), NULL);
						}
						g_object_unref (previous);
					}
		            else if (ch == '"' || ch == '\'')
		            {
						gchar *prev_char;
						IAnjutaIterable *previous;

						previous = ianjuta_iterable_clone (iter, NULL);
						ianjuta_iterable_previous (previous, NULL);
						prev_char = ianjuta_editor_get_text (editor, previous, iter, NULL);

						/* First iter*/
						ianjuta_iterable_next (iter, NULL);

						/*
						 * If the character is " we have to decide if we need insert
						 * another " or we have to skip the character
						 */
						if (ch == '"' || ch == '\'')
						{
							/*
							 * Now we have to detect if we want to manage " as a char
							 */
							if (*prev_char != '\'' && *prev_char != '\\')
							{
								gchar *c;

								if (ch == '"')
									c = g_strdup ("\"");
								else c = g_strdup ("'");

								ianjuta_document_begin_undo_action (IANJUTA_DOCUMENT (editor), NULL);
								insert_editor_blocked (editor, iter, c, plugin);
								ianjuta_editor_goto_position (editor, iter, NULL);
								ianjuta_document_end_undo_action (IANJUTA_DOCUMENT (editor), NULL);

								g_free (c);
							}
							g_object_unref (previous);
							g_object_unref (iter);
							return;
						}
						g_object_unref (previous);
					}
	            }
	g_object_unref (iter);
}

static void
erase_editor_blocked (IAnjutaEditor* editor,
                      IAnjutaIterable* start,
                      IAnjutaIterable* end,
                      IndentCPlugin* plugin)
{
	g_signal_handlers_block_by_func (editor, cpp_java_indentation_changed, plugin);
	ianjuta_editor_erase (editor, start, end, NULL);
	g_signal_handlers_unblock_by_func (editor, cpp_java_indentation_changed, plugin);
}

void
cpp_java_indentation_changed (IAnjutaEditor *editor,
                              IAnjutaIterable *position,
                              gboolean added,
                              gint length,
                              gint lines,
                              const gchar *text,
                              IndentCPlugin* plugin)
{
	/* If autoindent is enabled*/
	if (plugin->smart_indentation)
	{
		if (g_settings_get_boolean (plugin->settings, PREF_BRACE_AUTOCOMPLETION))
		{
			if (!added && length == 1 && (*text == '[' || *text == '('))
			{
				IAnjutaIterable *next;
				gchar *next_char;

				next = ianjuta_iterable_clone (position, NULL);
				ianjuta_iterable_next (next, NULL);
				next_char = ianjuta_editor_get_text (editor, position, next, NULL);
				if ((*text == '[' && *next_char == ']') || (*text == '(' && *next_char == ')'))
					erase_editor_blocked (editor, position, next, plugin);
			}
		}
	}
}

void
cpp_auto_indentation (IAnjutaEditor *editor,
                      IndentCPlugin *lang_plugin,
                      IAnjutaIterable *start,
                      IAnjutaIterable *end)
{
	gint line_start, line_end;
	gint insert_line;
	gint line_indent;
	gboolean has_selection;

	has_selection = ianjuta_editor_selection_has_selection
		(IANJUTA_EDITOR_SELECTION (editor), NULL);
	if (start == NULL || end == NULL)
	{
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
