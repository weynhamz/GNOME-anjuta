/*
    registers_cbs.c
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

#include <gnome.h>
#include "registers.h"
#include "registers_cbs.h"
#include "debugger.h"

gint
on_registers_delete_event(GtkWidget* w, GdkEvent *event, gpointer data)
{
  CpuRegisters* cr = data;
  cpu_registers_hide(cr);
  return TRUE;
}

void
on_registers_clist_select_row          (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    CpuRegisters *cr;
    cr = (CpuRegisters *)user_data;
    cr->current_index = row;
}

void
on_register_modify_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}

void
on_register_update_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
 debugger_put_cmd_in_queqe("info all-registers", DB_CMD_NONE, cpu_registers_update, debugger.cpu_registers);
 debugger_execute_cmd_in_queqe();
}


void
on_register_dock_undock_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
   CpuRegisters *cr;
    cr = debugger.cpu_registers;
    if(cr->is_docked) cpu_registers_undock(cr);
    else cpu_registers_dock(cr);
}


void
on_register_help_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}

gboolean
on_register_event                (GtkWidget       *widget,
                                          GdkEvent  *event,
                                        gpointer         user_data)
{
  GdkEventButton *bevent;
  CpuRegisters* ew = debugger.cpu_registers;
  if (event->type != GDK_BUTTON_PRESS) return FALSE;
  if (((GdkEventButton *)event)->button != 3) return FALSE;
  bevent =  (GdkEventButton *)event;
  bevent->button = 1;
  registers_update_controls(ew);
  gtk_menu_popup (GTK_MENU(ew->widgets.menu), NULL,
                NULL, NULL, NULL,
                bevent->button, bevent->time);
  return TRUE;
}