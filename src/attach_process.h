/*
    attach_process.h
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

#ifndef _ATTACH_PROCESS_H_
#define _ATTACH_PROCESS_H_

#include <gnome.h>
#include "properties.h"

typedef struct _AttachProcessGui AttachProcessGui;
typedef struct _AttachProcess AttachProcess;

struct _AttachProcessGui
{
    GtkWidget*   window;
    GtkWidget*   clist;
    GtkWidget*   update_button;
    GtkWidget*   attach_button;
    GtkWidget*   cancel_button;
};

struct _AttachProcess
{
  AttachProcessGui  widgets;
  gint		pid;
  gboolean	is_showing;
  gint		win_pos_x, win_pos_y;
  gint		win_width, win_height;
};

AttachProcess*
attach_process_new(void);

void
create_attach_process_gui(AttachProcess* ap);

void
attach_process_clear(AttachProcess *ap);

void
attach_process_update(AttachProcess *ap);

void
attach_process_destroy(AttachProcess*ap);

gboolean
attach_process_save_yourself(AttachProcess* ap, FILE* stream);

gboolean
attach_process_load_yourself(AttachProcess* ap, PropsID props);

void
attach_process_show(AttachProcess * ap);

void
attach_process_hide(AttachProcess * ap);

void
attach_process_add_pid (AttachProcess * ap, gchar * line);

void
attach_process_output_arrived(GList *outputs, gpointer data);

#endif
