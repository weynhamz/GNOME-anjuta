/*
    breakpoints.c
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

#include "anjuta.h"
#include "utilities.h"
#include "breakpoints.h"
#include "breakpoints_cbs.h"

void
create_breakpoints_dbase_gui (BreakpointsDBase *bd)
{
  GtkWidget *dialog1;
  GtkWidget *dialog_vbox1;
  GtkWidget *frame1;
  GtkWidget *scrolledwindow1;
  GtkWidget *clist1;
  GtkWidget *label1;
  GtkWidget *label2;
  GtkWidget *label3;
  GtkWidget *label4;
  GtkWidget *label5;
  GtkWidget *label6;
  GtkWidget *frame2;
  GtkWidget *table1;
  GtkWidget *button4;
  GtkWidget *button5;
  GtkWidget *button6;
  GtkWidget *button7;
  GtkWidget *button9;
  GtkWidget *button8;
  GtkWidget *button11;
  GtkWidget *button10;
  GtkWidget *dialog_action_area1;
  GtkWidget *button1;
  GtkWidget *button2;

  dialog1 = gnome_dialog_new (_("Breakpoints"), NULL);
  gtk_window_set_transient_for (GTK_WINDOW(dialog1), GTK_WINDOW(app->widgets.window));
  gtk_widget_set_usize (dialog1, 550, 352);
  gtk_window_set_policy (GTK_WINDOW (dialog1), FALSE, TRUE, FALSE);
  gtk_window_set_wmclass (GTK_WINDOW (dialog1), "breakpoints", "Anjuta");
  gnome_dialog_close_hides (GNOME_DIALOG (dialog1), TRUE);

  dialog_vbox1 = GNOME_DIALOG (dialog1)->vbox;
  gtk_widget_show (dialog_vbox1);

  frame1 = gtk_frame_new (NULL);
  gtk_widget_show (frame1);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), frame1, TRUE, TRUE, 0);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolledwindow1);
  gtk_container_add (GTK_CONTAINER (frame1), scrolledwindow1);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  clist1 = gtk_clist_new (6);
  gtk_widget_show (clist1);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), clist1);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 0, 52);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 1, 91);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 2, 35);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 3, 136);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 4, 44);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 5, 80);
  gtk_clist_set_selection_mode (GTK_CLIST (clist1), GTK_SELECTION_SINGLE);
  gtk_clist_column_titles_show (GTK_CLIST (clist1));
  gtk_clist_set_column_auto_resize (GTK_CLIST(clist1), 5, TRUE);

  label1 = gtk_label_new (_("Enabled"));
  gtk_widget_show (label1);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 0, label1);

  label2 = gtk_label_new (_("File"));
  gtk_widget_show (label2);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 1, label2);

  label3 = gtk_label_new (_("Line"));
  gtk_widget_show (label3);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 2, label3);

  label4 = gtk_label_new (_("Function"));
  gtk_widget_show (label4);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 3, label4);

  label5 = gtk_label_new (_("Pass"));
  gtk_widget_show (label5);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 4, label5);

  label6 = gtk_label_new (_("Condition"));
  gtk_widget_show (label6);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 5, label6);

  frame2 = gtk_frame_new (NULL);
  gtk_widget_show (frame2);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), frame2, FALSE, FALSE, 0);

  table1 = gtk_table_new (2, 4, TRUE);
  gtk_widget_show (table1);
  gtk_container_add (GTK_CONTAINER (frame2), table1);

  button4 = gtk_button_new_with_label (_("Add"));
  gtk_widget_show (button4);
  gtk_table_attach (GTK_TABLE (table1), button4, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (button4), 3);

  button5 = gtk_button_new_with_label (_("Edit"));
  gtk_widget_show (button5);
  gtk_table_attach (GTK_TABLE (table1), button5, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (button5), 3);

  button6 = gtk_button_new_with_label (_("Toggle Enable"));
  gtk_widget_show (button6);
  gtk_table_attach (GTK_TABLE (table1), button6, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (button6), 3);

  button7 = gtk_button_new_with_label (_("View Source"));
  gtk_widget_show (button7);
  gtk_table_attach (GTK_TABLE (table1), button7, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (button7), 3);

  button9 = gtk_button_new_with_label (_("Disable All"));
  gtk_widget_show (button9);
  gtk_table_attach (GTK_TABLE (table1), button9, 2, 3, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (button9), 3);

  button8 = gtk_button_new_with_label (_("Enable All"));
  gtk_widget_show (button8);
  gtk_table_attach (GTK_TABLE (table1), button8, 2, 3, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (button8), 3);

  button11 = gtk_button_new_with_label (_("Delete All"));
  gtk_widget_show (button11);
  gtk_table_attach (GTK_TABLE (table1), button11, 3, 4, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (button11), 3);

  button10 = gtk_button_new_with_label (_("Delete"));
  gtk_widget_show (button10);
  gtk_table_attach (GTK_TABLE (table1), button10, 3, 4, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (button10), 3);

  dialog_action_area1 = GNOME_DIALOG (dialog1)->action_area;
  gtk_widget_show (dialog_action_area1);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_SPREAD);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area1), 8);

  gnome_dialog_append_button (GNOME_DIALOG (dialog1), GNOME_STOCK_BUTTON_HELP);
  button1 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
  gtk_widget_show (button1);
  GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);

  gnome_dialog_append_button (GNOME_DIALOG (dialog1), GNOME_STOCK_BUTTON_CLOSE);
  button2 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
  gtk_widget_show (button2);
  GTK_WIDGET_SET_FLAGS (button2, GTK_CAN_DEFAULT);

  gtk_widget_grab_focus (clist1);

  gtk_accel_group_attach( app->accel_group, GTK_OBJECT(dialog1));

  gtk_signal_connect (GTK_OBJECT (button7), "clicked",
                      GTK_SIGNAL_FUNC (on_bk_view_clicked),
                      bd);
  gtk_signal_connect (GTK_OBJECT (button5), "clicked",
                      GTK_SIGNAL_FUNC (on_bk_edit_clicked),
                      bd);
  gtk_signal_connect (GTK_OBJECT (button10), "clicked",
                      GTK_SIGNAL_FUNC (on_bk_delete_clicked),
                      bd);
  gtk_signal_connect (GTK_OBJECT (button11), "clicked",
                      GTK_SIGNAL_FUNC (on_bk_delete_all_clicked),
                      bd);
  gtk_signal_connect (GTK_OBJECT (button6), "clicked",
                      GTK_SIGNAL_FUNC (on_bk_toggle_enable_clicked),
                      bd);
  gtk_signal_connect (GTK_OBJECT (button8), "clicked",
                      GTK_SIGNAL_FUNC (on_bk_enable_all_clicked),
                      bd);
  gtk_signal_connect (GTK_OBJECT (button9), "clicked",
                      GTK_SIGNAL_FUNC (on_bk_disable_all_clicked),
                      bd);
   gtk_signal_connect (GTK_OBJECT (button4), "clicked",
                      GTK_SIGNAL_FUNC (on_bk_add_breakpoint_clicked),
                      bd);
  gtk_signal_connect (GTK_OBJECT (clist1), "select_row",
                      GTK_SIGNAL_FUNC (on_bk_clist_select_row),
                      bd);
  gtk_signal_connect (GTK_OBJECT (clist1), "unselect_row",
                      GTK_SIGNAL_FUNC (on_bk_clist_unselect_row),
                      bd);
  gtk_signal_connect (GTK_OBJECT (button1), "clicked",
                      GTK_SIGNAL_FUNC (on_bk_help_clicked),
                      bd);
  gtk_signal_connect (GTK_OBJECT (button2), "clicked",
                      GTK_SIGNAL_FUNC (on_bk_close_clicked),
                      bd);
  gtk_signal_connect (GTK_OBJECT (dialog1), "close",
                      GTK_SIGNAL_FUNC (on_breakpoints_close),
                      bd);



  bd->widgets.window = dialog1; gtk_widget_ref(dialog1);
  bd->widgets.clist = clist1; gtk_widget_ref(clist1);
  bd->widgets.button_add = button4; gtk_widget_ref(button4);
  bd->widgets.button_edit = button5; gtk_widget_ref(button5);
  bd->widgets.button_toggle = button6; gtk_widget_ref(button6);
  bd->widgets.button_view = button7; gtk_widget_ref(button7);
  bd->widgets.button_enable_all = button8; gtk_widget_ref(button8);
  bd->widgets.button_disable_all = button9; gtk_widget_ref(button9);
  bd->widgets.button_delete = button10; gtk_widget_ref(button10);
  bd->widgets.button_delete_all = button11; gtk_widget_ref(button11);
}

GtkWidget*
create_bk_add_dialog (BreakpointsDBase *bd)
{
  GtkWidget *dialog1;
  GtkWidget *dialog_vbox1;
  GtkWidget *table1;
  GtkWidget *entry1;
  GtkWidget *entry2;
  GtkWidget *entry3;
  GtkWidget *label8;
  GtkWidget *label9;
  GtkWidget *label10;
  GtkWidget *dialog_action_area1;
  GtkWidget *button12;
  GtkWidget *button13;
  GtkWidget *button14;
  struct BkItemData *data;

  dialog1 = gnome_dialog_new (_("Add Breakpoint"), NULL);
  gtk_window_set_transient_for (GTK_WINDOW(dialog1), GTK_WINDOW(bd->widgets.window));
  gtk_window_set_position (GTK_WINDOW (dialog1), GTK_WIN_POS_MOUSE);
  gtk_window_set_policy (GTK_WINDOW (dialog1), FALSE, FALSE, FALSE);
  gtk_window_set_wmclass (GTK_WINDOW (dialog1), "bk_add", "Anjuta");

  dialog_vbox1 = GNOME_DIALOG (dialog1)->vbox;
  gtk_widget_show (dialog_vbox1);

  table1 = gtk_table_new (3, 2, FALSE);
  gtk_widget_show (table1);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), table1, TRUE, TRUE, 0);
  gtk_table_set_row_spacings (GTK_TABLE (table1), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table1), 5);

  entry1 = gtk_entry_new ();
  entry_set_text_n_select(entry1, bd->loc_history, TRUE);
  gtk_widget_show (entry1);
  gtk_table_attach (GTK_TABLE (table1), entry1, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  entry2 = gtk_entry_new ();
  entry_set_text_n_select(entry2, bd->cond_history, FALSE);
  gtk_widget_show (entry2);
  gtk_table_attach (GTK_TABLE (table1), entry2, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  entry3 = gtk_entry_new ();
  entry_set_text_n_select(entry3, bd->pass_history, FALSE);
  gtk_widget_show (entry3);
  gtk_table_attach (GTK_TABLE (table1), entry3, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label8 = gtk_label_new (_("Location:"));
  gtk_widget_show (label8);
  gtk_table_attach (GTK_TABLE (table1), label8, 0, 1, 0, 1,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (label8), GTK_JUSTIFY_LEFT);

  label9 = gtk_label_new (_("Condition:"));
  gtk_widget_show (label9);
  gtk_table_attach (GTK_TABLE (table1), label9, 0, 1, 1, 2,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (label9), GTK_JUSTIFY_LEFT);

  label10 = gtk_label_new (_("Pass:"));
  gtk_widget_show (label10);
  gtk_table_attach (GTK_TABLE (table1), label10, 0, 1, 2, 3,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (label10), GTK_JUSTIFY_LEFT);

  dialog_action_area1 = GNOME_DIALOG (dialog1)->action_area;
  gtk_widget_show (dialog_action_area1);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area1), 8);

  gnome_dialog_append_button (GNOME_DIALOG (dialog1), GNOME_STOCK_BUTTON_HELP);
  button12 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
  gtk_widget_show (button12);
  GTK_WIDGET_SET_FLAGS (button12, GTK_CAN_DEFAULT);

  gnome_dialog_append_button (GNOME_DIALOG (dialog1), GNOME_STOCK_BUTTON_CANCEL);
  button13 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
  gtk_widget_show (button13);
  GTK_WIDGET_SET_FLAGS (button13, GTK_CAN_DEFAULT);

  gnome_dialog_append_button (GNOME_DIALOG (dialog1), GNOME_STOCK_BUTTON_OK);
  button14 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
  gtk_widget_show (button14);
  GTK_WIDGET_SET_FLAGS (button14, GTK_CAN_DEFAULT);

  gtk_accel_group_attach( app->accel_group, GTK_OBJECT(dialog1));

  data = g_malloc(sizeof(struct BkItemData));
  data->dialog = dialog1; gtk_widget_ref (dialog1);
  data->loc = entry1; gtk_widget_ref (entry1);
  data->cond = entry2; gtk_widget_ref (entry2);
  data->pass = entry3; gtk_widget_ref (entry3);
  data->bd = bd;

  gtk_signal_connect (GTK_OBJECT (dialog1), "delete_event",
                      GTK_SIGNAL_FUNC (on_bk_item_add_delete_event),
                      data);
  gtk_signal_connect (GTK_OBJECT (button12), "clicked",
                      GTK_SIGNAL_FUNC (on_bk_item_add_help_clicked),
                      data);
  gtk_signal_connect (GTK_OBJECT (button13), "clicked",
                      GTK_SIGNAL_FUNC (on_bk_item_add_cancel_clicked),
                      data);
  gtk_signal_connect (GTK_OBJECT (button14), "clicked",
                      GTK_SIGNAL_FUNC (on_bk_item_add_ok_clicked),
                     data);
  gtk_signal_connect (GTK_OBJECT (entry1), "activate",
                      GTK_SIGNAL_FUNC (on_bk_item_add_ok_clicked),
                     data);

  gtk_widget_grab_focus(entry1);

  return dialog1;
}

GtkWidget*
create_bk_edit_dialog(BreakpointsDBase * bd)
{
  GtkWidget *dialog1;
  GtkWidget *dialog_vbox1;
  GtkWidget *table1;
  GtkWidget *entry1;
  GtkWidget *entry2;
  GtkWidget *entry3;
  GtkWidget *label8;
  GtkWidget *label9;
  GtkWidget *label10;
  GtkWidget *dialog_action_area1;
  GtkWidget *button12;
  GtkWidget *button13;
  GtkWidget *button14;
  struct BkItemData *data;
  BreakpointItem *bi;
  gchar *buff;

  if(bd->current_index < 0) return NULL;
  bi = g_list_nth_data(bd->breakpoints, bd->current_index);

  dialog1 = gnome_dialog_new (_("Add Breakpoint"), NULL);
  gtk_window_set_transient_for (GTK_WINDOW(dialog1), GTK_WINDOW(bd->widgets.window));
  gtk_window_set_position (GTK_WINDOW (dialog1), GTK_WIN_POS_CENTER);
  gtk_window_set_policy (GTK_WINDOW (dialog1), FALSE, FALSE, FALSE);
  gtk_window_set_wmclass (GTK_WINDOW (dialog1), "bk_edit", "Anjuta");

  dialog_vbox1 = GNOME_DIALOG (dialog1)->vbox;
  gtk_widget_show (dialog_vbox1);

  table1 = gtk_table_new (3, 2, FALSE);
  gtk_widget_show (table1);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), table1, TRUE, TRUE, 0);
  gtk_table_set_row_spacings (GTK_TABLE (table1), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table1), 5);

  entry1 = gtk_entry_new ();
  gtk_widget_show (entry1);
  gtk_table_attach (GTK_TABLE (table1), entry1, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  if(bi->line >= 0) buff = g_strdup_printf("%s:%d", bi->file, bi->line);
  else  buff = g_strdup_printf("%s", bi->file);

  gtk_entry_set_text(GTK_ENTRY(entry1), buff);
  g_free(buff);

  entry2 = gtk_entry_new ();
  gtk_widget_show (entry2);
  gtk_table_attach (GTK_TABLE (table1), entry2, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_entry_set_text(GTK_ENTRY(entry2), bi->condition);

  entry3 = gtk_entry_new ();
  gtk_widget_show (entry3);
  gtk_table_attach (GTK_TABLE (table1), entry3, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  buff = g_strdup_printf("%d", bi->pass);
  gtk_entry_set_text(GTK_ENTRY(entry3), buff);
  g_free(buff);

  label8 = gtk_label_new (_("Location:"));
  gtk_widget_show (label8);
  gtk_table_attach (GTK_TABLE (table1), label8, 0, 1, 0, 1,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (label8), GTK_JUSTIFY_LEFT);

  label9 = gtk_label_new (_("Condition:"));
  gtk_widget_show (label9);
  gtk_table_attach (GTK_TABLE (table1), label9, 0, 1, 1, 2,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (label9), GTK_JUSTIFY_LEFT);

  label10 = gtk_label_new (_("Pass:"));
  gtk_widget_show (label10);
  gtk_table_attach (GTK_TABLE (table1), label10, 0, 1, 2, 3,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (label10), GTK_JUSTIFY_LEFT);

  dialog_action_area1 = GNOME_DIALOG (dialog1)->action_area;
  gtk_widget_show (dialog_action_area1);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area1), 8);

  gnome_dialog_append_button (GNOME_DIALOG (dialog1), GNOME_STOCK_BUTTON_HELP);
  button12 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
  gtk_widget_show (button12);
  GTK_WIDGET_SET_FLAGS (button12, GTK_CAN_DEFAULT);

  gnome_dialog_append_button (GNOME_DIALOG (dialog1), GNOME_STOCK_BUTTON_CANCEL);
  button13 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
  gtk_widget_show (button13);
  GTK_WIDGET_SET_FLAGS (button13, GTK_CAN_DEFAULT);

  gnome_dialog_append_button (GNOME_DIALOG (dialog1), GNOME_STOCK_BUTTON_OK);
  button14 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
  gtk_widget_show (button14);
  GTK_WIDGET_SET_FLAGS (button14, GTK_CAN_DEFAULT);

  gtk_accel_group_attach( app->accel_group, GTK_OBJECT(dialog1));

  data = g_malloc(sizeof(struct BkItemData));
  data->dialog = dialog1; gtk_widget_ref (dialog1);
  data->loc = entry1; gtk_widget_ref(entry1);
  data->cond = entry2; gtk_widget_ref(entry2);
  data->pass = entry3; gtk_widget_ref(entry3);
  data->bd = bd;

  gtk_signal_connect (GTK_OBJECT (dialog1), "delete_event",
                      GTK_SIGNAL_FUNC (on_bk_item_add_delete_event),
                      data);
  gtk_signal_connect (GTK_OBJECT (button12), "clicked",
                      GTK_SIGNAL_FUNC (on_bk_item_edit_help_clicked),
                      data);
  gtk_signal_connect (GTK_OBJECT (button13), "clicked",
                      GTK_SIGNAL_FUNC (on_bk_item_add_cancel_clicked),
                      data);
  gtk_signal_connect (GTK_OBJECT (button14), "clicked",
                      GTK_SIGNAL_FUNC (on_bk_item_edit_ok_clicked),
                     data);
  gtk_signal_connect (GTK_OBJECT (entry1), "activate",
                      GTK_SIGNAL_FUNC (on_bk_item_edit_ok_clicked),
                     data);

  gtk_widget_grab_focus(entry1);

  return dialog1;
}
