/*
    cpu_registers.c
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

CpuRegisters* cpu_registers_new()
{
  CpuRegisters* ew;
  ew = g_malloc(sizeof(CpuRegisters));
  if(ew)
  {
     create_cpu_registers_gui(ew);
     ew->current_index = 0;
     ew->is_showing = FALSE;
     ew->win_pos_x = 250;
     ew->win_pos_y = 100;
     ew->win_width = 200;
     ew->win_height = 300;
  }
  return ew;
}

void
cpu_registers_clear(CpuRegisters *ew)
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
cpu_registers_update(CpuRegisters *ew)
{
	g_return_if_fail (ew);

	if (ew->is_showing)
	{
		debugger_put_cmd_in_queqe ("info registers", DB_CMD_NONE,
					cpu_registers_update_cb, ew);
	}
}

void
cpu_registers_show(CpuRegisters* ew)
{
  gboolean was_showing;

  if(ew)
  {
     if ((was_showing = ew->is_showing))
     {
		 gdk_window_raise(ew->widgets.window->window);
     }
     gtk_window_move (GTK_WINDOW (ew->widgets.window),
					  ew->win_pos_x, ew->win_pos_y);
     gtk_window_set_default_size(GTK_WINDOW(ew->widgets.window),
								 ew->win_width, ew->win_height);
     gtk_widget_show(ew->widgets.window);
     ew->is_showing = TRUE;
	 if (debugger_is_active() && debugger_is_ready())
	 {
		 cpu_registers_update(ew);
		 if (! was_showing) debugger_execute_cmd_in_queqe();
	 }
  }
}

void
cpu_registers_hide(CpuRegisters* ew)
{
  if(ew)
  {
     if(ew->is_showing == FALSE) return;
     gdk_window_get_root_origin(ew ->widgets.window->window, &ew->win_pos_x, &ew->win_pos_y);
     gdk_drawable_get_size(GDK_DRAWABLE (ew ->widgets.window->window),
						   &ew->win_width, &ew->win_height);
     gtk_widget_hide(ew->widgets.window);
     ew->is_showing = FALSE;
  }
}

gboolean
cpu_registers_save_yourself(CpuRegisters* ew, FILE* stream)
{
	if (!ew) return FALSE;

	if (ew->is_showing)
	{
		gdk_window_get_root_origin (ew->widgets.window->window, &ew->win_pos_x,
		      &ew->win_pos_y);
		gdk_drawable_get_size (GDK_DRAWABLE (ew->widgets.window->window),
							   &ew->win_width, &ew->win_height);
	}
	fprintf(stream, "registers.win.pos.x=%d\n", ew->win_pos_x);
	fprintf(stream, "registers.win.pos.y=%d\n", ew->win_pos_y);
	fprintf(stream, "registers.win.width=%d\n", ew->win_width);
	fprintf(stream, "registers.win.height=%d\n", ew->win_height);
	return TRUE;
}

gboolean
cpu_registers_load_yourself(CpuRegisters* ew, PropsID props)
{
	if (!ew) return FALSE;
	
	ew->win_pos_x = prop_get_int (props, "registers.win.pos.x", 250);
	ew->win_pos_y = prop_get_int (props, "registers.win.pos.y", 100);
	ew->win_width = prop_get_int (props, "registers.win.width", 200);
	ew->win_height = prop_get_int (props, "registers.win.height", 300);
	return TRUE;
}

void
cpu_registers_destroy(CpuRegisters* cr)
{
  if(cr)
  {
     cpu_registers_clear(cr);
     gtk_widget_unref(cr->widgets.window);
     gtk_widget_unref(cr->widgets.view);
     gtk_widget_unref(cr->widgets.menu);
     gtk_widget_unref(cr->widgets.menu_modify);
     gtk_widget_unref(cr->widgets.menu_update);
     if(GTK_IS_WIDGET(cr->widgets.window))
              gtk_widget_destroy(cr->widgets.window);
     g_free(cr);
  }
}

void
registers_update_controls(CpuRegisters* ew)
{
     gboolean A, R;

     A = debugger_is_active();
     R = debugger_is_ready();

     gtk_widget_set_sensitive(ew->widgets.menu_modify, A && R);
     gtk_widget_set_sensitive(ew->widgets.menu_update, A && R);
}
