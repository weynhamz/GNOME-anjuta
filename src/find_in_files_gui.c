/*
    find_in_files_gui.c
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

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gnome.h>

#include "mainmenu_callbacks.h"

#include "anjuta.h"
#include "find_in_files_cbs.h"
#include "pixmaps.h"
#include "resources.h"

void
create_find_in_files_gui (FindInFiles *sf)
{
  GtkWidget *dialog3;
  GtkWidget *dialog_vbox3;
  GtkWidget *frame4;
  GtkWidget *vbox3;
  GtkWidget *hbox2;
  GtkWidget *label4;
  GtkWidget *fileentry1;
  GtkWidget *combo_entry5;
  GtkWidget *hbox1;
  GtkWidget *scrolledwindow1;
  GtkWidget *clist1;
  GtkWidget *label1;
  GtkWidget *vseparator1;
  GtkWidget *vbox4;
  GtkWidget *button10;
  GtkWidget *button11;
  GtkWidget *button15;
  GtkWidget *button16;
  GtkWidget *checkbutton3;
  GtkWidget *checkbutton4;
  GtkWidget *frame5;
  GtkWidget *entry1;
  GtkWidget *combo_entry4;
  GtkWidget *dialog_action_area3;
  GtkWidget *button8;
  GtkWidget *button9;

  dialog3 = gnome_dialog_new (_("Find in Files"), NULL);
  gtk_window_set_position (GTK_WINDOW (dialog3), GTK_WIN_POS_CENTER);
/*  gtk_window_set_modal (GTK_WINDOW (dialog3), TRUE); */
  gtk_window_set_policy (GTK_WINDOW (dialog3), FALSE, FALSE, FALSE);
  gtk_window_set_wmclass (GTK_WINDOW (dialog3), "find_files", "Anjuta");
  gnome_dialog_close_hides (GNOME_DIALOG (dialog3), TRUE);

  dialog_vbox3 = GNOME_DIALOG (dialog3)->vbox;
  gtk_widget_show (dialog_vbox3);

  frame4 = gtk_frame_new (NULL);
  gtk_widget_show (frame4);
  gtk_box_pack_start (GTK_BOX (dialog_vbox3), frame4, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame4), 5);

  vbox3 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox3);
  gtk_container_add (GTK_CONTAINER (frame4), vbox3);

  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox2);
  gtk_box_pack_start (GTK_BOX (vbox3), hbox2, FALSE, FALSE, 0);

  label4 = gtk_label_new (_("File/Directory to add:"));
  gtk_widget_show (label4);
  gtk_box_pack_start (GTK_BOX (hbox2), label4, FALSE, FALSE, 0);
  gtk_misc_set_padding (GTK_MISC (label4), 5, 0);

  fileentry1 = gnome_file_entry_new (NULL, NULL);
  gtk_widget_show (fileentry1);
  gtk_box_pack_start (GTK_BOX (hbox2), fileentry1, TRUE, TRUE, 0);
  gtk_widget_set_usize (fileentry1, 356, -2);
  gtk_container_set_border_width (GTK_CONTAINER (fileentry1), 5);

  combo_entry5 = gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (fileentry1));
  gtk_widget_show (combo_entry5);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (vbox3), hbox1, TRUE, TRUE, 0);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolledwindow1);
  gtk_box_pack_start (GTK_BOX (hbox1), scrolledwindow1, TRUE, TRUE, 0);
  gtk_widget_set_usize (scrolledwindow1, 301, 200);
  gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow1), 5);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  clist1 = gtk_clist_new (1);
  gtk_widget_show (clist1);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), clist1);
  gtk_clist_set_selection_mode (GTK_CLIST (clist1), GTK_SELECTION_BROWSE);
  gtk_clist_column_titles_show (GTK_CLIST (clist1));

  label1 = gtk_label_new (_("List of Files/Directories to be searched"));
  gtk_widget_show (label1);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 0, label1);

  vseparator1 = gtk_vseparator_new ();
  gtk_widget_show (vseparator1);
  gtk_box_pack_start (GTK_BOX (hbox1), vseparator1, FALSE, TRUE, 0);

  vbox4 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox4);
  gtk_box_pack_start (GTK_BOX (hbox1), vbox4, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox4), 5);

  button10 = gtk_button_new_with_label (_("Add"));
  gtk_widget_show (button10);
  gtk_box_pack_start (GTK_BOX (vbox4), button10, FALSE, FALSE, 0);

  button11 = gtk_button_new_with_label (_("Remove"));
  gtk_widget_show (button11);
  gtk_box_pack_start (GTK_BOX (vbox4), button11, FALSE, FALSE, 5);

  button15 = gtk_button_new_with_label (_("Clear"));
  gtk_widget_show (button15);
  gtk_box_pack_start (GTK_BOX (vbox4), button15, FALSE, FALSE, 0);

  button16 = gnome_stock_button (GNOME_STOCK_BUTTON_HELP);
  gtk_widget_show (button16);
  gtk_box_pack_start (GTK_BOX (vbox4), button16, FALSE, FALSE, 5);

  checkbutton3 = gtk_check_button_new_with_label (_("Case Sensitive"));
  gtk_widget_show (checkbutton3);
  gtk_box_pack_start (GTK_BOX (vbox4), checkbutton3, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton3), TRUE);
  
  checkbutton4 = gtk_check_button_new_with_label (_("Ignore binary files"));
  gtk_widget_show (checkbutton4);
  gtk_box_pack_start (GTK_BOX (vbox4), checkbutton4, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton4), TRUE);

  frame5 = gtk_frame_new (_(" Search String"));
  gtk_widget_show (frame5);
  gtk_box_pack_start (GTK_BOX (vbox3), frame5, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame5), 5);

  entry1 = gnome_entry_new (NULL);
  gtk_widget_show (entry1);
  gtk_container_add (GTK_CONTAINER (frame5), entry1);
  gtk_container_set_border_width (GTK_CONTAINER (entry1), 5);

  combo_entry4 = gnome_entry_gtk_entry (GNOME_ENTRY (entry1));
  gtk_widget_show (combo_entry4);

  dialog_action_area3 = GNOME_DIALOG (dialog3)->action_area;
  gtk_widget_show (dialog_action_area3);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area3), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area3), 8);

  gnome_dialog_append_button (GNOME_DIALOG (dialog3), "Close");
  button8 = g_list_last (GNOME_DIALOG (dialog3)->buttons)->data;
  gtk_widget_show (button8);
  GTK_WIDGET_SET_FLAGS (button8, GTK_CAN_DEFAULT);

  gnome_dialog_append_button (GNOME_DIALOG (dialog3), "Find");
  button9 = g_list_last (GNOME_DIALOG (dialog3)->buttons)->data;
  gtk_widget_show (button9);
  GTK_WIDGET_SET_FLAGS (button9, GTK_CAN_DEFAULT);

  gtk_accel_group_attach(app->accel_group, GTK_OBJECT(dialog3));

  gtk_signal_connect (GTK_OBJECT (clist1), "select_row",
                      GTK_SIGNAL_FUNC (on_search_in_files_clist_select_row),
                      sf);
  gtk_signal_connect (GTK_OBJECT (dialog3), "close",
                      GTK_SIGNAL_FUNC (on_search_in_files_close),
                      sf);
  gtk_signal_connect (GTK_OBJECT (button10), "clicked",
                      GTK_SIGNAL_FUNC (on_search_in_files_add_clicked),
                      sf);
  gtk_signal_connect (GTK_OBJECT (button11), "clicked",
                      GTK_SIGNAL_FUNC (on_search_in_files_remove_clicked),
                      sf);
  gtk_signal_connect (GTK_OBJECT (button15), "clicked",
                      GTK_SIGNAL_FUNC (on_search_in_files_clear_clicked),
                      sf);
  gtk_signal_connect (GTK_OBJECT (button16), "clicked",
                      GTK_SIGNAL_FUNC (on_search_in_files_help_clicked),
                      sf);
  gtk_signal_connect (GTK_OBJECT (button8), "clicked",
                      GTK_SIGNAL_FUNC (on_search_in_files_cancel_clicked),
                      sf);
  gtk_signal_connect (GTK_OBJECT (button9), "clicked",
                      GTK_SIGNAL_FUNC (on_search_in_files_ok_clicked),
                      sf);

  sf->widgets.window = dialog3;gtk_widget_ref(dialog3);
  sf->widgets.file_entry = fileentry1;gtk_widget_ref(fileentry1);
  sf->widgets.file_combo = combo_entry5;gtk_widget_ref(combo_entry5);
  sf->widgets.clist = clist1;gtk_widget_ref(clist1);
  sf->widgets.add = button10;gtk_widget_ref(button10);
  sf->widgets.remove = button11;gtk_widget_ref(button11);
  sf->widgets.clear = button15;gtk_widget_ref(button15);
  sf->widgets.regexp_entry = combo_entry4;gtk_widget_ref(combo_entry4);
  sf->widgets.regexp_combo = entry1;gtk_widget_ref(entry1);
  sf->widgets.help = button16;gtk_widget_ref(button16);
  sf->widgets.cancel = button8;gtk_widget_ref(button8);
  sf->widgets.ok = button9;gtk_widget_ref(button9);
  sf->widgets.case_sensitive_check = checkbutton3;
      gtk_widget_ref(checkbutton3);
  sf->widgets.ignore_binary = checkbutton4;
	  gtk_widget_ref(checkbutton4);

	gtk_window_set_transient_for(GTK_WINDOW(sf->widgets.window), GTK_WINDOW(app->widgets.window));
}
