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
#include "resources.h"
#include "debugger.h"
#include "messagebox.h"
#include "utilities.h"

/* Including pixmaps at compile time */
/* So that these at least work when gnome pixmaps are not found */
#include "../pixmaps/pointer.xpm"
#include "../pixmaps/blank.xpm"

GdkPixmap *pointer_pix;
GdkBitmap *pointer_pix_mask;

GdkPixmap *blank_pix;
GdkBitmap *blank_pix_mask;

StackTrace *
stack_trace_new ()
{
	StackTrace *st;

	st = g_malloc (sizeof (StackTrace));
	if (st)
	{
		create_stack_trace_gui (st);
		st->current_index = -1;
		st->current_frame = 0;
		st->is_showing = FALSE;
		st->is_docked = FALSE;
		st->win_pos_x = 30;
		st->win_pos_y = 200;
		st->win_width = 400;
		st->win_height = 150;

		pointer_pix =
			gdk_pixmap_colormap_create_from_xpm_d(NULL,
				gtk_widget_get_colormap(st->widgets.window),
				&pointer_pix_mask, NULL, pointer_xpm);
		blank_pix =
			gdk_pixmap_colormap_create_from_xpm_d(NULL,
				gtk_widget_get_colormap(st->widgets.window),
				&blank_pix_mask, NULL, blank_xpm);
	}
	return st;
}

void
stack_trace_clear (StackTrace * st)
{
	gtk_clist_unselect_all (GTK_CLIST (st->widgets.clist));
	gtk_clist_clear (GTK_CLIST (st->widgets.clist));
	st->current_index = -1;
}

void
stack_trace_update (GList * outputs, gpointer data)
{
	StackTrace *st;
	gchar *ptr;
	gfloat adj_value;
	GtkAdjustment *adj;
	GList *list, *node;

	st = (StackTrace *) data;
	list = remove_blank_lines (outputs);
	adj = gtk_clist_get_vadjustment (GTK_CLIST (st->widgets.clist));
	if (st->current_frame > 0)
		adj_value = adj->value;
	else
		adj_value = 0;

	stack_trace_clear (debugger.stack);
	if (g_list_length (list) < 1)
	{
		g_list_free (list);
		return;
	}
	node = list->next;
	ptr = g_strdup ((gchar *) list->data);
	while (node)
	{
		gchar *line = (gchar *) node->data;
		node = g_list_next (node);
		if (isspace (line[0]))	/* line break */
		{
			gchar *tmp;
			tmp = ptr;
			ptr = g_strconcat (tmp, line, NULL);
			g_free (tmp);
		}
		else
		{
			add_frame (st, ptr);
			g_free (ptr);
			ptr = g_strdup (line);
		}
	}
	if (ptr)
	{
		add_frame (st, ptr);
		g_free (ptr);
		ptr = NULL;
	}
	gtk_adjustment_set_value (adj, adj_value);
	g_list_free (list);
}

void
add_frame (StackTrace * st, gchar * line)
{
	gchar frame[10], *dummy_fn;
	gchar *row[3];
	gint count, last_row, dummy_int;
	GdkColor blue = { 16, 0, 0, -1 };
	GdkColor red = { 16, -1, 0, 0 };

	count = sscanf (line, "#%s", frame);
	if (count != 1)
		return;
	while (!isspace (*line))
		line++;
	while (isspace (*line))
		line++;
	row[0] = NULL;
	row[1] = frame;
	row[2] = line;
	gtk_clist_append (GTK_CLIST (st->widgets.clist), row);
	last_row =
		g_list_length (GTK_CLIST (st->widgets.clist)->row_list) - 1;
	if (parse_error_line (line, &dummy_fn, &dummy_int))
	{
		gtk_clist_set_foreground (GTK_CLIST (st->widgets.clist),
					  last_row, &red);
		g_free (dummy_fn);
	}
	else
	{
		gtk_clist_set_foreground (GTK_CLIST (st->widgets.clist),
					  last_row, &blue);
	}
	if (last_row == st->current_frame)
	{
		gtk_clist_set_pixmap (GTK_CLIST (st->widgets.clist), last_row,
				      0, pointer_pix, pointer_pix_mask);
	}
}

void
stack_trace_show (StackTrace * st)
{
	if (st)
	{
		if (st->is_showing)
		{
			if (st->is_docked == FALSE)
				gdk_window_raise (st->widgets.window->window);
			return;
		}
		if (st->is_docked)
		{
			stack_trace_attach (st);
		}
		else		/* Is not docked */
		{
			gtk_widget_set_uposition (st->widgets.window,
						  st->win_pos_x,
						  st->win_pos_y);
			gtk_window_set_default_size (GTK_WINDOW
						     (st->widgets.window),
						     st->win_width,
						     st->win_height);
			gtk_widget_show (st->widgets.window);
		}
		st->is_showing = TRUE;
	}
}

void
stack_trace_hide (StackTrace * st)
{
	if (st)
	{
		if (st->is_showing == FALSE)
			return;
		if (st->is_docked == TRUE)
		{
			stack_trace_detach (st);
		}
		else		/* Is not docked */
		{
			gdk_window_get_root_origin (st->widgets.window->
						    window, &st->win_pos_x,
						    &st->win_pos_y);
			gdk_window_get_size (st->widgets.window->window,
					     &st->win_width, &st->win_height);
			gtk_widget_hide (st->widgets.window);
		}
		st->is_showing = FALSE;
	}
}

void
stack_trace_attach (StackTrace * st)
{

}

void
stack_trace_detach (StackTrace * st)
{

}

void
stack_trace_dock (StackTrace * st)
{

}

void
stack_trace_undock (StackTrace * st)
{

}

gboolean
stack_trace_save_yourself (StackTrace * st, FILE * stream)
{
	if (!st)
		return FALSE;

	fprintf (stream, "stack.trace.is.docked=%d\n", st->is_docked);
	if (st->is_showing && !st->is_docked)
	{
		gdk_window_get_root_origin (st->widgets.window->window,
					    &st->win_pos_x, &st->win_pos_y);
		gdk_window_get_size (st->widgets.window->window,
				     &st->win_width, &st->win_height);
	}
	fprintf (stream, "stack.trace.win.pos.x=%d\n", st->win_pos_x);
	fprintf (stream, "stack.trace.win.pos.y=%d\n", st->win_pos_y);
	fprintf (stream, "stack.trace.win.width=%d\n", st->win_width);
	fprintf (stream, "stack.trace.win.height=%d\n", st->win_height);
	return TRUE;
}

gboolean
stack_trace_load_yourself (StackTrace * st, PropsID props)
{
	gboolean dock_flag;
	if (!st)
		return FALSE;

	dock_flag = prop_get_int (props, "stack.trace.is.docked", 0);
	st->win_pos_x = prop_get_int (props, "stack.trace.win.pos.x", 30);
	st->win_pos_y = prop_get_int (props, "stack.trace.win.pos.y", 200);
	st->win_width = prop_get_int (props, "stack.trace.win.width", 400);
	st->win_height = prop_get_int (props, "stack.trace.win.height", 150);
	if (dock_flag)
		stack_trace_dock (st);
	else
		stack_trace_undock (st);
	return TRUE;
}

void
stack_trace_destroy (StackTrace * st)
{
	if (st)
	{
		stack_trace_clear (st);
		gtk_widget_unref (st->widgets.window);
		gtk_widget_unref (st->widgets.clist);
		gtk_widget_unref (st->widgets.menu);
		gtk_widget_unref (st->widgets.menu_set);
		gtk_widget_unref (st->widgets.menu_info);
		gtk_widget_unref (st->widgets.menu_update);
		gtk_widget_unref (st->widgets.menu_view);

		if (GTK_IS_WIDGET (st->widgets.window))
			gtk_widget_destroy (st->widgets.window);
		g_free (st);
	}
}

void
stack_trace_update_controls (StackTrace * st)
{
	gboolean A, R, S;

	A = debugger_is_active ();
	R = debugger_is_ready ();
	S = (st->current_index >= 0);
	gtk_widget_set_sensitive (st->widgets.menu_set, A && R && S);
	gtk_widget_set_sensitive (st->widgets.menu_info, A && R);
	gtk_widget_set_sensitive (st->widgets.menu_update, A && R);
	gtk_widget_set_sensitive (st->widgets.menu_view, A && R && S);
}
