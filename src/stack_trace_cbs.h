/*
    stack_trace_cbs.h
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

#ifndef _STACK_TRACE_CBS_H_
#define _STACK_TRACE_CBS_H_


void
on_stack_trace_clist_select_row              (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_stack_trace_clist_unselect_row              (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_stack_trace_help_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_stack_frame_set_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_stack_frame_info_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);
void
on_stack_frame_args_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_stack_update_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_stack_view_src_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_stack_help_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_stack_trace_event                (GtkWidget       *widget,
                                        GdkEvent  *event,
                                        gpointer         user_data);

#endif
