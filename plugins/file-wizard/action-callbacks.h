/* 
 * action-callbacks.h Copyright (C) 2003 Naba Kumar  <naba@gnome.org>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along with 
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
 * Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _FILE_ACTION_CALLBACKS_H_
#define _FILE_ACTION_CALLBACKS_H_

#include <gtk/gtkaction.h>

void on_insert_header (GtkAction * action, gpointer user_data);
void on_insert_cvs_author (GtkAction * action, gpointer user_data);
void on_insert_cvs_date (GtkAction * action, gpointer user_data);
void on_insert_cvs_header (GtkAction * action, gpointer user_data);
void on_insert_cvs_id (GtkAction * action, gpointer user_data);
void on_insert_cvs_log (GtkAction * action, gpointer user_data);
void on_insert_cvs_name (GtkAction * action, gpointer user_data);
void on_insert_cvs_revision (GtkAction * action, gpointer user_data);
void on_insert_cvs_source (GtkAction * action, gpointer user_data);
void on_insert_switch_template (GtkAction * action, gpointer user_data);
void on_insert_for_template (GtkAction * action, gpointer user_data);
void on_insert_while_template (GtkAction * action, gpointer user_data);
void on_insert_ifelse_template (GtkAction * action, gpointer user_data);
void on_insert_c_gpl_notice (GtkAction * action, gpointer user_data);
void on_insert_cpp_gpl_notice (GtkAction * action, gpointer user_data);
void on_insert_py_gpl_notice (GtkAction * action, gpointer user_data);
void on_insert_date_time (GtkAction * action, gpointer user_data);
void on_insert_changelog_entry (GtkAction * action, gpointer user_data);
void on_insert_header_template (GtkAction * action, gpointer user_data);
void on_insert_username (GtkAction * action, gpointer user_data);

#endif
