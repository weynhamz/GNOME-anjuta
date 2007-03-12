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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "threads.h"

#include "plugin.h"

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <gnome.h>

#define DEBUG
#include <libanjuta/resources.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>

#include "utilities.h"
#include "info.h"

#define ANJUTA_PIXMAP_POINTER PACKAGE_PIXMAPS_DIR"/pointer.png"

struct _DmaThreads
{
	AnjutaPlugin *plugin;
	IAnjutaDebugger *debugger;

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
	guint selected_thread;
	
	selected_thread = get_current_index (self);
	
	/* No thread selected */
	if (selected_thread == -1)
		return;
	
	/* current thread is already active */
	if (selected_thread == self->current_thread)
		return;
	
	/* issue a command to switch active frame to new location */
	ianjuta_debugger_set_thread (self->debugger, selected_thread, NULL);
}

static void
on_threads_src_activate (GtkAction *action, gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeView *view;
	gchar *uri;
	guint line;
	DmaThreads* self = (DmaThreads*) user_data;		

	view = self->list;
	selection = gtk_tree_view_get_selection (view);
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;

	/* get the frame info */
	gtk_tree_model_get (model, &iter,
						THREAD_URI_COLUMN, &uri,
						THREAD_LINE_COLUMN, &line,
						-1);
	
	goto_location_in_editor (self->plugin, uri, line);
	g_free (uri);
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
		on_threads_src_activate (NULL, user_data);
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

		DEBUG_PRINT("frame thread %d current thread %d", frame->thread, self->current_thread);
		/* if we are on the current frame set iterator and pixmap correctly */
		if (frame->thread == self->current_thread)
			pic = gdk_pixbuf_new_from_file (ANJUTA_PIXMAP_POINTER, NULL);
		else
			pic = NULL;

		if (frame->address == 0)
		{
			/* Missing frame address, request more information */
			GtkTreeRowReference* reference;
			GtkTreePath *path;
			
			path = gtk_tree_model_get_path(GTK_TREE_MODEL (model), &iter); 
			reference = gtk_tree_row_reference_new (GTK_TREE_MODEL (model), path);
			gtk_tree_path_free (path);
			
			ianjuta_debugger_info_thread (
				self->debugger, frame->thread,
				(IAnjutaDebuggerCallback)on_info_thread,
				reference,
				NULL);
			
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
	ianjuta_debugger_list_thread (
			self->debugger,
			(IAnjutaDebuggerCallback)on_list_thread,
			self,
			NULL);
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
		G_CALLBACK (on_threads_src_activate)
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
	
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
										THREAD_ADDR_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Address"));
	gtk_tree_view_append_column (self->list, column);
	
	/* Create popup menu */
	ui = anjuta_shell_get_ui (self->plugin->shell, NULL);
	self->menu = GTK_MENU (gtk_ui_manager_get_widget (GTK_UI_MANAGER (ui), "/PopupThread"));
	
	/* FIXME: Temporary disable set thread menu item */
	GtkAction *action = anjuta_ui_get_action (ui, "ActionGroupThread", "ActionDmaSetCurrentThread");
	gtk_action_set_sensitive (GTK_ACTION (action), FALSE);
	
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
	
	anjuta_shell_add_widget (self->plugin->shell,
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
on_program_stopped (DmaThreads *self, guint thread)
{
	self->current_thread = thread;
	dma_threads_update (self);
}

static void
on_debugger_started (DmaThreads *self)
{
	dma_threads_create_gui (self);
}

static void
on_debugger_stopped (DmaThreads *self)
{
	dma_threads_clear (self);
	dma_destroy_threads_gui (self);
}

static void
on_frame_changed (DmaThreads *self, guint frame, guint thread)
{
}

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

DmaThreads*
dma_threads_new (IAnjutaDebugger *debugger, AnjutaPlugin *plugin)
{
	DmaThreads* self;
	AnjutaUI *ui;

	self = g_new0 (DmaThreads, 1);
	g_return_val_if_fail (self != NULL, NULL);

	self->plugin = plugin;
	self->debugger = debugger;
	if (debugger != NULL) g_object_ref (debugger);

	/* Register actions */
	ui = anjuta_shell_get_ui (self->plugin->shell, NULL);
	DEBUG_PRINT("add actions");
	self->action_group =
		anjuta_ui_add_action_group_entries (ui, "ActionGroupThread",
											_("Thread operations"),
											actions_threads,
											G_N_ELEMENTS (actions_threads),
											GETTEXT_PACKAGE, TRUE, self);
	DEBUG_PRINT("add actions end");

	g_signal_connect_swapped (self->debugger, "debugger-started", G_CALLBACK (on_debugger_started), self);
	g_signal_connect_swapped (self->debugger, "debugger-stopped", G_CALLBACK (on_debugger_stopped), self);
	g_signal_connect_swapped (self->debugger, "program-stopped", G_CALLBACK (on_program_stopped), self);
	g_signal_connect_swapped (self->debugger, "frame-changed", G_CALLBACK (on_frame_changed), self);
	
	return self;
}

void
dma_threads_free (DmaThreads *self)
{
	AnjutaUI *ui;
	
	g_return_if_fail (self != NULL);

	/* Disconnect from debugger */
	if (self->debugger != NULL)
	{	
		g_signal_handlers_disconnect_by_func (self->debugger, G_CALLBACK (on_debugger_started), self);
		g_signal_handlers_disconnect_by_func (self->debugger, G_CALLBACK (on_debugger_stopped), self);
		g_signal_handlers_disconnect_by_func (self->debugger, G_CALLBACK (on_program_stopped), self);
		g_signal_handlers_disconnect_by_func (self->debugger, G_CALLBACK (on_frame_changed), self);
		g_object_unref (self->debugger);	
	}

	/* Remove menu actions */
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (self->plugin)->shell, NULL);
	anjuta_ui_remove_action_group (ui, self->action_group);
	
	/* Destroy menu */
	dma_destroy_threads_gui	(self);
	
	g_free (self);
}
