/* 
    src_paths
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
#ifndef _SRC_PATHS_CBS_H_
#define _SRC_PATHS_CBS_H_

#include <gnome.h>

gboolean
on_src_paths_delete_event (GtkWidget *w, GdkEvent *event,
                                        gpointer         user_data);

void on_src_paths_ok_clicked(GtkButton *button, gpointer data);
void on_src_paths_help_clicked(GtkButton *button, gpointer data);
void on_src_paths_apply_clicked(GtkButton *button, gpointer data);
void on_src_paths_cancel_clicked(GtkButton *button, gpointer data);

void
on_src_paths_src_clist_select_row (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);
void
on_src_paths_src_add_clicked(GtkButton *button, gpointer data);

void
on_src_paths_src_update_clicked(GtkButton *button, gpointer data);

void
on_src_paths_src_remove_clicked(GtkButton *button, gpointer data);

void
on_src_paths_src_clear_clicked(GtkButton *button, gpointer data);

void
on_src_paths_src_help_clicked(GtkButton *button, gpointer data);

#endif
