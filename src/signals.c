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
#include "utilities.h"

Signals *
signals_new ()
{
  Signals *ew;
  ew = g_malloc (sizeof (Signals));
  if (ew)
  {
    ew->is_showing = FALSE;
    ew->win_width = 460;
    ew->win_height = 320;
    ew->win_pos_x = 150;
    ew->win_pos_y = 130;
    ew->idx = -1;
    create_signals_gui (ew);
  }
  return ew;
}

void
signals_clear (Signals * sg)
{
  if (GTK_IS_CLIST (sg->widgets.clist))
    gtk_clist_clear (GTK_CLIST (sg->widgets.clist));
  sg->idx = -1;
}

void
signals_update (GList * lines, gpointer data)
{
  Signals *sg;
  gint j, count;
  gchar *row[5], *str;
  gchar sig[32], stop[10], print[10], pass[10];
  GList *list, *node;

  sg = (Signals *) data;
  signals_clear (sg);
  list = remove_blank_lines (lines);
  if (g_list_length (list) < 2)
  {
    g_list_free (list);
    return;
  }
  gtk_clist_freeze (GTK_CLIST(sg->widgets.clist));
  node = list->next;
  while (node)
  {
    count =
      sscanf ((char *) node->data, "%s %s %s %s", sig, stop, print, pass);
    str = node->data;
    node = g_list_next (node);
    if (count != 4)
      continue;

    /* Do not worry. This is used to avoid the last line */
    if (node == NULL)
      break;
    row[0] = sig;
    row[1] = stop;
    row[2] = print;
    row[3] = pass;
    for (j = 0; j < 4; j++)
    {
      while (isspace (*str))
	str++;
      while (!isspace (*str))
	str++;
    }
    while (isspace (*str))
      str++;
    row[4] = str;
    gtk_clist_append (GTK_CLIST (sg->widgets.clist), row);
  }
  g_list_free (list);
  gtk_clist_thaw (GTK_CLIST(sg->widgets.clist));
}

void
signals_show (Signals * ew)
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
signals_hide (Signals * ew)
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
signals_save_yourself (Signals * ew, FILE * stream)
{
	if (!ew) return FALSE;

	if (ew->is_showing)
	{
		gdk_window_get_root_origin (ew->widgets.window->window, &ew->win_pos_x,
		      &ew->win_pos_y);
		gdk_window_get_size (ew->widgets.window->window, &ew->win_width, &ew->win_height);
	}
	fprintf(stream, "signals.win.pos.x=%d\n", ew->win_pos_x);
	fprintf(stream, "signals.win.pos.y=%d\n", ew->win_pos_y);
	fprintf(stream, "signals.win.width=%d\n", ew->win_width);
	fprintf(stream, "signals.win.height=%d\n", ew->win_height);
	return TRUE;
}

gboolean
signals_load_yourself (Signals * ew, PropsID props)
{
	if (!ew) return FALSE;
	
	ew->win_pos_x = prop_get_int (props, "signals.win.pos.x", 150);
	ew->win_pos_y = prop_get_int (props, "signals.win.pos.y", 130);
	ew->win_width = prop_get_int (props, "signals.win.width", 460);
	ew->win_height = prop_get_int (props, "signals.win.height", 320);
	return TRUE;
}

void
signals_destroy (Signals * sg)
{
  if (sg)
  {
    signals_clear (sg);
    gtk_widget_unref (sg->widgets.window);
    gtk_widget_unref (sg->widgets.clist);
    gtk_widget_unref (sg->widgets.menu);
    gtk_widget_unref (sg->widgets.menu_modify);
    gtk_widget_unref (sg->widgets.menu_signal);
    gtk_widget_unref (sg->widgets.menu_update);
    if (GTK_IS_WIDGET (sg->widgets.window))
      gtk_widget_destroy (sg->widgets.window);
    g_free (sg);
  }
}

void
signals_update_controls (Signals * ew)
{
  gboolean A, R, Pr;

  A = debugger_is_active ();
  R = debugger_is_ready ();
  Pr = debugger.prog_is_running;
  gtk_widget_set_sensitive (ew->widgets.menu_signal, A && Pr);
  gtk_widget_set_sensitive (ew->widgets.menu_modify, A && R);
  gtk_widget_set_sensitive (ew->widgets.menu_update, A && R);
}
