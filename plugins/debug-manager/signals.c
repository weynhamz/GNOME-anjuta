/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    signals.c
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
#include <ctype.h>

#include <libanjuta/anjuta-shell.h>
#include <libanjuta/resources.h>

#include "utilities.h"
#include "signals.h"


enum _SignalColums
{
	SIGNAL_COLUMN_NAME,
	SIGNAL_COLUMN_STOP,
	SIGNAL_COLUMN_PRINT,
	SIGNAL_COLUMN_PASS,
	SIGNAL_COLUMN_DESCRIPTION,
	SIGNAL_COLUMN_COUNT
};

static void
signals_update_controls (Signals * ew)
{
	gboolean R, Pr;
	
	R = dma_debugger_queue_get_state (ew->debugger) == IANJUTA_DEBUGGER_PROGRAM_STOPPED;
	Pr = dma_debugger_queue_get_state (ew->debugger) == IANJUTA_DEBUGGER_PROGRAM_RUNNING;
	gtk_action_group_set_sensitive(ew->action_group_program_stopped, R);
	gtk_action_group_set_sensitive(ew->action_group_program_running, Pr);
}

/*
 * signals_update:
 */
static void
signals_update (const GList * lines, gpointer data)
{
	Signals *sg;
	gint j, count;
	gchar *str;
	gchar sig[32], stop[10], print[10], pass[10];
	GList *list, *node;
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	sg = (Signals *) data;
	signals_clear (sg);
	list = gdb_util_remove_blank_lines (lines);
	if (g_list_length (list) < 2)
	{
		g_list_free (list);
		return;
	}

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (sg->widgets.treeview));
	gtk_tree_view_set_model (GTK_TREE_VIEW (sg->widgets.treeview), NULL);
	node = list->next;

	/* Skip the first two lines */
	if (node)
		node = node->next;
	if (node)
		node = node->next;
	while (node)
	{
		count =
			sscanf ((char *) node->data, "~%s %s %s %s", sig, stop, print, pass);
		str = node->data;
		node = g_list_next (node);
		if (count != 4)
			continue;

		/* Do not worry. This is used to avoid the last line */
		if (node == NULL)
			break;
		for (j = 0; j < 4; j++)
		{
			while (isspace (*str))
				str++;
			while (!isspace (*str))
				str++;
		}
		while (isspace (*str))
			str++;

		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				SIGNAL_COLUMN_NAME, sig,
				SIGNAL_COLUMN_STOP, strcmp (stop, "Yes") == 0,
				SIGNAL_COLUMN_PRINT, strcmp (print, "Yes") == 0,
				SIGNAL_COLUMN_PASS, strcmp (pass, "Yes") == 0,
				SIGNAL_COLUMN_DESCRIPTION, str,
				-1);
	}

	/* FIXME: do we need to free the char data as well? */
	g_list_free (list);
	gtk_tree_view_set_model (GTK_TREE_VIEW (sg->widgets.treeview), model);
}

static gint
on_signals_delete_event(GtkWidget* w, GdkEvent *event, gpointer data)
{
	Signals* cr = (Signals*) data;
	signals_hide (cr);
	return TRUE;
}

static gboolean
on_signals_key_press_event(GtkWidget *widget, GdkEventKey *event,
						   gpointer data)
{
	if (event->keyval == GDK_Escape)
	{
		Signals* cr = (Signals*) data;
  		signals_hide(cr);
		return TRUE;
	}
	return FALSE;
}

static void
on_signals_send_activate (GtkMenuItem *menuitem, gpointer user_data)
{
	Signals *s = (Signals*)user_data;
#if 0
	gchar* sig;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
#endif 
	if (dma_debugger_queue_get_state (s->debugger) != IANJUTA_DEBUGGER_PROGRAM_RUNNING)
		return;
	/* has FIXME in debugger.c in gdb plugin */
#if 0
	
	signals_show (s);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (s->widgets.treeview));
	gtk_tree_selection_get_selected (selection, &model, &iter);
	gtk_tree_model_get (model, &iter, 
			SIGNAL_COLUMN_NAME, &sig,
			-1);
	debugger_signal(sig, TRUE);
	g_free (sig);
#endif
}

static void
on_signals_update_activate (GtkMenuItem *menuitem, gpointer user_data)
{
    Signals *s = (Signals*)user_data;
	dma_queue_info_signal (
			s->debugger,
			(IAnjutaDebuggerCallback)signals_update,
			s);
}

/*
 * Show popup menu on #GtkTreeView
 */
static gboolean
on_signals_event (GtkWidget *widget, GdkEventButton  *bevent, gpointer user_data)
{
    Signals *ew = (Signals*)user_data;
	
	if (bevent->type != GDK_BUTTON_PRESS) return FALSE;
	if (bevent->button != 3) return FALSE;
	
	signals_update_controls(ew);
	gtk_menu_popup (GTK_MENU(ew->widgets.menu), NULL,
					NULL, NULL, NULL,
					bevent->button, bevent->time);
	
	return TRUE;
}

/*
 * on_column_toggled:
 *
 * @renderer: cell renderer on which the toggle happened
 * @path: row in list where the change happened
 * @sig: instance of #Signals
 *
 * Callback if one of the three boolean columns has been toggled by the 
 * user. If the debugged program is not stopped this function will
 * do nothing. Function will fetch state of signal from list store, update the 
 * affected column and pass on the data to the debugging backend using
 * dma_queue_handler_signal().
 */
static void
on_column_toggled(GtkCellRendererToggle *renderer, gchar *path, Signals *sg)
{
	GtkTreeIter iter;
	gchar *signal;
	gboolean data[SIGNAL_COLUMN_COUNT];
	guint column;

	if (dma_debugger_queue_get_state (sg->debugger) != IANJUTA_DEBUGGER_PROGRAM_STOPPED)
	{
		return;
	}

	column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (renderer), "__column_nr"));
	
	gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (sg->widgets.store),
			&iter,
			path);
	gtk_tree_model_get (GTK_TREE_MODEL (sg->widgets.store),
			&iter,
			SIGNAL_COLUMN_NAME, &signal,
			SIGNAL_COLUMN_STOP, &data[SIGNAL_COLUMN_STOP],
			SIGNAL_COLUMN_PRINT, &data[SIGNAL_COLUMN_PRINT],
			SIGNAL_COLUMN_PASS, &data[SIGNAL_COLUMN_PASS],
			-1);

	data[column] = !data[column];

	gtk_list_store_set (sg->widgets.store,
			&iter,
			column, data[column], -1);
	dma_queue_handle_signal (sg->debugger, signal, 
			data[SIGNAL_COLUMN_STOP],
			data[SIGNAL_COLUMN_PRINT],
			data[SIGNAL_COLUMN_PASS]);
	g_free (signal);
}

static void
signals_add_toggle_column (GtkTreeView *tv, const gchar *title, guint column_num, Signals *sg)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_toggle_new ();
	column = gtk_tree_view_column_new_with_attributes (title,
			renderer, 
			"active", column_num,
			NULL);
	gtk_tree_view_append_column (tv, column);
	g_object_set_data (G_OBJECT (renderer), "__column_nr", GINT_TO_POINTER (column_num));
	g_signal_connect (G_OBJECT (renderer), 
			"toggled", G_CALLBACK (on_column_toggled), sg);
}

static GtkWidget *
signals_create_list_store_and_treeview(Signals *sg)
{
	GtkListStore *store;
	GtkWidget *w;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	store = gtk_list_store_new (SIGNAL_COLUMN_COUNT, 
								G_TYPE_STRING,
								G_TYPE_BOOLEAN,
								G_TYPE_BOOLEAN,
								G_TYPE_BOOLEAN,
								G_TYPE_STRING
								);
	
	w = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
	
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Signal"),
			renderer, 
			"text", SIGNAL_COLUMN_NAME,
			NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (w), column);

	signals_add_toggle_column (GTK_TREE_VIEW (w), _("Stop"), SIGNAL_COLUMN_STOP, sg);
	signals_add_toggle_column (GTK_TREE_VIEW (w), _("Print"), SIGNAL_COLUMN_PRINT, sg);
	signals_add_toggle_column (GTK_TREE_VIEW (w), _("Pass"), SIGNAL_COLUMN_PASS, sg);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Description"),
			renderer, 
			"text", SIGNAL_COLUMN_DESCRIPTION,
			NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (w), column);

	g_signal_connect (G_OBJECT (w), "button-press-event", 
			G_CALLBACK (on_signals_event), sg);

	return w;
}

/* Actions table
 *---------------------------------------------------------------------------*/

static GtkActionEntry actions_signals_program_running[] = {
	{
		"ActionDmaSignalSend",         	/* Action name */
		NULL,                                 	/* Stock icon, if any */
		N_("Send to process"),               /* Display label */
		NULL,                                   /* short-cut */
		NULL,                                   /* Tooltip */
		G_CALLBACK (on_signals_send_activate) 	/* action callback */
	}
};

static GtkActionEntry actions_signals_program_stopped[] = {
	{
		"ActionDmaSignalUpdate",
		GTK_STOCK_REFRESH,
		N_("Update"),
		NULL,
		NULL,
		G_CALLBACK (on_signals_update_activate)
	}
};

static void
destroy_signals_gui (Signals *sg)
{

}

static void
create_signals_gui (Signals *sg)
{
	GtkWidget *window3;
	GtkWidget *scrolledwindow4;
	GtkWidget *tv;
	AnjutaUI *ui;	

	window3 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_size_request (window3, 170, -1);
	gtk_window_set_title (GTK_WINDOW (window3), _("Kernel signals"));
	gtk_window_set_wmclass (GTK_WINDOW (window3), "signals", "Anjuta");
	gtk_window_set_default_size (GTK_WINDOW (window3), 240, 230);

	scrolledwindow4 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow4);
	gtk_container_add (GTK_CONTAINER (window3), scrolledwindow4);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow4),
									GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	
	tv = signals_create_list_store_and_treeview (sg);
	gtk_widget_show (tv);
	gtk_container_add (GTK_CONTAINER (scrolledwindow4), tv);
	gtk_signal_connect (GTK_OBJECT (window3), "delete_event",
						GTK_SIGNAL_FUNC (on_signals_delete_event),sg);
	gtk_signal_connect (GTK_OBJECT (window3), "key-press-event",
						GTK_SIGNAL_FUNC (on_signals_key_press_event), sg);
	
	sg->widgets.window = window3;
	sg->widgets.treeview = tv;
	sg->widgets.store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (tv)));
	ui = anjuta_shell_get_ui (sg->plugin->shell, NULL);
	sg->widgets.menu = gtk_ui_manager_get_widget (GTK_UI_MANAGER (ui),
				"/PopupSignals");
	
	sg->is_showing = FALSE;
	sg->win_width = 460;
	sg->win_height = 320;
	sg->win_pos_x = 150;
	sg->win_pos_y = 130;
}

static void
on_program_exited (Signals * sg)
{
	g_signal_handlers_disconnect_by_func (sg->plugin, G_CALLBACK (on_program_exited), sg);

	destroy_signals_gui(sg);
}

static void
on_program_started (Signals * sg)
{
	create_signals_gui(sg);
	
	g_signal_connect_swapped (sg->plugin, "program-exited", G_CALLBACK (on_program_exited), sg);
}

/* Public functions
 *---------------------------------------------------------------------------*/

void
signals_clear (Signals * sg)
{
	g_return_if_fail (sg->widgets.store != NULL);
	g_return_if_fail (GTK_IS_LIST_STORE(sg->widgets.store));

	gtk_list_store_clear (sg->widgets.store);
}

void
signals_show (Signals * sg)
{
	if (sg)
	{
		if (sg->is_showing)
		{
			gdk_window_raise (sg->widgets.window->window);
		}
		else
		{
			gtk_widget_set_uposition (sg->widgets.window, sg->win_pos_x,
									  sg->win_pos_y);
			gtk_window_set_default_size (GTK_WINDOW (sg->widgets.window),
										 sg->win_width, sg->win_height);
			gtk_widget_show (sg->widgets.window);
			sg->is_showing = TRUE;
			dma_queue_info_signal (
					sg->debugger,
					(IAnjutaDebuggerCallback)signals_update,
					sg);
		}
	}
}

void
signals_hide (Signals * sg)
{
	if (sg)
	{
		if (sg->is_showing == FALSE)
			return;
		gdk_window_get_root_origin (sg->widgets.window->window, &sg->win_pos_x,
									&sg->win_pos_y);
		gdk_window_get_size (sg->widgets.window->window, &sg->win_width,
							 &sg->win_height);
		gtk_widget_hide (sg->widgets.window);
		sg->is_showing = FALSE;
	}
}

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

Signals *
signals_new (DebugManagerPlugin *plugin)
{
	Signals *sg;
	AnjutaUI *ui;
	
	sg = g_new0 (Signals, 1);
	g_return_val_if_fail (sg != NULL, NULL);
	
	sg->plugin = ANJUTA_PLUGIN (plugin);
	sg->debugger = dma_debug_manager_get_queue (plugin);
	
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN(plugin)->shell, NULL);
	sg->action_group_program_stopped = 
			anjuta_ui_add_action_group_entries (ui, "ActionGroupSignalsProgramStopped",
												_("Signal operations"),
												actions_signals_program_stopped,
												G_N_ELEMENTS (actions_signals_program_stopped),
												GETTEXT_PACKAGE, TRUE, sg);
	sg->action_group_program_running = 
			anjuta_ui_add_action_group_entries (ui, "ActionGroupSignalsProgramRunning",
												_("Signal operations"),
												actions_signals_program_running,
												G_N_ELEMENTS (actions_signals_program_running),
												GETTEXT_PACKAGE, TRUE, sg);

	g_signal_connect_swapped (plugin, "program-started", G_CALLBACK (on_program_started), sg);
	
	return sg;
}

void
signals_free (Signals * sg)
{
	AnjutaUI *ui;

	g_return_if_fail (sg != NULL);
	
	/* Disconnect from debugger */
	g_signal_handlers_disconnect_matched (sg->plugin, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, sg);	

	/* Remove menu actions */
	ui = anjuta_shell_get_ui (sg->plugin->shell, NULL);
	anjuta_ui_remove_action_group (ui, sg->action_group_program_stopped);
	anjuta_ui_remove_action_group (ui, sg->action_group_program_running);
	
	/* Destroy GUI */
	destroy_signals_gui (sg);
	
	g_free (sg);
}
