/*
    appwidzard_cbs.h
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
#ifndef _APP_WIDZARD_CBS_H_
#define _APP_WIDZARD_CBS_H_

#include <gnome.h>

void on_druid1_cancel (GnomeDruid * gnomedruid, gpointer user_data);

gboolean
on_druidpagestandard1_next (GnomeDruidPage * gnomedruidpage,
			    gpointer arg1, gpointer user_data);

gboolean
on_druidpagestandard2_next (GnomeDruidPage * gnomedruidpage,
			    gpointer arg1, gpointer user_data);

gboolean
on_druidpagestandard3_next (GnomeDruidPage * gnomedruidpage,
			    gpointer arg1, gpointer user_data);

gboolean
on_druidpagestandard4_next (GnomeDruidPage * gnomedruidpage,
			    gpointer arg1, gpointer user_data);

void
on_druidpagefinish1_finish (GnomeDruidPage * gnomedruidpage,
			    gpointer arg1, gpointer user_data);

gboolean
on_druidpagefinish1_back (GnomeDruidPage * gnomedruidpage,
			  gpointer arg1, gpointer user_data);

void on_app_widzard_help_clicked (GtkButton * button, gpointer data);
void on_app_widzard_prev_clicked (GtkButton * button, gpointer data);
void on_app_widzard_next_clicked (GtkButton * button, gpointer data);
void on_app_widzard_finish_clicked (GtkButton * button, gpointer data);

void on_aw_text_entry_changed (GtkEditable * editable, gpointer user_data);
void on_aw_text_entry_realize (GtkWidget * widget, gpointer user_data);

#endif
