/*
    goto_line.c
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

#include "text_editor.h"
#include "resources.h"
#include "anjuta.h"
#include "goto_line.h"

static void
on_go_to_line_ok_clicked               (GtkButton       *button,
                                        gpointer         user_data);

GtkWidget*
create_goto_line_dialog ()
{
  GtkWidget *dialog4;
  GtkWidget *dialog_vbox4;
  GtkWidget *frame6;
  GtkWidget *numberentry1;
  GtkWidget *combo_entry6;
  GtkWidget *dialog_action_area4;
  GtkWidget *button12;
  GtkWidget *button14;

  dialog4 = gnome_dialog_new (NULL, NULL);
  gtk_window_set_position (GTK_WINDOW (dialog4), GTK_WIN_POS_CENTER);
  gtk_window_set_policy (GTK_WINDOW (dialog4), FALSE, FALSE, FALSE);
  gnome_dialog_set_close (GNOME_DIALOG (dialog4), TRUE);

  dialog_vbox4 = GNOME_DIALOG (dialog4)->vbox;
  gtk_widget_show (dialog_vbox4);

  frame6 = gtk_frame_new (_(" Go to Line No. "));
  gtk_widget_show (frame6);
  gtk_box_pack_start (GTK_BOX (dialog_vbox4), frame6, FALSE, FALSE, 0);

  numberentry1 = gnome_number_entry_new (NULL, NULL);
  gtk_widget_show (numberentry1);
  gtk_container_add (GTK_CONTAINER (frame6), numberentry1);
  gtk_container_set_border_width (GTK_CONTAINER (numberentry1), 10);

  combo_entry6 = gnome_number_entry_gtk_entry (GNOME_NUMBER_ENTRY (numberentry1));
  gtk_widget_show (combo_entry6);

  dialog_action_area4 = GNOME_DIALOG (dialog4)->action_area;
  gtk_widget_show (dialog_action_area4);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area4), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area4), 8);

  gnome_dialog_append_button (GNOME_DIALOG (dialog4), GNOME_STOCK_BUTTON_CANCEL);
  button12 = g_list_last (GNOME_DIALOG (dialog4)->buttons)->data;
  gtk_widget_show (button12);
  GTK_WIDGET_SET_FLAGS (button12, GTK_CAN_DEFAULT);

  gnome_dialog_append_button (GNOME_DIALOG (dialog4), GNOME_STOCK_BUTTON_OK);
  button14 = g_list_last (GNOME_DIALOG (dialog4)->buttons)->data;
  gtk_widget_show (button14);
  GTK_WIDGET_SET_FLAGS (button14, GTK_CAN_DEFAULT);

  gtk_widget_ref(numberentry1);

  gtk_accel_group_attach( app->accel_group, GTK_OBJECT(dialog4));

  gtk_signal_connect (GTK_OBJECT (button14), "clicked",
                      GTK_SIGNAL_FUNC (on_go_to_line_ok_clicked),
                      numberentry1);

  gtk_widget_grab_focus(combo_entry6);
  return dialog4;
}

void
on_go_to_line_ok_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
  GnomeNumberEntry *ne;
  TextEditor *te;
  guint  num;

  ne = user_data;
  te = anjuta_get_current_text_editor();

  num = (guint)gnome_number_entry_get_number(ne);
  gtk_widget_unref(GTK_WIDGET(ne));
  if(te) text_editor_goto_line(te, num, TRUE);
}
