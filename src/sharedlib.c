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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gnome.h>
#include "resources.h"
#include "debugger.h"
#include "utilities.h"

Sharedlibs* sharedlibs_new()
{
  Sharedlibs* ew;
  ew = g_malloc(sizeof(Sharedlibs));
  if(ew)
  {
     ew->is_showing = FALSE;
	 ew->win_width = 410;
     ew->win_height = 370;
     ew->win_pos_x = 120;
     ew->win_pos_y = 140;
     create_sharedlibs_gui(ew);
  }
  return ew;
}

void
sharedlibs_clear(Sharedlibs *ew)
{
	GtkTreeModel *model;
	GtkListStore *store;

	g_return_if_fail (ew);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ew->widgets.view));
	store = GTK_LIST_STORE (model);
	if (store)
	{
		gtk_list_store_clear (store);
	}
}

void
sharedlibs_update(Sharedlibs *ew)
{
	g_return_if_fail (ew);

	if (ew->is_showing)
	{
		debugger_put_cmd_in_queqe ("info sharedlibrary", DB_CMD_NONE,
				sharedlibs_update_cb, ew);
	}
}

void
sharedlibs_show(Sharedlibs* ew)
{
  gboolean was_showing;

  if(ew)
  {
     if ((was_showing = ew->is_showing))
     {
         gdk_window_raise(ew->widgets.window->window);
		 return;
	 }
     gtk_widget_set_uposition(ew->widgets.window, ew->win_pos_x, ew->win_pos_y);
     gtk_window_set_default_size(GTK_WINDOW(ew->widgets.window), ew->win_width, ew->win_height);
     gtk_widget_show(ew->widgets.window);
     ew->is_showing = TRUE;
	 if (debugger_is_active() && debugger_is_ready())
	 {
		 sharedlibs_update(ew);
		 if (! was_showing) debugger_execute_cmd_in_queqe();
	 }
  }
}

void
sharedlibs_hide(Sharedlibs* ew)
{
  if(ew)
  {
     if(ew->is_showing == FALSE) return;
     gdk_window_get_root_origin(ew ->widgets.window->window, &ew->win_pos_x, &ew->win_pos_y);
     gdk_window_get_size(ew ->widgets.window->window, &ew->win_width, &ew->win_height);
     gtk_widget_hide(ew->widgets.window);
     ew->is_showing = FALSE;
  }
}

gboolean
sharedlibs_save_yourself(Sharedlibs* ew, FILE* stream)
{
	if (!ew) return FALSE;

	if (ew->is_showing)
	{
		gdk_window_get_root_origin (ew->widgets.window->window, &ew->win_pos_x,
		      &ew->win_pos_y);
		gdk_window_get_size (ew->widgets.window->window, &ew->win_width, &ew->win_height);
	}
	fprintf(stream, "sharedlibs.win.pos.x=%d\n", ew->win_pos_x);
	fprintf(stream, "sharedlibs.win.pos.y=%d\n", ew->win_pos_y);
	fprintf(stream, "sharedlibs.win.width=%d\n", ew->win_width);
	fprintf(stream, "sharedlibs.win.height=%d\n", ew->win_height);
	return TRUE;
}

gboolean
sharedlibs_load_yourself(Sharedlibs* ew, PropsID props)
{
	if (!ew) return FALSE;
	
	ew->win_pos_x = prop_get_int (props, "sharedlibs.win.pos.x", 120);
	ew->win_pos_y = prop_get_int (props, "sharedlibs.win.pos.y", 140);
	ew->win_width = prop_get_int (props, "sharedlibs.win.width", 410);
	ew->win_height = prop_get_int (props, "sharedlibs.win.height", 370);
	return TRUE;
}

void
sharedlibs_destroy(Sharedlibs* sg)
{
  if(sg)
  {
     sharedlibs_clear(sg);
     gtk_widget_unref(sg->widgets.window);
     gtk_widget_unref(sg->widgets.view);
     gtk_widget_unref(sg->widgets.menu);
     gtk_widget_unref(sg->widgets.menu_update);
     if(GTK_IS_WIDGET(sg->widgets.window))
              gtk_widget_destroy(sg->widgets.window);
     g_free(sg);
  }
}

void
sharedlibs_update_controls(Sharedlibs* ew)
{
     gboolean A, R;

     A = debugger_is_active();
     R = debugger_is_ready();

     gtk_widget_set_sensitive(ew->widgets.menu_update, A && R);
}
