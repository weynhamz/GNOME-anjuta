/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    sharedlibs.c
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

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <libanjuta/resources.h>
#include <libanjuta/anjuta-shell.h>

#include "utilities.h"
#include "sharedlib.h"
#include "queue.h"

enum 
{
	SHL_COLUMN_SHARED_OBJECT,
	SHL_COLUMN_FROM,
	SHL_COLUMN_TO,
	SHL_COLUMN_SYM_READ,
	SHL_COLUMN_COUNT
};

static gint
on_sharedlibs_delete_event (GtkWidget* w, GdkEvent *event, gpointer data)
{
  Sharedlibs* sl = data;
  sharedlibs_hide(sl);
  return TRUE;
}

static gboolean
on_sharedlibs_key_press_event (GtkWidget *widget, GdkEventKey *event,
							  gpointer data)
{
	if (event->keyval == GDK_Escape)
	{
		Sharedlibs* sl = data;
  		sharedlibs_hide(sl);
		return TRUE;
	}
	return FALSE;
}


	static void
sharedlibs_update (const GList *lines, gpointer data)
{
	Sharedlibs *sl;
	gchar obj[512], from[32], to[32], read[32];
	gint count;
	GList *list, *node;
	GtkTreeIter iter;

	sl = (Sharedlibs*)data;

	list = gdb_util_remove_blank_lines(lines);
	sharedlibs_clear(sl);
	if(g_list_length(list) < 2 ){g_list_free(list); return;}
	node = list->next;
	if (node)
		node = g_list_next (node);
	while(node)
	{
		count = sscanf((char*)node->data, "~%s %s %s %s", from,to,read,obj);
		node = g_list_next(node);
		if(count != 4) continue;
		gtk_list_store_append (sl->widgets.store, &iter);
		gtk_list_store_set (sl->widgets.store, &iter,
				SHL_COLUMN_SHARED_OBJECT, g_path_get_basename (obj),
				SHL_COLUMN_FROM, from,
				SHL_COLUMN_TO, to,
				SHL_COLUMN_SYM_READ, strcmp(read, "Yes") == 0,
				-1);
	}
	g_list_free(list);
}

static void
on_sharedlibs_update_activate (GtkMenuItem *menuitem, gpointer user_data)
{
	Sharedlibs *sl = (Sharedlibs *)user_data;
	
	dma_queue_info_sharedlib (
			sl->debugger,
			(IAnjutaDebuggerCallback)sharedlibs_update,
			sl);
}

static void
sharedlibs_update_controls (Sharedlibs* sl)
{
	gboolean R;

	R = dma_debugger_queue_get_state (sl->debugger) == IANJUTA_DEBUGGER_PROGRAM_STOPPED;

	gtk_action_group_set_sensitive (sl->action_group, R);
}

static gboolean
on_sharedlibs_event (GtkWidget *widget, GdkEvent  *event, gpointer user_data)
{
	GdkEventButton *bevent;
	Sharedlibs *ew = (Sharedlibs*)user_data;
	
	if (event->type != GDK_BUTTON_PRESS)
		return FALSE;
	if (((GdkEventButton *)event)->button != 3)
		return FALSE;
	bevent =  (GdkEventButton *)event;
	bevent->button = 1;
	sharedlibs_update_controls(ew);
	gtk_menu_popup (GTK_MENU(ew->widgets.menu), NULL,
					NULL, NULL, NULL,
					bevent->button, bevent->time);
	return TRUE;
}

static GtkActionEntry sharedlibs_menu_actions[] =
{
	{
		"ActionDmaSharedlibsUpdate",
		GTK_STOCK_REFRESH,
		N_("Update"),
		NULL, NULL,
		G_CALLBACK (on_sharedlibs_update_activate)
	}
};

static GtkWidget*
sharedlibs_ui_create_treeview_and_store (Sharedlibs *sl)
{
	GtkWidget *treeview;
	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	store = gtk_list_store_new (SHL_COLUMN_COUNT,
			G_TYPE_STRING,
			G_TYPE_STRING,
			G_TYPE_STRING,
			G_TYPE_BOOLEAN);
	treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Shared Object"),
			renderer,
			"text", SHL_COLUMN_SHARED_OBJECT,
			NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("From"),
			renderer,
			"text", SHL_COLUMN_FROM,
			NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("To"),
			renderer,
			"text", SHL_COLUMN_TO,
			NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	renderer = gtk_cell_renderer_toggle_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Symbols read"),
			renderer,
			"active", SHL_COLUMN_SYM_READ,
			NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	sl->widgets.treeview = treeview;
	sl->widgets.store = store;

	return treeview;
}

static void
destroy_sharedlibs_gui (Sharedlibs *sl)
{

}

static void
create_sharedlibs_gui (Sharedlibs *sl)
{
	GtkWidget *window3;
	GtkWidget *scrolledwindow4;
	GtkWidget *treeview;
	AnjutaUI *ui;
	
	window3 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_size_request (window3, 170, -2);
	gtk_window_set_title (GTK_WINDOW (window3), _("Shared libraries"));
	gtk_window_set_wmclass (GTK_WINDOW (window3), "sharedlibs", "Anjuta");
	gtk_window_set_default_size (GTK_WINDOW (window3), 240, 230);
	
	scrolledwindow4 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow4);
	gtk_container_add (GTK_CONTAINER (window3), scrolledwindow4);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow4),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);

	treeview = sharedlibs_ui_create_treeview_and_store (sl);
	gtk_widget_show (treeview);
	gtk_container_add (GTK_CONTAINER (scrolledwindow4), treeview);
	
	g_signal_connect (G_OBJECT (window3), "delete_event",
						G_CALLBACK (on_sharedlibs_delete_event), sl);
	g_signal_connect (G_OBJECT (window3), "key-press-event",
						G_CALLBACK (on_sharedlibs_key_press_event), sl);

	g_signal_connect (G_OBJECT (treeview), "event",
						G_CALLBACK (on_sharedlibs_event),
						sl);
	
	sl->widgets.window = window3;
	sl->widgets.treeview = treeview;
	ui = anjuta_shell_get_ui (sl->plugin->shell, NULL);
	sl->widgets.menu = gtk_ui_manager_get_widget (GTK_UI_MANAGER (ui),
			"/PopupSharedlibs");

	sl->is_showing = FALSE;
	sl->win_width = 410;
	sl->win_height = 370;
	sl->win_pos_x = 120;
	sl->win_pos_y = 140;
}

static void
on_program_unloaded (Sharedlibs *sl)
{
	g_signal_handlers_disconnect_by_func (sl->plugin, G_CALLBACK (on_program_unloaded), sl);

	destroy_sharedlibs_gui(sl);
}

static void
on_program_loaded (Sharedlibs *sl)
{
	create_sharedlibs_gui(sl);
	
	g_signal_connect_swapped (sl->plugin, "program-unloaded", G_CALLBACK (on_program_unloaded), sl);
}

/* Public functions
 *---------------------------------------------------------------------------*/

void
sharedlibs_clear (Sharedlibs *sl)
{
	g_return_if_fail (sl->widgets.store != NULL);
	g_return_if_fail (GTK_IS_LIST_STORE (sl->widgets.store));

	gtk_list_store_clear (sl->widgets.store);
}

void
sharedlibs_show (Sharedlibs* sl)
{
	if(sl)
	{
		if(sl->is_showing)
		{
			gdk_window_raise(sl->widgets.window->window);
		}
		else
		{
			gtk_widget_set_uposition(sl->widgets.window, sl->win_pos_x,
									 sl->win_pos_y);
			gtk_window_set_default_size(GTK_WINDOW(sl->widgets.window),
										sl->win_width, sl->win_height);
			gtk_widget_show(sl->widgets.window);
			sl->is_showing = TRUE;
			dma_queue_info_sharedlib (
					sl->debugger,
					(IAnjutaDebuggerCallback)sharedlibs_update,
					sl);
		}
	}
}

void
sharedlibs_hide (Sharedlibs* sl)
{
	if(sl)
	{
		if(sl->is_showing == FALSE) return;
			gdk_window_get_root_origin(sl->widgets.window->window,
									   &sl->win_pos_x, &sl->win_pos_y);
		gdk_window_get_size(sl ->widgets.window->window, &sl->win_width,
							&sl->win_height);
		gtk_widget_hide(sl->widgets.window);
		sl->is_showing = FALSE;
	}
}

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

Sharedlibs*
sharedlibs_new (DebugManagerPlugin *plugin)
{
	Sharedlibs* sl;
	AnjutaUI *ui;

	sl = g_new0 (Sharedlibs, 1);
	g_return_val_if_fail (sl != NULL, NULL);

	sl->plugin = ANJUTA_PLUGIN (plugin);
	sl->debugger = dma_debug_manager_get_queue (plugin);
		
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN(plugin)->shell, NULL);
	sl->action_group = 
		anjuta_ui_add_action_group_entries (ui,
				"ActionGroupSharedlibs",
				_("Shared library operations"),
				sharedlibs_menu_actions,
				G_N_ELEMENTS(sharedlibs_menu_actions),
				GETTEXT_PACKAGE, TRUE, sl);

	g_signal_connect_swapped (plugin, "program-loaded", G_CALLBACK (on_program_loaded), sl);

	return sl;
}

void
sharedlibs_free(Sharedlibs* sl)
{
	AnjutaUI *ui;

	g_return_if_fail (sl != NULL);
	
	/* Disconnect from debugger */
	g_signal_handlers_disconnect_matched (sl->plugin, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, sl);	

	/* Remove menu actions */
	ui = anjuta_shell_get_ui (sl->plugin->shell, NULL);
	anjuta_ui_remove_action_group (ui, sl->action_group);
	
	/* Destroy GUI */
	destroy_sharedlibs_gui (sl);
	
	g_free (sl);
}
