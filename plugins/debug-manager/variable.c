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
get_hovered_word (IAnjutaEditor* editor, IAnjutaIterable* iter)
{
	gchar *buf;
	IAnjutaIterable *start;
	IAnjutaIterable	*end;
	
	if (iter == NULL) return NULL;
	
	/* Get selected characters if possible */
	if (IANJUTA_IS_EDITOR_SELECTION (editor))
	{
		gint pos = ianjuta_iterable_get_position (iter, NULL);

		/* Check if hover on selection */
		start = ianjuta_editor_selection_get_start (IANJUTA_EDITOR_SELECTION (editor), NULL);
		if (start && (ianjuta_iterable_get_position (start, NULL) 
						   < pos))
		{
			end = ianjuta_editor_selection_get_end (IANJUTA_EDITOR_SELECTION (editor), NULL);
			if (end && (ianjuta_iterable_get_position (end, NULL) 
							 >= pos))
			{
				/* Hover on selection, get selected characters */
			    g_object_unref (end);
				return ianjuta_editor_selection_get (IANJUTA_EDITOR_SELECTION (editor),
													 NULL);
			}
			if (end) g_object_unref (end);
		}
		if (start) g_object_unref (start);
	}
	
	/* Find word below cursor */
	DEBUG_PRINT("current char %c", ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0, NULL));
	if (!is_name (ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0, NULL)))
	{
		/* Not hover on a name */
		return NULL;
	}
	
	/* Look for the beginning of the name */
	for (start = ianjuta_iterable_clone (iter, NULL); ianjuta_iterable_previous (start, NULL);)
	{
		if (!is_name (ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (start), 0, NULL)))
		{
			ianjuta_iterable_next (start, NULL);
			break;
		}
	}

	/* Look for the end of the name */
	for (end = ianjuta_iterable_clone (iter, NULL); ianjuta_iterable_next (end, NULL);)
	{
		if (!is_name (ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (end), 0, NULL)))
		{
			ianjuta_iterable_previous (end, NULL);
			break;
		}
	}

	buf = ianjuta_editor_get_text_iter (editor, start, end, NULL);
	DEBUG_PRINT("get name %s", buf == NULL ? "(null)" : buf);
	g_object_unref (start);
	g_object_unref (end);
	
	return buf;
}
	
/* Private functions
 *---------------------------------------------------------------------------*/

/* Callback functions
 *---------------------------------------------------------------------------*/

static void
on_hover_over (DmaVariableDBase *self, IAnjutaIterable* pos, IAnjutaEditorHover* editor)
{
	gchar *name;
	
	DEBUG_PRINT("Hover on editor %p at %d", editor, 
				pos == NULL ? -1 : ianjuta_iterable_get_position (pos, NULL));
	
	name = get_hovered_word (IANJUTA_EDITOR (editor), pos);
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
		ianjuta_editor_hover_display (editor, pos, display, NULL);
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
