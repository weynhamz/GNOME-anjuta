/* 
 * watch.c Copyright (C) 2000 Kh. Naba Kumar Singh
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along with 
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
 * Place, Suite 330, Boston, MA 02111-1307 USA */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gnome.h>
#include "resources.h"
#include "debugger.h"
#include "anjuta_info.h"
#include "utilities.h"

ExprWatch *
expr_watch_new ()
{
	ExprWatch *ew;

	ew = g_malloc (sizeof (ExprWatch));
	if (ew)
	{
		create_expr_watch_gui (ew);
		ew->exprs = NULL;
		ew->count = ew->current_index = 0;
		ew->is_showing = FALSE;
		ew->win_pos_x = 20;
		ew->win_pos_y = 325;
		ew->win_width = 400;
		ew->win_height = 150;
		ew->current_index = -1;
	}
	return ew;
}

void
expr_watch_clear (ExprWatch * ew)
{
	gint i;

	for (i = 0; i < g_list_length (ew->exprs); i++)
	{
		g_free ((gchar *) g_list_nth_data (ew->exprs, i));
	}
	g_list_free (ew->exprs);
	ew->exprs = NULL;
	gtk_clist_clear (GTK_CLIST (ew->widgets.clist));
	ew->current_index = -1;
	expr_watch_update_controls (ew);
}

void
expr_watch_cmd_queqe (ExprWatch * ew)
{
	gint i;
	gchar *buff;

	for (i = 0; i < g_list_length (ew->exprs); i++)
	{
		buff =
			g_strconcat ("print ", (gchar *) g_list_nth_data (ew->exprs, i),
						 NULL);
		debugger_put_cmd_in_queqe (buff, DB_CMD_NONE, expr_watch_update, ew);
		g_free (buff);
	}
	ew->count = 0;
}

void
expr_watch_update (GList * lines, gpointer data)
{
	ExprWatch *ew;
	gchar *ptr, *tmp;
	GdkColor *color;
	gchar not_def[] = N_("< Not defined in current context >");
	GdkColor red = { 16, -1, 0, 0 };
	GdkColor blue = { 16, 0, 0, -1 };
	GList *list, *node;

	ew = (ExprWatch *) data;
	list = remove_blank_lines (lines);
	if (g_list_length (list) < 1)
	{
		tmp = _(not_def);
		color = &red;
	}
	else
	{
		tmp = strchr ((gchar *) list->data, '=');
		color = &blue;
	}
	if (tmp == NULL)
	{
		tmp = _(not_def);
		color = &red;
	}
	ptr = g_strdup (tmp);
	if (list)
		node = list->next;
	else
		node = NULL;
	while (node)
	{
		tmp = ptr;
		ptr = g_strconcat (tmp, (gchar *) node->data, NULL);
		g_free (tmp);
		node = node->next;
	}
	gtk_clist_set_foreground (GTK_CLIST (ew->widgets.clist), ew->count, color);
	gtk_clist_set_text (GTK_CLIST (ew->widgets.clist), ew->count, 1, ptr);
	g_free (ptr);
	if (list)
		g_list_free (list);
	ew->count++;
	expr_watch_update_controls (ew);
}

void
expr_watch_update_controls (ExprWatch * ew)
{
	gboolean A, R, C, S;

	A = debugger_is_active ();
	R = debugger_is_ready ();
	C = (g_list_length (ew->exprs) > 0);
	S = (ew->current_index >= 0);
	gtk_widget_set_sensitive (ew->widgets.menu_add, A && R);
	gtk_widget_set_sensitive (ew->widgets.menu_update, A && R && C);
	gtk_widget_set_sensitive (ew->widgets.menu_remove, A && R && C && S);
	gtk_widget_set_sensitive (ew->widgets.menu_clear, A && R && C);
	gtk_widget_set_sensitive (ew->widgets.menu_toggle, FALSE);
	gtk_widget_set_sensitive (ew->widgets.menu_change, A && R && C && S);
}

void
eval_output_arrived (GList * lines, gpointer data)
{
	GList *list;

	list = lines;
	if (g_list_length (list) < 1)
		return;
	if (data)
	{
		gchar *tmp1, *tmp2, *tmp3;

		tmp1 = list->data;
		tmp2 = strchr (tmp1, '=');
		if (tmp2)
		{
			tmp3 = g_strconcat (data, " ", tmp2, NULL);
			list = g_list_remove (list, tmp1);
			if (tmp1)
				g_free (tmp1);
			list = g_list_prepend (list, tmp3);
		}
		g_free (data);
	}
	anjuta_info_show_list (lines, 0, 0);
}

void
expr_watch_show (ExprWatch * ew)
{
	if (ew)
	{
		if (ew->is_showing)
		{
			gdk_window_raise (ew->widgets.window->window);
			return;
		}
		gtk_widget_set_uposition (ew->widgets.window, ew->win_pos_x,
								  ew->win_pos_y);
		gtk_window_set_default_size (GTK_WINDOW (ew->widgets.window),
									 ew->win_width, ew->win_height);
		gtk_widget_show (ew->widgets.window);
		ew->is_showing = TRUE;
	}
}

void
expr_watch_hide (ExprWatch * ew)
{
	if (ew)
	{
		if (ew->is_showing == FALSE)
			return;
		gdk_window_get_root_origin (ew->widgets.window->window, &ew->win_pos_x,
									&ew->win_pos_y);
		gdk_window_get_size (ew->widgets.window->window, &ew->win_width,
							 &ew->win_height);
		gtk_widget_hide (ew->widgets.window);
		ew->is_showing = FALSE;
	}
}

gboolean
expr_watch_save_yourself (ExprWatch * ew, FILE * stream)
{
	if (!ew)
		return FALSE;

	if (ew->is_showing)
	{
		gdk_window_get_root_origin (ew->widgets.window->window, &ew->win_pos_x,
									&ew->win_pos_y);
		gdk_window_get_size (ew->widgets.window->window, &ew->win_width,
							 &ew->win_height);
	}
	fprintf (stream, "expr.watch.win.pos.x=%d\n", ew->win_pos_x);
	fprintf (stream, "expr.watch.win.pos.y=%d\n", ew->win_pos_y);
	fprintf (stream, "expr.watch.win.width=%d\n", ew->win_width);
	fprintf (stream, "expr.watch.win.height=%d\n", ew->win_height);
	return TRUE;
}

gboolean
expr_watch_load_yourself (ExprWatch * ew, PropsID props)
{
	if (!ew)
		return FALSE;

	ew->win_pos_x = prop_get_int (props, "expr.watch.win.pos.x", 20);
	ew->win_pos_y = prop_get_int (props, "expr.watch.win.pos.y", 323);
	ew->win_width = prop_get_int (props, "expr.watch.win.width", 400);
	ew->win_height = prop_get_int (props, "expr.watch.win.height", 120);
	return TRUE;
}

void
expr_watch_destroy (ExprWatch * ew)
{
	if (ew)
	{
		expr_watch_clear (ew);
		gtk_widget_unref (ew->widgets.window);
		gtk_widget_unref (ew->widgets.clist);
		gtk_widget_unref (ew->widgets.menu_add);
		gtk_widget_unref (ew->widgets.menu_remove);
		gtk_widget_unref (ew->widgets.menu_clear);
		gtk_widget_unref (ew->widgets.menu_update);
		gtk_widget_unref (ew->widgets.menu_toggle);
		gtk_widget_unref (ew->widgets.menu);
		if (GTK_IS_WIDGET (ew->widgets.window))
			gtk_widget_destroy (ew->widgets.window);
		g_free (ew);
	}
}
