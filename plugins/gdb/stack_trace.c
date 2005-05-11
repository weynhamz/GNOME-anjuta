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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <gnome.h>

#include <libanjuta/resources.h>

#include "debugger.h"
#include "utilities.h"
#include "info.h"
#include "stack_trace.h"

#define ANJUTA_PIXMAP_POINTER PACKAGE_PIXMAPS_DIR"/pointer.png"

typedef struct _StackTraceGui StackTraceGui;
struct _StackTraceGui
{
	GtkWidget *scrolledwindow;
	GtkWidget *clist;
	GtkWidget *menu;
	GtkWidget *menu_set;
	GtkWidget *menu_info;
	GtkWidget *menu_update;
	GtkWidget *menu_view;
};

struct _StackTrace
{
	Debugger *debugger;
	StackTraceGui widgets;
	gint current_frame;
};

enum {
	STACK_TRACE_ACTIVE_COLUMN,
	STACK_TRACE_FRAME_COLUMN,
	STACK_TRACE_FILE_COLUMN,
	STACK_TRACE_LINE_COLUMN,
	STACK_TRACE_FUNC_COLUMN,
	STACK_TRACE_ADDR_COLUMN,
	STACK_TRACE_ARGS_COLUMN,
	STACK_TRACE_N_COLUMNS
};

/*
 * returns the current stack frame or -1 on error
 */
static gint
get_index_from_iter (StackTrace* st, GtkTreeIter* iter)
{
	GtkTreeModel *model;		
	gchar* count = NULL;
	gint frame_no;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (st->widgets.clist));		

	gtk_tree_model_get (model, iter, STACK_TRACE_FRAME_COLUMN, &count, -1);
	frame_no = atoi(count);
	g_free (count);
	return frame_no;
}

static gboolean 
get_current_iter (StackTrace* st, GtkTreeIter* iter)
{
	GtkTreeView *view;
	GtkTreeSelection *selection;
	
	view = GTK_TREE_VIEW (st->widgets.clist);
	selection = gtk_tree_view_get_selection (view);

	if (!gtk_tree_selection_get_selected (selection, NULL, iter))
		return FALSE;
	return TRUE;
}

static void
set_func_args (const GDBMIValue *frame_hash, StackTrace * st)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	const gchar *level, *args;
	const GDBMIValue *literal, *args_list, *arg_hash;
	gint frame, i;
	GString *args_str;
	
	level = args = "";
	
	literal = gdbmi_value_hash_lookup (frame_hash, "level");
	if (!literal)
		return;
	
	level = gdbmi_value_literal_get (literal);
	if (!level)
		return;
	
	frame = atoi(level);
	
	args_list = gdbmi_value_hash_lookup (frame_hash, "args");
	if (!args_list)
		return;
	
	args_str = g_string_new ("(");
	for (i = 0; i < gdbmi_value_get_size (args_list); i++)
	{
		const gchar *name, *value;
		
		arg_hash = gdbmi_value_list_get_nth (args_list, i);
		if (!arg_hash)
			continue;
		
		literal = gdbmi_value_hash_lookup (arg_hash, "name");
		if (!literal)
			continue;
		name = gdbmi_value_literal_get (literal);
		if (!name)
			continue;
		
		literal = gdbmi_value_hash_lookup (arg_hash, "value");
		if (!literal)
			continue;
		value = gdbmi_value_literal_get (literal);
		if (!value)
			continue;
		args_str = g_string_append (args_str, name);
		args_str = g_string_append (args_str, "=");
		args_str = g_string_append (args_str, value);
		if (i < (gdbmi_value_get_size (args_list) - 1))
			args_str = g_string_append (args_str, ", ");
	}
	args_str = g_string_append (args_str, ")");
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (st->widgets.clist));
	if (gtk_tree_model_iter_nth_child (model, &iter, NULL, frame))
	{
		gtk_list_store_set(GTK_LIST_STORE(model), &iter, 
						   STACK_TRACE_ARGS_COLUMN, args_str->str,
						   -1);
	}
	g_string_free (args_str, TRUE);
}

static void
stack_trace_update_func_args (Debugger *debugger, const GDBMIValue *mi_results,
							  const GList *cli_results, gpointer data)
{
	StackTrace *st;
	const GDBMIValue *stack_list;
	
	st = (StackTrace *) data;

	if (!mi_results)
		return;
	
	stack_list = gdbmi_value_hash_lookup (mi_results, "stack-args");
	if (stack_list)
		gdbmi_value_foreach (stack_list, (GFunc)set_func_args, st);
}

static void
add_frame (const GDBMIValue *frame_hash, StackTrace * st)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	const gchar *level, *file, *line, *func, *addr, *args;
	const GDBMIValue *literal;
	gint frame;
	GdkPixbuf *pic;
	
	level = file = line = func = addr = args = "";
	
	literal = gdbmi_value_hash_lookup (frame_hash, "level");
	if (literal)
		level = gdbmi_value_literal_get (literal);
	
	literal = gdbmi_value_hash_lookup (frame_hash, "file");
	if (literal)
		file = gdbmi_value_literal_get (literal);
	
	literal = gdbmi_value_hash_lookup (frame_hash, "line");
	if (literal)
		line = gdbmi_value_literal_get (literal);
	
	literal = gdbmi_value_hash_lookup (frame_hash, "func");
	if (literal)
		func = gdbmi_value_literal_get (literal);
	
	literal = gdbmi_value_hash_lookup (frame_hash, "addr");
	if (literal)
		addr = gdbmi_value_literal_get (literal);
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (st->widgets.clist));	

	gtk_list_store_append (GTK_LIST_STORE (model), &iter);

	frame = atoi(level);

	/* if we are on the current frame set iterator and pixmap correctly */
	if (frame == st->current_frame)
		pic = gdk_pixbuf_new_from_file (ANJUTA_PIXMAP_POINTER, NULL);
	else
		pic = NULL;

	gtk_list_store_set(GTK_LIST_STORE(model), &iter, 
					   STACK_TRACE_ACTIVE_COLUMN, pic,
					   STACK_TRACE_FRAME_COLUMN, level, 
					   STACK_TRACE_FILE_COLUMN, file,
					   STACK_TRACE_LINE_COLUMN, line,
					   STACK_TRACE_FUNC_COLUMN, func,
					   STACK_TRACE_ADDR_COLUMN, addr,
					   STACK_TRACE_ARGS_COLUMN, args,
					   -1);
	if (pic)
		gdk_pixbuf_unref (pic);
}

static void
stack_trace_update (Debugger *debugger, const GDBMIValue *mi_results,
					const GList *cli_results, gpointer data)
{
	StackTrace *st;
	const GDBMIValue *stack_list;
	
	st = (StackTrace *) data;
	stack_trace_clear (st);

	if (!mi_results)
		return;
	
	stack_list = gdbmi_value_hash_lookup (mi_results, "stack");
	if (stack_list)
		gdbmi_value_foreach (stack_list, (GFunc)add_frame, st);
}

static void
on_debugger_dialog_message (Debugger *debugger, const GDBMIValue *mi_result,
							const GList *cli_result, gpointer user_data)
{
	if (g_list_length ((GList*)cli_result) < 1)
		return;
	gdb_info_show_list (user_data, cli_result, 0, 0);
}

static void 
stack_trace_set_active_cbs (Debugger *debugger, const GDBMIValue *mi_results,
							const GList * outputs, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;	
	GdkPixbuf *pointer_pix;
	const GDBMIValue *frame, *literal;
	const gchar *level_str;
	StackTrace *st = (StackTrace*) data;
	
	if (mi_results == NULL)
		return;
	
	frame = gdbmi_value_hash_lookup (mi_results, "frame");
	if (frame == NULL)
		return;
	
	literal = gdbmi_value_hash_lookup (frame, "level");
	if (literal == NULL)
		return;
	
	level_str = gdbmi_value_literal_get (literal);
	if (level_str == NULL || strlen (level_str) <= 0)
		return;
	st->current_frame = atoi (level_str);
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (st->widgets.clist));
	
	/* Clear old pointer */
	if(gtk_tree_model_get_iter_first (model, &iter))
	{
		do {
			/* clear pixmap on the previous active frame */
			gtk_list_store_set (GTK_LIST_STORE (model), &iter,
								STACK_TRACE_ACTIVE_COLUMN, NULL, -1);
		} while (gtk_tree_model_iter_next (model, &iter));
	}
	
	/* Set pointer to current frame */
	if (gtk_tree_model_iter_nth_child (model, &iter, NULL,
									   st->current_frame))
	{
		pointer_pix = gdk_pixbuf_new_from_file (ANJUTA_PIXMAP_POINTER, NULL);

		/* set pointer on this frame */
		gtk_list_store_set (GTK_LIST_STORE(model), &iter,
							STACK_TRACE_ACTIVE_COLUMN, pointer_pix,
							-1);
		gdk_pixbuf_unref (pointer_pix);
	}
}

static void
stack_trace_update_controls (StackTrace * st)
{
	gboolean R;

	R = debugger_is_ready (st->debugger);

	gtk_widget_set_sensitive (st->widgets.menu_set, R);
	gtk_widget_set_sensitive (st->widgets.menu_info, R);
	gtk_widget_set_sensitive (st->widgets.menu_update, R);
	gtk_widget_set_sensitive (st->widgets.menu_view, R);
}

static gboolean
on_stack_trace_event (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
	StackTrace *st = (StackTrace*) user_data;

	if (event->type == GDK_BUTTON_PRESS)
	{
		GdkEventButton *bevent;
		bevent = (GdkEventButton *) event;
		if (bevent->button != 3)
			return FALSE;
		bevent->button = 1;
		stack_trace_update_controls (st);
		gtk_menu_popup (GTK_MENU (st->widgets.menu), NULL, NULL, NULL, NULL,
						bevent->button, bevent->time);
		return TRUE;
	}
	else
		return FALSE;
}

static void
on_stack_frame_set_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	StackTrace *st;
	gchar* cmd;
	GtkTreeIter iter;
	gint selected_frame;
	
	st = (StackTrace*) user_data;
	
	if (!get_current_iter (st, &iter))
		return;
	
	selected_frame = get_index_from_iter (st, &iter);
	
	/* current frame is already active */
	if (selected_frame == st->current_frame)
		return;
	
	/* issue a command to switch active frame to new location */
	cmd = g_strdup_printf ("frame %d", selected_frame);

	debugger_command (st->debugger, cmd, FALSE,
					  stack_trace_set_active_cbs, st);
	g_free (cmd);
}

static void
on_stack_frame_info_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	StackTrace *st;	
	GtkTreeIter iter;
	gint selected_frame;
	gchar *cmd;
	
	st = (StackTrace*) user_data;
	
	if (!get_current_iter (st, &iter))
		return;
	selected_frame = get_index_from_iter (st, &iter);
	
	cmd = g_strdup_printf ("info frame %d", selected_frame);
	debugger_command (st->debugger, cmd, FALSE,
					  on_debugger_dialog_message, NULL);
	g_free (cmd);
}

static void
on_stack_update_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	StackTrace* st = (StackTrace*) user_data;
	debugger_command (st->debugger, "-stack-list-frames", FALSE,
					  stack_trace_update, st);
	debugger_command (st->debugger, "-stack-list-arguments 1", FALSE,
					  stack_trace_update_func_args, st);
}

static void
on_stack_view_src_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	GtkTreeView *view;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gchar *file, *line, *addr;
	
	StackTrace* st = (StackTrace*) user_data;		

	view = GTK_TREE_VIEW (st->widgets.clist);
	model = gtk_tree_view_get_model (view);
	selection = gtk_tree_view_get_selection (view);

	if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
		return;

	/* get the frame info */
	gtk_tree_model_get (model, &iter,
						STACK_TRACE_FILE_COLUMN, &file,
						STACK_TRACE_LINE_COLUMN, &line,
						STACK_TRACE_ADDR_COLUMN, &addr,
						-1);
	
	debugger_change_location (st->debugger, file, atoi (line), addr);
	g_free (file);
	g_free (line);
	g_free (addr);
}

/* Context pop up menu */
static GnomeUIInfo stack_menu_uiinfo[] =
{
  {
    GNOME_APP_UI_ITEM, N_("Set current frame"),
    NULL,
    on_stack_frame_set_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  {
    GNOME_APP_UI_ITEM, N_("Frame info"),
    NULL,
    on_stack_frame_info_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  {
    GNOME_APP_UI_ITEM, N_("Update"),
    NULL,
    on_stack_update_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  {
    GNOME_APP_UI_ITEM, N_("View Source"),
    NULL,
    on_stack_view_src_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  GNOMEUIINFO_END
};

static GtkWidget*
create_stack_menu (StackTrace *st)
{
	GtkWidget *menu4;
	int i;
	int entries = sizeof (stack_menu_uiinfo) / sizeof (GnomeUIInfo);
	
	/* Set all user data in the stack trace menu to the StackTrace
	 * struct parameter
	 */
	for (i = 0; i < entries; i++)
		stack_menu_uiinfo[i].user_data = st;
	
	menu4 = gtk_menu_new ();
	gnome_app_fill_menu (GTK_MENU_SHELL (menu4), stack_menu_uiinfo,
						NULL, FALSE, 0);
	return menu4;
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
create_stack_trace_gui(StackTrace *st)
{
	GtkTreeModel* model;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;	
	
	model = GTK_TREE_MODEL(gtk_list_store_new (STACK_TRACE_N_COLUMNS,
											   GDK_TYPE_PIXBUF,
											   G_TYPE_STRING,
											   G_TYPE_STRING,
											   G_TYPE_STRING,
											   G_TYPE_STRING,
											   G_TYPE_STRING,
											   G_TYPE_STRING));
	st->widgets.scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (st->widgets.scrolledwindow);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (st->widgets.scrolledwindow),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (st->widgets.scrolledwindow),
										 GTK_SHADOW_IN);
	
	st->widgets.clist = gtk_tree_view_new_with_model (model);
	gtk_widget_show (st->widgets.clist);
	gtk_container_add (GTK_CONTAINER (st->widgets.scrolledwindow),
					   st->widgets.clist);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (st->widgets.clist));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	g_object_unref (G_OBJECT (model));
	
	/* Columns */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Active"));
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "pixbuf",
										STACK_TRACE_ACTIVE_COLUMN);
	gtk_tree_view_append_column (GTK_TREE_VIEW (st->widgets.clist), column);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (st->widgets.clist),
									   column);
	
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Frame"));
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
										STACK_TRACE_FRAME_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (st->widgets.clist), column);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (st->widgets.clist),
									   column);
	
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
										STACK_TRACE_FILE_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("File"));
	gtk_tree_view_append_column (GTK_TREE_VIEW (st->widgets.clist), column);

	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
										STACK_TRACE_LINE_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Line"));
	gtk_tree_view_append_column (GTK_TREE_VIEW (st->widgets.clist), column);
	
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
										STACK_TRACE_FUNC_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Function"));
	gtk_tree_view_append_column (GTK_TREE_VIEW (st->widgets.clist), column);
	
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
										STACK_TRACE_ADDR_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Address"));
	gtk_tree_view_append_column (GTK_TREE_VIEW (st->widgets.clist), column);
	
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
										STACK_TRACE_ARGS_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Arguments"));
	gtk_tree_view_append_column (GTK_TREE_VIEW (st->widgets.clist), column);
	
	g_signal_connect (st->widgets.clist, "event",
					  G_CALLBACK (on_stack_trace_event), st);  
	g_signal_connect (st->widgets.clist, "row_activated",
					  G_CALLBACK (on_stack_trace_row_activated), st);  
	
	st->widgets.menu = create_stack_menu(st);
	st->widgets.menu_set = stack_menu_uiinfo[0].widget;
	st->widgets.menu_info = stack_menu_uiinfo[1].widget; 
	st->widgets.menu_update = stack_menu_uiinfo[2].widget;
	st->widgets.menu_view = stack_menu_uiinfo[3].widget;
}

static void
on_program_stopped (Debugger *debugger, GDBMIValue *mi_results,
					StackTrace *stack_trace)
{
	stack_trace->current_frame = 0;
	debugger_command (debugger, "-stack-list-frames", TRUE,
					  stack_trace_update, stack_trace);
	debugger_command (debugger, "-stack-list-arguments 1", TRUE,
					  stack_trace_update_func_args, stack_trace);
}

static void
on_results_arrived (Debugger *debugger, const gchar *command,
					const GDBMIValue *mi_results, StackTrace *stack_trace)
{
	stack_trace_set_active_cbs (debugger, mi_results, NULL, stack_trace);
}

StackTrace *
stack_trace_new (Debugger *debugger)
{
	StackTrace *st;

	st = g_malloc (sizeof (StackTrace));
	if (st)
	{
		create_stack_trace_gui (st);
		st->current_frame = 0;
		st->debugger = debugger;
		
		g_object_ref (debugger);
		
		g_signal_connect (debugger, "program-stopped",
						  G_CALLBACK (on_program_stopped), st);
		g_signal_connect_swapped (debugger, "program-exited",
								  G_CALLBACK (stack_trace_clear), st);
		g_signal_connect (debugger, "results-arrived",
						  G_CALLBACK (on_results_arrived), st);
	}
	return st;
}

void
stack_trace_clear (StackTrace * st)
{
	GtkTreeModel* model;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (st->widgets.clist));
	gtk_list_store_clear (GTK_LIST_STORE (model));
}

void
stack_trace_destroy (StackTrace * st)
{
	g_return_if_fail (st != NULL);
	
	g_signal_handlers_disconnect_by_func (st->debugger,
										  G_CALLBACK (on_program_stopped),
										  st);
	g_signal_handlers_disconnect_by_func (st->debugger,
										  G_CALLBACK (stack_trace_update),
										  st);
	g_signal_handlers_disconnect_by_func (st->debugger,
										  G_CALLBACK (on_results_arrived),
										  st);
	g_object_unref (st->debugger);
	
	gtk_widget_destroy (st->widgets.menu);
	gtk_widget_destroy (st->widgets.scrolledwindow);
	g_free (st);
}

void
stack_trace_set_frame (StackTrace *st, gint frame)
{
	gchar *cmd;
	
	/* issue a command to switch active frame to new location */
	cmd = g_strdup_printf ("frame %d", frame);
	debugger_command (st->debugger, cmd, FALSE,
					  stack_trace_set_active_cbs, st);
	g_free (cmd);
}

GtkWidget*
stack_trace_get_main_widget (StackTrace *st)
{
	return st->widgets.scrolledwindow;
}
