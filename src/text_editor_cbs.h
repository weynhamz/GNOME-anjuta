/* 
    text_editor_cbs.h
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
#ifndef _TEXT_EDITOR_CBS_H_
#define _TEXT_EDITOR_CBS_H_

#include <gnome.h>
#include "text_editor.h"

void
on_text_editor_window_realize          (GtkWidget       *widget,
                                        gpointer         user_data);
void
on_text_editor_window_size_allocate               (GtkWidget       *widget,
                                        GtkAllocation   *allocation,
                                        gpointer         user_data);
void
on_text_editor_line_label_realize      (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_text_editor_dock_activate           (GtkButton       *button,
                                        gpointer         user_data);

void
on_text_editor_client_realize          (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_text_editor_text_realize            (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_text_editor_text_cut_clipboard      (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_text_editor_text_copy_clipboard     (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_text_editor_text_paste_clipboard    (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_text_editor_text_changed            (GtkEditable     *editable,
                                        gpointer         user_data);
void
on_text_editor_notebook_close_page		(GtkNotebook    *notebook,
										 GtkNotebookPage *page,
										 gint page_num,
										 gpointer user_data);
gboolean
on_text_editor_auto_save               (gpointer         user_data);

gboolean
on_text_editor_window_focus_in_event   (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);
gboolean
on_text_editor_window_delete           (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);
gboolean
on_text_editor_text_event              (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
on_text_editor_text_buttonpress_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_text_editor_insert_text   (GtkEditable *editable,
  							  const gchar *text,
							  gint length,
							  gint *position,
							  TextEditor *te);
gint
on_text_editor_insert_text_after (GtkEditable *editable,
  								  const gchar *text,
								  gint length,
								  gint *position,
								  TextEditor *te);

void
on_text_editor_delete_text (GtkEditable *editable,
							gint start_pos,
							gint end_pos,
							TextEditor *te );

void
on_text_editor_scintilla_notify(GtkWidget* sci,	gint wParam,
								gpointer lParam, gpointer data);

void
on_text_editor_scintilla_command(GtkWidget* sci, gint wParam,
								 gpointer lParam, gpointer data);

void
on_text_editor_scintilla_size_allocate (GtkWidget *widget,
										GtkAllocation *allocation,
										gpointer data);

#endif
