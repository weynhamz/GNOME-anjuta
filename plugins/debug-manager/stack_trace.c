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
#include <stdlib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

/*#define DEBUG*/
#include <libanjuta/resources.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>

#include "utilities.h"
#include "info.h"
#include "queue.h"

#define ANJUTA_PIXMAP_POINTER PACKAGE_PIXMAPS_DIR"/pointer.png"

struct _StackTrace
{
	DebugManagerPlugin *plugin;
	DmaDebuggerQueue *debugger;

	GtkActionGroup *action_group;

	gint current_thread;
	guint current_frame;

	gulong changed_handler;

	GtkTreeView *treeview;
	GtkMenu *menu;
	GtkWidget *scrolledwindow;
};

typedef struct _StackPacket StackPacket;

struct _StackPacket {
	StackTrace* self;
	guint thread;
	gboolean scroll;
	gboolean unblock;
};

enum {
	STACK_TRACE_ACTIVE_COLUMN,
	STACK_TRACE_THREAD_COLUMN,
	STACK_TRACE_FRAME_COLUMN,
	STACK_TRACE_FILE_COLUMN,
	STACK_TRACE_LINE_COLUMN,
	STACK_TRACE_FUNC_COLUMN,
	STACK_TRACE_ADDR_COLUMN,
	STACK_TRACE_ARGS_COLUMN,
	STACK_TRACE_DIRTY_COLUMN,
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
 * returns TRUE if a frame is selected
 */
static gboolean
get_current_frame_and_thread (StackTrace* st, guint *frame, gint *thread)
{
	GtkTreeIter iter;

	if (get_current_iter (st->treeview, &iter))
	{
		GtkTreeModel *model;
		GtkTreeIter parent;
		gchar *str;

		model = gtk_tree_view_get_model (st->treeview);
		*frame = 0;
		if (gtk_tree_model_iter_parent (model, &parent, &iter))
		{
			gtk_tree_model_get (model, &iter, STACK_TRACE_FRAME_COLUMN, &str, -1);
			if (str != NULL)
			{
				*frame = strtoul (str, NULL, 10);
				g_free (str);
			}
			gtk_tree_model_get (model, &parent, STACK_TRACE_THREAD_COLUMN, &str, -1);
		}
		else
		{
			gtk_tree_model_get (model, &iter, STACK_TRACE_THREAD_COLUMN, &str, -1);
		}
		*thread = (str != NULL) ? strtoul (str, NULL, 10) : 0;
		g_free (str);

		return thread != 0 ? TRUE : FALSE;
	}
	else
	{
		return FALSE;
	}
}

static gboolean
my_gtk_tree_model_get_iter_last(GtkTreeModel *model, GtkTreeIter *parent, GtkTreeIter *last)
{
	gboolean exist;
 	GtkTreeIter iter;

	exist = gtk_tree_model_iter_children(model, &iter, parent);
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

static gboolean
find_thread (GtkTreeModel *model, GtkTreeIter *iter, guint thread)
{
	gboolean found;

	for (found = gtk_tree_model_get_iter_first (model, iter); found; found = gtk_tree_model_iter_next (model, iter))
	{
		gchar *str;
		guint id;

		gtk_tree_model_get (model, iter, STACK_TRACE_THREAD_COLUMN, &str, -1);
		if (str != NULL)
		{
			id = strtoul (str, NULL, 10);
			g_free (str);
			if (id == thread) break;
		}
	}

	return found;
}

/* Private functions
 *---------------------------------------------------------------------------*/

static void
set_stack_frame (StackTrace *self, guint frame, gint thread)
{
	GtkTreeIter parent;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gboolean valid;

	model = gtk_tree_view_get_model (self->treeview);

	/* Clear old pointer */
	valid = find_thread (model, &parent, self->current_thread);
	if (valid)
	{
		if (gtk_tree_model_iter_nth_child (model, &iter, &parent, self->current_frame))
		{
        	/* clear pixmap on the previous active frame */
            gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
            					STACK_TRACE_ACTIVE_COLUMN, NULL, -1);
		}
	}

	if (self->current_thread != thread)
	{
		self->current_thread = thread;
		valid = find_thread (model, &parent, self->current_thread);
	}

	/* Set pointer on current frame if possible */
	self->current_frame = frame;
	if (valid)
	{
		if (gtk_tree_model_iter_nth_child (model, &iter, &parent, self->current_frame))
		{
			GdkPixbuf *pointer_pix = gdk_pixbuf_new_from_file (ANJUTA_PIXMAP_POINTER, NULL);

			gtk_tree_store_set (GTK_TREE_STORE(model), &iter,
								STACK_TRACE_ACTIVE_COLUMN, pointer_pix,
								-1);
			g_object_unref (pointer_pix);
		}
	}
}

static void
on_stack_trace_updated (const GList *stack, gpointer user_data, GError *error)
{
	StackPacket *packet = (StackPacket *)user_data;
	StackTrace *self;
	guint thread;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeIter parent;
	const GList *node;
	gboolean exist;
	GtkTreePath *path;

	g_return_if_fail (packet != NULL);

	self = packet->self;
	thread = packet->thread;
	if (packet->unblock) g_signal_handler_unblock (self->plugin, self->changed_handler);
	g_slice_free (StackPacket, packet);

	if (error != NULL) return;

	model = gtk_tree_view_get_model (self->treeview);

	if (!find_thread (model, &parent, thread)) return;

	/* Check if there are already some data */
	exist = my_gtk_tree_model_get_iter_last (GTK_TREE_MODEL (model), &parent, &iter);
	if (exist)
	{
		GValue val = {0};

		gtk_tree_model_get_value(model, &iter, STACK_TRACE_FRAME_COLUMN, &val);
		if (!G_IS_VALUE (&val))
		{
			/* Remove dummy child */
			gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);

			exist = FALSE;
		}
	}

	for (node = (const GList *)g_list_last((GList *)stack); node != NULL; node = node->prev)
	{
		IAnjutaDebuggerFrame *frame;
		gchar *frame_str;
		gchar *adr_str;
		gchar *line_str;
		gchar *uri;
		gchar *file;

		frame = (IAnjutaDebuggerFrame *)node->data;

		if (exist)
		{
			/* Check if it's the same stack frame */
			gchar *args;
			gulong address;
			guint line;
			gboolean same;

			gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
									STACK_TRACE_ADDR_COLUMN, &adr_str,
									STACK_TRACE_LINE_COLUMN, &line_str,
									STACK_TRACE_ARGS_COLUMN, &args,
								-1);
			address = adr_str != NULL ? strtoul (adr_str, NULL, 0) : 0;
			line = line_str != NULL ? strtoul (line_str, NULL, 10) : 0;
			same = (address == frame->address) && (line == frame->line);
			if ((args == NULL) || (frame->args == NULL))
			{
				same = same && (args == frame->args);
			}
			else
			{
				same = same && (strcmp (args, frame->args) == 0);
			}
			g_free (adr_str);
			g_free (args);

			if (same)
			{
				/* Same frame, just change the color */
				frame_str = g_strdup_printf ("%d", frame->level);
				gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
								   	STACK_TRACE_ACTIVE_COLUMN, NULL,
									STACK_TRACE_FRAME_COLUMN, frame_str,
								   	STACK_TRACE_COLOR_COLUMN, "black", -1);
				g_free (frame_str);

				/* Check previous frame */
				exist = my_gtk_tree_model_iter_prev (model, &iter);
				if (!exist || (node->prev != NULL))
				{
					/* Last frame or Upper frame still exist */
					continue;
				}
				/* Upper frame do not exist, remove all them */
			}

			/* New frame, remove all previous frame */
			GtkTreeIter first;

			gtk_tree_model_iter_children (model, &first, &parent);
			while (my_gtk_tree_iter_compare (model, &first, &iter) < 0)
			{
				gtk_tree_store_remove (GTK_TREE_STORE (model), &first);
			}
			gtk_tree_store_remove (GTK_TREE_STORE (model), &first);

			if (same)
			{
				break;
			}
			else
			{
				exist = FALSE;
			}
		}

		gtk_tree_store_prepend (GTK_TREE_STORE (model), &iter, &parent);

		frame_str = g_strdup_printf ("%d", frame->level);
		adr_str = g_strdup_printf ("0x%lx", frame->address);
		if (frame->file)
		{
			if (g_path_is_absolute (frame->file))
			{
				GFile *gio_file = g_file_new_for_path (frame->file);
				uri = g_file_get_uri (gio_file);
				file = strrchr(frame->file, G_DIR_SEPARATOR) + 1;
				g_object_unref (gio_file);
			}
			else
			{
				uri = NULL;
				file = frame->file;
			}
			line_str = g_strdup_printf ("%d", frame->line);
		}
		else
		{
			uri = NULL;
			file = frame->library;
			line_str = NULL;
		}

		gtk_tree_store_set(GTK_TREE_STORE (model), &iter,
					   STACK_TRACE_ACTIVE_COLUMN, NULL,
					   STACK_TRACE_FRAME_COLUMN, frame_str,
					   STACK_TRACE_FILE_COLUMN, file,
					   STACK_TRACE_LINE_COLUMN, line_str,
					   STACK_TRACE_FUNC_COLUMN, frame->function,
					   STACK_TRACE_ADDR_COLUMN, adr_str,
					   STACK_TRACE_ARGS_COLUMN, frame->args,
					   STACK_TRACE_URI_COLUMN, uri,
					   STACK_TRACE_COLOR_COLUMN, "red",
					   -1);
		g_free (uri);
		g_free (line_str);
		g_free (adr_str);
		g_free (frame_str);
	}
	gtk_tree_store_set(GTK_TREE_STORE (model), &parent,
	                   STACK_TRACE_DIRTY_COLUMN, FALSE,
	                   -1);

	/* Expand and show new path */
	path = gtk_tree_model_get_path (model, &parent);
	gtk_tree_view_expand_row (self->treeview, path, FALSE);
	if (self->current_thread == thread)
	{
		set_stack_frame (self, self->current_frame, self->current_thread);
		gtk_tree_view_scroll_to_cell (self->treeview, path, NULL, FALSE, 0, 0);
	}
	gtk_tree_path_free (path);
}


static void
list_stack_frame (StackTrace *self, guint thread, gboolean update)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean found;
	gboolean dirty;

	model = gtk_tree_view_get_model (self->treeview);

	if (!update)
	{
		/* Search thread */
		found = find_thread (model, &iter, thread);
		if (found)
		{
			/* Check if stack trace need to be updated */
			gtk_tree_model_get(model, &iter, STACK_TRACE_DIRTY_COLUMN, &dirty, -1);
		}
	}

	/* Update stack trace */
	if (update || !found || dirty)
	{
		StackPacket *packet;

		if (thread != self->current_thread)
		{
			/* Change current thread temporarily */
			dma_queue_set_thread (self->debugger, thread);
			g_signal_handler_block (self->plugin, self->changed_handler);
		}
		packet = g_slice_new (StackPacket);
		packet->thread = thread;
		packet->self = self;
		packet->scroll = update;
		packet->unblock = thread != self->current_thread;
		dma_queue_list_frame (self->debugger,
							(IAnjutaDebuggerCallback)on_stack_trace_updated,
							packet);
		if (thread != self->current_thread) dma_queue_set_thread (self->debugger, self->current_thread);
	}
}

static void
on_thread_updated (const GList *threads, gpointer user_data)
{
	StackTrace *self = (StackTrace *)user_data;
	GList *node;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	GList *new_threads;

	model = gtk_tree_view_get_model (self->treeview);

	/* Remove completed threads */
	new_threads = g_list_copy ((GList *)threads);
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid)
	{
		gchar *str;
		gint thread;

		gtk_tree_model_get(model, &iter, STACK_TRACE_THREAD_COLUMN, &str, -1);
		thread = (str != NULL) ? strtoul (str, NULL, 10) : 0;
		g_free (str);

		for (node = new_threads; node != NULL; node = node->next)
		{
			if (((IAnjutaDebuggerFrame *)node->data)->thread == thread) break;
		}

		if (node != NULL)
		{
			GtkTreePath *path;

			/* Thread still existing */
			new_threads = g_list_delete_link (new_threads, node);
			/* Set content as dirty */
			gtk_tree_store_set(GTK_TREE_STORE (model), &iter,
			                   STACK_TRACE_DIRTY_COLUMN, TRUE,
			                   STACK_TRACE_COLOR_COLUMN, "black",
			                   -1);

			/* Update stack frame if it is visible */
			path = gtk_tree_model_get_path (model, &iter);
			if (gtk_tree_view_row_expanded (self->treeview, path))
			{
				list_stack_frame (self, thread, TRUE);
			}
			gtk_tree_path_free (path);

			valid = gtk_tree_model_iter_next (model, &iter);
		}
		else
		{
			/* Thread completed */
			valid = gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
		}
	}


	/* Add new threads */
	while (new_threads != NULL)
	{
		GtkTreeIter child;
		gchar *str;

		str = g_strdup_printf ("%d", ((IAnjutaDebuggerFrame *)new_threads->data)->thread);
		gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
		gtk_tree_store_set(GTK_TREE_STORE (model), &iter,
		                   STACK_TRACE_THREAD_COLUMN, str,
		                   STACK_TRACE_DIRTY_COLUMN, TRUE,
		                   STACK_TRACE_COLOR_COLUMN, "red",
		                   -1);
		g_free (str);

		/* Add a dummy child, to get the row expander */
		gtk_tree_store_append (GTK_TREE_STORE (model), &child, &iter);

		new_threads = g_list_delete_link (new_threads, new_threads);
	}
}

static void
list_threads (StackTrace *self)
{
	dma_queue_list_thread (
			self->debugger,
			(IAnjutaDebuggerCallback)on_thread_updated,
			self);
}



/* Callback functions
 *---------------------------------------------------------------------------*/

static void
on_stack_frame_set_activate (GtkAction *action, gpointer user_data)
{
	StackTrace *self;
	gint thread;
	guint frame;

	self = (StackTrace*) user_data;

	if (get_current_frame_and_thread (self, &frame, &thread))
	{
		if (thread != self->current_thread)
		{
			dma_queue_set_thread (self->debugger, thread);
			dma_queue_set_frame (self->debugger, frame);
			set_stack_frame (self, frame, thread);
			list_stack_frame (self, thread, FALSE);
		}
		else if (frame != self->current_frame)
		{
			dma_queue_set_frame (self->debugger, frame);
			set_stack_frame (self, frame, thread);
			list_stack_frame (self, thread, FALSE);
		}
	}
}

static void
on_stack_view_source_activate (GtkAction *action, gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeView *view;
	gchar *uri;
	gchar *line_str;
	guint line;
	gchar *adr_str;
	gulong address;

	StackTrace* st = (StackTrace*) user_data;

	view = st->treeview;
	selection = gtk_tree_view_get_selection (view);
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;

	/* get the frame info */
	gtk_tree_model_get (model, &iter,
						STACK_TRACE_URI_COLUMN, &uri,
						STACK_TRACE_LINE_COLUMN, &line_str,
						STACK_TRACE_ADDR_COLUMN, &adr_str,
						-1);

	address = adr_str != NULL ? strtoul (adr_str, NULL, 0) : 0;
	line = line_str != NULL ? strtoul (line_str, NULL, 0) : 0;
	g_signal_emit_by_name (st->plugin, "location-changed", address, uri, line);
	g_free (uri);
	g_free (line_str);
	g_free (adr_str);
}

static void
on_got_stack_trace (const gchar *trace, gpointer user_data, GError *error)
{
	StackTrace* st = (StackTrace*) user_data;
	IAnjutaDocumentManager *docman;

	docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (st->plugin)->shell, IAnjutaDocumentManager, NULL);
	if (docman != NULL)
	{
		ianjuta_document_manager_add_buffer (docman, "Stack Trace", trace, NULL);
	}
}

static void
on_stack_get_trace (GtkAction *action, gpointer user_data)
{
	StackTrace* st = (StackTrace*) user_data;

	/* Ask debugger to get all frame data */
	dma_queue_dump_stack_trace (
			st->debugger,
			(IAnjutaDebuggerCallback)on_got_stack_trace,
			st);
}

static void
on_stack_trace_row_activated           (GtkTreeView     *treeview,
                                        GtkTreePath     *arg1,
                                        GtkTreeViewColumn *arg2,
                                        StackTrace      *st)
{
	on_stack_frame_set_activate (NULL, st);
}

static void
on_stack_trace_row_expanded            (GtkTreeView     *treeview,
                                        GtkTreeIter     *iter,
                                        GtkTreePath     *path,
                                        StackTrace      *st)
{
	GtkTreeModel *model;
	gchar *str;
	guint thread;

	model = gtk_tree_view_get_model (treeview);

	gtk_tree_model_get (model, iter, STACK_TRACE_THREAD_COLUMN, &str, -1);
	thread = (str != NULL) ? strtoul (str, NULL, 10) : 0;
	g_free (str);

	list_stack_frame (st, thread, FALSE);
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
	},
	{
		"ActionDmaDumpStackTrace",
		NULL,
		N_("Get Stack trace"),
		NULL,
		NULL,
		G_CALLBACK (on_stack_get_trace)
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
	model = GTK_TREE_MODEL(gtk_tree_store_new (STACK_TRACE_N_COLUMNS,
											   GDK_TYPE_PIXBUF,
	                                           G_TYPE_STRING,
	                                           G_TYPE_STRING,
											   G_TYPE_STRING,
											   G_TYPE_STRING,
											   G_TYPE_STRING,
											   G_TYPE_STRING,
											   G_TYPE_STRING,
	                                           G_TYPE_BOOLEAN,
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

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Thread"));
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
										STACK_TRACE_THREAD_COLUMN);
	gtk_tree_view_column_add_attribute (column, renderer, "foreground",
										STACK_TRACE_COLOR_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
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

	if (dma_debugger_queue_is_supported (st->debugger, HAS_MEMORY))
	{
		/* Display address only if debugger has such concept */
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
	}

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
	g_signal_connect (st->treeview, "row-expanded", G_CALLBACK (on_stack_trace_row_expanded), st);

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
on_frame_changed (StackTrace *self, guint frame, gint thread)
{
	set_stack_frame (self, frame, thread);
	list_stack_frame (self, thread, FALSE);
}

static void
on_program_moved (StackTrace *self, guint pid, gint thread)
{
	set_stack_frame (self, 0, thread);

	list_threads (self);
	list_stack_frame (self, thread, TRUE);
}

static void
on_program_exited (StackTrace *self)
{
	g_signal_handlers_disconnect_by_func (self->plugin, G_CALLBACK (on_program_exited), self);
	g_signal_handlers_disconnect_by_func (self->plugin, G_CALLBACK (on_program_moved), self);
	g_signal_handlers_disconnect_by_func (self->plugin, G_CALLBACK (on_frame_changed), self);

	destroy_stack_trace_gui (self);
}

static void
on_program_started (StackTrace *self)
{
	create_stack_trace_gui (self);
	self->current_thread = 0;
	self->current_frame = 0;

	g_signal_connect_swapped (self->plugin, "program-exited", G_CALLBACK (on_program_exited), self);
	g_signal_connect_swapped (self->plugin, "program-moved", G_CALLBACK (on_program_moved), self);
	self->changed_handler = g_signal_connect_swapped (self->plugin, "frame-changed", G_CALLBACK (on_frame_changed), self);
}

/* Public functions
 *---------------------------------------------------------------------------*/

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

StackTrace *
stack_trace_new (DebugManagerPlugin *plugin)
{
	StackTrace *st;
	AnjutaUI *ui;

	st = g_new0 (StackTrace, 1);
	if (st == NULL) return NULL;

	st->plugin = plugin;
	st->debugger = dma_debug_manager_get_queue (plugin);

	/* Register actions */
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN(st->plugin)->shell, NULL);
	st->action_group =
		anjuta_ui_add_action_group_entries (ui, "ActionGroupStack",
											_("Stack frame operations"),
											actions_stack_trace,
											G_N_ELEMENTS (actions_stack_trace),
											GETTEXT_PACKAGE, TRUE, st);

	g_signal_connect_swapped (st->plugin, "program-started", G_CALLBACK (on_program_started), st);

	return st;
}

void
stack_trace_free (StackTrace * st)
{
	AnjutaUI *ui;

	g_return_if_fail (st != NULL);

	/* Disconnect from debugger */
	g_signal_handlers_disconnect_matched (st->plugin, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, st);

	/* Remove menu actions */
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (st->plugin)->shell, NULL);
	anjuta_ui_remove_action_group (ui, st->action_group);

	/* Destroy menu */
	destroy_stack_trace_gui	(st);

	g_free (st);
}
