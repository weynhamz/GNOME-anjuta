/*
    compiler_options_cbs.h
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
#ifndef _COMPILER_OPTIONS_CBS_H_
#define _COMPILER_OPTIONS_CBS_H_

#include <gnome.h>

gboolean
on_comopt_delete_event (GtkWidget *w, GdkEvent *event,
                                        gpointer         user_data);

void on_comopt_ok_clicked(GtkButton *button, gpointer data);
void on_comopt_help_clicked(GtkButton *button, gpointer data);
void on_comopt_apply_clicked(GtkButton *button, gpointer data);
void on_comopt_cancel_clicked(GtkButton *button, gpointer data);

void
on_co_supp_clist_select_row (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_co_inc_clist_select_row (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_co_lib_clist_select_row (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_co_lib_stock_clist_select_row (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_co_lib_paths_clist_select_row (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_co_def_clist_select_row (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_co_supp_info_clicked(GtkButton *button, gpointer data);

void
on_co_supp_help_clicked(GtkButton *button, gpointer data);

void
on_co_inc_add_clicked(GtkButton *button, gpointer data);

void
on_co_inc_update_clicked(GtkButton *button, gpointer data);

void
on_co_inc_remove_clicked(GtkButton *button, gpointer data);

void
on_co_inc_clear_clicked(GtkButton *button, gpointer data);

void
on_co_inc_help_clicked(GtkButton *button, gpointer data);

void
on_co_lib_paths_add_clicked(GtkButton *button, gpointer data);

void
on_co_lib_paths_update_clicked(GtkButton *button, gpointer data);

void
on_co_lib_paths_remove_clicked(GtkButton *button, gpointer data);

void
on_co_lib_paths_clear_clicked(GtkButton *button, gpointer data);

void
on_co_lib_paths_help_clicked(GtkButton *button, gpointer data);

void
on_co_lib_add_clicked(GtkButton *button, gpointer data);

void
on_co_lib_update_clicked(GtkButton *button, gpointer data);

void
on_co_lib_remove_clicked(GtkButton *button, gpointer data);

void
on_co_lib_clear_clicked(GtkButton *button, gpointer data);

void
on_co_lib_help_clicked(GtkButton *button, gpointer data);

void
on_co_def_add_clicked(GtkButton *button, gpointer data);

void
on_co_def_update_clicked(GtkButton *button, gpointer data);

void
on_co_def_remove_clicked(GtkButton *button, gpointer data);

void
on_co_def_clear_clicked(GtkButton *button, gpointer data);

void
on_co_def_help_clicked(GtkButton *button, gpointer data);

#endif
