/*
    appwizard_gui.c
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

#include "anjuta.h"
#include "appwizard.h"
#include "appwizard_cbs.h"
#include "resources.h"
#include "pixmaps.h"

static gchar*
greetings_text ()
{
return _("The Application Wizard will generate a basic\n\
skeleton for a project, including all of the\n\
build files. It will ask for details of the initial\n\
structure for the application.\n\n\
Please answer the questions carefully, as it\n\
may not be possible to change some of the\n\
settings later.");
}

void create_new_project (AppWizard * aw);
void create_app_wizard_gui (AppWizard * aw);
void create_app_wizard_page0 (AppWizard * aw);
void create_app_wizard_page1 (AppWizard * aw);
void create_app_wizard_page2 (AppWizard * aw);
void create_app_wizard_page3 (AppWizard * aw);
void create_app_wizard_page4 (AppWizard * aw);

void
create_app_wizard_gui (AppWizard * aw)
{
	GtkWidget *window1;
	GtkWidget *druid1;

	/* here we set up the colours to be used on each of the pages */
	GtkWidget *druidpagestart1;
	GdkColor druidpagestart1_bg_color = { 0, 15616, 33280, 46848 };
	GdkColor druidpagestart1_logo_bg_color = { 0, 15616, 33280, 46848 };
	GdkColor druidpagestart1_textbox_color = { 0, 65535, 65535, 65535 };
	GdkColor druidpagestart1_title_color = { 0, 65535, 65535, 65535 };

	GtkWidget *druidpagestandard1;
	GdkColor druidpagestandard1_bg_color = { 0, 15616, 33280, 46848 };
	GdkColor druidpagestandard1_logo_bg_color = { 0, 15616, 33280, 46848 };
	GdkColor druidpagestandard1_title_color = { 0, 65535, 65535, 65535 };
	GtkWidget *druid_vbox1;

	GtkWidget *druidpagestandard2;
	GdkColor druidpagestandard2_bg_color = { 0, 15616, 33280, 46848 };
	GdkColor druidpagestandard2_logo_bg_color = { 0, 15616, 33280, 46848 };
	GdkColor druidpagestandard2_title_color = { 0, 65535, 65535, 65535 };
	GtkWidget *druid_vbox2;

	GtkWidget *druidpagestandard3;
	GdkColor druidpagestandard3_bg_color = { 0, 15616, 33280, 46848 };
	GdkColor druidpagestandard3_logo_bg_color = { 0, 15616, 33280, 46848 };
	GdkColor druidpagestandard3_title_color = { 0, 65535, 65535, 65535 };
	GtkWidget *druid_vbox3;

	GtkWidget *druidpagestandard4;
	GdkColor druidpagestandard4_bg_color = { 0, 15616, 33280, 46848 };
	GdkColor druidpagestandard4_logo_bg_color = { 0, 15616, 33280, 46848 };
	GdkColor druidpagestandard4_title_color = { 0, 65535, 65535, 65535 };
	GtkWidget *druid_vbox4;

	GtkWidget *druidpagefinish1;
	GdkColor druidpagefinish1_bg_color = { 0, 15616, 33280, 46848 };
	GdkColor druidpagefinish1_logo_bg_color = { 0, 15616, 33280, 46848 };
	GdkColor druidpagefinish1_textbox_color = { 0, 65535, 65535, 65535 };
	GdkColor druidpagefinish1_title_color = { 0, 65535, 65535, 65535 };

	/* Now set up the wizard window */
	window1 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window1),
			      _("Application Wizard"));
	gtk_window_set_wmclass (GTK_WINDOW (window1), "appwizard", "Anjuta");
	gtk_window_set_position (GTK_WINDOW (window1), GTK_WIN_POS_CENTER);

	druid1 = gnome_druid_new ();
	gtk_widget_show (druid1);
	gtk_container_add (GTK_CONTAINER (window1), druid1);

	/* first page - the introduction */
	/* this is just a placeholder, the real pages start after this */
	/* start and finish pages are special since they have the "watermark" image */
	druidpagestart1 = gnome_druid_page_start_new ();
	gtk_widget_show (druidpagestart1);
	gnome_druid_append_page (GNOME_DRUID (druid1),
				 GNOME_DRUID_PAGE (druidpagestart1));
	gnome_druid_set_page (GNOME_DRUID (druid1),
			      GNOME_DRUID_PAGE (druidpagestart1));
	gnome_druid_page_start_set_bg_color (GNOME_DRUID_PAGE_START
					     (druidpagestart1),
					     &druidpagestart1_bg_color);
	gnome_druid_page_start_set_logo_bg_color (GNOME_DRUID_PAGE_START
						     (druidpagestart1),
						     &druidpagestart1_logo_bg_color);
	gnome_druid_page_start_set_textbox_color (GNOME_DRUID_PAGE_START
						  (druidpagestart1),
						  &druidpagestart1_textbox_color);
	gnome_druid_page_start_set_title_color (GNOME_DRUID_PAGE_START
						(druidpagestart1),
						&druidpagestart1_title_color);
	gnome_druid_page_start_set_title (GNOME_DRUID_PAGE_START
					  (druidpagestart1),
					  _
					  ("Application Wizard"));
	gnome_druid_page_start_set_text (GNOME_DRUID_PAGE_START
					 (druidpagestart1),
					 greetings_text());

	gnome_druid_page_start_set_logo (GNOME_DRUID_PAGE_START
					 (druidpagestart1),
					 anjuta_res_get_image (ANJUTA_PIXMAP_APPWIZ_LOGO));
	gnome_druid_page_start_set_watermark (GNOME_DRUID_PAGE_START
					      (druidpagestart1),
					      anjuta_res_get_image (ANJUTA_PIXMAP_APPWIZ_WATERMARK));

	/* second page (really "page1") */
	druidpagestandard1 =
		gnome_druid_page_standard_new_with_vals ("", NULL);
	gtk_widget_show_all (druidpagestandard1);
	gnome_druid_append_page (GNOME_DRUID (druid1),
				 GNOME_DRUID_PAGE (druidpagestandard1));
	gnome_druid_page_standard_set_bg_color (GNOME_DRUID_PAGE_STANDARD
						(druidpagestandard1),
						&druidpagestandard1_bg_color);
	gnome_druid_page_standard_set_logo_bg_color (GNOME_DRUID_PAGE_STANDARD
						     (druidpagestandard1),
						     &druidpagestandard1_logo_bg_color);
	gnome_druid_page_standard_set_title_color (GNOME_DRUID_PAGE_STANDARD
						   (druidpagestandard1),
						   &druidpagestandard1_title_color);
	gnome_druid_page_standard_set_title (GNOME_DRUID_PAGE_STANDARD
					     (druidpagestandard1),
					     _
					     ("Project Type"));
	gnome_druid_page_standard_set_logo (GNOME_DRUID_PAGE_STANDARD
					    (druidpagestandard1),
					    anjuta_res_get_image  (ANJUTA_PIXMAP_APPWIZ_LOGO));

	druid_vbox1 = GNOME_DRUID_PAGE_STANDARD (druidpagestandard1)->vbox;
	gtk_widget_show (druid_vbox1);

	/* third page (really "page2") */
	druidpagestandard2 =
		gnome_druid_page_standard_new_with_vals ("", NULL);
	gtk_widget_show_all (druidpagestandard2);
	gnome_druid_append_page (GNOME_DRUID (druid1),
				 GNOME_DRUID_PAGE (druidpagestandard2));
	gnome_druid_page_standard_set_bg_color (GNOME_DRUID_PAGE_STANDARD
						(druidpagestandard2),
						&druidpagestandard2_bg_color);
	gnome_druid_page_standard_set_logo_bg_color (GNOME_DRUID_PAGE_STANDARD
						     (druidpagestandard2),
						     &druidpagestandard2_logo_bg_color);
	gnome_druid_page_standard_set_title_color (GNOME_DRUID_PAGE_STANDARD
						   (druidpagestandard2),
						   &druidpagestandard2_title_color);
	gnome_druid_page_standard_set_title (GNOME_DRUID_PAGE_STANDARD
					     (druidpagestandard2),
					     _("Basic Information"));
	gnome_druid_page_standard_set_logo (GNOME_DRUID_PAGE_STANDARD
					    (druidpagestandard2),
					    anjuta_res_get_image  (ANJUTA_PIXMAP_APPWIZ_LOGO));

	druid_vbox2 = GNOME_DRUID_PAGE_STANDARD (druidpagestandard2)->vbox;
	gtk_widget_show (druid_vbox2);

	/* fourth page (really "page3") */
	druidpagestandard3 =
		gnome_druid_page_standard_new_with_vals ("", NULL);
	gtk_widget_show_all (druidpagestandard3);
	gnome_druid_append_page (GNOME_DRUID (druid1),
				 GNOME_DRUID_PAGE (druidpagestandard3));
	gnome_druid_page_standard_set_bg_color (GNOME_DRUID_PAGE_STANDARD
						(druidpagestandard3),
						&druidpagestandard3_bg_color);
	gnome_druid_page_standard_set_logo_bg_color (GNOME_DRUID_PAGE_STANDARD
						     (druidpagestandard3),
						     &druidpagestandard3_logo_bg_color);
	gnome_druid_page_standard_set_title_color (GNOME_DRUID_PAGE_STANDARD
						   (druidpagestandard3),
						   &druidpagestandard3_title_color);
	gnome_druid_page_standard_set_title (GNOME_DRUID_PAGE_STANDARD
					     (druidpagestandard3),
					     _
					     ("Project Description"));
	gnome_druid_page_standard_set_logo (GNOME_DRUID_PAGE_STANDARD
					    (druidpagestandard3),
					    anjuta_res_get_image  (ANJUTA_PIXMAP_APPWIZ_LOGO));
	druid_vbox3 = GNOME_DRUID_PAGE_STANDARD (druidpagestandard3)->vbox;
	gtk_widget_show (druid_vbox3);

	/* fifth page (really "page4") */
	druidpagestandard4 =
		gnome_druid_page_standard_new_with_vals ("", NULL);
	gtk_widget_show_all (druidpagestandard4);
	gnome_druid_append_page (GNOME_DRUID (druid1),
				 GNOME_DRUID_PAGE (druidpagestandard4));
	gnome_druid_page_standard_set_bg_color (GNOME_DRUID_PAGE_STANDARD
						(druidpagestandard4),
						&druidpagestandard4_bg_color);
	gnome_druid_page_standard_set_logo_bg_color (GNOME_DRUID_PAGE_STANDARD
						     (druidpagestandard4),
						     &druidpagestandard4_logo_bg_color);
	gnome_druid_page_standard_set_title_color (GNOME_DRUID_PAGE_STANDARD
						   (druidpagestandard4),
						   &druidpagestandard4_title_color);
	gnome_druid_page_standard_set_title (GNOME_DRUID_PAGE_STANDARD
					     (druidpagestandard4),
					     _
					     ("Additional Options"));
	gnome_druid_page_standard_set_logo (GNOME_DRUID_PAGE_STANDARD
					    (druidpagestandard4),
					    anjuta_res_get_image  (ANJUTA_PIXMAP_APPWIZ_LOGO));

	druid_vbox4 = GNOME_DRUID_PAGE_STANDARD (druidpagestandard4)->vbox;
	gtk_widget_show (druid_vbox4);

	/* final page - the summary */
	/* start and finish pages are special since they have the "watermark" image */
	druidpagefinish1 = gnome_druid_page_finish_new ();
	gtk_widget_show (druidpagefinish1);
	gnome_druid_append_page (GNOME_DRUID (druid1),
				 GNOME_DRUID_PAGE (druidpagefinish1));
	gnome_druid_page_finish_set_bg_color (GNOME_DRUID_PAGE_FINISH
					      (druidpagefinish1),
					      &druidpagefinish1_bg_color);
	gnome_druid_page_finish_set_logo_bg_color (GNOME_DRUID_PAGE_FINISH
						     (druidpagefinish1),
						     &druidpagefinish1_logo_bg_color);
	gnome_druid_page_finish_set_textbox_color (GNOME_DRUID_PAGE_FINISH
						   (druidpagefinish1),
						   &druidpagefinish1_textbox_color);
	gnome_druid_page_finish_set_title_color (GNOME_DRUID_PAGE_FINISH
						 (druidpagefinish1),
						 &druidpagefinish1_title_color);
	gnome_druid_page_finish_set_title (GNOME_DRUID_PAGE_FINISH
					   (druidpagefinish1),
					   _("Summary"));
	gnome_druid_page_finish_set_logo (GNOME_DRUID_PAGE_FINISH
					  (druidpagefinish1),
					    anjuta_res_get_image  (ANJUTA_PIXMAP_APPWIZ_LOGO));
	gnome_druid_page_finish_set_watermark (GNOME_DRUID_PAGE_FINISH
					       (druidpagefinish1),
					       anjuta_res_get_image (ANJUTA_PIXMAP_APPWIZ_WATERMARK));

	/* buttons, callbacks etc. */
	gtk_accel_group_attach (app->accel_group, GTK_OBJECT (window1));

	gtk_signal_connect (GTK_OBJECT (druid1), "cancel",
			    GTK_SIGNAL_FUNC (on_druid1_cancel), aw);
	gtk_signal_connect (GTK_OBJECT (druidpagestandard1), "next",
			    GTK_SIGNAL_FUNC (on_druidpagestandard1_next), aw);
	gtk_signal_connect (GTK_OBJECT (druidpagestandard2), "next",
			    GTK_SIGNAL_FUNC (on_druidpagestandard2_next), aw);
	gtk_signal_connect (GTK_OBJECT (druidpagestandard3), "next",
			    GTK_SIGNAL_FUNC (on_druidpagestandard3_next), aw);
	gtk_signal_connect (GTK_OBJECT (druidpagestandard4), "next",
			    GTK_SIGNAL_FUNC (on_druidpagestandard4_next), aw);
	gtk_signal_connect (GTK_OBJECT (druidpagefinish1), "finish",
			    GTK_SIGNAL_FUNC (on_druidpagefinish1_finish), aw);
	gtk_signal_connect (GTK_OBJECT (druidpagefinish1), "back",
			    GTK_SIGNAL_FUNC (on_druidpagefinish1_back), aw);

	aw->widgets.page[0] = druidpagestart1;
	gtk_widget_ref (druidpagestart1);
	aw->widgets.page[1] = druidpagestandard1;
	gtk_widget_ref (druidpagestandard1);
	aw->widgets.page[2] = druidpagestandard2;
	gtk_widget_ref (druidpagestandard2);
	aw->widgets.page[3] = druidpagestandard3;
	gtk_widget_ref (druidpagestandard3);
	aw->widgets.page[4] = druidpagestandard4;
	gtk_widget_ref (druidpagestandard4);
	aw->widgets.page[5] = druidpagefinish1;
	gtk_widget_ref (druidpagefinish1);

	aw->widgets.window = window1;
	gtk_widget_ref (window1);
	aw->widgets.druid = druid1;
	gtk_widget_ref (druid1);
}
