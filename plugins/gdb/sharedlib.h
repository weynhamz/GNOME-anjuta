/*
    sharedlibs.h
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

#ifndef _SHAREDLIBS_H_
#define _SHAREDLIBS_H_

#include <gnome.h>
/* TODO #include "properties.h" */

typedef struct _SharedlibsGui SharedlibsGui;
typedef struct _Sharedlibs Sharedlibs;

struct _SharedlibsGui
{
    GtkWidget*   window;
    GtkWidget*   clist;
    GtkWidget*   menu;
    GtkWidget*   menu_update;
};

struct _Sharedlibs
{
  SharedlibsGui  widgets;
  gboolean         is_showing;
  gint             win_pos_x, win_pos_y, win_width, win_height;
};

Sharedlibs*
sharedlibs_new(void);

void
create_sharedlibs_gui(Sharedlibs* ew);

GtkWidget*
create_sharedlibs_modify_dialog(void);

void
sharedlibs_clear(Sharedlibs *ew);

void
sharedlibs_update(GList *lines, gpointer  ew);

void
sharedlibs_destroy(Sharedlibs*ew);

gboolean
sharedlibs_save_yourself(Sharedlibs* ew, FILE* stream);

/* TODO
gboolean
sharedlibs_load_yourself(Sharedlibs* ew, PropsID props);
*/

void
sharedlibs_show(Sharedlibs * ew);

void
sharedlibs_hide(Sharedlibs * ew);

void
sharedlibs_update_controls(Sharedlibs* ew);

#endif
