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
on_search_in_files_close			  (GtkWidget		*widget,
									   gpointer			user_data)
{
  FindInFiles * ff = user_data;
  find_in_files_hide(ff);
  return FALSE;
}

void
on_search_in_files_add_clicked         (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	const gchar *filename[1];
	FindInFiles* ff = user_data;
	filename[0] = gtk_entry_get_text(GTK_ENTRY(ff->widgets.file_entry));
	if(strlen(filename[0]) == 0) return;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ff->widgets.clist));
	gtk_list_store_append (GTK_LIST_STORE(model), &iter);
	gtk_list_store_set (GTK_LIST_STORE(model), &iter, 0, filename, -1);
	gtk_entry_set_text(GTK_ENTRY(ff->widgets.file_entry), "");
	ff->length++;
}

void
on_search_in_files_remove_clicked      (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	FindInFiles* ff = user_data;
	if(ff->cur_row == NULL) return;
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ff->widgets.clist));
	valid = gtk_tree_model_get_iter_from_string (model, &iter, ff->cur_row);
	gtk_list_store_remove (GTK_LIST_STORE(model), &iter);
	ff->length--;
}

void
on_search_in_files_clear_clicked       (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkTreeModel *model;
	FindInFiles* ff = user_data;
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ff->widgets.clist));
	gtk_list_store_clear (GTK_LIST_STORE(model));
	if (ff->cur_row)
		g_free (ff->cur_row);
	ff->cur_row = NULL;
	ff->length = 0;
}

void
on_search_in_files_ok_clicked          (GtkButton       *button,
                                        gpointer         user_data)
{
   FindInFiles* ff = user_data;
   const gchar *temp;
   gchar *temp2;
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
  if (NULL != ff)
	gtk_dialog_close(GTK_DIALOG(ff->widgets.window));
  find_in_files_process(ff);
}

void
on_search_in_files_cancel_clicked      (GtkButton       *button,
                                        gpointer         user_data)
{
  FindInFiles* ff = user_data;
  if (NULL != ff)
	gtk_dialog_close(GTK_DIALOG(ff->widgets.window));
}

void
on_search_in_files_clist_row_activated (GtkTreeView     *treeview,
                                        GtkTreePath     *arg1,
                                        GtkTreeViewColumn *arg2,
                                        gpointer         user_data)

{
  FindInFiles* ff = user_data;
  if (ff->cur_row)
	  g_free (ff->cur_row);
  ff->cur_row = gtk_tree_path_to_string(arg1);
}
