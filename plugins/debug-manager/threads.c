/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    threads.c
    Copyright (C) 2007  Sébastien Granjoux

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

#include "threads.h"

#include "queue.h"

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

struct _DmaThreads
{
	DebugManagerPlugin *plugin;
	DmaDebuggerQueue *debugger;

	/* GUI */
	GtkWidget *scrolledwindow;
	GtkTreeView *list;
	GtkMenu *menu;
	GtkActionGroup *action_group;
	
	gint current_thread;
};

struct _DmaThreadPacket {
	GtkTreeModel *model;
	GtkTreeRowReference* reference;
	DmaThreads* self;
};

enum {
	THREAD_ACTIVE_COLUMN,
	THREAD_ID_COLUMN,
	THREAD_FILE_COLUMN,
	THREAD_LINE_COLUMN,
	THREAD_FUNC_COLUMN,
	THREAD_ADDR_COLUMN,
	THREAD_URI_COLUMN,
	THREAD_N_COLUMNS
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
 * returns the current stack thread or -1 on error
 */
static gint
get_current_index (DmaThreads *self)
{
	GtkTreeIter iter;
	
	if (get_current_iter (self->list, &iter))
	{
		GtkTreeModel *model;
		gint thread_no;
	
		model = gtk_tree_view_get_model (self->list);		
		gtk_tree_model_get (model, &iter, THREAD_ID_COLUMN, &thread_no, -1);
		
		return thread_no;
	}
	else
	{
		return -1;
	}
}

/* Private functions
 *---------------------------------------------------------------------------*/

static void
dma_threads_clear (DmaThreads *self)
{
	GtkTreeModel* model;
	
	model = gtk_tree_view_get_model (self->list);
	gtk_list_store_clear (GTK_LIST_STORE (model));
}

/* Callback functions
 *---------------------------------------------------------------------------*/

static void
on_threads_set_activate (GtkAction *action, gpointer user_data)
{
	DmaThreads *self = (DmaThreads *)user_data;
	gint selected_thread;
	
	selected_thread = get_current_index (self);
	
	/* No thread selected */
	if (selected_thread == -1)
		return;
	
	/* current thread is already active */
	if (selected_thread == self->current_thread)
		return;
	
	/* issue a command to switch active frame to new location */
	dma_queue_set_thread (self->debugger, selected_thread);
}

static void
on_threads_source_activate (GtkAction *action, gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeView *view;
	gchar *uri;
	guint line;
	gchar *adr;
	guint address;
	DmaThreads* self = (DmaThreads*) user_data;		

	view = self->list;
	selection = gtk_tree_view_get_selection (view);
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;

	/* get the frame info */
	gtk_tree_model_get (model, &iter,
						THREAD_URI_COLUMN, &uri,
						THREAD_LINE_COLUMN, &line,
						THREAD_ADDR_COLUMN, &adr,
						-1);

	address = adr != NULL ? strtoul (adr, NULL, 0) : 0;
	g_signal_emit_by_name (self->plugin, "location-changed", address, uri, line);
	g_free (uri);
	g_free (adr);
}

static void
on_threads_row_activated           (GtkTreeView     *treeview,
                                        GtkTreePath     *arg1,
                                        GtkTreeViewColumn *arg2,
                                        DmaThreads* self)
{
	on_threads_set_activate (NULL, self);
}

static gboolean
on_threads_button_press (GtkWidget *widget, GdkEventButton *bevent, gpointer user_data)
{
	DmaThreads *self = (DmaThreads*) user_data;

	if ((bevent->type == GDK_BUTTON_PRESS) && (bevent->button == 3))
	{
		/* Right mouse click */
		g_return_val_if_fail (self->menu != NULL, FALSE);
		gtk_menu_popup (GTK_MENU (self->menu), NULL, NULL, NULL, NULL,
						bevent->button, bevent->time);
	}
	else if ((bevent->type == GDK_2BUTTON_PRESS) && (bevent->button == 1))
	{
		/* Double left mouse click */
		on_threads_source_activate (NULL, user_data);
	}
	
	return FALSE;
}

static void
on_info_thread (const IAnjutaDebuggerFrame* frame, gpointer user_data)
{
	GtkTreeRowReference* reference = (GtkTreeRowReference*)user_data;
	gchar* adr;
	gchar* uri;
	gchar* file;
	
	if (frame == NULL) return;
	
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

	if (gtk_tree_row_reference_valid (reference))
	{
		GtkTreeIter iter;
		GtkTreeModel *model;
		GtkTreePath *path;
		gboolean ok;

		model = gtk_tree_row_reference_get_model(reference);
		path = gtk_tree_row_reference_get_path (reference);
		ok = gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_path_free (path);

		if (ok)
		{
			gtk_list_store_set(GTK_LIST_STORE (model), &iter, 
					THREAD_FILE_COLUMN, file,
			   		THREAD_LINE_COLUMN, frame->line,
			   		THREAD_FUNC_COLUMN, frame->function,
			   		THREAD_ADDR_COLUMN, adr,
			   		THREAD_URI_COLUMN, uri,
			   		-1);
		}
		gtk_tree_row_reference_free(reference);
	}

	g_free (uri);
	g_free (adr);
}

static void
on_list_thread (const GList *threads, gpointer user_data)
{
	DmaThreads *self = (DmaThreads *)user_data;
	const GList *node;
	GtkListStore *model;		

	dma_threads_clear (self);

	model = GTK_LIST_STORE (gtk_tree_view_get_model (self->list));

	for (node = threads; node != NULL; node = node->next)
	{
		GdkPixbuf *pic;
		GtkTreeIter iter;
		IAnjutaDebuggerFrame *frame;
		gchar *adr;
		gchar *uri;
		gchar *file;

		frame = (IAnjutaDebuggerFrame *)node->data;

		gtk_list_store_append (model, &iter);

		/* if we are on the current frame set iterator and pixmap correctly */
		if (frame->thread == self->current_thread)
			pic = gdk_pixbuf_new_from_file (ANJUTA_PIXMAP_POINTER, NULL);
		else
			pic = NULL;

		if ((dma_debugger_queue_is_supported (self->debugger, HAS_CPU) && (frame->address == 0))
			|| (frame->function == NULL))
		{
			/* Missing frame address, request more information */
			GtkTreeRowReference* reference;
			GtkTreePath *path;
			
			path = gtk_tree_model_get_path(GTK_TREE_MODEL (model), &iter); 
			reference = gtk_tree_row_reference_new (GTK_TREE_MODEL (model), path);
			gtk_tree_path_free (path);
			
			dma_queue_info_thread (
				self->debugger, frame->thread,
				(IAnjutaDebuggerCallback)on_info_thread,
				reference);
			
			gtk_list_store_set(model, &iter, 
						   	THREAD_ACTIVE_COLUMN, pic,
						   	THREAD_ID_COLUMN, frame->thread,
							-1);
		}
		else
		{
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
						   	THREAD_ACTIVE_COLUMN, pic,
						   	THREAD_ID_COLUMN, frame->thread, 
							THREAD_FILE_COLUMN, file,
					   		THREAD_LINE_COLUMN, frame->line,
					   		THREAD_FUNC_COLUMN, frame->function,
					   		THREAD_ADDR_COLUMN, adr,
					   		THREAD_URI_COLUMN, uri,
					   		-1);
		
			g_free (uri);
			g_free (adr);
		}
		
		if (pic)
			gdk_pixbuf_unref (pic);
	}
}

static void
dma_threads_update (DmaThreads *self)
{
	dma_queue_list_thread (
			self->debugger,
			(IAnjutaDebuggerCallback)on_list_thread,
			self);
}

/* Actions table
 *---------------------------------------------------------------------------*/

static GtkActionEntry actions_threads[] = {
	{
		"ActionDmaSetCurrentThread",         	/* Action name */
		NULL,                                 	/* Stock icon, if any */
		N_("Set current thread"),               /* Display label */
		NULL,                                   /* short-cut */
		NULL,                                   /* Tooltip */
		G_CALLBACK (on_threads_set_activate) 	/* action callback */
	},
	{
		"ActionDmaJumpToThread",
		NULL,
		N_("View Source"),
		NULL,
		NULL,
		G_CALLBACK (on_threads_source_activate)
	}
};		

static void
dma_threads_create_gui(DmaThreads *self)
{
	GtkTreeModel* model;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	AnjutaUI *ui;

	g_return_if_fail (self->scrolledwindow == NULL);
	
	/* Create tree view */
	model = GTK_TREE_MODEL(gtk_list_store_new (THREAD_N_COLUMNS,
											   GDK_TYPE_PIXBUF,
											   G_TYPE_UINT,
											   G_TYPE_STRING,
											   G_TYPE_UINT,
											   G_TYPE_STRING,
											   G_TYPE_STRING,
											   G_TYPE_STRING));
	self->list = GTK_TREE_VIEW (gtk_tree_view_new_with_model (model));
	g_object_unref (G_OBJECT (model));
	
	selection = gtk_tree_view_get_selection (self->list);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	/* Columns */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Active"));
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "pixbuf",
										THREAD_ACTIVE_COLUMN);
	gtk_tree_view_append_column (self->list, column);
	gtk_tree_view_set_expander_column (self->list, column);
	
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Id"));
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
										THREAD_ID_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (self->list, column);
	gtk_tree_view_set_expander_column (self->list, column);
	
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
										THREAD_FILE_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("File"));
	gtk_tree_view_append_column (self->list, column);

	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
										THREAD_LINE_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Line"));
	gtk_tree_view_append_column (self->list, column);
	
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
										THREAD_FUNC_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Function"));
	gtk_tree_view_append_column (self->list, column);
	
	if (dma_debugger_queue_is_supported (self->debugger, HAS_CPU))
	{
		/* Display address only if debugger has such concept */
		column = gtk_tree_view_column_new ();
		renderer = gtk_cell_renderer_text_new ();
		gtk_tree_view_column_pack_start (column, renderer, TRUE);
		gtk_tree_view_column_add_attribute (column, renderer, "text",
											THREAD_ADDR_COLUMN);
		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_column_set_title (column, _("Address"));
		gtk_tree_view_append_column (self->list, column);
	}
	
	/* Create popup menu */
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN(self->plugin)->shell, NULL);
	self->menu = GTK_MENU (gtk_ui_manager_get_widget (GTK_UI_MANAGER (ui), "/PopupThread"));
	
	/* Connect signal */
	g_signal_connect (self->list, "button-press-event", G_CALLBACK (on_threads_button_press), self);  
	g_signal_connect (self->list, "row-activated", G_CALLBACK (on_threads_row_activated), self);  
	
	/* Add stack window */
	self->scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (self->scrolledwindow),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (self->scrolledwindow),
										 GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (self->scrolledwindow),
					   GTK_WIDGET (self->list));
	gtk_widget_show_all (self->scrolledwindow);
	
	anjuta_shell_add_widget (ANJUTA_PLUGIN(self->plugin)->shell,
							 self->scrolledwindow,
							 "AnjutaDebuggerThread", _("Thread"),
							 "gdb-stack-icon", ANJUTA_SHELL_PLACEMENT_BOTTOM,
							 NULL);

}

static void
dma_destroy_threads_gui (DmaThreads *self)
{
	if (self->scrolledwindow != NULL)
	{
		gtk_widget_destroy (self->scrolledwindow);
		self->scrolledwindow = NULL;
	}
}

static void
on_program_moved (DmaThreads *self, guint pid, gint thread)
{
	self->current_thread = thread;
	dma_threads_update (self);
}

static gboolean
on_mark_selected_thread (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data)
{
	DmaThreads* self = (DmaThreads *)user_data;
	GdkPixbuf *pic;
	gint thread;

	gtk_tree_model_get (model, iter, THREAD_ACTIVE_COLUMN, &pic, THREAD_ID_COLUMN, &thread, -1);
	
	if (pic != NULL)
	{
		/* Remove previously selected thread marker */
		gdk_pixbuf_unref (pic);
		pic = NULL;
	}		
	
	if (self->current_thread == thread)
	{
		/* Create marker if needed */
		pic = gdk_pixbuf_new_from_file (ANJUTA_PIXMAP_POINTER, NULL);
	}
		
	gtk_list_store_set (GTK_LIST_STORE (model), iter, 
						   	THREAD_ACTIVE_COLUMN, pic, -1);
	
	if (pic != NULL)
	{
		gdk_pixbuf_unref (pic);
	}
	
	return FALSE;
}

static void
on_frame_changed (DmaThreads *self, guint frame, gint thread)
{
	if (thread != self->current_thread)
	{
		GtkTreeModel *model;
	
		self->current_thread = thread;
		model = gtk_tree_view_get_model (self->list);		
		
		gtk_tree_model_foreach (model, on_mark_selected_thread, self);
	}
}

static void
on_program_exited (DmaThreads *self)
{
	g_signal_handlers_disconnect_by_func (self->plugin, G_CALLBACK (on_program_exited), self);
	g_signal_handlers_disconnect_by_func (self->plugin, G_CALLBACK (on_program_moved), self);
	g_signal_handlers_disconnect_by_func (self->plugin, G_CALLBACK (on_frame_changed), self);
	
	dma_threads_clear (self);
	dma_destroy_threads_gui (self);
}

static void
on_program_started (DmaThreads *self)
{
	dma_threads_create_gui (self);
	
	g_signal_connect_swapped (self->plugin, "program-exited", G_CALLBACK (on_program_exited), self);
	g_signal_connect_swapped (self->plugin, "program-moved", G_CALLBACK (on_program_moved), self);
	g_signal_connect_swapped (self->plugin, "frame-changed", G_CALLBACK (on_frame_changed), self);
}

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

DmaThreads*
dma_threads_new (DebugManagerPlugin *plugin)
{
	DmaThreads* self;
	AnjutaUI *ui;

	self = g_new0 (DmaThreads, 1);
	g_return_val_if_fail (self != NULL, NULL);

	self->plugin = plugin;
	self->debugger = dma_debug_manager_get_queue (plugin);

	/* Register actions */
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN(self->plugin)->shell, NULL);
	self->action_group =
		anjuta_ui_add_action_group_entries (ui, "ActionGroupThread",
											_("Thread operations"),
											actions_threads,
											G_N_ELEMENTS (actions_threads),
											GETTEXT_PACKAGE, TRUE, self);

	g_signal_connect_swapped (self->plugin, "program-started", G_CALLBACK (on_program_started), self);
	
	return self;
}

void
dma_threads_free (DmaThreads *self)
{
	AnjutaUI *ui;
	
	g_return_if_fail (self != NULL);
	
	/* Disconnect from debugger */
	g_signal_handlers_disconnect_matched (self->plugin, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, self);	

	/* Remove menu actions */
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (self->plugin)->shell, NULL);
	anjuta_ui_remove_action_group (ui, self->action_group);
	
	/* Destroy menu */
	dma_destroy_threads_gui	(self);
	
	g_free (self);
}
