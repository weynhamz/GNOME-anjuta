/*
    appwidz_page2.c
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

#include "resources.h"
#include "appwidzard.h"
#include "appwidzard_cbs.h"

static gchar *app_group[] = {
	"Applications", "Games", "Graphics", "Internet", "Multimedia",
	"System", "Settings", "Utilities", "Development", "Networking",
	NULL
};

static void
on_gpl_checkbutton_toggled (GtkToggleButton * tb, gpointer user_data)
{
	AppWidzard *aw = user_data;
	aw->use_header = gtk_toggle_button_get_active (tb);
}

static void
on_gettext_support_checkbutton_toggled (GtkToggleButton * togglebutton,
					gpointer user_data)
{
	AppWidzard *aw;
	aw = user_data;
	aw->gettext_support = gtk_toggle_button_get_active (togglebutton);
}


static void
on_need_term_checkbutton_toggled (GtkToggleButton * tb, gpointer user_data)
{
	AppWidzard *aw = user_data;
	aw->need_terminal = gtk_toggle_button_get_active (tb);
}

void
create_app_widzard_page4 (AppWidzard * aw)
{
	GtkWidget *window3;
	GtkWidget *frame;
	GtkWidget *vbox1;
	GtkWidget *vbox2;
	GtkWidget *vbox3;

	GtkWidget *frame4;
	GtkWidget *gpl_checkbutton;
	GtkWidget *gettext_support_checkbutton;

	GtkWidget *frame1;
	GtkWidget *table1;
	GtkWidget *label;
	GtkWidget *label1;
	GtkWidget *label2;
	GtkWidget *label3;
	GtkWidget *entry1;
	GtkWidget *entry2;
	GtkWidget *combo1;
	GtkWidget *combo_entry1;
	GtkWidget *iconentry1;
	GtkWidget *checkbutton1;
	GtkWidget *druid_vbox4;
	GList *list;
	gint i;

	window3 = aw->widgets.window;

	frame = gtk_frame_new (NULL);
	gtk_widget_show (frame);
	druid_vbox4 = GNOME_DRUID_PAGE_STANDARD (aw->widgets.page[4])->vbox;
	gtk_box_pack_start (GTK_BOX (druid_vbox4), frame, TRUE, TRUE, 0);

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

	frame4 = gtk_frame_new (NULL);
	gtk_widget_show (frame4);
	gtk_container_add (GTK_CONTAINER (vbox3), frame4);
	gtk_container_set_border_width (GTK_CONTAINER (frame4), 5);
	gtk_frame_set_shadow_type (GTK_FRAME (frame4), GTK_SHADOW_IN);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER (frame4), vbox1);

	frame = gtk_frame_new (_(" Project Options "));
	gtk_widget_show (frame);
	gtk_container_set_border_width (GTK_CONTAINER (frame), 5);
	gtk_box_pack_start (GTK_BOX (vbox1), frame, TRUE, TRUE, 0);

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox2);
	gtk_container_add (GTK_CONTAINER (frame), vbox2);

	gpl_checkbutton =
		gtk_check_button_new_with_label (_
						 ("Include GNU Copyright statement in files heading"));
	gtk_widget_show (gpl_checkbutton);
	gtk_box_pack_start (GTK_BOX (vbox2), gpl_checkbutton, TRUE, TRUE, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gpl_checkbutton),
				      TRUE);

	gettext_support_checkbutton =
		gtk_check_button_new_with_label (_("Gettext Support Enable"));
	gtk_widget_show (gettext_support_checkbutton);
	gtk_box_pack_start (GTK_BOX (vbox2), gettext_support_checkbutton,
			    TRUE, TRUE, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (gettext_support_checkbutton), TRUE);

	frame1 = gtk_frame_new (_(" Gnome Menu Entry"));
	gtk_widget_show (frame1);
	gtk_box_pack_start (GTK_BOX (vbox1), frame1, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	table1 = gtk_table_new (3, 3, FALSE);
	gtk_widget_show (table1);
	gtk_container_add (GTK_CONTAINER (frame1), table1);
	gtk_container_set_border_width (GTK_CONTAINER (table1), 5);

	label1 = gtk_label_new (_("Entry name:"));
	gtk_widget_show (label1);
	gtk_misc_set_alignment (GTK_MISC (label1), 0, -1);
	gtk_table_attach (GTK_TABLE (table1), label1, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 5, 0);

	label2 = gtk_label_new (_("Comment:"));
	gtk_widget_show (label2);
	gtk_misc_set_alignment (GTK_MISC (label2), 0, -1);
	gtk_table_attach (GTK_TABLE (table1), label2, 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 5, 0);

	label3 = gtk_label_new (_("Group:"));
	gtk_widget_show (label3);
	gtk_misc_set_alignment (GTK_MISC (label3), 0, -1);
	gtk_table_attach (GTK_TABLE (table1), label3, 0, 1, 2, 3,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 5, 0);

	entry1 = gtk_entry_new ();
	gtk_widget_show (entry1);
	gtk_table_attach (GTK_TABLE (table1), entry1, 1, 2, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	entry2 = gtk_entry_new ();
	gtk_widget_show (entry2);
	gtk_table_attach (GTK_TABLE (table1), entry2, 1, 2, 1, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	combo1 = gtk_combo_new ();
	gtk_widget_show (combo1);
	gtk_table_attach (GTK_TABLE (table1), combo1, 1, 2, 2, 3,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	combo_entry1 = GTK_COMBO (combo1)->entry;
	gtk_entry_set_editable (GTK_ENTRY (combo_entry1), FALSE);

	list = NULL;
	i = 0;
	while (app_group[i])
		list = g_list_append (list, app_group[i++]);
	gtk_combo_set_popdown_strings (GTK_COMBO (combo1), list);
	g_list_free (list);

	iconentry1 = gnome_icon_entry_new (NULL, "Anjuta: Application Icon");
	gtk_widget_show (iconentry1);
	gtk_table_attach (GTK_TABLE (table1), iconentry1, 2, 3, 0, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);

	checkbutton1 = gtk_check_button_new_with_label (_("Needs terminal"));
	gtk_widget_show (checkbutton1);
	gtk_table_attach (GTK_TABLE (table1), checkbutton1, 2, 3, 2, 3,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (checkbutton1), 5);

	gtk_signal_connect (GTK_OBJECT (gpl_checkbutton), "toggled",
			    GTK_SIGNAL_FUNC (on_gpl_checkbutton_toggled), aw);
	gtk_signal_connect (GTK_OBJECT (gettext_support_checkbutton), "toggled",
			    GTK_SIGNAL_FUNC(on_gettext_support_checkbutton_toggled), aw);

	gtk_signal_connect (GTK_OBJECT (entry1), "changed",
			    GTK_SIGNAL_FUNC (on_aw_text_entry_changed), &aw->menu_entry);
	gtk_signal_connect (GTK_OBJECT (entry1), "realize",
			    GTK_SIGNAL_FUNC (on_aw_text_entry_realize), aw->menu_entry);

	gtk_signal_connect (GTK_OBJECT (entry2), "changed",
			    GTK_SIGNAL_FUNC (on_aw_text_entry_changed), &aw->menu_comment);
	gtk_signal_connect (GTK_OBJECT (entry2), "realize",
			    GTK_SIGNAL_FUNC (on_aw_text_entry_realize), aw->menu_comment);

	gtk_signal_connect (GTK_OBJECT (combo_entry1), "changed",
			    GTK_SIGNAL_FUNC (on_aw_text_entry_changed), &aw->app_group);
	
	gtk_signal_connect (GTK_OBJECT (checkbutton1), "toggled",
			    GTK_SIGNAL_FUNC(on_need_term_checkbutton_toggled), aw);

	aw->widgets.gettext_support_check = gettext_support_checkbutton;
	gtk_widget_ref (gettext_support_checkbutton);
	aw->widgets.file_header_check = gpl_checkbutton;
	gtk_widget_ref (gpl_checkbutton);
	aw->widgets.menu_frame = frame1;
	gtk_widget_ref (frame1);
	aw->widgets.menu_entry_entry = entry1;
	gtk_widget_ref (entry1);
	aw->widgets.menu_comment_entry = entry2;
	gtk_widget_ref (entry2);
	aw->widgets.icon_entry = iconentry1;
	gtk_widget_ref (iconentry1);
	aw->widgets.app_group_combo = combo1;
	gtk_widget_ref (combo1);
	aw->widgets.app_group_entry = combo_entry1;
	gtk_widget_ref (combo_entry1);

	aw->widgets.term_check = checkbutton1;
	gtk_widget_ref (checkbutton1);
}
