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
#include <libgnomeui/gnome-window-icon.h>

#include "anjuta.h"
#include "appwizard.h"
#include "appwizard_cbs.h"
#include "resources.h"
#include "pixmaps.h"
#include "wizard_gui.h"

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

void
create_app_wizard_gui (AppWizard * aw)
{
	GtkWidget *window1;
	GtkWidget *druid1;
	
	/* here we set up the colours to be used on each of the pages */
	GtkWidget *druidpagestart1;

	GtkWidget *druidpagestandard1;
	GtkWidget *druid_vbox1;
	
	GtkWidget *druidpagestandard2;
	GtkWidget *druid_vbox2;

	GtkWidget *druidpagestandard3;
	GtkWidget *druid_vbox3;

	GtkWidget *druidpagestandard4;
	GtkWidget *druid_vbox4;

	GtkWidget *druidpagefinish1;
	
	/* Now set up the wizard window */
	window1 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_transient_for(GTK_WINDOW(window1), GTK_WINDOW(app->widgets.window));
	gtk_window_set_title (GTK_WINDOW (window1),
			      _("Application Wizard"));
	gtk_window_set_wmclass (GTK_WINDOW (window1), "appwizard", "Anjuta");
	gtk_window_set_position (GTK_WINDOW (window1), GTK_WIN_POS_CENTER);
    gnome_window_icon_set_from_default(GTK_WINDOW(window1));

	druid1 = gnome_druid_new ();
	gtk_widget_show (druid1);
	gtk_container_add (GTK_CONTAINER (window1), druid1);

	/* first page - the introduction */
	/* this is just a placeholder, the real pages start after this */
	/* start and finish pages are special since they have the "watermark" image */
	druidpagestart1 = create_project_start_page(GNOME_DRUID(druid1), 
			greetings_text(),
			_("Application Wizard"));
	
	druidpagestandard1 = create_project_type_selection_page(GNOME_DRUID(druid1), 
			&aw->widgets.icon_list);
	druid_vbox1 = GNOME_DRUID_PAGE_STANDARD (druidpagestandard1)->vbox;
	gtk_widget_show (druid_vbox1);

	/* third page (really "page2") */
	druidpagestandard2 = create_project_props_page (GNOME_DRUID (druid1),
					   &aw->widgets.prj_name_entry,
					   &aw->widgets.author_entry,
					   &aw->widgets.version_entry, 
					   &aw->widgets.target_entry,
					   &aw->widgets.language_c_radio,
					   &aw->widgets.language_cpp_radio,
					   &aw->widgets.language_c_cpp_radio,
					   &aw->widgets.target_exec_radio,
					   &aw->widgets.target_slib_radio,
					   &aw->widgets.target_dlib_radio);
					   
	druid_vbox2 = GNOME_DRUID_PAGE_STANDARD (druidpagestandard2)->vbox;
	gtk_widget_show (druid_vbox2);
	
	druidpagestandard3 = create_project_description_page(GNOME_DRUID(druid1),
			&aw->widgets.description_text);
	druid_vbox3 = GNOME_DRUID_PAGE_STANDARD (druidpagestandard3)->vbox;
	gtk_widget_show(druid_vbox3);
	
	/* fifth page (really "page4") */
	druidpagestandard4 = create_project_menu_page (GNOME_DRUID(druid1),
			&aw->widgets.menu_frame,
			&aw->widgets.menu_entry_entry,
			&aw->widgets.menu_comment_entry,
			&aw->widgets.icon_entry,
			&aw->widgets.app_group_combo,
			&aw->widgets.app_group_entry,
			&aw->widgets.term_check,
			&aw->widgets.file_header_check,
			&aw->widgets.gettext_support_check,
			&aw->widgets.use_glade_check);

	druid_vbox4 = GNOME_DRUID_PAGE_STANDARD (druidpagestandard4)->vbox;
	gtk_widget_show (druid_vbox4);

	/* final page - the summary */
	/* start and finish pages are special since they have the "watermark" image */
	druidpagefinish1 = create_project_finish_page(GNOME_DRUID(druid1));
	
	
	/* buttons, callbacks etc. */
	gtk_window_add_accel_group (GTK_WINDOW (window1), app->accel_group);

	g_signal_connect (G_OBJECT (druid1), "cancel",
			    G_CALLBACK (on_druid1_cancel), aw);
	g_signal_connect (G_OBJECT (druidpagestandard1), "next",
			    G_CALLBACK (on_druidpagestandard1_next), aw);
	g_signal_connect (G_OBJECT (druidpagestandard2), "next",
			    G_CALLBACK (on_druidpagestandard2_next), aw);
	g_signal_connect (G_OBJECT (druidpagestandard3), "next",
			    G_CALLBACK (on_druidpagestandard3_next), aw);
	g_signal_connect (G_OBJECT (druidpagestandard4), "next",
			    G_CALLBACK (on_druidpagestandard4_next), aw);
	g_signal_connect (G_OBJECT (druidpagefinish1), "finish",
			    G_CALLBACK (on_druidpagefinish1_finish), aw);
	g_signal_connect (G_OBJECT (druidpagefinish1), "back",
			    G_CALLBACK (on_druidpagefinish1_back), aw);
	
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
