/*
    appwiz_page1.c
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

#include "resources.h"
#include "utilities.h"
#include "appwizard.h"
#include "appwizard_cbs.h"

static void
on_prj_name_entry_changed (GtkEditable * editable, gpointer user_data)
{
	gchar *text;
	gint i;
	
	AppWizard *aw = user_data;
	text = g_strdup (gtk_entry_get_text (GTK_ENTRY (editable)));
	for (i = 0; i <strlen (text); i++)
		text[i] = tolower (text[i]);
	if (text) 
		gtk_entry_set_text (GTK_ENTRY (aw->widgets.target_entry), text);
	else
		gtk_entry_set_text (GTK_ENTRY (aw->widgets.target_entry), "");
	string_assign (&aw->target, text);
	string_free (text);
}

static gboolean
on_prj_name_entry_focus_out_event (GtkWidget * widget,
				   GdkEventFocus * event, gpointer user_data)
{
	gchar *temp;
	AppWizard *aw = user_data;
	if (aw->prj_name)
	{
		aw->prj_name[0] = toupper (aw->prj_name[0]);
		gtk_entry_set_text (GTK_ENTRY (widget), aw->prj_name);
	}
	temp = g_strdup_printf ("%s Version %s", aw->prj_name, aw->version);
	gtk_entry_set_text (GTK_ENTRY (aw->widgets.menu_entry_entry), temp);
	gtk_entry_set_text (GTK_ENTRY (aw->widgets.menu_comment_entry), temp);
	g_free (temp);
	return FALSE;
}

static void
on_lang_c_toggled                      (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	AppWizard* aw;
	aw = user_data;
	
	g_return_if_fail (aw != NULL);
	if (gtk_toggle_button_get_active(togglebutton))
		aw->language = PROJECT_PROGRAMMING_LANGUAGE_C;
}

static void
on_lang_cpp_toggled                    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	AppWizard* aw;
	aw = user_data;
	
	g_return_if_fail (aw != NULL);
	if (gtk_toggle_button_get_active(togglebutton))
		aw->language = PROJECT_PROGRAMMING_LANGUAGE_CPP;
}

static void
on_lang_c_cpp_toggled                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	AppWizard* aw;
	aw = user_data;
	
	g_return_if_fail (aw != NULL);
	if (gtk_toggle_button_get_active(togglebutton))
		aw->language = PROJECT_PROGRAMMING_LANGUAGE_C_CPP;
}

void
create_app_wizard_page2 (AppWizard * aw)
{
	GtkWidget *window1;
	GtkWidget *frame;
	GtkWidget *vbox3;
	GtkWidget *frame1;
	GtkWidget *hbox2;
	
	GtkWidget *table1;
	GtkWidget *label;
	GtkWidget *prj_name_entry;
	GtkWidget *version_entry;
	GtkWidget *target_entry;
	GtkWidget *author_entry;
	GtkWidget *druid_vbox2;

	GtkWidget *frame2;
	GtkWidget *hbox3;
	GSList *hbox3_group = NULL;
	GtkWidget *radiobutton4;
	GtkWidget *radiobutton5;
	GtkWidget *radiobutton6;

	window1 = aw->widgets.window;

	frame = gtk_frame_new (NULL);
	gtk_widget_show (frame);
	druid_vbox2 = GNOME_DRUID_PAGE_STANDARD (aw->widgets.page[2])->vbox;
	gtk_box_pack_start (GTK_BOX (druid_vbox2), frame, TRUE, TRUE, 0);

	vbox3 = gtk_vbox_new (FALSE, 5);
	gtk_widget_show (vbox3);
	gtk_container_add (GTK_CONTAINER (frame), vbox3);

	frame1 = gtk_frame_new (NULL);
	gtk_widget_show (frame1);
	gtk_box_pack_start (GTK_BOX (vbox3), frame1, FALSE, FALSE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame1), GTK_SHADOW_IN);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	label = gtk_label_new (_("Enter the basic Project information"));
	gtk_widget_show (label);
	gtk_container_add (GTK_CONTAINER (frame1), label);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_FILL);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_misc_set_padding (GTK_MISC (label), 5, 5);

	hbox2 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox2);
	gtk_box_pack_start (GTK_BOX (vbox3), hbox2, TRUE, TRUE, 0);

	table1 = gtk_table_new (5, 2, FALSE);
	gtk_widget_show (table1);
	gtk_box_pack_start (GTK_BOX (hbox2), table1, TRUE, TRUE, 4);
	gtk_table_set_row_spacings (GTK_TABLE (table1), 6);

	label = gtk_label_new (_("Project Name:"));
	gtk_widget_show (label);
	gtk_misc_set_alignment (GTK_MISC(label), 0, -1);
	gtk_table_attach (GTK_TABLE (table1), label, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 5, 0);

	label = gtk_label_new (_("Project Version:"));
	gtk_widget_show (label);
	gtk_misc_set_alignment (GTK_MISC(label), 0, -1);
	gtk_table_attach (GTK_TABLE (table1), label, 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 5, 0);
	
	label = gtk_label_new (_("Project Author:"));
	gtk_widget_show (label);
	gtk_misc_set_alignment (GTK_MISC(label), 0, -1);
	gtk_table_attach (GTK_TABLE (table1), label, 0, 1, 2, 3,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 5, 0);

	label = gtk_label_new (_("Project Target:"));
	gtk_widget_show (label);
	gtk_misc_set_alignment (GTK_MISC(label), 0, -1);
	gtk_table_attach (GTK_TABLE (table1), label, 0, 1, 3, 4,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 5, 0);

	prj_name_entry = gtk_entry_new ();
	gtk_widget_show (prj_name_entry);
	gtk_table_attach (GTK_TABLE (table1), prj_name_entry, 1, 2, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	version_entry = gtk_entry_new ();
	gtk_widget_show (version_entry);
	gtk_table_attach (GTK_TABLE (table1), version_entry, 1, 2, 1, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	author_entry = gtk_entry_new ();
	gtk_widget_show (author_entry);
	gtk_table_attach (GTK_TABLE (table1), author_entry, 1, 2, 2, 3,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	target_entry = gtk_entry_new ();
	gtk_widget_show (target_entry);
	gtk_table_attach (GTK_TABLE (table1), target_entry, 1, 2, 3, 4,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	frame2 = gtk_frame_new (_("Programming language"));
	gtk_widget_show (frame2);
	gtk_table_attach (GTK_TABLE (table1), frame2, 1, 2, 4, 5,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame2), 5);
	
	hbox3 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox3);
	gtk_container_add (GTK_CONTAINER (frame2), hbox3);
	
	radiobutton4 = gtk_radio_button_new_with_label (hbox3_group, _("C"));
	hbox3_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton4));
	gtk_widget_show (radiobutton4);
	gtk_box_pack_start (GTK_BOX (hbox3), radiobutton4, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (radiobutton4), 5);
	
	radiobutton5 = gtk_radio_button_new_with_label (hbox3_group, _("C++"));
	hbox3_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton5));
	gtk_widget_show (radiobutton5);
	gtk_box_pack_start (GTK_BOX (hbox3), radiobutton5, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (radiobutton5), 5);
	
	radiobutton6 = gtk_radio_button_new_with_label (hbox3_group, _("Both C and C++"));
	hbox3_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton6));
	gtk_widget_show (radiobutton6);
	gtk_box_pack_start (GTK_BOX (hbox3), radiobutton6, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (radiobutton6), 5);

	gtk_signal_connect (GTK_OBJECT (prj_name_entry), "changed",
			    GTK_SIGNAL_FUNC (on_prj_name_entry_changed), aw);
	gtk_signal_connect (GTK_OBJECT (prj_name_entry), "focus_out_event",
			    GTK_SIGNAL_FUNC(on_prj_name_entry_focus_out_event), aw);

	gtk_signal_connect (GTK_OBJECT (prj_name_entry), "changed",
			    GTK_SIGNAL_FUNC (on_aw_text_entry_changed), &aw->prj_name);

	gtk_signal_connect (GTK_OBJECT (target_entry), "changed",
			    GTK_SIGNAL_FUNC (on_aw_text_entry_changed), &aw->target);

	gtk_signal_connect (GTK_OBJECT (version_entry), "changed",
			    GTK_SIGNAL_FUNC (on_aw_text_entry_changed), &aw->version);
	gtk_signal_connect (GTK_OBJECT (version_entry), "realize",
			    GTK_SIGNAL_FUNC (on_aw_text_entry_realize), aw->version);

	gtk_signal_connect (GTK_OBJECT (author_entry), "changed",
			    GTK_SIGNAL_FUNC (on_aw_text_entry_changed), &aw->author);
	gtk_signal_connect (GTK_OBJECT (author_entry), "realize",
			    GTK_SIGNAL_FUNC (on_aw_text_entry_realize), aw->author);
	gtk_signal_connect (GTK_OBJECT (radiobutton4), "toggled",
		      GTK_SIGNAL_FUNC (on_lang_c_toggled),
		      aw);
	gtk_signal_connect (GTK_OBJECT (radiobutton5), "toggled",
		      GTK_SIGNAL_FUNC (on_lang_cpp_toggled),
		      aw);
	gtk_signal_connect (GTK_OBJECT (radiobutton6), "toggled",
		      GTK_SIGNAL_FUNC (on_lang_c_cpp_toggled),
		      aw);

	aw->widgets.prj_name_entry = prj_name_entry;
	gtk_widget_ref (prj_name_entry);
	aw->widgets.version_entry = version_entry;
	gtk_widget_ref (version_entry);
	aw->widgets.target_entry = target_entry;
	gtk_widget_ref (target_entry);
	aw->widgets.author_entry = author_entry;
	gtk_widget_ref (author_entry);
	aw->widgets.language_c_radio = radiobutton4;
	gtk_widget_ref (radiobutton4);
	aw->widgets.language_cpp_radio = radiobutton5;
	gtk_widget_ref (radiobutton5);
	aw->widgets.language_c_cpp_radio = radiobutton6;
	gtk_widget_ref (radiobutton6);
}
