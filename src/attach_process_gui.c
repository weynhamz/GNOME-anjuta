/*
    attach_process_gui.h
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gnome.h>

#include "anjuta.h"
#include "attach_process.h"
#include "attach_process_cbs.h"
#include "resources.h"

void
create_attach_process_gui (AttachProcess* ap)
{
  GtkWidget *dialog1;
  GtkWidget *dialog_vbox1;
  GtkWidget *label7;
  GtkWidget *scrolledwindow1;
  GtkWidget *clist1;
  GtkWidget *label3;
  GtkWidget *label4;
  GtkWidget *label5;
  GtkWidget *label6;
  GtkWidget *dialog_action_area1;
  GtkWidget *button1;
  GtkWidget *button2;
  GtkWidget *button3;

  dialog1 = gnome_dialog_new (_("Attach to process"), NULL);
  gtk_widget_set_usize (dialog1, 509, 309);
  gtk_window_set_position (GTK_WINDOW (dialog1), GTK_WIN_POS_CENTER);
  gtk_window_set_policy (GTK_WINDOW (dialog1), FALSE, FALSE, FALSE);
  gtk_window_set_wmclass (GTK_WINDOW (dialog1), "attach_process", "Anjuta");
  gnome_dialog_close_hides (GNOME_DIALOG (dialog1), TRUE);

  dialog_vbox1 = GNOME_DIALOG (dialog1)->vbox;
  gtk_widget_show (dialog_vbox1);

  label7 = gtk_label_new (_("Select the process to attach:"));
  gtk_widget_show (label7);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), label7, FALSE, FALSE, 0);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolledwindow1);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), scrolledwindow1, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  clist1 = gtk_clist_new (4);
  gtk_widget_show (clist1);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), clist1);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 0, 80);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 1, 80);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 2, 80);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 3, 80);
  gtk_clist_set_selection_mode (GTK_CLIST (clist1), GTK_SELECTION_BROWSE);
  gtk_clist_column_titles_show (GTK_CLIST (clist1));
  gtk_clist_set_column_auto_resize (GTK_CLIST(clist1), 3, TRUE);

  label3 = gtk_label_new (_("Pid"));
  gtk_widget_show (label3);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 0, label3);

  label4 = gtk_label_new (_("Owner"));
  gtk_widget_show (label4);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 1, label4);

  label5 = gtk_label_new (_("Time"));
  gtk_widget_show (label5);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 2, label5);

  label6 = gtk_label_new (_("Command"));
  gtk_widget_show (label6);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 3, label6);

  dialog_action_area1 = GNOME_DIALOG (dialog1)->action_area;
  gtk_widget_show (dialog_action_area1);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_SPREAD);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area1), 8);

  gnome_dialog_append_button_with_pixmap (GNOME_DIALOG (dialog1),
                                          _("Update"), GNOME_STOCK_PIXMAP_JUMP_TO);
  button1 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
  gtk_widget_show (button1);
  GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);

  gnome_dialog_append_button_with_pixmap (GNOME_DIALOG (dialog1),
                                          _("Attach"), GNOME_STOCK_PIXMAP_JUMP_TO);
  button2 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
  gtk_widget_show (button2);
  GTK_WIDGET_SET_FLAGS (button2, GTK_CAN_DEFAULT);

  gnome_dialog_append_button (GNOME_DIALOG (dialog1), GNOME_STOCK_BUTTON_CANCEL);
  button3 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
  gtk_widget_show (button3);
  GTK_WIDGET_SET_FLAGS (button3, GTK_CAN_DEFAULT);

  gtk_accel_group_attach(app->accel_group, GTK_OBJECT(dialog1));

  gtk_signal_connect (GTK_OBJECT (dialog1), "close",
				  	  GTK_SIGNAL_FUNC (on_attach_process_close),
					  ap);
  gtk_signal_connect (GTK_OBJECT (clist1), "select_row",
                      GTK_SIGNAL_FUNC (on_attach_process_clist_select_row),
                      ap);
  gtk_signal_connect (GTK_OBJECT (button1), "clicked",
                      GTK_SIGNAL_FUNC (on_attach_process_update_clicked),
                      ap);
  gtk_signal_connect (GTK_OBJECT (button2), "clicked",
                      GTK_SIGNAL_FUNC (on_attach_process_attach_clicked),
                      ap);
  gtk_signal_connect (GTK_OBJECT (button3), "clicked",
                      GTK_SIGNAL_FUNC (on_attach_process_cancel_clicked),
                      ap);

  ap->widgets.window = dialog1;
  gtk_widget_ref(dialog1);
  ap->widgets.update_button = button1;
  gtk_widget_ref(button1);
  ap->widgets.attach_button = button2;
  gtk_widget_ref(button2);
  ap->widgets.cancel_button = button3;
  gtk_widget_ref(button3);
  ap->widgets.clist = clist1;
  gtk_widget_ref(clist1);
}

