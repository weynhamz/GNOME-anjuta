/*
 * src_paths_gui.c Copyright (C) 2000  Kh. Naba Kumar Singh
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59 
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gnome.h>

#include "anjuta.h"
#include "src_paths.h"
#include "src_paths_cbs.h"
#include "resources.h"

GtkWidget*
create_src_paths_page0(SrcPaths *co);

void
create_src_paths_gui (SrcPaths* co)
{
  GtkWidget *window1;
  GtkWidget *dialog1;
  GtkWidget *dialog_vbox1;
  GtkWidget *dialog_action_area1;
  GtkWidget *button_ok;
  GtkWidget *button_apply;
  GtkWidget *button_help;
  GtkWidget *button_cancel;
  GtkWidget *tmp_widget;

  dialog1 = gnome_dialog_new (_("Source files paths"), NULL);
  gtk_window_set_transient_for (GTK_WINDOW(dialog1), GTK_WINDOW(app->widgets.window));
  gtk_window_set_policy (GTK_WINDOW (dialog1), FALSE, FALSE, FALSE);
  gtk_window_set_wmclass (GTK_WINDOW (dialog1), "src_paths", "Anjuta");
  gnome_dialog_close_hides (GNOME_DIALOG (dialog1), TRUE);

  dialog_vbox1 = GNOME_DIALOG (dialog1)->vbox;
  gtk_widget_show (dialog_vbox1);

  window1 = dialog1;

  tmp_widget = create_src_paths_page0(co);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), tmp_widget, TRUE, TRUE, 0);

  dialog_action_area1 = GNOME_DIALOG (dialog1)->action_area;
  gtk_widget_show (dialog_action_area1);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area1), 8);

  gnome_dialog_append_button (GNOME_DIALOG (dialog1), GNOME_STOCK_BUTTON_HELP);
  button_help = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
  gtk_widget_show (button_help);
  GTK_WIDGET_SET_FLAGS (button_help, GTK_CAN_DEFAULT);

  gnome_dialog_append_button (GNOME_DIALOG (dialog1), GNOME_STOCK_BUTTON_OK);
  button_ok = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
  gtk_widget_show (button_ok);
  GTK_WIDGET_SET_FLAGS (button_ok, GTK_CAN_DEFAULT);

  gnome_dialog_append_button (GNOME_DIALOG (dialog1), GNOME_STOCK_BUTTON_APPLY);
  button_apply = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
  gtk_widget_show (button_apply);
  GTK_WIDGET_SET_FLAGS (button_apply, GTK_CAN_DEFAULT);

  gnome_dialog_append_button (GNOME_DIALOG (dialog1), GNOME_STOCK_BUTTON_CANCEL);
  button_cancel = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
  gtk_widget_show (button_cancel);
  GTK_WIDGET_SET_FLAGS (button_cancel, GTK_CAN_DEFAULT);

  gtk_accel_group_attach( app->accel_group, GTK_OBJECT(window1));

  gtk_signal_connect (GTK_OBJECT (dialog1), "close",
                      GTK_SIGNAL_FUNC (on_src_paths_close),
                      co);
  gtk_signal_connect (GTK_OBJECT (button_ok), "clicked",
                      GTK_SIGNAL_FUNC (on_src_paths_ok_clicked),
                      co);
  gtk_signal_connect (GTK_OBJECT (button_help), "clicked",
                      GTK_SIGNAL_FUNC (on_src_paths_help_clicked),
                      co);
  gtk_signal_connect (GTK_OBJECT (button_apply), "clicked",
                      GTK_SIGNAL_FUNC (on_src_paths_apply_clicked),
                      co);
  gtk_signal_connect (GTK_OBJECT (button_cancel), "clicked",
                      GTK_SIGNAL_FUNC (on_src_paths_cancel_clicked),
                      co);

  co->widgets.window = window1; gtk_widget_ref(window1);
}

GtkWidget*
create_src_paths_page0(SrcPaths *co)
{
  GtkWidget *table2;
  GtkWidget *scrolledwindow1;
  GtkWidget *clist1;
  GtkWidget *label7;
  GtkWidget *entry1;
  GtkWidget *vbox1;
  GtkWidget *button0;
  GtkWidget *button1;
  GtkWidget *button2;
  GtkWidget *button3;
  GtkWidget *button4;

  table2 = gtk_table_new (2, 2, FALSE);
  gtk_widget_show (table2);
  gtk_container_set_border_width (GTK_CONTAINER (table2), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table2), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table2), 6);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolledwindow1);
  gtk_table_attach (GTK_TABLE (table2), scrolledwindow1, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  clist1 = gtk_clist_new (1);
  gtk_widget_show (clist1);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), clist1);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 0, 80);
  gtk_clist_set_selection_mode (GTK_CLIST (clist1), GTK_SELECTION_BROWSE);
  gtk_clist_column_titles_show (GTK_CLIST (clist1));

  label7 = gtk_label_new (_("Search Paths"));
  gtk_widget_show (label7);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 0, label7);

	entry1 = gnome_file_entry_new(NULL, NULL);
	gnome_file_entry_set_directory(GNOME_FILE_ENTRY(entry1), TRUE);
  gtk_widget_show (entry1);
  gtk_table_attach (GTK_TABLE (table2), entry1, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox1);
  gtk_table_attach (GTK_TABLE (table2), vbox1, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  button0 = gtk_button_new_with_label (_("Add"));
  gtk_widget_show (button0);
  gtk_box_pack_start (GTK_BOX (vbox1), button0, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (button0), 5);

  button1 = gtk_button_new_with_label (_("Update"));
  gtk_widget_show (button1);
  gtk_box_pack_start (GTK_BOX (vbox1), button1, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (button1), 5);

  button2 = gtk_button_new_with_label (_("Remove"));
  gtk_widget_show (button2);
  gtk_box_pack_start (GTK_BOX (vbox1), button2, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (button2), 5);

  button3 = gtk_button_new_with_label (_("Clear"));
  gtk_widget_show (button3);
  gtk_box_pack_start (GTK_BOX (vbox1), button3, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (button3), 5);

  button4 = gnome_stock_button (GNOME_STOCK_BUTTON_HELP);
  gtk_widget_show (button4);
  gtk_box_pack_start (GTK_BOX (vbox1), button4, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (button4), 5);

  gtk_signal_connect (GTK_OBJECT (clist1), "select_row",
                      GTK_SIGNAL_FUNC (on_src_paths_src_clist_select_row),
                      co);
  gtk_signal_connect (GTK_OBJECT (button0), "clicked",
                      GTK_SIGNAL_FUNC (on_src_paths_src_add_clicked),
                      co);
  gtk_signal_connect (GTK_OBJECT (button1), "clicked",
                      GTK_SIGNAL_FUNC (on_src_paths_src_update_clicked),
                      co);
  gtk_signal_connect (GTK_OBJECT (button2), "clicked",
                      GTK_SIGNAL_FUNC (on_src_paths_src_remove_clicked),
                      co);
  gtk_signal_connect (GTK_OBJECT (button3), "clicked",
                      GTK_SIGNAL_FUNC (on_src_paths_src_clear_clicked),
                      co);
  gtk_signal_connect (GTK_OBJECT (button4), "clicked",
                      GTK_SIGNAL_FUNC (on_src_paths_src_help_clicked),
                      co);

  co->widgets.src_clist = clist1; gtk_widget_ref(clist1);
	co->widgets.src_entry = gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY (entry1));
	gtk_widget_ref(entry1);
  co->widgets.src_add_b = button0; gtk_widget_ref(button0);
  co->widgets.src_update_b = button1; gtk_widget_ref(button1);
  co->widgets.src_remove_b = button2; gtk_widget_ref(button2);
  co->widgets.src_clear_b = button3; gtk_widget_ref(button3);

  return table2;
}
