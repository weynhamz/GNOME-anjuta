/*
    find_in_files_cbs.c
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

#include <string.h>
#include <gnome.h>
#include "find_in_files.h"
#include "find_in_files_cbs.h"
#include  "anjuta.h"
#include "resources.h"

gboolean
on_search_in_files_delete_event        (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  FindInFiles*  ff = user_data;
  find_in_files_hide(ff);
  return TRUE;
}

void
on_search_in_files_add_clicked         (GtkButton       *button,
                                        gpointer         user_data)
{
  char *filename[1];
  FindInFiles* ff = user_data;
  filename[0] = gtk_entry_get_text(GTK_ENTRY(ff->widgets.file_combo));
  if(strlen(filename[0]) == 0) return;
  gtk_clist_append(GTK_CLIST(ff->widgets.clist), filename);
  gtk_entry_set_text(GTK_ENTRY(ff->widgets.file_combo), "");
  ff->length++;
}

void
on_search_in_files_remove_clicked      (GtkButton       *button,
                                        gpointer         user_data)
{
  FindInFiles* ff = user_data;
  if(ff->length <= 0) return;
  gtk_clist_remove(GTK_CLIST(ff->widgets.clist), ff->cur_row);
  ff->length--;
}

void
on_search_in_files_clear_clicked       (GtkButton       *button,
                                        gpointer         user_data)
{
  FindInFiles* ff = user_data;
  gtk_clist_clear(GTK_CLIST(ff->widgets.clist));
}

void
on_search_in_files_help_clicked        (GtkButton       *button,
                                        gpointer         user_data)
{

}

void
on_search_in_files_ok_clicked          (GtkButton       *button,
                                        gpointer         user_data)
{
   FindInFiles* ff = user_data;
   gchar* temp, *temp2;
   temp = gtk_entry_get_text(GTK_ENTRY(ff->widgets.regexp_entry));
   if(ff->length <= 0)
   {
       anjuta_error(_("No input files supplied."));
       return;
   }
   temp2 = g_strdup(temp);
   temp = g_strchomp(temp2);
   if(temp == NULL || strlen(temp) == 0)
   {
       anjuta_error(_("No search string supplied."));
       g_free(temp2);
       return;
   }
  find_in_files_hide(ff);
  find_in_files_process(ff);
}

void
on_search_in_files_cancel_clicked      (GtkButton       *button,
                                        gpointer         user_data)
{
  FindInFiles* ff = user_data;
  find_in_files_hide(ff);
}

void
on_search_in_files_clist_select_row                   (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  FindInFiles* ff = user_data;
  ff->cur_row = row;
}
