/*
    breakpoints_cbs.h
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

#ifndef _BREAKPOINTS_DBASE_CBS_H_
#define _BREAKPOINTS_DBASE_CBS_H_ 

#include <gnome.h>

gboolean on_breakpoints_close        (GtkWidget *w, gpointer data);

void    on_bk_view_clicked           (GtkWidget *button, gpointer data);
void    on_bk_edit_clicked           (GtkWidget *button, gpointer data);
void    on_bk_delete_clicked         (GtkWidget *button, gpointer data);
void    on_bk_delete_all_clicked     (GtkWidget *button, gpointer data);
void    on_bk_add_breakpoint_clicked (GtkWidget *button, gpointer data);
void    on_bk_toggle_enable_clicked  (GtkWidget *button, gpointer data);
void    on_bk_enable_all_clicked     (GtkWidget *button, gpointer data);
void    on_bk_disable_all_clicked    (GtkWidget *button, gpointer data);
void    on_bk_close_clicked          (GtkWidget *button, gpointer data);
void    on_bk_ok_clicked             (GtkWidget *button, gpointer data);
void    on_bk_help_clicked           (GtkWidget *button, gpointer data);

void    bk_item_add_mesg_arrived     (GList *outputs, gpointer data);
void    pass_item_add_mesg_arrived   (GList *outputs, gpointer data);

void	on_bk_delete_all_confirm_yes_clicked (GtkWidget *button, gpointer data);

/*****************************************************************************************/
gint    on_bk_item_add_delete_event   (GtkWidget *w, GdkEvent *event, gpointer data);

void    on_bk_item_add_cancel_clicked (GtkWidget *button, gpointer data);
void    on_bk_item_add_help_clicked   (GtkWidget *button, gpointer data);
void    on_bk_item_add_ok_clicked     (GtkButton *button, gpointer data);

/*********************************************************************************/
void    on_bk_item_edit_help_clicked  (GtkWidget *button, gpointer data);
void    on_bk_item_edit_ok_clicked    (GtkWidget *button, gpointer data);

#endif
