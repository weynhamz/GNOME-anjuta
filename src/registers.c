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
#include "messagebox.h"
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
     ew->is_docked   = FALSE;
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
   gtk_clist_clear(GTK_CLIST(ew->widgets.clist));
}

void
cpu_registers_update( GList *lines, gpointer data)
{
    CpuRegisters *ew;
    gchar reg[10], hex[32], dec[32];
    gchar *row[3];
    gint count;
    GList *node, *list;

    ew = (CpuRegisters*)data;

    list = remove_blank_lines(lines);
    cpu_registers_clear(ew);
    if(g_list_length(list) < 2 ){ g_list_free(list); return;}
    node = list->next;
    while(node)
    {
        count = sscanf((char*)node->data, "%s %s %s", reg, hex, dec);
        node = g_list_next(node);
        if(count != 3) continue;
        row[0]=reg;
        row[1]=hex;
        row[2]=dec;
        gtk_clist_append(GTK_CLIST(ew->widgets.clist), row);
     }
     g_list_free(list);
}

void
cpu_registers_show(CpuRegisters* ew)
{
  if(ew)
  {
     if(ew->is_showing)
     {
          if(ew->is_docked == FALSE)
               gdk_window_raise(ew->widgets.window->window);
          return;
     }
     if(ew->is_docked)
     {
        cpu_registers_attach(ew);
     }
     else    /* Is not docked */
     {
        gtk_widget_set_uposition(ew->widgets.window, ew->win_pos_x, ew->win_pos_y);
        gtk_window_set_default_size(GTK_WINDOW(ew->widgets.window), ew->win_width, ew->win_height);
        gtk_widget_show(ew->widgets.window);
     }
     ew->is_showing = TRUE;
  }
}

void
cpu_registers_hide(CpuRegisters* ew)
{
  if(ew)
  {
     if(ew->is_showing == FALSE) return;
     if(ew->is_docked == TRUE)
     {
       cpu_registers_detach(ew);
     }
     else  /* Is not docked */
     {
        gdk_window_get_root_origin(ew ->widgets.window->window, &ew->win_pos_x, &ew->win_pos_y);
        gdk_window_get_size(ew ->widgets.window->window, &ew->win_width, &ew->win_height);
        gtk_widget_hide(ew->widgets.window);
     }
     ew->is_showing = FALSE;
  }
}

void
cpu_registers_attach(CpuRegisters* ew)
{

}
 
void
cpu_registers_detach(CpuRegisters* ew)
{

}

void
cpu_registers_dock(CpuRegisters* ew)
{

}

void
cpu_registers_undock(CpuRegisters* ew)
{

}

gboolean
cpu_registers_save_yourself(CpuRegisters* ew, FILE* stream)
{
	if (!ew) return FALSE;

	fprintf(stream, "registers.is.docked=%d\n", ew->is_docked);
	if (ew->is_showing && !ew->is_docked)
	{
		gdk_window_get_root_origin (ew->widgets.window->window, &ew->win_pos_x,
		      &ew->win_pos_y);
		gdk_window_get_size (ew->widgets.window->window, &ew->win_width, &ew->win_height);
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
	gboolean dock_flag;
	if (!ew) return FALSE;
	
	dock_flag = prop_get_int (props, "registers.is.docked", 0);
	ew->win_pos_x = prop_get_int (props, "registers.win.pos.x", 250);
	ew->win_pos_y = prop_get_int (props, "registers.win.pos.y", 100);
	ew->win_width = prop_get_int (props, "registers.win.width", 200);
	ew->win_height = prop_get_int (props, "registers.win.height", 300);
	if (dock_flag)
		cpu_registers_dock (ew);
	else
		cpu_registers_undock (ew);
	return TRUE;
}

void
cpu_registers_destroy(CpuRegisters* cr)
{
  if(cr)
  {
     cpu_registers_clear(cr);
     gtk_widget_unref(cr->widgets.window);
     gtk_widget_unref(cr->widgets.clist);
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
