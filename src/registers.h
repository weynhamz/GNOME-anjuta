/*
    cpu_registers.h
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

#ifndef _REGISTERS_H_
#define _REGISTERS_H_

#include <gnome.h>
#include "properties.h"

typedef struct _CpuRegistersGui CpuRegistersGui;
typedef struct _CpuRegisters CpuRegisters;

struct _CpuRegistersGui
{
    GtkWidget*   window;
    GtkWidget*   view;
    GtkWidget*   menu;
    GtkWidget*   menu_modify;
    GtkWidget*   menu_update;
};

struct _CpuRegisters
{
  CpuRegistersGui  widgets;
  gint                      current_index;
  gboolean             is_showing;
  gint             win_pos_x, win_pos_y, win_width, win_height;
};

CpuRegisters*
cpu_registers_new(void);

void
create_cpu_registers_gui(CpuRegisters* ew);

GtkWidget*
create_cpu_registers_modify_dialog(void);

void
cpu_registers_clear(CpuRegisters *ew);

void
cpu_registers_update(CpuRegisters *ew);

void
cpu_registers_update_cb(GList *lines, gpointer  ew);

void
cpu_registers_destroy(CpuRegisters*ew);

gboolean
cpu_registers_save_yourself(CpuRegisters* ew, FILE* stream);

gboolean
cpu_registers_load_yourself(CpuRegisters* ew, PropsID props);

void
cpu_registers_show(CpuRegisters * ew);

void
cpu_registers_hide(CpuRegisters * ew);

void
registers_update_controls(CpuRegisters* ew);

#endif
