/*
    appwiz_page0.c
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
#include "appwizard.h"
#include "pixmaps.h"
#include "utilities.h"

static void
on_wizard_app_icon_select (GnomeIconList * gil, gint num,
			    GdkEvent * event, gpointer user_data)
{
  AppWizard *aw = user_data;
  switch (num)
  {
  case 0:
    aw->prj_type = PROJECT_TYPE_GENERIC;
    break;
  case 1:
    aw->prj_type = PROJECT_TYPE_GTK;
    break;
  case 2:
    aw->prj_type = PROJECT_TYPE_GNOME;
    break;
   case 3:
 	aw->prj_type = PROJECT_TYPE_GTKMM;
    	break;
  case 4:
 	aw->prj_type = PROJECT_TYPE_GNOMEMM;
  	break;
  case 5:
    aw->prj_type = PROJECT_TYPE_BONOBO;
    break;
 default: /* Invalid project type */
    aw->prj_type = PROJECT_TYPE_END_MARK;
    break;
  }
}

void
create_app_wizard_page1 (AppWizard * aw)
{
  GtkWidget *frame;
  GtkWidget *frame3;
  GtkWidget *vbox4;
  GtkWidget *scrolledwindow1;
  GtkWidget *iconlist1;
  GtkWidget *frame2;
  GtkWidget *label2;
  GtkWidget *druid_vbox1;

  gchar *icon1_file, *icon2_file, *icon3_file, *icon4_file, *icon5_file, *icon6_file;

  frame = gtk_frame_new (NULL);
  gtk_widget_show (frame);
  druid_vbox1 = GNOME_DRUID_PAGE_STANDARD (aw->widgets.page[1])->vbox;
  gtk_box_pack_start (GTK_BOX (druid_vbox1), frame, TRUE, TRUE, 0);

  vbox4 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox4);
  gtk_container_add (GTK_CONTAINER (frame), vbox4);

  frame2 = gtk_frame_new (NULL);
  gtk_widget_show (frame2);
  gtk_box_pack_start (GTK_BOX (vbox4), frame2, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame2), 5);
  gtk_frame_set_shadow_type (GTK_FRAME (frame2), GTK_SHADOW_IN);

  label2 =
    gtk_label_new (_("Select the type of application you want to develop"));
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
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
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

  if (icon1_file)
	  gnome_icon_list_append (GNOME_ICON_LIST (iconlist1),
			  icon1_file, _("Generic/Terminal project"));

  if (icon2_file)
	  gnome_icon_list_append (GNOME_ICON_LIST (iconlist1),
			  icon2_file, _("GTK project"));

  if (icon3_file)
	  gnome_icon_list_append (GNOME_ICON_LIST (iconlist1),
			  icon3_file, _("GNOME project"));

  if (icon4_file)
	  gnome_icon_list_append (GNOME_ICON_LIST (iconlist1),
			  icon4_file, _("GTK-- project"));
  
  if (icon5_file)
	  gnome_icon_list_append (GNOME_ICON_LIST (iconlist1),
 			  icon5_file, _("GNOME-- project"));
  
  if (icon6_file)
	  gnome_icon_list_append (GNOME_ICON_LIST (iconlist1),
			  icon6_file, _("Bonobo component"));

  string_free  (icon1_file);
  string_free (icon2_file);
  string_free (icon3_file);
  string_free (icon4_file);
  string_free (icon5_file);
  string_free (icon6_file);

  gtk_signal_connect (GTK_OBJECT (iconlist1), "select_icon",
		      GTK_SIGNAL_FUNC (on_wizard_app_icon_select), aw);
  gnome_icon_list_select_icon (GNOME_ICON_LIST (iconlist1), 2);
}
