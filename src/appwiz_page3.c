/*
    appwiz_page3.c
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
#include "pixmaps.h"

static void
on_target_exec_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	AppWizard* aw;
	aw = user_data;
	
	g_return_if_fail (aw != NULL);
	if (gtk_toggle_button_get_active(togglebutton))
		aw->target_type = PROJECT_TARGET_TYPE_EXECUTABLE;
}

static void
on_target_slib_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	AppWizard* aw;
	aw = user_data;
	
	g_return_if_fail (aw != NULL);
	if (gtk_toggle_button_get_active(togglebutton))
		aw->target_type = PROJECT_TARGET_TYPE_STATIC_LIB;
}

static void
on_target_dlib_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	AppWizard* aw;
	aw = user_data;
	
	g_return_if_fail (aw != NULL);
	if (gtk_toggle_button_get_active(togglebutton))
		aw->target_type = PROJECT_TARGET_TYPE_DYNAMIC_LIB;
}

void
create_app_wizard_page3 (AppWizard * aw)
{
	GtkWidget *window1;
	GtkWidget *frame;
	GtkWidget *vbox3;
	GtkWidget *frame1;
	GtkWidget *label;
	GtkWidget *scrolledwindow1;
	GtkWidget *description_text;
	GtkWidget *druid_vbox3;
	
	GtkWidget *frame2;
	GtkWidget *hbox1;
	gchar *pixmap1_filename;
	GtkWidget *pixmap1;
	GtkWidget *vseparator1;
	GtkWidget *vbox1;
	GSList *vbox1_group = NULL;
	GtkWidget *radiobutton1;
	GtkWidget *radiobutton2;
	GtkWidget *radiobutton3;

	window1 = aw->widgets.window;

	frame = gtk_frame_new (NULL);
	gtk_widget_show (frame);
	druid_vbox3 = GNOME_DRUID_PAGE_STANDARD (aw->widgets.page[3])->vbox;
	gtk_box_pack_start (GTK_BOX (druid_vbox3), frame, TRUE, TRUE, 0);

	vbox3 = gtk_vbox_new (FALSE, 5);
	gtk_widget_show (vbox3);
	gtk_container_add (GTK_CONTAINER (frame), vbox3);

	frame1 = gtk_frame_new (NULL);
	gtk_widget_show (frame1);
	gtk_box_pack_start (GTK_BOX (vbox3), frame1, FALSE, FALSE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame1), GTK_SHADOW_IN);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	label = gtk_label_new (_("Enter a short project description"));
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

	frame2 = gtk_frame_new (NULL);
	/* unimplemented */
	/* gtk_widget_show (frame2); */
	gtk_box_pack_start (GTK_BOX (vbox3), frame2, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame2), 5);
	
	hbox1 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox1);
	gtk_container_add (GTK_CONTAINER (frame2), hbox1);
	gtk_container_set_border_width (GTK_CONTAINER (hbox1), 5);
	
	pixmap1 = gtk_type_new (gnome_pixmap_get_type ());
	pixmap1_filename = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_APPWIZ_GEAR);
	if (pixmap1_filename)
	gnome_pixmap_load_file_at_size (GNOME_PIXMAP (pixmap1), pixmap1_filename, 100, 100);
	else
	g_warning (_("Couldn't find pixmap file: %s"), "appwid_gear.xpm");
	g_free (pixmap1_filename);
	gtk_widget_show (pixmap1);
	gtk_box_pack_start (GTK_BOX (hbox1), pixmap1, FALSE, FALSE, 0);
	
	vseparator1 = gtk_vseparator_new ();
	gtk_widget_show (vseparator1);
	gtk_box_pack_start (GTK_BOX (hbox1), vseparator1, TRUE, TRUE, 0);
	
	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox1);
	gtk_box_pack_start (GTK_BOX (hbox1), vbox1, TRUE, TRUE, 0);
	
	radiobutton1 = gtk_radio_button_new_with_label (vbox1_group, _("Executable target"));
	vbox1_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton1));
	gtk_widget_show (radiobutton1);
	gtk_box_pack_start (GTK_BOX (vbox1), radiobutton1, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (radiobutton1), 5);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobutton1), TRUE);
	
	radiobutton2 = gtk_radio_button_new_with_label (vbox1_group, _("Static library target"));
	vbox1_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton2));
	gtk_widget_show (radiobutton2);
	gtk_box_pack_start (GTK_BOX (vbox1), radiobutton2, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (radiobutton2), 5);
	
	radiobutton3 = gtk_radio_button_new_with_label (vbox1_group, _("Dynamic library target"));
	vbox1_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton3));
	gtk_widget_show (radiobutton3);
	gtk_box_pack_start (GTK_BOX (vbox1), radiobutton3, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (radiobutton3), 5);

	gtk_signal_connect (GTK_OBJECT (radiobutton1), "toggled",
		      GTK_SIGNAL_FUNC (on_target_exec_toggled),
		      aw);
	gtk_signal_connect (GTK_OBJECT (radiobutton2), "toggled",
		      GTK_SIGNAL_FUNC (on_target_slib_toggled),
		      aw);
	gtk_signal_connect (GTK_OBJECT (radiobutton3), "toggled",
		      GTK_SIGNAL_FUNC (on_target_dlib_toggled),
		      aw);

	aw->widgets.description_text = description_text;
	gtk_widget_ref (description_text);
	aw->widgets.target_exec_radio = radiobutton1;
	gtk_widget_ref (radiobutton1);
	aw->widgets.target_slib_radio = radiobutton2;
	gtk_widget_ref (radiobutton2);
	aw->widgets.target_dlib_radio = radiobutton3;
	gtk_widget_ref (radiobutton3);
}
