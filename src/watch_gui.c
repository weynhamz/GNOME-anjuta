/*
    watch_gui.c
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

#include <gtk/gtk.h>
#include <gnome.h>
#include <libgnomeui/gnome-window-icon.h>

#include "anjuta.h"
#include "watch.h"
#include "debugger.h"
#include "watch_cbs.h"
#include "utilities.h"



gchar *eval_entry_history;
gchar *expr_watch_entry_history;

static GnomeUIInfo watch_menu_uiinfo[] = {
  {
   GNOME_APP_UI_ITEM, N_("Add expression"),
   NULL,
   on_watch_add_activate, NULL, NULL,
   GNOME_APP_PIXMAP_NONE, NULL,
   0, 0, NULL},
  {
   GNOME_APP_UI_ITEM, N_("Remove"),
   NULL,
   on_watch_remove_activate, NULL, NULL,
   GNOME_APP_PIXMAP_NONE, NULL,
   0, 0, NULL},
  {
   GNOME_APP_UI_ITEM, N_("Update all"),
   NULL,
   on_watch_update_activate, NULL, NULL,
   GNOME_APP_PIXMAP_NONE, NULL,
   0, 0, NULL},
  {
   GNOME_APP_UI_ITEM, N_("Clear"),
   NULL,
   on_watch_clear_activate, NULL, NULL,
   GNOME_APP_PIXMAP_NONE, NULL,
   0, 0, NULL},
  {
   GNOME_APP_UI_ITEM, N_("Toggle enable"),
   NULL,
   on_watch_toggle_activate, NULL, NULL,
   GNOME_APP_PIXMAP_NONE, NULL,
   0, 0, NULL},
  {
   GNOME_APP_UI_ITEM, N_("Change item"),
   NULL,
   on_watch_change_activate, NULL, NULL,
   GNOME_APP_PIXMAP_NONE, NULL,
   0, 0, NULL},
  GNOMEUIINFO_SEPARATOR,
  {
   GNOME_APP_UI_ITEM, N_("Help"),
   NULL,
   on_watch_help_activate, NULL, NULL,
   GNOME_APP_PIXMAP_NONE, NULL,
   0, 0, NULL},
  GNOMEUIINFO_END
};

GtkWidget *
create_watch_menu ()
{
  GtkWidget *watch_menu;

  watch_menu = gtk_menu_new ();
  gnome_app_fill_menu (GTK_MENU_SHELL (watch_menu), watch_menu_uiinfo,
		       NULL, FALSE, 0);

  return watch_menu;
}

void
create_expr_watch_gui (ExprWatch * ew)
{
  GtkWidget *clist1;
  GtkWidget *label1;
  GtkWidget *label2;
  GtkTreeModel* model;
  GtkTreeSelection *selection;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;	
	
  model = GTK_TREE_MODEL(gtk_list_store_new(WATCH_N_COLUMNS,
											GTK_TYPE_STRING,
											GTK_TYPE_STRING));

  ew->widgets.clist = gtk_tree_view_new_with_model(model);
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ew->widgets.clist));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_object_unref (G_OBJECT (model));

  /* Columns */
  column = gtk_tree_view_column_new ();
  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_add_attribute (column, renderer, "text",	WATCH_VARIABLE_COLUMN);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_title (column, _("Variable"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (ew->widgets.clist), column);
  gtk_tree_view_set_expander_column (GTK_TREE_VIEW (ew->widgets.clist), column);

  column = gtk_tree_view_column_new ();
  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_add_attribute (column, renderer, "text", WATCH_VALUE_COLUMN);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_title (column, _("Value"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (ew->widgets.clist), column);

  g_signal_connect (ew->widgets.clist, "event", G_CALLBACK (on_watch_event), ew);

  ew->widgets.menu = create_watch_menu ();
  ew->widgets.menu_add = watch_menu_uiinfo[0].widget;
  ew->widgets.menu_remove = watch_menu_uiinfo[1].widget;
  ew->widgets.menu_update = watch_menu_uiinfo[2].widget;
  ew->widgets.menu_clear = watch_menu_uiinfo[3].widget;
  ew->widgets.menu_toggle = watch_menu_uiinfo[4].widget;
  ew->widgets.menu_change = watch_menu_uiinfo[5].widget;

  gtk_widget_ref (ew->widgets.clist);
  gtk_widget_ref (ew->widgets.menu_add);
  gtk_widget_ref (ew->widgets.menu_remove);
  gtk_widget_ref (ew->widgets.menu_clear);
  gtk_widget_ref (ew->widgets.menu_update);
  gtk_widget_ref (ew->widgets.menu_toggle);
  gtk_widget_ref (ew->widgets.menu_change);
  gtk_widget_ref (ew->widgets.menu);
}

GtkWidget *
create_watch_add_dialog ()
{
  GtkWidget *dialog3;
  GtkWidget *dialog_vbox3;
  GtkWidget *label15;
  GtkWidget *entry7;
  GtkWidget *dialog_action_area3;
  GtkWidget *button18;
  GtkWidget *button19;
  GtkWidget *button20;

  dialog3 = gnome_dialog_new (_("Add Watch Expression"), NULL);

  gtk_window_set_position (GTK_WINDOW (dialog3), GTK_WIN_POS_MOUSE);
  gtk_window_set_policy (GTK_WINDOW (dialog3), FALSE, FALSE, FALSE);
  gtk_window_set_wmclass (GTK_WINDOW (dialog3), "watch_add", "Anjuta");
  gnome_dialog_set_close (GNOME_DIALOG (dialog3), TRUE);

  dialog_vbox3 = GNOME_DIALOG (dialog3)->vbox;
  gtk_widget_show (dialog_vbox3);

  label15 = gtk_label_new (_("Add expression to watch"));
  gtk_widget_show (label15);
  gtk_box_pack_start (GTK_BOX (dialog_vbox3), label15, FALSE, FALSE, 0);

  entry7 = gtk_entry_new ();
  entry_set_text_n_select (entry7, expr_watch_entry_history, TRUE);
  gtk_widget_show (entry7);
  gtk_box_pack_start (GTK_BOX (dialog_vbox3), entry7, FALSE, FALSE, 0);

  dialog_action_area3 = GNOME_DIALOG (dialog3)->action_area;
  gtk_widget_show (dialog_action_area3);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area3),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area3), 8);

  gnome_dialog_append_button (GNOME_DIALOG (dialog3),
			      GNOME_STOCK_BUTTON_HELP);
  button18 = g_list_last (GNOME_DIALOG (dialog3)->buttons)->data;
  gtk_widget_show (button18);
  GTK_WIDGET_SET_FLAGS (button18, GTK_CAN_DEFAULT);

  gnome_dialog_append_button (GNOME_DIALOG (dialog3), GNOME_STOCK_BUTTON_CANCEL);
  button19 = g_list_last (GNOME_DIALOG (dialog3)->buttons)->data;
  gtk_widget_show (button19);
  GTK_WIDGET_SET_FLAGS (button19, GTK_CAN_DEFAULT);

  gnome_dialog_append_button (GNOME_DIALOG (dialog3),
			      GNOME_STOCK_BUTTON_OK);
  button20 = g_list_last (GNOME_DIALOG (dialog3)->buttons)->data;
  gtk_widget_show (button20);
  GTK_WIDGET_SET_FLAGS (button20, GTK_CAN_DEFAULT);

  gtk_signal_connect (GTK_OBJECT (button18), "clicked",
		      GTK_SIGNAL_FUNC (on_ew_add_help_clicked), NULL);
  gtk_signal_connect (GTK_OBJECT (button20), "clicked",
		      GTK_SIGNAL_FUNC (on_ew_add_ok_clicked), entry7);
  gtk_signal_connect (GTK_OBJECT (entry7), "activate",
		      GTK_SIGNAL_FUNC (on_ew_entry_activate), dialog3);

  gtk_widget_grab_focus(entry7);
  return dialog3;
}

GtkWidget *
create_watch_change_dialog ()
{
  GtkWidget *dialog3;
  GtkWidget *dialog_vbox3;
  GtkWidget *label15;
  GtkWidget *entry7;
  GtkWidget *dialog_action_area3;
  GtkWidget *button18;
  GtkWidget *button19;
  GtkWidget *button20;

  dialog3 = gnome_dialog_new (_("Modify Watch Expression"), NULL);

  gtk_window_set_position (GTK_WINDOW (dialog3), GTK_WIN_POS_MOUSE);
  gtk_window_set_policy (GTK_WINDOW (dialog3), FALSE, FALSE, FALSE);
  gtk_window_set_wmclass (GTK_WINDOW (dialog3), "watch_add", "Anjuta");
  gnome_dialog_set_close (GNOME_DIALOG (dialog3), TRUE);

  dialog_vbox3 = GNOME_DIALOG (dialog3)->vbox;
  gtk_widget_show (dialog_vbox3);

  label15 = gtk_label_new (_("Modify watched expression"));
  gtk_widget_show (label15);
  gtk_box_pack_start (GTK_BOX (dialog_vbox3), label15, FALSE, FALSE, 0);

  entry7 = gtk_entry_new ();
  entry_set_text_n_select (entry7, expr_watch_entry_history, TRUE);
  gtk_widget_show (entry7);
  gtk_box_pack_start (GTK_BOX (dialog_vbox3), entry7, FALSE, FALSE, 0);

  dialog_action_area3 = GNOME_DIALOG (dialog3)->action_area;
  gtk_widget_show (dialog_action_area3);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area3),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area3), 8);

  gnome_dialog_append_button (GNOME_DIALOG (dialog3),
			      GNOME_STOCK_BUTTON_HELP);
  button18 = g_list_last (GNOME_DIALOG (dialog3)->buttons)->data;
  gtk_widget_show (button18);
  GTK_WIDGET_SET_FLAGS (button18, GTK_CAN_DEFAULT);

  gnome_dialog_append_button (GNOME_DIALOG (dialog3), GNOME_STOCK_BUTTON_CANCEL);
  button19 = g_list_last (GNOME_DIALOG (dialog3)->buttons)->data;
  gtk_widget_show (button19);
  GTK_WIDGET_SET_FLAGS (button19, GTK_CAN_DEFAULT);

  gnome_dialog_append_button (GNOME_DIALOG (dialog3),
			      GNOME_STOCK_BUTTON_OK);
  button20 = g_list_last (GNOME_DIALOG (dialog3)->buttons)->data;
  gtk_widget_show (button20);
  GTK_WIDGET_SET_FLAGS (button20, GTK_CAN_DEFAULT);

  gtk_signal_connect (GTK_OBJECT (button18), "clicked",
		      GTK_SIGNAL_FUNC (on_ew_change_help_clicked), NULL);
  gtk_signal_connect (GTK_OBJECT (button20), "clicked",
		      GTK_SIGNAL_FUNC (on_ew_change_ok_clicked), entry7);
  gtk_signal_connect (GTK_OBJECT (entry7), "activate",
		      GTK_SIGNAL_FUNC (on_ew_entry_change_activate), dialog3);

  gtk_widget_grab_focus(entry7);
  return dialog3;
}

GtkWidget *
create_eval_dialog (GtkWindow* parent)
{
  GtkWidget *dialog4;
  GtkWidget *dialog_vbox4;
  GtkWidget *label16;
  GtkWidget *entry8;
  GtkWidget *dialog_action_area4;
  GtkWidget *button21;
  GtkWidget *button22;
  GtkWidget *button23;
  GtkWidget *addWatchButton;

  dialog4 = gnome_dialog_new (_("Inspect/Evaluate"), NULL);
  gtk_window_set_transient_for (GTK_WINDOW(dialog4), GTK_WINDOW(parent));
  gtk_window_set_position (GTK_WINDOW (dialog4), GTK_WIN_POS_MOUSE);
  gtk_window_set_policy (GTK_WINDOW (dialog4), FALSE, FALSE, FALSE);
  gtk_window_set_wmclass (GTK_WINDOW (dialog4), "inspeval", "Anjuta");
  gnome_dialog_set_close (GNOME_DIALOG (dialog4), TRUE);

  dialog_vbox4 = GNOME_DIALOG (dialog4)->vbox;
  gtk_widget_show (dialog_vbox4);

  label16 = gtk_label_new (_("Inspect/Evaluate Expression"));
  gtk_widget_show (label16);
  gtk_box_pack_start (GTK_BOX (dialog_vbox4), label16, FALSE, FALSE, 0);

  entry8 = gtk_entry_new ();
  entry_set_text_n_select (entry8, eval_entry_history, TRUE);
  gtk_widget_show (entry8);
  gtk_box_pack_start (GTK_BOX (dialog_vbox4), entry8, FALSE, FALSE, 0);
  
  addWatchButton = gtk_button_new_with_label(_("Add To Watch"));
  gtk_widget_show(addWatchButton);
  gtk_box_pack_end(GTK_BOX (dialog_vbox4), addWatchButton, FALSE, FALSE, 0);
  
  dialog_action_area4 = GNOME_DIALOG (dialog4)->action_area;
  gtk_widget_show (dialog_action_area4);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area4),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area4), 8);
  gnome_dialog_append_button (GNOME_DIALOG (dialog4),
			      GNOME_STOCK_BUTTON_HELP);
  button21 = g_list_last (GNOME_DIALOG (dialog4)->buttons)->data;
  gtk_widget_show (button21);
  GTK_WIDGET_SET_FLAGS (button21, GTK_CAN_DEFAULT);

  gnome_dialog_append_button (GNOME_DIALOG (dialog4), GNOME_STOCK_BUTTON_CANCEL);
  button22 = g_list_last (GNOME_DIALOG (dialog4)->buttons)->data;
  gtk_widget_show (button22);
  GTK_WIDGET_SET_FLAGS (button22, GTK_CAN_DEFAULT);

  gnome_dialog_append_button (GNOME_DIALOG (dialog4),
			      GNOME_STOCK_BUTTON_OK);
  button23 = g_list_last (GNOME_DIALOG (dialog4)->buttons)->data;
  gtk_widget_show (button23);
  GTK_WIDGET_SET_FLAGS (button23, GTK_CAN_DEFAULT);

  gtk_signal_connect (GTK_OBJECT (button21), "clicked",
		      GTK_SIGNAL_FUNC (on_eval_help_clicked), NULL);
  gtk_signal_connect (GTK_OBJECT (button23), "clicked",
		      GTK_SIGNAL_FUNC (on_eval_ok_clicked), entry8);
  gtk_signal_connect (GTK_OBJECT (entry8), "activate",
		      GTK_SIGNAL_FUNC (on_eval_entry_activate), dialog4);
  gtk_signal_connect (GTK_OBJECT (addWatchButton), "clicked",
		      GTK_SIGNAL_FUNC (on_eval_add_watch), entry8);

  gtk_widget_grab_focus(entry8);
  return dialog4;
}
