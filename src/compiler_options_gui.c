/*
 * compiler_options_gui.c Copyright (C) 2000  Kh. Naba Kumar Singh
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
#include "compiler_options.h"
#include "compiler_options_cbs.h"
#include "resources.h"

static GtkWidget *create_compiler_options_page0 (CompilerOptions * co);
static GtkWidget *create_compiler_options_page1 (CompilerOptions * co);
static GtkWidget *create_compiler_options_page2 (CompilerOptions * co);
static GtkWidget *create_compiler_options_page3 (CompilerOptions * co);
static GtkWidget *create_compiler_options_page4 (CompilerOptions * co);
static GtkWidget *create_compiler_options_page5 (CompilerOptions * co);
static GtkWidget *create_compiler_options_page6 (CompilerOptions * co);
static GtkWidget *create_compiler_options_page7 (CompilerOptions * co);

void
create_compiler_options_gui (CompilerOptions * co)
{
	GtkWidget *window1;
	GtkWidget *dialog1;
	GtkWidget *dialog_vbox1;
	GtkWidget *notebook1;
	GtkWidget *label2;
	GtkWidget *label3;
	GtkWidget *label4;
	GtkWidget *label5;
	GtkWidget *dialog_action_area1;
	GtkWidget *comopt_ok;
	GtkWidget *comopt_apply;
	GtkWidget *comopt_help;
	GtkWidget *comopt_cancel;
	GtkWidget *tmp_widget;
	GtkWidget *label101;
	GtkWidget *label102;
	GtkWidget *label103;
	GtkWidget *label111;

	dialog1 = gnome_dialog_new (_("Compiler Options"), NULL);
	gtk_window_set_policy (GTK_WINDOW (dialog1), FALSE, FALSE, FALSE);
	gtk_window_set_wmclass (GTK_WINDOW (dialog1), "comp_opt", "Anjuta");
	gnome_dialog_close_hides (GNOME_DIALOG (dialog1), TRUE);

	dialog_vbox1 = GNOME_DIALOG (dialog1)->vbox;
	gtk_widget_show (dialog_vbox1);

	window1 = dialog1;

	notebook1 = gtk_notebook_new ();
	gtk_widget_show (notebook1);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), notebook1, TRUE, TRUE, 0);

	tmp_widget = create_compiler_options_page0 (co);
	gtk_container_add (GTK_CONTAINER (notebook1), tmp_widget);

	label2 = gtk_label_new (_("Supports"));
	gtk_widget_show (label2);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook1),
							       0), label2);

	tmp_widget = create_compiler_options_page1 (co);
	gtk_container_add (GTK_CONTAINER (notebook1), tmp_widget);

	label101 = gtk_label_new (_("Include Paths"));
	gtk_widget_show (label101);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook1),
							       1), label101);

	tmp_widget = create_compiler_options_page2 (co);
	gtk_container_add (GTK_CONTAINER (notebook1), tmp_widget);

	label111 = gtk_label_new (_("Library Paths"));
	gtk_widget_show (label111);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook1),
							       2), label111);

	tmp_widget = create_compiler_options_page3 (co);
	gtk_container_add (GTK_CONTAINER (notebook1), tmp_widget);

	label102 = gtk_label_new (_("Libraries"));
	gtk_widget_show (label102);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook1),
							       3), label102);

	tmp_widget = create_compiler_options_page4 (co);
	gtk_container_add (GTK_CONTAINER (notebook1), tmp_widget);

	label103 = gtk_label_new (_("Defines"));
	gtk_widget_show (label103);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook1),
							       4), label103);

	tmp_widget = create_compiler_options_page5 (co);
	gtk_container_add (GTK_CONTAINER (notebook1), tmp_widget);

	label3 = gtk_label_new (_("Warnings"));
	gtk_widget_show (label3);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook1),
							       5), label3);

	tmp_widget = create_compiler_options_page6 (co);
	gtk_container_add (GTK_CONTAINER (notebook1), tmp_widget);

	label4 = gtk_label_new (_("Optimization"));
	gtk_widget_show (label4);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook1),
							       6), label4);

	tmp_widget = create_compiler_options_page7 (co);
	gtk_container_add (GTK_CONTAINER (notebook1), tmp_widget);

	label5 = gtk_label_new (_("Others"));
	gtk_widget_show (label5);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook1),
							       7), label5);

	dialog_action_area1 = GNOME_DIALOG (dialog1)->action_area;
	gtk_widget_show (dialog_action_area1);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1),
				   GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area1), 8);

	gnome_dialog_append_button (GNOME_DIALOG (dialog1),
				    GNOME_STOCK_BUTTON_HELP);
	comopt_help = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
	gtk_widget_show (comopt_help);
	GTK_WIDGET_SET_FLAGS (comopt_help, GTK_CAN_DEFAULT);

	gnome_dialog_append_button (GNOME_DIALOG (dialog1),
				    GNOME_STOCK_BUTTON_OK);
	comopt_ok = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
	gtk_widget_show (comopt_ok);
	GTK_WIDGET_SET_FLAGS (comopt_ok, GTK_CAN_DEFAULT);

	gnome_dialog_append_button (GNOME_DIALOG (dialog1),
				    GNOME_STOCK_BUTTON_APPLY);
	comopt_apply = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
	gtk_widget_show (comopt_apply);
	GTK_WIDGET_SET_FLAGS (comopt_apply, GTK_CAN_DEFAULT);

	gnome_dialog_append_button (GNOME_DIALOG (dialog1),
				    GNOME_STOCK_BUTTON_CANCEL);
	comopt_cancel = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
	gtk_widget_show (comopt_cancel);
	GTK_WIDGET_SET_FLAGS (comopt_cancel, GTK_CAN_DEFAULT);

	gtk_accel_group_attach (app->accel_group, GTK_OBJECT (window1));

	gtk_signal_connect (GTK_OBJECT (dialog1), "delete_event",
			    GTK_SIGNAL_FUNC (on_comopt_delete_event), co);
	gtk_signal_connect (GTK_OBJECT (comopt_ok), "clicked",
			    GTK_SIGNAL_FUNC (on_comopt_ok_clicked), co);
	gtk_signal_connect (GTK_OBJECT (comopt_help), "clicked",
			    GTK_SIGNAL_FUNC (on_comopt_help_clicked), co);
	gtk_signal_connect (GTK_OBJECT (comopt_apply), "clicked",
			    GTK_SIGNAL_FUNC (on_comopt_apply_clicked), co);
	gtk_signal_connect (GTK_OBJECT (comopt_cancel), "clicked",
			    GTK_SIGNAL_FUNC (on_comopt_cancel_clicked), co);

	co->widgets.window = window1;
	gtk_widget_ref (window1);
}


static GtkWidget *
create_compiler_options_page0 (CompilerOptions * co)
{
	GtkWidget *frame1;
	GtkWidget *table2;
	GtkWidget *scrolledwindow1;
	GtkWidget *clist1;
	GtkWidget *label7;
	GtkWidget *vbox1;
	GtkWidget *button0;
	GtkWidget *button2;

	frame1 = gtk_frame_new (NULL);
	gtk_widget_show (frame1);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	table2 = gtk_table_new (2, 1, FALSE);
	gtk_widget_show (table2);
	gtk_container_add (GTK_CONTAINER (frame1), table2);
	gtk_container_set_border_width (GTK_CONTAINER (table2), 5);
	gtk_table_set_row_spacings (GTK_TABLE (table2), 5);
	gtk_table_set_col_spacings (GTK_TABLE (table2), 6);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow1);
	gtk_table_attach (GTK_TABLE (table2), scrolledwindow1, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);

	clist1 = gtk_clist_new (2);
	gtk_widget_show (clist1);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), clist1);
	gtk_clist_set_column_width (GTK_CLIST (clist1), 0, 30);
	gtk_clist_set_column_width (GTK_CLIST (clist1), 1, 80);
	gtk_clist_set_column_auto_resize (GTK_CLIST (clist1), 1, TRUE);
	gtk_clist_set_selection_mode (GTK_CLIST (clist1),
				      GTK_SELECTION_BROWSE);
	gtk_clist_column_titles_show (GTK_CLIST (clist1));

	label7 = gtk_label_new (_("Select the Supports to add"));
	gtk_widget_show (label7);
	gtk_clist_set_column_widget (GTK_CLIST (clist1), 1, label7);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox1);
	gtk_table_attach (GTK_TABLE (table2), vbox1, 1, 2, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	button0 = gtk_button_new_with_label (_("Info"));
	gtk_widget_show (button0);
	gtk_box_pack_start (GTK_BOX (vbox1), button0, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (button0), 5);

	button2 = gnome_stock_button (GNOME_STOCK_BUTTON_HELP);
	gtk_widget_show (button2);
	gtk_box_pack_start (GTK_BOX (vbox1), button2, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (button2), 5);

	gtk_signal_connect (GTK_OBJECT (clist1), "select_row",
			    GTK_SIGNAL_FUNC (on_co_supp_clist_select_row),
			    co);

	gtk_signal_connect (GTK_OBJECT (button0), "clicked",
			    GTK_SIGNAL_FUNC (on_co_supp_info_clicked), co);

	gtk_signal_connect (GTK_OBJECT (button2), "clicked",
			    GTK_SIGNAL_FUNC (on_co_supp_help_clicked), co);

	co->widgets.supp_clist = clist1;
	gtk_widget_ref (clist1);

	co->widgets.supp_info_b = button0;
	gtk_widget_ref (button0);

	return frame1;
}

static GtkWidget *
create_compiler_options_page1 (CompilerOptions * co)
{
	GtkWidget *frame1;
	GtkWidget *table2;
	GtkWidget *scrolledwindow1;
	GtkWidget *clist1;
	GtkWidget *label7;
	GtkWidget *entry1;
	GtkWidget *label8;
	GtkWidget *vbox1;
	GtkWidget *button0;
	GtkWidget *button1;
	GtkWidget *button2;
	GtkWidget *button3;
	GtkWidget *button4;

	frame1 = gtk_frame_new (NULL);
	gtk_widget_show (frame1);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	table2 = gtk_table_new (2, 2, FALSE);
	gtk_widget_show (table2);
	gtk_container_add (GTK_CONTAINER (frame1), table2);
	gtk_container_set_border_width (GTK_CONTAINER (table2), 5);
	gtk_table_set_row_spacings (GTK_TABLE (table2), 5);
	gtk_table_set_col_spacings (GTK_TABLE (table2), 6);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow1);
	gtk_table_attach (GTK_TABLE (table2), scrolledwindow1, 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);

	clist1 = gtk_clist_new (1);
	gtk_widget_show (clist1);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), clist1);
	gtk_clist_set_column_width (GTK_CLIST (clist1), 0, 80);
	gtk_clist_set_selection_mode (GTK_CLIST (clist1),
				      GTK_SELECTION_BROWSE);
	gtk_clist_column_titles_show (GTK_CLIST (clist1));

	label7 = gtk_label_new (_("Include Paths"));
	gtk_widget_show (label7);
	gtk_clist_set_column_widget (GTK_CLIST (clist1), 0, label7);

	entry1 = gtk_entry_new ();
	gtk_widget_show (entry1);
	gtk_table_attach (GTK_TABLE (table2), entry1, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);

	label8 = gtk_label_new (_("<: Enter here"));
	gtk_widget_show (label8);
	gtk_table_attach (GTK_TABLE (table2), label8, 1, 2, 0, 1,
			  (GtkAttachOptions) (0), (GtkAttachOptions) (0), 0,
			  0);

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
			    GTK_SIGNAL_FUNC (on_co_inc_clist_select_row), co);
	gtk_signal_connect (GTK_OBJECT (button0), "clicked",
			    GTK_SIGNAL_FUNC (on_co_inc_add_clicked), co);
	gtk_signal_connect (GTK_OBJECT (button1), "clicked",
			    GTK_SIGNAL_FUNC (on_co_inc_update_clicked), co);
	gtk_signal_connect (GTK_OBJECT (button2), "clicked",
			    GTK_SIGNAL_FUNC (on_co_inc_remove_clicked), co);
	gtk_signal_connect (GTK_OBJECT (button3), "clicked",
			    GTK_SIGNAL_FUNC (on_co_inc_clear_clicked), co);
	gtk_signal_connect (GTK_OBJECT (button4), "clicked",
			    GTK_SIGNAL_FUNC (on_co_inc_help_clicked), co);

	co->widgets.inc_clist = clist1;
	gtk_widget_ref (clist1);
	co->widgets.inc_entry = entry1;
	gtk_widget_ref (entry1);
	co->widgets.inc_add_b = button0;
	gtk_widget_ref (button0);
	co->widgets.inc_update_b = button1;
	gtk_widget_ref (button1);
	co->widgets.inc_remove_b = button2;
	gtk_widget_ref (button2);
	co->widgets.inc_clear_b = button3;
	gtk_widget_ref (button3);

	return frame1;
}

static GtkWidget *
create_compiler_options_page2 (CompilerOptions * co)
{
	GtkWidget *frame1;
	GtkWidget *table2;
	GtkWidget *scrolledwindow1;
	GtkWidget *clist1;
	GtkWidget *label7;
	GtkWidget *entry1;
	GtkWidget *label8;
	GtkWidget *vbox1;
	GtkWidget *button0;
	GtkWidget *button1;
	GtkWidget *button2;
	GtkWidget *button3;
	GtkWidget *button4;

	frame1 = gtk_frame_new (NULL);
	gtk_widget_show (frame1);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	table2 = gtk_table_new (2, 2, FALSE);
	gtk_widget_show (table2);
	gtk_container_add (GTK_CONTAINER (frame1), table2);
	gtk_container_set_border_width (GTK_CONTAINER (table2), 5);
	gtk_table_set_row_spacings (GTK_TABLE (table2), 5);
	gtk_table_set_col_spacings (GTK_TABLE (table2), 6);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow1);
	gtk_table_attach (GTK_TABLE (table2), scrolledwindow1, 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);

	clist1 = gtk_clist_new (1);
	gtk_widget_show (clist1);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), clist1);
	gtk_clist_set_column_width (GTK_CLIST (clist1), 0, 80);
	gtk_clist_set_selection_mode (GTK_CLIST (clist1),
				      GTK_SELECTION_BROWSE);
	gtk_clist_column_titles_show (GTK_CLIST (clist1));

	label7 = gtk_label_new (_("Library Paths"));
	gtk_widget_show (label7);
	gtk_clist_set_column_widget (GTK_CLIST (clist1), 0, label7);

	entry1 = gtk_entry_new ();
	gtk_widget_show (entry1);
	gtk_table_attach (GTK_TABLE (table2), entry1, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);

	label8 = gtk_label_new (_("<: Enter here"));
	gtk_widget_show (label8);
	gtk_table_attach (GTK_TABLE (table2), label8, 1, 2, 0, 1,
			  (GtkAttachOptions) (0), (GtkAttachOptions) (0), 0,
			  0);

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
			    GTK_SIGNAL_FUNC
			    (on_co_lib_paths_clist_select_row), co);
	gtk_signal_connect (GTK_OBJECT (button0), "clicked",
			    GTK_SIGNAL_FUNC (on_co_lib_paths_add_clicked),
			    co);
	gtk_signal_connect (GTK_OBJECT (button1), "clicked",
			    GTK_SIGNAL_FUNC (on_co_lib_paths_update_clicked),
			    co);
	gtk_signal_connect (GTK_OBJECT (button2), "clicked",
			    GTK_SIGNAL_FUNC (on_co_lib_paths_remove_clicked),
			    co);
	gtk_signal_connect (GTK_OBJECT (button3), "clicked",
			    GTK_SIGNAL_FUNC (on_co_lib_paths_clear_clicked),
			    co);
	gtk_signal_connect (GTK_OBJECT (button4), "clicked",
			    GTK_SIGNAL_FUNC (on_co_lib_paths_help_clicked),
			    co);

	co->widgets.lib_paths_clist = clist1;
	gtk_widget_ref (clist1);
	co->widgets.lib_paths_entry = entry1;
	gtk_widget_ref (entry1);
	co->widgets.lib_paths_add_b = button0;
	gtk_widget_ref (button0);
	co->widgets.lib_paths_update_b = button1;
	gtk_widget_ref (button1);
	co->widgets.lib_paths_remove_b = button2;
	gtk_widget_ref (button2);
	co->widgets.lib_paths_clear_b = button3;
	gtk_widget_ref (button3);

	return frame1;
}

static GtkWidget *
create_compiler_options_page3 (CompilerOptions * co)
{
	GtkWidget *frame1;
	GtkWidget *table2;
	GtkWidget *scrolledwindow1;
	GtkWidget *scrolledwindow2;
	GtkWidget *clist1;
	GtkWidget *clist2;
	GtkWidget *label7;
	GtkWidget *entry1;
	GtkWidget *label8;
	GtkWidget *vbox1;
	GtkWidget *button0;
	GtkWidget *button1;
	GtkWidget *button2;
	GtkWidget *button3;
	GtkWidget *button4;

	frame1 = gtk_frame_new (NULL);
	gtk_widget_show (frame1);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	table2 = gtk_table_new (3, 2, FALSE);
	gtk_widget_show (table2);
	gtk_container_add (GTK_CONTAINER (frame1), table2);
	gtk_container_set_border_width (GTK_CONTAINER (table2), 5);
	gtk_table_set_row_spacings (GTK_TABLE (table2), 5);
	gtk_table_set_col_spacings (GTK_TABLE (table2), 6);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow1);
	gtk_table_attach (GTK_TABLE (table2), scrolledwindow1, 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);

	scrolledwindow2 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow2);
	gtk_table_attach (GTK_TABLE (table2), scrolledwindow2, 1, 2, 1, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow2),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);

	clist1 = gtk_clist_new (2);
	gtk_widget_show (clist1);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), clist1);
	gtk_clist_set_column_width (GTK_CLIST (clist1), 0, 25);
	gtk_clist_set_column_width (GTK_CLIST (clist1), 1, 80);
	gtk_clist_set_column_auto_resize (GTK_CLIST (clist1), 1, TRUE);
	gtk_clist_set_selection_mode (GTK_CLIST (clist1),
				      GTK_SELECTION_BROWSE);
	gtk_clist_column_titles_show (GTK_CLIST (clist1));

	clist2 = gtk_clist_new (2);
	gtk_widget_show (clist2);
	gtk_container_add (GTK_CONTAINER (scrolledwindow2), clist2);
	gtk_clist_set_column_width (GTK_CLIST (clist2), 0, 80);
	gtk_clist_set_column_width (GTK_CLIST (clist2), 1, 80);
	gtk_clist_set_column_auto_resize (GTK_CLIST (clist2), 1, TRUE);
	gtk_clist_set_selection_mode (GTK_CLIST (clist2),
				      GTK_SELECTION_BROWSE);
	gtk_clist_column_titles_show (GTK_CLIST (clist2));

	label7 = gtk_label_new (_("Added Libraries"));
	gtk_widget_show (label7);
	gtk_clist_set_column_widget (GTK_CLIST (clist1), 1, label7);

	label7 = gtk_label_new (_("Stock"));
	gtk_widget_show (label7);
	gtk_clist_set_column_widget (GTK_CLIST (clist2), 0, label7);

	label7 = gtk_label_new (_("Description"));
	gtk_widget_show (label7);
	gtk_clist_set_column_widget (GTK_CLIST (clist2), 1, label7);

	entry1 = gtk_entry_new ();
	gtk_widget_show (entry1);
	gtk_table_attach (GTK_TABLE (table2), entry1, 0, 2, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);

	label8 = gtk_label_new (_("<: Enter here"));
	gtk_widget_show (label8);
	gtk_table_attach (GTK_TABLE (table2), label8, 2, 3, 0, 1,
			  (GtkAttachOptions) (0), (GtkAttachOptions) (0), 0,
			  0);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox1);
	gtk_table_attach (GTK_TABLE (table2), vbox1, 2, 3, 1, 2,
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
			    GTK_SIGNAL_FUNC (on_co_lib_clist_select_row), co);
	gtk_signal_connect (GTK_OBJECT (clist2), "select_row",
			    GTK_SIGNAL_FUNC
			    (on_co_lib_stock_clist_select_row), co);
	gtk_signal_connect (GTK_OBJECT (button0), "clicked",
			    GTK_SIGNAL_FUNC (on_co_lib_add_clicked), co);
	gtk_signal_connect (GTK_OBJECT (button1), "clicked",
			    GTK_SIGNAL_FUNC (on_co_lib_update_clicked), co);
	gtk_signal_connect (GTK_OBJECT (button2), "clicked",
			    GTK_SIGNAL_FUNC (on_co_lib_remove_clicked), co);
	gtk_signal_connect (GTK_OBJECT (button3), "clicked",
			    GTK_SIGNAL_FUNC (on_co_lib_clear_clicked), co);
	gtk_signal_connect (GTK_OBJECT (button4), "clicked",
			    GTK_SIGNAL_FUNC (on_co_lib_help_clicked), co);

	co->widgets.lib_clist = clist1;
	gtk_widget_ref (clist1);
	co->widgets.lib_stock_clist = clist2;
	gtk_widget_ref (clist2);

	co->widgets.lib_entry = entry1;
	gtk_widget_ref (entry1);

	co->widgets.lib_add_b = button0;
	gtk_widget_ref (button0);
	co->widgets.lib_update_b = button1;
	gtk_widget_ref (button1);
	co->widgets.lib_remove_b = button2;
	gtk_widget_ref (button2);
	co->widgets.lib_clear_b = button3;
	gtk_widget_ref (button3);

	return frame1;
}

static GtkWidget *
create_compiler_options_page4 (CompilerOptions * co)
{
	GtkWidget *frame1;
	GtkWidget *table2;
	GtkWidget *scrolledwindow1;
	GtkWidget *clist1;
	GtkWidget *label7;
	GtkWidget *entry1;
	GtkWidget *label8;
	GtkWidget *vbox1;
	GtkWidget *button0;
	GtkWidget *button1;
	GtkWidget *button2;
	GtkWidget *button3;
	GtkWidget *button4;

	frame1 = gtk_frame_new (NULL);
	gtk_widget_show (frame1);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	table2 = gtk_table_new (2, 2, FALSE);
	gtk_widget_show (table2);
	gtk_container_add (GTK_CONTAINER (frame1), table2);
	gtk_container_set_border_width (GTK_CONTAINER (table2), 5);
	gtk_table_set_row_spacings (GTK_TABLE (table2), 5);
	gtk_table_set_col_spacings (GTK_TABLE (table2), 6);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow1);
	gtk_table_attach (GTK_TABLE (table2), scrolledwindow1, 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);

	clist1 = gtk_clist_new (1);
	gtk_widget_show (clist1);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), clist1);
	gtk_clist_set_column_width (GTK_CLIST (clist1), 0, 80);
	gtk_clist_set_selection_mode (GTK_CLIST (clist1),
				      GTK_SELECTION_BROWSE);
	gtk_clist_column_titles_show (GTK_CLIST (clist1));

	label7 = gtk_label_new (_("Defines"));
	gtk_widget_show (label7);
	gtk_clist_set_column_widget (GTK_CLIST (clist1), 0, label7);

	entry1 = gtk_entry_new ();
	gtk_widget_show (entry1);
	gtk_table_attach (GTK_TABLE (table2), entry1, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);

	label8 = gtk_label_new (_("<: Enter here"));
	gtk_widget_show (label8);
	gtk_table_attach (GTK_TABLE (table2), label8, 1, 2, 0, 1,
			  (GtkAttachOptions) (0), (GtkAttachOptions) (0), 0,
			  0);

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
			    GTK_SIGNAL_FUNC (on_co_def_clist_select_row), co);
	gtk_signal_connect (GTK_OBJECT (button0), "clicked",
			    GTK_SIGNAL_FUNC (on_co_def_add_clicked), co);
	gtk_signal_connect (GTK_OBJECT (button1), "clicked",
			    GTK_SIGNAL_FUNC (on_co_def_update_clicked), co);
	gtk_signal_connect (GTK_OBJECT (button2), "clicked",
			    GTK_SIGNAL_FUNC (on_co_def_remove_clicked), co);
	gtk_signal_connect (GTK_OBJECT (button3), "clicked",
			    GTK_SIGNAL_FUNC (on_co_def_clear_clicked), co);
	gtk_signal_connect (GTK_OBJECT (button4), "clicked",
			    GTK_SIGNAL_FUNC (on_co_def_help_clicked), co);

	co->widgets.def_clist = clist1;
	gtk_widget_ref (clist1);
	co->widgets.def_entry = entry1;
	gtk_widget_ref (entry1);
	co->widgets.def_add_b = button0;
	gtk_widget_ref (button0);
	co->widgets.def_update_b = button1;
	gtk_widget_ref (button1);
	co->widgets.def_remove_b = button2;
	gtk_widget_ref (button2);
	co->widgets.def_clear_b = button3;
	gtk_widget_ref (button3);

	return frame1;
}

static GtkWidget *
create_compiler_options_page5 (CompilerOptions * co)
{
	GtkWidget *frame;
	GtkWidget *vbox1;
	GtkWidget *table2;
	GtkWidget *scrolledwindow1;
	GtkWidget *viewport1;
	GtkWidget *checkbutton[26];
	gint i;

	frame = gtk_frame_new (NULL);
	gtk_widget_show (frame);
	gtk_container_set_border_width (GTK_CONTAINER (frame), 5);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER (frame), vbox1);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), 5);

	checkbutton[0] =
		gtk_check_button_new_with_label (_
						 ("Treat warnings as errors."));
	gtk_widget_show (checkbutton[0]);
	gtk_box_pack_start (GTK_BOX (vbox1), checkbutton[0], FALSE, FALSE, 0);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow1);
	gtk_box_pack_start (GTK_BOX (vbox1), scrolledwindow1, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);

	viewport1 = gtk_viewport_new (NULL, NULL);
	gtk_widget_show (viewport1);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), viewport1);

	table2 = gtk_table_new (15, 1, FALSE);
	gtk_widget_show (table2);
	gtk_container_add (GTK_CONTAINER (viewport1), table2);

	checkbutton[1] = gtk_check_button_new_with_label (_("No Warnings"));
	gtk_widget_show (checkbutton[1]);
	gtk_table_attach (GTK_TABLE (table2), checkbutton[1], 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	checkbutton[2] = gtk_check_button_new_with_label (_("All Warnings"));
	gtk_widget_show (checkbutton[2]);
	gtk_table_attach (GTK_TABLE (table2), checkbutton[2], 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton[2]),
				      TRUE);

	checkbutton[3] =
		gtk_check_button_new_with_label (_
						 ("Warning for Implicit declarations"));
	gtk_widget_show (checkbutton[3]);
	gtk_table_attach (GTK_TABLE (table2), checkbutton[3], 0, 1, 2, 3,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	checkbutton[4] =
		gtk_check_button_new_with_label (_
						 ("Warning for mismached Return types"));
	gtk_widget_show (checkbutton[4]);
	gtk_table_attach (GTK_TABLE (table2), checkbutton[4], 0, 1, 3, 4,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	checkbutton[5] =
		gtk_check_button_new_with_label (_
						 ("Warning for Unused Variables"));
	gtk_widget_show (checkbutton[5]);
	gtk_table_attach (GTK_TABLE (table2), checkbutton[5], 0, 1, 4, 5,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	checkbutton[6] =
		gtk_check_button_new_with_label (_
						 ("Warning for unhandled Switch case with enums"));
	gtk_widget_show (checkbutton[6]);
	gtk_table_attach (GTK_TABLE (table2), checkbutton[6], 0, 1, 5, 6,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	checkbutton[7] =
		gtk_check_button_new_with_label (_
						 ("Warning for nested Comment"));
	gtk_widget_show (checkbutton[7]);
	gtk_table_attach (GTK_TABLE (table2), checkbutton[7], 0, 1, 6, 7,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	checkbutton[8] =
		gtk_check_button_new_with_label (_
						 ("Warning for Uninitialized variable use"));
	gtk_widget_show (checkbutton[8]);
	gtk_table_attach (GTK_TABLE (table2), checkbutton[8], 0, 1, 7, 8,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	checkbutton[9] =
		gtk_check_button_new_with_label (_
						 ("Warning for missing Parentheses"));
	gtk_widget_show (checkbutton[9]);
	gtk_table_attach (GTK_TABLE (table2), checkbutton[9], 0, 1, 8, 9,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	checkbutton[10] =
		gtk_check_button_new_with_label (_
						 ("Warning for Un-Traditional syntax"));
	gtk_widget_show (checkbutton[10]);
	gtk_table_attach (GTK_TABLE (table2), checkbutton[10], 0, 1, 9, 10,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	checkbutton[11] =
		gtk_check_button_new_with_label (_
						 ("Warning for Variable Shadowing"));
	gtk_widget_show (checkbutton[11]);
	gtk_table_attach (GTK_TABLE (table2), checkbutton[11], 0, 1, 10, 11,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	checkbutton[12] =
		gtk_check_button_new_with_label (_
						 ("Warning for suspected Pointer-arith"));
	gtk_widget_show (checkbutton[12]);
	gtk_table_attach (GTK_TABLE (table2), checkbutton[12], 0, 1, 11, 12,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	checkbutton[13] =
		gtk_check_button_new_with_label (_
						 ("Warning for Missing-prototypes"));
	gtk_widget_show (checkbutton[13]);
	gtk_table_attach (GTK_TABLE (table2), checkbutton[13], 0, 1, 12, 13,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	checkbutton[14] =
		gtk_check_button_new_with_label (_
						 ("Warning for Un-Inline-able declarations"));
	gtk_widget_show (checkbutton[14]);
	gtk_table_attach (GTK_TABLE (table2), checkbutton[14], 0, 1, 13, 14,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	checkbutton[15] =
		gtk_check_button_new_with_label (_
						 ("Warning for Overloaded-virtuals"));
	gtk_widget_show (checkbutton[15]);
	gtk_table_attach (GTK_TABLE (table2), checkbutton[15], 0, 1, 14, 15,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	for (i = 0; i < 16; i++)
	{
		co->widgets.warning_button[i] = checkbutton[i];
		gtk_widget_ref (co->widgets.warning_button[i]);
	}
	return frame;
}

static GtkWidget *
create_compiler_options_page6 (CompilerOptions * co)
{
	GtkWidget *frame;
	GtkWidget *frame1;
	GtkWidget *vbox2;
	GSList *vbox2_group = NULL;
	GtkWidget *label1;
	GtkWidget *hseparator1;
	GtkWidget *radiobutton[4];
	gint i;

	frame = gtk_frame_new (NULL);
	gtk_widget_show (frame);
	gtk_container_set_border_width (GTK_CONTAINER (frame), 5);

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox2);
	gtk_container_add (GTK_CONTAINER (frame), vbox2);

	radiobutton[0] =
		gtk_radio_button_new_with_label (vbox2_group,
						 _("No optimization"));
	vbox2_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton[0]));
	gtk_widget_show (radiobutton[0]);
	gtk_box_pack_start (GTK_BOX (vbox2), radiobutton[0], FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (radiobutton[0]), 5);

	radiobutton[1] =
		gtk_radio_button_new_with_label (vbox2_group,
						 _
						 ("Low level optimization (Machine dependent)"));
	vbox2_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton[1]));
	gtk_widget_show (radiobutton[1]);
	gtk_box_pack_start (GTK_BOX (vbox2), radiobutton[1], FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (radiobutton[1]), 5);

	radiobutton[2] =
		gtk_radio_button_new_with_label (vbox2_group,
						 _
						 ("Higher level optimization (without space-speed tradeoff)"));
	vbox2_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton[2]));
	gtk_widget_show (radiobutton[2]);
	gtk_box_pack_start (GTK_BOX (vbox2), radiobutton[2], FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (radiobutton[2]), 5);

	radiobutton[3] =
		gtk_radio_button_new_with_label (vbox2_group,
						 _
						 ("Fastest code optimization (Function inlined where ever possible"));
	vbox2_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton[3]));
	gtk_widget_show (radiobutton[3]);
	gtk_box_pack_start (GTK_BOX (vbox2), radiobutton[3], FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (radiobutton[3]), 5);

	hseparator1 = gtk_hseparator_new ();
	gtk_widget_show (hseparator1);
	gtk_box_pack_start (GTK_BOX (vbox2), hseparator1, FALSE, TRUE, 0);

	frame1 = gtk_frame_new (_("Note"));
	gtk_box_pack_start (GTK_BOX (vbox2), frame1, FALSE, FALSE, 5);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);
	gtk_widget_show (frame1);

	label1 = gtk_label_new (
		_("With optimization enabled, you might have some difficulty\n"
	          "debugging your program. So it is not a good idea to enable\n"
		  "both debug and optimization options, unless what you want\n"
		  "is to debug the optimized code."));
	gtk_widget_show (label1);
	gtk_misc_set_alignment (GTK_MISC (label1), 0.5, 0.5);
	gtk_container_add (GTK_CONTAINER (frame1), label1);
	
	for (i = 0; i < 4; i++)
	{
		co->widgets.optimize_button[i] = radiobutton[i];
		gtk_widget_ref (co->widgets.optimize_button[i]);
	}

	return frame;
}

static GtkWidget *
create_compiler_options_page7 (CompilerOptions * co)
{
	GtkWidget *frame;
	GtkWidget *frame1;
	GtkWidget *event_box1;
	GtkWidget *event_box2;
	GtkWidget *vbox1;
	GtkWidget *entry1;
	GtkWidget *entry2;
	GtkWidget *checkbutton[2];
	GtkWidget *hseparator2;
	gint i;

	frame = gtk_frame_new (NULL);
	gtk_widget_show (frame);
	gtk_container_set_border_width (GTK_CONTAINER (frame), 5);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER (frame), vbox1);

	checkbutton[0] =
		gtk_check_button_new_with_label (_("Enable Debugging"));
	gtk_widget_show (checkbutton[0]);
	gtk_box_pack_start (GTK_BOX (vbox1), checkbutton[0], FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (checkbutton[0]), 5);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton[0]),
				      TRUE);

	checkbutton[1] =
		gtk_check_button_new_with_label (_("Enable Profiling"));
	gtk_widget_show (checkbutton[1]);
	gtk_box_pack_start (GTK_BOX (vbox1), checkbutton[1], FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (checkbutton[1]), 5);

	hseparator2 = gtk_hseparator_new ();
	gtk_widget_show (hseparator2);
	gtk_box_pack_start (GTK_BOX (vbox1), hseparator2, TRUE, TRUE, 0);

	frame1 =
		gtk_frame_new (_
			       ("Additional compiler options (command line options)"));
	gtk_widget_show (frame1);
	gtk_box_pack_start (GTK_BOX (vbox1), frame1, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	event_box1 = gtk_event_box_new ();
	gtk_widget_show (event_box1);
	gtk_container_add (GTK_CONTAINER (frame1), event_box1);
	gtk_container_set_border_width (GTK_CONTAINER (event_box1), 5);

	entry1 = gtk_entry_new ();
	gtk_widget_show (entry1);
	gtk_container_add (GTK_CONTAINER (event_box1), entry1);

	frame1 =
		gtk_frame_new (_
			       ("Additional linker options (command line options)"));
	gtk_widget_show (frame1);
	gtk_box_pack_start (GTK_BOX (vbox1), frame1, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	event_box2 = gtk_event_box_new ();
	gtk_widget_show (event_box2);
	gtk_container_add (GTK_CONTAINER (frame1), event_box2);
	gtk_container_set_border_width (GTK_CONTAINER (event_box2), 5);

	entry2 = gtk_entry_new ();
	gtk_widget_show (entry2);
	gtk_container_add (GTK_CONTAINER (event_box2), entry2);

	for (i = 0; i < 2; i++)
	{
		co->widgets.other_button[i] = checkbutton[i];
		gtk_widget_ref (co->widgets.other_button[i]);
	}

	co->widgets.other_c_options_entry = entry1;
	gtk_widget_ref (entry1);
	co->widgets.other_l_options_entry = entry2;
	gtk_widget_ref (entry2);

	return frame;
}
