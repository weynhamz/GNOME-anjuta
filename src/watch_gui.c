/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
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
#include "watch_gui.h"
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


static GtkWidget *
create_watch_menu (ExprWatch* ew)
{
  int i;
  GtkWidget *watch_menu;
  int entries = sizeof (watch_menu_uiinfo) / sizeof (GnomeUIInfo);

	/* set all user data in the watch menu to the ExprWatch struct parameter */
  for (i = 0 ; i < entries ; i++)
  	watch_menu_uiinfo[i].user_data = ew;

  watch_menu = gtk_menu_new ();

  gnome_app_fill_menu (GTK_MENU_SHELL (watch_menu), watch_menu_uiinfo,
		       NULL, FALSE, 0);

  return watch_menu;
}

void
create_expr_watch_gui (ExprWatch * ew)
{
	GtkTreeModel* model;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;	
	
	model = GTK_TREE_MODEL(gtk_list_store_new(WATCH_N_COLUMNS,
											G_TYPE_STRING,
											G_TYPE_STRING));
	
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
	
	ew->widgets.menu = create_watch_menu (ew);	
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
	
	expr_watch_entry_history = NULL;
	eval_entry_history = NULL;
}

GtkWidget *
create_watch_add_dialog (ExprWatch *ew)
{
	GtkWidget *dialog3;
	GtkWidget *dialog_vbox3;
	GtkWidget *label15;
	GtkWidget *entry7;
	
	dialog3 = gtk_dialog_new_with_buttons (_("Add Watch Expression"),
										   GTK_WINDOW (app->widgets.window),
										   GTK_DIALOG_DESTROY_WITH_PARENT,
										   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
										   GTK_STOCK_OK, GTK_RESPONSE_OK,
										   NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog3), GTK_RESPONSE_OK);
	gtk_window_set_position (GTK_WINDOW (dialog3), GTK_WIN_POS_MOUSE);
	gtk_window_set_resizable (GTK_WINDOW (dialog3), FALSE);
	gtk_window_set_wmclass (GTK_WINDOW (dialog3), "watch_add", "Anjuta");
	
	dialog_vbox3 = GTK_DIALOG (dialog3)->vbox;
	gtk_widget_show (dialog_vbox3);
	
	label15 = gtk_label_new (_("Add expression to watch"));
	gtk_widget_show (label15);
	gtk_box_pack_start (GTK_BOX (dialog_vbox3), label15, FALSE, FALSE, 0);
	
	entry7 = gtk_entry_new ();
	gtk_entry_set_activates_default (GTK_ENTRY (entry7), TRUE);
	entry_set_text_n_select (entry7, expr_watch_entry_history, TRUE);
	gtk_widget_show (entry7);
	gtk_box_pack_start (GTK_BOX (dialog_vbox3), entry7, FALSE, FALSE, 0);
	
	/* We will be passing the entry box as user data. So .. */
	g_object_set_data (G_OBJECT (entry7), "user_data", ew);
	
	g_signal_connect (G_OBJECT (dialog3), "response",
			  G_CALLBACK (on_ew_add_response), entry7);
	
	gtk_widget_grab_focus(entry7);
	return dialog3;
}

GtkWidget *
create_watch_change_dialog (ExprWatch *ew)
{
	GtkWidget *dialog3;
	GtkWidget *dialog_vbox3;
	GtkWidget *label15;
	GtkWidget *entry7;
	
	dialog3 = gtk_dialog_new_with_buttons (_("Modify Watch Expression"),
										   GTK_WINDOW (app->widgets.window),
										   GTK_DIALOG_DESTROY_WITH_PARENT,
										   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
										   GTK_STOCK_OK, GTK_RESPONSE_OK,
										   NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog3), GTK_RESPONSE_OK);
	gtk_window_set_position (GTK_WINDOW (dialog3), GTK_WIN_POS_MOUSE);
	gtk_window_set_resizable (GTK_WINDOW (dialog3), FALSE);
	gtk_window_set_wmclass (GTK_WINDOW (dialog3), "watch_add", "Anjuta");
	
	dialog_vbox3 = GTK_DIALOG (dialog3)->vbox;
	gtk_widget_show (dialog_vbox3);
	
	label15 = gtk_label_new (_("Modify watched expression"));
	gtk_widget_show (label15);
	gtk_box_pack_start (GTK_BOX (dialog_vbox3), label15, FALSE, FALSE, 0);
	
	entry7 = gtk_entry_new ();
	gtk_entry_set_activates_default (GTK_ENTRY (entry7), TRUE);
	entry_set_text_n_select (entry7, expr_watch_entry_history, TRUE);
	gtk_widget_show (entry7);
	gtk_box_pack_start (GTK_BOX (dialog_vbox3), entry7, FALSE, FALSE, 0);
	
	/* We will be passing the entry box as user data. So .. */
	g_object_set_data (G_OBJECT (entry7), "user_data", ew);

	g_signal_connect (G_OBJECT (dialog3), "response",
			  G_CALLBACK (on_ew_change_response), entry7);
	
	gtk_widget_grab_focus(entry7);
	return dialog3;
}

GtkWidget *
create_eval_dialog (GtkWindow* parent, ExprWatch *ew)
{
	GtkWidget *dialog4;
	GtkWidget *dialog_vbox4;
	GtkWidget *label16;
	GtkWidget *entry8;
	GtkWidget *addWatchButton;
	
	dialog4 = gtk_dialog_new_with_buttons (_("Inspect/Evaluate"),
										   GTK_WINDOW (app->widgets.window),
										   GTK_DIALOG_DESTROY_WITH_PARENT,
										   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
										   GTK_STOCK_OK, GTK_RESPONSE_OK,
										   NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog4), GTK_RESPONSE_OK);
	gtk_window_set_transient_for (GTK_WINDOW(dialog4), GTK_WINDOW(parent));
	gtk_window_set_position (GTK_WINDOW (dialog4), GTK_WIN_POS_MOUSE);
	gtk_window_set_resizable (GTK_WINDOW (dialog4), FALSE);
	gtk_window_set_wmclass (GTK_WINDOW (dialog4), "inspeval", "Anjuta");
	
	dialog_vbox4 = GTK_DIALOG (dialog4)->vbox;
	gtk_widget_show (dialog_vbox4);
	
	label16 = gtk_label_new (_("Inspect/Evaluate Expression"));
	gtk_widget_show (label16);
	gtk_box_pack_start (GTK_BOX (dialog_vbox4), label16, FALSE, FALSE, 0);
	
	entry8 = gtk_entry_new ();
	gtk_entry_set_activates_default (GTK_ENTRY (entry8), TRUE);
	entry_set_text_n_select (entry8, eval_entry_history, TRUE);
	gtk_widget_show (entry8);
	gtk_box_pack_start (GTK_BOX (dialog_vbox4), entry8, FALSE, FALSE, 0);

	/* We will be passing the entry box as user data. So .. */
	g_object_set_data (G_OBJECT (entry8), "user_data", ew);  

	addWatchButton = gtk_button_new_with_label(_("Add To Watch"));
	gtk_widget_show(addWatchButton);
	gtk_box_pack_end(GTK_BOX (dialog_vbox4), addWatchButton, FALSE, FALSE, 0);
	
	g_signal_connect (G_OBJECT (dialog4), "response",
			  G_CALLBACK (on_eval_response), entry8);
	g_signal_connect (G_OBJECT (addWatchButton), "clicked",
			  G_CALLBACK (on_eval_add_watch), entry8);
	
	gtk_widget_grab_focus(entry8);
	return dialog4;
}
