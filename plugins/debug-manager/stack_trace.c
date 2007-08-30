/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    stack_trace.c
    Copyright (C) 2000  Kh. Naba Kumar Singh

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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "stack_trace.h"

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <gnome.h>

/*#define DEBUG*/
#include <libanjuta/resources.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>

#include "utilities.h"
#include "info.h"

#define ANJUTA_PIXMAP_POINTER PACKAGE_PIXMAPS_DIR"/pointer.png"

typedef struct _DmaThreadStackTrace
{
	GtkTreeModel *model;
	gint thread;
	guint last_update;
} DmaThreadStackTrace;

struct _StackTrace
{
	DebugManagerPlugin *plugin;
	IAnjutaDebugger *debugger;
	
	GtkActionGroup *action_group;
	
	DmaThreadStackTrace* current;
	GList *list;

	gint current_frame;
	guint current_update;
	
	GtkTreeView *treeview;
	GtkMenu *menu;
	GtkWidget *scrolledwindow;
};

enum {
	STACK_TRACE_ACTIVE_COLUMN,
	STACK_TRACE_FRAME_COLUMN,
	STACK_TRACE_FILE_COLUMN,
	STACK_TRACE_LINE_COLUMN,
	STACK_TRACE_FUNC_COLUMN,
	STACK_TRACE_ADDR_COLUMN,
	STACK_TRACE_ARGS_COLUMN,
	STACK_TRACE_URI_COLUMN,
	STACK_TRACE_COLOR_COLUMN,
	STACK_TRACE_N_COLUMNS
};

/* Helpers functions
 *---------------------------------------------------------------------------*/

static gboolean 
get_current_iter (GtkTreeView *view, GtkTreeIter* iter)
{
	GtkTreeSelection *selection;
	
	selection = gtk_tree_view_get_selection (view);
	return gtk_tree_selection_get_selected (selection, NULL, iter);
}

/*
 * returns the current stack frame or -1 on error
 */
static gint
get_current_index (StackTrace* st)
{
	GtkTreeIter iter;
	
	if (get_current_iter (st->treeview, &iter))
	{
		GtkTreeModel *model;
		gint frame_no;
	
		model = gtk_tree_view_get_model (st->treeview);		
		gtk_tree_model_get (model, &iter, STACK_TRACE_FRAME_COLUMN, &frame_no, -1);
		
		return frame_no;
	}
	else
	{
		return -1;
	}
}

static gboolean
my_gtk_tree_model_get_iter_last(GtkTreeModel *model, GtkTreeIter *last)
{
	gboolean exist;
 	GtkTreeIter iter;

	exist = gtk_tree_model_get_iter_first(model, &iter);
	if (!exist) return FALSE;

	do
	{
		*last = iter;
    	exist = gtk_tree_model_iter_next(model, &iter);
    }
	while (exist);
	
	return TRUE;
}

static gboolean
my_gtk_tree_model_iter_prev(GtkTreeModel *model, GtkTreeIter *iter)
{
	GtkTreePath* path;
	gboolean exist;
	
	path = gtk_tree_model_get_path (model, iter);
	exist = gtk_tree_path_prev (path);
	if (exist)
	{
		exist = gtk_tree_model_get_iter (model, iter, path);
	}
	gtk_tree_path_free (path);
	
	return exist;
}

static gint
my_gtk_tree_iter_compare(GtkTreeModel *model, GtkTreeIter *itera, GtkTreeIter *iterb)
{
	GtkTreePath* patha;
	GtkTreePath* pathb;
	gint comp;

	patha = gtk_tree_model_get_path (model, itera);
	pathb = gtk_tree_model_get_path (model, iterb);

	comp = gtk_tree_path_compare (patha, pathb);
	
	gtk_tree_path_free (patha);
	gtk_tree_path_free (pathb);
	
	return comp;
}

/* Private functions
 *---------------------------------------------------------------------------*/

static void
on_clear_stack_trace (gpointer data, gpointer user_data)
{
	DmaThreadStackTrace *trace = (DmaThreadStackTrace *) data;
	
	g_object_unref (G_OBJECT (trace->model));
	g_free (trace);
}

static void
dma_thread_clear_all_stack_trace (StackTrace *self)
{
	/* Clear all GtkListStore */
	g_list_foreach (self->list, (GFunc)on_clear_stack_trace, NULL);
	g_list_free (self->list);
	
	self->current = NULL;
	self->list = NULL;
}

static void
on_stack_trace_updated (const GList *stack, gpointer user_data)
{
	StackTrace *self = (StackTrace *)user_data;
	const GList *node;
	GtkListStore *model;		
	GtkTreeIter iter;
	gboolean exist;
	GdkPixbuf *pic;

	model = GTK_LIST_STORE (self->current->model);

	pic = gdk_pixbuf_new_from_file (ANJUTA_PIXMAP_POINTER, NULL);
	
	exist = my_gtk_tree_model_get_iter_last (GTK_TREE_MODEL (model), &iter);
	
	for (node = (const GList *)g_list_last((GList *)stack); node != NULL; node = node->prev)
	{
		IAnjutaDebuggerFrame *frame;
		gchar *adr;
		gchar *uri;
		gchar *file;

		frame = (IAnjutaDebuggerFrame *)node->data;

		if (exist)
		{
			/* Check if it's the same stack frame */
			gchar *adr;
			gchar *args;
			guint address;
			guint line;
			gboolean same;
			
			gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
									STACK_TRACE_ADDR_COLUMN, &adr,
									STACK_TRACE_LINE_COLUMN, &line,
									STACK_TRACE_ARGS_COLUMN, &args,
								-1);
			address = adr != NULL ? strtoul (adr, NULL, 0) : 0;
			same = (address == frame->address) && (line == frame->line);
			if ((args == NULL) || (frame->args == NULL))
			{
				same = same && (args == frame->args);
			}
			else
			{
				same = same && (strcmp (args, frame->args) == 0);
			}
			g_free (adr);
			g_free (args);
			
			if (same)
			{
				/* Same frame, just change the color */
				gtk_list_store_set (model, &iter, 
								   	STACK_TRACE_ACTIVE_COLUMN, frame->level == self->current_frame ? pic : NULL,
									STACK_TRACE_FRAME_COLUMN, frame->level,
								   	STACK_TRACE_COLOR_COLUMN, "black", -1);
				
				/* Check previous frame */
				exist = my_gtk_tree_model_iter_prev (GTK_TREE_MODEL (model), &iter);
				if (!exist || (node->prev != NULL))
				{
					/* Last frame or Upper frame still exist */
					continue;
				}
				/* Upper frame do not exist, remove all them */
			}
			
			/* New frame, remove all previous frame */
			GtkTreeIter first;
				
			gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &first);
			while (my_gtk_tree_iter_compare (GTK_TREE_MODEL (model), &first, &iter) < 0)
			{
				gtk_list_store_remove (model, &first);
			}
			gtk_list_store_remove (model, &first);

			if (same)
			{
				break;
			}
			else
			{
				exist = FALSE;
			}
		}
					
		gtk_list_store_prepend (model, &iter);

		adr = g_strdup_printf ("0x%x", frame->address);
		if (frame->file)
		{
			uri = g_strconcat ("file://", frame->file, NULL);
			file = strrchr(uri, '/') + 1;
		}
		else
		{
			uri = NULL;
			file = frame->library;
		}
		
		gtk_list_store_set(model, &iter, 
					   STACK_TRACE_ACTIVE_COLUMN, frame->level == self->current_frame ? pic : NULL,
					   STACK_TRACE_FRAME_COLUMN, frame->level, 
					   STACK_TRACE_FILE_COLUMN, file,
					   STACK_TRACE_LINE_COLUMN, frame->line,
					   STACK_TRACE_FUNC_COLUMN, frame->function,
					   STACK_TRACE_ADDR_COLUMN, adr,
					   STACK_TRACE_ARGS_COLUMN, frame->args,
					   STACK_TRACE_URI_COLUMN, uri,
					   STACK_TRACE_COLOR_COLUMN, "red",
					   -1);
		g_free (uri);
		g_free (adr);
	}
	gdk_pixbuf_unref (pic);
	
}

static DmaThreadStackTrace*
dma_thread_create_new_stack_trace (StackTrace *self, gint thread)
{
	DmaThreadStackTrace *trace;
	GtkListStore *store;
	
	/* Create new list store */
	store = gtk_list_store_new (STACK_TRACE_N_COLUMNS,
								  GDK_TYPE_PIXBUF,
								   G_TYPE_UINT,
								   G_TYPE_STRING,
								   G_TYPE_UINT,
								   G_TYPE_STRING,
								   G_TYPE_STRING,
								   G_TYPE_STRING,
								   G_TYPE_STRING,
								   G_TYPE_STRING);
		
	trace = g_new (DmaThreadStackTrace, 1);
	trace->thread = thread;
	trace->model = GTK_TREE_MODEL (store);
	trace->last_update = self->current_update;

	self->current = trace;
		
	/* Ask debugger to get all frame data */
	ianjuta_debugger_list_frame (
			self->debugger,
			(IAnjutaDebuggerCallback)on_stack_trace_updated,
			self,
			NULL);
	
	self->list = g_list_append (self->list, trace);
	
	return trace;
}

static DmaThreadStackTrace*
dma_thread_update_stack_trace (StackTrace *self, DmaThreadStackTrace *trace)
{
	GtkListStore *store;
	
	trace->last_update = self->current_update;
	
	/* Ask debugger to get all frame data */
	ianjuta_debugger_list_frame (
			self->debugger,
			(IAnjutaDebuggerCallback)on_stack_trace_updated,
			self,
			NULL);
	
	return trace;
}

static gboolean
on_find_stack_trace (gconstpointer a, gconstpointer b)
{
	const DmaThreadStackTrace *trace = (const DmaThreadStackTrace *)a;
	guint thread = (gint)b;
	
	return trace->thread != thread;
}

static void
dma_thread_set_stack_trace (StackTrace *self, gint thread)
{
	GList *list;
	DmaThreadStackTrace *trace;

	if ((self->current == NULL) || (self->current->thread != thread) || (self->current->last_update != self->current_update))
	{
		self->current_frame = 0;
		
		list = g_list_find_custom (self->list, (gconstpointer) thread, on_find_stack_trace);
	
		if (list == NULL)
		{
			/* Create new stack trace */
			trace = dma_thread_create_new_stack_trace(self, thread);
		}
		else
		{
			trace = (DmaThreadStackTrace *)list->data;
			self->current = trace;
			
			if (trace->last_update != self->current_update)
			{
				/* Update stack trace */
				dma_thread_update_stack_trace (self, trace);
			}
		}
		gtk_tree_view_set_model (self->treeview, trace->model);
	}
}

/* Callback functions
 *---------------------------------------------------------------------------*/

static void
on_frame_changed (StackTrace *self, guint frame, gint thread)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	/* Change thread */
	dma_thread_set_stack_trace (self, thread);
	
	/* Change frame */
	self->current_frame = frame;
	
	model = self->current->model;
	
	/* Clear old pointer */
	if(gtk_tree_model_get_iter_first (model, &iter))
	{
    	do
		{
        	/* clear pixmap on the previous active frame */
            gtk_list_store_set (GTK_LIST_STORE (model), &iter,
            					STACK_TRACE_ACTIVE_COLUMN, NULL, -1);
        } while (gtk_tree_model_iter_next (model, &iter));
	}
	
	/* Set pointer to current frame */
	if (gtk_tree_model_iter_nth_child (model, &iter, NULL, self->current_frame))
	{
		GdkPixbuf *pointer_pix = gdk_pixbuf_new_from_file (ANJUTA_PIXMAP_POINTER, NULL);
		
		/* set pointer on this frame */
		gtk_list_store_set (GTK_LIST_STORE(model), &iter,
							STACK_TRACE_ACTIVE_COLUMN, pointer_pix,
							-1);
		gdk_pixbuf_unref (pointer_pix);
	}
}

static void
on_stack_frame_set_activate (GtkAction *action, gpointer user_data)
{
	StackTrace *st;
	guint selected_frame;
	
	st = (StackTrace*) user_data;
	
	selected_frame = get_current_index (st);
	
	/* No frame selected */
	if (selected_frame == -1)
		return;
	
	/* current frame is already active */
	if (selected_frame == st->current_frame)
		return;
	
	/* issue a command to switch active frame to new location */
	ianjuta_debugger_set_frame (st->debugger, selected_frame, NULL);
}

static void
on_stack_view_source_activate (GtkAction *action, gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeView *view;
	gchar *uri;
	guint line;
	gchar *adr;
	guint address;
	
	StackTrace* st = (StackTrace*) user_data;		

	view = st->treeview;
	selection = gtk_tree_view_get_selection (view);
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;

	/* get the frame info */
	gtk_tree_model_get (model, &iter,
						STACK_TRACE_URI_COLUMN, &uri,
						STACK_TRACE_LINE_COLUMN, &line,
						STACK_TRACE_ADDR_COLUMN, &adr,
						-1);
	
	address = adr != NULL ? strtoul (adr, NULL, 0) : 0;
	dma_debug_manager_goto_code (st->plugin, uri, line, address);
	g_free (uri);
	g_free (adr);
}

static void
on_stack_trace_row_activated           (GtkTreeView     *treeview,
                                        GtkTreePath     *arg1,
                                        GtkTreeViewColumn *arg2,
                                        StackTrace      *st)
{
	on_stack_frame_set_activate (NULL, st);
}

static gboolean
on_stack_trace_button_press (GtkWidget *widget, GdkEventButton *bevent, gpointer user_data)
{
	StackTrace *st = (StackTrace*) user_data;

	if ((bevent->type == GDK_BUTTON_PRESS) && (bevent->button == 3))
	{
		/* Right mouse click */
		g_return_val_if_fail (st->menu != NULL, FALSE);
		gtk_menu_popup (GTK_MENU (st->menu), NULL, NULL, NULL, NULL,
						bevent->button, bevent->time);
	}
	else if ((bevent->type == GDK_2BUTTON_PRESS) && (bevent->button == 1))
	{
		/* Double left mouse click */
		on_stack_view_source_activate (NULL, user_data);
	}
	
	return FALSE;
}

/* Actions table
 *---------------------------------------------------------------------------*/

static GtkActionEntry actions_stack_trace[] = {
	{
		"ActionDmaSetCurrentFrame",               /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("Set current frame"),                  /* Display label */
		NULL,                                     /* short-cut */
		NULL,                                     /* Tooltip */
		G_CALLBACK (on_stack_frame_set_activate)  /* action callback */
	},
	{
		"ActionDmaJumpToFrame",
		NULL,
		N_("View Source"),
		NULL,
		NULL,
		G_CALLBACK (on_stack_view_source_activate)
	}
};		

static void
create_stack_trace_gui(StackTrace *st)
{
	GtkTreeModel* model;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	AnjutaUI *ui;

	g_return_if_fail (st->scrolledwindow == NULL);
	
	/* Create tree view */
	model = GTK_TREE_MODEL(gtk_list_store_new (STACK_TRACE_N_COLUMNS,
											   GDK_TYPE_PIXBUF,
											   G_TYPE_UINT,
											   G_TYPE_STRING,
											   G_TYPE_UINT,
											   G_TYPE_STRING,
											   G_TYPE_STRING,
											   G_TYPE_STRING,
											   G_TYPE_STRING,
											   G_TYPE_STRING));
	st->treeview = GTK_TREE_VIEW (gtk_tree_view_new_with_model (model));
	g_object_unref (G_OBJECT (model));
	
	selection = gtk_tree_view_get_selection (st->treeview);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	/* Columns */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Active"));
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "pixbuf",
										STACK_TRACE_ACTIVE_COLUMN);
	gtk_tree_view_append_column (st->treeview, column);
	gtk_tree_view_set_expander_column (st->treeview,
									   column);
	
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Frame"));
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
										STACK_TRACE_FRAME_COLUMN);
	gtk_tree_view_column_add_attribute (column, renderer, "foreground",
										STACK_TRACE_COLOR_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (st->treeview, column);
	gtk_tree_view_set_expander_column (st->treeview,
									   column);
	
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
										STACK_TRACE_FILE_COLUMN);
	gtk_tree_view_column_add_attribute (column, renderer, "foreground",
										STACK_TRACE_COLOR_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("File"));
	gtk_tree_view_append_column (st->treeview, column);

	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
										STACK_TRACE_LINE_COLUMN);
	gtk_tree_view_column_add_attribute (column, renderer, "foreground",
										STACK_TRACE_COLOR_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Line"));
	gtk_tree_view_append_column (st->treeview, column);
	
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
										STACK_TRACE_FUNC_COLUMN);
	gtk_tree_view_column_add_attribute (column, renderer, "foreground",
											STACK_TRACE_COLOR_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Function"));
	gtk_tree_view_append_column (st->treeview, column);
	
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
										STACK_TRACE_ADDR_COLUMN);
	gtk_tree_view_column_add_attribute (column, renderer, "foreground",
										STACK_TRACE_COLOR_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Address"));
	gtk_tree_view_append_column (st->treeview, column);
	
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
										STACK_TRACE_ARGS_COLUMN);
	gtk_tree_view_column_add_attribute (column, renderer, "foreground",
										STACK_TRACE_COLOR_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Arguments"));
	gtk_tree_view_append_column (st->treeview, column);
	
	/* Create popup menu */
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN(st->plugin)->shell, NULL);
	st->menu = GTK_MENU (gtk_ui_manager_get_widget (GTK_UI_MANAGER (ui), "/PopupStack"));
	
	/* Connect signal */
	g_signal_connect (st->treeview, "button-press-event", G_CALLBACK (on_stack_trace_button_press), st);  
	g_signal_connect (st->treeview, "row-activated", G_CALLBACK (on_stack_trace_row_activated), st);  
	
	/* Add stack window */
	st->scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (st->scrolledwindow),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (st->scrolledwindow),
										 GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (st->scrolledwindow),
					   GTK_WIDGET (st->treeview));
	gtk_widget_show_all (st->scrolledwindow);
	
	anjuta_shell_add_widget (ANJUTA_PLUGIN(st->plugin)->shell,
							 st->scrolledwindow,
							 "AnjutaDebuggerStack", _("Stack"),
							 "gdb-stack-icon", ANJUTA_SHELL_PLACEMENT_BOTTOM,
							 NULL);

}

static void
destroy_stack_trace_gui (StackTrace *st)
{
	if (st->scrolledwindow != NULL)
	{
		gtk_widget_destroy (st->scrolledwindow);
		st->scrolledwindow = NULL;
	}
}

static void
on_program_moved (StackTrace *self, guint pid, gint thread)
{
	self->current_update++;
	if (ianjuta_debugger_get_status (self->debugger, NULL) == IANJUTA_DEBUGGER_PROGRAM_STOPPED)
		dma_thread_set_stack_trace (self, thread);
}

static void
on_debugger_started (StackTrace *self)
{
	self->current_update = 0;
	create_stack_trace_gui (self);
}

static void
on_debugger_stopped (StackTrace *self)
{
	dma_thread_clear_all_stack_trace (self);
	destroy_stack_trace_gui (self);
}

/* Public functions
 *---------------------------------------------------------------------------*/

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

StackTrace *
stack_trace_new (IAnjutaDebugger *debugger, DebugManagerPlugin *plugin)
{
	StackTrace *st;
	AnjutaUI *ui;

	st = g_new0 (StackTrace, 1);
	if (st == NULL) return NULL;

	st->plugin = plugin;
	st->debugger = debugger;
	if (debugger != NULL) g_object_ref (debugger);

	/* Register actions */
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN(st->plugin)->shell, NULL);
	st->action_group =
		anjuta_ui_add_action_group_entries (ui, "ActionGroupStack",
											_("Stack frame operations"),
											actions_stack_trace,
											G_N_ELEMENTS (actions_stack_trace),
											GETTEXT_PACKAGE, TRUE, st);

	g_signal_connect_swapped (st->debugger, "debugger-started", G_CALLBACK (on_debugger_started), st);
	g_signal_connect_swapped (st->debugger, "debugger-stopped", G_CALLBACK (on_debugger_stopped), st);
	g_signal_connect_swapped (st->debugger, "program-moved", G_CALLBACK (on_program_moved), st);
	g_signal_connect_swapped (st->debugger, "frame-changed", G_CALLBACK (on_frame_changed), st);
	
	return st;
}

void
stack_trace_free (StackTrace * st)
{
	AnjutaUI *ui;
	
	g_return_if_fail (st != NULL);

	/* Disconnect from debugger */
	if (st->debugger != NULL)
	{	
		g_signal_handlers_disconnect_by_func (st->debugger, G_CALLBACK (on_debugger_started), st);
		g_signal_handlers_disconnect_by_func (st->debugger, G_CALLBACK (on_debugger_stopped), st);
		g_signal_handlers_disconnect_by_func (st->debugger, G_CALLBACK (on_program_moved), st);
		g_signal_handlers_disconnect_by_func (st->debugger, G_CALLBACK (on_frame_changed), st);
		g_object_unref (st->debugger);	
	}

	/* Remove menu actions */
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (st->plugin)->shell, NULL);
	anjuta_ui_remove_action_group (ui, st->action_group);
	
	/* Destroy menu */
	destroy_stack_trace_gui	(st);
	
	g_free (st);
}
