/*  
 *  project_import_gui.c
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

#include "anjuta.h"
#include "resources.h"
#include "pixmaps.h"
#include "project_import.h"
#include "project_import_cbs.h"
#include "wizard_gui.h"

void create_import_wizard_page_start (ProjectImportWizard * piw);
void create_import_wizard_page2 (ProjectImportWizard * piw);
void create_import_wizard_page3 (ProjectImportWizard * piw);
void create_import_wizard_page4 (ProjectImportWizard * piw);
void create_import_wizard_page5 (ProjectImportWizard * piw);
void create_import_wizard_page6 (ProjectImportWizard * piw);
void create_import_wizard_page_finish (ProjectImportWizard * piw);

static gchar *
greetings_text ()
{
	return _("The Import Project Wizard scans the directory of an\n"
		 "existing project and tries to import the project structure\n"
		 "into an anjuta project. You can adjust the autodetected\n"
		 "values later.");
}

void
create_project_import_gui (void)
{
	piw = g_new0 (ProjectImportWizard, 1);
	piw->filename = NULL;

	piw->widgets.window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (piw->widgets.window),
			      _("Import Project Wizard"));
	gtk_window_set_wmclass (GTK_WINDOW (piw->widgets.window),
				"project_import_wizard", "anjuta");
	gtk_window_set_position (GTK_WINDOW (piw->widgets.window),
				 GTK_WIN_POS_CENTER);
	gtk_accel_group_attach (app->accel_group,
				GTK_OBJECT (piw->widgets.window));

	piw->widgets.druid = gnome_druid_new ();
	gtk_widget_show (piw->widgets.druid);
	gtk_container_add (GTK_CONTAINER (piw->widgets.window),
			   piw->widgets.druid);

	create_import_wizard_page_start (piw);
	create_import_wizard_page2 (piw);
	create_import_wizard_page3 (piw);
	create_import_wizard_page4 (piw);
	create_import_wizard_page5 (piw);
	create_import_wizard_page6 (piw);
	create_import_wizard_page_finish (piw);

	gtk_signal_connect (GTK_OBJECT (piw->widgets.druid), "cancel",
			    GTK_SIGNAL_FUNC (on_druid_cancel), piw);

	gtk_signal_connect (GTK_OBJECT (piw->widgets.page[0]), "next",
			    GTK_SIGNAL_FUNC (on_page_start_next), piw);
	gtk_signal_connect (GTK_OBJECT (piw->widgets.page[1]), "next",
			    GTK_SIGNAL_FUNC (on_page2_next), piw);
	gtk_signal_connect (GTK_OBJECT (piw->widgets.page[2]), "next",
			    GTK_SIGNAL_FUNC (on_page3_next), piw);
	gtk_signal_connect (GTK_OBJECT (piw->widgets.page[3]), "next",
			    GTK_SIGNAL_FUNC (on_page4_next), piw);
	gtk_signal_connect (GTK_OBJECT (piw->widgets.page[4]), "next",
			    GTK_SIGNAL_FUNC (on_page5_next), piw);
	gtk_signal_connect (GTK_OBJECT (piw->widgets.page[5]), "next",
			    GTK_SIGNAL_FUNC (on_page6_next), piw);
	gtk_signal_connect (GTK_OBJECT (piw->widgets.page[6]), "finish",
			    GTK_SIGNAL_FUNC (on_page_finish_finish), piw);

	gtk_signal_connect (GTK_OBJECT (piw->widgets.page[1]), "back",
			    GTK_SIGNAL_FUNC (on_page2_back), piw);
	gtk_signal_connect (GTK_OBJECT (piw->widgets.page[2]), "back",
			    GTK_SIGNAL_FUNC (on_page3_back), piw);
	gtk_signal_connect (GTK_OBJECT (piw->widgets.page[3]), "back",
			    GTK_SIGNAL_FUNC (on_page4_back), piw);
	gtk_signal_connect (GTK_OBJECT (piw->widgets.page[4]), "back",
			    GTK_SIGNAL_FUNC (on_page5_back), piw);
	gtk_signal_connect (GTK_OBJECT (piw->widgets.page[5]), "back",
			    GTK_SIGNAL_FUNC (on_page6_back), piw);
	gtk_signal_connect (GTK_OBJECT (piw->widgets.page[6]), "back",
			    GTK_SIGNAL_FUNC (on_page_finish_back), piw);

	gtk_widget_show (piw->widgets.window);
}

void
destroy_project_import_gui (void)
{
	// This should destroy all underlying widgets
	gtk_widget_hide(piw->widgets.window);
	gtk_widget_destroy (piw->widgets.window);
	string_assign (&piw->prj_name, NULL);
	string_assign (&piw->prj_source_target, NULL);
	string_assign (&piw->prj_author, NULL);
	string_assign (&piw->prj_version, NULL);
	string_assign (&piw->prj_menu_entry, NULL);
	string_assign (&piw->prj_menu_comment, NULL);
	string_assign (&piw->prj_menu_icon, NULL);
	string_assign (&piw->prj_menu_need_terminal, NULL);
	string_assign (&piw->prj_description, NULL);
	string_assign (&piw->prj_menu_icon, NULL);
	piw = NULL;
}

void
create_import_wizard_page_start (ProjectImportWizard * piw)
{
	piw->widgets.page[0] = 
			create_project_start_page (GNOME_DRUID(piw->widgets.druid), 
					greetings_text(), 
					_("Project Import Wizard"));
}


void
create_import_wizard_page2 (ProjectImportWizard * piw)
{
	GtkWidget *label1;
	GtkWidget *frame;
	GtkWidget *vbox1;
	GtkWidget *vbox2;
	GdkColor page2_bg_color = { 0, 15616, 33280, 46848 };
	GdkColor page2_logo_bg_color = { 0, 15616, 33280, 46848 };
	GdkColor page2_title_color = { 0, 65535, 65535, 65535 };

	piw->widgets.page[1] =
		gnome_druid_page_standard_new_with_vals ("", NULL);
	gtk_widget_show_all (piw->widgets.page[1]);
	gnome_druid_append_page (GNOME_DRUID (piw->widgets.druid),
				 GNOME_DRUID_PAGE (piw->widgets.page[1]));
	gnome_druid_page_standard_set_bg_color (GNOME_DRUID_PAGE_STANDARD
						(piw->widgets.page[1]),
						&page2_bg_color);
	gnome_druid_page_standard_set_logo_bg_color (GNOME_DRUID_PAGE_STANDARD
						     (piw->widgets.page[1]),
						     &page2_logo_bg_color);
	gnome_druid_page_standard_set_title_color (GNOME_DRUID_PAGE_STANDARD
						   (piw->widgets.page[1]),
						   &page2_title_color);
	gnome_druid_page_standard_set_title (GNOME_DRUID_PAGE_STANDARD
					     (piw->widgets.page[1]),
					     _("Select directory"));
	gnome_druid_page_standard_set_logo (GNOME_DRUID_PAGE_STANDARD
					    (piw->widgets.page[1]),
					    anjuta_res_get_image
					    (ANJUTA_PIXMAP_APPWIZ_LOGO));

	frame = gtk_frame_new (NULL);
	gtk_widget_show (frame);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_set_border_width (GTK_CONTAINER (frame), 5);
	vbox1 = GNOME_DRUID_PAGE_STANDARD (piw->widgets.page[1])->vbox;
	gtk_box_pack_start (GTK_BOX (vbox1), frame, TRUE, TRUE, 0);

	vbox2 = gtk_vbox_new (FALSE, 5);
	gtk_widget_show (vbox2);
	gtk_container_add (GTK_CONTAINER (frame), vbox2);

	label1 = gtk_label_new (_("Select project toplevel directory:"));
	gtk_widget_show (label1);

	piw->widgets.file_entry = gnome_file_entry_new ("project_import",
							_
							("Select project toplevel directory"));
	gtk_widget_show (piw->widgets.file_entry);
	gtk_box_pack_start (GTK_BOX (vbox2), label1, FALSE, TRUE, 5);
	gtk_box_pack_start (GTK_BOX (vbox2), piw->widgets.file_entry,
			    FALSE, TRUE, 5);

	piw->widgets.label =
		gtk_label_new (_("Press \"Next\" to start import"));
	gtk_widget_show (piw->widgets.label);
	gtk_box_pack_start (GTK_BOX (vbox2), piw->widgets.label, FALSE, TRUE,
			    40);

	piw->widgets.progressbar = gtk_progress_bar_new ();
	gtk_widget_show (piw->widgets.progressbar);
	gtk_box_pack_start (GTK_BOX (vbox2), piw->widgets.progressbar,
			    FALSE, TRUE, 5);
}

void
create_import_wizard_page3 (ProjectImportWizard * piw)
{
	piw->widgets.page[2] =
		create_project_type_selection_page (GNOME_DRUID
						    (piw->widgets.druid),
						    &piw->widgets.iconlist);
	gtk_signal_connect (GTK_OBJECT (piw->widgets.iconlist),
			    "select_icon",
			    GTK_SIGNAL_FUNC (on_wizard_import_icon_select),
			    piw);
}

void
create_import_wizard_page4 (ProjectImportWizard * piw)
{
	piw->widgets.page[3] =
		create_project_props_page (GNOME_DRUID (piw->widgets.druid),
					   &piw->widgets.prj_name_entry,
					   &piw->widgets.author_entry,
					   &piw->widgets.version_entry, 
					   &piw->widgets.target_entry,
					   &piw->widgets.language_c_radio,
					   &piw->widgets.language_cpp_radio,
					   &piw->widgets.language_c_cpp_radio);

	gtk_signal_connect (GTK_OBJECT (piw->widgets.prj_name_entry), "focus_out_event",
			    GTK_SIGNAL_FUNC
			    (on_prj_name_entry_focus_out_event), piw);

	gtk_signal_connect (GTK_OBJECT (piw->widgets.language_c_radio), "toggled",
			    GTK_SIGNAL_FUNC (on_lang_c_toggled), piw);
	gtk_signal_connect (GTK_OBJECT (piw->widgets.language_cpp_radio), "toggled",
			    GTK_SIGNAL_FUNC (on_lang_cpp_toggled), piw);
	gtk_signal_connect (GTK_OBJECT (piw->widgets.language_c_cpp_radio), "toggled",
			    GTK_SIGNAL_FUNC (on_lang_c_cpp_toggled), piw);
}

void
create_import_wizard_page5 (ProjectImportWizard * piw)
{
	piw->widgets.page[4] = create_project_description_page(GNOME_DRUID(piw->widgets.druid),
			&piw->widgets.description_text, NULL, NULL, NULL);
}

void create_import_wizard_page6 (ProjectImportWizard * piw)
{
	piw->widgets.page[5] = create_project_menu_page (GNOME_DRUID(piw->widgets.druid),
			&piw->widgets.menu_frame,
			&piw->widgets.menu_entry_entry,
			&piw->widgets.menu_comment_entry,
			&piw->widgets.icon_entry,
			&piw->widgets.app_group_combo,
			&piw->widgets.app_group_entry,
			&piw->widgets.term_check,
			&piw->widgets.file_header_check,
			&piw->widgets.gettext_support_check);
	
	gtk_signal_connect (GTK_OBJECT (piw->widgets.menu_entry_entry), "changed",
			    GTK_SIGNAL_FUNC (on_piw_text_entry_changed), &piw->prj_menu_entry);
	gtk_signal_connect (GTK_OBJECT (piw->widgets.menu_entry_entry), "realize",
			    GTK_SIGNAL_FUNC (on_piw_text_entry_realize), piw->prj_menu_entry);

	gtk_signal_connect (GTK_OBJECT (piw->widgets.menu_comment_entry), "changed",
			    GTK_SIGNAL_FUNC (on_piw_text_entry_changed), &piw->prj_menu_comment);
	gtk_signal_connect (GTK_OBJECT (piw->widgets.menu_comment_entry), "realize",
			    GTK_SIGNAL_FUNC (on_piw_text_entry_realize), piw->prj_menu_comment);

	gtk_signal_connect (GTK_OBJECT (piw->widgets.app_group_combo), "changed",
			    GTK_SIGNAL_FUNC (on_piw_text_entry_changed), &piw->prj_menu_group);
}

void
create_import_wizard_page_finish (ProjectImportWizard * piw)
{
	piw->widgets.page[6] = create_project_finish_page(GNOME_DRUID(piw->widgets.druid));
}
