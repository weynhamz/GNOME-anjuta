/*  wizard_gui.c
 *  Copyright (C) 2002 Johannes Schmid
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* Here is the code stored which is shared between appwizard and importwizard */

#include "wizard_gui.h"
#include "anjuta.h"
#include "pixmaps.h"
#include "resources.h"

GtkWidget *
create_project_start_page (GnomeDruid * druid, gchar * greetings,
			   gchar * title)
{
	GdkColor pagestart_bg_color = { 0, 15616, 33280, 46848 };
	GdkColor pagestart_logo_bg_color = { 0, 15616, 33280, 46848 };
	GdkColor pagestart_textbox_color = { 0, 65535, 65535, 65535 };
	GdkColor pagestart_title_color = { 0, 65535, 65535, 65535 };
	GtkWidget *page;

	page = gnome_druid_page_start_new ();
	gtk_widget_show (page);
	gnome_druid_append_page (GNOME_DRUID (druid),
				 GNOME_DRUID_PAGE (page));
	gnome_druid_set_page (GNOME_DRUID (druid), GNOME_DRUID_PAGE (page));
	gnome_druid_page_start_set_bg_color (GNOME_DRUID_PAGE_START
					     (page), &pagestart_bg_color);
	gnome_druid_page_start_set_logo_bg_color (GNOME_DRUID_PAGE_START
						  (page),
						  &pagestart_logo_bg_color);
	gnome_druid_page_start_set_textbox_color (GNOME_DRUID_PAGE_START
						  (page),
						  &pagestart_textbox_color);
	gnome_druid_page_start_set_title_color (GNOME_DRUID_PAGE_START
						(page),
						&pagestart_title_color);
	gnome_druid_page_start_set_text (GNOME_DRUID_PAGE_START
					 (page), greetings);
	gnome_druid_page_start_set_title (GNOME_DRUID_PAGE_START
					  (page), title);
	gnome_druid_page_start_set_logo (GNOME_DRUID_PAGE_START
					 (page),
					 anjuta_res_get_image
					 (ANJUTA_PIXMAP_APPWIZ_LOGO));
	gnome_druid_page_start_set_watermark (GNOME_DRUID_PAGE_START
					      (page),
					      anjuta_res_get_image
					      (ANJUTA_PIXMAP_APPWIZ_WATERMARK));

	return page;
}

GtkWidget *
create_project_type_selection_page (GnomeDruid * druid, GtkWidget ** iconlist)
{
	GtkWidget *frame;
	GtkWidget *frame3;
	GtkWidget *vbox4;
	GtkWidget *scrolledwindow1;
	GtkWidget *iconlist1;
	GtkWidget *frame2;
	GtkWidget *label2;
	GtkWidget *druid_vbox1;

	gchar *icon1_file, *icon2_file, *icon3_file, *icon4_file, *icon5_file,
		*icon6_file, *icon7_file, *icon8_file, *icon9_file, *icon10_file,
		*icon11_file, *icon12_file, *icon13_file, *icon14_file;

	GdkColor page_bg_color = { 0, 15616, 33280, 46848 };
	GdkColor page_logo_bg_color = { 0, 15616, 33280, 46848 };
	GdkColor page_title_color = { 0, 65535, 65535, 65535 };

	GtkWidget *page;

	page = gnome_druid_page_standard_new_with_vals ("", NULL);
	gtk_widget_show_all (page);
	gnome_druid_append_page (GNOME_DRUID (druid),
				 GNOME_DRUID_PAGE (page));
	gnome_druid_page_standard_set_bg_color (GNOME_DRUID_PAGE_STANDARD
						(page), &page_bg_color);
	gnome_druid_page_standard_set_logo_bg_color (GNOME_DRUID_PAGE_STANDARD
						     (page),
						     &page_logo_bg_color);
	gnome_druid_page_standard_set_title_color (GNOME_DRUID_PAGE_STANDARD
						   (page), &page_title_color);
	gnome_druid_page_standard_set_title (GNOME_DRUID_PAGE_STANDARD
					     (page), _("Project Type"));
	gnome_druid_page_standard_set_logo (GNOME_DRUID_PAGE_STANDARD
					    (page),
					    anjuta_res_get_image
					    (ANJUTA_PIXMAP_APPWIZ_LOGO));

	frame = gtk_frame_new (NULL);
	gtk_widget_show (frame);
	druid_vbox1 = GNOME_DRUID_PAGE_STANDARD (page)->vbox;
	gtk_box_pack_start (GTK_BOX (druid_vbox1), frame, TRUE, TRUE, 0);

	vbox4 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox4);
	gtk_container_add (GTK_CONTAINER (frame), vbox4);

	frame2 = gtk_frame_new (NULL);
	gtk_widget_show (frame2);
	gtk_box_pack_start (GTK_BOX (vbox4), frame2, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame2), 5);
	gtk_frame_set_shadow_type (GTK_FRAME (frame2), GTK_SHADOW_IN);

	label2 = gtk_label_new (_
				("Select the type of application to be developed"));
	gtk_misc_set_padding (GTK_MISC (label2), 5, 5);
	gtk_widget_show (label2);
	gtk_container_add (GTK_CONTAINER (frame2), label2);

	frame3 = gtk_frame_new (NULL);
	gtk_widget_show (frame3);
	gtk_box_pack_start (GTK_BOX (vbox4), frame3, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame3), 5);
	gtk_frame_set_shadow_type (GTK_FRAME (frame3), GTK_SHADOW_IN);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow1);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (frame3), scrolledwindow1);

	iconlist1 = gnome_icon_list_new_flags (110, NULL, 0);
	gnome_icon_list_set_selection_mode (GNOME_ICON_LIST (iconlist1),
					    GTK_SELECTION_BROWSE);
	gtk_widget_show (iconlist1);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), iconlist1);
	gnome_icon_list_set_icon_border (GNOME_ICON_LIST (iconlist1), 5);
	gnome_icon_list_set_row_spacing (GNOME_ICON_LIST (iconlist1), 20);
	gnome_icon_list_set_text_spacing (GNOME_ICON_LIST (iconlist1), 5);

	icon1_file = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_APP_GENERIC);
	icon2_file = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_APP_GTK);
	icon3_file = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_APP_GNOME);
	icon4_file = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_APP_GTKMM);
	icon5_file = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_APP_GNOMEMM);
	icon6_file = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_APP_COMPONENT);
	icon7_file = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_APP_LIBGLADE);
	icon8_file = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_APP_WXWIN);
	icon9_file = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_APP_GTK2);
	icon10_file = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_APP_GNOME2);
	icon11_file = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_APP_GTKMM2);
	icon12_file = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_APP_GNOMEMM2);
	icon13_file = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_APP_XWIN);
	icon14_file = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_APP_XWINDOCKAPP);
	
	if (icon1_file)
		gnome_icon_list_append (GNOME_ICON_LIST (iconlist1),
					icon1_file,
					_("Generic/Terminal project"));

	if (icon2_file)
		gnome_icon_list_append (GNOME_ICON_LIST (iconlist1),
					icon2_file, _("GTK project"));

	if (icon3_file)
		gnome_icon_list_append (GNOME_ICON_LIST (iconlist1),
					icon3_file, _("GNOME project"));

	if (icon4_file)
		gnome_icon_list_append (GNOME_ICON_LIST (iconlist1),
					icon4_file, _("GTKmm project"));

	if (icon5_file)
		gnome_icon_list_append (GNOME_ICON_LIST (iconlist1),
					icon5_file, _("GNOMEmm project"));

	if (icon6_file)
		gnome_icon_list_append (GNOME_ICON_LIST (iconlist1),
					icon6_file, _("Bonobo component"));

	if (icon7_file)
		gnome_icon_list_append (GNOME_ICON_LIST (iconlist1),
					icon7_file, _("LibGlade project"));
	if (icon8_file)
		gnome_icon_list_append (GNOME_ICON_LIST (iconlist1),
					icon8_file, _("wxWindows project"));
	if (icon9_file)
		gnome_icon_list_append (GNOME_ICON_LIST (iconlist1),
					icon9_file, _("GTK 2.0 project"));
	if (icon10_file)
		gnome_icon_list_append (GNOME_ICON_LIST (iconlist1),
					icon10_file, _("GNOME 2.0 project"));
	if (icon11_file)
		gnome_icon_list_append (GNOME_ICON_LIST (iconlist1),
					icon11_file, _("GTKmm 2.0 project"));
	if (icon12_file)
		gnome_icon_list_append (GNOME_ICON_LIST (iconlist1),
					icon12_file, _("GNOMEmm 2.0 project"));
	if (icon13_file)
		gnome_icon_list_append (GNOME_ICON_LIST (iconlist1),
					icon13_file, _("Xlib project"));
	if (icon14_file)
		gnome_icon_list_append (GNOME_ICON_LIST (iconlist1),
					icon14_file, _("X Dock App project"));

	g_free (icon1_file);
	g_free (icon2_file);
	g_free (icon3_file);
	g_free (icon4_file);
	g_free (icon5_file);
	g_free (icon6_file);
	g_free (icon7_file);
	g_free (icon8_file);
	g_free (icon9_file);
	g_free (icon10_file);
	g_free (icon11_file);
	g_free (icon12_file);
	g_free (icon13_file);
	g_free (icon14_file);	

	*iconlist = iconlist1;
	return page;
};

GtkWidget *
create_project_props_page (GnomeDruid * druid,
			   GtkWidget ** prj_name,
			   GtkWidget ** author,
			   GtkWidget ** version,
			   GtkWidget ** target,
			   GtkWidget ** language_c_radio,
			   GtkWidget ** language_cpp_radio,
			   GtkWidget ** language_c_cpp_radio,
			   GtkWidget ** target_exec_radio,
			   GtkWidget ** target_slib_radio,
			   GtkWidget ** target_dlib_radio)
{
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
	
	GtkWidget *radiobutton1;
	GtkWidget *radiobutton2;
	GtkWidget *radiobutton3;
	GtkWidget *frame3;
	GtkWidget *hbox4;
	GtkWidget *hbox5;
	GSList *hbox5_group = NULL;
	
	GdkColor page_bg_color = { 0, 15616, 33280, 46848 };
	GdkColor page_logo_bg_color = { 0, 15616, 33280, 46848 };
	GdkColor page_title_color = { 0, 65535, 65535, 65535 };

	GtkWidget *page = gnome_druid_page_standard_new_with_vals ("", NULL);
	gtk_widget_show_all (page);
	gnome_druid_append_page (GNOME_DRUID (druid),
				 GNOME_DRUID_PAGE (page));
	gnome_druid_page_standard_set_bg_color (GNOME_DRUID_PAGE_STANDARD
						(page), &page_bg_color);
	gnome_druid_page_standard_set_logo_bg_color (GNOME_DRUID_PAGE_STANDARD
						     (page),
						     &page_logo_bg_color);
	gnome_druid_page_standard_set_title_color (GNOME_DRUID_PAGE_STANDARD
						   (page), &page_title_color);
	gnome_druid_page_standard_set_title (GNOME_DRUID_PAGE_STANDARD
					     (page), _("Basic Information"));
	gnome_druid_page_standard_set_logo (GNOME_DRUID_PAGE_STANDARD
					    (page),
					    anjuta_res_get_image
					    (ANJUTA_PIXMAP_APPWIZ_LOGO));

	frame = gtk_frame_new (NULL);
	gtk_widget_show (frame);
	druid_vbox2 = GNOME_DRUID_PAGE_STANDARD (page)->vbox;
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

	table1 = gtk_table_new (6, 2, FALSE);
	gtk_widget_show (table1);
	gtk_box_pack_start (GTK_BOX (hbox2), table1, TRUE, TRUE, 4);
	gtk_table_set_row_spacings (GTK_TABLE (table1), 6);

	label = gtk_label_new (_("Project Name:"));
	gtk_widget_show (label);
	gtk_misc_set_alignment (GTK_MISC (label), 0, -1);
	gtk_table_attach (GTK_TABLE (table1), label, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 5, 0);

	label = gtk_label_new (_("Project Version:"));
	gtk_widget_show (label);
	gtk_misc_set_alignment (GTK_MISC (label), 0, -1);
	gtk_table_attach (GTK_TABLE (table1), label, 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 5, 0);

	label = gtk_label_new (_("Project Author:"));
	gtk_widget_show (label);
	gtk_misc_set_alignment (GTK_MISC (label), 0, -1);
	gtk_table_attach (GTK_TABLE (table1), label, 0, 1, 2, 3,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 5, 0);

	label = gtk_label_new (_("Project Target:"));
	gtk_widget_show (label);
	gtk_misc_set_alignment (GTK_MISC (label), 0, -1);
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
	hbox3_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton4));
	gtk_widget_show (radiobutton4);
	gtk_box_pack_start (GTK_BOX (hbox3), radiobutton4, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (radiobutton4), 5);

	radiobutton5 =
		gtk_radio_button_new_with_label (hbox3_group, _("C++"));
	hbox3_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton5));
	gtk_widget_show (radiobutton5);
	gtk_box_pack_start (GTK_BOX (hbox3), radiobutton5, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (radiobutton5), 5);

	radiobutton6 =
		gtk_radio_button_new_with_label (hbox3_group,
						 _("Both C and C++"));
	hbox3_group =
		gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton6));
	gtk_widget_show (radiobutton6);
	gtk_box_pack_start (GTK_BOX (hbox3), radiobutton6, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (radiobutton6), 5);
	
	frame3 = gtk_frame_new (_("Target type"));
	gtk_widget_show (frame3);
	gtk_table_attach (GTK_TABLE (table1), frame3, 1, 2, 5, 6,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame3), 5);
	
	hbox4 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox4);
	gtk_container_add (GTK_CONTAINER (frame3), hbox4);
	gtk_container_set_border_width (GTK_CONTAINER (hbox4), 5);
	
	hbox5 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox5);
	gtk_box_pack_start (GTK_BOX (hbox4), hbox5, TRUE, TRUE, 0);
	
	radiobutton1 = gtk_radio_button_new_with_label (hbox5_group, _("Executable target"));
	hbox5_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton1));
	gtk_widget_show (radiobutton1);
	gtk_box_pack_start (GTK_BOX (hbox5), radiobutton1, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (radiobutton1), 5);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobutton1), TRUE);
	
	radiobutton2 = gtk_radio_button_new_with_label (hbox5_group, _("Static library target"));
	hbox5_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton2));
	gtk_widget_show (radiobutton2);
	gtk_box_pack_start (GTK_BOX (hbox5), radiobutton2, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (radiobutton2), 5);
	
	radiobutton3 = gtk_radio_button_new_with_label (hbox5_group, _("Dynamic library target"));
	hbox5_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton3));
	gtk_widget_show (radiobutton3);
	gtk_box_pack_start (GTK_BOX (hbox5), radiobutton3, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (radiobutton3), 5);

	*prj_name = prj_name_entry;
	*author = author_entry;
	*version = version_entry;
	*target = target_entry;
	*language_c_radio = radiobutton4;
	*language_cpp_radio = radiobutton5;
	*language_c_cpp_radio = radiobutton6;
	
	if (target_exec_radio != NULL)
	{
		*target_exec_radio = radiobutton1;
		*target_slib_radio = radiobutton2;
		*target_dlib_radio = radiobutton3;
	}
	else
	{
		gtk_widget_set_sensitive(frame3, FALSE);
	}
	
	return page;
}

GtkWidget *
create_project_description_page (GnomeDruid * druid,
				 GtkWidget ** description)
{
	GdkColor page_bg_color = { 0, 15616, 33280, 46848 };
	GdkColor page_logo_bg_color = { 0, 15616, 33280, 46848 };
	GdkColor page_title_color = { 0, 65535, 65535, 65535 };

	GtkWidget *frame;
	GtkWidget *vbox3;
	GtkWidget *frame1;
	GtkWidget *label;
	GtkWidget *scrolledwindow1;
	GtkWidget *description_text;
	GtkWidget *druid_vbox3;
	GtkWidget *page = gnome_druid_page_standard_new_with_vals ("", NULL);
	gtk_widget_show_all (page);
	gnome_druid_append_page (GNOME_DRUID (druid),
				 GNOME_DRUID_PAGE (page));
	gnome_druid_page_standard_set_bg_color (GNOME_DRUID_PAGE_STANDARD
						(page), &page_bg_color);
	gnome_druid_page_standard_set_logo_bg_color (GNOME_DRUID_PAGE_STANDARD
						     (page),
						     &page_logo_bg_color);
	gnome_druid_page_standard_set_title_color (GNOME_DRUID_PAGE_STANDARD
						   (page), &page_title_color);
	gnome_druid_page_standard_set_title (GNOME_DRUID_PAGE_STANDARD
					     (page), _("Project Description"));
	gnome_druid_page_standard_set_logo (GNOME_DRUID_PAGE_STANDARD
					    (page),
					    anjuta_res_get_image
					    (ANJUTA_PIXMAP_APPWIZ_LOGO));

	frame = gtk_frame_new (NULL);
	gtk_widget_show (frame);
	druid_vbox3 = GNOME_DRUID_PAGE_STANDARD (page)->vbox;
	gtk_box_pack_start (GTK_BOX (druid_vbox3), frame, TRUE, TRUE, 0);

	vbox3 = gtk_vbox_new (FALSE, 5);
	gtk_widget_show (vbox3);
	gtk_container_add (GTK_CONTAINER (frame), vbox3);

	frame1 = gtk_frame_new (NULL);
	gtk_widget_show (frame1);
	gtk_box_pack_start (GTK_BOX (vbox3), frame1, FALSE, FALSE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame1), GTK_SHADOW_IN);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	label = gtk_label_new (_("Enter a short description of the Project"));
	gtk_widget_show (label);
	gtk_container_add (GTK_CONTAINER (frame1), label);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_FILL);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_misc_set_padding (GTK_MISC (label), 5, 5);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow1);
	gtk_box_pack_start (GTK_BOX (vbox3), scrolledwindow1, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow1), 5);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	description_text = gtk_text_new (NULL, NULL);
	gtk_widget_show (description_text);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), description_text);
	gtk_text_set_editable (GTK_TEXT (description_text), TRUE);

	*description = description_text;
	return page;
}

static gchar *app_group[] = {
	"Applications", "Games", "Graphics", "Internet", "Multimedia",
	"System", "Settings", "Utilities", "Development", "Finance",
	NULL
};

GtkWidget *
create_project_menu_page (GnomeDruid * druid,
			  GtkWidget ** menu_frame,
			  GtkWidget ** menu_entry_entry,
			  GtkWidget ** menu_comment,
			  GtkWidget ** icon_entry,
			  GtkWidget ** app_group_box,
			  GtkWidget ** app_group_combo,
			  GtkWidget ** term_check,
			  GtkWidget ** file_header_support,
			  GtkWidget ** gettext_support_check)
{
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

	GdkColor page_bg_color = { 0, 15616, 33280, 46848 };
	GdkColor page_logo_bg_color = { 0, 15616, 33280, 46848 };
	GdkColor page_title_color = { 0, 65535, 65535, 65535 };


	GtkWidget *page = gnome_druid_page_standard_new_with_vals ("", NULL);
	gtk_widget_show_all (page);
	gnome_druid_append_page (GNOME_DRUID (druid),
				 GNOME_DRUID_PAGE (page));
	gnome_druid_page_standard_set_bg_color (GNOME_DRUID_PAGE_STANDARD
						(page), &page_bg_color);
	gnome_druid_page_standard_set_logo_bg_color (GNOME_DRUID_PAGE_STANDARD
						     (page),
						     &page_logo_bg_color);
	gnome_druid_page_standard_set_title_color (GNOME_DRUID_PAGE_STANDARD
						   (page), &page_title_color);
	gnome_druid_page_standard_set_title (GNOME_DRUID_PAGE_STANDARD
					     (page), _("Additional Options"));
	gnome_druid_page_standard_set_logo (GNOME_DRUID_PAGE_STANDARD
					    (page),
					    anjuta_res_get_image
					    (ANJUTA_PIXMAP_APPWIZ_LOGO));

	frame = gtk_frame_new (NULL);
	gtk_widget_show (frame);
	druid_vbox4 = GNOME_DRUID_PAGE_STANDARD (page)->vbox;
	gtk_box_pack_start (GTK_BOX (druid_vbox4), frame, TRUE, TRUE, 0);

	vbox3 = gtk_vbox_new (FALSE, 5);
	gtk_widget_show (vbox3);
	gtk_container_add (GTK_CONTAINER (frame), vbox3);

	frame1 = gtk_frame_new (NULL);
	gtk_widget_show (frame1);
	gtk_box_pack_start (GTK_BOX (vbox3), frame1, FALSE, FALSE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame1), GTK_SHADOW_IN);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	label = gtk_label_new (_("Enter any additional Project options"));
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
						 ("Include GNU Copyright statement in file headings"));
	gtk_widget_show (gpl_checkbutton);
	gtk_box_pack_start (GTK_BOX (vbox2), gpl_checkbutton, TRUE, TRUE, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gpl_checkbutton),
				      TRUE);

	gettext_support_checkbutton =
		gtk_check_button_new_with_label (_("Enable Gettext Support"));
	gtk_widget_show (gettext_support_checkbutton);
	gtk_box_pack_start (GTK_BOX (vbox2), gettext_support_checkbutton,
			    TRUE, TRUE, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (gettext_support_checkbutton), TRUE);

	frame1 = gtk_frame_new (_("GNOME Menu Entry"));
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

	iconentry1 =
		gnome_icon_entry_new (NULL, "Select an Application Icon");
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

	*gettext_support_check = gettext_support_checkbutton;
	*file_header_support = gpl_checkbutton;
	*menu_frame = frame1;
	*menu_entry_entry = entry1;
	*menu_comment = entry2;
	*icon_entry = iconentry1;
	*app_group_combo = combo1;
	*app_group_box = combo_entry1;
	*term_check = checkbutton1;

	return page;
}

GtkWidget *
create_project_finish_page (GnomeDruid * druid)
{
	GdkColor pagefinish_bg_color = { 0, 15616, 33280, 46848 };
	GdkColor pagefinish_logo_bg_color = { 0, 15616, 33280, 46848 };
	GdkColor pagefinish_textbox_color = { 0, 65535, 65535, 65535 };
	GdkColor pagefinish_title_color = { 0, 65535, 65535, 65535 };

	GtkWidget *page = gnome_druid_page_finish_new ();
	gtk_widget_show (page);
	gnome_druid_append_page (GNOME_DRUID (druid),
				 GNOME_DRUID_PAGE (page));
	gnome_druid_page_finish_set_bg_color (GNOME_DRUID_PAGE_FINISH
					      (page), &pagefinish_bg_color);
	gnome_druid_page_finish_set_logo_bg_color (GNOME_DRUID_PAGE_FINISH
						   (page),
						   &pagefinish_logo_bg_color);
	gnome_druid_page_finish_set_textbox_color (GNOME_DRUID_PAGE_FINISH
						   (page),
						   &pagefinish_textbox_color);
	gnome_druid_page_finish_set_title_color (GNOME_DRUID_PAGE_FINISH
						 (page),
						 &pagefinish_title_color);
	gnome_druid_page_finish_set_title (GNOME_DRUID_PAGE_FINISH
					   (page), _("Summary"));
	gnome_druid_page_finish_set_logo (GNOME_DRUID_PAGE_FINISH
					  (page),
					  anjuta_res_get_image
					  (ANJUTA_PIXMAP_APPWIZ_LOGO));
	gnome_druid_page_finish_set_watermark (GNOME_DRUID_PAGE_FINISH
					       (page),
					       anjuta_res_get_image
					       (ANJUTA_PIXMAP_APPWIZ_WATERMARK));

	return page;
}
