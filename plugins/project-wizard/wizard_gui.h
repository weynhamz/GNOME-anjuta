/*  wizard_gui.h
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

/* Here is the code stored which is shared beetween appwizard and importwizard */

#ifndef WIZARD_GUI_H
#define WIZARD_GUI_H

#include <gnome.h>
#include "watch.h"

GtkWidget *create_project_start_page (GnomeDruid * druid, gchar * greetings,
				      gchar * title);

GtkWidget *create_project_type_selection_page (GnomeDruid * druid,
					       GtkWidget ** iconlist);

GtkWidget *create_project_props_page (GnomeDruid * druid,
				      GtkWidget ** prj_name,
				      GtkWidget ** author,
				      GtkWidget ** version,
				      GtkWidget ** target,
				      GtkWidget ** language_c_radio,
				      GtkWidget ** language_cpp_radio,
				      GtkWidget ** language_c_cpp_radio,
					  GtkWidget ** target_exec_radio,
					  GtkWidget ** target_slib_radio,
					  GtkWidget ** target_dlib_radio);

GtkWidget *create_project_description_page (GnomeDruid * druid,
					    GtkWidget ** description);

GtkWidget *create_project_menu_page (GnomeDruid * druid,
				     GtkWidget ** menu_frame,
				     GtkWidget ** menu_entry_entry,
				     GtkWidget ** menu_comment,
				     GtkWidget ** icon_entry,
				     GtkWidget ** app_group_box,
				     GtkWidget ** app_group_combo,
				     GtkWidget ** term_check,
				     GtkWidget ** file_header_support,
				     GtkWidget ** gettext_support_check,
					 GtkWidget ** use_glade_check);

GtkWidget *create_project_finish_page (GnomeDruid * druid);

GtkWidget *create_eval_dialog (GtkWindow* parent, ExprWatch *ew);


#endif
