/*
    signals_gui.c
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
#include <libgnomeui/gnome-window-icon.h>

#include "signals.h"
#include "signals_cbs.h"
#include "resources.h"

GtkWidget* create_signals_menu (void);

static GnomeUIInfo signals_menu_uiinfo[] =
{
  {
    GNOME_APP_UI_ITEM, N_("Modify Signal"),
    NULL,
    on_signals_modify_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  {
    GNOME_APP_UI_ITEM, N_("Send to process"),
    NULL,
    on_signals_send_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  {
    GNOME_APP_UI_ITEM, N_("Update"),
    NULL,
    on_signals_update_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  GNOMEUIINFO_SEPARATOR,
  {
    GNOME_APP_UI_ITEM, N_("Help"),
    NULL,
    on_signals_help_activate, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  GNOMEUIINFO_END
};

GtkWidget*
create_signals_menu ()
{
  GtkWidget *signals_menu;

  signals_menu = gtk_menu_new ();
  gnome_app_fill_menu (GTK_MENU_SHELL (signals_menu), signals_menu_uiinfo,
                       NULL, FALSE, 0);
  return signals_menu;
}

void
create_signals_gui(Signals *cr)
{
  GtkWidget *window3;
  GtkWidget *scrolledwindow4;
  GtkWidget *clist4;
  GtkWidget *label6, *label7, *label8, *label9, *label10;

  window3 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_usize (window3, 170, -2);
  gtk_window_set_title (GTK_WINDOW (window3), _("Kernel signals"));
  gtk_window_set_wmclass (GTK_WINDOW (window3), "signals", "Anjuta");
  gtk_window_set_default_size (GTK_WINDOW (window3), 240, 230);
  gnome_window_icon_set_from_default(GTK_WINDOW(window3));

  scrolledwindow4 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolledwindow4);
  gtk_container_add (GTK_CONTAINER (window3), scrolledwindow4);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow4), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  clist4 = gtk_clist_new (5);
  gtk_widget_show (clist4);
  gtk_container_add (GTK_CONTAINER (scrolledwindow4), clist4);
  gtk_clist_set_column_width (GTK_CLIST (clist4), 0, 100);
  gtk_clist_set_column_width (GTK_CLIST (clist4), 1, 50);
  gtk_clist_set_column_width (GTK_CLIST (clist4), 2, 50);
  gtk_clist_set_column_width (GTK_CLIST (clist4), 3, 50);
  gtk_clist_set_column_width (GTK_CLIST (clist4), 4, 80);
  gtk_clist_set_selection_mode (GTK_CLIST (clist4), GTK_SELECTION_BROWSE);
  gtk_clist_column_titles_show (GTK_CLIST (clist4));
  gtk_clist_set_column_auto_resize (GTK_CLIST(clist4), 4, TRUE);

  label6 = gtk_label_new (_("Signal"));
  gtk_widget_show (label6);
  gtk_clist_set_column_widget (GTK_CLIST (clist4), 0, label6);

  label7 = gtk_label_new (_("Stop"));
  gtk_widget_show (label7);
  gtk_clist_set_column_widget (GTK_CLIST (clist4), 1, label7);

  label8 = gtk_label_new (_("Print"));
  gtk_widget_show (label8);
  gtk_clist_set_column_widget (GTK_CLIST (clist4), 2, label8);

  label9 = gtk_label_new (_("Pass"));
  gtk_widget_show (label9);
  gtk_clist_set_column_widget (GTK_CLIST (clist4), 3, label9);

  label10 = gtk_label_new (_("Description"));
  gtk_widget_show (label10);
  gtk_clist_set_column_widget (GTK_CLIST (clist4), 4, label10);

  gtk_signal_connect (GTK_OBJECT (window3), "delete_event",
                             GTK_SIGNAL_FUNC (on_signals_delete_event), cr);

  gtk_signal_connect (GTK_OBJECT (clist4), "event",
                      GTK_SIGNAL_FUNC (on_signals_event),
                      cr);
  gtk_signal_connect (GTK_OBJECT (clist4), "select_row",
                      GTK_SIGNAL_FUNC (on_signals_clist_select_row),
                      cr);

  cr->widgets.window = window3;
  cr->widgets.clist = clist4;
  cr->widgets.menu = create_signals_menu();
  cr->widgets.menu_modify = signals_menu_uiinfo[0].widget;
  cr->widgets.menu_signal = signals_menu_uiinfo[1].widget;
  cr->widgets.menu_update = signals_menu_uiinfo[2].widget;

  gtk_widget_ref(cr->widgets.window);
  gtk_widget_ref(cr->widgets.clist);
  gtk_widget_ref(cr->widgets.menu);
  gtk_widget_ref(cr->widgets.menu_modify);
  gtk_widget_ref(cr->widgets.menu_signal);
  gtk_widget_ref(cr->widgets.menu_update);
}

GtkWidget*
create_signals_set_dialog (Signals *s)
{
  GtkWidget *dialog1;
  GtkWidget *dialog_vbox1;
  GtkWidget *hbox1;
  GtkWidget *label1;
  GtkWidget *label2;
  GtkWidget *hbox2;
  GtkWidget *label3;
  GtkWidget *label4;
  GtkWidget *hseparator1;
  GtkWidget *hbox4;
  GtkWidget *label5;
  GtkWidget *label6;
  GtkWidget *label7;
  GtkWidget *hbox3;
  GtkWidget *togglebutton1;
  GtkWidget *togglebutton2;
  GtkWidget *togglebutton3;
  GtkWidget *dialog_action_area1;
  GtkWidget *button1;
  GtkWidget *button2;
  GtkWidget *button3;
  gchar *row[5];

  if(s->index < 0) return NULL;
  gtk_clist_get_text(GTK_CLIST(s->widgets.clist), s->index,0, &row[0]);
  gtk_clist_get_text(GTK_CLIST(s->widgets.clist), s->index,1, &row[1]);
  gtk_clist_get_text(GTK_CLIST(s->widgets.clist), s->index,2, &row[2]);
  gtk_clist_get_text(GTK_CLIST(s->widgets.clist), s->index,3, &row[3]);
  gtk_clist_get_text(GTK_CLIST(s->widgets.clist), s->index,4, &row[4]);
  s->signal = row[0];

  if(strcasecmp(row[1], "Yes")==0)
       s->stop = TRUE;
  else
       s->stop = FALSE;

  if(strcasecmp(row[2], "Yes")==0)
       s->print = TRUE;
  else
       s->print = FALSE;

  if(strcasecmp(row[3], "Yes")==0)
       s->pass = TRUE;
  else
       s->pass = FALSE;

  dialog1 = gnome_dialog_new (_("Set Signal Property"), NULL);
  GTK_WINDOW (dialog1)->type = GTK_WINDOW_DIALOG;
  gtk_window_set_position (GTK_WINDOW (dialog1), GTK_WIN_POS_MOUSE);
  gtk_window_set_policy (GTK_WINDOW (dialog1), FALSE, FALSE, FALSE);
  gtk_window_set_wmclass (GTK_WINDOW (dialog1), "set_signal", "Anjuta");
  gnome_dialog_set_close (GNOME_DIALOG (dialog1), TRUE);

  dialog_vbox1 = GNOME_DIALOG (dialog1)->vbox;
  gtk_widget_show (dialog_vbox1);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), hbox1, TRUE, TRUE, 0);

  label1 = gtk_label_new ("Signal: ");
  gtk_widget_show (label1);
  gtk_box_pack_start (GTK_BOX (hbox1), label1, FALSE, FALSE, 0);

  label2 = gtk_label_new (row[0]);
  gtk_widget_show (label2);
  gtk_box_pack_start (GTK_BOX (hbox1), label2, FALSE, FALSE, 0);

  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox2);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), hbox2, TRUE, TRUE, 0);

  label3 = gtk_label_new ("Description: ");
  gtk_widget_show (label3);
  gtk_box_pack_start (GTK_BOX (hbox2), label3, FALSE, FALSE, 0);

  label4 = gtk_label_new (row[4]);
  gtk_widget_show (label4);
  gtk_box_pack_start (GTK_BOX (hbox2), label4, FALSE, FALSE, 0);

  hseparator1 = gtk_hseparator_new ();
  gtk_widget_show (hseparator1);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), hseparator1, TRUE, TRUE, 0);

  hbox4 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox4);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), hbox4, TRUE, TRUE, 0);

  label5 = gtk_label_new (_(" Stop: "));
  gtk_widget_show (label5);
  gtk_box_pack_start (GTK_BOX (hbox4), label5, TRUE, TRUE, 0);

  label6 = gtk_label_new (_(" Print: "));
  gtk_widget_show (label6);
  gtk_box_pack_start (GTK_BOX (hbox4), label6, TRUE, TRUE, 0);

  label7 = gtk_label_new (_("Pass:"));
  gtk_widget_show (label7);
  gtk_box_pack_start (GTK_BOX (hbox4), label7, TRUE, TRUE, 0);

  hbox3 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox3);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), hbox3, TRUE, TRUE, 0);

  togglebutton1 = gtk_toggle_button_new_with_label (row[1]);
  gtk_widget_show (togglebutton1);
  gtk_box_pack_start (GTK_BOX (hbox3), togglebutton1, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (togglebutton1), 3);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(togglebutton1), s->stop);

  togglebutton2 = gtk_toggle_button_new_with_label (row[2]);
  gtk_widget_show (togglebutton2);
  gtk_box_pack_start (GTK_BOX (hbox3), togglebutton2, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (togglebutton2), 3);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(togglebutton2), s->print);

  togglebutton3 = gtk_toggle_button_new_with_label (row[3]);
  gtk_widget_show (togglebutton3);
  gtk_box_pack_start (GTK_BOX (hbox3), togglebutton3, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (togglebutton3), 3);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(togglebutton3), s->pass);

  dialog_action_area1 = GNOME_DIALOG (dialog1)->action_area;
  gtk_widget_show (dialog_action_area1);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_SPREAD);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area1), 8);

  gnome_dialog_append_button (GNOME_DIALOG (dialog1), GNOME_STOCK_BUTTON_HELP);
  button1 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
  gtk_widget_show (button1);
  GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);

  gnome_dialog_append_button (GNOME_DIALOG (dialog1), GNOME_STOCK_BUTTON_OK);
  button2 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
  gtk_widget_show (button2);
  GTK_WIDGET_SET_FLAGS (button2, GTK_CAN_DEFAULT);

  gnome_dialog_append_button (GNOME_DIALOG (dialog1), GNOME_STOCK_BUTTON_CANCEL);
  button3 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
  gtk_widget_show (button3);
  GTK_WIDGET_SET_FLAGS (button3, GTK_CAN_DEFAULT);

  gtk_signal_connect (GTK_OBJECT (togglebutton1), "toggled",
                      GTK_SIGNAL_FUNC (on_signals_togglebutton1_toggled),
                      s);
  gtk_signal_connect (GTK_OBJECT (togglebutton2), "toggled",
                      GTK_SIGNAL_FUNC (on_signals_togglebutton2_toggled),
                      s);
  gtk_signal_connect (GTK_OBJECT (togglebutton3), "toggled",
                      GTK_SIGNAL_FUNC (on_signals_togglebutton3_toggled),
                      s);
  gtk_signal_connect (GTK_OBJECT (button1), "clicked",
                      GTK_SIGNAL_FUNC (on_signals_set_help_clicked),
                      s);
  gtk_signal_connect (GTK_OBJECT (button2), "clicked",
                      GTK_SIGNAL_FUNC (on_signals_set_ok_clicked),
                      s);

  return dialog1;
}

