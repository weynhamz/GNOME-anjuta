/*
    find_in_files_cbs.h
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
#ifndef _FIND_IN_FILES_CBS_H_
#define _FIND_IN_FILES_CBS_H_

#include <gnome.h>
#include "find_in_files.h"

gboolean
on_search_in_files_close			  (GtkWidget		*widget,
									   gpointer			user_data);

void
on_configure_ok_clicked                (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_search_in_files_delete_event        (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_search_in_files_add_clicked         (GtkButton       *button,
                                        gpointer         user_data);

void
on_search_in_files_remove_clicked      (GtkButton       *button,
                                        gpointer         user_data);

void
on_search_in_files_help_clicked        (GtkButton       *button,
                                        gpointer         user_data);

void
on_search_in_files_ok_clicked          (GtkButton       *button,
                                        gpointer         user_data);

void
on_search_in_files_cancel_clicked      (GtkButton       *button,
                                        gpointer         user_data);


void
on_search_in_files_fileentry_realize   (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_search_in_files_clear_clicked       (GtkButton       *button,
                                        gpointer         user_data);

void
on_search_in_files_clist_select_row                   (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);
#endif
