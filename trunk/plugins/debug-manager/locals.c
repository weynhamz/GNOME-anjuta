/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    locals.c
    Copyright (C) 2000  Kh. Naba Kumar Singh

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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "locals.h"

#include "debug_tree.h"

#include "queue.h"

/*#define DEBUG*/
#include <libanjuta/anjuta-debug.h>


typedef struct _DmaThreadLocal
{
	GtkTreeModel *model;
	gint thread;
	guint frame;
} DmaThreadLocal;

typedef struct _DmaThreadAndFrame
{
	gint thread;
	guint frame;
} DmaThreadAndFrame;

struct _Locals
{
	AnjutaPlugin *plugin;
	DmaDebuggerQueue *debugger;
	
	GtkWidget *main_w;
	DebugTree *debug_tree;
	
	DmaThreadLocal* current;
	GList *list;
};

static void
locals_updated (const gpointer data, gpointer user_data, GError *error)
{
	const GList *list = (const GList *)data;
	Locals *self = (Locals*) user_data;
	
	g_return_if_fail (self != NULL);

	if (error != NULL)
		return;
	
	if (g_list_length ((GList*)list) < 1)
		return;

	debug_tree_update_all(self->debug_tree);
	debug_tree_replace_list (self->debug_tree, list);
	debug_tree_update_all(self->debug_tree);
}

/* Private functions
 *---------------------------------------------------------------------------*/

static void
create_locals_gui (Locals *self)
{
	g_return_if_fail (self->debug_tree == NULL);
	g_return_if_fail (self->main_w == NULL);
	
	self->debug_tree = debug_tree_new (self->plugin);
	debug_tree_connect (self->debug_tree, self->debugger);

	/* Create local window */
	GtkWidget *main_w;
		
	main_w = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (main_w);		
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (main_w),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (main_w),
										 GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (main_w), debug_tree_get_tree_widget (self->debug_tree));
	gtk_widget_show_all (main_w);
	self->main_w = main_w;

	anjuta_shell_add_widget (self->plugin->shell,
							 self->main_w,
							 /* This is the list of local variables. */
							 "AnjutaDebuggerLocals", _("Locals"),
							 "gdb-locals-icon", ANJUTA_SHELL_PLACEMENT_BOTTOM,
							 NULL);
}

static void
destroy_locals_gui (Locals *self)
{
	if (self->debug_tree != NULL)
	{
		debug_tree_free (self->debug_tree);
		self->debug_tree = NULL;
	}

	if (self->main_w != NULL)
	{
		gtk_widget_destroy (GTK_WIDGET (self->main_w));
		self->main_w = NULL;
	}
}

static void
on_clear_locals (gpointer data, gpointer user_data)
{
	DmaThreadLocal *frame = (DmaThreadLocal *) data;
	Locals *self = (Locals *)user_data;

	debug_tree_remove_model (self->debug_tree, frame->model);
	g_object_unref (G_OBJECT (frame->model));
	g_free (frame);
}

static void
dma_thread_clear_all_locals (Locals *self)
{
	/* Clear all GtkListStore */
	g_list_foreach (self->list, (GFunc)on_clear_locals, self);
	g_list_free (self->list);
	
	self->current = NULL;
	self->list = NULL;
}

static gboolean
on_find_local (gconstpointer a, gconstpointer b)
{
	const DmaThreadLocal *frame = (const DmaThreadLocal *)a;
	const DmaThreadAndFrame *comp = (const DmaThreadAndFrame *)b;
	
	return !((frame->thread == comp->thread) && (frame->frame == comp->frame));
}

static DmaThreadLocal *
dma_thread_find_local (Locals *self, gint thread, guint frame)
{
	GList *list;
	DmaThreadAndFrame comp = {thread, frame};
	
	list = g_list_find_custom (self->list, (gconstpointer) &comp, on_find_local);
	
	return list == NULL ? NULL : (DmaThreadLocal *)list->data;
}

static void
dma_thread_add_local (Locals *self, GtkTreeModel *model, gint thread, guint frame)
{
	DmaThreadLocal *local;
	
	local = g_new (DmaThreadLocal, 1);
	local->thread = thread;
	local->frame = frame;
	local->model = model;
	g_object_ref (G_OBJECT (model));
	
	self->list = g_list_append (self->list, local);
	self->current = local;
}

/* Private functions
 *---------------------------------------------------------------------------*/

static void locals_update (Locals *self, gint thread)
{
	dma_thread_clear_all_locals (self);

	dma_thread_add_local (self, debug_tree_get_model (self->debug_tree), thread, 0);
	
	dma_queue_list_local (self->debugger, locals_updated, self);
}

static void
locals_change_frame (Locals *self, guint frame, gint thread)
{
	DmaThreadLocal *local;
	
	/* Check if we really need a change */
	if ((self->current != NULL) && (self->current->thread == thread) && (self->current->frame == frame)) return;

	local = dma_thread_find_local (self, thread, frame);
	if (local != NULL)
	{
		/* Find frame already read */
		self->current = local;
		debug_tree_set_model (self->debug_tree, self->current->model);
		
		return;
	}
	
	debug_tree_new_model (self->debug_tree);
	dma_thread_add_local (self, debug_tree_get_model (self->debug_tree), thread, frame);
	dma_queue_list_local (self->debugger, locals_updated, self);
}

static void
on_program_moved (Locals *self, guint pid, gint thread)
{
	locals_update (self, thread);
}

static void
on_frame_changed (Locals *self, guint frame, gint thread)
{
	/* Change thread and frame*/
	locals_change_frame (self, frame, thread);
}

static void
on_program_exited (Locals *self)
{
	/* Disconnect signals */
	g_signal_handlers_disconnect_by_func (self->plugin, on_program_exited, self);
	g_signal_handlers_disconnect_by_func (self->plugin, on_program_moved, self);
	g_signal_handlers_disconnect_by_func (self->plugin, on_frame_changed, self);
	
	dma_thread_clear_all_locals (self);

	destroy_locals_gui (self);
}

static void
on_program_started (Locals *self)
{
	if (!dma_debugger_queue_is_supported (self->debugger, HAS_VARIABLE)) return;

	create_locals_gui (self);
	
	g_signal_connect_swapped (self->plugin, "program-exited", G_CALLBACK (on_program_exited), self);
	g_signal_connect_swapped (self->plugin, "program-moved", G_CALLBACK (on_program_moved), self);
	g_signal_connect_swapped (self->plugin, "frame-changed", G_CALLBACK (on_frame_changed), self);
}

/* Public function
 *---------------------------------------------------------------------------*/

gchar*
locals_find_variable_value (Locals *l, const gchar *name)
{
	return debug_tree_find_variable_value (l->debug_tree, name);
}

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

Locals *
locals_new (DebugManagerPlugin *plugin)
{
	Locals *self = g_new0 (Locals, 1);

	self->plugin = ANJUTA_PLUGIN (plugin);
	self->debugger = dma_debug_manager_get_queue (plugin);

	g_signal_connect_swapped (self->plugin, "program-started", G_CALLBACK (on_program_started), self);
	
	return self;
}

void
locals_free (Locals *self)
{
	g_return_if_fail (self != NULL);

	g_signal_handlers_disconnect_matched (self->plugin, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, self);	

	dma_thread_clear_all_locals (self);
	
	/* Destroy gui */
	destroy_locals_gui (self);

	g_free (self);
}
