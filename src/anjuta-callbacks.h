/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta-app.h Copyright (C) 2003 Naba Kumar  <naba@gnome.org>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59 
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#ifndef _ANJUTA_CALLBACKS_H_
#define _ANJUTA_CALLBACKS_H_

/* Private callbacks */
#if 0
void on_anjuta_exit_yes_clicked (GtkButton * b, gpointer data);

void on_anjuta_notebook_switch_page (GtkNotebook * notebook,
									 GtkNotebookPage * page,
									 gint page_num, gpointer user_data);

void on_anjuta_dnd_drop (gchar* filename, gpointer data);

// void anjuta_refresh_breakpoints (TextEditor* te);

gboolean on_anjuta_window_focus_in_event (GtkWidget * w, GdkEventFocus * e,
										  gpointer d);

gint on_anjuta_window_key_press_event (GtkWidget   *widget,
									   GdkEventKey *event,
									   gpointer     user_data);

gint on_anjuta_window_key_release_event (GtkWidget   *widget,
										 GdkEventKey *event,
										 gpointer     user_data);
               
gdouble on_anjuta_progress_cb (gpointer data);

void on_anjuta_progress_cancel (gpointer data);
#endif
#endif
