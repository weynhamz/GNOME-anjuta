/*
    signals.cbs
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

#ifndef _SIGNALS_CBS_H_
#define _SIGNALS_CBS_H_

void
on_signals_clist_select_row          (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gint
on_signals_delete_event(GtkWidget* w, GdkEvent* event, gpointer data);

void
on_signals_modify_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_signals_send_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_signals_update_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);
										
void
on_signals_help_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);
gboolean
on_signals_event                (GtkWidget       *widget,
                                        GdkEvent  *event,
                                        gpointer         user_data);
gboolean
on_signals_set_dialog_delete_event     (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_signals_togglebutton1_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_signals_togglebutton2_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_signals_togglebutton3_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_signals_set_help_clicked            (GtkButton       *button,
                                        gpointer         user_data);

void
on_signals_set_ok_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_signals_set_cancel_clicked          (GtkButton       *button,
                                        gpointer         user_data);

#endif
