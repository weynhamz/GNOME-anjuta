/*  project_import_cbs.h
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

#ifndef PROJECT_IMPORT_CBS_H
#define PROJECT_IMPORT_CBS_H

#include "project_import.h"

gboolean
on_page_start_next (GnomeDruidPage * page, gpointer arg1, gpointer data);

gboolean on_page2_next (GnomeDruidPage * page, gpointer arg1, gpointer data);

gboolean on_page3_next (GnomeDruidPage * page, gpointer arg1, gpointer data);

gboolean on_page4_next (GnomeDruidPage * page, gpointer arg1, gpointer data);

gboolean on_page5_next (GnomeDruidPage * page, gpointer arg1, gpointer data);

gboolean on_page6_next (GnomeDruidPage * page, gpointer arg1, gpointer data);

gboolean
on_page_finish_finish (GnomeDruidPage * page, gpointer arg1, gpointer data);

gboolean on_page2_back (GnomeDruidPage * page, gpointer arg1, gpointer data);

gboolean on_page3_back (GnomeDruidPage * page, gpointer arg1, gpointer data);

gboolean on_page4_back (GnomeDruidPage * page, gpointer arg1, gpointer data);

gboolean on_page5_back (GnomeDruidPage * page, gpointer arg1, gpointer data);

gboolean on_page6_back (GnomeDruidPage * page, gpointer arg1, gpointer data);

gboolean
on_page_finish_back (GnomeDruidPage * page, gpointer arg1, gpointer data);

void on_druid_cancel (GnomeDruid * druid, gpointer user_data);

void
on_wizard_import_icon_select (GnomeIconList * gil, gint num,
			      GdkEvent * event, gpointer user_data);

void
on_prj_name_entry_changed (GtkEditable * editable, gpointer user_data);

gboolean
on_prj_name_entry_focus_out_event (GtkWidget * widget,
				   GdkEventFocus * event, gpointer user_data);

void
on_lang_c_toggled (GtkToggleButton * togglebutton, gpointer user_data);

void
on_lang_cpp_toggled (GtkToggleButton * togglebutton, gpointer user_data);

void
on_lang_c_cpp_toggled (GtkToggleButton * togglebutton, gpointer user_data);

void
on_piw_text_entry_realize (GtkWidget * widget, gpointer user_data);

void
on_piw_text_entry_changed (GtkEditable * editable, gpointer user_data);


#endif /* PROJECT_IMPORT_CBS_H */
