/*
    preferences_gui.c
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
#include <ctype.h>

#include <gnome.h>

#include "anjuta.h"
#include "message-manager.h"
#include "resources.h"
#include "preferences.h"
#include "properties.h"

gchar *format_style[] = {
	"Custom style", " -i8 -bl -bls -bli0 -ss",
	"GNU coding style", " -gnu",
	"Kernighan & Ritchie style", " -kr",
	"Original Berkeley style", " -orig",
	"Style of Kangleipak", " -i8 -sc -bli0 -bl0 -cbi0 -ss",
	"Hello World style", "  -gnu -i0 -bli0 -cbi0 -cdb -sc -bl0 -ss",
	"Crazy boy style", " ",
	NULL, NULL
};

gchar *hilite_style[] = {
	"Normal <Default>", "style.*.32",
	"Comments", "style.default.comment",
	"Numbers", "style.default.number",
	"Keywords", "style.default.keyword",
	"System Keywords", "style.default.syskeyword",
	"Local Keywords", "style.default.localkeyword",
	"Double Quoted Strings", "style.default.doublequote",
	"Single Quoted Strings", "style.default.singlequote",
	"Unclosed Strings", "style.default.unclosedstring",
	"Preprocessor Directives", "style.default.preprocessor",
	"Identifiers (Not C Style)", "style.default.identifier",
	"Definitions (Not C Style)", "style.default.definition",
	"Functions (Not C Style)", "style.default.function",
	"Matched Braces", "style.*.34",
	"Incomplete Brace", "style.*.35",
	"Control Characters", "style.*.36",
	"Line Numbers", "style.*.33",
	"Indentation Guides", "style.*.37",
	NULL, NULL
};

static void on_preferences_ok_clicked (GtkButton * button,
				       gpointer user_data);

static void on_preferences_apply_clicked (GtkButton * button,
					  gpointer user_data);

static void on_preferences_cancel_clicked (GtkButton * button,
					   gpointer user_data);

static gboolean on_preferences_close (GtkWidget * w,
					     gpointer user_data);

static void
on_format_style_entry_changed (GtkEditable * editable, gpointer user_data);
static void on_use_default_font_toggled (GtkToggleButton * tb, gpointer data);

static void
on_use_default_attrib_toggled (GtkToggleButton * tb, gpointer data);

static void on_use_default_fore_toggled (GtkToggleButton * tb, gpointer data);

static void on_use_default_back_toggled (GtkToggleButton * tb, gpointer data);

static void
on_format_style_check_clicked (GtkButton * but, gpointer user_data);

static void
on_trunc_mesg_check_clicked (GtkButton * button, gpointer user_data);

static void
on_notebook_tag_none_clicked (GtkButton * button, gpointer user_data);

static gboolean 
fontpicker_get_font_name (GtkWidget *gnomefontpicker, gchar**   font);

static GtkWidget *create_preferences_page0 (Preferences * p);

static GtkWidget *create_preferences_page1 (Preferences * p);

static GtkWidget *create_preferences_page2 (Preferences * p);

static GtkWidget *create_preferences_page3 (Preferences * p);

static GtkWidget *create_preferences_page4 (Preferences * p);

static GtkWidget *create_preferences_page5 (Preferences * p);

static GtkWidget *create_preferences_pagemsg (Preferences * p);

static GtkWidget *create_preferences_page7 (Preferences * p);

static GtkWidget *create_preferences_pageComp (Preferences * p);

void
create_preferences_gui (Preferences * pr)
{
	GtkWidget *dialog1;
	GtkWidget *dialog_vbox2;
	GtkWidget *dialog_action_area2;
	GtkWidget *window2;
	GtkWidget *notebook2;
	GtkWidget *page0;
	GtkWidget *page1;
	GtkWidget *page2;
	GtkWidget *page3;
	GtkWidget *page4;
	GtkWidget *page5;
	GtkWidget *pagemsg;
	GtkWidget *page7;
	GtkWidget *pageComponents;
	GtkWidget *label102;
	GtkWidget *label103;
	GtkWidget *label1;
	GtkWidget *label12;
	GtkWidget *label15;
	GtkWidget* labelmsg;
	GtkWidget *labelComps;	
	GtkWidget *preferences_ok;
	GtkWidget *preferences_apply;
	GtkWidget *preferences_cancel;

	dialog1 = gnome_dialog_new (_("Preferences"), NULL);
	gtk_window_set_policy (GTK_WINDOW (dialog1), FALSE, FALSE, TRUE);
	gnome_dialog_close_hides (GNOME_DIALOG (dialog1), TRUE);
	window2 = dialog1;
	gtk_window_set_wmclass (GTK_WINDOW (window2), "preferences", "Anjuta");

	pr->widgets.window = dialog1;

	dialog_vbox2 = GNOME_DIALOG (dialog1)->vbox;
	gtk_widget_show (dialog_vbox2);

	notebook2 = gtk_notebook_new ();
	gtk_box_pack_start (GTK_BOX (dialog_vbox2), notebook2, FALSE, TRUE,
			    0);
	gtk_widget_show (notebook2);

	page0 = create_preferences_page0 (pr);
	gtk_container_add (GTK_CONTAINER (notebook2), page0);

	label1 = gtk_label_new (_("General"));
	gtk_widget_show (label1);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook2),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook2),
							       0), label1);

	page1 = create_preferences_page1 (pr);
	gtk_container_add (GTK_CONTAINER (notebook2), page1);

	label1 = gtk_label_new (_("Build"));
	gtk_widget_show (label1);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook2),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook2),
							       1), label1);

	page2 = create_preferences_page2 (pr);
	gtk_container_add (GTK_CONTAINER (notebook2), page2);

	label1 = gtk_label_new (_("Styles"));
	gtk_widget_show (label1);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook2),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook2),
							       2), label1);

	page3 = create_preferences_page3 (pr);
	gtk_container_add (GTK_CONTAINER (notebook2), page3);

	label12 = gtk_label_new (_("Editor"));
	gtk_widget_show (label12);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook2),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook2),
							       3), label12);

	page4 = create_preferences_page4 (pr);
	gtk_container_add (GTK_CONTAINER (notebook2), page4);

	label15 = gtk_label_new (_("Print Setup"));
	gtk_widget_show (label15);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook2),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook2),
							       4), label15);

	page5 = create_preferences_page5 (pr);
	gtk_container_add (GTK_CONTAINER (notebook2), page5);

	label102 = gtk_label_new (_("Auto Format"));
	gtk_widget_show (label102);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook2),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook2),
							       5), label102);
	
	pagemsg = create_preferences_pagemsg (pr);
	gtk_container_add (GTK_CONTAINER (notebook2), pagemsg);
	
	labelmsg = gtk_label_new (_("Messages"));
	gtk_widget_show (labelmsg);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook2),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook2),
							       6), labelmsg);
	
	page7 = create_preferences_page7 (pr);
	gtk_container_add (GTK_CONTAINER (notebook2), page7);

	label103 = gtk_label_new (_("misc"));
	gtk_widget_show (label103);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook2),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook2),
							       7), label103);
	pageComponents = create_preferences_pageComp (pr);
	gtk_container_add (GTK_CONTAINER (notebook2), pageComponents);

	labelComps = gtk_label_new (_("Components"));
	gtk_widget_show (labelComps);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook2),
				    gtk_notebook_get_nth_page (GTK_NOTEBOOK
							       (notebook2),
							       8), labelComps);

	dialog_action_area2 = GNOME_DIALOG (dialog1)->action_area;
	gtk_widget_show (dialog_action_area2);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area2),
				   GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area2), 8);

	gnome_dialog_append_button (GNOME_DIALOG (dialog1),
				    GNOME_STOCK_BUTTON_OK);
	preferences_ok = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
	gtk_widget_show (preferences_ok);
	GTK_WIDGET_SET_FLAGS (preferences_ok, GTK_CAN_DEFAULT);

	gnome_dialog_append_button (GNOME_DIALOG (dialog1),
				    GNOME_STOCK_BUTTON_APPLY);
	preferences_apply =
		g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
	gtk_widget_show (preferences_apply);
	GTK_WIDGET_SET_FLAGS (preferences_apply, GTK_CAN_DEFAULT);

	gnome_dialog_append_button (GNOME_DIALOG (dialog1),
				    GNOME_STOCK_BUTTON_CANCEL);
	preferences_cancel =
		g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
	gtk_widget_show (preferences_cancel);
	GTK_WIDGET_SET_FLAGS (preferences_cancel, GTK_CAN_DEFAULT);

	gtk_accel_group_attach (app->accel_group, GTK_OBJECT (dialog1));

	gtk_signal_connect (GTK_OBJECT (dialog1), "close",
			    GTK_SIGNAL_FUNC (on_preferences_close),
			    pr);
	gtk_signal_connect (GTK_OBJECT (preferences_ok), "clicked",
			    GTK_SIGNAL_FUNC (on_preferences_ok_clicked), pr);
	gtk_signal_connect (GTK_OBJECT (preferences_apply), "clicked",
			    GTK_SIGNAL_FUNC (on_preferences_apply_clicked),
			    pr);
	gtk_signal_connect (GTK_OBJECT (preferences_cancel), "clicked",
			    GTK_SIGNAL_FUNC (on_preferences_cancel_clicked),
			    pr);

	pr->widgets.notebook = notebook2;
	pr->widgets.window = dialog1;

	gtk_widget_ref (pr->widgets.window);
	gtk_widget_ref (pr->widgets.notebook);
}

static GtkWidget *
create_preferences_page0 (Preferences * pr)
{
	GtkWidget *window1;
	GtkWidget *frame1;
	GtkWidget *vbox1;
	GtkWidget *entry1, *entry2, *entry3, *entry4;
	GtkWidget *label1, *label2, *label3, *label4;
	GtkWidget *table1;
	GtkWidget *table2;
	GtkWidget *eventbox3;
	GtkObject *spinbutton2_adj;
	GtkWidget *spinbutton2;
	GtkWidget *eventbox4;
	GtkObject *spinbutton3_adj;
	GtkWidget *spinbutton3;
	GtkWidget *eventbox5;
	GtkObject *spinbutton4_adj;
	GtkWidget *spinbutton4;
	GtkWidget *vseparator2;
	GtkWidget *vseparator1;
	GtkWidget *label5;
	GtkWidget *label6;
	GtkWidget *vseparator3;
	GtkWidget *boxLastPrj;
	GtkWidget *checkbutton1;
	GtkWidget *checkbutton2;

	window1 = pr->widgets.window;

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox1);

	frame1 = gtk_frame_new (_(" Directories "));
	gtk_widget_show (frame1);
	gtk_box_pack_start (GTK_BOX (vbox1), frame1, FALSE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	table1 = gtk_table_new (4, 2, FALSE);
	gtk_widget_show (table1);
	gtk_container_add (GTK_CONTAINER (frame1), table1);
	gtk_container_set_border_width (GTK_CONTAINER (table1), 5);
	gtk_table_set_row_spacings (GTK_TABLE (table1), 5);
	gtk_table_set_col_spacings (GTK_TABLE (table1), 5);

	entry1 = gtk_entry_new ();
	gtk_widget_show (entry1);
	gtk_table_attach (GTK_TABLE (table1), entry1, 1, 2, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	entry2 = gtk_entry_new ();
	gtk_widget_show (entry2);
	gtk_table_attach (GTK_TABLE (table1), entry2, 1, 2, 1, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	entry3 = gtk_entry_new ();
	gtk_widget_show (entry3);
	gtk_table_attach (GTK_TABLE (table1), entry3, 1, 2, 2, 3,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	entry4 = gtk_entry_new ();
	gtk_widget_show (entry4);
	gtk_table_attach (GTK_TABLE (table1), entry4, 1, 2, 3, 4,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	label1 = gtk_label_new (_("Projects:"));
	gtk_widget_show (label1);
	gtk_misc_set_alignment (GTK_MISC (label1), 0, -1);
	gtk_table_attach (GTK_TABLE (table1), label1, 0, 1, 0, 1,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 0, 0);

	label2 = gtk_label_new (_("Tarballs:"));
	gtk_widget_show (label2);
	gtk_misc_set_alignment (GTK_MISC (label2), 0, -1);
	gtk_table_attach (GTK_TABLE (table1), label2, 0, 1, 1, 2,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 0, 0);

	label3 = gtk_label_new (_("RPMs:"));
	gtk_widget_show (label3);
	gtk_misc_set_alignment (GTK_MISC (label3), 0, -1);
	gtk_table_attach (GTK_TABLE (table1), label3, 0, 1, 2, 3,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 0, 0);

	label4 = gtk_label_new (_("SRPMs:"));
	gtk_widget_show (label4);
	gtk_misc_set_alignment (GTK_MISC (label4), 0, -1);
	gtk_table_attach (GTK_TABLE (table1), label4, 0, 1, 3, 4,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 0, 0);

	frame1 = gtk_frame_new (_(" History size "));
	gtk_widget_show (frame1);
	gtk_box_pack_start (GTK_BOX (vbox1), frame1, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	table1 = gtk_table_new (2, 7, FALSE);
	gtk_widget_show (table1);
	gtk_container_add (GTK_CONTAINER (frame1), table1);

	label4 = gtk_label_new (_("Recent Projects:"));
	gtk_widget_show (label4);
	gtk_table_attach (GTK_TABLE (table1), label4, 0, 1, 0, 1,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_padding (GTK_MISC (label4), 5, 5);

	label5 = gtk_label_new (_("Recent Files:"));
	gtk_widget_show (label5);
	gtk_table_attach (GTK_TABLE (table1), label5, 2, 3, 0, 1,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 5, 0);
	gtk_misc_set_padding (GTK_MISC (label5), 0, 6);

	label6 = gtk_label_new (_("Combo Popdown:"));
	gtk_widget_show (label6);
	gtk_table_attach (GTK_TABLE (table1), label6, 4, 5, 0, 1,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 5, 0);
	gtk_misc_set_padding (GTK_MISC (label6), 0, 5);

	boxLastPrj = gtk_check_button_new_with_label (_("Load automatically last project"));
	gtk_widget_show (boxLastPrj);
	gtk_table_attach (GTK_TABLE (table1), boxLastPrj, 6, 7, 1, 2,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 5, 0);

	eventbox3 = gtk_event_box_new ();
	gtk_widget_show (eventbox3);
	gtk_table_attach (GTK_TABLE (table1), eventbox3, 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (eventbox3), 5);

	spinbutton2_adj = gtk_adjustment_new (8, 0, 20, 1, 4, 4);
	spinbutton2 =
		gtk_spin_button_new (GTK_ADJUSTMENT (spinbutton2_adj), 1, 0);
	gtk_widget_show (spinbutton2);
	gtk_container_add (GTK_CONTAINER (eventbox3), spinbutton2);

	eventbox4 = gtk_event_box_new ();
	gtk_widget_show (eventbox4);
	gtk_table_attach (GTK_TABLE (table1), eventbox4, 2, 3, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (eventbox4), 5);

	spinbutton3_adj = gtk_adjustment_new (8, 0, 20, 1, 4, 4);
	spinbutton3 =
		gtk_spin_button_new (GTK_ADJUSTMENT (spinbutton3_adj), 1, 0);
	gtk_widget_show (spinbutton3);
	gtk_container_add (GTK_CONTAINER (eventbox4), spinbutton3);

	eventbox5 = gtk_event_box_new ();
	gtk_widget_show (eventbox5);
	gtk_table_attach (GTK_TABLE (table1), eventbox5, 4, 5, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (eventbox5), 5);

	spinbutton4_adj = gtk_adjustment_new (8, 0, 20, 1, 4, 4);
	spinbutton4 =
		gtk_spin_button_new (GTK_ADJUSTMENT (spinbutton4_adj), 1, 0);
	gtk_widget_show (spinbutton4);
	gtk_container_add (GTK_CONTAINER (eventbox5), spinbutton4);

	vseparator3 = gtk_vseparator_new ();
	gtk_widget_show (vseparator3);
	gtk_table_attach (GTK_TABLE (table1), vseparator3, 5, 6, 0, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);

	vseparator2 = gtk_vseparator_new ();
	gtk_widget_show (vseparator2);
	gtk_table_attach (GTK_TABLE (table1), vseparator2, 3, 4, 0, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);

	vseparator1 = gtk_vseparator_new ();
	gtk_widget_show (vseparator1);
	gtk_table_attach (GTK_TABLE (table1), vseparator1, 1, 2, 0, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);

	frame1 = gtk_frame_new (_(" Job Options "));
	gtk_widget_show (frame1);
	gtk_box_pack_start (GTK_BOX (vbox1), frame1, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	table2 = gtk_table_new (1, 2, TRUE);
	gtk_widget_show (table2);
	gtk_container_add (GTK_CONTAINER (frame1), table2);

	checkbutton1 =
		gtk_check_button_new_with_label (_("Beep on job complete"));
	gtk_widget_show (checkbutton1);
	gtk_table_attach (GTK_TABLE (table2), checkbutton1, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (checkbutton1), 5);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton1), TRUE);

	checkbutton2 =
		gtk_check_button_new_with_label (_("Dialog on job complete"));
	gtk_widget_show (checkbutton2);
	gtk_table_attach (GTK_TABLE (table2), checkbutton2, 1, 2, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (checkbutton2), 5);

	pr->widgets.prj_dir_entry = entry1;
	pr->widgets.tarballs_dir_entry = entry2;
	pr->widgets.rpms_dir_entry = entry3;
	pr->widgets.srpms_dir_entry = entry4;
	pr->widgets.recent_prj_spin = spinbutton2;
	pr->widgets.recent_files_spin = spinbutton3;
	pr->widgets.combo_history_spin = spinbutton4;
	pr->widgets.beep_check = checkbutton1;
	pr->widgets.dialog_check = checkbutton2;
	pr->widgets.lastprj_check = boxLastPrj;

	gtk_widget_ref (entry1);
	gtk_widget_ref (entry2);
	gtk_widget_ref (entry3);
	gtk_widget_ref (entry4);
	gtk_widget_ref (spinbutton2);
	gtk_widget_ref (spinbutton3);
	gtk_widget_ref (spinbutton4);
	gtk_widget_ref (checkbutton1);
	gtk_widget_ref (checkbutton2);
	gtk_widget_ref (boxLastPrj);
	

	return vbox1;
}

static GtkWidget *
create_preferences_page1 (Preferences * p)
{
	GtkWidget *frame1;
	GtkWidget *vbox1;
	GtkWidget *checkbutton1;
	GtkWidget *checkbutton2;
	GtkWidget *checkbutton3;
	GtkWidget *checkbutton6;
	GtkWidget *checkbutton7;
	GtkWidget *hseparator1;
	GtkWidget *hbox1;
	GtkWidget *label1;
	GtkObject *spinbutton1_adj;
	GtkWidget *spinbutton1;

	frame1 = gtk_frame_new (NULL);
	gtk_widget_show (frame1);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER (frame1), vbox1);

	checkbutton1 =
		gtk_check_button_new_with_label (_
						 ("Keep going when some targets cannot be made"));
	gtk_widget_show (checkbutton1);
	gtk_box_pack_start (GTK_BOX (vbox1), checkbutton1, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (checkbutton1), 5);

	checkbutton2 =
		gtk_check_button_new_with_label (_
						 ("Silent: do not echo commands"));
	gtk_widget_show (checkbutton2);
	gtk_box_pack_start (GTK_BOX (vbox1), checkbutton2, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (checkbutton2), 5);

	checkbutton3 =
		gtk_check_button_new_with_label (_
						 ("Produce debugging outputs"));
	gtk_widget_show (checkbutton3);
	gtk_box_pack_start (GTK_BOX (vbox1), checkbutton3, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (checkbutton3), 5);

	checkbutton6 =
		gtk_check_button_new_with_label (_
						 ("Warn when an undefined variable is referenced"));
	gtk_widget_show (checkbutton6);
	gtk_box_pack_start (GTK_BOX (vbox1), checkbutton6, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (checkbutton6), 5);

	checkbutton7 =
		gtk_check_button_new_with_label(_
						("Autosave before Build"));
	gtk_widget_show(checkbutton7);
	gtk_box_pack_start(GTK_BOX (vbox1), checkbutton7, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (checkbutton7), 5);
	

	hseparator1 = gtk_hseparator_new ();
	gtk_widget_show (hseparator1);
	gtk_box_pack_start (GTK_BOX (vbox1), hseparator1, FALSE, FALSE, 0);

	hbox1 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox1);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox1, FALSE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox1), 5);

	label1 = gtk_label_new (_("Max number of jobs:"));
	gtk_widget_show (label1);
	gtk_box_pack_start (GTK_BOX (hbox1), label1, FALSE, TRUE, 0);
	gtk_misc_set_padding (GTK_MISC (label1), 19, 0);

	spinbutton1_adj = gtk_adjustment_new (1, 0, 100, 1, 10, 10);
	spinbutton1 =
		gtk_spin_button_new (GTK_ADJUSTMENT (spinbutton1_adj), 1, 0);
	gtk_widget_show (spinbutton1);
	gtk_box_pack_start (GTK_BOX (hbox1), spinbutton1, FALSE, FALSE, 0);

	p->widgets.build_keep_going_check = checkbutton1;
	p->widgets.build_silent_check = checkbutton2;
	p->widgets.build_debug_check = checkbutton3;
	p->widgets.build_warn_undef_check = checkbutton6;
	p->widgets.build_jobs_spin = spinbutton1;
	p->widgets.build_autosave_check = checkbutton7;
		
	gtk_widget_ref (checkbutton1);
	gtk_widget_ref (checkbutton2);
	gtk_widget_ref (checkbutton3);
	gtk_widget_ref (checkbutton6);
	gtk_widget_ref (spinbutton1);
	gtk_widget_ref (checkbutton7);
	
	return frame1;
}

static GtkWidget *
create_preferences_page2 (Preferences * p)
{
	GtkWidget *window1;
	GtkWidget *frame;
	GtkWidget *vbox1;
	GtkWidget *hbox1;
	GtkWidget *label2;
	GtkWidget *combo2;
	GtkWidget *combo_entry2;
	GtkWidget *frame1;
	GtkWidget *table1;
	GtkObject *spinbutton1_adj;
	GtkWidget *spinbutton1;
	GtkWidget *checkbutton1;
	GtkWidget *hbox2;
	GtkWidget *checkbutton2;
	GtkWidget *checkbutton3;
	GtkWidget *checkbutton4;
	GtkWidget *checkbutton5;
	GtkWidget *label1;
	GtkWidget *label5;
	GtkWidget *label3;
	GtkWidget *label4;
	GtkWidget *colorpicker1;
	GtkWidget *colorpicker2;
	GtkWidget *checkbutton6;
	GtkWidget *checkbutton7;
	GtkWidget *table2;
	GtkWidget *label6;
	GtkWidget *label7;
	GtkWidget *label8;
	GtkWidget *label9;
	GtkWidget *colorpicker3;
	GtkWidget *colorpicker4;
	GtkWidget *colorpicker5;
	GtkWidget *colorpicker6;
	GtkWidget *fontpicker1;
	GList *s_list;
	gint i;

	window1 = p->widgets.window;

	frame = gtk_frame_new (NULL);
	gtk_widget_show (frame);
	gtk_container_set_border_width (GTK_CONTAINER (frame), 5);
	
	vbox1 = gtk_vbox_new (FALSE, 5);
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER(frame), vbox1);

	hbox1 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox1);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox1, FALSE, TRUE, 0);

	label2 = gtk_label_new (_("Select Highlight item to edit:"));
	gtk_widget_show (label2);
	gtk_box_pack_start (GTK_BOX (hbox1), label2, FALSE, FALSE, 0);
	gtk_misc_set_padding (GTK_MISC (label2), 5, 0);

	combo2 = gtk_combo_new ();
	gtk_widget_show (combo2);
	gtk_box_pack_start (GTK_BOX (hbox1), combo2, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (combo2), 5);
	gtk_entry_set_editable (GTK_ENTRY(GTK_COMBO(combo2)->entry), FALSE);

	s_list = NULL;
	for (i = 0;; i += 2)
	{
		if (hilite_style[i] == NULL)
			break;
		s_list = g_list_append (s_list, hilite_style[i]);
	}
	gtk_combo_set_popdown_strings (GTK_COMBO (combo2), s_list);
	g_list_free (s_list);

	combo_entry2 = GTK_COMBO (combo2)->entry;
	gtk_widget_show (combo_entry2);

	frame1 = gtk_frame_new (_(" Highlight Style "));
	gtk_widget_show (frame1);
	gtk_box_pack_start (GTK_BOX (vbox1), frame1, FALSE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	table1 = gtk_table_new (4, 4, FALSE);
	gtk_widget_show (table1);
	gtk_container_add (GTK_CONTAINER (frame1), table1);
	gtk_container_set_border_width (GTK_CONTAINER (table1), 5);
	gtk_table_set_row_spacings (GTK_TABLE (table1), 2);
	gtk_table_set_col_spacings (GTK_TABLE (table1), 5);

	fontpicker1 = gnome_font_picker_new ();
	gtk_widget_show (fontpicker1);
	gtk_widget_set_usize (fontpicker1, 200, -1);
	gnome_font_picker_set_mode (GNOME_FONT_PICKER(fontpicker1), GNOME_FONT_PICKER_MODE_FONT_INFO);
	gnome_font_picker_fi_set_use_font_in_label (GNOME_FONT_PICKER(fontpicker1), TRUE, 14);
	gnome_font_picker_fi_set_show_size(GNOME_FONT_PICKER(fontpicker1), FALSE);
	gtk_table_attach (GTK_TABLE (table1), fontpicker1, 1, 2, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	spinbutton1_adj = gtk_adjustment_new (12, 0, 100, 1, 10, 10);
	spinbutton1 =
		gtk_spin_button_new (GTK_ADJUSTMENT (spinbutton1_adj), 1, 0);
	gtk_widget_show (spinbutton1);
	gtk_widget_set_usize (spinbutton1, 15, -1);
	gtk_table_attach (GTK_TABLE (table1), spinbutton1, 2, 3, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	checkbutton1 = gtk_check_button_new_with_label (_("Use default"));
	gtk_widget_show (checkbutton1);
	gtk_table_attach (GTK_TABLE (table1), checkbutton1, 3, 4, 0, 1,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton1), TRUE);

	hbox2 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox2);
	gtk_table_attach (GTK_TABLE (table1), hbox2, 1, 3, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

	checkbutton2 = gtk_check_button_new_with_label (_("Bold"));
	gtk_widget_show (checkbutton2);
	gtk_box_pack_start (GTK_BOX (hbox2), checkbutton2, FALSE, FALSE, 0);

	checkbutton3 = gtk_check_button_new_with_label (_("Italic"));
	gtk_widget_show (checkbutton3);
	gtk_box_pack_start (GTK_BOX (hbox2), checkbutton3, FALSE, FALSE, 0);

	checkbutton4 = gtk_check_button_new_with_label (_("Underlined"));
	gtk_widget_show (checkbutton4);
	gtk_box_pack_start (GTK_BOX (hbox2), checkbutton4, FALSE, FALSE, 0);

	checkbutton5 = gtk_check_button_new_with_label (_("Use default"));
	gtk_widget_show (checkbutton5);
	gtk_table_attach (GTK_TABLE (table1), checkbutton5, 3, 4, 1, 2,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton5), TRUE);

	label1 = gtk_label_new (_("Font:"));
	gtk_widget_show (label1);
	gtk_misc_set_alignment (GTK_MISC (label1), 0, -1);
	gtk_table_attach (GTK_TABLE (table1), label1, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	label5 = gtk_label_new (_("Attribute:"));
	gtk_widget_show (label5);
	gtk_misc_set_alignment (GTK_MISC (label5), 0, -1);
	gtk_table_attach (GTK_TABLE (table1), label5, 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	label3 = gtk_label_new (_("Fore color:"));
	gtk_widget_show (label3);
	gtk_misc_set_alignment (GTK_MISC (label3), 0, -1);
	gtk_table_attach (GTK_TABLE (table1), label3, 0, 1, 2, 3,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	label4 = gtk_label_new (_("Back color:"));
	gtk_widget_show (label4);
	gtk_misc_set_alignment (GTK_MISC (label4), 0, -1);
	gtk_table_attach (GTK_TABLE (table1), label4, 0, 1, 3, 4,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	colorpicker1 = gnome_color_picker_new ();
	gtk_widget_show (colorpicker1);
	gtk_table_attach (GTK_TABLE (table1), colorpicker1, 1, 2, 2, 3,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	colorpicker2 = gnome_color_picker_new ();
	gtk_widget_show (colorpicker2);
	gtk_table_attach (GTK_TABLE (table1), colorpicker2, 1, 2, 3, 4,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	checkbutton6 = gtk_check_button_new_with_label (_("Use default"));
	gtk_widget_show (checkbutton6);
	gtk_table_attach (GTK_TABLE (table1), checkbutton6, 3, 4, 2, 3,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton6), TRUE);

	checkbutton7 = gtk_check_button_new_with_label (_("Use default"));
	gtk_widget_show (checkbutton7);
	gtk_table_attach (GTK_TABLE (table1), checkbutton7, 3, 4, 3, 4,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton7), TRUE);

	table2 = gtk_table_new (4, 4, FALSE);
	gtk_widget_show (table2);
	gtk_box_pack_start (GTK_BOX (vbox1), table2, TRUE, TRUE, 2);
	gtk_container_set_border_width (GTK_CONTAINER (table2), 5);
	gtk_table_set_row_spacings (GTK_TABLE (table2), 2);
	gtk_table_set_col_spacings (GTK_TABLE (table2), 5);

	label6 = gtk_label_new (_("Caret color:"));
	gtk_widget_show (label6);
	gtk_misc_set_alignment (GTK_MISC (label6), 0, -1);
	gtk_table_attach (GTK_TABLE (table2), label6, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	colorpicker3 = gnome_color_picker_new ();
	gtk_widget_show (colorpicker3);
	gtk_table_attach (GTK_TABLE (table2), colorpicker3, 1, 2, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	label7 = gtk_label_new (_("Calltip color:"));
	gtk_widget_show (label7);
	gtk_misc_set_alignment (GTK_MISC (label7), 0, -1);
	gtk_table_attach (GTK_TABLE (table2), label7, 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	colorpicker4 = gnome_color_picker_new ();
	gtk_widget_show (colorpicker4);
	gtk_table_attach (GTK_TABLE (table2), colorpicker4, 1, 2, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	label8 = gtk_label_new (_("Selection Foreground:"));
	gtk_widget_show (label8);
	gtk_misc_set_alignment (GTK_MISC (label8), 0, -1);
	gtk_table_attach (GTK_TABLE (table2), label8, 2, 3, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	colorpicker5 = gnome_color_picker_new ();
	gtk_widget_show (colorpicker5);
	gtk_table_attach (GTK_TABLE (table2), colorpicker5, 3, 4, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);
	
	label9 = gtk_label_new (_("Selection Background:"));
	gtk_widget_show (label9);
	gtk_misc_set_alignment (GTK_MISC (label9), 0, -1);
	gtk_table_attach (GTK_TABLE (table2), label9, 2, 3, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	colorpicker6 = gnome_color_picker_new ();
	gtk_widget_show (colorpicker6);
	gtk_table_attach (GTK_TABLE (table2), colorpicker6, 3, 4, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);
	
	gtk_signal_connect (GTK_OBJECT (combo_entry2), "changed",
			    GTK_SIGNAL_FUNC (on_hilite_style_entry_changed), p);
	gtk_signal_connect (GTK_OBJECT (checkbutton1), "toggled",
			    GTK_SIGNAL_FUNC (on_use_default_font_toggled), p);
	gtk_signal_connect (GTK_OBJECT (checkbutton5), "toggled",
			    GTK_SIGNAL_FUNC (on_use_default_attrib_toggled), p);
	gtk_signal_connect (GTK_OBJECT (checkbutton6), "toggled",
			    GTK_SIGNAL_FUNC (on_use_default_fore_toggled), p);
	gtk_signal_connect (GTK_OBJECT (checkbutton7), "toggled",
			    GTK_SIGNAL_FUNC (on_use_default_back_toggled), p);

	p->widgets.font_picker = fontpicker1;
	p->widgets.font_size_spin = spinbutton1;
	p->widgets.hilite_item_combo = combo2;
	p->widgets.font_bold_check = checkbutton2;
	p->widgets.font_italics_check = checkbutton3;
	p->widgets.font_underlined_check = checkbutton4;
	p->widgets.fore_colorpicker = colorpicker1;
	p->widgets.back_colorpicker = colorpicker2;
	p->widgets.font_use_default_check = checkbutton1;
	p->widgets.font_attrib_use_default_check = checkbutton5;
	p->widgets.fore_color_use_default_check = checkbutton6;
	p->widgets.back_color_use_default_check = checkbutton7;
	p->widgets.caret_fore_color = colorpicker3;
	p->widgets.calltip_back_color = colorpicker4;
	p->widgets.selection_fore_color = colorpicker5;
	p->widgets.selection_back_color = colorpicker6;
	
	gtk_widget_ref (fontpicker1);
	gtk_widget_ref (combo2);
	gtk_widget_ref (spinbutton1);
	gtk_widget_ref (checkbutton1);
	gtk_widget_ref (checkbutton2);
	gtk_widget_ref (checkbutton3);
	gtk_widget_ref (checkbutton4);
	gtk_widget_ref (checkbutton5);
	gtk_widget_ref (checkbutton6);
	gtk_widget_ref (checkbutton7);
	gtk_widget_ref (colorpicker1);
	gtk_widget_ref (colorpicker2);
	gtk_widget_ref (colorpicker3);
	gtk_widget_ref (colorpicker4);
	gtk_widget_ref (colorpicker5);
	gtk_widget_ref (colorpicker6);

	return frame;
}

static GtkWidget *
create_preferences_page3 (Preferences * p)
{
	GtkWidget *frame1;
	GtkWidget *vbox2;
	GtkWidget *table2;
	GtkWidget *checkbutton9;
	GtkObject *spinbutton2_adj;
	GtkWidget *spinbutton2;
	GtkObject *spinbutton3_adj;
	GtkWidget *spinbutton3;
	GtkWidget *checkbutton10;
	GtkWidget *checkbutton12;
	GtkWidget *checkbutton11;
	GtkWidget *checkbutton13;
	GtkWidget *checkbutton8;
	GtkObject *spinbutton4_adj;
	GtkWidget *spinbutton4;
	GtkWidget *vseparator1;
	GtkObject *spinbutton5_adj;
	GtkWidget *spinbutton5;
	GtkObject *spinbutton6_adj;
	GtkWidget *spinbutton6;
	GtkWidget *label6;
	GtkWidget *label7;
	GtkWidget *label8;
	GtkWidget *label9;
	GtkWidget *label10;
	GtkWidget *checkbutton4;
	GtkWidget *checkbutton5;

	frame1 = gtk_frame_new (NULL);
	gtk_widget_show (frame1);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox2);
	gtk_container_add (GTK_CONTAINER (frame1), vbox2);

	table2 = gtk_table_new (8, 4, FALSE);
	gtk_widget_show (table2);
	gtk_box_pack_start (GTK_BOX (vbox2), table2, TRUE, TRUE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table2), 2);
	gtk_table_set_col_spacings (GTK_TABLE (table2), 5);
	gtk_container_set_border_width (GTK_CONTAINER(table2), 5);

	checkbutton8 =
		gtk_check_button_new_with_label (_
						 ("Disable syntax highlighting"));
	gtk_widget_show (checkbutton8);
	gtk_table_attach (GTK_TABLE (table2), checkbutton8, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	checkbutton9 =
		gtk_check_button_new_with_label (_("Enable Auto save"));
	gtk_widget_show (checkbutton9);
	gtk_table_attach (GTK_TABLE (table2), checkbutton9, 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	checkbutton10 =
		gtk_check_button_new_with_label (_("Enable Auto indent"));
	gtk_widget_show (checkbutton10);
	gtk_table_attach (GTK_TABLE (table2), checkbutton10, 0, 1, 2, 3,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	checkbutton11 =
		gtk_check_button_new_with_label (_
						 ("Use tabs for indentation"));
	gtk_widget_show (checkbutton11);
	gtk_table_attach (GTK_TABLE (table2), checkbutton11, 0, 1, 3, 4,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	checkbutton12 =
		gtk_check_button_new_with_label (_("Enable braces check"));
	gtk_widget_show (checkbutton12);
	gtk_table_attach (GTK_TABLE (table2), checkbutton12, 0, 1, 4, 5,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);
			  
	checkbutton13 =
		gtk_check_button_new_with_label(_("Filter extraneous chars in DOS mode"));
	gtk_widget_show (checkbutton13);
	gtk_table_attach (GTK_TABLE (table2), checkbutton13, 0, 1, 7, 8,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (0), 0, 0);

	vseparator1 = gtk_vseparator_new ();
	gtk_widget_show (vseparator1);
	gtk_table_attach (GTK_TABLE (table2), vseparator1, 1, 2, 0, 8,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (GTK_FILL), 6, 0);

	spinbutton5_adj = gtk_adjustment_new (1, 0, 100, 1, 10, 10);
	spinbutton5 =
		gtk_spin_button_new (GTK_ADJUSTMENT (spinbutton5_adj), 1, 0);
	gtk_widget_show (spinbutton5);
	gtk_table_attach (GTK_TABLE (table2), spinbutton5, 2, 3, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	spinbutton2_adj = gtk_adjustment_new (1, 0, 100, 1, 10, 10);
	spinbutton2 =
		gtk_spin_button_new (GTK_ADJUSTMENT (spinbutton2_adj), 1, 0);
	gtk_widget_show (spinbutton2);
	gtk_table_attach (GTK_TABLE (table2), spinbutton2, 2, 3, 1, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	spinbutton3_adj = gtk_adjustment_new (1, 0, 100, 1, 10, 10);
	spinbutton3 =
		gtk_spin_button_new (GTK_ADJUSTMENT (spinbutton3_adj), 1, 0);
	gtk_widget_show (spinbutton3);
	gtk_table_attach (GTK_TABLE (table2), spinbutton3, 2, 3, 2, 3,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	spinbutton4_adj = gtk_adjustment_new (1, 0, 100, 1, 10, 10);
	spinbutton4 =
		gtk_spin_button_new (GTK_ADJUSTMENT (spinbutton4_adj), 1, 0);
	gtk_widget_show (spinbutton4);
	gtk_table_attach (GTK_TABLE (table2), spinbutton4, 2, 3, 3, 4,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	spinbutton6_adj = gtk_adjustment_new (1, 0, 100, 1, 10, 10);
	spinbutton6 =
		gtk_spin_button_new (GTK_ADJUSTMENT (spinbutton6_adj), 1, 0);
	gtk_widget_show (spinbutton6);
	gtk_table_attach (GTK_TABLE (table2), spinbutton6, 2, 3, 4, 5,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	label6 = gtk_label_new (_("Tab size in spaces"));
	gtk_widget_show (label6);
	gtk_misc_set_alignment (GTK_MISC (label6), 0, -1);
	gtk_table_attach (GTK_TABLE (table2), label6, 3, 4, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 5, 0);

	label7 = gtk_label_new (_("Auto save timer interval in minutes"));
	gtk_widget_show (label7);
	gtk_misc_set_alignment (GTK_MISC (label7), 0, -1);
	gtk_table_attach (GTK_TABLE (table2), label7, 3, 4, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 5, 0);

	label8 = gtk_label_new (_("Auto indent size in spaces"));
	gtk_widget_show (label8);
	gtk_misc_set_alignment (GTK_MISC (label8), 0, -1);
	gtk_table_attach (GTK_TABLE (table2), label8, 3, 4, 2, 3,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 5, 0);

	label9 = gtk_label_new (_("Line number margin width in pixels"));
	gtk_widget_show (label9);
	gtk_misc_set_alignment (GTK_MISC (label9), 0, -1);
	gtk_table_attach (GTK_TABLE (table2), label9, 3, 4, 3, 4,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 5, 0);

	label10 = gtk_label_new (_("Save session time interval"));
	gtk_widget_show (label10);
	gtk_misc_set_alignment (GTK_MISC (label10), 0, -1);
	gtk_table_attach (GTK_TABLE (table2), label10, 3, 4, 4, 5,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 5, 0);

	checkbutton4 =
		gtk_check_button_new_with_label (_
						 ("Strip trailing spaces on file save"));
	gtk_widget_show (checkbutton4);
	gtk_table_attach (GTK_TABLE (table2), checkbutton4, 0, 1, 5, 6,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	checkbutton5 =
		gtk_check_button_new_with_label (_
						 ("Collapse all folds on file open"));
	gtk_widget_show (checkbutton5);
	gtk_table_attach (GTK_TABLE (table2), checkbutton5, 0, 1, 6, 7,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	p->widgets.disable_hilite_check = checkbutton8;
	p->widgets.auto_save_check = checkbutton9;
	p->widgets.auto_indent_check = checkbutton10;
	p->widgets.use_tabs_check = checkbutton11;
	p->widgets.braces_check_check = checkbutton12;
	p->widgets.dos_eol_check = checkbutton13;
	p->widgets.tab_size_spin = spinbutton5;
	p->widgets.autosave_timer_spin = spinbutton2;
	p->widgets.autoindent_size_spin = spinbutton3;
	p->widgets.linenum_margin_width_spin = spinbutton4;
	p->widgets.session_timer_spin = spinbutton6;
	
	p->widgets.strip_spaces_check = checkbutton4;
	p->widgets.fold_on_open_check = checkbutton5;

	gtk_widget_ref (spinbutton2);
	gtk_widget_ref (spinbutton3);
	gtk_widget_ref (spinbutton4);
	gtk_widget_ref (spinbutton5);
	gtk_widget_ref (spinbutton6);
	gtk_widget_ref (checkbutton4);
	gtk_widget_ref (checkbutton5);
	gtk_widget_ref (checkbutton8);
	gtk_widget_ref (checkbutton9);
	gtk_widget_ref (checkbutton10);
	gtk_widget_ref (checkbutton11);
	gtk_widget_ref (checkbutton12);
	gtk_widget_ref (checkbutton13);

	return frame1;
}

static GtkWidget *
create_preferences_page4 (Preferences * p)
{
	GtkWidget *window1;
	GtkWidget *vbox8;
	GtkWidget *frame1;
	GtkWidget *paperselector1;
	GtkWidget *eventbox1;
	GtkWidget *combo1;
	GtkWidget *combo_entry1;
	GList *combo1_items = NULL;

	window1 = p->widgets.window;

	vbox8 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox8);

	frame1 = gtk_frame_new (_(" Paper Size"));
	gtk_widget_show (frame1);
	gtk_box_pack_start (GTK_BOX (vbox8), frame1, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	paperselector1 = gnome_paper_selector_new ();
	gtk_widget_show (paperselector1);
	gtk_container_add (GTK_CONTAINER (frame1), paperselector1);
	gtk_container_set_border_width (GTK_CONTAINER (paperselector1), 5);

	frame1 = gtk_frame_new (_(" Print Command "));
	gtk_widget_show (frame1);
	gtk_box_pack_start (GTK_BOX (vbox8), frame1, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	eventbox1 = gtk_event_box_new ();
	gtk_widget_show (eventbox1);
	gtk_container_add (GTK_CONTAINER (frame1), eventbox1);
	gtk_container_set_border_width (GTK_CONTAINER (eventbox1), 5);

	combo1 = gtk_combo_new ();
	gtk_widget_show (combo1);
	gtk_container_add (GTK_CONTAINER (eventbox1), combo1);
	combo1_items = g_list_append (combo1_items, "lpr");
	gtk_combo_set_popdown_strings (GTK_COMBO (combo1), combo1_items);
	g_list_free (combo1_items);

	combo_entry1 = GTK_COMBO (combo1)->entry;
	gtk_widget_show (combo_entry1);
	gtk_entry_set_text (GTK_ENTRY (combo_entry1), "lpr");


	p->widgets.paperselector = paperselector1;
	p->widgets.pr_command_combo = combo1;
	p->widgets.pr_command_entry = combo_entry1;

	gtk_widget_ref (paperselector1);
	gtk_widget_ref (combo1);
	gtk_widget_ref (combo_entry1);

	return vbox8;
}

static GtkWidget *
create_preferences_page5 (Preferences * p)
{
	GtkWidget *window1;
	GtkWidget *checkbutton1;
	GtkWidget *frame1;
	GtkWidget *vbox1;
	GtkWidget *frame2;
	GtkWidget *combo1;
	GtkWidget *combo_entry1;
	GtkWidget *frame3;
	GtkWidget *vbox2;
	GtkWidget *label1;
	GtkWidget *entry1;
	gint i;
	GList *s_list = NULL;

	window1 = p->widgets.window;

	frame1 = gtk_frame_new (NULL);
	gtk_widget_show (frame1);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER (frame1), vbox1);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), 5);

	checkbutton1 =
		gtk_check_button_new_with_label (_("Disable Auto format"));
	gtk_widget_show (checkbutton1);
	gtk_box_pack_start (GTK_BOX (vbox1), checkbutton1, FALSE, FALSE, 5);

	frame2 = gtk_frame_new (_(" Format Style "));
	gtk_widget_show (frame2);
	gtk_box_pack_start (GTK_BOX (vbox1), frame2, FALSE, FALSE, 5);

	combo1 = gtk_combo_new ();
	gtk_widget_show (combo1);
	gtk_container_add (GTK_CONTAINER (frame2), combo1);
	gtk_container_set_border_width (GTK_CONTAINER (combo1), 5);

	for (i = 0;; i += 2)
	{
		if (format_style[i] == NULL)
			break;
		s_list = g_list_append (s_list, format_style[i]);
	}
	gtk_combo_set_popdown_strings (GTK_COMBO (combo1), s_list);
	g_list_free (s_list);

	combo_entry1 = GTK_COMBO (combo1)->entry;
	gtk_widget_show (combo_entry1);
	gtk_entry_set_editable (GTK_ENTRY (combo_entry1), FALSE);
	gtk_entry_set_text (GTK_ENTRY (combo_entry1), format_style[8]);

	frame3 = gtk_frame_new (_(" Custom Style "));
	gtk_widget_show (frame3);
	gtk_box_pack_start (GTK_BOX (vbox1), frame3, FALSE, FALSE, 0);

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox2);
	gtk_container_add (GTK_CONTAINER (frame3), vbox2);
	gtk_container_set_border_width (GTK_CONTAINER (vbox2), 5);

	label1 =
		gtk_label_new (_
			       ("Enter the command line arguments for the 'indent' program."
				"\nRead the info page for 'indent' for more details."));
	gtk_widget_show (label1);
	gtk_box_pack_start (GTK_BOX (vbox2), label1, FALSE, FALSE, 5);
	gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_LEFT);

	entry1 = gtk_entry_new ();
	gtk_widget_show (entry1);
	gtk_box_pack_start (GTK_BOX (vbox2), entry1, FALSE, FALSE, 5);

	gtk_signal_connect (GTK_OBJECT (combo_entry1), "changed",
			    GTK_SIGNAL_FUNC (on_format_style_entry_changed),
			    p);
	gtk_signal_connect (GTK_OBJECT (checkbutton1), "clicked",
			    GTK_SIGNAL_FUNC (on_format_style_check_clicked),
			    p);

	gtk_widget_ref (checkbutton1);
	p->widgets.format_disable_check = checkbutton1;
	gtk_widget_ref (combo1);
	p->widgets.format_style_combo = combo1;
	gtk_widget_ref (entry1);
	p->widgets.custom_style_entry = entry1;
	gtk_widget_ref (frame2);
	p->widgets.format_frame1 = frame2;
	gtk_widget_ref (frame3);
	p->widgets.format_frame2 = frame3;

	return frame1;
}

static GtkWidget *
create_preferences_pagemsg (Preferences* p)
{
	GtkWidget *frame1;
	GtkWidget *vbox1;
	GtkWidget *frame2;
	GtkWidget *table1;
	GtkWidget *checkbutton1;
	GtkObject *spinbutton1_adj;
	GtkWidget *spinbutton1;
	GtkObject *spinbutton2_adj;
	GtkWidget *spinbutton2;
	GtkWidget *label1;
	GtkWidget *label2;
	GtkWidget *frame3;
	GtkWidget *table2;
	GtkWidget *radiobutton1;
	GtkWidget *radiobutton2;
	GtkWidget *radiobutton3;
	GtkWidget *radiobutton4;
	GSList *table2_group = NULL;
	GtkWidget* frame_colors;
	GtkWidget* table_colors;
	GtkWidget* color_picker_error;
	GtkWidget* color_picker_warning;
	GtkWidget* color_picker_messages1;
	GtkWidget* color_picker_messages2;
	GtkWidget* label3;
	GtkWidget* label4;
	GtkWidget* label5;
	GtkWidget* label6;
	
	
	gint i;
	
	frame1 = gtk_frame_new (NULL);
	gtk_widget_show (frame1);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER (frame1), vbox1);

	frame2 = gtk_frame_new (_(" Messages"));
	gtk_widget_show (frame2);
	gtk_box_pack_start (GTK_BOX (vbox1), frame2, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame2), 5);

	table1 = gtk_table_new (2, 3, FALSE);
	gtk_widget_show (table1);
	gtk_container_add (GTK_CONTAINER (frame2), table1);
	gtk_table_set_row_spacings (GTK_TABLE (table1), 5);
	gtk_table_set_col_spacings (GTK_TABLE (table1), 5);

	checkbutton1 =
		gtk_check_button_new_with_label (_("Truncate long messages"));
	gtk_widget_show (checkbutton1);
	gtk_table_attach (GTK_TABLE (table1), checkbutton1, 0, 3, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (checkbutton1), 5);

	spinbutton1_adj = gtk_adjustment_new (15, 0, 100, 1, 10, 10);
	spinbutton1 =
		gtk_spin_button_new (GTK_ADJUSTMENT (spinbutton1_adj), 1, 0);
	gtk_widget_show (spinbutton1);
	gtk_table_attach (GTK_TABLE (table1), spinbutton1, 0, 1, 1, 2,
			  (GtkAttachOptions) (0), (GtkAttachOptions) (0), 0,
			  0);
	gtk_widget_set_usize (spinbutton1, 60, -2);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton1), TRUE);

	spinbutton2_adj = gtk_adjustment_new (15, 0, 100, 1, 10, 10);
	spinbutton2 =
		gtk_spin_button_new (GTK_ADJUSTMENT (spinbutton2_adj), 1, 0);
	gtk_widget_show (spinbutton2);
	gtk_table_attach (GTK_TABLE (table1), spinbutton2, 0, 1, 2, 3,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_widget_set_usize (spinbutton2, 60, -2);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton2), TRUE);
		
	label1 = gtk_label_new (_("Number of first characters to show"));
	gtk_widget_show (label1);
	gtk_table_attach (GTK_TABLE (table1), label1, 1, 2, 1, 2,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 5, 0);
	gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_LEFT);

	label2 = gtk_label_new (_("Number of last characters to show"));
	gtk_widget_show (label2);
	gtk_table_attach (GTK_TABLE (table1), label2, 1, 2, 2, 3,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 5, 0);
	gtk_label_set_justify (GTK_LABEL (label2), GTK_JUSTIFY_LEFT);

	
	frame3 = gtk_frame_new (_(" Notebook tags position "));
	gtk_widget_show (frame3);
	gtk_box_pack_start (GTK_BOX (vbox1), frame3, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame3), 5);

	table2 = gtk_table_new (4, 2, FALSE);
	gtk_widget_show (table2);
	gtk_container_add (GTK_CONTAINER (frame3), table2);
	radiobutton1 =
		gtk_radio_button_new_with_label (table2_group, _("Top"));
	table2_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton1));
	gtk_widget_show (radiobutton1);
	gtk_table_attach (GTK_TABLE (table2), radiobutton1, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	radiobutton2 =
		gtk_radio_button_new_with_label (table2_group, _("Bottom"));
	table2_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton2));
	gtk_widget_show (radiobutton2);
	gtk_table_attach (GTK_TABLE (table2), radiobutton2, 1, 2, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	radiobutton3 =
		gtk_radio_button_new_with_label (table2_group, _("Left"));
	table2_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton3));
	gtk_widget_show (radiobutton3);
	gtk_table_attach (GTK_TABLE (table2), radiobutton3, 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	radiobutton4 =
		gtk_radio_button_new_with_label (table2_group, _("Right"));
	table2_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton4));
	gtk_widget_show (radiobutton4);
	gtk_table_attach (GTK_TABLE (table2), radiobutton4, 1, 2, 1, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);


	gtk_widget_ref (checkbutton1);
	p->widgets.truncat_mesg_check = checkbutton1;
	gtk_widget_ref (spinbutton1);
	p->widgets.mesg_first_spin = spinbutton1;
	gtk_widget_ref (spinbutton2);
	p->widgets.mesg_last_spin = spinbutton2;

	on_trunc_mesg_check_clicked (GTK_BUTTON (p->widgets.truncat_mesg_check), p);
	
	p->widgets.tag_pos_msg_radio[0] = radiobutton1;
	p->widgets.tag_pos_msg_radio[1] = radiobutton2;
	p->widgets.tag_pos_msg_radio[2] = radiobutton3;
	p->widgets.tag_pos_msg_radio[3] = radiobutton4;
	for (i = 0; i < 4; i++)
		gtk_widget_ref (p->widgets.tag_pos_msg_radio[i]);

	gtk_signal_connect (GTK_OBJECT (checkbutton1), "clicked",
			    GTK_SIGNAL_FUNC (on_trunc_mesg_check_clicked), p);
	
	frame_colors = gtk_frame_new(_("Colors"));
	table_colors = gtk_table_new(4, 2, FALSE);
	
	color_picker_error = gnome_color_picker_new();
	color_picker_warning = gnome_color_picker_new();
	color_picker_messages1 = gnome_color_picker_new();
	color_picker_messages2 = gnome_color_picker_new();

	label3 = gtk_label_new(_("Errors:"));
	label4 = gtk_label_new(_("Warnings:"));
	label5 = gtk_label_new(_("Program messages:"));
	label6 = gtk_label_new(_("Other messages:"));
	
	gtk_table_attach (GTK_TABLE (table_colors), label3, 0, 1, 0, 1,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 5, 0);
	gtk_table_attach (GTK_TABLE (table_colors), label4, 0, 1, 1, 2,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 5, 0);
	gtk_table_attach (GTK_TABLE (table_colors), label5, 2, 3, 0, 1,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 5, 0);
	gtk_table_attach (GTK_TABLE (table_colors), label6, 2, 3, 1, 2,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 5, 0);

	gtk_table_attach (GTK_TABLE (table_colors), color_picker_error, 1, 2, 0, 1,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 5, 0);
	gtk_table_attach (GTK_TABLE (table_colors), color_picker_warning, 1, 2, 1, 2,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 5, 0);
	gtk_table_attach (GTK_TABLE (table_colors), color_picker_messages1, 3, 4, 0, 1,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 5, 0);
	gtk_table_attach (GTK_TABLE (table_colors), color_picker_messages2, 3, 4, 1, 2,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 5, 0);
	
	gtk_widget_ref(color_picker_error);
	p->widgets.color_picker[0] = color_picker_error;
	gtk_widget_ref(color_picker_warning);
	p->widgets.color_picker[1] = color_picker_warning;
	gtk_widget_ref(color_picker_messages1);
	p->widgets.color_picker[2] = color_picker_messages1;
	gtk_widget_ref(color_picker_messages2);
	p->widgets.color_picker[3] = color_picker_messages2;
	
	gtk_container_add(GTK_CONTAINER(frame_colors), table_colors);
	gtk_container_set_border_width(GTK_CONTAINER(frame_colors), 5);
	gtk_widget_show_all(frame_colors);
	gtk_box_pack_end(GTK_BOX(vbox1), frame_colors, FALSE, FALSE, 0);
	
	return frame1;
}

static GtkWidget *
create_preferences_page7 (Preferences * p)
{
	GtkWidget *window1;
	GtkWidget *frame1;
	GtkWidget *vbox1;
	GtkWidget *table2;
	GtkWidget *frame3;
	GtkWidget *checkbutton6;
	GtkWidget *hseparator1;
	GSList *table2_group = NULL;
	GtkWidget *radiobutton1;
	GtkWidget *radiobutton2;
	GtkWidget *radiobutton3;
	GtkWidget *radiobutton4;
	GtkWidget *frame4;
	GtkWidget *checkbutton7;
	gint i;

	window1 = p->widgets.window;
	
	frame1 = gtk_frame_new (NULL);
	gtk_widget_show (frame1);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER (frame1), vbox1);
	
	frame3 = gtk_frame_new (_(" Notebook tags position "));
	gtk_widget_show (frame3);
	gtk_box_pack_start (GTK_BOX (vbox1), frame3, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame3), 5);

	table2 = gtk_table_new (4, 2, FALSE);
	gtk_widget_show (table2);
	gtk_container_add (GTK_CONTAINER (frame3), table2);

	checkbutton6 =
		gtk_check_button_new_with_label (_
						 ("Do not show notebook title tags"));
	gtk_widget_show (checkbutton6);
	gtk_table_attach (GTK_TABLE (table2), checkbutton6, 0, 2, 3, 4,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (checkbutton6), 5);

	hseparator1 = gtk_hseparator_new ();
	gtk_widget_show (hseparator1);
	gtk_table_attach (GTK_TABLE (table2), hseparator1, 0, 2, 2, 3,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	frame4 = gtk_frame_new (_(" Tags Browser "));
	gtk_widget_show (frame4);
	gtk_box_pack_start (GTK_BOX (vbox1), frame4, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame4), 5);

	checkbutton7 =
		gtk_check_button_new_with_label (_
						 ("Update tags image automatically"));
	gtk_widget_show (checkbutton7);
	gtk_container_add (GTK_CONTAINER (frame4), checkbutton7);
	gtk_container_set_border_width (GTK_CONTAINER (checkbutton7), 5);

	gtk_signal_connect (GTK_OBJECT (checkbutton6), "clicked",
			    GTK_SIGNAL_FUNC (on_notebook_tag_none_clicked),
			    p);
	
	radiobutton1 =
		gtk_radio_button_new_with_label (table2_group, _("Top"));
	table2_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton1));
	gtk_widget_show (radiobutton1);
	gtk_table_attach (GTK_TABLE (table2), radiobutton1, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	radiobutton2 =
		gtk_radio_button_new_with_label (table2_group, _("Bottom"));
	table2_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton2));
	gtk_widget_show (radiobutton2);
	gtk_table_attach (GTK_TABLE (table2), radiobutton2, 1, 2, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	radiobutton3 =
		gtk_radio_button_new_with_label (table2_group, _("Left"));
	table2_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton3));
	gtk_widget_show (radiobutton3);
	gtk_table_attach (GTK_TABLE (table2), radiobutton3, 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	radiobutton4 =
		gtk_radio_button_new_with_label (table2_group, _("Right"));
	table2_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton4));
	gtk_widget_show (radiobutton4);
	gtk_table_attach (GTK_TABLE (table2), radiobutton4, 1, 2, 1, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);
	
	p->widgets.tag_pos_radio[0] = radiobutton1;
	p->widgets.tag_pos_radio[1] = radiobutton2;
	p->widgets.tag_pos_radio[2] = radiobutton3;
	p->widgets.tag_pos_radio[3] = radiobutton4;
	for (i = 0; i < 4; i++)
		gtk_widget_ref (p->widgets.tag_pos_radio[i]);

	gtk_widget_ref (checkbutton6);
	p->widgets.no_tag_check = checkbutton6;
	gtk_widget_ref (checkbutton7);
	p->widgets.tags_update_check = checkbutton7;

	return frame1;
}

static gint
IntFromHexDigit (const gchar ch)
{
	if (isdigit (ch))
		return ch - '0';
	else if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	else if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	else
		return 0;
}

static GtkWidget *
create_preferences_pageComp (Preferences * p)
{
	GtkWidget *window1;
	GtkWidget *frame1;
	GtkWidget *vbox1;
	GtkWidget *frame2;
	GtkWidget *table1;
	GtkWidget *checkbutton1;


	window1 = p->widgets.window;

	frame1 = gtk_frame_new (NULL);
	gtk_widget_show (frame1);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER (frame1), vbox1);

	frame2 = gtk_frame_new (_(" Components"));
	gtk_widget_show (frame2);
	gtk_box_pack_start (GTK_BOX (vbox1), frame2, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame2), 5);

	table1 = gtk_table_new (1, 1, FALSE);
	gtk_widget_show (table1);
	gtk_container_add (GTK_CONTAINER (frame2), table1);
	gtk_table_set_row_spacings (GTK_TABLE (table1), 5);
	gtk_table_set_col_spacings (GTK_TABLE (table1), 5);

	checkbutton1 =
		gtk_check_button_new_with_label (_("Use Glade Components (experimental)"));
	gtk_widget_show (checkbutton1);
	gtk_table_attach (GTK_TABLE (table1), checkbutton1, 0, 3, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (checkbutton1), 5);

	/*gtk_signal_connect (GTK_OBJECT (checkbutton1), "clicked",
			    GTK_SIGNAL_FUNC (on_use_components), p);*/

	gtk_widget_ref (checkbutton1);
	p->widgets.use_components	= checkbutton1;
	
	on_trunc_mesg_check_clicked (GTK_BUTTON (p->widgets.use_components), p);
	
	return frame1;
}


void
ColorFromString (const gchar * val, guint8 * r, guint8 * g, guint8 * b)
{
	*r = IntFromHexDigit (val[1]) * 16 + IntFromHexDigit (val[2]);
	*g = IntFromHexDigit (val[3]) * 16 + IntFromHexDigit (val[4]);
	*b = IntFromHexDigit (val[5]) * 16 + IntFromHexDigit (val[6]);
}

static gchar *
StringFromColor (guint8 r, guint8 g, guint8 b)
{
	gchar str[10];
	guint32 num;

	num = r;
	num <<= 8;
	num += g;
	num <<= 8;
	num += b;

	sprintf (str, "#%06X", num);
	return g_strdup (str);
}

static void
on_preferences_ok_clicked (GtkButton * button, gpointer user_data)
{
	Preferences *pr = (Preferences *) user_data;
	if (pr)
	{
		on_preferences_apply_clicked (NULL, user_data);
		gnome_dialog_close(GNOME_DIALOG(pr->widgets.window));
	}
}

static gboolean
on_preferences_close (GtkWidget * w, gpointer user_data)
{
	Preferences *pr = (Preferences *) user_data;
	if (pr)
	{
		preferences_hide (pr);
	}
	return FALSE;
}

static void
on_preferences_apply_clicked (GtkButton * button, gpointer user_data)
{
	gint i;
	gchar *str;
	gint8 r, g, b, a;

	Preferences *pr = (Preferences *) user_data;

	if (!pr)
		return;

	on_hilite_style_entry_changed (GTK_EDITABLE
				       (GTK_COMBO
					(pr->widgets.hilite_item_combo)->
					entry), pr);

/* Page 0 */
	preferences_set (pr, PROJECTS_DIRECTORY,
			 gtk_entry_get_text (GTK_ENTRY
					     (pr->widgets.prj_dir_entry)));
	preferences_set (pr, TARBALLS_DIRECTORY,
			 gtk_entry_get_text (GTK_ENTRY
					     (pr->widgets.
					      tarballs_dir_entry)));
	preferences_set (pr, RPMS_DIRECTORY,
			 gtk_entry_get_text (GTK_ENTRY
					     (pr->widgets.rpms_dir_entry)));
	preferences_set (pr, SRPMS_DIRECTORY,
			 gtk_entry_get_text (GTK_ENTRY
					     (pr->widgets.srpms_dir_entry)));

	preferences_set_int (pr, BEEP_ON_BUILD_COMPLETE,
			     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
							   (pr->widgets.
							    beep_check)));
	preferences_set_int (pr, DIALOG_ON_BUILD_COMPLETE,
			     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
							   (pr->widgets.
							    dialog_check)));
	preferences_set_int (pr, MAXIMUM_RECENT_PROJECTS,
			     gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
							       (pr->widgets.
								recent_prj_spin)));
	preferences_set_int (pr, MAXIMUM_RECENT_FILES,
			     gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
							       (pr->widgets.
								recent_files_spin)));
	preferences_set_int (pr, MAXIMUM_COMBO_HISTORY,
			     gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
							       (pr->widgets.
								combo_history_spin)));
	preferences_set_int (pr, RELOAD_LAST_PROJECT,
			     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
							   (pr->widgets.
							    lastprj_check)));

/* page 1 */
	preferences_set_int (pr, BUILD_OPTION_KEEP_GOING,
			     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
							   (pr->widgets.build_keep_going_check)));
	
	preferences_set_int (pr, BUILD_OPTION_DEBUG,
			     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
							   (pr->widgets.build_debug_check)));
	
	preferences_set_int (pr, BUILD_OPTION_SILENT,
			     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
							   (pr->widgets.build_silent_check)));
	
	preferences_set_int (pr, BUILD_OPTION_WARN_UNDEF,
			     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
							   (pr->widgets.build_warn_undef_check)));

	preferences_set_int (pr, BUILD_OPTION_AUTOSAVE, 
			     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
				     			   (pr->widgets.build_autosave_check)));
	preferences_set_int (pr, BUILD_OPTION_JOBS,
			     gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
							       (pr->widgets.build_jobs_spin)));


/* page 2 */
	for (i = 0;; i += 2)
	{
		StyleData *sdata;

		if (hilite_style[i] == NULL)
			break;
		sdata =
			gtk_object_get_data (GTK_OBJECT (pr->widgets.window),
					     hilite_style[i]);
		str = style_data_get_string (sdata);
		preferences_set (pr, hilite_style[i + 1], str);
		if (str)
			g_free (str);
	}
	gnome_color_picker_get_i8 (GNOME_COLOR_PICKER
				   (pr->widgets.caret_fore_color), &r,
				   &g, &b, &a);
	str = StringFromColor (r, g, b);
	if(str)
	{
		preferences_set(pr, CARET_FORE_COLOR, str);
		g_free (str);
	}
	gnome_color_picker_get_i8 (GNOME_COLOR_PICKER
				   (pr->widgets.calltip_back_color), &r,
				   &g, &b, &a);
	str = StringFromColor (r, g, b);
	if(str)
	{
		preferences_set(pr, CALLTIP_BACK_COLOR, str);
		g_free (str);
	}
	gnome_color_picker_get_i8 (GNOME_COLOR_PICKER
				   (pr->widgets.selection_fore_color), &r,
				   &g, &b, &a);
	str = StringFromColor (r, g, b);
	if(str)
	{
		preferences_set(pr, SELECTION_FORE_COLOR, str);
		g_free (str);
	}
	gnome_color_picker_get_i8 (GNOME_COLOR_PICKER
				   (pr->widgets.selection_back_color), &r,
				   &g, &b, &a);
	str = StringFromColor (r, g, b);
	if(str)
	{
		preferences_set(pr, SELECTION_BACK_COLOR, str);
		g_free (str);
	}

/* page 3 */
	preferences_set_int (pr, DISABLE_SYNTAX_HILIGHTING,
			     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
							   (pr->widgets.
							    disable_hilite_check)));
	preferences_set_int (pr, SAVE_AUTOMATIC,
			     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
							   (pr->widgets.
							    auto_save_check)));
	preferences_set_int (pr, INDENT_AUTOMATIC,
			     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
							   (pr->widgets.
							    auto_indent_check)));
	preferences_set_int (pr, USE_TABS,
			     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
							   (pr->widgets.
							    use_tabs_check)));
	preferences_set_int (pr, BRACES_CHECK,
			     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
							   (pr->widgets.
							    braces_check_check)));

	preferences_set_int (pr, DOS_EOL_CHECK,
			     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
							   (pr->widgets.
							    dos_eol_check)));

	preferences_set_int (pr, TAB_SIZE,
			     gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
							       (pr->widgets.
								tab_size_spin)));
	preferences_set_int (pr, AUTOSAVE_TIMER,
			     gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
							       (pr->widgets.
								autosave_timer_spin)));
	preferences_set_int (pr, INDENT_SIZE,
			     gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
							       (pr->widgets.
								autoindent_size_spin)));
	preferences_set_int (pr, MARGIN_LINENUMBER_WIDTH,
			     gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
							       (pr->widgets.
								linenum_margin_width_spin)));
	preferences_set_int (pr, SAVE_SESSION_TIMER,
			     gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
							       (pr->widgets.
								session_timer_spin)));

	preferences_set_int (pr, STRIP_TRAILING_SPACES,
			     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
							   (pr->widgets.strip_spaces_check)));
	
	preferences_set_int (pr, FOLD_ON_OPEN,
			     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
							   (pr->widgets.fold_on_open_check)));
/* page 4 */
	preferences_set (pr, COMMAND_PRINT,
			 gtk_entry_get_text (GTK_ENTRY
					     (pr->widgets.pr_command_entry)));

/* Page 5 */
	preferences_set (pr, AUTOFORMAT_STYLE,
			 gtk_entry_get_text (GTK_ENTRY
					     (GTK_COMBO
					      (pr->widgets.
					       format_style_combo)->entry)));
	preferences_set (pr, AUTOFORMAT_CUSTOM_STYLE,
			 gtk_entry_get_text (GTK_ENTRY
					     (pr->widgets.
					      custom_style_entry)));
	preferences_set_int (pr, AUTOFORMAT_DISABLE,
			     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
							   (pr->widgets.
							    format_disable_check)));

/* Page Messages */
	preferences_set_int (pr, TRUNCAT_MESG_FIRST,
			     gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
							       (pr->widgets.
								mesg_first_spin)));
	preferences_set_int (pr, TRUNCAT_MESG_LAST,
			     gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
							       (pr->widgets.
								mesg_last_spin)));
	preferences_set_int (pr, TRUNCAT_MESSAGES,
			     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
							   (pr->widgets.
							    truncat_mesg_check)));
								
	if (gtk_toggle_button_get_active
	    (GTK_TOGGLE_BUTTON (pr->widgets.tag_pos_msg_radio[0])) == TRUE)
	{
		preferences_set (pr, MESSAGES_TAG_POS, "top");
	}
	else
		if (gtk_toggle_button_get_active
		    (GTK_TOGGLE_BUTTON (pr->widgets.tag_pos_msg_radio[1])) ==
		    TRUE)
	{
		preferences_set (pr, MESSAGES_TAG_POS, "bottom");
	}
	else
		if (gtk_toggle_button_get_active
		    (GTK_TOGGLE_BUTTON (pr->widgets.tag_pos_msg_radio[2])) ==
		    TRUE)
	{
		preferences_set (pr, MESSAGES_TAG_POS, "left");
	}
	else
		if (gtk_toggle_button_get_active
		    (GTK_TOGGLE_BUTTON (pr->widgets.tag_pos_msg_radio[3])) ==
		    TRUE)
	{
		preferences_set (pr, MESSAGES_TAG_POS, "right");
	}
	else
	{
		preferences_set (pr, MESSAGES_TAG_POS, "top");
	}
	
	gnome_color_picker_get_i8(GNOME_COLOR_PICKER(pr->widgets.color_picker[0]), &r, &g, &b, &a);
	str = StringFromColor(r, g, b);
	if (str)
	{
		preferences_set(pr, MESSAGES_COLOR_ERROR, str);
		g_free(str);
	}
	
	gnome_color_picker_get_i8(GNOME_COLOR_PICKER(pr->widgets.color_picker[1]), &r, &g, &b, &a);
	str = StringFromColor(r, g, b);
	if (str)
	{
		preferences_set(pr, MESSAGES_COLOR_WARNING, str);
		g_free(str);
	}

	gnome_color_picker_get_i8(GNOME_COLOR_PICKER(pr->widgets.color_picker[2]), &r, &g, &b, &a);
	str = StringFromColor(r, g, b);
	if (str)
	{
		preferences_set(pr, MESSAGES_COLOR_MESSAGES1, str);
		g_free(str);
	}

	gnome_color_picker_get_i8(GNOME_COLOR_PICKER(pr->widgets.color_picker[3]), &r, &g, &b, &a);
	str = StringFromColor(r, g, b);
	if (str)
	{
		preferences_set(pr, MESSAGES_COLOR_MESSAGES2, str);
		g_free(str);
	}

	
/* Page Components */
	preferences_set_int (pr, USE_COMPONENTS,
			     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
							   (pr->widgets.
							    use_components)));

	if (gtk_toggle_button_get_active
	    (GTK_TOGGLE_BUTTON (pr->widgets.tag_pos_radio[0])) == TRUE)
	{
		preferences_set (pr, EDITOR_TAG_POS, "top");
	}
	else
		if (gtk_toggle_button_get_active
		    (GTK_TOGGLE_BUTTON (pr->widgets.tag_pos_radio[1])) ==
		    TRUE)
	{
		preferences_set (pr, EDITOR_TAG_POS, "bottom");
	}
	else
		if (gtk_toggle_button_get_active
		    (GTK_TOGGLE_BUTTON (pr->widgets.tag_pos_radio[2])) ==
		    TRUE)
	{
		preferences_set (pr, EDITOR_TAG_POS, "left");
	}
	else
		if (gtk_toggle_button_get_active
		    (GTK_TOGGLE_BUTTON (pr->widgets.tag_pos_radio[3])) ==
		    TRUE)
	{
		preferences_set (pr, EDITOR_TAG_POS, "right");
	}
	else
	{
		preferences_set (pr, EDITOR_TAG_POS, "top");
	}
	preferences_set_int (pr, EDITOR_TAG_HIDE,
			     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
							   (pr->widgets.
							    no_tag_check)));
	preferences_set_int (pr, AUTOMATIC_TAGS_UPDATE,
			     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
							   (pr->widgets.
							    tags_update_check)));

	anjuta_save_settings ();
	preferences_set_build_options(pr);
/* At last the end */
	anjuta_apply_preferences ();

/* preferences_save_yourself(pr); */
}

static void
on_preferences_cancel_clicked (GtkButton * button, gpointer user_data)
{
	Preferences *pr = (Preferences *) user_data;
	if (NULL != pr)
		gnome_dialog_close(GNOME_DIALOG(pr->widgets.window));
}

static void
on_format_style_entry_changed (GtkEditable * editable, gpointer user_data)
{
	Preferences *pr = user_data;
	if (strcmp
	    (gtk_entry_get_text (GTK_ENTRY (editable)), format_style[0]) == 0)
		gtk_widget_set_sensitive (pr->widgets.format_frame2, TRUE);
	else
		gtk_widget_set_sensitive (pr->widgets.format_frame2, FALSE);
}

static void
on_format_style_check_clicked (GtkButton * but, gpointer user_data)
{
	Preferences *pr = user_data;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (but)) == TRUE)
	{
		gtk_widget_set_sensitive (pr->widgets.format_frame1, FALSE);
		gtk_widget_set_sensitive (pr->widgets.format_frame2, FALSE);
	}
	else
	{
		if (strcmp
		    (gtk_entry_get_text
		     (GTK_ENTRY
		      (GTK_COMBO (pr->widgets.format_style_combo)->entry)),
		     format_style[0]) == 0)
			gtk_widget_set_sensitive (pr->widgets.format_frame2,
						  TRUE);
		else
			gtk_widget_set_sensitive (pr->widgets.format_frame2,
						  FALSE);
		gtk_widget_set_sensitive (pr->widgets.format_frame1, TRUE);
	}
}

void
on_hilite_style_entry_changed (GtkEditable * editable, gpointer user_data)
{
	Preferences *p;
	gchar *style_item;

	p = user_data;

	style_item = gtk_entry_get_text (GTK_ENTRY (editable));
	if (p->current_style)
	{
		guint8 r, g, b, a;
		gchar *str;

		str = NULL;
		if (fontpicker_get_font_name (p->widgets.font_picker, &str))
		{
			style_data_set_font (p->current_style, str);
			g_free (str);
		}
		else
		{
			style_data_set_font (p->current_style, p->default_style->font);
		}
		p->current_style->size =
			gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
							  (p->widgets.
							   font_size_spin));
		p->current_style->bold =
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
						      (p->widgets.
						       font_bold_check));
		p->current_style->italics =
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
						      (p->widgets.
						       font_italics_check));
		p->current_style->underlined =
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
						      (p->widgets.
						       font_underlined_check));
		p->current_style->eolfilled =
			(strcmp (style_item, hilite_style[1]) == 0);

		gnome_color_picker_get_i8 (GNOME_COLOR_PICKER
					   (p->widgets.fore_colorpicker), &r,
					   &g, &b, &a);
		str = StringFromColor (r, g, b);
		style_data_set_fore (p->current_style, str);
		g_free (str);

		gnome_color_picker_get_i8 (GNOME_COLOR_PICKER
					   (p->widgets.back_colorpicker), &r,
					   &g, &b, &a);
		str = StringFromColor (r, g, b);
		style_data_set_back (p->current_style, str);
		g_free (str);

		p->current_style->font_use_default =
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
						      (p->widgets.
						       font_use_default_check));
		p->current_style->attrib_use_default =
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
						      (p->widgets.
						       font_attrib_use_default_check));
		p->current_style->fore_use_default =
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
						      (p->widgets.
						       fore_color_use_default_check));
		p->current_style->back_use_default =
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
						      (p->widgets.
						       back_color_use_default_check));
	}
	p->current_style =
		gtk_object_get_data (GTK_OBJECT (p->widgets.window),
				     style_item);

	/* We need to first toggle then set active to work properly */
	gtk_toggle_button_toggled (GTK_TOGGLE_BUTTON
				   (p->widgets.font_use_default_check));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (p->widgets.font_use_default_check),
				      p->current_style->font_use_default);

	gtk_toggle_button_toggled (GTK_TOGGLE_BUTTON
				   (p->widgets.
				    font_attrib_use_default_check));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (p->widgets.
				       font_attrib_use_default_check),
				      p->current_style->attrib_use_default);

	gtk_toggle_button_toggled (GTK_TOGGLE_BUTTON
				   (p->widgets.fore_color_use_default_check));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (p->widgets.
				       fore_color_use_default_check),
				      p->current_style->fore_use_default);

	gtk_toggle_button_toggled (GTK_TOGGLE_BUTTON
				   (p->widgets.back_color_use_default_check));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (p->widgets.
				       back_color_use_default_check),
				      p->current_style->back_use_default);
}

static void
on_trunc_mesg_check_clicked (GtkButton * button, gpointer user_data)
{
	gboolean state;
	Preferences *pr = user_data;

	state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
	gtk_widget_set_sensitive (pr->widgets.mesg_first_spin, state);
	gtk_widget_set_sensitive (pr->widgets.mesg_last_spin, state);
}

static void
on_notebook_tag_none_clicked (GtkButton * button, gpointer user_data)
{
	gint i, state;
	Preferences *pr = user_data;

	state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
	for (i = 0; i < 4; i++)
		gtk_widget_set_sensitive (pr->widgets.tag_pos_radio[i],
					  !state);
}

StyleData *
style_data_new_parse (gchar * style_string)
{
	gchar *val, *opt;
	StyleData *style_data;

	style_data = style_data_new ();
	if (!style_data)
		return NULL;

	val = g_strdup (style_string);
	opt = val;
	while (opt)
	{
		gchar *cpComma, *colon;

		cpComma = strchr (opt, ',');
		if (cpComma)
			*cpComma = '\0';
		colon = strchr (opt, ':');
		if (colon)
			*colon++ = '\0';
		if (0 == strcmp (opt, "italics"))
		{
			style_data->italics = TRUE;
			style_data->attrib_use_default = FALSE;
		}
		if (0 == strcmp (opt, "notitalics"))
		{
			style_data->italics = FALSE;
			style_data->attrib_use_default = FALSE;
		}
		if (0 == strcmp (opt, "bold"))
		{
			style_data->bold = TRUE;
			style_data->attrib_use_default = FALSE;
		}
		if (0 == strcmp (opt, "notbold"))
		{
			style_data->bold = FALSE;
			style_data->attrib_use_default = FALSE;
		}
		if (0 == strcmp (opt, "font"))
		{
			style_data_set_font (style_data, colon);
			style_data->font_use_default = FALSE;
		}
		if (0 == strcmp (opt, "fore"))
		{
			style_data_set_fore (style_data, colon);
			style_data->fore_use_default = FALSE;
		}
		if (0 == strcmp (opt, "back"))
		{
			style_data_set_back (style_data, colon);
			style_data->back_use_default = FALSE;
		}
		if (0 == strcmp (opt, "size"))
		{
			style_data->size = atoi (colon);
			style_data->font_use_default = FALSE;
		}
		if (0 == strcmp (opt, "eolfilled"))
			style_data->eolfilled = TRUE;
		if (0 == strcmp (opt, "noteolfilled"))
			style_data->eolfilled = FALSE;
		if (0 == strcmp (opt, "underlined"))
		{
			style_data->underlined = TRUE;
			style_data->attrib_use_default = FALSE;
		}
		if (0 == strcmp (opt, "notunderlined"))
		{
			style_data->underlined = FALSE;
			style_data->fore_use_default = FALSE;
		}
		if (cpComma)
			opt = cpComma + 1;
		else
			opt = 0;
	}
	if (val)
		g_free (val);
	return style_data;
}

StyleData *
style_data_new (void)
{
	StyleData *sdata;
	sdata = g_malloc (sizeof (StyleData));
	if (!sdata)
		return NULL;
	/* Initialize to all zero */
	bzero ((char *) sdata, sizeof (StyleData));
	style_data_set_font (sdata, "");
	sdata->size = 12;
	sdata->font_use_default = TRUE;
	sdata->attrib_use_default = TRUE;
	sdata->fore_use_default = TRUE;
	sdata->back_use_default = TRUE;
	style_data_set_fore (sdata, "#000000");	/* Black */
	style_data_set_back (sdata, "#FFFFFF");	/* White */
	return sdata;
}

gchar *
style_data_get_string (StyleData * sdata)
{
	gchar *tmp, *str;

	str = NULL;
	if (strlen (sdata->font) && sdata->font_use_default == FALSE)
	{
		str = g_strconcat ("font:", sdata->font, NULL);
	}
	if (sdata->size > 0 && sdata->font_use_default == FALSE)
	{
		tmp = str;
		if (tmp)
		{
			str =
				g_strdup_printf ("%s,size:%d", tmp,
						 sdata->size);
			g_free (tmp);
		}
		else
			str = g_strdup_printf ("size:%d", sdata->size);

	}
	if (sdata->attrib_use_default == FALSE)
	{
		if (sdata->bold)
		{
			tmp = str;
			if (tmp)
			{
				str = g_strconcat (tmp, ",bold", NULL);
				g_free (tmp);
			}
			else
				str = g_strdup ("bold");
		}
		else
		{
			tmp = str;
			if (tmp)
			{
				str = g_strconcat (tmp, ",notbold", NULL);
				g_free (tmp);
			}
			else
				str = g_strdup ("notbold");
		}
		if (sdata->italics)
		{
			tmp = str;
			if (tmp)
			{
				str = g_strconcat (tmp, ",italics", NULL);
				g_free (tmp);
			}
			else
				str = g_strdup ("italics");
		}
		else
		{
			tmp = str;
			if (tmp)
			{
				str = g_strconcat (tmp, ",notitalics", NULL);
				g_free (tmp);
			}
			else
				str = g_strdup ("notitalics");
		}
		if (sdata->underlined)
		{
			tmp = str;
			if (tmp)
			{
				str = g_strconcat (tmp, ",underlined", NULL);
				g_free (tmp);
			}
			else
				str = g_strdup ("underlined");
		}
		else
		{
			tmp = str;
			if (tmp)
			{
				str =
					g_strconcat (tmp, ",notunderlined",
						     NULL);
				g_free (tmp);
			}
			else
				str = g_strdup ("notunderlined");
		}
		if (sdata->eolfilled)
		{
			tmp = str;
			if (tmp)
			{
				str = g_strconcat (tmp, ",eolfilled", NULL);
				g_free (tmp);
			}
			else
				str = g_strdup ("eolfilled");
		}
		else
		{
			tmp = str;
			if (tmp)
			{
				str =
					g_strconcat (tmp, ",noteolfilled",
						     NULL);
				g_free (tmp);
			}
			else
				str = g_strdup ("noteolfilled");
		}
	}
	if (sdata->fore_use_default == FALSE)
	{
		tmp = str;
		if (tmp)
		{
			str = g_strconcat (tmp, ",fore:", sdata->fore, NULL);
			g_free (tmp);
		}
		else
			str = g_strconcat ("fore:", sdata->fore, NULL);
	}
	if (sdata->back_use_default == FALSE)
	{
		tmp = str;
		if (tmp)
		{
			str = g_strconcat (tmp, ",back:", sdata->back, NULL);
			g_free (tmp);
		}
		else
			str = g_strconcat ("back:", sdata->back, NULL);
	}
	if (str == NULL)
		str = g_strdup ("");
	return str;
}

void
style_data_destroy (StyleData * sdata)
{
	if (!sdata)
		return;
	if (sdata->item)
		g_free (sdata->item);
	if (sdata->font)
		g_free (sdata->font);
	if (sdata->fore)
		g_free (sdata->fore);
	if (sdata->back)
		g_free (sdata->back);
	g_free (sdata);
}

void
style_data_set_font (StyleData * sdata, gchar * font)
{
	string_assign (&sdata->font, font);
}

void
style_data_set_fore (StyleData * sdata, gchar * fore)
{
	string_assign (&sdata->fore, fore);
}

void
style_data_set_back (StyleData * sdata, gchar * back)
{
	string_assign (&sdata->back, back);
}

void
style_data_set_item (StyleData * sdata, gchar * item)
{
	string_assign (&sdata->item, item);
}

static void
on_use_default_font_toggled (GtkToggleButton * tb, gpointer data)
{
	Preferences *p;
	gchar* font_name;
	gboolean state;

	p = data;

	gtk_widget_set_sensitive (p->widgets.font_picker, TRUE);
	gtk_widget_set_sensitive (p->widgets.font_size_spin, TRUE);
	state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (tb));
	if (state)
	{
		font_name = g_strdup_printf ("-*-%s-*-*-*-*-%d-*-*-*-*-*-*-*-", 
			p->default_style->font, p->default_style->size);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON
			(p->widgets.font_size_spin),
			p->default_style->size);
	}
	else
	{
		font_name = g_strdup_printf ("-*-%s-*-*-*-*-%d-*-*-*-*-*-*-*-", 
			p->current_style->font, p->current_style->size);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON
					   (p->widgets.font_size_spin),
					   p->current_style->size);
	}
	if (state)
	{
		gtk_widget_set_sensitive (p->widgets.font_picker, FALSE);
		gtk_widget_set_sensitive (p->widgets.font_size_spin, FALSE);
	}
	gnome_font_picker_set_font_name (GNOME_FONT_PICKER (p->widgets.font_picker),
		font_name);
}

static void
on_use_default_attrib_toggled (GtkToggleButton * tb, gpointer data)
{
	Preferences *p;

	p = data;
	gtk_widget_set_sensitive (p->widgets.font_bold_check, TRUE);
	gtk_widget_set_sensitive (p->widgets.font_italics_check, TRUE);
	gtk_widget_set_sensitive (p->widgets.font_underlined_check, TRUE);
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (tb)))
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (p->widgets.font_bold_check),
					      p->default_style->bold);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (p->widgets.font_italics_check),
					      p->default_style->italics);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (p->widgets.
					       font_underlined_check),
					      p->default_style->underlined);
		gtk_widget_set_sensitive (p->widgets.font_bold_check, FALSE);
		gtk_widget_set_sensitive (p->widgets.font_italics_check,
					  FALSE);
		gtk_widget_set_sensitive (p->widgets.font_underlined_check,
					  FALSE);
	}
	else
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (p->widgets.font_bold_check),
					      p->current_style->bold);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (p->widgets.font_italics_check),
					      p->current_style->italics);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (p->widgets.
					       font_underlined_check),
					      p->current_style->underlined);
	}
}

static void
on_use_default_fore_toggled (GtkToggleButton * tb, gpointer data)
{
	Preferences *p;
	guint8 r, g, b;

	p = data;

	gtk_widget_set_sensitive (p->widgets.fore_colorpicker, TRUE);
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (tb)))
	{
		ColorFromString (p->default_style->fore, &r, &g, &b);
		gnome_color_picker_set_i8 (GNOME_COLOR_PICKER
					   (p->widgets.fore_colorpicker), r,
					   g, b, 0);
		gtk_widget_set_sensitive (p->widgets.fore_colorpicker, FALSE);
	}
	else
	{
		ColorFromString (p->current_style->fore, &r, &g, &b);
		gnome_color_picker_set_i8 (GNOME_COLOR_PICKER
					   (p->widgets.fore_colorpicker), r,
					   g, b, 0);
	}
}

static void
on_use_default_back_toggled (GtkToggleButton * tb, gpointer data)
{
	Preferences *p;
	guint8 r, g, b;

	p = data;
	gtk_widget_set_sensitive (p->widgets.back_colorpicker, TRUE);
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (tb)))
	{
		ColorFromString (p->default_style->back, &r, &g, &b);
		gnome_color_picker_set_i8 (GNOME_COLOR_PICKER
					   (p->widgets.back_colorpicker), r,
					   g, b, 0);
		gtk_widget_set_sensitive (p->widgets.back_colorpicker, FALSE);
	}
	else
	{
		ColorFromString (p->current_style->back, &r, &g, &b);
		gnome_color_picker_set_i8 (GNOME_COLOR_PICKER
					   (p->widgets.back_colorpicker), r,
					   g, b, 0);
	}
}
static gchar* 
font_get_nth_field (gchar* font_name, gint n)
{
	gchar *start, *end, *tmp;
	gint i;
	
	g_return_val_if_fail (font_name != NULL, NULL);
	g_return_val_if_fail (strlen (font_name) !=0, NULL);
	g_return_val_if_fail (n >= 0 && n < 14, NULL);
	
	start = strchr (font_name, '-');
	for (i=0 ; i<n; i++)
	{
		tmp = start+1;
		start = strchr (tmp, '-');
		if (!start) return NULL;
	}
	tmp = start+1;
	end = strchr (tmp, '-');
	if (!end)
		end = &start[strlen(start)-1];
	return g_strndup (tmp, end-tmp);
}

static gboolean
fontpicker_get_font_name (GtkWidget *gnomefontpicker,
                                        gchar** font)
{
	gchar *font_name;
	
	font_name = gnome_font_picker_get_font_name (GNOME_FONT_PICKER(gnomefontpicker));
	g_return_val_if_fail (font_name != NULL, FALSE);
	*font = font_get_nth_field (font_name, 1);
	if (*font == NULL)
		return FALSE;
	return TRUE;
}

