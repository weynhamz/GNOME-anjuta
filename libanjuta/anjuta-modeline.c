/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-modeline.c
 * Copyright (C) Sébastien Granjoux 2013 <seb.sfo@free.fr>
 * 
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * SECTION:anjuta-modeline
 * @short_description: Parse editor mode line
 * @see_also:
 * @stability: Unstable
 * @include: libanjuta/anjuta-modeline.h
 *
 */

#include "anjuta-modeline.h"

#include "anjuta-debug.h"

#include <glib-object.h>

#include <stdlib.h>
#include <string.h>

/* Types declarations
 *---------------------------------------------------------------------------*/

enum {
	SET_USE_SPACES = 1 << 0,
	SET_STATEMENT_INDENTATION = 1 << 1,
	SET_TAB_SIZE = 1 << 2,
	CHECK_NEXT = 1 << 4
};

typedef struct {
	gint settings;

	gint use_spaces;
	gint statement_indentation;
	gint tab_size;
} IndentationParams;


/* Helpers functions
 *---------------------------------------------------------------------------*/

/* Private functions
 *---------------------------------------------------------------------------*/

static gchar *
get_editor_line (IAnjutaEditor *editor, gint line)
{
	IAnjutaIterable *start;
	IAnjutaIterable *end;
	gchar *content = NULL;

	if (line < 0)
	{
		gint last;
		
		end = ianjuta_editor_get_end_position(editor, NULL);
		last = ianjuta_editor_get_line_from_position (editor, end, NULL);
		line = last + line;
		g_object_unref (end);
	}
	if (line > 0)
	{
		start = ianjuta_editor_get_line_begin_position (editor, line, NULL);
		end = ianjuta_editor_get_line_end_position (editor, line, NULL);
		content = ianjuta_editor_get_text (editor, start, end, NULL);
		g_object_unref (start);
		g_object_unref (end);
	}

	return content;
}

static void
set_vim_params (IndentationParams *params, const gchar *key, const gchar *value)
{
	//DEBUG_PRINT ("Setting indent param: %s = %s", key, value);
	if ((strcmp (key, "expandtab") == 0) ||
	    (strcmp (key, "et") == 0))
	{
		params->use_spaces = 1;
		params->settings |= SET_USE_SPACES;
	}
	else if ((strcmp (key, "noexpandtab") == 0) ||
	         (strcmp (key, "noet") == 0))
	{
		params->use_spaces = 0;
		params->settings |= SET_USE_SPACES;
	}
	else if ((strcmp (key, "shiftwidth") == 0) ||
	         (strcmp (key, "sw") == 0))
	{
		params->statement_indentation = atoi (value);
		params->settings |= SET_STATEMENT_INDENTATION;
	}
	else if ((strcmp (key, "softtabstop") == 0) ||
	         (strcmp (key, "sts") == 0) ||
	         (strcmp (key, "tabstop") == 0) ||
	         (strcmp (key, "ts") == 0))
	{
		params->tab_size = atoi (value);
		params->settings |= SET_TAB_SIZE;
	}
}

static gboolean
parse_vim_modeline (IndentationParams *params, const gchar *line, gint linenum)
{
	gchar *ptr;
	gchar *end;
	gchar *key;
	gchar *value;

	/* Check the first 5 and last 5 lines */
	if ((linenum < -5) || (linenum == 0) || (linenum > 5))
	{
		return FALSE;
	}

	ptr = strstr (line, "vim:");
	if (ptr == NULL)
	{
		if ((linenum != -5) && (linenum != 5)) params->settings = CHECK_NEXT;
		return FALSE;
	}
	ptr += 4;
	while (g_ascii_isspace (*ptr)) ptr++;
	if (strncmp (ptr, "set", 3) != 0)
	{
		if ((linenum != -5) && (linenum != 5)) params->settings = CHECK_NEXT;
		return FALSE;
	}
	ptr += 3;

	for (end = ptr;; end++)
	{
		if ((*end == ':') && (*(end-1) != '\\')) break;
	}
	*end = '\0';

	while (*ptr != '\0')
	{
		gchar sep;
		
		while (g_ascii_isspace (*ptr)) ptr++;
		if (*ptr == '\0') break;

		/* Get key */
		key = ptr++;
		value = NULL;
		while ((*ptr != '\0') && (*ptr != '=') && !g_ascii_isspace(*ptr)) ptr++;
		sep = *ptr;
		*ptr = '\0';

		if (sep == '=')
		{
			/* Get value */
			value = ++ptr;
			while ((*ptr != '\0') && !g_ascii_isspace(*ptr)) ptr++;
			sep = *ptr;
			*ptr = '\0';
			
			if (sep != '\0') ptr++;
		}

		set_vim_params (params, key, value);
	}

	return TRUE;
}

static void
set_emacs_params (IndentationParams *params, const gchar *key, const gchar *value)
{
	//DEBUG_PRINT ("Setting indent param: %s = %s", key, value);
	if (strcmp (key, "indent-tabs-mode") == 0)
	{
		if (strcmp (value, "t") == 0)
		{
			params->use_spaces = 0;
			params->settings |= SET_USE_SPACES;
		}
		else if (strcmp (value, "nil") == 0)
		{
			params->use_spaces = 1;
			params->settings |= SET_USE_SPACES;
		}
	}
	else if ((strcmp (key, "c-basic-offset") == 0) ||
		(strcmp (key, "indent-offset") == 0))
	{
		params->statement_indentation = atoi (value);
		params->settings |= SET_STATEMENT_INDENTATION;
	}
	else if (strcasecmp (key, "tab-width") == 0)
	{
		params->tab_size = atoi (value);
		params->settings |= SET_TAB_SIZE;
	}
}

static gboolean
parse_emacs_modeline (IndentationParams *params, gchar *line, gint linenum)
{
	gchar *ptr;
	gchar *end;
	gchar *key;
	gchar *value;
	
	if (linenum == 1)
	{
		/* If first line is a shebang, check second line */
		if ((line[0] == '#') && (line[1] =='!'))
		{
			params->settings |= CHECK_NEXT;
			return FALSE;
		}
	}
	else if (linenum != 2)
	{
		/* Check only the 2 first lines */
		return FALSE;
	}

	ptr = strstr (line, "-*-");
	if (ptr == NULL) return FALSE;
	ptr += 3;
	end = strstr (ptr, "-*-");
	if (end == NULL) return FALSE;
	*end = '\0';

	while (*ptr != '\0')
	{
		gchar sep;
		
		while (g_ascii_isspace (*ptr)) ptr++;
		if (*ptr == '\0') break;

		/* Get key */
		key = ptr++;
		value = NULL;
		while ((*ptr != '\0') && (*ptr != ':') && (*ptr != ';')) ptr++;
		sep = *ptr;
		
		end = ptr - 1;
		while (g_ascii_isspace (*end)) end--;
		*(end + 1) = '\0';
			
		if (sep == ':')
		{
			/* Get value */
			ptr++;
			while (g_ascii_isspace (*ptr)) ptr++;
			if (*ptr != '\0')
			{
				value = ptr;
				while ((*ptr != '\0') && (*ptr != ';')) ptr++;
				sep = *ptr;
			
				end = ptr - 1;
				while (g_ascii_isspace (*end)) end--;
				*(end + 1) = '\0';
			
				if (sep == ';') ptr++;
			}
		}

		set_emacs_params (params, key, value);
	}

	return TRUE;
}


static gboolean 
set_indentation (IAnjutaEditor *editor, IndentationParams *params)
{
	if (params->settings == 0) return FALSE;

	if (params->settings & SET_USE_SPACES)
		ianjuta_editor_set_use_spaces (editor, params->use_spaces, NULL);

	if (params->settings & SET_STATEMENT_INDENTATION)
		ianjuta_editor_set_indentsize (editor, params->statement_indentation, NULL);

	if (params->settings & SET_TAB_SIZE)
		ianjuta_editor_set_tabsize (editor, params->tab_size, NULL);

	return TRUE;
}


/* Public functions
 *---------------------------------------------------------------------------*/


/**
 * anjuta_apply_modeline:
 * @editor: #IAnjutaEditor object
 *
 * Check the editor buffer to find a mode line and update the indentation
 * settings if found.
 * 
 * The mode line is special line used by the text editor to define settings for
 * the current file, typically indentation. Anjuta currently recognize two kinds
 * of mode line:
 *
 * Emacs mode line, on the first or the second line if the first one is a
 * shebang (#!) with the following format:
 * -*- key1: value1; key2: value2 -*-
 *
 * Vim mode line, one the first 5 or the last 5 lines with the following format:
 * vim:set key1=value1 key2=value2
 * 
 * Returns: %TRUE if a mode line has been found and applied.
 */
gboolean
anjuta_apply_modeline (IAnjutaEditor *editor)
{
	IndentationParams params = {CHECK_NEXT,0,0,0};
	gint line;
	gchar *content = NULL;
	
	g_return_val_if_fail (IANJUTA_IS_EDITOR (editor), FALSE);

	/* Check the first lines */
	for (line = 1; params.settings == CHECK_NEXT; line++)
	{
		g_free (content);
		content = get_editor_line (editor, line);
		if (content == NULL) return FALSE;

		params.settings = 0;
		if (parse_vim_modeline (&params, content, line)) break;
		if (parse_emacs_modeline (&params, content, line)) break;
	}

	/* Check the last lines */
	if (params.settings == 0) params.settings = CHECK_NEXT;
	for (line = -1;params.settings == CHECK_NEXT; line--)
	{
		g_free (content);
		content = get_editor_line (editor, line);
		if (content == NULL) return FALSE;

		params.settings = 0;
		if (parse_vim_modeline (&params, content, line)) break;
		if (parse_emacs_modeline (&params, content, line)) break;
	}
	g_free (content);

	/* Set indentation settings */
	return set_indentation (editor, &params);
}
