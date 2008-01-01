/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    variable.c
    Copyright (C) 2008  Sébastien Granjoux

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any laterdversion.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
 * Group all objects handling variables (watch, local) and take care of general
 * feature like variable tips.
 *---------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "variable.h"
#include "utilities.h"
#include "locals.h"
#include "watch.h"

/*#define DEBUG*/
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-editor-hover.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>

/* Constants
 *---------------------------------------------------------------------------*/

#define DEFAULT_VARIABLE_NAME	64

/* Types
 *---------------------------------------------------------------------------*/

struct _DmaVariableDBase
{
	AnjutaPlugin *plugin;

	Locals *locals;
	ExprWatch *watch;
	
	guint editor_watch;			/* Editor watch */
	IAnjutaEditor* editor;		/* Current editor */
};

/* Helper functions
 *---------------------------------------------------------------------------*/

static gboolean
is_name (gchar c)
{
	return g_ascii_isalnum (c) || (c == '_');
}

static gchar*
get_hovered_word (IAnjutaEditor* editor, gint position)
{
	gchar *buf;
	gchar *ptr;
	gint start;
	gint end;
	
	if (position == -1) return NULL;
	
	/* Get selected characters if possible */
	if (IANJUTA_IS_EDITOR_SELECTION (editor))
	{
		start = ianjuta_editor_selection_get_start (IANJUTA_EDITOR_SELECTION (editor), NULL);
		if ((start != -1) && (start <= position))
		{
			end = ianjuta_editor_selection_get_end (IANJUTA_EDITOR_SELECTION (editor), NULL);
			if ((end != -1) && (end >= position))
			{
				/* Hover on selection, get selected characters */
				return ianjuta_editor_get_text(editor, start, end - start, NULL);
			}
		}
	}
	
	/* Find word below cursor */
	if (position < DEFAULT_VARIABLE_NAME / 2)
	{
		start = 0;
		end = DEFAULT_VARIABLE_NAME;
	}
	else
	{
		start = position - DEFAULT_VARIABLE_NAME / 2;
		end = position + DEFAULT_VARIABLE_NAME / 2;
		position = DEFAULT_VARIABLE_NAME / 2;
	}
	/* FIXME: Temporary hack to avoid a bug in ianjuta_editor_get_text
	 * (bug #506740) */
	if (ianjuta_editor_get_length (editor, NULL) < end)
	{
		end = ianjuta_editor_get_length (editor, NULL) - start;
	}
	buf = ianjuta_editor_get_text (editor, start, end - start, NULL);

	/* Check if name is valid */
	ptr = buf + position;
	if (!is_name (*ptr))
	{
		g_free (buf);
		return NULL;
	}
				 
	/* Look for the beginning of the name */
	do
	{
		if (ptr == buf)
		{
			/* Need more data at beginning */
			gchar *head;
			gchar *tail = buf;
			gint len;
			
			if (start == 0)
			{
				/* No more data in buffer, find beginning */
				ptr--;
				break;
			}
			
			len = start < DEFAULT_VARIABLE_NAME ? start : DEFAULT_VARIABLE_NAME;
			start -= len;
			head = ianjuta_editor_get_text (editor, start, len, NULL);
			position += len;
			buf = g_strconcat (head, tail, NULL);
			g_free (head);
			g_free (tail);
			ptr = buf + len;
		}
		ptr--;
	} while (is_name (*ptr));
	start = ptr + 1 - buf;

	/* Look for the end of the name */
	ptr = buf + position;
	do
	{
		ptr++;
		if (*ptr == '\0')
		{
			/* Need more data at end */
			gchar *head = buf;
			gchar *tail;
			
			if (ianjuta_editor_get_length (editor, NULL) < (end + DEFAULT_VARIABLE_NAME))
			{
				/* FIXME: Temporary hack to avoid a bug in ianjuta_editor_get_text
		  		 * (bug #506740) */
				tail = ianjuta_editor_get_text (editor, end, -1, NULL);
			}
			else
			{
				tail = ianjuta_editor_get_text (editor, end, DEFAULT_VARIABLE_NAME, NULL);
			}
			if (tail == NULL)
			{
				/* No more data, find end */
				break;
			}
			end += DEFAULT_VARIABLE_NAME;
			
			buf = g_strconcat (head, tail, NULL);
			g_free (head);
			g_free (tail);
			ptr = (ptr - head) + buf;
		}
	} while (is_name (*ptr));
	*ptr = '\0';
	
	memmove (buf, buf + start, ptr - buf + 1 - start);
	
	return buf;
}
	
/* Private functions
 *---------------------------------------------------------------------------*/

/* Callback functions
 *---------------------------------------------------------------------------*/

static void
on_hover_over (DmaVariableDBase *self, gint position, IAnjutaEditorHover* editor)
{
	gchar *name;
	
	DEBUG_PRINT("Hover on editor %p at %d", editor, position);
	
	name = get_hovered_word (IANJUTA_EDITOR (editor), position);
	if (name != NULL)
	{
		gchar *value;
		
		/* Search for variable in local */
		value = locals_find_variable_value (self->locals, name);
		if (value == NULL)
		{
			/* Not found search in watch */
			value = expr_watch_find_variable_value (self->watch, name);
			if (value == NULL)
			{
				/* Does not exist */
				g_free (name);
				return;
			}
		}
		
		gchar *display;
		
		display = g_strconcat(name, " = ", value, NULL);
		ianjuta_editor_hover_display (editor, position, display, NULL);
		g_free (display);
		g_free (value);
		g_free (name);
	}
}

static void
on_added_current_editor (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer user_data)
{
	DmaVariableDBase *self = (DmaVariableDBase *)user_data;
	GObject *editor;
		
	editor = g_value_get_object (value);
	
    /* Connect to hover interface */
	if (IANJUTA_IS_EDITOR_HOVER (editor))
	{
		g_signal_connect_swapped (editor, "hover-over", G_CALLBACK (on_hover_over), self);
		self->editor = IANJUTA_EDITOR (editor);
	}
}

static void
on_removed_current_editor (AnjutaPlugin *plugin,
							  const char *name, gpointer user_data)
{
	DmaVariableDBase *self = (DmaVariableDBase *)user_data;

	/* Disconnect to previous hover interface */
	if (self->editor != NULL)
	{
		g_signal_handlers_disconnect_matched (self->editor, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, self);
		self->editor = NULL;
	}
}

static void
on_program_running (DmaVariableDBase *self)
{
	if (self->editor_watch != -1)
	{
		anjuta_plugin_remove_watch (ANJUTA_PLUGIN(self->plugin), self->editor_watch, TRUE);
		self->editor_watch = -1;
	}
}

static void
on_program_stopped (DmaVariableDBase *self)
{
	if (self->editor_watch == -1)
	{
		/* set editor watch */
		self->editor_watch = anjuta_plugin_add_watch (ANJUTA_PLUGIN(self->plugin), "document_manager_current_editor",
								 on_added_current_editor, on_removed_current_editor, self);
	}
}

static void
on_program_exited (DmaVariableDBase *self)
{
	if (self->editor_watch != -1)
	{
		anjuta_plugin_remove_watch (ANJUTA_PLUGIN(self->plugin), self->editor_watch, TRUE);
		self->editor_watch = -1;
	}

	/* Disconnect from other debugger signal */
	g_signal_handlers_disconnect_by_func (self->plugin, G_CALLBACK (on_program_exited), self);
	g_signal_handlers_disconnect_by_func (self->plugin, G_CALLBACK (on_program_stopped), self);
	g_signal_handlers_disconnect_by_func (self->plugin, G_CALLBACK (on_program_running), self);
}	

static void
on_program_started (DmaVariableDBase *self)
{
	/* Connect to other debugger signal */
	g_signal_connect_swapped (self->plugin, "program-stopped", G_CALLBACK (on_program_stopped), self);
	g_signal_connect_swapped (self->plugin, "program-exited", G_CALLBACK (on_program_exited), self);
	g_signal_connect_swapped (self->plugin, "program-running", G_CALLBACK (on_program_running), self);
}

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

DmaVariableDBase *
dma_variable_dbase_new (DebugManagerPlugin *plugin)
{
	DmaVariableDBase *self = g_new0 (DmaVariableDBase, 1);

	self->plugin = ANJUTA_PLUGIN (plugin);
	self->editor_watch = -1;
	self->editor = NULL;
	self->watch = expr_watch_new (ANJUTA_PLUGIN (plugin));
	self->locals = locals_new (plugin);
	
	g_signal_connect_swapped (self->plugin, "program-started", G_CALLBACK (on_program_started), self);
	
	return self;
}

void
dma_variable_dbase_free (DmaVariableDBase *self)
{
	g_return_if_fail (self != NULL);
	
	g_signal_handlers_disconnect_matched (self->plugin, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, self);

	locals_free (self->locals);
    expr_watch_destroy (self->watch);
	
	g_free (self);
}
